/*=========================================================================*/
/*
*      File: guidance_selectDialog.c
*
*   Purpose: Provides the mechanism for the selection of guidance products.
*            This code also handles insertion and deletion of lists and
*            fields and the updating of valid and run times.
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
/*=========================================================================*/

#include <string.h>
#include "global.h"
#include <Xm/ArrowB.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/ComboBox.h>
#include <Xm/TabStack.h>
#include <Xbae/Matrix.h>
#include "depiction.h"
#include "resourceDefines.h"
#include "help.h"
#include "iconBar.h"
#include "menu.h"
#include "observer.h"

#define GUIDANCE_MAIN
#include "guidance.h"

#define TEMP_LIST_LABEL "<temp>"
#define STATE_ID 		"gld"
#define DISPLAY_OPTION	"dopt"
#define NO_RUN          "1980:001:01"

/* The default frequency in seconds that fields will be checked for
 * availability after a source update and the maximum number of times
 * that the checking will be done.
 */
#define RECHECK_INTERVAL	10
#define MAX_RECHECKS		100

/* Id's for the pulldown menus
 */
#define LIST_ADD_ID		11
#define LIST_SAVE_AS_ID	12
#define LIST_DEL_ID		13
#define FIELD_ADD_ID	14
#define FIELD_DEL_ID	15

/* For the selection matrix
 */
#define LEVEL_COLUMN	0
#define FIELD_COLUMN	1
#define SOURCE_COLUMN	2
#define ISSUE_COLUMN	3
#define VALID_COLUMN	4
#define ARROW_COLUMN	5


/* Local variables and functions
*/
static void    add_list_item_to_list      (GuidlistStruct*);
static void    clear_lists_cb             (Widget, XtPointer, XtPointer);
static void    change_list                (int);
static void    create_field_valid_list    (GuidanceFieldStruct *, String *, int);
static void    delete_list_cb             (Widget, XtPointer, XtPointer);
static void    dialog_launch_cb           (Widget, XtPointer, XtPointer);
static void    exit_cb                    (Widget, XtPointer, XtPointer);
static void    field_available_observer   (String*, int);
static void    ingred_messages            (CAL, String*, int);
static Boolean is_run_available           (Source, String);
static void    track_cell_cb              (Widget, XtPointer, XtPointer);
static void    select_cell_cb             (Widget, XtPointer, XtPointer);
static void    fill_valid_time            (GuidanceFieldStruct *);
static void    label_sequence_btns        (void);
static void    list_cb                    (Widget, XtPointer, XtPointer);
static void    make_sequence_btns         (void);
static void    options_cb                 (Widget, XtPointer, XtPointer);
static void    ready_guidance_update      (String*, int);
static void    refresh_cb                 (Widget, XtPointer, XtPointer);
static void    run_time_popup_cb          (Widget, XtPointer, XtPointer);
static Boolean check_for_run_time_change  (Boolean);
static void    set_appearance_btn_state   (void);
static void    set_list_valid_times       (GuidlistStruct *, String);
static void    set_field_for_show         (GuidanceFieldStruct *, Boolean);
static void    set_field_show_state       (GuidanceFieldStruct *, Boolean);
static void    show_valid_popup_cb        (Widget, XtPointer, XtPointer);
static void    save_active_list           (String*, int);
static void    save_lists                 (void);
static void    tabs_cb                    (Widget, XtPointer, XtPointer);
static void    update_field_display       (void);
static void    update_list_fld_diff_flags (GuidlistStruct *);
static void    update_guidance            (String*, int);
static void    update_list                (GuidlistStruct *);
static void    update_select_to_current_cb(Widget, XtPointer, XtPointer);
static void    force_availability_check   (Widget, XtPointer, XtPointer);
static void    valid_popup_cb             (Widget, XtPointer, XEvent *);
static void    valid_time_cb              (Widget, XtPointer, XtPointer);

static Widget     selectMatrix = NullWidget;
static Widget     menuBar;
static Widget     btnBarSW, btnBarRC;
static Widget     listCombo;
static Widget     listTypeDisplay;
static Widget     validPopup, validSelect;
static Widget     fieldAppBtn;
static int        narrows = 0;
static WidgetList arrows  = NullWidgetList;
static Widget     runTimeDisplay;
static Widget     runTimePopup;
static Widget     selectBtn;
static int        list_id_count = 0;
static int        num_matrix_rows = 1;
static Boolean    no_reselection = False;
static Boolean    option_nearest = False;
static Boolean    show_fields_in_effect = True;
static Boolean    show_depictions = True;
static Pixmap     redflag   = 0;
static Pixmap     greenflag = 0;

static Pixel run_ok_colour;
static Pixel caution_colour;
static Pixel not_available_colour;
static Pixel field_missing_colour;


/* Ingest the guidance field information in a work procedure so as to not slow
*  down the startup procedure. This gives a much better feel to the program than
*  the user having to wait for startup. Once done schedule the check procedure
*  which will update the guidance.
*/
/*ARGSUSED*/
static Boolean InitWP(XtPointer client_data )
{
	int nrun;
	String *rtimes;
	GuidanceFieldStruct *fld;

	static int nl = 1, nf = 0;

	if(nl >= GVG_nguidlist)
	{
		/* Register ourselves to be notified of various actions that change
		 * the depictions. Note that we do not deregister these observer
		 * additions when the dialog is not visible as the changes will be
		 * applied to the guidance database even when it is not visible.
		 */
		AddObserver(OB_DEPICTION_ABOUT_TO_CHANGE, ready_guidance_update   );
		AddObserver(OB_DEPICTION_TZERO_CHANGE,    update_guidance         );
		AddObserver(OB_DEPICTION_CHANGE,          update_guidance         );
		AddObserver(OB_FIELD_AVAILABLE,           field_available_observer);
		AddObserver(OB_INTERPOLATE,               update_guidance         );
		AddObserver(OB_TIMELINK_EXIT,             update_guidance         );
		AddObserver(OB_PROFILE_SAVE,              save_active_list        );

		AddIngredObserver(ingred_messages);
		AddSourceObserver(check_for_run_time_change,"GuidanceRunTimeChange");

		InitSamplingPanel();

		NotifyObservers(OB_GUIDANCE_READY, NULL, 0);

		SetIconBarButtonSensitivity(GUIDANCE_SELECT_ICON, True);
		XtSetSensitive(selectBtn, True);

		return True;
	}
	if(nf >= GVG_guidlist[nl]->nfield)
	{
		update_list_fld_diff_flags(GVG_guidlist[nl]);
		set_list_valid_times(GVG_guidlist[nl], GVG_active_time);
		nl++;
		nf = 0;
	}
	else
	{
		fld = GVG_guidlist[nl]->field[nf];
		nrun = source_run_time_list(fld->source->fd, &rtimes);
		switch(fld->rtype)
		{
		case GUID_CURRENT:  fld->run = CreateRunTimeEntry((nrun>0)?rtimes[0]:NULL); break;
		case GUID_PREVIOUS: fld->run = CreateRunTimeEntry((nrun>1)?rtimes[1]:NULL); break;
		}
		(void)source_run_time_list_free(&rtimes, nrun);
		create_field_valid_list(fld, NULL, 0);
		nf++;
	}
	return False;
}


/****************************************************************************/
/*
*   Retrieve the guidance lists from the state store file.
*/
/****************************************************************************/
void InitGuidanceLists(void)
{
	int i, j, nlist, nfld;
	char rtype, vtype, lbuf[16], fbuf[16];
	String ptr, sbuf, element, level, source, subsrc, rtime;
	Boolean ok, prev;
	Pixel legend_default;
	PARM *setup;
	GuidlistStruct *lst;
	GuidanceFieldStruct *fld;
	FpaConfigFieldStruct *fptr;
	Source src;
	INFOFILE fd;

	/* Pixmaps of flags to be displayed on the button bar button.
	 */
	redflag   = XuGetPixmap(GW_menuBar, "redflag");
	greenflag = XuGetPixmap(GW_menuBar, "greenflag");

	/* Set the guidance dialog selection button insensitive untill we are
	*  finished with the initialization.
	*/
	selectBtn = XuMenuFindButton(GW_menuBar, MENU_Guidance_select);
	XtSetSensitive(selectBtn, False);

	/* Set the active time to that of the active depiction.
	*/
	(void) safe_strcpy(GVG_active_time, ActiveDepictionTime(FIELD_INDEPENDENT));

	/* Legend default */
	XtVaGetValues(GW_mainWindow, XmNforeground, &legend_default, NULL);

	/* Create the temporary list.
	*/
	GVG_guidlist            = OneMem(GuidlistStruct *);
	GVG_guidlist[0]         = OneMem(GuidlistStruct);
	GVG_guidlist[0]->label  = XtNewString(XuGetLabel(TEMP_LIST_LABEL));
	GVG_guidlist[0]->ndx    = 0;
	GVG_guidlist[0]->id_key = list_id_count++;
	GVG_nguidlist++;

	/* Look for predefined lists.
	*/
	setup = GetSetupKeyParms(PRESET_LISTS, GUIDANCE_LIST_FILE);
	fd = info_file_open(get_file(PRESET_LISTS, NotNull(setup) ? setup->parm[1] : GUIDANCE_LIST_FILE));
	if(NotNull(fd))
	{
		while(!blank(info_file_find_next_block(fd)))
		{
			lst          = OneMem(GuidlistStruct);
			lst->label   = XtNewString(info_file_get_block_label(fd));
			lst->nfield  = 0;
			lst->field   = (GuidanceFieldStruct **)NULL;
			lst->showing = False;
			lst->fixed   = True;
			lst->ndx     = GVG_nguidlist;
			lst->id_key  = list_id_count++;

			while(!blank(ptr = info_file_get_next_line(fd)))
			{
				element = strtok_arg(ptr);
				level   = strtok_arg(NULL);
				source  = strtok_arg(NULL);
				subsrc  = strtok_arg(NULL);
				prev    = same_start_ic(strtok_arg(NULL),"prev");

				fptr = identify_field(element,level);
				src  = FindSourceByName(source, subsrc);

				if(src && fptr)
				{
					if(NotNull(src->fd->sdef) && src->fd->sdef->src_type == FpaC_DEPICTION)
					{
						rtype = GUID_DEPICT;
						vtype = (prev) ? GUID_PREVIOUS : GUID_CURRENT;
					}
					else
					{
						rtype = (prev) ? GUID_PREVIOUS : GUID_CURRENT;
						vtype = GUID_NOT_DEFINED;
					}

					fld = OneMem(GuidanceFieldStruct);
					fld->info = fptr;
					fld->source = src;
					fld->rtype = rtype;
					fld->run = CreateEmptyRunTimeEntry();
					fld->valid = CreateEmptyValidTimeEntry();
					fld->vtype = vtype;
					fld->vsel = GUID_NO_SEL;
					fld->ndx = lst->nfield;
					fld->list = lst;
					fld->id_key = lst->nfield;
					fld->legend_colour = legend_default;
					fld->legend_colour_default = legend_default;

					lst->field = MoreMem(lst->field, GuidanceFieldStruct*, lst->nfield+1);
					lst->field[lst->nfield] = fld;
					lst->nfield++;
				}
			}
			add_list_item_to_list(lst);
		}
		info_file_close(fd);
	}

	/* Now search the state file for previous lists and load them in sorted order.
	*  We need first of all to find out how many lists there.
	*/
	nlist = 0;
	XuVaStateDataGet(STATE_ID, "ln", NULL, "%d", &nlist);

	/* Scan for the number of lists which should be there.
	*/
	for(i = 1; i < nlist; i++)
	{
		String list_id;

		snprintf(lbuf, sizeof(lbuf), "%d", i);
		if(!XuStateDataGet(STATE_ID, lbuf, NULL, &sbuf)) continue;

		list_id = string_arg(sbuf);
		/* This should never happen - but ... */
		if(blank(list_id))
		{
			FreeItem(sbuf);
			continue;
		}

		/* Ok, we have a list so add it to our structure and find the number
		*  of fields which go into this list.
		*/
		lst          = OneMem(GuidlistStruct);
		lst->label   = XtNewString(list_id);
		lst->nfield  = 0;
		lst->field   = (GuidanceFieldStruct **)NULL;
		lst->showing = False;
		lst->fixed   = False;
		lst->ndx     = GVG_nguidlist;
		lst->id_key  = list_id_count++;

		nfld = int_arg(sbuf, &ok);
		FreeItem(sbuf);

		for(j = 0; j < nfld; j++)
		{
			snprintf(fbuf, sizeof(fbuf), "%d", j);
			if(!XuStateDataGet(STATE_ID, lbuf, fbuf, &sbuf)) continue;

			element = strtok_arg(sbuf);
			level   = strtok_arg(NULL);
			source  = strtok_arg(NULL);
			subsrc  = strtok_arg(NULL);
			rtime   = strtok_arg(NULL);
			rtype   = *strtok_arg(NULL);
			vtype   = *strtok_arg(NULL);

			fptr = identify_field(element,level);
			src  = FindSourceByName(source,subsrc);

			/* Correct errors in state file for depiction fields
			*/
			if(rtype != GUID_DEPICT &&
					NotNull(src) && NotNull(src->fd->sdef) && src->fd->sdef->src_type == FpaC_DEPICTION)
			{
				rtype = GUID_DEPICT;
				vtype = GUID_CURRENT;
			}
			if(rtype == GUID_DEPICT &&
					(vtype != GUID_CURRENT && vtype != GUID_PREVIOUS))
			{
				vtype = GUID_CURRENT;
			}

			/* Accept the field only if the field and soruce pointers are valid and if the
			*  run time is absolute only if the run data still exists.
			*/
			if(src && fptr && (rtype != GUID_ABSOLUTE || is_run_available(src,rtime)))
			{
				fld = OneMem(GuidanceFieldStruct);
				fld->info = fptr;
				fld->source = src;
				fld->rtype = rtype;
				fld->run = CreateRunTimeEntry((rtype == GUID_ABSOLUTE)? rtime:NULL);
				fld->vtype = vtype;
				fld->valid = CreateEmptyValidTimeEntry();
				fld->vsel = GUID_NO_SEL;
				fld->ndx = lst->nfield;
				fld->list = lst;
				fld->id_key = lst->nfield;
				fld->legend_colour = legend_default;
				fld->legend_colour_default = legend_default;

				lst->field = MoreMem(lst->field, GuidanceFieldStruct*, lst->nfield+1);
				lst->field[lst->nfield] = fld;
				lst->nfield++;
			}
			FreeItem(sbuf);
		}
		add_list_item_to_list(lst);
	}

	/* Get the label of the list that was active last time.
	*/
	GVG_active_guidlist = GVG_guidlist[0];
	if(XuVaStateDataGet(STATE_ID,"ln","a","%d",&nlist) && nlist < GVG_nguidlist)
		GVG_active_guidlist = GVG_guidlist[nlist];

	/* Now find out the last display type options set.
	*/
	if(XuStateDataGet(STATE_ID, DISPLAY_OPTION, NULL, &ptr))
	{
		Boolean ok;
		i = int_arg(ptr, &ok); if (ok) GVG_option_full_display = (Boolean) i;
		i = int_arg(ptr, &ok); if (ok) GVG_option_synchro      = (Boolean) i;
		i = int_arg(ptr, &ok); if (ok) option_nearest         = (Boolean) i;
		i = int_arg(ptr, &ok); if (ok) show_depictions        = (Boolean) i;
		XtFree(ptr);
	}

	/* Do the rest of the initialization in a work procedure to fool the user into
	*  thinking that things are going faster than they really are.
	*/
	(void) XtAppAddWorkProc(GV_app_context, InitWP, NULL);
}


