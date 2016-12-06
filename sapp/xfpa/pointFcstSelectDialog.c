/*========================================================================*/
/*
*	File:		pointFcstSelectDialog.c
*
*	Purpose:	Provides a mechanism for the selection and viewing of
*               the point forecasts to be generated.
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
#include <unistd.h>
#include <sys/stat.h>
#include "global.h"
#include <Xm/ArrowB.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include "resourceDefines.h"
#include "help.h"
#include "productStatus.h"
#include "fpapm.h"
#include "pointFcst.h"


static Widget dialog = NULL;
static Widget classSelect;
static Widget generateBtn;
static Widget fcstSelectList;
static Widget fcstNameDisplay;
static Widget fcstText;

/* local functions
*/
static String read_forecast_file (PFDATA *);
static void   display_forecast_cb (Widget, XtPointer, XtPointer);
static void   modify_list_cb (Widget, XtPointer, XtPointer);
static void   generate_cb (Widget, XtPointer, XtPointer);
static void   release_cb (Widget, XtPointer, XtPointer);
static void   edit_text_cb (Widget, XtPointer, XtPointer);
static void   select_and_close_cb (Widget, XtPointer, XtPointer);
static void   class_select_cb (Widget, XtPointer, XtPointer);
static void   print_cb (Widget, XtPointer, XtPointer);

extern int pf_nclass;
extern PFCLASS *pf_class;
extern PFCLASS *pf_active_class;
extern String  pf_info_file;

/* Local static variables
*/
static PFDATA *active_fcst = NULL;


/*========================================================================*/
/*
*	ACTIVATE_pointFcstSelectDialog() - Create all of the dialogs in this
*	activity group and show the first forecast.
*/
/*========================================================================*/
/*ARGSUSED*/
void ACTIVATE_pointFcstSelectDialog(Widget parent)

