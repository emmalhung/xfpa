/***********************************************************************
*                                                                      *
*     l a y o u t . c                                                  *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     manages the various graphics panels.                             *
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

#undef DEBUG_PRESENT
#undef DEBUG_EMPTY
#undef DEBUG_DISPNODES

/* Define panel fill and edge colours */
#define RootBgnd NullBox, NullBox, NULL, NULL, "DarkSlateBlue",    NULL
#define MapBgnd  NullBox, NullBox, NULL, NULL, "MediumAquamarine", "White"
#define NormBgnd NullBox, NullBox, NULL, NULL, NULL,               "White"

/* Internal functions */
static	LOGICAL		setup_modules(void);
static	LOGICAL		reset_modules(void);

/* Internal variables */

/***********************************************************************
*                                                                      *
*     s e t u p _ p a n e l s                                          *
*     r e s e t _ p a n e l s                                          *
*                                                                      *
***********************************************************************/

LOGICAL	setup_panels(void)

	{
	BOX		window;
	int		nx, ny;

	/* Set up the root panel to match the window */
	DnRoot = init_panel((DISPNODE) NULL, "root", RootBgnd);
	glGetWindowSize(&nx, &ny);
	copy_box(&window, &UnitBox);
	window.right = (float) nx;
	window.top   = (float) ny;
	define_dn_xform(DnRoot, "root", NullBox, &window, NullMapProj, NullXform);

	/* Initialize master element list and individual modules */
	(void) strcpy(MapName, "");
	init_dfields();
	(void) setup_modules();

	/* Display the graphics */
	(void) present_all();

	return TRUE;
	}

/**********************************************************************/

