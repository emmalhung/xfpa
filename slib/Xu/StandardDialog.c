/*==========================================================================*/
/*
*	File: StandardDialog. Contains the code to create "standard" dialogs.
*
*   Dscription:
*
*      Standard dialogs are organized as follows: the bottom of the dialog
*      consists of action area buttons with a separator above them. Above
*      the separator is the widget of the class specified which is actually
*      the widget id returned. The function also takes care of setting the
*      cursor on the created dialog, setting up the proper cursor on the
*      parent if the created dialog is modal and saving the position and
*      size info to a state file so that the next time the dialog is
*      activated its position and/or size will be restored to what they were
*      the last time the dialog was active. Note that because the returned
*      widget is not that of the dialog, special functions must be used
*      to show and hide the dialog, not the standard Xt ones. See below.
*
*   Functions:
*
*      Widget XuCreateDialog(Widget refw, WidgetClass class, String id, ...)
*      Widget XuCreateToplevelDialog(Widget refw, WidgetClass class, String id, ...)
*
*       refw       - reference widget used for placement
*       class      - class of active area widget in the dialog.
*       id         - dialog id as required by resource file
*       ...        - a NULL terminated list of resource-value pairs.
*
*      Resources special to the dialog, with defaults in (), are:
*
*      XuNactionAreaItems
*        - A list of action area structures of type XuDialogActionsStruct.
*
*      XuNnumActionAreaItems
*        - The number of items in the action list.
*
*      XuNallowIconify
*        - Dialog is allowed to be iconified (False).
*
*      XuNdefaultActionItemId
*        - The id of the action item to be shown as the selection default.
*
*      XuNdestroyCallback
*        - Function to call when dialog is being destroyed.
*
*      XuNdestroyData
*        - Data to pass to destroy function.
*
*      XuNmwmDeleteOverride
*        - The given function will override the default action of the window manager
*          close window action (normally the x on the upper part of a window). It
*          must be in the form of a standard callback function.
*
*      XuNdialogDisplay
*        - Display dialog is to appear on (as host:dyp.screen),
*
*      XuNdialogID
*        - Normally the dialog name is used as the key for the profile and for
*          the dialog title in the resource database. Sometimes it is useful
*          to allow for alternate ids for an alternate title and geometry
*          specification. This, for example, would be the case when a single
*          dialog is used for multiple purposes. These ids can specified by
*          XuNdialogID. The id can also be divided into two parts by using
*          XuDIALOG_ID_PART_SEPARATOR (a null terminated string) to divide the
*          parts. The part before the string is for the alternate dialog title
*          and geometry for resource purposes and the entire thing is used to
*          save the geometry in the state files. Id example:
*          sprintf(id, "%s%s%s",alt_id, XuDIALOG_ID_PART_SEPARATOR, detail)
*
*      XuNiconPixmapFile
*        - File holding icon descrpition for the XmNiconPixmap property of
*          the dialog shell.
*
*      XuNiconMaskFile
*        - File containing a bitmap that the window manager can use to clip
*          the icon pixmap into a non-rectangular shape.
*
*      XuNminDialogSize
*        - the mimumum alowable size of the dialog. A string with size as
*          <width>x<height>.
*
*      XuNuseXCursor
*        - Use the X standard cursor for this dialog and not the special one
*          defined for the application.
*
*      XuNrelativePosition
*        - If true then the position of the dialog is always relative to the
*          reference widget passed in at creation time. Default False.
*
*                   
*      All other resources are those XmN types that are valid for the type of dialog,
*
*      Note that is the XuNactionAreaItems resource is not set then no action
*      area or the separator associated with the area will be created. The dialog
*      could then consist simply of a shell with the given widget class as a child.
*
*   Convienience Functions with XmCreate style calling arguments. The form widget
*   that is passed back and used for dialog layout is named "mainForm" and may be
*   referenced in resource files this way.
*
*       XuCreateFormDialog(Widget refw, String id, ...)
*       XuCreateToplevelFormDialog(Widget refw, String id, ...)
*
*   Special cases are (see function description below):
*
*       Widget XuCreateMainWindowDialog(Widget refw, String id, ...)
*
*   If the user resizes and/or repositions the dialog this can be remembered
*   upon exit if the "retainGeometry" resource is either ALL, SIZE_ONLY
*   or POSN_ONLY. If ALL then x, y, width and height are retained. If SIZE_ONLY
*   then only width and height are retained and restored to the dialog next
*   time in. If POSN_ONLY then the x and y position is retained. If NONE
*   then no geometry information is retained. The resource "retainGeometry" can
*   be used to override this and turn it on. This resource can be in two places,
*   a global one and a dialog specific one. Thus we can have:
*
*        pgm*retainGeometry: SIZE_ONLY
*        pgm*dialogID.retainGeometry: ALL
*
*   The default is SIZE_ONLY for standard dialogs and ALL for top level dialogs.
*
*   As the Widget returned is several layers down, showing and hiding the
*   dialog must be done with the functions:
*
*      void XuShowDialog(<returned widget>);
*      void XuHideDialog(<returned widget>);
*      void XuDestroyDialog(<returned widget>);
*
*   The location and size information is stored in a state file. The function
*
*      Boolean  XuSetDialogStateFile( String fname )
*
*   sets the location. Note that this is a global setting and applies to all
*   dialogs. Valid input is:
*
*      "default" - use the internal default ( $HOME/.xurescache )
*      NULL      - Use the global state file if it has been defined. If not
*                  then the default action is used.
*      <other>   - a fully qualified file name to be used. If the file is not
*                  accessable then the equivalent of NULL happens.
*
*   by default the NULL action is assumed.
*
*   Information on the location and size of the dialogs can be saved in
*   named profiles in the active state file. The functions for this follow.
*   (See the function code for a complete description of what they do):
*   
*		void    XuSaveDialogProfile()
*		Boolean	XuIsProfiledDialog()
*
* ----------------------------------------------------------------------------
*
* Development Notes: 
* 1. Under linux the va_arg() function will not load with a second argument
*    of Boolean. Thus one must use (Boolean) va_arg(parm, int)
*
* ----------------------------------------------------------------------------
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
/*==========================================================================*/

#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/IntrinsicP.h>
#include <X11/Xproto.h>
#include "XuP.h"
#ifdef MACHINE_PCLINUX
#	include <X11/xpm.h>
#else
#	include <xpm.h>
#endif
#include <Xm/AtomMgr.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MwmUtil.h>
#include <Xm/PushB.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/VendorS.h>

/* Set the maximum frame thickness. This is entirely unreasonable
 * but it at least will avoid really bad problems
 */
#define MAX_FRAME_SIZE	100

/* Dialog data formats
 */
#define FMT6D	"%d %d %d %d %d %d"

/* The name of the acton area. This is the same for both constructs
 * as resources are defined at this level. This means some complexity
 * in finding these, but this can not be avoided.
 */
#define ACTION_AREA	"action_area"


/*============= LOCAL PRIVATE FNCTIONS ===================*/


/* Save the position and size of the dialog. If the pin state is XuPIN_NONE
 * the position will always be saved. If XuPIN_INITIAL any changes in the
 * dialog position will remain for the duration of the program run but will not be
 * saved to the profile file. If XuPIN_ALWAYS the dialog will always be
 * placed at the saved profile position on creation.
 */
static void save_geometry( XuDSP d )
{
	int       dx, dy, dh, dw;
	Position  x, y;

    if(!d || !d->dialog) return;

	XtTranslateCoords(d->dialog, 0, 0, &x, &y);
	dx = (int)x;
	dy = (int)y;
	dw = d->width;
	dh = d->height;

	/* If the position is always relative to the reference widget we must change
	 * our position into one relative to the reference. The reference position must
	 * be determined in case it has moved.
	 */
	if(d->relative_pos && d->ref.w)
	{
		/* Make sure reference widget still exists */
		if(XtIsWidget(d->ref.w))
		{
			XtTranslateCoords(d->ref.w, 0, 0, &x, &y);
			d->ref.x = (int) x;
			d->ref.y = (int) y;
		}
		dx -= d->ref.x;
		dy -= d->ref.y;
	}

	if(d->resize)
	{
		Dimension w, h;
		XtVaGetValues(d->dialog, XmNwidth, &w, XmNheight, &h, NULL);
		if((abs(dw-(int)w) > MIN_SIZE_CHANGE) || (abs(dh-(int)h) > MIN_SIZE_CHANGE))
		{
			dw = (int)w;
			dh = (int)h;
		}
	}

	if( d->pin == XuPIN_INITIAL)
	{
		d->x = dx;
		d->y = dy;
		if(d->width != dw || d->height != dh)
		{
			d->width  = dw;
			d->height = dh;
			if(XuVaStateDataGet(d->name, d->dd->id, NULL, "%d %d", &dx, &dy) == 2)
				XuVaStateDataSave(d->name, d->dd->id, NULL, FMT6D, dx, dy, d->width, d->height, 
						d->offset.x, d->offset.y);
		}
	}
	else if (d->pin == XuPIN_ALWAYS)
	{
		if((d->width != dw || d->height != dh) && 
				XuVaStateDataGet(d->name, d->dd->id, NULL, "%d %d", &dx, &dy) == 2)
		{
			XuVaStateDataSave(d->name, d->dd->id, NULL, FMT6D, dx, dy, d->width, d->height, d->offset.x, d->offset.y);
		}
	}
	else if( d->x != dx || d->y != dy || d->width != dw || d->height != dh)
	{
		d->x      = dx;
		d->y      = dy;
		d->width  = dw;
		d->height = dh;
		XuVaStateDataSave(d->name, d->dd->id, NULL, FMT6D, d->x, d->y, d->width, d->height, d->offset.x, d->offset.y);
	}
}