void ACTIVATE_guidanceDialog(Widget reference_widget )
{
	int    i;
	Widget w, btn, topForm, mainForm, crc, tabs;
	Widget displayTab;
	String headers[6];

	static XuMenuItemStruct list_menu[] = {
		{"listAdd", &xmPushButtonWidgetClass, 0, None, LIST_ADD_ID,
			dialog_launch_cb, (XtPointer)1, NULL },
		{"saveAs",  &xmPushButtonWidgetClass, 0, None, LIST_SAVE_AS_ID,
			dialog_launch_cb,(XtPointer)2, NULL },
		{"listDel", &xmPushButtonWidgetClass, 0, None, LIST_DEL_ID,
			delete_list_cb, NULL, NULL },
		{"sepDbl",  &xmSeparatorWidgetClass,  0, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"exit",    &xmPushButtonWidgetClass, 0, None, NoId,
			exit_cb, NULL, NULL },
		NULL
	};

	static XuMenuItemStruct field_menu[] = {
		{"fieldAdd", &xmPushButtonWidgetClass, 0, None, FIELD_ADD_ID,
			dialog_launch_cb, (XtPointer)3, NULL },
		{"fieldDel", &xmPushButtonWidgetClass, 0, None, FIELD_DEL_ID,
			dialog_launch_cb, (XtPointer)4, NULL },
		NULL
	};

	static XuMenuItemStruct option_menu[] = {
		{"change", &xmToggleButtonWidgetClass, 0, None, NoId,
			options_cb, (XtPointer)1, NULL },
		{"nearest", &xmToggleButtonWidgetClass, 0, None, NoId,
			options_cb, (XtPointer)2, NULL },
		{"fullDisp", &xmToggleButtonWidgetClass, 0, None, NoId,
			options_cb, (XtPointer)3, NULL },
		{"showDepict", &xmToggleButtonWidgetClass, 0, None, NoId,
			options_cb, (XtPointer)4, NULL },
		NULL
	};

	static XuMenuItemStruct action_menu[] = {
		{"refreshFields", &xmPushButtonWidgetClass, 0, None, NoId,
			refresh_cb, (XtPointer)False, NULL },
		{"update", &xmPushButtonWidgetClass, 0, None, NoId,
			update_select_to_current_cb, NULL, NULL },
		{"forceCheck", &xmPushButtonWidgetClass, 0, None, NoId,
			force_availability_check, NULL, NULL },
		{"sep", &xmSeparatorWidgetClass, 0, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"clear", &xmPushButtonWidgetClass, 0, None, NoId,
			clear_lists_cb, NULL, NULL },
		NULL
	};

	static XuMenuItemStruct help_menu[] = {
		{"mainHelp",      &xmPushButtonWidgetClass, 0, None, NoId,
			HelpCB, HELP_GUIDANCE, NULL },
		{"listAddHelp",   &xmPushButtonWidgetClass, 0, None, NoId,
			HelpCB, HELP_GUIDANCE_LIST_ADD, NULL },
		{"fieldAddHelp",  &xmPushButtonWidgetClass, 0, None, NoId,
			HelpCB, HELP_GUIDANCE_FIELD_ADD, NULL },
		{"fieldAppHelp",  &xmPushButtonWidgetClass, 0, None, NoId,
			HelpCB, HELP_GUIDANCE_FIELD_APPEAR, NULL },
		{"fieldDelHelp",  &xmPushButtonWidgetClass, 0, None, NoId,
			HelpCB, HELP_GUIDANCE_FIELD_DEL, NULL },
		{"fieldSampHelp", &xmPushButtonWidgetClass, 0, None, NoId,
			HelpCB, HELP_GUIDANCE_FIELD_SAMPLE, NULL },
		NULL
	};

	static XuMenuBarItemStruct menu_bar[] = {
		{"lists",   None, NoId, list_menu},
		{"fields",  None, NoId, field_menu},
		{"options", None, NoId, option_menu},
		{"actions", None, NoId, action_menu},
		{"help",    None, NoId, help_menu},
		NULL
	};

	if(NotNull(GVG_selectDialog))
	{
		XuShowDialog(GVG_selectDialog);
		return;
	}

	show_fields_in_effect = True;

    GVG_selectDialog = XuCreateMainWindowDialog(reference_widget, "guidSelect",
		XuNallowIconify, True,
		XuNmwmDeleteOverride, exit_cb,
        NULL);

	option_menu[0].options = GVG_option_synchro ?      XuMENU_SET:XuMENU_NONE;
	option_menu[1].options = option_nearest ?         XuMENU_SET:XuMENU_NONE;
	option_menu[2].options = GVG_option_full_display ? XuMENU_SET:XuMENU_NONE;
	option_menu[3].options = show_depictions ?        XuMENU_SET:XuMENU_NONE;
	
	/* Set our status colours */
	run_ok_colour        = XuLoadColorResource(GVG_selectDialog, RNguidanceRunOk, "Green");
	caution_colour       = XuLoadColorResource(GVG_selectDialog, RNguidanceRunCaution, "Yellow");
	not_available_colour = XuLoadColorResource(GVG_selectDialog, RNguidanceRunNA, "Red");
	field_missing_colour = XuLoadColorResource(GVG_selectDialog, RNguidanceFieldMissing, "IndianRed");

	/* If not in edit the menu bar is limited in what is shown.
	 */
	if(!GV_edit_mode) menu_bar[4].name = NULL;

	menuBar = XuMenuBuildMenuBar(GVG_selectDialog, "menuBar", menu_bar);

	topForm = XmCreateForm(GVG_selectDialog, "topForm", NULL, 0);

	/* The height of this scrolled window is adjusted to just fit the time
	 * selection buttons when the buttons are activated. 50 is just close
	 * to the expected size to reduce the amount of change.
	 */
	btnBarSW = XmVaCreateManagedScrolledWindow(topForm, "btnBarSW",
		XmNheight, 50,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtVaGetValues(btnBarSW, XmNverticalScrollBar, &w, NULL);
	XtUnmanageChild(w);

	btnBarRC = XmVaCreateManagedRowColumn(btnBarSW, "btnBarRC",
		XmNorientation, XmHORIZONTAL,
		XmNradioBehavior, True,
		NULL);

	mainForm = XmVaCreateForm(topForm, "mainForm",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, btnBarSW,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	crc = XmVaCreateManagedRowColumn(mainForm, "crc",
		XmNorientation, XmHORIZONTAL,
		XmNpacking, XmPACK_TIGHT,
		XmNspacing, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(crc, "comboLabel",
		XmNmarginHeight, 0,
		NULL);

	listCombo = XmVaCreateManagedComboBox(crc, "listSelect", NULL);
	XtAddCallback(listCombo, XmNselectionCallback, list_cb, (XtPointer)NULL);

	listTypeDisplay = XmVaCreateManagedLabel(mainForm, "ltd",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, crc,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, crc,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, crc,
		XmNbottomOffset, 0,
		NULL);

	/* If in edit mode we will control and sample guidance. If not then we
	 * are in display only mode and only the guidance display is available.
	 * If both we use two tabs, if not just one form.
	 */
	if(GV_edit_mode)
	{
			tabs = XmVaCreateTabStack(mainForm, "tabs",
			XmNtopAttachment,     XmATTACH_WIDGET,
			XmNtopWidget,         crc,
			XmNleftAttachment,    XmATTACH_FORM,
			XmNrightAttachment,   XmATTACH_FORM,
			XmNbottomAttachment,  XmATTACH_FORM,
			NULL);
		XtAddCallback(tabs, XmNtabSelectedCallback, tabs_cb, NULL);

		displayTab = XmVaCreateForm(tabs, "displayTab",
			XmNhorizontalSpacing, 9,
			XmNverticalSpacing, 9,
			NULL);

		GVG_animateTab = XmVaCreateForm(tabs, "animateTab",
			XmNhorizontalSpacing, 9,
			XmNverticalSpacing, 9,
			NULL);

		GVG_sampleTab = XmVaCreateForm(tabs, "sampleTab",
			XmNhorizontalSpacing, 9,
			XmNverticalSpacing, 9,
			NULL);
	}
	else
	{
		displayTab = XmVaCreateForm(mainForm, "displayTab",
			XmNhorizontalSpacing, 9,
			XmNverticalSpacing,   9,
			XmNtopAttachment,     XmATTACH_WIDGET,
			XmNtopWidget,         crc,
			XmNleftAttachment,    XmATTACH_FORM,
			XmNrightAttachment,   XmATTACH_FORM,
			XmNbottomAttachment,  XmATTACH_FORM,
			NULL);
	}

	/* ============ Create Display Field Tab Content ==============*/

	btn = XmVaCreateManagedPushButton(displayTab, "legend",
		XmNmarginHeight, 3,
		XmNmarginWidth, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, ShowGuidanceLegendCB, NULL);

	fieldAppBtn = XmVaCreateManagedPushButton(displayTab, "fieldApp",
		XmNmarginHeight, 3,
		XmNmarginWidth, 6,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, btn,
		NULL);
	XtAddCallback(fieldAppBtn, XmNactivateCallback, dialog_launch_cb, (XtPointer)5);

	/* It is important that the runTimePopup be created before the selectMatrix widget
	 * so that it will appear on top of the matrix when managed.
	 */
	runTimePopup = XmVaCreatePushButton(displayTab, "runTimePopup",
		XmNborderWidth, 1,
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNshadowThickness, 0,
		XmNforeground, XuLoadColor(GVG_selectDialog,"Black"),
		XmNbackground, XuLoadColor(GVG_selectDialog,"White"),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(runTimePopup, XmNactivateCallback, run_time_popup_cb, NULL);

	headers[0] = XuGetStringResource(RNlevelHeader, "Level");
	headers[1] = XuGetStringResource(RNfieldHeader, "Field");
	headers[2] = XuGetStringResource(RNsourceHeader, "Source");
	headers[3] = XuGetStringResource(RNissueHeader, "Issue Time");
	headers[4] = XuGetStringResource(RNvalidHeader, "Valid Time");
	headers[5] = XuGetStringResource(RNissueHeader, " ");

	selectMatrix = CreateXbaeMatrix(displayTab, "selectMatrix",
		XmNrows, num_matrix_rows,
		XmNcolumns, 6,
		XmNcolumnLabels, headers,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNshadowThickness, 2,
		XmNcellShadowThickness, 1,
		XmNgridType, XmGRID_ROW_SHADOW,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, fieldAppBtn,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(selectMatrix, XmNselectCellCallback, select_cell_cb, NULL); 
	XtAddCallback(selectMatrix, XmNtrackCellCallback, track_cell_cb, NULL); 

	XbaeMatrixSetColumnWidth(selectMatrix, ARROW_COLUMN, 19);
	XbaeMatrixResizeColumnsToCells(selectMatrix, True);

	/* Make the run time display.
	*/
	runTimeDisplay = XmVaCreateLabel(displayTab, "runTimeDisplay",
		XmNborderWidth, 1,
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNforeground, XuLoadColor(GVG_selectDialog,"Black"),
		XmNbackground, XuLoadColor(GVG_selectDialog,"White"),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	/* Make the popup for valid time selection.
	*/
	validPopup = XuVaCreatePopupShell("validPopup",
		transientShellWidgetClass, GVG_selectDialog,
		XmNoverrideRedirect, True,
		XmNsaveUnder,        False,
		XmNallowShellResize, True,
		NULL);
	XtAddEventHandler(validPopup,
		LeaveWindowMask,
		False, (XtEventHandler) valid_popup_cb, NULL);

	validSelect = XmVaCreateManagedScrolledList(validPopup, "validSelect",
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNlistMarginWidth, 6,
		XmNlistMarginHeight, 6,
		NULL);
	XtAddCallback(validSelect, XmNbrowseSelectionCallback, valid_time_cb, (XtPointer)NULL);

	/* ============== Manage all items ==================== */

	XmMainWindowSetAreas(GVG_selectDialog, menuBar, NULL, NULL, NULL, topForm);
	XtManageChild(selectMatrix);
	XtManageChild(displayTab);
	/*
	 * if in edit mode we include sampling and animation tabs.
	 */
	if (GV_edit_mode)
	{
		LayoutGuidanceSampleTab();
		XtManageChild(GVG_sampleTab);
		LayoutGuidanceAnimationTab();
		XtManageChild(GVG_animateTab);
		XtManageChild(tabs);
	}
	XtManageChild(mainForm);
	XtManageChild(menuBar);
	XtManageChild(topForm);
	XuShowDialog(GVG_selectDialog);

	XuSetBusyCursor(ON);

	update_list(GVG_active_guidlist);
	SetGuidanceDisplayState(True);
	if(!show_depictions) SetDepictionVisibility(OFF);

	XuSetBusyCursor(OFF);
}


/* Dialog to process selected fields when new guidance has arrived and moved the current
 * guidance to the previous guidance directory. This dialog is activated by the red flag
 * button widget located in the icon bar in main.c. This button is only made visible when
 * guidance is updating and the user has selected guidance that will be impacted by this
 * update. This dialog allows the user to select the action to be taken. This dialog was
 * placed in this file instead of in it's own file as it is very simple and used functions
 * in this file.
 *
 * Two choices here - keep using the existing fields, which will be indicated by the field
 * selection moving to an entry for a previous issue time, or update the display field to
 * the current issue time.
 */
void ACTIVATE_processGuidanceUpdateDialog (Widget refw)
{
	Widget w = GetIconBarWidget(GUID_SELECT_STATUS_ICON);
	if (w) XtUnmanageChild(w);
	StopGuidanceArrivedIndicator();

	/* Ask the user what they want to do.  */
	if(XuAskUser(refw, (no_reselection)? "GuidancePartialReselection":"GuidanceReselection", NULL) == XuYES)
		update_select_to_current_cb(NULL, NULL, NULL);
	no_reselection = False;
}


/****************************************************************************/
/*
*   Display guidance for the given time.
*/
/****************************************************************************/
void DisplayGuidanceAtTime( String dt )
{
	int i;
	Boolean state;

	if( GVG_nguidlist < 1                    ) return;
	if( !GVG_option_synchro                  ) return;
	if( !valid_tstamp(dt)                   ) return;
	if( matching_tstamps(GVG_active_time,dt) ) return;

	(void) safe_strcpy(GVG_active_time, dt);

	for( i = 0; i < GVG_nguidlist; i++ )
		set_list_valid_times(GVG_guidlist[i], dt);

	if(show_fields_in_effect)
		(void) IngredCommand(GE_GUIDANCE, "SHOW");

	if(NotNull(GVG_selectDialog))
	{
		for(i = 0; i < GVG_nvalid_seq; i++)
		{
			state = matching_tstamps(dt, GVG_valid_seq[i]);
			XuToggleButtonSet(GVG_btnBarBtns[i], state, False);
		}
	}
}


/****************************************************************************/
/*
*   The function called in response to the global pulldown buttons to hide
*   and show the guidance fields.
*/
/****************************************************************************/
void SetGuidanceDisplayState(Boolean state )
{
	int     i, n;
	String  notify[1];
	Boolean have_one = False;

	show_fields_in_effect = state;
	/*
	 * If the guidance is to be shown reset the valid time of the fields
	 * just in case they were modified. This usually happens in animation.
	 */
	if (state)
	{
		for(i = 0; i < GVG_nguidlist; i++)
		{
			for(n = 0; n < GVG_guidlist[i]->nfield; n++)
			{
				GuidanceFieldStruct *fld = GVG_guidlist[i]->field[n]; 
				/*
				 * Skip fields that are not registered
				 */
				if(blank(fld->id)) continue;

				/*
				 * Reset visibility for registered fields
				 */
				if(fld->showing)
				{
					(void) IngredVaCommand(GE_GUIDANCE, "FIELD_VISIBILITY %s %s ON",
							fld->id, fld->valid->times[fld->vsel]);
					have_one = True;
				}
				else if(fld->vsel != GUID_NO_SEL)
				{
					(void) IngredVaCommand(GE_GUIDANCE, "FIELD_VISIBILITY %s %s OFF",
							fld->id, fld->valid->times[fld->vsel]);
				}
			}
		}
	}
	/*
	 * The guidance legend will only be activated if at least
	 * one field is showing.
	 */
	if(have_one)
		ACTIVATE_guidanceLegendDialog(GW_mainWindow);

	(void) IngredCommand(GE_GUIDANCE, (state)? "SHOW":"HIDE");
	notify[0] = (String) ((long)state);
	NotifyObservers(OB_GUIDANCE_VISIBILITY, notify, 1);
}


Boolean GetGuidanceDisplayState(void)
{
	return show_fields_in_effect;
}


/****************************************************************************/
/*
*   Add a new guidance list item to the list of guidance lists.
*/
/****************************************************************************/
Boolean GuidanceListAddItem(String id , Boolean copy_active )
{
	int    i;
	String *items;
	GuidlistStruct *lp;

	/* If we already have the given id return false.
	*/
	for(i = 0; i < GVG_nguidlist; i++)
	{
		if(same_ic(GVG_guidlist[i]->label, id)) return False;
	}

	lp          = OneMem(GuidlistStruct);
	lp->label   = XtNewString(id);
	lp->nfield  = 0;
	lp->field   = NULL;
	lp->showing = False;
	lp->fixed   = False;
	lp->id_key  = list_id_count++;

	if(copy_active && GVG_active_guidlist->nfield > 0)
	{
		lp->nfield = GVG_active_guidlist->nfield;
		lp->field  = NewMem(GuidanceFieldStruct*, GVG_active_guidlist->nfield);

		for(i = 0; i < GVG_active_guidlist->nfield; i++)
		{
			lp->field[i] = OneMem(GuidanceFieldStruct);
			CopyStruct(lp->field[i], GVG_active_guidlist->field[i], GuidanceFieldStruct, 1);
			lp->field[i]->id[0]        = '\0';
			lp->field[i]->show         = False;
			lp->field[i]->showing      = False;
			lp->field[i]->list         = lp;
		}
	}

	add_list_item_to_list(lp);

	items = NewStringArray(GVG_nguidlist);
	for(i = 0; i < GVG_nguidlist; i++)
	{
		GVG_guidlist[i]->ndx = i;
		items[i] = GVG_guidlist[i]->label;
	}
	XuComboBoxDeleteAllItems(listCombo);
	XuComboBoxAddItems(listCombo, items, GVG_nguidlist, 0);
	XuComboBoxSelectItem(listCombo, lp->label, False);
	FreeItem(items);

	change_list(lp->ndx);
	XuUpdateDisplay(GVG_selectDialog);
	save_lists();
	return True;
}


/****************************************************************************/
/*
*  Add a guidance field to the currently active list. The data structure
*  passed in must be allocated memory. The variable posn gives the position
*  in the list to which to add the field. If posn = 0, the data is added to
*  the bottom of the list (as in XmList).
*/
/****************************************************************************/
Boolean GuidanceListAddField(GuidanceFieldStruct *data , int posn )
{
	int i, ndx;
	String s1, s2;
	GuidanceFieldStruct **newfld;

	/* Check to see that this field is not already in the list.
	*/
	for(i = 0; i < GVG_active_guidlist->nfield; i++)
	{
		if(!same(data->info->label, GVG_active_guidlist->field[i]->info->label)) continue;

		s1 = SrcShortLabel(data->source);
		s2 = SrcShortLabel(GVG_active_guidlist->field[i]->source);
		if(!same(s1, s2)) continue;

		if(data->rtype != GVG_active_guidlist->field[i]->rtype) continue;

		if(data->rtype != GUID_ABSOLUTE) continue;

		if(!matching_tstamps(RunTime(data->run), RunTime(GVG_active_guidlist->field[i]->run))) continue;

		return False;
	}

	newfld = NewMem(GuidanceFieldStruct*, GVG_active_guidlist->nfield+1);

	if(posn) posn--;
	else     posn = GVG_active_guidlist->nfield;

	for(i = 0; i < posn; i++)
	{
		newfld[i] = GVG_active_guidlist->field[i];
		newfld[i]->ndx = i;
	}
	for(i = posn+1; i <= GVG_active_guidlist->nfield; i++)
	{
		newfld[i] = GVG_active_guidlist->field[i-1];
		newfld[i]->ndx = i;
	}

	FreeItem(GVG_active_guidlist->field);
	GVG_active_guidlist->field = newfld;
	GVG_active_guidlist->nfield++;

	newfld[posn]         = OneMem(GuidanceFieldStruct);
	newfld[posn]->info   = data->info;
	newfld[posn]->source = data->source;
	newfld[posn]->rtype	 = data->rtype;
	newfld[posn]->run	 = CopyRunTimeEntry(data->run);
	newfld[posn]->vtype	 = data->vtype;
	newfld[posn]->vsel	 = data->vsel;
	newfld[posn]->valid	 = CreateEmptyValidTimeEntry();
	newfld[posn]->ndx    = posn;
	newfld[posn]->list   = GVG_active_guidlist;
	newfld[posn]->id_key = -1;

	/* The id_key must be unique within this list. Scan all of the
	 * other fields and assign a key that is not already taken.
	 */
	ndx = 0;
	while(newfld[posn]->id_key < 0)
	{
		Boolean unique;
		for(unique = True, i = 0; i < GVG_active_guidlist->nfield && unique; i++)
		{
			if(GVG_active_guidlist->field[i]->id_key == ndx) unique = False;
		}
		if(unique) newfld[posn]->id_key = ndx;
		ndx++;
	}

	create_field_valid_list(GVG_active_guidlist->field[posn], NULL, 0);
	update_list_fld_diff_flags(GVG_active_guidlist);
	update_field_display();
	save_lists();
	return True;
}


/****************************************************************************/
/*
*   Remove a list of guidance fields from the specified list.
*/
/****************************************************************************/
void GuidanceListRemoveFields(int *sl , int nsl )
{
	int i, j, n;
	Boolean delete;
	GuidanceFieldStruct **newfld, *fld;

	XuSetBusyCursor(ON);
	XbaeMatrixDisableRedisplay(selectMatrix);

	newfld = NewMem(GuidanceFieldStruct*, GVG_active_guidlist->nfield - nsl);
	n = 0;
	(void) IngredCommand(GE_GUIDANCE, "HIDE");
	for(i = 0; i < GVG_active_guidlist->nfield; i++)
	{
		fld = GVG_active_guidlist->field[i];
		delete = False;
		for(j = 0; j < nsl; j++)
		{
			if((delete = (i == sl[j]))) break;
		}
		if(delete)
		{
			if(fld->show)
			{
				XbaeMatrixDeselectRow(selectMatrix, fld->ndx);
				set_field_show_state(fld, False);
			}
			if(!blank(fld->id))
				(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld->id);
			RemoveRunTimeEntry(fld->run);
			RemoveValidTimeEntry(fld->valid);
			FreeItem(fld);
		}
		else
		{
			newfld[n] = fld;
			newfld[n]->ndx = n;
			n++;
		}
	}
	SetGuidanceDisplayState(True);
	GVG_active_guidlist->nfield -= nsl;
	FreeItem(GVG_active_guidlist->field);
	GVG_active_guidlist->field = newfld;
	XbaeMatrixEnableRedisplay(selectMatrix, True);

	update_field_display();
	save_lists();
	XuSetBusyCursor(OFF);
}


void SetGuidanceDialogSensitivity(Boolean state )
{
	Widget w;

	if(IsNull(GVG_selectDialog)) return;

	w = XuMenuFindButton(menuBar, LIST_ADD_ID);
	XtSetSensitive(w, state);
	w = XuMenuFindButton(menuBar, LIST_SAVE_AS_ID);
	XtSetSensitive(w, state);
	w = XuMenuFindButton(menuBar, LIST_DEL_ID);
	XtSetSensitive(w, state && !(GVG_active_guidlist->fixed || GVG_active_guidlist->ndx < 1));
	XtSetSensitive(XtNameToWidget(menuBar, "fields"), state);
	XtSetSensitive(XtNameToWidget(menuBar, "options"), state);
	XtSetSensitive(XtNameToWidget(menuBar, "actions"), state);
}



/*=========================== LOCAL FUNCTIONS =============================*/



/* Callback used to popup the label containing the actual issue time of a field
 */
/*ARGSUSED*/
static void track_cell_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XbaeMatrixTrackCellCallbackStruct *rtn = (XbaeMatrixTrackCellCallbackStruct *)call_data;

	if( rtn->row < 0 || 
		rtn->column != ISSUE_COLUMN || 
		rtn->row >= GVG_active_guidlist->nfield || 
		blank(RunTime(GVG_active_guidlist->field[rtn->row]->run)) )
	{
		XtUnmanageChild(runTimePopup);
	}
	else
	{
		int x, y, height;
		Position wx, wy, dx, dy, cx, cy;
		XtWidgetGeometry dsize;
		Widget clipWindow;

		XtTranslateCoords(XtParent(runTimePopup), 0, 0, &dx, &dy);
		XtVaGetValues(w, XmNclipWindow, &clipWindow, NULL);
		XtTranslateCoords(clipWindow, 0, 0, &wx, &wy);

		height = XbaeMatrixGetRowHeight(w, rtn->row);
		(void) XbaeMatrixRowColToXY(w, rtn->row, rtn->column, &x, &y);
		cx = (Position) x;
		cy = (Position) y;

		dsize.request_mode = CWWidth|CWHeight;
		(void) XtQueryGeometry(runTimeDisplay, NULL, &dsize);

		XtVaSetValues(runTimePopup,
			XmNleftOffset, wx + cx - dx, 
			XmNtopOffset, wy + cy - dy - (dsize.height - height)/2, 
			XmNuserData, INT2PTR(rtn->row),
			NULL);
		XuWidgetLabel(runTimePopup, DateString(RunTime(GVG_active_guidlist->field[rtn->row]->run),HOURS));
		XtManageChild(runTimePopup);
	}
}


static void set_field_show_state( GuidanceFieldStruct *fld, Boolean state )
{
	set_field_for_show(fld, OFF);
	fld->show = state;
	make_sequence_btns();
	set_field_for_show(fld, ON);
	SetGuidanceDisplayState(True);
	UpdateAppearanceDialog();
	if (GV_edit_mode) UpdateGuidanceSampleTab(False);
}


/*ARGSUSED*/
static void select_cell_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	Boolean show;
	GuidanceFieldStruct *fld;
	XbaeMatrixSelectCellCallbackStruct *rtn = (XbaeMatrixSelectCellCallbackStruct *)call_data;

	if( rtn->row < 0 || rtn->column < 0 || rtn->column == ARROW_COLUMN ) return;
	if( rtn->row < 0 || rtn->row >= GVG_active_guidlist->nfield) return;

	fld = GVG_active_guidlist->field[rtn->row];

	if( !fld->available ) return;

	if(XbaeMatrixIsRowSelected(w, rtn->row))
	{
		show = False;
		XbaeMatrixDeselectRow(w, rtn->row);
		if(!blank(fld->id))
		{
			(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld->id);
			fld->id[0] = '\0';
		}
	}
	else
	{
		show = True;
		XbaeMatrixSelectRow(w, rtn->row);
		if(blank(fld->id))
		{
			snprintf(fld->id, sizeof(fld->id), "%d.%d", fld->list->id_key, fld->id_key);
			(void) IngredVaCommand(GE_GUIDANCE, "FIELD_REGISTER %s %s %s %s %s %s",
				fld->id,
				fld->info->element->name, fld->info->level->name,
				SrcName(fld->source), SrcSubDashName(fld->source),
				blank(RunTime(fld->run))? "-":RunTime(fld->run));
		}
	}

	set_field_show_state(fld, show);
	ACTIVATE_guidanceLegendDialog(GVG_selectDialog);
}


/*  If the guidance mode is set to follow the depictions, this function
 *  turns off the existing guidance display. This allows the guidance fields
 *  to be hidden before the active field is changed so that the depiction
 *  and guidance fields appear to change together.
 */
/*ARGSUSED*/
static void ready_guidance_update( String *unused, int n )
{
	if(GVG_option_synchro && !matching_tstamps(GVG_active_time,ActiveDepictionTime(FIELD_INDEPENDENT)))
		(void) IngredCommand(GE_GUIDANCE, "HIDE");
}

/* Check for changes to depictions in the depiction sequence. It is done as a work
 * procedure to minimize any interruptions to the user. The sequence_change_lock
 * is just additional paranoia.
 */
static Boolean sequence_change_lock = False;

/*ARGSUSED*/
static Boolean sequence_change_wp(XtPointer client_data )
{
	static int i = 0, j = 0;

	if(i >= GVG_nguidlist)
	{
		i = 0;
		j = 0;
		sequence_change_lock = False;
		return True;
	}

	if(j >= GVG_guidlist[i]->nfield)
	{
		j = 0;
		update_list_fld_diff_flags(GVG_guidlist[i]);
		i++;
		return False;
	}

	while(j < GVG_guidlist[i]->nfield)
	{
		GuidanceFieldStruct *fld = GVG_guidlist[i]->field[j];
		if(fld->rtype == GUID_DEPICT)
		{
			FLD_DESCRIPT fd;
			copy_fld_descript(&fd, fld->source->fd);

			if(set_fld_descript(&fd,FpaF_ELEMENT,fld->info->element,FpaF_LEVEL,fld->info->level,FpaF_END_OF_LIST))
			{
				String  *vt;
				int     nvt = FilteredValidTimeList(&fd, fld->info->element->elem_tdep->time_dep, &vt);

				/* Bug #20050621.1 - added brackets around the (nvt + ...) section. Without them
				*                   the logical operation did not function properly.
				*/
				if(fld->valid->ntimes != (nvt + 2))
				{
					if(!blank(fld->id))
					{
						if(fld->show)
						{
							set_field_for_show(fld, OFF);
							fld->show = False;
							XbaeMatrixDeselectRow(selectMatrix, fld->ndx);
						}
						(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld->id);
						fld->id[0] = '\0';
					}
					create_field_valid_list(fld, vt, nvt);
				}
				nvt = FilteredValidTimeListFree(&vt, nvt);
			}
			j++;
			break;
		}
		j++;
	}
	return False;
}


/* Just in case the above work procedure might be called multiple times a timeout
 * call is used to filter out any calls that are really close together. A half
 * second delay is used for this purpose. The delay_count variable is used to
 * prevent any possible race conditions.
 */
static void sequence_change_tm( XtPointer client_data, XtIntervalId *id )
{
	static int delay_count = 0;
	static XtIntervalId current_id = (XtIntervalId) 0;

	if(!id)
	{
		delay_count = 0;
		if (current_id) XtRemoveTimeOut(current_id);
		current_id = XtAppAddTimeOut(GV_app_context, 500, sequence_change_tm, NULL);
	}
	else if(delay_count > 300)
	{
		delay_count = 0;
		sequence_change_lock = False;
		if (current_id) XtRemoveTimeOut(current_id);
		current_id = (XtIntervalId) 0;
	}
	else if(sequence_change_lock)
	{
		delay_count++;
		if (current_id) XtRemoveTimeOut(current_id);
		current_id = XtAppAddTimeOut(GV_app_context, 500, sequence_change_tm, NULL);
	}
	else
	{
		delay_count = 0;
		sequence_change_lock = True;
		(void) XtAppAddWorkProc(GV_app_context, sequence_change_wp, NULL);
		current_id = (XtIntervalId) 0;
	}
}


/* If a field has been added or removed just process with the sequence
 * changed work procedure. 
 */
/*ARGSUSED*/
static void field_available_observer(String *parms, int nparms)
{
	sequence_change_tm(NULL, NULL);
}



/* Handle messages fronm Ingred. When a field is registered Ingred will send
 * information required to display the field in the legend dialog. The syntax
 * for this is:
 *
 *    GUIDANCE LEGEND tag colour_index
 *
 * and thus will be found in parameters 1 to 5.   
 */
static void ingred_messages( CAL cal, String *parms, int nparms )
{
	int i, j;

	if(!same_ic(parms[0], "GUIDANCE")) return;
	if(!same_ic(parms[1], "LEGEND")) return;
	
	for(i = 0; i < GVG_nguidlist; i++)
	{
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			if(same(GVG_guidlist[i]->field[j]->id, parms[2]))
			{
				ColorIndex ci;
				Boolean state;
				GuidanceFieldStruct *fld = GVG_guidlist[i]->field[j];
				ci = (ColorIndex) atoi(parms[3]);
				if(fld->legend_colour_default != ci || fld->legend_colour != ci)
				{
					fld->legend_colour_default = ci;
					fld->legend_colour         = ci;
					state = GuidanceLegendDialogActivationState(True);
					ACTIVATE_guidanceLegendDialog(GVG_selectDialog);
					(void) GuidanceLegendDialogActivationState(state);
				}
				return;
			}
		}
	}
}



/*  Respond to a change in the active depiction. Note that if "Previous" is
 *  showing as the valid time selection we have a special case and do not 
 *  want to change this as we do with the other valid times.
 */
static void update_guidance( String *parms, int nparms )
{
	int i;
	Boolean state, depict_sequence_has_changed;
	String dt;

	if(GVG_nguidlist < 1) return;

	depict_sequence_has_changed = (nparms > 0 && PTR2INT(parms[0]) != 0);

	/* If the depiction sequence has changed we must update the valid time
	*  list of all depiction related fields.
	*/
	if(depict_sequence_has_changed)
		sequence_change_tm(NULL,NULL);

	/* If we want the fields to change in synch with the depictions we 
	*  must check to see if the active depiction has changed.
	*/
	dt = ActiveDepictionTime(FIELD_INDEPENDENT);
	if(GVG_option_synchro && !matching_tstamps(GVG_active_time,dt))
	{
		(void) safe_strcpy(GVG_active_time, dt);

		for( i = 0; i < GVG_nguidlist; i++ )
			set_list_valid_times(GVG_guidlist[i], GVG_active_time);

		if(show_fields_in_effect)
			(void) IngredCommand(GE_GUIDANCE, "SHOW");

		if(NotNull(GVG_selectDialog))
		{
			for(i = 0; i < GVG_nvalid_seq; i++)
			{
				state = matching_tstamps(GVG_active_time, GVG_valid_seq[i]);
				XuToggleButtonSet(GVG_btnBarBtns[i], state, False);
			}
		}
	}
	else if(NotNull(GVG_selectDialog))
	{
		label_sequence_btns();
	}
	return;
}



static void add_list_item_to_list(GuidlistStruct *lst)
{
	int i, k;

	GVG_guidlist = MoreMem(GVG_guidlist, GuidlistStruct*, GVG_nguidlist+1);

	/* The temporary list is not sorted against.
	*/
	for(i = 1, k = 1; i < GVG_nguidlist; i++,k++)
	{
		GVG_guidlist[i]->ndx = i;
		if(strcmp(GVG_guidlist[i]->label, lst->label) > 0) break;
	}
	for(i = GVG_nguidlist; i > k; i--)
	{
		GVG_guidlist[i] = GVG_guidlist[i-1];
		GVG_guidlist[i]->ndx = i;
	}
	GVG_guidlist[k] = lst;
	GVG_guidlist[k]->ndx = k;
	GVG_nguidlist++;
}


/*ARGSUSED*/
static void tabs_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTabStackCallbackStruct *rtn = (XmTabStackCallbackStruct *)call_data;

	if((GVG_sample_tab_active = (rtn->selected_child == GVG_sampleTab)))
		ActivateGuidanceSampleTab();
	else
		DeactivateGuidanceSampleTab();

	if(rtn->selected_child == GVG_animateTab)
		ActivateGuidanceAnimationTab();
	else
		DeactivateGuidanceAnimationTab();
}


static Boolean is_run_available(Source source , String rtime )
{
	int nr;
	String *rl;
	Boolean found;

	nr = source_run_time_list(source->fd, &rl);
	found = InTimeList(rtime, rl, nr, NULL);
	(void)source_run_time_list_free(&rl, nr);
	return found;
}



/* This function is somewhat complicated due to the code which attempts to
 * minimize the number of times that the closest_source_valid_time() function
 * is called as this function is rather slow.
 */
static void set_list_valid_times(GuidlistStruct *l, String match_time )
{
	int i, posn;
	char show_dt[32];
	String dt;
	GuidanceFieldStruct *fld;
	FLD_DESCRIPT fd;

	(void) safe_strcpy(show_dt, match_time);
	for(i = 0; i < l->nfield; i++)
	{
		fld = l->field[i];
		set_field_for_show(fld, OFF);
		if(fld->vtype != GUID_CURRENT && fld->vtype != GUID_PREVIOUS)
		{
			if(option_nearest && fld->diff)
			{
				copy_fld_descript(&fd, fld->source->fd);
				if(set_fld_descript(&fd, FpaF_RUN_TIME, RunTime(fld->run), FpaF_END_OF_LIST))
				{
					closest_source_valid_time(&fd, fld->info->element->elem_tdep->time_dep,
						match_time, &dt);
					(void) safe_strcpy(show_dt, dt);
				}
			}
			if(InValidTimeEntry(show_dt, fld->valid, &posn))
				fld->vsel = posn;
			else
				fld->vsel = GUID_NO_SEL;
		}
		set_field_for_show(fld, ON);
		fill_valid_time(fld);
	}
	XbaeMatrixResizeColumnsToCells(selectMatrix, True);
}


static void set_field_for_show(GuidanceFieldStruct *fld , Boolean show )
{
	int     i, pos;
	String  dt;
	Boolean old_registration = True;

	if(fld->vsel == GUID_NO_SEL || fld->valid->ntimes < 1) return;

	if(show)
	{
		if(!fld->show || fld->showing) return;
		if(blank(fld->id))
		{
			old_registration = False;
			snprintf(fld->id, sizeof(fld->id), "%d.%d", fld->list->id_key, fld->id_key);
			(void) IngredVaCommand(GE_GUIDANCE, "FIELD_REGISTER %s %s %s %s %s %s",
				fld->id,
				fld->info->element->name, fld->info->level->name,
				SrcName(fld->source), SrcSubDashName(fld->source),
				blank(RunTime(fld->run))? "-":RunTime(fld->run));
		}
		switch(fld->vtype)
		{
			case GUID_CURRENT:
				if(!InValidTimeEntry(GVG_active_time, fld->valid, &pos)) return;
				if(pos < 2) return;
				fld->vsel = pos;
				break;

			case GUID_PREVIOUS:
				if(!InValidTimeEntry(GVG_active_time, fld->valid, &pos)) return;
				if(pos < 3) return;
				fld->vsel = pos - 1;
				break;
		}
		dt = fld->valid->times[fld->vsel];
		fld->showing = True;
		fld->list->showing = True;
		(void) IngredVaCommand(GE_GUIDANCE, "FIELD_VISIBILITY %s %s ON", fld->id, dt);
	}
	else
	{
		if(!fld->showing) return;
		dt = fld->valid->times[fld->vsel];
		fld->showing = False;
		(void) IngredVaCommand(GE_GUIDANCE, "FIELD_VISIBILITY %s %s OFF", fld->id, dt);
		fld->list->showing = False;
		for(i = 0; i < fld->list->nfield; i++)
		{
			if((fld->list->showing = fld->list->field[i]->showing)) break;
		}
	}
	set_appearance_btn_state();

	/* If the field was already registered then display the legend. If not then ingred will
	 * end up calling the ingred_message function and the legend is displayed there.
	 */
	if (old_registration) ACTIVATE_guidanceLegendDialog(GVG_selectDialog);
}


static void fill_valid_time(GuidanceFieldStruct *fld )
{
	char  mbuf[256], *rt, *dt;
	Pixel color;

	if(IsNull(GVG_selectDialog) || fld->list != GVG_active_guidlist) return;

	color = XuLoadColor(selectMatrix, XmNforeground);

	if(fld->vsel == GUID_NO_SEL || fld->valid->ntimes < 1)
	{
		(void) safe_strcpy(mbuf, XuGetLabel((GVG_option_full_display)? "na":"na-short"));
	}
	else if(fld->vtype == GUID_CURRENT)
	{
		(void) safe_strcpy(mbuf, XuGetLabel((GVG_option_full_display)? CURRENT_STRING:SHORT_CURR_STRING));
	}
	else if(fld->vtype == GUID_PREVIOUS)
	{
		(void) safe_strcpy(mbuf, XuGetLabel((GVG_option_full_display)? PREVIOUS_STRING:SHORT_PREV_STRING));
	}
	else
	{
		dt = fld->valid->times[fld->vsel];
		if(valid_tstamp(dt))
		{
			rt = (fld->rtype == GUID_DEPICT)? GV_T0_depict:RunTime(fld->run);
			if(GVG_option_full_display)
				snprintf(mbuf, sizeof(mbuf), "%s (%s)", TimeDiffFormat(rt, dt, SHOW_MINUTES(fld)), GuidFieldDateFormat(fld,dt));
			else
				(void) safe_strcpy(mbuf, TimeDiffFormat(rt, dt, SHOW_MINUTES(fld)));
			if(!matching_tstamps(dt, ActiveDepictionTime(FIELD_INDEPENDENT)))
			{
				color = caution_colour;
			}
		}
		else
			(void) safe_strcpy(mbuf, XuGetLabel(dt));
	}

	XbaeMatrixSetCell(selectMatrix, fld->ndx, VALID_COLUMN, mbuf);
	XbaeMatrixSetCellColor(selectMatrix, fld->ndx, VALID_COLUMN, color);
}



/* The currently active list will only be saved if the profile
 * "automatically save on exit" flag is on. nparms will only be
 * > 0 if this function is called from the notification function.
 */
static void save_active_list(String *parms, int nparms)
{
	if(nparms < 1)
	{
		Boolean update = False;
		if(XuGetProfileStateData(XuActiveProfile, &update, NULL) && update)
			XuVaStateDataSave(STATE_ID, "ln", "a", "%d", GVG_active_guidlist->ndx);
	}
	else
	{
		XuVaStateDataSave(STATE_ID, "ln", "a", "%d", GVG_active_guidlist->ndx);
	}
}


/*
*  Save the guidance lists in the state file. Note that the list count starts
*  as 1 as the first list is the temporary list and is never saved.
*/
static void save_lists(void)
{
	int     i, j, count;
	char    lbuf[16], fbuf[32];

	XuStateDataRemove(STATE_ID, "*", "*");

	for(count = 1, i = 1; i < GVG_nguidlist; i++)
	{
		if(GVG_guidlist[i]->fixed) continue;

		snprintf(lbuf, sizeof(lbuf), "%d", count);
		XuVaStateDataSave(STATE_ID, lbuf, NULL, "\"%s\" %d",
			GVG_guidlist[i]->label, GVG_guidlist[i]->nfield);
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			snprintf(lbuf, sizeof(lbuf), "%d", count);
			snprintf(fbuf, sizeof(fbuf), "%d", GVG_guidlist[i]->field[j]->ndx);
			XuVaStateDataSave(STATE_ID, lbuf, fbuf,
				"\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" %c %c",
				GVG_guidlist[i]->field[j]->info->element->name,
				GVG_guidlist[i]->field[j]->info->level->name,
				SrcName(GVG_guidlist[i]->field[j]->source),
				SrcSubName(GVG_guidlist[i]->field[j]->source),
				RunTime(GVG_guidlist[i]->field[j]->run),
				GVG_guidlist[i]->field[j]->rtype,
				GVG_guidlist[i]->field[j]->vtype);
		}
		count++;
	}
	XuVaStateDataSave(STATE_ID, "ln", NULL, "%d", count);
	save_active_list(NULL, 0);
}


/*  A general function for launching related dialogs.
*/
/*ARGSUSED*/
static void dialog_launch_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2INT(client_data))
	{
		case 1: ACTIVATE_guidanceListAddItemDialog(GVG_selectDialog, False); break;
		case 2: ACTIVATE_guidanceListAddItemDialog(GVG_selectDialog, True); break;
		case 3: ACTIVATE_guidanceFieldSelectDialog(GVG_selectDialog); break;
		case 4: ACTIVATE_guidanceFieldRemoveDialog(GVG_selectDialog); break;
		case 5: ACTIVATE_guidanceFieldAppearanceDialog(GVG_selectDialog); break;
	}
}



