/*============================================================================*/
/*
 *   File:      Menu.c
 *
 *   Purpose:   Motif menu creation and convenience routines
 *
 *   Functions: Contains the following functions
 *
 *               void    XuMenuAddButton()
 *               void    XuMenuAddPixmapButton()
 *               Widget  XuMenuAddShared()
 *               Widget  XuMenuBuildOption()
 *               Widget  XuMenuBuildPopup()
 *               Widget  XuMenuBuild()
 *               Widget  XuMenuBuildShared()
 *               int     XuMenuButtonGetId()
 *               void    XuMenuButtonSetSensitivity()
 *               Widget  XuMenuFindButton()
 *               Widget  XuMenuFindButtonByName()
 *               Widget  XuMenuFindByName()
 *               Widget  XuMenuFind()
 *               void    XuMenuMakeToggle()
 *               void    XuMenuMakeTearOff()
 *               void    XuMenuMakeHelp()
 *               void    XuMenuSelectItem()
 *               void    XuMenuSelectItemByName()
 *               int     XuMenuGetSelected()
 *               Boolean XuMenuToggleSelected()
 *               void    XuMenuToggleSetState()
 *               Boolean XuMenuToggleGetState()
 *
 *              Variable argument functions
 *
 *               Widget  XuVaMenuBuildOption()
 *               Widget  XuVaMenuBuildSharedOption()
 *
 *   Structures:
 *
 *  The id field in this structure is stored as XmNuserData for the created
 *  menu item. Setting it to less than 1 will make MenuBuild ignore it. The
 *  NoId define is for convienience.
 *
 *#define NoId -1
 *
 *
 * Options for the menu item. These may be or'ed together to provide more
 * than one type of funcionality for the menu item. 
 *
 *	#define XuMENU_NONE			(0)		* for when we have nothing                 *
 *	#define XuMENU_INSENSITIVE	(1L)	* initialize the menu item to insensitive  *
 *	#define XuMENU_SET			(1L<<1)	* set the item on - must be of type toggle *
 *	#define XuMENU_RADIO_LIST	(1L<<3)	* menu is a radio list type                *
 *	#define XuMENU_TEAR_OFF		(1L<<4)	* we want the menu to be tear off          *
 *
 *
 *	typedef struct _xu_menu_item{
 *		char                 *name;        * menu item name		                   *
 *		WidgetClass          *class;       * menu item class		               *
 *		unsigned long        options;      * XuMENU_... (see above)                *
 *		KeySym               mnemonic;     * menu item mnemonic (pulldowns only)   *
 *		int                  id;           * menu button id (must be > 0)          *
 *		XtCallbackProc       callback;     * menu callback                         *
 *		XtPointer            client_data;  * menu callback data                    *
 *		struct _xu_menu_item *subitems;    * pullright or menu bar menu items      *
 *	} XuMenuItemStruct, *XuMenuItem; 
 *
 *	typedef struct _xu_menu_bar_item {
 *		char          *name;               * menu bar item name                     *
 *		KeySym        mnemonic;            * menu item mnemonic                     *
 *		int           id;                  * menu button id (must be > 0)           *
 *		XuMenuItem    subitems;            * pulldown menu items                    *
 *	} XuMenuBarItemStruct, *XuMenuBarItem;
 *
 *
 *
 *   Note: The XmNuserData resource is used for storing the menu id's. If this
 *         is to be used for other purposes then the id ability of the menus
 *         will not be avaliable.
 *
 *         The numeric identifier which can be associated with each menu can
 *         have a value of 0 or greater. Internally is is stored as id + 1, so
 *         that there can be no ambiguity. A 0 value will mean that no id
 *         has been assigned.
 *
 *         If the class name of a button is xmDrawnButtonWidgetClass then it is
 *         assumed that an emulation of the look of a tear off button is wanted
 *         so that the tear off action can be emulated within the program. In this
 *         case a dashed line is drawn into the button and its resources are set
 *         to emulate the look of a tear off button.
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
/*============================================================================*/

#include <string.h>
#include <stdarg.h>
#include "XuP.h"
#include <Xm/CascadeBG.h> 
#include <Xm/CascadeB.h> 
#include <Xm/DrawnB.h>
#include <Xm/DrawP.h>
#include <Xm/MenuShell.h> 
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h> 
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>


#define PULLDOWN_EXTENSION	"_pulldown"
#define OPTION_EXTENSION	"_option"



/*============== PRIVATE FUNCTIONS =================*/


/* We use this procedure to ensure that the visual, depth and colormap of the
 * pulldown menu shell agrees with the parent. If this is not the case then
 * you get an X error complaining about mismatched characteristics if the
 * visual of the program is not the default visual of the display. This is
 * handled in Motif 2.x but not in 1.x, so we keep this here for both
 * versions of Motif. Also, the given name is appended with the appropriate
 * extension except in the case of the popup which uses the name.
 */
static Widget create_popup_menu(Widget _parent, String _name, int _type)
{
	int      depth;
	Widget   mp;
	Colormap colormap;
	Visual   *visual;
	char     mbuf[256];

	XtVaGetValues(XuGetShell(_parent),
		XtNvisual,   &visual,
		XtNdepth,    &depth,
		XtNcolormap, &colormap,
		NULL);

	(void)sprintf(mbuf, "%s_popup", _name);
	mp = XtVaCreatePopupShell(mbuf, xmMenuShellWidgetClass, _parent, 
		XtNdepth, depth,
		XtNvisual, visual,
		XtNcolormap, colormap,
		XtNwidth, 5,
		XtNheight, 5,
		XmNallowShellResize, True,
		XtNoverrideRedirect, True,
		NULL);

	(void) safe_strcpy(mbuf, _name);
	switch(_type)
	{
		case XmMENU_PULLDOWN:
			(void) safe_strcat(mbuf, PULLDOWN_EXTENSION);
			break;

		case XmMENU_OPTION:
			(void) safe_strcat(mbuf, OPTION_EXTENSION);
			_type = XmMENU_PULLDOWN;
			break;
	}

	return (XtVaCreateWidget(mbuf, xmRowColumnWidgetClass, mp, XmNrowColumnType, _type, NULL));
}


