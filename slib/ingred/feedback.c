/***********************************************************************
*                                                                      *
*  f e e d b a c k . c                                                 *
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

#include "ingred_private.h"
#include <stdarg.h>

#undef DEBUG_FEEDBACK

/* Special Function Pointers */
static	void	(*PostMessage)(STRING, STRING)   = NULL;	/* print message */
static	void	(*DefineCursor)(STRING, LOGICAL) = NULL;	/* set the cursor */
static	void	(*PostStatus)(STRING, CAL)       = NULL;	/* notify xfpa */
static	LOGICAL	Suspend                          = TRUE;

static	void	add_chart_status(DFLIST *, int);

/* Error Dialog Message */
static	const	STRING	ErrorDialogType = "ErrorDialog";

/***********************************************************************
*                                                                      *
*     d e f i n e _ f e e d b a c k                                    *
*     s u s p e n d _ f e e d b a c k                                  *
*     r e s u m e _ f e e d b a c k                                    *
*                                                                      *
***********************************************************************/

void	define_feedback

	(
	void	(*CursorFcn)(STRING, LOGICAL),
	void	(*MessageFcn)(STRING, STRING),
	void	(*StatusFcn)(STRING, CAL)
	)

	{
	DefineCursor = CursorFcn;
	PostMessage  = MessageFcn;
	PostStatus   = StatusFcn;
	Suspend      = FALSE;
	}

void	suspend_feedback(void)

	{
	Suspend = TRUE;
	}

void	resume_feedback(void)

	{
	Suspend = FALSE;
	}

/***********************************************************************
*                                                                      *
*     p u t _ m e s s a g e                                            *
*     s t r i n g _ m e s s a g e                                      *
*     c l e a r _ m e s s a g e                                        *
*     o b s c u r e _ m e s s a g e                                    *
*                                                                      *
***********************************************************************/

static	char	Message[1024] = "";
static	STRING	Mtype         = NULL;
static	STRING	OMkey         = "obscured";
static	STRING	OMessage      = NULL;
static	STRING	OMtype        = "warning";
static	LOGICAL	Mobscured     = FALSE;

void	put_message

	(
	STRING	key,
	...
	)

	{
	va_list args;
	STRING	fmt;
	static	STRING	mtype = NULL;

	if (Spawned)      return;
	if (Suspend)      return;
	if (!PostMessage) return;

	fmt = XuGetMdbLine(key, "msg");
	if (blank(fmt) || same(fmt, key))
		{
		/* Probably no message in message db */
		Mtype = "system";
		(void) sprintf(Message, "No Message for \"%s\"", SafeStr(key));
		(void) printf("Error: %s\n", Message);
		PostMessage(Mtype, Message);
		XuFree(fmt);
		return;
		}
	FREEMEM(mtype);
	mtype = strdup_arg(fmt);

	Mtype = mtype;
	va_start(args, key);
	(void) vsprintf(Message, fmt, args);
	va_end(args);

#	ifdef DEBUG_FEEDBACK
	if (Mobscured && same(Mtype, "prompt"))
		pr_diag("Editor.Feedback",
			"[put_message] (Obscured) Type: %s   Message: %s\n",
			OMtype, OMessage);
	else
		pr_diag("Editor.Feedback",
			"[put_message] Type: %s   Message: %s\n",
			Mtype, Message);
#	endif /* DEBUG_FEEDBACK */

	if (Mobscured && same(Mtype, "prompt")) PostMessage(OMtype, OMessage);
	else                                    PostMessage(Mtype, Message);
	XuFree(fmt);
	}

/**********************************************************************/

void	string_message

	(
	STRING	type,
	STRING	format,
	...
	)

	{
	va_list args;

	if (Spawned)      return;
	if (Suspend)      return;
	if (!PostMessage) return;

	Mtype = type;
	va_start(args, format);
	(void) vsprintf(Message, format, args);
	va_end(args);

#	ifdef DEBUG_FEEDBACK
	if (Mobscured && same(Mtype, "prompt"))
		pr_diag("Editor.Feedback",
			"[string_message] (Obscured) Type: %s   Message: %s\n",
			OMtype, OMessage);
	else
		pr_diag("Editor.Feedback",
			"[string_message] Type: %s   Message: %s\n", Mtype, Message);
#	endif /* DEBUG_FEEDBACK */

	if (Mobscured && same(Mtype, "prompt")) PostMessage(OMtype, OMessage);
	else                                    PostMessage(Mtype, Message);
	}

