/*========================================================================*/
/*
*	File:		text_dialog.c
*
*	Purpose:	Provides an independent interface for handling regular
*               issue text forecasts.
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

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "global.h"
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include "resourceDefines.h"
#include "help.h"
#include "fpapm.h"
#include "productStatus.h"
#include "fcstText.h"

#define  INFO_FILE  "FcstAreas"
#define  WORKING    "working"
#define  RELEASED   "released"
#define  MERGED     "merged"
#define  STATUS		"status"


static void    edit_cb				 (Widget, XtPointer, XtPointer);
static void    forecast_cb			 (Widget, XtPointer, XtPointer);
static void    generate_cb			 (Widget, XtPointer, XtPointer);
static Boolean is_released           (int);
static void    language_cb			 (Widget, XtPointer, XtPointer);
static void    print_cb			     (Widget, XtPointer, XtPointer);
static void    release_cb			 (Widget, XtPointer, XtPointer);
static void    run_fog				 (void);
static void    set_status_display	 (void);
static void    set_status_indicators (FOG_DATA_PTR, TEXT_STATE);
static void    source_cb			 (Widget, XtPointer, XtPointer);
static void    special_settings_cb	 (Widget, XtPointer, XtPointer);

static Widget dialog        = NULL;
static Widget fcstList      = NULL;
static Widget text          = NULL;
static Widget statusLabel   = NULL;
static Widget workStatus    = NULL;
static Widget releaseStatus = NULL;
static Widget amendStatus   = NULL;
static Widget editBtn       = NULL;
static Widget areasBtn      = NULL;

static String language          = NULL;
static String source            = WORKING;
static int nfcsts               = 0;
static FOG_DATA_PTR active_fcst = NULL;
static FOG_DATA_PTR fcsts       = NULL;


/*============================================================================*/
/*
*	InitFcstTextDialog() - Read the setup file and initialize the product
*	status database.
*/
/*============================================================================*/
void InitFcstTextDialog(void)
{
	int	    i;
	FOG_DATA_PTR f;
	SETUP   *setup;
	INFOFILE  fh;

	language = GV_language[0].key;
	for(i = 0; i < GV_nlanguages; i++)
	{
		if(same(getenv("LANG"),GV_language[i].key)) language = GV_language[i].key;
	}

	setup  = GetSetup(PROD_TEXT);
	nfcsts = setup->nentry;
	fcsts  = NewMem(FOG_DATA, setup->nentry);
	active_fcst = (nfcsts > 0) ? fcsts:NULL;

	fh = info_file_open(get_path(FCST_WORK,INFO_FILE));

	for ( i = 0; i < nfcsts; i++)
	{
		/* Allocate the key information from the setup file */
		f = fcsts + i;
		f->order_no = i;
		f->label = SetupParm(setup, i, 0);
		f->key   = SetupParm(setup, i, 1);

		/* Is this a spacing entry?
		*/
		if(same(f->label,"<->"))
		{
			f->label       = " ";
			f->key         = NULL;
			f->product_id  = 0;
			f->state       = SPACER;
			f->area_states = NULL;
		}
		else if(IsNull(f->key))
		{
			f->product_id  = 0;
			f->state       = HEADER;
			f->area_states = NULL;
		}
		else
		{
			f->product_id    = ProductStatusAddInfo(PS_TEXT_FCST, f->label, is_released);
			f->state         = READY;
			f->area_states   = NULL;
			f->element_order = NULL;

			/* XXX - Not used for now...
			InitFcstTextPriority(f, fh);
			*/
			InitFcstTextAreas(f, fh);
		}
	}
	info_file_close(fh);
}


