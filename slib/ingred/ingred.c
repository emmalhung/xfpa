/***********************************************************************
*                                                                      *
*  i n g r e d . c                                                     *
*                                                                      *
*  Interactive Graphics Editor module of FPA.                          *
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

#define INGRED_MASTER
#include "ingred_private.h"
#include "ingred.h"
/* >>>>> december addition <<<<< */
#include <X11/keysym.h>
/* >>>>> december addition <<<<< */
#include <time.h>
#include <sys/stat.h>

#undef DEBUG_EVENTS

/* Internal functions */
static	LOGICAL	OpenGraphics(void);
static	LOGICAL	CloseGraphics(void);
static	LOGICAL	ResizeGraphics(void);
static	LOGICAL	ObtainDblock(void);
static	LOGICAL	ReleaseDblock(void);
static	void	UpdateDblock(XtPointer, XtIntervalId *);
static	void	EventHandler(Widget, XtPointer, XEvent *, LOGICAL *);
static	GEREPLY	ContinueEdit(void);
static	LOGICAL	PerformEdit(STRING);

/* Internal variables */
static	LOGICAL	GraphicsOn = FALSE;
static	int		Gwidth     = 0;
static	int		Gheight    = 0;
static	LOGICAL	ArgOK      = FALSE;
static	char	Line[265]  = "";
static	STRING	Command    = NULL;
static	STRING	Mode       = NULL;

/*******************************************************************************
*                                                                              *
*     INTERFACE FUNCTIONS:                                                     *
*                                                                              *
*     The following functions (beginning with "GE") are the only advertised    *
*     external interface to this library.                                      *
*                                                                              *
*******************************************************************************/

/*******************************************************************************
*                                                                              *
*     G E C o n n e c t                                                        *
*     G E D i s c o n n e c t                                                  *
*                                                                              *
*******************************************************************************/

GEREPLY			GEConnect

	(
	XtAppContext	appContext,
	Widget			drawingWindow,
	void			(*CursorFcn)(STRING, LOGICAL),
	void			(*MessageFcn)(STRING, STRING),
	void			(*StatusFcn)(STRING, CAL)
	)

	{
	STRING	demo;

	pr_status("Editor.API", "GEConnect: Graphics editor connected.\n");
	put_message("hello");

	demo = getenv("FPA_DEMO_DATE");
	if (!blank(demo))
		{
		pr_status("Editor", "Demo date set to: %s.\n", demo);
		}

	ConserveMem = XuGetBooleanResource(".conserveMemory", True);
	if (!ConserveMem)
		{
		pr_status("Editor", "Large memory buffers will be retained.\n");
		}

	X_appcon  = appContext;
	X_widget  = drawingWindow;
	X_display = XtDisplay(drawingWindow);
	X_window  = XtWindow(drawingWindow);

	XSync(X_display, 0);

	HomeDir = work_directory();

	/* Set up function pointers */
	define_feedback(CursorFcn, MessageFcn, StatusFcn);

	/* Set up Xgl graphics */
	if (!OpenGraphics())
		{
		pr_error("Editor", "Problem with graphics startup - Aborting!\n");
		exit(1);
		}
	put_message("startup");

	/* Interested in button up and motion events and Escape key */
	/* >>>>> december addition <<<<< */
	XtAddEventHandler(X_widget,
				  ExposureMask | StructureNotifyMask | VisibilityChangeMask |
				  KeyPressMask | ButtonPressMask | ButtonReleaseMask,
				  FALSE, EventHandler, NULL);
	/* >>>>> december addition <<<<< */

	/* Set up depiction sequence and graphics panels */
	set_metafile_input_mode(MetaNoRetainPlot);	/* Temporary plot support */
	init_sequence();
	setup_panels();

	tell_active_status();
	return GE_VALID;
	}

/******************************************************************************/

GEREPLY	GEDisconnect(void)

	{
	if (EditState != EDIT_SUSPEND)
		{
		pr_status("Editor", "Terminating graphics editor.\n");
		(void) PerformEdit("confirm");
		(void) CloseGraphics();
		}
	pr_status("Editor.API", "GEDisonnect: Graphics editor shutting down.\n");

	/* Remove database lock if there */
	(void) ReleaseDblock();

	/* Remove event handler */
	XtRemoveEventHandler(X_widget, XtAllEvents, TRUE, EventHandler, NULL);

	put_message("bye");
	return GE_VALID;
	}


/*******************************************************************************
*                                                                              *
*     G E S t a t u s                                                          *
*                                                                              *
*******************************************************************************/

