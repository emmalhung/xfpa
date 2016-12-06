/***********************************************************************
*                                                                      *
*     s e q u e n c e . c                                              *
*                                                                      *
*     This module of the INteractive GRaphics EDitor (INGRED)          *
*     handles all manipulations of the chart sequence (depiction and   *
*     guidance).                                                       *
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

#undef DEBUG_LINK
#undef DEBUG_INTERP

/* Local functions */
static	LOGICAL	set_reference_time(STRING);
static	int		set_active_time(int, int, int);
static	int		delete_valid_time(int);
static	int		insert_valid_time(STRING);

/***********************************************************************
*                                                                      *
*     i n i t _ s e q u e n c e                                        *
*     r e a d _ s e q u e n c e                                        *
*                                                                      *
***********************************************************************/

LOGICAL	init_sequence(void)

	{
	FLD_DESCRIPT	fd;

	/* Build the basic three directories if necessary */
	(void) init_fld_descript(&fd);
	(void) set_fld_descript(&fd, FpaF_SOURCE_NAME, "depict", FpaF_END_OF_LIST);
	(void) prepare_source_directory(&fd);
	(void) set_fld_descript(&fd, FpaF_SOURCE_NAME, "backup", FpaF_END_OF_LIST);
	(void) prepare_source_directory(&fd);
	(void) set_fld_descript(&fd, FpaF_SOURCE_NAME, "interp", FpaF_END_OF_LIST);
	(void) prepare_source_directory(&fd);

	/* Initialize the time list */
	TimeList = NULL;
	NumTime  = 0;

	/* Initialize the guidance sequence data */
	GuidFlds = NULL;

	/* Initialize the depiction field list data */
	DfldList = NULL;
	NumDfld  = 0;

	/* Set to no active depiction */
	(void) set_active_time(-1, -1, -1);

	return TRUE;
	}

LOGICAL	read_sequence

	(
	STRING	mode,
	STRING	srange,
	STRING	erange,
	LOGICAL	save
	)

	{
	int				nt, it;
	STRING			*tlist;
	FLD_DESCRIPT	fd;
	int				idfld, ichain;
	DFLIST			*dfld;
	LCHAIN			chain;

	/* Read in the working depictions */
	(void) init_fld_descript(&fd);
	if (!set_fld_descript(&fd,
					FpaF_SOURCE_NAME,		"depict",
					FpaF_END_OF_LIST)) return FALSE;
	nt = source_valid_time_list(&fd, FpaC_TIMEDEP_ANY, &tlist);
	if (nt > 0) put_message("depict-read");
	for (it=0; it<nt; it++)
		{
		(void) insert_sequence("depict", NULL, NULL, tlist[it], tlist[it]);
		}

	/* Read and confirm the Links file */
	put_message("link-check");
	(void) read_links();
	LinkRead = TRUE;

	/* Blow away unwanted depictions in optional startup modes */
	if (same(mode, "NONE") || same(mode, "RANGE"))
		{
		for (it=0; it<nt; it++)
			{
			if (same(mode, "RANGE"))
				{
				if (strcmp(srange, tlist[it]) <= 0 &&
					strcmp(erange, tlist[it]) >= 0)
					continue;
				}

			if (save) (void) save_sequence(tlist[it]);
			(void) delete_sequence(tlist[it]);
			}
		}
	(void) source_valid_time_list_free(&tlist, nt);

#	ifdef DEVELOPMENT
	if (SequenceReady)
		pr_info("Editor.Reported", "In read_sequence() - SequenceReady is T\n");
	else
		pr_info("Editor.Reported", "In read_sequence() - SequenceReady is F\n");
#	endif /* DEVELOPMENT */

	/* >>> try this turned on!!! <<< */
	/* Initialize interpolated link nodes */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			(void) interpolate_lchain(chain);

			}

#		ifdef DEVELOPMENT
		if (dfld->reported)
			pr_info("Editor.Reported",
				"In read_sequence() - T %s %s\n", dfld->element, dfld->level);
		else
			pr_info("Editor.Reported",
				"In read_sequence() - F %s %s\n", dfld->element, dfld->level);
#		endif /* DEVELOPMENT */

		link_status(dfld);
		}
	/* >>> try this turned on!!! <<< */

	put_message("depict-ready");
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     i n s e r t _ s e q u e n c e                                    *
*     d e l e t e _ s e q u e n c e                                    *
*     s a v e _ s e q u e n c e                                        *
*                                                                      *
***********************************************************************/

LOGICAL	insert_sequence

	(
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime,
	STRING	ttime
	)

	{
	int		ivt;
	LOGICAL	status, active, adding=TRUE, empty=FALSE;

	/* First make sure requested depiction exists */
	empty = blank(source);
	if (!empty)
		{
		status = find_depiction(source, subsrc, rtime, vtime);
		if (!status)
			{
			return FALSE;
			}
		}

	/* See if target time is already in the sequence */
	ivt = find_valid_time(ttime);
	if (ivt >= 0)
		{
		/* There is a valid time there already */
		/* Check whether it contains only special fields */
		(void) check_depict_times();
		if (TimeList[ivt].depict) return FALSE;
		adding = FALSE;
		}

	/* Insert the target time in the sequence */
	if (adding)
		{
		ivt = insert_valid_time(ttime);
		if (ivt < 0)
			{
			return FALSE;
			}
		}
	active = (LOGICAL) (ivt==EditTime || ivt==ViewTime);

	/* Pack away the current data if inserting at the current active time */
	if (DepictShown && active)
		{
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(TRUE);
		}
	if (SequenceReady && active)
		{
		present_all();
		}

	/* Now read in the requested depiction and matching guidance charts */
	status = empty;
	if (!empty)
		{
		status = insert_depiction(source, subsrc, rtime, vtime, ivt);
		if (!same(source, "depict") || !same(vtime, ttime))
			(void) update_depiction(ivt);
		if (status) put_message("depict-added");
		}
	(void) check_depict_times();

	/* Interpolate new link nodes if possible */
	(void) interp_links(ivt, TRUE);
	(void) revise_links();

	/* Re-display if active time */
	if (SequenceReady && DepictShown && active)
		{
		/* >>>>> remove Jul2008
		(void) present_all();
		remove Jul 2008 <<<<< */
		(void) extract_edit_field();
		(void) extract_special_tags();
		(void) extract_links();
		(void) show_edit_field();
		(void) present_all();
		}

	return status;
	}

/**********************************************************************/

