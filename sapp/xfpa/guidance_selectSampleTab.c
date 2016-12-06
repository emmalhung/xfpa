/*=========================================================================*/
/*
*      File: guidance_selectSampleTab.c
*
*   Purpose: Tab contents for sampling fields from the active list. Note
*            that any field, selected for display or not, can be sampled.
*            Upon first entering this tab the first selected field, if any,
*            is used as the active sampling selection.
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
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ComboBox.h>
#include <Xbae/Matrix.h>
#include <Xm/Column.h>
#include "depiction.h"
#include "editor.h"
#include "resourceDefines.h"
#include "help.h"
#include "guidance.h"
#include "observer.h"
#include "selector.h"
#include "contextMenu.h"

#define SAMPLE_GUIDANCE "sg"

static void send_cancel_sampling_to_ingred (void);
static void clear_cb   			   (Widget, XtPointer, XtPointer);
static void context_menu_cb		   (Widget, XtPointer, XtPointer);
static void field_select_cb		   (Widget, XtPointer, XtPointer);
static void flash_border           (Boolean);
static void on_off_cb	    	   (Widget, XtPointer, XtPointer);
static void reselect	           (void);
static void select_field_to_sample (int);
static void what_to_sample_cb  	   (Widget, XtPointer, XtPointer);
static void when_to_sample_cb	   (Widget, XtPointer, XtPointer);
static void sampling_observed      (String*, int);
static void send_sample_command	   (void);
static void set_sample_attributes  (FontSelectorStruct *);
static void set_tab_sensitivity    (void);
static void update_sample_tab      (Boolean);

static Widget  fieldList;
static Widget  sampleGrid, predefPoints;
static Widget  whatList, whenList;
static Widget  bottomLayoutForm, onOffGroup, onBtn, offBtn, clearBtn;
static Widget  panelContextMenu = NullWidget;

static int     nsample_fields   = 0;
static String  font_type        = NULL;
static String  font_size        = NULL;
static String  sample_colour    = NULL;
static String  sample_time      = NULL;
static Boolean sampling_allowed = False;
static Boolean sampling_active  = False;
static Boolean sampling_lockout = False;

static GuidanceFieldStruct *fld          = NULL;
static SampleListStruct    *sample_list  = NULL;
static SampleListStruct    selected_what = {NULL,NULL,NULL,NULL};


void InitSamplingPanel(void)
{
	Widget btn;

	/* The context menu is created only once as this is parented to the map window.
	 * This init function should only be called once so this is paranoia.
	 */
	if(panelContextMenu) return;

	panelContextMenu = CreatePanelContextMenu("guidanceContextMenu");

	btn = XmVaCreateManagedPushButton(panelContextMenu, "clearBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, context_menu_cb, (XtPointer) -1);

	/* Add an observer function to capture any other dialog going into
	 * a sampling mode. If this happens guidance sampling must be locked out.
	 */
	AddObserver(OB_DIALOG_SAMPLING, sampling_observed);
}


