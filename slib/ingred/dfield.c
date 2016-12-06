/***********************************************************************
*                                                                      *
*     d f i e l d . c                                                  *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all manipulations of the depiction field information     *
*     list.                                                            *
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

#undef DEBUG_DFIELD

/* Units cross-referenced in configuration file */
static	const	STRING	Hrs = "hr";

/***********************************************************************
*                                                                      *
*     p i c k _ a c t i v e _ g r o u p                                *
*     p i c k _ a c t i v e _ d f i e l d                              *
*                                                                      *
***********************************************************************/

LOGICAL	pick_active_group

	(
	STRING	new_group
	)

	{
	LOGICAL	new;
	int		idfld;
	DFLIST	*dfld;

	/* Turn off previous field */
	pr_info("Fields", "Group: %s.\n", SafeStr(new_group));
	ActiveDfld  = NULL;
	CurrGroup   = NULL;
	CurrElement = NULL;
	CurrLevel   = NULL;
	CurrEditor  = FpaCnoMacro;
	new         = FALSE;

	/* Make sure the new group is valid */
	if (blank(new_group) || same(new_group, "NONE"))
		{
		CurrGroup = "NONE";
		new = TRUE;
		}
	else if (identify_fields_by_group(new_group, NULL) > 0)
		{
		CurrGroup = "NONE";
		new = TRUE;
		for (idfld=0; idfld<NumDfld; idfld++)
			{
			dfld = DfldList + idfld;
			if (same(dfld->group, new_group))
				{
				CurrGroup = dfld->group;
				break;
				}
			}
		}
	else
	    {
	    pr_warning("Fields", "Invalid group: %s.\n", new_group);
	    }

	/* Reset the states of each field since the active group has changed */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		set_dfield_visibility(dfld);
		}
	(void) present_depiction(FALSE);

	return new;
	}

/**********************************************************************/

LOGICAL	pick_active_dfield

	(
	STRING	new_elem,
	STRING	new_level
	)

	{
	LOGICAL	new;
	int		idfld;
	DFLIST	*dfld;

	/* Turn off previous field */
	pr_info("Fields", "Field: %s %s.\n", SafeStr(new_elem), SafeStr(new_level));
	ActiveDfld  = NULL;
	CurrElement = NULL;
	CurrLevel   = NULL;
	CurrEditor  = FpaCnoMacro;
	new         = FALSE;

	/* Reset special fields after using the secondary time bar */
	if (EditTime != ViewTime)
		{
		/* EditTime = ViewTime; */
		(void) pick_depiction();
		}

	/* Make sure the new field is valid */
	if (blank(new_elem) || same(new_elem, "NONE"))
		{
		CurrElement = "NONE";
		new = TRUE;
		}
	else if (same(new_elem, "MASTER_LINK"))
		{
		/* Find pointer to the field in master link buffer */
		/* new_level contains the group name */
		ActiveDfld = find_master_dfield(new_level);
		if (ActiveDfld)
			{
			/* "Pick" the new field */
			CurrElement = "MASTER_LINK";
			CurrLevel   = ActiveDfld->level;
			CurrEditor  = ActiveDfld->editor;
			CurrGroup   = ActiveDfld->group;
			new         = TRUE;
			}
		}
	else if (identify_field(new_elem, new_level))
		{
		/* Find pointer to the field in field buffer */
		ActiveDfld = find_dfield(new_elem, new_level);
		if (ActiveDfld)
			{
			/* "Pick" the new field */
			CurrElement = ActiveDfld->element;
			CurrLevel   = ActiveDfld->level;
			CurrEditor  = ActiveDfld->editor;
			CurrGroup   = ActiveDfld->group;
			new         = TRUE;
			}
		}
	else
	    {
	    pr_warning("Fields", "Invalid field: %s %s.\n", new_elem, new_level);
	    }

	/* Reset the states of each field in case the active group has changed */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		set_dfield_visibility(dfld);
		}
	(void) present_depiction(FALSE);

	return new;
	}