/* Allows save_geometry to be put into a callback */
/*ARGSUSED*/
static void save_geometry_cb( Widget wid, XtPointer client_data, XtPointer call_data)
{
	save_geometry((XuDSP)client_data);
}


/* Note that the data structure is not destroyed here. All data not related to a
 * specific instance of the dialog is saved and will be reused if the dialog is
 * created again.
 */
/*ARGSUSED*/
static void being_destroyed_cb( Widget wid, XtPointer client_data, XtPointer call_data)
{
	XuDSP d = (XuDSP)client_data;

	/* Remove the mwm installed callback. Only those dialogs that will actually
	 * be destroyed will call this function. XuDestroyDialogCB is the only
	 * function set in this case.
	 */
	Atom delete_atom = XInternAtom(XtDisplay(wid), WM_DELETE_WINDOW, 0);
   	XmRemoveWMProtocolCallback( wid, delete_atom, d->delete_fcn, d->delete_data );

	if(d->modal)
		XuSetDialogCursor(d->stopw, XuSTOP_CURSOR, False);

	XuFreePixmap(d->dialog, d->icon_pixmap);
	d->icon_pixmap = XmUNSPECIFIED_PIXMAP;

	XuFreePixmap(d->dialog, d->icon_mask);
	d->icon_mask = XmUNSPECIFIED_PIXMAP;

	d->ref.w       = NullWidget;
	d->ref.x       = 0;
	d->ref.y       = 0;
	d->stopw       = NullWidget;
	d->dialog      = NullWidget;
	d->mainForm    = NullWidget;
	d->delete_fcn  = XuDestroyDialogCB;
	d->delete_data = NULL;
}


/*ARGSUSED*/
static void dialog_popdown( Widget w, XtPointer client_data, XtPointer call_data)
{
	XuDSP d = (XuDSP)client_data;
	if(d->type == TOPLEVEL)
		XtPopdown(XtParent(d->dialog));
	else
		XtUnmanageChild(d->dialog);
}


/*ARGSUSED*/
static void set_dialog_cursor_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	XuSetDialogCursor(w, XuDEFAULT_CURSOR, True);
}


/*ARGSUSED*/
static void set_parent_cursor_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	XuDSP d = (XuDSP)client_data;
	XmAnyCallbackStruct *rtn = (XmAnyCallbackStruct *)call_data;

	if(!d->modal) return;

	if(rtn->reason == XmCR_MAP)
	{
		w = XmGetPostedFromWidget(XuGetShellChild(d->stopw));
		if(NotNull(w)) d->stopw = XuGetShell(w);
		XuSetDialogCursor(d->stopw, XuSTOP_CURSOR, True);
	}
	else if(rtn->reason == XmCR_UNMAP)
	{
		XuSetDialogCursor(d->stopw, XuSTOP_CURSOR, False);
	}
}


/* Builds the action area of the dialog. The buttons will show a selection
 * border around them when the mouse enters them (assuming that take focus
 * on mouse entry if the operating norm).
 */