/*ARGSUSED*/
static void options_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2INT(client_data))
	{
	case 1:
		GVG_option_synchro = XmToggleButtonGetState(w);
		break;
	case 2:
		option_nearest = XmToggleButtonGetState(w);
		set_list_valid_times(GVG_active_guidlist, GVG_active_time);
		SetGuidanceDisplayState(True);
		break;
	case 3:
		GVG_option_full_display = XmToggleButtonGetState(w);
		update_field_display();
		ACTIVATE_guidanceLegendDialog(GVG_selectDialog);
		break;
	case 4:
		show_depictions = XmToggleButtonGetState(w);
		SetDepictionVisibility(show_depictions);
		break;
	}
	XuVaStateDataSave(STATE_ID, DISPLAY_OPTION, NULL, "%d %d %d %d %d",
		(int) GVG_option_full_display,
		(int) GVG_option_synchro,
		(int) option_nearest,
		(int) show_depictions);
}


/*  Delete a list of fields. This function must delete all data associated
*   with the list and turn off all fields currently showing.
*/
/*ARGSUSED*/
static void delete_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
    int    i, ndx;
	GuidlistStruct *afl;

    if(!GVG_active_guidlist || GVG_active_guidlist->ndx == 0) return;

    if(XuAskUser(w, "NoUndo", NULL) == XuNO) return;

    XuSetBusyCursor(ON);

	afl = GVG_active_guidlist;
	ndx = afl->ndx;

	/* Turn off any active fields in the list
	 */
    for(i = 0; i < afl->nfield; i++)
        set_field_for_show(afl->field[i], OFF);

	/* Reset the active list to the first one we have
	 */
    change_list(0);
	XuDelay(GVG_selectDialog, 100);

    /* Destroy the list and its' fields.
    */
    (void) IngredCommand(GE_GUIDANCE, "HIDE");
    for(i = 0; i < afl->nfield; i++)
    {
        if(!blank(afl->field[i]->id))
            (void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", afl->field[i]->id);
        RemoveRunTimeEntry(afl->field[i]->run);
        FreeItem(afl->field[i]);
    }
    FreeItem(afl->label);
    FreeItem(afl->field);
	FreeItem(afl);

	/* Compact the array
	*/
    GVG_nguidlist--;
    for(i = ndx; i < GVG_nguidlist; i++)
    {
        GVG_guidlist[i] = GVG_guidlist[i+1];
		GVG_guidlist[i]->ndx = i;
    }

	/* Regenerate the combobox list entries
	 */
	XuComboBoxDeleteAllItems(listCombo);
	if(GVG_nguidlist > 0)
	{
		String *list = NewStringArray(GVG_nguidlist);
		for(i = 0; i < GVG_nguidlist; i++) list[i] = GVG_guidlist[i]->label;
		XuComboBoxAddItems(listCombo, list, GVG_nguidlist, 0);
		XuComboBoxSelectPos(listCombo, 1, False);
		FreeItem(list);
	}

	SetGuidanceDisplayState(True);
    save_lists();
    XuSetBusyCursor(OFF);
}