/*============================================================================*/
/*
*	WriteTextFcstInfoFile() - This function is called by the utility dialogs
*                             associated with this main dialog. This function
*   writes out the information stored with each forecast into the "info" file
*   required by FoG. Only element priority and forecast areas are written out.
*/
/*============================================================================*/
void WriteTextFcstInfoFile(void)
{
	int i, j, nareas;
    char sbuf[1025];
    String *name_list;
    INFOFILE fh;

    fh = info_file_create(get_path(FCST_WORK, INFO_FILE));
    for(i = 0; i < nfcsts; i++)
    {
        if(fcsts[i].state == SPACER) continue;
        if(fcsts[i].state == HEADER) continue;

		if(!fcsts[i].element_order && !fcsts[i].area_states) continue;

        /* First write out the block header for this forecast.
        */
        info_file_write_block_header(fh, fcsts[i].key);

        /* Now create the ordered list of elements to mention in forecast
        *  and update the forecast element order list. Note that the
		*  element_order array will always have a trailing NULL.
        */
		if(fcsts[i].element_order)
		{
			sbuf[0] = '\0';
			j = 0;
			while(!blank(fcsts[i].element_order[j]))
			{
				strcat(sbuf, fcsts[i].element_order[j]);
				strcat(sbuf, ",");
				j++;
			}
			sbuf[safe_strlen(sbuf)-1] = '\0';
			info_file_write_line(fh, FOG_ELEMENT_ORDER, sbuf);
		}
        if(fcsts[i].area_states)
		{
			sbuf[0] = '\0';
			nareas = FcstAreaList(&fcsts[i], &name_list, NULL);
			for(j = 0; j < nareas; j++)
			{
				if(!fcsts[i].area_states[j]) continue;
				strcat(sbuf, name_list[j]);
				strcat(sbuf, ",");
			}
			sbuf[safe_strlen(sbuf)-1] = '\0';
			info_file_write_line(fh, FOG_AREAS, sbuf);
			FreeList(name_list, nareas);
		}
    }
    info_file_close(fh);
}