LOGICAL	reset_panels(void)

	{
	BOX		window;
	int		nx, ny;

	/* Suspend zoom feature */
	suspend_zoom();

	/* Change the root panel to match the window */
	glGetWindowSize(&nx, &ny);
	copy_box(&window, &UnitBox);
	window.right = (float) nx;
	window.top   = (float) ny;
	define_dn_xform(DnRoot, "root", NullBox, &window, NullMapProj, NullXform);

	/* Adjust the map panel */
	define_dn_xform(DnMap, "map", &window, NullBox, MapProj, NullXform);

	/* Adjust the link panels */
	define_dn_xform(DnExtrap, "window", NullBox, &window, MapProj, NullXform);
	define_dn_xform(DnLNode,  "window", NullBox, &window, MapProj, NullXform);

	/* resume zoom feature (with resized drawing area) */
	resume_zoom(TRUE);

	/* Re-capture the map background */
	if (SequenceReady && !Spawned) capture_dn_raster(DnBgnd);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     m e t a _ i n p u t                                              *
*                                                                      *
***********************************************************************/

METAFILE	meta_input

	(
	FLD_DESCRIPT	*fd
	)

	{
	STRING			name;
	METAFILE		meta, meta1, meta2;
	FIELD			fld;
	SURFACE			sfc1, sfc2;
	MAP_PROJ		*smp;
	LOGICAL			reproj, ok;
	COMPONENT		comp1, comp2;
	STRING			c2name;
	FLD_DESCRIPT	c2fd;

	/* Get the file name */
	/* If anything is wrong with fd name will be NULL */
	name = check_meta_filename(fd);
	if (blank(name)) return NullMeta;

	/* Read the metafile using the normal reprojection method */
	meta   = read_metafile(name, MapProj);

	/* See if this is a component field in need of being reprojected */
	smp    = find_meta_map_projection(name);
	reproj = check_reprojection_for_components(fd->edef->name, smp, MapProj);
	
	/* If no reprojection needed, just give back the metafile */
	if (!reproj) return meta;

	/****************************************************************
	*  Must reproject component field to target co-ordinate system  *
	****************************************************************/

	/* Read the component we asked for, without reprojecting */
	meta1 = read_metafile(name, NullMapProj);
	sfc1  = take_mf_sfc(meta1, NULL, NULL, NULL);
	meta1 = destroy_metafile(meta1);
	if (IsNull(sfc1))
		{
		meta = destroy_metafile(meta);
		sfc1 = destroy_surface(sfc1);
		return NullMeta;
		}

	/* Figure out the filename of the matching component */
	comp1 = which_components(fd->edef->name, &c2name, &comp2);
	(void) copy_fld_descript(&c2fd, fd);
	(void) set_fld_descript(&c2fd,
							FpaF_ELEMENT_NAME, c2name,
							FpaF_END_OF_LIST);
	name = check_meta_filename(&c2fd);
	if (blank(name))
		{
		meta = destroy_metafile(meta);
		sfc1 = destroy_surface(sfc1);
		return NullMeta;
		}

	/* Read the second component without reprojecting */
	/* Make sure it has the same source projection as the first component */
	meta2 = read_metafile(name, NullMapProj);
	if (IsNull(meta2) || !same_map_projection(&(meta2->mproj), smp))
		{
		meta  = destroy_metafile(meta);
		sfc1  = destroy_surface(sfc1);
		meta2 = destroy_metafile(meta2);
		return NullMeta;
		}
	sfc2  = take_mf_sfc(meta2, NULL, NULL, NULL);
	meta2 = destroy_metafile(meta2);
	if (IsNull(sfc2))
		{
		meta = destroy_metafile(meta);
		sfc1 = destroy_surface(sfc1);
		sfc2 = destroy_surface(sfc2);
		return NullMeta;
		}

	/* Which component did we ask for? */
	ok = FALSE;
	switch (comp1)
		{
		case X_Comp:
			ok = reproject_xy_surfaces(sfc1, sfc2, smp, MapProj);
			break;
						
		case Y_Comp:
			ok = reproject_xy_surfaces(sfc2, sfc1, smp, MapProj);
			break;
		}
	if (!ok)
		{
		meta = destroy_metafile(meta);
		sfc1 = destroy_surface(sfc1);
		sfc2 = destroy_surface(sfc2);
		return NullMeta;
		}

	/* Replace the field in meta with the correctly reprojected one */
	fld = find_mf_field(meta, "surface", NULL, NULL, NULL, NULL);
	define_fld_data(fld, "surface", (POINTER)sfc1);

	/* We don't need the second component any more */
	sfc1 = NullSfc;
	sfc2 = destroy_surface(sfc2);
	return meta;
	}

/***********************************************************************
*                                                                      *
*     m a p _ b a c k g r o u n d                                      *
*     m a p _ o v e r l a y                                            *
*     m a p _ i n p u t                                                *
*                                                                      *
***********************************************************************/

LOGICAL	map_background

	(
	STRING	name,
	int		numov
	)

	{
	float		lat, lon;
	POINT		centre;
	BOX			*view;
	MAP_PROJ	*mp;
	STRING		file;
	GRID_DEF	grid;
	float		gridlen;

	static	MAP_PROJ	Map  = NO_MAPPROJ;
	static	BOX			View = ZERO_BOX;

	/* Allocate the map overlay list the first time */
	if (!DnMap)
		{
		/* Create panel for base map */
		DnMap = init_panel(DnRoot, "map", MapBgnd);

		/* Set number of overlays */
		NumOverlay = numov;
		}

	/* If we already have a base map, remapping may be required */
	else
		{
		if (same(name, MapName)) return TRUE;
		(void) fprintf(stderr, "[map_background]");
		(void) fprintf(stderr, " Attempt to override existing background");
		(void) fprintf(stderr, " - Ignored!\n");
		return FALSE;
		}

	/* Save info about the standard (ingest) map and grid */
	MapProj = get_target_map();
	if (!MapProj)
		{
		pr_warning("Editor",
			"Standard grid not properly defined in target_map block\n");
		pr_warning("Editor", "Using background map \"%s\"\n", SafeStr(name));

		/* Set map projection from background file map projection */
		file = background_file(name);
		if (blank(file))
			{
			pr_error("Editor", "Cannot find background map - Aborting!\n");
			exit(1);
			}
		mp   = find_meta_map_projection(file);
		if (IsNull(mp))
			{
			pr_error("Editor", "No map projection in background map - Aborting!\n");
			exit(1);
			}

		grid.units   = mp->definition.units;
		gridlen      = mp->definition.ylen / 10;
		grid.gridlen = gridlen;
		grid.xgrid   = gridlen;
		grid.ygrid   = gridlen;
		grid.ny      = 11;
		grid.nx      = (mp->definition.xlen + gridlen) / gridlen;
		(void) printf("Grid: %d %d %g %g\n", grid.nx, grid.ny, grid.gridlen,
			   grid.units);

		define_map_projection(&Map, &mp->projection, &mp->definition, &grid);
		MapProj = &Map;
		}

	/* Calculate GMT-Local offset for the chart centre */
	/* Use as: GMT = LMT + LocalOffset */
	centre[X] = MapProj->definition.xlen / 2.0;
	centre[Y] = MapProj->definition.ylen / 2.0;
	pos_to_ll(MapProj, centre, &lat, &lon);
	LocalOffset = -hours_from_gmt(lon);

	/* Set up DnMap to contain the map projection only */
	view = &(DnMap->viewport);
	define_dn_xform(DnMap, "map", view, NullBox, MapProj, NullXform);

	/* Save info about the base map */
	(void) safe_strcpy(MapName, name);
	copy_box(&View, &DnMap->window);
	MapView = &View;

	/* Try to input the map background */
	DnBgnd = init_panel(DnMap, "map", NormBgnd);
	(void) map_input(DnBgnd, name, FALSE);

	/* Construct all modules to fit */
	(void) reset_modules();
	if (SequenceReady && !Spawned) capture_dn_raster(DnBgnd);
	(void) present_all();

	return TRUE;
	}

LOGICAL	map_overlay

	(
	int		mapov,
	STRING	name,
	STRING	reread
	)

	{
	LOGICAL		shown, show, need, under=FALSE;
	OVERLAY		*ov;
	DISPNODE	dn;

	/* Make sure overlay number is legal */
	if (mapov<1 || mapov>NumOverlay)
		{
		put_message("map-ov-invalid", mapov);
		(void) fprintf(stderr, "[map_overlay] Unknown map overlay: %d\n", mapov);
		(void) sleep(1);
		clear_message();
		return FALSE;
		}

	/* Find overlay by number */
	ov = Overlays + mapov-1;
	dn = ov->dn;

	/* See if we need to read it in */
	show = !same(name, "OFF");
	need = FALSE;
	if (show)
		{
		if (blank(ov->name))
			{
			/* Don't already have this one - read only if "ON" */
			need = TRUE;
			}
		else if (!same(ov->name, name))
			{
			/* Already have a different one */
			need = TRUE;
			FREEMEM(ov->name);
			}
		}

	/* Input the metafile if required */
	if (need)
		{
#		ifdef DEBUG
		pr_diag("Editor",
			"Reading Map Overlay file \"%s\"\n", SafeStr(name));
#		endif /* DEBUG */

		if (!map_input(dn, name, TRUE)) return FALSE;
		
		/* Save info about the map overlay */
		ov->name = strdup(name);
		}

	/* Re-read the metafile if required */
	else if (same(reread, "REREAD"))
		{
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-reading Map Overlay file \"%s\"\n", SafeStr(name));
#		endif /* DEBUG */

		if (!map_input(dn, name, TRUE)) return FALSE;

		/* Turn the dispnode off to force a redisplay */
		define_dn_vis(dn, FALSE);
		if (under && SequenceReady && !Spawned) capture_dn_raster(DnBgnd);
		(void) present_all();
		}

	/* Turn the dispnode on or off, as requested, and display the map */
	recall_dn_vis(dn, &shown);
	if (shown && show)   return TRUE;
	if (!shown && !show) return TRUE;
	define_dn_vis(dn, show);
	if (under && SequenceReady && !Spawned) capture_dn_raster(DnBgnd);
	(void) present_all();

	return TRUE;
	}

