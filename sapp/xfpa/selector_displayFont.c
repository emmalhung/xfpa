/*****************************************************************************
*
*  File:     selector_displayFont.c
*
*  Purpose:  Provides the font characteristics selection object to set
*            the sampling characteristics.
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
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <ingred.h>
#include "resourceDefines.h"
#include "editor.h"
#include "selector.h"

static int     ncolours = 0;
static String  *colours = NULL;

typedef struct {
	Widget             typeOption;
	Widget             sizeOption;
	Widget             colourOption;
	void               (*rtnFcn)(FontSelectorStruct*);
	FontSelectorStruct font;
} FIS;


/*ARGSUSED*/
static void font_type_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	FIS       *fip;

	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	fip = (FIS *)ptr;
	fip->font.reason = SELECT_FONT_TYPE;
	fip->font.type   = (String)client_data;
	fip->rtnFcn(&fip->font);
}


/*ARGSUSED*/
static void font_size_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	FIS       *fip;

	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	fip = (FIS *)ptr;
	fip->font.reason = SELECT_FONT_SIZE;
	fip->font.size   = (String)client_data;
	fip->rtnFcn(&fip->font);
}


/*ARGSUSED*/
static void font_colour_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	XtPointer ptr;
	FIS       *fip;
	Pixmap    pm;

	XtVaGetValues(w, XmNuserData, &ptr, XmNlabelPixmap, &pm, NULL);
	fip = (FIS *)ptr;
	fip->font.reason = SELECT_COLOUR;
	fip->font.colour = (String)client_data;
	fip->rtnFcn(&fip->font);
}


/*ARGSUSED*/
static void being_destroyed_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	FreeItem(client_data);
}



/*
 *  Set the given font selector values. When done the callback function is executed
 *  with a reason of select none so that the callback function can be coded to ignore
 *  any possible edit actions.
 */
void SetFontSelector(Widget selector, String font_type, String font_size, String colour)
{
	int       i, nfont_types, nfont_sizes;
	String    *font_types, *font_sizes;
	XtPointer ptr;
	FIS       *fip;

	if (IsNull(selector)) return;

	XtVaGetValues(selector, XmNuserData, &ptr, NULL);
	fip = (FIS *)ptr;
	if (IsNull(fip)) return;

	GetMapFontInfo(&font_types, &nfont_types, &font_sizes, NULL, &nfont_sizes);

	for(i = 0; i < nfont_types; i++)
	{
		if(!same(font_type, font_types[i])) continue;
		fip->font.type = font_types[i];
		XuMenuSelectItemByName(fip->typeOption, fip->font.type);
		break;
	}

	for(i = 0; i < nfont_sizes; i++)
	{
		if(!same(font_size, font_sizes[i])) continue;
		fip->font.size = font_sizes[i];
		XuMenuSelectItemByName(fip->sizeOption, fip->font.size);
		break;
	}

	for(i = 0; i < ncolours; i++)
	{
		if(!same(colour, colours[i])) continue;
		fip->font.colour = colours[i];
		XuMenuSelectItemByName(fip->colourOption, fip->font.colour);
		break;
	}

	fip->font.reason = SELECT_NONE;
	fip->rtnFcn(&fip->font);
}


String GetFontSelectorValue(Widget selector, SELECT_TYPE type)
{
	XtPointer ptr;
	FIS       *fip;

	if (IsNull(selector)) return NULL;

	XtVaGetValues(selector, XmNuserData, &ptr, NULL);
	fip = (FIS *)ptr;
	if (IsNull(fip)) return NULL;

	switch(type)
	{
		case SELECT_FONT_TYPE: return fip->font.type;
		case SELECT_FONT_SIZE: return fip->font.size;
		case SELECT_COLOUR:    return fip->font.colour;
	}

	return NULL;
}


/*=======================================================================*/
/*
*	CreateFontSelector() -  Create the font selection object. The fcn
*                           parameters are:
*
*   parent - The parent widget.
*   rtnFcn - The function to call when any of the attributes are changed.
*   font, size, colour - These are returned with the values as set when
*                        this function is called. Use for initializing
*                        the parent function.
*/
/*=======================================================================*/
Widget CreateFontSelector(Widget parent, void (*rtnFcn)(), ...)
{
	int	      ac, i, nfont_types, nfont_sizes;
	String    cmd, *font_types, *font_sizes, *font_size_labels;
	Arg       al[20];
	Widget    w, form, frame;
	FIS       *fi;
	va_list   args;

	static Boolean first = True;

	if (first)
	{
		String ptr, list;

		first = False;

		ptr  = XuGetStringResource(RNsampleFontColors, "Yellow");
		list = XtNewString(ptr);
		ncolours = 0;
		ptr = strtok_arg(list);
		do {
			colours = (String*)XtRealloc((void*)colours, (ncolours+1)*sizeof(String));
			colours[ncolours] = ptr;
			ncolours++;
		} while((ptr = strtok_arg(NULL)));
	}

	GetMapFontInfo(&font_types, &nfont_types, &font_sizes, &font_size_labels, &nfont_sizes);

	fi = OneMem(FIS);
	fi->rtnFcn = rtnFcn;

	ac = 0;
    XtSetArg(al[ac], XmNuserData, (XtPointer)fi);        ac++;
    XtSetArg(al[ac], XmNshadowType, XmSHADOW_ETCHED_IN); ac++;
    XtSetArg(al[ac], XmNresizable, False);               ac++;

    va_start(args, rtnFcn);
    while((cmd = va_arg(args, String)) != (String)NULL && ac < 20)
    {
		XtSetArg(al[ac], cmd, va_arg(args, XtPointer)); (ac)++;
    }
    va_end(args);

	frame = XmCreateFrame(parent, "setSampleFontManager", al, ac);
	XtAddCallback(frame, XmNdestroyCallback, being_destroyed_cb, (XtPointer)fi);

	(void) XmVaCreateManagedLabel(frame, "setSampleFont",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(frame, "sampleDisplay", NULL);

	fi->typeOption = XuVaMenuBuildOption(form, "fontType", NULL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < nfont_types; i++)
	{
		w = XuMenuAddButton(fi->typeOption, font_types[i], 0, NoId, font_type_cb, font_types[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer)fi, NULL);
	}
	fi->font.type = font_types[0];
	XuMenuSelectItemByName(fi->typeOption,  fi->font.type);

	fi->sizeOption = XuVaMenuBuildOption(form, "fontSize", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fi->typeOption,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < nfont_sizes; i++)
	{
		w = XuMenuAddButton(fi->sizeOption, font_sizes[i], font_size_labels[i], NoId,
									font_size_cb, (XtPointer)font_sizes[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer)fi, NULL);

	}
	fi->font.size = font_sizes[nfont_sizes/2];
	XuMenuSelectItemByName(fi->sizeOption,  fi->font.size);

	fi->colourOption = XuVaMenuBuildOption(form, "fontColor", NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fi->typeOption,
		XmNtopOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < ncolours; i++)
	{
		Pixmap ins, pix;
		pix = XuCreateColoredPixmap(parent, colours[i], 32, 12);
		ins = XuCreateInsensitivePixmap(parent, pix);
		w = XuMenuAddPixmapButton(fi->colourOption, colours[i], NoId, pix, ins,
									 font_colour_cb, (XtPointer)colours[i]);
		XtVaSetValues(w, XmNuserData, (XtPointer)fi, NULL);
	}
	fi->font.colour = colours[0];
	XuMenuSelectItemByName(fi->colourOption, fi->font.colour);


	XtManageChild(frame);

	return frame;
}
