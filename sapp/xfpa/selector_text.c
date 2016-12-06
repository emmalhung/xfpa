/****************************************************************************/
/*
*  File:	 selector_text.c
*
*  Purpose:	 Provides a "Widget" to allow the specification of text strings
*            for insertion in such places as the scratchpad and so on.
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
/****************************************************************************/

#include <string.h>
#include <stdarg.h>
#include "global.h"
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include "resourceDefines.h"
#include "selector.h"

#define MY_NAME "textSelect"

typedef struct {
	Widget textDisplay;
	Widget fontType;
	Widget fontSize;
	Widget fontColour;
	int    ncolours;
	String *colours;
	String input_colour_string;
	void (*CBF)(TextSelectorStruct*);
	TextSelectorStruct data;
} TMS, *TMSP;


static Widget textEnterDialog = NullWidget;
static Widget textW           = NullWidget;



static void entered_text_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	TMSP tp = (TMSP)client_data;

	XtFree(tp->data.text);
	tp->data.reason = SELECT_TEXT_MODS;
	tp->data.text = XmTextFieldGetString(textW);
	tp->CBF(&tp->data);
	tp->data.reason = SELECT_TEXT_FOCUS;
	tp->CBF(&tp->data);

	XuWidgetLabel(tp->textDisplay, blank(tp->data.text)? " ":tp->data.text);
	XuDestroyDialog(textEnterDialog);
}


/*ARGSUSED*/
static void make_enter_text_dialog_cb(Widget w, XtPointer client_data, XtPointer unused)
{
	TMSP   data = (TMSP) client_data;

	static XuDialogActionsStruct action_items[] = {
		{"setBtn",    entered_text_cb,   NULL},
		{"cancelBtn", XuDestroyDialogCB, NULL}
	};

	action_items[0].data = (XtPointer) data;

	textEnterDialog = XuCreateFormDialog(data->textDisplay, "textEnterDialog",
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNkeyboardFocusPolicy, XmEXPLICIT,
		XuNdefaultActionItemId, "setBtn",
		XuNrelativePosition, True,
		XuNretainGeometry, XuRETAIN_NONE,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &textEnterDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		NULL);

	textW = XmVaCreateManagedTextField(textEnterDialog, "text",
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* This enables the text, set and cancel widgets to act as a tab group.
	 * That is the user can use the tab key to navigate between then.
	 */
	XtVaSetValues(XuGetActionAreaBtnByName(textEnterDialog, "setBtn"),
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
	XtVaSetValues(XuGetActionAreaBtnByName(textEnterDialog, "cancelBtn"),
			XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
			NULL);
	XtVaSetValues(textW, XmNnavigationType,
			XmEXCLUSIVE_TAB_GROUP,
			NULL);

	XuShowDialog(textEnterDialog);

	/* Ensure that the text box has focus when shown */
	XmProcessTraversal(textW, XmTRAVERSE_CURRENT);
}



/*ARGSUSED*/
static void being_destroyed_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	TMSP tp = (TMSP)client_data;
	FreeItem(tp->colours);
	FreeItem(tp->input_colour_string);
	FreeItem(tp->data.text);
	FreeItem(tp);
}


/*ARGSUSED*/
static void font_type_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	TMSP      tp;
	
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	tp = (TMSP)ptr;
	tp->data.reason = SELECT_FONT_TYPE;
	tp->data.font = (String)client_data;
	tp->CBF(&tp->data);
}


/*ARGSUSED*/
static void font_size_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	TMSP      tp;
	
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	tp = (TMSP)ptr;
	tp->data.reason = SELECT_FONT_SIZE;
	tp->data.size = (String)client_data;
	tp->CBF(&tp->data);
}


/*ARGSUSED*/
static void font_colour_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	TMSP      tp;
	
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	tp = (TMSP)ptr;
	tp->data.reason = SELECT_FONT_COLOUR;
	tp->data.colour = (String)client_data;
	tp->CBF(&tp->data);
}



void SetTextSelector(Widget selector, String font_type, String font_size, String font_colour)
{
	int       i, nfont_types, nfont_sizes;
	String    *font_types, *font_sizes;
	XtPointer ptr;
	TMSP       tp;

	if (IsNull(selector)) return;

	XtVaGetValues(selector, XmNuserData, &ptr, NULL);
	tp = (TMSP)ptr;
	if (IsNull(tp)) return;

	GetMapFontInfo(&font_types, &nfont_types, &font_sizes, NULL, &nfont_sizes);

	/* Initialize in case the input font information is not recognized */
	if(nfont_types  > 0) tp->data.font = font_types[0];
	if(nfont_sizes  > 0) tp->data.size = font_sizes[nfont_sizes/2];
	if(tp->ncolours > 0) tp->data.colour = tp->colours[0];

	for(i = 0; i < nfont_types; i++)
	{
		if(!same(font_type, font_types[i])) continue;
		tp->data.font = font_types[i];
		break;
	}

	for(i = 0; i < nfont_sizes; i++)
	{
		if(!same(font_size, font_sizes[i])) continue;
		tp->data.size = font_sizes[i];
		break;
	}

	for(i = 0; i < tp->ncolours; i++)
	{
		if(!same(font_colour, tp->colours[i])) continue;
		tp->data.colour = tp->colours[i];
		break;
	}

	if(nfont_types  > 0) XuMenuSelectItemByName(tp->fontType, tp->data.font);
	if(nfont_sizes  > 0) XuMenuSelectItemByName(tp->fontSize, tp->data.size);
	if(tp->ncolours > 0) XuMenuSelectItemByName(tp->fontColour, tp->data.colour);

	tp->data.reason = SELECT_NONE;
	tp->CBF(&tp->data);
}