/***********************************************************************
*                                                                      *
*     s e t _ g r o u p _ s t a t e                                    *
*     s e t _ d f i e l d _ s t a t e                                  *
*     s e t _ d f i e l d _ v i s i b i l i t y                        *
*     d f i e l d _ s h o w n                                          *
*                                                                      *
***********************************************************************/

LOGICAL	set_group_state

	(
	STRING	group,
	STRING	state
	)

	{
	int		idfld;
	LOGICAL	showgrp;
	DFLIST	*dfld;

	if (blank(group)) return FALSE;
	if (blank(state)) return FALSE;

	/* Do we turn it on or off ? */
	if (same(state, "OFF"))     showgrp = FALSE;
	else if (same(state, "ON")) showgrp = TRUE;
	else                        return FALSE;

	/* Set the states of each field in the given group */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		if (!same(dfld->group, group)) continue;

		dfld->showgrp = showgrp;
		set_dfield_visibility(dfld);
		}

	/* Re-display if the active frame is affected */
	if (!DepictShown) return TRUE;
	if (ViewTime < 0) return TRUE;
	present_all();
	return TRUE;
	}

/**********************************************************************/

LOGICAL	set_dfield_state

	(
	STRING	elem,
	STRING	levl,
	STRING	state
	)

	{
	SHOW	show;
	DFLIST	*dfld;

	if (blank(elem))  return FALSE;
	if (blank(levl))  return FALSE;
	if (blank(state)) return FALSE;

	/* Find the given field */
	dfld = find_dfield(elem, levl);
	if (!dfld) return FALSE;

	/* Do we turn it on or off ? */
	if (same(state, "ON_WHEN_GROUP_VISIBLE")) show = SHOW_ACTIVE_GRP;
	else if (same(state, "OFF"))              show = SHOW_ACTIVE_FLD;
	else if (same(state, "ON"))               show = SHOW_ALWAYS;
	else                                      return FALSE;

	dfld->showdep = show;
	set_dfield_visibility(dfld);

	/* Re-display if the active frame is affected */
	if (!DepictShown) return TRUE;
	if (ViewTime < 0) return TRUE;

	/* Wait for SHOW command */
	DepictVis = TRUE;
	/* present_all(); */
	return TRUE;
	}

/**********************************************************************/

void	set_dfield_visibility

	(
	DFLIST	*dfld
	)

	{
	LOGICAL		shown;

	if (!dfld) return;

	/* Do we need to show or hide this field? */
	shown = (MovieShown)? dfld->showmov: dfield_shown(dfld);
	shown = ((dfld == ActiveDfld) && EditShown)? FALSE: shown;

	define_dn_vis(dfld->dn, shown);
	}

/**********************************************************************/

