/*****************************************************************************/
/*
*  File:	 panel_interpolate.c
*  
*  Purpose:  Provides the controlling logic for the intepolation operation.
*
*  Note:     This file also contains the code to:
*
*            1.Put up the selector for which field(s) to interpolate.
*            2.Put up the interpolation progress and cancel button display.
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
/*****************************************************************************/

#include <string.h>
#include "global.h"
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include <ingred.h>
#include "resourceDefines.h"
#include "guidance.h"
#include "observer.h"
#include "productStatus.h"
#include "timelink.h"


static Widget  groupWindow;
static int     ngrp_last = 0;
static int     ngrpBtns = 0;
static Widget  grpBtnsMan;			/* rowColumn manager for the group toggles */
static Widget  *grpBtns = NULL;
static Widget  grpIndsMan;			/* rowColumn manager for the group indicators */
static Widget  *grpInds = NULL;
static Widget  fieldWindow;
static int     nfldLabels = 0;
static Widget  fldLabelsMan;		/* rowColumn manager for the field toggles */
static Widget  *fldLabels = NULL;
static Widget  fldIndsMan;			/* rowColumn manager for the field indicators */
static Widget  *fldInds = NULL;

static Pixel   notLinkableColor;
static Pixel   noLinkColor;		/* colour to use for no link */
static Pixel   partialColor;	/* colour to use for partial links */
static Pixel   almostColor;		/* colour to use for linked but not interpolated */
static Pixel   fldIntColor;     /* colour to use for field but not labels interpolated */
static Pixel   allSetColor;		/* colour for linked and interpolated */

static Dimension grp_btn_height, grp_ind_height, grp_ind_width;
static Dimension fld_btn_height, fld_ind_height;
static Boolean   panel_active   = False;
static int       selected_group = 0;


static void display_group_list       (void);
static void destroy_progress_display (void);
static void group_cb 		         (Widget, XtPointer, XtPointer);
static void make_select_field_list   (Widget);
static void interpolate_cmd_cb       (Widget, XtPointer, XtPointer);
static void interpolate_panel_update (CAL, String*, int);
static void update_progress_display  (CAL, String*, int);