LOGICAL	delete_sequence

	(
	STRING	vtime
	)

	{
	int			idfld, ivt, oldvvt, oldevt, oldgvt;
	LOGICAL		active;
	METAFILE	meta;

	/* Make sure target time is in the sequence */
	ivt = find_valid_time(vtime);
	if (ivt < 0)
		{
		return FALSE;
		}
	active = (LOGICAL) (ivt==EditTime || ivt==ViewTime);

	/* Pack away the current data if deleting active time */
	if (DepictShown && active)
		{
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(TRUE);
		}
	if (active)
		{
		oldvvt = ViewTime;
		oldevt = EditTime;
		oldgvt = GrpTime;
		(void) set_active_time(-1, -1, -1);
		}
	(void) delete_depiction(ivt);
	(void) check_depict_times();
	put_message("depict-deleted");

	/* See if there are any special fields remaining at this valid time */
	meta = NULL;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		meta = DfldList[idfld].frames[ivt].meta;
		if (meta) break;
		}

	/* If no fields remain at this valid time remove the whole valid time */
	if (!meta)
		{
		/* Delete the depiction valid time */
		(void) delete_valid_time(ivt);
		(void) cleanup_dfields();
		if (DepictShown && active) present_all();

		return TRUE;
		}

	/* Re-display the depiction if necessary */
	(void) cleanup_dfields();
	if (active) (void) set_active_time(oldvvt, oldevt, oldgvt);
	if (DepictShown && active)
		{
		/* >>>>> remove Jul2008
		(void) present_all();
		remove Jul 2008 <<<<< */
		(void) extract_edit_field();
		(void) extract_special_tags();
		(void) extract_links();
		(void) show_edit_field();
		}
	if (active) (void) present_all();

	/* Notify the appropriate edit module */
	if (active) (void) depiction_check();

	return TRUE;
	}

/**********************************************************************/

LOGICAL	save_sequence

	(
	STRING	vtime
	)

	{
	int		ivt;

	ivt = find_valid_time(vtime);

#	ifdef DEVELOPMENT
	pr_diag("Editor.Reported",
		"In save_sequence() - %s (%d)\n", vtime, ivt);
#	endif /* DEVELOPMENT */

	if (ivt < 0)
		{
		return FALSE;
		}

	put_message("depict-saving");
	(void) backup_depiction(ivt);
	put_message("depict-saved");
	(void) make_depict_status(ivt, "Saved", NULL);
	return give_active_status();
	}

/***********************************************************************
*                                                                      *
*     i m p o r t _ f i e l d                                          *
*     d e l e t e _ f i e l d                                          *
*     s a v e _ f i e l d                                              *
*                                                                      *
***********************************************************************/

LOGICAL	import_field

	(
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime,
	STRING	ttime,
	STRING	elem,
	STRING	level
	)

	{
	FLD_DESCRIPT	*fd;
	int				ivt, jvt;
	LOGICAL			valid, active;
	DFLIST			*dfld;
	LOGICAL			adding = FALSE;

	/* Alter the source and target valid times for daily fields */
	dfld = make_dfield(elem, level);
	if (!dfld) return FALSE;

	/* Create field if requested */
	if (blank(source))
		{
		/* Don't create it if it is already in the depiction sequence */
		fd = find_depiction_field("depict", NULL, NULL, vtime, elem, level);
		if (fd)
			{
			return FALSE;
			}
		}

	/* Otherwise make sure requested source exists */
	else
		{
		fd = find_depiction_field(source, subsrc, rtime, vtime, elem, level);
		if (!fd)
			{
			return FALSE;
			}
		}

	/* Make sure target time is in the sequence */
	ivt = find_valid_time(ttime);
	if (ivt < 0)
		{
		if (tdep_special(dfld->tdep))
			{
			/* Insert the target time only if the field is daily or static */
			ivt = insert_valid_time(ttime);
			if (ivt < 0)
				{
				return FALSE;
				}
			adding = TRUE;
			(void) check_depict_times();
			}
		else
			{
			return FALSE;
			}
		}
	active = (LOGICAL) (ivt==EditTime || ivt==ViewTime);

	/* Pack away the current data if inserting at the current active time */
	if (DepictShown && active)
		{
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(TRUE);
		}

	/* Now insert the requested field in the target depiction */
	valid = import_depiction_field(fd, dfld, ivt, TRUE, TRUE);
	if (!valid)
		{
		return FALSE;
		}
	put_message("fld-added", level, elem);
	(void) update_depiction_field(dfld, ivt);
	(void) recompute_dependent_winds(dfld, TRUE, ivt);
	(void) check_depict_times();
	if (adding)
		{
		}
	if (!active && (tdep_special(dfld->tdep)))
		{
		if (dfld == ActiveDfld) jvt = pick_dfield_frame(dfld, EditTime);
		else                    jvt = pick_dfield_frame(dfld, ViewTime);
		active = (LOGICAL) (jvt==ivt);
		}

	/* Interpolate new link nodes if possible */
	(void) verify_dfield_links(dfld, TRUE);
	(void) interp_dfield_links(dfld, ivt, TRUE);
	if (dfld->linkto) (void) revise_dependent_links(dfld->linkto);
	(void) save_links();

	if (DepictShown && active)
		{
		(void) pick_depiction();
		/* >>>>> remove Jul2008
		(void) present_all();
		remove Jul 2008 <<<<< */
		(void) extract_edit_field();
		(void) extract_special_tags();
		(void) extract_links();
		(void) show_edit_field();
		}
	if (active) (void) present_all();
	(void) update_depiction(ivt);

	/* Notify the appropriate edit module */
	if (active) (void) depiction_check();

	return TRUE;
	}

/**********************************************************************/

LOGICAL	delete_field

	(
	STRING	vtime,
	STRING	elem,
	STRING	level
	)

	{
	int			idfld, ivt, jvt, oldevt, oldvvt, oldgvt;
	LOGICAL		active;
	DFLIST		*dfld;
	METAFILE	meta;

	/* Alter the target valid time for daily fields */
	dfld = find_dfield(elem, level);
	if (IsNull(dfld)) return FALSE;

	/* Make sure target time is in the sequence */
	ivt = find_valid_time(vtime);
	if (ivt < 0)
		{
		return FALSE;
		}

	active = (LOGICAL) (ivt==EditTime || ivt==ViewTime);
	if (!active && (tdep_special(dfld->tdep)))
		{
		if (dfld == ActiveDfld) jvt = pick_dfield_frame(dfld, EditTime);
		else                    jvt = pick_dfield_frame(dfld, ViewTime);
		active = (LOGICAL) (jvt==ivt);
		}

	/* Pack away the current data if deleting active field */
	if (DepictShown && active)
		{
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(TRUE);
		}
	if (active)
		{
		oldvvt = ViewTime;
		oldevt = EditTime;
		oldgvt = GrpTime;
		(void) set_active_time(-1, -1, -1);
		}
	(void) delete_depiction_field(dfld, ivt);
	(void) check_depict_times();
	put_message("fld-deleted", level, elem);

	/* See if this was the last field at this valid time */
	/* If so remove the whole valid time */
	meta = NULL;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		meta = DfldList[idfld].frames[ivt].meta;
		if (meta) break;
		}
	if (!meta)
		{
		put_message("depict-deleted");

		/* Delete the depiction valid time */
		(void) delete_valid_time(ivt);
		(void) cleanup_dfields();
		if (DepictShown && active) present_all();

		return TRUE;
		}
	(void) revise_dependent_links(dfld);

	/* Reset the depict flag for this time if necessary */
	if (tdep_normal(dfld->tdep)) (void) revise_depict_time(ivt);

	/* Re-compute link status and re-display the depiction if necessary */
	(void) cleanup_dfields();
	(void) update_depiction(ivt);
	(void) make_depict_status(ivt, NULL, NULL);
	(void) verify_dfield_links(dfld, TRUE);
	(void) save_links();
	if (active) (void) set_active_time(oldvvt, oldevt, oldgvt);
	if (DepictShown && active)
		{
		/* >>>>> remove Jul2008
		(void) present_all();
		remove Jul 2008 <<<<< */
		(void) extract_edit_field();
		(void) extract_special_tags();
		(void) extract_links();
		(void) show_edit_field();
		}
	if (active) (void) present_all();

	/* Notify the appropriate edit module */
	if (active) (void) depiction_check();

	return TRUE;
	}