void LayoutGuidanceSampleTab(void)
{
	Widget     btn, fontSetManager, form, rc, onOffForm;
	XmString   header[4];
	Pixel      bkgnd;

	/* This form is used to hold the sampling composite widgets. This
	 * is done so that all of these widgets will layout with their tops
	 * aligned.
	 */
	bottomLayoutForm = XmVaCreateForm(GVG_sampleTab, "blf",
		XmNhorizontalSpacing, 0,
		XmNverticalSpacing, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	sampleGrid = CreateGridSelector(bottomLayoutForm, SELECT_GUID_SAMPLE,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	predefPoints = CreatePredefinedPointsSelector(bottomLayoutForm, SELECT_GUID_SAMPLE,
		XmNmarginWidth, 9,
		XmNmarginHeight, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, sampleGrid,
		XmNleftOffset, 5,
		NULL);

	fontSetManager = CreateFontSelector(bottomLayoutForm, set_sample_attributes,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, predefPoints,
		XmNleftOffset, 5,
		NULL);

	font_type     = GetFontSelectorValue(fontSetManager, SELECT_FONT_TYPE);
	font_size     = GetFontSelectorValue(fontSetManager, SELECT_FONT_SIZE);
	sample_colour = GetFontSelectorValue(fontSetManager, SELECT_COLOUR);

	XtManageChild(bottomLayoutForm);

	XtVaGetValues(GVG_sampleTab, XmNbackground, &bkgnd, NULL);

	form = XmVaCreateColumn(GVG_sampleTab, "aligner",
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNitemSpacing, 6,
		XmNlabelSpacing, 3,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, bottomLayoutForm,
		XmNbottomOffset, 19,
		NULL);

	whatList = XmVaCreateManagedComboBox(form, "whatList",
		XmNborderWidth, 2,
		XmNborderColor, bkgnd,
		XmNcolumns, 30,
		NULL);
	XtAddCallback(whatList, XmNselectionCallback, what_to_sample_cb, NULL);

	whenList = XmVaCreateManagedComboBox(form, "whenList",
		XmNborderWidth, 2,
		XmNborderColor, bkgnd,
		XmNcolumns, 30,
		NULL);
	XtAddCallback(whenList, XmNselectionCallback, when_to_sample_cb, NULL);
	XtManageChild(form);

	onOffGroup = XmVaCreateManagedFrame(GVG_sampleTab, "onOffGroup",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, form,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, form,
		XmNleftOffset, 19,
		NULL);
	
	(void)XmVaCreateManagedLabel(onOffGroup, "label",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	onOffForm = XmVaCreateForm(onOffGroup, "onOffForm",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 0,
		NULL);

	rc = XmVaCreateManagedRowColumn(onOffForm, "frame",
		XmNorientation, XmVERTICAL,
		XmNradioBehavior, True,
		XmNspacing, 0,
		XmNmarginWidth, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	onBtn = XmVaCreateManagedToggleButton(rc, "onBtn", NULL);
	XtAddCallback(onBtn, XmNvalueChangedCallback, on_off_cb, NULL);

	offBtn = XmVaCreateManagedToggleButton(rc, "offBtn", NULL);

	clearBtn = XmVaCreateManagedPushButton(onOffForm, "clearBtn",
		XmNmarginWidth, 15,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 15,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftWidget, rc,
		XmNleftOffset, 9,
		NULL);
	XtAddCallback(clearBtn, XmNactivateCallback, clear_cb, NULL);

	XtManageChild(onOffForm);


	btn = XmVaCreateManagedPushButton(GVG_sampleTab, "legend",
		XmNmarginWidth, 6,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, ShowGuidanceLegendCB, (XtPointer)'s');

	header[0] = XuGetXmStringResource(RNlevelHeader, "Level");
	header[1] = XuGetXmStringResource(RNfieldHeader, "Field");
	header[2] = XuGetXmStringResource(RNsourceHeader, "Source");
	header[3] = XuGetXmStringResource(RNissueHeader, "Issue Time");

	fieldList = CreateXbaeMatrix(GVG_sampleTab, "fieldList",
		XmNrows, 1,
		XmNcolumns, 4,
		XmNxmColumnLabels, header,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNshadowThickness, 2,
		XmNcellShadowThickness, 1,
		XmNgridType, XmGRID_ROW_SHADOW,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, btn,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, form,
		NULL);
	
	XtAddCallback(fieldList, XmNselectCellCallback, field_select_cb, NULL); 

	XmStringFree(header[0]);
	XmStringFree(header[1]);
	XmStringFree(header[2]);
	XmStringFree(header[3]);
}


void ActivateGuidanceSampleTab(void)
{
	String ob_parms[3];

	if (sampling_active) return;

	sampling_active = (!sampling_lockout && sampling_allowed);
	DeactivateMenu();
	update_sample_tab(True);
	XtSetSensitive(clearBtn, sampling_active);
	XuUpdateDisplay(GVG_sampleTab);
	/*
	 * Careful here. Notification of sampling needs to be send before
	 * sending ingred a sampling command as any other dialogs that are
	 * in sample mode need to send an exit sampling command.
	 */
	if(sampling_active)
	{
		ob_parms[0] = OB_KEY_GUIDANCE;
		ob_parms[1] = "on";
		ob_parms[2] = (String) ((GVG_option_synchro)? reselect : send_sample_command);
		NotifyObservers(OB_DIALOG_SAMPLING, ob_parms, 3);
		XuSetCursor(GW_mapWindow, XuSAMPLE_CURSOR, ON);
		SetActiveContextMenu(panelContextMenu);
		SetGuidanceDialogSensitivity(OFF);
		reselect();
	}
	else
	{
		ActivateMenu();
	}
	set_tab_sensitivity();
}


void DeactivateGuidanceSampleTab(void)
{
	String ob_parms[] = {OB_KEY_GUIDANCE,"off"};

	if (sampling_active)
	{
		sampling_active = False;
		send_cancel_sampling_to_ingred();
		SetActiveContextMenu(None);
		flash_border(False);
		sample_time = NULL;
		FreeItem(sample_list);
		XtSetSensitive(clearBtn, False);
		/* 
		 * Only send the off command if the deactivation was not caused by
		 * another dialog entering sampling mode.
		 */
		if (!sampling_lockout)
			NotifyObservers(OB_DIALOG_SAMPLING, ob_parms, 2);

		SetGuidanceDialogSensitivity(ON);
		set_tab_sensitivity();
		XuSetCursor(GW_mapWindow, XuSAMPLE_CURSOR, OFF);
	}
	set_tab_sensitivity();
}


/* Respond to selections made in the guidanceDialog. The list_change parameter
*  should be False unless the guidance list was changed just before calling
*  this function, in which case it will be True.
*/
void UpdateGuidanceSampleTab(Boolean list_change)
{
	if(GVG_sample_tab_active)
	{
		if (sampling_active)
		{
			send_cancel_sampling_to_ingred();
			update_sample_tab(True);
			SendGuidanceSampleCommand(NULL);
		}
		else
		{
			DeactivateMenu();
			update_sample_tab(True);
			ActivateMenu();
		}
	}
	else
	{
		update_sample_tab(list_change);
	}
}


/* Send the command to sample to Ingred.
 */
void SendGuidanceSampleCommand(String option)
{
	Pixel colour;

	if (sampling_lockout || !sampling_active)
	{
		flash_border(False);
	}
	else
	{
		if(matching_tstamps(sample_time, ActiveDepictionTime(FIELD_INDEPENDENT)))
		{
			flash_border(False);
			if(fld->rtype == GUID_DEPICT)
			{
				if(XuComboBoxGetSelectedPos(whenList) == 2)
					colour = XuLoadColorResource(XtParent(whenList), RNguidanceRunNA, "Red");
				else
					XtVaGetValues(XtParent(whenList), XmNbackground, &colour, NULL);
			}
			else
			{
				XtVaGetValues(XtParent(whenList), XmNbackground, &colour, NULL);
			}
		}
		else
		{
			flash_border(True);
		}

		(void) IngredVaCommand(GE_GUIDANCE,"SAMPLE %s %s %s %s %s %s %s%% %s",
			blank(option)? "NORMAL":option,
			fld->id,
			sample_time? sample_time:GVG_active_time,
			selected_what.type, selected_what.name,
			font_type, font_size, sample_colour);
	}
}


FpaConfigFieldStruct *GetGuidanceSampleField(void)
{
	return (sampling_active? fld->info : (FpaConfigFieldStruct *)NULL);
}


/************************** LOCAL FUNCTIONS ************************/


/* Any dialog entering into sampling mode will trigger this function,
 * including ourselves. Thus if guidance is the origionator the call
 * must be ignored. A valid call from any other dialog will result in
 * this dialog putting itself into a "I cannot enter into sampling"
 * mode. This is done by making the tab insensitive so that it or
 * anything on it cannot be selected.
 */
static void sampling_observed( String *parms, int nparms )
{
	if( nparms < 2 ) return;					/* something wrong */
	if( same(parms[0],OB_KEY_GUIDANCE) ) return;/* this is our own call */
	if( !GVG_selectDialog ) return;				/* the guidance dialog must exist */

	if(same(parms[1],"on"))
	{
		sampling_lockout = True;
		if(sampling_active)
			XuToggleButtonSet(offBtn, True, True);
		else
			set_tab_sensitivity();
	}
	else
	{
		sampling_lockout = False;
		set_tab_sensitivity();
	}
}


/* Set the sensitivity state of the tab widgets. Note that the on/off
 * block always stays sensitive.
 */
static void set_tab_sensitivity (void)
{
	int    n;
	Widget *kids[6] = { &fieldList, &sampleGrid, &predefPoints, &whatList, &whenList, &clearBtn };

	for(n = 0; n < XtNumber(kids); n++)
	{
		if(!sampling_allowed)
			XtSetSensitive(*(kids[n]), False);
		else if(!sampling_active && *(kids[n]) != onOffGroup)
			XtSetSensitive(*(kids[n]), False);
		else
			XtSetSensitive(*(kids[n]), True);
	}
	/*
	 * Reset the button state in the on/off group.
	 */
	if(!sampling_allowed)
	{
		XmToggleButtonSetState(onBtn, False, False);
		XmToggleButtonSetState(offBtn, False, False);
	}
	else
	{
		XmToggleButtonSetState(onBtn, sampling_active, False);
		XmToggleButtonSetState(offBtn, !sampling_active, False);
	}
}




/* Update the contents of the tab widgets.
*/
static void update_sample_tab(Boolean list_change)
{
	int i, j, n, nrows, active = 0;

	SelectedFieldInfo(fld, False, NULL, &n);

	if(!(sampling_allowed = (n > 0)))
	{
		nsample_fields = 0;
		nrows = XbaeMatrixNumRows(fieldList);
		XbaeMatrixDeleteRows(fieldList, 0, nrows);
		XuComboBoxSetString(whatList, " ");
		XuComboBoxSetString(whenList, " ");
		flash_border(False);
	}
	else
	{
		/*
		 * In the following code, if there was a field previously selected for sampling we want to
		 * find out what its index will be in the list of fields available for sampling. This has
		 * to be done if we regenerate the list or not. Note that the SelectedFieldInfo function will
		 * not return the correct value for the active field for sampling.
		 */
		if( list_change || n!= nsample_fields )
		{
			String  cell_data[4];
			char    mbuf[256];

			cell_data[3] = mbuf;

			nsample_fields = n;
			nrows = XbaeMatrixNumRows(fieldList);
			XbaeMatrixDeleteRows(fieldList, 0, nrows);

			for(nrows = 0, i = 0; i < GVG_nguidlist; i++)
			{
				for(j = 0; j < GVG_guidlist[i]->nfield; j++)
				{
					if(GVG_guidlist[i] != GVG_active_guidlist && !GVG_guidlist[i]->field[j]->showing) continue;
					if(!GVG_guidlist[i]->field[j]->available) continue;
					if(!GVG_guidlist[i]->field[j]->info->element->elem_detail) continue;
					if(!GVG_guidlist[i]->field[j]->info->element->elem_detail->sampling->type.continuous) continue;

					if(GVG_option_full_display)
					{
						cell_data[0] = GVG_guidlist[i]->field[j]->info->level->label;
						cell_data[1] = GVG_guidlist[i]->field[j]->info->element->label;
					}
					else
					{
						cell_data[0] = GVG_guidlist[i]->field[j]->info->level->sh_label;
						cell_data[1] = GVG_guidlist[i]->field[j]->info->element->sh_label;
					}
					cell_data[2] = SrcShortLabel(GVG_guidlist[i]->field[j]->source);
					switch(GVG_guidlist[i]->field[j]->rtype)
					{
						case GUID_CURRENT:
							(void)strcpy(cell_data[3], XuGetLabel(CURRENT_STRING));
							break;
						case GUID_PREVIOUS:
							(void)strcpy(cell_data[3], XuGetLabel(PREVIOUS_STRING));
							break;
						case GUID_ABSOLUTE:
							(void)strcpy(cell_data[3], DateString(GVG_guidlist[i]->field[j]->run->time,HOURS));
							break;
						default:
							(void)strcpy(cell_data[3], XuGetLabel("na-short"));
							break;
					}
					XbaeMatrixAddRows(fieldList, nrows, cell_data, NULL, NULL, 1);
					if( fld == GVG_guidlist[i]->field[j] ) active = nrows;
					nrows++;
				}
			}
			if(nrows < 1) XbaeMatrixAddRows(fieldList, 0, NULL, NULL, NULL, 1);
		}
		else if(fld)
		{
			for(nrows = 0, i = 0; i < GVG_nguidlist; i++)
			{
				for(j = 0; j < GVG_guidlist[i]->nfield; j++)
				{
					if(GVG_guidlist[i] != GVG_active_guidlist && !GVG_guidlist[i]->field[j]->showing) continue;
					if(!GVG_guidlist[i]->field[j]->available) continue;
					if(!GVG_guidlist[i]->field[j]->info->element->elem_detail) continue;
					if(!GVG_guidlist[i]->field[j]->info->element->elem_detail->sampling->type.continuous) continue;
					if(fld == GVG_guidlist[i]->field[j]) active = nrows;
					nrows++;
				}
			}
		}
		set_tab_sensitivity();
		XbaeMatrixResizeColumnsToCells(fieldList, True);
		if (sampling_active)
		{
			select_field_to_sample(active);
			XbaeMatrixSelectRow(fieldList, active);
		}
	}
}



static void flash_border_cb(XtPointer client_data, XtIntervalId *id)
{
	Boolean show;

	static Boolean      first = True;
	static XtIntervalId myid = (XtIntervalId)NULL;
	static Pixel        bkgnd, yellow;

	if (first)
	{
		first = False;
		XtVaGetValues(XtParent(whenList), XmNbackground, &bkgnd, NULL);
		yellow = XuLoadColorResource(XtParent(whenList), RNguidanceRunCaution, "Yellow");
	}
	show = PTR2BOOL(client_data);
	if(id)
	{
		XtVaSetValues(whenList, XmNborderColor, show? yellow:bkgnd, NULL);
		show = !show;
		myid = XtAppAddTimeOut(GV_app_context, 500, flash_border_cb, INT2PTR(show));
	}
	else if(show)
	{
		if (IsNull(myid))
		{
			XtVaSetValues(whenList, XmNborderColor, show? yellow:bkgnd, NULL);
			show = !show;
			myid = XtAppAddTimeOut(GV_app_context, 500, flash_border_cb, INT2PTR(show));
		}
	}
	else
	{
		if (NotNull(myid)) XtRemoveTimeOut(myid);
		myid = (XtIntervalId)NULL;
		XtVaSetValues(whenList, XmNborderColor, bkgnd, NULL);
	}
}


static void flash_border(Boolean on)
{
	flash_border_cb(INT2PTR(on), NULL);
}

/* This function is registered with the menu control functions via the
*  NotifyObservers() function. It will be called when the active depiction
*  is changed by the user.
*/
static void reselect(void)
{
	int ndx;
	String dt;
	FLD_DESCRIPT fd, *fdptr;

	if (!sampling_active) return;
	if (!fld) return;
	if (!fld->info) return;
	if (!fld->info->element) return;

	if(fld->rtype == GUID_DEPICT)
	{
		fdptr = fld->source->fd;
	}
	else
	{
		fdptr = &fd;
		copy_fld_descript(&fd, fld->source->fd);
		(void)set_fld_descript(&fd, FpaF_RUN_TIME, fld->run->time, FpaF_END_OF_LIST);
	}
	closest_source_valid_time(fdptr, fld->info->element->elem_tdep->time_dep,
			GVG_active_time, &dt);
	(void)InTimeList(dt, fld->valid->times, fld->valid->ntimes, &ndx);
	XuComboBoxSelectPos(whenList, ndx+1, True);
}


static void select_field_to_sample( int list_ndx )
{
	int i, j, ndx, sel, nlist;
	char mbuf[32];
	String dt, rt, *items;
	FLD_DESCRIPT fd;

	XuComboBoxDeleteAllItems(whatList);
	XuComboBoxDeleteAllItems(whenList);
	FreeItem(sample_list);

	list_ndx++;
	ndx = 0;
	for(i = 0; i < GVG_nguidlist; i++)
	{
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			fld = GVG_guidlist[i]->field[j];
			if(!fld->showing && GVG_active_guidlist != GVG_guidlist[i]) continue;
			if(!fld->available) continue;
			if(!fld->info->element->elem_detail) continue;
			if(!fld->info->element->elem_detail->sampling) continue;
			/* Note that the sampling "type" is a union, so we only need
			*  to check for "type.continuous"
			*/
			if(!fld->info->element->elem_detail->sampling->type.continuous)
				continue;
			ndx++;
			if(list_ndx == ndx) break;
		}
		if(list_ndx == ndx) break;
	}
	if(ndx < 1) return;

	/* If the field has not been previously registered we must do it now.
	*/
	if(blank(fld->id))
	{
		(void)snprintf(fld->id, sizeof(fld->id), "%d.%d", fld->list->id_key, fld->id_key);
		(void) IngredVaCommand(GE_GUIDANCE, "FIELD_REGISTER %s %s %s %s %s %s",
			fld->id,
			fld->info->element->name, fld->info->level->name,
			SrcName(fld->source), SrcSubDashName(fld->source),
			blank(fld->run->time)? "-":fld->run->time);
	}

	/* Now make the list of things which can be sampled.
	*/
	nlist = 0;
	copy_fld_descript(&fd, fld->source->fd);
	if(set_fld_descript(&fd,
		FpaF_RUN_TIME, (fld->rtype == GUID_DEPICT)? NULL:fld->run->time,
		FpaF_ELEMENT, fld->info->element,
		FpaF_LEVEL, fld->info->level,
		FpaF_VALID_TIME, sample_time? sample_time:GVG_active_time,
		FpaF_END_OF_LIST))
	{
		MakeSampleItemList(&fd, &sample_list, &nlist);
	}

	sel = 0;
	items = NewStringArray(nlist);
	for(i = 0; i < nlist; i++) items[i] = sample_list[i].label;
	XuComboBoxAddItems(whatList, items, nlist, 0);
	XtVaSetValues(whatList, XmNvisibleItemCount, MIN(nlist,10), NULL);
	FreeItem(items);
	if(nlist > 0)
	{
		Boolean sensitive;
		XuComboBoxSelectPos(whatList, (int) fld->sample_what_ndx+1, False);
		selected_what.type = sample_list[(int) fld->sample_what_ndx].type;
		selected_what.name = sample_list[(int) fld->sample_what_ndx].name;
		sensitive = (!same_ic(selected_what.name, AttribAll) &&
					!(same_ic(selected_what.name, AttribFieldLabels) || same_ic(selected_what.name, AttribLinkNodes)));
		XtSetSensitive(predefPoints, sensitive);
		XtSetSensitive(sampleGrid,   sensitive);
	}

	/* Make the list of when we can sample.
	*/
	items = NewStringArray(fld->valid->ntimes);
	rt = (fld->rtype == GUID_DEPICT)? GV_T0_depict:fld->run->time;
	for(i = 0; i < fld->valid->ntimes; i++)
	{
		if(valid_tstamp(dt = fld->valid->times[i]))
		{
			(void) snprintf(mbuf, sizeof(mbuf), "%s (%s)",
							TimeDiffFormat(rt, dt, SHOW_MINUTES(fld)), GuidFieldDateFormat(fld,dt));
			items[i] = XtNewString(mbuf);
		}
		else
		{
			items[i] = XtNewString(XuGetLabel(dt));
		}
	}
	XuComboBoxAddItems(whenList, items, fld->valid->ntimes, 0);
	FreeList(items, fld->valid->ntimes);
	XtVaSetValues(whenList, XmNvisibleItemCount, MIN(fld->valid->ntimes,10), NULL);
	reselect();
}


/*ARGSUSED*/
static void field_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, nrows;
	XbaeMatrixSelectCellCallbackStruct *rtn = (XbaeMatrixSelectCellCallbackStruct *)call_data;

	if( rtn->row < 0 || rtn->column < 0 || rtn->row >= nsample_fields ) return;

	XtVaGetValues(w, XmNrows, &nrows, NULL);
	for( i = 0; i < nrows; i++ )
		XbaeMatrixDeselectRow(w, i);
	XbaeMatrixSelectRow(w, rtn->row);
	select_field_to_sample(rtn->row);
}


/*ARGSUSED*/
static void what_to_sample_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	Boolean sensitive;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(rtn->item_position < 1) return;
	if(sample_list == NULL) return;

	send_cancel_sampling_to_ingred();
	fld->sample_what_ndx = MIN(126,(char) (rtn->item_position - 1));
	selected_what.type = sample_list[(int) fld->sample_what_ndx].type;
	selected_what.name = sample_list[(int) fld->sample_what_ndx].name;
	send_sample_command();

	sensitive = (!same_ic(selected_what.name, AttribAll) &&
				!(same_ic(selected_what.name, AttribFieldLabels) || same_ic(selected_what.name, AttribLinkNodes)));
	XtSetSensitive(predefPoints, sensitive);
	XtSetSensitive(sampleGrid,   sensitive);
}


/*ARGSUSED*/
static void when_to_sample_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int n, ndx;
	String dt;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(rtn->item_position < 1) return;

	ndx = rtn->item_position - 1;

	if(fld->rtype == GUID_DEPICT && ndx < 2)
	{
		closest_source_valid_time(fld->source->fd,
			fld->info->element->elem_tdep->time_dep, GVG_active_time, &dt);
		(void)InTimeList(dt, fld->valid->times, fld->valid->ntimes, &n);
		if(ndx == 1) n--;
		ndx = MAX(n, 2);
	}
	sample_time = fld->valid->times[ndx];
	send_sample_command();
}