/*============================================================================*/
/*
*	ACTIVATE_fcstTextDialog() - Popup the dialog and update displays.  If the
*	dialog is already active we use the X functions to pop the window to the
*	front and set the input focus.
*/
/*============================================================================*/
/*ARGSUSED*/
void ACTIVATE_fcstTextDialog(Widget parent)
{
	int i;
	Boolean generating, amending, updating;
	XmString label;
	Widget w, controlForm, languageManager, sourceManager;
	Widget statusFrame, statusForm, rc;
	Widget langFrame, sourceFrame;

	static XuDialogActionsStruct action_items[] = {
		{ "genBtn",    generate_cb,        (XtPointer)GENERATING },
		{ "amdBtn",    generate_cb,        (XtPointer)AMENDING },
		{ "updateBtn", generate_cb,        (XtPointer)UPDATING },
		{ "editBtn",   edit_cb,            (XtPointer)NULL },
		{ "releaseBtn",release_cb,         (XtPointer)NULL },
		{ "printBtn",  print_cb,           (XtPointer)NULL },
		{ "closeBtn",  XuDestroyDialogCB, (XtPointer)NULL },
		{ "helpBtn",   HelpCB,            HELP_TEXT_FCST  }
	};

	if (dialog)
	{
		XuShowDialog(dialog);
		return;
	}

	source = WORKING;

	/* Ok now we can create the dialog.
	*/
	dialog = XuCreateToplevelFormDialog(GW_mainWindow, "fcstText",
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 13,
		XmNverticalSpacing, 13,
		NULL);

	editBtn = XtNameToWidget(XtParent(dialog), "*.editBtn");

	controlForm = XmVaCreateForm(dialog, "controlForm",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	langFrame = XmVaCreateFrame(controlForm, "langFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(langFrame, "languageLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	languageManager = XmVaCreateRowColumn(langFrame, "languageManager",
		XmNmarginWidth, 6,
		XmNradioBehavior, True,
		NULL);

	for(i = 0; i < GV_nlanguages; i++)
	{
		XmString xmlabel = XmStringCreateSimple(GV_language[i].label);
		w = XmVaCreateManagedToggleButton(languageManager, GV_language[i].key,
			XmNlabelString, xmlabel,
			XmNset, (same(language,GV_language[i].key)) ? XmSET:XmUNSET,
			NULL);
		XtAddCallback(w, XmNvalueChangedCallback, language_cb, GV_language[i].key);
		XmStringFree(xmlabel);
	}
	XtManageChild(languageManager);
	if(GV_nlanguages > 1) XtManageChild(langFrame);

	sourceFrame = XmVaCreateManagedFrame(controlForm, "sourceFrame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNbottomAttachment, (GV_nlanguages > 1)? XmATTACH_WIDGET:XmATTACH_FORM,
		XmNbottomWidget, langFrame,
		XmNbottomOffset, (GV_nlanguages > 1)? 9:0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(sourceFrame, "sourceLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	sourceManager = XmVaCreateManagedRowColumn(sourceFrame, "sourceManager",
		XmNmarginWidth, 6,
		XmNradioBehavior, True,
		NULL);

	w = XmVaCreateManagedToggleButton(sourceManager, "workBtn",
		XmNset, XmSET,
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, source_cb, WORKING);

	w = XmVaCreateManagedToggleButton(sourceManager, "releasedBtn",
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, source_cb, RELEASED);

	w = XmVaCreateManagedToggleButton(sourceManager, "mergedBtn",
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, source_cb, MERGED);

	areasBtn = XmVaCreateManagedPushButton(controlForm, "areaBtn",
		XmNmarginHeight, 5,
		XmNmarginWidth, 6,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, sourceFrame,
		XmNbottomOffset, 19,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(areasBtn, XmNactivateCallback, special_settings_cb, (XtPointer)'A');

/* XXX Not used for now XXXX
	orderBtn = XmVaCreateManagedPushButton(controlForm, "elementOrderBtn",
		XmNsensitive, False,
		XmNmarginHeight, 5,
		XmNmarginWidth, 6,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, areasBtn,
		XmNbottomOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(orderBtn, XmNactivateCallback, special_settings_cb, (XtPointer)'P');

	Set the order button only if at least one forecast has an ordering.
	for ( i = 0; i < nfcsts; i++)
	{
		if(IsNull(fcsts[i].element_order)) continue;
		XtSetSensitive(orderBtn, True);
		break;
	}
XXX */

	w = XmVaCreateManagedLabel(controlForm, "fcstLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	fcstList = XmVaCreateManagedScrolledList(controlForm, "fcstList",
		XmNlistMarginHeight, 3,
		XmNlistMarginWidth, 5,
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, w,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomOffset, 19,
		XmNbottomWidget, areasBtn,
		/* XXX - See note above re the order button.
		XmNbottomWidget, orderBtn,
		*/
		NULL);
	XtAddCallback(fcstList, XmNextendedSelectionCallback, forecast_cb, NULL);

	generating = amending = updating = False;
	for ( i = 0; i < nfcsts; i++)
	{
		label = XmStringCreate(fcsts[i].label,
			(fcsts[i].state == READY || fcsts[i].state == HEADER) ? NORMAL_FONT:ITALIC_FONT);
		XmListAddItem(fcstList, label, 0);
		XmStringFree(label);
		switch(fcsts[i].state)
		{
			case GENERATING: generating = True; break;
			case AMENDING:   amending = True;   break;
			case UPDATING:   updating = True;   break;
		}
	}
	XtManageChild(controlForm);

	{
	Pixel active, bkgnd;

	active = XuLoadColorResource(dialog, RNfcstTextGenerateColor, "Green");
	bkgnd  = XuLoadColor(dialog, XmNbackground);

	XtVaSetValues(XtNameToWidget(XtParent(dialog), "*.genBtn"),
		XmNbackground, generating ? active : bkgnd,
		NULL);

	XtVaSetValues(XtNameToWidget(XtParent(dialog), "*.amdBtn"),
		XmNbackground, amending ? active : bkgnd,
		NULL);

	XtVaSetValues(XtNameToWidget(XtParent(dialog), "*.updateBtn"),
		XmNbackground, updating ? active : bkgnd,
		NULL);
	}

	statusFrame = XmVaCreateManagedFrame(dialog, "sf",
		XmNmarginHeight, 3,
		XmNmarginWidth, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, controlForm,
		NULL);

	statusLabel = XmVaCreateManagedLabel(statusFrame, "status",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	statusForm = XmVaCreateManagedForm(statusFrame, "statusForm",
		XmNorientation, XmHORIZONTAL,
		NULL);

	rc = XmVaCreateManagedRowColumn(statusForm, "rc",
		XmNentryAlignment, XmALIGNMENT_END,
		XmNspacing, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(rc, "workVer", NULL);

	(void) XmVaCreateManagedLabel(rc, "relVer", NULL);

	rc = XmVaCreateManagedRowColumn(statusForm, "rc",
		XmNentryAlignment, XmALIGNMENT_BEGINNING,
		XmNspacing, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, rc,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	workStatus = XmVaCreateManagedLabel(rc, "workStatus", NULL);

	rc = XmVaCreateManagedRowColumn(rc, "rc",
		XmNorientation, XmHORIZONTAL,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);

	releaseStatus = XmVaCreateManagedLabel(rc, "releaseStatus", NULL);

	(void) XmVaCreateManagedLabel(rc, "amend",
		XmNmarginLeft, 30,
		NULL);

	amendStatus = XmVaCreateManagedLabel(rc, "amendStatus", NULL);

	text = XmVaCreateManagedScrolledText(dialog, "text",
		XmNautoShowCursorPosition, False,
		XmNcursorPositionVisible, False,
		XmNeditable, False,
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNcolumns, 70,
		XmNrows, 24,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, controlForm,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, statusFrame,
		NULL);

	if (active_fcst) XmListSelectPos(fcstList, active_fcst->order_no+1, True);

	XuShowDialog(dialog);
}

/*======================== LOCAL FUNCTIONS ===================================*/


/*============================================================================*/
/*
*	show_forecast() - Display the forecast text.
*/
/*============================================================================*/
static void show_forecast(void)
{
	XmTextPosition posn;
	char	line[256];
	String fname;
	FILE * fp, *fopen();

	if(IsNull(dialog)) return;

	if( active_fcst->state != READY )
	{
		XmTextSetString(text, "");
		return;
	}

	fname = FoG_file(active_fcst->key, source, language, 0);

	if ( (fp = fopen(fname, "r")) == NULL )
	{
		XmTextSetString(text, " ");
	}
	else
	{
		XmTextSetString(text, "");
		posn = 0;
		while ( getfileline(fp, line, 256) )
		{
			strcat(line, "\n");
			XmTextReplace(text, posn, posn, line);
			posn += safe_strlen(line);
		}
		(void) fclose(fp);
	}
}


/*============================================================================*/
/*
*	set_status_display() - Makes and displays the status info which appears
*	                     above the text window.
*/
/*============================================================================*/
static void set_status_display(void)
{
	char line[128], mbuf[256], regular_issue[32];
	String  dir, prod, *files;
	struct stat statinfo;
	time_t  concept_time;
	XmString label;
	FILE *fp, *fopen();

	if(IsNull(dialog)) return;

	label = XuNewXmStringFmt("%s  %s", active_fcst->label, XuGetLabel("status"));
	XtVaSetValues(statusLabel, XmNlabelString, label, NULL);
	XmStringFree(label);

	/* First do the status of the working directory forecast.
	*/
	prod = FoG_file(active_fcst->key, WORKING, language, 0);

	if(blank(prod) || stat(prod, &statinfo) != 0) 
	{
		label = XuNewXmString(XuGetLabel("na"));
	}
	else if(active_fcst->state == GENERATING ||
			active_fcst->state == AMENDING ||
			active_fcst->state == UPDATING )
	{
		label = XuNewXmString(XuGetLabel("generating"));
	}
	else if ( active_fcst->state == PROCESSING )
	{
		label = XuNewXmString(XuGetLabel("processing"));
	}
	else
	{
		/* Determine if the forecast is current ( newer than interpolations )
		*/
		concept_time = statinfo.st_mtime;
		dir = source_directory_by_name(INTERP, NULL, NULL);
		if(dirlist(dir, ":", &files) > 0 )
		{
			prod = source_path_by_name(INTERP, NULL, NULL, files[0]);
			if(stat(prod, &statinfo) == 0 && concept_time > statinfo.st_mtime) 
			{
				if(is_released(active_fcst->product_id))
				{
					label = XuNewXmString(XuGetLabel("sameReleased"));
				}
				else
				{
					label = XuNewXmString(XuGetLabelUc("new"));
				}
			}
			else
			{
				label = XuNewXmString(XuGetLabel("outdated"));
			}
		}
		else
		{
			label = XuNewXmString(XuGetLabelUc("noInterp"));
		}
	}
	XtVaSetValues(workStatus, XmNlabelString, label, NULL);
	XmStringFree(label);

	/* Now the released forecast.
	*/
	strcpy(regular_issue, XuGetLabel("none"));
	strcpy(mbuf, "");
	prod = FoG_file(active_fcst->key, RELEASED, STATUS, 0);
	if(!blank(prod) && (fp = fopen(prod, "r")) != NULL)
	{
		while(getfileline(fp, line, 127))
		{
			if(!same_ic(string_arg(line), "type")) continue;
			switch(tolower(*string_arg(line)))
			{
				case 'n':
				case 'u':
					strcpy(regular_issue, strrem_arg(line));
					break;
				case 'a':
					if(!blank(mbuf)) strcat(mbuf, " , ");
					strcat(mbuf, strrem_arg(line));
					break;
			}
		}
		(void) fclose(fp);
	}
	XtUnmanageChild(XtParent(amendStatus));
	XtUnmanageChild(XtParent(releaseStatus));

	label = XuNewXmString(regular_issue);
	XtVaSetValues(releaseStatus, XmNlabelString, label, NULL);
	XmStringFree(label);

	label = XuNewXmString(blank(mbuf)? XuGetLabel("none"):mbuf);
	XtVaSetValues(amendStatus, XmNlabelString, label, NULL);
	XmStringFree(label);

	XtManageChild(XtParent(amendStatus));
	XtManageChild(XtParent(releaseStatus));
}


/*============================================================================*/
/*
*	is_released() - Determines if the forecast identified by the given product
*                  key has been released or not. It is considered released if
*   the released status file is the same or newer than the last generate time
*   of the product.
*/
/*============================================================================*/
static Boolean is_released(int pid )
{
	int i;
	long dt;
	String prod;
	struct stat rinfo;

	for(i = 0; i < nfcsts; i++)
	{
		if(fcsts[i].product_id != pid) continue;
		dt = ProductStatusGetGenerateTime(pid);
		if(dt == 0) return False;
		prod  = FoG_file(fcsts[i].key, RELEASED, STATUS, 0);
		if(stat(prod, &rinfo) == -1) return False;
		if(dt > rinfo.st_mtime) return False;
		return True;
	}
	return False;
}

/*============================================================================*/
/*
*	forcast_status_update() - Called from when notification has been received
*	of the end of the FoG forecast generation run. 
*/
/*============================================================================*/
static void forcast_status_update(XtPointer client_data , int status_key , String status )
{
	char mbuf[128];
	String statfile;
	struct stat statinfo;
	FOG_DATA_PTR fcst = (FOG_DATA_PTR)client_data;

	switch(status_key)
	{
		case XuRUN_STATUS:
			if(same_ic("start", string_arg(status)))
			{
				strcpy(mbuf, XuGetLabel(string_arg(status)));
				strcat(mbuf, " ");
				strcat(mbuf, XuGetLabel("running"));
				ProductStatusUpdateInfo(fcst->product_id, PS_UPDATE, mbuf);
			}
			else
			{
				ProductStatusUpdateInfo(fcst->product_id, PS_UPDATE, status);
			}
			break;

		case XuRUN_ENDED:
			statfile = FoG_file(active_fcst->key, WORKING, STATUS, 0);
			if(statfile && stat(statfile, &statinfo) == 0)
			{
				(void) snprintf(mbuf, sizeof(mbuf), "%ld", statinfo.st_mtime);
				ProductStatusUpdateInfo(fcst->product_id, PS_ENDED, mbuf);
			}
			else
			{
				ProductStatusUpdateInfo(fcst->product_id, PS_ENDED, NULL);
			}
			set_status_indicators(fcst, READY);
			if(NotNull(dialog) && fcst == active_fcst)
				XmListSelectPos(fcstList, fcst->order_no+1, True);
			run_fog();
			break;

		case XuRUN_ERROR:
			strcpy(mbuf, XuGetLabel(status));
			strcat(mbuf, " ");
			strcat(mbuf, XuGetLabel("abort"));
			ProductStatusUpdateInfo(fcst->product_id, PS_ERROR, mbuf);
			set_status_indicators(fcst, READY);
			if(NotNull(dialog))
			{
				XuShowError(dialog, "fogAbort", NULL);
				if(fcst == active_fcst)
					XmListSelectPos(fcstList, fcst->order_no+1, True);
			}
			else
			{
				XuShowError(GW_mainWindow, "fogAbort", NULL);
			}
			run_fog();
			break;
	}
}


/*============================================================================*/
/*
*	set_status_indicators() - Displays visual information giving the activity
*	status of "Generate" and "Update" buttons.  If either are active the 
*	background color is set to the active color and the font of the fcst
*	in the forecast selection list is set to indicate an active forecast.
*/
/*============================================================================*/
static void set_status_indicators(FOG_DATA_PTR fcst , TEXT_STATE state )
{
	int      i;
	XmString label[1];
	Boolean  generating, amending, updating;
	Pixel active, bkgnd;

	fcst->state = state;

	if(IsNull(dialog)) return;

	active = XuLoadColorResource(dialog, RNfcstTextGenerateColor, "Green");
	bkgnd  = XuLoadColor(dialog, XmNbackground);

	label[0] = XmStringCreate(fcst->label, (state == READY) ? NORMAL_FONT:ITALIC_FONT);
	XmListReplaceItemsPos(fcstList, label, 1, fcst->order_no+1);
	XmStringFree(label[0]);

	generating = amending = updating = False;
	for( i = 0; i < nfcsts; i++ )
	{
		switch(fcsts[i].state)
		{
			case GENERATING: generating = True; break;
			case AMENDING:   amending = True;   break;
			case UPDATING:   updating = True;   break;
		}
	}

	XtVaSetValues(XtNameToWidget(XtParent(dialog), "*.genBtn"),
		XmNbackground, generating ? active : bkgnd,
		NULL);

	XtVaSetValues(XtNameToWidget(XtParent(dialog), "*.amdBtn"),
		XmNbackground, amending ? active : bkgnd,
		NULL);

	XtVaSetValues(XtNameToWidget(XtParent(dialog), "*.updateBtn"),
		XmNbackground, updating ? active : bkgnd,
		NULL);
}


/*============================================================================*/
/*
*	forecast_cb() - Callback function for forecast selection buttons. We look
*	at the entire selected list as we want to respond only to the first
*	selected item in the list.
*/
/*============================================================================*/
/*ARGSUSED*/
static void forecast_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int     n, *list, count;
	String  editor;
	Boolean ok, ready;

	if( !XmListGetSelectedPos(fcstList, &list, &count) ) return;

	/* Are any of the selected forecasts in a ready state?
	*/
	for(n = 0; n < count; n++)
	{
		ready = (fcsts[list[n]-1].state == READY);
		if (ready) break;
	}

	if(ready)
	{
		/* At least one is selected and ready so set this to the active and
		*  deselect any which are not ready.
		*/
		ok = True;
		for(n = 0; n < count; n++)
		{
			if(fcsts[list[n]-1].state == READY)
			{
				if (ok)
				{
					ok = False;
					active_fcst = fcsts + list[n] - 1;
					editor = FoG_setup_parm(active_fcst->key, FOG_EDITOR);
					XtSetSensitive(editBtn, (!blank(editor) && !same_ic(editor,"none")));
					XtSetSensitive(areasBtn, NotNull(active_fcst->area_states));
					set_status_display();
					show_forecast();
				}
			}
			else
			{
				XmListDeselectPos(fcstList, list[n]);
			}
		}
	}
	else
	{
		/* None are so select the first one which is in the ready state.
		*/
		for(n = list[0]; n <= nfcsts; n++)
		{
			if(fcsts[n-1].state == READY)
			{
				XmListSelectPos(fcstList, n, True);
				break;
			}
			XmListDeselectPos(fcstList, n);
		}
	}
	FreeItem(list);
}


/*============================================================================*/
/*
*   run_fog() - Runs FoG for the next forecast with the state flag as either
*              GENERATING or UPDATING.
*/
/*============================================================================*/
static void run_fog(void)
{
	int  i, ac;
	Arg  al[6];

	for(i = 0; i < nfcsts; i++)
	{
		if(fcsts[i].state == READY     ) continue;
		if(fcsts[i].state == PROCESSING) continue;
		if(fcsts[i].state == SPACER    ) continue;
		if(fcsts[i].state == HEADER    ) continue;

		ac = 0;
		XtSetArg(al[ac], PmNprogram, PmFOG); ac++;
		XtSetArg(al[ac], PmNselect, "regular_fcst"); ac++;
		XtSetArg(al[ac], PmNfcstId, fcsts[i].key); ac++;
		XtSetArg(al[ac], PmNinfoFile, get_path(FCST_WORK,INFO_FILE)); ac++;
		if(fcsts[i].state == AMENDING)
		{
			XtSetArg(al[ac], PmNkey, PmAMEND); ac++;
		}
		else if(fcsts[i].state == UPDATING)
		{
			XtSetArg(al[ac], PmNkey, PmUPDATE); ac++;
		}
		if(!RunProgramManager(forcast_status_update, &fcsts[i], al, ac))
		{
			fcsts[i].state = READY;
		}
		break;
	}
}


/*============================================================================*/
/*
*   generate_cb() - Runs FoG to produce the requested forecasts.  Note that
*   more than one forecast can be run at the same time by the use of multiple
*   selection from the forecast list.  The client_data parameter will be True
*   if an update is to be performed, False if a standard generation is to be
*   done.  The run is time offset by calling FogRun to avoid collisions.
*/
/*============================================================================*/
/*ARGSUSED*/
static void generate_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int     i, idx, pos_count, *pos_list;
	String  ptr, str_list;

	/* Check for pending edit and if there is one ask permission to run. */
	if(XtIsSensitive(GW_editorAcceptBtn))
	{
		if(XuAskUser(w, "pending_edit", NULL) == XuNO) return;
	}

	if( XmListGetSelectedPos(fcstList, &pos_list, &pos_count) )
	{
		for( i = 0; i < pos_count; i++ )
		{
			idx = pos_list[i] - 1;
			if( fcsts[idx].state == SPACER ) continue;
			if( fcsts[idx].state == HEADER ) continue;
			str_list = FoG_setup_parm(fcsts[idx].key, FOG_REQ_FIELDS);
			if(fcsts[idx].state != UPDATING)
			{
				int    nl = 0;
				String *l = NULL;
				String s  = XtNewString(str_list);
				while((ptr = string_arg(s)))
				{
					l = MoreStringArray(l, nl+1);
					l[nl++] = XtNewString(ptr);
				}
				XtFree(s);
				if(!InterpFieldsAvailable("Text", fcsts[idx].label, w, nl, l)) continue;
				FreeList(l, nl);
			}
			if( fcsts[idx].state == READY )
			{
				set_status_indicators(&fcsts[idx], PTR2INT(client_data));
				ProductStatusUpdateInfo(fcsts[idx].product_id, PS_RUNNING, NULL);
			}
		}
		FreeItem(pos_list);
		set_status_display();
		run_fog();
	}
}


/* Unless ther is an error running the editor we have no way of knowing if
*  the user has edited the forecast. We assume that they have and mark the
*  generate time with the end of edit session time.
*/
static void TextEditorFinished(XtPointer data , int status_key , String status )
{
	FOG_DATA_PTR fcst = (FOG_DATA_PTR)data;

	if(status_key == XuRUN_ERROR)
	{
		if(dialog) XuShowError(dialog, "NoEditor", status);
	}
	else
	{
		char mbuf[128];
		(void) snprintf(mbuf, sizeof(mbuf), "%ld", (long int) time((time_t*)0));
		ProductStatusUpdateInfo(fcst->product_id, PS_ENDED, mbuf);
	}
	set_status_indicators(fcst, READY);
	set_status_display();
	show_forecast();
	XtSetSensitive(editBtn, True);
}


/*============================================================================*/
/*
*   edit_cb() - Runs the forecast text editor. Note that the special case
*              "text_edit" is also recognized as "text_editor" because the
*   former was in the System Admin manual and the latter in the actual code.
*   Both are thus recoginzed for compatability.
*/
/*============================================================================*/
/*ARGSUSED*/
static void edit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	Arg    al[3];
	String editor, fname;

	editor = FoG_setup_parm(active_fcst->key, FOG_EDITOR);
	fname  = FoG_file(active_fcst->key, source, language, 0);

	/* Check for the special key word which uses the environment variable.
	*/
	if(blank(editor) || same_ic(editor,"text_edit") || same_ic(editor,"text_editor"))
		editor = getenv("FPA_TEXT_EDITOR");

	XtSetArg(al[0], PmNprogram,  PmEDITOR);
	XtSetArg(al[1], PmNselect,   editor  );
	XtSetArg(al[2], PmNfileName, fname   );
	if(RunProgramManager(TextEditorFinished, active_fcst, al, 3))
	{
		XtSetSensitive(editBtn, False);
		set_status_indicators(active_fcst, PROCESSING);
	}
}


/*ARGSUSED*/
static void ReleaseStatusUpdate(XtPointer client_data , int status_key , String status )
{
	FOG_DATA_PTR fcst = (FOG_DATA_PTR)client_data;
	UpdateProductDialogFromId(fcst->product_id);
}


/*============================================================================*/
/*
*	release_cb() - Release and archive the selected forecast(s).
*/
/*============================================================================*/
/*ARGSUSED*/
static void release_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, pc, *pl;
	Arg al[3];

	if(!XmListGetSelectedPos(fcstList, &pl, &pc)) return;

	for( i = 0; i < pc; i++ )
	{
		if(FoG_copy_files(fcsts[pl[i]-1].key, "to_release"))
		{
			XtSetArg(al[0], PmNprogram, PmARCHIVE);
			XtSetArg(al[1], PmNfcstId, fcsts[pl[i]-1].key);
			XtSetArg(al[2], PmNtime, GV_depict[0]);
			(void)RunProgramManager(ReleaseStatusUpdate, (XtPointer)&fcsts[pl[i]-1], al, 3);
		}
		else
		{
			XuShowError(dialog, "NoFcstRelease", NULL);
		}
	}
	FreeItem(pl);
	set_status_display();
}


/*============================================================================*/
/*
*	print_cb() - Print the selected forecast(s).
*/
/*============================================================================*/
/*ARGSUSED*/
static void print_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int  i, pc, *pl;
	Arg al[3];
	String fname;
	struct stat statinfo;

	if(!XmListGetSelectedPos(fcstList, &pl, &pc)) return;

	for( i = 0; i < pc; i++ )
	{
		fname = FoG_file(fcsts[pl[i]-1].key, source, language, 0);
		if(stat(fname, &statinfo) != 0) continue;
		XtSetArg(al[0], PmNprogram, PmFILE_PRINT);
		XtSetArg(al[1], PmNfileName, fname);
		(void)RunProgramManager(NULL, NULL, al, 2);
	}
	FreeItem(pl);
}


/*============================================================================*/
/*
*	special_settings_cb() - Called to launch the area setting and priority
*                         setting dialogs.
*/
/*============================================================================*/
/*ARGSUSED*/
static void special_settings_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2CHAR(client_data))
	{
		case 'A': ACTIVATE_fcstTextSetAreasDialog(w, active_fcst); break;
		case 'P': ACTIVATE_fcstTextPriorityDialog(w, nfcsts, fcsts, active_fcst); break;
	}

}


/*============================================================================*/
/*
*	language_cb() - Function for language selection.
*/
/*============================================================================*/
/*ARGSUSED*/
static void language_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if( !XmToggleButtonGetState(w) ) return;

	language = (String)client_data;
	show_forecast();
	set_status_display();
}


/*============================================================================*/
/*
*	source_cb() - Function for source selection.
*/
/*============================================================================*/
/*ARGSUSED*/
static void source_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if( !XmToggleButtonGetState(w) )return;

	source = (String)client_data;
	show_forecast();
	set_status_display();
}