LOGICAL		map_input

	(
	DISPNODE	dn,
	STRING		name,
	LOGICAL		over
	)

	{
	STRING	file;

	if (!dn)         return FALSE;
	if (blank(name)) return FALSE;

	/* Construct map background filename */
	file = background_file(name);
	if (blank(file))
		{
		if (over)
			{
			put_message("map-ov-access", name);
			pr_error("Editor", "Map Overlay \"%s\" not found\n",
					SafeStr(name));
			}
		else
			{
			put_message("map-bg-access", name);
			pr_error("Editor", "Map Background \"%s\" not found\n",
					SafeStr(name));
			}
		(void) sleep(4);
		return FALSE;
		}

	/* Suspend zoom feature */
	suspend_zoom();

	/* Read the map background into the dispnode */
	if (over) put_message("map-ov-loading", name);
	else      put_message("map-bg-loading");
	if (!input_map(dn, file, MapProj, FALSE))
		{
		if (over)
			{
			put_message("map-ov-access", file);
			pr_error("Editor", "Error reading Map Overlay file \"%s\"\n",
					SafeStr(file));
			}
		else
			{
			put_message("map-bg-access", file);
			pr_error("Editor", "Error reading Map Background file \"%s\"\n",
					SafeStr(file));
			}
		(void) sleep(4);
		resume_zoom(FALSE);
		return FALSE;
		}
	if (over) put_message("map-ov-loaded", name);
	else      put_message("map-bg-loaded");

	/* Setup background presentation and invoke current map palette */
	setup_metafile_presentation(dn->data.meta, "FPA");
	(void) change_geog_colours(dn, GeogLand, GeogWater, GeogCoast, GeogBorder,
						GeogLatlon, GeogFarea, GeogFbord);

	/* Resume zoom feature */
	resume_zoom(FALSE);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     m a p _ p a l e t t e                                            *
*                                                                      *
***********************************************************************/

LOGICAL	map_palette

	(
	STRING	string
	)

	{
	STRING		cname;
	COLOUR		colour, lcolour, wcolour, ccolour, bcolour, llcolour;
	COLOUR		facolour, fbcolour;
	LOGICAL		status;
	DISPNODE	dn;
	int			iov;

	/* Get new land colour */
	lcolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			lcolour = colour;
			GeogLand = lcolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map land colour: %s -> %d\n", cname, lcolour);
#		endif /* DEBUG */
		}

	/* Get new water colour */
	wcolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			wcolour   = colour;
			GeogWater = wcolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map water colour: %s -> %d\n", cname, wcolour);
#		endif /* DEBUG */
		}

	/* Get new coast colour */
	ccolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			ccolour   = colour;
			GeogCoast = ccolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map coast colour: %s -> %d\n", cname, ccolour);
#		endif /* DEBUG */
		}

	/* Get new border colour */
	bcolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			bcolour    = colour;
			GeogBorder = bcolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map border colour: %s -> %d\n", cname, bcolour);
#		endif /* DEBUG */
		}

	/* Get new latlon colour */
	llcolour = SkipColour;
	cname    = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			llcolour   = colour;
			GeogLatlon = llcolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map latlon colour: %s -> %d\n", cname, llcolour);
#		endif /* DEBUG */
		}

	/* Get new forecast area colour */
	facolour = SkipColour;
	cname    = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			facolour  = colour;
			GeogFarea = facolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map fcst area colour: %s -> %d\n", cname, facolour);
#		endif /* DEBUG */
		}

	/* Get new forecast area border colour */
	fbcolour = SkipColour;
	cname    = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status)
			{
			fbcolour  = colour;
			GeogFbord = fbcolour;
			}
#		ifdef DEBUG
		pr_diag("Editor",
			"Re-setting Map fcst border colour: %s -> %d\n", cname, fbcolour);
#		endif /* DEBUG */
		}

	/* Now change the geography if present */
	if (!DnBgnd) return TRUE;
	(void) change_geog_colours(DnBgnd, lcolour, wcolour, ccolour, bcolour,
						llcolour, facolour, fbcolour);
	for (iov=0; iov<NumOverlay; iov++)
		{
		dn = Overlays[iov].dn;
		if (!dn) continue;
		(void) change_geog_colours(dn, lcolour, wcolour, ccolour, bcolour,
							llcolour, facolour, fbcolour);
		}
	if (SequenceReady && !Spawned) capture_dn_raster(DnBgnd);
	(void) present_all();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     l i n k _ p a l e t t e                                          *
*                                                                      *
***********************************************************************/

LOGICAL	link_palette

	(
	STRING	string
	)

	{
	STRING		cname;
	COLOUR		colour, lcolour, tcolour, gcolour;
	LOGICAL		status;

	/* Get new link chain colour */
	lcolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status) lcolour = colour;
		}

	/* Get new link label colour */
	tcolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status) tcolour = colour;
		}

	/* Get new guess link colour */
	gcolour = SkipColour;
	cname   = string_arg(string);
	if (!blank(cname) && !same(cname, "-"))
		{
		colour = find_colour(cname, &status);
		if (status) gcolour = colour;
		}

	/* Now change the link colours */
	(void) change_link_colours(lcolour, tcolour, gcolour);
	(void) present_all();

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p r e s e n t _ a l l                                            *
*     p r e s e n t _ n o d e                                          *
*     p r e s e n t _ m e t a                                          *
*     p r e s e n t _ f i e l d                                        *
*     s y n c _ d i s p l a y                                          *
*                                                                      *
***********************************************************************/

LOGICAL	present_all(void)

	{
	pr_diag("Editor", "Present All\n");

#	ifdef DEBUG_PRESENT
	(void) printf("[present_all] Begin at: %d\n", (long) clock());
#	endif /* DEBUG_PRESENT */

	update_screen(DnRoot);

#	ifdef DEBUG_PRESENT
	(void) printf("[present_all] After update_screen at: %d\n", (long) clock());
#	endif /* DEBUG_PRESENT */

	(void) sync_display();

#	ifdef DEBUG_PRESENT
	(void) printf("[present_all] After sync_display at: %d\n", (long) clock());
#	endif /* DEBUG_PRESENT */

	(void) present_imagery(FALSE);
	(void) present_guidance(FALSE);
	(void) present_scratch(FALSE);
	(void) present_depiction(FALSE);
	(void) present_timelink(FALSE);
	(void) present_sample(FALSE);
	(void) present_temp(FALSE);
	(void) present_extrap(FALSE);
	(void) present_ambiguous_nodes(FALSE);

	tell_active_status();

#	ifdef DEBUG_PRESENT
	(void) printf("[present_all] End at: %d\n", (long) clock());
#	endif /* DEBUG_PRESENT */

	return TRUE;
	}

LOGICAL		present_node

	(
	DISPNODE	dn
	)

	{
	update_map(dn);
	(void) sync_display();
	return TRUE;
	}

LOGICAL		present_meta

	(
	DISPNODE	dn
	)

	{
	METAFILE	meta;
	FIELD		fld;
	int			i;

	/* Make sure dn contains a metafile */
	if (!dn)                  return FALSE;
	if (dn->dntype != DnMeta) return FALSE;
	meta = dn->data.meta;
	if (!meta)                return TRUE;

	/* Display all fields and contour surfaces that require it */
	for (i=0; i<meta->numfld; i++)
	    {
	    fld = meta->fields[i];
		(void) present_field(dn, fld);
	    }

	put_message("fld-contour-done");
	return TRUE;
	}