/*
*  The appearance dialog can only be called if there is at least one field
*  showing just not selected to show.
*/
static void set_appearance_btn_state(void)
{
	int     i, n;
	Boolean state;

	if(IsNull(GVG_selectDialog)) return;

	for( i = 0, state = False; i < GVG_nguidlist && state == False; i++)
	{
		for( n = 0; n < GVG_guidlist[i]->nfield && state == False; n++)
		{
			state = GVG_guidlist[i]->field[n]->showing;
		}
	}
	XtSetSensitive(fieldAppBtn, state);
}


/*
*  Change the active list. Any fields which are not selected for showing
*  and which are registered in the old list are de-registered.
*/
static void change_list(int pos)
{
	int      i;
	Widget   w;

	if (!GVG_sample_tab_active) DeactivateMenu();
	XuSetBusyCursor(ON);

	for(i = 0; i < GVG_active_guidlist->nfield; i++)
	{
		if(GVG_active_guidlist->field[i]->showing) continue;
		if(blank(GVG_active_guidlist->field[i]->id)) continue;
		(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", GVG_active_guidlist->field[i]->id);
		GVG_active_guidlist->field[i]->id[0] = '\0';
	}
	GVG_active_guidlist = GVG_guidlist[pos];

	update_field_display();
	set_appearance_btn_state();
	make_sequence_btns();
	save_active_list(NULL, 0);

	w = XuMenuFindButton(menuBar,LIST_DEL_ID);
	XtSetSensitive(w, !GVG_active_guidlist->fixed && GVG_active_guidlist->ndx > 0);

	w = XuMenuFindButton(menuBar, FIELD_ADD_ID);
	XtSetSensitive(w, !GVG_active_guidlist->fixed);

	w = XuMenuFindButton(menuBar, FIELD_DEL_ID);
	XtSetSensitive(w, !GVG_active_guidlist->fixed);

	/* Set the list type display label to show what type of list we have.
	*/
	XuWidgetLabel(listTypeDisplay, XuGetLabel(GVG_active_guidlist->fixed ? "presetList":"userList"));

	if (GV_edit_mode) UpdateGuidanceSampleTab(True);

	XuSetBusyCursor(OFF);
	if (!GVG_sample_tab_active) ActivateMenu();
}


/*ARGSUSED*/
static void delayed_change_list_cb(XtPointer client_data , XtIntervalId *id )
{
	change_list(PTR2INT(client_data));
}


/*
*   Respond to request for a new list to be shown. The actual call to 
*   change lists is put into a time out to allow the combo box widget to
*   go away before the list is created to give a better "feel" to it.
*/
/*ARGSUSED*/
static void list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int posn;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if(rtn->item_position < 1) return;

	posn = rtn->item_position - 1;
	if(posn == GVG_active_guidlist->ndx || posn < 0 || posn >= GVG_nguidlist) return;
	(void) XtAppAddTimeOut(GV_app_context, 50, delayed_change_list_cb, INT2PTR(posn));
}