LOGICAL	dfield_shown

	(
	DFLIST	*dfld
	)

	{
	if (!dfld) return FALSE;

	/* Do we need to show or hide this field? */
	switch (dfld->showdep)
		{
		case SHOW_ALWAYS:		return TRUE;

		case SHOW_ACTIVE_GRP:	if (dfld->showgrp)                return TRUE;
								if (same(dfld->group, CurrGroup)) return TRUE;

		default:				return FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*     i n i t _ d f i e l d s                                          *
*                                                                      *
***********************************************************************/

void	init_dfields(void)

	{
	int		idfld, nhrs, shrs, ehrs, vhrs;
	STRING	elem, ent, levl, grp, bsub, bval, blab;
	LOGICAL	doedit, dohilo, dolink, reorder, local;
	int		ftyp, wind, tdep;
	double	nhour, shour, ehour;
	DFLIST	*dfld=0;
	int		labpos = 0;

	FpaConfigFieldStruct			**fdefs, *fdef;
	FpaConfigElementEditorStruct	*ed;
	FpaConfigElementTimeDepStruct	*td;
	FpaConfigGroupStruct			**gdefs;
	FLD_DESCRIPT					fdes;

	/* Build dfield list for the required fields first */
	NumDfld  = depict_field_list(&fdefs);
	DfldList = GETMEM(DfldList, DFLIST, NumDfld);
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		fdef = fdefs[idfld];
		elem = fdef->element->name;
		levl = fdef->level->name;

		(void) init_fld_descript(&fdes);
		(void) set_fld_descript(&fdes,
				FpaF_ELEMENT,	fdef->element,
				FpaF_LEVEL,		fdef->level,
				FpaF_END_OF_LIST);

		grp  = fdef->group->name;
		wind = fdef->element->elem_detail->wd_class;

		/* Interpret editor info */
		ed   = fdef->element->elem_detail->editor;
		ftyp = fdef->element->fld_type;
		ent  = entity_from_field_type(ftyp);
		bsub = NULL;
		bval = NULL;
		blab = NULL;
		switch (ftyp)
			{
			case FpaC_VECTOR:		doedit  = (ed)? TRUE: FALSE;
									dohilo  = (ed)? ed->type.vector->hilo:
												FALSE;
									dolink  = TRUE;
									reorder = FALSE;
									break;

			case FpaC_CONTINUOUS:	doedit  = (ed)? TRUE: FALSE;
									dohilo  = (ed)? ed->type.continuous->hilo:
												FALSE;
									dolink  = TRUE;
									reorder = FALSE;
									break;

			case FpaC_DISCRETE:		doedit  = (ed)? TRUE: FALSE;
									dohilo  = FALSE;
									dolink  = TRUE;
									reorder = (ed)?
												ed->type.discrete->display_order:
												FALSE;
									break;
									/*
									>>> get default attr list from config
									>>> need elem_detail->
									*/

			case FpaC_WIND:			doedit  = (ed)? TRUE: FALSE;
									dohilo  = FALSE;
									dolink  = TRUE;
									reorder = (ed)?
												ed->type.wind->display_order:
												FALSE;
									break;

			case FpaC_LINE:			doedit  = (ed)? TRUE: FALSE;
									dohilo  = FALSE;
									dolink  = TRUE;
									reorder = FALSE;
									break;

			case FpaC_SCATTERED:	doedit  = (ed)? TRUE: FALSE;
									dohilo  = FALSE;
									dolink  = FALSE;
									reorder = FALSE;
									break;

			case FpaC_LCHAIN:		doedit  = (ed)? TRUE: FALSE;
									dohilo  = FALSE;
									dolink  = FALSE;
									reorder = FALSE;
									break;

			default:				doedit  = FALSE;
									dohilo  = FALSE;
									dolink  = FALSE;
									reorder = FALSE;
			}

		/* Interpret time dependence - extract daily period if appropriate */
		td   = fdef->element->elem_tdep;
		tdep = td->time_dep;
		switch (tdep)
			{
			case FpaC_DAILY:	(void) convert_value(td->units->name,
											td->normal_time, Hrs, &nhour);
								(void) convert_value(td->units->name,
											td->begin_time,  Hrs, &shour);
								(void) convert_value(td->units->name,
											td->end_time,    Hrs, &ehour);
								ehour = (td)? td->end_time:    0;
								local = TRUE;
								nhrs  = NINT(nhour);
								while (nhrs<0)  nhrs += 24;
								while (nhrs>24) nhrs -= 24;
								shrs  = NINT(shour);
								ehrs  = NINT(ehour);
								while (ehrs<shrs)    ehrs += 24;
								while (ehrs-shrs>24) ehrs -= 24;
								while (nhrs>ehrs)
									{
									shrs += 24;
									ehrs += 24;
									}
								while (nhrs<shrs)
									{
									shrs -= 24;
									ehrs -= 24;
									}
								vhrs  = (nhrs>=shrs)? nhrs: shrs;
								vhrs  = (nhrs<=ehrs)? nhrs: ehrs;
								break;

			case FpaC_STATIC:
			case FpaC_NORMAL:
			default:			local = FALSE;
								shrs  = 0;
								ehrs  = 0;
								vhrs  = 0;
			}

		/* Build structure for this field */
		dfld->element  = strdup(elem);
		dfld->level    = strdup(levl);
		dfld->entity   = (NotNull(ent))? strdup(ent): NullString;
		dfld->group    = strdup(grp);
		dfld->editor   = ftyp;
		dfld->bgset    = FALSE;
		dfld->bgcal    = CAL_create_by_edef(fdef->element);
		dfld->defcal   = CAL_create_by_edef(fdef->element);
		dfld->there    = FALSE;
		dfld->showgrp  = FALSE;
		dfld->showdep  = SHOW_ACTIVE_GRP;
		dfld->showmov  = FALSE;
		dfld->doedit   = doedit;
		dfld->dohilo   = dohilo;
		dfld->dowind   = (LOGICAL) (wind != FpaC_NOWIND);
		dfld->dolink   = dolink;
		dfld->reorder  = reorder;
		dfld->required = TRUE;
		dfld->tdep     = tdep;
		dfld->period.local = local;
		dfld->period.shrs  = shrs;
		dfld->period.ehrs  = ehrs;
		dfld->period.vhrs  = vhrs;
		dfld->labpos   = (tdep_special(tdep))? labpos++: -1;
		dfld->dn	   = NULL;
		dfld->frames   = NULL;
		dfld->fields   = NullFldList;
		dfld->flabs    = NullSetPtr;
		dfld->snaps    = NullFldList;
		dfld->slabs    = NullSetPtr;
		dfld->tweens   = NullFldList;
		dfld->tlabs    = NullSetPtr;
		dfld->linkto   = NULL;
		dfld->nchain   = 0;
		dfld->chains   = NULL;
		dfld->linked   = (!dfld->dolink || tdep_special(dfld->tdep));
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = TRUE;
		}

	/* Set up master links and group master links */
	NumMaster   = identify_groups_for_fields(&gdefs) + 1;
	MasterLinks = GETMEM(MasterLinks, DFLIST, NumMaster);
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;
		if (idfld <= 0)
			{
			elem = "MASTER_LINK";
			levl = "GLOBAL";
			grp  = "GLOBAL";
			}
		else
			{
			elem = "MASTER_LINK";
			levl = "GROUP";
			grp  = gdefs[idfld-1]->name;
			}

		/* Build structure for this field */
		dfld->element  = strdup(elem);
		dfld->level    = strdup(levl);
		dfld->entity   = NULL;
		dfld->group    = strdup(grp);
		dfld->editor   = FpaCnoMacro;
		dfld->bgset    = FALSE;
		dfld->bgcal    = NullCal;
		dfld->defcal   = NullCal;
		dfld->there    = FALSE;
		dfld->showgrp  = FALSE;
		dfld->showdep  = FALSE;
		dfld->showmov  = FALSE;
		dfld->doedit   = FALSE;
		dfld->dohilo   = FALSE;
		dfld->dowind   = FALSE;
		dfld->dolink   = TRUE;
		dfld->reorder  = FALSE;
		dfld->required = TRUE;
		dfld->tdep     = (int) FpaC_NORMAL;
		dfld->period.local = FALSE;
		dfld->period.shrs  = 0;
		dfld->period.ehrs  = 0;
		dfld->period.vhrs  = 0;
		dfld->labpos   = 0;
		dfld->dn	   = NULL;
		dfld->frames   = NULL;
		dfld->fields   = NullFldList;
		dfld->flabs    = NullSetPtr;
		dfld->snaps    = NullFldList;
		dfld->slabs    = NullSetPtr;
		dfld->tweens   = NullFldList;
		dfld->tlabs    = NullSetPtr;
		dfld->linkto   = NULL;
		dfld->nchain   = 0;
		dfld->chains   = NULL;
		dfld->linked   = FALSE;
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = TRUE;
		}
	(void) identify_groups_for_fields_free(&gdefs, NumMaster-1);
	}

