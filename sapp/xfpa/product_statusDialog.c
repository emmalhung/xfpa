/*========================================================================*/
/*
*	File:		product_statusDialog.c
*
*   Functions:  ACTIVATE_productStatusDialog()
*
*	Purpose:	Displays the status of all dependent product and model
*               product. 
*
*   Note:       When a product is running the foreground and background
*               colours of the labels are changed to indicate this. These
*               are set in the resource file as productRunningIndicatorFg
*               and productRunningIndicatorBg.
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
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include "resourceDefines.h"
#include "help.h"
#define  PS_MAIN
#include "productStatus.h"		/* Note: prod_data[] is defined here */

#define AUTO_UPDATE_INTERVAL 30000

static Widget dialog = NULL;
static Widget genTimeList[XtNumber(prod_data)];
static Widget releaseList[XtNumber(prod_data)];
static Pixel frgnd, bkgnd, runfg, runbg;
static XtIntervalId timeout_id;

/*======================= Private Functions ================================*/

/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer unused , XtPointer notused )
{
	XtRemoveTimeOut(timeout_id);
	dialog = NULL;
}


/* This function is called to update the product status. This is
*  for the case where several FPA's are running on the same database
*  as is the case in the Arctic Weather Centre.
*/
/*ARGSUSED*/
static void status_update(XtPointer unused , XtIntervalId *id )
{
	int i, n, nproduct;
	PRODUCT_INFO *product;

	for(n = 0; n < XtNumber(prod_data); n++)
	{
		ProductStatusGetInfo(n, &nproduct, &product);
		for(i = 0; i < nproduct; i++)
			ProductStatusDialogUpdate(&product[i]);
		FreeItem(product);
	}
	timeout_id = XtAppAddTimeOut(GV_app_context, AUTO_UPDATE_INTERVAL, status_update, NULL);
}


/*ARGSUSED*/
static void show_legend_cb(Widget w , XtPointer unused , XtPointer notused )
{
	static Widget legendDialog;
	static XuDialogActionsStruct action_items[] = {
		{ "closeBtn",  XuDestroyDialogCB, NULL }
	};

	if (legendDialog) return;

	legendDialog = XuCreateFormDialog(w, "productStatusLegend",
		XmNdialogStyle, XmDIALOG_MODELESS,
		XmNnoResize, True,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &legendDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		NULL);

	(void) XtVaCreateManagedWidget("info",
		xmLabelWidgetClass, legendDialog,
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNmarginHeight, 6,
		XmNmarginWidth, 6,
		NULL);

	XuShowDialog(legendDialog);
}


/*======================= Public Functions ================================*/