/*
*   Turn off all selected fields and deregister any field which is found to
*   be on but not in the active list.
*/
/*ARGSUSED*/
static void clear_lists_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j;
	GuidanceFieldStruct **flds;

	XbaeMatrixDisableRedisplay(selectMatrix);
	for(i = 0; i < GVG_nguidlist; i++)
	{
		flds = GVG_guidlist[i]->field;
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			if(!flds[j]->show) continue;
			set_field_for_show(flds[j], OFF);
			flds[j]->show = False;
			if(GVG_guidlist[i] == GVG_active_guidlist)
			{
				XbaeMatrixDeselectRow(selectMatrix, flds[j]->ndx);
			}
			else
			{
				(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", flds[j]->id);
				flds[j]->id[0] = '\0';
			}
		}
	}
	XbaeMatrixEnableRedisplay(selectMatrix, True);
	SetGuidanceDisplayState(True);
	UpdateAppearanceDialog();
	if (GV_edit_mode) UpdateGuidanceSampleTab(False);
}

/*  Refresh the guidance fields by de-registering all fields then registering
 *  them again. Those fields which are displaying are shown again.
 */
/*ARGSUSED*/
static void refresh_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j;
	GuidanceFieldStruct **fld;

	for(i = 0; i < GVG_nguidlist; i++)
	{
		fld = GVG_guidlist[i]->field;
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			if(blank(fld[j]->id)) continue;
			set_field_for_show(fld[j], OFF);
			(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld[j]->id);
			fld[j]->id[0] = '\0';
		}
	}
	for(i = 0; i < GVG_nguidlist; i++)
	{
		fld = GVG_guidlist[i]->field;
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			if(fld[j]->show) set_field_for_show(fld[j], ON);
		}
	}
	SetGuidanceDisplayState(True);
}



