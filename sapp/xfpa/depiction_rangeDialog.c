/*========================================================================*/
/*
*	File:		depiction_rangeDialog.c.c
*
*   Purpose:    Used to ask the user which range of depictions Ingred
*               should read up on initialization. The choices are all
*               none or a range from a list of depiction times. This
*               dialog syncs untill answered.
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
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>

static Widget dialog = NULL;
static Widget saveBtn;
static Widget selectList;
static String command_buffer;
static int    ntimes = 0;
static String *times = NULL;


/*ARGSUSED*/
static void DoneCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int     *list, nlist;
	Boolean save = GV_edit_mode && XmToggleButtonGetState(saveBtn);

	if (!dialog) return;

	switch(PTR2CHAR(client_data))
	{
		case 'a':
			strcat(command_buffer, " ALL");
			break;

		case 's':
			if(XmListGetSelectedPos(selectList, &list, &nlist))
			{
				strcat(command_buffer, " RANGE ");
				strcat(command_buffer, times[list[0]-1]);
				strcat(command_buffer, " ");
				strcat(command_buffer, times[list[nlist-1]-1]);
				FreeItem(list);
				break;
			}

		case 'n':
			strcat(command_buffer, " NONE");
			break;
	}
	strcat(command_buffer, save? " SAVE":" NOSAVE");
	ntimes = FilteredValidTimeListFree(&times, ntimes);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}


void ACTIVATE_getDepictionRangeDialog(String cmd_buffer )
{
	int i;
	XmString item;
	FLD_DESCRIPT fd;
	Widget header, noteRC, note;
	XtAppContext ac;

   static XuDialogActionsStruct action_items[] = {
        { "allBtn",  DoneCB, (XtPointer)'a' },
        { "listBtn", DoneCB, (XtPointer)'s' },
        { "noneBtn", DoneCB, (XtPointer)'n' }
    };

	command_buffer = cmd_buffer;

    dialog = XuCreateFormDialog(GW_mainWindow, "getDepictionRange",
        XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XuNmwmDeleteOverride, DoneCB,
		XuNmwmDeleteData, (XtPointer)'n',
       	XmNdefaultPosition, True,
		XmNhorizontalSpacing, 20,
		XmNverticalSpacing, 20,
		NULL);

	header = XmVaCreateManagedLabel(dialog, "header",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	if(GV_edit_mode)
	{
		note = XmVaCreateManagedFrame(dialog, "frame",
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		(void) XmVaCreateManagedLabel(note, "noteLabel",
			XmNchildType, XmFRAME_TITLE_CHILD,
			NULL);

		noteRC = XmVaCreateManagedRowColumn(note, "noteRC",
			XmNentryAlignment, XmALIGNMENT_BEGINNING,
			XmNspacing, 6,
			XmNchildType, XmFRAME_WORKAREA_CHILD,
			NULL);

		(void) XmVaCreateManagedLabel(noteRC, "note", NULL);

		saveBtn = XmVaCreateManagedToggleButton(noteRC, "saveBtn",
			XmNset, XmSET,
			NULL);
	}

    selectList = XmVaCreateManagedScrolledList(dialog, "selectList",
    	XmNselectionPolicy, XmEXTENDED_SELECT,
    	XmNscrollBarDisplayPolicy, XmSTATIC,
    	XmNlistMarginHeight, 3,
    	XmNlistMarginWidth, 5,
    	XmNtopAttachment, XmATTACH_WIDGET,
    	XmNtopWidget, header,
    	XmNleftAttachment, XmATTACH_FORM,
    	XmNrightAttachment, XmATTACH_FORM,
    	XmNbottomAttachment, GV_edit_mode? XmATTACH_WIDGET:XmATTACH_FORM,
		XmNbottomWidget, note,
		NULL);

	init_fld_descript(&fd);
	(void) set_fld_descript(&fd, FpaF_SOURCE_NAME, DEPICT, FpaF_END_OF_LIST);
	ntimes = FilteredValidTimeList(&fd, FpaC_NORMAL, &times);
	for(i = 0; i < ntimes; i++)
	{
		if (fd.sdef->minutes_rqd)
			item = XuNewXmString(DateString(times[i], MINUTES));
		else
			item = XuNewXmString(DateString(times[i], HOURS));
		XmListAddItem(selectList, item, 0);
		XmStringFree(item);
	}
	XuShowDialog(dialog);

	/* This puts the dialog into a local loop thus locking out any other actions.
	 */
	ac = XtWidgetToApplicationContext(dialog);
	while((dialog != NullWidget && XtIsManaged(dialog)) || XtAppPending(ac))
	{
		XtAppProcessEvent(ac, XtIMAll);
	}
}