/* Callback to draw our fake tear off menu button by using a drawn button.
 * The code here is somewhat inefficient as it creates and uses the GC's
 * but given that this is not called all that often it is not worth while
 * getting fancy.
 */
/*ARGSUSED*/
static void tear_redraw(Widget w, XtPointer cld, XtPointer cd)
{
	Dimension width, height, shadow_thickness, margin;
	Pixel     foreground, background, top_shadow, bottom_shadow;
	XGCValues values;
	XtGCMask  valueMask;
	GC        sgc, tsc, bsc;

	XtVaGetValues(w,
		XmNheight, &height,
		XmNwidth, &width,
		XmNshadowThickness, &shadow_thickness,
		XmNmarginHeight, &margin,
		XmNforeground, &foreground,
		XmNbackground, &background,
		XmNtopShadowColor, &top_shadow,
		XmNbottomShadowColor, &bottom_shadow,
		NULL);

	valueMask = GCForeground | GCBackground | GCLineStyle;
	values.foreground = foreground;
	values.background = background;
	values.line_style = LineDoubleDash;
	sgc = XtGetGC(w, valueMask, &values);

	valueMask = GCForeground;
	values.foreground = top_shadow;
	tsc = XtGetGC(w, valueMask, &values);

	values.foreground = bottom_shadow;
	bsc = XtGetGC(w, valueMask, &values);

	XmeDrawSeparator(XtDisplay(w), XtWindow(w), tsc, bsc, sgc, 0, 0, width, height,
		shadow_thickness, margin, XmHORIZONTAL, XmSHADOW_ETCHED_OUT_DASH);

	XtReleaseGC(w, sgc);
	XtReleaseGC(w, tsc);
	XtReleaseGC(w, bsc);
}


/* Common functionality to build the XuMenuItems for the menu creation functions below
 */
static void build_menu_items(Widget menu, int _type, XuMenuItem _items)
{
	int        i, argc;
	Widget     widget; 
	String     resource;
	Arg        args[5];
	XuMenuItem m;

	/* add all menu items */
	for(i = 0; _items[i].name != NULL; i++) 
	{ 
		m = _items + i;

		argc = 0;

		/* check for pull-rights */
		if(m->subitems) 
		{
			if(_type == XmMENU_PULLDOWN)
			{
				widget = XuMenuBuild(menu, m->name, m->mnemonic, m->subitems); 
			}
			else if(_type == XmMENU_POPUP)
			{
				widget = XuMenuBuildPopup(menu, m->name, m->subitems);
			}
			else
			{
				(void)fprintf(stderr, "%s: submenus not allowed in option menus\n", m->name);
				continue;
			}
			if(m->options)
			{
				if(m->options & XuMENU_RADIO_LIST) XuMenuMakeToggle(widget);
				if(m->options & XuMENU_TEAR_OFF  ) XuMenuMakeTearOff(widget);
			}
		}
		else
		{
			widget = XtCreateManagedWidget(m->name, *m->class, menu, args, argc);
		}

		if(m->class == &xmSeparatorGadgetClass)  continue;
		if(m->class == &xmSeparatorWidgetClass)  continue;

		/* If the input class is a drawn button assume that we want to emulate
		 * a tear off button but handle the tear off ourselves, probably because
		 * we want to programatically show the tear off menu.  This is done by 
		 * using a drawn button and drawing a separator ito it (like the tear off
		 * button does).  
		 */
		if(m->class == &xmDrawnButtonWidgetClass)
		{
			XtVaSetValues(widget, XmNpushButtonEnabled, True, XmNmarginHeight, 5, NULL);
			XtAddCallback(widget, XmNexposeCallback, tear_redraw, NULL);
			XtAddCallback(widget, XmNresizeCallback, tear_redraw, NULL);
		}

		/* configure the new menu item */
		argc = 0;

		if(m->callback) 
		{
			if(m->class == &xmToggleButtonWidgetClass || m->class == &xmToggleButtonGadgetClass)
				resource = XmNvalueChangedCallback;
			else
				resource = XmNactivateCallback;

			XtAddCallback(widget, resource, m->callback, m->client_data);
		}
		/* mnemonic */
		if(m->mnemonic != None) 
		{
			XtSetArg(args[argc], XmNmnemonic, m->mnemonic); argc++;
		}

		/* button id */
		if(m->id > NoId)
		{
			XtSetArg(args[argc], XmNuserData, INT2PTR(m->id+1)); argc++;
		}

		if(m->options)
		{
			if(m->options & XuMENU_INSENSITIVE)
			{
				XtSetArg(args[argc], XmNsensitive, False); argc++;
			}
			if(m->options & XuMENU_SET)
			{
				XtSetArg(args[argc], XmNset, True); argc++;
			}
		}

		/* set them */
		if(argc) XtSetValues(widget, args, argc);
	} 
}