static void set_sample_attributes(FontSelectorStruct *font)
{
	font_type     = font->type;
	font_size     = font->size;
	sample_colour = font->colour;
	
	if(font->reason != SELECT_NONE)
		SendGuidanceSampleCommand(NULL);
}


static void send_sample_command(void)
{
	SendGuidanceSampleCommand(NULL);
}


/* Send the SAMPLE CANCEL command to ingred */
static void send_cancel_sampling_to_ingred(void)
{
	if (!fld) return;
	if (blank(fld->id)) return;
	if (blank(selected_what.type)) return;
	if (blank(selected_what.name)) return;

	(void) IngredVaCommand(GE_GUIDANCE,"SAMPLE CANCEL %s %s %s %s",
		fld->id,
		sample_time? sample_time:GVG_active_time,
		selected_what.type, selected_what.name);
}


/*ARGSUSED*/
static void clear_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	/*
	send_cancel_sampling_to_ingred();
	*/
	(void) IngredCommand(GE_EDIT, E_CLEAR);
}


/*ARGSUSED*/
static void on_off_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(XmToggleButtonGetState(w))
	{
		sampling_lockout = False;
		ActivateGuidanceSampleTab();
	}
	else
	{
		DeactivateGuidanceSampleTab();
	}
}


static void context_menu_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int ndx = PTR2INT(client_data);

	if(ndx < 0)
	{
		(void) IngredCommand(GE_EDIT, E_CLEAR);
	}
	else
	{
	}
}
