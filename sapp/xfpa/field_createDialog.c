/*========================================================================*/
/*
*   File:	  field_createDialog.c
*
*   Purpose:  Provides the management mechanism for creating fields.
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
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include "selector.h"
#include "depiction.h"
#include "editor.h"
#include "help.h"
#include "observer.h"

static void create_cb	(Widget, XtPointer, XtPointer);
static void exit_cb		(Widget, XtPointer, XtPointer);
static void list_cb		(Widget, XtPointer, XtPointer);
static void type_cb		(Widget, XtPointer, XtPointer);


static Widget     dialog = NullWidget;
static Widget     fieldSelect;
static Widget     targetTime;
static Widget     createBtn;
static XmString   NA[1];
static int        nflds;
static FpaConfigFieldStruct **flds;

static struct {
	FpaCtimeDepTypeOption time_dep;
	String                btn_name;
	DATE_DISPLAY          date_type;
} fld_type[] = {
	{ FpaC_NORMAL, "normalFields", DATE_TO_HOUR },
	{ FpaC_DAILY , "dailyFields" , DATE_TO_DAY  },
	{ FpaC_STATIC, "staticFields", DATE_TO_HOUR }
};

static XuDialogActionsStruct action_items[] = {
	{ "createBtn", create_cb,            NULL },
	{ "closeBtn",  XuDestroyDialogCB,   NULL }
};


void ACTIVATE_createFieldsDialog(Widget parent )
{
	int    n;
	Widget label, frame, rc, btn;

	if(dialog) return;

	set_locale_from_environment(LC_TIME);

	/* Are minutes allowed in the depictions?
	*/
	if (minutes_in_depictions())
	{
		fld_type[0].date_type = DATE_TO_MINUTE;
		fld_type[2].date_type = DATE_TO_MINUTE;
	}

	NA[0] = XuGetXmLabel("none_found");
	flds  = NewMem(FpaConfigFieldStruct*, GV_nfield);

    dialog = XuCreateFormDialog( parent, "createFields",
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XuNdestroyCallback, exit_cb,
		XmNresizePolicy, XmRESIZE_GROW,
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	frame = XmVaCreateManagedFrame(dialog, "frame",
		XmNmarginWidth, 5,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "typeLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(frame, "rc",
		XmNradioBehavior, True,
		XmNorientation, XmVERTICAL,
		NULL);

	for( n = 0; n < XtNumber(fld_type); n++ )
	{
		btn = XmVaCreateManagedToggleButton(rc, fld_type[n].btn_name, NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, type_cb, INT2PTR(n));
	}

	label = XmVaCreateManagedLabel(dialog, "fieldSelect",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	targetTime = CreateTargetTimeControl(dialog, fld_type[0].date_type, NULL,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	fieldSelect = XmVaCreateManagedScrolledList(dialog, "sl",
		XmNlistMarginHeight, 5,
		XmNlistMarginWidth, 5,
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, targetTime,
		NULL);
	XtAddCallback(fieldSelect, XmNextendedSelectionCallback, list_cb, NULL);

	createBtn = XuGetActionAreaBtn(dialog, action_items[0]);
	XtSetSensitive(createBtn, False);

	XuShowDialog(dialog);
	XuDelay(dialog, 100);

	XuToggleButtonSet(XtNameToWidget(rc,fld_type[0].btn_name), True, True);
}


/*ARGSUSED*/
static void type_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int      i, nsl, *sl;
	int      ndx = PTR2INT(client_data);
	String   dt;
	XmString *labels;

	if(!XmToggleButtonGetState(w)) return;

	nflds  = 0;
	labels = NewMem(XmString, GV_nfield);

	for ( i = 0; i < GV_nfield; i++)
	{
		if( GV_field[i]->info->element->elem_tdep->time_dep != fld_type[ndx].time_dep ) continue;
		if( GV_field[i]->info->element->fld_type == FpaC_CONTINUOUS   ) continue;
		if( GV_field[i]->info->element->fld_type == FpaC_VECTOR       ) continue;
		if( GV_field[i]->info->element->fld_type == FpaC_ELEM_NOTUSED ) continue;
		flds[nflds]   = GV_field[i]->info;
		labels[nflds] = XuNewXmString(GV_field[i]->info->label);
		nflds++;
	}

    XtVaSetValues(fieldSelect,
		XmNitemCount, (nflds < 1) ?  1 : nflds,
		XmNitems,     (nflds < 1) ? NA : labels,
		XmNselectedItemCount, 0,
		NULL);

	XmStringArrayFree(labels, nflds);

	SetT0Depiction(T0_INITIALIZE_NEW_ONLY);
	dt = ActiveDepictionTime(FIELD_INDEPENDENT);
	TargetTimeSetFormatType(targetTime, fld_type[ndx].date_type);
	TargetTimeSetStrTime(targetTime, valid_tstamp(dt) ? dt:GV_T0_depict, False);
	XtSetSensitive(createBtn, (XmListGetSelectedPos(w, &sl, &nsl) && nsl > 0));
}


/*ARGSUSED*/
static void list_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int nsl, *sl;
	XtSetSensitive(createBtn, (XmListGetSelectedPos(w, &sl, &nsl) && nsl > 0));
}


