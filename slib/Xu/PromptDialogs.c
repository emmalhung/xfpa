/****************************************************************************/
/*
*  File:     XuPromptDialogs.c
*
*  Purpose:  Contains general purpose reusable dialogs which read their
*            title, message and position from the a special message database
*            file named XFpaMdb and stored in the app-defaults directory.
*
*  Note: The following resources must be defined in the resource file of
*        the program using these dialogs.
*
*             *.cancelBtn.labelString: Cancel
*             *.okBtn.labelString: Ok
*             *.noBtn.labelString: No
*             *.yesBtn.labelString: Yes
*
*        The actual label, will of course, be language dependent.
*
*  Functions: XuMakeActionRequest()
*             XuShowError()
*             XuShowMessage()
*             XuAskUser()
*
*  Note: The XuAskUser function has a special mode that will include a toggle
*        button in the dialog. See the function description for details. The
*        label for this button is stored in the XFpaMdb file with the key
*        "toggleLabel".
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
#include "XuP.h"
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/MessageB.h>
#include <Xm/ToggleB.h>
#include <Xm/MwmUtil.h>
#include <stdarg.h>

const String cancelBtn = "*.cancelBtn.labelString";
const String okBtn     = "*.okBtn.labelString";
const String noBtn     = "*.noBtn.labelString";
const String yesBtn    = "*.yesBtn.labelString";
const String yesAllBtn = "*.yesAllBtn.labelString";

/*
 *	Function for putting dialogs into synchronous mode. All other activity
 *	will be locked out until the given widget is unmanaged or set NULL.
 *	Note that the address of the widget must be passed in. The variable
 *	sync_return_value is used to set and get a return value from within
 *	the various dialogs.
 */
static XuRETURN sync_return_value;

static void conduct_sync_interaction(Widget *w )
{
	XtAppContext ac = XtWidgetToApplicationContext(*w);
	sync_return_value = XuNO_RETURN;
	while((*w != (Widget)NULL && XtIsManaged(*w)) || XtAppPending(ac))
	{
		XtAppProcessEvent(ac, XtIMAll);
	}
}


static Cardinal set_visuals(Widget refw, ArgList al, Cardinal ac)
{
	int depth;
	Visual *visual;
	Colormap colormap;

	XtVaGetValues(XuGetShell(refw),
		XmNdepth, &depth,
		XmNvisual, &visual,
		XmNcolormap, &colormap,
		NULL);

	XtSetArg(al[ac], XmNdepth, depth); ac++;
	XtSetArg(al[ac], XmNvisual, visual); ac++;
	XtSetArg(al[ac], XmNcolormap, colormap); ac++;
	return ac;
}


static void dialog_info( Widget ref_widget, String key, Widget *parent, String *title,
						String *msg, Position *x, Position *y, Boolean *default_pos)
{
	int fx, fy;
	String ptr;
	Position dx, dy;

	static String errmsg = "Error";

	*parent = XuGetShell(ref_widget);

	*title = XuFindKeyLine(key, "title", errmsg);

	ptr = XuFindKeyLine(key, "posn", "0 0");
	*default_pos = (*ptr == 'd' || *ptr == 'D');
	if(*default_pos)
		fx = fy = 0;
	else
		(void) sscanf(ptr,"%d %d", &fx, &fy);
	XtTranslateCoords(ref_widget, 0, 0, &dx, &dy);
	*x = dx + (Position)fx;
	*y = dy + (Position)fy;

	*msg = XuFindKeyLine( key, "dialog", errmsg);
	if(same(*msg,errmsg))
		(void) fprintf(stderr,"XuPromptDialog: Key item \"%s\" not in message database.\n", key);
}