/* This allows mouse button selections done in the run time display
 * popup to select and deselect the appropriate field just like the
 * rest of the matrix cells.
 */
/*ARGSUSED*/
static void run_time_popup_cb(Widget w, XtPointer cd, XtPointer ld)
{
	int       row;
	XtPointer data;
	Boolean   show;
	GuidanceFieldStruct *fld;

	XtVaGetValues(w, XmNuserData, &data, NULL);
	row = PTR2INT(data);
	fld = GVG_active_guidlist->field[row];
	if( !fld->available ) return;

	if(XbaeMatrixIsRowSelected(selectMatrix, row))
	{
		show = False;
		XbaeMatrixDeselectRow(selectMatrix, row);
	}
	else
	{
		show = True;
		XbaeMatrixSelectRow(selectMatrix, row);
	}
	set_field_show_state(fld, show);
}


/* Check any fields that are not available to see if data has arrived since
 * last we looked.
 *
 * Ok, the following two routines are messy, but the idea is to check the
 * fields for availability in a work procedure so as to minimize the impact
 * on interface performance when many of the fields will not be available
 * when guidance first arrives. The time out procedure runs a work procedure
 * and checks for completion of the work procedure activities. The number of
 * times any given field is checked is limited by retry_count to ensure that
 * fields that do not become available are not checked every 5 seconds forever.
 * Also if the work proc does not finish in 30 seconds something has gone
 * wrong and we reset the count.
 */
static int retry_wp_count = 0;

/*ARGSUSED*/
static Boolean field_available_retry_wp( XtPointer client_data )
{
	GuidanceFieldStruct *fld;
	FLD_DESCRIPT fd;

	/* Being a work procedure we need to keep the index values around */
	static int i = 0;
	static int j = 0;
	static Boolean found_one = False;

	/* Initialize */
	if( i == 0 && j == 0 )
	{
		found_one = False;
		retry_wp_count = 1;
	}

	/* We have finished processing one of the lists */
	if(i < GVG_nguidlist && j >= GVG_guidlist[i]->nfield)
	{
		if (found_one)
		{
			update_list_fld_diff_flags(GVG_guidlist[i]);
			if(NotNull(GVG_selectDialog) && GVG_guidlist[i] == GVG_active_guidlist)
			{
				valid_popup_cb(NULL, NULL, NULL);
				update_field_display();
				make_sequence_btns();
			}
		}
		found_one = False;
		i++;
		j = 0;
		return False;
	}

	/* We have finished processing all of the lists */
	if( i >= GVG_nguidlist )
	{
		retry_wp_count = 0;
		i = 0;
		j = 0;
		return True;
	}

	/* From here on we process one of the fields */
	fld = GVG_guidlist[i]->field[j];

	if(!fld->available && fld->retry_count > 0)
	{
		copy_fld_descript(&fd, fld->source->fd);
		if(set_fld_descript(&fd,
					FpaF_ELEMENT, fld->info->element,
					FpaF_LEVEL,fld->info->level,
					FpaF_RUN_TIME, (blank(RunTime(fld->run)) && fld->rtype != GUID_DEPICT)? NO_RUN : RunTime(fld->run),
					FpaF_END_OF_LIST))
		{
			int    nvt;
			String *vt;
			
			nvt = FilteredValidTimeList(&fd, fld->info->element->elem_tdep->time_dep, &vt);
			if(nvt < 1)
			{
				/* Still not available */
				fld->retry_count--;
			}
			else
			{
				/* Data now available */
				found_one = True;
				fld->retry_count = 0;
				create_field_valid_list(fld, vt, nvt);
			}
			nvt = FilteredValidTimeListFree(&vt, nvt);
		}
		else
		{
			fld->retry_count = 0;
		}
	}
	j++;

	return False;
}



/* This function just launches the above function in a work procedure after checking
 * the state flags. The done flag is in case the work procedure is not finished before
 * this function runs again.
 */
/*ARGSUSED*/
static void field_available_retry( XtPointer client_data , XtIntervalId *id )
{
	static XtIntervalId last_id = (XtIntervalId) NULL;

	/* No id means that the function was called from one of our functions */
	if (!id)
	{
		unsigned long interval = RECHECK_INTERVAL * 1000;
		if (last_id) XtRemoveTimeOut(last_id);
		last_id = XtAppAddTimeOut(GV_app_context, interval, field_available_retry, NULL);
		return;
	}
	last_id = (XtIntervalId) NULL;

	/* Only launch the work procedure if it has finished prior processing */
	if (retry_wp_count == 0)
	{
		int i, j;
		unsigned long interval = RECHECK_INTERVAL * 1000;

		/* Launch again if fields are still not available */
		for(i = 0; i < GVG_nguidlist; i++)
		{
			for(j = 0; j < GVG_guidlist[i]->nfield; j++)
			{
				if(GVG_guidlist[i]->field[j]->available) continue;
				if(GVG_guidlist[i]->field[j]->retry_count < 1) continue;
				last_id = XtAppAddTimeOut(GV_app_context, interval, field_available_retry, NULL);
				(void) XtAppAddWorkProc(GV_app_context, field_available_retry_wp, NULL);
				return;
			}
		}
	}
	else if (retry_wp_count < 30)
	{
		/* Work proc not finished so call ourselves in one second */
		retry_wp_count++;
		last_id = XtAppAddTimeOut(GV_app_context, 1000, field_available_retry, NULL);
	}
	else
	{
		/* Something went wrong - reset */
		retry_wp_count = 0;
	}
}



/* This is the function run when a notification of a guidance source update
 * has been received. The form of notification used requires the function to
 * return False if it has not finished processing and must be called again
 * and True if it has finished processing. 
 */
/*ARGSUSED*/
static Boolean check_for_run_time_change(Boolean unused)
{
	/* Structure to hold temporary information about the fields */
	typedef struct {
		Boolean changed;    /* is this different from what is now in the field? */
		RunTimeEntry run;   /* run time */
		int     nvtime;     /* number of valid times */
		String  *vtime;     /* valid times */
		TSTAMP  old_rtime;	/* previous run time */
		TSTAMP  old_vtime;	/* previous selected valid time (if applicable) */
	} GuidFldTimeStruct;

	GuidanceFieldStruct *fld;
	FLD_DESCRIPT fd;

	/* Variables which must remember their state between function calls.
	 */
	static int i = 0, j = 0;
	static Boolean redisplay = False;
	static Boolean field_reselect = False;
	static Boolean retry = False;
	static GuidFldTimeStruct *newtimes = (GuidFldTimeStruct *)NULL;

	if(IsNull(GVG_guidlist)) return True;

	/* If we have more guidance lists to process but have finished processing all
	 * of the fields in the list being processed.
	 */
	if(j < GVG_nguidlist && i >= GVG_guidlist[j]->nfield)
	{
		/* Update all of the showing fields at once. The non-show fields have already
		 * been updated at this point.
		 */
		for(i = 0; i < GVG_guidlist[j]->nfield; i++)
		{
			/* Changed will only be true if we have new times and fld->show == True */
			if(!newtimes[i].changed) continue;

			fld = GVG_guidlist[j]->field[i];
			fld->show = False;
			field_reselect = True;
			set_field_show_state(fld, OFF);
			(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld->id);
			fld->id[0] = '\0';
			RemoveRunTimeEntry(fld->run);
			fld->run = newtimes[i].run;
			create_field_valid_list(fld, newtimes[i].vtime, newtimes[i].nvtime);
			(void) FilteredValidTimeListFree(&newtimes[i].vtime, newtimes[i].nvtime);
		}

		/* For any fields that have changed and were selected, search for another instance
		 * of the field that can replace it. Normally this will be an entry with the issue
		 * time of "previous" replacing an issue time of "current". If just the valid time
		 * list has changed then the same field will replace itself ;-)
		 */
		for(i = 0; i < GVG_guidlist[j]->nfield; i++)
		{
			int n;
			fld = GVG_guidlist[j]->field[i];
			/* Set the retry count flag if the field is not available */
			fld->retry_count = 0;
			if(!fld->available)
			{
				fld->retry_count = MAX_RECHECKS;
				retry = True;
			}
			if(!newtimes[i].changed) continue;
			for(n = 0; n < GVG_guidlist[j]->nfield; n++)
			{
				int posn;
				GuidanceFieldStruct *fl = GVG_guidlist[j]->field[n];
				if(fl->info->element != fld->info->element) continue;
				if(fl->info->level   != fld->info->level  ) continue;
				if(fl->source        != fld->source       ) continue;
				if(!matching_tstamps(RunTime(fl->run), newtimes[i].old_rtime)) continue;
				if(!InValidTimeEntry(newtimes[i].old_rtime, fl->valid, &posn)) continue;
				fl->vsel = posn;
				fl->show = True;
				set_field_show_state(fl, ON);
				break;
			}
			/* If a reselection of the item in the list is not possible then set a global
			 * flag to acknowledge this. It is used in ACTIVATE_processGuidanceUpdateDialog
			 */
			if( n >= GVG_guidlist[j]->nfield ) no_reselection = True;
		}

		update_list_fld_diff_flags(GVG_guidlist[j]);

		if(redisplay && NotNull(GVG_selectDialog) && GVG_guidlist[j] == GVG_active_guidlist)
		{
			valid_popup_cb(NULL, NULL, NULL);
			update_field_display();
			make_sequence_btns();
		}
		i = 0;
		j++;
		redisplay = False;
		FreeItem(newtimes);
		return False;
	}

	/* If we have finished processing all of the guidance lists
	 */
	if(j >= GVG_nguidlist)
	{
		/* Show the button on the main application button bar that flags the fact the a guidance
		 * field has been updated that the user is currently displaying.
		 */
		if (field_reselect)
		{
			Widget w = GetIconBarWidget(GUID_SELECT_STATUS_ICON);
			if (w) XtManageChild(w);
			SetGuidanceDisplayState(True);
		}
		/*
		 * If true this means that some of the fields have been flagged as missing. This is
		 * probably due to the timing of the guidance arrival so schedule a recheck,
		 */
		if (retry)
		{
			/* When inotify is used this function will be called within seconds of new
			 * guidance fields arriving so field_available_retry is not needed.
			*/
			if (!GV_inotify_process_used)
			{
				field_available_retry(NULL, 0);
			}
		}
		j = 0;
		field_reselect = False;
		retry = False;
		return True;
	}

	/* This section runs if there are more fields to be processed in the list.
	 */
	if (!newtimes) newtimes = NewMem(GuidFldTimeStruct, GVG_guidlist[j]->nfield);
	(void) memset((void *)&newtimes[i], 0, sizeof(GuidFldTimeStruct));

	fld = GVG_guidlist[j]->field[i];

	/* Show interest only if the source has been modified
	 */
	if(fld->source->modified)
	{
		int     nrun, nvt;
		char    rtime[16];
		String *rtimes, *vt;
		Boolean new_run = False;
		Boolean new_vtime = False;

		/* Check to see if the run time (issue time) has changed. This is for the special cases of
		 * current and previous issue times. We do not concern ourselves with absolute times.
		 */
		copy_fld_descript(&fd, fld->source->fd);
		if(set_fld_descript(&fd,FpaF_ELEMENT,fld->info->element,FpaF_LEVEL,fld->info->level,FpaF_END_OF_LIST))
		{
			switch(fld->rtype)
			{
				case GUID_CURRENT:
					nrun = source_run_time_list(&fd, &rtimes);
					new_run = (nrun < 1 || !matching_tstamps(RunTime(fld->run), rtimes[0]));
					(void) safe_strcpy(rtime, (nrun > 0)? rtimes[0]:NO_RUN);
					(void) source_run_time_list_free(&rtimes, nrun);
					break;
				case GUID_PREVIOUS:
					nrun = source_run_time_list(&fd, &rtimes);
					new_run = (nrun < 2 || !matching_tstamps(RunTime(fld->run), rtimes[1]));
					(void) safe_strcpy(rtime, (nrun > 1)? rtimes[1]:NO_RUN);
					(void) source_run_time_list_free(&rtimes, nrun);
					break;
				default:
					(void) safe_strcpy(rtime, blank(RunTime(fld->run))? NO_RUN:RunTime(fld->run));
					new_run = False;
			}

			(void) set_fld_descript(&fd, FpaF_RUN_TIME, rtime, FpaF_END_OF_LIST);
			nvt = FilteredValidTimeList(&fd, fld->info->element->elem_tdep->time_dep, &vt);

			/* Have things changed since we last were here? Check the valid times list
			 * count and if the same check the actual entries
			 */
			if(!new_run)
			{
				if(!(new_vtime = (fld->valid->ntimes != (nvt + ((fld->rtype == GUID_DEPICT)? 2:0)))))
				{
					int n;
					for(n = (fld->rtype == GUID_DEPICT)? 2:0; n < nvt; n++)
					{
						if(matching_tstamps(fld->valid->times[n], vt[n])) continue;
						new_vtime = True;
						break;
					}
				}
			}

			if(!new_run && !new_vtime)
			{
				nvt = FilteredValidTimeListFree(&vt, nvt);
			}
			else if(fld->show)
			{
				redisplay = True;
				newtimes[i].changed  = True;
				newtimes[i].run      = CreateRunTimeEntry(rtime);
				newtimes[i].nvtime   = nvt;
				newtimes[i].vtime    = vt;
				(void) safe_strcpy(newtimes[i].old_rtime, RunTime(fld->run));
				if(fld->vsel != GUID_NO_SEL)
					(void) safe_strcpy(newtimes[i].old_vtime, fld->valid->times[fld->vsel]);
			}
			else
			{
				redisplay = True;
				if(!blank(fld->id))
				{
					(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld->id);
					fld->id[0] = '\0';
				}
				RemoveRunTimeEntry(fld->run);
				fld->run = CreateRunTimeEntry(rtime);
				create_field_valid_list(fld, vt, nvt);
				nvt = FilteredValidTimeListFree(&vt, nvt);
			}
		}
	}
	i++;
	return False;
}