LOGICAL		present_field

	(
	DISPNODE	dn,
	FIELD		fld
	)

	{
	SURFACE	sfc;
	SET		set;
	LOGICAL	drawn=FALSE;

	/* Make sure fld is there */
	if (!dn)  return FALSE;
	if (!fld) return FALSE;

	/* Contour and display if fld contains a surface */
	if (fld->ftype == FtypeSfc)
		{
		/* Contour surface */
		sfc = fld->data.sfc;
		put_message("fld-contour-cont", fld->level, fld->element);
		contour_surface(sfc);
		band_contour_surface(sfc);
		put_message("fld-contour-disp", fld->level, fld->element);
		drawn = TRUE;
		}
	else if (fld->ftype == FtypeSet)
		{
		set = fld->data.set;
		prepare_set(set);
		drawn = TRUE;
		}

	if (drawn)
		{
		/* Display field */
		gxSetupTransform(dn);
		glClipMode(glCLIP_ON);
		display_field(fld);
		glClipMode(glCLIP_OFF);
		display_dn_edge(dn);
		glFlush();
		(void) sync_display();
		}

	return TRUE;
	}

LOGICAL	sync_display(void)

	{
	glSwapBuffers();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ t e m p                                                *
*     h i d e _ t e m p                                                *
*     p r e s e n t _ t e m p                                          *
*     e m p t y _ t e m p                                              *
*                                                                      *
***********************************************************************/

LOGICAL	show_temp(void)

	{
	if (TempShown) return TRUE;

	/* Set display state on */
	define_dn_vis(DnTemp, TRUE);
	TempShown = TRUE;
	empty_metafile(TempMeta);
	return present_all();
	}

LOGICAL	hide_temp(void)

	{
	if (!TempShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnTemp, FALSE);
	TempShown = FALSE;
	empty_metafile(TempMeta);
	return present_all();
	}

LOGICAL	present_temp

	(
	LOGICAL	all
	)

	{
	if (!TempShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) (void) present_node(DnTemp);
	return TRUE;
	}

LOGICAL	empty_temp(void)

	{
	if (!TempMeta)             return TRUE;
	if (TempMeta->numfld <= 0) return TRUE;

#	ifdef DEBUG_EMPTY
	(void) printf("[empty_temp] Call to empty_temp()\n");
#	endif /* DEBUG_EMPTY */

	empty_metafile(TempMeta);
	if (TempShown) (void) present_all();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ s a m p l e                                            *
*     h i d e _ s a m p l e                                            *
*     p r e s e n t _ s a m p l e                                      *
*     e m p t y _ s a m p l e                                          *
*                                                                      *
***********************************************************************/

LOGICAL	show_sample(void)

	{
	if (SampleShown) return TRUE;

	/* Set display state on */
	define_dn_vis(DnSample, TRUE);
	SampleShown = TRUE;
	empty_metafile(SampleMeta);
	return present_all();
	}

LOGICAL	hide_sample(void)

	{
	if (!SampleShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnSample, FALSE);
	SampleShown = FALSE;
	empty_metafile(SampleMeta);
	return present_all();
	}

LOGICAL	present_sample

	(
	LOGICAL	all
	)

	{
	if (!SampleShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) (void) present_node(DnSample);
	return TRUE;
	}

LOGICAL	empty_sample(void)

	{
	if (!SampleMeta)             return TRUE;
	if (SampleMeta->numfld <= 0) return TRUE;

#	ifdef DEBUG_EMPTY
	(void) printf("[empty_sample] Call to empty_sample()\n");
#	endif /* DEBUG_EMPTY */

	empty_metafile(SampleMeta);
	if (SampleShown) (void) present_all();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ b l a n k                                              *
*     h i d e _ b l a n k                                              *
*     p r e s e n t _ b l a n k                                        *
*     e m p t y _ b l a n k                                            *
*                                                                      *
***********************************************************************/

LOGICAL	show_blank(void)

	{
	if (BlankShown) return TRUE;

	/* Set all other module display states off */
	(void) global_display_state(FALSE);

	/* Set display state on */
	define_dn_vis(DnBlank, TRUE);
	BlankShown = TRUE;
	empty_metafile(BlankMeta);
	return present_all();
	}

LOGICAL	hide_blank(void)

	{
	if (!BlankShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnBlank, FALSE);
	BlankShown = FALSE;

	/* Restore all other module display states */
	(void) global_display_state(TRUE);
	empty_metafile(BlankMeta);
	return present_all();
	}

LOGICAL	present_blank

	(
	LOGICAL	all
	)

	{
	if (!BlankShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) (void) present_node(DnBlank);
	return TRUE;
	}

LOGICAL	empty_blank(void)

	{
	if (!BlankMeta)             return TRUE;
	if (BlankMeta->numfld <= 0) return TRUE;

#	ifdef DEBUG_EMPTY
	(void) printf("[empty_blank] Call to empty_blank()\n");
#	endif /* DEBUG_EMPTY */

	empty_metafile(BlankMeta);
	if (BlankShown) (void) present_all();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g l o b a l _ d i s p l a y _ s t a t e                          *
*                                                                      *
***********************************************************************/

LOGICAL	global_display_state

	(
	LOGICAL	mode
	)

	{
	if (DepictShown)  define_dn_vis(DnDepict, mode);
	if (EditShown)    define_dn_vis(DnOrig, mode);
	if (EditShown)    define_dn_vis(DnEdit, mode);
	if (LinkShown)    define_dn_vis(DnLink, mode);
	if (ScratchShown) define_dn_vis(DnScratch, mode);
	if (GuidShown)    define_dn_vis(DnGuid, mode);
	if (ExtrapShown)  define_dn_vis(DnExtrap, mode);
	if (LNodeShown)   define_dn_vis(DnLNode, mode);
	if (SampleShown)  define_dn_vis(DnSample, mode);
	if (TempShown)    define_dn_vis(DnTemp, mode);
	if (BlankShown)   define_dn_vis(DnBlank, mode);

	return mode;
	}

/***********************************************************************
*                                                                      *
*     d e f a u l t _ g e o g _ c o l o u r s                          *
*     p r i n t e r _ g e o g _ c o l o u r s                          *
*     c h a n g e _ g e o g _ c o l o u r s                            *
*     c h a n g e _ g e o g _ c a t s p e c                            *
*                                                                      *
***********************************************************************/

LOGICAL		default_geog_colours

	(
	COLOUR	*lcolour,
	COLOUR	*wcolour,
	COLOUR	*ccolour,
	COLOUR	*bcolour,
	COLOUR	*llcolour,
	COLOUR	*facolour,
	COLOUR	*fbcolour
	)

	{
	static	LOGICAL	first = TRUE;
	static	COLOUR	DefaultLand   = 0;
	static	COLOUR	DefaultWater  = 0;
	static	COLOUR	DefaultCoast  = 1;
	static	COLOUR	DefaultBorder = 1;
	static	COLOUR	DefaultLatlon = 1;
	static	COLOUR	DefaultFarea  = 0;
	static	COLOUR	DefaultFbord  = 1;

	/* Get the default geography colours */
	if (first)
		{
		int		nspec, ispec;
		CATSPEC	*cspecs, *cspec;
		STRING	cat;

		first = FALSE;
		nspec = get_catspecs("land_water", "geography", "FPA", "discrete", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "land"))  DefaultLand  = cspec->fspec.colour;
			if (same(cat, "water")) DefaultWater = cspec->fspec.colour;
			}

		nspec = get_catspecs("land_water", "geography", "FPA", "curve", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "coast")) DefaultCoast = cspec->lspec.colour;
			}

		nspec = get_catspecs("boundaries", "geography", "FPA", "curve", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "border"))     DefaultBorder = cspec->lspec.colour;
			if (same(cat, "lat_lon_10")) DefaultLatlon = cspec->lspec.colour;
			}

		nspec = get_catspecs("forecast_areas", "NULL", "FPA", "discrete", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "marine_area")) DefaultFarea = cspec->fspec.colour;
			if (same(cat, "marine_area")) DefaultFbord = cspec->lspec.colour;
			}
		}

	/* Substitute the default for unspecified input colours */
	if (*lcolour  < 0) *lcolour  = DefaultLand;
	if (*wcolour  < 0) *wcolour  = DefaultWater;
	if (*ccolour  < 0) *ccolour  = DefaultCoast;
	if (*bcolour  < 0) *bcolour  = DefaultBorder;
	if (*llcolour < 0) *llcolour = DefaultLatlon;
	if (*facolour < 0) *facolour = DefaultFarea;
	if (*fbcolour < 0) *fbcolour = DefaultFbord;
	return TRUE;
	}

