/***********************************************************************
*                                                                      *
*     d e p i c t i o n . c                                            *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all edit and display functions for the active weather    *
*     depiction.                                                       *
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
#undef DEBUG_MOD

/***********************************************************************
*                                                                      *
*     i n s e r t _ d e p i c t i o n                                  *
*     b u i l d _ d e p i c t i o n                                    *
*     f i n d _ d e p i c t i o n                                      *
*                                                                      *
***********************************************************************/

LOGICAL	insert_depiction

	(
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime,
	int		itime
	)

	{
	int			idfld;
	DFLIST		*dfld;
	LOGICAL		redisp, valid, active;

	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;
	active = (LOGICAL) (itime==ViewTime || itime==EditTime);

	/* Erase current depiction and guidance fields */
	if (active)
		{
		define_dn_xform(DnDepict, "map", MapView, NullBox, MapProj, NullXform);
		}

	/* Build depiction from individual fields */
	put_message("depict-build");
	pr_diag("Fields", "Building Depiction From Fields: %s\n", vtime);

	/* Construct metafile from individual fields */
	valid = build_depiction(source, subsrc, rtime, vtime, itime);
	if (!valid)
		{
		put_message("depict-access");
		(void) sleep(2);
		pr_error("Fields", "Depiction Not Accessible '%s'\n", vtime);
		return FALSE;
		}
	put_message("depict-built");

	/* Place in Depiction panel if active */
	if (active) (void) pick_depiction();
	(void) make_depict_status(itime, source, NULL);

	/* Turn selected fields off if previously specified */
	redisp = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		if (dfield_shown(dfld)) continue;
		/* if (dfld->showdep == SHOW_ALWAYS) continue; */
		redisp = TRUE;
		}

	if (active && DepictShown)
		{
		if (redisp) (void) present_all();
		else        (void) present_depiction(TRUE);
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	build_depiction

	(
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime,
	int		itime
	)

	{
	STRING			elem, level;
	int	   			idfld;
	DFLIST			*dfld;
	LOGICAL			there, tsame;
	FLD_DESCRIPT	*fd;

	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Try to import all fields that are currently defined */
	/* >>>
	n = source_field_list(fd, FpaC_TIMEDEP_ANY, &flist);
	...
	(void) source_field_list_free(&flist, n);
	>>> */

	pr_diag("Fields", "Extracting Depiction Fields: %s\n", vtime);
	there = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld  = DfldList + idfld;
		elem  = dfld->element;
		level = dfld->level;

		/* See if field is available */
		fd = find_depiction_field(source, subsrc, rtime, vtime, elem, level);
		if (!fd)
			{
			pr_diag("Fields", "  %s:%s - Not Found\n", elem, level);
			continue;
			}

		/* Extract the field */
		tsame = matching_tstamps(vtime, TimeList[itime].jtime);
		if (!import_depiction_field(fd, dfld, itime, tsame, FALSE))
			{
			pr_diag("Fields", "  %s:%s - Not Accessible\n", elem, level);
			continue;
			}

		pr_diag("Fields", "  %s:%s - Extracted\n", elem, level);
		there = TRUE;
		}

	/* Done - did anything get saved? */
	return there;
	}

/**********************************************************************/

LOGICAL	find_depiction

	(
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime
	)

	{
	STRING			elem, level;
	int				ifld, nfld, there;
	FLD_DESCRIPT	*fd;
	FpaConfigFieldStruct	**fdefs;

	/* Determine what fields make up the standard depiction set */
	/* Really only need to look for the mandatory fields */
	nfld = depict_field_list(&fdefs);

	/* See if each field is present in the guidance database */
	pr_diag("Fields", "Checking (%d) Depiction Fields: %s\n", nfld, vtime);
	there = FALSE;
	for (ifld=0; ifld<nfld; ifld++)
	    {
		elem  = fdefs[ifld]->element->name;
		level = fdefs[ifld]->level->name;

		/* See if the field is present */
		fd = find_depiction_field(source, subsrc, rtime, vtime, elem, level);
		if (!fd)
			{
			pr_diag("Fields", "  %s:%s - Unavailable\n", elem, level);
			continue;
			}

		there = TRUE;
		pr_diag("Fields", "  %s:%s - Available\n", elem, level);
	    }

	/* Done */
	return there;
	}

/***********************************************************************
*                                                                      *
*     p i c k _ d e p i c t i o n                                      *
*                                                                      *
***********************************************************************/

