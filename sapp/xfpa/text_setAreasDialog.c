/***************************************************************************/
/*
*  File:     text_setAreasDialog.c
*
*  Function: ACTIVATE_fcstTextSetAreasDialog()
*            InitFcstTextAreas()
*
*  Purpose:  Provides user text forecast area selecting capability. This
*            allows some forecasts to only mention those areas which the
*            forecaster chooses. When done the results are written out
*            to a forecast info file by a call to WriteTextFcstInfoFile().
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
/***************************************************************************/
#include "global.h"
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ToggleB.h>
#include "help.h"
#include "fcstText.h"


static Widget dialog = NULL;
static Widget *btns  = NullWidgetList;
static FOG_DATA_PTR fcst = NULL;

static void exit_cb		(Widget, XtPointer, XtPointer);
static void set_areas_cb(Widget, XtPointer, XtPointer);


/*============================================================================*/
/*
*	InitFcstTextAreas() - Initialize the forecast structure for areas.
*                         The parameters are: infcst - the forecast structure
*                                             fh     - info file handle
*/
/*============================================================================*/
void InitFcstTextAreas(FOG_DATA_PTR infcst , INFOFILE fh )
{
	int posn, nareas;
	String data, ptr, key, *name_list;

	infcst->area_states = NULL;

	key = FoG_setup_parm(infcst->key,FOG_SELECT_AREAS);
	if(!same_ic(key,"true")) return;

	nareas = FcstAreaList(infcst, &name_list, NULL);
	infcst->area_states = NewBooleanArray(nareas);
	memset((void *)infcst->area_states, True, nareas*sizeof(Boolean));

	if(info_file_find_block(fh, infcst->key))
	{
		data = info_file_get_data(fh,FOG_AREAS);
		if(!blank(data))
		{
			memset((void *)infcst->area_states, False, nareas*sizeof(Boolean));
			while((ptr = strchr(data,','))) *ptr = ' ';
			while((ptr = string_arg(data)))
			{
				if(InList(ptr, nareas, name_list, &posn))
					infcst->area_states[posn] = True;
			}
		}
	}
	FreeList(name_list, nareas);
}


/*============================================================================*/
/*
*	FcstAreaList() - Returns the list of forecast area id's as recognized by
*	                 FoG and a list of corresponding area labels which the
*   user sees. Note that is the responsibility of the calling procedure to
*   free the memory allocated for the area_id and area_label return arrays.
*/
/*============================================================================*/
int FcstAreaList(FOG_DATA_PTR f , String **area_id , String **area_label )
{
	int    nareas;
	String str, areas, *ids, *labels;

	nareas = 0;
	ids    = NULL;
	labels = NULL;

	areas = XtNewString(FoG_setup_parm(f->key, FOG_AREAS));

	while((str = string_arg(areas)))
	{
		if(area_id)
		{
			ids = MoreStringArray(ids, nareas+1);
			ids[nareas] = XtNewString(str);
		}
		str = string_arg(areas);
		if(area_label)
		{
			labels = MoreStringArray(labels, nareas+1);
			labels[nareas] = XtNewString(str);
		}
		nareas++;
	}
	FreeItem(areas);

	if(area_id)    *area_id    = ids;
	if(area_label) *area_label = labels;

	return nareas;
}


/*============================================================================*/
/*
*	ACTIVATE_fcstTextSetAreasDialog() - Create and popup the dialog.
*/
/*============================================================================*/
void ACTIVATE_fcstTextSetAreasDialog(Widget parent , FOG_DATA_PTR infcst )
{
	int	      n, nareas;
	String    *label_list;
	Dimension width, maxwidth;
	Widget    activeAreaWindow , rc;

	static XuDialogActionsStruct action_items[] = {
		{"okBtn",    set_areas_cb,        NULL},
		{"cancelBtn",XuDestroyDialogCB, NULL},
		{"helpBtn",  HelpCB,            HELP_TEXT_FCST_AREAS}
	};

	if(dialog) return;

	fcst = infcst;

	dialog = XuCreateFormDialog(parent, "fcstTextSetAreas",
		XuNdestroyCallback, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing,9,
		NULL);

	activeAreaWindow = XmVaCreateManagedScrolledWindow(dialog, "activeAreaWindow",
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	/* Now create the toggle buttons for the forecast areas
	*/
	nareas = FcstAreaList(fcst, NULL, &label_list);
	if(nareas > 0)
	{
		rc   = XmCreateRowColumn(activeAreaWindow, "areaManager", NULL, 0);
		btns = NewWidgetArray(nareas);
		maxwidth = 0;
		for ( n = 0; n < nareas; n++)
		{
			btns[n] = XmVaCreateManagedToggleButton(rc, label_list[n],
				XmNset, (fcst->area_states[n]) ? XmSET:XmUNSET,
				NULL);
			XtVaGetValues(btns[n], XmNwidth, &width, NULL);
			maxwidth = MAX(maxwidth,width);
		}
		XtManageChild(rc);
		XtVaSetValues(activeAreaWindow, XmNwidth, maxwidth+50, NULL );
		FreeList(label_list, nareas);
	}
	XuShowDialog(dialog);
}


/*ARGSUSED*/
static void set_areas_cb(Widget w , XtPointer client_data , XtPointer call_data )

{
	int	i, nareas;

	nareas = FcstAreaList(fcst, NULL, NULL);
	for( i = 0; i < nareas; i++ )
	{
		fcst->area_states[i] = XmToggleButtonGetState(btns[i]);
	}
	WriteTextFcstInfoFile();
	XuDestroyDialog(dialog);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer cld , XtPointer cd )
{
	FreeItem(btns);
	dialog = NULL;
}
