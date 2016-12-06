/*========================================================================*/
/*
*	File:		selector_color.c
*
*	Purpose:	Creates the palette from which the user can select color
*               and line style.  The return is the top level manager widget.
*               The parameters are:
*
*                   parent   - the widget to use as the parent widget.
*                   toggle   - if true the line style is a toggle else it
*                              acts as a push button.
*                   CBF(ColorSelectorStruct *)- callback function 
*                   ...      - variable argument list into which we can
*                              put widget attachments.
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
#include <stdarg.h>
#include <Xm/Label.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include "resourceDefines.h"
#include "global.h"
#include "selector.h"

#define MY_NAME "colorSelect"

/* The avaliable line keys. There need to be pixmap file which will
*  be used to generate pixmaps of these lines. The files must be
*  named, for example, "line.thin.solid.xpm"
*/
static int    nlines  = 9;
static String lines[] = { "thin.solid" ,"medium.solid" ,"thick.solid" ,
						 "thin.dashed","medium.dashed","thick.dashed",
						 "thin.dotted","medium.dotted","thick.dotted"
					   };


/* Define the structure to hold the creation data of a given
*  instance of the colour selector widget. NOTE that the number of
*  pixmaps needs to be the same as the number of lines.
*/
typedef struct {
	Widget              indicatorW;
	Widget              lineW;
	Pixmap              pixmaps[9];
	int                 ncolours;
	String              *colours;
	String              input_colour_string;
	void                (*CBF)(ColorSelectorStruct*);
	ColorSelectorStruct data;
} CSM, *CSMP;



/*ARGSUSED*/
static void being_destroyed_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	int   i;
	CSMP  cp;
	
	cp = (CSMP)client_data;
	for(i = 0; i < nlines; i++)
		XuFreePixmap(cp->lineW, cp->pixmaps[i]);
	FreeItem(cp->input_colour_string);
	FreeItem(cp->colours);
	FreeItem(cp);
}


/*ARGSUSED*/
static void color_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	CSMP      cp;
	
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	cp = (CSMP)ptr;
	cp->data.reason = SELECT_COLOUR;
	cp->data.colour = (String)client_data;
	cp->CBF(&cp->data);
}


/*ARGSUSED*/
static void style_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	CSMP      cp;
	
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	cp = (CSMP)ptr;
	cp->data.reason = SELECT_STYLE;
	cp->data.style = (String)client_data;
	cp->CBF(&cp->data);
}


/* When toggle is true this will change the background color of the
*  indicator widget when one of the color push buttons is activated.
*/
/*ARGSUSED*/
static void indicator_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	XtPointer ptr;
	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	XtVaSetValues(((CSMP)ptr)->indicatorW, XmNbackground, XuLoadColor(w, (String)client_data), NULL);
}


/* Return the current value of the selected line style and colour depending on
 * the type argument.
 */
String GetColorSelectorValue(Widget selector, SELECT_TYPE type)
{
	XtPointer ptr;
	CSMP      cp;

	if(!selector) return NULL;

	XtVaGetValues(selector, XmNuserData, &ptr, NULL);
	cp = (CSMP)ptr;
	if (!cp) return NULL;

	switch(type)
	{
		case SELECT_STYLE:  return cp->data.style;
		case SELECT_COLOUR: return cp->data.colour;
	}
	return NULL;
}


/* Set the selector according to the entered style and colour. Note that
 * we search our internal lists and set pointers to these lists. Once
 * done call the callback function with a reason of SELECT_NONE so that
 * the callback function can be coded to not send a message to ingred.
 */
void SetColorSelector(Widget selector, String style, String colour)
{
	int       n;
	XtPointer ptr;
	CSMP      cp;

	if(!selector) return;

	XtVaGetValues(selector, XmNuserData, &ptr, NULL);
	cp = (CSMP)ptr;
	if (!cp) return;

	for( n = 0; n < cp->ncolours; n++ )
	{
		if(!same(colour, cp->colours[n])) continue;
		cp->data.colour = cp->colours[n];
		XtVaSetValues(cp->indicatorW, XmNbackground, XuLoadColor(selector, cp->data.colour), NULL);
		break;
	}

	for( n = 0; n < nlines; n++ )
	{
		if(!same(style, lines[n])) continue;
		cp->data.style = lines[n];
		XuMenuSelectItemByName(cp->lineW, cp->data.style);
		break;
	}

	cp->data.reason = SELECT_NONE;
	cp->CBF(&cp->data);
}