/**********************************************************************/
/*
*   Create the interpolate control panel widgets.
*/
/**********************************************************************/
void CreateInterpolatePanel(Widget parent)
{
	int n;
	Dimension height, spacing;
	Widget btn, groupLabel, groupForm, fieldLabel, fieldForm, frame, rc;


	GW_interpolatePanel = parent;

	groupLabel = XmVaCreateManagedLabel(GW_interpolatePanel, "group",
		XmNresizable, False,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		NULL);

	groupWindow = XmVaCreateManagedScrolledWindow(GW_interpolatePanel, "groupWindow",
		XmNresizable, False,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNspacing, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, groupLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 4,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 4,
		NULL);

	groupForm = XmVaCreateManagedForm(groupWindow, "groupForm", NULL);

	grpIndsMan = XmVaCreateManagedRowColumn(groupForm, "indsMan",
		XmNpacking, XmPACK_COLUMN,
		XmNadjustLast, False,
		XmNadjustMargin, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	grpBtnsMan = XmVaCreateManagedRowColumn(groupForm, "grpBtnsMan",
		XmNradioBehavior, True,
		XmNmarginWidth, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, grpIndsMan,
		NULL);

	/* Set the spacing of the row column managers used by the indicator
	*  widgets depending on the size of the selector buttons.
	*/
	btn = XmVaCreateManagedToggleButton(grpBtnsMan, "tmp", NULL);
	XtVaGetValues(btn, XmNheight, &grp_btn_height, NULL);
	grp_ind_height = grp_btn_height - INDICATOR_DIFF + INDICATOR_MARGIN*2;
	grp_ind_width = (grp_btn_height - INDICATOR_DIFF)/2 + INDICATOR_MARGIN*2;
	height = grp_btn_height - grp_ind_height;
	XtVaSetValues(grpIndsMan, XmNspacing, height, XmNmarginHeight, height-2, NULL);
	XtDestroyWidget(btn);

	fieldLabel = XmVaCreateManagedLabel(GW_interpolatePanel, "fieldLabel",
		XmNresizable, False,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, groupWindow,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		NULL);

	fieldWindow = XmVaCreateManagedScrolledWindow(GW_interpolatePanel, "fieldWindow",
		XmNresizable, False,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNspacing, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldLabel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 4,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 4,
		NULL);

	fieldForm = XmVaCreateManagedForm(fieldWindow, "fieldForm", NULL);

	fldIndsMan = XmVaCreateManagedRowColumn(fieldForm, "indsMan",
		XmNspacing, height,
		XmNmarginHeight, height-2,
		XmNadjustLast, False,
		XmNadjustMargin, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	fldLabelsMan = XmVaCreateManagedRowColumn(fieldForm, "fldLabelsMan",
		XmNmarginWidth, 0,
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, fldIndsMan,
		NULL);

	/* The size of labels is different from toggle buttons.
	*/
	btn = XmVaCreateManagedLabel(fldLabelsMan, "tmp", NULL);
	XtVaGetValues(btn, XmNheight, &fld_btn_height, NULL);
	fld_ind_height = fld_btn_height - INDICATOR_DIFF + INDICATOR_MARGIN*2;
	height = fld_btn_height - fld_ind_height;
	XtVaSetValues(fldIndsMan, XmNspacing, height, XmNmarginHeight, height-2, NULL);
	XtDestroyWidget(btn);

	btn = XmVaCreateManagedPushButton(GW_interpolatePanel, "legendBtn",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldWindow,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 9,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, ShowFieldStatusLegendCB, NULL);

	frame = XmVaCreateManagedFrame(GW_interpolatePanel, "frame",
		XmNmarginWidth, 9,
		XmNmarginHeight, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, btn,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 4,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 4,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "frameLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(frame, "rc", 
		XmNspacing, 9,
		XmNentryAlignment, XmALIGNMENT_CENTER,
		NULL);

	btn = XmVaCreateManagedPushButton(rc, "selectInterp",
		XmNmarginHeight, 5,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, interpolate_cmd_cb, (XtPointer)False);

	btn = XmVaCreateManagedPushButton(rc, "allInterp",
		XmNmarginHeight, 5,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, interpolate_cmd_cb, (XtPointer)True);


	/* Get the indicator colours from the resource database.
	*/
	notLinkableColor = XuLoadColorResource(parent, RNnotLinkableColorBg,"IndianRed");
	noLinkColor      = XuLoadColorResource(parent, RNnoLinkColorBg,"red");
	partialColor     = XuLoadColorResource(parent, RNpartialLinkColorBg,"blue");
	almostColor      = XuLoadColorResource(parent, RNalmostColorBg,"yellow");
	fldIntColor      = XuLoadColorResource(parent, RNfieldInterpColorBg,"green");
	allSetColor      = XuLoadColorResource(parent, RNallSetColorBg,"ForestGreen");


	/* Set the size of the windows containing the groups and fields
	*  to that as found in the resource file. This will minimize the
	*  amount of change later - it "feels" better.
	*/
	XtVaGetValues(grpBtnsMan,
		XmNspacing, &spacing,
		XmNmarginHeight, &height,
		NULL);

	n = XuGetIntResource(RNinterpPanelMaxGroupBtns, 6);
	XtVaSetValues(groupWindow,
		XmNheight, (Dimension)((int)(grp_btn_height+spacing)*n+(int)height*4),
		NULL);

	n = XuGetIntResource(RNinterpPanelMinFieldBtns, 6);
	XtVaSetValues(fieldWindow,
		XmNheight, (Dimension)((int)(fld_btn_height+spacing)*n+(int)height*4),
		NULL);

	XtManageChild(GW_interpolatePanel);

	AddIngredObserver(update_progress_display);
	AddIngredObserver(interpolate_panel_update);
}


/**********************************************************************/
/*
*  Controls the startup of the timelink program. If an error is
*  detected the return is False.
*/
/**********************************************************************/
void InterpolateStartup(void)
{
	panel_active = True;
	display_group_list();
	selected_group = 0;
	if(GV_ngroups > 0) XuToggleButtonSet(grpBtns[selected_group], True, True);
	(void) IngredCommand(GE_TIMELINK, "ENTER");
}


/**********************************************************************/
/*
*   Handle exiting from the panel.
*/
/**********************************************************************/
void InterpolateExit(void)
{
	panel_active = False;
	(void) IngredCommand(GE_TIMELINK, EXIT);
	XtUnmanageChildren(grpBtns, ngrpBtns);
	XtUnmanageChildren(grpInds, ngrpBtns);
	XtUnmanageChildren(fldLabels, nfldLabels);
	XtUnmanageChildren(fldInds, nfldLabels);
}


/*===================== LOCAL FUNCTIONS FOLLOW ============================*/


/* Create and display the list of available groups along with their status.
*/
static void display_group_list(void)
{
	int i, nbtns, ngrp_btns, extra;
	Dimension height, spacing, grp_height, fld_height, scroll_spacing;
	Widget scroll;
	XmString label;
	Pixel color;
	XtWidgetGeometry size;

	XtUnmanageChildren(grpBtns, ngrpBtns);
	XtUnmanageChildren(grpInds, ngrpBtns);

	if(GV_ngroups > ngrpBtns)
	{
		label = XuNewXmString("");
		grpBtns = MoreWidgetArray(grpBtns, GV_ngroups);
		grpInds = MoreWidgetArray(grpInds, GV_ngroups);
		for(i = ngrpBtns; i < GV_ngroups; i++)
		{
			grpBtns[i] = XmCreateToggleButton(grpBtnsMan, "gb", NULL, 0);
			XtAddCallback(grpBtns[i], XmNvalueChangedCallback, group_cb, INT2PTR(i));
			grpInds[i] = XmVaCreateLabel(grpIndsMan, "gb",
				XmNlabelString, label,
				XmNrecomputeSize, False,
				XmNheight, grp_ind_height,
				XmNwidth, grp_ind_width,
				XmNmarginWidth, 0,
				XmNmarginHeight, 0,
				NULL);
		}
		ngrpBtns = GV_ngroups;
		XmStringFree(label);
	}

	for(i = 0; i < GV_ngroups; i++)
	{
		XuWidgetLabel(grpBtns[i], GV_groups[i]->label);
		SetGroupLinkStatus(GV_groups[i]);
		switch(GV_groups[i]->fields_link_status)
		{
			case NOT_LINKABLE: color = notLinkableColor; break;
			case NOLINKS:	   color = noLinkColor;  break;
			case SOME_LINKS:   color = partialColor; break;
			case LINKED:	   color = almostColor;  break;
			case FIELD_INTERP: color = fldIntColor;  break;
			case INTERPOLATED: color = allSetColor;  break;
		}
		XtVaSetValues(grpInds[i], XmNbackground, color, NULL);
	}

	XtManageChildren(grpInds, GV_ngroups);
	XtManageChildren(grpBtns, GV_ngroups);

	/* Set the height of the group and field windows to fit the number of
	*  buttons specified for each in the resource file. Note that the group
	*  window is only allowed to shrink while the field window is only
	*  allowed to grow.
	*/
	if(GV_ngroups != ngrp_last)
	{
		ngrp_last = GV_ngroups;
		ngrp_btns = XuGetIntResource(RNinterpPanelMaxGroupBtns, 6);
		nbtns = MIN(ngrp_btns, GV_ngroups);
		XtVaGetValues(grpBtnsMan,
			XmNspacing, &spacing,
			XmNmarginHeight, &height,
			NULL);
		height = 4*height;
		XtVaGetValues(groupWindow,
			XmNhorizontalScrollBar, &scroll,
			XmNspacing, &scroll_spacing,
			NULL);
		extra = 0;
		if(XtIsManaged(scroll) && nbtns > ngrp_btns)
		{
			size.request_mode = CWHeight;
			(void) XtQueryGeometry(scroll, NULL, &size);
			extra = (int)(size.height + scroll_spacing);
		}
		grp_height = (Dimension)((int)(grp_btn_height+spacing)*nbtns+(int)height+extra);
		XtVaSetValues(groupWindow, XmNheight, grp_height, NULL);

		nbtns = XuGetIntResource(RNinterpPanelMinFieldBtns, 6);
		fld_height = (Dimension)((int)(fld_btn_height+spacing)*nbtns+(int)height);
		XtVaSetValues(fieldWindow, XmNtopOffset, 1, NULL);
		XtVaSetValues(fieldWindow, XmNheight, fld_height, XmNtopOffset, 0, NULL);
	}
	XuUpdateDisplay(GW_interpolatePanel);
}


/*ARGSUSED*/
static void group_cb(Widget w, XtPointer client_data, XtPointer call_data )
{
	int i, n;
	XmString label;
	Pixel color;
	GROUP *g;

	if(!XmToggleButtonGetState(w)) return;

	g = GV_groups[PTR2INT(client_data)];

	XtUnmanageChildren(fldLabels, nfldLabels);
	XtUnmanageChildren(fldInds, nfldLabels);

	if(g->nfield > nfldLabels)
	{
		label = XuNewXmString("");
		fldLabels = MoreWidgetArray(fldLabels, g->nfield);
		fldInds   = MoreWidgetArray(fldInds, g->nfield);
		for(i = nfldLabels; i < g->nfield; i++)
		{
			fldLabels[i] = XmCreateLabel(fldLabelsMan, "fb", NULL, 0);
			fldInds[i] = XmVaCreateLabel(fldIndsMan, "gb",
				XmNlabelString, label,
				XmNrecomputeSize, False,
				XmNheight, fld_ind_height,
				XmNwidth, grp_ind_width,
				XmNmarginWidth, 0,
				XmNmarginHeight, 0,
				NULL);
		}
		nfldLabels = g->nfield;
		XmStringFree(label);
	}

	/* Display the list of fields. Not only must a field exist to be put
	 * in the list but it must be editable as well.
	 */
	n = 0;
	for(i = 0; i < g->nfield; i++)
	{
		if(!g->field[i]->exists) continue;
		if(!g->field[i]->info->element->elem_detail->editor) continue;

		XuWidgetLabel(fldLabels[n], g->field[i]->info->sh_label);
		switch(g->field[i]->link_status)
		{
			case NOT_LINKABLE: color = notLinkableColor; break;
			case NOLINKS:	   color = noLinkColor;  break;
			case SOME_LINKS:   color = partialColor; break;
			case LINKED:	   color = almostColor;  break;
			case FIELD_INTERP: color = fldIntColor;  break;
			case INTERPOLATED: color = allSetColor;  break;
		}
		XtVaSetValues(fldInds[n], XmNbackground, color, NULL);
		n++;
	}
	XtManageChildren(fldInds, n);
	XtManageChildren(fldLabels, n);
	XuUpdateDisplay(GW_interpolatePanel);
}


/*ARGSUSED*/
static void interpolate_cmd_cb(Widget w, XtPointer client, XtPointer user)
{
	int     nproduct;
	String  notify[1] = {(String) True};
	Boolean do_all = PTR2BOOL(client);

	ProductStatusGetInfo(PS_RUNNING, &nproduct, NULL);
	if(nproduct > 0)
	{
		if( XuAskUser(w, "SomethingGenerating", NULL) == XuNO ) return;
	}
	if (do_all)
	{
		(void) IngredCommand(GE_TIMELINK, "INTERPOLATE");
		destroy_progress_display();
	}
	else
	{
		/* The user wants to select the fields to interpolate. Check to see if
		*  there are any to put into a list. If not put up message.
		*/
		int i;
		for(i = 0; i < GV_nfield; i++)
			if(GV_field[i]->exists && 
			(GV_field[i]->link_status == LINKED || GV_field[i]->link_status == FIELD_INTERP))
				break;
		if(i < GV_nfield) make_select_field_list(w);
		else              XuShowMessage(w, "NoFieldsToInterp", NULL);
	}
	NotifyObservers(OB_INTERPOLATE, notify, 1);
}


/* =========== Field Select for Interpolation Dialog ==================*/

static Widget  select_dialog  = NULL;


/* Send Ingred the list of fields to the interpolated.
*/
/*ARGSUSED*/
static void selected_field_cb(Widget w, XtPointer client, XtPointer user)
{
	int     i, n;
	Widget  rc;
	Boolean *selected;
	String  id[] = {"rc1","rc2"};

	selected = NewBooleanArray(GV_nfield);

	for( n = 0; n < 2; n++ )
	{
		rc = XtNameToWidget(select_dialog, id[n]);
		for(i = 0; i < GV_nfield; i++)
		{
			w = XtNameToWidget(rc, GV_field[i]->info->label);
			if(NotNull(w) && XmToggleButtonGetState(w)) selected[i] = True;
		}
	}

	XuDestroyDialog(select_dialog);

	for(i = 0; i < GV_nfield; i++)
	{
		if(!selected[i]) continue;
		(void) IngredVaCommand(GE_TIMELINK, "INTERPOLATE FIELD %s %s",
						GV_field[i]->info->element->name,
						GV_field[i]->info->level->name   );
	}

	destroy_progress_display();
	XtFree(selected);
}


/* Make the list of fields which can be interpolated.
*/
static void make_select_field_list(Widget parent)
{
	int    i, count;
	Widget label, rc;

	static XuDialogActionsStruct action_items[] = {
		{ "acceptBtn", selected_field_cb,   NULL },
		{ "cancelBtn", XuDestroyDialogCB, NULL }
	};

	if(NotNull(select_dialog)) return;

	select_dialog = XuCreateDialog(parent, xmScrolledWindowWidgetClass, "interpSelectDialog",
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &select_dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	label = XmVaCreateManagedLabel(select_dialog, "fieldToInterp",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	rc = XmVaCreateManagedRowColumn(select_dialog, "rc1",
		XmNmarginWidth, 5,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(count = 0, i = 0; i < GV_nfield; i++)
	{
		if(	GV_field[i]->exists && GV_field[i]->link_status == LINKED )
		{
			(void)XmVaCreateManagedToggleButton(rc, GV_field[i]->info->label, NULL);
			count++;
		}
	}
	if( count == 0 ) (void)XmVaCreateManagedLabel(rc, "none", NULL);

	label = XmVaCreateManagedLabel(select_dialog, "labelToInterp",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, rc,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	rc = XmVaCreateManagedRowColumn(select_dialog, "rc2",
		XmNmarginWidth, 5,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(count = 0, i = 0; i < GV_nfield; i++)
	{
		if(	GV_field[i]->exists && GV_field[i]->link_status == FIELD_INTERP )
		{
			(void)XmVaCreateManagedToggleButton(rc, GV_field[i]->info->label, NULL);
			count++;
		}
	}
	if( count == 0 ) (void)XmVaCreateManagedLabel(rc, "none", NULL);

	XuShowDialog(select_dialog);
}


/* =========== Interpolation Progress Display ==================*/


static Widget dialog = NullWidget;
static Widget progressBarLabel, progressBar, totalProgressBar;


/* According to the documentation on the XRT progress widget, it should
*  only be destroyed in a timeout or a work procedure. Why? Who knows,
*  but this conforms to the manual. I maintained ths same procecdure for
*  the normal Motif scale widget version.
*/
/*ARGSUSED*/
static Boolean destroy_progress_displayWP(XtPointer data)
{
	if(NotNull(dialog)) XuDestroyDialog(dialog);
	dialog = NullWidget;
	return True;
}


static void destroy_progress_display(void)
{
	(void) XtAppAddWorkProc(GV_app_context, destroy_progress_displayWP, NULL);
}

/* This function both creates the display if it does not exist and updates
*  it if it does. In this way the display will only appear if Ingred actually
*  sends status data to the interface.
*/
static void update_progress_display( CAL cal, String *parms, int nparms)
{
	FIELD_INFO *fld;
	Widget     label;
	
	if(!same_ic(parms[0], "INTERPOLATE")) return;

	/* If the dialog does not yet exist create it
	*/
	if(!dialog)
	{
		dialog = XuCreateFormDialog(GW_mainWindow, "interpStatus",
			XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			XmNnoResize, True,
			XmNresizePolicy, XmRESIZE_GROW,
			XmNdefaultPosition, True,
			XmNhorizontalSpacing, 15,
			XmNverticalSpacing, 15,
			NULL);

		progressBarLabel = XmVaCreateManagedLabel(dialog, " ",
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		progressBar = XmVaCreateManagedScale(dialog, "pb",
			XmNorientation, XmHORIZONTAL,
			XmNscaleHeight, 16,
			XmNslidingMode, XmTHERMOMETER,
			XmNsliderVisual, XmFOREGROUND_COLOR,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, progressBarLabel,
			XmNtopOffset, 0,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		label = XmVaCreateManagedLabel(dialog, "totalProgressLabel",
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, progressBar,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		totalProgressBar = XmVaCreateManagedScale(dialog, "tpb",
			XmNorientation, XmHORIZONTAL,
			XmNscaleHeight, 16,
			XmNslidingMode, XmTHERMOMETER,
			XmNsliderVisual, XmFOREGROUND_COLOR,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 0,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
		XuShowDialog(dialog);
	}
	/*
	 * Update the display to the new data
	 */
	if((fld = FindField(parms[1], parms[2])))
	{
		XuWidgetLabel(progressBarLabel, fld->info->label);
		XmScaleSetValue(progressBar, atoi(parms[3]));
		XmScaleSetValue(totalProgressBar, atoi(parms[4]));
	}
	/*
	 * Allow the progress bar to update
	 */
	XuDelay(dialog, 10);
}



/* Update the display on a timelink status from ingred.
*/
static void interpolate_panel_update(CAL cal, String *parms, int nparms)
{
	if(!panel_active) return;
	if(!same_ic(parms[0], "TIMELINK")) return;

	display_group_list();
	if(GV_ngroups > 0)
		XuToggleButtonSet(grpBtns[selected_group], True, True);
}

