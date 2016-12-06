/*========================================================================*/
/*
*   File:    load_hourlyFields.c
*
*   Purpose: Provides the process for importing specific fields from
*            a guidance field into a depiction in the sequence.
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*/
/*========================================================================*/

#include "global.h"
#include <ingred.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include "resourceDefines.h"
#include "depiction.h"
#include "selector.h"
#include "guidance.h"
#include "editor.h"
#include "loadFields.h"
#include "observer.h"

#define hour_font_tag	"normal"
#define minute_font_tag	"very_small"


static void configure_target_btn_manager (void);
static void create_valid_time_list       (void);
static void create_field_list            (void);
static void clear_field_list             (void);
static void action_import                (String, int, int *);
static void model_select_cb              (Widget, XtPointer, XtPointer);
static void valid_time_cb                (Widget, XtPointer, XtPointer);
static void target_cb                    (Widget, XtPointer, XtPointer);
static void run_time_cb                  (Widget, XtPointer, XtPointer);
static void source_cb                    (Widget, XtPointer, XtPointer);
static void prefs_cb                     (Widget, XtPointer, XtPointer);
static void target_time_block_cb         (Widget, XtPointer, XtPointer);
static void reset_target_btns            (void);
static void target_time_control_cb       (Widget);



static FLD_DESCRIPT fd;
static XmString     NA             = NULL;
static SRCINFO      *active_source = (SRCINFO *)NULL;
static int          nfield_list    = 0;
static int          *field_list    = NULL;
static int          nrun_list      = 0;
static String       *run_list      = NULL;
static int          nvalid_list    = 0;
static String       *valid_list    = NULL;
static Widget       *target_btns   = NULL;
static int          active_target  = -1;
static TSTAMP       target_time;
static Widget       topForm;
static Widget       runTime;
static Widget       validTime;
static Widget       fieldSelect;
static Widget       modelSelect;
static Widget       guidanceFrame;
static Widget       prefsFrame;
static Widget       runTimeLabel;
static Widget       targetSW;
static Widget       targetFrame;
static Widget       targetTime;
static Widget       targetManager;
static Widget       sourceSelect;
static Boolean      select_all_fields       = True;
static Boolean      ask_to_create_depiction = True;
static Boolean      ask_to_overwrite_field  = True;
static Boolean      show_target_time_block  = True;
static Boolean      depict_seq_change       = False;


/*---------------------------- Public Functions -----------------------*/


