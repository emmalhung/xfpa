/*========================================================================*/
/*
*	File:		dataEntryDialog_wind.c
*
*	Purpose:	Provides the input dialog for entering the wind
*               into the depiction.
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
#include "depiction.h"
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Column.h>
#include "editor.h"
#include "resourceDefines.h"
#include "wind.h"
#include "help.h"

/* Set the maximum number of wind models that we can handle.*/
#define NMODELS	30


static void    angle_cb     (Widget, XtPointer, XtPointer);
static void    exit_cb      (Widget, XtPointer, XtPointer);
static void    gust_cb      (Widget, XtPointer, XtPointer);
static void    type_cb      (Widget, XtPointer, XtPointer);
static void    model_cb     (Widget, XtPointer, XtPointer);
static Boolean parse_cal    (CAL);
static void    set_cb       (Widget, XtPointer, XtPointer);
static void    speed_cb     (Widget, XtPointer, XtPointer);
static void    store_cb     (Widget, XtPointer, XtPointer);
static void    display_data (void);

static String         entry_labels[]   = { "angle", "speed", "gusts" };
static XtCallbackProc entry_callback[] = { angle_cb, speed_cb, gust_cb  };
static int            entry_vals[]     = { -20, -10, -5, 5, 10, 20   };
static String         type_labels[]    = { "mdms", "mdas", "adms", "adas" };

static String  module        = "windEntryDialog";
static Widget  dialog        = NullWidget;
static Widget  entryDisplay  = NullWidget;
static String  model_name    = NULL;
static int     rel_dirn      = -30;
static int     rel_speed     = 70;
static int     rel_gust      = 70;
static int     abs_dirn      = 270;
static int     abs_speed     = 15;
static int     abs_gust      = 15;
static Boolean use_abs_dirn  = False;
static Boolean use_abs_speed = False;


