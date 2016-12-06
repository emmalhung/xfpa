/*========================================================================*/
/*
*	File:		pointFcstEditDialog.c
*
*	Purpose:	Provides a mechanism for editing the point fcst list.
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
#include <unistd.h>
#include <ctype.h>
#include "global.h"
#include <Xm/ArrowB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include "help.h"
#include "productStatus.h"
#include "pointFcst.h"


static Widget dialog = NULL;
static Widget moveUpBtn;
static Widget moveDownBtn;
static Widget deleteBtn;
static Widget entryForm;
static Widget issueTimeEntry;
static Widget labelEntry;
static Widget fcstList;
static Widget latEntry;
static Widget latQuadList[2];
static Widget longEntry;
static Widget longQuadList[2];
static Widget langOptions;
static Widget tzOptions;

/* local functions
*/
static void    AcceptCB					(Widget, XtPointer, XtPointer);
static void    AddToListCB				(Widget, XtPointer, XtPointer);
static Boolean CheckLatData				(void);
static Boolean CheckLongData			(void);
static void    CopySelectListData		(XtPointer, XtIntervalId*);
static Boolean CurrentEntryValidation	(void);
static void    DeleteFromListCB			(Widget, XtPointer, XtPointer);
static void    EditCloseCB				(Widget, XtPointer, XtPointer);
static void    LabelEntryCB				(Widget, XtPointer, XtPointer);
static void    LanguageSetCB			(Widget, XtPointer, XtPointer);
static void    LatEntryCB				(Widget, XtPointer, XtPointer);
static void    LongEntryCB				(Widget, XtPointer, XtPointer);
static void    ListSelectCB				(Widget, XtPointer, XtPointer);
static void    QuadrentCB				(Widget, XtPointer, XtPointer);
static void    SetEditBtnState			(void);
static void    SwapListItemsCB			(Widget, XtPointer, XtPointer);
static void    TimeEntryCB				(Widget, XtPointer, XtPointer);
static void    TimezoneSetCB			(Widget, XtPointer, XtPointer);

extern int pf_nclass;
extern PFCLASS *pf_class;
extern PFCLASS *pf_active_class;
extern String  pf_info_file;

/* local variables
*/
static int selected = 1;
static int active_data = -1;
static int neditData = 0;
static PFDATA **editData = NULL;
static String current_lang_value;
static String current_tz_value;
static float latitude_sign = 1;
static float longitude_sign = -1;
static Boolean add_to_list = False;


void ACTIVATE_pointFcstEditDialog(Widget parent )

