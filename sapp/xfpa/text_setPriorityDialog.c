/***************************************************************************/
/*
*  File:     text_setPriorityDialog.c
*
*  Function: ACTIVATE_fcstTextPriorityDialog()
*            InitFcstTextPriority()
*
*  Purpose:  Provides user text forecast element priority setting capability.
*            The user can set, by forecast, the importance order of the
*            forecast elements (wind, weather, etc.)  The results of this
*            are written out by WriteTextFcstInfoFile().
*
*  Function: There are two lists, one which gives the elements in default
*            order and one which gives the override order. If only one
*            element is put into the override list, then that element will
*            come first and the rest in the default order. If an element
*            is put into the override list then the element name is put
*            into a "light" font and the "<" symbol placed in front. If
*            the element is removed, then a dash "-" is put in front.
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
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include "help.h"
#include "fcstText.h"


/* Define the list of forecast elements that could be mentioned in a
*  text forecast.  Note that if this list is changed then the entries
*  in the associated X resource file must be changed as well. The
*  entries in the Forecast Congfiguration File must use these keys
*  as well.
*
*  XXX This should come from a configuration file entry XXX
*/
static String element_list[] = {"weather","wind","sky-cover","temperature","sea-state"};

typedef struct {
	String name;				/* Pointer into element_list[] */
	Boolean is_turned_on;		/* Do we use this default element? */
	int override_pos;			/* If > 0, the position in the override list */
} DL;

#define NELEMENTS XtNumber(element_list)

static int ndefault_list = 0;
static DL *default_list = NULL;
static int override_index_list_len = 0;
static int override_index_list[NELEMENTS];
static Widget dialog = NULL;
static Widget overrideList = NULL;
static Widget defaultList = NULL;
static Widget offBtn = NULL;
static int nfcsts = 0;
static FOG_DATA_PTR *fcsts = NULL;
static int active_fcst_index = -1;
static String **element_order = NULL;
static int default_pos = 0;
static int default_index = 0;
static int override_pos = 0;
static int override_index = 0;

static void accept_cb				 (Widget, XtPointer, XtPointer);
static void fcst_list_cb			 (Widget, XtPointer, XtPointer);
static void exit_cb					 (Widget, XtPointer, XtPointer);
static void change_priority_cb		 (Widget, XtPointer, XtPointer);
static void default_list_cb			 (Widget, XtPointer, XtPointer);
static void override_list_cb		 (Widget, XtPointer, XtPointer);
static void display_lists			 (void);
static void remove_from_override_list(int);
static void update_element_order_list(void);


/*=====================================================================*/
/*
*  Initialize the element priority list for a forecast. Parameters:
*     fcst - the forecast structure
*     fh   - forecast info file handle.
*/
/*=====================================================================*/
void InitFcstTextPriority(FOG_DATA_PTR fcst , INFOFILE fh )
{
	int n, posn;
	char data[256];
	String default_data, ptr;

	/* If there is no default element order provided for the given
	*  forecast then the element order is not allowed to be set.
	*/
	fcst->element_order = NULL;
	data[0] = '\0';
	strcpy(data, FoG_setup_parm(fcst->key,FOG_ELEMENT_ORDER));
	if(blank(data)) return;

	/* Allocate one more in the order list than is required so
	*  the list will always be NULL terminated.
	*/
	fcst->element_order = NewStringArray(NELEMENTS+1);

	/* If there is a list in the info file use this to set the element
	*  order for this forecast. If not use the default list.
	*/
	if(info_file_find_block(fh, fcst->key))
	{
		default_data = info_file_get_data(fh, FOG_ELEMENT_ORDER);
		if(!blank(default_data))
		{
			strcpy(data, default_data);
			while(ptr = strchr(data, ',')) *ptr = ' ';
		}
	}
	n = 0;
	while((ptr = string_arg(data)) != NULL && n < NELEMENTS)
	{
		if(!InList(ptr, NELEMENTS, element_list, &posn)) continue;
		fcst->element_order[n] = element_list[posn];
		n++;
	}
}


