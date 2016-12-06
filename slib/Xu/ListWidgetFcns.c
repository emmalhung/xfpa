/*================================================================*/
/*
 * Purpose:  Utility functions for handling lists. These take
 *           char* strings instead of XmString.
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

#include "XuP.h"
#include <Xm/List.h>

void XuListEmpty(Widget _w )
{
	XtVaSetValues(_w,
		XmNitemCount, 0,
		XmNitems, NULL,
		XmNselectedItemCount, 0,
		XmNselectedItems, NULL,
		NULL);
}

void XuListSetToItem(Widget _w, String _item)
{
	XmString item[1];

	item[0] = XuNewXmString(_item);
	XtVaSetValues(_w, XmNselectedItemCount, 1, XmNselectedItems, item, NULL);
	XmStringFree(item[0]);
}

void XuListAddItem(Widget _w, String _item)
{
	XmString item = XuNewXmString(_item);
	XmListAddItem(_w, item, 0);
	XmStringFree(item);
}


/* Load the given list with the item list.
 */
void XuListLoad(Widget _w, String *_items, int _nitems, int _visible )
{
	int n, ac;
	XmString *xm_items;
	Arg al[4];

	/* Clear the list */
	ac = 0;
	XtSetArg(al[ac], XmNitemCount,         0   ); ac++;
	XtSetArg(al[ac], XmNitems,             NULL); ac++;
	XtSetArg(al[ac], XmNselectedItemCount, 0   ); ac++;
	XtSetArg(al[ac], XmNselectedItems,     NULL); ac++;
	XtSetValues(_w, al, ac);

	xm_items = XTCALLOC(_nitems, XmString);
	for(n = 0; n < _nitems; n++) xm_items[n] = XuNewXmString(_items[n]);

	ac = 0;
	XtSetArg(al[ac], XmNitemCount, _nitems ); ac++;
	XtSetArg(al[ac], XmNitems,     xm_items); ac++;

	/* Only change visible item count if we have a valid value */
	if(_visible > 0)
	{
		if(_visible > _nitems) _visible = _nitems;
		XtSetArg(al[ac], XmNvisibleItemCount, _visible); ac++;
	}
	XtSetValues(_w, al, ac);

	for(n = 0; n < _nitems; n++) XmStringFree(xm_items[n]);
	XtFree((void*)xm_items);
}

void XuListMakePosVisible(Widget list_w, int item_no)
{
	int top, visible;

	XtVaGetValues(list_w,
		XmNtopItemPosition, &top,
		XmNvisibleItemCount, &visible,
		NULL);
	if( item_no < top)
		XmListSetPos(list_w, item_no);
	else if( item_no >= top+visible)
		XmListSetBottomPos(list_w, item_no);
}