/***********************************************************************
*                                                                      *
*     e x t r a c t _ d f i e l d s                                    *
*     e x t r a c t _ d f i e l d                                      *
*     c l e a n u p _ d f i e l d s                                    *
*                                                                      *
***********************************************************************/

LOGICAL	extract_dfields

	(
	int		ivt
	)

	{
	int			idfld;
	DFLIST		*dfld;

	if (ivt < 0)        return FALSE;
	if (ivt >= NumTime) return FALSE;

	/* Check each field in the corresponding metafile */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		(void) extract_dfield(dfld, ivt);
		}

	(void) save_links();
	return TRUE;
	}

/**********************************************************************/

LOGICAL	extract_dfield

	(
	DFLIST	*dfld,
	int		ivt
	)

	{
	int			ifld, jvt;
	FRAME		*frame;
	METAFILE	meta;
	FIELD		fld;
	STRING		element, entity, level;

	if (ivt < 0)        return FALSE;
	if (ivt >= NumTime) return FALSE;

	if (!dfld) return FALSE;

	frame = dfld->frames + ivt;
	if (!frame) return FALSE;

	meta = frame->meta;
	if (!meta) return FALSE;

	/* Check each field in the corresponding metafile */
	element = dfld->element;
	level   = dfld->level;
	entity  = dfld->entity;
	fld     = find_mf_field(meta, NULL, NULL, entity, element, level);
	if (!fld) return FALSE;

	/* Initialize field list in DFLIST structure if necessary */
	if (!dfld->there)
		{
		dfld->there  = TRUE;

		dfld->linked   = (!dfld->dolink || tdep_special(dfld->tdep));
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		link_status(dfld);

		if (!dfld->fields)
			{
			pr_diag("Fields", "Adding field: %s %s\n", level, element);
			dfld->fields = GETMEM(dfld->fields, FIELD, NumTime);
			dfld->flabs  = GETMEM(dfld->flabs,  SET,   NumTime);
			for (jvt=0; jvt<NumTime; jvt++)
				{
				dfld->fields[jvt] = NullFld;
				dfld->flabs[jvt]  = NullSet;
				}
			}
		}

	/* Keep a pointer to the field */
	dfld->fields[ivt] = fld;
	dfld->flabs[ivt]  = find_mf_set(meta, "spot", "d", element, level);
	check_labels(dfld->flabs[ivt]);
#	ifdef DEBUG_DFIELD
	printf("     Adding %s %s at %s\n", level, element,
		   TimeList[ivt].jtime);
	if (fld->ftype == FtypeSfc)
		printf("            units = %s\n", fld->data.sfc->units.name);
#	endif /* DEBUG_DFIELD */

	/* Set depiction flag on for this time if this is a regular field */
	if (tdep_normal(dfld->tdep))
		{
		if (!TimeList[ivt].depict) (void) revise_depict_time(ivt);
		}

	/* >>> save_links() for this dfld? <<< */
	return TRUE;
	}