/**********************************************************************/

LOGICAL	save_field

	(
	STRING	vtime,
	STRING	elem,
	STRING	level
	)

	{
	int		ivt;
	DFLIST	*dfld;

	/* Alter the target valid time for daily fields */
	dfld = find_dfield(elem, level);
	if (IsNull(dfld)) return FALSE;

	ivt = find_valid_time(vtime);
	if (ivt < 0)
		{
		return FALSE;
		}

	put_message("fld-saving", level, elem);
	(void) backup_depiction_field(dfld, ivt);
	put_message("fld-saved", level, elem);
	return give_active_status();
	}

/***********************************************************************
*                                                                      *
*     p i c k _ s e q u e n c e                                        *
*     z e r o _ s e q u e n c e                                        *
*     f i n d _ v a l i d _ t i m e                                    *
*     f i n d _ p r o g _ t i m e                                      *
*                                                                      *
***********************************************************************/

LOGICAL	pick_sequence

	(
	STRING	vtime,
	STRING	etime
	)

	{
	int		ivt, jvt, gvt, hrs, mins;
	FRAME	*vframe, *eframe;

	/* Set the view time */
	ivt = find_valid_time(vtime);
	if (ivt < 0)
		{
		return FALSE;
		}

	/* Set the edit time and group time if no edit time passed */
	/*  or if the edit time and view time are the same         */
	if (blank(etime) || matching_tstamps(etime, vtime))
		{
		jvt = pick_dfield_frame(ActiveDfld, ivt);
		if (jvt < 0)
			{
			etime = vtime;
			jvt   = ivt;
			}
		else
			{
			etime = TimeList[jvt].jtime;
			}
		gvt = ivt;
		}

	/* Set the edit time and group time if edit time passed       */
	/* Note that group time is set to view time for static fields */
	else
		{
		jvt = find_valid_time(etime);
		if (jvt < 0) jvt = ivt;

		/* Set group time to view time for static fields */
		if (NotNull(ActiveDfld) && ActiveDfld->tdep == FpaC_STATIC)
			gvt = ivt;

		/* Set group time to edit time for daily or normal fields */
		else
			gvt = jvt;
		}

	/* Reset active time (even if no change) to handle daily field display */
	if (ivt==ViewTime && jvt==EditTime && gvt==GrpTime && !Spawned)
		{
		pr_info("Editor", "No change to active times.\n");
		(void) set_active_time(ivt, jvt, gvt);
		/* >>>>> remove Jul2008
		(void) present_all();
		remove Jul 2008 <<<<< */
		return give_active_status();
		}

	pr_info("Editor", "View time: %s (T%s).\n",
			vtime, hour_minute_string(0, TimeList[ivt].mplus));
	pr_info("Editor", "Edit time: %s (T%s).\n",
			etime, hour_minute_string(0, TimeList[jvt].mplus));
	put_message("depict-active", vtime, etime);
	if (DepictShown)
		{
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(TRUE);
		}
	(void) set_active_time(ivt, jvt, gvt);
	if (DepictShown && (EditTime >= 0) && NotNull(ActiveDfld)
		&& NotNull(ActiveDfld->frames))
		{
		/* Display if daily or static fields need to be contoured */
		vframe = ActiveDfld->frames + ViewTime;
		eframe = ActiveDfld->frames + EditTime;
		if (!vframe->contoured || !eframe->contoured) (void) present_all();
		(void) extract_edit_field();
		(void) extract_special_tags();
		(void) extract_links();
		(void) show_edit_field();
		}
	if (GuidShown && (ViewTime >= 0))
		{
		/* reset_active_product(); */
		}
	(void) present_all();
	LinkVtime = -1;
	LinkEtime = -1;
	LinkDir   = 0;

	/* See if it worked */
	return give_active_status();
	}

LOGICAL	zero_sequence

	(
	STRING	vtime
	)

	{
	if (!set_reference_time(vtime)) return FALSE;
	if (LinkShown)
		{
		(void) hide_timelink();
		(void) show_timelink();
		}
	return TRUE;
	}

int		find_valid_time

	(
	STRING	vtime
	)

	{
	int		mplus;
	int		year, jday, hour, min;
	LOGICAL	local;

	/* Parse valid time string into date components */
	/* and turn valid time into a prog time */
	if (!parse_tstamp(vtime, &year, &jday, &hour, &min, &local,
			NullLogicalPtr)) return -1;
	if (local)
		{
		hour += LocalOffset;
		tnorm(&year, &jday, &hour, &min, NullInt);
		}
	mplus = mdif(Syear, Sjday, Shour, Sminute, year, jday, hour, min);

	return find_prog_time(mplus, local);
	}

