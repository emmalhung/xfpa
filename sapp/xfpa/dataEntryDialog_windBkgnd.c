/*========================================================================*/
/*
*	File:		dataEntryDialog_windBkgnd.c
*
*	Purpose:	Sets the background wind for all of the depictions in the
*               existing sequence.
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
#include <string.h>
#include "global.h"
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/Column.h>
#include "depiction.h"
#include "help.h"
#include "resourceDefines.h"
#include "wind.h"

/* Set the maximum number of wind models that we can handle.*/
#define NMODELS 30

static void angle_delta_cb	(Widget, XtPointer, XtPointer);
static void display_data	(void);
static void enter_cb		(Widget, XtPointer, XtPointer);
static void exit_cb			(Widget, XtPointer, XtPointer);
static void model_cb		(Widget, XtPointer, XtPointer);
static void speed_delta_cb	(Widget, XtPointer, XtPointer);
static void gust_delta_cb	(Widget, XtPointer, XtPointer);

static String    module       = "bkgndWindEntryDialog";
static Widget    dialog       = NullWidget;
static Widget    entryDisplay = NullWidget;
static WIND_CALC wind;



/*=========================================================================*/
/*
*	ACTIVATE_bkgndWindEntryDialog() - Popup the dialog.  The
*	position at which to place the dialog is given by x and y.
*/
/*=========================================================================*/
void ACTIVATE_bkgndWindEntryDialog( Widget refw , WINDFCN rtnFcn )
{
	int i, j, nwcref, selnum;
	char mbuf[20];
	String mfp, menu_file, name;
	Widget w, label, baseModel, btn, selbtn, rc, aligner;
	INFOFILE     ifd;
	FpaConfigWindEditorStruct *wptr;
	FpaConfigCrossRefStruct   *wcref[NMODELS];

	static String         label_vals[]     = { "angle","speed","gusts" };
	static XtCallbackProc entry_callback[] = { angle_delta_cb, speed_delta_cb, gust_delta_cb };
	static int            btn_vals[]       = { -10, -5, 5, 10 };

	static XuDialogActionsStruct action_items[] = {
		{ "setBtn",    enter_cb,           NULL            },
		{ "cancelBtn", XuDestroyDialogCB,  NULL            },
		{ "helpBtn",   HelpCB,             HELP_BKGND_WIND }
	};

	static String bwed = "background wind entry dialog";

	if (dialog) return;

	 wptr = GV_active_field->info->element->elem_detail->editor->type.wind;

     /* Make sure that we have the wind entry menu definition file and that it
     *  contains the data block that we need.
	 *  
     */
	if(blank(wptr->back_entry_file))
	{
		pr_error(module, "%s menu file not defined in configuration files\n", bwed);
		return;
	}

	if(IsNull(mfp = get_file(MENUS_CFG,wptr->back_entry_file)))
	{
		pr_error(module, "%s menu file \"%s\" not found\n", bwed, wptr->back_entry_file);
		return;
	}
	menu_file = XtNewString(mfp);

    if(IsNull(ifd = info_file_open(menu_file)))
    {
        pr_error(module, "Unable to open %s menu file \"%s\"\n", bwed, menu_file);
		FreeItem(menu_file);
        return;
    }

    if(!info_file_find_block(ifd, "models"))
    {
        pr_error(module, "No \"models\" block in %s menu file \"%s\"\n", bwed, menu_file);
		FreeItem(menu_file);
        return;
    }

	/* Make sure that we have at least one recognized wind model.
	 */
	nwcref = 0;
	selnum = 0;
	selbtn = NullWidget;

	while(!(blank(name = info_file_get_next_line(ifd))) && nwcref < NMODELS)
	{
		wcref[nwcref] = identify_crossref(FpaCcRefsWinds, name);
		if (NotNull(wcref[nwcref]))
		{
			if(same(wind.model,name)) selnum = nwcref;
			nwcref++;
		}
		else
		{
			 pr_error(module, "Unknown model \"%s\" in %s menu file \"%s\"\n",
				 name, bwed, menu_file);
		}
	 }
	info_file_close(ifd);

	if(!nwcref)
	{
		pr_error(module, "No valid wind models found in menu file \"%s\"\n", menu_file);
		pr_error(module, "Background wind entry dialog terminated.\n");
		FreeItem(menu_file);
		return;
	}

	action_items[0].data = (XtPointer)rtnFcn;

	dialog = XuCreateFormDialog(refw, "bkgndWindEntry",
		XmNnoResize, True,
		XuNdestroyCallback, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	label = XmVaCreateManagedLabel(dialog, "baseModelSelectLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	baseModel = XmVaCreateManagedRowColumn(dialog, "baseModel",
		XmNborderWidth, 1,
		XmNmarginWidth, 9,
		XmNmarginHeight, 9,
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 0,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	wptr = GV_active_field->info->element->elem_detail->editor->type.wind;

	/* The background wind CAL parse should only fail if a background has
	*  not been set previously. Set some defaults.
	*/
	if(!parse_wind_attribs(GV_active_field->bkgnd_cal, &wind))
	{
		wind.model     = NULL;
		wind.rel_dir   = True;
		wind.rel_speed = True;
		wind.dir       = -30;
		wind.speed     = 70;
		wind.gust      = 0;
	}

	for( i = 0; i < nwcref; i++ )
	{
		btn = XmVaCreateManagedToggleButton(baseModel, wcref[i]->label, NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, model_cb, wcref[i]->name);
		if(selnum == i) selbtn = btn;
	}

	if (selbtn) XuToggleButtonSet(selbtn, True, True);

	 w = XmVaCreateManagedFrame(dialog, "displayFrame",
		XmNshadowType, XmSHADOW_IN,
		XmNbackground, XuLoadColorResource(dialog, RNtextFieldBg, "black"),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, baseModel,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	entryDisplay = XmVaCreateManagedLabel(w, "entryDisplay",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 6,
		XmNmarginWidth, 6,
		NULL);

	aligner = XmVaCreateColumn(dialog, "aligner",
		XmNborderWidth, 1,
		XmNmarginHeight, 6,
		XmNmarginWidth, 6,
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, w,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for( i = 0; i < XtNumber(label_vals); i++ )
	{
		rc = XmVaCreateRowColumn(aligner, label_vals[i],
			XmNorientation, XmHORIZONTAL,
			XmNspacing, 4,
			NULL);

		for( j = 0; j < XtNumber(btn_vals); j++)
		{
			snprintf(mbuf, sizeof(mbuf), "%+.d", btn_vals[j]);
			btn = XmVaCreateManagedPushButton(rc, mbuf,
				XmNmarginWidth, 5,
				NULL);
			XtAddCallback(btn, XmNactivateCallback, entry_callback[i], INT2PTR(btn_vals[j]));
		}
		XtManageChild(rc);
	}
	XtManageChild(aligner);
	XuShowDialog(dialog);
	display_data();
	FreeItem(menu_file);
}


void DestroyBkgndWindEntryDialog(void)
{
	if (dialog) XuDestroyDialog(dialog);
	dialog       = NullWidget;
	entryDisplay = NullWidget;
}


/*========================= LOCAL FUNCTIONS ===============================*/


static void display_data(void)
{
    CAL cal;

    if(IsNull(entryDisplay)) return;

    cal = CAL_create_by_edef(GV_active_field->info->element);
    (void) build_wind_attribs(cal, &wind);
    XuWidgetLabel(entryDisplay, CAL_get_attribute(cal, CALuserlabel));
    (void) CAL_destroy(cal);
}


/*ARGSUSED*/
static void model_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(!XmToggleButtonGetState(w)) return;
	wind.model = (String)client_data;
	display_data();
}


/*ARGSUSED*/
static void angle_delta_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	wind.dir += PTR2INT(client_data);
	display_data();
}


/*ARGSUSED*/
static void speed_delta_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	wind.speed += PTR2INT(client_data);
	wind.speed = MAX(wind.speed, 10);
	display_data();
}


/*ARGSUSED*/
static void gust_delta_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int incr = PTR2INT(client_data);
	if( incr > 0 && wind.gust < wind.speed ) wind.gust = wind.speed;
	wind.gust += incr;
	if( wind.gust <= wind.speed ) wind.gust = 0.0;
	display_data();
}


/*ARGSUSED*/
static void enter_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	WINDFCN  StoreEntryFcn = (WINDFCN)client_data;
	XuSetBusyCursor(ON);
	StoreEntryFcn(&wind);
	XuSetBusyCursor(OFF);
	XuDestroyDialog(dialog);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	dialog       = NullWidget;
	entryDisplay = NullWidget;
}