/*==================================================================*/
/*
*	ACTIVATE_windEntryDialog() - Creates and popups the wind
*	entry dialog.  The parameter store_fcn is the function to call
*	when the enter button is pressed.
*/
/*==================================================================*/
void ACTIVATE_windEntryDialog(Widget posnWidget, CAL cal, WINDFCN set_fcn, WINDFCN store_fcn)
{
	int	     i, j, n, nwcref;
	char     mbuf[20];
	String   mf, mfp, menu_file, name, model;
	Widget   w, selector, option, entryManager, rc, displayFrame;
	INFOFILE ifd;

	XuDialogActionsStruct     action_items[4];
    FpaConfigWindEditorStruct *wptr;
	FpaConfigCrossRefStruct   *wcref[NMODELS];

	static String wed = "wind entry dialog";

	if (NotNull(dialog))
	{
		XuShowDialog(dialog);
		return;
	}

	wptr = GV_active_field->info->element->elem_detail->editor->type.wind;

	/* Make sure that we have the wind entry menu definition file and that it
	*  contains the data block that we need.
	*/
	if(InEditMode(E_MODIFY))
		mf = wptr->modify_file;
	else
		mf = wptr->entry_file;

	if(blank(mf))
	{
		pr_error(module, "%s %s menu file not defined in configuration files\n",
				wed, (InEditMode(E_MODIFY)) ? "modify":"entry");
		return;
	}

	if(IsNull(mfp = get_file(MENUS_CFG,mf)))
	{
		pr_error(module, "%s menu file \"%s\" not found\n", wed, mf);
		return;
	}

	/* We need to copy menu file as the info_file functions may call functions that
	 * overwrite mfp as it points to an internal buffer.
	 */
	menu_file = XtNewString(mfp);

	if(IsNull(ifd = info_file_open(menu_file)))
	{
		pr_error(module, "Unable to open %s menu file \"%s\"\n", wed, menu_file);
		FreeItem(menu_file);
		return;
	}

	menu_file = XtNewString(mfp);
	if(!info_file_find_block(ifd, "models"))
	{
		pr_error(module, "No \"models\" block in %s menu file \"%s\"\n", wed, menu_file);
		FreeItem(menu_file);
		info_file_close(ifd);
		return;
	}

	/* Read the entry menu definition file and get the wind models.
	*/
	nwcref = 0;
	model  = NULL;
	while(!(blank(name = info_file_get_next_line(ifd))) && nwcref < NMODELS)
	{
		wcref[nwcref] = identify_crossref(FpaCcRefsWinds, name);
		if (NotNull(wcref[nwcref]))
		{
			if(same(model_name,name) || IsNull(model)) model = wcref[nwcref]->label;
			nwcref++;
		}
		else
		{
			pr_error(module, "Unknown model \"%s\" in %s menu file \"%s\"\n",
				name, wed, menu_file);
		}
	}
	info_file_close(ifd);

	if(!nwcref)
	{
		pr_error(module, "No valid wind models found in menu file \"%s\"\n", menu_file);
		pr_error(module, "Wind entry dialog terminated.\n");
		FreeItem(menu_file);
		return;
	}

	/* Load the wind into the dialog internal variables.
	*/
	if(!parse_cal((cal)? cal : GV_active_field->cal))
	{
		(void) parse_cal(GV_active_field->bkgnd_cal);
	}

	/* Set up the action buttons depending on the availability of entry
	*  functions passed in the parameter list.
	*/
	n = 0;
	if(NotNull(set_fcn))
	{
		action_items[n].id       = "setBtn";
		action_items[n].callback = set_cb;
		action_items[n].data     = (XtPointer) set_fcn;
		n++;
	}
	if(NotNull(store_fcn))
	{
		action_items[n].id       = "addToListBtn";
		action_items[n].callback = store_cb;
		action_items[n].data     = (XtPointer) store_fcn;
		n++;
	}
	action_items[n].id       = "closeBtn";
	action_items[n].callback = exit_cb;
	action_items[n].data     = NULL;
	n++;
	action_items[n].id       = "helpBtn";
	action_items[n].callback = HelpCB;
	action_items[n].data     = HELP_WIND_ENTRY;
	n++;

	dialog = XuCreateToplevelFormDialog(posnWidget, "windEntry",
		XuNmwmDeleteOverride, exit_cb,
		XmNnoResize, True,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, n,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	selector = XmVaCreateManagedColumn(dialog, "selector",
		XmNborderWidth, 1,
		XmNmarginWidth, 6,
		XmNmarginHeight, 6,
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	option = XuVaMenuBuildOption(selector, "type", NULL,
		XmNlabelString, NULL,
		NULL);

	for( i = 0; i < XtNumber(type_labels); i++ )
	{
		(void)XuMenuAddButton(option, type_labels[i], 0, NoId, type_cb, INT2PTR(i)); 
	}

	name = NULL;
	if( !use_abs_dirn && !use_abs_speed ) name = type_labels[0];
	if( !use_abs_dirn &&  use_abs_speed ) name = type_labels[1];
	if(  use_abs_dirn && !use_abs_speed ) name = type_labels[2];
	if(  use_abs_dirn &&  use_abs_speed ) name = type_labels[3];

	XuMenuSelectItemByName(option, name);

	option = XuVaMenuBuildOption(selector, "model", NULL,
		XmNlabelString, NULL,
		NULL);

	/* Put wind models into buttons */
	for( i = 0; i < nwcref; i++ )
		(void)XuMenuAddButton(option, wcref[i]->label, 0, NoId, model_cb, wcref[i]->name); 

	XuMenuSelectItemByName(option, model);

	displayFrame = XmVaCreateManagedFrame(dialog, "displayFrame",
		XmNshadowType, XmSHADOW_IN,
		XmNbackground, XuLoadColorResource(dialog, RNtextFieldBg, "black"),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, selector,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	
	entryDisplay = XmVaCreateManagedLabel(displayFrame, "entryDisplay",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 6,
		NULL);

	entryManager = XmVaCreateColumn(dialog, "entryManager",
		XmNborderWidth, 1,
		XmNmarginWidth, 6,
		XmNmarginHeight, 6,
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, displayFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for( i = 0; i < XtNumber(entry_labels); i++)
	{
		rc = XmVaCreateRowColumn(entryManager, entry_labels[i],
			XmNpacking, XmPACK_COLUMN,
			XmNorientation, XmHORIZONTAL,
			XmNspacing, 3,
			NULL);

		for(j = 0; j < XtNumber(entry_vals); j++)
		{
			(void) snprintf(mbuf, sizeof(mbuf), "%+.d", entry_vals[j]);
			w = XmVaCreateManagedPushButton(rc, mbuf,
				XmNmarginWidth, 5,
				NULL);
			XtAddCallback(w, XmNactivateCallback, entry_callback[i], INT2PTR(entry_vals[j]));
		}
		XtManageChild(rc);
	}
	XtManageChild(entryManager);
	XuShowDialog(dialog);
	display_data();
	FreeItem(menu_file);
}


/* Updates the dialog from the given CAL. Usually activated when Ingred
*  needs to send data to the interface (ie. on a modify).
*/
void UpdateWindEntryDialog(CAL cal)
{
	if(parse_cal(cal)) display_data();
}


void DestroyWindEntryDialog(void)
{
	if (dialog) XuDestroyDialog(dialog);
	dialog       = NullWidget;
	entryDisplay = NullWidget;
}


/*=================== LOCAL FUNCTIONS ==============================*/


static Boolean parse_cal(CAL cal)
{
	WIND_CALC wind;

	if(IsNull(cal) || !parse_wind_attribs(cal, &wind)) return False;

	model_name = wind.model;

	use_abs_dirn  = !wind.rel_dir;
	use_abs_speed = !wind.rel_speed;

	if (use_abs_dirn)  abs_dirn  = NINT(wind.dir);
	else               rel_dirn  = NINT(wind.dir);

	if (use_abs_speed) abs_speed = NINT(wind.speed);
	else               rel_speed = NINT(wind.speed);

	if (use_abs_speed) abs_gust  = NINT(wind.gust);
	else               rel_gust  = NINT(wind.gust);

	return True;
}


static void FillWindStruct(WIND_CALC *wind)
{
	wind->model     = (use_abs_dirn && use_abs_speed)? FpaAbsWindModel : model_name;
	wind->rel_dir   = !use_abs_dirn;
	wind->rel_speed = !use_abs_speed;
	wind->dir       = (float) (use_abs_dirn  ? abs_dirn  : rel_dirn);
	wind->speed     = (float) (use_abs_speed ? abs_speed : rel_speed);
	wind->gust      = (float) (use_abs_speed ? abs_gust  : rel_gust);
}


static void display_data(void)
{
	WIND_CALC wind;
	CAL       cal;

	if(IsNull(entryDisplay)) return;

	cal = CAL_create_by_edef(GV_active_field->info->element);
	FillWindStruct(&wind);
	(void) build_wind_attribs(cal, &wind);
	XuWidgetLabel(entryDisplay, CAL_get_attribute(cal, CALuserlabel));
	(void) CAL_destroy(cal);
}


/*ARGSUSED*/
static void type_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int ndx = PTR2INT(client_data);
	switch(ndx)
	{
		case 0: use_abs_dirn = False; use_abs_speed = False; break;
		case 1: use_abs_dirn = False; use_abs_speed = True;  break;
		case 2: use_abs_dirn = True;  use_abs_speed = False; break;
		case 3: use_abs_dirn = True;  use_abs_speed = True;  break;
	}
	display_data();
}


/*ARGSUSED*/
static void model_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	model_name = (String)client_data;
	display_data();
}


/*ARGSUSED*/
static void angle_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(use_abs_dirn)
	{
		abs_dirn += PTR2INT(client_data);
		if ( abs_dirn <= 0 ) abs_dirn += 360;
		if ( abs_dirn > 360) abs_dirn = abs_dirn % 360;
	}
	else
	{
		rel_dirn += PTR2INT(client_data);
	}
	display_data();
}


/*ARGSUSED*/
static void speed_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(use_abs_speed)
	{
		abs_speed += PTR2INT(client_data);
		abs_speed = MAX(abs_speed, 0);
	}
	else
	{
		rel_speed += PTR2INT(client_data);
		rel_speed = MAX(rel_speed, 10);
	}
	display_data();
}


/*ARGSUSED*/
static void gust_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int incr = PTR2INT(client_data);

	if (use_abs_speed)
	{
		if(incr > 0 && abs_gust < abs_speed) abs_gust = abs_speed;
 		abs_gust += incr;
		if(abs_gust <= abs_speed) abs_gust = 0;
	}
	else
	{
		if(incr > 0 && rel_gust < rel_speed) rel_gust = rel_speed;
		rel_gust += incr;
		if(rel_gust <= rel_speed) rel_gust = 0;
	}
	display_data();
}


/*ARGSUSED*/
static void set_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	WIND_CALC wind;
	WINDFCN   setfcn = (WINDFCN)client_data;
	FillWindStruct(&wind);
	setfcn(&wind);
}


/*ARGSUSED*/
static void store_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	WIND_CALC wind;
	WINDFCN   storefcn = (WINDFCN)client_data;
	FillWindStruct(&wind);
	storefcn(&wind);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	DestroyWindEntryDialog();
}