{
	int	 i, ac;
	Arg al[18];
	XmString empty_label;
	XtWidgetGeometry size;
	Dimension width, height;
	Widget w, editBtns, addBefore, addAfter, timeFrame, timeManager;
	Widget posnManager, posnFrame;

	static XuDialogActionsStruct action_items[] = {
		{ "okBtn",     AcceptCB,    NULL },
		{ "cancelBtn", XuDestroyDialogCB, NULL },
		{ "helpBtn",   HelpCB,  HELP_POINT_FCST_EDIT }
	};

	if(dialog) return;

	active_data = -1;
	empty_label = XuNewXmString("");

	dialog = XuCreateFormDialog(parent, "pointFcstEdit",
		XmNkeyboardFocusPolicy, XmEXPLICIT,
		XuNdestroyCallback, EditCloseCB,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNfractionBase, 400,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	ac = 0;
	XtSetArg(al[ac], XmNborderWidth, 0); ac++;
	XtSetArg(al[ac], XmNentryAlignment, XmALIGNMENT_CENTER); ac++;
	XtSetArg(al[ac], XmNmarginHeight, 0); ac++;
	XtSetArg(al[ac], XmNmarginWidth, 0); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtraversalOn, False); ac++;
	editBtns = XmCreateRowColumn(dialog, "editBtns", al, ac);

	addBefore = XmCreatePushButton(editBtns, "addBeforeBtn", NULL, 0);
	XtAddCallback(addBefore, XmNactivateCallback, AddToListCB, (XtPointer)0 );
	
	addAfter = XmCreatePushButton(editBtns, "addAfterBtn", NULL, 0);
	XtAddCallback(addAfter, XmNactivateCallback, AddToListCB, (XtPointer)1 );

	moveUpBtn = XmCreatePushButton(editBtns, "moveUpBtn", NULL, 0);
	XtAddCallback(moveUpBtn, XmNactivateCallback, SwapListItemsCB, (XtPointer)-1 );

	moveDownBtn = XmCreatePushButton(editBtns, "moveDownBtn", NULL, 0);
	XtAddCallback(moveDownBtn, XmNactivateCallback, SwapListItemsCB, (XtPointer)1 );

	deleteBtn = XmCreatePushButton(editBtns, "deleteBtn", NULL, 0);
	XtAddCallback(deleteBtn, XmNactivateCallback, DeleteFromListCB, NULL );

	XtManageChild(addBefore);
	XtManageChild(addAfter);
	XtManageChild(moveUpBtn);
	XtManageChild(moveDownBtn);
	XtManageChild(deleteBtn);
	XtManageChild(editBtns);

	ac = 0;
	XtSetArg(al[ac], XmNresizable, False); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	entryForm = XmCreateForm(dialog, "entryForm", al, ac);

	ac = 0;
	XtSetArg(al[ac], XmNresizable, False); ac++;
	XtSetArg(al[ac], XmNscrollBarDisplayPolicy, XmSTATIC); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 20); ac++;
	XtSetArg(al[ac], XmNbottomWidget, entryForm); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, editBtns); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtraversalOn, False); ac++;
	XtSetArg(al[ac], XmNselectionPolicy, XmBROWSE_SELECT); ac++;
	fcstList = XmCreateScrolledList(dialog, "fcstList", al, ac);
	XtManageChild(fcstList);
	XtAddCallback(fcstList, XmNbrowseSelectionCallback, ListSelectCB, NULL );

	ac = 0;
	XtSetArg(al[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	w = XmCreateLabel(entryForm, "labelEntryLabel", al, ac);
	XtManageChild(w);

	ac = 0;
	XtSetArg(al[ac], XmNresizable, False); ac++;
	XtSetArg(al[ac], XmNmaxLength, 250); ac++;
	XtSetArg(al[ac], XmNhighlightOnEnter, True); ac++;
	XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, w); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	labelEntry = XmCreateTextField(entryForm, "labelEntry", al, ac);
	XtManageChild(labelEntry);
	XtAddCallback(labelEntry, XmNmodifyVerifyCallback, LabelEntryCB, NULL);
	XtAddCallback(labelEntry, XmNlosingFocusCallback, LabelEntryCB, NULL);
	XtAddCallback(labelEntry, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);

	ac = 0;
	XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, labelEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 9); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	timeFrame = XmCreateFrame(entryForm, "timeFrame", al, ac);
	XtManageChild(timeFrame);

	ac = 0;
	XtSetArg(al[ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
	w = XmCreateLabel(timeFrame, "timeParmLabel", al, ac);
	XtManageChild(w);

	ac = 0;
	XtSetArg(al[ac], XmNhorizontalSpacing, 9); ac++;
	XtSetArg(al[ac], XmNverticalSpacing, 9); ac++;
	timeManager = XmCreateForm(timeFrame, "timeManager", al, ac);

	ac = 0;
	XtSetArg(al[ac], XmNcolumns, 10); ac++;
	XtSetArg(al[ac], XmNhighlightOnEnter, True); ac++;
	XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	issueTimeEntry = XmCreateTextField(timeManager, "issueTimeEntry", al, ac);
	XtManageChild(issueTimeEntry);
	XtAddCallback(issueTimeEntry, XmNmodifyVerifyCallback, TimeEntryCB , keyissue);
	XtAddCallback(issueTimeEntry, XmNlosingFocusCallback, TimeEntryCB,  keyissue);
	XtAddCallback(issueTimeEntry, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);

	ac = 0;
	XtSetArg(al[ac], XmNalignment, XmALIGNMENT_END); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, issueTimeEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 0); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, issueTimeEntry); ac++;
	XtSetArg(al[ac], XmNrightOffset, 0); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNbottomWidget, issueTimeEntry); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
	w = XmCreateLabel(timeManager, "issueHeader", al, ac);
	XtManageChild(w);

	tzOptions = XuVaMenuBuildOption(timeManager, "tzOptions", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, issueTimeEntry,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < GV_ntimezones; i++)
	{
		(void)XuMenuAddButton(tzOptions, GV_timezone[i].key, GV_timezone[i].label, NoId,
									TimezoneSetCB, (XtPointer)(GV_timezone+i));
	}
	current_tz_value = GV_timezone[0].key;


	XtManageChild(timeManager);

	ac = 0;
	XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, labelEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 9); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNleftWidget, timeFrame); ac++;
	XtSetArg(al[ac], XmNleftOffset, 9); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNbottomWidget, timeFrame); ac++;
	posnFrame = XmCreateFrame(entryForm, "posnFrame", al, ac);
	XtManageChild(posnFrame);

	ac = 0;
	XtSetArg(al[ac], XmNchildType, XmFRAME_TITLE_CHILD); ac++;
	w = XmCreateLabel(posnFrame, "positionLabel", al, ac);
	XtManageChild(w);

	ac = 0;
	XtSetArg(al[ac], XmNhorizontalSpacing, 9); ac++;
	XtSetArg(al[ac], XmNverticalSpacing, 9); ac++;
	posnManager = XmCreateForm(posnFrame, "posnManager", al, ac);

	ac = 0;
	XtSetArg(al[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopOffset, 11); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	latQuadList[0] = XmCreatePushButton(posnManager, "N", al, ac);
	XtAddCallback(latQuadList[0], XmNactivateCallback, QuadrentCB, (XtPointer)'N');

	ac = 0;
	XtSetArg(al[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(al[ac], XmNmappedWhenManaged, False); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopOffset, 11); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	latQuadList[1] = XmCreatePushButton(posnManager, "S", al, ac);
	XtAddCallback(latQuadList[1], XmNactivateCallback, QuadrentCB, (XtPointer)'S');

	XtManageChildren(latQuadList, 2);

	ac = 0;
	XtSetArg(al[ac], XmNcolumns, 12); ac++;
	XtSetArg(al[ac], XmNhighlightOnEnter, True); ac++;
	XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, latQuadList[0]); ac++;
	XtSetArg(al[ac], XmNrightOffset, 2); ac++;
	latEntry = XmCreateTextField(posnManager, "latEntry", al, ac);
	XtManageChild(latEntry);
	XtAddCallback(latEntry, XmNmodifyVerifyCallback, LatEntryCB, NULL);
	XtAddCallback(latEntry, XmNlosingFocusCallback,  LatEntryCB, NULL);
	XtAddCallback(latEntry, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);
	XtManageChild(w);

	ac = 0;
	XtSetArg(al[ac], XmNalignment, XmALIGNMENT_END); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, latEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 0); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, latEntry); ac++;
	XtSetArg(al[ac], XmNrightOffset, 2); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNbottomWidget, latEntry); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
	w = XmCreateLabel(posnManager, "latLabel", al, ac);
	XtManageChild(w);

	ac = 0;
	XtSetArg(al[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, latEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 11); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	longQuadList[0] = XmCreatePushButton(posnManager, "W", al, ac);
	XtAddCallback(longQuadList[0], XmNactivateCallback, QuadrentCB, (XtPointer)'W');

	ac = 0;
	XtSetArg(al[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(al[ac], XmNmappedWhenManaged, False); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, latEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 11); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	longQuadList[1] = XmCreatePushButton(posnManager, "E", al, ac);
	XtAddCallback(longQuadList[1], XmNactivateCallback, QuadrentCB, (XtPointer)'E');

	XtManageChildren(longQuadList, 2);

	ac = 0;
	XtSetArg(al[ac], XmNcolumns, 12); ac++;
	XtSetArg(al[ac], XmNhighlightOnEnter, True); ac++;
	XtSetArg(al[ac], XmNhighlightThickness, 2); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, latEntry); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, longQuadList[0]); ac++;
	XtSetArg(al[ac], XmNrightOffset, 2); ac++;
	longEntry = XmCreateTextField(posnManager, "longEntry", al, ac);
	XtManageChild(longEntry);
	XtAddCallback(longEntry, XmNmodifyVerifyCallback, LongEntryCB, (XtPointer)"long" );
	XtAddCallback(longEntry, XmNlosingFocusCallback,  LongEntryCB, (XtPointer)"long" );
	XtAddCallback(longEntry, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);

	ac = 0;
	XtSetArg(al[ac], XmNalignment, XmALIGNMENT_END); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopWidget, longEntry); ac++;
	XtSetArg(al[ac], XmNtopOffset, 0); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, longEntry); ac++;
	XtSetArg(al[ac], XmNrightOffset, 2); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
	XtSetArg(al[ac], XmNbottomWidget, longEntry); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
	w = XmCreateLabel(posnManager, "longLabel", al, ac);
	XtManageChild(w);

	/* We want the size of the direction selection buttons to be the same.
	*  To do this we set the height to that of the lat and long input text
	*  field widget and the width to the max of the height or the width of
	*  the longest selection button.
	*/
	size.request_mode = CWHeight;
	(void) XtQueryGeometry(latEntry, NULL, &size);
	height = size.height - 4;
	width = height;
	size.request_mode = CWWidth;
	(void) XtQueryGeometry(latQuadList[0], NULL, &size);
	if(size.width > width) width = size.width;
	(void) XtQueryGeometry(latQuadList[1], NULL, &size);
	if(size.width > width) width = size.width;
	(void) XtQueryGeometry(longQuadList[0], NULL, &size);
	if(size.width > width) width = size.width;
	(void) XtQueryGeometry(longQuadList[1], NULL, &size);
	if(size.width > width) width = size.width;
	XtVaSetValues(latQuadList[0], XmNheight, height, XmNwidth, width, NULL);
	XtVaSetValues(latQuadList[1], XmNheight, height, XmNwidth, width, NULL);
	XtVaSetValues(longQuadList[0], XmNheight, height, XmNwidth, width, NULL);
	XtVaSetValues(longQuadList[1], XmNheight, height, XmNwidth, width, NULL);

	XtManageChild(posnManager);


	langOptions = XuVaMenuBuildOption(entryForm, "langOptions", NULL,
		XmNmanageChild, False,
		XmNtraversalOn, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, posnFrame,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < GV_nlanguages; i++)
	{
		(void) XuMenuAddButton(langOptions, GV_language[i].key, GV_language[i].label, NoId,
									 LanguageSetCB, (XtPointer)(GV_language+i));
	}
	current_lang_value = GV_language[0].key;


	/* Map this widget only if we have more than one product language.
	*/
	if(GV_nlanguages > 1) XtManageChild(langOptions);

	XtManageChild(entryForm);

	/* Order the tab groups for a more natural input order
	*/
	XmAddTabGroup(labelEntry);
	XmAddTabGroup(issueTimeEntry);
	XmAddTabGroup(latEntry);
	XmAddTabGroup(longEntry);

	XmStringFree(empty_label);
	XuShowDialog(dialog);
	(void) XtAppAddTimeOut(GV_app_context, 200, CopySelectListData, NULL);
	SetEditBtnState();
}

