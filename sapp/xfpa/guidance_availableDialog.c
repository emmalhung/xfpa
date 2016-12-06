/*=========================================================================*/
/*
*      File: guidance_availableDialog.c
*
*   Purpose: Displays the availability status of all guidance fields.
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

#include <string.h>
#include "global.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/ComboBox.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include "guidance.h"
#include "observer.h"
#include "help.h"


/* These are the sources that will appear in the source list of the dialog.
 */
#define SOURCE_LIST  SRC_FPA|SRC_NWP|SRC_ALLIED

/* Maximum number of sources and maximum number of issue times 
 * to appear before scrolling the list.
 */
#define MAX_VISIBLE_SOURCES	15
#define MAX_VISIBLE_ISSUES	3


static void    exit_cb			 (Widget, XtPointer, XtPointer);
static void    source_cb		 (Widget, XtPointer, XtPointer);
static void    issue_cb			 (Widget, XtPointer, XtPointer);
static void    level_filter_cb	 (Widget, XtPointer, XtPointer);
static void    update_field_list (void);
static Boolean update_available  (Boolean);
static void    update_run_list	 (void);

static Widget dialog            = NULL;
static Widget runList           = NULL;
static Widget fieldDisplay      = NULL;
static Widget fieldDisplayLabel = NULL;
static int    nlabelList        = 0;
static Widget *labelList        = NULL;
static String filter_level      = NULL;
static Source source            = NULL;
static int    nrun_list         = 0;
static String *run_list         = NULL;
static String run_time          = NULL;
static String *level_name       = NULL;