Widget CreateColorSelector(Widget parent, PANEL_ID panel, Boolean toggle, void (*CBF)(), ...)
{
	int       ac, i, depth, offset;
	Dimension spacing;
	char      mbuf[128];
	String    cmd, c, fname, res;
	XtPointer ptr;
	Widget    w, manager, palette, paletteControl;
	XmString  blank_label;
	Pixel     fg, bg;
	Arg       al[20];
	CSMP      cp;
	va_list   args;

	blank_label = XuNewXmString(" ");

	/* Initialize the input data structure
	*/
	cp  = OneMem(CSM);
	cp->CBF         = CBF;
	cp->data.style  = lines[0];

	switch (panel)
	{
		case SCRATCHPAD:
			res = RNscratchpadLineColours;
			spacing = 14;
			offset  = 11;
			break;
		default:
			res = RNguidanceColours;
			spacing = 5;
			offset  = 2;
			break;
	}
	cp->input_colour_string = XtNewString(XuGetStringResource(res,"White"));
	c = strtok_arg(cp->input_colour_string);
	while(c)
	{
		cp->colours = MoreStringArray(cp->colours, cp->ncolours+1);
		cp->colours[cp->ncolours] = c;
		cp->ncolours++;
		c = strtok_arg(NULL);
	}
	cp->data.colour = cp->colours[0];

	ac = 0;
	XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
	XtSetArg(al[ac], XmNuserData, (XtPointer) cp); ac++;
	va_start(args, CBF);
	while((cmd = va_arg(args, String)))
	{
		ptr = va_arg(args, XtPointer);
		XtSetArg(al[ac], cmd, ptr); ac++;
	}
	va_end(args);

	paletteControl = XmCreateFrame(parent, MY_NAME, al, ac);
	XtAddCallback(paletteControl, XmNdestroyCallback, being_destroyed_cb, (XtPointer) cp);

	(void) XmVaCreateManagedLabel(paletteControl, "label",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	palette = XmVaCreateForm(paletteControl, "palette",
		XmNhorizontalSpacing, spacing,
		XmNverticalSpacing, 5,
		NULL);

	if (toggle)
	{
		cp->indicatorW = XmVaCreateManagedLabel(palette, "ci",
			XmNbackground, XuLoadColor(paletteControl, cp->colours[0]),
			XmNheight, 15,
			XmNborderWidth, 1,
			XmNlabelString, blank_label,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 8,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
	}

	manager = XmVaCreateBulletinBoard(palette, "colorManager",
		XmNheight, ((cp->ncolours-1)/6)*23+21,
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		XmNtopAttachment, toggle ? XmATTACH_WIDGET:XmATTACH_FORM,
		XmNtopOffset, toggle ? 7:9,
		XmNtopWidget, toggle ? cp->indicatorW:NULL,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	/* Create the color selection buttons.
	*/
    for( i = 0; i < cp->ncolours; i++ )
	{
		w = XmVaCreateManagedPushButton(manager, "cbtn",
			XmNuserData, (XtPointer) cp,
			XmNx, (i%6)*23,
			XmNy, (i/6)*23,
			XmNwidth, 19,
			XmNheight, 19,
			XmNshadowThickness, 0,
			XmNborderWidth, 1,
			XmNbackground, XuLoadColor(paletteControl, cp->colours[i]),
			XmNfillOnArm, False,
			XmNlabelString, blank_label,
			NULL );
		XtAddCallback(w, XmNarmCallback, color_cb, cp->colours[i]);
		if (toggle) XtAddCallback(w, XmNarmCallback, indicator_cb, cp->colours[i]);
	}
	XmStringFree(blank_label);
	XtManageChild(manager);

	fg = XuLoadColorResource(parent, RNforeground,"White");
	bg = XuLoadColorResource(parent, RNbackground,"Black");

	/* The left offset needs to be slightly different from the form horizontal
	 * spacing setting due to the way the option menu widget is structured.
	 */
	cp->lineW = XuVaMenuBuildOption(palette, "lineStyle", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, manager,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, offset,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtVaGetValues(XuGetShell(cp->lineW), XmNdepth, &depth, NULL);
	for(i = 0; i < nlines; i++)
	{
		(void) snprintf(mbuf, sizeof(mbuf), "line.%s", lines[i]);
		cp->pixmaps[i] = XuGetPixmap(cp->lineW, mbuf);
		w = XuMenuAddPixmapButton(cp->lineW, lines[i], NoId, cp->pixmaps[i], 0, style_cb, (XtPointer)lines[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer) cp, NULL);
	}
	XuMenuSelectItemByName(cp->lineW, lines[0]);

	XtManageChild(palette);
	XtManageChild(paletteControl);

	return paletteControl;
}