/*
*	Sets the sensitivity of the edit action buttons according to the
*   position of the item in the list and as to if it is allowed to
*   be modified or not.
*/
static void SetEditBtnState(void)
{
	Boolean ok;

	XtSetSensitive( deleteBtn,   (neditData > 0));
	XtSetSensitive( moveUpBtn,   (selected > 1));
	XtSetSensitive( moveDownBtn, (selected < neditData));
	ok = (active_data >= 0);
	XtSetSensitive( deleteBtn, ok ? editData[active_data]->modifable : False);
	XtSetSensitive( entryForm, ok ? editData[active_data]->modifable : False);
}


/*ARGSUSED */
static void CopySelectListData(XtPointer  client_data , XtIntervalId *id )
{
	int i;
	XmString label;

	XmListSetPos(fcstList, 1);
	XmListDeselectAllItems(fcstList);
	XmListDeleteAllItems(fcstList);
	selected = 1;
	if( (neditData = pf_active_class->ndata) > 0 )
	{
		editData = NewMem(PFDATA*, neditData);
		for( i = 0; i < neditData; i++ )
		{
			editData[i] = OneMem(PFDATA);
			CopyStruct(editData[i], pf_active_class->data[i], PFDATA, 1);
			editData[i]->id = XtNewString(pf_active_class->data[i]->id);
			editData[i]->label = XtNewString(pf_active_class->data[i]->label);
			label = XmStringCreate(editData[i]->label,
				editData[i]->modifable ? NORMAL_FONT : ITALIC_FONT);
			XmListAddItem(fcstList,label,0);
			XmStringFree(label);
		}
		XmListSelectPos(fcstList,1,True);
	}
	else
	{
		AddToListCB(NullWidget,(XtPointer)0,(XtPointer)NULL);
	}
}


