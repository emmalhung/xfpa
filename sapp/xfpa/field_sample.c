/*****************************************************************************
*
*  File:     field_sample.c
*
*  Purpose:  Provides the program interface panel for field sampling.
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

#include "global.h"
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ComboBox.h>
#include <ingred.h>
#include "depiction.h"
#include "editor.h"
#include "observer.h"
#include "selector.h"
#include "resourceDefines.h"

#define SAMPLE_PANEL	"sp"
#define NITEM_BTNS		5


static void cancel_sampling        (void);
static void sampling_cb            (Widget, XtPointer, XtPointer);
static void set_font_attributes    (FontSelectorStruct *);
static void sample_display_control (CAL, String*, int);

static Widget panel = NullWidget;
static Widget predefFrame = NullWidget;
static Widget sampleGrid;
static Widget itemComboFrame;
static Widget itemComboList;
static Widget itemListFrame;
static Widget itemBtns[NITEM_BTNS];
static Widget fontSetManager = NullWidget;
static SAMPLE data;
static int    max_list_items = NITEM_BTNS;
static int    nitem_list = 0;
static SampleListStruct *item_list = NULL;


/*=======================================================================*/
/*
*  SetFieldSampleInfo() - Sets the initial field related sampling
*                         information. The type and item are field
*  specific but the rest are not unless the values have been stored.
*/
/*=======================================================================*/
void SetFieldSampleInfo(FIELD_INFO *field, String type, String item)
{
	String ptr;

	field->sample.process   = "NORMAL";
	field->sample.type      = type;
	field->sample.item      = item;
	field->sample.font_type = data.font_type;
	field->sample.font_size = data.font_size;
	field->sample.colour    = data.colour;

	/* Note that since this is only done once per field, the returned allocated memory is
	 * not freed but used to set the font values and colour.
	 */
	if(XuStateDataGet(SAMPLE_PANEL, field->info->element->name, field->info->level->name, &ptr))
	{
		field->sample.font_type = strtok_arg(ptr);
		field->sample.font_size = strtok_arg(NULL);
		field->sample.colour    = strtok_arg(NULL);
	}
}


/*=======================================================================*/
/*
*   ConfigureSamplingPanel() - Configure the sampling panel. If the given
*                              list is > 0, then the item to be sampled
*   widget is filled in and made visible.  list_select is the item in
*   the list to be shown as currently selected.  If the list is NITEM_BTNS
*   or more items in length then a ComboBox is used, else a row column set
*   of toggle buttons is used.
*/
/*=======================================================================*/
void ConfigureSamplingPanel(void)
{
	int          i;
	FLD_DESCRIPT fd;

    FreeItem(item_list);
	nitem_list = 0;
	init_fld_descript(&fd);
	if(set_fld_descript(&fd,
		FpaF_SOURCE_NAME, DEPICT,
		FpaF_ELEMENT, GV_active_field->info->element,
		FpaF_LEVEL, GV_active_field->info->level,
		FpaF_VALID_TIME, ActiveDepictionTime(FIELD_DEPENDENT),
		FpaF_END_OF_LIST))
	{
    	MakeSampleItemList(&fd, &item_list, &nitem_list);
	}

    if(nitem_list < 1)
	{
		XtVaSetValues(predefFrame,
			XmNtopAttachment, XmATTACH_FORM,
			NULL);
    	XtUnmanageChild(itemComboFrame);
    	XtUnmanageChild(itemListFrame);
	}
	else
    {
		int selected = -1;
		String *items = NewStringArray(nitem_list);

    	for(i = 0; i < nitem_list; i++)
    	{
			items[i] = item_list[i].sh_label;
        	if(!same(GV_active_field->sample.type, item_list[i].type)) continue;
        	if(!same(GV_active_field->sample.item, item_list[i].name)) continue;
			selected = i;
    	}
		if(selected < 0)
		{
			GV_active_field->sample.type = item_list[0].type;
			GV_active_field->sample.item = item_list[0].name;
			selected = 0;
		}
		if(nitem_list > max_list_items)
		{
			XtManageChild(itemComboFrame);
			XuComboBoxDeleteAllItems(itemComboList);
			XuComboBoxAddItems(itemComboList, items, nitem_list, 0);
			XuComboBoxSelectPos(itemComboList, selected+1, False);
			XtVaSetValues(predefFrame, XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, itemComboFrame, NULL);
			XtUnmanageChild(itemListFrame);
		}
		else
		{
			XtManageChild(itemListFrame);
			XtUnmanageChildren(itemBtns, NITEM_BTNS);
			for( i = 0; i < nitem_list; i++ )
			{
				XuWidgetLabel(itemBtns[i], items[i]);
				XmToggleButtonSetState(itemBtns[i], False, False);
			}
			XtManageChildren(itemBtns, nitem_list);
			XmToggleButtonSetState(itemBtns[selected], True, False);
			XtVaSetValues(predefFrame, XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, itemListFrame, NULL);
			XtUnmanageChild(itemComboFrame);
		}
		FreeItem(items);
    }

	switch(GV_active_field->info->element->fld_type)
	{
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_WIND:
			XtVaSetValues(fontSetManager, XmNtopWidget, sampleGrid, NULL);
			XtSetMappedWhenManaged(sampleGrid, True);
			break;

		default:
			XtSetMappedWhenManaged(sampleGrid, False);
			XtVaSetValues(fontSetManager, XmNtopWidget, predefFrame, NULL);
			break;
	}

	/* Set the values for the active field into the font, size and colour
	*  selectors.
	*/
	CopyStruct(&data, &GV_active_field->sample, SAMPLE, 1);
	SetFontSelector(fontSetManager, GV_active_field->sample.font_type,
			GV_active_field->sample.font_size, GV_active_field->sample.colour);
}


