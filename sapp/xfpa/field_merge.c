/*****************************************************************************
*
*  File:     field_merge.c
*
*  Purpose:  Common editor interface for the merge edit function. The merge
*            field is selected (optional and defaults to the current active
*            field), the source, run time and valid time. All are interlinked
*            to depend in a cascading way on the previous setting.
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
*
*****************************************************************************/

#include <string.h>
#include "global.h"
#include <Xm/ComboBox.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include "depiction.h"
#include "editor.h"
#include "observer.h"
#include "contextMenu.h"
#include <ingred.h>

#define DRAW_OUTLINE	"*D*"
#define Module			"Field Merge"

/* Local functions */
static void action_edit_commands    (String*, int);
static void action_depiction_change (String*, int);
static void action_depiction_saved  (String*, int);
static void action_ingred_messages  (CAL, String*, int);
static void field_cb                (Widget, XtPointer, XtPointer);
static void source_cb               (Widget, XtPointer, XtPointer);
static void issue_time_cb           (Widget, XtPointer, XtPointer);
static void create_valid_time_list  (void);
static void valid_time_cb           (Widget, XtPointer, XtPointer);
static void fetch_cb                (Widget, XtPointer, XtPointer);
static void merge_labels_cb         (Widget, XtPointer, XtPointer);
static void select_all_cb           (Widget, XtPointer, XtPointer);
static void set_fetch_button_sensitive(Boolean);
static void update_sources          (void);

/* Local widgets */
static Widget mergePanel = NullWidget;
static Widget fieldLabel;
static Widget fieldSelect;
static Widget sourceLabel;
static Widget mergeSource;
static Widget issueSelect;
static Widget validSelect;
static Widget mergeLabels;
static Widget fetchBtn;
static Widget selectAllBtn;
static Widget issueTitle;
static Widget validTitle;

/* Local variables */
static FLD_DESCRIPT mfd;					/* Field descriptor for selected field */
static int nsources = 0;					/* number of sources */
static SourceList sources = NULL;			/* sources available */
static Source source = NULL;				/* Selected source */
static int nrun_times = 0;
static String *run_times = NULL;
static int nvalid_times = 0;
static String *valid_times = NULL;
static Boolean no_field = False;			/* Does the current field not exist? */
static Boolean showing = False;				/* Is the merge sub-panel showing? */
static Boolean allow_fetch = True;			/* Are we allowed to fetch a field? */
static Boolean have_valid_time = False;		/* Is a valid time selected? */
static unsigned char merge_labels = XmUNSET;/* Merge labels or not? */
static int nfield = 0;						/* Number of fields available for merging */
static int selected_field_ndx = 0;			/* Index number of the selected field */
static FpaConfigFieldStruct **fields = NULL;/* Pointers to valid field structs */