/*
*  Function: XuMakeActionRequest()
*
*  Purpose:  Provides a general purpose confirmation dialog which displays
*			a message and three buttons - yes, no and cancel.
*
*  Parameters:	parent- The widget which is to be used to as a reference
*                       point for the positioning of this dialog.
*				key   - The key which identifier which message to use from
*                       the interface message file.
*/
static void action_request_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XuREQUEST_TYPE type = (XuREQUEST_TYPE)client_data;
	XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)call_data;

	if( type == XuYYAN )
	{
		if( cbs->reason == XmCR_OK ) {
			sync_return_value = XuYES;
		} else if( cbs->reason == XmCR_CANCEL ) {
			sync_return_value = XuYES_ALL;
		} else {
			sync_return_value = XuNO;
		}
	}
	else
	{
		if( cbs->reason == XmCR_OK ) {
			sync_return_value = XuYES;
		} else if( cbs->reason == XmCR_CANCEL ) {
			sync_return_value = XuNO;
		} else {
			sync_return_value = XuCANCEL;
		}
	}
	XtUnmanageChild(w);
}

/*
*  Parameters: widget   - the reference widget to use for positional reference
*              key      - The key into the message database.
*              va_alist - variable argument list of string to go into message.
*/
XuRETURN XuMakeActionRequest( Widget ref_widget, XuREQUEST_TYPE type, String key, ...)
{
	Cardinal ac;
	char mbuf[1024];
	String rmsg, rtitle;
	XmString msg, title, yes, no, cancel;
	Boolean default_pos;
	Position x, y;
	Widget dialog, parent;
	Arg al[15];
	va_list args;

	dialog_info(ref_widget, key, &parent, &rtitle, &rmsg, &x, &y, &default_pos);
	title = XuNewXmString(rtitle);
	va_start(args, key);
	(void) vsnprintf(mbuf, 1024, rmsg, args);
	va_end(args);
	msg = XuNewXmString(mbuf);

	if( type == XuYYAN )
	{
		yes = XuGetXmStringResource(yesBtn, "Yes");
		no = XuGetXmStringResource(yesAllBtn, "Yes To All");
		cancel = XuGetXmStringResource(noBtn, "No");
	}
	else
	{
		yes = XuGetXmStringResource(yesBtn, "Yes");
		no = XuGetXmStringResource(noBtn, "No");
		cancel = XuGetXmStringResource(cancelBtn, "Cancel");
	}

	ac = 0;
	ac = set_visuals(ref_widget, al, ac);
	XtSetArg(al[ac], XmNx, x); ac++;
	XtSetArg(al[ac], XmNy, y); ac++;
	XtSetArg(al[ac], XmNdialogTitle, title); ac++;
	XtSetArg(al[ac], XmNokLabelString, yes); ac++;
	XtSetArg(al[ac], XmNcancelLabelString, no); ac++;
	XtSetArg(al[ac], XmNhelpLabelString, cancel); ac++;
	XtSetArg(al[ac], XmNmessageString, msg); ac++;
	XtSetArg(al[ac], XmNdefaultPosition, default_pos); ac++;
	XtSetArg(al[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
	XtSetArg(al[ac], XmNnoResize, True); ac++;
	XtSetArg(al[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
	XtSetArg(al[ac], XmNautoUnmanage, False); ac++;
	dialog = XmCreateQuestionDialog( parent, "XuMAR", al, ac);

	XtAddCallback(dialog, XmNokCallback,     action_request_cb, (XtPointer)type);
	XtAddCallback(dialog, XmNcancelCallback, action_request_cb, (XtPointer)type);
	XtAddCallback(dialog, XmNhelpCallback,   action_request_cb, (XtPointer)type);

	XtManageChild(dialog);

	XmStringFree(title);
	XmStringFree(msg);
	XmStringFree(yes);
	XmStringFree(no);
	XmStringFree(cancel);

	XuSetDialogCursor(dialog, XuDEFAULT_CURSOR, True);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, True);
	conduct_sync_interaction(&dialog);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, False);
	XuDestroyDialog(dialog);
	return sync_return_value;
}


/*
*  Function: XuShowError()
*
*  Parameters: widget - the reference widget to use for positional reference
*              key    - The key into the message database.
*              ...    - variable argument list of strings to go into message.
*/
void XuShowError(Widget ref_widget, String key, ...)
{
	Cardinal ac;
	char mbuf[1024];
	Arg al[12];
	String rmsg, rtitle;
	Boolean default_pos;
	Position x, y;
	XmString msg, title, ok;
	Widget   parent, dialog;
	va_list args;

	dialog_info(ref_widget, key, &parent, &rtitle, &rmsg, &x, &y, &default_pos);
	title = XuNewXmString(rtitle);
	va_start(args, key);
	(void) vsnprintf(mbuf, 1024, rmsg, args);
	va_end(args);
	msg = XuNewXmString(mbuf);

	ok = XuGetXmStringResource(okBtn,"Ok");
	ac = 0;
	ac = set_visuals(ref_widget, al, ac);
	XtSetArg(al[ac], XmNx, x); ac++;
	XtSetArg(al[ac], XmNy, y); ac++;
	XtSetArg(al[ac], XmNdialogTitle, title); ac++;
	XtSetArg(al[ac], XmNcancelLabelString, ok); ac++;
	XtSetArg(al[ac], XmNmessageString, msg); ac++;
	XtSetArg(al[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
	XtSetArg(al[ac], XmNnoResize, True); ac++;
	XtSetArg(al[ac], XmNdialogType, XmDIALOG_ERROR); ac++;
	XtSetArg(al[ac], XmNdefaultPosition, default_pos); ac++;
	dialog = XmCreateErrorDialog( parent, "XuED", al, ac);

	XtUnmanageChild( XmMessageBoxGetChild( dialog, XmDIALOG_OK_BUTTON)   );
	XtUnmanageChild( XmMessageBoxGetChild( dialog, XmDIALOG_HELP_BUTTON) );
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)XtUnmanageChild, NULL);
	XtManageChild(dialog);

	XmStringFree(title);
	XmStringFree(msg);
	XmStringFree(ok);

	XuSetDialogCursor(dialog, XuDEFAULT_CURSOR, True);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, True);
	conduct_sync_interaction(&dialog);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, False);
	XuDestroyDialog(dialog);
}


