/*========================================================================*/
/*
*   File:       load_staticFields.c
*
*   Purpose:    Provides the process for creating new (empty) Static fields
*               or for copying Static Fields into the depiction sequence.
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
#include <Xm/ToggleB.h>
#include "depiction.h"
#include "resourceDefines.h"
#include "selector.h"
#include "observer.h"
#include "loadFields.h"


/* Local functions and variables */
static void create_field_list (XmListCallbackStruct *);
static void model_select_cb   (Widget, XtPointer, XtPointer);
static void field_cb          (Widget, XtPointer, XtPointer);
static void issue_time_cb     (Widget, XtPointer, XtPointer);
static void source_cb         (Widget, XtPointer, XtPointer);
static void valid_time_cb     (Widget, XtPointer, XtPointer);

typedef struct {
	FpaConfigFieldStruct *fld;
	int nvalid;
	String *valid;
} StaticStruct;

static XmString     NA = NULL;
static SRCINFO      *active_source = NULL;
static int          nfield = 0;
static StaticStruct *field = (StaticStruct *)NULL;
static StaticStruct *sel_field = (StaticStruct *)NULL;
static FLD_DESCRIPT fd;
static int          len_issue_list = 0;
static String       *issue_list = NULL;
static char         valid_time[20];
static String       no_time = "              ";
static XmString     no_time_item[1];
static Widget       topForm;
static Widget       targetTime;
static Widget       issueTime;
static Widget       issueTimeLabel;
static Widget       validTime;
static Widget       fieldSelect;
static Widget       modelSelect;
static Widget       guidanceFrame;
static Widget       sourceSelect;


