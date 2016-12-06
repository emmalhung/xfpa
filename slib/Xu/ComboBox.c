/*======================================================================*/
/**
 *    \file Combobox.c
 *
 *    \brief These are convienience functions that work with Motif
 *           Vertsion 2.x ComboBox widget and provide an ascii input
 *           to the functions rather than XmString.
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
/*======================================================================*/

#include "XuP.h"
#include <Xm/ComboBox.h>
#include <Xm/List.h>
#include <Xm/TextF.h>

/**
 * \brief Selects an item in a combo box by its name.
 *
 * \param[in] w			The combobox widget
 * \param[in] item		The list item to select
 * \param[in] notify	If True activate the callback function upon selection
 */
void XuComboBoxSelectItem(Widget w, String item, const Boolean notify)
{
	Widget   list, text;
    XmString xmitem;

	XtVaGetValues(w, XmNlist, &list, XmNtextField, &text, NULL);
	xmitem = XmStringCreateSimple(item);
	if(XmListItemPos(list, xmitem) > 0)
	{
		XmListSelectItem(list, xmitem, notify);
		XmTextFieldSetString(text, item);
	}
	XmStringFree(xmitem);
	XmComboBoxUpdate(w);
}

/**
 * \brief Add an item to a combobox
 *
 * \param[in] w		The combobox widget
 * \param[in] item	The item to add
 * \param[in] pos	The position in the list at which to add the
 *                  item. Pos of 0 puts it at the end of the list.
 */
void XuComboBoxAddItem(Widget w, String item, int pos)
{
	XmString xmitem = XmStringCreateSimple(item);
	XmComboBoxAddItem(w, xmitem, pos, True);
	XmStringFree(xmitem);
}

/**
 * \brief Add a list of items to a combobox
 *
 * \param[in] w				The combobox widget
 * \param[in] items			The list of items
 * \param[in] item_count	The number of items
 * \param[in] pos           The position in the list at which to start adding
 *                          the item list. A pos of 0 adds the list at the end.
 */
void XuComboBoxAddItems(Widget w, String *items, int item_count, int pos)
{
	Widget   list;
	XmString *xmitems, *xmlist, olditem = NULL;
	int      n, nlist;
	XtEnum   mode;

	XtVaGetValues(w,
		XmNlist, &list,
		XmNitems, &xmlist,
		XmNitemCount, &nlist,
		XmNselectedPosition, &n,
		XmNpositionMode, &mode,
		NULL);

	if(mode == XmONE_BASED) n--;

	if(n >= 0 && nlist > 0)
		olditem = XmStringCopy(xmlist[n]);

	xmitems = (XmString *)XtCalloc(item_count, sizeof(XmString));
	for( n = 0; n < item_count; n++ ) xmitems[n] = XmStringCreateSimple(items[n]);
	XmListAddItems(list, xmitems, item_count, pos);
	for( n = 0; n < item_count; n++ ) XmStringFree(xmitems[n]);
	XtFree((void*)xmitems);
	if(olditem)
	{
		XmListSelectItem(list, olditem, False);
		XmStringFree(olditem);
	}
	XmComboBoxUpdate(w);
}

/**
 * \brief Delete all of the items in a combobox.
 *
 * \param[in] w	The combobox
 */
void XuComboBoxDeleteAllItems(Widget w)
{
	Widget list, text;
	XtVaGetValues(w, XmNlist, &list, XmNtextField, &text, NULL);
    XmListDeleteAllItems(list);    
	XmTextFieldSetString(text, "");
    XmComboBoxUpdate(w);
}


/**
 * \brief Return the position in the list where item is found.
 *
 * \param[in] w		The combobox
 * \param[in] item	The item the position is for
 */
int XuComboBoxItemPos(Widget w, String item)
{
	int      n;
	XmString xmitem;
	Widget   list;
	XtVaGetValues(w, XmNlist, &list, NULL);
	xmitem = XmStringCreateSimple(item);
    n = XmListItemPos(list, xmitem);
	XmStringFree(xmitem);
	return n;
}

/**
 * \brief Select the item at the given position in the list. Note that
 *        if more than one item in the list is selected this function
 *        will deselect everything but the position requested.
 *
 * \param[in] w			The combobox
 * \param[in] pos		The position to make active
 * \param[in] notify	Activate the callback when the selection is made
 */
void XuComboBoxSelectPos(Widget w, int pos, Boolean notify)
{
	int      n;
	XtEnum   mode;
	String   str;
	XmString *items;
	Widget   list, text;

	XtVaGetValues(w,
		XmNlist, &list, 
		XmNtextField, &text, 
		XmNpositionMode, &mode,
		NULL);
	XmListDeselectAllItems(list);
	XmListSetPos(list, pos);
	XmListSelectPos(list, pos, notify);
	if( !notify )  /* put selection into text field ourselves */
	{
		XmTextFieldSetString(text, "");
		XtVaGetValues(list, XmNitems, &items, XmNitemCount, &n, NULL);
		if(mode == XmONE_BASED) pos--;
		if(n > 0 && pos >= 0 && pos < n && XmStringGetLtoR(items[pos], XmSTRING_DEFAULT_CHARSET, &str))
		{
			XmTextFieldSetString(text, str);
			XtFree(str);
		}
	}
	XmComboBoxUpdate(w);
}


/**
 * \brief Return the position in the list of the selected item.
 */
int XuComboBoxGetSelectedPos(Widget w)
{
	int pos;
	XtVaGetValues(w, XmNselectedPosition, &pos, NULL);
	return (pos);
}

/**
 * \brief Set the given value into the text field of the combobox.
 *
 * \param[in] w		The combobox
 * \param[in] value	The string to insert into the text field.
 */
void XuComboBoxSetString(Widget w, String value)
{
	Widget text;
	XtVaGetValues(w, XmNtextField, &text, NULL);

	if(value == NULL || *value == 0)
		XmTextFieldSetString(text, "");
	else
		XmTextFieldSetString(text, value);
}