LOGICAL		printer_geog_colours

	(
	COLOUR	*lcolour,
	COLOUR	*wcolour,
	COLOUR	*ccolour,
	COLOUR	*bcolour,
	COLOUR	*llcolour,
	COLOUR	*facolour,
	COLOUR	*fbcolour
	)

	{
	static	LOGICAL	first = TRUE;
	static	COLOUR	PrinterLand   = 255;
	static	COLOUR	PrinterWater  = 255;
	static	COLOUR	PrinterCoast  = 0;
	static	COLOUR	PrinterBorder = 0;
	static	COLOUR	PrinterLatlon = 0;
	static	COLOUR	PrinterFarea  = 255;
	static	COLOUR	PrinterFbord  = 0;

	/* Get the hardcopy geography colours */
	if (first)
		{
		int		nspec, ispec;
		CATSPEC	*cspecs, *cspec;
		STRING	cat;

		first = FALSE;
		nspec = get_catspecs("land_water", "geography", "hardcopy", "discrete",
							 NULL, &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "land"))  PrinterLand  = cspec->fspec.colour;
			if (same(cat, "water")) PrinterWater = cspec->fspec.colour;
			}

		nspec = get_catspecs("land_water", "geography", "hardcopy", "curve",
							 NULL, &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "coast")) PrinterCoast = cspec->lspec.colour;
			}

		nspec = get_catspecs("boundaries", "geography", "hardcopy", "curve",
							 NULL, &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "border"))     PrinterBorder = cspec->lspec.colour;
			if (same(cat, "lat_lon_10")) PrinterLatlon = cspec->lspec.colour;
			}

		nspec = get_catspecs("forecast_areas", "NULL", "hardcopy", "discrete",
							 NULL, &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "marine_area")) PrinterFarea = cspec->fspec.colour;
			if (same(cat, "marine_area")) PrinterFbord = cspec->lspec.colour;
			}
		}

	/* Substitute the printer colour for unspecified input colours */
	*lcolour  = PrinterLand;
	*wcolour  = PrinterWater;
	*ccolour  = PrinterCoast;
	*bcolour  = PrinterBorder;
	*llcolour = PrinterLatlon;
	*facolour = PrinterFarea;
	*fbcolour = PrinterFbord;
	return TRUE;
	}

LOGICAL		change_geog_colours

	(
	DISPNODE	dn,
	COLOUR		lcolour,
	COLOUR		wcolour,
	COLOUR		ccolour,
	COLOUR		bcolour,
	COLOUR		llcolour,
	COLOUR		facolour,
	COLOUR		fbcolour
	)

	{
	METAFILE	meta;
	int			ifld, ic;
	FIELD		fld;
	SET			set;
	CATSPEC		*cspec;
	COLOUR		coast;

	/* Set default geography colours */
	(void) default_geog_colours(&lcolour, &wcolour, &ccolour, &bcolour,
						&llcolour, &facolour, &fbcolour);

	/* Set the background fill colour if this is the base map */
	if (!dn) return TRUE;
	if ((dn == DnBgnd) && (wcolour >= 0)) dn->wfill = wcolour;

	/* Access the contents */
	if (dn->dntype != DnMeta) return TRUE;
	if (!dn->data.meta)       return TRUE;
	meta = dn->data.meta;

	/* Adjust each field */
	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = meta->fields[ifld];

		/* Change presentation of set fields */
		if (fld->ftype == FtypeSet)
			{
			set = fld->data.set;
			if (!set) continue;

			/* Use land edge as coast in old map files */
			coast = ccolour;
			if (!same(fld->element, "geography") && same(fld->entity, "b"))
				coast = lcolour;

			for (ic=0; ic<set->ncspec; ic++)
				{
				cspec = set->cspecs + ic;
				(void) change_geog_catspec(cspec, set->type, lcolour, wcolour,
								coast, bcolour, llcolour, facolour, fbcolour);
				}
			invoke_set_catspecs(set);
			}
		}

	return TRUE;
	}

