/*=========================================================================*/
/*
*      File: guidance_fieldAddDialog.c
*
*   Purpose: Dialog for adding fields to the active guidance list or 
*            selecting a field to be sampled.
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
/*=========================================================================*/

#include "global.h"
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include "help.h"
#include "guidance.h"
#include "source.h"

static Widget dialog = NullWidget;
static Widget elemGroupList, elemList, levelList, sourceList;
static Widget elemListWidthSep;
static Widget curBtn, prvBtn, absBtn, absRunList, selBtn;

static char                   run_btn_ndx         = GUID_CURRENT;
static int                    nelem_group_list    = 0;
static FpaConfigGroupStruct   **elem_group_list   = NULL;
static int                    *elem_group_xref    = NULL;
static XmString               elem_group_selected = NULL;
static int                    nelem_list          = 0;
static FpaConfigElementStruct **elem_list         = NULL;
static XmString               elem_selected       = NULL;
static int                    nlevel_list         = 0;
static FpaConfigLevelStruct   **level_list        = NULL;
static XmString               level_selected      = NULL;
static XmString               source_selected     = NULL;
static int                    run_list_len        = 0;
static String                 *run_list           = NULL;
static GuidanceFieldStruct    fdata;
static String                 selected_element    = NULL;


static void AbsIssueCB			(Widget, XtPointer, XtPointer);
static void AddCB				(Widget, XtPointer, XtPointer);
static void ElemGroupCB			(Widget, XtPointer, XtPointer);
static void ElemCB				(Widget, XtPointer, XtPointer);
static void LevelCB				(Widget, XtPointer, XtPointer);
static void LoadElemGroupList	(void);
static void LoadSourceList		(void);
static void SourceCB			(Widget, XtPointer, XtPointer);
static void IssueCB				(Widget, XtPointer, XtPointer);
static void ExitCB				(Widget, XtPointer, XtPointer);


void ACTIVATE_guidanceFieldSelectDialog(Widget refw )