static Widget build_action_area(Widget parent, ResInfoStruct *r)
{
	int     i, n, k, ac, rpos, lpos, num_actions, *ndx_list;
	Arg     al[12];
	Widget  *wid_list;
	Widget  dw, fw;
	Boolean none;

	dw   = NullWidget;
	fw   = NullWidget;
	none = True;

	num_actions = MAX(r->num_actions[0],r->num_actions[1]);
	wid_list    = XTCALLOC(num_actions, Widget);
	ndx_list    = XTCALLOC(num_actions, int);
	/*
	 * Any widgets parented to the dialog form are under the control of this
	 * library and do not use the fraction base. Thus we can set it here.
	 */
	XtVaSetValues(parent, XmNfractionBase, Fxu.tightness*num_actions - 1, NULL);
	/*
	 * The following code is complicated by the fact that the buttons in the action
	 * area need to be paranted by the dialog form so that the default button will
	 * be outlined when the cursor is anywhere inside an active dialog. It would have
	 * been easier using a second form if not for this requirement.
	 */
	for( n = 1; n >= 0; n-- )
	{
		int       max_ndx = 0;
		Dimension max_height = 0;

		if(r->num_actions[n] < 1) continue;
		/*
		 * As we want the height of the buttons in a row to all be the same, we
		 * need to set the attachments of the buttons to the button with the maximum
		 * height. Thus we need to create the buttons in two stages. First the buttons
		 * are created and the button with the max height found, then the attachments
		 * are done using this widget.
		 */
		for( i = 0; i < r->num_actions[n]; i++ )
		{
			Dimension height;

			wid_list[i] = XtVaCreateManagedWidget(r->actions[n][i].id, xmPushButtonWidgetClass, parent,
						XmNdefaultButtonShadowThickness, 1,
						XmNmarginHeight, Fxu.button_margins,
						NULL);
			if(r->actions[n][i].callback)
				XtAddCallback(wid_list[i], XmNactivateCallback, r->actions[n][i].callback, r->actions[n][i].data);

			XtVaGetValues(wid_list[i], XmNheight, &height, NULL);
			if( height > max_height)
			{
				max_ndx = i;
				max_height = height;
			}
		}
		/*
		 * Sort the button index list and put the button used as a height reference
		 * into the first location. This one will be attached at the bottom first
		 * and provide a valid reference for the other buttons.
		 */
		ndx_list[0] = max_ndx;
		for( k = 1, i = 0; i < r->num_actions[n]; i++)
		{
			if(i != max_ndx) ndx_list[k++] = i;
		}
		/*
		 * Development note. For some reason the last button in the row, especially
		 * if there is more than one row, would have a greater height than the rest
		 * even though the height came back as the same as the rest of the buttons.
		 * Thus the need for opposite widget attachment. Also it turns out, when this
		 * is done the bottom needed to be attached in the same way or the "bailed
		 * out after 10000 iterations" message would appear.
		 */
		for( k = 0; k < r->num_actions[n]; k++ )
		{
			int tg;
			Boolean dpos;

			i = ndx_list[k];

			dpos = (none && r->default_action == r->actions[n]+i);
			tg   = Fxu.tightness * i;

			ac = 0;
			if (dpos) none = False;
			rpos = (i == (num_actions-1))? tg+Fxu.tightness-2 : tg+Fxu.tightness-1;
			lpos = (i ==  0)? 1:tg;

			if(k != 0)	/* ndx_list[0] contains the reference widget index */
			{
				XtSetArg(al[ac], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
				XtSetArg(al[ac], XmNtopOffset, 0); ac++;
				XtSetArg(al[ac], XmNtopWidget, wid_list[max_ndx]); ac++;
				XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); ac++;
				XtSetArg(al[ac], XmNbottomWidget, wid_list[max_ndx]); ac++;
				XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
			}
			else if(fw)
			{
				XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
				XtSetArg(al[ac], XmNbottomWidget, fw); ac++;
				XtSetArg(al[ac], XmNbottomOffset, 0); ac++;
			}
			else
			{
				XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
				XtSetArg(al[ac], XmNbottomOffset, Fxu.margins); ac++;
			}
			XtSetArg(al[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
			XtSetArg(al[ac], XmNleftPosition, lpos); ac++;
			XtSetArg(al[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
			XtSetArg(al[ac], XmNrightPosition, rpos); ac++;
			XtSetValues(wid_list[i], al, ac);

			if(dpos) dw = wid_list[i];
			if(i==0) fw = wid_list[i];
		}
	}
	if (!dw) dw = fw;
	if (dw)  XtVaSetValues(parent, XmNdefaultButton, dw, NULL);

	XtFree((void*)wid_list);
	XtFree((void*)ndx_list);

	/* Return the first widget created in the last row created for separator attachment */
	return fw;
}


static void make_standard_layout( XuDSP d, WidgetClass class, ResInfoStruct *r, Arg *in_al, int in_ac)
{
	Cardinal ac;
	Boolean no_actions;
	ArgList al;
	Widget actionArea, sep, sw;

	al = XTCALLOC(in_ac+10, Arg);

	ac = 0;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopOffset, 1); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftOffset, 1); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightOffset, 1); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 1); ac++;

	no_actions = (r->num_actions[0] < 1 && r->num_actions[1] < 1);

	if(no_actions)
	{
		XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	}
	else
	{
		actionArea = build_action_area(d->dialog, r);

		sep = XtVaCreateManagedWidget("actionAreaSep",
			xmSeparatorWidgetClass, d->dialog,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 1,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 1,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomOffset, Fxu.margins,
			XmNbottomWidget, actionArea,
			NULL);

		XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
		XtSetArg(al[ac], XmNbottomWidget, sep); ac++;
	}

	if(no_actions && class == xmFormWidgetClass)
	{
		d->mainForm = d->dialog;
		XtSetValues(d->mainForm, in_al, (Cardinal) in_ac);
	}
	else if(class == xmScrolledWindowWidgetClass)
	{
		XtSetArg(al[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
		sw = XtCreateManagedWidget("scrolledWindow",
			xmScrolledWindowWidgetClass, d->dialog,
			al, ac);
		d->mainForm = XmCreateForm(sw, "mainForm", in_al, (Cardinal) in_ac);
	}
	else
	{
		(void) memcpy((void*)(al + ac), (void*)in_al, (size_t)in_ac * sizeof(Arg));
		ac += in_ac;
		d->mainForm = XtCreateWidget("mainForm", class, d->dialog, al, ac);
	}
	XtFree((void*)al);
}


/* This code checks the size of the frame that the Motif window manager
*  puts around the shell and adjusts the offsets accordingly. This is
*  done in a time out as the dialog needs to settle in the process loop
*  before the coordinate translation will work properly. Unfortunatly
*  there seems to be no way to ask the manager what this size will be
*  and this information is needed if the dialog is to be positioned on
*  the screen correctly the next time is is shown.
*/
/*ARGSUSED*/
static void frame_check( XtPointer client_data, XtIntervalId id)
{
	Position x, y, dx, dy;

	/* We pass the widget id and not a data structure pointer in
	 * case the widget has disappeared before this function is called.
	 */
	XuDSP d = _xu_dialog_data((Widget)client_data);
	if (!d) return;

	XSync(XtDisplay(d->dialog), 0);

	/* Special case for the application shell. The shell must not be checked
	 * for offsets until it has been realized and with delay loops this
	 * checking function could be called before then. The doit increment is
	 * to limit this to some sane interval if something is wrong.
	 */
	if(same(d->name,TOP_LEVEL_ID) && !XtIsRealized(d->dialog))
	{
		if(d->offset.doit > 200) return;
		d->offset.doit++;
		(void) XtAppAddTimeOut(Fxu.app_context, 2000, (XtTimerCallbackProc)frame_check, client_data);
		return;
	}

	XtTranslateCoords(d->dialog, 0, 0, &x, &y);
	dx = (Position)(d->x - (int)x);
	dy = (Position)(d->y - (int)y);

	/* If the frame size is correct then dx and dy will be zero. If they are not
	 * zero but greater than MAX_FRAME_SIZE something strange happened as it is
	 * very unlikely that frames will be this big.
	 */
	if((dx != 0 || dy != 0) && abs(dx) < MAX_FRAME_SIZE && abs(dy) < MAX_FRAME_SIZE)
	{
		int      tx, ty, wx, wy;
		Window   rw, pw, *cw, ww, kw;
		unsigned int nc;

		pw = XtWindow(d->dialog);
		tx = (int)x;
		ty = (int)y;
		do {
			ww = pw;
			wx = tx;
			wy = ty;
			XQueryTree(XtDisplay(d->dialog), ww, &rw, &pw, &cw, &nc);
			XTranslateCoordinates(XtDisplay(d->dialog), ww, rw, 0, 0, &tx, &ty, &kw);
			XFree(cw);
		} while(pw != rw);
		d->offset.x = tx - wx;
		d->offset.y = ty - wy;
		save_geometry(d);
	}
}


/*  Trap close protocol issued from the window manager and redirect it to a user function. */
static void install_mwm_close_callback( Widget top, XtCallbackProc fcn, XtPointer client_data )
{
	Atom delete_atom = XInternAtom(XtDisplay(top), WM_DELETE_WINDOW, True);
	if(fcn)
	{
		XtVaSetValues(top, XmNdeleteResponse, XmDO_NOTHING, NULL);
		XmAddWMProtocolCallback(top, delete_atom, fcn, client_data);
	}
}


/* If defined for the dialog, or if not, for the main application, add the icon
 * to the dialog shell. This must be done after the dialog widget has been
 * created as the XuGetPixmap function used information from the widget in the
 * creation of the pixmap. Note that the main application icon pixmap can not
 * be used directly as there is not guarantee that the depth of the sceeen of
 * this dialog will be the same as that of the main application window.
 */
static void set_icon_pixmap(XuDSP d, ResInfoStruct *r)
{
	Pixmap pix = XmUNSPECIFIED_PIXMAP;
	Pixmap msk = XmUNSPECIFIED_PIXMAP;

	if(!blank(r->icon_file))
	{
		pix = XuGetPixmap(d->dialog, r->icon_file);
		if(!blank(r->mask_file))
			msk = XuGetPixmap(d->dialog, r->mask_file);
	}

	if(pix == XmUNSPECIFIED_PIXMAP && !blank(Fxu.top_icon_file))
	{
		pix = XuGetPixmap(d->dialog, Fxu.top_icon_file);
		if(!blank(Fxu.top_mask_file))
			msk = XuGetPixmap(d->dialog, Fxu.top_mask_file);
	}

	d->icon_pixmap = pix;
	d->icon_pixmap = msk;

	if(pix != XmUNSPECIFIED_PIXMAP && msk != XmUNSPECIFIED_PIXMAP)
	{
		XtVaSetValues(XuGetShell(d->dialog), XmNiconPixmap, pix, XmNiconMask, msk, NULL);
	}
	else if(pix != XmUNSPECIFIED_PIXMAP)
	{
		XtVaSetValues(XuGetShell(d->dialog), XmNiconPixmap, pix, NULL);
	}
}


/* Generic dialog creation function where the dialog is a normally created child of the parent.
 * Note that in this case the geometry setting is not used as the motif creation function seems
 * to respond only to the x and y positioning.
 */
static Widget create_normal_dialog( String id, WidgetClass class, Widget refw,
									ResInfoStruct *r, ArgList al, int ac)
{
	Cardinal n;
	Arg an[14];
	XuDSP d;
	Widget toplevel;

	XuSetBusyCursor(True);

	d = _xu_create_data_structure(id, refw, r);
	d->type = STANDARD;
	d->modal = (r->mode == XmDIALOG_FULL_APPLICATION_MODAL ||
				r->mode == XmDIALOG_APPLICATION_MODAL      ||
				r->mode == XmDIALOG_PRIMARY_APPLICATION_MODAL );

	n = 0;
	XtSetArg(an[n], XtNdepth,           r->dpyinfo->depth ); n++;
	XtSetArg(an[n], XtNvisual,          r->dpyinfo->visual); n++;
	XtSetArg(an[n], XtNcolormap,        r->dpyinfo->cmap  ); n++;
	XtSetArg(an[n], XmNdefaultPosition, r->default_pos    ); n++;
	XtSetArg(an[n], XmNresizePolicy,    r->resize_policy  ); n++;
	XtSetArg(an[n], XmNnoResize,        r->no_resize      ); n++;
	XtSetArg(an[n], XmNdialogStyle,     r->mode           ); n++;
	XtSetArg(an[n], XmNautoUnmanage,    False             ); n++;

	/* We do not need to check limits here as the set_geometry function that
	 * is called before we get here ensures that the position is ok.
	 */
	if(d->relative_pos && d->ref.w)
	{
		XtSetArg(an[n], XmNx, (Position)(d->ref.x + d->x + d->offset.x)); n++;
		XtSetArg(an[n], XmNy, (Position)(d->ref.y + d->y + d->offset.y)); n++;
	}
	else
	{
		XtSetArg(an[n], XmNx, (Position)(d->x + d->offset.x)); n++;
		XtSetArg(an[n], XmNy, (Position)(d->y + d->offset.y)); n++;
	}

	if(d->resize && d->width > 0 && d->height > 0)
	{
		XtSetArg(an[n], XmNwidth,  (Dimension) d->width ); n++;
		XtSetArg(an[n], XmNheight, (Dimension) d->height); n++;
	}

	d->dialog = XmCreateFormDialog(XuGetShell(refw), id, an, n);
	toplevel  = XuGetShell(d->dialog);

	set_icon_pixmap(d, r);

	if(r->is_kb_focus_policy)
	{
		XtVaSetValues(toplevel, XmNkeyboardFocusPolicy, r->kb_focus_policy, NULL);
	}

	_xu_get_minimum_size(id, d, r);

	XtVaSetValues(toplevel,
		XmNminWidth,       r->min_width,
		XmNminHeight,      r->min_height,
		XmNwidthInc,       r->width_inc,
		XmNheightInc,      r->height_inc,
		XmNbaseWidth,      r->base_width,
		XmNbaseHeight,     r->base_height,
		NULL);

	if (r->use_xu_cursor) XtAddCallback(d->dialog, XmNmapCallback, set_dialog_cursor_cb, NULL);
	XtAddCallback(d->dialog, XmNmapCallback,     set_parent_cursor_cb, (XtPointer)d);
	XtAddCallback(d->dialog, XmNunmapCallback,   set_parent_cursor_cb, (XtPointer)d);
	XtAddCallback(toplevel,  XmNpopdownCallback, save_geometry_cb,     (XtPointer)d);

	if(r->destroy)
	{
		install_mwm_close_callback(toplevel, r->delete_fcn, r->delete_data);
		XtAddCallback(d->dialog, XmNdestroyCallback, being_destroyed_cb,  (XtPointer)d);
		if (r->CBFcn) XtAddCallback(d->dialog, XmNdestroyCallback, r->CBFcn, r->CBFcn_data);
	}
	else
	{
		install_mwm_close_callback(toplevel, dialog_popdown, (XtPointer)d);
		if (r->CBFcn) XtAddCallback(d->dialog, XmNunmapCallback, r->CBFcn, r->CBFcn_data);
	}

	make_standard_layout(d, class, r, al, ac);

	return d->mainForm;
}


static Widget create_toplevel_dialog( String id, WidgetClass class, Widget refw,
										ResInfoStruct *r, ArgList al, int ac)
{
	Cardinal  argc;
	char mbuf[200];
	Arg args[24];
	Widget topLevel;
	XuDSP d;

	XuSetBusyCursor(True);

	d = _xu_create_data_structure(id, refw, r);
	d->type = TOPLEVEL;

	_xu_get_minimum_size(id, d, r);

	argc = 0;
	XtSetArg(args[argc], XtNdepth,          r->dpyinfo->depth ); argc++;
	XtSetArg(args[argc], XtNvisual,         r->dpyinfo->visual); argc++;
	XtSetArg(args[argc], XtNcolormap,       r->dpyinfo->cmap  ); argc++;
	XtSetArg(args[argc], XmNmwmDecorations, r->decorations    ); argc++;
	XtSetArg(args[argc], XmNmwmFunctions,   r->functions      ); argc++;
	XtSetArg(args[argc], XmNminWidth,       r->min_width      ); argc++;
	XtSetArg(args[argc], XmNminHeight,      r->min_height     ); argc++;
	XtSetArg(args[argc], XmNwidthInc,       r->width_inc      ); argc++;
	XtSetArg(args[argc], XmNheightInc,      r->height_inc     ); argc++;
	XtSetArg(args[argc], XmNbaseWidth,      r->base_width     ); argc++;
	XtSetArg(args[argc], XmNbaseHeight,     r->base_height    ); argc++;
	XtSetArg(args[argc], XmNgeometry,       d->geometry       ); argc++;

	if(r->is_kb_focus_policy)
	{
		XtSetArg(args[argc], XmNkeyboardFocusPolicy, r->kb_focus_policy); argc++;
	}

	if(r->dpyinfo == DefaultAppDisplayInfo)
	{
		(void) snprintf(mbuf, sizeof(mbuf), "%s_popup", id);
		topLevel = XtCreatePopupShell(mbuf, topLevelShellWidgetClass, Fxu.top_level, args, argc);
	}
	else
	{
		topLevel = XtAppCreateShell(Fxu.app_name, Fxu.app_class, topLevelShellWidgetClass, r->dpyinfo->display, args, argc);
	}

	XtAddCallback(topLevel, XmNpopdownCallback, save_geometry_cb, (XtPointer)d);

	argc = 0;
	XtSetArg(args[argc], XmNdefaultPosition, False); argc++;
	XtSetArg(args[argc], XmNresizePolicy, r->resize_policy); argc++;
	if(r->dialogTitle)
	{
		XtSetArg(args[argc], XmNdialogTitle, r->dialogTitle); argc++;
	}

	d->dialog = XtCreateWidget(id, xmFormWidgetClass, topLevel, args, argc);

	set_icon_pixmap(d, r);

	if(r->destroy)
	{
		install_mwm_close_callback(topLevel, r->delete_fcn, r->delete_data);
		XtAddCallback(d->dialog, XmNdestroyCallback, being_destroyed_cb, (XtPointer)d);
		if (r->CBFcn) XtAddCallback(topLevel, XmNdestroyCallback, r->CBFcn, r->CBFcn_data);
	}
	else
	{
		install_mwm_close_callback(topLevel, dialog_popdown, (XtPointer)d);
		if (r->CBFcn) XtAddCallback(topLevel, XmNpopdownCallback, r->CBFcn, r->CBFcn_data);
	}

	if (r->use_xu_cursor)
	{
		XtAddCallback(topLevel, XmNpopupCallback, set_dialog_cursor_cb, NULL);
	}

	make_standard_layout(d, class, r, al, ac);

	return d->mainForm;
}



/*=============== INTERNAL LIBRARY FUNCTIONS ======================*/



/* Create the resource string from the dialog id and the resource item.
 * This will give result in "*dialog.item". This is defined as RES in XuP.h
 * and thus will not show up in any code if a grep is done. I state this
 * here so that you will not think that this function is not used ;-)
 */
String _xu_make_resource_id(String id, String res)
{
	static int    size   = 0;
	static String buffer = NULL;

	int n = 3;

	if (id)  n += strlen(id);
	n += strlen(res);

	if( n > size )
	{
		size = n;
		if((buffer = XtRealloc(buffer, (Cardinal) size)) == NULL) return NULL;
	}

	(void) strcpy(buffer, "");
	if(id)
	{
		(void) strcpy(buffer, "*");
		(void) strcat(buffer, id);
	}
	(void) strcat(buffer, ".");
	(void) strcat(buffer, res);

	return buffer;
}


/* The frame offset checking function needs to be in a timer loop to ensure that
 * all dialog parameters have settled down.
 */
void _xu_check_frame_size(XuDSP d, unsigned long timeout)
{
	if(!d->default_pos && d->offset.doit == 0)
	{
		/* so that the check is only done once per invocation */
		d->offset.doit++;
		(void) XtAppAddTimeOut(Fxu.app_context, timeout, (XtTimerCallbackProc)frame_check, (XtPointer)(d->dialog));
	}
}



/* find the dialog data structure given a widget in the widget tree */
XuDSP _xu_dialog_data(Widget w)
{
	while(w)
	{
		int i;
		for(i = 0; i < Fxu.ndl; i++)
		{
			if(!Fxu.dl[i]->dialog) continue;
			if(w == Fxu.dl[i]->dialog) return Fxu.dl[i];
			if(w == Fxu.dl[i]->mainForm) return Fxu.dl[i];
			if(w == XuGetShell(Fxu.dl[i]->dialog)) return Fxu.dl[i];
		}
		if(XtIsShell(w)) break;
		w = XtParent(w);
	}
	return (XuDSP)NULL;
}


/* This function works in conjunction with set_geometry. If the x or y are negative
 * they are set to very large values and it is assumed that the set_geometry function
 * will clip them. The code can not handle negative zero values directly.
 */
void _xu_parse_geometry_string( String geometry, int *x, int *y, int *w, int *h)
{
	int rtn, wx, wy;
	unsigned int wh, ht;
	rtn = XParseGeometry(geometry, &wx, &wy, &wh, &ht);
	if(rtn & WidthValue ) *w = (int) wh;
	if(rtn & HeightValue) *h = (int) ht;
	if(rtn & XValue     ) *x = wx;
	if(rtn & YValue     ) *y = wy;
	if(rtn & XNegative  ) *x = 10000000;
	if(rtn & YNegative  ) *y = 10000000;
}


/* Set the geometry string. The size and position of top level shells created with
 * XtAppCreateShell only seem to respond to the geometry resource and not the x, y,
 * width and height resources.  The position and size of the dialog is checked against
 * the screen limits and set so that the dialog will always remain on the screen.
 */
 void _xu_set_geometry( XuDSP d, int x, int y )
{
	int nx, ny;

	/* If the position is realtive to the reference widget we must position
	 * ourselves accordingly
	 */
	if(d->relative_pos && d->ref.w)
	{
		nx = x + d->ref.x + d->offset.x;
		ny = y + d->ref.y + d->offset.y;
	}
	else
	{
		nx = x + d->offset.x;
		ny = y + d->offset.y;
	}

	if(d->dd->display)
	{
		int mx, my;

		if(d->width  > d->dd->width ) d->width  = d->dd->width;
		if(d->height > d->dd->height) d->height = d->dd->height;

		/* The bottom border normally is the same thickness as the size borders */
		mx = d->dd->width  - abs(d->offset.x) - abs(d->offset.x) - ((d->resize && d->width  > 0) ? d->width  : 20);
		my = d->dd->height - abs(d->offset.y) - abs(d->offset.x) - ((d->resize && d->height > 0) ? d->height : 30);
	
		if(nx > mx) nx = mx;
		if(ny > my) ny = my;
	}

	if(nx < 0) nx = 0;
	if(ny < 0) ny = 0;

	if(d->relative_pos && d->ref.w)
	{
		d->x = nx - d->ref.x - d->offset.x;
		d->y = ny - d->ref.y - d->offset.y;
	}
	else
	{
		d->x = nx - d->offset.x;
		d->y = ny - d->offset.y;
	}

	if(d->resize && d->width > 0 && d->height > 0)
		snprintf(d->geometry, sizeof(d->geometry), "%dx%d%+d%+d", d->width, d->height, nx, ny);
	else
		snprintf(d->geometry, sizeof(d->geometry), "%+d%+d", nx, ny);
}


/* Note that if the sizes are not set by a hard coded XmNxxx resource then
 * they will have a value of 0 when they enter this function. When written
 * top level dialogs had problems if they had a minumum size of less than
 * about 350x250. This may have been corrected in later versions of Motif.
 */
void _xu_get_minimum_size(String id, XuDSP d, ResInfoStruct *res)
{
	int    minw, minh;
	String data;

	if(res->min_width > 0 && res->min_height > 0) return;

	data = XuGetStringResource(RES(id,XuNminDialogSize), NULL);
	if(data != NULL && sscanf(data, "%dx%d", &minw, &minh) == 2)
	{
		res->min_width  = minw;
		res->min_height = minh;
	}
	if(res->min_width < 1)
	{
		int min = XuGetIntResource(RES(id,XmNminWidth), 0);
		if(min > 0) res->min_width = min;
	}
	if(res->min_height < 1)
	{
		int min = XuGetIntResource(RES(id,XmNminHeight), 0);
		if(min > 0) res->min_height = min;
	}
	if(res->min_width < 1 && res->min_height < 1)
	{
		data = XuGetStringResource(RES(NULL,XuNminDialogSize), NULL);
		if(data != NULL && sscanf(data, "%dx%d", &minw, &minh) == 2)
		{
			res->min_width  = minw;
			res->min_height = minh;
		}
	}

	if( res->min_width  < 1 )
		res->min_width = (d->type == TOPLEVEL) ? 350:100;

	if( res->min_height < 1 )
		res->min_height = (d->type == TOPLEVEL) ? 250:100;
}



/* Create the structure to hold the dialog info.
 */
XuDSP _xu_create_data_structure( String id, Widget refw, ResInfoStruct *r)
{
	int       nx, x, y, width, height, dx, dy, dh, dw;
	Position  px = 0, py = 0;
	String    name;
	Boolean   profiled;
	XuDSP     d;

	/* If a alternate dialogID has been specified this takes precedence over id.  */
	name = blank(r->dialog_id)? id : r->dialog_id;

	/* Get reference widget information */
	if (refw)
	{
		if(XtIsWidget(refw))
			XtTranslateCoords(refw, 0, 0, &px, &py);
		else
			refw = NullWidget;
	}

	/* If the dialog has been created before we do not want to change the
	 * geometry settings. Note that the display can have changed.
	 */
	for(nx = 0; nx < Fxu.ndl; nx++)
	{
		d = Fxu.dl[nx];
		if(!same(d->name, name)) continue;

		d->ref.w        = refw;
		d->ref.x        = (int) px;
		d->ref.y        = (int) py;
		d->stopw        = XuGetShell(refw);
		d->dd           = r->dpyinfo;
		d->resize       = !r->no_resize;
		d->default_pos  = r->default_pos;
		d->relative_pos = r->relative_pos;
		d->offset.doit  = 0;
		d->delete_fcn   = r->delete_fcn;
		d->delete_data  = r->delete_data;
		_xu_set_geometry(d, d->x, d->y);
		return d;
	}

	/* Allocate in reasonable chunks to lessen memory fragmentation */
	if(Fxu.ndl >= Fxu.dlalloc)
	{
		Fxu.dlalloc += (Fxu.dlalloc)? 10:20;
		Fxu.dl = (XuDStruct **)XtRealloc((void *)Fxu.dl, Fxu.dlalloc*sizeof(XuDStruct*));
	}
	Fxu.dl[Fxu.ndl] = XTCALLOC(1, XuDStruct);
	d = Fxu.dl[Fxu.ndl];
	Fxu.ndl++;

	d->name         = XtNewString(name);
	d->ref.w        = refw;
	d->ref.x        = (int) px;
	d->ref.y        = (int) py;
	d->stopw        = XuGetShell(refw);
	d->pin          = XuPIN_NONE;
	d->resize       = !r->no_resize;
	d->default_pos  = r->default_pos;
	d->relative_pos = r->relative_pos;
	d->dd           = r->dpyinfo;
	d->delete_fcn   = r->delete_fcn;
	d->delete_data  = r->delete_data;

	/* Find the window position, size and frame offsets. The default frame values are
	 * just for the first time the dialog is realized and were valid for my development
	 * machine running RedHat enterprise 3. After that the real values will have been
	 * calculated and stored. The doit parameter restricts the number of frame size
	 * determinations to one.
	 */
	d->offset.doit = 0;

	/* Was the position of this dialog saved as a profile? */
	if((profiled = XuIsProfiledDialog(d->name)))
	{
		int pin;
		profiled = (XuVaStateDataGet(d->name, d->dd->id, NULL, FMT6D,
						&d->x, &d->y, &d->width, &d->height, &d->offset.x, &d->offset.y) == 6);
		if(XuVaStateDataGet(PROFILED_DIALOGS, PINSTATE, NULL, "%d", &pin) == 1)
			d->pin = (XuPIN_STYLE) pin;
	}
	if(!profiled)
	{
		String resg, res_name, ptr;
		x = y  = 0;
		width  = 400;
		height = 200;

		/* Return the part of the dialog name before the XuDIALOG_ID_PART_SEPARATOR string.
		 * This is the part recognized in the resource file by the get geometry actions
		 */
		res_name = XtNewString(d->name);
		if((ptr = strstr(res_name,XuDIALOG_ID_PART_SEPARATOR))) *ptr = '\0';

		/* Get the geometry specification from the resource database. We first look
		 * for a geometry string (w*h+x+y). If there is none we look for the separate
		 * components.
		 */
		if((resg = XuGetStringResource(RES(res_name,XmNgeometry), NULL)))
		{
			_xu_parse_geometry_string(resg, &x, &y, &width, &height);
		}
		else
		{
			x = XuGetIntResource(RES(res_name,".x"), 0);
			y = XuGetIntResource(RES(res_name,".y"), 0);
			width = XuGetIntResource(RES(res_name,".width"), 0);
			height = XuGetIntResource(RES(res_name,".height"), 0);
		}

		XtFree(res_name);

		/* If appropriate set the screen position relative to the reference widget. The positions
		 * in the resource file are all offsets from the reference widget. If the relativePosition
		 * resoure is True this is taken care of elsewhere in the code and must not be done here.
		 */
		if(!r->relative_pos && refw && r->dpyinfo == DefaultAppDisplayInfo)
		{
			Position  ref_x, ref_y;
			XtTranslateCoords(refw, 0, 0, &ref_x, &ref_y);
			x = (int) ref_x + x;
			y = (int) ref_y + y;
		}

		/* If the dialog has had it's position saved then use these values for the position rather than
		 * the default as found above. 20070713: Do no read for RETAIN_NONE.
		 */
		if(r->retain == XuRETAIN_NONE ||
				XuVaStateDataGet(d->name, d->dd->id, NULL, FMT6D, &dx, &dy, &dw, &dh, &d->offset.x, &d->offset.y) != 6)
		{
			dx = x;
			dy = y;
			dw = width;
			dh = height;
			d->offset.x = -6;
			d->offset.y = -21;
		}

		/* If we maintain a relative position then no matter what
		 * our retain state the position must be obtained.
		 */
		switch(r->retain)
		{
			case XuRETAIN_POSN_ONLY:
				x = dx;
				y = dy;
				if(d->resize)
				{
					width = 0;
					height = 0;
				}
				break;

			case XuRETAIN_SIZE_ONLY:
				if(d->relative_pos)
				{
					x = dx;
					y = dy;
				}
				if(d->resize)
				{
					if(dw > 0) width  = dw;
					if(dh > 0) height = dh;
				}
				break;

			case XuRETAIN_ALL:
				x = dx;
				y = dy;
				if(d->resize)
				{
					if(dw > 0) width  = dw;
					if(dh > 0) height = dh;
				}
				break;

			default:
				if(d->relative_pos)
				{
					x = dx;
					y = dy;
				}
		}

		/* Finally set our dialog geometry */
		d->x      = x;
		d->y      = y;
		d->width  = width;
		d->height = height;
	}

	_xu_set_geometry(d, d->x, d->y);
	return d;
}


/* Scan the variable resource argument list and return a fixed list of arguments.
 * Note that those resources specifically handled by this function are stripped
 * and not returned in the list.
 */
void _xu_scan_resource_strings(String id, Widget refw, Boolean is_top_level, ResInfoStruct *res,
								ArgList *al, int *ac, va_list args)
{
	int      i, n;
	int      ac_alloc          = 0;
	String   cmd               = NULL;
	String   retain            = NULL;
	String   dpyname           = NULL;
	Boolean  decor_specified   = False;
	Boolean  retain_specified  = False;
	String   default_action_id = Fxu.default_action_id;
	XuDSP    refd              = _xu_dialog_data(refw);

	/* Zero the structure */
	(void) memset((void*)res, 0, sizeof(ResInfoStruct));

	/* Set any non-zero structure defaults
	 */
	res->toplevel_type = is_top_level;
	res->dpyinfo       = (refd)? refd->dd : _xu_find_display_info_from_widget(refw);
	res->destroy       = True;
	res->resize_policy = XmRESIZE_ANY;
	res->mode          = XmDIALOG_PRIMARY_APPLICATION_MODAL;
	res->decorations   = (int) MWM_DECOR_BORDER;
	res->retain        = XuRETAIN_ALL;
	res->delete_fcn    = XuDestroyDialogCB;
	res->delete_data   = NULL;

	/* Read items out of the resource database ahead of the hard coded resources.
	 * Note that a dialogTitle trumps a title.
	*/
	res->use_xu_cursor = !XuGetBooleanResource(RES(id,XuNuseXCursor), False);
	res->no_resize     = XuGetBooleanResource(RES(id,XmNnoResize), False);
	res->default_pos   = XuGetBooleanResource(RES(id,XmNdefaultPosition), False);
	res->width_inc     = XuGetIntResource(RES(id,XmNwidthInc), 1);
	res->height_inc    = XuGetIntResource(RES(id,XmNheightInc), 1);
	res->base_width    = XuGetIntResource(RES(id,XmNbaseWidth), 1);
	res->base_height   = XuGetIntResource(RES(id,XmNbaseHeight), 1);
	res->relative_pos  = XuGetBooleanResource(RES(id,XuNrelativePosition), False);

	dpyname  = XuGetStringResource(RES(id,XuNdialogDisplay), NULL);

	/* The retainGeometry resource needs processing */
	if((retain = XuGetStringResource(RES(id,XuNretainGeometry), NULL)))
	{
		retain_specified = True;
		if     (same_ic(retain,"SIZE_ONLY")) res->retain = XuRETAIN_SIZE_ONLY;
		else if(same_ic(retain,"POSN_ONLY")) res->retain = XuRETAIN_POSN_ONLY;
		else if(same_ic(retain,"ALL"      )) res->retain = XuRETAIN_ALL;
		else if(same_ic(retain,"NONE"     )) res->retain = XuRETAIN_NONE;
	}

	if(XuGetBooleanResource(RES(id,XuNallowIconify), False))
	{
		res->decorations |= MWM_DECOR_MINIMIZE;
	}

	if(NotNull(cmd = XuGetStringResource(RES(id,XuNiconPixmapFile), NULL)))
	{
		res->icon_file = XtNewString(cmd);
	}

	if(NotNull(cmd = XuGetStringResource(RES(id,XuNiconMaskFile), NULL)))
	{
		res->mask_file = XtNewString(cmd);
	}

	if(NotNull(cmd = XuGetStringResource(RES(id,XmNkeyboardFocusPolicy), NULL)))
	{
		if(same_start_ic(cmd,"e"))
		{
			res->is_kb_focus_policy = True;
			res->kb_focus_policy = XmEXPLICIT;
		}
		else if(same_start_ic(cmd,"p"))
		{
			res->is_kb_focus_policy = True;
			res->kb_focus_policy = XmPOINTER;
		}
	}

	/* Now set the hard coded resources
	*/
	*ac = 0;
	*al = NULL;

	while((cmd = va_arg(args, XtPointer)))
	{
		/* First come the XuNxxx resources
		 */
		if(same(cmd, XuNdialogID))
		{
			/* Override the dialogTitle resource if a dialogID equivalent is found. The given
			 * id can be in two parts separated by a XuDIALOG_ID_PART_SEPARATOR. The first
			 * part is used to override the dialog title and the second part adds additional
			 * granularity to the id which is used for geometry information in the state file,
			 */
			String ptr = va_arg(args, String);
			if (ptr)
			{
				String did, partid, p;
				res->dialog_id = ptr;
				partid = XtNewString(res->dialog_id);
				if((p = strstr(partid,XuDIALOG_ID_PART_SEPARATOR))) *p = '\0'; 
				if((did = XuGetStringResource(RES(partid,XmNdialogTitle), NULL)))
				{
					char buf[256];
					snprintf(buf, 256, "%s*.%s.%s", Fxu.app_name, id, XmNdialogTitle);
					XuPutStringResource(buf, did);
				}
				XtFree(partid);
			}
		}
		else if(same(cmd, XuNallowIconify))
		{
			if(va_arg(args, int))
				res->decorations |= MWM_DECOR_MINIMIZE;
			else
				res->decorations &= ~MWM_DECOR_MINIMIZE;
		}
		else if(same(cmd, XuNiconPixmapFile))
		{
			String ptr = va_arg(args, String);
			if (ptr) res->icon_file = XtNewString(ptr);
		}
		else if(same(cmd, XuNiconMaskFile))
		{
			String ptr = va_arg(args, String);
			if (ptr) res->mask_file = XtNewString(ptr);
		}
		else if(same(cmd, XuNdestroyCallback))
		{
			res->CBFcn = va_arg(args, XtCallbackProc);
		}
		else if(same(cmd, XuNdestroyData))
		{
			res->CBFcn_data = va_arg(args, XtPointer);
		}
		else if(same(cmd, XuNuseXCursor))
		{
			res->use_xu_cursor = (Boolean)!va_arg(args, int);
		}
		else if(same(cmd, XuNactionAreaRow1Items))
		{
			res->actions[0] = va_arg(args, XuDialogActionsStruct *);
		}
		else if(same(cmd, XuNnumActionAreaRow1Items))
		{
			res->num_actions[0] = va_arg(args, int);
		}
		else if(same(cmd, XuNactionAreaRow2Items))
		{
			res->actions[1] = va_arg(args, XuDialogActionsStruct *);
		}
		else if(same(cmd, XuNnumActionAreaRow2Items))
		{
			res->num_actions[1] = va_arg(args, int);
		}
		else if(same(cmd, XuNdefaultActionItemId))
		{
			String ptr = va_arg(args, String);
			if (ptr) default_action_id = ptr;
		}
		else if(same(cmd, XuNretainGeometry))
		{
			retain_specified = True;
			res->retain      = (XuRETAIN) va_arg(args, int);
		}
		else if(same(cmd, XuNdialogDisplay))
		{
			String ptr = va_arg(args, String);
			if (ptr) dpyname = ptr;
		}
		else if(same(cmd, XuNrelativePosition))
		{
			res->relative_pos = (Boolean) va_arg(args, int);
		}
		else if(same(cmd, XuNmwmDeleteOverride))
		{
			res->delete_fcn = va_arg(args, XtCallbackProc);
		}
		else if(same(cmd, XuNmwmDeleteData))
		{
			res->delete_data = va_arg(args, XtPointer);
		}
		/*
		 * Everything from now on are specially handled XmNxxx resources
		 */
		else if(same(cmd, XmNnoResize))
		{
			res->no_resize = (Boolean)va_arg(args, int);
		}
		else if(same(cmd, XmNresizePolicy))
		{
			res->resize_policy = va_arg(args, int);
		}
		else if(same(cmd, XmNmwmDecorations))
		{
			res->decorations |= va_arg(args, int);
			decor_specified  =  True;
		}
		else if(same(cmd, XmNminWidth))
		{
			res->min_width = va_arg(args, int);
		}
		else if(same(cmd, XmNminHeight))
		{
			res->min_height = va_arg(args, int);
		}
		else if(same(cmd, XmNwidthInc))
		{
			res->width_inc = va_arg(args, int);
		}
		else if(same(cmd, XmNheightInc))
		{
			res->height_inc = va_arg(args, int);
		}
		else if(same(cmd, XmNbaseWidth))
		{
			res->base_width = va_arg(args, int);
		}
		else if(same(cmd, XmNbaseHeight))
		{
			res->base_height = va_arg(args, int);
		}
		else if(same(cmd, XmNdialogStyle))
		{
			res->mode = va_arg(args, int);
		}
		else if(same(cmd, XmNdefaultPosition))
		{
			res->default_pos = (Boolean) va_arg(args, int);
		}
		else if(same(cmd, XmNdialogTitle))
		{
			XmString xms = va_arg(args, XmString);
			if (xms) res->dialogTitle = xms;
		}
		else if(same(cmd, XmNkeyboardFocusPolicy))
		{
			res->is_kb_focus_policy = True;
			res->kb_focus_policy    = va_arg(args, int);
		}
		else
		{
			if(*ac >= ac_alloc)
			{
				ac_alloc += 20;
				*al = (ArgList) XtRealloc((void*)(*al), (size_t) ac_alloc * sizeof(Arg));
			}
			XtSetArg((*al)[*ac], cmd, va_arg(args, XtPointer));
			(*ac)++;
		}
	}

	/* Identify the default action item given the action item id. Also for
	 * backwards compatability when the number of actions was specified by
	 * a NULL id as the last structure element
	 */
	for(i = 0; i < 2; i++)
	{
		if(res->num_actions[i] == 0 && res->actions[i] != NULL)
		{ 
			while(res->actions[i][res->num_actions[i]].id) res->num_actions[i]++; 
		}
		for(n = 0; n < res->num_actions[i]; n++)
		{
			if(_xu_in_comma_separated_list(default_action_id, res->actions[i][n].id))
				res->default_action = res->actions[i] + n;
		}
	}

	/* Now for the rest of those things that can only be taken care of after we
	 * have checked for all of the possible resource settings.
	 */
	if(!res->no_resize)
		res->decorations |= MWM_DECOR_RESIZEH;

	/* If the decorations are not specified set them to our default state */
	if(!decor_specified)
		res->decorations |= MWM_DECOR_TITLE|MWM_DECOR_MENU|MWM_DECOR_MAXIMIZE;

	/* Set the functions corresponding to the decorations */
	if( res->decorations & MWM_DECOR_MENU    ) res->functions |= MWM_FUNC_MOVE|MWM_FUNC_CLOSE;
	if( res->decorations & MWM_DECOR_RESIZEH ) res->functions |= MWM_FUNC_RESIZE;
	if( res->decorations & MWM_DECOR_MINIMIZE) res->functions |= MWM_FUNC_MINIMIZE;
	if( res->decorations & MWM_DECOR_MAXIMIZE) res->functions |= MWM_FUNC_MAXIMIZE;

	if(dpyname)
	{
		DPYPTR dp = _xu_open_display(dpyname, NULL, 0, NULL, NULL);
		if (dp) res->dpyinfo = dp;
	}
	
	/* If the dialog is not top level but it has been asked to be on
	 * another screen or display then it must be forced to top level.
	 */
	if(!res->toplevel_type)
	{
		res->toplevel_type = (refd != NULL && res->dpyinfo != refd->dd);
	}

	/* The default for toplevel dialogs is to retain all. We can not check
	 * before this point as both the toplevel and retainGeometry resources
	 * can be hard coded.
	 */
	if(res->toplevel_type && !retain_specified)
		res->retain = XuRETAIN_ALL;
}



/*=============== PUBLIC FUNCTIONS ====================*/

/* As it says in the function name ;-). The somewhat complex nature of the
 * scan is due to both potential action areas having the same name. This was
 * done for ease of resource setting.
 */
Widget XuGetActionAreaBtnByName(Widget _w, String _name)
{
	int        n, nchild;
	WidgetList childs;
	XuDSP      d = _xu_dialog_data(_w);

	if (!d) return NULL;

	XtVaGetValues(d->dialog, XmNnumChildren, &nchild, XmNchildren, &childs, NULL);
	for(n=0; n<nchild; n++)
	{
		if(strcmp(_name, XtName(childs[n])) == 0) return childs[n];
	}
	return NULL;
}


/* Generic dialog creation function where the widget to be the child of the
 * form child of the dialog shell is passed in as an argument. If a standard
 * dialog is requested but it is put on a different screen or display from the
 * reference widget then it is forced to a top level type. See the
 * _xu_scan_resource_strings function.
 */
Widget XuCreateDialog( Widget refw, WidgetClass class, String id, ...)
{
	int ac;
	ArgList al;
	ResInfoStruct r;
	Widget w;
	va_list args;

	va_start(args, id);
	_xu_scan_resource_strings(id, refw, False, &r, &al, &ac, args);
	va_end(args);
	if(r.toplevel_type)
		w = create_toplevel_dialog(id, class, refw, &r, al, ac);
	else
		w = create_normal_dialog(id, class, refw, &r, al, ac);
	XtFree((void*)al);
	return w;
}


Widget XuCreateToplevelDialog( Widget refw, WidgetClass class, String id, ...)
{
	int ac;
	ArgList al;
	ResInfoStruct r;
	Widget w;
	va_list args;

	va_start(args, id);
	_xu_scan_resource_strings(id, refw, True, &r, &al, &ac, args);
	va_end(args);
	w = create_toplevel_dialog(id, class, refw, &r, al, ac);
	XtFree((void*)al);
	return w;
}


/* The following are for convienience as special cases of the general dialog
 * creation functions above as this is the most commonly created dialog type.
 */
Widget XuCreateFormDialog( Widget refw, String id, ...)
{
	int ac;
	ArgList al;
	ResInfoStruct r;
	Widget w;
	va_list args;

	va_start(args, id);
	_xu_scan_resource_strings(id, refw, False, &r, &al, &ac, args);
	va_end(args);
	if(r.toplevel_type)
		w = create_toplevel_dialog(id, xmFormWidgetClass, refw, &r, al, ac);
	else
		w = create_normal_dialog(id, xmFormWidgetClass, refw, &r, al, ac);
	XtFree((void*)al);
	return w;
}


Widget XuCreateToplevelFormDialog( Widget refw, String id, ...)
{
	int ac;
	ArgList al;
	ResInfoStruct r;
	Widget w;
	va_list args;

	va_start(args, id);
	_xu_scan_resource_strings(id, refw, True, &r, &al, &ac, args);
	va_end(args);
	w = create_toplevel_dialog(id, xmFormWidgetClass, refw, &r, al, ac);
	XtFree((void*)al);
	return w;
}


/* Creating a main window requires special handling and so it can not use the
 * standard creation functions as the above functions do.
 */
Widget XuCreateMainWindowDialog( Widget refw, String id, ...)
{
	int ac;
	Cardinal argc;
	ArgList al;
	Arg  args[20];
	char *cmd, mbuf[200];
	Widget topLevel;
	ResInfoStruct r;
	XuDSP d;
	va_list ar;

	XuSetBusyCursor(True);
	XuUpdateDisplay(refw);

	va_start(ar, id);
	_xu_scan_resource_strings(id, refw, True, &r, &al, &ac, ar);
	va_end(ar);

	/* force these as a main window default state */
	r.no_resize = False;
	r.retain    = XuRETAIN_ALL;

	d = _xu_create_data_structure(id, refw, &r);
	d->type = TOPLEVEL;

	_xu_get_minimum_size(id, d, &r);

	argc = 0;
	XtSetArg(args[argc], XmNmwmDecorations, r.decorations); argc++;
	XtSetArg(args[argc], XmNmwmFunctions,   r.functions  ); argc++;
	XtSetArg(args[argc], XmNminWidth,       r.min_width  ); argc++;
	XtSetArg(args[argc], XmNminHeight,      r.min_height ); argc++;
	XtSetArg(args[argc], XmNwidthInc,       r.width_inc  ); argc++;
	XtSetArg(args[argc], XmNheightInc,      r.height_inc ); argc++;
	XtSetArg(args[argc], XmNbaseWidth,      r.base_width ); argc++;
	XtSetArg(args[argc], XmNbaseHeight,     r.base_height); argc++;
	XtSetArg(args[argc], XmNgeometry,       d->geometry  ); argc++;
		
	cmd = XuGetStringResource(RES(id,XmNiconName), NULL);
	if(cmd)
	{
		XtSetArg(args[argc], XmNiconName, cmd); argc++;
	}
	cmd = XuGetStringResource(RES(id,XmNtitle), NULL);
	if (!cmd) cmd = XuGetStringResource(RES(id,XmNdialogTitle), NULL);
	if(cmd)
	{
		XtSetArg(args[argc], XmNtitle, cmd); argc++;
	}
	if(r.is_kb_focus_policy)
	{
		XtSetArg(args[argc], XmNkeyboardFocusPolicy, r.kb_focus_policy); argc++;
	}

	if(r.dpyinfo == DefaultAppDisplayInfo)
	{
		XtSetArg(args[argc], XtNdepth,    r.dpyinfo->depth ); argc++;
		XtSetArg(args[argc], XtNvisual,   r.dpyinfo->visual); argc++;
		XtSetArg(args[argc], XtNcolormap, r.dpyinfo->cmap  ); argc++;

		(void) snprintf(mbuf, sizeof(mbuf), "%s_popup", id);
		topLevel = XtCreatePopupShell(mbuf, topLevelShellWidgetClass, XuGetShell(refw), args, argc);
	}
	else
	{
		topLevel = XtAppCreateShell(Fxu.app_name, Fxu.app_class, topLevelShellWidgetClass, r.dpyinfo->display, args, argc);
	}
	XtAddCallback(topLevel, XmNpopdownCallback, save_geometry_cb, (XtPointer)d);

	d->mainForm = d->dialog = XmCreateMainWindow(topLevel, id, al, (Cardinal) ac);

	XtFree((void*)al);

	set_icon_pixmap(d, &r);

	if(r.destroy)
	{
		install_mwm_close_callback(topLevel, r.delete_fcn, r.delete_data);
		XtAddCallback(d->dialog, XmNdestroyCallback, being_destroyed_cb, (XtPointer)d);
		if (r.CBFcn) XtAddCallback(topLevel, XmNdestroyCallback, r.CBFcn, r.CBFcn_data);
	}
	else
	{
		install_mwm_close_callback(topLevel, dialog_popdown, (XtPointer)d);
		if (r.CBFcn) XtAddCallback(topLevel, XmNpopdownCallback, r.CBFcn, r.CBFcn_data);
	}
	if (r.use_xu_cursor) XtAddCallback(topLevel, XmNpopupCallback, set_dialog_cursor_cb, NULL);

	return d->mainForm;
}



/* Function used to show all of the dialogs created by this library. This is needed as
 * the widget passed back is not that of the shell but that of the main widget which is
 * several layers down.
 */
void XuShowDialog(Widget w)
{
	XuDSP d = _xu_dialog_data(w);
	if (!d || !d->dialog) return;

	w = XtParent(d->dialog);
	if(XtIsManaged(d->dialog))
	{
		/* Already managed so just pop it up */
		if(d->type == TOPLEVEL)
		{
			XtPopdown(w);
			XuDelay(w, 25);
			XtPopup(w, XtGrabNone);
		}
	}
	else
	{
		XtManageChild(d->mainForm);
		XtManageChild(d->dialog);
		if(d->type == TOPLEVEL) XtPopup(w, XtGrabNone);
		XuSetBusyCursor(False);
	}
	_xu_check_frame_size(d, 500);
}


/*  Show the dialog with the given name. The return is either
 *  the widget of the dialog on success or NULL on failure.
 */
Widget XuShowNamedDialog(String name)
{
	int i;
	for(i = 0; i < Fxu.ndl; i++)
	{
		if(!same(Fxu.dl[i]->name, name)) continue;
		XuShowDialog(Fxu.dl[i]->dialog);
		return(Fxu.dl[i]->dialog);
	}
	return (Widget)NULL;
}


void XuHideDialog(Widget w)
{
	XuDSP d = _xu_dialog_data(w);
	if (!d) return;
	if(d->type == TOPLEVEL)
		XtPopdown(XtParent(d->dialog));
	else
		XtUnmanageChild(d->dialog);
	XuUpdateDisplay(d->dialog);
}




/* This will destroy the toplevel shell associated with the widget
 * passed in the calling argument.
 */
void XuDestroyDialog (Widget w)
{
	XuDSP d = _xu_dialog_data(w);

	/* This function should never destroy the application shell */
	if (!d || !d->dialog || same(d->name,TOP_LEVEL_ID)) return;

	/* It results in a more responsive feel if the dialog is popped down
	 * or unmanaged before being destroyed.
	 */
	if(d->type == TOPLEVEL)
		XtPopdown(XtParent(d->dialog));
	else
		XtUnmanageChild(d->dialog);

	XtDestroyWidget(XtParent(d->dialog));
	d->dialog = NullWidget;
}


/* Does the same as XuDestroyDialog but is meant to be used as a
 * callback function.
 */
/*ARGSUSED*/
void XuDestroyDialogCB (Widget w, XtPointer cd, XtPointer notused)
{
	XuDestroyDialog(w);
}


/* Provides a standard exit function for dialogs where the dialog
 * widget is to be set to null when	the dialog is destroyed.  The
 * client data for this function must be the pointer to the dialog
 * widget (ie. &dialog). Usual use with the dialog creation functions:
 *
 *     ....
 *     XuNdestroyCallback, XuExitOnDestroyCallback,
 *     XuNdestroyData,     &dialog,
 *     ....
 */
/*ARGSUSED*/
void XuExitOnDestroyCallback(Widget w, XtPointer cd, XtPointer unused)
{
	if (cd) *((Widget *)cd) = NULL;
}


/* Save dialog locations. Normally this function will not have to be called by the
 * application developer as this is done automatically by the XuDestroyApplication
 * function.
 *
 * Note that the default profile will always save on exit. There is no override
 * option for this action.
 */
void XuSavePositionOfDialogs(void)
{
	int         n = 0;
	Boolean     update = (Fxu.gndx == Fxu.bndx);
	XuPIN_STYLE pinstate = XuPIN_NONE;

	/* Check for the update on exit flag. If present we save the position
	 * of all visible dialogs that can be profiled.
	 */
	if(update || (XuGetProfileStateData(XuActiveProfile, &update, NULL) && update))
	{
		XuPIN_STYLE pin = XuPIN_INITIAL;
		if(XuVaStateDataGet(PROFILED_DIALOGS, PINSTATE, NULL, "%d", &n) == 1)
			pin = (XuPIN_STYLE) n;
		XuSaveDialogProfile(pin);
	}

	/* Save the location of all visible dialogs that are not part of the profile
	 * and the size changes of the profiled dialogs.
	 */
	for( n = 0; n < Fxu.ndl; n++ )
	{
		if(!Fxu.dl[n]->dialog) continue;
		if(XuIsProfiledDialog(Fxu.dl[n]->name))
		{
			if (!update) save_geometry(Fxu.dl[n]);
		}
		else
		{
			pinstate = Fxu.dl[n]->pin;
			Fxu.dl[n]->pin = XuPIN_NONE;
			save_geometry(Fxu.dl[n]);
			Fxu.dl[n]->pin = pinstate;
		}
	}
}


/* Save the position and size of all active dialogs. If the parameter
 * pin is one of XuPIN_INITIAL or XuPIN_ALWAYS then the position of the
 * dialogs will be "pinned" to their saved position and will always appear
 * in the saved position upon initial startup.
 */
void XuSaveDialogProfile(XuPIN_STYLE pin)
{
	int         n;
	int         buflen = 500;
	char        pinbuf[24];
	XuPIN_STYLE pinstate;
	String      buffer;

	/* The default is always to pin initial */
	if(Fxu.gndx == Fxu.bndx) pin = XuPIN_INITIAL;

	/* Scan through our dialog data structure looking for dialogs that
	 * are currently active and save their data.
	 */
	buffer = XTCALLOC(buflen, char);
	for( n = 0; n < Fxu.ndl; n++ )
	{
		if(!Fxu.dl[n]->dialog) continue;
		pinstate = Fxu.dl[n]->pin;
		Fxu.dl[n]->pin = XuPIN_NONE;
		save_geometry(Fxu.dl[n]);
		Fxu.dl[n]->pin = pinstate;
		if(strlen(buffer) + strlen(Fxu.dl[n]->name) + 2 > buflen)
		{
			buffer = (String) XtRealloc(buffer, (buflen+500)*sizeof(char));
			buffer[buflen] = '\0';
			buflen += 500;
		}
		strcat(buffer, Fxu.dl[n]->name);
		strcat(buffer, ",");
	}
	/* strip trailing comma */
	if(strlen(buffer) > 0) buffer[strlen(buffer)-1] = '\0';

	/* Do an immediate save here in case this is done when the application
	 * is shutting down.
	 */
	_xu_state_data_save(PROFILED_DIALOGS, NULL, NULL, buffer);
	(void) snprintf(pinbuf, 24, "%d", (int) pin);
	_xu_state_data_save(PROFILED_DIALOGS, PINSTATE, NULL, pinbuf);
	XtFree(buffer);
}


/* Return true if the given named dialog is a profiled dialog. This means that the
 * dialog is flagged as a profiled dialog and will be displayed at its given
 * position the next time the program using this library is run (assuming the
 * program actually does this)
 */
Boolean XuIsProfiledDialog(String name)
{
	Boolean rtn = False;
	String  data;

	if(XuStateDataGet(PROFILED_DIALOGS, NULL, NULL, &data))
	{
		rtn = _xu_in_comma_separated_list(data, name);
		XtFree(data);
	}
	return rtn;
}