/*=====================================================================*/
/*
*	Create the interface dialog along with all of the objects required
*   for option setting.  Note that some of the code is written in such
*   a way as to prevent "jittering" of the dialog when parameters and
*   data change.
*/
/*=====================================================================*/
void ACTIVATE_fcstTextPriorityDialog(Widget parent, int nfcst,
										FOG_DATA_PTR fcst, FOG_DATA_PTR afcst)
{
	int i, j, chosen;
	char mbuf[50];
	XmString xmlabel, *xmlabels;
	Widget fcstListLabel, fcstList;
	Widget label, btn;

	static XuDialogActionsStruct action_items[] = {
		{ "okBtn",     accept_cb,          NULL },
		{ "cancelBtn", XuDestroyDialogCB, NULL },
		{ "helpBtn",   HelpCB,        HELP_TEXT_FCST_PRIORITY}
	};

	if(dialog) return;

	/* Create the list of valid forecasts. We filter out those for
	*  which the element order is not allowed to be modified. We
	*  also make a copy of the element order.
	*/
	chosen = 0;
	nfcsts = 0;
	fcsts  = NewMem(FOG_DATA_PTR, nfcst);
	element_order = NewMem(String*, nfcst);
	for(i = 0; i < nfcst; i++)
	{
		if(!fcst[i].element_order) continue;
		fcsts[nfcsts] = &fcst[i];
		if(fcsts[nfcsts] == afcst) chosen = nfcsts + 1;
		element_order[nfcsts] = NewStringArray(NELEMENTS);
		j = 0;
		while(fcsts[nfcsts]->element_order[j] && j < NELEMENTS)
		{
			element_order[nfcsts][j] = fcsts[nfcsts]->element_order[j];
			j++;
		}
		nfcsts++;
	}

	/* Create labels for initialization of the list widgets below.
	*  This is done as we don't want the lists to flash when changing
	*  contents and this will pre-determine the size.
	*/
	xmlabels = NewMem(XmString, NELEMENTS);
	for(i = 0; i < NELEMENTS; i++)
	{
		snprintf(mbuf, sizeof(mbuf), "%d %s", i, XuGetLabel(element_list[i]));
		xmlabels[i] = XmStringCreate(mbuf, LARGE_BOLD_FIXED_FONT);
	}

	/* Create the default list structure.
	*/
	default_list = NewMem(DL, NELEMENTS);

	/* Now create the dialog itself.
	*/
    dialog = XuCreateFormDialog(parent, "fcstTextPriority",
		XmNnoResize, True,
        XuNdestroyCallback, exit_cb,
        XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		NULL);

    fcstListLabel = XmVaCreateManagedLabel(dialog, "fcstListLabel",
        XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 19,
        XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 19,
        NULL);

    fcstList = XmVaCreateManagedScrolledList(dialog, "fcstList",
    	XmNselectionPolicy, XmBROWSE_SELECT,
    	XmNscrollBarDisplayPolicy, XmSTATIC,
    	XmNlistMarginHeight, 3,
    	XmNlistMarginWidth, 5,
    	XmNtopAttachment, XmATTACH_WIDGET,
    	XmNtopWidget, fcstListLabel,
    	XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
    	XmNleftWidget, fcstListLabel,
    	XmNbottomAttachment, XmATTACH_FORM,
    	XmNbottomOffset, 19,
		NULL);
    XtAddCallback(fcstList, XmNbrowseSelectionCallback, fcst_list_cb, NULL);

	for(i = 0; i < nfcsts; i++)
	{
        xmlabel = XmStringCreate(fcsts[i]->label, LARGE_BOLD_FIXED_FONT);
        XmListAddItem(fcstList, xmlabel, 0);
        XmStringFree(xmlabel);
	}

    label = XmVaCreateManagedLabel(dialog, "overrideLabel",
        XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 19,
        XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftOffset, 29,
		XmNleftWidget, fcstList,
        NULL);

	overrideList = XmVaCreateManagedList(dialog, "overrideList",
		XmNlistSizePolicy, XmCONSTANT,
		XmNvisibleItemCount, NELEMENTS,
		XmNitems, xmlabels,
		XmNitemCount, NELEMENTS,
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNlistMarginWidth, 6,
		XmNlistMarginHeight, 6,
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
        XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, label,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 19,
		NULL);
	XtAddCallback(overrideList, XmNbrowseSelectionCallback, override_list_cb, NULL);

	btn = XmVaCreateManagedPushButton(dialog, "<--",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNmarginWidth, 6,
        XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopOffset, 20,
		XmNtopWidget, overrideList,
        XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftOffset, 9,
		XmNleftWidget, overrideList,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, change_priority_cb, (XtPointer)'O');

	btn = XmVaCreateManagedPushButton(dialog, "-->",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNmarginWidth, 6,
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 9,
		XmNtopWidget, btn,
        XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, btn,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, change_priority_cb, (XtPointer)'D');

	offBtn = XmVaCreateManagedPushButton(dialog, XuGetLabelUc("on"),
		XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopOffset, 19,
		XmNtopWidget, btn,
        XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, btn,
        XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, btn,
		NULL);
	XtAddCallback(offBtn, XmNactivateCallback, change_priority_cb, (XtPointer)'T');

    label = XmVaCreateManagedLabel(dialog, "defaultLabel",
        XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 19,
        XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftOffset, 9,
		XmNleftWidget, btn,
        NULL);

	defaultList = XmVaCreateManagedList(dialog, "defaultList",
		XmNlistSizePolicy, XmCONSTANT,
		XmNvisibleItemCount, NELEMENTS,
		XmNitems, xmlabels,
		XmNitemCount, NELEMENTS,
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNlistMarginWidth, 6,
		XmNlistMarginHeight, 6,
        XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
        XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, label,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 19,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 19,
		NULL);
	XtAddCallback(defaultList, XmNbrowseSelectionCallback, default_list_cb, NULL);

	XuShowDialog(dialog);

	XmStringArrayFree(xmlabels, NELEMENTS);

	if(nfcsts > 0) XmListSelectPos(fcstList, chosen, True);
}