Widget CreateTextSelector(Widget parent, PANEL_ID panel, void (*CBF)(), ...)
{
	int       ac, i, nfont_types, nfont_sizes;
	String    cmd, *font_types, *font_sizes, *font_size_labels;
	String    ptr, res;
	XmString  xmlabel;
	Widget    w, setBtn, topWidget;
	Widget    textForm;
	Arg       al[20];
	TMSP      ts;
	va_list   args;

	GetMapFontInfo(&font_types, &nfont_types, &font_sizes, &font_size_labels, &nfont_sizes);
		
	ts = OneMem(TMS);
	ts->CBF       = CBF;
	ts->data.text = NULL;
	ts->data.font = font_types[0];
	ts->data.size = font_sizes[nfont_sizes/2];

	/* Get colours from the resource database */
	switch(panel)
	{
		case GUIDANCE: res = RNsampleFontColors;	break;
		default:       res = RNscratchpadTextColours;
	}
	ptr  = XuGetStringResource(res, "black");
	ts->input_colour_string = XtNewString(ptr);
	ptr = strtok_arg(ts->input_colour_string);
	do {
		ts->colours = (String*)XtRealloc((void*)ts->colours, (ts->ncolours+1)*sizeof(String));
		ts->colours[ts->ncolours] = ptr;
		ts->ncolours++;
	} while((ptr = strtok_arg(NULL)));
	
    /* Apply the variable argument list.
    */
	ac = 0;
	XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	XtSetArg(al[ac], XmNuserData, (XtPointer)ts); ac++;
    va_start(args, CBF);
    while((cmd = va_arg(args, String)) != NULL && ac < 20)
    {
        XtSetArg(al[ac], cmd, va_arg(args, XtPointer)); ac++;
    }
    va_end(args);

	topWidget = XmCreateFrame(parent, MY_NAME, al, ac);
    XtAddCallback(topWidget, XmNdestroyCallback, being_destroyed_cb, (XtPointer)ts);

	(void) XmVaCreateManagedLabel(topWidget, "textLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	textForm = XmVaCreateForm(topWidget, "textForm",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		NULL);

	xmlabel = XmStringCreateLocalized(" ");
	ts->textDisplay = XmVaCreateManagedLabel(textForm, "textDisplay",
		XmNborderWidth, 1,
		XmNmarginHeight, 5,
		XmNmarginWidth, 5,
		XmNlabelString, xmlabel,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XmStringFree(xmlabel);

	setBtn = XmVaCreateManagedPushButton(textForm, "setBtn",
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, ts->textDisplay,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(setBtn, XmNactivateCallback, make_enter_text_dialog_cb, (XtPointer) ts);

	ts->fontType = XuVaMenuBuildOption(textForm, "fontType", NULL,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, setBtn,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < nfont_types; i++)
	{
		w = XuMenuAddButton(ts->fontType, font_types[i], 0, NoId, font_type_cb, font_types[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer)ts, NULL);
	}
	XuMenuSelectItemByName(ts->fontType, ts->data.font);

	ts->fontSize = XuVaMenuBuildOption(textForm, "fontSize", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, ts->fontType,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < nfont_sizes; i++)
	{
		w = XuMenuAddButton(ts->fontSize, font_sizes[i], font_size_labels[i], NoId,
									font_size_cb, font_sizes[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer)ts, NULL);
	}
	XuMenuSelectItemByName(ts->fontSize, ts->data.size);
	
	ts->fontColour = XuVaMenuBuildOption(textForm, "fontColor", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, ts->fontType,
		XmNtopOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < ts->ncolours; i++)
	{
		Pixmap ins, pix;
		pix = XuCreateColoredPixmap(parent, ts->colours[i], 32, 12);
		ins = XuCreateInsensitivePixmap(parent, pix);
		w = XuMenuAddPixmapButton(ts->fontColour, ts->colours[i], NoId, pix, ins,
									 font_colour_cb, (XtPointer)ts->colours[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer)ts, NULL);
	}
	ts->data.colour = ts->colours[0];
	XuMenuSelectItemByName(ts->fontColour, ts->data.colour);


	XtManageChild(textForm);
	XtManageChild(topWidget);
	return topWidget;
}
