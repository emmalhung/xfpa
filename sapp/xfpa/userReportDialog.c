/***************************************************************************/
/*
*  File:     userReportDialog.c
*
*  Function: ACTIVATE_problemReportingDialog()
*
*  Purpose:  Provides the mechanism by which the user can report bugs and
*            or changes to the FPA development team.  The reports
*            entered are automatically forwarded via email to the development
*            team.  Note that this file will create one of two dialogs
*            depending on the function call invoked.
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
/***************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <netdb.h>
#include "global.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include "resourceDefines.h"
#include "help.h"
#include "userReport.h"


static Widget dialog = NULL;
static Widget userName;
static Widget phoneNumber;
static Widget synopsis;
static Widget problem;
static char   ref_number[15];
static String save_file = NULL;

static void ClassCB    (Widget, XtPointer, XtPointer);
static void PriorityCB (Widget, XtPointer, XtPointer);

static XuMenuItemStruct class_list[] = {
	{"support",        &xmPushButtonWidgetClass, 0, None, 1, ClassCB, (XtPointer)0, NULL},
	{"change_request", &xmPushButtonWidgetClass, 0, None, 2, ClassCB, (XtPointer)1, NULL},
	{"doc_bug",        &xmPushButtonWidgetClass, 0, None, 3, ClassCB, (XtPointer)2, NULL},
	{"sw_bug",         &xmPushButtonWidgetClass, 0, None, 4, ClassCB, (XtPointer)3, NULL},
	NULL
};
static String class = NULL;

static XuMenuItemStruct priority_list[] = {
	{"low",    &xmPushButtonWidgetClass, 0, None, 1, PriorityCB, (XtPointer)0, NULL},
	{"medium", &xmPushButtonWidgetClass, 0, None, 2, PriorityCB, (XtPointer)1, NULL},
	{"high",   &xmPushButtonWidgetClass, 0, None, 3, PriorityCB, (XtPointer)2, NULL},
	NULL
};
static String priority = NULL;


/*ARGSUSED*/
static void ClassCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	class = class_list[PTR2INT(client_data)].name;
}


/*ARGSUSED*/
static void PriorityCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	priority = priority_list[PTR2INT(client_data)].name;
}


static void WriteLine(FILE *fp , String header , String data , int ncr )
{
	int n;
	if (header) (void) fputs(header,fp);
	if (header) (void) fputs(" ",fp);
	if (data)   (void) fputs(data,fp);
	for( n=0; n<ncr; n++) (void) fputs("\n",fp);
}


