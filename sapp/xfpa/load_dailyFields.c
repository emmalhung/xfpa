/*========================================================================*/
/*
*   File:	   load_dailyFields.c
*
*   Purpose:	Provides the process for creating new (empty) Daily fields
*			   or for loading Daily Fields into the depiction sequence.
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
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include "resourceDefines.h"
#include "selector.h"
#include "observer.h"
#include "loadFields.h"

static void create_field_list      (XmListCallbackStruct *);
static void create_valid_time_list (void);
static void model_select_cb        (Widget, XtPointer, XtPointer);
static void issue_time_cb          (Widget, XtPointer, XtPointer);
static void source_cb              (Widget, XtPointer, XtPointer);
static void valid_time_cb          (Widget, XtPointer, XtPointer);
static void prefs_cb               (Widget, XtPointer, XtPointer);

typedef struct {
	char time[18];
	int nflds;
	FpaConfigFieldStruct **flds;
	String *valid;
} DailyStruct;

static XmString     NA                = NULL;
static SRCINFO      *active_source    = (SRCINFO *)0;
static int          nruns             = 0;
static String       *runs             = NULL;
static int          ndaily            = 0;
static DailyStruct  *daily            = (DailyStruct *)NULL;
static DailyStruct  *active_daily     = (DailyStruct *)NULL;
static String       no_time           = "              ";
static XmString     no_time_item[1];
static Boolean      select_all_fields = True;
static FLD_DESCRIPT fd;
static Widget       topForm;
static Widget       targetTime;
static Widget       issueTime;
static Widget       issueTimeLabel;
static Widget       validTime;
static Widget       fieldList;
static Widget       modelSelect;
static Widget       guidanceFrame;
static Widget       sourceSelect;


static void free_daily_struct(void)
{
	int i;
	for(i = 0; i < ndaily; i++)
	{
		FreeList(daily[i].valid, daily[i].nflds);
		FreeItem(daily[i].flds);
	}
	FreeItem(daily);
	ndaily = 0;
}


Widget CreateLoadDailyFields(Widget parent)
{
	int	i, m;
	SourceList src;
	SRCINFO *ls;
	XmString items[1];
	Widget frame, modelLabel, rc, btn;
	Widget guidanceForm;
	Widget validTimeLabel;

	static char blanks[] = {"					"};

	NA = XuNewXmString(XuGetLabel("na"));
	no_time_item[0] = XuNewXmString(no_time);

    topForm = XmVaCreateForm( parent, "loadDailyFields",
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
		ls = load_sources + m;
		btn = XmVaCreateManagedToggleButton(sourceSelect, ls->btn_id, NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, source_cb, INT2PTR(m));

		SourceListByType(ls->id | SRC_HAS_DATA, FpaC_DAILY, &src, &ls->d.number);

		if(ls->d.number > 0)
		{
			ls->d.ids   = NewMem(Source,   ls->d.number);
			ls->d.names = NewMem(XmString, ls->d.number);
			for(i = 0; i < ls->d.number; i++)
			{
				ls->d.ids[i]   = src[i];
				ls->d.names[i] = XuNewXmString(SrcLabel(src[i]));
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

	issueTimeLabel =  XtVaCreateManagedWidget("issueHeader",
		xmLabelWidgetClass, guidanceForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, 	XtParent(issueTime),
		XmNbottomOffset, 0,
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

	/* Create the preferences frame and its children.
	*/
	frame = XtVaCreateManagedWidget("prefsFrame",
		xmFrameWidgetClass, topForm,
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	(void) XtVaCreateManagedWidget("prefsLabel",
		xmLabelWidgetClass, frame,
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XtVaCreateManagedWidget("rc",
		xmRowColumnWidgetClass, frame,
		NULL);

	btn = XtVaCreateManagedWidget("selectAllFields",
		xmToggleButtonWidgetClass, rc,
		XmNset, select_all_fields,
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, prefs_cb, (XtPointer)&select_all_fields);

	targetTime = CreateTargetTimeControl(topForm, DATE_TO_DAY, NULL,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, frame,
		NULL);

	validTimeLabel = XtVaCreateManagedWidget("validHeader",
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
		XmNtopWidget, validTimeLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, targetTime,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		NULL);
	XtAddCallback(validTime, XmNbrowseSelectionCallback, valid_time_cb, NULL);

	items[0] = XuNewXmString(blanks);
	fieldList = XmVaCreateManagedScrolledList(topForm, "fieldList",
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNitemCount, 1,
		XmNitems, items,
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, validTimeLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, 	XtParent(validTime),
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, targetTime,
		NULL);
	XmStringFree(items[0]);

	(void) XtVaCreateManagedWidget("fieldLabel",
		xmLabelWidgetClass, topForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, XtParent(fieldList),
		XmNbottomOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, XtParent(fieldList),
		XmNleftOffset, 0,
		NULL);

	XtManageChild(topForm);
	return topForm;
}


void InitLoadDailyFields(void)
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


/*ARGSUSED*/
static void source_cb(Widget	w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;

	active_source = load_sources + PTR2INT(client_data);

	XuListEmpty(modelSelect);
	XuListEmpty(issueTime);
	XtSetSensitive(issueTime, False);
	XtSetSensitive(issueTimeLabel, False);
	XtSetSensitive(guidanceFrame, active_source->is_model);

	init_fld_descript(&fd);

	if(active_source->is_model)
	{
		XmListAddItems(modelSelect, active_source->d.names, active_source->d.number, 0);
		XmListSelectPos(modelSelect, active_source->d.last_select, True);
	}
	else
	{
		if(active_source->d.number > 0)
		{
			copy_fld_descript(&fd, active_source->d.ids[0]->fd);
			if(set_fld_descript(&fd, FpaF_RUN_TIME, GV_T0_depict, FpaF_END_OF_LIST))
			{
				create_valid_time_list();
			}
			else
			{
				XuListEmpty(validTime);
				XmListAddItem(validTime, NA, 0);
			}
		}
		else
		{
			XuListEmpty(validTime);
			XmListAddItem(validTime, NA, 0);
		}
	}
}


/*ARGSUSED*/
static void model_select_cb(Widget	w , XtPointer client_data , XtPointer call_data )
{
	int	i, ndx;
	XmString item;

	XmListCallbackStruct * rtn;

	rtn = (XmListCallbackStruct * )call_data;
	active_source->d.last_select = rtn->item_position;
	ndx = rtn->item_position - 1;

	copy_fld_descript(&fd, active_source->d.ids[ndx]->fd);

	XuListEmpty(issueTime);
	XtSetSensitive(issueTime, False);
	XtSetSensitive(issueTimeLabel, False);

	if(active_source->has_run_time)
	{
		(void)source_run_time_list_free(&runs, nruns);
		nruns = source_run_time_list(&fd, &runs);
		if( nruns > 0 && set_fld_descript(&fd, FpaF_RUN_TIME, runs[0], FpaF_END_OF_LIST))
		{
			XtSetSensitive(issueTime, True);
			XtSetSensitive(issueTimeLabel, True);
			for ( i = 0; i < nruns; i++)
			{
				item = XuNewXmString(DateString(runs[i],HOURS));
				XmListAddItem(issueTime, item, 0);
				XmStringFree(item);
			}
			XmListSelectPos(issueTime, 1, True);
		}
	}
	else
	{
		create_valid_time_list();
	}
}


/*ARGSUSED*/
static void issue_time_cb(Widget	w , XtPointer client_data , XtPointer call_data )
{
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;
	if(XmStringCompare(rtn->item,NA))
	{
		XuListEmpty(validTime);
		XuListEmpty(fieldList);
		XmListAddItem(validTime, NA, 0);
		XmListSelectPos(validTime, 1, True);
	}
	else
	{
		(void) set_fld_descript(&fd,
			FpaF_RUN_TIME, runs[rtn->item_position-1],
			FpaF_VALID_TIME, NULL,
			FpaF_END_OF_LIST);
		create_valid_time_list();
	}
}


static void create_valid_time_list(void)
{
	int i, j, k, nvl;
	String  *vl;
	Boolean found;
	XmString item;
	DailyStruct *p, d;

	free_daily_struct();
	XuListEmpty(validTime);
	XuListEmpty(fieldList);

	for(i = 0; i < GV_nfield; i++)
	{
		if(GV_field[i]->info->element->elem_tdep->time_dep != FpaC_DAILY) continue;
		if(GV_field[i]->info->element->fld_type == FpaC_ELEM_NOTUSED) continue;

		(void) set_fld_descript(&fd,
			FpaF_ELEMENT, GV_field[i]->info->element,
			FpaF_LEVEL, GV_field[i]->info->level,
			FpaF_VALID_TIME, NULL,
			FpaF_END_OF_LIST);

		nvl = FilteredValidTimeList(&fd, FpaC_DAILY, &vl);

		for(j = 0; j < nvl; j++)
		{
			int yr1, jd1, yr2, jd2;

			(void) set_fld_descript(&fd, FpaF_VALID_TIME, vl[j], FpaF_END_OF_LIST);
			if(!check_retrieve_metasfc(&fd)) continue;
			p = (DailyStruct *)NULL;
			(void)parse_tstamp(vl[j], &yr1, &jd1, NULL, NULL, NULL, NULL);
			for(k = 0; k < ndaily; k++)
			{
				(void)parse_tstamp(daily[k].time, &yr2, &jd2, NULL, NULL, NULL, NULL);
				if(yr1 != yr2 || jd1 != jd2) continue;
				p = daily + k;
				break;
			}
			if(!p)
			{
				daily = MoreMem(daily, DailyStruct, ndaily+1);
				p = daily + ndaily;
				strcpy(p->time, vl[j]);
				p->nflds = 0;
				p->flds = (FpaConfigFieldStruct **)NULL;
				p->valid = (String *)NULL;
				ndaily++;
			}

			found = False;
			for(k = 0; k < p->nflds; k++)
			{
				if(GV_field[i]->info != p->flds[k]) continue;
				found = True;
				break;
			}

			if(found) continue;

			p->flds = MoreMem(p->flds, FpaConfigFieldStruct*, p->nflds+1);
			p->flds[p->nflds] = GV_field[i]->info;
			p->valid = MoreStringArray(p->valid, p->nflds+1);
			p->valid[p->nflds] = XtNewString(vl[j]);
			p->nflds++;
		}
		nvl = FilteredValidTimeListFree(&vl, nvl);
	}

	/* Put the daily field structures into time order.
	*/
	found = True;
	while(found)
	{
		int yr1, yr2, jd1, jd2;
		found = False;
		for(i = 1; i < ndaily; i++)
		{
			(void)parse_tstamp(daily[i-1].time, &yr1, &jd1, NULL, NULL, NULL, NULL);
			(void)parse_tstamp(daily[i].time,   &yr2, &jd2, NULL, NULL, NULL, NULL);
			if(jorder(yr1,jd1, yr2,jd2) >= 0) continue;
			found = True;
			CopyStruct(&d,          &daily[i-1], DailyStruct, 1);
			CopyStruct(&daily[i-1], &daily[i],   DailyStruct, 1);
			CopyStruct(&daily[i],   &d,          DailyStruct, 1);
		}
	}

	if(ndaily > 0)
	{
		for(i = 0; i < ndaily; i++)
		{
			item = XuNewXmString(DateString(daily[i].time,DAYS));
			XmListAddItem(validTime, item, 0);
			XmStringFree(item);
		}
	}
	else
	{
		XuListEmpty(validTime);
		XmListAddItem(validTime, NA, 0);
	}
	XmListSelectPos(validTime, 1, True);
}


/*ARGSUSED*/
static void valid_time_cb(Widget	w , XtPointer client_data , XtPointer call_data )
{
	int sel;
	String target_time;
	XmListCallbackStruct *rtn = (XmListCallbackStruct * )call_data;

	/* Find the selected valid time
	*/
	if(XmStringCompare(rtn->item,NA)) return;
	sel = rtn->item_position - 1;
	active_daily = daily + sel;

	/* Determine what the target time default should be.  What we do
	*  depends on the source type.  For all but the sequence the target
	*  will be the same as the valid time.  For fields from the sequence
	*  if the most recent valid time was selected set the target time to
	*  this time + 24 hours.  If not set to the next valid time
	*  in the list.
	*/
	if(active_source->is_depict)
	{
		if( sel < ndaily - 1 )
			target_time = daily[sel+1].time;
		else
			target_time = calc_valid_time(daily[sel].time, 24);
	}
	else
	{
		target_time = daily[sel].time;
	}
	TargetTimeSetStrTime(targetTime, target_time, False);
	create_field_list(rtn);
}


static void create_field_list(XmListCallbackStruct *rtn )

{
	int i, nfld;
	XmString *labels;

	XuListEmpty(fieldList);

	if(rtn && XmStringCompare(rtn->item,NA)) return;

	nfld   = 0;
	labels = NewMem(XmString, GV_nfield);

	if(active_daily)
	{
		for(i = 0; i < active_daily->nflds; i++)
		{
			labels[nfld] = XuNewXmString(active_daily->flds[i]->sh_label);
			nfld++;
		}
	}

	if(nfld == 0)
	{
		XmListAddItem(fieldList, NA, 0);
		XmListSelectPos(fieldList, 1, False);
	}
	else
	{
		XtVaSetValues(fieldList,
			XmNitemCount, nfld,
			XmNitems, labels,
			XmNselectedItemCount, select_all_fields? nfld:1,
			XmNselectedItems, labels,
			NULL);
	}
	XmStringArrayFree(labels, nfld);
}

/*ARGSUSED*/
static void prefs_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XmToggleButtonCallbackStruct *rtn = (XmToggleButtonCallbackStruct *)call_data;
	*((Boolean *)client_data) = (Boolean)rtn->set;
}