static void get_children(Widget _menu, WidgetList *_childs, int *_num_childs)
{ 
	Widget w = (Widget)0; 

	*_childs = NULL; 
	*_num_childs = 0; 

	if (XtIsSubclass(_menu, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(_menu, xmCascadeButtonGadgetClass))
	{ 
		XtVaGetValues(_menu,
			XmNsubMenuId, &w,
			NULL);
		XtVaGetValues(w,
			XmNchildren,    _childs,
			XmNnumChildren, _num_childs,
			NULL);
	} 
	else if (XtIsSubclass(_menu, xmRowColumnWidgetClass))
	{ 
		XtVaGetValues(_menu,
			XmNchildren,    _childs,
			XmNnumChildren, _num_childs,
			NULL);
	} 
} 


static Boolean is_option_menu(Widget _menu, String _module)
{
	Boolean rtn = False;

	if(XtIsSubclass(_menu, xmRowColumnWidgetClass))
	{
		Widget sid = (Widget)0;
		unsigned char type;
		XtVaGetValues(_menu,
			XmNrowColumnType, &type,
			XmNsubMenuId,     &sid,
			NULL);

		if(!sid)
			(void)fprintf(stderr, "%s: Menu \"%s\" has no child.\n", _module, XtName(_menu));
		else if(type == XmMENU_OPTION)
			rtn = True;
	}
	if (!rtn) (void)fprintf(stderr, "%s: menu \"%s\" is not option type.", _module, XtName(_menu));
	return rtn;
}



/* Recursive search function to find a menu by its name. This is separate from
 * the main functions as _name is the name passed into the function and then
 * appended by the appropriate suffix.
 */
static Widget find_menu_by_name(Widget _parent, String _name)
{ 
	Widget     w;
	WidgetList childs;
	int        n, num_child;

	/* If this is a cascade button check it's subMenuId widget.
	*/
	if (XtIsSubclass(_parent, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(_parent, xmCascadeButtonGadgetClass))
	{
		XtVaGetValues(_parent, XmNsubMenuId, &w, NULL); 
		if(strcmp(_name, XtName(w)) == 0) return w;
		_parent = w;
	}

	/* Once here we must have a row column widget of some sort to proceed.
	*/
	if (!XtIsSubclass(_parent, xmRowColumnWidgetClass)) return (Widget)NULL;

	/* Scan all of the children of the row column for a match (recursive).
	*/
	XtVaGetValues(_parent,
		XmNchildren, &childs,
		XmNnumChildren, &num_child,
		NULL);

	w = (Widget)0;
	for( n = 0; n < num_child; n++ )
	{
		w = find_menu_by_name(childs[n], _name);
		if (w) break;
	}
	return w;
} 



/* Find the pulldown menu corresponding to the input menu. If it is a cascade button
 * we get its subMenuId. If a row column, we check to see if it is an option type
 * and if so get its subMenuId. At this point we finally return the information.
 */
static Boolean find_menu(String module, Widget _menu, Widget *_rtn, Boolean *_is_option)
{
	unsigned char type;
	Widget        menu = _menu;

	if(!menu)
	{
		(void)fprintf(stderr, "%s: Menu widget is NULL", module);
		return False;
	}

	if (XtIsSubclass(menu, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(menu, xmCascadeButtonGadgetClass))
	{
		XtVaGetValues(menu, XmNsubMenuId, &menu, NULL);
		if(!menu)
		{
			(void)fprintf(stderr, "%s: cascade button \"%s\" has no pulldown", module, XtName(_menu));
			return False;
		}
	}

	if (!XtIsSubclass(menu, xmRowColumnWidgetClass))
	{
		(void)fprintf(stderr, "%s: Menu \"%s\" is not of type RowColumn.\n", module, XtName(_menu));
		return False;
	}

	XtVaGetValues(menu, XmNrowColumnType, &type, NULL);

	/* If we are an option menu we can get the subMenuId directly.
	 */
	if(type == XmMENU_OPTION)
	{
		XtVaGetValues(menu, XmNsubMenuId, &menu, NULL);
		if(!menu)
		{
			(void)fprintf(stderr, "%s: option menu \"%s\" has no pulldown", module, XtName(_menu));
			return False;
		}
	}

	*_rtn = menu;
	if (_is_option) *_is_option = (type == XmMENU_OPTION);

	return True;
}



/* In Motif 1.x, option menus with buttons having pixmaps do not automatically
 * handle the calls properly so we must do it for them. This function is as a
 * callback to buttons with pixmap labels.
*/
#if (XmVERSION < 2)
/*ARGSUSED*/
static void label_pixmaps_cb(Widget _w, XtPointer _client_data, XtPointer _unused)
{
	int    type;
	Pixmap lpx, ipx;

	XtVaGetValues(_w,
		XmNlabelType, &type,
		NULL);

	if(type == XmPIXMAP)
	{
		XtVaGetValues(_w,
			XmNlabelPixmap, &lpx,
			XmNlabelInsensitivePixmap, &ipx,
			NULL);

		XtVaSetValues(XmOptionButtonGadget((Widget)_client_data),
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, lpx,
			XmNlabelInsensitivePixmap, ipx,
			NULL);
	}
}
#endif



/*===================== PUBLIC FUNCTIONS =======================*/



/*********************************************************************************/
/*
*  Name:         XuMenuBuildMenuBar
*
*  Return Type:  Widget
*
*  Description:  Creates an unmanaged menu bar.
*
*  In: 
*     parent   - parent for this menu
*     title    - title for this menu
*     items    - menu descriptions, see menu.h and note below
*
*  Returns:
*     The created menu bar.
*/
/*********************************************************************************/
Widget XuMenuBuildMenuBar(Widget _parent, String _title, XuMenuBarItem _items)
{
	int    i;
	Widget menu, w;

	menu = XtVaCreateWidget(_title, xmRowColumnWidgetClass, _parent,
		XmNrowColumnType, XmMENU_BAR,
		NULL);

	/* add all menu items */
	for(i = 0; _items[i].name != NULL; i++) 
	{
		w = XuMenuBuild(menu, _items[i].name, _items[i].mnemonic, _items[i].subitems);

		if(_items[i].id > NoId)
		{
			XtVaSetValues(w, XmNuserData, INT2PTR(_items[i].id+1), NULL); 
		}
	}

	return menu;
}



/*********************************************************************************/
/*
*  Name:         XuMenuBuild
*
*  Return Type:  Widget
*
*  Description:  multiple pulldown menu creation routine
*
*  In: 
*     parent   - parent for this menu
*     title    - title for this menu
*     mnemonic - shortcut key for this menu
*     items    - menu descriptions, see menu.h
*
*  Returns:
*     On success, the created menu else NULL.
*/
/*********************************************************************************/
Widget XuMenuBuild(Widget _parent, String _title, KeySym _mnemonic, XuMenuItem _items) 
{ 
	int        argc = 0;
	Widget     menu, cascade; 
	Arg        args[2];

	menu = create_popup_menu(_parent, _title, XmMENU_PULLDOWN);

	XtSetArg(args[argc], XmNsubMenuId, menu); argc++;
	if(_mnemonic != None)
	{
		XtSetArg(args[argc], XmNmnemonic, _mnemonic); argc++;
	}
	cascade = XtCreateManagedWidget(_title, xmCascadeButtonWidgetClass, _parent, args, argc);

	if (_items) build_menu_items(menu, XmMENU_PULLDOWN, _items);

	return(cascade);
} 





/*********************************************************************************/
/*
*  Name:         XuVaMenuBuildOption
*
*  Return Type:  Widget
*
*  Description:  Creates an option menu from an (optional) data structure.
*
*  In: 
*     parent   - parent for this menu
*     title    - title for this menu
*     items    - menu descriptions, see menu.h
*     ...      - Xm style argument value pairs terminated by a NULL. For example
*
*                   XuMenuItemStruct menu_args[] = { .... };
*
*     				XuVaMenuBuildOption( parent, "my_option", menu_args,
*                       XmNtopAttachment, XmATTACH_FORM,
*                       XmNtopWidget, w,
*                       NULL );
*
*                If XmNlabelString is passed through explicitely as NULL, then
*                the label gadget of the option menu is unmanaged. For example:
*
*                   XuVaMenuBuildOption( parent, ...
*                       XmNlabelString, NULL,
*                       ...
*                       NULL);
*
*                This is useful if there are label resources for the widget name
*                in the resource file, but they are not wanted in this particular
*                instance.
*
*  Returns:
*     created option menu
*/
/*********************************************************************************/
Widget XuVaMenuBuildOption(Widget _parent, String _title, XuMenuItem _items, ...) 
{ 
	int      manage = 1, argc = 0;
	Arg      args[50];
	Widget   menu, option; 
	String   argname;
	Boolean  is_label = True;
	XmString xmlabel = (XmString)NULL;
	va_list  ap;
	
	va_start(ap,_items);
	while((argname = va_arg(ap,String)) != (String)NULL && argc < 49)
	{
		if(strcmp(argname, XuNmanageChild) == 0)
		{
			manage = va_arg(ap, int);
		}
		else if(strcmp(argname, XmNlabelString) == 0)
		{
			xmlabel  = va_arg(ap, XmString);
			is_label = NotNull(xmlabel);
			if (is_label) { XtSetArg(args[argc], XmNlabelString, xmlabel); argc++; }
		}
		else
		{
			XtSetArg(args[argc], argname, va_arg(ap,XtArgVal)); argc++;
		}
	}
	va_end(ap);

	menu = create_popup_menu(_parent, _title, XmMENU_OPTION);
	XtSetArg(args[argc], XmNsubMenuId, menu); argc++;
	option = XmCreateOptionMenu(_parent, _title, args, argc);

	/* Has labelString has been assigned? If not remove the label gadget */
	if (is_label) XtVaGetValues(option, XmNlabelString, &xmlabel, NULL);
	if (_items)   build_menu_items(menu, XmMENU_OPTION, _items);
	if (!xmlabel) XtUnmanageChild(XmOptionLabelGadget(option));
	if (manage)   XtManageChild(option);

	return option;
} 



/*********************************************************************************/
/*
*  Name:         XuMenuBuildOption
*
*  Return Type:  Widget
*
*  Description:  Creates an option menu from an (optional) data structure.
*
*  In: 
*     parent - parent for this menu
*     title  - title for this menu
*     items  - menu descriptions, see menu.h
*     al     - argument list
*     ac     - length of argument list.
*
*  Note:
*
*    If XmNlabelString is passed through explicitely as NULL, then the label
*    gadget of the option menu is unmanaged. For example:
*
*        XtSetArg(al[ac], XmNlabelString, NULL); ac++;
*
*    This is useful if there are label resources for the widget name in the
*    resource file, but they are not wanted in this particular instance.
*
*  Returns:
*     created option menu
*/
/*********************************************************************************/
Widget XuMenuBuildOption(Widget _parent, String _title, XuMenuItem _items, ArgList al, int ac)
{ 
	int      n, argc;
	Arg      args[50];
	Widget   menu, option; 
	Boolean  manage = True, is_label = True;
	XmString xmlabel = (XmString)0;

	for( argc = 0, n = 0; n < ac; n++ )
	{
		if(strcmp(al[n].name, XuNmanageChild) == 0)
		{
			manage = (Boolean)al[n].value;
		}
		else if(strcmp(al[n].name, XmNlabelString) == 0)
		{
			is_label = (al[n].value != 0);
			if(is_label)
			{
				args[argc].name  = al[n].name;
				args[argc].value = al[n].value;
				argc++;
			}
		}
		else
		{
			args[argc].name  = al[n].name;
			args[argc].value = al[n].value;
			argc++;
		}
	}

	menu = create_popup_menu(_parent, _title, XmMENU_OPTION);
	XtSetArg(args[argc], XmNsubMenuId, menu); argc++;
	option = XmCreateOptionMenu(_parent, _title, args, argc);

	/* Has labelString has been assigned? If not remove the label gadget */
	if (is_label) XtVaGetValues(option, XmNlabelString, &xmlabel, NULL);
	if (!xmlabel) XtUnmanageChild(XmOptionLabelGadget(option));
	if (_items)   build_menu_items(menu, XmMENU_OPTION, _items);
	if (manage)   XtManageChild(option);

	return option;
} 



/*********************************************************************************/
/*
*  Name:         XuMenuBuildPopup
*
*  Return Type:  Widget
*
*  Description:  popup menu type creation routine
*
*  In: 
*     parent   - parent for this menu
*     title    - title for this menu
*     items    - menu descriptions, see menu.h
*
*  Returns:
*     the created menu
*/
/*********************************************************************************/
Widget XuMenuBuildPopup(Widget _parent, String _title, XuMenuItem _items) 
{ 
	Widget menu = create_popup_menu(_parent, _title, XmMENU_POPUP);

	if (_items) build_menu_items(menu, XmMENU_POPUP, _items);

	return(menu);
} 



/*********************************************************************************/
/*
*  Name:         XuMenuBuildShared
*
*  Return Type:  Widget
*
*  Description:  creates a "floating" menu suitable for sharing between
*                multiple menubars
*
*  In: 
*    parent - parent for this menu
*    title  - name of this menu
*    items  - items for this menu
*
*  Returns:
*    the created menu
*/
/*********************************************************************************/
Widget XuMenuBuildShared(Widget _parent, String _title, XuMenuItem _items)
{
	Widget menu = create_popup_menu(_parent, _title, XmMENU_PULLDOWN);

	if (_items) build_menu_items(menu, XmMENU_PULLDOWN, _items);

	return(menu);
}



/*********************************************************************************/
/*
*  Name:         XuMenuAddShared
*
*  Return Type:  Widget
*
*  Description:  creates a menubar entry and adds the given menu to it.
*
*  In: 
*     parent      - parent for this menu
*     title       - title for this menu
*     mnemonic    - shortcut key for this menu
*     shared_menu - the shared menu to add (created with above function)
*
*  Returns:
*     The new menubar entry 
*/
/*********************************************************************************/

Widget XuMenuAddShared(Widget _parent, String _title, KeySym _mnemonic, Widget _shared_menu)
{
	int argc = 0;
	Arg args[2];

	/* create menu triggerer */
	XtSetArg(args[argc], XmNsubMenuId, _shared_menu); argc++;
	if(_mnemonic != None)
	{
		XtSetArg(args[argc], XmNmnemonic, _mnemonic); argc++;
	}
	return(XtCreateManagedWidget(_title, xmCascadeButtonWidgetClass, _parent, args, argc));
}




/*********************************************************************************/
/*
*  Name:         XuVaMenuBuildSharedOption
*
*  Return Type:  Widget
*
*  Description:  Create an option menu with the given shared_menu as the pulldown.
*
*  Notes:        There is only a variable argument form of this function as option
*                menus are usually parented and thus require argument pairs for
*                attachments and such.
*
*  In: 
*     parent      - parent for this menu
*     title       - title for this menu
*     shared_menu - the shared menu to add (created with above function)
*     ...         - Xm style argument value pairs terminated by a NULL. For example
*
*                      XuMenuItemStruct menu_args[] = { .... };
*
*     				   XuVaMenuBuildOption( parent, "my_option", menu_args,
*                          XmNtopAttachment, XmATTACH_FORM,
*                          XmNtopWidget, w,
*                          NULL );
*
*                If XmNlabelString is passed through explicitely as NULL, then
*                the label gadget of the option menu is unmanaged. For example:
*
*                   XuVaMenuBuildOption( parent, ...
*                       XmNlabelString, NULL,
*                       ...
*                       NULL);
*
*                This is useful if there are label resources for the widget name
*                in the resource file, but they are not wanted in this particular
*                instance.
*
*  Returns:
*     The new option menu
*/
/*********************************************************************************/
Widget XuVaMenuBuildSharedOption(Widget _parent, String _title, Widget _shared_menu, ...)
{
	int      manage = 1, argc = 0;
	Arg      args[50];
	Widget   option; 
	String   argname;
	Boolean  is_label = True;
	XmString xmlabel  = (XmString)NULL;
	va_list  ap;
	
	va_start(ap,_shared_menu);
	while((argname = va_arg(ap,String)) != (String)NULL && argc < 50)
	{
		if(strcmp(argname, XuNmanageChild) == 0)
		{
			manage = va_arg(ap, int);
		}
		else if(strcmp(argname, XmNlabelString) == 0)
		{
			xmlabel  = va_arg(ap, XmString);
			is_label = NotNull(xmlabel);
			if (is_label) { XtSetArg(args[argc], XmNlabelString, xmlabel); argc++; }
		}
		else
		{
			XtSetArg(args[argc], argname, va_arg(ap,XtArgVal)); argc++;
		}
	}
	va_end(ap);
	XtSetArg(args[argc], XmNsubMenuId, _shared_menu); argc++;
	option = XmCreateOptionMenu(_parent, _title, args, argc);

	/* Check for a label string. If none unmanage the appropriate widget.
	 */
	if (is_label) XtVaGetValues(option, XmNlabelString, &xmlabel, NULL);
	if (!xmlabel) XtUnmanageChild(XmOptionLabelGadget(option));
	if (manage)   XtManageChild(option);

	return option;
}



/*******************************************************************************/
/*
*  Name:         XuMenuFind
*
*  Return Type:  Widget
*
*  Description:  Find a menu by the id of the cascade button that activates it.
*
*  In: 
*     parent - widget to use a starting reference
*     id     - id of menu cascade button to find. If the menu is an option menu
*              there is only one pulldown and id is ignored so set it to NoId.
*
*  Returns:
*      widget found or NULL on failure.
*/
/*******************************************************************************/
Widget XuMenuFind(Widget _parent, int _id)
{ 
	Widget        w;
	WidgetList    childs;
	int           n, id, num_child;
	unsigned char type;
	XtPointer     pid;

	/* If this is a cascade button check it's id widget.
	*/
	if (XtIsSubclass(_parent, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(_parent, xmCascadeButtonGadgetClass))
	{
		pid = (XtPointer)NoId;
		XtVaGetValues(_parent,
			XmNuserData, &pid,
			XmNsubMenuId, &w,
			NULL);
		id = PTR2INT(pid) - 1;
		if(id == _id) return w;
		_parent = w;
	}

	/* Once here we must have a row column widget of some sort to proceed.
	*/
	if (!XtIsSubclass(_parent, xmRowColumnWidgetClass)) return (Widget)0;

	/* Scan all of the children of the row column for a match (recursive).
	*/
	num_child = 0;
	XtVaGetValues(_parent,
		XmNchildren, &childs,
		XmNnumChildren, &num_child,
		XmNrowColumnType, &type,
		NULL);

	/* If we are an option menu we can get the subMenuId directly.
	 */
	if(type == XmMENU_OPTION)
	{
		XtVaGetValues(_parent, XmNsubMenuId, &w, NULL);
	}
	else
	{
		w = (Widget)0;
		for( n = 0; n < num_child; n++ )
		{
			w = XuMenuFind(childs[n], _id);
			if (w) break;
		}
	}
	return w;
} 



/*******************************************************************************/
/*
*  Name:         XuMenuFindByName
*
*  Return Type:  Widget
*
*  Description:  Find a menu by name
*
*  In: 
*     parent - widget to use a starting reference
*     name   - name of menu to find
*
*  Returns:
*      widget found or NULL on failure.
*
*  Note: If there is more than one menu with the same name, the function will
*        find the first occurance within the parent tree.
*/
/*******************************************************************************/
Widget XuMenuFindByName(Widget _parent, String _name)
{
	char menu_name[256];
	(void) safe_strcpy(menu_name, _name);
	(void) safe_strcat(menu_name, PULLDOWN_EXTENSION);
	return find_menu_by_name(_parent, menu_name);
}



/*********************************************************************************/
/*
*  Name:         XuMenuFindButton
*
*  Return Type:  Widget
*
*  Description:  Find a widget identified by Id in a menu.
*
*  In: 
*     menu - menu to search.
*     id   - id of widget to find
*
*  Returns:
*      widget found or NULL on failure.
*/
/*******************************************************************************/

Widget XuMenuFindButton(Widget _menu, int _id)
{
	int        n, id, num_childs;
	WidgetList childs;
	Widget     w;
	XtPointer  pid;
	
	get_children(_menu, &childs, &num_childs);

	for( n = 0; n < num_childs; n++ )
	{ 
    	pid = (XtPointer)NoId;
		XtVaGetValues(childs[n], XmNuserData, &pid, NULL);
		id = PTR2INT(pid) - 1;
		if(id == _id) return(childs[n]);	

		if (XtIsSubclass(childs[n], xmCascadeButtonWidgetClass) || 
			XtIsSubclass(childs[n], xmCascadeButtonGadgetClass))
		{
			w = XuMenuFindButton(childs[n], _id);
			if (w) return w;
		}
	} 
	return(NULL);
} 


/*******************************************************************************/
/*
*  Name:         XuMenuFindButtonByName
*
*  Return Type:  Widget
*
*  Description:  Find a named widget in a menu
*
*  In: 
*     wlist - list of widgets to search
*     name  - name of widget to find
*
*  Returns:
*      widget found or NULL on failure.
*/
/*******************************************************************************/
Widget XuMenuFindButtonByName(Widget _menu, String _name)
{ 
	int        n, num_childs;
	WidgetList childs;
	
	get_children(_menu, &childs, &num_childs);

	for( n = 0; n < num_childs; n++ )
	{
		if(strcmp(XtName(childs[n]),_name) == 0) return childs[n];

		if (XtIsSubclass(childs[n], xmCascadeButtonWidgetClass) || 
			XtIsSubclass(childs[n], xmCascadeButtonGadgetClass))
		{
			Widget w = XuMenuFindButtonByName(childs[n], _name);
			if (w) return w;
		}
	}
	return(NULL);
} 


/*******************************************************************************/
/*
*  Name:         XuMenuButtonGetId
*
*  Return Type:  int
*
*  Description:  This routine returns the member id of the given widget by
*                returning the XmNuserData field of the widget
*
*  In: 
*     button - button for which to fetch the userData value
*
*  Returns:
*     value of the XmNuserData resource
*/
/*******************************************************************************/
int XuMenuButtonGetId(Widget _button)
{   
	XtPointer pid = (XtPointer)NoId;
	XtVaGetValues(_button, XmNuserData, &pid, NULL);
    return (PTR2INT(pid) - 1);
}       


/*******************************************************************************/
/*
*  Name:         XuMenuMakeToggle
*
*  Return Type:  void
*
*  Description:  Make a menu with togglebuttons act as a radiobox
*
*  In: 
*     menu - menu of which to set state
*/
/*******************************************************************************/
void XuMenuMakeToggle(Widget _menu)
{ 
	Widget w = NULL; 

	if (XtIsSubclass(_menu, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(_menu, xmCascadeButtonGadgetClass))
	{ 
		XtVaGetValues(_menu, XmNsubMenuId, &w, NULL);
		if (w) XtVaSetValues(w, XmNradioBehavior, True, XmNradioAlwaysOne, True, NULL);
	}
	else
	{
		(void) fprintf(stderr, "XuMenuMakeToggle: menu %s is no subclass "
			"of CascadeButton\n", XtName(_menu));
	}
}


/*******************************************************************************/
/*
*  Name:         XuMenuMakeTearOff
*
*  Return Type:  void
*
*  Description:  Make a menu a tear off menu
*
*  In: 
*     menu - menu to make tear off
*/
/*******************************************************************************/
void XuMenuMakeTearOff(Widget _menu)
{ 
	Widget w = NULL; 

	if (XtIsSubclass(_menu, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(_menu, xmCascadeButtonGadgetClass))
	{ 
		XtVaGetValues(_menu, XmNsubMenuId, &w, NULL);
		XtVaSetValues(w, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL); 
	}
	else
	{
		(void) fprintf(stderr, "XuMenuMakeTearOff: menu %s is no subclass "
			"of CascadeButton\n", XtName(_menu));
	}
}


/*********************************************************************************/
/*
*  Name:         XuMenuMakeHelp
*
*  Return Type:  void
*
*  Description:  convinience function to set the help pulldown
*
*  In: 
*     menu  - Widget (return from MenuBuild) to set as help pulldown
*/
/*********************************************************************************/
void XuMenuMakeHelp(Widget _btn)
{
	if (XtIsSubclass(_btn, xmCascadeButtonWidgetClass) || 
		XtIsSubclass(_btn, xmCascadeButtonGadgetClass))
	{
		XtVaSetValues(XtParent(_btn), XmNmenuHelpWidget, _btn, NULL);
	}
	else
	{
		(void) fprintf(stderr, "XuMenuMakeHelp: menu %s is no subclass "
			"of CascadeButton\n", XtName(_btn));
	}
}


/*******************************************************************************/
/*
*  Name:         XuMenuAddButton
*
*  Return Type:  Widget
*
*  Description:  Adds a button gadget to a pulldown.
*
*  In: 
*     menu  - the pulldown menu pane of a pulldown menu or the cascade button
*             which activates it, or an option menu
*     name  - name to assign to the button
*     label - label to assign to the button. If NULL then name is used.
*     id    - button id as an XtPointer type. This stored in XmNuserData.
*     proc  - button callback function
*     cbd   - client data for callback
*
*  Return:
*     The button gadget widget.
*/
/*******************************************************************************/
Widget XuMenuAddButton(	Widget _menu, String _name, String _label, int _id,
								XtCallbackProc proc, XtPointer cbd)
{
	int           argc = 0;
	Arg           args[2];
	Widget        btn;
	XmString      xmlabel = (XmString)NULL;
	const String  module  = "XuMenuAddButton";

	if(!find_menu(module, _menu, &_menu, NULL)) return NULL;

	if(_label)
	{
		xmlabel = XuNewXmString(_label);
		XtSetArg(args[argc], XmNlabelString, xmlabel); argc++;
	}
	if(_id > NoId)
	{
		XtSetArg(args[argc], XmNuserData, INT2PTR(_id+1)); argc++;
	}
	btn = XtCreateManagedWidget(_name, xmPushButtonWidgetClass, _menu, args, argc);
	XtAddCallback(btn, XmNactivateCallback, proc, cbd);

	if(xmlabel) XmStringFree(xmlabel);
	return btn;
}




/*******************************************************************************/
/*
*  Name:         XuMenuAddCascadeButton
*
*  Return Type:  Widget
*
*  Description:  Adds a cascade button to a pulldown.
*
*  In: 
*     menu  - the pulldown menu pane of a pulldown menu or the cascade button
*             which activates it, or an option menu
*     name  - name to assign to the button
*     label - label to assign to the button. If NULL then name is used.
*     id    - button id as an XtPointer type. This stored in XmNuserData.
*     wid   - widget of the menu that the cascase button operates on
*
*  Return:
*     The button widget.
*/
/*******************************************************************************/
Widget XuMenuAddCascadeButton(	Widget _menu, String _name, String _label, int _id, Widget _wid )
{
	int           argc = 0;
	Arg           args[3];
	Widget        btn;
	XmString      xmlabel = (XmString)NULL;
	const String  module  = "XuMenuAddCascadeButton";

	if(!find_menu(module, _menu, &_menu, NULL)) return NULL;

	if(_label)
	{
		xmlabel = XuNewXmString(_label);
		XtSetArg(args[argc], XmNlabelString, xmlabel); argc++;
	}
	if(_id > NoId)
	{
		XtSetArg(args[argc], XmNuserData, INT2PTR(_id+1)); argc++;
	}
	XtSetArg(args[argc], XmNsubMenuId, _wid); argc++;
	btn = XtCreateManagedWidget(_name, xmCascadeButtonWidgetClass, _menu, args, argc);

	if(xmlabel) XmStringFree(xmlabel);
	return btn;
}





/*******************************************************************************/
/*
*  Name:         XuMenuAddPixmapButton
*
*  Return Type:  Widget
*
*  Description:  Adds a button gadget, containing pixmaps, to a pulldown.
*
*  In: 
*     menu - the pulldown menu pane of a pulldown menu or the cascade button
*            which activates it, or an option menu
*     name - name to assign to the button
*     id   - button id as an XtPointer type. This stored in XmNuserData.
*     lpx  - label pixmap
*     ipx  - insensitive pixmap
*     proc - button callback function
*     cbd  - client data for callback
*
*  Return:
*     The button gadget widget.
*/
/*******************************************************************************/
Widget XuMenuAddPixmapButton(	Widget _menu, String _name, int _id, Pixmap _lpx, Pixmap _ipx,
								XtCallbackProc _proc, XtPointer _cbd)
{
	int           argc = 0;
	Arg           args[4];
	Boolean       is_option;
	Widget        menu, btn;
	const String  module = "XuMenuAddPixmapButton";

	if(!find_menu(module, _menu, &menu, &is_option)) return NULL;

	if(_lpx)
	{
		XtSetArg(args[argc], XmNlabelType, XmPIXMAP); argc++;
		XtSetArg(args[argc], XmNlabelPixmap, _lpx); argc++;
		if(_ipx)
		{
			XtSetArg(args[argc], XmNlabelInsensitivePixmap, _ipx); argc++;
		}
	}

	if(_id > NoId)
	{
		XtSetArg(args[argc], XmNuserData, INT2PTR(_id+1)); argc++;
	}

	btn = XtCreateManagedWidget(_name, xmPushButtonWidgetClass, menu, args, argc);
	XtAddCallback(btn, XmNactivateCallback, _proc, _cbd);

#if (XmVERSION < 2)
	if (is_option) XtAddCallback(btn, XmNactivateCallback, label_pixmaps_cb, (XtPointer)_menu);
#endif

	return btn;
}



/*******************************************************************************/
/*
*  Name:         XuMenuAddSeparator
*
*  Return Type:  Widget
*
*  Description:  Adds a separator to a pulldown.
*
*  In: 
*     menu  - the pulldown menu pane of a pulldown menu or the cascade button
*             which activates it, or an option menu
*     type  - the separator type
*
*  Return:
*     The button gadget widget.
*/
/*******************************************************************************/
Widget XuMenuAddSeparator(	Widget _menu, unsigned char type )
{
	int           argc = 0;
	Arg           args[2];
	const String  module  = "XuMenuAddSeparator";

	if(!find_menu(module, _menu, &_menu, NULL)) return NULL;

	if(type)
	{
		XtSetArg(args[argc], XmNseparatorType, type); argc++;
	}
	return  XtCreateManagedWidget("menuSep", xmSeparatorWidgetClass, _menu, args, argc);
}


/*******************************************************************************/
/*
*  Name:         XuMenuSelectItem
*
*  Return Type:  void
*
*  Description:  selects the given option menu button by id
*
*  In: 
*     menu - menu of which to select a menu item
*     id   - id of menu item to set
*/
/*******************************************************************************/
void XuMenuSelectItem(Widget _menu, int _id)
{
	Widget pb;

	const String module = "XuMenuSelectItem";

	if(!is_option_menu(_menu, module)) return;

	/* get correct menu entry */
	if((pb = XuMenuFindButton(_menu, _id)) == NULL)
	{
		(void) fprintf(stderr, "%s: can not select requested item \"%s\": id out of range.\n",
				module, XtName(_menu));
		return;
	}

	/* Set it */
	XtVaSetValues(_menu, XmNmenuHistory, pb, NULL);

#if (XmVERSION < 2)
{
	int type;
	XtVaGetValues(pb, XmNlabelType, &type, NULL);

	if(type == XmPIXMAP)
	{
		Pixmap lpx, ipx;

		XtVaGetValues(pb,
			XmNlabelPixmap, &lpx,
			XmNlabelInsensitivePixmap, &ipx,
			NULL);

		XtVaSetValues(XmOptionButtonGadget(_menu),
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, lpx,
			XmNlabelInsensitivePixmap, ipx,
			NULL);
	}
	else
	{
		XmString label;
		XtVaGetValues(pb, XmNlabelString, &label, NULL);
		XtVaSetValues(XmOptionButtonGadget(_menu),
			XmNlabelType, XmSTRING,
			XmNlabelString, label,
			NULL);
	}
}
#endif
}


/*******************************************************************************/
/*
*  Name:         XuMenuSelectItemByName
*
*  Return Type:  void
*
*  Description:  selects the given option menu button by name
*
*  In: 
*     menu - menu of which to select a menu item
*     name - name of menu item to set
*/
/*******************************************************************************/
void XuMenuSelectItemByName(Widget _menu, String _name)
{
	Widget pb;

	const String module = "XuMenuSelectItemByName";

	if(!is_option_menu(_menu, module)) return;

	pb = XuMenuFindButtonByName(_menu, _name);
	if (!pb)
	{
		(void) fprintf(stderr, "%s: can not select menu \"%s\": name \"%s\" unrecognized.\n",
			module, XtName(_menu), _name);
		return;
	}

	XtVaSetValues(_menu, XmNmenuHistory, pb, NULL);

#if (XmVERSION < 2)
{
	int type;
	XtVaGetValues(pb, XmNlabelType, &type, NULL); 

	if(type == XmPIXMAP)
	{
		Pixmap lpx, ipx;

		XtVaGetValues(pb,
			XmNlabelPixmap, &lpx,
			XmNlabelInsensitivePixmap, &ipx,
			NULL);

		XtVaSetValues(XmOptionButtonGadget(_menu),
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, lpx,
			XmNlabelInsensitivePixmap, ipx,
			NULL);
	}
	else
	{
		XmString label;
		XtVaGetValues(pb, XmNlabelString, &label, NULL);
		XtVaSetValues(XmOptionButtonGadget(_menu),
			XmNlabelType, XmSTRING,
			XmNlabelString, label,
			NULL);
	}
}
#endif
}



/*******************************************************************************/
/*
*  Name:         XuMenuButtonSetSensitivity
*
*  Return Type:  void
*
*  Description:  Set the sensitivity state of an item in a option menu
*
*  In: 
*     menu  -  option menu id
*     id    -  id of menu item of which the state is to be set.
*              Zero based index.
*     state -  state of the item.
*/
/*******************************************************************************/
void XuMenuButtonSetSensitivity(Widget _menu, int _id, Boolean _state)
{
	Widget pb = XuMenuFindButton(_menu, _id);
	if (pb) XtSetSensitive(pb, _state);
}


/*******************************************************************************/
/*
*  Name:         XuMenuButtonSetVisibility
*
*  Return Type:  void
*
*  Description:  Set the visibility state of an item in a option menu
*
*  In: 
*     menu  -  option menu id
*     id    -  id of menu item of which the state is to be set.
*              Zero based index.
*     state -  True  - item is visible
*              False - item is not visible
*/
/*******************************************************************************/
void XuMenuButtonSetVisibility(Widget _menu, int _id, Boolean _state)
{
	Widget pb = XuMenuFindButton(_menu, _id);
	if (!pb) return;

	if (_state) XtManageChild(pb);
	else        XtUnmanageChild(pb);
}



/*******************************************************************************/
/*
*  Name:         XuMenuGetSelected
*
*  Return Type:  int
*
*  Description:  Return the id of the selected option menu item
*
*  In: 
*     menu - Option menu of which to return the selected menu item
*
*  Returns:
*     the id (XmNuserData field) of the selected menu item. NoId upon failure.
*/
/*******************************************************************************/
int XuMenuGetSelected(Widget _menu)
{
	int id = NoId;

	if(is_option_menu(_menu, "XuMenuGetSelected"))
	{
		Widget item = (Widget)NULL;
		XtVaGetValues(_menu, XmNmenuHistory, &item, NULL);
		if(item)
		{
			XtPointer pid;
			XtVaGetValues(item, XmNuserData, &pid, NULL);
			id = PTR2INT(pid) - 1;
		}
	}
	return(id);
}


void XuMenuClear( Widget menu )
{
	int        n;
	Cardinal   numKids  = 0;
	WidgetList kids = NULL;

	if (!menu) return;

	XtVaGetValues(menu, XmNnumChildren, &numKids, XmNchildren, &kids, NULL);
	for( n = numKids-1; n >= 0; n--)
		XtDestroyWidget(kids[n]);
}


/*============== Toggle menu specific convenience routines ===================*/


/*******************************************************************************/
/*
*  Name:         XuMenuToggleSelected
*
*  Return Type:  Boolean
*
*  Description: See if a toggle button is checked
*
*  In: 
*     toggle - toggle button widget to check
*
*  Returns:
*     selection state of toggle
*/
/*******************************************************************************/
Boolean XuMenuToggleSelected(Widget _toggle)
{
	Boolean value = False;
	XtVaGetValues(_toggle,  XmNset, &value, NULL);
	return(value);
}


/*******************************************************************************/
/*
*  Name:            XuMenuToggleSetState
*
*  Return Type:     void
*
*  Description:     sets the selection state of a togglebutton.
*
*  In: 
*     menu  - menu of which an item has to be set;
*     id    - id of menubutton to be set;
*     state - new toggle state;
*/
/*******************************************************************************/
void XuMenuToggleSetState(Widget _menu, int _id, Boolean _state)
{
	Widget toggle;

	/* get correct menu entry */
	if((toggle = XuMenuFindButton(_menu, _id)) == NULL)
	{
		(void) fprintf(stderr, "XuMenuToggleSetState:"
			" can not select requested menu: requested id out of range.\n");
		return;
	}
	XtVaSetValues(toggle, XmNset, _state, NULL); 
}


/*******************************************************************************/
/*
*  Name:         XuMenuToggleGetState
*
*  Return Type:  void
*
*  Description:  gets the selection state of a togglebutton.
*
*  In: 
*     menu - menu for which an item has to be checked;
*     id   - id of menubutton to be checked;
*
*  Returns:
*     False when it's not selected, True otherwise.
*/
/*******************************************************************************/
Boolean XuMenuToggleGetState(Widget _menu, int _id)
{
	Widget  toggle;
	Boolean value = False;

	/* get correct menu entry */
	if((toggle = XuMenuFindButton(_menu, _id)) == NULL)
	{
		(void) fprintf(stderr, "XuMenuToggleGetState:"
			" can not get requested menu: requested id out of range.\n");
		return(False);
	}
	XtVaGetValues(toggle, XmNset, &value, NULL);
	return(value);
}