{
	int i, ac;
	XmString item;
	Arg al[16];
	Widget nameDisplayForm, btn, btn1;
	Widget selectForm, rightArrow, leftArrow;

	static XuDialogActionsStruct action_items[] = {
		{ "generateBtn",   generate_cb,       NULL },
		{ "editBtn",       edit_text_cb,      NULL },
		{ "printBtn",      print_cb,          NULL },
		{ "releaseBtn",    release_cb,        NULL },
		{ "closeBtn",      XuDestroyDialogCB, NULL },
		{ "helpBtn",       HelpCB,            HELP_POINT_FCSTS }
	};

	if (dialog)
	{
		XuShowDialog(dialog);
		return;
	}

	dialog = XuCreateToplevelFormDialog( GW_mainWindow, "pointFcst",
		XuNdestroyCallback, select_and_close_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	generateBtn = XtNameToWidget(XtParent(dialog), "*.generateBtn");

	ac = 0;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	selectForm = XmCreateForm(dialog, "selectForm", al, ac);

	ac = 0;
	XtSetArg(al[ac], XmNradioBehavior, True); ac++;
	XtSetArg(al[ac], XmNborderWidth, 1); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	classSelect = XmCreateRowColumn(selectForm, "classSelect", al, ac);

	btn1 = NULL;
	for( i = 0; i < pf_nclass; i++ )
	{
		item = XmStringCreateLocalized(pf_class[i].label);
		btn = XtVaCreateManagedWidget(pf_class[i].id,
			xmToggleButtonWidgetClass, classSelect,
			XmNlabelString, item,
			NULL);
		XmStringFree(item);
    	XtAddCallback(btn, XmNvalueChangedCallback,
			class_select_cb, (XtPointer)(pf_class+i));
		if(!i) btn1 = btn;
	}

	btn = XtVaCreateManagedWidget("modifyListBtn",
		xmPushButtonWidgetClass, selectForm,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, modify_list_cb, NULL);

	ac = 0;
	XtSetArg(al[ac], XmNscrollBarDisplayPolicy, XmSTATIC); ac++;
	XtSetArg(al[ac], XmNselectionPolicy, XmEXTENDED_SELECT); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNtopOffset, 9); ac++;
	XtSetArg(al[ac], XmNtopWidget, classSelect); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNbottomOffset, 9); ac++;
	XtSetArg(al[ac], XmNbottomWidget, btn); ac++;
	XtSetArg(al[ac], XmNlistMarginHeight, 6); ac++;
	XtSetArg(al[ac], XmNlistMarginWidth, 6); ac++;
	fcstSelectList = XmCreateScrolledList(selectForm, "fcstSelectList", al, ac);
    XtAddCallback(fcstSelectList, XmNextendedSelectionCallback, display_forecast_cb, (XtPointer)0 );

	XtManageChild(classSelect);
	XtManageChild(fcstSelectList);
	XtManageChild(selectForm);

	ac = 0;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, selectForm); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	nameDisplayForm = XmCreateForm(dialog, "nameDisplayForm", al, ac);

	ac = 0;
	XtSetArg(al[ac], XmNwidth, 24); ac++;
	XtSetArg(al[ac], XmNarrowDirection, XmARROW_RIGHT); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	rightArrow = XmCreateArrowButton(nameDisplayForm, "selectArrowRight", al, ac);
    XtAddCallback(rightArrow, XmNactivateCallback, display_forecast_cb, (XtPointer)1);

	ac = 0;
	XtSetArg(al[ac], XmNwidth, 24); ac++;
	XtSetArg(al[ac], XmNarrowDirection, XmARROW_LEFT); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, rightArrow); ac++;
	leftArrow = XmCreateArrowButton(nameDisplayForm, "selectArrowRight", al, ac);
    XtAddCallback(leftArrow, XmNactivateCallback, display_forecast_cb, (XtPointer)-1);

	ac = 0;
	XtSetArg(al[ac], XmNmarginWidth, 5); ac++;
	XtSetArg(al[ac], XmNcursorPositionVisible, False); ac++;
	XtSetArg(al[ac], XmNeditable, False); ac++;
	XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(al[ac], XmNrightAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(al[ac], XmNrightWidget, leftArrow); ac++;
	XtSetArg(al[ac], XmNrightOffset, 9); ac++;
	fcstNameDisplay = XmCreateTextField(nameDisplayForm, "fcstNameDisplay", al, ac);

	XtManageChild(rightArrow);
	XtManageChild(leftArrow);
	XtManageChild(fcstNameDisplay);
	XtManageChild(nameDisplayForm);

	fcstText = XmVaCreateManagedScrolledText(dialog, "fcstText",
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNwordWrap, True,
		XmNscrollHorizontal, False,
		XmNeditable, False,
		XmNcursorPositionVisible, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, nameDisplayForm,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, selectForm,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	if (btn1) XuToggleButtonSet(btn1, True, True);

	XuShowDialog(dialog);
}


void pf_MakeClassSelection(void)
{
	Widget w;

	w = XtNameToWidget(classSelect, pf_active_class->id);
	XuToggleButtonSet(w, True, True);
}

/*========================================================================*/
/*
*	read_forecast_file() - Read the file containing the forecast.  This function
*	returns the forecast in a Malloc'ed string.  It is the responsibility
*	of the calling procedure to free the memory.
*/
/*========================================================================*/
static String read_forecast_file(PFDATA *data )

{
	char   line[256];
	String ptr;
	FILE   *fp, *fopen();
	static String text;

	pf_CreateFileName(data, line);
	ptr = get_file(FCST_WORK,line);
	if( IsNull(ptr) || (fp = fopen(ptr,"r")) == NULL ) return NULL;

	text = NULL;
	while(NotNull(fgets(line, 256, fp)))
	{
		if (text)
			text = MoreMem(text, char, safe_strlen(text)+safe_strlen(line)+2);
		else
			text = NewMem(char, safe_strlen(line)+2);
		strcat(text, line);
		strcat(text, "\n");
	}
	if (text) text[safe_strlen(text)-1] = '\0';
	fclose(fp);
	return text;
}


/*========================================================================*/
/*
*   forecast_status_update() - Called when forecast generation has been
*   completed for a sepecific point. First the product status is updated,
*   then if the forecast has completed generating, the forecast label is
*   turned back to the default font.  If no forecasts remain in a
*   the list of forecasts is scanned for a request to generate a forecast
*   and if found the forecast is generated. If still there are no forecasts
*   left in a generating state, the generate button is turned back to its
*   normal color.
*/
/*========================================================================*/
static void forecast_status_update(XtPointer client_data ,
                              int status_key ,
                              String status )

{
	int i, j, ngenerating, pid, ac;
	char mbuf[128];
	XmString label[1];
	Pixel color;
	Arg	  al[5];

	pid = PTR2INT(client_data);
	ngenerating = 0;
	for( i = 0; i < pf_nclass; i++ )
	{
		for( j = 0; j < pf_class[i].ndata; j++ )
		{
			if(pid == pf_class[i].data[j]->pid)
			{
				switch(status_key)
				{
					case XuRUN_STATUS:
						if(same_ic("start", string_arg(status)))
						{
							strcpy(mbuf, XuGetLabel(string_arg(status)));
							strcat(mbuf, " ");
							strcat(mbuf, XuGetLabel("running"));
							ProductStatusUpdateInfo(pf_class[i].data[j]->pid, PS_UPDATE, mbuf);
						}
						else
						{
							ProductStatusUpdateInfo(pf_class[i].data[j]->pid, PS_UPDATE, status);
						}
						break;

					case XuRUN_ENDED:
						pf_class[i].data[j]->generating = False;
						ProductStatusUpdateInfo(pf_class[i].data[j]->pid, PS_ENDED, NULL);
						if(NotNull(dialog) && pf_active_class == &pf_class[i] )
						{
							label[0] = XuNewXmString(pf_class[i].data[j]->label);
							XmListReplaceItemsPos(fcstSelectList,label,1,j+1);
							XmStringFree(label[0]);
							display_forecast_cb(NULL, 0, NULL);
						}
						break;

					case XuRUN_ERROR:
						pf_class[i].data[j]->generating = False;
						strcpy(mbuf, XuGetLabel(status));
						strcat(mbuf, " ");
						strcat(mbuf, XuGetLabel("abort"));
						ProductStatusUpdateInfo(pf_class[i].data[j]->pid, PS_ERROR, mbuf);
						if(NotNull(dialog) && pf_active_class == pf_class+i )
						{
							label[0] = XuNewXmString(pf_class[i].data[j]->label);
							XmListReplaceItemsPos(fcstSelectList,label,1,j+1);
							XmStringFree(label[0]);
							display_forecast_cb(NULL, 0, NULL);
						}
						XuShowError(GW_mainWindow, "fogAbort", NULL);
						break;
				}
			}
			if(pf_class[i].data[j]->generating) ngenerating++;
		}
	}

	/* If no forecasta are generating look for another request to generate a
	*  forecast.
	*/
	for(i = 0; i < pf_nclass && ngenerating == 0; i++)
	{
		for( j = 0; j < pf_class[i].ndata; j++ )
		{
			if(!pf_class[i].data[j]->generate) continue;
			pf_class[i].data[j]->generate = False;
			ac = 0;
			XtSetArg(al[ac], PmNprogram, PmFOG); ac++;
			XtSetArg(al[ac], PmNselect, "point_fcst"); ac++;
			XtSetArg(al[ac], PmNinfoFile, pf_info_file); ac++;
			XtSetArg(al[ac], PmNfcstId, pf_class[i].data[j]->id); ac++;
			if( RunProgramManager(forecast_status_update, INT2PTR(pf_class[i].data[j]->pid), al, ac) )
			{
				pf_class[i].data[j]->generating = True;
				ngenerating++;
				XtVaSetValues( generateBtn,
					XmNbackground, XuLoadColorResource(generateBtn, RNpointFcstGenerateColor, "Green"),
					NULL );
				ProductStatusUpdateInfo(pf_class[i].data[j]->pid, PS_RUNNING, NULL);
				break;
			}
			else if(pf_class+i == pf_active_class)
			{
				label[0] = XuNewXmString(pf_class[i].data[j]->label);
				XmListReplaceItemsPos(fcstSelectList, label, 1, j+1);
				XmStringFree(label[0]);
			}
		}
	}

	/* Reset the generate button if no forecasts are actually generating.
	*/
	if(NotNull(dialog) && ngenerating <= 0 )
	{
		XtVaGetValues(dialog, XmNbackground, &color, NULL);
		XtVaSetValues( generateBtn, XmNbackground, color, NULL );
	}
}

/*========================================================================*/
/*
*   class_select_cb() - Selects the forecast class now being handled.
*/
/*========================================================================*/
/*ARGSUSED*/
static void class_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i;
	XmString label;

	if( !XmToggleButtonGetState(w) ) return;

	pf_active_class = (PFCLASS *)client_data;

	XmTextFieldSetString(fcstNameDisplay, " ");
	XmTextSetString(fcstText, " ");
	XmListSetPos(fcstSelectList, 1);
	XmListDeselectAllItems(fcstSelectList);
	XmListDeleteAllItems(fcstSelectList);
	for( i = 0; i < pf_active_class->ndata; i++ )
	{
		if(pf_active_class->data[i]->generating)
			label = XmStringCreate(pf_active_class->data[i]->label, ITALIC_FONT);
		else
			label = XuNewXmString(pf_active_class->data[i]->label);
		XmListAddItem(fcstSelectList,label,0);
		XmStringFree(label);
	}
	if(pf_active_class->ndata > 0 &&
		!(  pf_active_class->data[pf_active_class->selected-1]->generate ||
			pf_active_class->data[pf_active_class->selected-1]->generating))
		XmListSelectPos(fcstSelectList, pf_active_class->selected, True);
}