LOGICAL	pick_depiction(void)

	{
	int		idfld;
	DFLIST	*dfld;

	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		(void) pick_depiction_field(dfld);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     u p d a t e _ d e p i c t i o n                                  *
*     b a c k u p _ d e p i c t i o n                                  *
*     d e l e t e _ d e p i c t i o n                                  *
*                                                                      *
***********************************************************************/

LOGICAL	update_depiction

	(
	int		itime
	)

	{
	int		idfld;
	DFLIST	*dfld;

	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Save each depiction field in the appropriate metafile */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		/* >>>>>
		if (tdep_special(dfld->tdep)) continue;
		<<<<< */
		(void) update_depiction_field(dfld, itime);
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	backup_depiction

	(
	int		itime
	)

	{
	int		idfld;
	DFLIST	*dfld;

	/* Confirm any pending edits */
	accept_mod();

	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Save each depiction field in the appropriate metafile */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		/* >>>>>
		if (tdep_special(dfld->tdep)) continue;
		<<<<< */
		(void) backup_depiction_field(dfld, itime);
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	delete_depiction

	(
	int		itime
	)

	{
	int		idfld;
	DFLIST	*dfld;

	/* Dispose of any pending edits */
	/* reject_mod(); */

	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Delete each depiction field metafile */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld  = DfldList + idfld;
		if (tdep_special(dfld->tdep)) continue;
		(void) delete_depiction_field(dfld, itime);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     i m p o r t _ d e p i c t i o n _ f i e l d                      *
*     f i n d _ d e p i c t i o n _ f i e l d                          *
*     r e f i t _ d e p i c t i o n _ f i e l d                        *
*     l a b e l _ d e p i c t i o n _ f i e l d                        *
*                                                                      *
***********************************************************************/

LOGICAL			import_depiction_field

	(
	FLD_DESCRIPT	*fd,
	DFLIST			*dfld,
	int				itime,
	LOGICAL			tsame,
	LOGICAL			calc
	)

	{
	METAFILE	fmeta;
	FRAME		*frame;
	FIELD		fld;
	STRING		elem, level;
	SET			set;

	if (!dfld) return FALSE;
	elem  = dfld->element;
	level = dfld->level;

	/* Ignore daily fields if source and target times don't match */
	if (tdep_daily(dfld->tdep) && !tsame)
		{
		pr_error("Fields", "Cannot import to different target %s:%s\n",
				 elem, level);
		put_message("depict-ignore", level, elem);
		(void) sleep(2);
		return FALSE;
		}

	/* Remove the field from the corresponding frame if currently there */
	frame       = dfld->frames + itime;
	frame->meta = destroy_metafile(frame->meta);

	/* Clear internal buffers in "luke-warm" database */
	clear_equation_database();

	/* Read in the metafile containing the requested field */
	if (!fd)
		{
		/* If field descriptor is NULL we must be asking to create one */
		fmeta = create_metafile();
		define_mf_projection(fmeta, MapProj);
		switch (dfld->editor)
			{
			case FpaC_VECTOR:
			case FpaC_CONTINUOUS:
				break;
			case FpaC_DISCRETE:
			case FpaC_WIND:
				set = create_set("area");
				define_set_bg_attribs(set, dfld->bgcal);
				add_set_to_metafile(fmeta, "b", elem, level, set);
				break;
			case FpaC_LINE:
				set = create_set("curve");
				add_set_to_metafile(fmeta, "c", elem, level, set);
				break;
			case FpaC_SCATTERED:
				set = create_set("spot");
				add_set_to_metafile(fmeta, "d", elem, level, set);
				break;
			case FpaC_LCHAIN:
				set = create_set("lchain");
				add_set_to_metafile(fmeta, "l", elem, level, set);
				break;
			}
		}
	else
		{
		fmeta = meta_input(fd);
		if (!fmeta)
			{
			if (calc && check_retrieve_metasfc(fd))
				{
				fld = retrieve_field(fd);
				if (!fld) return FALSE;

				fmeta = create_metafile();
				define_mf_projection(fmeta, MapProj);
				add_field_to_metafile(fmeta, fld);
				}
			else return FALSE;
			}
		}

	/* Remove old label fields */
	fld = take_mf_field(fmeta, "plot", NULL, NULL, elem, level);
	fld = destroy_field(fld);
	fld = take_mf_field(fmeta, "set", "label", NULL, elem, level);
	fld = destroy_field(fld);
	fld = take_mf_field(fmeta, "set", "barb", NULL, elem, level);
	fld = destroy_field(fld);

	/* Save in structures */
	frame->meta      = fmeta;
	frame->modified  = FALSE; /*(int) !same(source, "backup")*/
	frame->contoured = FALSE;

	/* Reset the transformation if this is the current depiction time */
	if (itime == ViewTime || itime == EditTime)
		{

		/* Set the transformation for this field */
		/* >>>>> replace this ...
		define_dn_xform(dfld->dn, "map", MapView, NullBox, MapProj, NullXform);
		... with the following <<<<< */

		/* Re-define the viewport and window for this field */
		copy_box(&(dfld->dn->viewport), MapView);
		dfld->dn->window.left   = 0.0;
		dfld->dn->window.right  = MapProj->definition.xlen;
		dfld->dn->window.bottom = 0.0;
		dfld->dn->window.top    = MapProj->definition.ylen;

		/* Re-define the map projection */
		copy_map_projection(&(dfld->dn->mproj), MapProj);

		/* Re-set the transformation for this field */
		block_xform(dfld->dn->xform, &(dfld->dn->viewport), &(dfld->dn->window));

		/* Set this as the current depiction field */
		(void) pick_depiction_field(dfld);
		}

	/* Redisplay the depiction field */
	(void) refit_depiction_field(dfld, itime);
	setup_metafile_presentation(fmeta, "FPA");
	(void) label_depiction_field(dfld, itime);

	/* >>> take these out for now!!
	dfld->interp   = FALSE;
	dfld->intlab   = FALSE;
	dfld->saved    = FALSE;
	dfld->reported = FALSE;
	link_status(dfld);
	<<< */

	(void) extract_dfield(dfld, itime);
	(void) check_dfield_background(dfld, itime);

	/* Done */
	return (LOGICAL) (fmeta->numfld > 0);
	}

/**********************************************************************/

FLD_DESCRIPT	*find_depiction_field

	(
	STRING			source,
	STRING			subsrc,
	STRING			rtime,
	STRING			vtime,
	STRING			elem,
	STRING			level
	)

	{
	static	FLD_DESCRIPT	fd;

	/* Build field descriptor for this field */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_MAP_PROJECTION,	MapProj,
					FpaF_SOURCE_NAME,		source,
					FpaF_SUBSOURCE_NAME,	subsrc,
					FpaF_RUN_TIME,			rtime,
					FpaF_VALID_TIME,		vtime,
					FpaF_ELEMENT_NAME,		elem,
					FpaF_LEVEL_NAME,		level,
					FpaF_END_OF_LIST)) return (FLD_DESCRIPT *) NULL;

	/* See if the file is present */
	/* Don't use find_retrieve_metasfc() since this also finds the covering */
	/* valid time for quasi-static fields */
	if (check_retrieve_metasfc(&fd)) return &fd;

	/* Not found */
	return (FLD_DESCRIPT *) NULL;
	}

/**********************************************************************/

LOGICAL	refit_depiction_field

	(
	DFLIST	*dfld,
	int		itime
	)

	{
	METAFILE	meta;
	int			ifld;
	FIELD		fld;
	BOX			box;

	if (!dfld)            return FALSE;
	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	meta = dfld->frames[itime].meta;
	if (!meta)            return FALSE;

	box.left   = 0;
	box.right  = MapProj->definition.xlen;
	box.bottom = 0;
	box.top    = MapProj->definition.ylen;

	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = meta->fields[ifld];
		if (!fld) continue;

		/* If the field is a surface, refit it to the standard grid */
		/* Otherwise just throw away bits that are entirely outside the map */
		switch (fld->ftype)
			{
			case FtypeSfc:	reproject_surface(fld->data.sfc, MapProj, MapProj,
									&(MapProj->grid));
							break;

			case FtypeSet:	strip_set(fld->data.set, &box);
							break;

			case FtypePlot:	strip_plot(fld->data.plot, &box);
							break;
			}
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	label_depiction_field

	(
	DFLIST	*dfld,
	int		itime
	)

	{
	COLOUR		colour;
	METAFILE	meta;
	FIELD		fld;
	SET			lset;
	STRING		elem, level, mtime;
	float		xmax, xinc, xval, ymax, yinc, yval;

	if (!dfld)            return FALSE;
	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	meta = dfld->frames[itime].meta;
	if (!meta)                     return FALSE;
	if (!tdep_special(dfld->tdep)) return TRUE;

	mtime  = TimeList[itime].mtime;
	elem   = dfld->element;
	level  = dfld->level;

	/* See if this static field is there */
	switch (dfld->editor)
		{
		case FpaC_VECTOR:
			fld = find_mf_field(meta, "surface", NULL, "v", elem, level);
			break;
		case FpaC_CONTINUOUS:
			fld = find_mf_field(meta, "surface", NULL, "a", elem, level);
			break;
		case FpaC_DISCRETE:
		case FpaC_WIND:
			fld = find_mf_field(meta, "set", "area", "b", elem, level);
			break;
		case FpaC_LINE:
			fld = find_mf_field(meta, "set", "curve", "c", elem, level);
			break;
		case FpaC_SCATTERED:
			fld = find_mf_field(meta, "set", "spot", "d", elem, level);
			break;
		case FpaC_LCHAIN:
			fld = find_mf_field(meta, "set", "lchain", "l", elem, level);
			break;
		default:
			fld = NullFld;
		}
	if (!fld) return FALSE;

	xmax = MapProj->definition.xlen;
	ymax = MapProj->definition.ylen;
	xinc = xmax/20;
	yinc = ymax/20;
	xval = xinc;
	yval = yinc*0.5*(1+dfld->labpos);
	colour = SkipColour;
	if (colour < 0) recall_fld_pspec(fld, LINE_COLOUR, (POINTER)&colour);
	if (colour < 0) recall_fld_pspec(fld, TEXT_COLOUR, (POINTER)&colour);

	/* Make sure it has a label field */
	lset = make_mf_set(meta, "spot", "d", elem, level);

	/* Create or adjust the legend */
	(void) make_depict_legend(lset, dfld, mtime, colour, xval, yval);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p i c k _ d e p i c t i o n _ f i e l d                          *
*                                                                      *
***********************************************************************/

LOGICAL	pick_depiction_field

	(
	DFLIST	*dfld
	)

	{
	int			itime;
	FRAME		*frame;
	METAFILE	meta;

	if (!dfld)                   itime = -1;
	else if (!ActiveDfld)        itime = pick_dfield_frame(dfld, ViewTime);
	else if (dfld == ActiveDfld) itime = EditTime;
	else if (same(dfld->group, ActiveDfld->group))
						     	 itime = pick_dfield_frame(dfld, GrpTime);
	else                         itime = pick_dfield_frame(dfld, ViewTime);

	/* Put metafile into corresponding dispnode */
	frame = (itime >= 0)? dfld->frames + itime: NULL;
	meta  = (frame)? frame->meta: NullMeta;
	dfld->dn->data.meta = NullMeta;
	define_dn_data(dfld->dn, "metafile", (POINTER) meta);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     u p d a t e _ d e p i c t i o n _ f i e l d                      *
*     b a c k u p _ d e p i c t i o n _ f i e l d                      *
*     d e l e t e _ d e p i c t i o n _ f i e l d                      *
*                                                                      *
***********************************************************************/

LOGICAL	update_depiction_field

	(
	DFLIST	*dfld,
	int		itime
	)

	{
	FRAME	*frame;

	if (ViewOnly)         return FALSE;
	if (!dfld)            return FALSE;
	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Save depiction field in the appropriate metafile */
	frame = dfld->frames + itime;
	if (frame->meta)
		{
		(void) remove_file(frame->wfile,  NULL);
		(void) remove_file(frame->owfile, NULL);
		write_metafile(frame->wfile, frame->meta, MaxDigits);
		}

	/* Clear internal buffers in "luke-warm" database */
	clear_equation_database();

	/* Notify the interface */
	field_status(dfld, itime);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	backup_depiction_field

	(
	DFLIST	*dfld,
	int		itime
	)

	{
	FRAME	*frame;

	/* Confirm any pending edits */
	accept_mod();

	if (!dfld)            return FALSE;
	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Save depiction field in the appropriate metafile */
	frame = dfld->frames + itime;
	if (frame->meta)
		{
		(void) remove_file(frame->bfile,  NULL);
		(void) remove_file(frame->obfile, NULL);
		write_metafile(frame->bfile, frame->meta, MaxDigits);
		}
	frame->modified = FALSE;

	return TRUE;
	}

/**********************************************************************/

LOGICAL	delete_depiction_field

	(
	DFLIST	*dfld,
	int		itime
	)

	{
	FRAME	*frame;
	int		status;

	/* Dispose of any pending edits */
	/* reject_mod(); */

	if (!dfld)            return FALSE;
	if (itime < 0)        return FALSE;
	if (itime >= NumTime) return FALSE;

	/* Delete each depiction field metafile */
	frame = dfld->frames + itime;
	frame->meta = destroy_metafile(frame->meta);
	frame->modified = TRUE;
	if (dfld->fields && dfld->fields[itime])
		{

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In delete_depiction_field() - T %s %s\n",
				dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In delete_depiction_field() - F %s %s\n",
				dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->fields[itime] = NullFld;
		if (dfld->flabs) dfld->flabs[itime] = NullSet;

		/* If a field existed at this time ... report the change */
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);
		}

	if (ViewOnly) return TRUE;

	if (find_file(frame->wfile))
		{
		status = unlink(frame->wfile);
		if (status != 0)
			{
			pr_error("Fields", "Cannot delete file \"%s\" (Error %d)\n",
					 frame->wfile, errno);
			}
		}

	if (find_file(frame->owfile))
		{
		status = unlink(frame->owfile);
		if (status != 0)
			{
			pr_error("Fields", "Cannot delete file \"%s\" (Error %d)\n",
					 frame->owfile, errno);
			}
		}

	/* Notify the interface */
	field_status(dfld, itime);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ d e p i c t i o n                                      *
*     h i d e _ d e p i c t i o n                                      *
*     p r e s e n t _ d e p i c t i o n                                *
*                                                                      *
***********************************************************************/

LOGICAL	show_depiction(void)

	{
	if (DepictShown)
		{
		if (DepictVis)
			{
			DepictVis = FALSE;
			return present_all();
			}
		return TRUE;
		}

	/* Set display state on */
	define_dn_vis(DnDepict, TRUE);
	DepictShown = TRUE;

	/* Re-display */
	return present_all();
	}

/**********************************************************************/

LOGICAL	hide_depiction(void)

	{
	if (!DepictShown) return TRUE;

	/* Turn off active field */
	(void) hide_edit_field();
	(void) release_links();
	(void) release_special_tags();
	(void) release_edit_field(TRUE);
	(void) pick_active_dfield("NONE", NULL);

	/* Set display state off */
	define_dn_vis(DnDepict, FALSE);
	DepictShown = FALSE;
	return present_all();
	}

/**********************************************************************/

LOGICAL	present_depiction

	(
	LOGICAL	all
	)

	{
	int		idfld;
	DFLIST	*dfld;

	if (ViewTime < 0) return FALSE;
	if (!DepictShown) return TRUE;

	/* Show the whole thing if requested */
	if (all) (void) present_node(DnDepict);

	/* Contour the depiction if necessary */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		(void) present_depiction_field(dfld);
		}

	/* Display active field */
	return present_edit_field(all);
	}

/***********************************************************************
*                                                                      *
*     p r e s e n t _ d e p i c t i o n _ f i e l d                    *
*                                                                      *
***********************************************************************/

LOGICAL	present_depiction_field

	(
	DFLIST	*dfld
	)

	{
	int		itime;
	FRAME	*frame;

	if (!DepictShown) return TRUE;
	if (ViewTime < 0) return TRUE;
	if (!dfld)        return TRUE;

	/* Find the latest valid time where this field is present */
	if (dfld == ActiveDfld) itime = EditTime;
	else                    itime = pick_dfield_frame(dfld, ViewTime);
	if (itime < 0) return TRUE;

	/* Still need to contour in case we want to edit */
	if (!dfld->dn->shown && dfld != ActiveDfld) return TRUE;

	/* Contour if necessary */
	frame = dfld->frames + itime;
	if (!frame->contoured)
		{
		if (!present_meta(dfld->dn)) return TRUE;
		(void) label_depiction_field(dfld, itime);
		frame->contoured = TRUE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s e t _ b a c k g r o u n d _ v a l u e                          *
*                                                                      *
***********************************************************************/

LOGICAL	set_background_value

	(
	STRING	elem,
	STRING	level
	)

	{
	DFLIST	*dfld;
	int		itime, ifrst, ilast;
	LOGICAL	redisp = FALSE;
	LOGICAL	change;
	LOGICAL	create_all = FALSE;
	FRAME	*frame;
	SET		areas;

	if (IsNull(EditCal)) return FALSE;

	/* Find given field and make sure background is settable */
	dfld = find_dfield(elem, level);
	if (!dfld)                        return FALSE;
	if (dfld->editor != FpaC_DISCRETE &&
		dfld->editor != FpaC_WIND)    return FALSE;

	/* Save the new values */
	CAL_empty(dfld->bgcal);
	CAL_merge(dfld->bgcal, EditCal, TRUE);
	CAL_clean(dfld->bgcal);
	dfld->bgset = TRUE;

	/* Initialize field buffer if originally empty */
	if (!dfld->there)
		{
		if (!SequenceReady) return FALSE;

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In set_background_value() - T %s %s\n",
				dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In set_background_value() - F %s %s\n",
				dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		dfld->there    = TRUE;
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;

		if (!dfld->fields)
			{
			dfld->fields = GETMEM(dfld->fields, FIELD, NumTime);
			dfld->flabs  = GETMEM(dfld->flabs,  SET,   NumTime);
			for (itime=0; itime<NumTime; itime++)
				{
				dfld->fields[itime] = NullFld;
				dfld->flabs[itime]  = NullSet;
				}
			}
		}

	/*** PATCH: FPA V4.0 P4 ***/
	/*** Create all wind backgrounds if there are no wind charts yet ***/
	/* See if there are any charts with areas anywhere in the sequence */
	create_all = TRUE;
	for (itime=0; itime<NumTime; itime++)
		{
        /* Only makes sense for depictions */
        if (!TimeList[itime].depict) continue;

		/* See if there is an actual chart with areas */
        frame = dfld->frames + itime;
        if (!frame->meta)    continue;
		areas = find_mf_set(frame->meta, "area", "b", elem, level);
		if (!areas)          continue;
		if (areas->num <= 0) continue;

		/* If there is a chart, we cannot create empty fields everywhere */
		create_all = FALSE;
		}

	/* Set background in all frames */
	ifrst = first_depict_time();
	ilast = last_depict_time();
	for (itime=0; itime<NumTime; itime++)
		{
        /* Only makes sense for depictions */
        if (!TimeList[itime].depict) continue;

		/* Create an empty field in first/last depiction frame if necessary */
        frame = dfld->frames + itime;
        if (!frame->meta)
			{
			if (create_all || itime==ifrst || itime==ilast)
				{
				change = TRUE;
				(void) import_depiction_field(NULL, dfld, itime, TRUE, TRUE);
				(void) interp_dfield_links(dfld, itime, TRUE);
				if (dfld->linkto) (void) revise_dependent_links(dfld->linkto);
				}
			else continue;
			}

		/* Save to disk */
		(void) update_depiction_field(dfld, itime);

		/* Set the background in this chart */
		change = check_dfield_background(dfld, itime);
		if (!change) continue;

		/* Redefine background in edit buffers if necessary */
		if (itime != EditTime) continue;
		if (dfld->showdep) redisp = TRUE;
		if (dfld != ActiveDfld) continue;
		define_set_bg_attribs(NewAreas, EditCal);
		empty_set(NewLabs);
		append_set(NewLabs, OldLabs);
		highlight_set(NewLabs, 1);
		redisp = TRUE;
		}

	/* Re-display if required */
	if (redisp) (void) present_all();

	(void) extract_dfields(EditTime);
	(void) cleanup_dfields();
	(void) ready_dfield_links(dfld, TRUE);
	(void) save_links();

	if (dfld == ActiveDfld)
		{
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(TRUE);
		if ( extract_edit_field() )
			{
			(void) extract_special_tags();
			(void) extract_links();
			(void) show_edit_field();
			}
		(void) present_all();
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     a c t i v e _ e d i t _ g r o u p                                *
*     a c t i v e _ e d i t _ f i e l d                                *
*     e x t r a c t _ e d i t _ f i e l d                              *
*     r e l e a s e _ e d i t _ f i e l d                              *
*                                                                      *
***********************************************************************/

LOGICAL	active_edit_group

	(
	STRING	new_group
	)

	{
	/* See if this one is already picked */
	if (ActiveDfld && DepictReady)
		{
		if (same(new_group, CurrGroup)) return TRUE;

		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		if ( !release_edit_field(TRUE) )
			{
			(void) present_all();
			return FALSE;
			}
		if ( !pick_active_dfield("NONE", NULL) )
			{
			(void) present_all();
			return FALSE;
			}
		}

	(void) pick_active_group(new_group);
	return present_all();
	}

/**********************************************************************/

LOGICAL	active_edit_field

	(
	STRING	new_elem,
	STRING	new_level
	)

	{
	/* See if this one is already picked */
	if (ActiveDfld && DepictReady)
		{
		if (same(new_elem, CurrElement) && same(new_level, CurrLevel))
			return TRUE;
		}

	(void) hide_edit_field();
	(void) release_links();
	(void) release_special_tags();
	if ( !release_edit_field(TRUE) )
		{
		(void) present_all();
		return FALSE;
		}
	if ( !pick_active_dfield(new_elem, new_level) )
		{
		(void) present_all();
		return FALSE;
		}
	if ( !extract_edit_field() )
		{
		(void) present_all();
		return FALSE;
		}
	(void) extract_special_tags();
	(void) extract_links();
	(void) show_edit_field();

	return present_all();
	}

/**********************************************************************/

LOGICAL	extract_edit_field(void)

	{
	int		ifld;
	FIELD	fld;

	/* A field must be picked */
	if (!ActiveDfld)                 return FALSE;
	if (same(CurrElement, "NONE"))   return TRUE;

	/* Set the field times */
	field_times(ActiveDfld);

	/* Only times matter for MASTER_LINK field */
	if (same(CurrElement, "MASTER_LINK")) return TRUE;
	/* >>>>> moved to here!!!!! <<<<< */

	/* Set metafile for this field and time ... if it exists! */
	ActiveMeta = ActiveDfld->dn->data.meta;

	/* Check for time sequence for daily or static fields */
	if (tdep_special(ActiveDfld->tdep))
		{
		if (!ActiveDfld->frames)                return FALSE;
		if (EditTime < 0)                       return FALSE;
		if (!ActiveDfld->frames[EditTime].meta) return FALSE;
		}

	/* Check for depiction status */
	if (DepictReady) return TRUE;
	DepictReady = TRUE;

	/* Empty out the original field node */
	/* Do not destroy field data because we did not use a copy! */
	for (ifld=0; ifld<OrigMeta->numfld; ifld++)
		{
		fld = OrigMeta->fields[ifld];
		if (!fld) continue;
		fld->data.sfc = (POINTER) 0;
		}
	empty_metafile(OrigMeta);

	/* Empty out the edit node */
	empty_metafile(EditMeta);

	/* Re-initialize working fields */
	OldSurface = NullSfc;	NewSurface = NullSfc;
	OldAreas   = NullSet;	NewAreas   = NullSet;
	OldCurves  = NullSet;	NewCurves  = NullSet;
	OldPoints  = NullSet;	NewPoints  = NullSet;
	OldLchains = NullSet;	NewLchains = NullSet;
	OldLabs    = NullSet;	NewLabs    = NullSet;

	ActiveField = NullFld;
	LabField    = NullFld;

	/* Clear internal buffers in "luke-warm" database */
	clear_equation_database();

	switch (CurrEditor)
		{
		/* Extract continuous fields */
		case FpaC_VECTOR:
			ActiveField = find_mf_field(ActiveMeta, "surface", NULL,
										"v", CurrElement, CurrLevel);
			OldSurface  = (ActiveField) ? ActiveField->data.sfc
										: NullSfc;
			NewSurface  = (OldSurface)  ? copy_surface(OldSurface, TRUE)
										: NullSfc;

			/* Set up contour specs for brand new surfaces */
			if (!OldSurface)
				setup_sfc_presentation(NewSurface, CurrElement, CurrLevel,
									   "FPA");

			/* Highlight new surface */
			highlight_surface(NewSurface, 1);

			/* Add to edit node */
			add_sfc_to_metafile(OrigMeta, "v", CurrElement, CurrLevel,
								OldSurface);

			/* Leave old surface visible */
			add_sfc_to_metafile(EditMeta, "v", CurrElement, CurrLevel,
								NewSurface);
			break;

		case FpaC_CONTINUOUS:
			ActiveField = find_mf_field(ActiveMeta, "surface", NULL,
										"a", CurrElement, CurrLevel);
			OldSurface  = (ActiveField) ? ActiveField->data.sfc
										: NullSfc;
			NewSurface  = (OldSurface)  ? copy_surface(OldSurface, TRUE)
										: NullSfc;

			/* Set up contour specs for brand new surfaces */
			if (!OldSurface)
				setup_sfc_presentation(NewSurface, CurrElement, CurrLevel,
									   "FPA");

			/* Highlight new surface */
			highlight_surface(NewSurface, 1);

			/* Add to edit node */
			add_sfc_to_metafile(OrigMeta, "a", CurrElement, CurrLevel,
								OldSurface);

			/* Leave old surface visible */
			add_sfc_to_metafile(EditMeta, "a", CurrElement, CurrLevel,
								NewSurface);
			break;

		/* Extract discrete and wind fields */
		case FpaC_DISCRETE:
		case FpaC_WIND:
			ActiveField = find_mf_field(ActiveMeta, "set", "area",
										"b", CurrElement, CurrLevel);
			OldAreas    = (ActiveField) ? ActiveField->data.set : NullSet;
			NewAreas    = (OldAreas)    ? copy_set(OldAreas)    : NullSet;

			/* Set up category specs for brand new area fields */
			if (!OldAreas)
				{
				NewAreas = create_set("area");
				define_set_bg_attribs(NewAreas, ActiveDfld->bgcal);
				setup_set_presentation(NewAreas, CurrElement, CurrLevel, "FPA");
				}

			/* Set up background attributes if not yet defined */
			if (!NewAreas->bgnd || !((AREA)NewAreas->bgnd)->attrib )
				{
				define_set_bg_attribs(NewAreas, ActiveDfld->bgcal);
				}

			/* Save area link nodes at this edit time */
			(void) post_area_link_nodes(ActiveDfld, EditTime);

			/* Highlight new area field */
			highlight_set(NewAreas, 1);

			/* Add to edit node */
			add_set_to_metafile(EditMeta, "b", CurrElement, CurrLevel,
								NewAreas);
			break;

		/* Extract line fields */
		case FpaC_LINE:
			ActiveField = find_mf_field(ActiveMeta, "set", "curve",
										"c", CurrElement, CurrLevel);
			OldCurves   = (ActiveField) ? ActiveField->data.set : NullSet;
			NewCurves   = (OldCurves)   ? copy_set(OldCurves)   : NullSet;

			/* Set up presentation specs for brand new line fields */
			if (!OldCurves)
				{
				NewCurves = create_set("curve");
				setup_set_presentation(NewCurves, CurrElement, CurrLevel,
									   "FPA");
				}

			/* Highlight new line field */
			highlight_set(NewCurves, 1);

			/* Add to edit node */
			add_set_to_metafile(EditMeta, "c", CurrElement, CurrLevel,
								NewCurves);
			break;

		/* Extract point (spot) fields */
		case FpaC_SCATTERED:
			ActiveField = find_mf_field(ActiveMeta, "set", "spot",
										 "d", CurrElement, CurrLevel);
			OldPoints   = (ActiveField) ? ActiveField->data.set : NullSet;
			NewPoints   = (OldPoints)   ? copy_set(OldPoints)   : NullSet;

			/* Highlight new line field */
			highlight_set(NewPoints, 1);

			/* Add to edit node */
			add_set_to_metafile(EditMeta, "d", CurrElement, CurrLevel,
								NewPoints);
			break;

		/* Extract link chain fields */
		case FpaC_LCHAIN:
			ActiveField = find_mf_field(ActiveMeta, "set", "lchain",
										 "l", CurrElement, CurrLevel);
			OldLchains  = (ActiveField) ? ActiveField->data.set : NullSet;
			NewLchains  = (OldLchains)  ? copy_set(OldLchains)  : NullSet;

			/* Set up presentation specs for brand new link chain fields */
			if (!OldLchains)
				{
				NewLchains = create_set("lchain");
				setup_set_presentation(NewLchains, CurrElement, CurrLevel,
									   "FPA");
				}

			/* Highlight new link chain field */
			highlight_set(NewLchains, 1);

			/* Add to edit node */
			add_set_to_metafile(EditMeta, "l", CurrElement, CurrLevel,
								NewLchains);
			break;

		default:
			ActiveField = NullFld;
		}

	/* Extract label field for given field */
	switch (CurrEditor)
		{
		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
			LabField = find_mf_field(ActiveMeta, "set", "spot",
								 	NULL, CurrElement, CurrLevel);
			OldLabs  = (LabField) ? LabField->data.set : NullSet;
			NewLabs  = (OldLabs)  ? copy_set(OldLabs)  : NullSet;

			/* Set up presentation specs for brand new label fields */
			if (!OldLabs)
				{
				NewLabs = create_set("spot");
				setup_set_presentation(NewLabs, CurrElement, CurrLevel, "FPA");
				}

			/* Highlight new labels */
			highlight_set(NewLabs, 1);

			/* Add to edit node */
			add_set_to_metafile(EditMeta, "d", CurrElement, CurrLevel, NewLabs);
			break;

		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
		default:
			LabField = NullFld;
		}

	return TRUE;
	}

/**********************************************************************/

LOGICAL	release_edit_field

	(
	LOGICAL	reset
	)

	{
	int		ifld;
	FIELD	fld;

	if (!ActiveDfld)  return TRUE;
	if (!DepictReady) return TRUE;
	DepictReady = FALSE;

	/* Accept previous pending edits */
	accept_mod();

	/* Empty out the original field node */
	/* Do not destroy field data because we did not use a copy! */
	for (ifld=0; ifld<OrigMeta->numfld; ifld++)
		{
		fld = OrigMeta->fields[ifld];
		if (!fld) continue;
		fld->data.sfc = (POINTER) 0;
		}
	empty_metafile(OrigMeta);

	/* Empty out the edit node */
	empty_metafile(EditMeta);

	/* Re-initialize working fields */
	OldSurface = NullSfc;	NewSurface = NullSfc;
	OldAreas   = NullSet;	NewAreas   = NullSet;
	OldCurves  = NullSet;	NewCurves  = NullSet;
	OldPoints  = NullSet;	NewPoints  = NullSet;
	OldLchains = NullSet;	NewLchains = NullSet;
	OldLabs    = NullSet;	NewLabs    = NullSet;

	ActiveField = NullFld;
	LabField    = NullFld;
	ActiveMeta  = NullMeta;

	if (reset)
		{
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
		}
	ActiveMeta = NullMeta;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s h o w _ e d i t _ f i e l d                                    *
*     h i d e _ e d i t _ f i e l d                                    *
*     p r e s e n t _ e d i t _ f i e l d                              *
*                                                                      *
***********************************************************************/

LOGICAL	show_edit_field(void)

	{
	if (EditShown) return TRUE;

	/* Set display state on */
	define_dn_vis(DnOrig, TRUE);
	define_dn_vis(DnEdit, TRUE);
	EditShown = TRUE;

	set_dfield_visibility(ActiveDfld);

	/* Show any unlinked features */
	if (LinkShown) extract_unlinked();

	return TRUE;
	}

/**********************************************************************/

LOGICAL	hide_edit_field(void)

	{
	if (!EditShown) return TRUE;

	/* Set display state off */
	define_dn_vis(DnOrig, FALSE);
	define_dn_vis(DnEdit, FALSE);
	EditShown = FALSE;

	set_dfield_visibility(ActiveDfld);

	/* Hide any unlinked features */
	if (LinkShown) release_unlinked();

	return TRUE;
	}

/**********************************************************************/

LOGICAL	present_edit_field

	(
	LOGICAL	all
	)

	{
	/* Do nothing if depiction or edit field are hidden */
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	/* Show the whole thing if requested */
	if (all)
		{
		(void) present_node(DnOrig);
		(void) present_node(DnEdit);
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     e x t r a c t _ s p e c i a l _ t a g s                          *
*     r e l e a s e _ s p e c i a l _ t a g s                          *
*                                                                      *
***********************************************************************/

LOGICAL	extract_special_tags(void)

	{
	if (NewAreas) return extract_area_order_tags(FALSE);
	return FALSE;
	}

/**********************************************************************/

LOGICAL	release_special_tags(void)

	{
	if (NewAreas) return release_area_order_tags(FALSE);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     Functions which control editing of the active depiction field:   *
*                                                                      *
***********************************************************************/

static	LOGICAL	depiction_spline_2D_edit(STRING);
static	LOGICAL	depiction_spline_edit(STRING);
static	LOGICAL	depiction_area_edit(STRING);
static	LOGICAL	depiction_line_edit(STRING);
static	LOGICAL	depiction_point_edit(STRING);
static	LOGICAL	depiction_lchain_edit(STRING);

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ c h e c k                                    *
*     d e p i c t i o n _ e d i t                                      *
*     d e p i c t i o n _ l a b e l                                    *
*     d e p i c t i o n _ s a m p l e                                  *
*                                                                      *
***********************************************************************/

LOGICAL	depiction_check(void)

	{
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	/* Set edit fields for first time */
	active_field_info(CurrElement, CurrLevel, "depict", NULL, NULL,
					TimeList[EditTime].jtime);
	active_spline_fields(TRUE, NewSurface, NewLabs);
	active_area_fields(TRUE, NewAreas, NewLabs);
	active_line_fields(TRUE, NewCurves, NewLabs);
	active_point_fields(TRUE, NewPoints);
	active_lchain_fields(TRUE, NewLchains);
	label_appearance(SkipColour, SkipLstyle, SkipWidth, ActiveDfld->dohilo);
	sample_appearance(SkipFont, SkipTsize, SkipColour);

	set_Xcurve_modes("draw");

	return TRUE;
	}

/**********************************************************************/

LOGICAL	depiction_edit

	(
	STRING	mode
	)

	{
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	switch (CurrEditor)
		{
		case FpaC_VECTOR:		return depiction_spline_2D_edit(mode);
		case FpaC_CONTINUOUS:	return depiction_spline_edit(mode);
		case FpaC_DISCRETE:		return depiction_area_edit(mode);
		case FpaC_WIND:			return depiction_area_edit(mode);
		case FpaC_LINE:			return depiction_line_edit(mode);
		case FpaC_SCATTERED:	return depiction_point_edit(mode);
		case FpaC_LCHAIN:		return depiction_lchain_edit(mode);
		}

	return FALSE;
	}

/**********************************************************************/

LOGICAL	depiction_label

	(
	STRING	mode
	)

	{
	LOGICAL	valid;

	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	switch (CurrEditor)
		{
		case FpaC_VECTOR:		if (!edit_ready_spline_field()) return FALSE;
								break;
		case FpaC_CONTINUOUS:	if (!edit_ready_spline_field()) return FALSE;
								break;
		case FpaC_DISCRETE:
		case FpaC_WIND:			if (!edit_ready_area_field(mode))  return FALSE;
								break;
		case FpaC_LINE:			if (!edit_ready_line_field(mode))  return FALSE;
								break;
		case FpaC_SCATTERED:	if (!edit_ready_point_field(mode)) return FALSE;
								break;
		}

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

	/* Unknown edit mode */
	else
		{
		put_message("label-unsupported", EditMode, "");
		return FALSE;
		}


	/* Came here from "ADD", or "LIST GO" */
	/* Resume in "ADD" mode */
	(void) strcpy(EditMode, "ADD");

	(void) label_add(mode, EditVal[0], EditCal);

	return TRUE;
	}

/**********************************************************************/

LOGICAL	depiction_sample

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
		fnt   = SafeFont;
		sz    = SafeTsize;
		clr   = SafeColour;
		first = FALSE;
		}

	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	switch (CurrEditor)
		{
		case FpaC_VECTOR:		if (!edit_ready_spline_field()) return FALSE;
								break;
		case FpaC_CONTINUOUS:	if (!edit_ready_spline_field()) return FALSE;
								break;
		case FpaC_DISCRETE:
		case FpaC_WIND:			if (!edit_ready_area_field(mode))  return FALSE;
								break;
		case FpaC_LINE:			if (!edit_ready_line_field(mode))  return FALSE;
								break;
		case FpaC_SCATTERED:	if (!edit_ready_point_field(mode)) return FALSE;
								break;
		case FpaC_LCHAIN:		if (!edit_ready_lchain_field(mode)) return FALSE;
								break;
		}

	/* Handle "cancel" sample */
	if (same(EditMode, "CANCEL"))
		{
		mode = "cancel";
		(void) sample_by_type(mode, EditVal[0], EditVal[1],
					"depict", NULL, NULL, CurrElement, CurrLevel);

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

	/* Unknown edit mode */
	else
		{
		put_message("sample-unsupported", EditMode, "");
		return FALSE;
		}

	/* Came here from "NORMAL", "GRID" or "LIST GO" */
	if (!blank(EditVal[2])) fnt = find_font(EditVal[2], &valid);
	if (!blank(EditVal[3])) sz  = find_size(EditVal[3], &valid);
	if (!blank(EditVal[4])) clr = find_colour(EditVal[4], &valid);
	sample_appearance(fnt, sz, clr);

	/* Resume in normal mode */
	(void) strcpy(EditMode, "NORMAL");

	(void) sample_by_type(mode, EditVal[0], EditVal[1],
				"depict", NULL, NULL, CurrElement, CurrLevel);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ s p l i n e _ 2 D _ e d i t                  *
*                                                                      *
***********************************************************************/

static	LOGICAL	depiction_spline_2D_edit

	(
	STRING	mode
	)

	{
	float	dmag, ddir, edge;

	/* Make sure the current field uses this editor */
	if (CurrEditor != FpaC_VECTOR) return FALSE;
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	if (!edit_ready_spline_field()) return FALSE;

	/* Handle "poke" edit */
	if (same(EditMode, "POKE"))
	    {
		if (sscanf(EditVal[0], "%g", &dmag) < 1) dmag = 0;
		if (sscanf(EditVal[1], "%g", &ddir) < 1) ddir = 0;
	    (void) edit_poke_spline_2D(mode, dmag, ddir);
	    }

	/* Handle "stomp" edit */
	else if (same(EditMode, "STOMP"))
	    {
		if (same(EditVal[0], "PRESET_OUTLINE"))
			{
	    	(void) edit_stomp_spline_2D(mode, EditVal[1], 0, 0, -999.0);
			}
		else
			{
			if (sscanf(EditVal[0], "%g", &dmag) < 1) dmag = 0;
			if (sscanf(EditVal[1], "%g", &ddir) < 1) ddir = 0;
			if (sscanf(EditVal[2], "%g", &edge) < 1) edge = 50;
			(void) edit_stomp_spline_2D(mode, NULL, dmag, ddir, edge);
			}
	    }

	/* Handle "drag" edit */
	else if (same(EditMode, "DRAG"))
	    {
	    (void) edit_drag_spline_2D(mode);
	    }

	/* Handle "area" edit */
	else if (same(EditMode, "AREA"))
	    {
	    (void) edit_block_spline_2D(mode, EditVal[1]);
	    }

	/* Handle "merge" edit */
	else if (same(EditMode, "MERGE"))
	    {
	    (void) edit_merge_spline_2D(mode,
					EditVal[1], EditVal[2], EditVal[3], EditVal[4],
					EditVal[5], EditVal[6]);
	    }

	/* Handle "smooth" edit */
	else if (same(EditMode, "SMOOTH"))
	    {
	    (void) edit_smooth_spline_2D(mode, EditVal[1]);
	    }

	/* Unknown edit mode */
	else
		{
		put_message("spline-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ s p l i n e _ e d i t                        *
*                                                                      *
***********************************************************************/

static	LOGICAL	depiction_spline_edit

	(
	STRING	mode
	)

	{
	float	amount, edge;

	/* Make sure the current field uses this editor */
	if (CurrEditor != FpaC_CONTINUOUS) return FALSE;
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	if (!edit_ready_spline_field()) return FALSE;

	/* Handle "poke" edit */
	if (same(EditMode, "POKE"))
	    {
		if (sscanf(EditVal[0], "%g", &amount) < 1) amount = 0;
	    (void) edit_poke_spline(mode, amount);
	    }

	/* Handle "stomp" edit */
	else if (same(EditMode, "STOMP"))
	    {
		if (same(EditVal[0], "PRESET_OUTLINE"))
			{
	    	(void) edit_stomp_spline(mode, EditVal[1], 0, -999.0);
			}
		else
			{
			if (sscanf(EditVal[0], "%g", &amount) < 1) amount = 0;
			if (sscanf(EditVal[1], "%g", &edge)   < 1) edge   = 50;
	    	(void) edit_stomp_spline(mode, NULL, amount, edge);
			}
	    }

	/* Handle "drag" edit */
	else if (same(EditMode, "DRAG"))
	    {
	    (void) edit_drag_spline(mode);
	    }

	/* Handle "area" edit */
	else if (same(EditMode, "AREA"))
	    {
	    (void) edit_block_spline(mode, EditVal[1]);
	    }

	/* Handle "merge" edit */
	else if (same(EditMode, "MERGE"))
	    {
	    (void) edit_merge_spline(mode,
					EditVal[1], EditVal[2], EditVal[3], EditVal[4],
					EditVal[5], EditVal[6]);
	    }

	/* Handle "smooth" edit */
	else if (same(EditMode, "SMOOTH"))
	    {
	    (void) edit_smooth_spline(mode, EditVal[1]);
	    }

	/* Unknown edit mode */
	else
		{
		put_message("spline-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ a r e a _ e d i t                            *
*                                                                      *
***********************************************************************/

static	LOGICAL	depiction_area_edit

	(
	STRING	mode
	)

	{
	/* Make sure the current field uses this editor */
	if (CurrEditor != FpaC_DISCRETE &&
		CurrEditor != FpaC_WIND) return FALSE;
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	if (!edit_ready_area_field(mode)) return FALSE;

	/* Handle "draw" edit */
	if (same(EditMode, "DRAW"))
	    {
		(void) edit_draw_area(mode, EditCal);
	    }

#ifdef NOT_READY
#endif
	/* Handle "draw_hole" edit */
	else if (same(EditMode, "DRAW_HOLE"))
	    {
	    (void) edit_addhole_area(mode, EditVal[1]);
	    }

	/* Handle "move" edit */
	else if (same(EditMode, "MOVE"))
	    {
	    (void) edit_move_area(mode, EditVal[1]);
	    }

	/* Handle "divide" edit */
	else if (same(EditMode, "DIVIDE"))
	    {
	    (void) edit_divide_area(mode, EditCal);
	    }

	/* Handle "modify" edit */
	else if (same(EditMode, "MODIFY"))
	    {
		set_Xcurve_modes("modify");
		(void) edit_modify_area(mode, EditVal[1], EditCal);
	    }

	/* Handle "merge" edit */
	else if (same(EditMode, "MERGE"))
	    {
	    (void) edit_merge_area(mode,
					EditVal[1], EditVal[2], EditVal[3], EditVal[4],
					EditVal[5], EditVal[6]);
	    }

	/* Unknown edit mode */
	else
		{
		put_message("area-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ l i n e _ e d i t                            *
*                                                                      *
***********************************************************************/

static	LOGICAL	depiction_line_edit

	(
	STRING	mode
	)

	{
	/* Make sure the current field uses this editor */
	if (CurrEditor != FpaC_LINE) return FALSE;
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	if (!edit_ready_line_field(mode)) return FALSE;

	/* Handle "draw" edit */
	if (same(EditMode, "DRAW"))
	    {
	    (void) edit_draw_line(mode, EditCal);
	    }

	/* Handle "flip" edit */
	else if (same(EditMode, "FLIP"))
	    {
		(void) edit_flip_line(mode);
	    }

	/* Handle "move" edit */
	else if (same(EditMode, "MOVE"))
	    {
	    (void) edit_move_line(mode, EditVal[1]);
	    }

	/* Handle "modify" edit */
	else if (same(EditMode, "MODIFY"))
	    {
		set_Xcurve_modes("modify");
		(void) edit_modify_line(mode, EditCal);
	    }

	/* Handle "merge" edit */
	else if (same(EditMode, "MERGE"))
	    {
	    (void) edit_merge_line(mode,
					EditVal[1], EditVal[2], EditVal[3], EditVal[4],
					EditVal[5], EditVal[6]);
	    }

	/* Handle "join" edit */
	else if (same(EditMode, "JOIN"))
	    {
	    (void) edit_join_line(mode);
	    }

	/* Unknown edit mode */
	else
		{
		put_message("line-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ p o i n t _ e d i t                          *
*                                                                      *
***********************************************************************/

/*ARGSUSED*/
static	LOGICAL	depiction_point_edit

	(
	STRING	mode
	)

	{
	/* Make sure the current field uses this editor */
	if (CurrEditor != FpaC_SCATTERED) return FALSE;
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	if (!edit_ready_point_field(mode)) return FALSE;

	/* Handle "draw" edit */
	if (same(EditMode, "DRAW"))
	    {
	    (void) edit_draw_point(mode, EditVal[0], EditCal);
	    }

	/* Handle "move" edit */
	else if (same(EditMode, "MOVE"))
	    {
	    (void) edit_move_point(mode, EditVal[1]);
	    }

	/* Handle "modify" edit */
	else if (same(EditMode, "MODIFY"))
	    {
		(void) edit_modify_point(mode, EditVal[0], EditCal);
	    }

	/* Handle "merge" edit */
	else if (same(EditMode, "MERGE"))
	    {
	    (void) edit_merge_point(mode,
					EditVal[1], EditVal[2], EditVal[3], EditVal[4],
					EditVal[5], EditVal[6]);
	    }

	/* Unknown edit mode */
	else
		{
		put_message("point-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     d e p i c t i o n _ l c h a i n _ e d i t                        *
*                                                                      *
***********************************************************************/

static	LOGICAL	depiction_lchain_edit

	(
	STRING	mode
	)

	{
	int		ndelta, splus, eplus;

	/* Make sure the current field uses this editor */
	if (CurrEditor != FpaC_LCHAIN) return FALSE;
	if (EditTime < 0) return FALSE;
	if (!ActiveDfld)  return FALSE;
	if (!DepictShown) return FALSE;
	if (!EditShown)   return FALSE;

	if (!edit_ready_lchain_field(mode)) return FALSE;

	/* Handle "add" edit */
	if (same(EditMode, "ADD"))
	    {
		if (same(mode, "new_chain")) move_edit_vals(0, 1);
		if (sscanf(EditVal[1], "%d", &ndelta) < 1) ndelta = LchainDelta;
	    (void) edit_add_lchain(mode, EditVal[0], ndelta, EditVal[2],
													EditCal, EditCalX);
	    }

	/* Handle "move" edit */
	else if (same(EditMode, "MOVE"))
	    {
	    (void) edit_move_lchain(mode, EditVal[1]);
	    }

	/* Handle "modify" edit */
	else if (same(EditMode, "MODIFY"))
	    {
		if (sscanf(EditVal[2], "%d", &splus) < 1) splus = 0;
		if (sscanf(EditVal[3], "%d", &eplus) < 1) eplus = 0;
		(void) edit_modify_lchain(mode, EditVal[1], splus, eplus, EditCal);
	    }

	/* Handle "merge" edit */
	else if (same(EditMode, "MERGE"))
	    {
	    (void) edit_merge_lchain(mode,
					EditVal[1], EditVal[2], EditVal[3], EditVal[4],
					EditVal[5], EditVal[6]);
	    }

	/* Handle "nodes" edit */
	else if (same(EditMode, "NODES"))
	    {
		/* >>>>> for testing <<<<< */
		printf("Call to edit_nodes_lchain with mode: %s %s\n", mode, EditVal[1]);
		/* >>>>> for testing <<<<< */

	    (void) edit_nodes_lchain(mode, EditVal[1], EditCal);
	    }

	/* Unknown edit mode */
	else
		{
		put_message("lchain-unsupported", EditMode, "", "");
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     e d i t _ p o s t e d   - Has an edit been posted.               *
*                                                                      *
*         p o s t _ m o d     - Post the given changed field.          *
*                                                                      *
*     a c c e p t _ m o d     - Accept the posted changes.             *
*                                                                      *
*     r e j e c t _ m o d     - Reject the posted changes.             *
*                                                                      *
***********************************************************************/

static	int		posted			= FALSE;
static	int		post_surface	= FALSE;
static	int		post_areas		= FALSE;
static	int		post_curves		= FALSE;
static	int		post_points		= FALSE;
static	int		post_lchains	= FALSE;
static	int		post_labs		= FALSE;

/**********************************************************************/

LOGICAL	edit_posted(void)

	{
	return posted;
	}

/**********************************************************************/

void	post_mod

	(
	STRING	mode
	)

	{
	/* Decide what has potentially changed */
	post_surface |= same(mode, "surface");
	post_areas   |= same(mode, "areas");
	post_curves  |= same(mode, "curves");
	post_points  |= same(mode, "points");
	post_lchains |= same(mode, "lchains");
	post_labs    |= same(mode, "labs");

#	ifdef DEBUG_MOD
	printf("         Posting %s edit\n", mode);
#	endif /* DEBUG_MOD */

	/* Make sure the request is legitimate */
	posted =  post_surface
		   || post_areas
		   || post_curves
		   || post_points
		   || post_lchains
		   || post_labs;

	if (posted) edit_pending();
	else        edit_complete();
	}

/**********************************************************************/

void	accept_mod(void)

	{
	METAFILE	meta;
	FIELD		fld;
	FRAME		*frame;
	LOGICAL		redo_bgnd = FALSE;
	LOGICAL		refresh   = FALSE;
	STRING		ent;

	cancel_partial();
	if (!posted) return;

	/* Make sure we have something to accept an edit in */
	if (!ActiveDfld)
		{
		reject_mod();
		return;
		}

	/* Accept surface changes */
	if (post_surface)
	    {
#	    ifdef DEBUG_MOD
		printf("         Accepting surface edit\n");
#	    endif /* DEBUG_MOD */
		switch (CurrEditor)
			{
			case FpaC_VECTOR:		ent = "v";	break;
			case FpaC_CONTINUOUS:	ent = "a";	break;
			}

		/* Add field to metafile if necessary */
		if (!ActiveField)
			{
			if (!ActiveMeta)
				{
				frame = ActiveDfld->frames + EditTime;
				meta  = frame->meta = create_metafile();
				define_mf_projection(meta, MapProj);
				ActiveDfld->dn->data.meta = NullMeta;
				define_dn_data(ActiveDfld->dn, "metafile", (POINTER) meta);
				ActiveMeta = ActiveDfld->dn->data.meta;
				}
			ActiveField = create_field(ent, CurrElement, CurrLevel);
			add_field_to_metafile(ActiveMeta, ActiveField);
			}

	    /* Replace old surface with new surface */
	    OldSurface = copy_surface(NewSurface, TRUE);
		highlight_surface(OldSurface, 0);
		define_fld_data(ActiveField, "surface", (POINTER)OldSurface);

	    /* Leave old surface visible */
		/* Do not destroy field data because we did not use a copy! */
		fld = take_mf_field(OrigMeta, "surface", NULL, ent, CurrElement,
								CurrLevel);
		fld->data.sfc = (POINTER) 0;
		fld = destroy_field(fld);
		add_sfc_to_metafile(OrigMeta, ent, CurrElement, CurrLevel, OldSurface);
		refresh = TRUE;

#		ifdef DEVELOPMENT
		if (ActiveDfld->reported)
			pr_info("Editor.Reported",
				"In accept_mod() - T %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
		else
			pr_info("Editor.Reported",
				"In accept_mod() - F %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
#		endif /* DEVELOPMENT */

		/* Turn off interpolated flag */
		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;

		/* Update this field so that the wind calculation will work */
		if (dependent_winds_affected(ActiveDfld, FALSE))
			{
#		    ifdef DEBUG_MOD
			printf("         Recomputing wind barbs\n");
#			endif /* DEBUG_MOD */
			(void) update_depiction_field(ActiveDfld, EditTime);
			recompute_dependent_winds(ActiveDfld, FALSE, EditTime);
			}
	    }

	/* Accept area field changes */
	if (post_areas)
	    {
#	    ifdef DEBUG_MOD
		printf("         Accepting areas edit\n");
#	    endif /* DEBUG_MOD */

		/* Add field to metafile if necessary */
		if (!ActiveField)
			{
			if (!ActiveMeta)
				{
				frame = ActiveDfld->frames + EditTime;
				meta  = frame->meta = create_metafile();
				define_mf_projection(meta, MapProj);
				ActiveDfld->dn->data.meta = NullMeta;
				define_dn_data(ActiveDfld->dn, "metafile", (POINTER) meta);
				ActiveMeta = ActiveDfld->dn->data.meta;
				}
			ActiveField = create_field("b", CurrElement, CurrLevel);
			add_field_to_metafile(ActiveMeta, ActiveField);
			}

	    /* Replace old area field with new area field */
		redo_bgnd = (LOGICAL) IsNull(OldAreas);
	    OldAreas  = copy_set(NewAreas);
		highlight_set(OldAreas, 0);
	    define_fld_data(ActiveField, "set", (POINTER)OldAreas);

		/* Save area link nodes at this edit time */
		(void) post_area_link_nodes(ActiveDfld, EditTime);

#		ifdef DEVELOPMENT
		if (ActiveDfld->reported)
			pr_info("Editor.Reported",
				"In accept_mod() - T %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
		else
			pr_info("Editor.Reported",
				"In accept_mod() - F %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
#		endif /* DEVELOPMENT */

		/* Turn off interpolated flag */
		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;
	    }

	/* Accept line field changes */
	if (post_curves)
	    {
#	    ifdef DEBUG_MOD
		printf("         Accepting curves edit\n");
#	    endif /* DEBUG_MOD */

		/* Add field to metafile if necessary */
		if (!ActiveField)
			{
			if (!ActiveMeta)
				{
				frame = ActiveDfld->frames + EditTime;
				meta  = frame->meta = create_metafile();
				define_mf_projection(meta, MapProj);
				ActiveDfld->dn->data.meta = NullMeta;
				define_dn_data(ActiveDfld->dn, "metafile", (POINTER) meta);
				ActiveMeta = ActiveDfld->dn->data.meta;
				}
			ActiveField = create_field("c", CurrElement, CurrLevel);
			add_field_to_metafile(ActiveMeta, ActiveField);
			}

	    /* Replace old line field with new line field */
	    OldCurves = copy_set(NewCurves);
		highlight_set(OldCurves, 0);
	    define_fld_data(ActiveField, "set", (POINTER)OldCurves);

#		ifdef DEVELOPMENT
		if (ActiveDfld->reported)
			pr_info("Editor.Reported",
				"In accept_mod() - T %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
		else
			pr_info("Editor.Reported",
				"In accept_mod() - F %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
#		endif /* DEVELOPMENT */

		/* Turn off interpolated flag */
		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;
	    }

	/* Accept point field changes */
	if (post_points)
	    {
#	    ifdef DEBUG_MOD
		printf("         Accepting points edit\n");
#	    endif /* DEBUG_MOD */

		/* Add field to metafile if necessary */
		if (!ActiveField)
			{
			if (!ActiveMeta)
				{
				frame = ActiveDfld->frames + EditTime;
				meta  = frame->meta = create_metafile();
				define_mf_projection(meta, MapProj);
				ActiveDfld->dn->data.meta = NullMeta;
				define_dn_data(ActiveDfld->dn, "metafile", (POINTER) meta);
				ActiveMeta = ActiveDfld->dn->data.meta;
				}
			ActiveField = create_field("d", CurrElement, CurrLevel);
			add_field_to_metafile(ActiveMeta, ActiveField);
			}

	    /* Replace old point field with new point field */
	    OldPoints = copy_set(NewPoints);
		highlight_set(OldPoints, 0);
	    define_fld_data(ActiveField, "set", (POINTER)OldPoints);

#		ifdef DEVELOPMENT
		if (ActiveDfld->reported)
			pr_info("Editor.Reported",
				"In accept_mod() - T %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
		else
			pr_info("Editor.Reported",
				"In accept_mod() - F %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
#		endif /* DEVELOPMENT */

		/* Turn off interpolated flag */
		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;
	    }

	/* Accept link chain field changes */
	if (post_lchains)
	    {
#	    ifdef DEBUG_MOD
		printf("         Accepting link chains edit\n");
#	    endif /* DEBUG_MOD */

		/* Add field to metafile if necessary */
		if (!ActiveField)
			{
			if (!ActiveMeta)
				{
				frame = ActiveDfld->frames + EditTime;
				meta  = frame->meta = create_metafile();
				define_mf_projection(meta, MapProj);
				ActiveDfld->dn->data.meta = NullMeta;
				define_dn_data(ActiveDfld->dn, "metafile", (POINTER) meta);
				ActiveMeta = ActiveDfld->dn->data.meta;
				}
			ActiveField = create_field("l", CurrElement, CurrLevel);
			add_field_to_metafile(ActiveMeta, ActiveField);
			}

	    /* Replace old link chain field with new link chain field */
	    OldLchains = copy_set(NewLchains);
		highlight_set(OldLchains, 0);
	    define_fld_data(ActiveField, "set", (POINTER)OldLchains);

#		ifdef DEVELOPMENT
		if (ActiveDfld->reported)
			pr_info("Editor.Reported",
				"In accept_mod() - T %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
		else
			pr_info("Editor.Reported",
				"In accept_mod() - F %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
#		endif /* DEVELOPMENT */

		/* Turn off interpolated flag */
		ActiveDfld->interp   = FALSE;
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->saved    = FALSE;
		ActiveDfld->reported = FALSE;
	    }

	/* Accept generic label changes */
	if (post_labs)
	    {
#	    ifdef DEBUG_MOD
		printf("         Accepting labs edit\n");
#	    endif /* DEBUG_MOD */

		/* Add field to metafile if necessary */
		if (!LabField)
			{
			if (!ActiveMeta)
				{
				frame = ActiveDfld->frames + EditTime;
				meta  = frame->meta = create_metafile();
				define_mf_projection(meta, MapProj);
				ActiveDfld->dn->data.meta = NullMeta;
				define_dn_data(ActiveDfld->dn, "metafile", (POINTER) meta);
				ActiveMeta = ActiveDfld->dn->data.meta;
				}
			LabField = create_field("d", CurrElement, CurrLevel);
			add_field_to_metafile(ActiveMeta, LabField);
			}

	    /* Replace old labs with new labs */
	    OldLabs = copy_set(NewLabs);
	    highlight_set(OldLabs, 0);
	    define_fld_data(LabField, "set", (POINTER)OldLabs);

#		ifdef DEVELOPMENT
		if (ActiveDfld->reported)
			pr_info("Editor.Reported",
				"In accept_mod() (labels) - T %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
		else
			pr_info("Editor.Reported",
				"In accept_mod() (labels) - F %s %s\n",
				ActiveDfld->element, ActiveDfld->level);
#		endif /* DEVELOPMENT */

		/* Turn off interpolated flag for labels */
		ActiveDfld->intlab   = FALSE;
		ActiveDfld->reported = FALSE;
	    }

	release_special_tags();
	extract_special_tags();

	/* Reset background value if not currently set */
	if (redo_bgnd && !ActiveDfld->bgset)
		(void) check_dfield_background(ActiveDfld, EditTime);

	/* Refresh display */
	if (refresh) (void) present_all();

	/* Save new fields */
	ActiveDfld->frames[EditTime].modified = TRUE;
	(void) extract_dfields(EditTime);
	(void) cleanup_dfields();
	(void) update_depiction(EditTime);
	(void) make_depict_status(EditTime, NULL, NULL);
	(void) verify_dfield_links(ActiveDfld, TRUE);
	(void) save_links();

	/* Done */
	post_surface = FALSE;
	post_areas   = FALSE;
	post_curves  = FALSE;
	post_points  = FALSE;
	post_lchains = FALSE;
	post_labs    = FALSE;
	posted       = FALSE;

	edit_complete();
	}

/**********************************************************************/

void	reject_mod(void)

	{
	FIELD	fld;
	STRING	ent;

	cancel_partial();
	if (!posted) return;

	/* Reject surface changes */
	if (post_surface)
	    {
#	    ifdef DEBUG_MOD
		printf("         Rejecting surface edit\n");
#	    endif /* DEBUG_MOD */
		switch (CurrEditor)
			{
			case FpaC_VECTOR:		ent = "v";	break;
			case FpaC_CONTINUOUS:	ent = "a";	break;
			}

	    /* Replace new surface with old surface */
		if (OldSurface)
			{
			fld = find_mf_field(EditMeta, "surface", NULL, ent, CurrElement,
								CurrLevel);
			NewSurface = destroy_surface(NewSurface);
			NewSurface = copy_surface(OldSurface, TRUE);
			highlight_surface(NewSurface, 1);
			if (fld) fld->data.sfc = NewSurface;
			}
		else
			{
			/* Was already NULL - just making sure */
			NewSurface = NullSfc;
			}
	    }

	/* Reject area field changes */
	if (post_areas)
	    {
#	    ifdef DEBUG_MOD
		printf("         Rejecting areas edit\n");
#	    endif /* DEBUG_MOD */

	    /* Replace new area field with old area field */
		if (OldAreas)
			{
			fld = find_mf_field(EditMeta, "set", "area", "b", CurrElement,
								CurrLevel);
			NewAreas = destroy_set(NewAreas);
			NewAreas = copy_set(OldAreas);
			highlight_set(NewAreas, 1);
			if (fld) fld->data.set = NewAreas;
			}
		else
			{
			empty_set(NewAreas);
			}

		/* Reset area link nodes at this edit time */
		(void) reset_area_link_nodes(ActiveDfld, EditTime);
	    }

	/* Reject line field changes */
	if (post_curves)
	    {
#	    ifdef DEBUG_MOD
		printf("         Rejecting curves edit\n");
#	    endif /* DEBUG_MOD */

	    /* Replace new line field with old line field */
		if (OldCurves)
			{
			fld = find_mf_field(EditMeta, "set", "curve", "c", CurrElement,
								CurrLevel);
			NewCurves = destroy_set(NewCurves);
			NewCurves = copy_set(OldCurves);
			highlight_set(NewCurves, 1);
			if (fld) fld->data.set = NewCurves;
			}
		else
			{
			empty_set(NewCurves);
			}
	    }

	/* Reject point field changes */
	if (post_points)
	    {
#	    ifdef DEBUG_MOD
		printf("         Rejecting points edit\n");
#	    endif /* DEBUG_MOD */

	    /* Replace new point field with old point field */
		if (OldPoints)
			{
			fld = find_mf_field(EditMeta, "set", "spot", "d", CurrElement,
								CurrLevel);
			NewPoints = destroy_set(NewPoints);
			NewPoints = copy_set(OldPoints);
			highlight_set(NewPoints, 1);
			if (fld) fld->data.set = NewPoints;
			}
		else
			{
			empty_set(NewPoints);
			}
	    }

	/* Reject link chain field changes */
	if (post_lchains)
	    {
#	    ifdef DEBUG_MOD
		printf("         Rejecting link chains edit\n");
#	    endif /* DEBUG_MOD */

	    /* Replace new link field with old link field */
		if (OldLchains)
			{
			fld = find_mf_field(EditMeta, "set", "lchain", "l", CurrElement,
								CurrLevel);
			NewLchains = destroy_set(NewLchains);
			NewLchains = copy_set(OldLchains);
			highlight_set(NewLchains, 1);
			if (fld) fld->data.set = NewLchains;
			}
		else
			{
			empty_set(NewLchains);
			}
	    }

	/* Reject lab changes */
	if (post_labs)
	    {
#	    ifdef DEBUG_MOD
		printf("         Rejecting labs edit\n");
#	    endif /* DEBUG_MOD */

	    /* Replace new labs with old labs */
		if (OldLabs)
			{
			fld = find_mf_field(EditMeta, "set", "spot", NULL, CurrElement,
								CurrLevel);
			NewLabs = destroy_set(NewLabs);
			NewLabs = copy_set(OldLabs);
			highlight_set(NewLabs, 1);
			if (fld) fld->data.set = NewLabs;
			}
		else
			{
			empty_set(NewLabs);
			}
	    }

	release_special_tags();
	extract_special_tags();

	/* Refresh display */
	(void) present_all();

	/* Done */
	post_surface = FALSE;
	post_areas   = FALSE;
	post_curves  = FALSE;
	post_lchains = FALSE;
	post_labs    = FALSE;
	posted       = FALSE;

	edit_complete();
	}

/***********************************************************************
*                                                                      *
*     p o s t _ p a r t i a l       - Indicate whether an edit is in   *
*                                     progress.                        *
*                                                                      *
*     i g n o r e _ p a r t i a l   - Ignore an edit in progress.      *
*                                                                      *
*     c a n c e l _ p a r t i a l   - Cancel an edit in progress.      *
*                                                                      *
***********************************************************************/

static	MODULE	PendingModule   = MODULE_NONE;
static	EDITOR	PendingEditor   = EDITOR_NONE;
static	char	PendingMode[20] = "";

/**********************************************************************/

void	post_partial

	(
	LOGICAL	cancel	/* enable cancel button? */
	)

	{
	if (cancel && !blank(EditMode)) edit_in_progress();

	PendingModule = Module;
	PendingEditor = Editor;
	(void) strcpy(PendingMode, EditMode);
	}

/**********************************************************************/

void	ignore_partial(void)

	{
	if (!blank(PendingMode)) edit_ignore();

	PendingModule = MODULE_NONE;
	PendingEditor = EDITOR_NONE;
	(void) strcpy(PendingMode, "");
	}

/**********************************************************************/

void	cancel_partial(void)

	{
	STRING	mode = "cancel all";

	switch (PendingModule)
	{
	case MODULE_DEPICT:
		switch (PendingEditor)
		{
		case EDITOR_SAMPLE:
			break;

		case EDITOR_LABEL:
			if (same(PendingMode, "SHOW"))
				(void) label_show(mode);

			else if (same(PendingMode, "MODIFY"))
				(void) label_modify(mode, NULL, NullCal);

			break;

		case EDITOR_EDIT:
			switch (CurrEditor)
				{
				case FpaC_VECTOR:
					if (same(PendingMode, "DRAG"))
						(void) edit_drag_spline_2D(mode);

					else if (same(PendingMode, "AREA"))
						(void) edit_block_spline_2D(mode, NULL);

					else if (same(PendingMode, "STOMP"))
						(void) edit_stomp_spline_2D(mode, NULL, 0.0, 0.0,
																		-555.0);

					else if (same(PendingMode, "MERGE"))
						(void) edit_merge_spline_2D(mode, NULL, NULL, NULL,
									NULL, NULL, NULL);

					else if (same(PendingMode, "SMOOTH"))
						(void) edit_smooth_spline_2D(mode, NULL);

					break;

				case FpaC_CONTINUOUS:
					if (same(PendingMode, "DRAG"))
						(void) edit_drag_spline(mode);

					else if (same(PendingMode, "AREA"))
						(void) edit_block_spline(mode, NULL);

					else if (same(PendingMode, "STOMP"))
						(void) edit_stomp_spline(mode, NULL, 0.0, -555.0);

					else if (same(PendingMode, "MERGE"))
						(void) edit_merge_spline(mode, NULL, NULL, NULL,
									NULL, NULL, NULL);

					else if (same(PendingMode, "SMOOTH"))
						(void) edit_smooth_spline(mode, NULL);

					break;

				case FpaC_DISCRETE:
				case FpaC_WIND:
					if (same(PendingMode, "DRAW"))
						(void) edit_draw_area(mode, NullCal);

					else if (same(PendingMode, "DRAW_HOLE"))
						(void) edit_addhole_area(mode, NULL);

					else if (same(PendingMode, "MOVE"))
						(void) edit_move_area(mode, NULL);

					else if (same(PendingMode, "MODIFY"))
						(void) edit_modify_area(mode, NULL, NullCal);

					else if (same(PendingMode, "DIVIDE"))
						(void) edit_divide_area(mode, NullCal);

					else if (same(PendingMode, "MERGE"))
						(void) edit_merge_area(mode, NULL, NULL, NULL,
									NULL, NULL, NULL);

					break;

				case FpaC_LINE:
					if (same(PendingMode, "DRAW"))
						(void) edit_draw_line(mode, NullCal);

					else if (same(PendingMode, "FLIP"))
						(void) edit_flip_line(mode);

					else if (same(PendingMode, "MOVE"))
						(void) edit_move_line(mode, NULL);

					else if (same(PendingMode, "MODIFY"))
						(void) edit_modify_line(mode, NullCal);

					else if (same(PendingMode, "MERGE"))
						(void) edit_merge_line(mode, NULL, NULL, NULL,
									NULL, NULL, NULL);

					else if (same(PendingMode, "JOIN"))
						(void) edit_join_line(mode);

					break;

				case FpaC_SCATTERED:
					if (same(PendingMode, "DRAW"))
						(void) edit_draw_point(mode, NULL, NullCal);

					else if (same(PendingMode, "MOVE"))
						(void) edit_move_point(mode, NULL);

					else if (same(PendingMode, "MODIFY"))
						(void) edit_modify_point(mode, NULL, NullCal);

					else if (same(PendingMode, "MERGE"))
						(void) edit_merge_point(mode, NULL, NULL, NULL,
									NULL, NULL, NULL);

					break;

				case FpaC_LCHAIN:
					if (same(PendingMode, "ADD"))
						(void) edit_add_lchain(mode, NULL, 0, NULL, NullCal,
									NullCal);

					else if (same(PendingMode, "MOVE"))
						(void) edit_move_lchain(mode, NULL);

					else if (same(PendingMode, "MODIFY"))
						(void) edit_modify_lchain(mode, NULL, 0, 0, NullCal);

					else if (same(PendingMode, "MERGE"))
						(void) edit_merge_lchain(mode, NULL, NULL, NULL,
									NULL, NULL, NULL);
					else if (same(PendingMode, "NODES"))
						(void) edit_nodes_lchain(mode, NULL, NULL);

					break;
				}
			break;
		}
		break;

	case MODULE_SCRATCH:
		switch (PendingEditor)
		{
		case EDITOR_EDIT:
			if (same(PendingMode, "DRAW"))
				(void) edit_draw_generic_line(mode, NullCal);
			else if (same(PendingMode, "TEXT"))
				(void) edit_place_generic_label(mode, NullCal);
			else if (same(PendingMode, "DISTANCE")
					|| same(PendingMode, "PRESET_DISTANCE"))
				(void) edit_draw_generic_span(mode, NullCal, NullCal, NullCal,
																		0.0);
			else if (same(PendingMode, "SELECT"))
				(void) edit_select_generic_feature(mode);
			break;
		}
		break;

	case MODULE_LINK:
			if (same(PendingMode, "FORWARD"))
				switch (CurrEditor)
					{
					case FpaCnoMacro:		link_master(mode, TRUE);
											break;
					case FpaC_VECTOR:
					case FpaC_CONTINUOUS:	link_spline(mode, TRUE);
											break;
					case FpaC_DISCRETE:
					case FpaC_WIND:			link_area(mode, TRUE);
											break;
					case FpaC_LINE:			link_line(mode, TRUE);
											break;
					}

			else if (same(PendingMode, "BACKWARD"))
				switch (CurrEditor)
					{
					case FpaCnoMacro:		link_master(mode, FALSE);
											break;
					case FpaC_VECTOR:
					case FpaC_CONTINUOUS:	link_spline(mode, FALSE);
											break;
					case FpaC_DISCRETE:
					case FpaC_WIND:			link_area(mode, FALSE);
											break;
					case FpaC_LINE:			link_line(mode, FALSE);
											break;
					}

			else if (same(PendingMode, "MOVE"))
				switch (CurrEditor)
					{
					case FpaCnoMacro:		(void) mvlink_master(mode);
											break;
					case FpaC_VECTOR:
					case FpaC_CONTINUOUS:	(void) mvlink_spline(mode);
											break;
					case FpaC_DISCRETE:
					case FpaC_WIND:			(void) mvlink_area(mode);
											break;
					case FpaC_LINE:			(void) mvlink_line(mode);
											break;
					}

			else if (same(PendingMode, "MERGE"))
				switch (CurrEditor)
					{
					case FpaCnoMacro:		(void) mrglink_master(mode,
													NULL, NULL, NULL, NULL);
											break;
					case FpaC_VECTOR:
					case FpaC_CONTINUOUS:	(void) mrglink_spline(mode,
													NULL, NULL, NULL, NULL);
											break;
					case FpaC_DISCRETE:
					case FpaC_WIND:			(void) mrglink_area(mode,
													NULL, NULL, NULL, NULL);
											break;
					case FpaC_LINE:			(void) mrglink_line(mode,
													NULL, NULL, NULL, NULL);
											break;
					}

			else if (same(PendingMode, "DELINK"))
				switch (CurrEditor)
					{
					case FpaCnoMacro:		(void) delink_master(mode, NULL);
											break;
					case FpaC_VECTOR:
					case FpaC_CONTINUOUS:	(void) delink_spline(mode, NULL);
											break;
					case FpaC_DISCRETE:
					case FpaC_WIND:			(void) delink_area(mode, NULL);
											break;
					case FpaC_LINE:			(void) delink_line(mode, NULL);
											break;
					}
	}

	ignore_partial();
	}

/***********************************************************************
*                                                                      *
*     c o p y _ p o s t e d    - Indicate whether something has been   *
*                                selected for pasting.                 *
*                                                                      *
*     p o s t _ . . . _ c o p y        - Save selected objects for     *
*                                        pasting.                      *
*                                                                      *
*     p a s t e _ . . . _ c o p y      - Return selected objects for   *
*                                        pasting.                      *
*                                                                      *
*     r e l e a s e _ . . . _ c o p y  - Remove selected objects.      *
*                                                                      *
***********************************************************************/

static	STRING	CopyElemS     = FpaCnone;
static	STRING	CopyLevelS    = FpaCnone;
static	SURFACE	CopySfc       = NullSfc;
static	CURVE	CopyBound     = NullCurve;
static	int		NumSfcLabs    = 0;
static	SPOT	*SfcLabels    = NullSpotList;

static	STRING	CopyElemA     = FpaCnone;
static	STRING	CopyLevelA    = FpaCnone;
static	int		NumCopyAreas  = 0;
static	AREA	*CopyAreas    = NullAreaList;
static	int		NumAreaLabs   = 0;
static	SPOT	*AreaLabels   = NullSpotList;
static	int		*AreaIds      = NullInt;

static	STRING	CopyElemL      = FpaCnone;
static	STRING	CopyLevelL     = FpaCnone;
static	int		NumCopyCurves  = 0;
static	CURVE	*CopyCurves    = NullCurveList;
static	int		NumCurveLabs   = 0;
static	SPOT	*CurveLabels   = NullSpotList;
static	int		*CurveIds      = NullInt;

static	STRING	CopyElemP      = FpaCnone;
static	STRING	CopyLevelP     = FpaCnone;
static	int		NumCopyPoints  = 0;
static	SPOT	*CopyPoints    = NullSpotList;

static	STRING	CopyElemC      = FpaCnone;
static	STRING	CopyLevelC     = FpaCnone;
static	int		NumCopyLchains = 0;
static	LCHAIN	*CopyLchains   = NullLchainList;

static	STRING	CopyElemN      = FpaCnone;
static	STRING	CopyLevelN     = FpaCnone;
static	LMEMBER	CopyLtype      = LchainUnknown;
static	LNODE	CopyLnode      = NullLnode;

/**********************************************************************/

LOGICAL	copy_posted

	(
	STRING	which,
	STRING	elem,
	STRING	level
	)

	{

	if (blank(which))
		return FALSE;
	if (same(which, "surface"))
		return (LOGICAL) (same(elem, CopyElemS) && same(level, CopyLevelS));
	if (same(which, "areas"))
		return (LOGICAL) (same(elem, CopyElemA) && same(level, CopyLevelA));
	if (same(which, "curves"))
		return (LOGICAL) (same(elem, CopyElemL) && same(level, CopyLevelL));
	if (same(which, "points") )
		return (LOGICAL) (same(elem, CopyElemP) && same(level, CopyLevelP));
	if (same(which, "lchains") )
		return (LOGICAL) (same(elem, CopyElemC) && same(level, CopyLevelC));
	if (same(which, "lnodes") )
		return (LOGICAL) (same(elem, CopyElemN) && same(level, CopyLevelN));

	return FALSE;
	}

/**********************************************************************/

void	post_spline_copy

	(
	STRING	elem,
	STRING	level,
	SURFACE	sfc,
	CURVE	bound,
	int		nlabs,
	SPOT	*labels
	)

	{
	int		i;

	release_spline_copy();
	if (!sfc)   return;
	if (!bound) return;

	CopyElemS   = elem;
	CopyLevelS  = level;
	CopySfc     = copy_surface(sfc, TRUE);
	CopyBound   = copy_curve(bound);
	NumSfcLabs  = nlabs;
	SfcLabels   = INITMEM(SPOT, nlabs);
	for (i=0; i<nlabs; i++)
		SfcLabels[i] = copy_spot(labels[i]);

	edit_can_paste(TRUE);
	}

/**********************************************************************/

void	post_area_copy

	(
	STRING	elem,
	STRING	level,
	int		num,
	AREA	*areas,
	int		nlabs,
	SPOT	*labels,
	int		*ids
	)

	{
	int		i;

	release_area_copy();
	if (num <= 0) return;
	if (!areas)   return;

	CopyElemA    = elem;
	CopyLevelA   = level;
	NumCopyAreas = num;
	CopyAreas    = INITMEM(AREA, num);
	for (i=0; i<num; i++)
		CopyAreas[i] = copy_area(areas[i], TRUE);
	NumAreaLabs  = nlabs;
	AreaLabels   = INITMEM(SPOT, nlabs);
	AreaIds      = INITMEM(int,  nlabs);
	for (i=0; i<nlabs; i++)
		{
		AreaLabels[i] = copy_spot(labels[i]);
		AreaIds[i]    = ids[i];
		}

	edit_can_paste(TRUE);
	}

/**********************************************************************/

void	post_line_copy

	(
	STRING	elem,
	STRING	level,
	int		num,
	CURVE	*curves,
	int		nlabs,
	SPOT	*labels,
	int		*ids
	)

	{
	int		i;

	release_line_copy();
	if (num <= 0) return;
	if (!curves)  return;

	CopyElemL     = elem;
	CopyLevelL    = level;
	NumCopyCurves = num;
	CopyCurves    = INITMEM(CURVE, num);
	for (i=0; i<num; i++)
		CopyCurves[i] = copy_curve(curves[i]);
	NumCurveLabs  = nlabs;
	CurveLabels   = INITMEM(SPOT, nlabs);
	CurveIds      = INITMEM(int,  nlabs);
	for (i=0; i<nlabs; i++)
		{
		CurveLabels[i] = copy_spot(labels[i]);
		CurveIds[i]    = ids[i];
		}

	edit_can_paste(TRUE);
	}

/**********************************************************************/

void	post_point_copy

	(
	STRING	elem,
	STRING	level,
	int		num,
	SPOT	*spots
	)

	{
	int		i;

	release_point_copy();
	if (num <= 0) return;
	if (!spots)  return;

	CopyElemP     = elem;
	CopyLevelP    = level;
	NumCopyPoints = num;
	CopyPoints    = INITMEM(SPOT, num);
	for (i=0; i<num; i++)
		CopyPoints[i] = copy_spot(spots[i]);

	edit_can_paste(TRUE);
	}

/**********************************************************************/

void	post_lchain_copy

	(
	STRING	elem,
	STRING	level,
	int		num,
	LCHAIN	*lchains
	)

	{
	int		i;

	release_lchain_copy();
	if (num <= 0) return;
	if (!lchains) return;

	CopyElemC      = elem;
	CopyLevelC     = level;
	NumCopyLchains = num;
	CopyLchains    = INITMEM(LCHAIN, num);
	for (i=0; i<num; i++)
		CopyLchains[i] = copy_lchain(lchains[i]);

	edit_can_paste(TRUE);
	}

/**********************************************************************/

void	post_lchain_node_copy

	(
	STRING	elem,
	STRING	level,
	LMEMBER	ltype,
	LNODE	lnode
	)

	{

	release_lchain_node_copy();
	if (ltype != LchainNode
			&& ltype != LchainControl
			&& ltype != LchainFloating) return;
	if (!lnode) return;

	CopyElemN  = elem;
	CopyLevelN = level;
	CopyLtype  = ltype;
	CopyLnode  = copy_lnode(lnode);

	edit_can_paste(TRUE);
	}

/**********************************************************************/

void	paste_spline_copy

	(
	STRING	elem,
	STRING	level,
	SURFACE	*sfc,
	CURVE	*bound,
	int		*nlabs,
	SPOT	**labels
	)

	{
	int		i;
	SPOT	*cspots = NullSpotList;

	if (!same(elem, CopyElemS) || !same(level, CopyLevelS))
		{
		if (NotNull(sfc))    *sfc    = NullSfc;
		if (NotNull(bound))  *bound  = NullCurve;
		if (NotNull(nlabs))  *nlabs  = 0;
		if (NotNull(labels)) *labels = NullSpotList;
		return;
		}

	if (NotNull(sfc) && NotNull(bound))
		{
		*sfc   = copy_surface(CopySfc, TRUE);
		*bound = copy_curve(CopyBound);
		}
	if (NotNull(nlabs) && NotNull(labels))
		{
		cspots = INITMEM(SPOT, NumSfcLabs);
		for (i=0; i<NumSfcLabs; i++)
			{
			cspots[i] = copy_spot(SfcLabels[i]);
			}
		*nlabs  = NumSfcLabs;
		*labels = cspots;
		}
	}

/**********************************************************************/

void	paste_area_copy

	(
	STRING	elem,
	STRING	level,
	int		*num,
	AREA	**areas,
	int		*nlabs,
	SPOT	**labels,
	int		**ids
	)

	{
	int		i;
	AREA	*careas = NullAreaList;
	SPOT	*cspots = NullSpotList;
	int		*cids   = NullInt;

	if (!same(elem, CopyElemA) || !same(level, CopyLevelA))
		{
		if (NotNull(num))    *num    = 0;
		if (NotNull(areas))  *areas  = NullAreaList;
		if (NotNull(nlabs))  *nlabs  = 0;
		if (NotNull(labels)) *labels = NullSpotList;
		if (NotNull(ids))    *ids    = NullInt;
		return;
		}

	if (NotNull(num) && NotNull(areas))
		{
		careas = INITMEM(AREA, NumCopyAreas);
		for (i=0; i<NumCopyAreas; i++)
			{
			careas[i] = copy_area(CopyAreas[i], TRUE);
			}
		*num   = NumCopyAreas;
		*areas = careas;
		}
	if (NotNull(nlabs) && NotNull(labels) && NotNull(ids))
		{
		cspots = INITMEM(SPOT, NumAreaLabs);
		cids   = INITMEM(int,  NumAreaLabs);
		for (i=0; i<NumAreaLabs; i++)
			{
			cspots[i] = copy_spot(AreaLabels[i]);
			cids[i]   = AreaIds[i];
			}
		*nlabs  = NumAreaLabs;
		*labels = cspots;
		*ids    = cids;
		}
	}

/**********************************************************************/

void	paste_line_copy

	(
	STRING	elem,
	STRING	level,
	int		*num,
	CURVE	**curves,
	int		*nlabs,
	SPOT	**labels,
	int		**ids
	)

	{
	int		i;
	CURVE	*ccurves = NullCurveList;
	SPOT	*cspots = NullSpotList;
	int		*cids   = NullInt;

	if (!same(elem, CopyElemL) || !same(level, CopyLevelL))
		{
		if (NotNull(num))    *num    = 0;
		if (NotNull(curves)) *curves = NullCurveList;
		if (NotNull(nlabs))  *nlabs  = 0;
		if (NotNull(labels)) *labels = NullSpotList;
		if (NotNull(ids))    *ids    = NullInt;
		return;
		}

	if (NotNull(num) && NotNull(curves))
		{
		ccurves = INITMEM(CURVE, NumCopyCurves);
		for (i=0; i<NumCopyCurves; i++)
			ccurves[i] = copy_curve(CopyCurves[i]);
		*num    = NumCopyCurves;
		*curves = ccurves;
		}
	if (NotNull(nlabs) && NotNull(labels) && NotNull(ids))
		{
		cspots = INITMEM(SPOT, NumCurveLabs);
		cids   = INITMEM(int,  NumCurveLabs);
		for (i=0; i<NumCurveLabs; i++)
			{
			cspots[i] = copy_spot(CurveLabels[i]);
			cids[i]   = CurveIds[i];
			}
		*nlabs  = NumCurveLabs;
		*labels = cspots;
		*ids    = cids;
		}
	}

/**********************************************************************/

void	paste_point_copy

	(
	STRING	elem,
	STRING	level,
	int		*num,
	SPOT	**spots
	)

	{
	int		i;
	SPOT	*cspots = NullSpotList;

	if (!num || !spots) return;

	if (!same(elem, CopyElemP) || !same(level, CopyLevelP))
		{
		*num    = 0;
		*spots = NullSpotList;
		return;
		}

	cspots = INITMEM(SPOT, NumCopyPoints);
	for (i=0; i<NumCopyPoints; i++)
		cspots[i] = copy_spot(CopyPoints[i]);
	*num   = NumCopyPoints;
	*spots = cspots;
	}

/**********************************************************************/

void	paste_lchain_copy

	(
	STRING	elem,
	STRING	level,
	int		*num,
	LCHAIN	**lchains
	)

	{
	int		i;
	LCHAIN	*clchains = NullLchainList;

	if (!same(elem, CopyElemC) || !same(level, CopyLevelC))
		{
		if (NotNull(num))     *num     = 0;
		if (NotNull(lchains)) *lchains = NullLchainList;
		return;
		}

	if (NotNull(num) && NotNull(lchains))
		{
		clchains = INITMEM(LCHAIN, NumCopyLchains);
		for (i=0; i<NumCopyLchains; i++)
			clchains[i] = copy_lchain(CopyLchains[i]);
		*num     = NumCopyLchains;
		*lchains = clchains;
		}
	}

/**********************************************************************/

void	paste_lchain_node_copy

	(
	STRING	elem,
	STRING	level,
	LMEMBER	*ltype,
	LNODE	*lnode
	)

	{

	if (!same(elem, CopyElemN) || !same(level, CopyLevelN))
		{
		if (NotNull(ltype)) *ltype = LchainUnknown;
		if (NotNull(lnode)) *lnode = NullLnode;
		return;
		}

	if (NotNull(ltype)) *ltype = CopyLtype;
	if (NotNull(lnode)) *lnode = copy_lnode(CopyLnode);
	}

/**********************************************************************/

void	release_spline_copy(void)

	{
	int		i;

	for (i=0; i<NumSfcLabs; i++)
		SfcLabels[i] = destroy_spot(SfcLabels[i]);
	FREEMEM(SfcLabels);
	NumSfcLabs = 0;

	CopySfc   = destroy_surface(CopySfc);
	CopyBound = destroy_curve(CopyBound);

	CopyElemS   = FpaCnone;
	CopyLevelS  = FpaCnone;
	}

/**********************************************************************/

void	release_area_copy(void)

	{
	int		i;

	for (i=0; i<NumAreaLabs; i++)
		AreaLabels[i] = destroy_spot(AreaLabels[i]);
	FREEMEM(AreaLabels);
	NumAreaLabs = 0;

	for (i=0; i<NumCopyAreas; i++)
		CopyAreas[i] = destroy_area(CopyAreas[i]);
	FREEMEM(CopyAreas);
	NumCopyAreas = 0;

	CopyElemA   = FpaCnone;
	CopyLevelA  = FpaCnone;
	}

/**********************************************************************/

void	release_line_copy(void)

	{
	int		i;

	for (i=0; i<NumCurveLabs; i++)
		CurveLabels[i] = destroy_spot(CurveLabels[i]);
	FREEMEM(CurveLabels);
	NumCurveLabs = 0;

	for (i=0; i<NumCopyCurves; i++)
		CopyCurves[i] = destroy_curve(CopyCurves[i]);
	FREEMEM(CopyCurves);
	NumCopyCurves = 0;

	CopyElemL   = FpaCnone;
	CopyLevelL  = FpaCnone;
	}

/**********************************************************************/

void	release_point_copy(void)

	{
	int		i;

	for (i=0; i<NumCopyPoints; i++)
		CopyPoints[i] = destroy_spot(CopyPoints[i]);
	FREEMEM(CopyPoints);
	NumCopyPoints = 0;

	CopyElemP   = FpaCnone;
	CopyLevelP  = FpaCnone;
	}

/**********************************************************************/

void	release_lchain_copy(void)

	{
	int		i;

	for (i=0; i<NumCopyLchains; i++)
		CopyLchains[i] = destroy_lchain(CopyLchains[i]);
	FREEMEM(CopyLchains);
	NumCopyLchains = 0;

	CopyElemC   = FpaCnone;
	CopyLevelC  = FpaCnone;
	}

/**********************************************************************/

void	release_lchain_node_copy(void)

	{

	CopyLnode = destroy_lnode(CopyLnode);
	CopyLtype = LchainUnknown;

	CopyElemC   = FpaCnone;
	CopyLevelC  = FpaCnone;
	}


/***********************************************************************
*                                                                      *
*     d r a w n _ o u t l i n e _ p o s t e d                          *
*                  - Indicate whether drawn outline has been saved.    *
*                                                                      *
*     p o s t _ d r a w n _ o u t l i n e                              *
*                  - Save drawn outline for move/merge.                *
*                                                                      *
*     r e t r i e v e _ d r a w n _ o u t l i n e                      *
*                  - Retrieve last drawn outline from move/merge.      *
*                                                                      *
*     r e t r i e v e _ n a m e d _ o u t l i n e                      *
*                  - Retrieve named outline for move/merge.            *
*                                                                      *
*     m o v e d _ o u t l i n e _ p o s t e d                          *
*                  - Indicate whether moved outline has been saved.    *
*                                                                      *
*     p o s t _ m o v e d _ o u t l i n e                              *
*                  - Save moved outline for move/merge.                *
*                                                                      *
*     r e t r i e v e _ m o v e d _ o u t l i n e                      *
*                  - Retrieve last moved outline from move/merge.      *
*                                                                      *
*     s t o m p _ o u t l i n e _ p o s t e d                          *
*                  - Indicate whether stomp outline has been saved.    *
*                                                                      *
*     p o s t _ s t o m p _ o u t l i n e                              *
*                  - Save last stomp outline.                          *
*                                                                      *
*     r e t r i e v e _ s t o m p _ o u t l i n e                      *
*                  - Retrieve last stomp outline.                      *
*                                                                      *
*     d r a w n _ h o l e _ p o s t e d                                *
*                  - Indicate whether drawn hole has been saved.       *
*                                                                      *
*     p o s t _ d r a w n _ h o l e                                    *
*                  - Save drawn hole for draw hole.                    *
*                                                                      *
*     r e t r i e v e _ d r a w n _ h o l e                            *
*                  - Retrieve last drawn hole from draw hole.          *
*                                                                      *
*     r e t r i e v e _ n a m e d _ h o l e s                          *
*                  - Retrieve named holes for draw hole.               *
*                                                                      *
***********************************************************************/

static	CURVE	DrawnOutline = NullCurve;
static	CURVE	MovedOutline = NullCurve;
static	CURVE	StompOutline = NullCurve;
static	CURVE	DrawnHole    = NullCurve;

/**********************************************************************/

LOGICAL	drawn_outline_posted

	(
	)

	{

	if (IsNull(DrawnOutline))           return FALSE;
	if (IsNull(DrawnOutline->line))     return FALSE;
	if (DrawnOutline->line->numpts < 3) return FALSE;

	return TRUE;
	}

/**********************************************************************/

void	post_drawn_outline

	(
	CURVE	curve
	)

	{
	if (IsNull(curve))           return;
	if (IsNull(curve->line))     return;
	if (curve->line->numpts < 3) return;

	if (NotNull(DrawnOutline)) (void) destroy_curve(DrawnOutline);
	else                       edit_drawn_outline_posted();

	DrawnOutline = copy_curve(curve);
	}

/**********************************************************************/

CURVE	retrieve_drawn_outline

	(
	)

	{
	return (DrawnOutline)? copy_curve(DrawnOutline): NullCurve;
	}

/**********************************************************************/

CURVE	retrieve_named_outline

	(
	STRING	name
	)

	{
	int			ifld, imem;
	STRING		filename;
	METAFILE	meta;
	FIELD		fld;
	SET			set;
	BOX			box;
	CURVE		curve;
	AREA		area;
	CURVE		outline = NullCurve;

	/* Get the file name containing the named outline */
	if (blank(name))     return NullCurve;
	filename = background_file(name);
	if (blank(filename)) return NullCurve;

	/* Read the metafile containing the named outline */
	meta = read_metafile(filename, MapProj);

	/* Return the first curve found */
	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = meta->fields[ifld];
		if (!fld) continue;

		/* Search each field for a curve */
		switch (fld->ftype)
			{
			case FtypeSet:

				set = fld->data.set;

				/* Extract curve from "curve" type sets */
				if (same(set->type, "curve"))

					{
					/* Strip out curves outside the map */
					box.left   = 0;
					box.right  = MapProj->definition.xlen;
					box.bottom = 0;
					box.top    = MapProj->definition.ylen;
					strip_set(set, &box);
					if (set->num <= 0) break;

					/* Copy the first curve found */
					for (imem = 0; imem<set->num; imem++)
						{
						curve = (CURVE) set->list[imem];
						if (!curve)                  continue;
						if (!curve->line)            continue;
						if (curve->line->numpts < 3) continue;
						outline = copy_curve(curve);
						close_line(outline->line);
						break;
						}
					}

				/* Extract area boundary from "area" type sets */
				else if (same(set->type, "area"))

					{
					/* Strip out areas outside the map */
					box.left   = 0;
					box.right  = MapProj->definition.xlen;
					box.bottom = 0;
					box.top    = MapProj->definition.ylen;
					strip_set(set, &box);
					if (set->num <= 0) break;

					/* Copy the first area boundary found */
					for (imem = 0; imem<set->num; imem++)
						{
						area = (AREA) set->list[imem];
						if (!area)                             continue;
						if (!area->bound)                      continue;
						if (!area->bound->boundary)            continue;
						if (area->bound->boundary->numpts < 3) continue;
						outline = create_curve("", "", "");
						add_line_to_curve(outline, area->bound->boundary);
						break;
						}
					}
				break;;

			default:
				break;
			}
		}

	/* Return what we found */
	(void) destroy_metafile(meta);
	return outline;
	}

/**********************************************************************/

LOGICAL	moved_outline_posted

	(
	)

	{

	if (IsNull(MovedOutline))           return FALSE;
	if (IsNull(MovedOutline->line))     return FALSE;
	if (MovedOutline->line->numpts < 3) return FALSE;

	return TRUE;
	}

/**********************************************************************/

void	post_moved_outline

	(
	CURVE	curve
	)

	{
	if (IsNull(curve))           return;
	if (IsNull(curve->line))     return;
	if (curve->line->numpts < 3) return;

	if (NotNull(MovedOutline)) (void) destroy_curve(MovedOutline);
	else                       edit_moved_outline_posted();

	MovedOutline = copy_curve(curve);
	}

/**********************************************************************/

CURVE	retrieve_moved_outline

	(
	)

	{
	return (MovedOutline)? copy_curve(MovedOutline): NullCurve;
	}

/**********************************************************************/

LOGICAL	stomp_outline_posted

	(
	)

	{

	if (IsNull(StompOutline))           return FALSE;
	if (IsNull(StompOutline->line))     return FALSE;
	if (StompOutline->line->numpts < 3) return FALSE;

	return TRUE;
	}

/**********************************************************************/

void	post_stomp_outline

	(
	CURVE	curve
	)

	{
	if (IsNull(curve))           return;
	if (IsNull(curve->line))     return;
	if (curve->line->numpts < 3) return;

	if (NotNull(StompOutline)) (void) destroy_curve(StompOutline);
	else                       edit_stomp_outline_posted();

	StompOutline = copy_curve(curve);
	}

/**********************************************************************/

CURVE	retrieve_stomp_outline

	(
	)

	{
	return (StompOutline)? copy_curve(StompOutline): NullCurve;
	}

/**********************************************************************/

LOGICAL	drawn_hole_posted

	(
	)

	{

	if (IsNull(DrawnHole))           return FALSE;
	if (IsNull(DrawnHole->line))     return FALSE;
	if (DrawnHole->line->numpts < 3) return FALSE;

	return TRUE;
	}

/**********************************************************************/

void	post_drawn_hole

	(
	CURVE	hole
	)

	{
	if (IsNull(hole))           return;
	if (IsNull(hole->line))     return;
	if (hole->line->numpts < 3) return;

	if (NotNull(DrawnHole)) (void) destroy_curve(DrawnHole);
	else                       edit_drawn_hole_posted();

	DrawnHole = copy_curve(hole);
	}

/**********************************************************************/

CURVE	retrieve_drawn_hole

	(
	)

	{
	return (DrawnHole)? copy_curve(DrawnHole): NullCurve;
	}

/**********************************************************************/

int		retrieve_named_holes

	(
	STRING	name,
	CURVE	**holes
	)

	{
	int			ifld, imem, numhole, nh;
	STRING		filename;
	METAFILE	meta;
	FIELD		fld;
	SET			set;
	BOX			box;
	CURVE		curve;
	AREA		area;
	CURVE		*curves = NullCurveList;

	/* Initialize return variables */
	numhole = 0;
	if (NotNull(holes)) *holes = NullCurveList;

	/* Get the file name containing the named holes */
	if (blank(name))     return numhole;
	filename = background_file(name);
	if (blank(filename)) return numhole;

	/* Read the metafile containing the named holes */
	meta = read_metafile(filename, MapProj);

	/* Return all curves found */
	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = meta->fields[ifld];
		if (!fld) continue;

		/* Search each field for a curve or an area boundary */
		switch (fld->ftype)
			{
			case FtypeSet:

				set = fld->data.set;

				/* Extract curves from "curve" type sets */
				if (same(set->type, "curve"))

					{
					/* Strip out curves outside the map */
					box.left   = 0;
					box.right  = MapProj->definition.xlen;
					box.bottom = 0;
					box.top    = MapProj->definition.ylen;
					strip_set(set, &box);
					if (set->num <= 0) break;

					/* Copy the each curve found */
					for (imem = 0; imem<set->num; imem++)
						{
						curve = (CURVE) set->list[imem];
						if (!curve)                  continue;
						if (!curve->line)            continue;
						if (curve->line->numpts < 3) continue;
						nh = numhole++;
						curves = GETMEM(curves, CURVE, numhole);
						curves[nh] = copy_curve(curve);
						close_line(curves[nh]->line);
						}
					}

				/* Extract area boundaries from "area" type sets */
				else if (same(set->type, "area"))

					{
					/* Strip out areas outside the map */
					box.left   = 0;
					box.right  = MapProj->definition.xlen;
					box.bottom = 0;
					box.top    = MapProj->definition.ylen;
					strip_set(set, &box);
					if (set->num <= 0) break;

					/* Copy the each area boundary found */
					for (imem = 0; imem<set->num; imem++)
						{
						area = (AREA) set->list[imem];
						if (!area)                             continue;
						if (!area->bound)                      continue;
						if (!area->bound->boundary)            continue;
						if (area->bound->boundary->numpts < 3) continue;
						nh = numhole++;
						curves = GETMEM(curves, CURVE, numhole);
						curves[nh] = create_curve("", "", "");
						add_line_to_curve(curves[nh], area->bound->boundary);
						break;
						}
					}
				break;;

			default:
				break;
			}
		}

	/* Return what we found */
	(void) destroy_metafile(meta);
	if (NotNull(holes)) *holes = curves;
	return numhole;
	}