/**********************************************************************/

LOGICAL	cleanup_dfields(void)

	{
	int			idfld, mplus, ichain, inode, ivt;
	LOGICAL		check_bg, empty, unlinked, there;
	STRING		type;
	POINTER		data;
	DFLIST		*dfld;
	FIELD		fld;
	METAFILE	meta;
	SET			areas;
	AREA		bgnd;
	LCHAIN		chain;

	/* Check each depiction field */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		if (!dfld->there) continue;

		/* Get background value */
		check_bg = (LOGICAL) (dfld->editor == FpaC_DISCRETE ||
							  dfld->editor == FpaC_WIND);

		/* See if whole sequence is empty */
		empty    = TRUE;
		unlinked = FALSE;
		if (dfld->fields)
			{
			for (ivt=0; ivt<NumTime; ivt++)
				{
				there = TRUE;
				mplus = TimeList[ivt].mplus;

				/* Field is empty if it is absent */
				fld  = dfld->fields[ivt];
				meta = dfld->frames[ivt].meta;
				if (!fld || !meta || meta->numfld<=0)
					{
					dfld->fields[ivt] = NullFld;
					dfld->flabs[ivt]  = NullSet;
					there = FALSE;
					}

				/* Area fields are considered empty if they have no */
				/* members and their background value is not set */
				/* In fact we should remove these */
				else if (check_bg)
					{
					there = FALSE;

					recall_fld_data(fld, &type, &data);
					if (same(type, "set") && NotNull(data))
						{
						areas = (SET) data;
						if (areas->num > 0)
							{
							there = TRUE;
							}
						else
							{
							bgnd = (AREA) areas->bgnd;
							if ( NotNull(bgnd) && NotNull(bgnd->attrib) )
								{
								there = TRUE;
								}
							}
						}
					}

				if (there)
					{
					/* Field is there at this time */
					/* Therefore sequence is not empty */
					empty = FALSE;
					}
				else
					{
					/* Field not there at this time */
					dfld->fields[ivt] = NullFld;
					dfld->flabs[ivt]  = NullSet;

					/* Remove corresponding nodes but leave rest of chain */
					for (ichain=0; ichain<dfld->nchain; ichain++)
						{
						chain = dfld->chains[ichain];
						/* >>>>> replace this ...
						if (chain->nodes[ivt]->there)
						... with this <<<<< */
						inode = which_lchain_node(chain, LchainNode, mplus);
						if (inode >= 0 && chain->nodes[inode]->there)
							{
							empty_lnode(chain->nodes[inode]);
							if (chain->inum > 0) chain->dointerp = TRUE;
							unlinked = TRUE;
							}
						}
					}
				}
			}

		/* If any links have been removed reset states */
		if (!empty && unlinked)
			{

#			ifdef DEVELOPMENT
			if (dfld->reported)
				pr_info("Editor.Reported",
					"In cleanup_dfields() - T %s %s\n",
					dfld->element, dfld->level);
			else
				pr_info("Editor.Reported",
					"In cleanup_dfields() - F %s %s\n",
					dfld->element, dfld->level);
#			endif /* DEVELOPMENT */

			dfld->interp   = FALSE;
			dfld->intlab   = FALSE;
			dfld->saved    = FALSE;
			dfld->reported = FALSE;
			verify_dfield_links(dfld, TRUE);
			/* link_status(dfld); */
			}

		/* If sequence is empty reinitialize the field */
		if (empty)
			{
			dfld->there = FALSE;

			/* Get rid of all links */
			clear_dfield_links(dfld, FALSE);

			/* Delete keyframe sequence */
			FREEMEM(dfld->fields);
			FREEMEM(dfld->flabs);

			/* Delete special field snapshots or interpolated sequence */
			release_dfield_interp(dfld);

			/* Re-initialize backgrounds??? and reset states */
			if (check_bg && dfld->bgset)
				{
				CAL_empty(dfld->bgcal);
				CAL_merge(dfld->bgcal, dfld->defcal, TRUE);
				dfld->bgset = FALSE;

				/* Notify the interface */
				reset_background(dfld);
				}

#			ifdef DEVELOPMENT
			if (dfld->reported)
				pr_info("Editor.Reported",
					"In cleanup_dfields() - T %s %s\n",
					dfld->element, dfld->level);
			else
				pr_info("Editor.Reported",
					"In cleanup_dfields() - F %s %s\n",
					dfld->element, dfld->level);
#			endif /* DEVELOPMENT */

			dfld->linked   = (!dfld->dolink || tdep_special(dfld->tdep));
			dfld->interp   = FALSE;
			dfld->intlab   = FALSE;
			dfld->saved    = FALSE;
			dfld->reported = FALSE;
			verify_dfield_links(dfld, TRUE);
			/* link_status(dfld); */
			}

		} /* Next field */

	save_links();
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c h e c k _ d f i e l d _ b a c k g r o u n d                    *
*                                                                      *
***********************************************************************/