/*========================================================================*/
/*
*   generate_cb() - Function called by the generate button.  The message to
*   send to FoG is composed and then run as a background task.  The notify
*   program is appended so that the Pointforecast_status_update function will be
*   called when the forecast generation is complete.  The entry in the
*   forecast list is changed to light italic as an indication that the
*   entry is being generated.
*/
/*========================================================================*/
/*ARGSUSED*/
static void generate_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, sc, *sl, ndx;
	XmString label[1];

	if( !XmListGetSelectedPos(fcstSelectList,&sl,&sc) ) return;

	/* Check for a pending edit and ask permission to run if there is one. */
	if(XtIsSensitive(GW_editorAcceptBtn))
	{
		if(XuAskUser(w, "pending_edit", NULL) == XuNO) return;
	}

	for( i = 0; i < sc; i++ )
	{
		ndx = sl[i]-1;
		if( pf_active_class->data[ndx]->generating ) continue;
		pf_active_class->data[ndx]->generate = True;
		label[0] = XmStringCreate(pf_active_class->data[ndx]->label, ITALIC_FONT);
		XmListReplaceItemsPos(fcstSelectList, label, 1, sl[i]);
		XmStringFree(label[0]);
	}
	forecast_status_update((XtPointer)0, 0, NULL);
	FreeItem(sl);
}