int		find_prog_time

	(
	int		mplus,
	LOGICAL	local
	)

	{
	int		ivt;
	VTIME	*ivalid;

	/* Search through current list of valid times until given time */
	/* is equalled or exceeded - local flag must match! */
	for (ivt=0; ivt<NumTime; ivt++)
		{
		ivalid = TimeList + ivt;
		if (ivalid->mplus == mplus)
			{
			if (LogicalAgree(local, ivalid->local)) return ivt;
			continue;
			}
		if (ivalid->mplus > mplus)
			{
			return -1;
			}
		}

	/* Not found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*     m a k e _ d e p i c t _ s t a t u s                              *
*     t e l l _ a c t i v e _ s t a t u s                              *
*     g i v e _ a c t i v e _ s t a t u s                              *
*                                                                      *
***********************************************************************/

LOGICAL	make_depict_status

	(
	int		ivt,
	STRING	source,
	STRING	model
	)

	{
	VTIME	*vt;

	if (ivt < 0)        return FALSE;
	if (ivt >= NumTime) return FALSE;

	vt = TimeList + ivt;
	(void) sprintf(Msg, "Valid time: %sZ", vt->mtime);
	if (blank(source))                    { }
	else if (same(source, FpaDir_Depict)) { }
	else if (same(source, FpaDir_Backup)) (void) strcat(Msg, " (From Backup)");
	else if (same(source, "Previous"))    (void) strcat(Msg, " (From Preceding Chart)");
	else if (same(source, "Default"))     (void) strcat(Msg, " (From Flat Fields)");
	else if (same(source, "Model"))
		{
		if (blank(model))               (void) strcat(Msg, " (From Model)");
		else                            {
										(void) strcat(Msg, " (From ");
										(void) strcat(Msg, model);
										(void) strcat(Msg, ")");
										}
		}
	else                                {
										(void) strcat(Msg, " (");
										(void) strcat(Msg, source);
										(void) strcat(Msg, ")");
										}
	vt->status = SETSTR(vt->status, Msg);
	return TRUE;
	}


LOGICAL	tell_active_status(void)

	{
	VTIME	*vt;

	if (ViewTime < 0)
		{
		if (!SequenceReady) put_message("depict-process");
		else                put_message("depict-active-none");
		return FALSE;
		}

	if (MovieGoing)
		{
		put_message(MovieMsg);
		return TRUE;
		}

	vt = TimeList + ViewTime;
	if (IsNull(vt->status)) make_depict_status(ViewTime, NULL, NULL);
	string_message("status", vt->status);	/* does not use msg db */
	return TRUE;
	}


LOGICAL	give_active_status(void)

	{
	if (!SequenceReady) return TRUE;
	if (ViewTime < 0)   return FALSE;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     LOCAL STATIC FUNCTIONS:                                          *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*     s e t _ r e f e r e n c e _ t i m e                              *
*                                                                      *
***********************************************************************/

static	LOGICAL	set_reference_time

	(
	STRING	vtime
	)

	{
	int		year, jday, month, mday, hour, min;
	LOGICAL	local;
	int		idfld, ichain, ivt, idif;
	DFLIST	*dfld;

	/* Parse valid time string into date components */
	if (!parse_tstamp(vtime, &year, &jday, &hour, &min, &local,
			NullLogicalPtr)) return FALSE;
	if (local)
		{
		hour += LocalOffset;
		tnorm(&year, &jday, &hour, &min, NullInt);
		}

	/* Reset if previously set */
	if (Syear != 0)
		{
		/* See if it is different from the original */
		idif = mdif(Syear, Sjday, Shour, Sminute, year, jday, hour, min);
		if (idif == 0) return TRUE;
		if (minutes_in_depictions())
			{
			pr_info("Editor", "Resetting T0 by %d minutes.\n", idif);
			}
		else
			{
			pr_info("Editor", "Resetting T0 by %d hours.\n", idif/60);
			}

		/* Change the prog times for the time list */
		for (ivt=0; ivt<NumTime; ivt++)
			{
			TimeList[ivt].mplus -= idif;
			}
		}

	/* Save the new T0 */
	mdate(&year, &jday, &month, &mday);
	Syear   = year;
	Sjday   = jday;
	Smonth  = month;
	Smday   = mday;
	Shour   = hour;
	Sminute = min;
	(void) strcpy(Stime, build_tstamp(Syear, Sjday, Shour, Sminute,
											FALSE, minutes_in_depictions()));

	/* Reset link start and end times if needed */
	if (LinkRead)
		{
		for (idfld=0; idfld<NumDfld; idfld++)
			{
			dfld = DfldList + idfld;

			for (ichain=0; ichain<dfld->nchain; ichain++)
				{
				define_lchain_reference_time(dfld->chains[ichain], Stime);
				define_lchain_default_attribs(dfld->chains[ichain]);
				}
			}

		for (idfld=0; idfld<NumMaster; idfld++)
			{
			dfld = MasterLinks + idfld;

			for (ichain=0; ichain<dfld->nchain; ichain++)
				{
				define_lchain_reference_time(dfld->chains[ichain], Stime);
				define_lchain_default_attribs(dfld->chains[ichain]);
				}
			}
		}

	/* Return when finished */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     s e t _ a c t i v e _ t i m e                                    *
*                                                                      *
***********************************************************************/

static	int		set_active_time

	(
	int		ivt,
	int		jvt,
	int		gvt
	)

	{
	/* Can be negative - but cannot exceed last depiction */
	if (ivt >= NumTime) ivt = NumTime - 1;
	if (jvt >= NumTime) jvt = NumTime - 1;
	if (gvt >= NumTime) gvt = NumTime - 1;
	ViewTime = ivt;
	EditTime = jvt;
	GrpTime  = gvt;

	/* Set depiction fields to point to active metafile */
	(void) pick_depiction();

	return ViewTime;
	}

/***********************************************************************
*                                                                      *
*     i n s e r t _ v a l i d _ t i m e                                *
*     d e l e t e _ v a l i d _ t i m e                                *
*                                                                      *
***********************************************************************/

static	int		insert_valid_time

	(
	STRING	vtime
	)

	{
	int				ivt, jvt, idfld, ichain;
	int				ifirst, ilast;
	int				year, jday, month, mday, hour, min, nm, xmplus;
	LOGICAL			local;
	TSTAMP			mtime;
	VTIME			*ivalid, *jvalid;
	FRAME			*iframe, *jframe;
	DFLIST			*dfld;
	LCHAIN			chain;
	LNODE			lnode;
	FLD_DESCRIPT	fd;
	LOGICAL			new;

	/* Parse valid time string into date components */
	if (!parse_tstamp(vtime, &year, &jday, &hour, &min, &local,
			NullLogicalPtr)) return -1;
	if (local)
		{
		hour += LocalOffset;
		tnorm(&year, &jday, &hour, &min, NullInt);
		}

	/* Ensure that valid time is a multiple of interpolation delta */
	/* Note that local times are not checked ! */
	if (!local)
		{

		/* Determine time (in minutes) */
		xmplus = hour*60 + min;

		/* Error if valid time is not a multiple of interpolation delta */
		/* Assumes that interpolation times will be with respect to */
		/*  0 hours and 0 minutes of any particular day!            */
		if ( xmplus % DTween != 0 )
			{
			pr_error("Interp",
				"Valid time %s (%d) does not match interpolation delta (%d)\n",
				vtime, xmplus, DTween);

			put_error_dialog("InterpDelta");
			return -1;
			}
		}

	/* Search through current list of valid times until given time */
	/* is equalled or exceeded */
	for (ivt=0; ivt<NumTime; ivt++)
		{
		ivalid = TimeList + ivt;
		nm = mdif(ivalid->year, ivalid->jday, ivalid->hour, ivalid->minute,
				year, jday, hour, min);
		if (nm == 0)
			{
			if (LogicalAgree(local, ivalid->local)) return ivt;
			continue;
			}
		if (nm < 0)  break;
		}

	/* Insert new valid time in the time sequence */
	pr_diag("Editor", "Adding valid time %s.\n", vtime);
	pr_info("Editor.Sequence",
			"Inserting time %s  - Active frame: %d (Edit: %d  View: %d).\n",
			vtime, ivt, EditTime, ViewTime);

	mdate(&year, &jday, &month, &mday);
	if (minutes_in_depictions())
		{
		(void) sprintf(mtime, "%.4d/%.2d/%.2d %.2d:%.2d",
				year, month, mday, hour, min);
		}
	else
		{
		(void) sprintf(mtime, "%.4d/%.2d/%.2d %.2d",
				year, month, mday, hour);
		}

	/* Set T0 if never set before */
	if (Syear == 0)
		{
		Syear   = year;
		Sjday   = jday;
		Smonth  = month;
		Smday   = mday;
		Shour   = hour;
		Sminute = min;
		(void) strcpy(Stime, build_tstamp(Syear, Sjday, Shour, Sminute,
											FALSE, minutes_in_depictions()));
		}

	/* Insert new valid time */
	NumTime++;
	TimeList = GETMEM(TimeList, VTIME, NumTime);
	for (jvt=NumTime-1; jvt>ivt; jvt--)
		{
		jvalid = TimeList + jvt;
		ivalid = jvalid - 1;
		jvalid->jtime  = ivalid->jtime;
		jvalid->mtime  = ivalid->mtime;
		jvalid->year   = ivalid->year;
		jvalid->jday   = ivalid->jday;
		jvalid->month  = ivalid->month;
		jvalid->mday   = ivalid->mday;
		jvalid->hour   = ivalid->hour;
		jvalid->minute = ivalid->minute;
		jvalid->mplus  = mdif(Syear, Sjday, Shour, Sminute, jvalid->year,
								jvalid->jday, jvalid->hour, jvalid->minute);
		jvalid->tplus  = jvalid->mplus / 60;
		jvalid->status = ivalid->status;
		jvalid->local  = ivalid->local;
		jvalid->depict = ivalid->depict;
		}
	ivalid = TimeList + ivt;
	ivalid->jtime  = safe_strdup(vtime);
	ivalid->mtime  = safe_strdup(mtime);
	ivalid->year   = year;
	ivalid->jday   = jday;
	ivalid->month  = month;
	ivalid->mday   = mday;
	ivalid->hour   = hour;
	ivalid->minute = min;
	ivalid->mplus  = mdif(Syear, Sjday, Shour, Sminute, year, jday, hour, min);
	ivalid->tplus  = ivalid->mplus / 60;
	ivalid->status = NULL;
	ivalid->local  = local;
	ivalid->depict = FALSE;

	/* Recall first and last "depict" times */
	ifirst = first_depict_time();
	ilast  = last_depict_time();

	/* Insert corresponding space in field sequence buffer */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;

		/* Insert new frame in depiction sequence */
		dfld->frames = GETMEM(dfld->frames, FRAME, NumTime);
		for (jvt=NumTime-1; jvt>ivt; jvt--)
			{
			jframe = dfld->frames + jvt;
			iframe = jframe - 1;
			jframe->wfile     = iframe->wfile;
			jframe->bfile     = iframe->bfile;
			jframe->owfile    = iframe->owfile;
			jframe->obfile    = iframe->obfile;
			jframe->meta      = iframe->meta;
			jframe->modified  = iframe->modified;
			jframe->contoured = iframe->contoured;
			}
		iframe = dfld->frames + ivt;
		(void) init_fld_descript(&fd);
		(void) set_fld_descript(&fd,
						FpaF_SOURCE_NAME,		"depict",
						FpaF_VALID_TIME,		vtime,
						FpaF_ELEMENT_NAME,		dfld->element,
						FpaF_LEVEL_NAME,		dfld->level,
						FpaF_END_OF_LIST);
		iframe->wfile     = safe_strdup(construct_meta_filename(&fd));
		iframe->owfile    = safe_strdup(build_meta_filename(&fd));
		(void) set_fld_descript(&fd,
						FpaF_SOURCE_NAME,		"backup",
						FpaF_END_OF_LIST);
		iframe->bfile     = safe_strdup(construct_meta_filename(&fd));
		iframe->obfile    = safe_strdup(build_meta_filename(&fd));
		iframe->meta      = NULL;
		iframe->modified  = FALSE;
		iframe->contoured = FALSE;

		/* Insert new field in keyframe sequence */
		new = IsNull(dfld->fields);
		dfld->fields = GETMEM(dfld->fields, FIELD, NumTime);
		dfld->flabs  = GETMEM(dfld->flabs,  SET,   NumTime);
		if (new)
			{
			for (jvt=0; jvt<NumTime; jvt++)
				{
				dfld->fields[jvt] = NullFld;
				dfld->flabs[jvt]  = NullSet;
				}
			}
		else
			{
			for (jvt=NumTime-1; jvt>ivt; jvt--)
				{
				dfld->fields[jvt] = dfld->fields[jvt-1];
				dfld->flabs[jvt]  = dfld->flabs[jvt-1];
				}
			dfld->fields[ivt] = NullFld;
			dfld->flabs[ivt]  = NullSet;
			}

		/* Expand current link chains */
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			chain->nodes = GETMEM(chain->nodes, LNODE, NumTime);
			for (jvt=NumTime-1; jvt>ivt; jvt--)
				{
				chain->nodes[jvt] = chain->nodes[jvt-1];
				}
			chain->nodes[ivt] = create_lnode(ivalid->mplus);
			chain->lnum++;
			... with this <<<<< */
			lnode = create_lnode(ivalid->mplus);
			(void) add_lchain_lnode(chain, lnode);
			}

		/* >>>>> try without doing this yet!
		dfld->interp = FALSE;
		dfld->intlab = FALSE;
		dfld->saved  = FALSE;
		<<<<< */

		/* Delete special field snapshots or interpolated sequence */
		release_dfield_interp(dfld);
		}

	/* Insert corresponding space in master links buffer */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;

		/* Expand current link chains */
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			/* >>>>> replace this ...
			chain->nodes = GETMEM(chain->nodes, LNODE, NumTime);
			for (jvt=NumTime-1; jvt>ivt; jvt--)
				{
				chain->nodes[jvt] = chain->nodes[jvt-1];
				}
			chain->nodes[ivt] = create_lnode(ivalid->mplus);
			chain->lnum++;
			... with this <<<<< */
			lnode = create_lnode(ivalid->mplus);
			(void) add_lchain_lnode(chain, lnode);
			}

		/* Master links are not interpolated */
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		}

	/* Reset number of interpolated frames */
	NumTween = 0;
	if (ifirst>=0 && ilast>ifirst)
		NumTween = (TimeList[ilast].mplus - TimeList[ifirst].mplus) /DTween + 1;

	/* Check field and master links */
	(void) verify_links(TRUE);


	/* Reset interpolated link nodes */
	/* >>>>> Does this need to be done yet???  <<<<< */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			(void) interpolate_lchain(chain);
			}
		}
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			(void) interpolate_lchain(chain);
			}
		}

	/* Reset current times */
	if (EditTime >= ivt) EditTime++;
	if (ViewTime >= ivt) ViewTime++;
	pr_info("Editor.Sequence",
			"After inserting %s - Active frame: %d (Edit: %d  View: %d).\n",
			vtime, ivt, EditTime, ViewTime);

	/* Done - return the position of the new valid time */
	return ivt;
	}

