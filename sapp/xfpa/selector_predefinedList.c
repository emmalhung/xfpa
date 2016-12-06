/*****************************************************************************
*
*  File:     selector_predefinedList.c
*
*  Purpose:  Creates the selector for predefined points.
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

#include <stdarg.h>
#include "global.h"
#include <Xm/ComboBox.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include "editor.h"
#include "guidance.h"
#include "selector.h"


/* The type of calls to use to send the point data is of type GE_CMD passed
*  in client_data. GE_SAMPLE is synthetic for use in this function.
*/
/*ARGSUSED*/
static void SendPointsCB(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int         sel, count = 0;
	String      line, ptr, lat, lon;
	SELECT_TYPE type;
	INFOFILE    fd;

	static String list_go = "LIST GO";

	type = (SELECT_TYPE)client_data;

	sel = XuComboBoxGetSelectedPos(XtNameToWidget(XtParent(w), "predefList")) - 1;
	if(sel < 0) return;
	fd = info_file_open(get_file(PRESET_LISTS, POINTS_FILE));
	while(!blank(info_file_find_next_block(fd)))
	{
		if(sel == count) break;
		count++;
	}
	XuSetBusyCursor(ON);
	while(!(blank(line = info_file_get_next_line(fd))))
	{
		/* Find the comment character in the line (if any) and set this to
		*  NULL as everything after this is a comment.
		*/
		if(NotNull(ptr = strstr(line,"#!"))) *ptr = '\0';

		lat = strtok_arg(line);
		lon = strtok_arg(NULL);
		switch(type)
		{
		case SELECT_EDIT_LABEL:
			(void) IngredVaCommand(GE_DEPICTION, "LABEL LIST %s %s" , lat,lon);
			break;
		case SELECT_EDIT_SAMPLE:
			(void) IngredVaCommand(GE_DEPICTION, "SAMPLE LIST %s %s", lat,lon);
			break;
		case SELECT_GUID_SAMPLE:
			(void) IngredVaCommand(GE_GUIDANCE,  "SAMPLE LIST %s %s", lat,lon);
			break;
		case SELECT_IMAGE_SAMPLE:
			(void) IngredVaCommand(GE_IMAGERY,  "SAMPLE LIST %s %s", lat,lon);
			break;
		}
	}
	info_file_close(fd);

	switch(type)
	{
		case SELECT_EDIT_LABEL:   SendEditLabelCommand(list_go);      break;
		case SELECT_EDIT_SAMPLE:  SendEditSampleCommand(list_go);     break;
		case SELECT_GUID_SAMPLE:  SendGuidanceSampleCommand(list_go); break;
		case SELECT_IMAGE_SAMPLE: SendImageSampleCommand(list_go);    break;
	}
	XuSetBusyCursor(OFF);
}


/*=======================================================================*/
/*
*	CreateSampleFieldPanel() -  Create the sampling panel and all the
*	sub-panels.
*/
/*=======================================================================*/
Widget CreatePredefinedPointsSelector( Widget parent, SELECT_TYPE type, ... )
{
	int      ac, nlist;
	String   cmd;
	Widget   predefList, frame, form, btn;
	INFOFILE fd;
	Arg      al[20];
	va_list args;

	ac = 0;
	XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	XtSetArg(al[ac], XmNresizable, False);               ac++;

	va_start(args, type);
	while((cmd = va_arg(args, String)) != (String)NULL && ac < 20)
	{
		XtSetArg(al[ac], cmd, va_arg(args, XtPointer)); (ac)++;
	}
	va_end(args);

	frame = XmCreateFrame(parent, "predefFrame", al, ac);

	(void) XmVaCreateManagedLabel(frame, "predefPoints",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateForm(frame, "predefForm",
		XmNhorizontalSpacing, 3,
		XmNverticalSpacing, 3,
		NULL);

	predefList = XmVaCreateManagedComboBox(form, "predefList",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	btn = XmVaCreateManagedPushButton(form, "gridBtn",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, predefList,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, SendPointsCB, (XtPointer)type);

	/* Load the list of sample point definition files into the ComboBox
	*  selector. If there are none or if none of the files can not be
	*  opened deactivate the selection.
	*/
	nlist = 0;
	fd = info_file_open(get_file(PRESET_LISTS, POINTS_FILE));
	if(NotNull(fd))
	{
		String *items = NULL;
		while(!blank(info_file_find_next_block(fd)))
		{
			items = MoreStringArray(items, nlist+1);
			items[nlist] = XtNewString(info_file_get_block_label(fd));
			nlist++;
		}
		XuComboBoxAddItems(predefList, items, nlist, 0);
		FreeList(items, nlist);
		info_file_close(fd);
	}

	if(nlist < 1)
	{
		XuComboBoxAddItem(predefList, XuGetLabel("na"), 0);
		XtSetSensitive(predefList, False);
		XtSetSensitive(btn, False);
	}
	XuComboBoxSelectPos(predefList, 1, False);
	XtManageChild(form);
	XtManageChild(frame);

	return frame;
}