void CreateMergeFieldPanel(Widget parent, Widget topAttach)
{
	Widget frame, form;

	mergePanel = XmVaCreateForm(parent, "mergePanel",
		XmNverticalSpacing, 10,
		XmNhorizontalSpacing, 3,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, topAttach,
		NULL);

	frame = XmVaCreateManagedFrame(mergePanel, "frame",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "mergeTitle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(frame, "form",
		XmNverticalSpacing, 10,
		XmNhorizontalSpacing, 3,
		NULL);

	fieldLabel = XmVaCreateManagedLabel(form, "fieldHeader",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	fieldSelect = XmVaCreateManagedComboBox(form, "fieldSelect",
		XmNvalue, "",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, fieldLabel, XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(fieldSelect, XmNselectionCallback, field_cb, NULL);

	sourceLabel = XmVaCreateManagedLabel(form, "sourceHeader",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldSelect,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	mergeSource = XmVaCreateManagedComboBox(form, "mergeSource",
		XmNvalue, "",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, sourceLabel, XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(mergeSource, XmNselectionCallback, source_cb, NULL);

	issueTitle = XmVaCreateManagedLabel(form, "issueHeader",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, mergeSource,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	issueSelect = XmVaCreateManagedComboBox(form, "issueSelect",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, issueTitle, XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(issueSelect, XmNselectionCallback, issue_time_cb, NULL);

	validTitle = XmVaCreateManagedLabel(form, "validHeader",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, issueSelect,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	validSelect = XmVaCreateManagedComboBox(form, "validSelect",
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, validTitle, XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(validSelect, XmNselectionCallback, valid_time_cb, NULL);

	mergeLabels = XmVaCreateManagedToggleButton(form, "mergeLabels",
		XmNborderWidth, 1,
		XmNset, merge_labels,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, validSelect, XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(mergeLabels, XmNvalueChangedCallback, merge_labels_cb, NULL);

	fetchBtn = XmVaCreateManagedPushButton(form, "fetchBtn",
		XmNsensitive, False,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, mergeLabels, XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(fetchBtn, XmNactivateCallback, fetch_cb, NULL);

	selectAllBtn = XmVaCreatePushButton(form, "selectAll",
		XmNsensitive, False,
		XmNmarginHeight, 5,
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, mergeLabels, XmNtopOffset, 20,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(selectAllBtn, XmNactivateCallback, select_all_cb, NULL);

	/* Request to be notified about the following events.
	 */
	AddIngredObserver(action_ingred_messages);
	AddObserver(OB_MENU_ACTIVATION,  action_edit_commands);
	AddObserver(OB_DEPICTION_CHANGE, action_depiction_change);
	AddObserver(OB_DEPICTION_SAVED,  action_depiction_saved);
}


/*
 * Configures the merge panel for the currently active field. If the field
 * selection ability is not in the configuration then the field selection
 * widget is not managed and will not appear to the user.
 */
void ConfigureMergeFieldPanel(void)
{
	int n = 0;
	int nfld = 0;
	int lk_nfld = 0;
	int count = 0;
	FpaConfigElementStruct **elems, **lk_elm;
	FpaConfigLevelStruct   **levels, **lk_lev;
	FpaConfigElementEditorStruct *editor  = GV_active_field->info->element->elem_detail->editor;

	/* Just in case we ran into the switch default error */
	if(!XtIsSensitive(mergePanel))
		XtSetSensitive(mergePanel, True);

	/* Continuous and vector fields have no use for the select all button so unmanage it.
	 */
	switch(GV_active_field->info->element->fld_type)
	{
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
			XtVaSetValues(fetchBtn, XmNrightAttachment, XmATTACH_FORM, NULL);
			XtUnmanageChild(selectAllBtn);
			break;

		default:
			XtManageChild(selectAllBtn);
			XtVaSetValues(fetchBtn, XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, selectAllBtn, NULL);
			break;
	}

	/* Link chain fields have no use for the include labels toggle so unmanage it */
	if(GV_active_field->info->element->fld_type == FpaC_LCHAIN)
	{
		XtVaSetValues(fetchBtn, XmNtopWidget, validSelect, NULL);
		XtVaSetValues(selectAllBtn, XmNtopWidget, validSelect, NULL);
		XtUnmanageChild(mergeLabels);
	}
	else
	{
		XtManageChild(mergeLabels);
		XtVaSetValues(fetchBtn, XmNtopWidget, mergeLabels, NULL);
		XtVaSetValues(selectAllBtn, XmNtopWidget, mergeLabels, NULL);
	}

	/* We can only check for alternate merge fields if we have a valid editor. Note that
	 * lchain type fields have the possibility of merging link chains from timelinking
	 * and so have an additional set of fields to choose from.
	 */
	if (editor)
	{
		switch(GV_active_field->info->element->fld_type)
		{
			case FpaC_CONTINUOUS:
				nfld = editor->type.continuous->nmerge;
				elems  = editor->type.continuous->merge_elems;
				levels = editor->type.continuous->merge_levels;
				break;

			case FpaC_VECTOR:    
				nfld = editor->type.vector->nmerge;
				elems  = editor->type.vector->merge_elems;
				levels = editor->type.vector->merge_levels;
				break;

			case FpaC_DISCRETE:
				nfld = editor->type.discrete->nmerge;
				elems  = editor->type.discrete->merge_elems;
				levels = editor->type.discrete->merge_levels;
				break;

			case FpaC_WIND:
				nfld = editor->type.wind->nmerge;
				elems  = editor->type.wind->merge_elems;
				levels = editor->type.wind->merge_levels;
				break;

			case FpaC_LINE:
				nfld = editor->type.line->nmerge;
				elems  = editor->type.line->merge_elems;
				levels = editor->type.line->merge_levels;
				break;

			case FpaC_SCATTERED:
				nfld = editor->type.scattered->nmerge;
				elems  = editor->type.scattered->merge_elems;
				levels = editor->type.scattered->merge_levels;
				break;

			case FpaC_LCHAIN:
				nfld    = editor->type.lchain->nmerge;
				elems   = editor->type.lchain->merge_elems;
				levels  = editor->type.lchain->merge_levels;
				lk_nfld = editor->type.lchain->nlink;
				lk_elm  = editor->type.lchain->link_elems;
				lk_lev  = editor->type.lchain->link_levels;
				break;

			default:
				pr_error(Module,"Unrecognized field type: Merge disallowed\n");
				XtSetSensitive(mergePanel, False);
				return;
		}
	}

	XuComboBoxDeleteAllItems(fieldSelect);

	/* The active field is always the first field in the merge field list and the
	 * list is always at least one element long.
	 */
	nfield = 1;
	fields = MoreMem(fields, FpaConfigFieldStruct*, nfield+nfld+lk_nfld);
	fields[count++] = GV_active_field->info;
	XuComboBoxAddItem(fieldSelect, GV_active_field->info->sh_label, 0);
	selected_field_ndx = 0;

	/* Add in any fields from the additional merge list.
	 */
	for(n = 0; n < nfld; n++)
	{
		FLD_DESCRIPT fd;
		FpaConfigLevelStruct *lev;

		init_fld_descript(&fd);
		if(!elems[n]) continue;
		lev = (levels[n])? levels[n] : GV_active_field->info->level;
		if(set_fld_descript(&fd, FpaF_ELEMENT, elems[n], FpaF_LEVEL, lev, FpaF_END_OF_LIST))
		{
			/* The active field is already in the list */
			if(fd.fdef != GV_active_field->info)
			{
				fields[count++] = fd.fdef;
				nfield++;
				XuComboBoxAddItem(fieldSelect, fd.fdef->sh_label, 0);
			}
		}
		else
		{
			pr_error(Module,"Unable to create a field from element \"%s\" and level \"%s\"\n",
				elems[n]->name, lev->name);
		}
	}

	/* If there are any timelink link chains that can be merged in add them to the end of the list.
	 * Note that these are prefaced by "Link:".
	 */
	for(n = 0; n < lk_nfld; n++)
	{
		char buf[256];
		FLD_DESCRIPT fd;
		FpaConfigLevelStruct *lev;

		init_fld_descript(&fd);
		if(!lk_elm[n]) continue;
		
		lev = (lk_lev[n])? lk_lev[n] : GV_active_field->info->level;
		if(set_fld_descript(&fd, FpaF_ELEMENT, lk_elm[n], FpaF_LEVEL, lev, FpaF_END_OF_LIST))
		{
			fields[count++] = fd.fdef;
			(void) snprintf(buf, sizeof(buf), "%s: %s", XuGetLabel("link"), fd.fdef->sh_label);
			XuComboBoxAddItem(fieldSelect, buf, 0);
		}
		else
		{
			pr_error(Module,"Unable to create a field from element \"%s\" and level \"%s\"\n",
				lk_elm[n]->name, lev->name);
		}
	}

	/* If there is only the active field available for selection the field selector is done away
	 * with as it is not revelant to the selection process.
	 */
	if(count > 1)
	{
		XtManageChild(fieldLabel);
		XtManageChild(fieldSelect);
		XtVaSetValues(sourceLabel, XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, fieldSelect, NULL);
		XtVaSetValues(fieldSelect, XmNvisibleItemCount, MIN(count,10), NULL);
		XuComboBoxSelectPos(fieldSelect, 1, True);
	}
	else
	{
		XtVaSetValues(sourceLabel, XmNtopAttachment, XmATTACH_FORM, NULL);
		XtUnmanageChild(fieldSelect);
		XtUnmanageChild(fieldLabel);
		update_sources();
	}


	XmToggleButtonSetState(mergeLabels, merge_labels, False);
	set_fetch_button_sensitive(False);
	(void) IngredVaCommand(GE_ACTION, "STATE MERGE_MODE %s", (merge_labels == XmSET)? "FIELD_AND_LABELS":"FIELD");
}


void ShowMergeFieldPanel()
{
	if(!mergePanel) return;
	showing = True;
	XtManageChild(mergePanel);
	if(no_field)
		set_fetch_button_sensitive(False);
	else
		create_valid_time_list();
}


void HideMergeFieldPanel()
{
	if(mergePanel)
	{
		XtUnmanageChild(mergePanel);
		SetContextPasteSelectAllButtonsState(SELECT_ALL_BUTTON_OFF);
		ReleaseMainContextMenuAuxPushButton();
	}
	showing = False;
}


/*======================== LOCAL FUNCTIONS ==============================*/


/* When setting the visibleItemCount parameter of the various combo boxes
 * the boxes would not lay out properly on the form afterwards unless the
 * parent form was nugged a bit. This function does this in a way that I
 * found not visibly disturbing.
 */
static void giggle(void)
{
	Dimension spacing;
	XtVaGetValues(mergePanel, XmNhorizontalSpacing, &spacing,  NULL);
	XtVaSetValues(mergePanel, XmNhorizontalSpacing, spacing+1, NULL);
	XtVaSetValues(mergePanel, XmNhorizontalSpacing, spacing,   NULL);
}


/* Callback for ConfigureMainContextMenuAuxPushButton */
/*ARGSUSED*/
static void aux_push_button_cb(XtPointer unused)
{
	fetch_cb(NULL,NULL,NULL);
}


/* Besides setting the sensitivity of the panel fetch button this function
 * also configures the main context menu general push button.
 */
static void set_fetch_button_sensitive(Boolean state)
{
	if(state)
	{
		XtSetSensitive(fetchBtn, True);
		ConfigureMainContextMenuAuxPushButton("fetchBtn", aux_push_button_cb, NULL);
	}
	else
	{
		XtSetSensitive(fetchBtn, False);
		ReleaseMainContextMenuAuxPushButton();
	}
}


/*
*  Called when a field to merge in is selected. This assumes that the first
*  field in the list will be the currently active one.
*/
/*ARGSUSED*/
static void field_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;
	if(rtn->item_position > 0)
	{
		selected_field_ndx = rtn->item_position - 1;
		update_sources();
	}
}


/* The variable selected_field_ndx is what selects the active field and is a global
 * so that no arguments are required for this function. The fields variable
 * is guaranteed to have at least one element that points to the active field
 * so that selected_field_ndx = 0 is always valid.
 */
static void update_sources(void)
{
	int  val = (int) XmUNSET;
	char srcid[128], subsrc[128];
	Source *src;
	long key = SRC_DEPICT|SRC_FPA|SRC_INTERP|SRC_BACKUP|SRC_NWP|SRC_ALLIED;

	/* Get the last source selected for this field type */
	if(XuVaStateDataGet("fe", FieldTypeID(NULL), "ms", "%s %s %d", srcid, subsrc, &val))
	{
		source = FindSourceByName(srcid, subsrc);
		merge_labels = (unsigned char) val;
	}

	/* Get the list of possible sources. 
	 */
	SourceListByField(key, fields[selected_field_ndx], &src, &nsources);

	XuComboBoxDeleteAllItems(mergeSource);
	XuComboBoxDeleteAllItems(issueSelect);
	XuComboBoxDeleteAllItems(validSelect);

	FreeItem(sources);

	if(nsources > 0)
	{
		int i, selected = 0;
		/*
		 * Note that we need to copy the source list array instead of just
		 * using the returned source array because if the SourceListByField
		 * is called somewhere else the returned list could change underfoot.
		 */
		sources = NewMem(Source, nsources);
		for(i = 0; i < nsources; i++)
		{
			sources[i] = src[i];
			if(source == sources[i]) selected = i;
			/* The long label form is better for these sources */
			if(src[i]->type & (SRC_DEPICT|SRC_INTERP|SRC_BACKUP))
				XuComboBoxAddItem(mergeSource, SrcLabel(sources[i]), 0);
			else
				XuComboBoxAddItem(mergeSource, SrcShortLabel(sources[i]), 0);
		}
		XtVaSetValues(mergeSource, XmNvisibleItemCount, MIN(nsources,10), NULL);
		XuComboBoxSelectPos(mergeSource, selected+1, True);
	}
	else
	{
		have_valid_time = False;
		XuComboBoxSetString(mergeSource, XuGetLabel("na"));
		set_fetch_button_sensitive(False);
		XtVaSetValues(mergeSource, XmNvisibleItemCount, 1, NULL);
		giggle();
	}
}


/* The source list will contain a not applicable message if there are no
 * sources available.
 */
/*ARGSUSED*/
static void source_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int ndx;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(rtn->item_position < 1) return;
	ndx = rtn->item_position - 1;

	source = sources[ndx];
	XuVaStateDataSave("fe",FieldTypeID(NULL),"ms", "%s %s %d",
			SrcName(source), SrcSubDashName(source), (int) merge_labels);

	init_fld_descript(&mfd);
	copy_fld_descript(&mfd, source->fd);
	(void) set_fld_descript(&mfd,
		FpaF_ELEMENT, fields[selected_field_ndx]->element,
		FpaF_LEVEL, fields[selected_field_ndx]->level,
		FpaF_END_OF_LIST);

	nrun_times = source_run_time_list_free(&run_times, nrun_times);

	/* A run time may only exist for a regular field merge and not a
	 * link chain from timelinking.
	 */
	if(selected_field_ndx < nfield)
		nrun_times = source_run_time_list(&mfd, &run_times);

	XuComboBoxDeleteAllItems(issueSelect);

	if(nrun_times > 0)
	{
		int i;
		XtSetSensitive(issueTitle, True);
		XtSetSensitive(issueSelect, True);
		for(i = 0; i < nrun_times; i++)
			XuComboBoxAddItem(issueSelect, DateString(run_times[i], HOURS), 0);
		XtVaSetValues(issueSelect, XmNvisibleItemCount, MIN(nrun_times,10), NULL);
		XuComboBoxSelectPos(issueSelect, 1, True);
	}
	else
	{
		XtVaSetValues(issueSelect, XmNvisibleItemCount, 1, NULL);
		XuComboBoxAddItem(issueSelect, XuGetLabel("napp"), 0);
		XuComboBoxSelectPos(issueSelect, 1, False);
		XtSetSensitive(issueTitle, False);
		XtSetSensitive(issueSelect, False);
		create_valid_time_list();
		giggle();
	}
	XtSetSensitive(issueSelect, (nrun_times > 0));
}


/*
*  Called when a merge issue time is selected. If the list contains
*  the "Not Applicable" phrase then there are no run times.
*/
/*ARGSUSED*/
static void issue_time_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(rtn->item_position < 1) return;

	if(XuComboBoxItemPos(issueSelect, XuGetLabel("napp")) < 1)
	{
		int ndx = rtn->item_position - 1;
		(void) set_fld_descript(&mfd, FpaF_RUN_TIME, run_times[ndx], FpaF_END_OF_LIST);
	}
	create_valid_time_list();
}


static void create_valid_time_list(void)
{
	int i, pos;

	XuComboBoxDeleteAllItems(validSelect);

	nvalid_times = FilteredValidTimeListFree(&valid_times, nvalid_times);

	/* A valid time may only exist for a regular field merge and not a
	 * link chain from timelinking.
	 */
	if(selected_field_ndx < nfield)
		nvalid_times = FilteredValidTimeList(&mfd, mfd.fdef->element->elem_tdep->time_dep, &valid_times);

	if(nvalid_times > 0)
	{
		XtSetSensitive(validSelect, True);
		XtSetSensitive(validTitle, True);

		for(i = 0; i < nvalid_times; i++)
			XuComboBoxAddItem(validSelect, DepictFieldDateFormat(GV_active_field, valid_times[i]), 0);

		if((have_valid_time = InTimeList(ActiveDepictionTime(FIELD_DEPENDENT), valid_times, nvalid_times, &pos)))
		{
			XuComboBoxSelectPos(validSelect, pos+1, True);
			set_fetch_button_sensitive(allow_fetch);
		}
		else
		{
			XuComboBoxSetString(validSelect, XuGetLabel("no_time_match"));
			(void) set_fld_descript(&mfd, FpaF_VALID_TIME, NULL, FpaF_END_OF_LIST);
			set_fetch_button_sensitive(False);
		}
		XtVaSetValues(validSelect, XmNvisibleItemCount, MIN(nvalid_times,10), NULL);
	}
	else
	{
		/* If the selected field is from timelinking then the fetch is legal */
		set_fetch_button_sensitive(selected_field_ndx >= nfield);
		XtVaSetValues(validSelect, XmNvisibleItemCount, 1, NULL);
		XuComboBoxSetString(validSelect, XuGetLabel("na"));
		XuComboBoxSelectPos(validSelect, 1, False);
		XtSetSensitive(validSelect, False);
		XtSetSensitive(validTitle, False);
	}
	giggle();
}


/*
*  Called when a merge valid time is selected.
*/
/*ARGSUSED*/
static void valid_time_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int ndx;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(rtn->item_position < 1) return;
	ndx = rtn->item_position - 1;
	(void) set_fld_descript(&mfd, FpaF_VALID_TIME, valid_times[ndx], FpaF_END_OF_LIST);
	set_fetch_button_sensitive(allow_fetch);
	have_valid_time = True;
}


/*ARGSUSED*/
static void merge_labels_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) call_data;

	merge_labels = cbs->set;
	XuVaStateDataSave("fe",FieldTypeID(NULL),"ms", "%s %s %d",
			SrcName(source), SrcSubDashName(source), (int) merge_labels);
	(void) IngredVaCommand(GE_ACTION, "STATE MERGE_MODE %s", (merge_labels == XmSET)? "FIELD_AND_LABELS":"FIELD");
}


/*
*	Gets the data from the ComboBox widgets and calls Ingred to fetch
*   the field. The select all button on the context menus only makes
*   sense for the point, line and area fields so it is not made visible
*   otherwise. Note that selections >= nfield are from a lchain type
*   field that can import links from the timelinking ans thus is a
*   special case.
*/
/*ARGSUSED*/
static void fetch_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if(selected_field_ndx < nfield)
	{
		if(!blank(mfd.vtime))
			(void) IngredVaCommand(GE_DEPICTION, "EDIT MERGE FETCH %s %s %s %s %s %s",
				mfd.sdef->name, blank(mfd.subdef->name)? "-":mfd.subdef->name,
				blank(mfd.rtime)? "-":mfd.rtime, mfd.vtime,
				mfd.fdef->element->name, mfd.fdef->level->name);
	}
	else
	{
		(void) IngredVaCommand(GE_DEPICTION, "EDIT MERGE FETCH %s %s - %s %s %s",
			mfd.sdef->name, blank(mfd.subdef->name)? "-":mfd.subdef->name,
			FpaFile_Links,
			mfd.fdef->element->name, mfd.fdef->level->name);
	}

	SetContextPasteSelectAllButtonsState(XtIsManaged(selectAllBtn)? SELECT_ALL_BUTTON_ON:SELECT_ALL_BUTTON_OFF);
}


/*
*	Sends a SELECT_ALL command to Ingred.
*/
/*ARGSUSED*/
static void select_all_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	(void) IngredCommand(GE_DEPICTION, "EDIT MERGE SELECT_ALL");
}


/*========== Process observer notification messages =================*/


/* In order to configure the merge panel SourceListByField() is called.
 * This function obtains its information from data that is not stable at
 * the time ingred sends us notification that a field has been created.
 * The configure function is thus run in a time out procedure that leaves
 * enough time for ingred to finish its processing and update the data
 * fields so that SourceListByField() will return the correct info.
 * Multiple calls within the timeout limit will reset the time out delay
 * so that the configure function is only called once.
 */
/*ARGSUSED*/
static void reconfigure(XtPointer data, XtIntervalId *id)
{
	static XtIntervalId tmid = 0;

	if (!id)
	{
		if (tmid) XtRemoveTimeOut(tmid);
		tmid = XtAppAddTimeOut(GV_app_context, 1000, reconfigure, data);
	}
	else
	{
		tmid = 0;
		no_field = False;
		ConfigureMergeFieldPanel();
		if(InEditMode(E_MERGE)) create_valid_time_list();
	}
}


/* Process edit select and deselect messages from Ingred to set
 * the sensitivity of the command buttons.
 */
static void action_ingred_messages( CAL cal, String *parms, int nparms )
{
	if (!GV_edit_mode) return;
	if (nparms < 3)    return;

	/* The E_FIELD and E_INTEPOLATE checks are to check for a creation or
	 * modification of the active field and for its interpolations and to
	 * regenerate the merge information.
	 */
	if(same_ic(parms[0],E_FIELD))
	{
		if(same_ic(parms[1],E_STATUS))
		{
			if (FindField(parms[2],parms[3]) == GV_active_field)
				reconfigure(NULL, NULL);
		}
	}
	else if(same_ic(parms[0],E_INTERPOLATE))
	{
			if (FindField(parms[1],parms[2]) == GV_active_field)
				reconfigure(NULL, NULL);
	}
	else if(same_ic(parms[0],E_EDIT) && InEditMode(E_MERGE))
	{
		if(same_ic(parms[1],E_BUTTON))
		{
			/* If the command arrives to turn off the cancel button it means that
			 * there are no fetched fields on the depiction and thus nothing to
			 * select so we turn off the select all buttons.
			 */
			if(same_ic(parms[2],E_CANCEL))
			{
				if(XtIsManaged(selectAllBtn))
					XtSetSensitive(selectAllBtn, same_ic(parms[3],E_ON));
				SetContextPasteSelectAllButtonsState(
						(XtIsManaged(selectAllBtn) && same_ic(parms[3],E_ON))? SELECT_ALL_BUTTON_ON:SELECT_ALL_BUTTON_OFF);
			}
		}
		else if(same_ic(parms[1],E_LINE))
		{
			/* If a line is selected we do not want the fetch button active.
			 */
			if(same_ic(parms[2], E_SELECT))
			{
				allow_fetch = False;
				set_fetch_button_sensitive(False);
			}
			else if(same_ic(parms[2], E_DESELECT))
			{
				allow_fetch = True;
				set_fetch_button_sensitive(have_valid_time);
			}
		}
	}
	else if(same_ic(parms[0],E_EDIT) && same_ic(parms[1],E_BUTTON))
	{
		/* This message means that the field needs to be created or not. */
		no_field = (same_ic(parms[2],E_CREATE) && same_ic(parms[3],E_ON));
	}
}


/* Intercept the message that says that a context menu command
 * has been issued for action on selected objects. When this
 * last happens we do not want the select all button to be
 * selectable.
 */
static void action_edit_commands( String *parms, int nparms )
{
	if(nparms > 0 && same_ic(parms[0], E_MERGE))
	{
		XtSetSensitive(selectAllBtn, False);
		SetContextPasteSelectAllButtonsState(SELECT_ALL_BUTTON_OFF);
	}
}


/* Update the valid time list on notification of a change in the
 * depiction time.
 */
static void action_depiction_change( String *parms, int nparms )
{
	if (showing) create_valid_time_list();
}


/* Update the merge source list when depictions are saved. See
 * the function reconfigure for an explanation of the process.
 */
static void action_depiction_saved( String *parms, int nparms )
{
	reconfigure(NULL, NULL);
}