/**********************************************************************/

void	clear_message(void)

	{
	if (Spawned)      return;
	if (Suspend)      return;
	if (!PostMessage) return;

	Mtype = "status";
	(void) strcpy(Message, "");

#	ifdef DEBUG_FEEDBACK
	pr_diag("Editor.Feedback",
		"[clear_message] Type: %s   Message: %s\n", Mtype, Message);
#	endif /* DEBUG_FEEDBACK */

	PostMessage(Mtype, Message);
	}

/**********************************************************************/

void	obscure_message

	(
	LOGICAL	state
	)

	{
	if (Spawned)      return;
	if (Suspend)      return;
	if (!PostMessage) return;

	if (state && !Mobscured)
		{
		Mobscured = TRUE;
		if (!OMessage)
			{
			OMessage = XuGetMdbLine(OMkey, OMtype);
			}

#		ifdef DEBUG_FEEDBACK
		if (same(Mtype, "prompt"))
			pr_diag("Editor.Feedback",
				"[obscure_message] (T & !Obscured) Type: %s   Message: %s\n",
				OMtype, OMessage);
		else
			pr_diag("Editor.Feedback",
				"[obscure_message] (T & !Obscured) Type: %s   Message: %s\n",
				Mtype, Message);
#		endif /* DEBUG_FEEDBACK */

		if (same(Mtype, "prompt")) PostMessage(OMtype, OMessage);
		else                       PostMessage(Mtype, Message);
		}

	else if (!state && Mobscured)
		{
		Mobscured = FALSE;

#		ifdef DEBUG_FEEDBACK
		pr_diag("Editor.Feedback",
			"[obscure_message] (F & Obscured) Type: %s   Message: %s\n",
			Mtype, Message);
#		endif /* DEBUG_FEEDBACK */

		PostMessage(Mtype, Message);
		}
	}

/***********************************************************************
*                                                                      *
*     p u t _ e r r o r _ d i a l o g                                  *
*                                                                      *
***********************************************************************/

void	put_error_dialog

	(
	STRING	key
	)

	{
	if (Spawned)      return;
	if (Suspend)      return;
	if (!PostMessage) return;

#	ifdef DEBUG_FEEDBACK
	pr_diag("Editor.Feedback",
		"[put_error_dialog] Type: ErrorDialog   Key: %s\n", key);
#	endif /* DEBUG_FEEDBACK */

	PostMessage(ErrorDialogType, key);
	}

/*******************************************************************************
*                                                                              *
*     b u s y _ c u r s o r                                                    *
*     s t o p _ c u r s o r                                                    *
*     p i c k i n g _ c u r s o r                                              *
*     d r a w i n g _ c u r s o r                                              *
*     o b s c u r e _ c u r s o r                                              *
*                                                                              *
*******************************************************************************/

void	busy_cursor

	(
	LOGICAL	state
	)

	{
	if (Spawned)   return;
	if (Suspend)   return;
	if (DefineCursor) DefineCursor("busy", state);
	}

void	stop_cursor

	(
	LOGICAL	state
	)

	{
	if (Spawned)   return;
	if (Suspend)   return;
	if (DefineCursor) DefineCursor("stop", state);
	}

void	picking_cursor

	(
	LOGICAL	state
	)

	{
	if (Spawned)   return;
	if (Suspend)   return;
	if (DefineCursor) DefineCursor("finger", state);
	}

void	drawing_cursor

	(
	LOGICAL	state
	)

	{
	if (Spawned)   return;
	if (Suspend)   return;
	if (DefineCursor) DefineCursor("pen", state);
	}

void	obscure_cursor

	(
	LOGICAL	state
	)

	{
	if (Spawned)   return;
	if (Suspend)   return;
	if (DefineCursor) DefineCursor("obscured", state);
	}