/* Function: ShowMessage()
*
*  Purpose:  Provides a general purpose message dialog.
*
*  Parameters: widget - the reference widget to use for positional reference
*              key    - The key into the message database.
*              ...    - variable argument list of strings to go into message.
*/
void XuShowMessage(Widget ref_widget, String key, ...)
{
	Cardinal ac;
	char mbuf[1024];
	String rmsg, rtitle;
	Arg al[13];
	Boolean default_pos;
	Position x, y;
	XmString msg, title, ok;
	Widget   parent, dialog;
	va_list args;

	dialog_info(ref_widget, key, &parent, &rtitle, &rmsg, &x, &y, &default_pos);
	title = XuNewXmString(rtitle);
	va_start(args, key);
	(void) vsnprintf(mbuf, 1024, rmsg, args);
	va_end(args);
	msg = XuNewXmString(mbuf);

	ok = XuGetXmStringResource(okBtn,"Ok");
	ac = 0;
	ac = set_visuals(ref_widget, al, ac);
	XtSetArg(al[ac], XmNx, x); ac++;
	XtSetArg(al[ac], XmNy, y); ac++;
	XtSetArg(al[ac], XmNdialogTitle, title); ac++;
	XtSetArg(al[ac], XmNcancelLabelString, ok); ac++;
	XtSetArg(al[ac], XmNmessageString, msg); ac++;
	XtSetArg(al[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
	XtSetArg(al[ac], XmNnoResize, True); ac++;
	XtSetArg(al[ac], XmNdialogType, XmDIALOG_WARNING); ac++;
	XtSetArg(al[ac], XmNmessageAlignment, XmALIGNMENT_CENTER); ac++;
	XtSetArg(al[ac], XmNdefaultPosition, default_pos); ac++;
	dialog = XmCreateMessageDialog( parent, "XuMD", al, ac );

	XmStringFree(title);
	XmStringFree(msg);
	XmStringFree(ok);

	XtUnmanageChild( XmMessageBoxGetChild( dialog, XmDIALOG_OK_BUTTON   ) );
	XtUnmanageChild( XmMessageBoxGetChild( dialog, XmDIALOG_HELP_BUTTON ) );
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)XtUnmanageChild, NULL);
	XtManageChild(dialog);

	XuSetDialogCursor(dialog, XuDEFAULT_CURSOR, True);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, True);
	conduct_sync_interaction(&dialog);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, False);
	XuDestroyDialog(dialog);
}