/*ARGSUSED*/
static void SendCB(Widget w , XtPointer notused , XtPointer unused )
{
	int     ipa, ipb, ipc, ipd;
	char    mbuf[2048];
	String  name, phone, syno, prob;
	String  syno_lined, prob_lined;
	String  address, dir;
	FILE    *fp, *fopen();
	struct  utsname vinfo;
	struct  hostent *ent;


	/* Create a working save file. This is in the program working directory
	 * and will be removed when FPA does it directory cleanup.
	 */
	if (!save_file) save_file = tempnam(GV_working_directory, NULL);
	if (!save_file) return;

	name  = XmTextFieldGetString(userName);
	phone = XmTextFieldGetString(phoneNumber);
	syno  = XmTextFieldGetString(synopsis);
	prob  = XmTextGetString(problem);

	if( blank(name) || blank(syno) || blank(prob) )
	{
		XuShowError(dialog, "ProblemReportEntryError", NULL);
		return;
	}

	XuSetBusyCursor(ON);

	/* Send the message via email
	*/
	syno_lined = LineBreak(syno,65,5);
	prob_lined = LineBreak(prob,65,100);

	(void) uname(&vinfo);
	fp = fopen(save_file,"w");
	if (!fp) return;

	/* Set up the reply address line in the message. This is done as some
	*  systems messages would come in without a return address.
	*/
	(void)gethostname(mbuf, 256);
	ent = gethostbyname(mbuf);

	/* There must not be a space before Subject or Reply-to below.
	*/
	if(ent)
	{
		(void) snprintf(mbuf, sizeof(mbuf), " %s@'%s'", getlogin(), ent->h_aliases[0]);
		WriteLine(fp, "Reply-To:", mbuf, 1);
	}
	WriteLine(fp, "Subject:", "Fpa Auto Mail: Problem Report", 2);

	WriteLine(fp, " Senders Name: ", name, 1);
	WriteLine(fp, " Phone Number: ", phone, 1);

	if(ent)
	{
		ipa = ent->h_addr[0];  if (ipa < 0) ipa += 256;
		ipb = ent->h_addr[1];  if (ipb < 0) ipb += 256;
		ipc = ent->h_addr[2];  if (ipc < 0) ipc += 256;
		ipd = ent->h_addr[3];  if (ipd < 0) ipd += 256;
		(void) snprintf(mbuf, sizeof(mbuf), " %s@%s   (%d.%d.%d.%d)",
			getlogin(), ent->h_aliases[0], ipa, ipb, ipc, ipd);
		WriteLine(fp, "Reply Address:", mbuf, 1);
	}

	WriteLine(fp, "  Reference No: ", ref_number, 1);
	WriteLine(fp, "   FPA Version: ", FpaRevision, 1);
	WriteLine(fp, "   System Type: ", vinfo.sysname, 1);
	WriteLine(fp, "System Version: ", vinfo.release, 1);
	WriteLine(fp, "  Machine Type: ", vinfo.machine, 1);
	WriteLine(fp, "         Class: ", XuGetLabel(class), 1);
	WriteLine(fp, "      Priority: ", XuGetLabel(priority), 2);
	WriteLine(fp, "Problem Report", "Synopsis:", 1);
	WriteLine(fp, syno_lined, NULL, 2);
	WriteLine(fp, "Problem Report", "Description:", 1);
	WriteLine(fp, prob_lined, NULL, 2);

	if(same(class, class_list[0].name))
	{
		memset(mbuf, (Byte)'-', 65);
		mbuf[65] = '\0';
		WriteLine(fp, mbuf, "", 2);
	}
	fclose(fp);

	FreeItem(name);
	FreeItem(phone);
	FreeItem(syno);
	FreeItem(syno_lined);
	FreeItem(prob);
	FreeItem(prob_lined);

	/* The sendmail program is found in /usr/lib in the HP
	*  systems so we include this in the path just in case.
	*/
	strcpy(mbuf, "(PATH=$PATH:/usr/lib;");

	/* If the problem class is bug we package up the depiction, interpolation
	*  and forecast working directories and ship them as well.
	*/
	address = XuGetStringResource(RNmailAddress,NULL);
	if (!address) return;

	if(same(class, class_list[3].name))
	{
		strcat(mbuf, "cp ");
		strcat(mbuf, GetSetupFile(0,NULL));
		strcat(mbuf, " /tmp/fpa-setupfile;");
		strcat(mbuf, "env > /tmp/fpa-environment;");
		strcat(mbuf, "xrdb -query > /tmp/fpa-X-server;");
		strcat(mbuf, "tar cf - ");
		dir = source_directory_by_name(DEPICT, NULL, NULL);
		strcat(mbuf, dir);
		strcat(mbuf, " ");
		dir = source_directory_by_name(INTERP, NULL, NULL);
		strcat(mbuf, dir);
		strcat(mbuf, " -C ");
		dir = get_directory(FCST_WORK);
		strcat(mbuf, dir_name(dir));
		strcat(mbuf, " ");
		strcat(mbuf, base_name(dir, NULL));
		strcat(mbuf, " -C /tmp fpa-setupfile fpa-environment fpa-X-server");
		strcat(mbuf, "|compress");
		strcat(mbuf, "|uuencode bug.data.Z");
		strcat(mbuf, "|sed s\"/begin 0 bug.data.Z/begin 666 bug.data.Z/1\" >> ");
		strcat(mbuf, save_file);
		strcat(mbuf, "; rm /tmp/fpa-setupfile /tmp/fpa-environment /tmp/fpa-X-server;");
	}
	strcat(mbuf, "sendmail ");
	strcat(mbuf, address);
	strcat(mbuf, " < ");
	strcat(mbuf, save_file);
	strcat(mbuf, ") &");
	system(mbuf);
	XuSetBusyCursor(OFF);
	XuDestroyDialog(dialog);
}