/***********************************************************************
*                                                                      *
*     s h o w i n g _ c h a r t                                        *
*     l i n k i n g _ c h a r t                                        *
*     l i n k _ s t a t u s                                            *
*     e n d _ c o n t r o l _ l i n k                                  *
*     i n t e r p _ p r o g r e s s                                    *
*     r e s e t _ b a c k g r o u n d                                  *
*     z o o m _ s t a r t                                              *
*     z o o m _ a r e a                                                *
*     z o o m _ p a n _ e n d                                          *
*     e d i t _ m e n u                                                *
*     e d i t _ s e l e c t                                            *
*     e d i t _ s e l e c t _ h o l e                                  *
*     e d i t _ s e l e c t _ n o d e                                  *
*     e d i t _ a d d i n g _ l c h a i n                              *
*     l a b e l _ s e l e c t                                          *
*     l i n k _ s e l e c t                                            *
*     f i e l d _ t i m e s                                            *
*     f i e l d _ s t a t u s                                          *
*     f i e l d _ c r e a t e                                          *
*     g u i d a n c e _ l e g e n d _ l a b e l s                      *
*     s a m p l e _ b o x                                              *
*                                                                      *
***********************************************************************/

static	char	sbuf[5000] = "";

/******************************************************************************/

void	showing_chart

	(
	STRING	tstamp
	)

	{
	if (Spawned) return;
	if (Suspend) return;
	(void) sprintf(sbuf, "ANIMATION SHOWING %s", tstamp);
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	linking_chart

	(
	STRING	tstamp
	)

	{
	if (Spawned) return;
	if (Suspend) return;
	(void) sprintf(sbuf, "TIMELINK LINKING %s", tstamp);
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	link_status

	(
	DFLIST	*dfld
	)

	{
	STRING	elem, level, group;

	if (Spawned)        return;
	if (Suspend)        return;

	if (IsNull(dfld))   return;
	if (!SequenceReady) return;

	if (!dfld->dolink && !tdep_special(dfld->tdep)) return;

	if (dfld->reported) return;

	elem  = dfld->element;
	level = dfld->level;

	if (same(elem, "MASTER_LINK"))
		{
		if (same(level, "GROUP")) group = dfld->group;
		else                      group = "GLOBAL";
		(void) sprintf(sbuf, "TIMELINK MASTER_LINK_STATUS_UPDATE %s ", group);
		if (dfld->linked)          (void) strcat(sbuf, "LINKED");
		else if (dfld->nchain > 0) (void) strcat(sbuf, "PARTIAL");
		else                       (void) strcat(sbuf, "NONE");
		}

	else
		{
		(void) sprintf(sbuf, "TIMELINK STATUS_UPDATE %s %s ", elem, level);
		if (dfld->interp && dfld->intlab) (void) strcat(sbuf, "INTERP");
		else if (dfld->interp)            (void) strcat(sbuf, "FIELD");
		else if (dfld->linked)            (void) strcat(sbuf, "LINKED");
		else if (dfld->nchain > 0)        (void) strcat(sbuf, "PARTIAL");
		else                              (void) strcat(sbuf, "NONE");
		}

	dfld->reported = TRUE;
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;

	if (dfld==ActiveDfld && Module==MODULE_LINK) field_times(dfld);
	}

/******************************************************************************/

void	end_control_link (void)

	{
	if (Spawned)        return;
	if (Suspend)        return;

	(void) sprintf(sbuf, "TIMELINK ACTION INTERMEDIATE_DONE");
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	interp_progress

	(
	DFLIST	*dfld,
	int		icur,
	int		ncur,
	int		itot,
	int		ntot
	)

	{
	STRING	elem, level;
	int		pcur, ptot;

	static	DFLIST	*PrevDfld = (DFLIST *)0;
	static	int		PrevItot = 0;
	static	int		PrevNtot = 0;

	if (Spawned)        return;
	if (Suspend)        return;

	if (IsNull(dfld))   return;
	if (!SequenceReady) return;

	if (!dfld->dolink)  return;

	elem  = dfld->element;
	level = dfld->level;

	pcur  = NINT(100.0*icur/ncur);
	if (ntot <= 0)
		{
		if (dfld != PrevDfld) return;
		itot = PrevItot;
		ntot = PrevNtot;
		}
	else
		{
		PrevItot = itot;
		PrevNtot = ntot;
		PrevDfld = dfld;
		}
	ptot = NINT((100.0*itot + pcur)/ntot);

	(void) sprintf(sbuf, "INTERPOLATE %s %s %d %d", elem, level, pcur, ptot);
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void    reset_background

	(
	DFLIST   *dfld
	)

    {
    if (Spawned)        return;
	if (Suspend)        return;

	if (IsNull(dfld))   return;
    if (!SequenceReady) return;

	(void) sprintf(sbuf, "BACKGROUND %s %s", dfld->element, dfld->level);

    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, dfld->bgcal);
	Posting = FALSE;
    }

/******************************************************************************/

void    zoom_start

	(
	)

    {
    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

    (void) sprintf(sbuf, "ZOOM START");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
    }

/******************************************************************************/

void    zoom_area

	(
	BOX		*box
	)

    {
    if (Spawned)        return;
	if (Suspend)        return;

	if (IsNull(box))    return;
    if (!SequenceReady) return;

    (void) sprintf(sbuf, "ZOOM AREA %f %f %f %f",
			box->left,
			(100 - box->top),
			(box->right - box->left),
			(box->top - box->bottom));
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
    }

/******************************************************************************/

void    zoom_pan_end(void)

    {
    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

    (void) sprintf(sbuf, "ZOOM PAN END");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
    }

/******************************************************************************/

void    edit_menu

	(
	STRING	mname
	)

    {
	STRING	type;

    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	switch (CurrEditor)
		{
		case FpaC_CONTINUOUS:
					type = "CONTOUR";
					break;

		case FpaC_VECTOR:
					type = "VECTOR";
					break;

		case FpaC_DISCRETE:
					type = "AREA";
					break;

		case FpaC_WIND:
					type = "WIND";
					break;

		case FpaC_LINE:
					type = "LINE";
					break;

		case FpaC_SCATTERED:
					type = "POINT";
					break;

		case FpaC_LCHAIN:
					type = "LCHAIN";
					break;

		default:	return;
		}

	if (NotNull(mname))
		(void) sprintf(sbuf, "EDIT %s MENU %s",      type, mname);
	else
		(void) sprintf(sbuf, "EDIT %s MENU DEFAULT", type);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void    edit_select

	(
	CAL		cal,
	LOGICAL	menu
	)

    {
	STRING	type;

    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	switch (CurrEditor)
		{
		case FpaC_CONTINUOUS:
					type = "CONTOUR";
					break;

		case FpaC_VECTOR:
					type = "VECTOR";
					break;

		case FpaC_DISCRETE:
					type = "AREA";
					break;

		case FpaC_WIND:
					type = "WIND";
					break;

		case FpaC_LINE:
					type = "LINE";
					break;

		case FpaC_SCATTERED:
					type = "POINT";
					break;

		case FpaC_LCHAIN:
					type = "LCHAIN";
					break;

		default:	return;
		}

	(void) sprintf(sbuf, "EDIT %s",   type);
	if (menu)              (void) strcat(sbuf, " SELECT SET");
	else if (NotNull(cal)) (void) strcat(sbuf, " SELECT");
	else                   (void) strcat(sbuf, " DESELECT");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
#	ifdef DEBUG
	if (pr_level(NULL, 5)) debug_attrib_list(NULL, cal);
#	endif /* DEBUG */
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, cal);
	Posting = FALSE;
	}

/******************************************************************************/

void    edit_select_hole(void)

    {
	STRING	type;

    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	switch (CurrEditor)
		{
		case FpaC_DISCRETE:
					type = "AREA";
					break;

		case FpaC_WIND:
					type = "WIND";
					break;

		default:	return;
		}

	(void) sprintf(sbuf, "EDIT %s", type);
	(void) strcat(sbuf, " SELECT HOLE");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void    edit_select_node

	(
	CAL		cal,
	LOGICAL	menu
	)

    {
	STRING	type;

    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	switch (CurrEditor)
		{
		case FpaC_LCHAIN:
					type = "LCHAIN";
					break;

		default:	return;
		}

	(void) sprintf(sbuf, "EDIT %s", type);
	if (menu)              (void) strcat(sbuf, " SELECT NODE SET");
	else if (NotNull(cal)) (void) strcat(sbuf, " SELECT NODE");
	else                   (void) strcat(sbuf, " DESELECT NODE");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
#	ifdef DEBUG
	if (pr_level(NULL, 5)) debug_attrib_list(NULL, cal);
#	endif /* DEBUG */
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, cal);
	Posting = FALSE;
	}

/******************************************************************************/

void	edit_adding_lchain

	(
	STRING	tstamp
	)

	{
	if (Spawned) return;
	if (Suspend) return;
	(void) sprintf(sbuf, "EDIT ADDING %s", tstamp);
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void    label_select

	(
	CAL		cal
	)

    {
    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	(void) sprintf(sbuf, "LABEL");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, cal);
	Posting = FALSE;
	}

/******************************************************************************/

void    link_select

	(
	CAL		cal,
	LOGICAL	menu
	)

    {
	STRING	type;

    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	(void) sprintf(sbuf, "TIMELINK ACTION");
	if (menu)              (void) strcat(sbuf, " SELECT SET");
	else if (NotNull(cal)) (void) strcat(sbuf, " SELECT");
	else                   (void) strcat(sbuf, " DESELECT");
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, cal);
	Posting = FALSE;
	}

/******************************************************************************/

static	void	add_chart_status

	(
	DFLIST   *dfld,
	int		itime
	)

	{
	if (IsNull(dfld))     return;
	if (itime < 0)        return;
	if (itime >= NumTime) return;

	/* Add the valid time */
	(void) strcat(sbuf, " ");
	(void) strcat(sbuf, TimeList[itime].jtime);

	/* Add the status */
	(void) strcat(sbuf, " ");
	if (tdep_special(dfld->tdep))
		 (void) strcat(sbuf, "NONE");
	else (void) strcat(sbuf, link_frame_status(dfld, itime));
	}

void	field_times

	(
	DFLIST   *dfld
	)

	{
	int		itime, jtime, ntime;

    if (Spawned)        return;
	if (Suspend)        return;

    if (!SequenceReady) return;

	if (!dfld)          return;

	/* >>>>> try removing this!!!!!
	if (same(dfld->element, "MASTER_LINK")) return;
	<<<<< */

	(void) sprintf(sbuf, "FIELD TIMES %s %s", dfld->element, dfld->level);

	/* First count the valid times */
	/* Count is zero if not there */
	ntime = 0;
    if (NotNull(dfld) && dfld->there)
		{
		itime = first_depict_time();
		while (itime >= 0)
			{
			jtime = pick_dfield_frame(dfld, itime);
			if (jtime >= 0) ntime++;
			itime = next_depict_time(itime);
			}
		}
	(void) sprintf(sbuf+strlen(sbuf), " %d", ntime);

	/* Now add the times */
	if (ntime > 0)
		{
		itime = first_depict_time();
		while (itime >= 0)
			{
			jtime = pick_dfield_frame(dfld, itime);
			if (jtime >= 0) add_chart_status(dfld, itime);
			itime = next_depict_time(itime);
			}
		}

#ifdef OLD_WAY
		for (itime=0; itime<NumTime; itime++)
			{
			if (!dfld->there)              break;
			if (!dfld->frames[itime].meta) continue;
			ntime++;
			}
		(void) sprintf(sbuf+strlen(sbuf), " %d", ntime);

		/* Now add the times */
		for (itime=0; itime<NumTime; itime++)
			{
			if (!dfld->there)              break;
			if (!dfld->frames[itime].meta) continue;
			add_chart_status(dfld, itime);
			}
#endif

    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

void	field_status

	(
	DFLIST   *dfld,
	int		itime
	)

	{
    if (Spawned)        return;
	if (Suspend)        return;

    if (IsNull(dfld))   return;
    if (!SequenceReady) return;
	if (itime < 0)      return;

	(void) sprintf(sbuf, "FIELD STATUS %s %s", dfld->element, dfld->level);
	add_chart_status(dfld, itime);

    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	field_create

	(
	DFLIST   *dfld,
	LOGICAL	ask
	)

	{
    if (Spawned)        return;
	if (Suspend)        return;

    if (IsNull(dfld))   return;
    if (EditTime < 0)   return;
    if (!SequenceReady) return;

	(void) sprintf(sbuf, "FIELD CREATE %s %s %s", dfld->element, dfld->level,
			TimeList[EditTime].jtime);
	if (ask) (void) strcat(sbuf, " ASK");

    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	guidance_legend_labels

	(
	STRING	tag,
	COLOUR	colour
	)

	{
    if (Spawned)        return;
	if (Suspend)        return;

    if (blank(tag))     return;

	(void) sprintf(sbuf, "GUIDANCE LEGEND %s %d", tag, colour);

    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
    if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	sample_box

	(
	LOGICAL	on,
	CAL		cal
	)

	{
    if (Spawned)        return;
	if (Suspend)        return;

	if (on)
		{
		(void) sprintf(sbuf, "SAMPLE DISPLAY ON");
		pr_info("Editor.Feedback", "%s.\n", sbuf);
		if (pr_level(NULL, 5)) debug_attrib_list(NULL, cal);
		Posting = TRUE;
		if (PostStatus) PostStatus(sbuf, cal);
		Posting = FALSE;
		}
	else
		{
		(void) sprintf(sbuf, "SAMPLE DISPLAY OFF");
		pr_info("Editor.Feedback", "%s.\n", sbuf);
		Posting = TRUE;
		if (PostStatus) PostStatus(sbuf, NullCal);
		Posting = FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*     e d i t _ i n _ p r o g r e s s                                  *
*     e d i t _ i g n o r e                                            *
*     e d i t _ p e n d i n g                                          *
*     e d i t _ c o m p l e t e                                        *
*     e d i t _ a l l o w _ p r e s e t _ o u t l i n e                *
*     e d i t _ c a n _ c r e a t e                                    *
*     e d i t _ c a n _ c o p y                                        *
*     e d i t _ c a n _ p a s t e                                      *
*     e d i t _ c a n _ j o i n                                        *
*     e d i t _ c a n _ b r e a k                                      *
*     e d i t _ c a n _ r e j o i n                                    *
*     e d i t _ c a n _ p r o c e e d                                  *
*     e d i t _ c a n _ c l e a r                                      *
*     e d i t _ c o n t r o l                                          *
*                                                                      *
***********************************************************************/

void	edit_in_progress(void)

	{
	/*
	edit_control("UNDO",   FALSE);
	edit_control("UPDATE", FALSE);
	*/
	edit_control("CANCEL", TRUE);
	}

/******************************************************************************/

void	edit_ignore(void)

	{
	/*
	edit_control("UNDO",   FALSE);
	edit_control("UPDATE", FALSE);
	*/
	edit_control("CANCEL", FALSE);
	}

/******************************************************************************/

void	edit_pending(void)

	{
	edit_control("UNDO",   TRUE);
	edit_control("UPDATE", TRUE);
	edit_control("CANCEL", FALSE);
	}

/******************************************************************************/

void	edit_complete(void)

	{
	edit_control("UNDO",   FALSE);
	edit_control("UPDATE", FALSE);
	edit_control("CANCEL", FALSE);
	}

/******************************************************************************/

void	edit_allow_preset_outline

	(
	LOGICAL	yesno
	)

	{
	edit_control("PRESET_OUTLINE", yesno);
	}

/******************************************************************************/

void	edit_can_create

	(
	LOGICAL	yesno
	)

	{
	edit_control("CREATE", yesno);
	}

/******************************************************************************/

void	edit_can_copy

	(
	LOGICAL	yesno
	)

	{
	edit_control("COPY",  yesno);
	edit_control("PASTE", FALSE);
	}

/******************************************************************************/

void	edit_can_paste

	(
	LOGICAL	yesno
	)

	{
	edit_control("COPY",  FALSE);
	edit_control("PASTE", yesno);
	}

/******************************************************************************/

void	edit_can_join

	(
	LOGICAL	yesno
	)

	{
	edit_control("JOIN", yesno);
	}

/******************************************************************************/

void	edit_can_break

	(
	LOGICAL	yesno
	)

	{
	edit_control("BREAK", yesno);
	}

/******************************************************************************/

void	edit_can_rejoin

	(
	LOGICAL	yesno
	)

	{
	edit_control("REJOIN", yesno);
	}

/******************************************************************************/

void	edit_can_proceed

	(
	LOGICAL	yesno
	)

	{
	edit_control("PROCEED", yesno);
	}

/******************************************************************************/

void	edit_can_clear

	(
	LOGICAL	yesno
	)

	{
	edit_control("CLEAR", yesno);
	}

/******************************************************************************/

void	edit_control

	(
	STRING	button,
	LOGICAL	state
	)

	{
	STRING	mode;
	LOGICAL	pend, pnew;

	static	LOGICAL	UpdateState  = FALSE;
	static	LOGICAL	UndoState    = FALSE;
	static	LOGICAL	CancelState  = FALSE;
	static	LOGICAL	CreateState  = FALSE;
	static	LOGICAL	CopyState    = FALSE;
	static	LOGICAL	PasteState   = FALSE;
	static	LOGICAL	JoinState    = FALSE;
	static	LOGICAL	BreakState   = FALSE;
	static	LOGICAL	RejoinState  = FALSE;
	static	LOGICAL	ProceedState = FALSE;
	static	LOGICAL	ClearState   = FALSE;
	static	LOGICAL	PresetState  = FALSE;

    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	pend = (LOGICAL) (UpdateState || UndoState || CancelState);

	if (same(button, "UPDATE"))
		{
		if (state && UpdateState)   return;
		if (!state && !UpdateState) return;
		UpdateState = state;
		}
	else if (same(button, "UNDO"))
		{
		if (state && UndoState)   return;
		if (!state && !UndoState) return;
		UndoState = state;
		}
	else if (same(button, "CANCEL"))
		{
		if (state && CancelState)   return;
		if (!state && !CancelState) return;
		CancelState = state;
		}
	else if (same(button, "PRESET_OUTLINE"))
		{
		if (state && PresetState)   return;
		if (!state && !PresetState) return;
		PresetState = state;
		}
	else if (same(button, "CREATE"))
		{
		if (state && CreateState)   return;
		if (!state && !CreateState) return;
		CreateState = state;
		}
	else if (same(button, "COPY"))
		{
		if (state && CopyState)   return;
		if (!state && !CopyState) return;
		CopyState = state;
		}
	else if (same(button, "PASTE"))
		{
		if (state && PasteState)   return;
		if (!state && !PasteState) return;
		PasteState = state;
		}
	else if (same(button, "JOIN"))
		{
		if (state && JoinState)   return;
		if (!state && !JoinState) return;
		JoinState = state;
		}
	else if (same(button, "BREAK"))
		{
		if (state && BreakState)   return;
		if (!state && !BreakState) return;
		BreakState = state;
		}
	else if (same(button, "REJOIN"))
		{
		if (state && RejoinState)   return;
		if (!state && !RejoinState) return;
		RejoinState = state;
		}
	else if (same(button, "PROCEED"))
		{
		if (state && ProceedState)   return;
		if (!state && !ProceedState) return;
		ProceedState = state;
		}
	else if (same(button, "CLEAR"))
		{
		if (state && ClearState)   return;
		if (!state && !ClearState) return;
		ClearState = state;
		}
	else return;

	pnew = (LOGICAL) (UpdateState || UndoState || CancelState);
	if ((pnew && !pend) || (!pnew && pend)) interrupt_control(pend, FALSE);

	mode = (state)? "ON": "OFF";
	(void) sprintf(sbuf, "EDIT BUTTON %s %s", button, mode);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/***********************************************************************
*                                                                      *
*     e d i t _ d r a w n _ o u t l i n e _ p o s t e d                *
*     e d i t _ m o v e d _ o u t l i n e _ p o s t e d                *
*     e d i t _ s t o m p _ o u t l i n e _ p o s t e d                *
*     e d i t _ d r a w n _ h o l e _ p o s t e d                      *
*                                                                      *
***********************************************************************/

void	edit_drawn_outline_posted(void)

	{
    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	(void) sprintf(sbuf, "EDIT OUTLINE DRAWN");
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	edit_moved_outline_posted(void)

	{
    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	(void) sprintf(sbuf, "EDIT OUTLINE MOVED");
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	edit_stomp_outline_posted(void)

	{
    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	(void) sprintf(sbuf, "EDIT OUTLINE STOMP");
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	edit_drawn_hole_posted(void)

	{
    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	(void) sprintf(sbuf, "EDIT OUTLINE HOLE");
	pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/***********************************************************************
*                                                                      *
*     m o d e _ d r a w _ s e t a b l e                                *
*     m o d e _ c o n t r o l                                          *
*                                                                      *
***********************************************************************/

void	mode_draw_setable

	(
	LOGICAL	yesno
	)

	{
	mode_control("DRAW", yesno);
	}

/******************************************************************************/

void	mode_control

	(
	STRING	button,
	LOGICAL	state
	)

	{
	STRING	mode;

	static	LOGICAL	DrawState = TRUE;

    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	if (same(button, "DRAW"))
		{
		if (state && DrawState)   return;
		if (!state && !DrawState) return;
		DrawState = state;
		}
	else return;

	mode = (state)? "ON": "OFF";
	(void) sprintf(sbuf, "MODE BUTTON %s %s", button, mode);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/***********************************************************************
*                                                                      *
*     s c r a t c h _ c a n _ d e l e t e                              *
*     s c r a t c h _ c o n t r o l                                    *
*                                                                      *
***********************************************************************/

void	scratch_can_delete

	(
	LOGICAL	yesno
	)

	{
	scratch_control("DELETE", yesno);
	}

/******************************************************************************/

void	scratch_control

	(
	STRING	button,
	LOGICAL	state
	)

	{
	STRING	mode;

	static	LOGICAL	DeleteState = FALSE;

    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	if (same(button, "DELETE"))
		{
		if (state && DeleteState)   return;
		if (!state && !DeleteState) return;
		DeleteState = state;
		}
	else return;

	mode = (state)? "ON": "OFF";
	(void) sprintf(sbuf, "SCRATCHPAD BUTTON %s %s", button, mode);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/***********************************************************************
*                                                                      *
*     d r a w i n g _ c o n t r o l                                    *
*     m o d i f y i n g _ c o n t r o l                                *
*                                                                      *
***********************************************************************/

void	drawing_control

	(
	LOGICAL	state
	)

	{
	STRING	mode;

	static	LOGICAL	DrawState = FALSE;

    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	if (state) edit_control("CANCEL", TRUE);
	else       edit_control("CANCEL", FALSE);

	if (state && DrawState)   return;
	if (!state && !DrawState) return;
	DrawState = state;

	mode = (state)? "ON": "OFF";
	(void) sprintf(sbuf, "EDIT DRAWING %s", mode);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/******************************************************************************/

void	modifying_control

	(
	LOGICAL	state
	)

	{
	STRING	mode;

	static	LOGICAL	ModifyState = FALSE;

    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	if (state && ModifyState)   return;
	if (!state && !ModifyState) return;
	ModifyState = state;

	mode = (state)? "ON": "OFF";
	(void) sprintf(sbuf, "EDIT MODIFYING %s", mode);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}

/***********************************************************************
*                                                                      *
*     i n t e r r u p t _ c o n t r o l                                *
*                                                                      *
***********************************************************************/

void	interrupt_control

	(
	LOGICAL	state,
	LOGICAL	flush
	)

	{
	STRING	mode;

	static	int		Stack   = 0;
	static	LOGICAL	Enabled = TRUE;

    if (Spawned)        return;
	if (Suspend)        return;
    if (!SequenceReady) return;

	if (state)
		{
		if (Enabled) return;
		if (flush) Stack = 0;
		else       Stack--;
		if (Stack > 0) return;
		Stack   = 0;
		Enabled = TRUE;
		}
	else
		{
		if (flush) Stack = 1;
		else       Stack++;
		if (!Enabled) return;
		Enabled = FALSE;
		}

	mode = (Enabled)? "ON": "OFF";
	(void) sprintf(sbuf, "INTERRUPT %s", mode);
    pr_info("Editor.Feedback", "%s.\n", sbuf);
	Posting = TRUE;
	if (PostStatus) PostStatus(sbuf, NullCal);
	Posting = FALSE;
	}