/*=======================================================================*/
/*
*	Show and hide the panel.
*/
/*=======================================================================*/
void ShowSamplingPanel(void)
{
	if (panel && !XtIsManaged(panel) ) XtManageChild(panel);
}

void HideSamplingPanel(void)
{
	if (panel && XtIsManaged(panel) )
	{
		XtUnmanageChild(panel);
		cancel_sampling();
	}
}


/*=======================================================================*/
/*
*    Send the command to ingred to initiate sampling.
*/
/*=======================================================================*/
void SendEditSampleCommand(String process)
{
	Boolean sensitive;

	IngredVaEditCommand(GV_active_field->cal, NullCal,  "SAMPLE %s %s %s %s %s%% %s",
		blank(process) ? GV_active_field->sample.process : process,
		GV_active_field->sample.type,
		GV_active_field->sample.item,
		GV_active_field->sample.font_type,
		GV_active_field->sample.font_size,
		GV_active_field->sample.colour );

	/* If we are asked to sample all attributes or field labels set
	*  the pre-defined point selector insensitive.
	*/
	sensitive =	(!same_ic(data.item, AttribAll) &&
				 !(same_ic(data.item, AttribFieldLabels) || same_ic(data.item, AttribLinkNodes)));
	XtSetSensitive(predefFrame, sensitive);
	XtSetSensitive(sampleGrid,  sensitive);
}