LOGICAL	change_geog_catspec

	(
	CATSPEC	*cspec,
	STRING	type,
	COLOUR	lcolour,
	COLOUR	wcolour,
	COLOUR	ccolour,
	COLOUR	bcolour,
	COLOUR	llcolour,
	COLOUR	facolour,
	COLOUR	fbcolour
	)

	{
	STRING	cat;
	COLOUR	fc;

	if (!cspec) return TRUE;
	cat = cspec->val;

	/* Things that use land colour */
	if (lcolour >= 0)
		{
		if (same(type, "area"))
			{
			if (same(cat, "land"))        cspec->lspec.colour = lcolour;
			if (same(cat, "land"))        cspec->fspec.colour = lcolour;
			if (same(cat, "marine_hole")) cspec->fspec.colour = lcolour;
			}
		if (same(type, "curve"))
			{
			if (same(cat, "lat_lon_10")) cspec->lspec.colour = lcolour;
			if (same(cat, "lat_lon_05")) cspec->lspec.colour = lcolour;
			if (same(cat, "lat_lon_01")) cspec->lspec.colour = lcolour;
			}
		if (same(type, "label"))
			{
			if (same(cat, "latlon")) cspec->tspec.colour = lcolour;
			}
		}

	/* Things that use water colour */
	if (wcolour >= 0)
		{
		if (same(type, "area"))
			{
			if (same(cat, "ocean"))       cspec->lspec.colour = wcolour;
			if (same(cat, "water"))       cspec->lspec.colour = wcolour;
			if (same(cat, "water_minor")) cspec->lspec.colour = wcolour;
			if (same(cat, "ice_major"))   cspec->lspec.colour = wcolour;
			if (same(cat, "ice_minor"))   cspec->lspec.colour = wcolour;
			if (same(cat, "ocean"))       cspec->fspec.colour = wcolour;
			if (same(cat, "water"))       cspec->fspec.colour = wcolour;
			if (same(cat, "water_minor")) cspec->fspec.colour = wcolour;
			if (same(cat, "ice_major"))   cspec->fspec.colour = wcolour;
			if (same(cat, "ice_minor"))   cspec->fspec.colour = wcolour;
			}
		}

	/* Things that use coast colour */
	if (ccolour >= 0)
		{
		if (same(type, "area"))
			{
			if (same(cat, "land"))        cspec->lspec.colour = ccolour;
			if (same(cat, "water"))       cspec->lspec.colour = ccolour;
			}
		if (same(type, "curve"))
			{
			if (same(cat, "coast_major")) cspec->lspec.colour = ccolour;
			if (same(cat, "coast_minor")) cspec->lspec.colour = ccolour;
			if (same(cat, "coast"))       cspec->lspec.colour = ccolour;
			if (same(cat, "river"))       cspec->lspec.colour = ccolour;
			}
		if (same(type, "mark"))
			{
			if (same(cat, "land"))        cspec->mspec.colour = ccolour;
			if (same(cat, "water"))       cspec->mspec.colour = ccolour;
			}
		}

	/* Things that use border colour */
	if (bcolour >= 0)
		{
		if (same(type, "curve"))
			{
			if (same(cat, "border"))       cspec->lspec.colour = bcolour;
			if (same(cat, "border_major")) cspec->lspec.colour = bcolour;
			if (same(cat, "border_minor")) cspec->lspec.colour = bcolour;
			}
		}

	/* Things that use latlon colour */
	if (llcolour >= 0)
		{
		if (same(type, "curve"))
			{
			if (same(cat, "lat_lon_10")) cspec->lspec.colour = llcolour;
			if (same(cat, "lat_lon_05")) cspec->lspec.colour = llcolour;
			if (same(cat, "lat_lon_01")) cspec->lspec.colour = llcolour;
			}
		}

	/* Things that use forecast area fill colour */
	if (facolour >= 0)
		{
		if (same(type, "area"))
			{
			if (same(cat, "marine_area")) cspec->fspec.colour = facolour;
			if (same(cat, "public_area")) cspec->fspec.colour = facolour;
			if (same(cat, "wso_area"))    cspec->fspec.colour = facolour;
			}
		}

	/* Things that use forecast area border colour */
	if (fbcolour >= 0)
		{
		fc = (cspec->fspec.style == 1)? fbcolour: facolour;
		if (same(type, "area"))
			{
			if (same(cat, "marine_area")) cspec->lspec.colour = fc;
			if (same(cat, "marine_hole")) cspec->lspec.colour = fc;
			if (same(cat, "public_area")) cspec->lspec.colour = fc;
			if (same(cat, "public_hole")) cspec->lspec.colour = fc;
			if (same(cat, "wso_area"))    cspec->lspec.colour = fc;
			if (same(cat, "wso_hole"))    cspec->lspec.colour = fc;
			}
		if (same(type, "label"))
			{
			if (same(cat, "marine_area")) cspec->tspec.colour = fc;
			if (same(cat, "public_area")) cspec->tspec.colour = fc;
			if (same(cat, "wso_area"))    cspec->tspec.colour = fc;
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e f a u l t _ l i n k _ c o l o u r s                          *
*     c h a n g e _ l i n k _ c o l o u r s                            *
*                                                                      *
***********************************************************************/

LOGICAL		default_link_colours

	(
	COLOUR	*lcolour,
	COLOUR	*tcolour,
	COLOUR	*gcolour
	)

	{
	COLOUR	colour;
	LOGICAL	valid;

	static	LOGICAL	first = TRUE;
	static	COLOUR	DefaultLinkColour      = 0;
	static	COLOUR	DefaultLinkTextColour  = 1;
	static	COLOUR	DefaultLinkGuessColour = 2;

	/* Set the default link colours */
	if (first)
		{
		colour = find_colour("Cyan", &valid);
		if (valid) DefaultLinkColour = colour;
		colour = find_colour("Cadet Blue", &valid);
		if (valid) DefaultLinkTextColour = colour;
		colour = find_colour("Magenta", &valid);
		if (valid) DefaultLinkGuessColour = colour;

		first = FALSE;
		}

	/* Substitute the default for unspecified input colours */
	if (*lcolour  < 0) *lcolour  = DefaultLinkColour;
	if (*tcolour  < 0) *tcolour  = DefaultLinkTextColour;
	if (*gcolour  < 0) *gcolour  = DefaultLinkGuessColour;
	return TRUE;
	}

LOGICAL		change_link_colours

	(
	COLOUR		lcolour,
	COLOUR		tcolour,
	COLOUR		gcolour
	)

	{
#	ifdef DEBUG
	pr_diag("Editor", "Changing link colours: %d %d %d\n",
						lcolour, tcolour, gcolour);
#	endif /* DEBUG */

	/* Set default link colours */
	(void) default_link_colours(&lcolour, &tcolour, &gcolour);

#	ifdef DEBUG
	pr_diag("Editor", "Changing link colours (after defaults): %d %d %d\n",
						lcolour, tcolour, gcolour);
#	endif /* DEBUG */

	/* Reset link colours */
	LinkColour      = lcolour;
	LinkTextColour  = tcolour;
	LinkGuessColour = gcolour;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e f a u l t _ t e m p _ c o l o u r s                          *
*     p r i n t e r _ t e m p _ c o l o u r s                          *
*                                                                      *
***********************************************************************/

LOGICAL		default_temp_colours

	(
	COLOUR	*boxfill,
	COLOUR	*boxedge,
	COLOUR	*lineclr,
	COLOUR	*textclr
	)

	{
	static	LOGICAL	first = TRUE;
	static	COLOUR	DefaultBfill = 0;
	static	COLOUR	DefaultBedge = 1;
	static	COLOUR	DefaultLclr  = 1;
	static	COLOUR	DefaultTclr  = 1;

	/* Get the default geography colours */
	if (first)
		{
		int		nspec, ispec;
		CATSPEC	*cspecs, *cspec;
		STRING	cat;

		first = FALSE;
		nspec = get_catspecs("legend", "NULL", "FPA", "discrete", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "legend_box")) DefaultBfill = cspec->fspec.colour;
			if (same(cat, "legend_box")) DefaultBedge = cspec->lspec.colour;
			}

		nspec = get_catspecs("legend", "NULL", "FPA", "curve", NULL, &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "legend_divider")) DefaultLclr = cspec->lspec.colour;
			}

		nspec = get_catspecs("legend", "NULL", "FPA", "label", NULL, &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "legend_title")) DefaultTclr = cspec->tspec.colour;
			}
		}

	/* Substitute the default for unspecified input colours */
	*boxfill = DefaultBfill;
	*boxedge = DefaultBedge;
	*lineclr = DefaultLclr;
	*textclr = DefaultTclr;
	return TRUE;
	}