Widget CreateLoadStaticFields(Widget parent)
{
	int	i, m;
	SourceList src;
	SRCINFO *ls;
	Widget frame, fieldLabel, modelLabel;
	Widget guidanceForm;

	if(!NA)
	{
		NA = XuNewXmString(XuGetLabel("na"));
		no_time_item[0] = XuNewXmString(no_time);
	}

	topForm = XmVaCreateForm(parent, "loadStaticFields",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

    frame = XtVaCreateManagedWidget("sourceFrame",
        xmFrameWidgetClass, topForm,
        XmNshadowType, XmSHADOW_ETCHED_IN,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
        NULL);

    (void)XtVaCreateManagedWidget("sourceLabel",
        xmLabelWidgetClass, frame,
        XmNchildType, XmFRAME_TITLE_CHILD,
        NULL);

	sourceSelect = XtVaCreateWidget("sourceSelect",
		xmRowColumnWidgetClass, frame,
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
		Widget btn;

		ls = load_sources + m;
		btn = XmVaCreateManagedToggleButton(sourceSelect, ls->btn_id, NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, source_cb, INT2PTR(m));

		SourceListByType(ls->id | SRC_HAS_DATA, FpaC_STATIC, &src, &ls->s.number);

		if(ls->s.number > 0)
		{
			ls->s.ids   = NewMem(Source,   ls->s.number);
			ls->s.names = NewMem(XmString, ls->s.number);
			for(i = 0; i < ls->s.number; i++)
			{
				ls->s.ids[i]   = src[i];
				ls->s.names[i] = XuNewXmString(SrcLabel(src[i]));
			}
		}
		else
		{
			XtSetSensitive(btn, False);
			if(active_source == ls) active_source = (SRCINFO *)NULL;
		}
	}
	XtManageChild(sourceSelect);

    guidanceFrame = XtVaCreateManagedWidget("frame",
        xmFrameWidgetClass, topForm,
        XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginHeight, 6,
		XmNmarginWidth, 6,
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNtopOffset, 9,
        XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
        NULL);

    (void)XtVaCreateManagedWidget("modelSelectTitle",
        xmLabelWidgetClass, guidanceFrame,
        XmNchildType, XmFRAME_TITLE_CHILD,
        NULL);

	guidanceForm = XtVaCreateWidget("guidanceForm",
		xmFormWidgetClass, guidanceFrame,
		NULL);

	modelLabel = XtVaCreateManagedWidget("modelLabel",
		xmLabelWidgetClass, guidanceForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	issueTime = XmVaCreateManagedScrolledList(guidanceForm, "issueTime",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNitemCount, 1,
		XmNitems, no_time_item,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modelLabel,
		XmNtopOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(issueTime, XmNbrowseSelectionCallback, issue_time_cb, NULL);

	issueTimeLabel = XtVaCreateManagedWidget("issueHeader",
		xmLabelWidgetClass, guidanceForm,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, XtParent(issueTime),
		XmNleftOffset, 0,
		NULL);

	modelSelect = XmVaCreateManagedScrolledList(guidanceForm, "modelSelect",
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, modelLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, XtParent(issueTime),
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(modelSelect, XmNbrowseSelectionCallback, model_select_cb, NULL);

	XtManageChild(guidanceForm);

	targetTime = CreateTargetTimeControl(topForm,
		(minutes_in_depictions()) ? DATE_TO_MINUTE:DATE_TO_HOUR, NULL,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	fieldLabel = XtVaCreateManagedWidget("fieldLabel",
		xmLabelWidgetClass, topForm,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, guidanceFrame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	validTime = XmVaCreateManagedScrolledList(topForm, "validTime",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNitemCount, 1,
		XmNitems, no_time_item,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldLabel,
		XmNtopOffset, 0,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, guidanceFrame,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, targetTime,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		NULL);
	XtAddCallback(validTime, XmNbrowseSelectionCallback, valid_time_cb, NULL);

	(void) XtVaCreateManagedWidget("validHeader",
		xmLabelWidgetClass, topForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, XtParent(validTime),
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, XtParent(validTime),
		XmNleftOffset, 0,
		NULL);

	fieldSelect = XmVaCreateManagedScrolledList(topForm, "fieldSelect",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNitemCount, 1,
		XmNitems, no_time_item,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNlistSpacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, XtParent(validTime),
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, targetTime,
		NULL);
	XtAddCallback(fieldSelect, XmNbrowseSelectionCallback, field_cb, NULL);

	XtManageChild(topForm);

	return topForm;
}


void InitLoadStaticFields(void)
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
}


static void FreeStaticStruct(void)

{
	int i;
	for(i = 0; i < nfield; i++)
	{
		FreeList(field[i].valid, field[i].nvalid);
	}
	FreeItem(field);
	nfield = 0;
}


/*ARGSUSED*/
static void source_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	Boolean none_available = True;

	if(!XmToggleButtonGetState(w)) return;

	active_source = load_sources + PTR2INT(client_data);

	init_fld_descript(&fd);

	XuListEmpty(modelSelect);
	XuListEmpty(issueTime);

	if(active_source->is_model)
	{
		XtSetSensitive(guidanceFrame, True);
		XmListAddItems(modelSelect, active_source->s.names, active_source->s.number, 0);
		XmListSelectPos(modelSelect, active_source->s.last_select, True);
	}
	else
	{
		XtSetSensitive(guidanceFrame, False);

		if(active_source->s.number > 0)
		{
			copy_fld_descript(&fd, active_source->s.ids[0]->fd);
			if(set_fld_descript(&fd, FpaF_RUN_TIME, GV_T0_depict, FpaF_END_OF_LIST))
			{
				none_available = False;
				create_field_list(NULL);
			}
		}
		if (none_available)
		{
			XuListEmpty(fieldSelect);
			XmListAddItem(fieldSelect, NA, 0);
			XuListEmpty(validTime);
		}

	}
}


/*ARGSUSED*/
static void model_select_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int	i, ndx;
	XmString item;

	XmListCallbackStruct * rtn;

	rtn = (XmListCallbackStruct * )call_data;
	active_source->s.last_select = rtn->item_position;
	ndx = rtn->item_position - 1;

	copy_fld_descript(&fd, active_source->s.ids[ndx]->fd);

	XuListEmpty(issueTime);
	XtSetSensitive(issueTime, False);
	XtSetSensitive(issueTimeLabel, False);

	if(active_source->is_depict)
	{
		create_field_list(NULL);
	}
	else
	{
		(void)source_run_time_list_free(&issue_list, len_issue_list);
		len_issue_list = source_run_time_list(&fd, &issue_list);
		if(len_issue_list > 0 && set_fld_descript(&fd, FpaF_RUN_TIME, issue_list[0], FpaF_END_OF_LIST))
		{
			XtSetSensitive(issueTime, True);
			XtSetSensitive(issueTimeLabel, True);
			for ( i = 0; i < len_issue_list; i++)
			{
				item = XuNewXmString(DateString(issue_list[i],HOURS));
				XmListAddItem(issueTime, item, 0);
				XmStringFree(item);
			}
			XmListSelectPos(issueTime, 1, True);
		}
	}
}


/*ARGSUSED*/
static void issue_time_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	create_field_list((XmListCallbackStruct *)call_data);
}


static void create_field_list(XmListCallbackStruct *rtn )

{
	int i, j, nvl;
	String *vl;
	XmString item;
	StaticStruct *p;

	FreeStaticStruct();
	XuListEmpty(fieldSelect);
	XuListEmpty(validTime);
	strcpy(valid_time, "");
	TargetTimeSetStrTime(targetTime, no_time, False);

	if(rtn)
	{
		if(XmStringCompare(rtn->item,NA))
		{
			XmListAddItem(fieldSelect, NA, 0);
			XmListSelectPos(fieldSelect, 1, True);
			return;
		}
		if(!set_fld_descript(&fd,
			FpaF_RUN_TIME, issue_list[rtn->item_position-1],
			FpaF_END_OF_LIST)) return;
	}

	for ( i = 0; i < GV_nfield; i++)
	{
		if(GV_field[i]->info->element->elem_tdep->time_dep != FpaC_STATIC) continue;
		if(GV_field[i]->info->element->fld_type == FpaC_ELEM_NOTUSED) continue;

		item = XuNewXmString(GV_field[i]->info->sh_label);
		XmListAddItem(fieldSelect, item, 0);
		XmStringFree(item);

		field = MoreMem(field, StaticStruct, nfield+1);
		p = field + nfield;
		nfield++;
		p->fld = GV_field[i]->info;
		p->nvalid = 0;
		p->valid = (String *)NULL;
		(void) set_fld_descript(&fd,
			FpaF_ELEMENT, p->fld->element,
			FpaF_LEVEL, p->fld->level,
			FpaF_END_OF_LIST);

		nvl = FilteredValidTimeList(&fd, FpaC_STATIC, &vl);
		for(j = 0; j < nvl; j++)
		{
			(void) set_fld_descript(&fd, FpaF_VALID_TIME, vl[j], FpaF_END_OF_LIST);
			if(!check_retrieve_metasfc(&fd)) continue;
			p->valid = MoreStringArray(p->valid, p->nvalid+1);
			p->valid[p->nvalid] = XtNewString(vl[j]);
			p->nvalid++;
		}
		(void)FilteredValidTimeListFree(&vl, nvl);
	}

	(void) set_fld_descript(&fd,
		FpaF_ELEMENT, NULL,
		FpaF_LEVEL, NULL,
		FpaF_VALID_TIME, NULL,
		FpaF_END_OF_LIST);

	if(nfield < 1)
	{
		XmListAddItem(fieldSelect, NA, 0);
		XmListAddItem(validTime, NA, 0);
		TargetTimeSetStrTime(targetTime, no_time, False);
	}
	XmListSelectPos(fieldSelect, 1, True);
}

/*ARGSUSED*/
static void field_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
    int i;
	char mbuf[300];
    XmString item;
	XmListCallbackStruct *rtn = (XmListCallbackStruct * )call_data;

	strcpy(valid_time, "");
	XuListEmpty(validTime);
	TargetTimeSetStrTime(targetTime, no_time, False);

	if(XmStringCompare(rtn->item, NA))
	{
		XmListAddItem(validTime, NA, 0);
	}
	else
	{
		sel_field = field + ((int)rtn->item_position - 1);
		if(sel_field->nvalid > 0)
		{
			TargetTimeSetStrTime(targetTime, no_time, False);
			for(i = 0; i < sel_field->nvalid; i++)
			{
				if(minutes_in_depictions())
					item = XuNewXmString(DateString(sel_field->valid[i],MINUTES));
				else
					item = XuNewXmString(DateString(sel_field->valid[i],HOURS));

				XmListAddItem(validTime, item, 0);
				XmStringFree(item);
			}
			if(active_source->is_depict)
			{
				String *active;

				(void) snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s",
					sel_field->fld->element->name, sel_field->fld->level->name);
				(void) GEStatus(mbuf, &i, NULL, &active, NULL);

				for(i = 0; i < sel_field->nvalid; i++)
				{
					if(HourDif(active[0], sel_field->valid[i]) != 0) continue;
					XmListSelectPos(validTime, i+1, True);
					break;
				}
				if(i >= sel_field->nvalid) XmListSelectPos(validTime, 1, True);
			}
			else
			{
				XmListSelectPos(validTime, sel_field->nvalid, True);
				XmListSetBottomPos(validTime, sel_field->nvalid);
			}
			XmListSelectPos(validTime, sel_field->nvalid, True);
			XmListSetBottomPos(validTime, sel_field->nvalid);
		}
		else
		{
			XmListAddItem(validTime, NA, 0);
		}
	}
}


