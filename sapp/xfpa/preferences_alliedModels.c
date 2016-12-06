/*========================================================================*/
/*
*	   File:	preferences_alliedModels.c
*
*	Purpose:	Model options setting. 
*
*     Notes:	This is used in conjunction with the preferencesDialog.
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
#include <Xm/BulletinB.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include "alliedModel.h"
#include "preferences.h"
#include "resourceDefines.h"

#define IMPORT_ASK_KEY	"atiam"

static Widget board;
static Widget askbtn;


/*========================= LOCAL FUNCTIONS ============================*/

/*ARGSUSED*/
static void select_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	int n = PTR2INT(client_data);
	char mbuf[10];
	static int last_showing = 0;

	(void) snprintf(mbuf, sizeof(mbuf), "%d", n);
	XtSetMappedWhenManaged(XtNameToWidget(board, mbuf), True);
	(void) snprintf(mbuf, sizeof(mbuf), "%d", last_showing);
	XtSetMappedWhenManaged(XtNameToWidget(board, mbuf), False);
	last_showing = n;
}


/*============================= PUBLIC FUNCTIONS =======================*/


void InitAlliedModelOptions(void)
{
	int val;
	GV_pref.ask_to_import_allied_models = XuGetBooleanResource(RNaskBeforeImportingAllied, False);
	if(XuVaStateDataGet(ALLIED_MODEL_STATE_KEY,IMPORT_ASK_KEY,NULL, "%d", &val))
		GV_pref.ask_to_import_allied_models = (Boolean) val;
}


void AlliedModelOptions(Widget parent )
{
	int i, n;
	Dimension max_width, spacing, width;
	char mbuf[10];
	Widget sb, hsb, label, sw1, sw2, list, btn, fieldManager;
	XmString xmlabel;
	XtWidgetGeometry size;

	askbtn = XmVaCreateManagedToggleButton(parent, "alliedModelAskToImport",
		XmNset, GV_pref.ask_to_import_allied_models,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
		NULL);

	label = XmVaCreateManagedLabel(parent, "alliedModelLabel",
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, askbtn,
        XmNleftAttachment, XmATTACH_FORM,
		NULL);

	sw1 = XmVaCreateScrolledWindow(parent, "alliedModelList",
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNscrollBarDisplayPolicy, XmSTATIC,
        XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, label, XmNtopOffset, 0,
        XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	list = XmVaCreateManagedRowColumn(sw1, "modelBtns",
		XmNradioBehavior, True,
		NULL);

	label = XmVaCreateManagedLabel(parent, "alliedModelFieldLabel",
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, askbtn,
        XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget, sw1,
		NULL);

	sw2 = XmVaCreateScrolledWindow(parent, "sw2",
		XmNscrollingPolicy, XmAUTOMATIC,
        XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, label, XmNtopOffset, 0,
        XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget, sw1,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	board = XmVaCreateBulletinBoard(sw2, "modelBtns",
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		NULL);

	size.request_mode = CWWidth;
	max_width = 50;
	for(i = 0; i < GV_nallied_model; i++)
	{
		xmlabel = XuNewXmString(SrcLabel(GV_allied_model[i].source));
		btn = XmVaCreateManagedToggleButton(list, "tbtn",
			XmNset, (i==0)? XmSET:XmUNSET,
			XmNlabelString, xmlabel,
			NULL);
		XmStringFree(xmlabel);
		XtAddCallback(btn, XmNvalueChangedCallback, select_cb, INT2PTR(i));

		(void) snprintf(mbuf, sizeof(mbuf), "%d", i);
		fieldManager = XmVaCreateRowColumn(board, mbuf,
			XmNmappedWhenManaged, (i==0),
			NULL);

		if(IsNull(GV_allied_model[i].source->fd->sdef->allied->metafiles))
		{
			(void)XmCreateLabel(fieldManager, XuGetLabel("no_metafiles"), NULL, 0);
		}
		else
		{
			for(n = 0; n < GV_allied_model[i].source->fd->sdef->allied->metafiles->nfiles; n++)
			{
				xmlabel = XuNewXmString(GV_allied_model[i].source->fd->sdef->allied->metafiles->flds[n]->label);
				(void) snprintf(mbuf, sizeof(mbuf), "%d", n);
				btn = XmVaCreateManagedToggleButton(fieldManager, mbuf,
					XmNlabelString, xmlabel,
					XmNset, (GV_allied_model[i].import[n]) ? XmSET:XmUNSET,
					NULL);
				XmStringFree(xmlabel);
			}
		}
		XtManageChild(fieldManager);
		(void) XtQueryGeometry(fieldManager, NULL, &size);
		max_width = MAX(max_width, size.width);
	}
	XtManageChild(board);

	XtVaGetValues(sw1,
		XmNspacing, &spacing,
		XmNverticalScrollBar, &sb,
		XmNhorizontalScrollBar, &hsb,
		NULL);
	XtUnmanageChild(hsb);
	XtVaGetValues(sb, XmNwidth, &width, NULL);
	(void) XtQueryGeometry(list, NULL, &size);
	XtVaSetValues(sw1, XmNwidth, size.width+spacing+width+10, NULL);
	XtManageChild(sw1);

	XtVaGetValues(sw2,
		XmNspacing, &spacing,
		XmNverticalScrollBar, &sb,
		NULL);
	width = 0;
	if(XtIsManaged(sb)) XtVaGetValues(sb, XmNwidth, &width, NULL);
	(void) XtQueryGeometry(board, NULL, &size);
	XtVaSetValues(sw2, XmNwidth, max_width+spacing+10, NULL);
	XtManageChild(sw2);
}


void SetAlliedModelOptions(void)
{
	int i, j, nmeta;
	char mbuf[128], nbuf[128];
	Widget rc, btn;

	GV_pref.ask_to_import_allied_models = XmToggleButtonGetState(askbtn);
	XuVaStateDataSave(ALLIED_MODEL_STATE_KEY,IMPORT_ASK_KEY,NULL, "%d", (int) GV_pref.ask_to_import_allied_models);

	for(i = 0; i < GV_nallied_model; i++)
	{
		GV_allied_model[i].automatic_import = False;
		(void) snprintf(nbuf, sizeof(nbuf), "%d", i);
		rc = XtNameToWidget(board, nbuf);
		(void) strcpy(mbuf, "");
		nmeta = 0;
		if ( GV_allied_model[i].source->fd->sdef->allied &&
				GV_allied_model[i].source->fd->sdef->allied->metafiles)
			nmeta = GV_allied_model[i].source->fd->sdef->allied->metafiles->nfiles;
		for(j = 0; j < nmeta; j++)
		{
			(void) snprintf(nbuf, sizeof(nbuf), "%d", j);
			btn = XtNameToWidget(rc, nbuf);
			GV_allied_model[i].import[j] = XmToggleButtonGetState(btn);
			if(GV_allied_model[i].import[j]) GV_allied_model[i].automatic_import = True;
			(void) strcat(mbuf, (GV_allied_model[i].import[j])? "1 ":"0 ");
		}
		XuStateDataSave(ALLIED_MODEL_STATE_KEY, SrcName(GV_allied_model[i].source),
			SrcSubName(GV_allied_model[i].source), mbuf);
	}
}