/*=======================================================================*/
/*
*	CreateSampleFieldPanel() -  Create the sampling panel and all the
*	sub-panels.
*/
/*=======================================================================*/
void CreateSampleFieldPanel(Widget parent , Widget topAttach)
{
	int n;
	Widget rc;

	/* How many items can be in the sample item list before
	 * switching to the combox display?
	 */
	max_list_items = XuGetIntResource(RNmaxSampleItemListLen,5);

	panel = XmVaCreateForm(parent, "sampleFieldPanel",
		XmNborderWidth, 0,
		XmNresizable, False,
		XmNhorizontalSpacing, 3,
		XmNverticalSpacing, 20,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, topAttach,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	/* Create two frames here, one for the row column list and one
	 * for a combobox. Only one of these will be displayed at any
	 * given time. This way short lists will be displayed in their
	 * entirety and long lists in the combobox format. The callback
	 * function is the same for both, but which one is calling the
	 * function is determined by the client data value.
	 */
	itemListFrame = XmVaCreateManagedFrame(panel, "itemListFrame",
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(itemListFrame, "itemListTitle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(itemListFrame, "itemRC",
		XmNradioBehavior, True,	
		XmNorientation, XmVERTICAL,
		NULL);

	for(n = 0; n < NITEM_BTNS; n++)
	{
		itemBtns[n] = XmVaCreateToggleButton(rc, "item", NULL);
		XtAddCallback(itemBtns[n], XmNvalueChangedCallback, sampling_cb, INT2PTR(n+1));
	}

	itemComboFrame = XmVaCreateFrame(panel, "itemComboFrame",
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(itemComboFrame, "itemListTitle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	itemComboList = XmVaCreateManagedComboBox(itemComboFrame, "itemList",
		NULL);
	XtAddCallback(itemComboList, XmNselectionCallback, sampling_cb, NULL);

	/* A predefined point selector */
	predefFrame = CreatePredefinedPointsSelector( panel, SELECT_EDIT_SAMPLE,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, itemListFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	/* A sample on a grid selector */
	sampleGrid = CreateGridSelector(panel, SELECT_EDIT_SAMPLE,
		XmNtopAttachment, XmATTACH_WIDGET, XmNtopWidget, predefFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	/* A "what font and colour do you want" selector */
	fontSetManager = CreateFontSelector(panel, set_font_attributes,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sampleGrid,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	data.font_type = GetFontSelectorValue(fontSetManager, SELECT_FONT_TYPE);
	data.font_size = GetFontSelectorValue(fontSetManager, SELECT_FONT_SIZE);
	data.colour    = GetFontSelectorValue(fontSetManager, SELECT_COLOUR);

	AddIngredObserver(sample_display_control);
}


/*============ LOCAL STATIC FUNCTIONS ===================================*/


/* As with all ingred messages parms[0] contains the CAL pointer */
static void sample_display_control(CAL cal, String *parms, int nparms)
{
	if(nparms < 3) return;
	if(!same_ic(parms[0],"SAMPLE") || !same_ic(parms[1],"DISPLAY")) return;

	if(same_ic(parms[2],"ON"))
		ACTIVATE_attributeDisplayDialog(E_SAMPLE, cal);
	else if(same_ic(parms[2],"OFF"))
		DEACTIVATE_attributeDisplayDialogs();
}


static void set_sampling_data(void)
{
	cancel_sampling();
	CopyStruct(&GV_active_field->sample, &data, SAMPLE, 1);
	if(GV_edit_mode)
		GV_active_field->sendEditCmdFcn(ACCEPT_MODE);
	else
		SendEditSampleCommand(NULL);
}


/* Send the SAMPLE CANCEL command to ingred */
static void cancel_sampling(void)
{
	if (!GV_active_field) return;
	if (blank(GV_active_field->sample.type)) return;
	if (blank(GV_active_field->sample.item)) return;

	IngredVaEditCommand(NullCal, NullCal, "SAMPLE CANCEL %s %s",
		GV_active_field->sample.type, GV_active_field->sample.item);
}


/*ARGSUSED*/
static void sampling_cb(Widget  w , XtPointer client_data , XtPointer call_data )
{
	int     ndx;
	Boolean sensitive;

	/* If client_data has a value then the callback is from a toggle button.
	 * If null, then the callback is from the combobox.
	 */
	if(client_data)
	{
		if(!XmToggleButtonGetState(w)) return;
		ndx = PTR2INT(client_data) - 1;
	}
	else
	{
		ndx = ((XmComboBoxCallbackStruct*)call_data)->item_position - 1;
		if(ndx < 0) return;
	}

	data.type = item_list[ndx].type;
	data.item = item_list[ndx].name;
	set_sampling_data();

	/* If we are asked to sample all attributes or field labels set
	*  the pre-defined point selector insensitive.
	*/
	sensitive =	(!same_ic(data.item, AttribAll) &&
				 !(same_ic(data.item, AttribFieldLabels) ||same_ic(data.item, AttribLinkNodes)));
	XtSetSensitive(predefFrame, sensitive);
	XtSetSensitive(sampleGrid,  sensitive);
}


/*ARGSUSED*/
static void set_font_attributes(FontSelectorStruct *font)
{
	data.font_type = font->type;
	data.font_size = font->size;
	data.colour    = font->colour;

	if(font->reason != SELECT_NONE)
	{
		XuVaStateDataSave(
			SAMPLE_PANEL, GV_active_field->info->element->name, GV_active_field->info->level->name,
			"\"%s\" %s \"%s\"", data.font_type, data.font_size, data.colour,
			NULL);
		set_sampling_data();
	}
}