/* Respond to the selection of a forecast. Any forecast specific setup
*  must be done within this function.
*/
/*ARGSUSED*/
static void fcst_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, m, n, index;
	String data, ptr;
	Boolean found[NELEMENTS];
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	update_element_order_list();
	active_fcst_index = rtn->item_position - 1;

	/* Now set the element order in our management structure from the new
	*  forecast just selected.  First we get the default ordering.
	*/
	data = XtNewString(FoG_setup_parm(fcsts[active_fcst_index]->key, FOG_ELEMENT_ORDER));
	ndefault_list = 0;
	while((ptr = string_arg(data)) != NULL && ndefault_list < NELEMENTS)
	{
		if(!InList(ptr, NELEMENTS, element_list, &index)) continue;
		default_list[ndefault_list].name = element_list[index];
		default_list[ndefault_list].is_turned_on = False;
		default_list[ndefault_list].override_pos = 0;
		ndefault_list++;
	}
	FreeItem(data);

	/* Set the list as actually specified by the forecast.
	*/
	(void)memset((void*) found, 0, NELEMENTS*sizeof(Boolean));
	i = n = override_index_list_len = 0;
	while(i < NELEMENTS && n < NELEMENTS)
	{
		if((ptr = element_order[active_fcst_index][i]) == NULL) break;
		if(same(ptr, default_list[n].name))
		{
			default_list[n].is_turned_on = True;
			found[n] = True;
			n++;
			while(n < ndefault_list && found[n]) n++;
		}
		else
		{
			for(m = 0; m < ndefault_list; m++)
			{
				if(!same(ptr, default_list[m].name)) continue;
				override_index_list[override_index_list_len] = m;
				override_index_list_len++;
				default_list[m].is_turned_on = False;
				default_list[m].override_pos = override_index_list_len;
				found[m] = True;
				break;
			}
		}
		i++;
	}
	default_pos = override_pos = 0;
	display_lists();
}


/*ARGSUSED*/
static void change_priority_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2CHAR(client_data))
	{
		case 'O':

			if(!default_pos || !default_list[default_index].is_turned_on) break;
			override_index_list[override_index_list_len] = default_index;
			default_list[default_index].is_turned_on = False;
			override_index_list_len++;
			default_list[default_index].override_pos = override_index_list_len;
			default_pos = 0;
			break;

		case 'D':

			if(!override_pos) break;
			default_list[override_index_list[override_index]].is_turned_on = True;
			default_list[override_index_list[override_index]].override_pos = 0;
			remove_from_override_list(override_index);
			override_pos = 0;
			break;

		case 'T':

			if(default_pos)
			{
				if(default_list[default_index].is_turned_on)
				{
					default_list[default_index].is_turned_on = False;
				}
				else if(default_list[default_index].override_pos)
				{
					remove_from_override_list(default_list[default_index].override_pos-1);
					default_list[default_index].override_pos = 0;
				}
				else
				{
					default_list[default_index].is_turned_on = True;
				}
				default_pos = 0;
			}
			if(override_pos)
			{
				default_list[override_index_list[override_index]].is_turned_on = False;
				default_list[override_index_list[override_index]].override_pos = 0;
				remove_from_override_list(override_index);
				override_pos = 0;
			}
			break;
	}
	display_lists();
}