/*ARGSUSED*/
static void AddToListCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i;
	XmString label;

	if(!CurrentEntryValidation()) return;

	selected += PTR2INT(client_data);
	active_data = selected - 1;
	XmListDeselectAllItems(fcstList);

	/* Increase the size of the list and move data up the array.
	*/
	neditData++;
	editData = MoreMem(editData, PFDATA*, neditData);
	for( i = neditData-1; i > active_data; i-- )
		editData[i] = editData[i-1];

	/* Create new data entry
	*/
	editData[active_data] = OneMem(PFDATA);
	editData[active_data]->id = NULL;
	editData[active_data]->label = NULL;
	editData[active_data]->class = pf_active_class->id;
	editData[active_data]->nissuetimes = 0;
	editData[active_data]->generate = False;
	editData[active_data]->generating = False;
	editData[active_data]->modifable = True;
	editData[active_data]->language = current_lang_value;
	editData[active_data]->timezone = current_tz_value;
	editData[active_data]->latitude  = (float)MISSING;
	editData[active_data]->longitude = (float)MISSING;

	/* Add to the selection list
	*/
	label = XuNewXmString(dashes);
	XmListAddItem(fcstList,label,selected);
	XmStringFree(label);
	XmListSelectPos(fcstList,selected,False);

	/* Set visuals
	*/
	XmTextFieldSetString( labelEntry,       clear );
	XmTextFieldSetString( latEntry,         clear );
	XmTextFieldSetString( longEntry,        clear );
	XmTextFieldSetString( issueTimeEntry,   clear );
	SetEditBtnState();

	add_to_list = True;
}