void ACTIVATE_problemReportingDialog(Widget parent )

{
	int	   year, jday, hour, min, sec;
	Widget descriptionWindow, synopsisLabel, problemLabel, rowcol;
	Widget w, refNumb, header, classSelect;

	static XuDialogActionsStruct action_items[] = {
		{"sendBtn",   SendCB,            NULL},
		{"cancelBtn", XuDestroyDialogCB, NULL},
		{"helpBtn",   HelpCB,            HELP_PROBLEM_REPORTING}
	};

	if (dialog) XuShowDialog(dialog);
	if (dialog) return;

	/* Determine the reference number.  This is simply the last two digits
	*  of the year followed by the three digit julian day of the year and
	*  the hour and minute the report was generated.
	*/
	systime(&year,&jday,&hour,&min,&sec);
	(void) snprintf(ref_number,sizeof(ref_number), "P%.2d%.3d%.2d%.2d", year%100, jday, hour, min);

	/* Create the dialog
	*/
	dialog = XuCreateToplevelFormDialog(parent, "userReport",
		XmNkeyboardFocusPolicy, XmEXPLICIT,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	refNumb = XtVaCreateManagedWidget(ref_number,
		xmLabelWidgetClass, dialog,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	(void)XtVaCreateManagedWidget("refLabel",
		xmLabelWidgetClass, dialog,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, refNumb,
		XmNrightOffset, 6,
		NULL);

	header = XtVaCreateManagedWidget("problemHeader",
		xmLabelWidgetClass, dialog,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	classSelect = XuVaMenuBuildOption(dialog, "classSelect", class_list,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, header,
		XmNtopOffset, 19,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

	XuMenuSelectItem(classSelect, 1);
	class = class_list[0].name;

	w = XuVaMenuBuildOption(dialog, "prioritySelect", priority_list,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, header,
		XmNtopOffset, 19,
        XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, classSelect,
		XmNleftOffset, 40,
        NULL);

	XuMenuSelectItem(w, 1);
	priority = priority_list[0].name;

	rowcol = XtVaCreateWidget("rcname",
		xmRowColumnWidgetClass, dialog,
		XmNorientation, XmHORIZONTAL,
		XmNspacing, 3,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, classSelect,
		XmNtopOffset, 20,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XtVaCreateManagedWidget("userNameLabel",
		xmLabelWidgetClass, rowcol,
		NULL);

	userName = XtVaCreateManagedWidget("userName",
		xmTextFieldWidgetClass, rowcol,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		NULL);
	XtAddCallback(userName, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);

	(void) XtVaCreateManagedWidget("phoneLabel",
		xmLabelWidgetClass, rowcol,
		XmNmarginLeft, 30,
		NULL);

	phoneNumber = XtVaCreateManagedWidget("phoneNumber",
		xmTextFieldWidgetClass, rowcol,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		NULL);
	XtAddCallback(phoneNumber, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);

	XtManageChild(rowcol);

	synopsisLabel = XtVaCreateManagedWidget("synopsisLabel",
		xmLabelWidgetClass, dialog,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, rowcol,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	synopsis = XtVaCreateManagedWidget("synopsis",
		xmTextFieldWidgetClass, dialog,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, synopsisLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(synopsis, XmNactivateCallback,
		(XtCallbackProc)XmProcessTraversal, (XtPointer)XmTRAVERSE_NEXT_TAB_GROUP);

	problemLabel = XtVaCreateManagedWidget("problemLabel",
		xmLabelWidgetClass, dialog,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, synopsis,
		XmNtopOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	descriptionWindow = XtVaCreateManagedWidget("descriptionWindow",
		xmScrolledWindowWidgetClass, dialog,
		XmNborderWidth, 0,
		XmNshadowThickness, 0,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, problemLabel,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	problem = XtVaCreateManagedWidget("problem",
		xmTextWidgetClass, descriptionWindow,
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNwordWrap, False,
		XmNscrollHorizontal, False,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		NULL);

	XmAddTabGroup(userName);
	XmAddTabGroup(phoneNumber);
	XmAddTabGroup(synopsis);
	XmAddTabGroup(problem);

	XuShowDialog(dialog);
}