{
	Widget labelW, fieldForm, sourceForm, runForm, frame, rc;

	static XuDialogActionsStruct action_items[] = {
        { "addToList", AddCB,             NULL                    },
        { "closeBtn",  XuDestroyDialogCB, NULL                    },
        { "helpBtn",   HelpCB,            HELP_GUIDANCE_FIELD_ADD }
    };

	if(dialog) return;

	dialog = XuCreateFormDialog(refw, "guidanceFieldSelect",
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XuNdestroyCallback, ExitCB,
		XmNnoResize, True,
		XmNverticalSpacing, 19,
		XmNhorizontalSpacing, 19,
		NULL);

	frame = XmVaCreateManagedFrame(dialog, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginWidth, 9,
		XmNmarginHeight, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "fieldSelect",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	fieldForm = XmVaCreateForm(frame, "fieldForm",
		NULL);

	labelW = XmVaCreateManagedLabel(fieldForm, "elementGroup",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	elemGroupList = XmVaCreateManagedScrolledList(fieldForm, "elemGroupList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(elemGroupList, XmNbrowseSelectionCallback, ElemGroupCB, NULL);

	labelW = XmVaCreateManagedLabel(fieldForm, "element",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, elemGroupList,
		XmNleftOffset, 19,
		NULL);

	elemListWidthSep = XmVaCreateManagedSeparator(fieldForm, "widthSep",
		XmNseparatorType, XmNO_LINE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, elemGroupList,
		XmNleftOffset, 19,
		NULL);

	elemList = XmVaCreateManagedScrolledList(fieldForm, "elemList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, elemGroupList,
		XmNleftOffset, 19,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, elemListWidthSep,
		NULL);
	XtAddCallback(elemList, XmNbrowseSelectionCallback, ElemCB, NULL);

	labelW = XmVaCreateManagedLabel(fieldForm, "level",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, elemList,
		XmNleftOffset, 19,
		NULL);

	levelList = XmVaCreateManagedScrolledList(fieldForm, "levelList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, elemList,
		XmNleftOffset, 19,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(levelList, XmNbrowseSelectionCallback, LevelCB, NULL);

	frame = XmVaCreateManagedFrame(dialog, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginWidth, 9,
		XmNmarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "sourceSelect",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	sourceForm = XmVaCreateForm(frame, "sourceForm",
		NULL);

	labelW = XmVaCreateManagedLabel(sourceForm, "sourceHeader",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	sourceList = XmVaCreateManagedScrolledList(sourceForm, "sourceList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(sourceList, XmNbrowseSelectionCallback, SourceCB, NULL);

	frame = XmVaCreateManagedFrame(sourceForm, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginWidth, 9,
		XmNmarginHeight, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, sourceList,
		XmNleftOffset, 19,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "issueHeader",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	runForm = XmVaCreateForm(frame, "runForm",
		NULL);

	rc = XmVaCreateRowColumn(runForm, "rc",
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	curBtn = XmVaCreateManagedToggleButton(rc, "curBtn", NULL);
	XtAddCallback(curBtn, XmNvalueChangedCallback, IssueCB, (XtPointer)GUID_CURRENT);

	prvBtn = XmVaCreateManagedToggleButton(rc, "prevBtn", NULL);
	XtAddCallback(prvBtn, XmNvalueChangedCallback, IssueCB, (XtPointer)GUID_PREVIOUS);

	absBtn = XmVaCreateManagedToggleButton(rc, "absBtn", NULL);
	XtAddCallback(absBtn, XmNvalueChangedCallback, IssueCB, (XtPointer)GUID_ABSOLUTE);

	switch(run_btn_ndx)
	{
		case GUID_PREVIOUS:  selBtn = prvBtn; break;
		case GUID_ABSOLUTE:  selBtn = absBtn; break;
		default:             selBtn = curBtn; break;
	}

	absRunList = XmVaCreateManagedScrolledList(runForm, "sourceList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNvisibleItemCount, 3,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, rc,
		XmNleftOffset, 6,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(absRunList, XmNbrowseSelectionCallback, AbsIssueCB, NULL);

	XtManageChild(rc);
	XtManageChild(runForm);
	XtManageChild(fieldForm);
	XtManageChild(sourceForm);

	memset((void*)&fdata, 0, sizeof(GuidanceFieldStruct));
	LoadElemGroupList();
	LoadSourceList();

	XuShowDialog(dialog);
}


static void LoadElemGroupList(void)

{
	int i, pos, count;
	XmString label;

	nelem_group_list = identify_groups_for_elements_free(&elem_group_list, nelem_group_list);
	FreeItem(elem_group_xref);
	nelem_group_list = identify_groups_for_elements(&elem_group_list);
	elem_group_xref  = NewMem(int, nelem_group_list);
	for(count = 0, i = 0; i < nelem_group_list; i++)
	{
		if( identify_elements_by_group(elem_group_list[i]->name, NULL) > 0)
		{
			elem_group_xref[count] = i;
			label = XuNewXmString(elem_group_list[i]->label);
			XmListAddItem(elemGroupList, label, 0);
			XmStringFree(label);
			count++;
		}
		else
		{
			Warning("LoadElementGroup", "EmptyElementGroup", elem_group_list[i]->label);
		}
	}
	pos = MAX(1, XmListItemPos(elemGroupList, elem_group_selected));
	XmListSetBottomPos(elemGroupList, pos);
	XmListSelectPos(elemGroupList, pos, True);
}


/*ARGSUSED*/
static void ElemGroupCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, pos;
	XmString label;
	XtWidgetGeometry size;
	FpaConfigGroupStruct *tptr;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	static Dimension width = 50;

	XmStringFree(elem_group_selected);
	elem_group_selected = XmStringCopy(rtn->item);
	tptr = elem_group_list[elem_group_xref[rtn->item_position - 1]];

	(void)identify_elements_by_group_free(&elem_list, nelem_list);
	XmListDeleteAllItems(elemList);
	nelem_list = identify_elements_by_group(tptr->name, &elem_list);
	for(i = 0; i < nelem_list; i++)
	{
		label = XuNewXmString(elem_list[i]->label);
		XmListAddItem(elemList, label, 0);
		XmStringFree(label);
	}
	pos = MAX(1, XmListItemPos(elemList, elem_selected));
	XmListSetBottomPos(elemList, pos);
	XmListSelectPos(elemList, pos, True);

	/* This procedure is to stop the run time list from changing size every
	*  time a new list of elements is displayed.
	*/
	size.request_mode = CWWidth;
	(void) XtQueryGeometry(XtParent(elemList), NULL, &size);
	width = MAX(width, size.width);
	XtVaSetValues(elemListWidthSep, XmNwidth, width, NULL);
}


/*ARGSUSED*/
static void ElemCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, pos;
	XmString label;
	FpaConfigElementStruct *eptr;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	XmStringFree(elem_selected);
	elem_selected = XmStringCopy(rtn->item);
	eptr = elem_list[rtn->item_position - 1];
	selected_element = eptr->name;

	nlevel_list = identify_levels_by_type_free(&level_list, nlevel_list);
	XmListDeleteAllItems(levelList);
	nlevel_list = identify_levels_by_type(eptr->lvl_type, &level_list);
	for(i = 0; i < nlevel_list; i++)
	{
		label = XuNewXmString(level_list[i]->label);
		XmListAddItem(levelList, label, 0);
		XmStringFree(label);
	}
	pos = MAX(1, XmListItemPos(levelList, level_selected));
	XmListSetBottomPos(levelList, pos);
	XmListSelectPos(levelList, pos, True);
}


/*ARGSUSED*/
static void LevelCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	FpaConfigLevelStruct *lptr;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	XmStringFree(level_selected);
	level_selected = XmStringCopy(rtn->item);
	lptr = level_list[rtn->item_position - 1];
	fdata.info = identify_field(selected_element, lptr->name);
}


static void LoadSourceList(void)

{
	int i, nsrc, pos;
	XmString label;
	SourceList src;

	XmListDeleteAllItems(sourceList);
	SourceListByType(SRC_ALL, FpaC_TIMEDEP_ANY, &src, &nsrc);
	for(i = 0; i < nsrc; i++)
	{
		label = XuNewXmString(SrcLabel(src[i]));
		XmListAddItem(sourceList, label, 0);
		XmStringFree(label);
	}
	pos = MAX(1, XmListItemPos(sourceList, source_selected));
	XmListSetBottomPos(sourceList, pos);
	XmListSelectPos(sourceList, pos, True);
}


/*ARGSUSED*/
static void SourceCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int        i;
	SourceList src;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	XmStringFree(source_selected);
	source_selected = XmStringCopy(rtn->item);

	SourceListByType(SRC_ALL, FpaC_TIMEDEP_ANY, &src, &i);
	fdata.source = src[rtn->item_position - 1];

	run_list_len = source_run_time_list_free(&run_list, run_list_len);
	XuListEmpty(absRunList);

	XuToggleButtonSet(curBtn, False, False);
	XuToggleButtonSet(prvBtn, False, False);
	XuToggleButtonSet(absBtn, False, False);

	if((fdata.source->type & (SRC_FPA|SRC_DEPICT|SRC_INTERP)) != 0)
	{

		fdata.rtype = GUID_DEPICT;
		fdata.vtype = GUID_CURRENT;
		fdata.vsel  = GUID_NO_SEL;

		XuListAddItem(absRunList, "              ");
		RemoveRunTimeEntry(fdata.run);
		fdata.run = CreateRunTimeEntry(NULL);

		XtSetSensitive(curBtn,     False);
		XtSetSensitive(prvBtn,     False);
		XtSetSensitive(absBtn,     False);
		XtSetSensitive(absRunList, False);
	}
	else
	{

		fdata.rtype = GUID_CURRENT;
		fdata.vtype = GUID_ABSOLUTE;
		fdata.vsel  = GUID_NO_SEL;

		XtSetSensitive(curBtn, True);
		XtSetSensitive(prvBtn, True);

		run_list_len = source_run_time_list(fdata.source->fd, &run_list);
		if(run_list_len > 0)
		{
			XtSetSensitive(absBtn,     True);
			XtSetSensitive(absRunList, True);
			for(i = 0; i < run_list_len; i++)
			{
				XuListAddItem(absRunList, DateString(run_list[i],HOURS));
			}
			XmListSelectPos(absRunList, 1, True);
		}
		else
		{
			RemoveRunTimeEntry(fdata.run);
			fdata.run = CreateRunTimeEntry(NULL);
			selBtn = curBtn;
			XuListAddItem(absRunList, XuGetLabel("na"));
			XtSetSensitive(absBtn,     False);
			XtSetSensitive(absRunList, False);
		}
		XuToggleButtonSet(selBtn, True, True);
	}
}


/*ARGSUSED*/
static void IssueCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;

	selBtn      = w;
	fdata.rtype = PTR2CHAR(client_data);
	run_btn_ndx = fdata.rtype;

	switch(fdata.rtype)
	{
		case GUID_CURRENT:
			XtSetSensitive(absRunList, False);
			RemoveRunTimeEntry(fdata.run);
			fdata.run = CreateRunTimeEntry((run_list_len > 0)? run_list[0]:NULL);
			break;

		case GUID_PREVIOUS:
			XtSetSensitive(absRunList, False);
			RemoveRunTimeEntry(fdata.run);
			fdata.run = CreateRunTimeEntry((run_list_len > 1)? run_list[1]:NULL);
			break;

		case GUID_ABSOLUTE:
			XtSetSensitive(absRunList, True);
			XmListSelectPos(absRunList, 1, True);
			break;

		default:
			XtSetSensitive(absRunList, False);
			break;
	}
}


/*ARGSUSED*/
static void AbsIssueCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	RemoveRunTimeEntry(fdata.run);
	if(run_list_len > 0)
		fdata.run = CreateRunTimeEntry(run_list[rtn->item_position-1]);
	else
		fdata.run = CreateRunTimeEntry(NULL);
}


/* Add the field to the given list. If the field does not exist then
*  give the user the option to add it or not. Note that the data
*  structure passed in to GuidanceListAddField must be copied in
*  that function.
*/
/*ARGSUSED*/
static void AddCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int          i, nvtime, rtn;
	String       *vtimes;
	Boolean      ok;
	FLD_DESCRIPT fd;

	XuSetBusyCursor(ON);

	nvtime          = 0;
	vtimes          = NULL;
	fdata.available = False;

	/* Check to see if detailed field information is available. If not there is
	*  an error in the config files and we must inform the user.
	*/
	if(IsNull(get_field_info(fdata.info->element->name,fdata.info->level->name)))
	{
		XuSetBusyCursor(OFF);
		XuShowMessage(w, "FieldConfigError", NULL);
		return;
	}

	init_fld_descript(&fd);
	copy_fld_descript(&fd, fdata.source->fd);

	(void) set_fld_descript(&fd,
		FpaF_ELEMENT, fdata.info->element,
		FpaF_LEVEL,   fdata.info->level,
		FpaF_END_OF_LIST);

	if(NotNull(RunTime(fdata.run)))
	{
		(void) set_fld_descript(&fd,
			FpaF_RUN_TIME, RunTime(fdata.run),
			FpaF_END_OF_LIST);
	}

	nvtime = FilteredValidTimeList(&fd, fdata.info->element->elem_tdep->time_dep, &vtimes);

	for(i = 0; i < nvtime; i++)
	{
		(void) set_fld_descript(&fd, FpaF_VALID_TIME, vtimes[i], FpaF_END_OF_LIST);
		fdata.available = check_retrieve_metasfc(&fd);
		if(fdata.available) break;
	}

	if(fdata.available)
	{
		ok = GuidanceListAddField(&fdata, 0);
		XuSetBusyCursor(OFF);
		if (!ok) XuShowMessage(w, "FieldExists", NULL);
	}
	else
	{
		XuSetBusyCursor(OFF);
		rtn = XuAskUser(w, "NoFieldToAdd", NULL);
		if(rtn == XuYES)
		{
			XuSetBusyCursor(ON);
			ok = GuidanceListAddField(&fdata, 0);
			XuSetBusyCursor(OFF);
			if (!ok) XuShowMessage(w, "FieldExists", NULL);
		}
	}
	(void)FilteredValidTimeListFree(&vtimes, nvtime);
}


/*ARGSUSED*/
static void ExitCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	FreeItem(elem_group_xref);
	nelem_group_list = identify_groups_for_elements_free(&elem_group_list, nelem_group_list);
	nelem_list      = identify_elements_by_group_free(&elem_list, nelem_list);
	nlevel_list     = identify_levels_by_type_free(&level_list, nlevel_list);
	run_list_len    = source_run_time_list_free(&run_list, run_list_len);

	RemoveRunTimeEntry(fdata.run);
	dialog = NULL;
}