/*
*  This function will change all "previous" field selections to "current"
*  selections.
*/
/*ARGSUSED*/
static void update_select_to_current_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j, n;

	for(j = 0; j < GVG_nguidlist; j++)
	{
		for(i = 0; i < GVG_guidlist[j]->nfield; i++)
		{
			GuidanceFieldStruct *fld = GVG_guidlist[j]->field[i];
			if(!fld->show || fld->rtype != GUID_PREVIOUS) continue;

			fld->show = False;
			set_field_for_show(fld, OFF);
			(void) IngredVaCommand(GE_GUIDANCE, "FIELD_DEREGISTER %s", fld->id);
			fld->id[0] = '\0';

			for(n = 0; n < GVG_guidlist[j]->nfield; n++)
			{
				int posn;
				GuidanceFieldStruct *fl = GVG_guidlist[j]->field[n];
				if(fl->rtype         != GUID_CURRENT      ) continue;
				if(fl->info->element != fld->info->element) continue;
				if(fl->info->level   != fld->info->level  ) continue;
				if(fl->source        != fld->source       ) continue;
				if(InValidTimeEntry(fld->valid->times[fld->vsel], fl->valid, &posn))
					fl->vsel = posn;
				fl->show = True;
				set_field_show_state(fl, ON);
				break;
			}
		}

		update_list_fld_diff_flags(GVG_guidlist[j]);

		if(NotNull(GVG_selectDialog) && GVG_guidlist[j] == GVG_active_guidlist)
		{
			valid_popup_cb(NULL, NULL, NULL);
			update_field_display();
			make_sequence_btns();
		}
	}
}


/*
 *  Force a check of the guidance directories for changes. The checking is done in a
 *  work procedure to miminize any interface 'stickness' fell.
 */
/*ARGSUSED*/
static void force_availability_check(Widget w , XtPointer client_data , XtPointer call_data )
{
	(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc) check_for_run_time_change, NULL);
}


/*
*  Update the "diff" variable in the field structure. This variable is True
*  if the source or sub-source or time_dep or run time are different from
*  the field previous in the list.
*/
static void update_list_fld_diff_flags(GuidlistStruct *l )
{
	int i;
	FpaCtimeDepTypeOption last_tdep;

	if(l->nfield < 1) return;

	l->field[0]->diff = True;
	last_tdep = l->field[0]->info->element->elem_tdep->time_dep;

	for(i = 1; i < l->nfield; i++)
	{
		l->field[i]->diff = (	l->field[i]->source != l->field[i-1]->source ||
								l->field[i]->run != l->field[i-1]->run       ||
								l->field[i]->info->element->elem_tdep->time_dep != last_tdep);
		last_tdep = l->field[i]->info->element->elem_tdep->time_dep;
	}
}


/*
*  Update the list selector and set the active list to the
*  list given by the list label.
*/
static void update_list(GuidlistStruct *l )
{
	int i;
	String *list;

	list = NewStringArray(GVG_nguidlist);
	for(i = 0; i < GVG_nguidlist; i++)
	{
		list[i] = GVG_guidlist[i]->label;
	}
	XuComboBoxDeleteAllItems(listCombo);
	XuComboBoxAddItems(listCombo, list, GVG_nguidlist, 0);
	XuComboBoxSelectItem(listCombo, l->label, False);
	change_list(l->ndx);
	FreeItem(list);
}


/*
*  Callback function for the guidance sequence buttons. In this case only
*  fields that are an exact match to the sequence time are displayed.
*/
/*ARGSUSED*/
static void SequenceCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j, posn;
	GuidanceFieldStruct *fld;

	if(!XmToggleButtonGetState(w)) return;

	(void) safe_strcpy(GVG_active_time, GVG_valid_seq[PTR2INT(client_data)]);

	XbaeMatrixDisableRedisplay(selectMatrix);
	for(i = 0; i < GVG_nguidlist; i++)
	{
		for(j = 0; j < GVG_guidlist[i]->nfield; j++)
		{
			fld = GVG_guidlist[i]->field[j];
			set_field_for_show(fld, OFF);
			if(InValidTimeEntry(GVG_active_time, fld->valid, &posn))
				fld->vsel = posn;
			else
				fld->vsel = GUID_NO_SEL;
			set_field_for_show(fld, ON);
			fill_valid_time(fld);
		}
	}
	SetGuidanceDisplayState(True);
	UpdateAppearanceDialog();
	if (GV_edit_mode) UpdateGuidanceSampleTab(False);
	XbaeMatrixResizeColumnsToCells(selectMatrix, True);
	XbaeMatrixEnableRedisplay(selectMatrix, True);
}


/*
*  make up the sequence buttons which will allow the user to
*  toggle through the guidance fields.  Merge all of the valid time
*  sequences and make sure that the merged sequence is in time order.
*/
static void make_sequence_btns(void)
{
	int i, j, k, n;
	GuidanceFieldStruct *fld;

	GVG_nvalid_seq = 0;
	FreeItem(GVG_valid_seq);
	for(n = 0; n < GVG_nguidlist; n++)
	{
		for(i = 0; i < GVG_guidlist[n]->nfield; i++)
		{
			fld = GVG_guidlist[n]->field[i];
			if(!fld->show) continue;
			for(j = 0; j < fld->valid->ntimes; j++)
			{
				if(!valid_tstamp(fld->valid->times[j])) continue;
				if(InTimeList(fld->valid->times[j], GVG_valid_seq, GVG_nvalid_seq, NULL)) continue;
				GVG_valid_seq = MoreStringArray(GVG_valid_seq, GVG_nvalid_seq+1);
				for(k = GVG_nvalid_seq; k > 0; k--)
				{
					if(strcmp(fld->valid->times[j], GVG_valid_seq[k-1]) >= 0) break;
					GVG_valid_seq[k] = GVG_valid_seq[k-1];
				}
				GVG_valid_seq[k] = fld->valid->times[j];
				GVG_nvalid_seq++;
			}
		}
	}
	/* Now create the buttons.
	*/
	if(GVG_nvalid_seq > GVG_nbtnBarBtns)
	{
		GVG_btnBarBtns = MoreWidgetArray(GVG_btnBarBtns, GVG_nvalid_seq);
		for(i = GVG_nbtnBarBtns; i < GVG_nvalid_seq; i++)
		{
			GVG_btnBarBtns[i] = XmVaCreateToggleButton(btnBarRC, "btn",
				XmNmarginWidth, 5,
				XmNmarginHeight, 5,
				XmNshadowThickness, 2,
				XmNindicatorOn, False,
				NULL);
			XtAddCallback(GVG_btnBarBtns[i], XmNvalueChangedCallback, SequenceCB, INT2PTR(i));
		}
		GVG_nbtnBarBtns = GVG_nvalid_seq;
	}
	label_sequence_btns();
}


static void label_sequence_btns(void)
{
	int       i;
	Dimension hm = 0, sh, mh, st, sbh, tm;
	Widget    w;

	/* BUG 20080625.0: This unmanage and the ending manage call were just around
	 * the fuction call above and not in the other call when depictions changed.
	 * This resulted in some very slow updating when depictions did change.
	 */
	XtUnmanageChildren(GVG_btnBarBtns, GVG_nbtnBarBtns);

	for(i = 0; i < GVG_nvalid_seq; i++)
	{
		SetSequenceBtnTime(GVG_btnBarBtns[i], GVG_valid_seq[i], SPECIAL_SEQUENCE);
		XtVaSetValues(GVG_btnBarBtns[i],
			XmNset, matching_tstamps(GVG_active_time, GVG_valid_seq[i]),
			NULL);
	}

	/* Set the height of the button list scrolled window to accomodate
	 * the buttons and no more.
	 */
	if(GVG_nvalid_seq > 0)
	{
		XtVaGetValues(btnBarRC, XmNmarginHeight, &tm, NULL);
		XtVaGetValues(GVG_btnBarBtns[0], XmNheight, &hm, NULL);
		XtVaGetValues(btnBarSW,
			XmNhorizontalScrollBar, &w,
			XmNspacing, &sh,
			XmNscrolledWindowMarginHeight, &mh,
			XmNshadowThickness, &st,
			NULL);
		XtVaGetValues(w, XmNheight, &sbh, NULL);		
		XtVaSetValues(btnBarSW, XmNheight, hm+sh+sbh+mh+mh+st+st+tm+tm, NULL);
	}

	XtManageChildren(GVG_btnBarBtns, GVG_nvalid_seq);
	XuUpdateDisplay(GVG_selectDialog);
}