/*========================================================================*/
/*
*	edit_text_cb() - Call the default text editor.
*/
/*========================================================================*/
/*ARGSUSED*/
static void edit_text_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	char mbuf[256];
	Arg  al[2];

	if (!active_fcst || active_fcst->generating) return;

	pf_CreateFileName(active_fcst, mbuf);
	XtSetArg(al[0], PmNprogram, PmEDITOR);
	XtSetArg(al[1], PmNfileName, get_file(FCST_WORK, mbuf));
	(void)RunProgramManager(NULL, NULL, al, 2);
}


/*========================================================================*/
/*
*	print_cb() - Print the selected forecasts.
*/
/*========================================================================*/
/*ARGSUSED*/
static void print_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int    i, sc, *sl;
	char   mbuf[256];
	Arg    al[3];
	String fname;
	struct stat statinfo;

	if(!XmListGetSelectedPos(fcstSelectList,&sl,&sc)) return;

	for(i = 0; i < sc; i++)
	{
		pf_CreateFileName(pf_active_class->data[sl[i]-1], mbuf);
		fname = get_file(FCST_WORK, mbuf);
		if(NotNull(fname) && stat(fname, &statinfo) == 0)
		{
			XtSetArg(al[0], PmNprogram, PmFILE_PRINT);
			XtSetArg(al[1], PmNfileName, fname);
			(void)RunProgramManager(NULL, NULL, al, 2);
		}
	}
	FreeItem(sl);
}