GEREPLY	GEStatus

	(
	STRING	cmdstring,
	int		*nparm,
	STRING	**plist1,
	STRING	**plist2,
	CAL		*cal
	)

	{
	LOGICAL		status = FALSE;
	int			idfld, itime, itween, iparm, jparm, ifirst, snew;
	STRING		val, group, vt;
	char		vtime[40], element[128], level[128], arg[40];
	LOGICAL		fonly;
	METAFILE	meta;
	DFLIST		*dfld;

	static	int		smem    = 0;
	static	STRING	*slist1 = NULL;
	static	STRING	*slist2 = NULL;
	static	STRING	*vtween = NULL;

	pr_info("Editor.API", "GEStatus: %s.\n", cmdstring);
	if (plist1) *plist1 = NULL;
	if (plist2) *plist2 = NULL;
	if (cal)    *cal    = NullCal;
	if (nparm)  *nparm  = 0;
	status = FALSE;

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}

	/* Clean up the return lists from previous request */
	snew = smem;
	snew = MAX(snew, (2*NumTime));
	snew = MAX(snew, NumDfld);
	snew = MAX(snew, NumTween);
	if (snew > smem)
		{
		smem   = snew;
		slist1 = GETMEM(slist1, STRING, smem);
		slist2 = GETMEM(slist2, STRING, smem);
		}
	for (iparm=0; iparm<smem; iparm++)
		{
		slist1[iparm] = NULL;
		slist2[iparm] = NULL;
		}

	/* Remove interpolated time strings (if required) */
	if ( NotNull(vtween) ) FREELIST(vtween, NumTween);

	/* Status info about depictions */
	if (same(Command, "DEPICT"))
		{
		Mode = string_arg(Line);

		/* Provide "complete depiction" time list */
		if (same(Mode, "TIMES"))
			{
			iparm = 0;
			for (itime=0; itime<NumTime; itime++)
				{
				if (!TimeList[itime].depict) continue;

				slist1[iparm] = TimeList[itime].jtime;
				pr_diag("Editor", "%s\n", slist1[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = NULL;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide "modified depiction" time list */
		else if (same(Mode, "MODIFIED"))
			{
			iparm = 0;
			for (itime=0; itime<NumTime; itime++)
				{
				if (!TimeList[itime].depict) continue;

				for (idfld=0; idfld<NumDfld; idfld++)
					{
					/* Only return time if a regular field is present */
					dfld = DfldList + idfld;
					if (!dfld->there)                  continue;
					if (!tdep_normal(dfld->tdep))      continue;
					if (!dfld->frames[itime].meta)     continue;
					if (!dfld->frames[itime].modified) continue;

					slist1[iparm] = TimeList[itime].jtime;
					pr_diag("Editor", "%s\n", slist1[iparm]);
					iparm++;

					/* One is enough */
					break;
					}
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = NULL;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}
		}

	/* Status info about a field */
	else if (same(Command, "FIELD"))
		{
		Mode = string_arg(Line);

		/* Provide time list for given field */
		/* Secondary list contains the appropriate time to show with the */
		/* current view time */
		if (same(Mode, "TIMES"))
			{
			strcpy_arg(element, Line, &ArgOK);
			strcpy_arg(level,   Line, &ArgOK);

			dfld = find_dfield(element, level);
			if (IsNull(dfld)) return GE_INVALID;

			itime = pick_dfield_frame(dfld, ViewTime);
			val   = (itime >= 0)? TimeList[itime].jtime: NULL;
			iparm = 0;
			for (itime=0; itime<NumTime; itime++)
				{
				/* Only return time if the field is present */
				if (!dfld->there)                     break;
				if (IsNull(dfld->frames[itime].meta)) continue;

				slist1[iparm] = TimeList[itime].jtime;
				slist2[iparm] = val;
				if (NotNull(val))
					pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				else
					pr_diag("Editor", "%s\n", slist1[iparm]);
				iparm++;

				/* Only need secondary active time in first member */
				val = NULL;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide modified time list for given field */
		else if (same(Mode, "MODIFIED"))
			{
			strcpy_arg(element, Line, &ArgOK);
			strcpy_arg(level,   Line, &ArgOK);

			dfld = find_dfield(element, level);
			if (IsNull(dfld)) return GE_INVALID;

			iparm = 0;
			for (itime=0; itime<NumTime; itime++)
				{
				/* Only return time if the field is present and modified */
				if (!dfld->there)                     break;
				if (IsNull(dfld->frames[itime].meta)) continue;
				if (!dfld->frames[itime].modified)    continue;

				slist1[iparm] = TimeList[itime].jtime;
				pr_diag("Editor", "%s\n", slist1[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = NULL;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide active background values for given field */
		else if (same(Mode, "BACKGROUND"))
			{
			strcpy_arg(element, Line, &ArgOK);
			strcpy_arg(level,   Line, &ArgOK);

			dfld = find_dfield(element, level);
			if (IsNull(dfld)) return GE_INVALID;

			if (cal) *cal = dfld->bgcal;
			status = TRUE;
			}

		/* Provide all the link information for the given field */
		else if (same(Mode, "LINK_INFO"))
			{
			strcpy_arg(element, Line, &ArgOK);
			strcpy_arg(level,   Line, &ArgOK);

			dfld = find_dfield(element, level);
			if (IsNull(dfld)) return GE_INVALID;

			iparm = 0;
			if (!dfld->dolink && !tdep_special(dfld->tdep))
									   slist1[iparm] = "NOT_LINKABLE";
			else if (dfld->interp && dfld->intlab)
									   slist1[iparm] = "INTERP";
			else if (dfld->interp)     slist1[iparm] = "FIELD";
			else if (dfld->linked)     slist1[iparm] = "LINKED";
			else if (dfld->nchain > 0) slist1[iparm] = "PARTIAL";
			else                       slist1[iparm] = "NONE";
			if (dfld->linkto)
				{
				if (same(dfld->linkto->element, "MASTER_LINK"))
					{
					slist2[iparm] = "MASTER_LINK";
					pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
					iparm++;
					}
				else
					{
					slist2[iparm] = "FIELD";
					pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
					iparm++;
					slist1[iparm] = dfld->linkto->element;
					slist2[iparm] = dfld->linkto->level;
					pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
					iparm++;
					}
				}
			else
				{
				slist2[iparm] = "SELF";
				pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}
		}

	/* Status info about fields */
	else if (same(Command, "FIELDS"))
		{
		Mode = string_arg(Line);

		/* Provide "complete field" list */
		if (blank(Mode) || valid_tstamp(Mode))
			{
			(void) safe_strcpy(vtime, Mode);
			if (blank(vtime)) itime = -1;
			else              itime = find_valid_time(vtime);

			iparm = 0;
			for (idfld=0; idfld<NumDfld; idfld++)
				{
				dfld = DfldList + idfld;

				/* If we want one valid time, make sure the field is present */
				if (itime >= 0)
					{
					meta = dfld->frames[itime].meta;
					if (IsNull(meta))      continue;
					if (meta->numfld <= 0) continue;
					}

				if (!dfld->there) continue;
				slist1[iparm] = dfld->element;
				slist2[iparm] = dfld->level;
				pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide field for which an edit has been posted */
		else if (same(Mode, "EDIT_POSTED"))
			{
			iparm = 0;
			if (edit_posted())
				{
				slist1[iparm] = CurrElement;
				slist2[iparm] = CurrLevel;
				pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide "linkable field" list */
		else if (same(Mode, "LINKABLE"))
			{
			iparm = 0;
			for (idfld=0; idfld<NumDfld; idfld++)
				{
				dfld = DfldList + idfld;
				if (!dfld->there)  continue;
				if (!dfld->dolink) continue;
				slist1[iparm] = dfld->element;
				slist2[iparm] = dfld->level;
				pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide "linked field" list */
		else if (same(Mode, "LINKED"))
			{
			iparm = 0;
			for (idfld=0; idfld<NumDfld; idfld++)
				{
				dfld = DfldList + idfld;
				if (!dfld->there)  continue;
				if (!dfld->dolink && !tdep_special(dfld->tdep)) continue;
				verify_dfield_links(dfld, FALSE);
				verify_dfield_interp(dfld, FALSE, FALSE);
				if (!dfld->linked) continue;
				slist1[iparm] = dfld->element;
				slist2[iparm] = dfld->level;
				pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}

		/* Provide "interpolated field" list */
		else if (same(Mode, "INTERPOLATED"))
			{
			strcpy_arg(arg, Line, &ArgOK);
			fonly = same(arg, "FIELD");

			iparm = 0;
			for (idfld=0; idfld<NumDfld; idfld++)
				{
				dfld = DfldList + idfld;
				if (!dfld->there)  continue;
				if (!dfld->dolink && !tdep_special(dfld->tdep)) continue;
				verify_dfield_links(dfld, FALSE);
				verify_dfield_interp(dfld, FALSE, FALSE);
				if (fonly)
					{
					if (!dfld->interp) continue;
					if ( dfld->intlab) continue;
					}
				else
					{
					if (!dfld->interp) continue;
					if (!dfld->intlab) continue;
					}
				slist1[iparm] = dfld->element;
				slist2[iparm] = dfld->level;
				pr_diag("Editor", "%s %s\n", slist1[iparm], slist2[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = slist2;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}
		}

	/* Status info about a group */
	else if (same(Command, "GROUP"))
		{
		Mode = string_arg(Line);

		/* Provide status of group (or global) master link */
		if (same(Mode, "MASTER_LINK_STATUS"))
			{
			strcpy_arg(level, Line, &ArgOK);

			if (same(level, "GLOBAL")) group = NULL;
			else                       group = level;
			dfld = find_master_dfield(group);
			iparm = 0;
			if (dfld->linked)          slist1[iparm] = "LINKED";
			else if (dfld->nchain > 0) slist1[iparm] = "PARTIAL";
			else                       slist1[iparm] = "NONE";
			pr_diag("Editor", "%s\n", slist1[iparm]);
			iparm++;

			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = NULL;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}
		}

	/* Status info about groups */
	else if (same(Command, "GROUPS"))
		{
		Mode = string_arg(Line);

		/* Provide "complete group" list */
		if (blank(Mode) || valid_tstamp(Mode))
			{
			(void) safe_strcpy(vtime, Mode);
			if (blank(vtime)) itime = -1;
			else              itime = find_valid_time(vtime);

			iparm = 0;
			for (idfld=0; idfld<NumDfld; idfld++)
				{
				dfld = DfldList + idfld;
				if (!dfld->there) continue;
				group = dfld->group;

				/* If we want one valid time, make sure the field is present */
				if (itime >= 0)
					{
					meta = dfld->frames[itime].meta;
					if (IsNull(meta))      continue;
					if (meta->numfld <= 0) continue;
					}

				/* Make sure we don't already have this group */
				for (jparm=0; jparm<iparm; jparm++)
					{
					if (same(group, slist1[jparm])) break;
					}
				if (jparm < iparm) continue;

				slist1[iparm] = group;
				pr_diag("Editor", "%s\n", slist1[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = NULL;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}
		}

	/* Status info about interpolations */
	else if (same(Command, "INTERP"))
		{
		Mode = string_arg(Line);

		/* Provide "complete interpolation" time list */
		if (same(Mode, "TIMES"))
			{
			iparm  = 0;
			ifirst = first_depict_time();
			vtween = INITMEM(STRING, NumTween);
			for (itween=0; itween<NumTween; itween++)
				{
				vt = calc_valid_time_minutes(TimeList[ifirst].jtime,
						0, itween*DTween);
				vtween[itween] = safe_strdup(vt);
				slist1[iparm]  = vtween[itween];
				pr_diag("Editor", "%s\n", slist1[iparm]);
				iparm++;
				}
			if (plist1) *plist1 = slist1;
			if (plist2) *plist2 = NULL;
			if (nparm)  *nparm  = iparm;
			status = TRUE;
			}
		}

	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E A c t i o n                                                          *
*                                                                              *
*******************************************************************************/

GEREPLY GEAction

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status = FALSE;
	char	mapname[256];
	char	reread[20], state[20], stmode[20], smmode[20];
	char	defaults[20], units[64], label[64];
	char	vtime[40], rastfmt[20], rastmode[20], padmode[20];
	char	dfile[256], template[256];
	STRING	message;
	int		nmapov, mapov, width, height;
	float	spread, smooth, psize, wmin, wmax;

	pr_info("Editor.API", "GEAction: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;

	/* Handle "mode" command */
	if (same(Command, "MODE"))
		{
		Mode = string_arg(Line);
		if (same(Mode, "START_UP"))
			{
			EditState = EDIT_START_UP;
			status    = TRUE;
			}

		else if (same(Mode, "NORMAL"))
			{
			if (EditState == EDIT_SUSPEND)
				{
				/* Restart graphics */
				pr_status("Editor", "Restarting graphics editor.\n");
				(void) OpenGraphics();
				resume_feedback();
				}

			if (!SequenceReady)
				{
				SequenceReady = TRUE;
				show_depiction();
				}

			if (EditState == EDIT_START_UP)
				{
				Visible    = FALSE;
				AllowInput = IgnoreObsc;
				obscure_cursor(!AllowInput);
				obscure_message(!AllowInput);
				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				}

			EditState = EDIT_NORMAL;
			status    = TRUE;
			}

		else if (same(Mode, "SUSPEND"))
			{
			if (EditState != EDIT_SUSPEND)
				{
				pr_status("Editor", "Suspending graphics editor.\n");
				message = string_arg(Line);
				if (!blank(message)) string_message("status", message);
				else                 put_message("suspend");
				suspend_feedback();
				(void) PerformEdit("confirm");
				stop_animation();
				(void) CloseGraphics();
				}

			EditState = EDIT_SUSPEND;
			status    = TRUE;
			}
		}

	/* Handle "map" command */
	else if (same(Command, "MAP"))
		{
		Mode = string_arg(Line);
		if (same(Mode, "BASE"))
			{
			strcpy_arg(mapname, Line, &ArgOK);
			nmapov = int_arg(Line, &ArgOK);

			busy_cursor(TRUE);
			status = map_background(mapname, nmapov);
			busy_cursor(FALSE);
			}

		else if (same(Mode, "OVERLAY"))
			{
			mapov = int_arg(Line, &ArgOK);
			strcpy_arg(mapname, Line, &ArgOK);
			strcpy_arg(reread,  Line, &ArgOK);

			busy_cursor(TRUE);
			status = map_overlay(mapov, mapname, reread);
			(void) PerformEdit("resume");
			busy_cursor(FALSE);
			}

		else if (same(Mode, "PALETTE"))
			{
			busy_cursor(TRUE);
			status = map_palette(Line);
			busy_cursor(FALSE);
			}
		}

	/* Handle "link" command */
	else if (same(Command, "LINK"))
		{
		Mode = string_arg(Line);
		if (same(Mode, "PALETTE"))
			{
			busy_cursor(TRUE);
			status = link_palette(Line);
			busy_cursor(FALSE);
			}
		}

	/* Handle "state" command */
	else if (same(Command, "STATE"))
		{
		strcpy_arg(state,  Line, &ArgOK);
		strcpy_arg(stmode, Line, &ArgOK);

		if (same(state, "DRAW_MODE"))
			{
			if (same(stmode, "CONT"))
				{
				DrawMode = DRAW_CONT;
				DrawSmth = 100;
				status   = TRUE;
				}
			else if (same(stmode, "PPS"))
				{
				DrawMode = DRAW_PTPT;
				DrawSmth = 100;
				status   = TRUE;
				}
			else
				{
				status = FALSE;
				}

			if (status)
				{
				strcpy_arg(smmode, Line, &ArgOK);
				if (sscanf(smmode, "%g", &smooth) == 1)
					{
					DrawSmth = smooth;
					}
				set_Xcurve_modes("draw");
				}
			}

		else if (same(state, "MERGE_MODE"))
			{
			if (same(stmode, "FIELD"))
				{
				MergeMode = MERGE_FIELD;
				status    = TRUE;
				}
			else if (same(stmode, "FIELD_AND_LABELS"))
				{
				MergeMode = MERGE_FIELD_AND_LABELS;
				status    = TRUE;
				}
			else
				{
				status = FALSE;
				}
			(void) PerformEdit("resume");
			return (status)? GE_VALID: GE_INVALID;
			}

		else if (same(state, "MOVE_MODE"))
			{
			if (same(stmode, "FIELD"))
				{
				MoveMode = MOVE_FIELD;
				status    = TRUE;
				}
			else if (same(stmode, "FIELD_AND_LABELS"))
				{
				MoveMode = MOVE_FIELD_AND_LABELS;
				status    = TRUE;
				}
			else
				{
				status = FALSE;
				}
			(void) PerformEdit("resume");
			return (status)? GE_VALID: GE_INVALID;
			}

		else if (same(state, "MODIFY_MODE"))
			{
			if (same(stmode, "CONT"))
				{
				ModifyMode = MODIFY_CONT;
				ModifySmth = 100;
				status     = TRUE;
				}
			else if (same(stmode, "PPS"))
				{
				ModifyMode = MODIFY_PTPT;
				ModifySmth = 100;
				status     = TRUE;
				}
			else if (same(stmode, "PUCK"))
				{
				ModifyMode = MODIFY_PUCK;
				ModifySmth = 0;
				ModifyPuck = 50;
				strcpy_arg(smmode, Line, &ArgOK);
				if (sscanf(smmode, "%g", &psize) == 1)
					{
					ModifyPuck = psize;
					}
				status     = TRUE;
				}
			else
				{
				status = FALSE;
				}

			if (status)
				{
				strcpy_arg(smmode, Line, &ArgOK);
				if (sscanf(smmode, "%g", &smooth) == 1)
					{
					ModifySmth = smooth;
					}
				set_Xcurve_modes("modify");
				}
			}

		else if (same(state, "SMOOTHING_AMOUNT"))
			{
			if (sscanf(stmode, "%g", &smooth) == 1)
				{
				SplineSmth = smooth;
				status     = TRUE;
				}
			else
				{
				status = FALSE;
				}
			(void) PerformEdit("resume");
			return (status)? GE_VALID: GE_INVALID;
			}

		else if (same(state, "SPREAD"))
			{
			if (sscanf(stmode, "%g", &spread) == 1)
				{
				SpreadFact = MAX(0, MIN(spread, 100));
				status     = TRUE;
				}
			else
				{
				status = FALSE;
				}
			}

		else if (same(state, "STACK_ORDER"))
			{
			if (same(stmode, "TOP"))
				{
				StackMode = STACK_TOP;
				status    = TRUE;
				}
			else if (same(stmode, "BOTTOM"))
				{
				StackMode = STACK_BOTTOM;
				status    = TRUE;
				}
			else
				{
				status = FALSE;
				}
			}

		else if (same(state, "LINK_MODE"))
			{
			if (same(stmode, "NORMAL"))
				{
				LinkMode = LINK_NORMAL;
				status    = TRUE;
				}
			else if (same(stmode, "INTERMEDIATE"))
				{
				LinkMode = LINK_INTERMEDIATE;
				status    = TRUE;
				}
			else
				{
				status = FALSE;
				}
			(void) PerformEdit("resume");
			return (status)? GE_VALID: GE_INVALID;
			}

		else
			{
			status = FALSE;
			}
		}

	/* Handle "defaults" command */
	else if (same(Command, "DEFAULTS"))
		{
		strcpy_arg(defaults, Line, &ArgOK);

		if (same(defaults, "WINDS"))
			{
			wmin = float_arg(Line, &ArgOK); wmin = ArgOK? wmin: -999;
			wmax = float_arg(Line, &ArgOK); wmax = ArgOK? wmax: -999;
			strcpy_arg(units, Line, &ArgOK);
			if (ArgOK)
				{
				if (!same(units, "-")) (void) strcpy(WindDisplay, units);
				(void) gxSetWindParms(wmin, wmax, WindDisplay, WindUnits);
				pr_status("Editor",
					"Display units for winds: %s\n", WindDisplay);
				pr_status("Editor",
					"Default units for winds: %s\n", WindUnits);
				status = TRUE;
				}
			else
				{
				status = FALSE;
				}
			}

		else if (same(defaults, "TIMELINK"))
			{
			strcpy_arg(units, Line, &ArgOK);
			strcpy_arg(label, Line, &ArgOK);
			if (ArgOK)
				{
				if (!same(units, "-")) (void) strcpy(SpdUnits, units);
				if (!same(label, "-")) (void) strcpy(SpdLabel, label);
				status = TRUE;
				}
			else
				{
				status = FALSE;
				}
			}

		else
			{
			status = FALSE;
			}
		}

	/* Handle "raster_dump" command */
	else if (same(Command, "RASTER_DUMP"))
		{
		strcpy_arg(rastfmt, Line, &ArgOK);
		strcpy_arg(rastmode, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);
		width  = int_arg(Line, &ArgOK);
		height = int_arg(Line, &ArgOK);
		strcpy_arg(dfile, Line, &ArgOK);
		strcpy_arg(padmode, Line, &ArgOK);
		strcpy_arg(template, Line, &ArgOK);

		busy_cursor(TRUE);
		status = raster_dump(rastfmt, rastmode, vtime, width, height, padmode,
							dfile, template);
		busy_cursor(FALSE);
		}

	/* Force a redisplay of the graphics */
	else if (same(Command, "REDISPLAY"))
		{
		ResizeGraphics();
		reset_panels();
		if (DepictShown || LinkShown)
			{
			(void) extract_special_tags();
			(void) extract_links();
			}
		if (LinkShown)
			{
			(void) extract_unlinked();
			}
		(void) present_all();
		}

	/* Handle "FONT" command */
	else if (same(Command, "FONT"))
		{
		Mode = string_arg(Line);
		if(same(Mode, "ADD"))
			{
			/* Note that the font descriptor can have embedded spaces */
			/* so do not use string_arg, just pass the remainder in.  */
			status = gxAddFont(string_arg(Line), Line);
			}
		}

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E Z o o m                                                              *
*                                                                              *
*******************************************************************************/

GEREPLY GEZoom

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status;
	char	xbuf[20], ybuf[20], wbuf[20], hbuf[20];

	pr_info("Editor.API", "GEZoom: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;

	/* Handle zoom "in" command */
	if (same(Command, "IN"))
	    {
		(void) PerformEdit("confirm");
		Module = MODULE_ZOOM;
		Editor = EDITOR_ZOOM;
		(void) PerformEdit("begin");
		return GE_VALID;
	    }

	/* Handle zoom "out" command */
	else if (same(Command, "OUT"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_ZOOM;
		Editor = EDITOR_NONE;
	    status = zoom_out();
		busy_cursor(FALSE);
	    }

	/* Handle "pan" command */
	else if (same(Command, "PAN"))
	    {
		(void) PerformEdit("confirm");
		Module = MODULE_ZOOM;
		Editor = EDITOR_PAN;
		(void) PerformEdit("begin");
		return GE_VALID;
	    }

	/* Handle "pan_done" command */
	else if (same(Command, "PAN_DONE"))
	    {
		(void) PerformEdit("confirm");
		Module = MODULE_ZOOM;
		Editor = EDITOR_PAN;
		(void) PerformEdit("pan_done");
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
		return GE_VALID;
	    }

	/* Handle "area" command */
	else if (same(Command, "AREA") || same(Command, "MOVE"))
	    {
		strcpy_arg(xbuf, Line, &ArgOK);
		strcpy_arg(ybuf, Line, &ArgOK);
		strcpy_arg(wbuf, Line, &ArgOK);
		strcpy_arg(hbuf, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_ZOOM;
		Editor = EDITOR_NONE;
	    status = zoom_reset(xbuf, ybuf, wbuf, hbuf);
		busy_cursor(FALSE);
	    }

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E A n i m a t e                                                        *
*                                                                              *
*******************************************************************************/

GEREPLY GEAnimate

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status;
	char	element[128], level[128], mode[30], svt[40], evt[40], vtime[40];
	int		delay;

	pr_info("Editor.API", "GEAnimate: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;

	/* Handle "enter" command */
	if (same(Command, "ENTER"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		(void) release_guidance();
		Module = MODULE_ANIM;
		Editor = EDITOR_NONE;
	    status = init_animation();
		busy_cursor(FALSE);
	    }

	/* Handle "zoom" command */
	else if (same(Command, "ZOOM"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_ANIM;
		Editor = EDITOR_NONE;
	    status = suspend_animation();
		busy_cursor(FALSE);
	    }

	/* Handle "exit" command */
	else if (same(Command, "EXIT"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_ANIM;
		Editor = EDITOR_NONE;
	    status = exit_animation();
		busy_cursor(FALSE);
	    }

	/* Handle "show" command */
	else if (same(Command, "SHOW"))
	    {
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_ANIM;
		Editor = EDITOR_NONE;
	    status = animation_frame(vtime);
		busy_cursor(FALSE);
		}

	/* Handle "start" command */
	else if (same(Command, "START"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_ANIM;
		Editor = EDITOR_NONE;
	    status = start_animation();
		busy_cursor(FALSE);
	    }

	/* Handle "stop" command */
	else if (same(Command, "STOP"))
	    {
		busy_cursor(TRUE);
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
	    status = stop_animation();
		busy_cursor(FALSE);
	    }

	/* Handle "mode" command */
	else if (same(Command, "MODE"))
	    {
		strcpy_arg(mode, Line, &ArgOK);

		(void) strcpy(svt, FpaCblank);
		(void) strcpy(evt, FpaCblank);
		if ( !blank(Line) )
			{
			strcpy_arg(svt, Line, &ArgOK);
			strcpy_arg(evt, Line, &ArgOK);
			}

		busy_cursor(TRUE);
	    status = animation_mode(mode, svt, evt);
		busy_cursor(FALSE);
	    }

	/* Handle "delay" command */
	else if (same(Command, "DELAY"))
	    {
		delay = int_arg(Line, &ArgOK);

		busy_cursor(TRUE);
	    status = animation_delay(delay);
		busy_cursor(FALSE);
	    }

	/* Handle "field_visibility" command */
	else if (same(Command, "FIELD_VISIBILITY"))
	    {
		strcpy_arg(element, Line, &ArgOK);
		strcpy_arg(level,   Line, &ArgOK);
		strcpy_arg(mode,    Line, &ArgOK);

		busy_cursor(TRUE);
	    status = animation_dfield_state(element, level, mode);
		busy_cursor(FALSE);
	    }

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E D e p i c t i o n                                                    *
*                                                                              *
*******************************************************************************/

GEREPLY GEDepiction

	(
	STRING	cmdstring,
	CAL		cal,
	CAL		calx
	)

	{
	LOGICAL	status;
	char	group[128], element[128], level[128], mode[30];

	pr_info("Editor.API", "GEDepiction: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;

	/* Handle "show" command */
	if (same(Command, "SHOW"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = show_depiction();
		busy_cursor(FALSE);
	    }

	/* Handle "hide" command */
	else if (same(Command, "HIDE"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = hide_depiction();
		busy_cursor(FALSE);
	    }

	/* Handle "field" command */
	else if (same(Command, "FIELD"))
	    {
		strcpy_arg(element, Line, &ArgOK);
		strcpy_arg(level,   Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = active_edit_field(element, level);
		busy_cursor(FALSE);
		}

	/* Handle "group" command */
	else if (same(Command, "GROUP"))
	    {
		strcpy_arg(group, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = active_edit_group(group);
		busy_cursor(FALSE);
		}

	/* Handle "field_visibility" command */
	else if (same(Command, "FIELD_VISIBILITY"))
	    {
		strcpy_arg(element, Line, &ArgOK);
		strcpy_arg(level,   Line, &ArgOK);
		strcpy_arg(mode,    Line, &ArgOK);

		busy_cursor(TRUE);
	    status = set_dfield_state(element, level, mode);
		(void) PerformEdit("resume");
		busy_cursor(FALSE);
		}

	/* Handle "group_visibility" command */
	else if (same(Command, "GROUP_VISIBILITY"))
	    {
		strcpy_arg(group, Line, &ArgOK);
		strcpy_arg(mode,  Line, &ArgOK);

		busy_cursor(TRUE);
	    status = set_group_state(group, mode);
		(void) PerformEdit("resume");
		busy_cursor(FALSE);
		}

	/* Handle "background" command */
	else if (same(Command, "BACKGROUND"))
	    {
		MODULE	pmodule = Module;
		EDITOR	peditor = Editor;

		strcpy_arg(element, Line, &ArgOK);
		strcpy_arg(level,   Line, &ArgOK);
		read_edit_vals(0, Line);
		set_edit_cal(cal);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = set_background_value(element, level);
		busy_cursor(FALSE);

		Module = pmodule;
		Editor = peditor;

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
		}

	/* Handle "edit" command */
	else if (same(Command, "EDIT"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_DEPICT) EditResume = FALSE;
		if (Editor != EDITOR_EDIT)   EditResume = FALSE;

		Module = MODULE_DEPICT;
		Editor = EDITOR_EDIT;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
		set_edit_cal(cal);
		set_edit_calx(calx);
	    status = depiction_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	/* Handle "label" command */
	else if (same(Command, "LABEL"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_DEPICT) EditResume = FALSE;
		if (Editor != EDITOR_LABEL)  EditResume = FALSE;

		Module = MODULE_DEPICT;
		Editor = EDITOR_LABEL;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
		set_edit_cal(cal);
	    status = depiction_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	/* Handle "sample" command */
	else if (same(Command, "SAMPLE"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_DEPICT) EditResume = FALSE;
		if (Editor != EDITOR_SAMPLE) EditResume = FALSE;

		Module = MODULE_DEPICT;
		Editor = EDITOR_SAMPLE;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
		set_edit_cal(cal);
	    status = depiction_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E G u i d a n c e                                                      *
*                                                                              *
*******************************************************************************/

GEREPLY GEGuidance

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status;
	char	tag[30], mode[30];
	char	elem[128], level[128], source[128], subsrc[128], rtime[40];
	char	vtime[40], state[5];

	pr_info("Editor.API", "GEGuidance: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	(void) strcpy(tag,    "");
	(void) strcpy(elem,   "");
	(void) strcpy(level,  "");
	(void) strcpy(source, "");
	(void) strcpy(subsrc, "");
	(void) strcpy(rtime,  "");
	(void) strcpy(vtime,  "");
	(void) strcpy(state,  "");
	status = FALSE;

	/* Handle "show" command */
	if (same(Command, "SHOW"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = show_guidance();
		(void) PerformEdit("resume");
		busy_cursor(FALSE);
		return (status)? GE_VALID: GE_INVALID;
	    }

	/* Handle "hide" command */
	else if (same(Command, "HIDE"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = hide_guidance();
		(void) PerformEdit("resume");
		/* release_guidance(); */
		busy_cursor(FALSE);
		return (status)? GE_VALID: GE_INVALID;
	    }

	/* Handle "field_register" command */
	else if (same(Command, "FIELD_REGISTER"))
	    {
		strcpy_arg(tag,    Line, &ArgOK);
		strcpy_arg(elem,   Line, &ArgOK);
		strcpy_arg(level,  Line, &ArgOK);
		strcpy_arg(source, Line, &ArgOK);
		strcpy_arg(subsrc, Line, &ArgOK);
		strcpy_arg(rtime,  Line, &ArgOK);
		if (same(subsrc, "-")) (void) strcpy(subsrc, "");
		if (same(rtime,  "-")) (void) strcpy(rtime,  "");

		busy_cursor(TRUE);
	    status = guidance_fld_register(tag, elem, level, source, subsrc, rtime);
		busy_cursor(FALSE);
	    }

	/* Handle "field_deregister" command */
	else if (same(Command, "FIELD_DEREGISTER"))
	    {
		strcpy_arg(tag, Line, &ArgOK);

		busy_cursor(TRUE);
	    status = guidance_fld_deregister(tag);
		if (Module == MODULE_GUID) Editor = EDITOR_NONE;
		busy_cursor(FALSE);
		}

	/* Handle "field_visibility" command */
	else if (same(Command, "FIELD_VISIBILITY"))
	    {
		strcpy_arg(tag,   Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);
		strcpy_arg(state, Line, &ArgOK);

		busy_cursor(TRUE);
	    status = guidance_fld_visibility(tag, vtime, state);
		busy_cursor(FALSE);
		}

	/* Handle "edit" command */
	else if (same(Command, "EDIT"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_GUID) EditResume = FALSE;
		if (Editor != EDITOR_EDIT) EditResume = FALSE;

		Module = MODULE_GUID;
		Editor = EDITOR_EDIT;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
	    status = guidance_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	/* Handle "label" command */
	else if (same(Command, "LABEL"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_GUID)  EditResume = FALSE;
		if (Editor != EDITOR_LABEL) EditResume = FALSE;

		Module = MODULE_GUID;
		Editor = EDITOR_LABEL;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
	    status = guidance_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	/* Handle "sample" command */
	else if (same(Command, "SAMPLE"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_GUID)   EditResume = FALSE;
		if (Editor != EDITOR_SAMPLE) EditResume = FALSE;

		Module = MODULE_GUID;
		Editor = EDITOR_SAMPLE;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
	    status = guidance_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E I m a g e r y                                                        *
*                                                                              *
*******************************************************************************/

GEREPLY GEImagery

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status, blend;
	int		ratio, plane;
	char	tag[50], mode[50], vtime[50];

	pr_info("Editor.API", "GEImagery: %s.\n", cmdstring);
	(void) strcpy(tag,   "");
	(void) strcpy(mode,  "");
	(void) strcpy(vtime, "");

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}

	/* Handle "show" command */
	if (same(Command, "SHOW"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = show_imagery();
		(void) PerformEdit("resume");
		busy_cursor(FALSE);
		return (status)? GE_VALID: GE_INVALID;
	    }

	/* Handle "hide" command */
	else if (same(Command, "HIDE"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = hide_imagery();
		(void) PerformEdit("resume");
		/* release_imagery(); */
		busy_cursor(FALSE);
		return (status)? GE_VALID: GE_INVALID;
	    }

	else if (same(Command, "ACTIVE"))
		{
		strcpy_arg(tag,   Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		status = active_image(tag, vtime);
		busy_cursor(FALSE);
		}

	/* Handle "blend" command */
	else if (same(Command, "BLEND"))
	    {
		blend = FALSE;
		strcpy_arg(mode, Line, &ArgOK);
		if (ArgOK) blend = same(mode, "ON");

		ratio = int_arg(Line, &ArgOK);

		busy_cursor(TRUE);
		status = blend_imagery(blend, ratio);
		busy_cursor(FALSE);
		}

	/* Handle "display" command */
	else if (same(Command, "DISPLAY"))
	    {
		plane = 0;
		strcpy_arg(mode, Line, &ArgOK);
		if (ArgOK)
		{
			     if (same(mode, IMAGERY_DISPLAY_PLANE)) plane = 1;
			else if (same(mode, GEOGRAPHIC_DISPLAY_PLANE)) plane = 0;
		}

		strcpy_arg(tag,   Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		status = display_imagery(plane, tag, vtime);
		busy_cursor(FALSE);
		}

	else if (same(Command, "SETLUT"))
		{
		strcpy_arg(tag,  Line, &ArgOK);
		strcpy_arg(mode, Line, &ArgOK);

		busy_cursor(TRUE);
		status = lut_imagery(tag, mode);
		busy_cursor(FALSE);
		}

	else if (same(Command, "SAMPLE"))
		{
		strcpy_arg(mode,  Line, &ArgOK);
		if (same(mode, "OFF"))
			{
			Editor = EDITOR_NONE;
			return GE_VALID;
			}

		EditResume = same(mode, EditMode);
		if (Module != MODULE_IMAGERY) EditResume = FALSE;
		if (Editor != EDITOR_SAMPLE)  EditResume = FALSE;

		Module = MODULE_IMAGERY;
		Editor = EDITOR_SAMPLE;
		(void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
	    status = imagery_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
		}

	/* Handle "remove" command */
	else if (same(Command, "REMOVE"))
	    {
		strcpy_arg(tag,   Line, &ArgOK);

		busy_cursor(TRUE);
		status = remove_imagery(tag);
		busy_cursor(FALSE);
		}

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E S e q u e n c e                                                      *
*                                                                              *
*******************************************************************************/

GEREPLY GESequence

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status, save;
	char	vmode[40], vstep[40], rmode[40], smode[40];
	char	elem[128], level[128], group[128];
	char	source[128], subsrc[128], rtime[40], vtime[40], ttime[40];
	int		mstep, ivt;

	pr_info("Editor.API", "GESequence: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;
	(void) strcpy(source, "");
	(void) strcpy(subsrc, "");
	(void) strcpy(rtime, "");
	(void) strcpy(vtime, "");
	(void) strcpy(ttime, "");
	(void) strcpy(smode, "");

	if(same(Command, "INITIALIZE"))
		{
		/* Interpret edit/view mode */
		strcpy_arg(vmode, Line, &ArgOK);
		if (same(vmode, "EDIT"))
			{
			/* Optionally reset interpolation timestep */
			/*  and initialize link chain time delta   */
			strcpy_arg(vstep, Line, &ArgOK);
			mstep = interpret_hour_minute_string(vstep);
			if (mstep>0)
				{
				DTween      = mstep;
				LchainDelta = mstep;
				}
			ViewOnly = FALSE;

			/* Make sure this database is not currently being edited */
			if (!ObtainDblock()) return GE_INVALID;
			}
		else
			{
			ViewOnly = TRUE;
			}

		/* Interpret sequence range mode */
		strcpy_arg(rmode, Line, &ArgOK);
		if (same(rmode, "RANGE"))
			{
			strcpy_arg(vtime, Line, &ArgOK);
			strcpy_arg(ttime, Line, &ArgOK);
			}

		/* Interpret save mode */
		save = FALSE;
		strcpy_arg(smode, Line, &ArgOK);
		if (ArgOK) save = same(smode, "SAVE");

		status = read_sequence(rmode, vtime, ttime, save);
		if (!Spawned) capture_dn_raster(DnBgnd);
		/*
		SequenceReady = TRUE;
		show_depiction();
		*/
		}

	else if(same(Command, "CREATE_DEPICTION"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(source, Line, &ArgOK);
		strcpy_arg(subsrc, Line, &ArgOK);
		strcpy_arg(rtime, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);
		strcpy_arg(ttime, Line, &ArgOK);

		if (same(source, "-")) (void) strcpy(source, "");
		if (same(subsrc, "-")) (void) strcpy(subsrc, "");
		if (same(rtime, "-"))  (void) strcpy(rtime, "");
		if (same(vtime, "-"))  (void) strcpy(vtime, ttime);
		if (blank(ttime)) (void) strcpy(ttime, vtime);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = insert_sequence(source, subsrc, rtime, vtime, ttime);
		busy_cursor(FALSE);
		}

	else if(same(Command, "CREATE_FIELD"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(elem,  Line, &ArgOK);
		strcpy_arg(level, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		(void) strcpy(source, "");
		(void) strcpy(subsrc, "");
		(void) strcpy(rtime,  "");
		(void) strcpy(ttime,  vtime);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = import_field(source, subsrc, rtime, vtime, ttime, elem, level);
		busy_cursor(FALSE);
		}

	else if(same(Command, "GET_DEPICTION"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(source, Line, &ArgOK);
		strcpy_arg(vtime,  Line, &ArgOK);
		strcpy_arg(ttime,  Line, &ArgOK);

		(void) strcpy(subsrc, "");
		(void) strcpy(rtime,  "");
		if (blank(ttime))      (void) strcpy(ttime, vtime);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = insert_sequence(source, subsrc, rtime, vtime, ttime);
		busy_cursor(FALSE);
		}

	else if(same(Command, "GET_FIELD"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(source, Line, &ArgOK);
		strcpy_arg(subsrc, Line, &ArgOK);
		strcpy_arg(rtime,  Line, &ArgOK);
		strcpy_arg(elem,   Line, &ArgOK);
		strcpy_arg(level,  Line, &ArgOK);
		strcpy_arg(vtime,  Line, &ArgOK);
		strcpy_arg(ttime,  Line, &ArgOK);

		if (same(subsrc, "-")) (void) strcpy(subsrc, "");
		if (same(rtime, "-"))  (void) strcpy(rtime, "");
		if (blank(ttime))      (void) strcpy(ttime, vtime);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = import_field(source, subsrc, rtime, vtime, ttime, elem, level);
		busy_cursor(FALSE);
		}

	else if(same(Command, "DELETE_DEPICTION") || same(Command, "DELETE"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(group, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("cancel all");
		(void) PerformEdit("undo");
		status = delete_sequence(vtime);
		busy_cursor(FALSE);
		}

	else if(same(Command, "DELETE_FIELD"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(elem,  Line, &ArgOK);
		strcpy_arg(level, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = delete_field(vtime, elem, level);
		busy_cursor(FALSE);
		}

	else if(same(Command, "SAVE_DEPICTION") || same(Command, "SAVE"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(group, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		if (same_ic(vtime, "ALL"))
			{
			status = TRUE;
			for (ivt=0; ivt<NumTime; ivt++)
				if (!save_sequence(TimeList[ivt].jtime)) status = FALSE;
			}
		else
			status = save_sequence(vtime);
		busy_cursor(FALSE);
		}

	else if(same(Command, "SAVE_FIELD"))
		{
		if (!SequenceReady) return GE_INVALID;

		strcpy_arg(elem,  Line, &ArgOK);
		strcpy_arg(level, Line, &ArgOK);
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = save_field(vtime, elem, level);
		busy_cursor(FALSE);
		}

	else if(same(Command, "ACTIVE"))
		{
		strcpy_arg(vtime, Line, &ArgOK);
		strcpy_arg(ttime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
		status = pick_sequence(vtime, ttime);
		busy_cursor(FALSE);
		}

	else if(same(Command, "TZERO"))
		{
		strcpy_arg(vtime, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		status = zero_sequence(vtime);
		busy_cursor(FALSE);
		}

	tell_active_status();
	return (status)? GE_VALID: GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     G E S c r a t c h p a d                                                  *
*                                                                              *
*******************************************************************************/

GEREPLY GEScratchpad

	(
	STRING	cmdstring
	)

	{
	LOGICAL	status;
	char	mode[30];

	pr_info("Editor.API", "GEScratchpad: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;

	/* Handle "show" command */
	if (same(Command, "SHOW"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = show_scratch();
		busy_cursor(FALSE);
	    }

	/* Handle "hide" command */
	else if (same(Command, "HIDE"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = hide_scratch();
		busy_cursor(FALSE);
	    }

	/* Handle "edit" command */
	else if (same(Command, "EDIT"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_SCRATCH) EditResume = FALSE;
		if (Editor != EDITOR_EDIT)    EditResume = FALSE;

		Module = MODULE_SCRATCH;
		Editor = EDITOR_EDIT;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
	    status = scratch_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	tell_active_status();
	return GE_VALID;
	}


/*******************************************************************************
*                                                                              *
*     G E T i m e l i n k                                                      *
*                                                                              *
*******************************************************************************/

GEREPLY GETimelink

	(
	STRING	cmdstring
	)

	{
	char	elem[128], level[128], group[128], option[40], mode[40];
	LOGICAL	status, lmode;

	pr_info("Editor.API", "GETimelink: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		tell_active_status();
		return GE_INVALID;
		}
	status = FALSE;

	/* Handle "enter" command */
	if (same(Command, "ENTER"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		(void) release_guidance();
		Module = MODULE_LINK;
		Editor = EDITOR_NONE;
	    status = show_timelink();
		busy_cursor(FALSE);
	    }

	/* Handle "zoom" command */
	else if (same(Command, "ZOOM"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
		status = TRUE;
		busy_cursor(FALSE);
	    }

	/* Handle "exit" command */
	else if (same(Command, "EXIT"))
	    {
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
	    status = hide_timelink();
		busy_cursor(FALSE);
	    }

	/* Handle "show" command */
	else if (same(Command, "SHOW"))
	    {
		strcpy_arg(option, Line, &ArgOK);
		strcpy_arg(mode,   Line, &ArgOK);

		if (same_ic(mode, "OFF")) lmode = FALSE;
		else                      lmode = TRUE;

		if      (same_ic(option, "TIME"))		DisplayTime  = lmode;
		else if (same_ic(option, "EARLY_LATE"))	DisplayEarly = lmode;
		else if (same_ic(option, "CONTROL"))	DisplayCtrl  = lmode;
		else if (same_ic(option, "SPEED"))		DisplaySpeed = lmode;
		else
			{
			pr_warning("Editor.API",
				"Unrecognized timelink display option: %s\n", option);
			return GE_INVALID;
			}
		return GE_VALID;
		}

	/* Handle "interpolate" command */
	else if (same(Command, "INTERPOLATE"))
	    {
		strcpy_arg(mode, Line, &ArgOK);

		if (same(mode, "CANCEL"))
			{
			post_interp_cancel();
			return GE_VALID;
			}

		if (same(mode, "FIELD"))
			{
			strcpy_arg(elem,  Line, &ArgOK);
			strcpy_arg(level, Line, &ArgOK);
			}
		else
			{
			(void) strcpy(elem,  "ALL");
			(void) strcpy(level, "ALL");
			}
		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = interpolate(elem, level);
		busy_cursor(FALSE);
	    }

	/* Handle "master_link" command */
	else if (same(Command, "MASTER_LINK"))
	    {
		strcpy_arg(group, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
	    status = active_edit_field("MASTER_LINK", group);
		busy_cursor(FALSE);
		}

	/* Handle "link_to" command */
	else if (same(Command, "LINK_TO"))
	    {
		strcpy_arg(elem,  Line, &ArgOK);
		strcpy_arg(level, Line, &ArgOK);

		busy_cursor(TRUE);
		(void) PerformEdit("confirm");
		if (same(elem, "MASTER_LINK")) (void) strcpy(elem, "MASTER_LINK");
		status = borrow_links(ActiveDfld, elem, level, TRUE);
		busy_cursor(FALSE);
		}

	/* Handle "edit" command */
	else if (same(Command, "EDIT"))
	    {
		strcpy_arg(mode,  Line, &ArgOK);

		EditResume = same(mode, EditMode);
		if (Module != MODULE_LINK) EditResume = FALSE;
		if (Editor != EDITOR_EDIT) EditResume = FALSE;

		Module = MODULE_LINK;
		Editor = EDITOR_EDIT;
	    (void) safe_strcpy(EditMode, mode);

		read_edit_vals(0, Line);
	    status = timelink_check();

		/* We have a valid edit to perform */
		if (status) return ContinueEdit();
	    }

	tell_active_status();
	return GE_VALID;
	}


/*******************************************************************************
*                                                                              *
*     G E E d i t                                                              *
*                                                                              *
*******************************************************************************/

GEREPLY GEEdit

	(
	STRING	cmdstring
	)

	{
	pr_info("Editor.API", "GEEdit: %s.\n", cmdstring);

	/* Parse the command string */
	(void) strcpy(Line, cmdstring);
	Command = string_arg(Line);
	if ( blank(Command) )
		{
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
		}

	/* Handle "resume" command */
	else if (same(Command, "RESUME"))
	    {
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "update" command */
	else if (same(Command, "UPDATE"))
	    {
		(void) PerformEdit("confirm");
		(void) PerformEdit("begin");
		return GE_VALID;
		}

	/* Handle "create" command */
	else if (same(Command, "CREATE"))
	    {
		(void) PerformEdit("create");
		(void) PerformEdit("begin");
		return GE_VALID;
		}

	/* Handle "clear" command */
	else if (same(Command, "CLEAR"))
	    {
		(void) PerformEdit("cancel all");
		(void) PerformEdit("clear");
		(void) PerformEdit("begin");
		return GE_VALID;
		}

	/* Handle "delete" command */
	else if (same(Command, "DELETE"))
	    {
		(void) PerformEdit("delete");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "delete_hole" command */
	else if (same(Command, "DELETE_HOLE"))
	    {
		(void) PerformEdit("delete_hole");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "copy" command */
	else if (same(Command, "COPY"))
	    {
		(void) PerformEdit("copy");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "paste" command */
	else if (same(Command, "PASTE"))
	    {
		(void) PerformEdit("paste");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "proceed" command */
	else if (same(Command, "PROCEED"))
	    {
		(void) PerformEdit("proceed");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "cancel" command */
	else if (same(Command, "CANCEL"))
	    {
		(void) PerformEdit("cancel");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "undo" command */
	else if (same(Command, "UNDO"))
	    {
		(void) PerformEdit("cancel all");
		(void) PerformEdit("undo");
		(void) PerformEdit("begin");
		return GE_VALID;
	    }

	/* Handle "draw_done" command */
	else if (same(Command, "DRAW_DONE"))
	    {
		(void) PerformEdit("draw_done");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "modify_confirm" command */
	else if (same(Command, "MODIFY_CONFIRM"))
	    {
		(void) PerformEdit("modify_confirm");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "join" command */
	else if (same(Command, "JOIN"))
	    {
		(void) PerformEdit("join");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "break" command */
	else if (same(Command, "BREAK"))
	    {
		(void) PerformEdit("break");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "rejoin" command */
	else if (same(Command, "REJOIN"))
	    {
		(void) PerformEdit("rejoin");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "end_chain" command */
	else if (same(Command, "END_CHAIN"))
	    {
		(void) PerformEdit("end_chain");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "noextrap" command */
	else if (same(Command, "NOEXTRAP"))
	    {
		(void) PerformEdit("noextrap");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* Handle "new_chain" command */
	else if (same(Command, "NEW_CHAIN"))
	    {
		(void) PerformEdit("new_chain");
		(void) PerformEdit("resume");
		return GE_VALID;
		}

	/* No current editor */
	else
		{
		Module = MODULE_NONE;
		Editor = EDITOR_NONE;
		}

	/* Not a valid edit */
	tell_active_status();
	return GE_INVALID;
	}


/*******************************************************************************
*                                                                              *
*     PRIVATE FUNCTIONS:                                                       *
*                                                                              *
*     The following functions are for use only by other modules of the INGRED  *
*     library, and are not advertised for use by applications software.        *
*                                                                              *
*******************************************************************************/

/*******************************************************************************
*                                                                              *
*     r e a d _ e d i t _ v a l s                                              *
*     m o v e _ e d i t _ v a l s                                              *
*     s e t _ e d i t _ c a l                                                  *
*                                                                              *
*******************************************************************************/

void	read_edit_vals

	(
	int		start,
	STRING	line
	)

	{
	int		ival;

	for (ival=start; ival<nEditVal; ival++)
		{
		strcpy_arg(EditVal[ival], line, &ArgOK);
		}
	}

/******************************************************************************/

void	move_edit_vals

	(
	int		start,
	int		shift
	)

	{
	int		ival, jval;

	for (ival=start; ival<nEditVal; ival++)
		{
		jval = ival + shift;
		if (jval < nEditVal) (void) strcpy(EditVal[ival], EditVal[ival+shift]);
		else                 (void) strcpy(EditVal[ival], "");
		}
	}

/******************************************************************************/

void	set_edit_cal

	(
	CAL		cal
	)

	{
	if (NotNull(EditCal))
		{
		CAL_destroy(EditCal);
		EditCal = NullCal;
		}
	if (NotNull(cal)) EditCal = CAL_duplicate(cal);
	}

/******************************************************************************/

void	set_edit_calx

	(
	CAL		calx
	)

	{
	if (NotNull(EditCalX))
		{
		CAL_destroy(EditCalX);
		EditCalX = NullCal;
		}
	if (NotNull(calx)) EditCalX = CAL_duplicate(calx);
	}

/*
void	make_edit_cal

	(
	int		start
	)

	{
	if ( NotNull(EditCal) )      return;
	if ( IsNull(ActiveDfld) )    return;
	if ( blank(EditVal[start]) ) return;

	EditCal = CAL_duplicate(ActiveDfld->bgcal);
	CAL_set_defaults(EditCal, EditVal[start], EditVal[start+1],
			EditVal[start+2]);
	}
*/

/*******************************************************************************
*                                                                              *
*     INTERNAL FUNCTIONS:                                                      *
*                                                                              *
*     The following functions are only available within this file.             *
*                                                                              *
*******************************************************************************/

/*******************************************************************************
*                                                                              *
*     U p d a t e D b l o c k                                                  *
*     O b t a i n D b l o c k                                                  *
*     R e l e a s e D b l o c k                                                *
*                                                                              *
*******************************************************************************/

static	char			Lpath[1024] = "";
static	LOGICAL			Locked      = FALSE;
static	unsigned long	DbWait      = 60;	/* Lock update interval (sec) */
static	int				DbMult		= 5;	/* No. of intervals until stale */
static	XtIntervalId	DbTimer     = 0;

/*ARGSUSED*/
static	void	UpdateDblock

	(
	XtPointer		data,
	XtIntervalId	*interval
	)

	{
	if (Locked)
		{
		/* Touch the lock file */
		pr_diag("Editor", "Updating DB Lock\n");
		(void) utime(Lpath, NULL);
		}

	DbTimer = XtAppAddTimeOut(X_appcon, DbWait*1000, UpdateDblock,
							(XtPointer) NULL);
	}

/******************************************************************************/

static	LOGICAL	ObtainDblock(void)

	{
	char		host[50], lhost[50], lpath[1024];
	pid_t		pid, lpid;
	STRING		dir, *locks, lock, tpath, c;
	int			nlock, ilock;
	struct stat	sbuf, tbuf;
	long		dtime;
	LOGICAL		busy;

	/* Obtain the depiction directory */
	dir = source_directory_by_name("depict", NULL, NULL);
	if (blank(dir))
		{
		put_message("db-no-access");
		pr_error("Editor", "Cannot access depiction directory\n");
		pr_error("Editor", "Database access denied!\n");
		return FALSE;
		}

	/* Obtain hostname and pid */
	(void) gethostname(host, sizeof(host));
	pid = getpid();

	/* Search for other lock files */
	nlock = dirlist(dir, "^\\.LOCK:.*\\.[0-9]*$", &locks);
	for (ilock=0; ilock<nlock; ilock++)
		{
		lock  = locks[ilock];
		(void) safe_strcpy(lpath, pathname(dir, lock));

		/* Read host and pid from lock filename */
		c = strchr(lock, ':') + 1;
		(void) strcpy(lhost, c);
		c = strrchr(lhost, '.');
		(void) sscanf(c, ".%d", &lpid);
		(void) strcpy(c, "");

		/* There is a lock on the database - see if it is still busy */
		busy = TRUE;
		if (same(lhost, host))
			{
			/* If locked to a process on this host, check to see if the */
			/* process is still running */
			busy = running(lpid);
			}
		else
			{
			/* If locked to a process on another host, check the modify time */
			/* of the lock file to see if it has been dropped */
			/* Note that we create a temporary file to compare times to */
			/* avoid problems with clock skews between machines */
			tpath = pathname(dir, ".LOCKTEMP");
			if (!create_file(tpath, NULL))
				{
				put_message("db-no-lock");
				pr_error("Editor", "Cannot establish database lock test file\n");
				pr_error("Editor", "Database access denied!\n");
				return FALSE;
				}
			(void) stat(lpath, &sbuf);
			(void) stat(tpath, &tbuf);
			dtime = tbuf.st_mtime - sbuf.st_mtime;
			pr_diag("Editor", "Checking DB Lock times: %d  %d  Difference: %d\n",
					tbuf.st_mtime, sbuf.st_mtime, dtime);
			busy  = (LOGICAL) (dtime < (long) (DbWait*DbMult));
			(void) remove_file(tpath, NULL);
			}

		if (busy)
			{
			/* Database is truly busy - deny access */
			put_message("db-busy", lpid, lhost);
			pr_error("Editor", "Database in use (PID:%d on %s)\n", lpid, lhost);
			pr_error("Editor", "Database access denied!\n");
			return FALSE;
			}
		else
			{
			/* Locking process is no longer running - remove that lock */
			(void) unlink(lpath);
			pr_warning("Editor",
					"Removing stale database lock (PID:%d on %s)\n",
					lpid, lhost);
			}
		}

	/* If we got through, its not locked - establish a lock */
	(void) sprintf(lhost, ".LOCK:%s.%d", host, pid);
	(void) safe_strcpy(Lpath, pathname(dir, lhost));
	if (!create_file(Lpath, NULL))
		{
		put_message("db-no-lock");
		pr_error("Editor", "Cannot establish database lock\n");
		pr_error("Editor", "Database access denied!\n");
		return FALSE;
		}

	/* Now it is locked */
	Locked  = TRUE;
	DbTimer = XtAppAddTimeOut(X_appcon, DbWait*1000, UpdateDblock,
							(XtPointer) NULL);
	return TRUE;
	}

/******************************************************************************/

static	LOGICAL	ReleaseDblock(void)

	{
	if (Locked) (void) unlink(Lpath);
	Locked = FALSE;
	if (DbTimer) XtRemoveTimeOut(DbTimer);
	DbTimer = 0;
	return TRUE;
	}

/*******************************************************************************
*                                                                              *
*     O p e n G r a p h i c s                                                  *
*     C l o s e G r a p h i c s                                                *
*     R e s i z e G r a p h i c s                                              *
*                                                                              *
*******************************************************************************/

static LOGICAL	OpenGraphics(void)

	{
	int		width, height;

	if (!GraphicsOn)
		{
		if ( !gxOpenGraphics(X_display, X_window) ) return FALSE;
		glGetWindowSize(&width, &height);
		pr_info("Editor", "Opening graphics %dX%d.\n", width, height);

		GraphicsOn = TRUE;
		Gwidth     = width;
		Gheight    = height;
		}

	return TRUE;
	}

/******************************************************************************/

static LOGICAL	CloseGraphics(void)

	{
	if (GraphicsOn)
		{
		gxCloseGraphics(FALSE);
		pr_info("Editor", "Closing graphics.\n");

		GraphicsOn = FALSE;
		}

	return TRUE;
	}
 
/******************************************************************************/

static LOGICAL	ResizeGraphics(void)

	{
	int		width, height;

	circle_echo(FALSE);

	if (GraphicsOn)
		{
		XSync(X_display, 0);
		glGetWindowSize(&width, &height);
		if (width==Gwidth && height==Gheight) return FALSE;
		glResetViewport();
		pr_info("Editor", "Resizing graphics %dX%d.\n", width, height);
		Gwidth  = width;
		Gheight = height;
		}

	return TRUE;
	}

/*******************************************************************************
*                                                                              *
*     E v e n t H a n d l e r                                                  *
*                                                                              *
*******************************************************************************/

/*ARGSUSED*/
static void	EventHandler

	(
	Widget		w,
	XtPointer	unused,
	XEvent		*event,
	LOGICAL		*dispatch
	)

	{
	int		button, dx, dy, vstate;

	/* >>>>> december addition <<<<< */
	KeySym			keysym;
	char			buf[16];
	int				lbuf = 16;
	XComposeStatus	compose;
	/* >>>>> december addition <<<<< */

	if (dispatch) *dispatch = TRUE;

#	ifdef DEBUG_EVENTS
	pr_diag("Editor.Events", "[EventHandler] Event Type: %d\n", event->type);
#	endif /* DEBUG_EVENTS */

	switch(event->type)
		{
		/* Redraw if expose event */
		case Expose:
			if (event->xexpose.count == 0)
				{
				pr_diag("Editor.Events", "Redraw\n");
				/* busy_cursor(TRUE); */
				put_message("redraw");
				sync_display();
				tell_active_status();
				/* busy_cursor(FALSE); */
				}
			break;

		/* Resize if configure event */
		case ConfigureNotify:
			pr_diag("Editor.Events", "Resize\n");
			busy_cursor(TRUE);
			if (ResizeGraphics())
				{
				put_message("resize");
				reset_panels();
				if (DepictShown || LinkShown)
					{
					(void) extract_special_tags();
					(void) extract_links();
					}
				if (LinkShown)
					{
					(void) extract_unlinked();
					}
				(void) present_all();
				tell_active_status();
				}
			busy_cursor(FALSE);
			break;

		/* Keep track of window visibility */
		/* Mouse input will be ignored if window is not fully visible */
		case VisibilityNotify:
			vstate = event->xvisibility.state;
			switch (vstate)
				{
				case VisibilityUnobscured:
					Visible    = TRUE;
					AllowInput = TRUE;
					obscure_cursor(FALSE);
					obscure_message(FALSE);
					pr_diag("Editor.Events", "Unobscured\n");
					break;
				case VisibilityPartiallyObscured:
				case VisibilityFullyObscured:
					Visible    = FALSE;
					AllowInput = IgnoreObsc;
					obscure_cursor(!AllowInput);
					obscure_message(!AllowInput);
					ButtonDown = NoButton;
					ButtonX    = 0;
					ButtonY    = 0;
					pr_diag("Editor.Events", "Obscured\n");
					break;
				default:
					pr_diag("Editor.Events", "Unknown visibility state\n");
					break;
				}
			break;

		/* >>>>> december addition <<<<< */
		/* Look for Escape key to correct problems */
		case KeyPress:
			(void) XLookupString(&(event->xkey), buf, lbuf, &keysym, &compose);
			if (keysym != XK_Escape) break;

			pr_diag("Editor.Events", "[EventHandler] Escape Key Pressed!\n");

			/* Reset whatever parameters required to clean up and exit */
			if (AllowInput)
				{
				ButtonDown = NoButton;
				ButtonX    = event->xbutton.x;
				ButtonY    = event->xbutton.y;

				put_message("draw-done");
				if (DrawWait) picking_cursor(FALSE);
				DrawWait = FALSE;
				end_draw();
				}

			/* Clear the event queue */
			XSync(XtDisplay(w), TRUE);
			break;
		/* >>>>> december addition <<<<< */

		/* Save button and point for pick or draw if button pressed */
		case ButtonPress:
			button = event->xbutton.button;
			dx     = event->xbutton.x;
			dy     = event->xbutton.y;

			if (AllowInput && button == Button1)
				{
				ButtonDown = button;
				ButtonX    = dx;
				ButtonY    = dy;
				if (Drawing)
					{
					/* Finish drawing */
					pr_diag("Editor.Events",
						"[EventHandler] Button %d pressed (Drawing)\n",
						button);
					if (button == Button1)
						{
						put_message("draw-add");
						if (!DrawWait) picking_cursor(TRUE);
						DrawWait = TRUE;
						return;
						}
					}
				pr_diag("Editor.Events",
					"[EventHandler] Button %d pressed\n", button);
				}

			else
				{
				pr_diag("Editor.Events",
					"[EventHandler] Ignoring button %d press\n", button);
				}
			break;

		/* Discard button and point for pick or draw if button released */
		case ButtonRelease:
			button = event->xbutton.button;
			dx     = event->xbutton.x;
			dy     = event->xbutton.y;

			if (button == ButtonDown)
				{
				ButtonDown = NoButton;
				ButtonX    = 0;
				ButtonY    = 0;
				if (Drawing)
					{
					/* Continue drawing - do not resume */
					pr_diag("Editor.Events",
						"[EventHandler] Button %d released (Drawing)\n",
						button);
					if (button == Button1)
						{
						put_message("drawing");
						if (DrawWait) picking_cursor(FALSE);
						DrawWait = FALSE;
						add_draw(dx, dy);
						return;
						}
					}
				pr_diag("Editor.Events",
					"[EventHandler] Button %d released\n", button);
				}

			else
				{
				pr_diag("Editor.Events",
					"[EventHandler] Ignoring button %d release\n", button);
				}
			break;

		/* Unknown event */
		default:
			break;
		}

	/* In all cases - resume what we were doing */
	if (AllowInput) (void) PerformEdit("resume");
	}

/*******************************************************************************
*                                                                              *
*     C o n t i n u e E d i t                                                  *
*     P e r f o r m E d i t                                                    *
*                                                                              *
*******************************************************************************/

static	GEREPLY ContinueEdit(void)

	{
	STRING	mode;

	/* Handle resume and non-confirming modes */
	if (EditResume)
		{
		mode = "resume";
		if (Module==MODULE_DEPICT && Editor==EDITOR_EDIT)
			{
			if (same(EditVal[0], "SET"))               mode = "set";
			else if (same(EditVal[1], "SET"))          mode = "set";
			else if (same(EditVal[2], "SET"))          mode = "set";
			else if (same(EditVal[0], "STACK"))        mode = "stack";
			else if (same(EditVal[0], "FLIP"))         mode = "flip";
			else if (same(EditVal[0], "REVERSE"))      mode = "reverse";
			else if (same(EditVal[0], "FLIP-REVERSE")) mode = "flip-reverse";
			else if (same(EditVal[0], "DELETE"))       mode = "delete";
			else if (same(EditVal[0], "DELETE_HOLE"))  mode = "delete_hole";
			/* >>>>>
			else if (same(EditVal[1], "DELETE"))       mode = "delete";
			<<<<< */
			/* >>>>> where is this from??? <<<<< */
			else if (same(EditVal[1], "DELETE"))
				{
				pr_diag("Editor", "Continue edit: %s %s\n",
						EditVal[0], EditVal[1]);
				mode = "delete";
				}
			/* >>>>> where is this from??? <<<<< */
			else if (same(EditVal[0], "CUT"))          mode = "cut";
			else if (same(EditVal[0], "COPY"))         mode = "copy";
			else if (same(EditVal[0], "PASTE"))        mode = "paste";
			else if (same(EditVal[0], "MERGE"))        mode = "merge";
			else if (same(EditVal[0], "TRANSLATE"))    mode = "translate";
			else if (same(EditVal[0], "ROTATE"))       mode = "rotate";
			else if (same(EditVal[0], "FETCH"))        mode = "fetch";
			else if (same(EditVal[0], "MODIFY"))       mode = "modify";
			else if (same(EditVal[0], "MOVE"))         mode = "move";
			else if (same(EditVal[0], "SHOW"))         mode = "show";
			else if (same(EditVal[0], "NEW_CHAIN"))    mode = "new_chain";
			else if (same(EditVal[0], "END_CHAIN"))    mode = "end_chain";
			else if (same(EditVal[0], "CHOOSE_CHAIN")) mode = "choose_chain";
			else if (same(EditVal[0], "SET_TIMES"))    mode = "set_times";
			else if (same(EditVal[0], "SELECT_ALL"))   mode = "select all";
			else if (same(EditVal[0], "DRAW_OUTLINE")) mode = "draw outline";
			else if (same(EditVal[0], "PRESET_OUTLINE"))
													   mode = "preset outline";
			}
		else if (Module==MODULE_DEPICT && Editor==EDITOR_LABEL)
			{
			if (same(EditVal[1], "SET"))               mode = "set";
			}
		else if (Module==MODULE_SCRATCH && Editor==EDITOR_EDIT)
			{
			if (same(EditMode, "SELECT")
					&& same(EditVal[0], "ALL"))        mode = "select all";
			else if (same(EditMode, "SELECT")
					&& same(EditVal[0], "DELETE"))     mode = "delete";
			}
		else if (Module==MODULE_LINK)
			{
			if (same(EditVal[0], "CLEAR"))             mode = "clear";
			else if (same(EditVal[0], "MERGE"))        mode = "merge";
			else if (same(EditVal[0], "TRANSLATE"))    mode = "translate";
			else if (same(EditVal[0], "ROTATE"))       mode = "rotate";
			else if (same(EditVal[0], "FETCH"))        mode = "fetch";
			else if (same(EditVal[0], "SELECT_ALL"))   mode = "select all";
			/* >>>>> this gets removed!!!!! <<<<< */
			else if (same(EditVal[0], "INTERMEDIATE")) mode = "inter";
			/* >>>>> this gets removed!!!!! <<<<< */
			}
		(void) PerformEdit(mode);
		}

	/* Any change confirms the previous edit */
	else
		{
		(void) PerformEdit("confirm");
		(void) PerformEdit("begin");
		}

	EditResume = TRUE;
	return GE_VALID;
	}

static	LOGICAL PerformEdit

	(
	STRING	mode
	)

	{
	static	LOGICAL	begin = TRUE;

	if (!SequenceReady) return FALSE;
	if (Posting)        return FALSE;

	/* Fix it so resume cannot follow confirm */
	if (begin && same(mode, "resume")) mode = "begin";
	begin = same(mode, "confirm");

	(void) track_Xpointer(NullDn, NullSet, FALSE);
	edit_can_create(FALSE);	/* Turn off CREATE button */
	edit_can_clear(FALSE);	/* Turn off CLEAR button */
	edit_can_paste(FALSE);	/* Turn off PASTE button (turns off COPY too!) */

	if (Drawing)
		{
		pr_diag("Editor", "Perform edit: %s\n", mode);
		if (!same(mode, "resume") && !same(mode, "draw_done"))
				(void) ignore_Xcurve();
		}

	/* Handle confirm and undo and draw_done */
	if (same(mode, "confirm"))
		{
		EditResume = FALSE;
		allow_obscured_input(FALSE);
		/* Handle changes partially completed in field edit or timelink */
		/*  or scratchpad */
		if (Module == MODULE_DEPICT)
			{
			accept_mod();
			return depiction_check();
			}
		else if (Module == MODULE_LINK || Module == MODULE_SCRATCH)
			{
			accept_mod();
			}
		return TRUE;
		}
	if (same(mode, "undo"))
		{
		EditResume = FALSE;
		allow_obscured_input(FALSE);
		if (Module == MODULE_DEPICT)
			{
			reject_mod();
			return depiction_check();
			}
		else if (Module == MODULE_LINK)
			{
			return timelink_edit(mode);
			}
		return TRUE;
		}
	if (same(mode, "draw_done"))
		{
		put_message("draw-done");
		if (DrawWait) picking_cursor(FALSE);
		DrawWait = FALSE;
		end_draw();
		return TRUE;
		}

	switch (Module)
		{
		case MODULE_DEPICT:
			switch (Editor)
				{
				case EDITOR_EDIT:	return depiction_edit(mode);
				case EDITOR_SAMPLE:	return depiction_sample(mode);
				case EDITOR_LABEL:	return depiction_label(mode);
				}
			break;
		case MODULE_SCRATCH:		return scratch_edit(mode);
		case MODULE_GUID:
			switch (Editor)
				{
				case EDITOR_EDIT:	return guidance_edit(mode);
				case EDITOR_SAMPLE:	return guidance_sample(mode);
				case EDITOR_LABEL:	return guidance_label(mode);
				}
			break;
		case MODULE_IMAGERY:
			switch (Editor)
				{
				case EDITOR_SAMPLE:	return imagery_sample(mode);
				}
			break;
		case MODULE_LINK:			return timelink_edit(mode);
		case MODULE_ZOOM:
			switch (Editor)
				{
				case EDITOR_ZOOM:	return zoom_in(mode);
				case EDITOR_PAN:	return zoom_pan(mode);
				}
			break;
		}

	tell_active_status();
	return FALSE;
	}
