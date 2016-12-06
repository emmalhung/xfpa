/*========================================================================*/
/*
*	File:		userLicense.c
*
*	Purpose:	Show the opening copyright notice and show notice of
*				license expiry date or notice of license expiry.
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

#include "global.h"
#include <FpaXgl.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/MwmUtil.h>
#include "resourceDefines.h"
#include "fallback.h"
#include "userReport.h"

#define CHECK_INTERVAL	1800000		/* Interval between license checks in milliseconds */

enum { OK = 100, ALMOST_EXPIRED, NOT_FOUND, EXPIRED, LOST, IN_USE };

/* Local variables.
*/
static Widget  dialog  = NullWidget;
static Pixmap  cdnflag = (Pixmap)NULL;
static int     status  = OK;


/*======================= Private Functions =========================*/

/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer unused )
{
	if (status > ALMOST_EXPIRED) exit(0);
	status = OK;
	RemoveHelloMessage();
}


/* The license is checked once in a while to make sure that it is still around.
 * This is really to prevent some forms of cheating.
 */
/*ARGSUSED*/
static void check_license_timeout(XtPointer  client_data , XtIntervalId *id )
{
	int code, days;
	Boolean valid = check_license();
	if(!valid)
		valid = fpa_license(GV_edit_mode? "editor":"viewer", &days, &code);
	if(valid)
	{
		(void) XtAppAddTimeOut(GV_app_context, CHECK_INTERVAL, check_license_timeout, NULL);
	}
	else
	{
		status = LOST;
		ShowHelloMessage();
	}
}


/*======================== Public Functions ==========================*/


/* Check the license. If we have one check to see if it is within two weeks
 * of expiring, and if so set status to the number of days left. If not in
 * edit mode exit the program, else put up a dialog with a message depending
 * on the code return and exit once it is answered. If there is a no license
 * condition a toplevel widget needs to be created as this function must be
 * called before any widget activity.
*/
void GetLicense(int argc, String *argv, String font_path)
{
	int code, days;

	if(GV_edit_mode)
	{
		if(fpa_license("editor", &days, &code))
		{
			if(days < 15) status = days;
		}
		else
		{
			switch(code)
			{
				case 1:  status = EXPIRED;   break;
				case 2:  status = NOT_FOUND; break;
				case 3:  status = IN_USE;    break;
				default: status = NOT_FOUND; break;
			}

			GW_topLevel = XuVaAppInitialize(&GV_app_context, GV_app_class,
				NULL, 0,
				&argc, argv,
				fallback_resources,
				XuNdestroyCallback, exit_cb,
				XuNallowProfileSelection, False,
				XuNfontPath, font_path,
				NULL);

			ShowHelloMessage();
		}
	}
	else if(!fpa_license("viewer", &days, &code))
	{
		exit(1);
	}
}


/* Normally this just displays a hello and the copyright information. If status is
 * not OK, then there is some problem and we must put up a message that the user
 * must respond to by clicking an ok button. The result will depend on the status.
 * Note that the fpalogo pixmap is not destroyed later in the code as the main
 * program uses this same pixmap and it is cached so we keep it around.
 */
void ShowHelloMessage(void)
{
	Widget  btn, contact, icon, message, sep, welcome;
	Pixmap  fpalogo;
	String  errmsg = "License check error";

	if(!GV_edit_mode) return;

	/* GW_mainWindow == NULL means that we are dealing with an application that
	 * has not gone through the normal route and GW_topLevel has been created
	 * by the above function.Thus we must take a slightly different path at the
	 * start and end of the dialog.
	 */
	if(GW_mainWindow)
	{
		dialog = XuCreateFormDialog(GW_mainWindow, "ul",
			XmNmanageChild, False,
			XmNnoResize, True,
			XmNdefaultPosition, True,
			XmNhorizontalSpacing, 15,
			XmNverticalSpacing, 15,
			XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
			NULL);

		XtVaSetValues(XuGetShell(dialog),
			XmNmwmDecorations, MWM_DECOR_BORDER,
			NULL);
	}
	else
	{
		XtVaSetValues(GW_topLevel,
			XmNmwmDecorations, MWM_DECOR_BORDER,
			XmNgeometry, NULL,
			NULL);

		dialog = XmVaCreateForm(GW_topLevel, "ul",
			XmNdefaultPosition, True,
			XmNhorizontalSpacing, 15,
			XmNverticalSpacing, 15,
			NULL);
	}

 	fpalogo = XuGetPixmap(dialog, "fpalogo");
	cdnflag = XuGetPixmap(dialog, "flag.canadian.64");

	icon = XmVaCreateManagedLabel(dialog, "icon",
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, fpalogo,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	welcome = XmVaCreateManagedLabel(dialog, "welcome",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, icon,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, icon,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, icon,
		XmNbottomOffset, 0,
		NULL);

	(void) XmVaCreateManagedLabel(dialog, "fcr",
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, cdnflag,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, icon,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, welcome,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, icon,
		XmNbottomOffset, 0,
		NULL);

	sep = XmVaCreateManagedSeparator(dialog, "sep",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, welcome,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	if(status == OK)
	{
		(void) XmVaCreateManagedLabel(dialog, "cr",
			XmNalignment, XmALIGNMENT_CENTER,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	}
	else
	{
		message = XmVaCreateManagedLabel(dialog, "message",
			XmNalignment, XmALIGNMENT_CENTER,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		switch(status)
		{
			case IN_USE:
				XuWidgetLabel(message, XuGetStringResource(RNuserLicenseAllInUse,errmsg));
				break;
			case EXPIRED:
				XuWidgetLabel(message, XuGetStringResource(RNuserLicenseExpired,errmsg));
				break;
			case LOST:
				XuWidgetLabel(message, XuGetStringResource(RNuserLicenseLost,errmsg));
				break;
			case NOT_FOUND:
				XuWidgetLabel(message, XuGetStringResource(RNuserLicenseNoLicense,errmsg));
				break;
			default:
				XuWidgetPrint(message, XuGetStringResource(RNuserLicenseAboutToExpire,errmsg), status);
				break;
		}

		contact = XmVaCreateManagedLabel(dialog, "contact",
			XmNalignment, XmALIGNMENT_CENTER,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, message,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		sep = XmVaCreateManagedSeparator(dialog, "sep",
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, contact,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		btn = XmVaCreateManagedPushButton(dialog, "okBtn",
			XmNrecomputeSize, False,
			XmNwidth, 100,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 50,
			XmNleftOffset, -50,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtAddCallback(btn, XmNactivateCallback, exit_cb, NULL);
	}

	if(GW_mainWindow)
	{
		XuShowDialog(dialog);
		XuSetDialogCursor(dialog, XuDEFAULT_CURSOR, ON);
		(void) XtAppAddTimeOut(GV_app_context, CHECK_INTERVAL, check_license_timeout, NULL);
		XuUpdateDisplay(dialog);
	}
	else
	{
		XtManageChild(dialog);
		XtRealizeWidget(GW_topLevel);
		XuShellCenter(dialog);
		XtAppMainLoop(GV_app_context);
	}
}


/* We do not remove the mesage if the status is not ok.  We want
 * the user to push the button so as to acknowledge the message.
 */
void RemoveHelloMessage(void)
{
	if (!dialog)       return;
	if (!GV_edit_mode) return;
	if (status != OK ) return;

	XtUnmapWidget(XuGetShell(dialog));
	XuDelay(dialog, 100);
	glSwapBuffers();
	XuFreePixmap(dialog, cdnflag);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}