/*ARGSUSED*/
static void SwapListItemsCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  pos1, pos2;
	PFDATA *tmp;
	XmString labels[2];

	if(!CurrentEntryValidation()) return;

	XmListDeselectPos(fcstList,selected);
	if( PTR2INT(client_data) > 0 )
	{
		if( selected + 1 > neditData ) return;
		selected++;
		active_data++;
		pos1 = active_data - 1;
		pos2 = active_data;
	}
	else
	{
		if( selected - 1 < 1 ) return;
		selected--;
		active_data--;
		pos1 = active_data;
		pos2 = active_data + 1;
	}

	tmp = editData[pos2];
	editData[pos2] = editData[pos1];
	editData[pos1] = tmp;

	labels[0] = XmStringCreate(
		blank(editData[pos1]->label) ? dashes : editData[pos1]->label,
		editData[pos1]->modifable ? NORMAL_FONT:ITALIC_FONT);
	labels[1] = XmStringCreate(
		(blank(editData[pos2]->label)) ? dashes : editData[pos2]->label,
		editData[pos2]->modifable ? NORMAL_FONT:ITALIC_FONT);
	XmListReplaceItemsPos(fcstList,labels,2,pos2);
	XmStringFree(labels[0]);
	XmStringFree(labels[1]);

	SetEditBtnState();
	XmListSelectPos(fcstList,selected,True);
}


/*ARGSUSED*/
static void DeleteFromListCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i;

	add_to_list = False;
	XmListDeletePos(fcstList, selected );
	pf_FreeData(editData[active_data]);
	neditData--;
	if( neditData > 0 )
	{
		for(i = active_data; i < neditData; i++)
			editData[i] = editData[i+1];
		active_data = -1;
		selected = MAX(1, selected-1);
		XmListSelectPos(fcstList, selected, True);
	}
	else
	{
		XmTextFieldSetString( labelEntry,       clear );
		XmTextFieldSetString( latEntry,         clear );
		XmTextFieldSetString( longEntry,        clear );
		XmTextFieldSetString( issueTimeEntry,   clear );
		active_data = -1;
		selected = 1;
		SetEditBtnState();
	}
}