/*========================================================================*/
/*
*	release_cb() - The forecast is released into the forecast release
*	directory with a name made from the label assigned by the user. The
*	name is constructed by converting all spaces to underscors and 
*	appending .pf to the end.  The release script is then called.
*/
/*========================================================================*/
/*ARGSUSED*/
static void release_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, ac, nsel, *sel;
	char mbuf[256];
	String fcst, textfile;
	Arg al[5];

	if( !XmListGetSelectedPos(fcstSelectList,&sel,&nsel) ) return;
	for( i = 0; i < nsel; i++ )
	{
		fcst = read_forecast_file(pf_active_class->data[sel[i]-1]);
		if( !blank(fcst) )
		{
			pf_CreateFileName(pf_active_class->data[sel[i]-1], mbuf);
			textfile = get_file(FCST_WORK, mbuf);
			ac = 0;
			XtSetArg(al[ac], PmNprogram, PmFOG); ac++;
			XtSetArg(al[ac], PmNselect, "release_point_fcst"); ac++;
			XtSetArg(al[ac], PmNdirectory, get_directory(FCST_RELEASE)); ac++;
			XtSetArg(al[ac], PmNfileName, textfile); ac++;
			(void)RunProgramManager(NULL, NULL, al, ac);
		}
		FreeItem(fcst);
	}
	FreeItem(sel);
}

/*========================================================================*/
/*
*	modify_list_cb() - Edit the point forecast list.
*/
/*========================================================================*/
/*ARGSUSED*/
static void modify_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	ACTIVATE_pointFcstEditDialog(dialog);
}

/*========================================================================*/
/*
*	select_and_close_cb() - Exit out of this dialog.
*/
/*========================================================================*/
/*ARGSUSED*/
static void select_and_close_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	dialog = NULL;
}

/*========================================================================*/
/*
*	display_forecast_cb() - Gets and displays the forecast for a forecast
*	from the selected list.  This function responds to either a forecast
*	list selection or an arrow selection.  The client_data parameter will
*	be True for a list selection.
*/
/*========================================================================*/
/*ARGSUSED*/
static void display_forecast_cb(Widget w, XtPointer client_data, XtPointer unused )
{
	int      i, sc, *sl;
	String   fcst;
	int ndx = PTR2INT(client_data);

	static int viewing = 0;

	if( !XmListGetSelectedPos(fcstSelectList,&sl,&sc) )
	{
		XmTextFieldSetString(fcstNameDisplay, " ");
		XmTextSetString( fcstText, " ");
		active_fcst = NULL;
		return;
	}

	if(ndx == 0)
	{
		viewing = 0;
		for( i = 0; i < sc; i++ )
			if( sl[i] == pf_active_class->selected ) viewing = i;
	}
	else
	{
		viewing += ndx;
		if( viewing >= sc ) viewing = 0;
		if( viewing < 0   ) viewing = sc - 1;
	}
	pf_active_class->selected = sl[viewing];

	/* Now display the requested new forecast.  First the label then text.
	*/
	active_fcst = pf_active_class->data[sl[viewing] - 1];
	XmTextFieldSetString(fcstNameDisplay, active_fcst->label);

	fcst = read_forecast_file(active_fcst);
	XmTextSetString( fcstText, blank(fcst) ? XuGetLabel("fcstNa") : fcst );
	FreeItem(fcst);

	FreeItem(sl);
}