/*ARGSUSED*/
static void default_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	String str;
	XmString label;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	default_pos = rtn->item_position;
	default_index = default_pos - 1;
	str = XuGetLabelUc("on");;
	if(default_list[default_index].is_turned_on || default_list[default_index].override_pos)
		str = XuGetLabelUc("off");;
	label = XuNewXmString(str);
	XtVaSetValues(offBtn,
		XmNlabelString, label,
		NULL);
	XmStringFree(label);
	if(override_pos) XmListDeselectPos(overrideList, override_pos);
	override_pos = 0;
}


/*ARGSUSED*/
static void override_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XmString label;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	override_pos = rtn->item_position;
	override_index = override_pos - 1;
	label = XuNewXmString(XuGetLabelUc("off"));
	XtVaSetValues(offBtn,
		XmNlabelString, label,
		NULL);
	XmStringFree(label);
	if(default_pos) XmListDeselectPos(defaultList, default_pos);
	default_pos = 0;
}


static void display_lists(void)
{
	int i, n;
	char mbuf[50], *ptr;
	XmString label;

	XmListDeleteAllItems(overrideList);
	n = 1;
	for(i = 0; i < override_index_list_len; i++)
	{
		snprintf(mbuf, sizeof(mbuf), "%d %s", n, XuGetLabel(default_list[override_index_list[i]].name));
		n++;
		label = XmStringCreate(mbuf, LARGE_BOLD_FIXED_FONT);
        XmListAddItem(overrideList, label, 0);
		XmStringFree(label);
	}

	XmListDeleteAllItems(defaultList);
	for(i = 0; i < ndefault_list; i++)
	{
		ptr = XuGetLabel(default_list[i].name);
		if(default_list[i].is_turned_on)
		{
			snprintf(mbuf, sizeof(mbuf), "%d %s", n, ptr); n++;
			label = XmStringCreate(mbuf, LARGE_BOLD_FIXED_FONT);
		}
		else if(default_list[i].override_pos)
		{
			snprintf(mbuf, sizeof(mbuf), "< %s", ptr);
			label = XmStringCreate(mbuf, LARGE_FIXED_FONT);
		}
		else
		{
			snprintf(mbuf, sizeof(mbuf), "- %s", ptr);
			label = XmStringCreate(mbuf, LARGE_FIXED_FONT);
		}
        XmListAddItem(defaultList, label, 0);
		XmStringFree(label);
	}

	/* Restore the selections
	*/
	if(default_pos) XmListSelectPos(defaultList, default_pos, False);
	if(override_pos) XmListSelectPos(overrideList, override_pos, False);
}


static void update_element_order_list(void)
{
	int i, n;

	if(active_fcst_index < 0) return;

	(void)memset((void*)element_order[active_fcst_index], 0, NELEMENTS*sizeof(String));
	for(i = 0; i < override_index_list_len; i++)
	{
		element_order[active_fcst_index][i] =
			default_list[override_index_list[i]].name;
	}
	n = override_index_list_len;
	for(i = 0; i < ndefault_list; i++)
	{
		if(!default_list[i].is_turned_on) continue;
		if(default_list[i].override_pos) continue;
		element_order[active_fcst_index][n] = default_list[i].name;
		n++;
	}
}


static void remove_from_override_list(int n )
{
	int i;
	if(n < 0) return;
	override_index_list_len--;
	for(i = n; i < override_index_list_len; i++)
	{
		override_index_list[i] = override_index_list[i+1];
	}
}


/* Accept the changes made and stuff them into the info file.
*/
/*ARGSUSED*/
static void accept_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j;

	update_element_order_list();
	for(i = 0; i < nfcsts; i++)
	{
    	if(fcsts[i]->state == SPACER || fcsts[i]->state == HEADER) continue;
		for(j = 0; j < NELEMENTS; j++)
		{
			fcsts[i]->element_order[j] = element_order[i][j];
		}
	}
	WriteTextFcstInfoFile();
	XuDestroyDialog(dialog);
}


/* Exit and clean up.
*/
/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	active_fcst_index = -1;
	FreeItem(fcsts);
	FreeItem(default_list);
	FreeList(element_order, nfcsts);
	nfcsts = 0;
	dialog = NULL;
}