/**********************************************************************/

static	int		delete_valid_time

	(
	int		ivt
	)

	{
	int			jvt, idfld, iguid, ichain, inode;
	int			ifirst, ilast, mplus;
	LOGICAL		rok;
	VTIME		*ivalid, *jvalid;
	FRAME		*iframe, *jframe;
	DFLIST		*dfld;
	LCHAIN		chain;

	if (ivt < 0)        return -1;
	if (ivt >= NumTime) return -1;

	pr_info("Editor.Sequence",
			"Deleting time  - Active frame: %d (Edit: %d  View: %d).\n",
			ivt, EditTime, ViewTime);

	/* Save the valid time for removing link chain nodes */
	mplus = TimeList[ivt].mplus;

	/* Free allocated space in time list and close gap */
	ivalid = TimeList + ivt;
	pr_diag("Editor", "Removing valid time %s.\n", ivalid->jtime);
	FREEMEM(ivalid->jtime);
	FREEMEM(ivalid->mtime);
	FREEMEM(ivalid->status);
	for (jvt=ivt+1; jvt<NumTime; jvt++)
		{
		jvalid = TimeList + jvt;
		ivalid = jvalid - 1;
		ivalid->jtime  = jvalid->jtime;
		ivalid->mtime  = jvalid->mtime;
		ivalid->year   = jvalid->year;
		ivalid->jday   = jvalid->jday;
		ivalid->month  = jvalid->month;
		ivalid->mday   = jvalid->mday;
		ivalid->hour   = jvalid->hour;
		ivalid->minute = jvalid->minute;
		ivalid->mplus  = jvalid->mplus;
		ivalid->tplus  = jvalid->tplus;
		ivalid->status = jvalid->status;
		ivalid->local  = jvalid->local;
		ivalid->depict = jvalid->depict;
		}

	/* Recall new first and last "depict" times */
	ifirst = first_depict_time();
	ilast  = last_depict_time();

	/* Free allocated space in field sequence buffer and move sequence up */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;

		/* Free allocated space in depiction list and close gap */
		iframe = dfld->frames + ivt;
		FREEMEM(iframe->wfile);
		FREEMEM(iframe->bfile);
		FREEMEM(iframe->owfile);
		FREEMEM(iframe->obfile);
		if (iframe->meta) destroy_metafile(iframe->meta);
		for (jvt=ivt+1; jvt<NumTime; jvt++)
			{
			jframe = dfld->frames + jvt;
			iframe = jframe - 1;
			iframe->wfile     = jframe->wfile;
			iframe->bfile     = jframe->bfile;
			iframe->owfile    = jframe->owfile;
			iframe->obfile    = jframe->obfile;
			iframe->meta      = jframe->meta;
			iframe->modified  = jframe->modified;
			iframe->contoured = jframe->contoured;
			}

		/* Move keyframe sequence up to close gap */
		if (dfld->fields)
			{
			dfld->fields[ivt] = NullFld;
			dfld->flabs[ivt]  = NullSet;
			for (jvt=ivt+1; jvt<NumTime; jvt++)
				{
				dfld->fields[jvt-1] = dfld->fields[jvt];
				dfld->flabs[jvt-1]  = dfld->flabs[jvt];
				}
			}

		/* Get rid of link nodes at this time */
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];

			/* >>>>> replace this ...
			if (chain->nodes[ivt]->there && !chain->nodes[ivt]->guess)
			... with this <<<<< */

			/* Check for a link node at this time */
			inode = which_lchain_node(chain, LchainNode, mplus);
			if (inode < 0) continue;

			/* If a link node exists ... have to re-interpolate */
			if (chain->nodes[inode]->there)
				{

#				ifdef DEVELOPMENT
				if (dfld->reported)
					pr_info("Editor.Reported",
						"In delete_valid_time() - T %s %s\n",
						dfld->element, dfld->level);
				else
					pr_info("Editor.Reported",
						"In delete_valid_time() - F %s %s\n",
						dfld->element, dfld->level);
#				endif /* DEVELOPMENT */

				dfld->interp   = FALSE;
				dfld->intlab   = FALSE;
				dfld->saved    = FALSE;
				dfld->reported = FALSE;
				}

			/* Remove the start/end link node ... which shortens the link chain */
			/* >>>>> replace this ...
			chain->nodes[ivt] = destroy_lnode(chain->nodes[ivt]);
			for (jvt=ivt+1; jvt<NumTime; jvt++)
				{
				chain->nodes[jvt-1] = chain->nodes[jvt];
				}
			chain->lnum--;
			... with this <<<<< */
			rok = remove_link_node(dfld, ichain, inode);

			/* Remove an intermediate node */
			if (!rok) empty_lnode(chain->nodes[inode]);
			}

		/* Delete special field snapshots or interpolated sequence */
		release_dfield_interp(dfld);
		}

	/* Free allocated space in master links buffer and move sequence up */
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;

		/* Get rid of link nodes at this time */
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];

			/* Check for a link node at this time */
			inode = which_lchain_node(chain, LchainNode, mplus);
			if (inode < 0) continue;

			/* Remove the link node ... which may shorten the link chain */
			/* >>>>> replace this ...
			chain->nodes[ivt] = destroy_lnode(chain->nodes[ivt]);
			for (jvt=ivt+1; jvt<NumTime; jvt++)
				{
				chain->nodes[jvt-1] = chain->nodes[jvt];
				}
			chain->lnum--;
			... with this <<<<< */
			(void) remove_link_node(dfld, ichain, inode);
			}

		/* >>> take this out for now!!! master links do not get interpolated!!!
		dfld->interp   = FALSE;
		dfld->intlab   = FALSE;
		dfld->saved    = FALSE;
		dfld->reported = FALSE;
		<<< */
		}

	/* Now reset number of keyframes and interpolated frames */
	NumTime--;
	NumTween = 0;
	if (ifirst>=0 && ilast>ifirst)
		NumTween = (TimeList[ilast].mplus - TimeList[ifirst].mplus) /DTween + 1;
	if (NumTime <= 0)
		{
		/* >>> release_guidance() may be better */
		for (iguid=0; iguid<MaxGuid; iguid++)
			{
			GuidFlds[iguid].erase  = FALSE;
			GuidFlds[iguid].colour = -1;
			GuidFlds[iguid].style  = -1;
			GuidFlds[iguid].width  = -1;
			GuidFlds[iguid].labpos = 0;
			}
		}

	/* Check field and master links */
	(void) verify_links(TRUE);

	/* Reset interpolated link nodes */
	/* >>>>> Does this need to be done yet???  <<<<< */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			(void) interpolate_lchain(chain);

			}
		}
	for (idfld=0; idfld<NumMaster; idfld++)
		{
		dfld = MasterLinks + idfld;
		for (ichain=0; ichain<dfld->nchain; ichain++)
			{
			chain = dfld->chains[ichain];
			(void) interpolate_lchain(chain);

			}
		}

	/* Reset current times */
	if (EditTime == ivt) EditTime = -1;
	if (ViewTime == ivt) ViewTime = -1;
	if (EditTime  > ivt) EditTime--;
	if (ViewTime  > ivt) ViewTime--;
	pr_info("Editor.Sequence",
			"After deleting - Active frame: %d (Edit: %d  View: %d).\n",
			ivt, EditTime, ViewTime);

	return (-1);
	}