void ImportDailyFields(Widget w)
{
	int      i, ndx, nsl, *sl;
	int      year, jday, hour;
	char     mbuf[512];
	String   target_time, notify[1] = {(String) False};
	XmString *selected_items;

	target_time = TargetTimeGetStrTime(targetTime);

	if(!XmListGetSelectedPos(fieldList, &sl, &nsl))
	{
		XuSetBusyCursor(OFF);
		XuShowError(w, "NoImportSelect", NULL);
	}
	else
	{
		/* The hour specific to the daily field must be substituted into the
		*  target time.
		*/
		double dhr;
		XtVaGetValues(fieldList, XmNselectedItems, &selected_items, NULL);
		for(i = 0; i < nsl; i++)
		{
			if(XmStringCompare(selected_items[i],NA)) continue;
			ndx = sl[i]-1;

			(void)parse_tstamp(target_time, &year, &jday, NULL, NULL, NULL, NULL);
			(void)convert_value(active_daily->flds[ndx]->element->elem_tdep->units->name,
								active_daily->flds[ndx]->element->elem_tdep->normal_time,
								"hr", &dhr);
			hour = NINT(dhr);
			(void) tnorm(&year, &jday, &hour, NullInt, NullInt);

			(void) snprintf(mbuf, sizeof(mbuf), "GET_FIELD %s %s %s %s %s %s %s",
				fd.sdef->name, blank(fd.subdef->name)? "-":fd.subdef->name,
				blank(fd.rtime)? "-":fd.rtime,
				active_daily->flds[ndx]->element->name,
				active_daily->flds[ndx]->level->name,
				active_daily->valid[ndx],
				build_tstamp(year,jday,hour,0,True,minutes_in_depictions()));

			(void) IngredCommand(GE_SEQUENCE, mbuf);
		}
		FreeItem(sl);
		NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);
	}
}


/* Upon exiting update the existance status of the various daily fields.
*/
void LoadDailyFieldsExit(void)
{
	int n;

	nruns = source_run_time_list_free(&runs, nruns);

	free_daily_struct();

	XmStringFree(NA);
	XmStringFree(no_time_item[0]);
	
	for( n = 0; n < nload_sources; n++ )
	{
		XmStringArrayFree(load_sources[n].d.names, load_sources[n].d.number);
		FreeItem(load_sources[n].d.ids);
	}
}