/*ARGSUSED*/
static void create_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int          i, n, nvl, nsl, *sl;
	TSTAMP       tbuf;
	String       target_time, *vl;
	FLD_DESCRIPT fd;
	Boolean      created = False;
	String       notify[1] = {NULL};


	if(nflds < 1 || !XmListGetSelectedPos(fieldSelect, &sl, &nsl)) return;

	XuSetBusyCursor(ON);
	DeactivateMenu();
	init_fld_descript(&fd);

	target_time = TargetTimeGetStrTime(targetTime);

	for( i = 0; i < nsl; i++ )
	{
		n = sl[i] - 1;

		/* If the field is a daily type then we must substitute the hour as specified
		*  in the config file for whatever hour we have in the target time.
		*/
		if(flds[n]->element->elem_tdep->time_dep == FpaC_DAILY)
		{
			int   year, jday, hour;
			double dhr;
			(void)parse_tstamp(target_time, &year, &jday, NULL, NULL, NULL, NULL);
			(void)convert_value(flds[n]->element->elem_tdep->units->name,
								flds[n]->element->elem_tdep->normal_time, "hr", &dhr);
			hour = NINT(dhr);
			(void) tnorm(&year, &jday, &hour, NullInt, NullInt);
			strcpy(tbuf, build_tstamp(year,jday,hour,0,True,minutes_in_depictions()));
		}
		else
		{
			strcpy(tbuf, target_time);
		}

		/* Next check to see if the field already exists at the specified target time
		*/
		(void) set_fld_descript(&fd,
			FpaF_SOURCE_NAME, DEPICT,
			FpaF_ELEMENT,     flds[n]->element,
			FpaF_LEVEL,       flds[n]->level,
			FpaF_END_OF_LIST);
		nvl = FilteredValidTimeList(&fd, flds[n]->element->elem_tdep->time_dep, &vl);

		if(InTimeList(tbuf, vl, nvl, NULL))
		{
			Boolean add;

			XuSetBusyCursor(OFF);
			add = (XuAskUser(XuGetActionAreaBtn(dialog, action_items[0]), "coc", flds[n]->label) == XuYES);
			XuSetBusyCursor(ON);
			if(add)
			{
				(void) IngredVaCommand(GE_SEQUENCE, "DELETE_FIELD %s %s %s",
					flds[n]->element->name,
					flds[n]->level->name,
					tbuf);
				(void) IngredVaCommand(GE_SEQUENCE,"CREATE_FIELD %s %s %s",
					flds[n]->element->name,
					flds[n]->level->name,
					tbuf);
			}
		}
		else
		{
			if(flds[n]->element->elem_tdep->time_dep == FpaC_NORMAL)
				(void)CreateDepiction(tbuf);
			(void) IngredVaCommand(GE_SEQUENCE,"CREATE_FIELD %s %s %s",
				flds[n]->element->name,
				flds[n]->level->name,
				tbuf);
			created = True;
		}
		nvl = FilteredValidTimeListFree(&vl, nvl);
	}
	ActivateMenu();
	XuSetBusyCursor(OFF);
	FreeItem(sl);

	/* We only notify observers if a field was created, not deleted and then created */
	if (created) NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);
}


/*ARGSUSED*/
static void exit_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	if (!dialog) return;

	reset_locale();

	if(IsNull(GV_active_group) || IsNull(GV_active_field))
	{
		ResetActiveField();
		InitToActiveGroup();
		InitToActiveDepiction();
	}
	FreeItem(flds);
	XmStringFree(NA[0]);
	dialog = NullWidget;
}