LOGICAL	check_dfield_background

	(
	DFLIST	*dfld,
	int		ivt
	)

	{
	STRING	elem, level;
	FRAME	*frame;
	FIELD	fld;
	SET		areas, spots;
	AREA	barea;
	LOGICAL	change;

	if (ivt < 0)        return FALSE;
	if (ivt >= NumTime) return FALSE;
	if (!dfld)            return FALSE;
	switch (dfld->editor)
		{
		case FpaC_DISCRETE:	break;
		case FpaC_WIND:		break;
		default:			return FALSE;
		}

	if (!dfld->there)  return FALSE;
	if (!dfld->fields) return FALSE;
	frame = dfld->frames + ivt;
	if (!frame)        return FALSE;

	elem  = dfld->element;
	level = dfld->level;
	areas = find_mf_set(frame->meta, "area", "b", elem, level);
	spots = find_mf_set(frame->meta, "spot", NULL, elem, level);
	barea = (NotNull(areas))? (AREA) areas->bgnd: NullArea;

	/* If we haven't saved a background yet get it from chart if possible */
	change = FALSE;
	if (!dfld->bgset && NotNull(barea))
		{
		switch (dfld->editor)
			{
			/* Save the chart background for DISCRETE fields */
			case FpaC_DISCRETE:
				CAL_merge(dfld->bgcal, barea->attrib, TRUE);
				CAL_clean(dfld->bgcal);
				dfld->bgset = TRUE;

				/* Notify the interface */
				reset_background(dfld);
				return FALSE;

			/* Save the chart background for WIND fields */
			/*  ... but only if the chart values are consistent! */
			case FpaC_WIND:
				if (!consistent_wind_attribs(barea->attrib))
					{
					change = TRUE;
					break;
					}
				CAL_merge(dfld->bgcal, barea->attrib, TRUE);
				CAL_clean(dfld->bgcal);
				dfld->bgset = TRUE;

				/* Notify the interface */
				reset_background(dfld);
				return FALSE;
			}
		}

	/* Otherwise force this chart to agree */
	/* Find affected fields */
	if (IsNull(areas))
		{
		/* Create an empty field if necessary */
		change = TRUE;
		areas  = make_mf_set(frame->meta, "area", "b", elem, level);
		fld    = find_mf_field(frame->meta, "set", "area", "b", elem, level);
		dfld->fields[ivt] = fld;
		}

	/* See if chart agrees */
	if (!change )
		{
		if (IsNull(barea))                              change = TRUE;
		else if (!CAL_same(barea->attrib, dfld->bgcal)) change = TRUE;
		}
	if (!change) return FALSE;

	/* Recompute affected fields */
	define_set_bg_attribs(areas, dfld->bgcal);
	if (!dfld->bgset)
		{
		dfld->bgset = TRUE;
		reset_background(dfld);
		}

	(void) update_depiction_field(dfld, ivt);
	active_field_info(elem, level, "depict", NULL, NULL, TimeList[ivt].jtime);
	recompute_areaset_labs(areas, spots, TRUE);
	/* <<< save revised labels etc. >>> */

	/* Save the results */
	(void) update_depiction_field(dfld, ivt);
	frame->modified = TRUE;

	/* Check if anything is changing */
	/* >>>
	if (dfld->interp || dfld->intlab || dfld->saved)
		{
		dfld->reported = FALSE;
		}
	<<< */

#	ifdef DEVELOPMENT
	if (dfld->reported)
		pr_info("Editor.Reported",
			"In check_dfield_background() - T %s %s\n",
			dfld->element, dfld->level);
	else
		pr_info("Editor.Reported",
			"In check_dfield_background() - F %s %s\n",
			dfld->element, dfld->level);
#	endif /* DEVELOPMENT */

	dfld->interp = FALSE;
	dfld->intlab = FALSE;
	dfld->saved  = FALSE;
	dfld->reported = FALSE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     f i n d _ m a s t e r _ d f i e l d                              *
*     f i n d _ d f i e l d                                            *
*     m a k e _ d f i e l d                                            *
*                                                                      *
***********************************************************************/

DFLIST	*find_master_dfield

	(
	STRING	group
	)

	{
	int		idfld;

	if (blank(group)) return MasterLinks;

	for (idfld=0; idfld<NumMaster; idfld++)
		{
		if (!same(group, MasterLinks[idfld].group)) continue;

		return MasterLinks + idfld;
		}

	return NULL;
	}

/**********************************************************************/

DFLIST	*find_dfield

	(
	STRING	elem,
	STRING	level
	)

	{
	int		idfld;

	if (blank(elem))  return NULL;
	if (blank(level)) return NULL;

	for (idfld=0; idfld<NumDfld; idfld++)
		{
		if (!same(elem, DfldList[idfld].element)) continue;
		if (!same(level, DfldList[idfld].level))  continue;

		return DfldList + idfld;
		}

	return NULL;
	}

/**********************************************************************/

DFLIST	*make_dfield

	(
	STRING	elem,
	STRING	level
	)

	{
	DFLIST	*dfld;

	/* For now, find will do */
	dfld = find_dfield(elem, level);
	if (!dfld)
		{
		pr_warning("Fields", "Cannot make field: %s %s.\n",
			       elem, level);
		}

	return dfld;
	}

/***********************************************************************
*                                                                      *
*     p i c k _ d f i e l d _ f r a m e                                *
*                                                                      *
***********************************************************************/

int		pick_dfield_frame

	(
	DFLIST	*dfld,
	int		ivt
	)

	{
	FRAME	*frame;
	int		shrs, ehrs, smins, emins, pmins, dmins, jvt;

	if (!dfld)          return -1;
	if (ivt < 0)        return -1;
	if (ivt >= NumTime) return -1;

	/* For MASTER_LINK link field, a normal depiction time is OK */
	if (same(dfld->element, "MASTER_LINK"))
		return (TimeList[ivt].depict)? ivt: -1;

	/* For daily fields, a frame within the daily period will do */
	switch (dfld->tdep)
		{
		case FpaC_DAILY:
			shrs = dfld->period.shrs;
			ehrs = dfld->period.ehrs;
			if (dfld->period.local)
				{
				shrs += LocalOffset;
				ehrs += LocalOffset;
				}
			if (shrs>=24 && ehrs>=24)
				{
				shrs -= 24;
				ehrs -= 24;
				}
			else if (shrs<0 || ehrs<0)
				{
				shrs += 24;
				ehrs += 24;
				}
			smins = shrs * 60;
			emins = ehrs * 60;

			/* Check if requested time is within the daily period */
			pmins = TimeList[ivt].hour * 60 + TimeList[ivt].minute;
			if (pmins < smins)
				{
				if (pmins >= emins - 1440) return -1;
				pmins += 1440;
				}
			if (pmins > emins) return -1;

			/* Search for a chart within the same period */
			dmins = pmins - TimeList[ivt].mplus;
			for (jvt=0; jvt<NumTime; jvt++)
				{
				frame = dfld->frames + jvt;
				if (!frame->meta) continue;

				pmins = TimeList[jvt].mplus + dmins;
				if (pmins < smins) continue;
				if (pmins > emins) break;

				return jvt;
				}
			break;

		/* For static fields, the preceding frame will do */
		case FpaC_STATIC:
			for (jvt=ivt; jvt>=0; jvt--)
				{
				frame = dfld->frames + jvt;
				if (frame->meta) return jvt;
				}
			break;

		/* For normal fields, must have an exact match */
		case FpaC_NORMAL:
		default:
			frame = dfld->frames + ivt;
			if (frame->meta) return ivt;
		}

	/* Not found */
	return -1;
	}