LOGICAL		printer_temp_colours

	(
	COLOUR	*boxfill,
	COLOUR	*boxedge,
	COLOUR	*lineclr,
	COLOUR	*textclr
	)

	{
	static	LOGICAL	first = TRUE;
	static	COLOUR	PrinterBfill = 255;
	static	COLOUR	PrinterBedge = 0;
	static	COLOUR	PrinterLclr  = 0;
	static	COLOUR	PrinterTclr  = 0;

	/* Get the hardcopy geography colours */
	if (first)
		{
		int		nspec, ispec;
		CATSPEC	*cspecs, *cspec;
		STRING	cat;

		first = FALSE;
		nspec = get_catspecs("legend", "NULL", "hardcopy", "discrete", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "legend_box")) PrinterBfill = cspec->fspec.colour;
			if (same(cat, "legend_box")) PrinterBedge = cspec->lspec.colour;
			}

		nspec = get_catspecs("legend", "NULL", "hardcopy", "curve", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "legend_divider")) PrinterLclr = cspec->lspec.colour;
			}

		nspec = get_catspecs("legend", "NULL", "hardcopy", "label", NULL,
							 &cspecs);
		for (ispec=0; ispec<nspec; ispec++)
			{
			cspec = cspecs + ispec;
			cat   = cspec->val;
			if (same(cat, "legend_title")) PrinterTclr = cspec->tspec.colour;
			}
		}

	/* Substitute the printer colour for unspecified input colours */
	*boxfill = PrinterBfill;
	*boxedge = PrinterBedge;
	*lineclr = PrinterLclr;
	*textclr = PrinterTclr;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     LOCAL STATIC FUNCTIONS                                           *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*     s e t u p _ m o d u l e s                                        *
*     r e s e t _ m o d u l e s                                        *
*                                                                      *
*     Builds the display node tree.  The current layout is as          *
*     follows:                                                         *
*                                                                      *
*     DnRoot                                                           *
*       |                                                              *
*       |---> DnMap                                                    *
*       |       |                                                      *
*       |       |---> DnBgnd  . . . . . . . . base map                 *
*       |       |       |                                              *
*       |       |       ----> un->dn  . . . . map underlays            *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> DnImage . . . . . . . . image panel              *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> ov->dn  . . . . . . . . map overlays             *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> DnGuid  . . . . . . . . master guidance panel    *
*       |       |       |                                              *
*       |       |       ----> guid->dn  . . . guidance fields          *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> DnScratch . . . . . . . scratchpad               *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> DnDepict  . . . . . . . master depiction panel   *
*       |       |       |                                              *
*       |       |       |---> dfld->dn  . . . depiction fields         *
*       |       |       |                                              *
*       |       |       |---> DnOrig  . . . . original edit field      *
*       |       |       |                                              *
*       |       |       |---> DnEdit  . . . . pending edit field       *
*       |       |       |                                              *
*       |       |       ----> DnLink  . . . . link chains              *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> DnSample  . . . . . . . sample display           *
*       |       |                                                      *
*       |       |                                                      *
*       |       |---> DnTemp  . . . . . . . . temporary display        *
*       |       |                                                      *
*       |       |                                                      *
*       |       ----> DnBlank . . . . . . . . temporary display        *
*       |                                                              *
*       |                                                              *
*       |                                                              *
*       ----> DnExtrap  . . . . . . . . . . . extrapolation menu       *
*       |                                                              *
*       |                                                              *
*       ----> DnLNode . . . . . . . . . . . . link node menu           *
*                                                                      *
***********************************************************************/

static	LOGICAL	setup_modules(void)

	{
	/* Initialize map background and overlays */
	DnMap      = NullDn;
	DnBgnd     = NullDn;
	NumOverlay = 0;
	Overlays   = NULL;

	/* Initialize imagery */
	DnImage    = NullDn;

	/* Initialize depiction */
	DnDepict   = NullDn;

	/* Initialize guidance frame sequences */
	DnGuid   = NullDn;
	GuidFlds = GETMEM(GuidFlds, GLIST, MaxGuid);
	(void) init_guidance();

	/* Initialize scratchpad global frame */
	DnScratch            = NullDn;
	ScratchMeta          = NullMeta;
	ScratchPad.wfile     = NULL;
	ScratchPad.bfile     = NULL;
	ScratchPad.owfile    = NULL;
	ScratchPad.obfile    = NULL;
	ScratchPad.meta      = NULL;
	ScratchPad.modified  = FALSE;
	ScratchPad.contoured = FALSE;

	/* Initialize assorted other panels */
	DnExtrap   = NullDn;
	DnLNode    = NullDn;
	DnOrig     = NullDn;
	DnEdit     = NullDn;
	DnLink     = NullDn;
	DnSample   = NullDn;
	DnTemp     = NullDn;
	OrigMeta   = NullMeta;
	EditMeta   = NullMeta;
	LinkMeta   = NullMeta;
	SampleMeta = NullMeta;
	TempMeta   = NullMeta;
	return TRUE;
	}