void ACTIVATE_guidanceAvailabilityDialog(Widget reference_widget )
{
	int i, n, nlev, nsrc, pos;
	String *combo_list;
	SourceList src;
	Widget selectForm, filter, labelW, sourceList, runLabel, fieldSW;
	FpaConfigLevelStruct **level_list = NULL;

	static XuDialogActionsStruct action_items[] = {
		{ "closeBtn", XuDestroyDialogCB, NULL },
		{ "helpBtn",  HelpCB, HELP_GUIDANCE_AVAIL }
	};

	if (dialog)
	{
		XuShowDialog(dialog);
		return;
	}

	dialog = XuCreateToplevelFormDialog(reference_widget, "guidanceAvailability",
		XuNmwmDeleteOverride, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	selectForm = XmVaCreateForm(dialog, "selectForm",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* The level filter is used to reduce the number of items in the
	*  displayed field list to those at the given level. The first 
	*  item in the list will always be "None".
	*/
	labelW = XmVaCreateManagedLabel(selectForm, "levelFilterLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	filter = XmVaCreateManagedComboBox(selectForm, "levelFilter",
		XmNvalue, "",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(filter, XmNselectionCallback, level_filter_cb, NULL);

	filter_level = NULL;
	nlev = identify_levels_by_type(FpaC_LVL_ANY, &level_list);
	level_name = NewStringArray(nlev+1);
	level_name[0] = NULL;
	combo_list = NewStringArray(nlev+1);
	combo_list[0] = XuGetLabel("none");
	for(n = 1, i = 0; i < nlev; i++)
	{
		/* Skip magic level types */
		if(level_list[i]->lvl_type == FpaC_LVL_ANY) continue;
		if(level_list[i]->lvl_type == FpaC_LVL_NOTUSED) continue;
		if(blank(level_list[i]->label)) continue;
		combo_list[n] = level_list[i]->label;
		level_name[n] = level_list[i]->name;
		n++;
	}
	XuComboBoxAddItems(filter, combo_list, n, 0);
	XuComboBoxSelectPos(filter, 1, False);
	FreeItem(combo_list);
	FreeItem(level_list);

	/* Create the source selection widgets.
	*/
	labelW = XmVaCreateManagedLabel(selectForm, "sourceHeader",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, filter,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	SourceListByType(SOURCE_LIST, FpaC_TIMEDEP_ANY, &src, &nsrc);

	sourceList = XmVaCreateManagedScrolledList(selectForm, "sourceList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNvisibleItemCount, MIN(nsrc,MAX_VISIBLE_SOURCES),
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, labelW,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(sourceList, XmNbrowseSelectionCallback, source_cb, NULL);

	runLabel = XmVaCreateManagedLabel(selectForm, "issueHeader",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sourceList,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	runList = XmVaCreateManagedScrolledList(selectForm, "issueList",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNvisibleItemCount, MAX_VISIBLE_ISSUES,
		XmNlistMarginWidth, 9,
		XmNlistMarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, runLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtManageChild(runList);
	XtAddCallback(runList, XmNbrowseSelectionCallback, issue_cb, NULL);

	/* Create the field display.
	*/
	fieldDisplayLabel = XmVaCreateManagedLabel(dialog, "fieldDisplayLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, selectForm,
		NULL);

	fieldSW = XmVaCreateManagedScrolledWindow(dialog, "fieldSW",
		XmNscrollBarDisplayPolicy, XmCONSTANT,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldDisplayLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, selectForm,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	fieldDisplay = XmVaCreateManagedRowColumn(fieldSW, "fieldDisplay", NULL);

	/* The number of labels created here is arbitrary but 20 seems right.
	 */
	nlabelList = 20;
	labelList = MoreWidgetArray(labelList, nlabelList);
	for(i = 0; i < nlabelList; i++)
		labelList[i] = XmCreateLabel(fieldDisplay, "label", NULL, 0);

	/* Put the sources into the selection list and select the
	 * previously selected source if possible.
	 */
	for(pos = 1, i = 0; i < nsrc; i++)
	{
		XuListAddItem(sourceList, SrcLabel(src[i]));
		if(source == src[i]) pos = i+1;
	}

	XmListSetBottomPos(sourceList, pos);
	XmListSelectPos(sourceList, pos, True);

	XtManageChild(selectForm);
	XuShowDialog(dialog);
}


void InitGuidanceAvailabilitySystem(void)
{
	AddSourceObserver(update_available,"GuidanceAvailability");
}


/*======================== LOCAL FUNCTIONS ===============================*/


/*ARGSUSED*/
static Boolean update_available(Boolean unused)
{
	if(NotNull(dialog) && NotNull(source) && source->modified)
	{
		update_run_list();
	}
	return True;
}


static int stracmp(const void *a , const void *b )
{
	return strcmp(*((String *)b), *((String *)a));
}


static void update_field_list(void)
{
	int i, nfield;
	String *labels;
	FpaConfigFieldStruct **flist;
	FLD_DESCRIPT fd;

	XtUnmanageChildren(labelList, nlabelList);

	if(source == NULL || source->fd == NULL) return;

	/* Get the field list.
	*/
	init_fld_descript(&fd);
	copy_fld_descript(&fd, source->fd);
	if(!set_fld_descript(&fd, FpaF_RUN_TIME, run_time, FpaF_LEVEL_NAME, filter_level, FpaF_END_OF_LIST))
	{
		XuWidgetLabel(labelList[0], XuGetLabel("none"));
		XtManageChildren(labelList, 1);
	}
	else
	{
		nfield = source_field_list(&fd, FpaC_TIMEDEP_ANY, &flist);
		if(nfield > 0)
		{
			if(nfield > nlabelList)
			{
				labelList = MoreWidgetArray(labelList, nfield);
				for(i = nlabelList; i < nfield; i++)
					labelList[i] = XmCreateLabel(fieldDisplay, "label", NULL, 0);
				nlabelList = nfield;
			}
			labels = NewStringArray(nfield);
			for(i = 0; i < nfield; i++) labels[i] = flist[i]->label;
			qsort((String)labels, nfield, sizeof(String), stracmp);
			for(i = 0; i < nfield; i++) XuWidgetLabel(labelList[i], labels[i]);
			FreeItem(labels);

			XtManageChildren(labelList, nfield);
		}
		else
		{
			XuWidgetLabel(labelList[0], XuGetLabel("none"));
			XtManageChildren(labelList, 1);
		}
		(void)source_field_list_free(&flist, nfield);
	}
}


/*ARGSUSED*/
static void level_filter_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;
	if(rtn->item_position > 0)
	{
		filter_level = level_name[rtn->item_position-1];
		update_field_list();
	}
}


/*ARGSUSED*/
static void source_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int ndx;
	SourceList src;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	ndx = rtn->item_position - 1;
	SourceListByType(SOURCE_LIST, FpaC_TIMEDEP_ANY, &src, NULL);
	source = src[ndx];
	update_run_list();
}


static void update_run_list(void)
{
	int i;

	if(nrun_list > 0)
		nrun_list = source_run_time_list_free(&run_list, nrun_list);
	XmListDeleteAllItems(runList);
	run_time = NULL;

	nrun_list = source_run_time_list(source->fd, &run_list);
	if(nrun_list > 0)
	{
		for(i = 0; i < nrun_list; i++)
			XuListAddItem(runList, DateString(run_list[i],HOURS));
	}
	else
	{
		XuListAddItem(runList, XuGetLabel("na")); 
	}
	XmListSelectPos(runList, 1, True);
}


/*ARGSUSED*/
static void issue_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(nrun_list > 0)
		run_time = run_list[((XmListCallbackStruct *)call_data)->item_position - 1];
	update_field_list();
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	FreeItem(labelList);
	nlabelList = 0;
	nrun_list = source_run_time_list_free(&run_list, nrun_list);
	XuDestroyDialog(dialog);
	dialog = NULL;
}