/***********************************************************************
*                                                                      *
*     r e v i s e _ d e p i c t _ t i m e                              *
*                                                                      *
***********************************************************************/

LOGICAL	revise_depict_time

	(
	int		ivt
	)

	{
	int		idfld, oldfrst, oldlast, newfrst, newlast;
	DFLIST	*dfld;
	LOGICAL	depict;
	FIELD	*oldtweens;
	SET		*oldlabs;
	int		itween, jtween, dtween, oldntween;

	if (ivt < 0)        return FALSE;
	if (ivt >= NumTime) return FALSE;

	/* See if there are any regular fields left at this time */
	depict = FALSE;
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		if (!tdep_normal(dfld->tdep)) continue;
		if (NotNull(dfld->frames[ivt].meta))
			{
			depict = TRUE;
			break;
			}
		}

	/* That is all if this agrees with the current depict flag */
	if ( LogicalAgree(depict, TimeList[ivt].depict) ) return FALSE;

	/* Remember the original first and last depiction time */
	/* and reset the depict flag for this time */
	oldfrst = first_depict_time();
	oldlast = last_depict_time();
	TimeList[ivt].depict = depict;
	newfrst = first_depict_time();
	newlast = last_depict_time();

	/* That is all if the first and last depiction time are not affected */
	if (newfrst==oldfrst && newlast==oldlast) return TRUE;

	/* First or last depiction time has changed... */
	/* We need to change the interpolation buffer size */
	oldntween = NumTween;
	NumTween  = 0;
	if (newfrst>=0 && newlast>newfrst)
		NumTween =
			(TimeList[newlast].mplus - TimeList[newfrst].mplus) /DTween + 1;
	dtween = 0;
	if (oldfrst>=0 && newfrst>=0)
		dtween = (TimeList[oldfrst].mplus - TimeList[newfrst].mplus) /DTween;

	/* Now we must reallocate the interpolation buffers */
	for (idfld=0; idfld<NumDfld; idfld++)
		{
		dfld = DfldList + idfld;
		if (!tdep_normal(dfld->tdep)) continue;
		if (!dfld->there)  continue;
		if (!dfld->dolink) continue;
		if (!dfld->tweens) continue;

		/* Keep a copy of the old interpolations */
		oldtweens = dfld->tweens;
		oldlabs   = dfld->tlabs;
		dfld->tweens  = NullFldList;
		dfld->tlabs   = NullSetPtr;

		/* Allocate a new buffer and fill in with the matching ones */
		if (NumTween > 0)
			{
			prepare_dfield_tweens(dfld);

			/* Move interpolations if times still match */
			for (itween=0; itween<NumTween; itween++)
				{
				jtween = itween - dtween;
				if (jtween < 0)          continue;
				if (jtween >= oldntween) continue;

				if (oldtweens) dfld->tweens[itween] = oldtweens[jtween];
				if (oldlabs)   dfld->tlabs[itween]  = oldlabs[jtween];

				if (oldtweens) oldtweens[jtween] = NullFld;
				if (oldlabs)   oldlabs[jtween]   = NullSet;
				}
			}

		/* Destroy what's left of the old interpolations */
		for (itween=0; itween<oldntween; itween++)
			{
			if (oldtweens) destroy_field(oldtweens[itween]);
			if (oldlabs)   destroy_set(oldlabs[jtween]);
			}
		FREEMEM(oldtweens);
		FREEMEM(oldlabs);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c h e c k _ d e p i c t _ t i m e s                              *
*     f i r s t _ d e p i c t _ t i m e                                *
*     l a s t _ d e p i c t _ t i m e                                  *
*     n e x t _ d e p i c t _ t i m e                                  *
*     p r e v _ d e p i c t _ t i m e                                  *
*                                                                      *
***********************************************************************/

LOGICAL	check_depict_times(void)

	{
	METAFILE	meta;
	int			ivt, idfld;

	for (ivt=0; ivt<NumTime; ivt++)
		{
		meta = NullMeta;
		for (idfld=0; idfld<NumDfld; idfld++)
			{
			if (!tdep_normal(DfldList[idfld].tdep)) continue;
			meta = DfldList[idfld].frames[ivt].meta;
			if (meta) break;
			}

		TimeList[ivt].depict = (meta)? TRUE: FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

int		first_depict_time(void)

	{
	int		ivt;

	if (NumTime <= 0) return -1;

	for (ivt=0; ivt<NumTime; ivt++)
		{
		if (TimeList[ivt].depict) return ivt;
		}

	return -1;
	}

/**********************************************************************/

int		last_depict_time(void)

	{
	int		ivt;

	if (NumTime <= 0) return -1;

	for (ivt=NumTime-1; ivt>=0; ivt--)
		{
		if (TimeList[ivt].depict) return ivt;
		}

	return -1;
	}

/**********************************************************************/

int		next_depict_time

	(
	int		cvt
	)

	{
	int		ivt;

	if (NumTime <= 0)     return -1;
	if (cvt >= NumTime-1) return -1;
	if (cvt <  0)         return first_depict_time();

	for (ivt=cvt+1; ivt<NumTime; ivt++)
		{
		if (TimeList[ivt].depict) return ivt;
		}

	return -1;
	}

/**********************************************************************/

int		prev_depict_time

	(
	int		cvt
	)

	{
	int		ivt;

	if (NumTime <= 0)     return -1;
	if (cvt <= 0)         return -1;
	if (cvt >  NumTime-1) return last_depict_time();

	for (ivt=cvt-1; ivt>=0; ivt--)
		{
		if (TimeList[ivt].depict) return ivt;
		}

	return -1;
	}

/**********************************************************************/

int		which_depict_time

	(
	STRING	vtime
	)

	{
	int		ivt;

	if (NumTime <= 0) return -1;
	if (blank(vtime)) return -1;

	for (ivt=0; ivt<NumTime; ivt++)
		{
		if (TimeList[ivt].depict
			&& same(vtime, TimeList[ivt].jtime)) return ivt;
		}

	return -1;
	}

/***********************************************************************
*                                                                      *
*     n e x t _ a c t i v e _ t i m e                                  *
*     p r e v _ a c t i v e _ t i m e                                  *
*                                                                      *
***********************************************************************/

int		next_active_time

	(
	int		cvt
	)

	{
	if (NumTime <= 0)     return -1;
	if (cvt >= NumTime-1) return -1;
	if (cvt <  0)         cvt = -1;	/* Find first */

	while (TRUE)
		{
		cvt = next_depict_time(cvt);
		if (cvt < 0) return -1;
		if (NotNull(ActiveDfld->fields[cvt])) return cvt;
		}

	return -1;
	}

/**********************************************************************/

int		prev_active_time

	(
	int		cvt
	)

	{
	if (NumTime <= 0)     return -1;
	if (cvt <= 0)         return -1;
	if (cvt >  NumTime-1) cvt = NumTime;	/* Find last */

	while (TRUE)
		{
		cvt = prev_depict_time(cvt);
		if (cvt < 0) return -1;
		if (NotNull(ActiveDfld->fields[cvt])) return cvt;
		}

	return -1;
	}

/***********************************************************************
*                                                                      *
*     s t a r t _ l i n k                                              *
*     n e x t _ l i n k                                                *
*     e x t r a p _ l i n k                                            *
*     e n d _ l i n k                                                  *
*                                                                      *
***********************************************************************/

#ifdef DEBUG_LINK
static	int	StartLinkCount = -1;
static	int	NextLinkCount = -1;
static	int	EndLinkCount = -1;
#endif /* DEBUG_LINK */

LOGICAL	start_link

	(
	LOGICAL	forward
	)

	{
	LOGICAL	nofld, nodata;
	char	xbuf[64];

#	ifdef DEBUG_LINK
	int	LinkCount;
#	endif /* DEBUG_LINK */

	(void) end_link();

	if (EditTime < 0)        return FALSE;
	if (EditTime >= NumTime) return FALSE;
	if (!DepictShown)        return FALSE;
	if (!LinkShown)          return FALSE;

#	ifdef DEBUG_LINK
	StartLinkCount++;
	LinkCount = StartLinkCount;
	pr_diag("Timelink", "[start_link] Begin: %d\n", LinkCount);
#	endif /* DEBUG_LINK */

	/* Make sure there is somthing to link */
	nofld  = FALSE;
	nodata = FALSE;
	switch (CurrEditor)
		{
		case FpaC_VECTOR:
		case FpaC_CONTINUOUS:
			if (IsNull(NewSurface)) nofld  = TRUE;
			break;

		case FpaC_DISCRETE:
		case FpaC_WIND:
			if (IsNull(NewAreas))        nofld  = TRUE;
			else if (NewAreas->num <= 0) nodata = TRUE;
			break;

		case FpaC_LINE:
			if (IsNull(NewCurves))        nofld  = TRUE;
			else if (NewCurves->num <= 0) nodata = TRUE;
			break;

		case FpaC_SCATTERED:
			if (IsNull(NewPoints))        nofld  = TRUE;
			else if (NewPoints->num <= 0) nodata = TRUE;
			break;
		}

	/* Check for missing field or data */
	if (nofld)
		{
		(void) sprintf(xbuf, "%s %s",
			TimeList[EditTime].jtime, "NO_FIELD");
		linking_chart(xbuf);
		return FALSE;
		}
	if (nodata)
		{
		(void) sprintf(xbuf, "%s %s",
			TimeList[EditTime].jtime, "NO_DATA");
		linking_chart(xbuf);
		return FALSE;
		}

	/* Already showing the correct valid time - so just set mode */
	LinkVtime = ViewTime;
	LinkEtime = EditTime;
	LinkGtime = GrpTime;
	LinkDir   = (forward)? 1: -1;
	empty_temp();

#	ifdef DEBUG_LINK
	pr_diag("Timelink", "[start_link] Got to end of %d OK!\n", LinkCount);
#	endif /* DEBUG_LINK */

	return TRUE;
	}

/**********************************************************************/

LOGICAL	next_link

	(
	LCHAIN	chain
	)

	{
	int		next;
	FRAME	*dframe;
	FIELD	fld;
	SET		set;

#	ifdef DEBUG_LINK
	int	LinkCount;
#	endif /* DEBUG_LINK */

	if (LinkDir == 0) return FALSE;

#	ifdef DEBUG_LINK
	NextLinkCount++;
	LinkCount = NextLinkCount;
	pr_diag("Timelink", "[next_link] Begin: %d\n", LinkCount);
#	endif /* DEBUG_LINK */

	empty_metafile(TempMeta);
	(void) build_link_chain(ActiveDfld, chain, TRUE);
	(void) present_temp(TRUE);

	/* Make sure there is another chart to link */
	/* Skip over absent charts */
	next = EditTime;
	while (TRUE)
		{
		next += SIGN(LinkDir);
		if (next < 0)        return FALSE;
		if (next >= NumTime) return FALSE;
		if (NotNull(ActiveDfld->frames) &&
			NotNull(ActiveDfld->frames[next].meta)) break;
		if (same(CurrElement, "MASTER_LINK"))
			{
			if (TimeList[next].depict) break;
			}
		}

	/* Make sure there is somthing to link */
	if (CurrEditor == FpaC_DISCRETE ||
		CurrEditor == FpaC_WIND     ||
		CurrEditor == FpaC_LINE     ||
		CurrEditor == FpaC_SCATTERED)
		{
		fld = ActiveDfld->fields[next];
		if (IsNull(fld))            return FALSE;
		if (fld->ftype != FtypeSet) return FALSE;
		set = fld->data.set;
		if (IsNull(set))            return FALSE;
		if (set->num <= 0)          return FALSE;
		}

	/* Show next chart to be linked */
	busy_cursor(TRUE);
	(void) ignore_partial();
	(void) hide_edit_field();
	(void) release_links();
	(void) release_special_tags();
	(void) release_edit_field(FALSE);
	(void) set_active_time(next, next, next);
	if (NotNull(ActiveDfld->frames))
		{
		/* Display if not yet contoured */
		dframe = ActiveDfld->frames + EditTime;
		if (!dframe->contoured) (void) present_all();
		}
	(void) extract_edit_field();
	(void) extract_special_tags();
	(void) extract_links();
	(void) show_edit_field();
	(void) post_partial(TRUE);
	busy_cursor(FALSE);

#	ifdef DEBUG_LINK
	pr_diag("Timelink", "[next_link] Got to end of %d OK!\n", LinkCount);
#	endif /* DEBUG_LINK */

	return TRUE;
	}

/**********************************************************************/

LOGICAL	extrap_link(void)

	{
	linking_chart("EXTRAP");
	return TRUE;
	}

/**********************************************************************/

LOGICAL	new_link(void)

	{

	if (LinkDir == 0) return TRUE;

	linking_chart("NEW");
	return TRUE;
	}

/**********************************************************************/

LOGICAL	end_link(void)

	{
	FRAME	*dframe;

#	ifdef DEBUG_LINK
	int	LinkCount;
#	endif /* DEBUG_LINK */

	if (LinkDir == 0) return TRUE;

#	ifdef DEBUG_LINK
	EndLinkCount++;
	LinkCount = EndLinkCount;
	pr_diag("Timelink", "[end_link] Begin: %d\n", LinkCount);
#	endif /* DEBUG_LINK */

	/* Show original active chart */
	if (LinkEtime != EditTime && LinkVtime != ViewTime)
		{
		busy_cursor(TRUE);
		empty_metafile(TempMeta);

		(void) ignore_partial();
		(void) hide_edit_field();
		(void) release_links();
		(void) release_special_tags();
		(void) release_edit_field(FALSE);
		(void) set_active_time(LinkVtime, LinkEtime, LinkGtime);
		if (NotNull(ActiveDfld->frames))
			{
			/* Display if not yet contoured */
			dframe = ActiveDfld->frames + EditTime;
			if (!dframe->contoured) (void) present_all();
			}
		(void) extract_edit_field();
		(void) extract_special_tags();
		(void) extract_links();
		(void) show_edit_field();

		linking_chart("");
		(void) present_all();

		busy_cursor(FALSE);
		}

	LinkVtime = -1;
	LinkEtime = -1;
	LinkGtime = -1;
	LinkDir   = 0;

#	ifdef DEBUG_LINK
	pr_diag("Timelink", "[end_link] Got to end of %d OK!\n", LinkCount);
#	endif /* DEBUG_LINK */

	return TRUE;
	}