/*
*	Create the list of valid times for the given field. If a list of valid
*	times is given as the parameter valid_list then we use this list. If
*	not given then an internal call is made to obtain the list.
*/
static void create_field_valid_list(GuidanceFieldStruct *fld , String *valid_list , int nvalid_list )
{
	int nvt, posn;
	TSTAMP cur_vtime;
	String *vt, *t;
	FLD_DESCRIPT fd;

	/* If the field has an existing valid list we want to retain the existing
	*  time selection.
	*/
	if(fld->vsel == GUID_NO_SEL)
		(void) safe_strcpy(cur_vtime,  GVG_active_time);
	else
		(void) safe_strcpy(cur_vtime, fld->valid->times[fld->vsel]);

	/* Remove the connection to the valid list and set it to the null list.
	*/
	RemoveValidTimeEntry(fld->valid);
	fld->valid = CreateEmptyValidTimeEntry();
	fld->vsel  = GUID_NO_SEL;

	nvt = 0;
	vt  = (String *)NULL;
	if(valid_list)
	{
		nvt = nvalid_list;
		vt  = valid_list;
	}
	else
	{
		copy_fld_descript(&fd, fld->source->fd);
		if(set_fld_descript(&fd,
			FpaF_ELEMENT, fld->info->element,
			FpaF_LEVEL, fld->info->level,
			FpaF_RUN_TIME, (blank(RunTime(fld->run)) && fld->rtype != GUID_DEPICT)? NO_RUN : RunTime(fld->run),
			FpaF_END_OF_LIST))
		{
			nvt = FilteredValidTimeList(&fd, fld->info->element->elem_tdep->time_dep, &vt);
		}
	}

	if((fld->available = (nvt > 0)))
	{
		/* Assign a valid time list entry.
		*/
		if(fld->rtype == GUID_DEPICT)
		{
			t = NewStringArray(nvt+2);
			t[0] = CURRENT_STRING;
			t[1] = PREVIOUS_STRING;
			CopyStruct(&t[2], vt, String, nvt);
			fld->valid = CreateValidTimeEntry(t, nvt+2);
			FreeItem(t);
			fld->vsel = 0;
			if(fld->vtype != GUID_CURRENT && fld->vtype != GUID_PREVIOUS)
			{
				if(InValidTimeEntry(cur_vtime, fld->valid, &posn))
					fld->vsel = posn;
				if(fld->vsel == 0) fld->vtype = GUID_CURRENT;
				if(fld->vsel == 1) fld->vtype = GUID_PREVIOUS;
			}
		}
		else
		{
			fld->vtype = GUID_ABSOLUTE;
			fld->valid = CreateValidTimeEntry(vt, nvt);
			if(InValidTimeEntry(cur_vtime, fld->valid, &posn))
				fld->vsel = posn;
		}
	}
	else
	{
		fld->show = False;
	}

	if (!valid_list) FilteredValidTimeListFree(&vt, nvt);
}


/*
*  Update the field selection widget to correspond to the active list.
*/
static void update_field_display(void)
{
	int i;
	char mbuf[128];
	String *rtimes;
	Pixel colour, bkgnd, bgnd[1];
	GuidanceFieldStruct *fld;
	Widget w;

	int nrtime = 0;
	Source source = NULL;
	Boolean show_rtime = False;

	if(IsNull(GVG_selectDialog)) return;

	XuSetBusyCursor(ON);
	XuUpdateDisplay(GVG_selectDialog);
	XbaeMatrixDisableRedisplay(selectMatrix);

	bkgnd = XuLoadColor(selectMatrix, XmNbackground);

	XtUnmanageChildren(arrows, narrows);

	if(GVG_active_guidlist->nfield > narrows)
	{
		arrows = MoreWidgetArray(arrows, GVG_active_guidlist->nfield);
		for(i = narrows; i < GVG_active_guidlist->nfield; i++)
		{
			arrows[i] = XtVaCreateWidget("va", xmArrowButtonWidgetClass, selectMatrix,
				XmNarrowDirection, XmARROW_DOWN,
				XmNborderWidth, 0,
				XmNshadowThickness, 0,
				NULL);
			XtAddCallback(arrows[i], XmNactivateCallback, show_valid_popup_cb, INT2PTR(i));
		}
		narrows = GVG_active_guidlist->nfield;
	}

	if(GVG_active_guidlist->nfield > num_matrix_rows)
		XbaeMatrixAddRows(selectMatrix, num_matrix_rows, NULL, NULL, NULL, GVG_active_guidlist->nfield - num_matrix_rows);
	else if(GVG_active_guidlist->nfield < num_matrix_rows)
		XbaeMatrixDeleteRows(selectMatrix, GVG_active_guidlist->nfield, num_matrix_rows - GVG_active_guidlist->nfield);
	num_matrix_rows = GVG_active_guidlist->nfield;

	for(i = 0; i < GVG_active_guidlist->nfield; i++)
	{
		fld = GVG_active_guidlist->field[i];

		show_rtime = True;
		switch(fld->rtype)
		{
			case GUID_CURRENT:
				(void) safe_strcpy(mbuf, XuGetLabel("current"));
				break;
			case GUID_PREVIOUS:
				(void) safe_strcpy(mbuf, XuGetLabel(PREVIOUS_STRING));
				break;
			case GUID_ABSOLUTE:
				(void) safe_strcpy(mbuf, DateString(RunTime(fld->run), HOURS));
				break;
			default:
				(void) safe_strcpy(mbuf, XuGetLabel("na-short"));
				show_rtime = False;
				break;
		}

		colour = not_available_colour;
		if(show_rtime && fld->available)
		{
			if(fld->source != source)
			{
				source = fld->source;
				if(nrtime > 0) source_run_time_list_free(&rtimes, nrtime);
				nrtime = source_run_time_list(fld->source->fd, &rtimes);
			}
			if(InTimeList(RunTime(fld->run), rtimes, nrtime, NULL))
			{
				switch(fld->rtype)
				{
					case GUID_CURRENT:
						if(!matching_tstamps(RunTime(GVG_active_guidlist->field[i]->run), rtimes[0]))
							colour = caution_colour;
						break;
					case GUID_PREVIOUS:
						if(!matching_tstamps(RunTime(GVG_active_guidlist->field[i]->run), rtimes[1]))
							colour = caution_colour;
						break;
					default:
						colour = run_ok_colour;
						break;
				}
			}
		}

		fill_valid_time(fld);

		XbaeMatrixSetCell(selectMatrix, i, LEVEL_COLUMN, (GVG_option_full_display)?
				fld->info->level->label : fld->info->level->sh_label);
		XbaeMatrixSetCell(selectMatrix, i, FIELD_COLUMN, (GVG_option_full_display)?
				fld->info->element->label : fld->info->element->sh_label);
		XbaeMatrixSetCell(selectMatrix, i, SOURCE_COLUMN, SrcShortLabel(fld->source));
		XbaeMatrixSetCell(selectMatrix, i, ISSUE_COLUMN, mbuf);
		XbaeMatrixSetCellColor(selectMatrix, i, ISSUE_COLUMN, colour);

		XtManageChild(arrows[i]);
		/* Reset background in case the arrow was last in field not available colour */
		XtVaSetValues(arrows[i], XmNbackground, bkgnd, NULL);
		XtSetSensitive(arrows[i], fld->available);
		XbaeMatrixSetCellWidget(selectMatrix, i, ARROW_COLUMN, arrows[i]);

		if(fld->available)
			XtVaGetValues(GVG_selectDialog, XmNbackground, &bgnd[0], NULL);
		else
			bgnd[0] = field_missing_colour;
		XbaeMatrixSetRowBackgrounds(selectMatrix, i, bgnd, 1);
		XtVaSetValues(arrows[i], XmNbackground, bgnd[0], NULL);

		if(fld->show) XbaeMatrixSelectRow(selectMatrix, i);
		else          XbaeMatrixDeselectRow(selectMatrix, i);
	}
	XbaeMatrixResizeColumnsToCells(selectMatrix, True);

	if(nrtime > 0) source_run_time_list_free(&rtimes, nrtime);

	/* If there are no fields we set the delete pulldown insensitive.
	*/
	w = XuMenuFindButton(menuBar, FIELD_DEL_ID);
	if(NotNull(w)) XtSetSensitive(w, (Boolean)(GVG_active_guidlist->nfield > 0));

	XbaeMatrixEnableRedisplay(selectMatrix, True);
	XuSetBusyCursor(OFF);
}


/*==================== Valid Time Popup Window ===============================*/

static Widget popup_call_btn = NullWidget;
static GuidanceFieldStruct *popup_fld = NULL;

/*ARGSUSED*/
static void show_valid_popup_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, x, y, height;
	Position wx, wy;
	char mbuf[256];
	Byte arrow_direction;
	String dt, rt;
	XmString label;
	Widget clipWindow;
	int ndx = PTR2INT(client_data);


	if(NotNull(popup_call_btn) && popup_call_btn != w)
		XtVaSetValues(popup_call_btn, XmNarrowDirection, XmARROW_DOWN, NULL);

	/* Check the arrow direction to get the current state.
	*/
	XtVaGetValues(w, XmNarrowDirection, &arrow_direction, NULL);
	if(arrow_direction == XmARROW_UP)
	{
		popup_call_btn = NullWidget;
		XtVaSetValues(w, XmNarrowDirection, XmARROW_DOWN, NULL);
		XtPopdown(validPopup);
	}
	else
	{
		popup_call_btn = w;
		popup_fld = GVG_active_guidlist->field[ndx];
		XtVaSetValues(w, XmNarrowDirection, XmARROW_UP, NULL);

		/* Update the list to reflect the appropriate field valid times.
		*/
		XmListDeleteAllItems(validSelect);
		for(i = 0; i < popup_fld->valid->ntimes; i++)
		{
			dt = popup_fld->valid->times[i];
			if(valid_tstamp(dt))
			{
				rt = (popup_fld->rtype == GUID_DEPICT)?
					GV_T0_depict:RunTime(popup_fld->run);
				snprintf(mbuf, sizeof(mbuf), "%s (%s)", TimeDiffFormat(rt,dt, SHOW_MINUTES(popup_fld)),
						GuidFieldDateFormat(popup_fld,dt));
			}
			else
				(void) safe_strcpy(mbuf, XuGetLabel(dt));
			label = XuNewXmString(mbuf);
			XmListAddItem(validSelect, label, 0);
			XmStringFree(label);
		}

		/* Position dialog just under the appropriate valid time display.
		*/
		XtVaGetValues(selectMatrix, XmNclipWindow, &clipWindow, NULL);
		XtTranslateCoords(clipWindow, 0, 0, &wx, &wy);

		height = XbaeMatrixGetRowHeight(selectMatrix, ndx);
		(void) XbaeMatrixRowColToXY(selectMatrix, ndx, VALID_COLUMN, &x, &y);

		XtVaSetValues(validPopup, XmNx, wx + (Position) x, XmNy, wy + (Position) y + (Position) height, NULL);
		XtPopup(validPopup, XtGrabNone);
		XuSetDialogCursor(validPopup, XuDEFAULT_CURSOR, ON);
	}
}



/*ARGSUSED*/
static void valid_popup_cb(Widget w , XtPointer client_data , XEvent *event)
{
	XtPopdown(validPopup);
	if(popup_call_btn)
		XtVaSetValues(popup_call_btn, XmNarrowDirection, XmARROW_DOWN, NULL);
}


/*
*   Responds to selections from the validSelect list of the validPopup.
*/
/*ARGSUSED*/
static void valid_time_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, count;
	char lbuf[32], fbuf[32];
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	set_field_for_show(popup_fld, OFF);
	popup_fld->vsel = rtn->item_position - 1;
	if(popup_fld->rtype == GUID_DEPICT)
	{
		switch(popup_fld->vsel)
		{
			case 0:  popup_fld->vtype = GUID_CURRENT;  break;
			case 1:  popup_fld->vtype = GUID_PREVIOUS; break;
			default: popup_fld->vtype = GUID_ABSOLUTE; break;
		}
	}
	else
	{
		popup_fld->vtype = GUID_ABSOLUTE;
	}

	/* Store the field into the state store file.
	*/
	if(popup_fld->list->ndx > 0 && !popup_fld->list->fixed)
	{
		count = 1;
		for(i = 1; i < popup_fld->list->ndx; i++)
		{
			if(!GVG_guidlist[i]->fixed) count++;
		}
		snprintf(lbuf, sizeof(lbuf), "%d", count);
		snprintf(fbuf, sizeof(fbuf), "%d", popup_fld->ndx);
		XuVaStateDataSave(STATE_ID, lbuf, fbuf,
			"\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" %c %c",
			popup_fld->info->element->name,
			popup_fld->info->level->name,
			SrcName(popup_fld->source),
			SrcSubName(popup_fld->source),
			RunTime(popup_fld->run),
			popup_fld->rtype,
			popup_fld->vtype);
	}

	fill_valid_time(popup_fld);
	set_field_for_show(popup_fld, ON);
	SetGuidanceDisplayState(True);
	XtPopdown(validPopup);
	if(popup_call_btn)
		XtVaSetValues(popup_call_btn, XmNarrowDirection, XmARROW_DOWN, NULL);
	XbaeMatrixResizeColumnsToCells(selectMatrix, True);
	UpdateAppearanceDialog();
	if (GV_edit_mode) UpdateGuidanceSampleTab(False);
}


/*
*  Here we must make sure we free all of the allocated memory to avoid any
*  leaks! If the depictions are hidden we now force them to show.
*/
/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if (!GVG_selectDialog) return;

	if (GV_edit_mode) DeactivateGuidanceSampleTab();
	if (!show_depictions) SetDepictionVisibility(ON);

	FreeItem(GVG_btnBarBtns);
	FreeItem(GVG_valid_seq);
	FreeItem(arrows);

	XtDestroyWidget(validPopup);
	XuDestroyDialog(GVG_selectDialog);

	narrows         = 0;
	GVG_nbtnBarBtns = 0;
	GVG_nvalid_seq  = 0;

	popup_call_btn = NullWidget;
	GVG_selectDialog = NullWidget;
	selectMatrix = NullWidget;
}