Widget CreateLoadHourlyFields(Widget parent)
{
	int	i, m;
	Boolean ok;
	String data;
	XmString items[1];
	SourceList src;
	Widget validTimeLabel, modelLabel;
	Widget rc, btn, label, form, w;
	Widget sourceFrame, guidanceForm;
	SRCINFO *ls;

	/* Get preferences from state store.
	*/
	if(XuStateDataGet("cnf","pref", NULL, &data))
	{
		m = int_arg(data, &ok);
		if (ok) select_all_fields = (Boolean)m;
		m = int_arg(data, &ok);
		if (ok) ask_to_create_depiction = (Boolean)m;
		m = int_arg(data, &ok);
		if (ok) ask_to_overwrite_field = (Boolean)m;
		m = int_arg(data, &ok);
		if (ok) show_target_time_block = (Boolean)m;
		FreeItem(data);
	}

	if(minutes_in_depictions())
		items[0] = XuNewXmString(DateString("2005:256:12:30",MINUTES));
	else
		items[0] = XuNewXmString(DateString("2005:256:12",HOURS));

	NA = XuNewXmString(XuGetLabel("na"));

	topForm = XmVaCreateForm(parent, "loadHourlyFields",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	sourceFrame = XmVaCreateManagedFrame(topForm, "sourceFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(sourceFrame, "sourceLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	sourceSelect = XmVaCreateRowColumn(sourceFrame, "sourceSelect",
		XmNradioBehavior, True,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 2,
		NULL);

	/* Get the model information from the setup file.  This must be done before
	*  activating the guidance source selection button. Filter out any model which
	*  does not have any normal fields associated with it.
	*/
	for(m = 0; m < nload_sources; m++)
	{
		ls = load_sources + m;
		btn = XmVaCreateManagedToggleButton(sourceSelect, ls->btn_id, NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, source_cb, INT2PTR(m));

		SourceListByType(ls->id | SRC_HAS_DATA, FpaC_NORMAL, &src, &ls->h.number);

		if(ls->h.number > 0)
		{
			ls->h.ids   = NewMem(Source,   ls->h.number);
			ls->h.names = NewMem(XmString, ls->h.number);
			for(i = 0; i < ls->h.number; i++)
			{
				ls->h.ids[i]   = src[i];
				ls->h.names[i] = XuNewXmString(SrcLabel(src[i]));
			}
		}
		else
		{
			XtSetSensitive(btn, False);
			if(active_source == ls) active_source = (SRCINFO *)NULL;
		}
	}

	XtManageChild(sourceSelect);

	/* The following layout is rather messy, but I wanted the run time list to
	 * depend only on its own width and the model list to stretch to match the
	 * run time list. Thus if the user resizes the dialog the model list width
	 * will increase but no the run time list Trying to do this in one form
	 * resulted in the 10,000 iterations message when run in the french locale.
	 */
	guidanceFrame = XmVaCreateManagedFrame(topForm, "guidanceFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginHeight, 6,
		XmNmarginWidth, 6,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 9,
		XmNtopWidget, sourceFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(guidanceFrame, "modelSelectTitle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	guidanceForm = XmCreateForm(guidanceFrame, "guidanceForm", NULL, 0);

	form = XmVaCreateManagedForm(guidanceForm, "form",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	runTimeLabel = XmVaCreateManagedLabel(form, "issueHeader",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	runTime = XmVaCreateManagedScrolledList(form, "issueTime",
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNitemCount, 1,
		XmNitems, items,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, runTimeLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(runTime, XmNbrowseSelectionCallback, run_time_cb, NULL);

	form = XmVaCreateManagedForm(guidanceForm, "form",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, form,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	modelLabel = XmVaCreateManagedLabel(form, "modelLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	modelSelect = XmVaCreateManagedScrolledList(form, "modelSelect",
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modelLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(modelSelect, XmNbrowseSelectionCallback, model_select_cb, NULL);

	XtManageChild(guidanceForm);

	/* Create the preferences frame and its children.
	*/
	prefsFrame = XmVaCreateManagedFrame(topForm, "prefsFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(prefsFrame, "prefsLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(prefsFrame, "rc", NULL);

	btn = XmVaCreateManagedToggleButton(rc, "selectAllFields",
		XmNset, select_all_fields ? XmSET:XmUNSET,
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, prefs_cb, (XtPointer)&select_all_fields);

	btn = XmVaCreateManagedToggleButton(rc, "createAsk",
		XmNset, ask_to_create_depiction ? XmSET:XmUNSET,
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, prefs_cb, (XtPointer)&ask_to_create_depiction);

	btn = XmVaCreateManagedToggleButton(rc, "overwriteAsk",
		XmNset, ask_to_overwrite_field ? XmSET:XmUNSET,
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, prefs_cb, (XtPointer)&ask_to_overwrite_field);

	btn = XmVaCreateManagedToggleButton(rc, "showTargetTime",
		XmNset, XmSET,
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, target_time_block_cb, NULL);

	/* Create the target time frame and the targeting widgets.
	*/
	targetFrame = XmVaCreateFrame(topForm, "targetFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, prefsFrame,
		NULL);

	if (show_target_time_block) XtManageChild(targetFrame);

	(void) XmVaCreateManagedLabel(targetFrame, "targetLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateForm(targetFrame, "form",
		XmNhorizontalSpacing, 6,
		XmNverticalSpacing, 6,
		NULL);

	label = XmVaCreateManagedLabel(form, "existingDepict",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	targetSW = XmVaCreateManagedScrolledWindow(form, "targetSW",
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 4,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtVaGetValues(targetSW, XmNverticalScrollBar, &w, NULL);
	XtUnmanageChild(w);

	targetManager = XmVaCreateManagedRowColumn(targetSW, "targetManager",
		XmNpacking, XmPACK_COLUMN,
		XmNorientation, XmHORIZONTAL,
		XmNradioBehavior, True,
		NULL);

	/* Create the target depiction selection buttons.
	*/
	target_btns = NewWidgetArray(GV_ndepict);
	for ( i = 0; i < GV_ndepict; i++)
	{
		Pixel fg, bg;

		target_btns[i] = XmVaCreateToggleButton(targetManager, " ",
			XmNmarginHeight, 4,
			XmNmarginWidth, 4,
			XmNborderWidth, 0,
			XmNshadowThickness, 2,
			XmNindicatorOn, False,
			NULL);
		XtAddCallback(target_btns[i], XmNvalueChangedCallback, target_cb, INT2PTR(i));
		SetSequenceBtnTime(target_btns[i], GV_depict[i], SPECIAL_SEQUENCE);
		GetSequenceBtnColor(GV_depict[i], &fg, &bg);
		XtVaSetValues( target_btns[i],
			XmNforeground, fg,
			XmNbackground, bg,
			NULL);
	}
	XtManageChildren(target_btns, GV_ndepict);

	label = XmVaCreateManagedLabel(form, "or",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, label,
		XmNrightOffset, 0,
		NULL);

	targetTime = CreateTargetTimeControl(form,
		minutes_in_depictions() ? DATE_TO_MINUTE : DATE_TO_HOUR,
		target_time_control_cb,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(form);

	validTimeLabel = XmVaCreateManagedLabel(topForm, "validHeader",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 9,
		XmNtopWidget, guidanceFrame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	validTime = XmVaCreateManagedScrolledList(topForm, "validTime",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNitemCount, 1,
		XmNitems, items,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, validTimeLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomOffset, 9,
		XmNbottomWidget, (show_target_time_block)? targetFrame : prefsFrame,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		NULL);
	XtAddCallback(validTime, XmNbrowseSelectionCallback, valid_time_cb, NULL);

	fieldSelect = XmVaCreateManagedScrolledList(topForm, "fieldSelect",
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, XtParent(validTime),
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftOffset, 9,
		XmNleftWidget, XtParent(validTime),
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, XtParent(validTime),
		XmNtopOffset, 0,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		NULL);

	(void) XmVaCreateManagedLabel(topForm, "fieldLabel",
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, fieldSelect,
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, fieldSelect,
		XmNleftOffset, 0,
		NULL);

	XmStringFree(items[0]);

	XuToggleButtonSet(btn, show_target_time_block, True);

	XtManageChild(topForm);

	/* This can not be done previously as the parts need to be activated. */
	configure_target_btn_manager();

	return topForm;
}


void InitLoadHourlyFields(void)
{
	int     m;
	String  btn_id;
	SRCINFO *ls;

	if(IsNull(active_source))
	{
		for(m = 0; m < nload_sources; m++)
		{
			ls = load_sources + m;
			if(!ls->is_model || ls->h.number > 0)
			{
				active_source = ls;
				break;
			}
		}
	}
	btn_id = NotNull(active_source) ? active_source->btn_id:load_sources[0].btn_id;
	XuToggleButtonSet(XtNameToWidget(sourceSelect,btn_id), True, True);

	/* Bug 20071015: This must follow the source select */
	if(GV_ndepict < 1)
	{
		(void) safe_strcpy(GV_T0_depict, "");
		AppDateTime(target_time, 1);
	}
	else
	{
		(void) safe_strcpy(target_time, ActiveDepictionTime(FIELD_INDEPENDENT));
	}
	TargetTimeSetStrTime(targetTime, target_time, True);
}


/*---------------- Private Functions ----------------------*/


/* Here we set the height of the target time scrolled window. We find the
 * height of various components of the combined widget and use these. This
 * assures us that we do not need the vertical scroll bar and yet get the
 * minimum size required.
 */
static void configure_target_btn_manager(void)
{
	Widget    w = NULL;
	Dimension hm = 20, sh = 0, mh = 0, st = 0, sbh = 0, tm = 0;

	XtVaGetValues(targetManager, XmNmarginHeight, &tm, NULL);
	if(GV_ndepict > 0) XtVaGetValues(target_btns[0], XmNheight, &hm, NULL);
	XtVaGetValues(targetSW,
		XmNhorizontalScrollBar, &w,
		XmNspacing, &sh,
		XmNscrolledWindowMarginHeight, &mh,
		XmNshadowThickness, &st,
		NULL);
	if (w) XtVaGetValues(w, XmNheight, &sbh, NULL);		
	XtVaSetValues(targetSW, XmNheight, hm+sh+sbh+mh+mh+st+st+tm+tm, NULL);
}


/*ARGSUSED*/
static void source_cb( Widget w ,XtPointer client_data, XtPointer call_data )
{
	Boolean none_available = True;

	if(!XmToggleButtonGetState(w)) return;

	active_source = load_sources + PTR2INT(client_data);

	init_fld_descript(&fd);

	XuListEmpty(modelSelect);
	XuListEmpty(runTime);

	if(active_source->is_model)
	{
		XtSetSensitive(guidanceFrame, True);
		XmListAddItems(modelSelect, active_source->h.names, active_source->h.number, 0);
		XmListSelectPos(modelSelect, active_source->h.last_select, True);
	}
	else
	{
		XtSetSensitive(guidanceFrame, False);
		if(active_source->h.number > 0)
		{
			copy_fld_descript(&fd, active_source->h.ids[0]->fd);
			if(set_fld_descript(&fd, FpaF_RUN_TIME, NULL, FpaF_END_OF_LIST))
			{
				none_available = False;
				create_valid_time_list();
			}
		}
		if (none_available)
		{
			XuListEmpty(runTime);
			XuListAddItem(runTime, XuGetLabel("na"));
			XuListEmpty(validTime);
			XuListAddItem(validTime, XuGetLabel("na"));
			clear_field_list();
			XmListAddItem(fieldSelect, NA, 0);
		}
	}
}


/*ARGSUSED*/
static void model_select_cb( Widget w ,XtPointer client_data ,XtPointer call_data )
{
	int	i, ndx;
	XmString item;

	XmListCallbackStruct * rtn;

	rtn = (XmListCallbackStruct * )call_data;
	ndx = rtn->item_position - 1;

	copy_fld_descript(&fd, active_source->h.ids[ndx]->fd);

	XuListEmpty(runTime);

	/* For some of the models we need to set a run time.
	*/
	if(active_source->has_run_time)
	{
		XtSetSensitive(runTimeLabel, True);
		nrun_list = source_run_time_list_free(&run_list, nrun_list);
		nrun_list = source_run_time_list(&fd, &run_list);
		for(i = 0; i < nrun_list; i++)
		{
			item = XuNewXmString(DateString(run_list[i],HOURS));
			XmListAddItem(runTime, item, 0);
			XmStringFree(item);
		}
		XuUpdateDisplay(topForm);
		if(nrun_list > 0)
		{
			XmListSelectPos(runTime, 1, True);
		}
		else
		{
			XmListAddItem(runTime, NA, 0);
			XuListEmpty(validTime);
			XuListAddItem(validTime, XuGetLabel("na"));
			clear_field_list();
			XmListAddItem(fieldSelect, NA, 0);
		}
	}
	else
	{
		XtSetSensitive(runTimeLabel, False);
		create_valid_time_list();
	}
}


/*ARGSUSED*/
static void run_time_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	Boolean ok;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	reset_target_btns();

	if((ok = !XmStringCompare(rtn->item, NA)))
	{
		ok = set_fld_descript(&fd,
			FpaF_RUN_TIME, run_list[rtn->item_position-1],
			FpaF_ELEMENT, NULL,
			FpaF_LEVEL, NULL,
			FpaF_VALID_TIME, NULL,
			FpaF_END_OF_LIST);
		if (ok) create_valid_time_list();
	}
	if(!ok)
	{
		XuListEmpty(validTime);
		XuListAddItem(validTime, XuGetLabel("na"));
		clear_field_list();
		XmListAddItem(fieldSelect, NA, 0);
	}
}


/* This variable is used to ensure that a particular item in the valid
 * time list can not be selected multiple times. Doing so confuses the code.
 */
static int last_valid_time_list_select = -1;

/*  Makes the list of valid times. Note that if a source valid time does
 *  not contain any recognized fields we discard the given valid time.
*/
static void create_valid_time_list(void)
{
	int	i, j, nfld, active;
	String dt, *list;
	Boolean found;
	XmString item;

	last_valid_time_list_select = -1;

	XuListEmpty(validTime);
	XuListEmpty(fieldSelect);

	reset_target_btns();

	active = 1;
	nvalid_list = FilteredValidTimeListFree(&valid_list, nvalid_list);
	nvalid_list = FilteredValidTimeList(&fd, FpaC_NORMAL, &valid_list);

	if(nvalid_list > 0)
	{
		found = False;
		nfld = 0;
		list = NewStringArray(nvalid_list);
		for ( i = 0; i < nvalid_list; i++)
		{
			for(j = 0; j < GV_nfield; j++)
			{
				if(GV_field[j]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
				found = set_fld_descript(&fd,
					FpaF_ELEMENT, GV_field[j]->info->element,
					FpaF_LEVEL, GV_field[j]->info->level,
					FpaF_VALID_TIME, valid_list[i],
					FpaF_END_OF_LIST);
				if(found && (found = check_retrieve_metasfc(&fd))) break;
			}
			if(found)
			{
				list[nfld] = valid_list[i];

				dt = DepictFieldDateFormat(GV_field[j], list[nfld]);

				if(!blank(fd.rtime))
				{
					String toff = TimeOffsetString(fd.rtime,list[nfld]);
					item = XuNewXmStringFmt("%s - %s", toff, dt);
				}
				else if(valid_tstamp(GV_T0_depict))
				{
					String toff = TimeOffsetString(GV_T0_depict,list[nfld]);
					item = XuNewXmStringFmt("%s - %s", toff, dt);
				}
				else
				{
					item = XuNewXmStringFmt("%s        ", dt);
				}
				XmListAddItem(validTime, item, 0);
				XmStringFree(item);
				if(IsActiveDepiction(list[nfld])) active = nfld + 1;
				nfld++;
			}
			else
			{
				FreeItem(valid_list[i]);
			}
		}
		XuUpdateDisplay(topForm);
		FreeItem(valid_list);
		nvalid_list = nfld;
		valid_list = list;
		if(nfld > 0)
		{
			XmListSelectPos(validTime, active, True);
		}
		else
		{
			XmListAddItem(validTime, NA, 0);
			clear_field_list();
			XmListAddItem(fieldSelect, NA, 0);
		}
	}
	else
	{
		XmListAddItem(validTime, NA, 0);
		clear_field_list();
		XmListAddItem(fieldSelect, NA, 0);
	}
}


/*ARGSUSED*/
static void valid_time_cb(Widget w ,XtPointer client_data ,XtPointer call_data )
{
	int	ndx;
	XmListCallbackStruct *rtn = (XmListCallbackStruct * )call_data;

	if( rtn->item_position == last_valid_time_list_select ) return;

	last_valid_time_list_select = rtn->item_position;

	reset_target_btns();
	active_target = -1;

	if( XmStringCompare(rtn->item, NA) )
	{
		clear_field_list();
		XmListAddItem(fieldSelect, NA, 0);
	}
	else
	{
		ndx = rtn->item_position - 1;
		if(set_fld_descript(&fd, FpaF_VALID_TIME, valid_list[ndx], FpaF_END_OF_LIST))
		{
			create_field_list();
		}
		else
		{
			clear_field_list();
			XmListAddItem(fieldSelect, NA, 0);
		}
		(void) safe_strcpy(target_time, valid_list[ndx]);
		if(active_source->is_depict)
		{
			if(rtn->item_position == GV_ndepict)
				(void) safe_strcpy(target_time, calc_valid_time(target_time, 12));
			else
				(void) safe_strcpy(target_time, GV_depict[rtn->item_position]);
		}
		TargetTimeSetStrTime(targetTime, target_time, True);
	}
}


/*  The list of fields which may be selected is generated by looking at every
*   field as a possible candidate.  The element and level are used to generate
*   the guidance file name and then the existance of the guidance file is
*   confirmed before putting the field into the list.
*/
static void create_field_list(void)
{
	int i;
	XmString *list;

	XuListEmpty(fieldSelect);

	FreeItem(field_list);
	nfield_list = 0;
	field_list = NewMem(int, GV_nfield);
	list = NewMem(XmString, GV_nfield);

	for ( i = 0; i < GV_nfield; i++)
	{
		if(GV_field[i]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
		if(!set_fld_descript(&fd,
			FpaF_ELEMENT, GV_field[i]->info->element,
			FpaF_LEVEL, GV_field[i]->info->level,
			FpaF_END_OF_LIST)) continue;
		if(!check_retrieve_metasfc(&fd)) continue;
		field_list[nfield_list] = i;
		list[nfield_list] = XuNewXmString(GV_field[i]->info->sh_label);
		nfield_list++;
	}

	if(nfield_list < 1)
	{
		nfield_list = 1;
		list[0] = XmStringCopy(NA);
	}

	XtVaSetValues(fieldSelect,
		XmNitemCount, nfield_list,
		XmNitems, list,
		XmNselectedItemCount, (select_all_fields && nfield_list > 0)? nfield_list:0,
		XmNselectedItems, list,
		NULL);
	XuUpdateDisplay(topForm);

	XmStringArrayFree(list, nfield_list);
}


static void clear_field_list(void)
{
	XuListEmpty(fieldSelect);
    FreeItem(field_list);
	nfield_list = 0;
}


/*ARGSUSED*/
static void target_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;

	active_target = PTR2INT(client_data);
	(void) safe_strcpy(target_time,GV_depict[active_target]);
	TargetTimeSetStrTime(targetTime, target_time, False);
	reset_target_btns();
}


static void reset_target_btns(void)
{
	int i;
	Pixel fg, bg;

	if (!active_source) return;

	for ( i = 0; i < GV_ndepict; i++)
	{
		if(same(GV_depict[i], target_time))
		{
			bg = XuLoadColorResource(target_btns[i], RNarmColorBg, "Black");
			fg = XuLoadColorResource(target_btns[i], RNarmColorFg, "White");
			XtVaSetValues(target_btns[i],
				XmNset, True,
				XmNbackground, bg,
				XmNforeground, fg,
				XmNsensitive, !(active_source->is_depict && same(fd.vtime,GV_depict[i])),
				NULL);
		}
		else
		{
			GetSequenceBtnColor(GV_depict[i], &fg, &bg);
			XtVaSetValues(target_btns[i],
				XmNset, False,
				XmNbackground, bg,
				XmNforeground, fg,
				XmNsensitive, !(active_source->is_depict && same(fd.vtime,GV_depict[i])),
				NULL);
		}
	}
}


/*ARGSUSED*/
static void target_time_control_cb( Widget w )
{
	int posn, del;
	String dt;
	
	active_target = -1;
	reset_target_btns();
	dt = TargetTimeGetStrTime(w);
	if(InTimeList(dt, GV_depict, GV_ndepict, &posn))
	{
		if(NotNull(active_source) && active_source->is_depict && same(dt,fd.vtime))
		{
			del = (HourDif(target_time,dt) >= 0)? 1:-1;
			TargetTimeSetStrTime(targetTime, calc_valid_time(dt,del), True);
		}
		else
		{
			active_target = posn;
			XuToggleButtonSet(target_btns[active_target], True, True);
		}
	}
	else
	{
		(void) safe_strcpy(target_time, dt);
	}
}
	

static void action_import(String save , int  nsel_list , int  *sel_list )
{
	int	 k;
	FIELD_INFO *fld;

	if(same(save, "yes"))
	{
		(void) IngredVaCommand(GE_SEQUENCE, "SAVE_DEPICTION ALL %s", target_time);
	}
	for ( k = 0; k < nsel_list; k++)
	{
		fld = GV_field[field_list[sel_list[k]-1]];

		(void) IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s %s %s %s %s %s %s",
			fd.sdef->name, (blank(fd.subdef->name))? "-":fd.subdef->name,
			(blank(fd.rtime))? "-":fd.rtime,
			fld->info->element->name, fld->info->level->name,
			fd.vtime, target_time);
	}
	depict_seq_change = True;
}


/* Note: When coming into this function the busy cursor will have been set to
*        on by the calling function. This must be turned off when any of the
*        prompt dialogs are used.
*/
void ImportHourlyFields(Widget w)
{
	int k, response, nsel_list, *sel_list, nlist;
	char mbuf[128];
	String *elm, *lev, tz, notify[1];
	XmString item;
	Boolean field_selected, exists, is_backup;

	field_selected = XmListGetSelectedPos(fieldSelect, &sel_list, &nsel_list);
	if(!field_selected)
	{
		XuSetBusyCursor(OFF);
		XuShowError(w, "NoImportSelect", NULL);
	}
	else if(active_target < 0)
	{
		response = XuYES;
		if(ask_to_create_depiction)
		{
			XuSetBusyCursor(OFF);
			response = XuAskUser(w, "CreateDepiction", NULL);
			XuSetBusyCursor(ON);
		}
		if(response == XuYES && CreateDepiction(target_time))
		{
			action_import(NULL, nsel_list, sel_list);
			target_btns = MoreWidgetArray(target_btns, GV_ndepict);
			k = GV_ndepict - 1;
			target_btns[k] = XmVaCreateManagedToggleButton(targetManager, " ",
				XmNmarginHeight, 4,
				XmNmarginWidth, 4,
				XmNborderWidth, 0,
				XmNshadowThickness, 2,
				XmNindicatorOn, False,
				NULL);
			XtAddCallback(target_btns[k], XmNvalueChangedCallback, target_cb, INT2PTR(k));
			for(k = 0; k < GV_ndepict; k++)
			{
				tz = valid_tstamp(GV_T0_depict)? GV_T0_depict:GV_depict[0];
				item = XuNewXmStringFmt("%.2d", HourDif(tz, GV_depict[k]));
				XtVaSetValues(target_btns[k], XmNlabelString, item, NULL);
				XmStringFree(item);
				if(same(GV_depict[k],target_time))
					XuToggleButtonSet(target_btns[k], True, True);
			}
			configure_target_btn_manager();
		}
	}
	else
	{
		exists = False;
		(void) snprintf(mbuf, sizeof(mbuf), "FIELDS %s", target_time);
		(void) GEStatus(mbuf, &nlist, &elm, &lev, NULL);
		for ( k = 0; k < nsel_list; k++)
		{
			if(!InFieldList(GV_field[field_list[sel_list[k]-1]], nlist, elm, lev, NULL)) continue;
			exists = True;
			break;
		}
		if(exists)
		{
			is_backup = (active_source->id == SRC_BACKUP && same(fd.vtime,target_time));
			if(!ask_to_overwrite_field)
			{
				action_import(is_backup? "no":"yes", nsel_list, sel_list);
			}
			else if(is_backup)
			{
				XuSetBusyCursor(OFF);
				response = XuAskUser(w, "field_overwrite_confirm", NULL);
				XuSetBusyCursor(ON);
				if(response == XuYES) action_import("no", nsel_list, sel_list);
			}
			else
			{
				XuSetBusyCursor(OFF);
				response = XuMakeActionRequest(w, XuYNC, "import_confirm", NULL);
				XuSetBusyCursor(ON);
				if(response == XuYES) action_import("yes", nsel_list, sel_list);
				if(response == XuNO ) action_import("no", nsel_list, sel_list);
			}
		}
		else
		{
			action_import(NULL, nsel_list, sel_list);
		}
	}
	if (field_selected) FreeItem(sel_list);
	notify[0] = (String) ((long) depict_seq_change);
	NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);
	depict_seq_change = False;
}


/*ARGSUSED*/
static void prefs_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	XmToggleButtonCallbackStruct *rtn = (XmToggleButtonCallbackStruct *)call_data;
	*((Boolean *)client_data) = (Boolean)rtn->set;
}


/*ARGSUSED*/
static void target_time_block_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	XmToggleButtonCallbackStruct *rtn = (XmToggleButtonCallbackStruct *)call_data;

	if((show_target_time_block = (Boolean)rtn->set))
	{
		XtManageChild(targetFrame);
		XtVaSetValues(XtParent(validTime), XmNbottomWidget, targetFrame, NULL);
	}
	else
	{
		XtVaSetValues(XtParent(validTime), XmNbottomWidget, prefsFrame, NULL);
		XtUnmanageChild(targetFrame);
	}
}


/* On exit check the existance state of all normal fields.
*/
void LoadHourlyFieldsExit(void)
{
	int  n;
	char mbuf[128];

	(void) snprintf(mbuf, sizeof(mbuf), "%d %d %d %d", select_all_fields, ask_to_create_depiction,
			ask_to_overwrite_field, show_target_time_block);
	XuStateDataSave("cnf","pref",NULL, mbuf);

	nrun_list   = source_run_time_list_free(&run_list, nrun_list);
	nvalid_list = FilteredValidTimeListFree(&valid_list, nvalid_list);

	FreeItem(target_btns);
	FreeItem(field_list);

	XmStringFree(NA);

	for( n = 0; n < nload_sources; n++ )
	{
		XmStringArrayFree(load_sources[n].h.names, load_sources[n].h.number);
		FreeItem(load_sources[n].h.ids);
	}
}