static	LOGICAL	reset_modules(void)

	{
	int			iguid, idfld;
	int			iov;
	OVERLAY		*ov;
	STRING		sfile;

	/* Assume map subtree is already empty */

	/* Open the imagery panel */
	DnImage = init_panel(DnMap, "map", NormBgnd);
	define_dn_vis(DnImage, ImageShown);

	/* Open panels for map overlays */
	Overlays = INITMEM(OVERLAY, NumOverlay);
	for (iov=0; iov<NumOverlay; iov++)
		{
		ov       = Overlays + iov;
		ov->dn   = init_panel(DnMap, "map", NormBgnd);
		ov->name = NULL;
		define_dn_vis(ov->dn, FALSE);
		}

	/* Open the guidance master panel plus a panel for each guidance field */
	DnGuid = init_panel(DnMap, "map", NormBgnd);
	define_dn_vis(DnGuid, GuidShown);
	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		GuidFlds[iguid].dn = init_panel(DnGuid, "map", NormBgnd);
		define_dn_vis(GuidFlds[iguid].dn, FALSE);
		}

	/* Open a panel for the scratchpad */
	DnScratch        = init_panel(DnMap, "map", NormBgnd);
	ScratchMeta      = NullMeta;
	sfile            = depiction_scratch_file();
	ScratchPad.wfile = STRMEM(ScratchPad.wfile, sfile);
	define_dn_vis(DnScratch, ScratchShown);

	/* Open a panel for link start/end menu */
	DnExtrap   = init_panel(DnRoot, "window", NormBgnd);
	ExtrapMeta = create_metafile();
	define_mf_projection(ExtrapMeta, &NoMapProj);
	define_dn_data(DnExtrap, "metafile", (POINTER) ExtrapMeta);
	define_dn_vis(DnExtrap, ExtrapShown);

	/* Open a panel for link control node menu */
	DnLNode   = init_panel(DnRoot, "window", NormBgnd);
	LNodeMeta = create_metafile();
	define_mf_projection(LNodeMeta, &NoMapProj);
	define_dn_data(DnLNode, "metafile", (POINTER) LNodeMeta);
	define_dn_vis(DnLNode, LNodeShown);

	/* Open the depiction master panel plus a panel for each depiction field */
	DnDepict = init_panel(DnMap, "map", NormBgnd);
	define_dn_vis(DnDepict, DepictShown);
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		DfldList[idfld].dn = init_panel(DnDepict, "map", NormBgnd);
		define_dn_vis(DfldList[idfld].dn, FALSE);
		}

	/* Open a panel for original field during edit */
	DnOrig   = init_panel(DnDepict, "map", NormBgnd);
	OrigMeta = create_metafile();
	define_mf_projection(OrigMeta, MapProj);
	define_dn_data(DnOrig, "metafile", (POINTER) OrigMeta);
	define_dn_vis(DnOrig, EditShown);

	/* Open a panel for current edit */
	DnEdit   = init_panel(DnDepict, "map", NormBgnd);
	EditMeta = create_metafile();
	define_mf_projection(EditMeta, MapProj);
	define_dn_data(DnEdit, "metafile", (POINTER) EditMeta);
	define_dn_vis(DnEdit, EditShown);

	/* Open a panel for time links */
	DnLink   = init_panel(DnDepict, "map", NormBgnd);
	LinkMeta = create_metafile();
	define_mf_projection(LinkMeta, MapProj);
	define_dn_data(DnLink, "metafile", (POINTER) LinkMeta);
	define_dn_vis(DnLink, LinkShown);

	/* Open a panel for samples */
	DnSample   = init_panel(DnMap, "map", NormBgnd);
	SampleMeta = create_metafile();
	define_mf_projection(SampleMeta, MapProj);
	define_dn_data(DnSample, "metafile", (POINTER) SampleMeta);
	define_dn_vis(DnSample, SampleShown);

	/* Open a panel for temporary graphics */
	DnTemp   = init_panel(DnMap, "map", NormBgnd);
	TempMeta = create_metafile();
	define_mf_projection(TempMeta, MapProj);
	define_dn_data(DnTemp, "metafile", (POINTER) TempMeta);
	define_dn_vis(DnTemp, TempShown);

	/* Open a panel for temporary graphics with no other modules */
	DnBlank   = init_panel(DnMap, "map", NormBgnd);
	BlankMeta = create_metafile();
	define_mf_projection(BlankMeta, MapProj);
	define_dn_data(DnBlank, "metafile", (POINTER) BlankMeta);
	define_dn_vis(DnBlank, BlankShown);

	/* Setup graphics editor parameters */
	define_edit_resolution();

#	ifdef DEBUG_DISPNODES
	(void) printf("[reset_modules] DnRoot    Dispnode: %x\n", DnRoot);
	(void) printf("[reset_modules] DnMap     Dispnode: %x\n", DnMap);
	(void) printf("[reset_modules] DnBgnd    Dispnode: %x\n", DnBgnd);
	(void) printf("[reset_modules] DnImage   Dispnode: %x\n", DnImage);
	for (iov=0; iov<NumOverlay; iov++)
		{
		ov = Overlays + iov;
		(void) printf("[reset_modules] Map %3d     Dispnode: %x\n", iov, ov->dn);
		}
	(void) printf("[reset_modules] DnGuid    Dispnode: %x\n", DnGuid);
	for (iguid=0; iguid<MaxGuid; iguid++)
		{
		(void) printf("[reset_modules] GuidFld %3d Dispnode: %x\n",
				iguid, GuidFlds[iguid].dn);
		}
	(void) printf("[reset_modules] DnScratch Dispnode: %x\n", DnScratch);
	(void) printf("[reset_modules] DnExtrap  Dispnode: %x\n", DnExtrap);
	(void) printf("[reset_modules] DnLNode   Dispnode: %x\n", DnLNode);
	(void) printf("[reset_modules] DnDepict  Dispnode: %x\n", DnDepict);
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		(void) printf("[reset_modules] Dfld %3d    Dispnode: %x\n",
				idfld, DfldList[idfld].dn);
		}
	(void) printf("[reset_modules] DnOrig    Dispnode: %x\n", DnOrig);
	(void) printf("[reset_modules] DnEdit    Dispnode: %x\n", DnEdit);
	(void) printf("[reset_modules] DnLink    Dispnode: %x\n", DnLink);
	(void) printf("[reset_modules] DnSample  Dispnode: %x\n", DnSample);
	(void) printf("[reset_modules] DnTemp    Dispnode: %x\n", DnTemp);
	(void) printf("[reset_modules] DnBlank   Dispnode: %x\n", DnBlank);
#	endif /* DEBUG_DISPNODES */

	return TRUE;
	}
