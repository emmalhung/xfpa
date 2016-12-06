/*================================================================*/
/*
*   File:       Fonts.c
*
*   Functions:
*
*	  XuExtendFontList() - Add a series of named fonts and a font
*                          tag to the existing font list.
*
*     XuGetWidgetFontHeight() - Get height of widget font.
*
*     XuGetWidgetFontStruct() - Get font structure used in widget.
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
/*================================================================*/

#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include "XuP.h"


/*==================== Public Functions ==========================*/


XmFontList XuExtendFontList(XmFontList fontlist, String name, String tag)
{
	XmFontListEntry entry = XmFontListEntryLoad(DefaultAppDisplayInfo->display, name, XmFONT_IS_FONT, tag);
	if(entry)
	{
		fontlist = XmFontListAppendEntry(fontlist, entry);
		XtFree((void*)entry);
	}
	else
	{
		(void) fprintf(stderr,"XuExtendFontList: Font \"%s\" not found.\n", name);
	}
	return fontlist;
}


int XuGetWidgetFontHeight(Widget _w)
{
	XFontStruct *fs = XuGetWidgetFontStruct(_w);
	return (fs->ascent + fs->descent);
}


XFontStruct *XuGetWidgetFontStruct(Widget _w)
{
	String          *font_names;
	XmFontList      font_list;
	XmFontContext   font_context;
	XmFontListEntry font_entry;
	XtPointer       font_return;
	XmFontType      font_type;
	XFontSet        font_set;
	XFontStruct     **font_struct_list;
	XFontStruct     *font_struct = (XFontStruct *)NULL;

	XtVaGetValues(_w, XmNfontList, &font_list, NULL);

	if(XmFontListInitFontContext(&font_context, font_list))
	{
		font_entry  = XmFontListNextEntry(font_context);
		font_return = XmFontListEntryGetFont(font_entry, &font_type);

		switch(font_type)
		{
			case XmFONT_IS_FONT:
				font_struct = (XFontStruct *)font_return;
				break;

			case XmFONT_IS_FONTSET:
				font_set = (XFontSet)font_return;
				(void)XFontsOfFontSet(font_set, &font_struct_list, &font_names);
				font_struct = font_struct_list[0];
				break;
		}
		XmFontListFreeFontContext(font_context);
	}
	return font_struct;
}