/*ARGSUSED*/
static void ListSelectCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i;
	XmListCallbackStruct *rtrn;

	if(active_data >= 0)
	{
		if(!CurrentEntryValidation())
		{
			XmListSelectPos(fcstList, active_data+1, False);
			return;
		}
	}

	rtrn = (XmListCallbackStruct *)call_data;
	selected = rtrn->item_position;
	active_data = selected - 1;

	XmTextFieldSetString( labelEntry, editData[active_data]->label );
	XmTextFieldSetString( latEntry, pf_FmtData(editData[active_data], keylat) );
	XmTextFieldSetString( longEntry, pf_FmtData(editData[active_data], keylong));
	XmTextFieldSetString( issueTimeEntry, pf_FmtData(editData[active_data], keyissue));

	latitude_sign = 1;
	if(editData[active_data]->latitude < 0) latitude_sign = -1;
	XtMapWidget(latQuadList[(latitude_sign < 0)? 1:0]);
	XtUnmapWidget(latQuadList[(latitude_sign < 0)? 0:1]);

	longitude_sign = -1;
	if(editData[active_data]->longitude > 0) longitude_sign = 1;
	XtMapWidget(longQuadList[(longitude_sign < 0)? 0:1]);
	XtUnmapWidget(longQuadList[(longitude_sign < 0)? 1:0]);

	for( i=0; i < GV_ntimezones; i++ ) {
		if(!same(editData[active_data]->timezone,GV_timezone[i].key)) continue;
		current_tz_value = GV_timezone[i].key;
		XuMenuSelectItemByName(tzOptions, GV_timezone[i].key);
		break;
	}

	for( i=0; i < GV_nlanguages; i++ ) {
		if(!same(editData[active_data]->language,GV_language[i].key)) continue;
		current_lang_value = GV_language[i].key;
		XuMenuSelectItemByName(langOptions, GV_language[i].key);
		break;
	}
	SetEditBtnState();
	(void) XmProcessTraversal(labelEntry,XmTRAVERSE_CURRENT);
}


/* This was done as a delay as anything else would not work!
*/
/*ARGSUSED*/
static void ResetTabGroup(XtPointer data , XtIntervalId *id )
{
	Widget w = (Widget)data;
	(void) XmProcessTraversal(w, XmTRAVERSE_CURRENT);
}


/*
*	Function to verify that no non-printing characters make it into the
*   label and to copy the resulting label into the forecast list when
*   the text widget loses focus.
*/
/*ARGSUSED*/
static void LabelEntryCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, len;
	XmString label[1];
	XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)call_data;

	if(cbs->reason == XmCR_LOSING_FOCUS)
	{
		FreeItem(editData[active_data]->label);
		editData[active_data]->label = XmTextFieldGetString(labelEntry);
		if(blank(editData[active_data]->label)) return;

		XmListDeselectPos(fcstList, selected);
		label[0] = XmStringCreate(editData[active_data]->label,
			editData[active_data]->modifable ? NORMAL_FONT : ITALIC_FONT);
		XmListReplaceItemsPosUnselected(fcstList, label, 1, selected);
		XmStringFree(label[0]);
		XmListSelectPos(fcstList, selected, False);
	}
	else if( cbs->reason == XmCR_MODIFYING_TEXT_VALUE )
	{
		if(IsNull(cbs->text->ptr) || *cbs->text->ptr == 0 ) return;
		for(len = 0; len < cbs->text->length; len++)
		{
			if( isprint((int)cbs->text->ptr[len]) ) continue;
			for( i = len; (i+1) < cbs->text->length; i++ )
				cbs->text->ptr[i] = cbs->text->ptr[i+1];
			cbs->text->length--;
			len--;
		}
		if( cbs->text->length <= 0 ) cbs->doit = False;
	}
}


/*ARGSUSED*/
static void LatEntryCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i, len;
	XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct * )call_data;

	if(cbs->reason == XmCR_LOSING_FOCUS)
	{
		if(CheckLatData())
			XmTextFieldSetString(w, pf_FmtData(editData[active_data], keylat));
	}
	else if( cbs->reason == XmCR_MODIFYING_TEXT_VALUE )
	{
		if(IsNull(cbs->text->ptr) || *cbs->text->ptr == 0 ) return;
		for( len = 0; len < cbs->text->length; len++ )
		{
			if( isdigit(cbs->text->ptr[len]) ) continue;
			if( strchr(" ³\'\"",cbs->text->ptr[len]) != NULL ) continue;
			for( i = len; (i+1) < cbs->text->length; i++ )
				cbs->text->ptr[i] = cbs->text->ptr[i+1];
			cbs->text->length--;
			len--;
		}
		if( cbs->text->length <= 0 ) cbs->doit = False;
	}
}


/*ARGSUSED*/
static void LongEntryCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i, len;
	XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct * )call_data;

	if(cbs->reason == XmCR_LOSING_FOCUS)
	{
		if(CheckLongData())
			XmTextFieldSetString(w, pf_FmtData(editData[active_data], keylong));
	}
	else if( cbs->reason == XmCR_MODIFYING_TEXT_VALUE )
	{
		if(IsNull(cbs->text->ptr) || *cbs->text->ptr == 0 ) return;
		for( len = 0; len < cbs->text->length; len++ )
		{
			if( isdigit(cbs->text->ptr[len]) ) continue;
			if( strchr(" ³\'\"",cbs->text->ptr[len]) != NULL ) continue;
			for( i = len; (i+1) < cbs->text->length; i++ )
				cbs->text->ptr[i] = cbs->text->ptr[i+1];
			cbs->text->length--;
			len--;
		}
		if( cbs->text->length <= 0 ) cbs->doit = False;
	}
}