/*ARGSUSED*/
static void valid_time_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int index;
	String target_time;
	XmListCallbackStruct *rtn = (XmListCallbackStruct * )call_data;

	strcpy(valid_time, "");
	if(XmStringCompare(rtn->item, NA)) return;

	index = rtn->item_position - 1;
	strcpy(valid_time, sel_field->valid[index]);

	/* Determine what the target time default should be.  What we do 
	*  depends on the source type.  For all but the sequence the target
	*  will be the same as the valid time.  For fields from the sequence
	*  if the most recent valid time was selected set the target time to
	*  this time + 24 hours.  If not set to the next valid time
	*  in the list.
	*/
	if(active_source->is_depict)
	{
		if(valid_tstamp(ActiveDepictionTime(FIELD_INDEPENDENT)))
			target_time = ActiveDepictionTime(FIELD_INDEPENDENT);
		else if( index < sel_field->nvalid - 1 )
			target_time = sel_field->valid[index+1];
		else
			target_time = calc_valid_time(sel_field->valid[index], 24);
	}
	else
	{
		target_time = valid_time;
	}
	TargetTimeSetStrTime(targetTime, target_time, False);
}


void ImportStaticFields(Widget w)
{
	String target_time;

	target_time = TargetTimeGetStrTime(targetTime);

	if(blank(valid_time))
	{
		XuSetBusyCursor(OFF);
		XuShowError(w, "NoValidTimeSelect", NULL);
	}
	else
	{
		String notify[1] = {(String) False};
		(void) IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s %s %s %s %s %s %s",
			fd.sdef->name, blank(fd.subdef->name)? "-":fd.subdef->name,
			blank(fd.rtime)? "-":fd.rtime,
			sel_field->fld->element->name, sel_field->fld->level->name,
			valid_time, target_time);
		NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);
	}
}


void LoadStaticFieldsExit()
{
	int n;

	FreeStaticStruct();
	len_issue_list = source_run_time_list_free(&issue_list, len_issue_list);
	
	for( n = 0; n < nload_sources; n++ )
	{
		XmStringArrayFree(load_sources[n].s.names, load_sources[n].s.number);
		FreeItem(load_sources[n].s.ids);
	}
}