void ACTIVATE_productStatusDialog(Widget w )
{
	int i, n, last, nproduct, count;
	char mbuf[128];
	Boolean top;
	PRODUCT_INFO *product;
	XmString label;
	Widget subArea, header, underline, nameList, dashList1, dashList2;

	static XuDialogActionsStruct action_items[] = {
		{ "legendBtn", show_legend_cb, NULL },
		{ "closeBtn",  XuDestroyDialogCB, NULL },
		{ "helpBtn",   HelpCB, HELP_PRODUCT_STATUS }
	};

	if (dialog) return;


	dialog = XuCreateDialog(w, xmScrolledWindowWidgetClass, "productStatus",
		XmNdialogStyle, XmDIALOG_MODELESS,
		XuNdestroyCallback, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 10,
		XmNverticalSpacing, 10,
		NULL);

	runfg = XuLoadColorResource(dialog, RNproductRunningIndicatorFg, "Black");
	runbg = XuLoadColorResource(dialog, RNproductRunningIndicatorBg, "Green");

	XtVaGetValues(dialog,
		XmNforeground, &frgnd,
		XmNbackground, &bkgnd,
		NULL);

	last = -1;
	count = 0;
	subArea = NULL;
	for(n = 0; n < XtNumber(prod_data); n++)
	{
		ProductStatusGetInfo(n, &nproduct, &product);
		if(nproduct < 1) continue;

		top = (count%3 == 0 || last < 0);
		if(top)
		{
			subArea = XtVaCreateWidget("subArea",
				xmFormWidgetClass, dialog,
				XmNtopAttachment, XmATTACH_FORM,
				XmNleftAttachment, (count == 0)? XmATTACH_FORM:XmATTACH_WIDGET,
				XmNleftOffset, (count == 0)? 10:40,
				XmNleftWidget, subArea,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
		}
		count++;

		label = XuGetXmLabel(prod_data[n].label);
		header = XtVaCreateManagedWidget("header",
			xmLabelWidgetClass, subArea,
			XmNlabelString, label,
			XmNtopAttachment, top? XmATTACH_FORM:XmATTACH_WIDGET,
			XmNtopOffset, top? 10:30,
			XmNtopWidget, nameList,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);
		XmStringFree(label);
		last = n;

		underline = XtVaCreateManagedWidget("underline",
			xmSeparatorWidgetClass, subArea,
			XmNseparatorType, XmSINGLE_LINE,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, header,
			XmNtopOffset, 0,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, header,
			XmNleftOffset, 0,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, header,
			XmNrightOffset, 0,
			NULL);

		nameList = XtVaCreateWidget("nameList",
			xmRowColumnWidgetClass, subArea,
			XmNentryAlignment, XmALIGNMENT_BEGINNING,
			XmNspacing, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, underline,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		dashList1 = XtVaCreateWidget("genTimeList",
			xmRowColumnWidgetClass, subArea,
			XmNentryAlignment, XmALIGNMENT_CENTER,
			XmNspacing, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, underline,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, nameList,
			NULL);

		genTimeList[n] = XtVaCreateWidget("genTimeList",
			xmRowColumnWidgetClass, subArea,
			XmNentryAlignment, XmALIGNMENT_BEGINNING,
			XmNspacing, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, underline,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, dashList1,
			NULL);

		dashList2 = XtVaCreateWidget("releaseList",
			xmRowColumnWidgetClass, subArea,
			XmNentryAlignment, XmALIGNMENT_CENTER,
			XmNspacing, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, underline,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, genTimeList[n],
			NULL);

		releaseList[n] = XtVaCreateWidget("releaseList",
			xmRowColumnWidgetClass, subArea,
			XmNentryAlignment, XmALIGNMENT_BEGINNING,
			XmNspacing, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, underline,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, dashList2,
			NULL);

		for(i = 0; i < nproduct; i++)
		{
			label = XuNewXmString(product[i].label);
			(void) snprintf(mbuf, sizeof(mbuf), "name%d", product[i].id);
			(void) XtVaCreateManagedWidget(mbuf,
				xmLabelWidgetClass, nameList,
				XmNlabelString, label,
				NULL);
			XmStringFree(label);

			(void) XtVaCreateManagedWidget("-",
				xmLabelWidgetClass, dashList1,
				NULL);

			(void) snprintf(mbuf, sizeof(mbuf), "run%d", product[i].id);
			(void) XtVaCreateManagedWidget(mbuf,
				xmLabelWidgetClass, genTimeList[n],
				NULL);

			if(prod_data[n].has_release_status)
			{
				(void) XtVaCreateManagedWidget("-",
					xmLabelWidgetClass, dashList2,
					NULL);

				(void) snprintf(mbuf, sizeof(mbuf), "rel%d", product[i].id);
				(void) XtVaCreateManagedWidget(mbuf,
					xmLabelWidgetClass, releaseList[n],
					NULL);
			}
			ProductStatusDialogUpdate(&product[i]);
		}
		FreeItem(product);
		XtManageChild(nameList);
		XtManageChild(dashList1);
		XtManageChild(genTimeList[n]);
		XtManageChild(dashList2);
		XtManageChild(releaseList[n]);
		if((count+1)%3 == 0) XtManageChild(subArea);
	}
	if (subArea) XtManageChild(subArea);
	XuShowDialog(dialog);
	timeout_id = XtAppAddTimeOut(GV_app_context, AUTO_UPDATE_INTERVAL, status_update, NULL);
}


void UpdateProductDialogFromId(int id )
{
	int i, n, nproduct;
	PRODUCT_INFO *product;

	for(n = 0; n < XtNumber(prod_data); n++)
	{
		ProductStatusGetInfo(n, &nproduct, &product);
		for(i = 0; i < nproduct; i++)
		{
			if(product[i].id != id) continue;
			ProductStatusDialogUpdate(&product[i]);
			break;
		}
		FreeItem(product);
		if(i < nproduct) break;
	}
}


void ProductStatusDialogUpdate(PRODUCT_INFO *product )
{
	long dt;
	char mbuf[128];
	Boolean rel;
	XmString label;

	if(!dialog) return;

	dt = ProductStatusGetGenerateTime(product->id);
	if(dt > 0)
	{
		label = XuNewXmStringFmt(UnixSecToMinuteDateFormat(dt));
	}
	else
		label = XuNewXmString(product->statinfo);

	(void) snprintf(mbuf, sizeof(mbuf), "run%d", product->id);
	XtVaSetValues(XtNameToWidget(genTimeList[product->type], mbuf),
		XmNlabelString, label,
		XmNforeground, product->is_running ? runfg:frgnd,
		XmNbackground, product->is_running ? runbg:bkgnd,
		NULL);
	XmStringFree(label);

	if(prod_data[product->type].has_release_status)
	{
		rel = False;
		(void) strcpy(mbuf, " ");
		if(product->release_fcn && !product->is_running)
		{
			if((rel = ProductStatusReleaseCheck(product->id)))
				(void) strcpy(mbuf, XuGetLabel("released"));
			else
				(void) strcpy(mbuf, XuGetLabel("unreleased"));
		}
		label = XuNewXmString(mbuf);
		(void) snprintf(mbuf, sizeof(mbuf), "rel%d", product->id);
		XtVaSetValues(XtNameToWidget(releaseList[product->type], mbuf),
			XmNlabelString, label,
			XmNforeground, rel? runfg:frgnd,
			XmNbackground, rel? runbg:bkgnd,
			NULL);
		XmStringFree(label);
	}

	XuUpdateDisplay(dialog);
}