static Boolean CheckLatData(void)

{
	float lat, float_arg();
	String text, ptr;
	Boolean ok;

	text = XmTextFieldGetString(latEntry);
	if((ok = !blank(text)))
	{
		while((ptr = strpbrk(text, degree_symbols))) *ptr = ' ';
		lat = float_arg(text, &ok);
		lat += float_arg(text, &ok)/60;
		lat += float_arg(text, &ok)/3600;
		ok = (lat >= 0.0 && lat <= 90.0);
		if(ok) editData[active_data]->latitude = lat*latitude_sign;
	}
	FreeItem(text);
	return ok;
}


static Boolean CheckLongData(void)

{
	float lon, float_arg();
	String text, ptr;
	Boolean ok;

	text = XmTextFieldGetString(longEntry);
	if((ok = !blank(text)))
	{
		while((ptr = strpbrk(text, degree_symbols))) *ptr = ' ';
		lon = float_arg(text, &ok);
		lon += float_arg(text, &ok)/60;
		lon += float_arg(text, &ok)/3600;
		ok = (lon >= 0.0 && lon <= 180.0);
		if(ok) editData[active_data]->longitude = lon*longitude_sign;
	}
	FreeItem(text);
	return ok;
}


/* Carefull here! When the client_data entry is say 'W' this means that
*  the west button was pushed and we are going to east!
*/
/*ARGSUSED*/
static void QuadrentCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2BOOL(client_data))
	{
		case 'N':
			XtMapWidget(latQuadList[1]);
			XtUnmapWidget(latQuadList[0]);
			latitude_sign = -1;
			break;
		case 'S':
			XtMapWidget(latQuadList[0]);
			XtUnmapWidget(latQuadList[1]);
			latitude_sign = 1;
			break;
		case 'E':
			XtMapWidget(longQuadList[0]);
			XtUnmapWidget(longQuadList[1]);
			longitude_sign = -1;
			break;
		case 'W':
			XtMapWidget(longQuadList[1]);
			XtUnmapWidget(longQuadList[0]);
			longitude_sign = 1;
			break;
	}
	editData[active_data]->latitude =
		(float)copysign((double)editData[active_data]->latitude, (double)latitude_sign);
	editData[active_data]->longitude =
		(float)copysign((double)editData[active_data]->longitude, (double)longitude_sign);
}


/*ARGSUSED*/
static void TimeEntryCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i, len;
	XmTextVerifyCallbackStruct *cbs;

	cbs = (XmTextVerifyCallbackStruct * )call_data;
	if( cbs->reason == XmCR_MODIFYING_TEXT_VALUE )
	{
		if(IsNull(cbs->text->ptr) || *cbs->text->ptr == 0 ) return;	/* backspace or null */

		/* We allow only digits and commas
		*/
		for( len = 0; len < cbs->text->length; len++ )
		{
			if( isdigit(cbs->text->ptr[len]) ) continue;
			if( same((String)client_data,keyissue) && cbs->text->ptr[len] == ',' ) continue;
			/* not valid - move all chars down one and decrememt length
			*/
			for( i = len; (i+1) < cbs->text->length; i++ )
				cbs->text->ptr[i] = cbs->text->ptr[i+1];
			cbs->text->length--;
			len--;
		}
		if( cbs->text->length <= 0 ) cbs->doit = False;
	}
}


/*ARGSUSED*/
static void TimezoneSetCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	KEYINFO *rtn = (KEYINFO *)client_data;
	current_tz_value = rtn->key;
	editData[active_data]->timezone = current_tz_value;
}


/*ARGSUSED*/
static void LanguageSetCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	KEYINFO *rtn = (KEYINFO *)client_data;
	current_lang_value = rtn->key;
	editData[active_data]->language = current_lang_value;
}


static Boolean CurrentEntryValidation(void)