/*
*  Function: XuAskUser()
*
*  Purpose:  Provides a general purpose confirmation dialog which displays
*            a message and two buttons - yes and no.
*
*  Parameters: widget - the reference widget to use for positional reference
*              key    - The key into the message database.
*              ...    - variable argument list of strings to go into message.
*
*  Special: If after supplying all of the necessary variable arguments to
*           the function as required by the message in the message database
*           the keyword XuNaskUserToggle is found then a toggle button will be
*           created as a child of the dialog. The second following argument
*           must the a pointer to the address of a variable which will contain
*           the state of the toggle button on exit from this function.
*           Thus " XuNaskUserToggle,&state ". Note that the label for the toggle 
*           button must be in the XFpaMdb message database file used by this
*           dialog. It is recognized by the tag "toggleLabel" so for a given
*           key this will be <key>.toggleLabel:
*/
/*ARGSUSED*/
static void ask_user_cb(Widget w, XtPointer unused, XmAnyCallbackStruct *cbs)
{
    if(cbs->reason == XmCR_OK )
		sync_return_value = XuYES;
	else
		sync_return_value = XuNO;
	XtUnmanageChild(w);
}


static void ask_user_toggle_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int *state = (int *) client_data;
	*state = XmToggleButtonGetState(w);
}


XuRETURN XuAskUser( Widget ref_widget, String key, ...)
{
	Cardinal ac;
	char mbuf[1204];
	String rtitle, rmsg, ptr, toggle_label = NULL;
	Arg al[14];
	Boolean default_pos;
	int *toggle_state = NULL;
	Position x, y;
	XmString msg, title, yes, no;
	Widget   dialog, parent;
	va_list args;


	dialog_info(ref_widget, key, &parent, &rtitle, &rmsg, &x, &y, &default_pos);
	title = XuNewXmString(rtitle);
	va_start(args, key);
	(void) vsnprintf(mbuf, 1024, rmsg, args);
	while((ptr = va_arg(args,String)))
	{
		if(!same(ptr,XuNaskUserToggle)) continue;
		toggle_state = va_arg(args,int*);
		break;
	}
	va_end(args);
	msg = XuNewXmString(mbuf);

	yes = XuGetXmStringResource(yesBtn, "Yes");
	no = XuGetXmStringResource(noBtn, "No");

	ac = 0;
	ac = set_visuals(ref_widget, al, ac);
	XtSetArg(al[ac], XmNx, x); ac++;
	XtSetArg(al[ac], XmNy, y); ac++;
	XtSetArg(al[ac], XmNdialogTitle, title); ac++;
	XtSetArg(al[ac], XmNokLabelString, yes); ac++;
	XtSetArg(al[ac], XmNcancelLabelString, no); ac++;
	XtSetArg(al[ac], XmNmessageString, msg); ac++;
	XtSetArg(al[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
	XtSetArg(al[ac], XmNnoResize, True); ac++;
	XtSetArg(al[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
	XtSetArg(al[ac], XmNdefaultPosition, default_pos); ac++;
	dialog = XmCreateQuestionDialog( parent, "XuYND", al, ac);

	if(toggle_state)
	{
		XmString xmlabel = XmStringGenerate(XuFindKeyLine(key,"toggleLabel","??"), NULL, XmCHARSET_TEXT, NULL);
		Widget w = XmVaCreateManagedToggleButton(dialog, "tb",
			XmNset, (Boolean) *toggle_state,
			XmNlabelString, xmlabel,
			NULL);
		XmStringFree(xmlabel);
		XtAddCallback(w, XmNvalueChangedCallback, ask_user_toggle_cb, (XtPointer) toggle_state);
	}

	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback(dialog, XmNokCallback,     (XtCallbackProc)ask_user_cb, NULL);
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)ask_user_cb, NULL);
	XtManageChild(dialog);

	XmStringFree(title);
	XmStringFree(msg);
	XmStringFree(yes);
	XmStringFree(no);

	XuSetDialogCursor(dialog, XuDEFAULT_CURSOR, True);
	XuSetDialogCursor(parent,XuSTOP_CURSOR, True);
	conduct_sync_interaction(&dialog);
	XuSetDialogCursor(parent,XuSTOP_CURSOR,False);
	XuDestroyDialog(dialog);
	return sync_return_value;
}