{
	String text;
	XmString label[1];

	if(IsNull(editData) || neditData < 1 || active_data < 0) return True;

	FreeItem(editData[active_data]->label);
	editData[active_data]->label = XmTextFieldGetString(labelEntry);
	if(add_to_list && !blank(editData[active_data]->label))
	{
		label[0] = XmStringCreate(editData[active_data]->label,
			editData[active_data]->modifable ? NORMAL_FONT : ITALIC_FONT);
		XmListReplaceItemsPosUnselected(fcstList, label, 1, selected);
		XmStringFree(label[0]);
		add_to_list = False;
	}
	text = XmTextFieldGetString(issueTimeEntry);
	pf_GetFmtData(editData[active_data], text, keyissue);
	FreeItem(text);

	if(!CheckLatData())
	{
		XuShowError(dialog, "LatLimit", NULL);
		(void) XtAppAddTimeOut(GV_app_context, 0, ResetTabGroup, (XtPointer)latEntry);
		return False;
	}
	if(!CheckLongData())
	{
		XuShowError(dialog, "LongLimit", NULL);
		(void) XtAppAddTimeOut(GV_app_context, 0, ResetTabGroup, (XtPointer)longEntry);
		return False;
	}
	if(	editData[active_data]->latitude  != (float)MISSING &&
		editData[active_data]->longitude != (float)MISSING    )
	{
		if(!IsPointInsideMap(editData[active_data]->latitude, editData[active_data]->longitude))
		{
			XuShowError(dialog, "OutsideMap", NULL);
			(void) XtAppAddTimeOut(GV_app_context, 0, ResetTabGroup, (XtPointer)latEntry);
			return False;
		}
	}
	if(blank(editData[active_data]->label))
	{
		XuShowError(dialog, "MissingPointFcstLabel", NULL);
		(void) XtAppAddTimeOut(GV_app_context, 0, ResetTabGroup, (XtPointer)labelEntry);
		return False;
	}
	return True;
}


/*ARGSUSED*/
static void AcceptCB(Widget w , XtPointer notused , XtPointer unused )
{
	int i, j;
	char *ptr, mbuf[256];
	Boolean found;

	if(!CurrentEntryValidation()) return;

	/* If a forecast has been removed from the currently active list
	*  then we must remove the associated forecast files and the
	*  entry in the process status list as well.
	*/
	for( i = 0; i < pf_active_class->ndata; i++ )
	{
		found = False;
		for( j = 0; j < neditData; j++ )
		{
			if(same(pf_active_class->data[i]->label, editData[j]->label) &&
			   pf_active_class->data[i]->language[0] == editData[j]->language[0])
			{
				found = True;
				break;
			}
		}
		if (!found)
		{
			strcpy(mbuf, pf_active_class->data[i]->label);
			strcat(mbuf, ".*");
			while((ptr = strchr(mbuf, ' '))) *ptr = '_';
			ptr = get_file(FCST_WORK,mbuf);
			if (ptr) (void)unlink(ptr);
			ptr = get_file(FCST_RELEASE,mbuf);
			if (ptr) (void)unlink(ptr);
		}
	}

	/* Now destroy the currently active forecast list.
	*/
	for( i = 0; i < pf_active_class->ndata; i++ )
	{
		ProductStatusRemoveInfo(pf_active_class->data[i]->pid);
		pf_FreeData(pf_active_class->data[i]);
	}
	FreeItem(pf_active_class->data);
	pf_active_class->ndata = 0;

	/* Create a new list from the editor list.
	*/
	pf_active_class->data = NewMem(PFDATA*, neditData);
	for( i = 0; i < neditData; i++ )
	{
		pf_active_class->data[pf_active_class->ndata] = editData[i];
		pf_MakeDataId(mbuf, pf_active_class);
		FreeItem(editData[i]->id);
		editData[i]->id = XtNewString(mbuf);
		editData[i]->pid = ProductStatusAddInfo(PS_POINT_FCST, editData[i]->label, pf_IsReleased);
		pf_active_class->ndata++;
	}
	if(pf_active_class->selected > pf_active_class->ndata)
		pf_active_class->selected = 1;

	pf_MakeClassSelection();
	pf_WriteInfoData();

	/* This zero is important to ensure that the EditCloseCB below does not try
	*  and destroy the data and mess things up.
	*/
	neditData = 0;
	XuDestroyDialog(dialog);
}


/*ARGSUSED*/
static void EditCloseCB(Widget w , XtPointer notused , XtPointer unused )
{
	int i;

	for( i = 0; i < neditData; i++ )
	{
		pf_FreeData(editData[i]);
	}
	neditData = 0;
	FreeItem(editData);
	dialog = NULL;
}
