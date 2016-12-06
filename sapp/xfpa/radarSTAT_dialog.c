/*=========================================================================*/
/*
*      File: radarStatDialog.c
*
*   Purpose:
*
*      To display radar statistics tables (also known as SCIT tables).
*
*      As part of the display individual elements are flagged if a value is
*      forecast to reach the severe level in the next 30 minutes. This is
*      done using a least square line fit to a storm at 3 data points.
*      This means that the storm will need to have existed for at least 30
*      minutes before the trend estimate is done.
*
*      All elements are colour coded according to the severity level of the
*      element. The colours are set in the config file.
*
*      If a column header is selected, the rows will be sorted according to
*      the values in the column. The order of the sorting is controlled through
*      and entry in the config file.
*
*      If a storm identifier is selected a dialog showing the trend of the
*      storm rank weight and of all of the elements that make up the weight
*      is shown.
*
*   Notes:
*
*      1. In order to operate this dialog requires two things to be
*         defined in the setup file. 
*         a) In the directories block under the key word "radar_stat"
*            the location of the directory where the STAT (SCIT) files
*            are to be found.
*         b) In the config_files block under the key word "radar_stat"
*            the name of the configuration file to be read by this dialog.
*
*       2. There is a simple file backup system implemented using an xml
*          property for the element node that will recover the state of
*          the file before any edits were done. All changes are undone.
*
*       3. Elements for a particular storm can be turned off. The indicator
*          for this is stored as an xml property for the element node. This
*          should be backwards compatable with other software that uses the
*          tables as they will not be looking for node properties.
*
*       4. Like everything there is a special case. If a value has the '/'
*          symbol in it the assumption is made that it is a wind and the
*          value after the '/' is to be used for sorting and thresholding.
*
*       5. This dialog runs in conjunction with the program rankweightDaemon.
*          If new files arrive in the stat directory this program will not
*          propogate any turned off element states to the new files. It is
*          expected that the daemon program rankweightDaemon will be
*          runnning and will do this as well as create forecast files.
*
*------------------------------------------------------------------------------
*
*     Version 8 (c) Copyright 2013 Environment Canada
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
*
****************************************************************************/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "global.h"
#include <Xm/ArrowB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xbae/Matrix.h>
#include "menu.h"
#include "depiction.h"
#include "observer.h"
#include "source.h"
#include "radarSTAT.h"


#define DEFAULT_ORDER	0
#define KEEP_ORDER		-2
#define REDISPLAY		-10
#define STATE_SAVE_KEYS	"radarStat","elem","view"
#define pgm				"radarStatDialog"


/* external public variables */
extern STATCFG	statcfg;
extern String   stat_data_dir;	/* stat file data directory */
extern VIEWS	*elem_view;		/* The list of element views as obtained from config file */
extern int		nelem_view;		/* number of views */
extern ELEMCFG	**elemp;		/* pointer to active elem_view item */
extern int		nelemp;			/* number of elemp in active item */
extern STORM   *storm_list;		/* storm information array */
extern int     storm_list_len;	/* number of storms in list */


/* Private widgets */
static Widget  dialog = NullWidget;
static Widget  statDisplay = NullWidget;
static Widget  selectedDisplay = NullWidget;
static Widget  userMessage = NullWidget;
static Widget  environSelectPopup = NullWidget;
static Widget  environSelectList = NullWidget;
static Widget  menuBar;
static Widget  arrowBack, arrowForward;
static Widget  timeLabel, saveBtn, recoverBtn, undoBtn;

/* Private variables */
static xmlDocPtr docp = NULL;			/* pointer to xml document */
static String    current_fname = NULL;	/* file name of currently displayed stat data. NULL if none */
static time_t    current_mod_time = 0;	/* last mod time of current file */
static Boolean   current_is_fcst = 0;	/* Is current file a forecast file? */
static time_t    requested_time = 0;	/* requested time to display stat data for */
static String    *filelist = NULL;		/* the current list of files in STAT directory */
static int       nfilelist = 0;			/* number of files in the list */
static int       sort_on_column = -1;	/* which column to sort storm order by */
static int       selected_storm_num = -1;/* which storm is selected for element trend graphing */
static XPoint    selected_row_column;	/* used for passing coordinates when needed for callbacks */       

/* Forward function definitions */
static void    activate_storm_trend_dialog(int);
static void    arrow_cb (Widget, XtPointer, XtPointer);
static void    clear_cell_threshold_color (int, int);
static Boolean data_dir_change_observer (Boolean);
static void    depiction_change_observer (String*, int);
static void    display_all_storms (void);
static int     matrix_column_from_element_array_pos (int);
static int     matrix_column_to_element_array_pos (int);
static void    double_click_cb (Widget, XtPointer, XtPointer);
static void    element_view_cb (Widget, XtPointer, XtPointer);
static void    enter_cell_cb (Widget, XtPointer, XtPointer);
static void    environ_select_cb (Widget, XtPointer, XtPointer);
static void    environ_select_popup_cb (Widget, XtPointer, XEvent*);
static void    exit_cb (Widget, XtPointer, XtPointer);
static void    free_doc (void);
static int     get_matrix_column_from_id (String);
static void    leave_cell_cb (Widget, XtPointer, XtPointer);
static void    modify_verify_cb (Widget, XtPointer, XtPointer);
static void    populate_selected_storm_display (void);
static String  print_value (STORM*, int);
static Boolean read_stat_data (String);
static void    read_stat_directory (void);
static void    recover_file_cb (Widget, XtPointer, XtPointer);
static void    redisplay_storm (STORM*);
static void    resize_selected_storm_display (void);
static void    save_data_check (void);
static void    save_to_file_cb (Widget, XtPointer, XtPointer);
static void    select_cell_cb (Widget, XtPointer, XtPointer);
static void    select_label_cb (Widget, XtPointer, XtPointer);
static void    set_cell_highlight(Widget, STORM*, int, int);
static void    set_cell_threshold_color (int, int);
static void    set_element_order (int);
static void    set_saveBtn_state (void);
static void    set_table_selection_arrows (void);
static void    undo_file_cb (Widget, XtPointer, XtPointer);
static void    update_filelist (void);
static void    update_xml_doc (void);


/**************** Public Functions ****************/


/* Read the radar statistics system configuration file and add an
 * observer to react to changes in the statistics source directory.
 */
void InitRadarStatSystem(void)
{
	if(rs_read_config(GW_mainWindow))
	{
		AddSourceObserver(data_dir_change_observer,"NewRadarStat");
	}
}


/* Is the radar statistics system initialized.
 */
Boolean RadarStatSystemInitialized(void)
{
	return (statcfg.element != NULL);
}


void ACTIVATE_radarStatDialog(Widget parent)
{
	int    i, num, elem_view_ndx;
	String buf, fname;
	XmString *table;
	XtTranslations translations;
	Widget w, textField, mainForm, selectForm;

	/* Translation override to allow the 'right' button
	 * to accept the current edit (or cancel if nothing
	 * has been done).
	 */
	static String translation_string = {
		"#override \n\
		 <Btn3Down> : SelectCell(cell)"
	};

	static XuMenuItemStruct menu_list[] = {
		{"undoBtn",&xmPushButtonWidgetClass, XuMENU_NONE, None, NoId, undo_file_cb, NULL, NULL},
		{"recover",&xmPushButtonWidgetClass, XuMENU_NONE, None, NoId, recover_file_cb, NULL, NULL},
		{"sep",    &xmSeparatorWidgetClass,  XuMENU_NONE, None, NoId, NULL, 0, NULL},
		{"exit",   &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId, exit_cb, NULL, NULL},
		NULL
	};

	static XuMenuItemStruct view_list[] = {
		NULL
	};

	static XuMenuBarItemStruct menu_bar[] = {
		{"actions", None, NoId, menu_list},
		{"viewBtn", None, NoId, view_list},
		NULL
	};

	if(NotNull(dialog))
	{
		XuShowDialog(dialog);
		return;
	}
	else if(!rs_validate_config())
	{
		return;
	}

    dialog = XuCreateMainWindowDialog(GW_mainWindow, "radarStatDialog",
		XuNallowIconify, True,
		XuNmwmDeleteOverride, exit_cb,
        NULL);

	menuBar = XuMenuBuildMenuBar(dialog, "menuBar", menu_bar);
	recoverBtn = XuMenuFindButtonByName(menuBar, "recover");
	undoBtn = XuMenuFindButtonByName(menuBar, "undoBtn");
	XtSetSensitive(recoverBtn, False);
	XtSetSensitive(undoBtn, False);

	/* Add the element views found in the config file to the viewBtn pulldown.
	 * First find which one, if any, was the active view last time.
	 */
	XuStateDataGet(STATE_SAVE_KEYS, &buf);
	for(elem_view_ndx = 0, i = 0; i < nelem_view; i++)
	{
		if(!same(buf,elem_view[i].label)) continue;
		elem_view_ndx = i;
		break;
	}
	FreeItem(buf);
	/*
	 * Make the toggle button radio list of view selections
	 */
	XuMenuMakeToggle(XuMenuFindButtonByName(menuBar, "viewBtn"));
	w = XuMenuFindByName(menuBar, "viewBtn");
	for(i = 0; i < nelem_view; i++)
	{
		Widget tb = XmVaCreateManagedToggleButton(w, elem_view[i].label,
				 XmNset, (i == elem_view_ndx),
				 NULL);
		XtAddCallback(tb, XmNvalueChangedCallback, element_view_cb, INT2PTR(i));
	}

	mainForm = XmVaCreateForm(dialog, "mainForm",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	selectForm = XmVaCreateForm(mainForm, "selectForm",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, mainForm,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	
	arrowBack = XmVaCreateManagedArrowButton(selectForm, "arrowBack",
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 1,
		XmNshadowThickness, 0,
		XmNwidth, 20,
		XmNarrowDirection, XmARROW_LEFT,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
    XtAddCallback(arrowBack, XmNactivateCallback, arrow_cb, INT2PTR(-1));

	timeLabel = XmVaCreateManagedLabel(selectForm, "timeLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, arrowBack,
		NULL);
	
	arrowForward = XmVaCreateManagedArrowButton(selectForm, "arrowForward",
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 1,
		XmNshadowThickness, 0,
		XmNwidth, 20,
		XmNarrowDirection, XmARROW_RIGHT,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, timeLabel,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
    XtAddCallback(arrowForward, XmNactivateCallback, arrow_cb, INT2PTR(1));

	saveBtn = XmVaCreateManagedPushButton(selectForm, "saveFile",
		XmNsensitive, False,
		XmNmarginWidth, 7,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, arrowForward,
		XmNleftOffset, 15,
		NULL);
	XtAddCallback(saveBtn, XmNactivateCallback, save_to_file_cb, NULL);

	XtManageChild(selectForm);

	userMessage = XmVaCreateLabel(mainForm, "userMessage",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, selectForm,
		XmNleftOffset, 20,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 20,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, selectForm,
		XmNbottomOffset, 0,
		NULL);

	selectedDisplay = XtVaCreateWidget("statMatrix",
		xbaeMatrixWidgetClass, mainForm,
		XmNrows, 4,	/* see note with resize_selected_storm_display() function */
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNshadowThickness, 2,
		XmNcellShadowThickness, 1,
		XmNcolumnWidthInPixels, True,
		XmNbuttonLabels, True,
		XmNgridType, XmGRID_CELL_SHADOW,
		XmNhighlightColor, statcfg.threshold_bg[SEVERE_THRESHOLD_NDX],
		XmNselectScrollVisible, False,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 0,
		NULL);

	XtAddCallback(selectedDisplay, XmNenterCellCallback, enter_cell_cb, (XtPointer) 1);

	statDisplay = XtVaCreateWidget("statMatrix", xbaeMatrixWidgetClass, mainForm,
		XmNrows, 0,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNshadowThickness, 2,
		XmNcellShadowThickness, 1,
		XmNcolumnWidthInPixels, True,
		XmNbuttonLabels, True,
		XmNgridType, XmGRID_CELL_SHADOW,
		XmNhighlightColor, statcfg.threshold_bg[SEVERE_THRESHOLD_NDX],
		XmNselectScrollVisible, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, selectForm,
		XmNtopOffset, 15,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, selectedDisplay,
		NULL);

	translations = XtParseTranslationTable(translation_string);
	XtOverrideTranslations(statDisplay, translations);
	XtVaGetValues(statDisplay, XmNtextField, &textField, NULL);
	XtOverrideTranslations(textField, translations);

	XtAddCallback(statDisplay, XmNenterCellCallback, enter_cell_cb, NULL);
	XtAddCallback(statDisplay, XmNleaveCellCallback, leave_cell_cb, NULL);
	XtAddCallback(statDisplay, XmNlabelActivateCallback, select_label_cb, NULL);
	XtAddCallback(statDisplay, XmNselectCellCallback, select_cell_cb, NULL);
	XtAddCallback(statDisplay, XmNmodifyVerifyCallback, modify_verify_cb, NULL);
	XtAddCallback(statDisplay, XmNdefaultActionCallback, double_click_cb, NULL);

	set_element_order(elem_view_ndx);

	/* Make the popup for selecting the environment. The details are filled
	 * when reading the environment configuration section.
	 */
	environSelectPopup = XuVaCreatePopupShell("environSelectPopup",
		transientShellWidgetClass, dialog,
		XmNoverrideRedirect, True,
		XmNsaveUnder,        False,
		XmNallowShellResize, True,
		NULL);

	XtAddEventHandler(environSelectPopup,
		LeaveWindowMask,
		False, (XtEventHandler) environ_select_popup_cb, NULL);

	table = NewXmStringArray(statcfg.nenvironment);
	for(i = 0; i < statcfg.nenvironment; i++)
		table[i] = XmStringCreateLocalized(statcfg.environment[i].label);

	environSelectList = XmVaCreateManagedList(environSelectPopup, "esrc",
		XmNselectionPolicy, XmBROWSE_SELECT,
		XmNlistMarginWidth, 5,
		XmNlistMarginHeight, 5,
		XmNitems, table,
		XmNitemCount, statcfg.nenvironment,
		XmNvisibleItemCount, statcfg.nenvironment,
		XmNshadowThickness, 1,
		NULL);

	XtAddCallback(environSelectList, XmNbrowseSelectionCallback, environ_select_cb,
		(XtPointer) &selected_row_column);
	XmStringArrayFree(table, statcfg.nenvironment);

	XmMainWindowSetAreas(dialog, menuBar, NULL, NULL, NULL, mainForm);
	XtManageChild(menuBar);
	XtManageChild(selectedDisplay);
	XtManageChild(statDisplay);
	XtManageChild(mainForm);
	/*
	 * This causes the widget to resize itself to contain only one row.
	 * Need to do it here after the widget is managed for the layout to
	 * take properly.
	 */
	XtVaSetValues(selectedDisplay, XmNvisibleRows, 1, NULL);
	XuShowDialog(dialog);

	update_filelist();
	if(nfilelist == 0)
	{
		/* Ignore the leading '^' as this is not in the config file definition */
		pr_error(pgm,"Did not find any files in directory \'%s\' matching STAT file mask \'%s\'\n",
			stat_data_dir, statcfg.file_mask+1);
	}

	RadarStatSetTime(ActiveDepictionTime(FIELD_INDEPENDENT));
	set_table_selection_arrows();

	AddObserver(OB_DEPICTION_CHANGE, depiction_change_observer);
}


/* Show the data from the radar STAT file for the given time. 
 * Public for future possible use.
 */
void RadarStatSetTime(String time)
{
	int n, year, jday, hour, min;

	if(!dialog) return;

	FreeItem(current_fname);
	rs_user_message(UM_STATUS, NULL);
	if(parse_tstamp(time, &year, &jday, &hour, &min, NULL, NULL))
	{
		char fname[300];
		requested_time = encode_clock(year, jday, hour, min, 0);
		(void) strftime(fname, sizeof(fname), statcfg.file_time_mask, gmtime(&requested_time));
		if(access(pathname(stat_data_dir,fname),R_OK) == 0 && read_stat_data(fname))
		{
			set_table_selection_arrows();
			return;
		}
		strftime(fname, sizeof(fname), statcfg.user_time_format, gmtime(&requested_time));
		XuWidgetLabel(timeLabel, fname);
	}
	else
	{
		rs_user_message(UM_ERROR,"ERROR: Depiction time parse failure.");
		XuWidgetLabel(timeLabel, "???");
	}
	XbaeMatrixDeleteRows(statDisplay, 0, XbaeMatrixNumRows(statDisplay));
	XbaeMatrixAddRows(statDisplay, 0, NULL, NULL, NULL, 1);
	for(n = 0; n < XbaeMatrixNumColumns(statDisplay); n++)
	{
		XbaeMatrixSetCell(statDisplay, 0, n, "---");
		XbaeMatrixSetCell(selectedDisplay, 0, n, "---");
	}
	XbaeMatrixResizeColumnsToCells(statDisplay, True);
	resize_selected_storm_display();
	set_table_selection_arrows();
}


/**************** Functions shared between radar source files only **************/

/* Called from radarStormTrendDialog to set modified rank weights forecast.
 * Note that vtimes is in forecast minutes (10, 20, ...).
 * The changes do not affect the xml file until the save button is activated.
 */
void rs_modify_forecast_rankweights(int ndata, int *vtimes, float *values)
{
	int n, k, row, col, pos;

	STORM *storm = rs_get_storm_ptr_from_storm_number(selected_storm_num);
	if(!storm) return;

	/* Initialize the rank weight forecast array to not available just
	 * to be paranoid.
	 */
	if((k = rs_get_element_array_pos_from_id(statcfg.rankweight.element_id)) < 0)
		return;

	for(n = 0; n < statcfg.num_forecasts; n++)
	{
		memset((void *) &storm->fcst[n].elem[k], 0, sizeof(DELEM));
		storm->fcst[n].elem[k].na = True;
	}
	/*
	 * If the valid times align with a forecast rank weight put the manual
	 * override values into the elements.
	 */
	for(n = 0; n < ndata; n++)
	{
		if((pos = rs_get_rankweight_element_array_pos_from_time(vtimes[n])) >= 0)
		{
			storm->data[pos].value = values[n];
			storm->data[pos].off = False;
			storm->data[pos].na = False;
			storm->data[pos].usermod = True;

			for(row = 0; row < XbaeMatrixNumRows(statDisplay); row++)
			{
				STORM *s = (STORM *)XbaeMatrixGetRowUserData(statDisplay, row);
				if(s->num == storm->num)
				{
					String ptr = print_value(storm, pos);
					col = matrix_column_from_element_array_pos(pos);
					if(col >= 0)
					{
						XbaeMatrixSetCell(statDisplay, row, col, ptr);
						set_cell_threshold_color(row, col);
					}
				}
			}
			if(n < statcfg.num_forecasts)
				memcpy((void *) &storm->fcst[n].elem[k], &storm->data[pos], sizeof(DELEM));
		}
	}
	populate_selected_storm_display();
	set_saveBtn_state();
}


/* Gets a valid set of thresholds for the given element, storm and severity
 * level. The cfgdef variable in the structure will be true if the thresholds
 * were defined in the configuration file. The define SCIT_ENVIRON is the
 * set as defined in the SCIT file. The active threshold is stored in the
 * storm->data[ndx].value as a float.
 */
THRESH *rs_active_thresholds(ELEMCFG *elem, STORM *storm, int level)
{
	int ndx = rs_get_element_array_pos_from_id(STORM_ENV_ID);
	THRESH *thresholds = elem->thresholds + NINT(storm->data[ndx].value);
	if(!thresholds->cfgdef[level])
		thresholds = elem->thresholds;
	if(!thresholds->cfgdef[level])
		thresholds = elem->thresholds + SCIT_ENVIRON;
	return thresholds;
}


/* Given the element id, return the index into the statcfg.element array
 * where the element is found. If not found return -1.
 */
int rs_get_element_array_pos_from_id(String id)
{
	int n;
	for(n = 0; n < statcfg.nelement; n++)
	{
		if(xmlStrcmp(id,statcfg.element[n].id) == 0) return n;
	}
	rs_user_message(UM_ERROR, "Storm element \'%s\' is not defined in the configuration.", id);
	return -1;
}


/* This function returns the position in the element array where the forecast
 * rank weight is found that corresponds to the given delta time in minutes.
 * The element name is RankWeightxx where xx is the time, so this function looks
 * for the element.
 */
int rs_get_rankweight_element_array_pos_from_time(int minutes)
{
	int n;
	int len = (int) strlen(statcfg.rankweight.element_id);
	for(n = 0; n < statcfg.nelement; n++)
	{
		if(same_start(statcfg.element[n].id, statcfg.rankweight.element_id))
		{
			/* get the time from the id */
			if(atoi(&statcfg.element[n].id[len]) == minutes) return n;
		}
	}
	return -1;
}



/***************** Private Functions ********************/


/* Compares two values as they would be represented as a string
 * at the precision (number of decimals) for the element at the
 * position pos in the element array.
 */
static Boolean same_values(float v1, float v2, int pos)
{
	char buf1[32], buf2[32];
	if(v1 == v2) return True;
	snprintf(buf1, 32, "%.*f", statcfg.element[pos].ndecimals, v1);
	snprintf(buf2, 32, "%.*f", statcfg.element[pos].ndecimals, v2);
	return same(buf1,buf2);
}


/* Print the value associated with the storm data at element
 * position pos in the array.
 */
static String print_value(STORM *storm, int pos)
{
	static char buf[32];

	if(storm->data[pos].off)
		snprintf(buf, 32, "%s", statcfg.data_off_string);
	else if(storm->data[pos].na)
		strcpy(buf, DATA_NA);
	else
		snprintf(buf, 32, "%.*f", statcfg.element[pos].ndecimals, storm->data[pos].value);

	/* For the elements that can be modified, if the values are not the same as
	 * the original show the original value in brackets, but only if the element
	 * has not been turned off.
	 */
	if(statcfg.element[pos].type == ET_RANK_CALC && !storm->data[pos].off)
	{
		if(storm->unmod[pos].na != storm->data[pos].na  ||
		   !same_values(storm->unmod[pos].value, storm->data[pos].value, pos))
		{
			size_t len = strlen(buf);
			if(storm->unmod[pos].na)
				strcat(buf, " (N/A)");
			else if(!storm->unmod[pos].off)
				snprintf(buf+len, 32-len, " (%.*f)", statcfg.element[pos].ndecimals, storm->unmod[pos].value);
		}
	}
	return buf;
}


/* Set or unset the ratio property of the given storm element defined by array
 * position pos. The element must be one used in generating rank weight.
 */
static void set_ratio_prop(STORM *storm, int pos)
{
	if(statcfg.element[pos].type != ET_RANK_CALC) return;

	if(storm->unmod[pos].na || storm->unmod[pos].value == 0.0 || storm->data[pos].na)
	{
		xmlUnsetProp(storm->nodes[pos], RATIO_PROP);
	}
	else
	{
		char buf[32];
		snprintf(buf, 32, "%f", storm->data[pos].value / storm->unmod[pos].value);
		xmlSetProp(storm->nodes[pos], RATIO_PROP, buf);
	}
}


/* Compare storm->data[pos] and storm->stat[pos] for the given element
 * array position to see if there are any differences.
 */
static Boolean same_storm_value(STORM *storm, int pos)
{
	/* Compare against one more decimal point than specified for the element.
	 * eg. ndecimals = 1 will result in a comparison against 0.1
	 */
	return (same_values(storm->data[pos].value, storm->stat[pos].value, pos)
			&& storm->data[pos].off == storm->stat[pos].off
			&& storm->data[pos].na == storm->stat[pos].na
			&& storm->data[pos].usermod == storm->stat[pos].usermod);
}

/* Pops down the storm environment popup when the mouse leaves the popup */
/*ARGSUSED*/
static void environ_select_popup_cb(Widget w , XtPointer client_data , XEvent *event)
{
	XtPopdown(w);
}


/* Reacts to a selection from the environment list in the environSelectPopup.
 * The row and column are stored in a static variable passed through client_data.
 * The environment cell entry is changed and the storm thresholds re-evaluated
 * for all of the appropriate elements of the storm.
 */
/*ARGSUSED*/
static void environ_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int row, col, pos, envnum;
	STORM *storm;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *) call_data;

	XPoint *location = (XPoint *) client_data;
	row = (int) location->x;
	col = (int) location->y;

	XtPopdown(environSelectPopup);

	storm = (STORM *) XbaeMatrixGetRowUserData(statDisplay, row);

	/* list return is 1 origin */
	envnum = rtn->item_position - 1;

	/* save the environment array position number in the data array */
	pos = matrix_column_to_element_array_pos(col);
	storm->data[pos].value = (float) envnum;
	/*
	 * Set the selected environment into the matrix and redo thresholds
	 */
	XbaeMatrixDisableRedisplay(statDisplay);
	XbaeMatrixSetCell(statDisplay, row, col, statcfg.environment[envnum].label);
	for(col = 0; col < XbaeMatrixNumColumns(statDisplay); col++)
	{
		if(elemp[col]->has_threshold && elemp[col]->has_data)
			set_cell_threshold_color(row, col);
	}
	XbaeMatrixEnableRedisplay(statDisplay, True);
	XbaeMatrixResizeColumnsToCells(statDisplay, True);
	set_saveBtn_state();
}


/* Set the message to the user into the userMessage widget. If
 * the message is an error output it in pr_error as well.
 */
void rs_user_message(int msg_type, String text, ...)
{
	if(!dialog || !XtIsRealized(userMessage)) return;
	if(blank(text))
	{
		XtUnmanageChild(userMessage);
	}
	else
	{
		char buf[200];
		Pixel fg;
		XmString label;
		va_list args;
		va_start(args, text);
		(void) vsnprintf(buf, sizeof(buf), text, args);
		va_end(args);
		switch(msg_type)
		{
			case UM_ERROR: fg = XuLoadColor(dialog,ERROR_COLOR); break;
			default:       fg = XuLoadColor(dialog,XmNbackground); break;
		}
		label = XmStringGenerate(buf, NULL, XmCHARSET_TEXT, NULL);
		XtVaSetValues(userMessage,
			XmNlabelString, label,
			XmNborderColor, fg,
			NULL);
		XmStringFree(label);
		XtManageChild(userMessage);
		strcat(buf, "\n");
		switch(msg_type)
		{
			case UM_ERROR: pr_error(pgm, buf); break;
		}
	}
}


/* When the statDisplay columns are resized to fit the data the selected
 * storm display widget at the bottom of the window needs to have its 
 * columns resized to the same width as the statDisplay. Also, in order
 * to get the storm display to layout it's height properly I needed to
 * specify an initial row count of 4 (trial and error). I put the code to
 * set the row number to 1 here for convienience.
 */
static void resize_selected_storm_display(void)
{
	int n;
	XbaeMatrixDisableRedisplay(selectedDisplay);
	for(n = 0; n < XbaeMatrixNumColumns(statDisplay); n++)
	{
		int width = XbaeMatrixGetColumnWidth(statDisplay, n);
		XbaeMatrixSetColumnWidth(selectedDisplay, n, width);
	}
	n = XbaeMatrixNumRows(selectedDisplay);
	if(n > 1) XbaeMatrixDeleteRows(selectedDisplay, 0, n - 1);
	XbaeMatrixEnableRedisplay(selectedDisplay, True);
}


/* Populate the special selected storm display row at the bottom of the
 * window with the storm information.
 */
static void populate_selected_storm_display(void)
{
	int row, col, sel = -1;
	Pixel pix, fg, bg;
	STORM *storm = NULL;

	/* Make sure that the selected storm exists in the data. If it
	 * does not blank out the selectedDisplay row.
	 */
	for(row = 0; row < XbaeMatrixNumRows(statDisplay); row++)
	{
		storm = (STORM *) XbaeMatrixGetRowUserData(statDisplay, row);
		if(storm != NULL && storm->num == selected_storm_num)
		{
			sel = selected_storm_num;
			break;
		}
	}

	/* Transfer the data and the cell threshold colour */
	XtVaGetValues(statDisplay, XmNforeground, &fg, XmNbackground, &bg, NULL);
	for(col = 0; col < XbaeMatrixNumColumns(statDisplay); col++)
	{
		if(sel < 0)
		{
			XbaeMatrixSetCell(selectedDisplay, 0, col, NULL);
			XbaeMatrixSetCellColor(selectedDisplay, 0, col, fg);
			XbaeMatrixSetCellBackground(selectedDisplay, 0, col, bg);
			XbaeMatrixUnhighlightCell(selectedDisplay, 0, col);
		}
		else
		{
			String val = XbaeMatrixGetCell(statDisplay, row, col);
			XbaeMatrixSetCell(selectedDisplay, 0, col, val);
			pix = XbaeMatrixGetCellColor(statDisplay, row, col);
			XbaeMatrixSetCellColor(selectedDisplay, 0, col, pix);
			pix = XbaeMatrixGetCellBackground(statDisplay, row, col);
			XbaeMatrixSetCellBackground(selectedDisplay, 0, col, pix);
			/* There is no get function for cell highlighting so must do from data */
			set_cell_highlight(selectedDisplay, storm, 0, col);
		}
	}
}


/* Undo any changes back to the state of the file as read. Note that this will
 * backup the display to include any edits that were saved  previously.
 */
static void undo_file_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int n, col, row, nelements;
	Boolean undo = False;
	Boolean recovering = (client_data != NULL && PTR2BOOL(client_data));
	ELEMCFG *elements = NewMem(ELEMCFG, statcfg.nelement);

	for(row = 0; row < storm_list_len; row++)
	{
		Boolean storm_undo = False;
		Boolean rank_fcst_recover = False;
		Boolean rank_elem_recover = False;

		nelements = 0;
		STORM *storm = storm_list + row;
		for(col = 0; col < statcfg.nelement; col++)
		{
			if(same_storm_value(storm, col)) continue;
			if(recovering)
			{
				if(statcfg.element[col].type == ET_RANK_ELEM)
					rank_elem_recover = True;
				if(statcfg.element[col].type == ET_RANK_FCST)
					rank_fcst_recover = True;
			}
			storm_undo = True;
			storm->data[col].value   = storm->stat[col].value;
			storm->data[col].off     = storm->stat[col].off;
			storm->data[col].na      = storm->stat[col].na;
			storm->data[col].usermod = storm->stat[col].usermod;
			for(n = 1; n < storm->nhist; n++)
				storm->hist[n].elem[col].off = storm->data[col].off;
			for(n = 0; n < statcfg.num_forecasts; n++)
				storm->fcst[n].elem[col].off = storm->data[col].off;
			memcpy((void*) elements+nelements, (void*) statcfg.element+col, sizeof(ELEMCFG));
			nelements++;
		}
		if(storm_undo)
		{
			undo = True;
			rs_make_storm_element_trend_forecast(storm, elements, nelements);
			if(!rank_elem_recover)
				rs_calc_storm_rankweight(storm);
			if(!rank_fcst_recover)
				rs_make_rankweight_forecast(storm);
		}
	}
	FreeItem(elements);

	/* update the matrix */
	if(undo)
	{
		display_all_storms();
		XbaeMatrixResizeColumnsToCells(statDisplay, True);
		resize_selected_storm_display();
		populate_selected_storm_display();
	}

	XtSetSensitive(undoBtn, False);
	XtSetSensitive(saveBtn, False);
	activate_storm_trend_dialog(REDISPLAY);
}


/* Respond to the button that recovers the file data back to its original state. 
 */
static void recover_file_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int col, row;
	Boolean save_needed = False;

	if(!docp) return;
	XtSetSensitive(recoverBtn, False);

	for(row = 0; row < storm_list_len; row++)
	{
		STORM *storm = storm_list + row;
		for(col = 0; col < statcfg.nelement; col++)
		{
			if(storm->unmod[col].usermod || storm->stat[col].off)
			{
				storm->stat[col].value = storm->unmod[col].value;
				storm->stat[col].na    = storm->unmod[col].na;
				storm->stat[col].off   = False;
				storm->stat[col].usermod = False;
				save_needed = True;
			}
		}
	}
	undo_file_cb(w, INT2PTR(True), call_data);
	XtSetSensitive(saveBtn, save_needed);
}


/* Respond to a toggle button that selects the element view that the user wants.
 * The selection is saved to the state file and reset the next time in.
 */
static void element_view_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if(XmToggleButtonGetState(w))
	{
		int ndx = PTR2INT(client_data);
		set_element_order(ndx);
		XuStateDataSave(STATE_SAVE_KEYS, elem_view[ndx].label);
	}
}


/* Return the timestamp of the file name in unix seconds
 */
static time_t file_time(String fname)
{
	struct tm dt;
	memset((void*)&dt, 0, sizeof(struct tm));
	strptime(fname, statcfg.file_time_mask, &dt);
	return encode_clock(dt.tm_year+1900, dt.tm_yday+1, dt.tm_hour, dt.tm_min, 0);
}


/* Set the element pointer to point to the element view array given by ndx. 
 */
static void set_element_order(int ndx)
{
	int n;
	String *labels;
	UCHAR *alignment, *label_alignment;

	if(ndx >= 0)
	{
		elemp  = elem_view[ndx].elemp;
		nelemp = elem_view[ndx].nelemp;
	}

	/* The rest of the code must only be run when the
	 * dialog exists.
	 */
	if(!dialog) return;

	labels = NewMem(String, nelemp);
	alignment = NewMem(UCHAR, nelemp);
	label_alignment = NewMem(UCHAR, nelemp);

	for(n = 0; n < nelemp; n++)
	{
		labels[n] = elemp[n]->header;
		alignment[n] = elemp[n]->alignment;
		label_alignment[n] = statcfg.label_alignment;
	}

	XtVaSetValues(statDisplay,
		XmNcolumns, nelemp,
		XmNcolumnLabels, labels,
		XmNcolumnLabelAlignments, label_alignment,
		XmNcolumnAlignments, alignment,
		NULL);

	XtVaSetValues(selectedDisplay,
		XmNcolumns, nelemp,
		XmNcolumnLabels, labels,
		XmNcolumnLabelAlignments, label_alignment,
		XmNcolumnAlignments, alignment,
		NULL);

	FreeItem(labels);
	FreeItem(alignment);
	FreeItem(label_alignment);

	read_stat_data(current_fname);
	XbaeMatrixResizeColumnsToCells(statDisplay, True);
	resize_selected_storm_display();
}


/* Set the sensitivity state of the table selection arrows depending
 * on if there is a valid data file in the forwards or backwards
 * direction from the current file.
 */
static void set_table_selection_arrows(void)
{
	int year, jday, hour, min;
	time_t depict_time;

	if(!dialog) return;

	parse_tstamp(ActiveDepictionTime(FIELD_INDEPENDENT), &year, &jday, &hour, &min, NULL, NULL);
	depict_time = encode_clock(year, jday, hour, min, 0);

	if(current_fname)
	{
		int pos;
		if(InList(current_fname, nfilelist, filelist, &pos))
		{
			XtSetSensitive(arrowBack, (pos > 0));
			XtSetSensitive(arrowForward, (pos < nfilelist-1 && requested_time < depict_time));
			if(!XtIsSensitive(arrowBack) && !XtIsSensitive(arrowForward))
				rs_user_message(UM_STATUS,"No STAT files before depiction time.");
			else if(!XtIsSensitive(arrowForward) && pos == nfilelist - 1)
				rs_user_message(UM_STATUS, "Showing most recent STAT file.");
			else
				rs_user_message(UM_STATUS, NULL);
		}
		else
		{
			rs_user_message(UM_ERROR,"Unable to find STAT file \'%s\' anymore!",
					current_fname);
		}
	}
	else if(nfilelist > 0)
	{
		time_t dt = file_time(filelist[0]);
		XtSetSensitive(arrowBack, (requested_time > dt));
		dt = file_time(filelist[nfilelist-1]);
		XtSetSensitive(arrowForward, (requested_time < dt && requested_time < depict_time));
		if(!XtIsSensitive(arrowBack) && !XtIsSensitive(arrowForward))
			rs_user_message(UM_STATUS,"No STAT files at or before depiction time.");
		else if(XtIsSensitive(arrowBack) && !XtIsSensitive(arrowForward))
			rs_user_message(UM_STATUS,"No STAT file at depiction time.");
		else
			rs_user_message(UM_STATUS, NULL);
	}
	else
	{
		XtSetSensitive(arrowBack, False);
		XtSetSensitive(arrowForward, False);
		rs_user_message(UM_STATUS,"No STAT files are available.");
	}
}


/* As only the last non-forecast SCIT file can have its forecast
 * rank weights edited there is a need to identify this last file
 * before the forecast files (if there are any).
 */
static void activate_storm_trend_dialog(int snum)
{
	Boolean editable = False;

	if(nfilelist > 0 && !current_is_fcst)
	{
		int pos;
		if(!(editable = (InList(current_fname, nfilelist, filelist, &pos) && pos == nfilelist-1)))
		{
			/* if the next file in the list is a forecast file then we are editable */
			xmlNodePtr root;
			xmlDocPtr doc = xmlReadFile(pathname(stat_data_dir,filelist[pos+1]), NULL, 0);
			if(NotNull(doc) && NotNull(root = xmlDocGetRootElement(doc)))
			{
				String val = xmlGetProp(root,TYPE_PROP);
				editable = (!blank(val) && NotNull(strstr(val,FCST_KEY)));
				xmlFree(val);
			}
			xmlFreeDoc(doc);
		}
	}

	if(snum == REDISPLAY)
	{
		STORM *storm = rs_get_storm_ptr_from_storm_number(selected_storm_num);
		ACTIVATE_stormTrendDialog(storm, editable);
	}
	else if(snum == selected_storm_num)
	{
		selected_storm_num = -1;
		ACTIVATE_stormTrendDialog(NULL, editable);
	}
	else
	{
		STORM *storm = rs_get_storm_ptr_from_storm_number(snum);
		selected_storm_num = snum;
		ACTIVATE_stormTrendDialog(storm, editable);
	}

	populate_selected_storm_display();
}


/* Respond to the the arrows and move the displayed STAT table up or down
 * the list of available table files.
 */
static void arrow_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int n, pos;
	int arrow_dirn = PTR2INT(client_data);

	if(current_fname)
	{
		if(InList(current_fname, nfilelist, filelist, &pos))
		{
			save_data_check();
			if(pos < nfilelist - 1 && arrow_dirn > 0)
			{
				pos++;
				requested_time = file_time(filelist[pos]);
				(void)read_stat_data(filelist[pos]);
			}
			else if(pos > 0 && arrow_dirn < 0)
			{
				pos--;
				requested_time = file_time(filelist[pos]);
				(void)read_stat_data(filelist[pos]);
			}
		}
	}
	else
	{
		if(arrow_dirn > 0)
		{
			for(n = nfilelist-1; n >= 0; n--)
			{
				time_t dt = file_time(filelist[n]);
				if(requested_time < dt) continue;
				requested_time = dt;
				(void)read_stat_data(filelist[n]);
				break;
			}
			if(n < 0 && nfilelist > 0)
			{
				requested_time = file_time(filelist[0]);
				(void) read_stat_data(filelist[0]);
			}
		}
		else
		{
			for(n = 0; n < nfilelist; n++)
			{
				time_t dt = file_time(filelist[n]);
				if(requested_time > dt) continue;
				requested_time = dt;
				(void)read_stat_data(filelist[n]);
				break;
			}
			if(n >= nfilelist && nfilelist > 0)
			{
				requested_time = file_time(filelist[nfilelist-1]);
				(void) read_stat_data(filelist[nfilelist-1]);
			}
		}
	}
	set_table_selection_arrows();
	activate_storm_trend_dialog(REDISPLAY);
}


/* This function is called when the active depiction changes.
 */
static void depiction_change_observer (String *parms, int nparms)
{
	if(!dialog) return;
	update_filelist();
	save_data_check();
	RadarStatSetTime(ActiveDepictionTime(FIELD_INDEPENDENT));
	set_table_selection_arrows();
	activate_storm_trend_dialog(REDISPLAY);
}


/* This function is activated when the source system detects a change in
 * the directory containing the radar STAT files. 
 */
static Boolean data_dir_change_observer(Boolean unused)
{
	int n;
	SourceList slist;

	update_filelist();
	if(!InList(current_fname, nfilelist, filelist, NULL))
		FreeItem(current_fname);
	set_table_selection_arrows();

	SourceListByType(SRC_RADAR_STAT, FpaC_TIMEDEP_ANY, &slist, &n);
	if(dialog && current_fname && n > 0 && slist[0]->modified)
	{
		struct stat sbuf;
		if( stat(pathname(stat_data_dir,current_fname),&sbuf) == 0 &&
			current_mod_time != sbuf.st_mtime)
		{
			read_stat_data(current_fname);
			activate_storm_trend_dialog(REDISPLAY);
		}
	}
	return True;
}



/******************* Matrix Manipulation ********************/


/* The cell values can have a space or the severe flag character
 * appended to the end of the value. This must be stripped before
 * returning the actual value.
 */
static String get_cell_value(int row, int col)
{
	static char baeval[32];
	String p, val = XbaeMatrixGetCell(statDisplay, row, col);
	memset(baeval, 0, 32*sizeof(char));
	if(val) strncpy(baeval, val, 31);
	no_white(baeval);
	return baeval;
}

/* Function to sort the rows depending which column button was pushed.
 */
static int row_sort_fcn(Widget w, int row1, int row2, void *user_data)
{
	int column = PTR2INT(user_data);

	if(elemp[column]->data_type == DT_NONE) return 0;

	if(elemp[column]->data_type == DT_STRING)
	{
		char *p, buf1[100], buf2[100];
		strcpy(buf1,"");
		strcpy(buf2,"");

		/* Need to copy return from get_cell_value function */
		if((p = get_cell_value(row1, column)))
			snprintf(buf1, sizeof(buf1), "%s", p);
		if((p = get_cell_value(row2, column)))
			snprintf(buf2, sizeof(buf2), "%s", p);

		if(elemp[column]->sort_ascending)
			return strcmp(buf2,buf1);
		else
			return strcmp(buf1,buf2);
	}
	else
	{
		int pos;
		float v1 = FLT_MIN, v2 = FLT_MIN;
		STORM *s1 = (STORM *) XbaeMatrixGetRowUserData(statDisplay, row1);
		STORM *s2 = (STORM *) XbaeMatrixGetRowUserData(statDisplay, row2);

		pos = elemp[column]->ndx;
		if(!s1->data[pos].na && !s1->data[pos].off)
			v1 = s1->data[pos].value;
		if(!s2->data[pos].na && !s2->data[pos].off)
			v2 = s2->data[pos].value;

		if(elemp[column]->sort_ascending)
		{
			if(v1 > v2) return 1;
			if(v1 < v2) return -1;
		}
		else
		{
			if(v2 > v1) return 1;
			if(v2 < v1) return -1;
		}
		return 0;
	}
}


/* Respond to activation of the matrix widget column label buttons.
 */
static void select_label_cb(Widget w, XtPointer user_data, XtPointer call_data)
{
	XbaeMatrixLabelActivateCallbackStruct *rtn = (XbaeMatrixLabelActivateCallbackStruct *)call_data;
	if(!rtn->row_label)
	{
		sort_on_column = rtn->column;
		XbaeMatrixSortRows(statDisplay, row_sort_fcn, INT2PTR(rtn->column));
	}
}


static void popup_environ_selector(STORM *storm, int row, int column)
{
	int x, y, ndx, pos;
	Position wx, wy;
	Widget clipWindow;
	/*
	 * Show the environment selection dialog and position it at the appropriate cell.
	 */
	pos = matrix_column_to_element_array_pos(column);
	ndx = NINT(storm->data[pos].value);
	XuListSetToItem(environSelectList, statcfg.environment[ndx].label);

	XtVaGetValues(statDisplay, XmNclipWindow, &clipWindow, NULL);
	XtTranslateCoords(clipWindow, 0, 0, &wx, &wy);
	(void) XbaeMatrixRowColToXY(statDisplay, row, column, &x, &y);

	selected_row_column.x = (short) row;
	selected_row_column.y = (short) column;

	XtVaSetValues(environSelectPopup,
		XmNx, wx + (Position) x,
		XmNy, wy + (Position) y,
		NULL);
	XtPopup(environSelectPopup, XtGrabNone);
	XuSetDialogCursor(environSelectPopup, XuDEFAULT_CURSOR, ON);
}


/* If the cell contains the storm id, then launch the storm trend graph display.
 * If the cell contains the storm environment then launch a dialog to select an
 * environment. Action in any other cell is an edit action if it is an element
 * used to calculate rank weight.
 * For some reason enter_cell_cb is always called after the double click. The dc
 * variable is used as an interlock to indicate that double_click_cb was called
 * immediately before this and an edit is not wanted.
 */
static Boolean dc = False;

static void enter_cell_cb(Widget w, XtPointer user_data, XtPointer call_data)
{
	XbaeMatrixEnterCellCallbackStruct *rtn = (XbaeMatrixEnterCellCallbackStruct *)call_data;

	rtn->doit = False;
	rtn->map  = False;
	if(user_data) return;

	if(dc)
	{
		dc = False;
	}
	else
	{
		STORM *storm = (STORM *)XbaeMatrixGetRowUserData(statDisplay, rtn->row);
		if(!storm) return;

		if(elemp[rtn->column]->type == ET_STORM_NUMBER)
		{
			activate_storm_trend_dialog(storm->num);
		}
		else if(elemp[rtn->column]->type == ET_ENVIRON)
		{
			popup_environ_selector(storm, rtn->row, rtn->column);
		}
		else
		{
			int pos = matrix_column_to_element_array_pos(rtn->column);
			if(!storm->data[pos].off)
				rtn->doit = (elemp[rtn->column]->type == ET_RANK_CALC);
			rtn->map = rtn->doit;
			rtn->select_text = rtn->doit;
			set_saveBtn_state();
		}
	}
}


/* The first double click on a cell will deactivate the table value. A second double
 * click will restore the cell value. The node of the associated data is stored in
 * the user data of the cell. 
 */
static void double_click_cb(Widget w, XtPointer user_data, XtPointer call_data)
{
	int i, pos;
	String ptr;
	STORM *storm;
	XbaeMatrixDefaultActionCallbackStruct *rtn = (XbaeMatrixDefaultActionCallbackStruct *)call_data;

	XbaeMatrixCancelEdit(statDisplay, True);
	if(elemp[rtn->column]->type != ET_RANK_CALC) return;

	storm = (STORM*) XbaeMatrixGetRowUserData(statDisplay, rtn->row);
	if(!storm) return;

	pos = matrix_column_to_element_array_pos(rtn->column);

	/* Note that the off state is applied to all instances of the element
	 * in the storm, not just the one that triggers the event.
	 */
	storm->data[pos].off = !storm->data[pos].off;
	ptr = print_value(storm, pos);
	XbaeMatrixSetCell(statDisplay, rtn->row, rtn->column, ptr);
	set_cell_threshold_color(rtn->row, rtn->column);
	/* storm->data and storm->hist[0].elem are the same */
	for(i = 1; i < storm->nhist; i++)
		storm->hist[i].elem[pos].off = storm->data[pos].off;
	for(i = 0; i < statcfg.num_forecasts; i++)
		storm->fcst[i].elem[pos].off = storm->data[pos].off;
	rs_calc_storm_rankweight(storm);
	rs_make_storm_element_trend_forecast(storm, &statcfg.element[pos], 1);
	rs_make_rankweight_forecast(storm);
	redisplay_storm(storm);
	XbaeMatrixResizeColumnsToCells(statDisplay, True);
	resize_selected_storm_display();
	populate_selected_storm_display();
	activate_storm_trend_dialog(REDISPLAY);
	set_saveBtn_state();
	dc = True;
}


/* There is an interaction between the select_cell_cb function call and
 * leave_cell_cb that results in the wrong value being displayed in the
 * cell when an output like "24 (48)" is wanted. Putting the display code
 * in a timeout, thus letting the main loop operate on XbaeMatrix fixes
 * the problem. It would not be an issue if the value entered when editing
 * the cell was the value to be displayed, but we don't do that.
 */
struct CellInfo { STORM *storm; int pos; };

static void process_cell_edit(XtPointer client_data , XtIntervalId *id)
{
	struct CellInfo *cell = (struct CellInfo *) client_data;
	rs_calc_storm_T0_rankweight(cell->storm);
	rs_make_storm_element_trend_forecast(cell->storm, &statcfg.element[cell->pos], 1);
	rs_make_rankweight_forecast(cell->storm);
	redisplay_storm(cell->storm);
	XbaeMatrixResizeColumnsToCells(statDisplay, True);
	resize_selected_storm_display();
	populate_selected_storm_display();
	activate_storm_trend_dialog(REDISPLAY);
	set_saveBtn_state();
}


/* When leaving the cell recalculate and reprocess the storm row.
 */
static void leave_cell_cb(Widget w, XtPointer user_data, XtPointer call_data)
{
	static struct CellInfo cell;
	XbaeMatrixLeaveCellCallbackStruct *rtn = (XbaeMatrixLeaveCellCallbackStruct *)call_data;

	if(blank(rtn->value))
	{
		rtn->doit = False;
		set_saveBtn_state();
	}
	else if((cell.storm = (STORM*) XbaeMatrixGetRowUserData(statDisplay, rtn->row)))
	{
		Boolean ok;
		cell.pos = matrix_column_to_element_array_pos(rtn->column);
		cell.storm->data[cell.pos].value = rs_float_parse(rtn->value, &ok);
		cell.storm->data[cell.pos].na = !ok;
		(void) XtAppAddTimeOut(GV_app_context, 50, process_cell_edit, (XtPointer) &cell);
	}
}


/* Responds to a 'right' mouse click and commits the edit.
 */
static void select_cell_cb(Widget w, XtPointer user_data, XtPointer call_data)
{
	XbaeMatrixCommitEdit(statDisplay, True);
}


/* Verifys the text as it is being entered by the user while editing. Note that this is
 * for interactive text editing and will not be useful on a cut and paste except for the
 * first character.
 */
static void modify_verify_cb(Widget w, XtPointer user_data, XtPointer call_data)
{
	XbaeMatrixModifyVerifyCallbackStruct *rtn = (XbaeMatrixModifyVerifyCallbackStruct *)call_data;
	if(rtn->verify->text->length < 1) return;
	switch(elemp[rtn->column]->data_type)
	{
		case DT_NUMERIC:
			rtn->verify->doit = (strchr("-+.1234567890",*rtn->verify->text->ptr) != NULL);
			break;
		case DT_STRING:
			rtn->verify->doit = True;
			break;
		default:
			rtn->verify->doit = False;
	}
}


/* This timeout function updates the current data file if the modification time
 * changes within a 10 second window. The delay is to allow the background
 * rankweightDaemon program time to respond to file changes.
 */
static void check_for_current_file_update(XtPointer client_data , XtIntervalId *id)
{
	struct stat sbuf;
	static int loop_count = 0;

	if(!dialog) return;
	if(!current_fname) return;
	if(!id)
	{
		loop_count = 0;
		(void) XtAppAddTimeOut(GV_app_context, 500, check_for_current_file_update, NULL);
	}
	else if(loop_count < 20 && stat(pathname(stat_data_dir,current_fname),&sbuf) == 0)
	{
		loop_count++;
		if(current_mod_time != sbuf.st_mtime)
		{
			read_stat_data(current_fname);
			activate_storm_trend_dialog(REDISPLAY);
		}
		else
		{
			(void) XtAppAddTimeOut(GV_app_context, 500, check_for_current_file_update, NULL);
		}
	}
}


/* For the save button callback. Once the file is saved a timeout is done
 * using the above update function.
 */
static void save_to_file_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
        String ptr;
	struct stat sbuf;
	struct tm dt;
	char   tbuf[20], ebuf[2480], lbuf[2480];
	char   pbuf[2480];
	update_xml_doc();
	XtSetSensitive(saveBtn, False);
	XtSetSensitive(undoBtn, False);
	if(current_fname && stat(pathname(stat_data_dir,current_fname),&sbuf) == 0)
	{
		current_mod_time = sbuf.st_mtime;
		check_for_current_file_update(NULL, 0);
	}
	memset((void*)&dt, 0, sizeof(struct tm));
	strftime(tbuf, sizeof(tbuf), "%Y%m%d%H%M", gmtime(&requested_time));

	safe_strcpy(ebuf, get_path("python_exe","tracker.py"));
	safe_strcpy(lbuf, get_path("python_log","tracker.log"));
        if ( !blank(ebuf) && !blank(lbuf) ){
	  (void) snprintf(pbuf, sizeof(pbuf),"python %s -t %s -log %s -urp operdp -noplot" , ebuf, tbuf, lbuf);
	
	  if(shrun(pbuf, False) == 0) {
	    pr_status(pgm,"Running external process: \'%s\' \n", pbuf );
	  }
	  else{
	    pr_warning(pgm,"Unable to run external process: \'%s\' \n", pbuf );
	  }
	}
	else {
	    pr_warning(pgm,"Missing path for tracker.py or tracker.log\n" );
	}
}


/* Check to see if there have been any changes to the data and ask
 * if a save is wanted before proceeding. The message is found in
 * the app-defaults/XFpaMdb file.
 */
static void save_data_check(void)
{
	if(XtIsSensitive(saveBtn))
	{
		if(XuAskUser(dialog, "radarStatMod", NULL) == XuYES )
			update_xml_doc();
	}
	XtSetSensitive(saveBtn, False);
}


/************* Utility functions used for data extraction ************/


/* This exists to make what is being done clear as just using the
 * elemp[]->ndx form can be confusing.
 */
static int matrix_column_to_element_array_pos(int col)
{
	if(col >=0 && col < nelemp)
		return elemp[col]->ndx;
	return -1;
}


/* The reverse */
static int matrix_column_from_element_array_pos(int pos)
{
	int col;
	for(col = 0; col < nelemp; col++)
	{
		if(elemp[col]->ndx == pos) return col;
	}
	return -1;
}


static int get_matrix_column_from_id(String id)
{
	int n;
	for(n = 0; n < nelemp; n++)
	{
		if(xmlStrcmp(id,elemp[n]->id) == 0) return n;
	}
	return -1;
}


/* Find the data position in the statcfg.element array that the
 * element is found in. This exists because no assumption is
 * made as to element order.
 */
static int get_element_pos_from_node(xmlNodePtr node)
{
	if(node->type != XML_ELEMENT_NODE) return -1;
	return rs_get_element_array_pos_from_id((String) node->name);
}


/* Set the cell to the default colour.
 */
static void clear_cell_threshold_color(int row, int col)
{
	Pixel fg, bg;
	if(row < 0 || col < 0) return;

	/* These are the default colours */
	XtVaGetValues(statDisplay, XmNforeground, &fg, XmNbackground, &bg, NULL);

	/* Set the colours only if changed */
	if(XbaeMatrixGetCellColor(statDisplay, row, col) != fg)
		XbaeMatrixSetCellColor(statDisplay, row, col, fg);
	if(XbaeMatrixGetCellBackground(statDisplay, row, col) != bg)
		XbaeMatrixSetCellBackground(statDisplay, row, col, bg);
}


/* Determine if the data value for the given element exceeds any
 * of the threshold values or is a new storm. If so the function will
 * set the appropriate foreground and background colours for the cell
 * defined by the row,col reference. If the storm is new, the rank weight
 * is set to the new storm colour.
 */
static void set_cell_threshold_color(int row, int col)
{
	int pos;
	Pixel fg, bg;
	STORM *storm;

	if(row < 0 || col < 0) return;

	storm = (STORM *)XbaeMatrixGetRowUserData(statDisplay, row);
	if(!storm) return;

	/* The row user data indicates if this is a new storm or not. Only change
	 * the colour of the rank weight element in the row to indicate this.
	 */
	if(storm->nhist < 2 && elemp[col]->type == ET_RANK_ELEM)
	{
		/* The new storm colour is in threshold pixel array element MAXTHRESH */
		XbaeMatrixSetCellColor(statDisplay, row, col, statcfg.threshold_fg[MAXTHRESH]);
		XbaeMatrixSetCellBackground(statDisplay, row, col, statcfg.threshold_bg[MAXTHRESH]);
		return;
	}

	/* These are the default colours */
	XtVaGetValues(statDisplay, XmNforeground, &fg, XmNbackground, &bg, NULL);

	pos = matrix_column_to_element_array_pos(col);

	if( elemp[col]->has_threshold    &&
		elemp[col]->has_data         &&
		!storm->data[pos].na &&
		!storm->data[pos].off   )
	{
		int n;
		float val = storm->data[pos].value;

		for(n = 0; n < MAXTHRESH; n++)
		{
			THRESH *thresholds = rs_active_thresholds(elemp[col], storm, n);
			if(thresholds->isabs[n])
			{
				if(fabsf(val) >= thresholds->value[n])
				{
					fg = statcfg.threshold_fg[n];
					bg = statcfg.threshold_bg[n];
				}
			}
			else
			{
				if(val >= thresholds->value[n])
				{
					fg = statcfg.threshold_fg[n];
					bg = statcfg.threshold_bg[n];
				}
			}
		}
	}

	/* Set the colours only if changed */
	if(XbaeMatrixGetCellColor(statDisplay, row, col) != fg)
		XbaeMatrixSetCellColor(statDisplay, row, col, fg);
	if(XbaeMatrixGetCellBackground(statDisplay, row, col) != bg)
		XbaeMatrixSetCellBackground(statDisplay, row, col, bg);
}


/************* Start read STAT data file section ******************/



/* Get the valid time as unix seconds from the STAT data file.
 */
static Boolean get_valid_time(xmlNodePtr node, time_t *time)
{
	xmlNodePtr cur = NULL;
	for (cur = node; cur; cur = cur->next)
	{
		String sdt;
		struct tm dt;

		if(!KEYNODE(cur,VALIDTIME)) continue;
		sdt = xmlNodeGetContent(cur);
		memset((void*)&dt, 0, sizeof(struct tm));
		strptime(sdt, statcfg.internal_time_format, &dt);
		*time = encode_clock(dt.tm_year+1900, dt.tm_yday+1, dt.tm_hour, dt.tm_min, 0);
		xmlFree(sdt);
		return True;
	}
	return False;
}


/* Get the thresholds associated with the various levels from
 * the STAT data file.
 */
static void get_thresholds(xmlNodePtr node)
{
	int column, level;
	String data_val, slevel;
	xmlNodePtr top = NULL, cur = NULL, data = NULL;

	column = 0;
	for(top = node; top; top = top->next)
	{
		if(!KEYNODE(top,THRESHOLDS)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if(!KEYNODE(cur,THRESHOLD)) continue;
			slevel = xmlGetProp(cur, (const xmlChar*)"level");
			if(!slevel)
			{
				rs_user_message(UM_ERROR, "Unable to get the threshold level.");
				continue;
			}
			level = atoi(slevel) - 1;	/* level is 1 origin */
			xmlFree(slevel);
			if(level < 0 || level >= MAXTHRESH)
			{
				rs_user_message(UM_ERROR, "Illegal element threshold level.");
				continue;
			}
			for(data = cur->children; data; data = data->next)
			{
				if((column = get_element_pos_from_node(data)) < 0) continue;
				data_val = xmlNodeGetContent(data);
				if(data_val)
				{
					/* Any value like '|20|' means use absolute value for comparison. */
					Boolean ok;
					String val = data_val;
					THRESH *thresholds = &statcfg.element[column].thresholds[SCIT_ENVIRON];
					no_white(val);
					if((thresholds->isabs[level] = (*val == '|'))) val++;
					thresholds->value[level] = rs_float_parse(val,&ok);
					if(!ok)
					{
						thresholds->value[level] = NO_THRESHOLD;
						rs_user_message(UM_ERROR, "Threshold parse error for element \'%s\'.",
							statcfg.element[column].id);
					}
					xmlFree(data_val);
					statcfg.element[column].has_threshold = True;
				}
			}
		}
		break;
	}
}



/* Read through all of the elements under a "STORM" xml tree node and populate the arrays.
 */
static Boolean get_storm_data(xmlNodePtr storm_node, int *snum, DELEM *data, DELEM *stat, DELEM *unmod, xmlNodePtr *nodes)
{
	int i;
	String val;
	Boolean ok;
	xmlNodePtr node;
	Boolean set_recover_btn = False;

	/* Initialize all variables to not available */
	memset((void*) data,  0, statcfg.nelement*sizeof(DELEM));
	if (unmod) memset((void*) unmod, 0, statcfg.nelement*sizeof(DELEM));
	if (nodes) memset((void*) nodes, 0, statcfg.nelement*sizeof(xmlNodePtr));
	for(i = 0; i < statcfg.nelement; i++)
	{
		data[i].na  = True;
		if (unmod) unmod[i].na = True;
	}

	for(node = storm_node->children; node; node = node->next)
	{
		int pos = get_element_pos_from_node(node);
		if(pos < 0) continue;

		if(statcfg.element[pos].type == ET_STORM_NUMBER)
		{
			val = xmlNodeGetContent(node);
			i = rs_int_parse(val, &ok);
			if(ok && snum) *snum = i;
			xmlFree(val);
		}
		else if(statcfg.element[pos].type == ET_ENVIRON)
		{
			if (nodes) nodes[pos] = node;
			val = xmlNodeGetContent(node);
			for(i = 0; i < statcfg.nenvironment; i++)
			{
				if(!same(val,statcfg.environment[i].key)) continue;
				data[pos].value = (float) i;
				data[pos].na = False;
				break;
			}
			if(data[pos].na)
				rs_user_message(UM_ERROR,"sEnviron key \'%s\' not defined in the configuration.", val);
			xmlFree(val);
		}
		else if(statcfg.element[pos].type == ET_RANK_CALC)
		{
			if (nodes) nodes[pos] = node;
			if((val = xmlGetProp(node,OFF_PROP)))
				data[pos].off = True;
			else
				val = xmlNodeGetContent(node);
			data[pos].value = rs_float_parse(val, &ok);
			data[pos].na = !ok;
			xmlFree(val);
		}
		else if(statcfg.element[pos].type == ET_RANK_FCST)
		{
			if (nodes) nodes[pos] = node;
			val = xmlGetProp(node,OVERRIDE);
			data[pos].usermod = !blank(val);
			xmlFree(val);
			val = xmlNodeGetContent(node);
			data[pos].value = rs_float_parse(val, &ok);
			data[pos].na = !ok;
			xmlFree(val);
		}
		else if(statcfg.element[pos].data_type == DT_NUMERIC)
		{
			String p;
			val = xmlNodeGetContent(node);
			/*
			 * Special test for direction/speed. We want the speed
			 */
			if((p = strchr(val,'/')))
				data[pos].value = rs_float_parse(p+1, &ok);
			else
				data[pos].value = rs_float_parse(val, &ok);
			data[pos].na = !ok;
			xmlFree(val);
		}
	}

	/* The stat array is set identical to the data array when the file is read. */
	if (stat)  memcpy((void*) stat,  (void*) data, statcfg.nelement*sizeof(DELEM));

	/* If the unmodified STAT data array exists, copy the data array into it
	 * and then scan for the recover key. If found set the value to the recover
	 * value and usermod to True to indicate that a recovery value was found.
	 */
	if (unmod)
	{
		memcpy((void*) unmod, (void*) data, statcfg.nelement*sizeof(DELEM));

		for(node = storm_node->children; node; node = node->next)
		{
			int pos = get_element_pos_from_node(node);
			if(pos < 0) continue;

			if( statcfg.element[pos].type == ET_ENVIRON   ||
				statcfg.element[pos].type == ET_RANK_CALC ||
				statcfg.element[pos].type == ET_RANK_ELEM ||
				statcfg.element[pos].type == ET_RANK_FCST )
			{
				String val;
				if((val = xmlGetProp(node, RECOVER_PROP)))
				{
					Boolean ok;
					unmod[pos].value = rs_float_parse(val, &ok);
					unmod[pos].na = !ok;
					unmod[pos].usermod = True;
					xmlFree(val);
					set_recover_btn = True;
				}
			}
		}
	}

	return set_recover_btn;
}


/* Create an entry in the storm information array for the storm associated with
 * the given storm_node. The storm info is stored in both storm->data
 * storm->stat. They will both be the same on creation but storm->data is subject to
 * change by the forecaster.
 */
static Boolean create_storm_array_entry(xmlNodePtr storm_node, time_t valid_time)
{
	int i, snum = -1;
	Boolean set_recover_btn;
	DELEM *data, *stat, *unmod;
	xmlNodePtr *nodes;

	data = NewMem(DELEM, statcfg.nelement);
	stat = NewMem(DELEM, statcfg.nelement);
	unmod = NewMem(DELEM, statcfg.nelement);
	nodes = NewMem(xmlNodePtr, statcfg.nelement);

	set_recover_btn = get_storm_data(storm_node, &snum, data, stat, unmod, nodes);

	if(snum < 0)
	{
		FreeItem(data);
		FreeItem(stat);
		FreeItem(unmod);
		FreeItem(nodes);
		set_recover_btn = False;
	}
	else
	{
		storm_list = MoreMem(storm_list, STORM, storm_list_len+1);
		storm_list[storm_list_len].num = snum;
		storm_list[storm_list_len].nodes = nodes;
		storm_list[storm_list_len].vtime = valid_time;
		storm_list[storm_list_len].data = data;
		storm_list[storm_list_len].stat = stat;
		storm_list[storm_list_len].unmod = unmod;
		storm_list[storm_list_len].fcst = NULL;
		/*
		 * The first element of the data array is the same as storm.vtime and storm.data
		 */
		storm_list[storm_list_len].nhist = 1;
		storm_list[storm_list_len].hist = OneMem(SDATA);
		storm_list[storm_list_len].hist[0].vtime = storm_list[storm_list_len].vtime;
		storm_list[storm_list_len].hist[0].elem = storm_list[storm_list_len].data;
		storm_list_len++;
	}

	return set_recover_btn;
}


/* If we read the same data file again we do not want to redo the
 * storm array but reread the information for the existing storms.
 */
static Boolean reread_storm_array(xmlNodePtr storm_node)
{
	STORM *storm = rs_get_storm_ptr_from_node(storm_node);
	if(!storm) return False;
	return get_storm_data(storm_node, NULL, storm->data, storm->stat, storm->unmod, storm->nodes);
}



/* Add storm element data to the storm structure array. If first is true then any
 * new storm creates a new array element for the storm. If false this will not be
 * done but any existing storm in the array will have its data section extended.
 * This of course is because only the storms in the table currently being viewed
 * are of interest and only their data need to be stored. If not the first instance
 * and the storm is not added then return False, else return True;
 */
static Boolean add_storm_data(xmlNodePtr storm_node, time_t valid_time)
{
	int i, k;

	STORM *storm = rs_get_storm_ptr_from_node(storm_node);
	if(!storm) return False;

	k = storm->nhist++;
	storm->hist = MoreMem(storm->hist, SDATA, storm->nhist);
	storm->hist[k].vtime = valid_time;
	storm->hist[k].elem = NewMem(DELEM, statcfg.nelement);
	for(i = 0; i < statcfg.nelement; i++)
		storm->hist[k].elem[i].na = True;
	(void) get_storm_data(storm_node, NULL, storm->hist[k].elem, NULL, NULL, NULL);

	return True;
}


/* Free all of the allocated memory used by the storm data array.
 */
static void free_storm_array(void)
{
	int i, n;
	for(i = 0; i < storm_list_len; i++)
	{
		FreeItem(storm_list[i].stat);
		FreeItem(storm_list[i].unmod);
		FreeItem(storm_list[i].nodes);
		for(n = 0; n < storm_list[i].nhist; n++)
			FreeItem(storm_list[i].hist[n].elem);
		FreeItem(storm_list[i].hist);
		for(n = 0; n < statcfg.num_forecasts; n++)
			FreeItem(storm_list[i].fcst[n].elem);
		FreeItem(storm_list[i].fcst);
	}
	FreeItem(storm_list);
	storm_list = NULL;
	storm_list_len = 0;
}


STORM *rs_get_storm_ptr_from_storm_number(int snum)
{
	int n;
	for(n = 0; n < storm_list_len; n++)
	{
		if(storm_list[n].num == snum)
			return &storm_list[n];
	}
	return NULL;
}


/* Return the storm structure associated with the node containing
 * storm information. Run through the node children until the
 * storm id is found and then return the structure pointer.
 */
STORM *rs_get_storm_ptr_from_node(xmlNodePtr storm_node)
{
	int snum = 0;
	String id;
	xmlNodePtr data;

	for(data = storm_node->children; data; data = data->next)
	{
		if(!KEYNODE(data,STORM_NUMBER)) continue;
		id = xmlNodeGetContent(data);
		snum = atoi(id);
		xmlFree(id);
		break;
	}
	return rs_get_storm_ptr_from_storm_number(snum);
}


/* Determine if the element in the column ncol is forecast to reach
 * severe. A sequence of statcfg.num_fcst_data_points storm instances is
 * required to do the forecast.
 */
static Boolean storm_elem_fcst_severe(STORM *storm, int ncol)
{
	int i;
	THRESH *thresholds;

	if(!storm) return False;
	if(!elemp[ncol]->flag_severe) return False;
	if(storm->nhist < statcfg.num_fcst_data_points) return False;

	thresholds = rs_active_thresholds(elemp[ncol], storm, SEVERE_THRESHOLD_NDX);
	for(i = 0; i < statcfg.num_forecasts; i++)
	{
		int pos = matrix_column_to_element_array_pos(ncol);
		float val = storm->fcst[i].elem[pos].value;
		if(val >= thresholds->value[SEVERE_THRESHOLD_NDX] ||
			(thresholds->isabs[SEVERE_THRESHOLD_NDX] &&
			 fabsf(val) >= thresholds->value[SEVERE_THRESHOLD_NDX]))
		{
			return True;
		}
	}
	return False;
}


/* Sets the highlight around the specified cell if the associated storm element
 * forecast value exceeds the severe threshold in the next 30 minutes.
 */
static void set_cell_highlight(Widget w, STORM *storm, int row, int col)
{
	THRESH *thresholds = rs_active_thresholds(elemp[col], storm, SEVERE_THRESHOLD_NDX);

	if(storm != NULL && storm->nhist >= statcfg.num_fcst_data_points && elemp[col]->flag_severe)
	{
		int i;
		for(i = 0; i < statcfg.num_forecasts; i++)
		{
			int pos = matrix_column_to_element_array_pos(col);
			float val = storm->fcst[i].elem[pos].value;
			if(val >= thresholds->value[SEVERE_THRESHOLD_NDX] ||
				(thresholds->isabs[SEVERE_THRESHOLD_NDX] &&
				 fabsf(val) >= thresholds->value[SEVERE_THRESHOLD_NDX]))
			{
				XbaeMatrixHighlightCell(w, row, col);
				return;
			}
		}
	}
	XbaeMatrixUnhighlightCell(w, row, col);
}


/* Get data from storms previous to table currently being displayed
 * and store it in the storms structure array.
 */
static void get_previous_storms_data(void)
{
	int    pos = 0;
	String data_val;
	xmlDocPtr stat = NULL;
	xmlNodePtr root = NULL, cur = NULL, data = NULL;
	time_t prev_time = requested_time;

	if(!current_fname) return;

	/* Note that filelist[0] contains the oldest file */
	if(InList(current_fname, nfilelist, filelist, &pos))
	{
		Boolean storm_found = True;

		/* Decrement as pos is the current storm */
		pos--;
		while(pos >= 0 && storm_found)
		{
			time_t valid_time = file_time(filelist[pos]);
			storm_found = False;
			/* Check for valid maximum time interval. max_file_time_diff is in minutes */
			if(prev_time - valid_time <= (time_t) (statcfg.max_file_time_diff*60))
			{
				stat = xmlReadFile(pathname(stat_data_dir,filelist[pos]), NULL, 0);
				if(!stat) continue;
				if((root = xmlDocGetRootElement(stat)))
				{
					for (cur = root->children; cur; cur = cur->next)
					{
						if(KEYNODE(cur,STORM_KEY))
						{
							if(add_storm_data(cur, valid_time))
								storm_found = True;
						}
					}
				}
				xmlFreeDoc(stat);
			}
			prev_time = valid_time;
			pos--;
		}
	}
}


/* Produce a set of forecasts for each element. Note that rank weight is
 * a special case in that the forecasts are already in the STAT file and
 * thus are not calculated. Forecasts are also not produced for the forecast
 * rank weight elements.
 */
static String storm_element_forecast(void)
{
	int j, n;

	for(n = 0; n < storm_list_len; n++)
	{
		STORM *storm = storm_list + n;
		storm->fcst = NewMem(SDATA, statcfg.num_forecasts);
		for(j = 0; j < statcfg.num_forecasts; j++)
		{
			storm->fcst[j].elem = NewMem(DELEM, statcfg.nelement);
		}
	}
	rs_make_all_storms_element_trend_forecasts();
	/*
	 * Rank weight is a special case as the forecast values are taken from the
	 * rank weight forecast elements. These are produced by rankweightDaemon.
	 */
	for(n = 0; n < storm_list_len; n++)
	{
		int dt;
		STORM *storm = storm_list + n;
		for(dt = statcfg.time_interval, j = 0; j < statcfg.num_forecasts; j++, dt += statcfg.time_interval)
		{
			int m, k;
			k = rs_get_element_array_pos_from_id(statcfg.rankweight.element_id);
			if((m = rs_get_rankweight_element_array_pos_from_time(dt)) >= 0)
			{
				storm->fcst[j].elem[k].value = storm->data[m].value;
				storm->fcst[j].elem[k].na = storm->data[m].na;
			}
		}
	}
}


/* Extract data from the xml data file and put it into the storm structure.
 * Once this is done, forecasts of the data elements involved in calculating
 * the rank weight are done. The rank weight forecasts which should be in the
 * file are inserted into the forecast array of the rank weight element.
 */
static void extract_storm_data(xmlNodePtr node, Boolean newfile)
{
	xmlNodePtr cur = NULL;
	Boolean set_recover_btn = False;
	if(newfile)
	{
		free_storm_array();
		for (cur = node; cur; cur = cur->next)
		{
			if(!KEYNODE(cur,STORM_KEY)) continue;
			if(create_storm_array_entry(cur, requested_time))
				set_recover_btn = True;
		}
		get_previous_storms_data();
	}
	else
	{
		for (cur = node; cur; cur = cur->next)
		{
			if(!KEYNODE(cur,STORM_KEY)) continue;
			if(reread_storm_array(cur))
				set_recover_btn = True;
		}
	}
	storm_element_forecast();
	XtSetSensitive(recoverBtn, set_recover_btn);
}


/* Get the storm data from the STAT data file and display it in
 * the matrix. Any data which can be modified is taken from the
 * storm array;
 */
static void display_all_storms(void)
{
	int n, nrow = 0, ncolumn = 0;
	String id, data_val;
	Pixel fg, bg;
	STORM *storm;
	xmlNodePtr root, node, cur = NULL, data = NULL;

	root = xmlDocGetRootElement(docp);

	/* Initialize elements assuming no data will be found */
	for(n = 0; n < statcfg.nelement; n++)
		statcfg.element[n].has_data = False;

	XbaeMatrixDisableRedisplay(statDisplay);

	/* Add or remove rows as necessary. This is done to minimize
	 * the amount of visual 'flashing' that otherwise occurs.
	 */
	nrow = XbaeMatrixNumRows(statDisplay);
	if(storm_list_len < nrow)
		XbaeMatrixDeleteRows(statDisplay, storm_list_len, nrow - storm_list_len);
	else if(storm_list_len > nrow)
		XbaeMatrixAddRows(statDisplay, nrow, NULL, NULL, NULL, storm_list_len - nrow);

	/* Clear all cells */
	for(ncolumn = 0; ncolumn < nelemp; ncolumn++)
	{
		for(nrow = 0; nrow < storm_list_len; nrow++)
		{
			XbaeMatrixSetCell(statDisplay, nrow, ncolumn, NULL);
			clear_cell_threshold_color(nrow, ncolumn);
		}
	}

	/* Scan storms again and display in the matrix */
	for(nrow = 0, cur = root->children; cur; cur = cur->next)
	{
		if(!KEYNODE(cur,STORM_KEY)) continue;

		storm = rs_get_storm_ptr_from_node(cur);
		if(!storm) continue;

		/* Set the storm pointer into the row user data */
		XbaeMatrixSetRowUserData(statDisplay, nrow, (XtPointer) storm);

		for(data = cur->children; data; data = data->next)
		{
			if(data->type != XML_ELEMENT_NODE) continue;
			if((ncolumn = get_matrix_column_from_id((String) data->name)) < 0) continue;

			data_val = xmlNodeGetContent(data);
			if(!data_val)
			{
				XbaeMatrixSetCell(statDisplay, nrow, ncolumn, "");
				clear_cell_threshold_color(nrow, ncolumn);
			}
			else
			{
				int pos = matrix_column_to_element_array_pos(ncolumn);
				elemp[ncolumn]->has_data = True;
				/*
				 * If element is modifiable take value from storms array
				 */
				if( elemp[ncolumn]->type == ET_RANK_CALC ||
					elemp[ncolumn]->type == ET_RANK_ELEM ||
					elemp[ncolumn]->type == ET_RANK_FCST  )
				{
					String ptr = print_value(storm, pos);
					XbaeMatrixSetCell(statDisplay, nrow, ncolumn, ptr);
					set_cell_threshold_color(nrow, ncolumn);
					set_cell_highlight(statDisplay, storm, nrow, ncolumn);
				}
				else if(elemp[ncolumn]->type == ET_ENVIRON)
				{
					int ndx = NINT(storm->data[pos].value);
					XbaeMatrixSetCell(statDisplay, nrow, ncolumn,
							statcfg.environment[ndx].label);
				}
				else
				{
					XbaeMatrixSetCell(statDisplay, nrow, ncolumn, data_val);
					set_cell_threshold_color(nrow, ncolumn);
					set_cell_highlight(statDisplay, storm, nrow, ncolumn);
				}
			}
			xmlFree(data_val);
		}
		nrow++;
	}
	XbaeMatrixEnableRedisplay(statDisplay, True);
}



/* Redisplay a storm which is already displayed in the matrix. This just
 * refreshes the one row with the storm in it and avoids display flashing.
 */
static void redisplay_storm(STORM *instorm)
{
	int n, row, col;
	String id, data_val;
	Pixel fg, bg;
	STORM *storm;
	xmlNodePtr root, node, cur = NULL, data = NULL;

	for(row = 0; row < XbaeMatrixNumRows(statDisplay); row++)
	{
		STORM *s = (STORM *) XbaeMatrixGetRowUserData(statDisplay, row);
		if(s == instorm) break;
	}
	if(row >= XbaeMatrixNumRows(statDisplay)) return;

	XbaeMatrixDisableRedisplay(statDisplay);
	root = xmlDocGetRootElement(docp);

	for(cur = root->children; cur; cur = cur->next)
	{
		if(!KEYNODE(cur,STORM_KEY)) continue;

		storm = rs_get_storm_ptr_from_node(cur);
		if(!storm || storm != instorm) continue;

		for(data = cur->children; data; data = data->next)
		{
			if(data->type != XML_ELEMENT_NODE) continue;
			if((col = get_matrix_column_from_id((String) data->name)) < 0) continue;
			/*
			 * If element is modifiable take value from storms array
			 */
			if( elemp[col]->type == ET_RANK_CALC ||
				elemp[col]->type == ET_RANK_ELEM ||
				elemp[col]->type == ET_RANK_FCST  )
			{
				int pos = matrix_column_to_element_array_pos(col);
				data_val = print_value(storm, pos);
				XbaeMatrixSetCell(statDisplay, row, col, data_val);
				set_cell_threshold_color(row, col);
				set_cell_highlight(statDisplay, storm, row, col);
			}
			else if(elemp[col]->type == ET_ENVIRON)
			{
				int pos = matrix_column_to_element_array_pos(col);
				XbaeMatrixSetCell(statDisplay, row, col,
						statcfg.environment[NINT(storm->data[pos].value)].label);
			}
			else
			{
				data_val = xmlNodeGetContent(data);
				if(!data_val)
				{
					XbaeMatrixSetCell(statDisplay, row, col, "");
					clear_cell_threshold_color(row, col);
				}
				else
				{
					XbaeMatrixSetCell(statDisplay, row, col, data_val);
					set_cell_threshold_color(row, col);
					set_cell_highlight(statDisplay, storm, row, col);
					xmlFree(data_val);
				}
			}
		}
		break;
	}
	XbaeMatrixEnableRedisplay(statDisplay, True);
}



/* Read the STAT data file and extract the information. If fname
 * is current_fname then reread the file.
 */
static Boolean read_stat_data(String fname)
{
	char buf[200];
	String val;
	time_t valid_time;
	struct stat sbuf;
    xmlNodePtr root = NULL;
	Boolean new_file = (fname != current_fname);

	rs_user_message(UM_STATUS, NULL);
	if(blank(fname)) return False;

	XtSetSensitive(recoverBtn, False);
	if(new_file)
		FreeItem(current_fname);

	free_doc();

	if((docp = xmlReadFile(pathname(stat_data_dir,fname), NULL, 0)) == NULL)
	{
		rs_user_message(UM_ERROR, "Could not parse file %s.", fname);
		return False;
	}

	root = xmlDocGetRootElement(docp);
	if(root == NULL)
	{
		rs_user_message(UM_ERROR, "File \'%s\' is empty.", fname);
		free_doc();
		return False;
	}
	if(!NODENAME(root,ROOT_ID))
	{
		rs_user_message(UM_ERROR, "File \'%s\' is not a \'%s\" table.", fname, ROOT_ID);
		free_doc();
		return False;
	}

	val = xmlGetProp(root,TYPE_PROP);
	current_is_fcst = (!blank(val) && NotNull(strstr(val,FCST_KEY)));
	xmlFree(val);

	rs_user_message(UM_STATUS,"");

	if(new_file)
		current_fname = XtNewString(fname);
	if(stat(pathname(stat_data_dir,fname),&sbuf) == 0)
		current_mod_time = sbuf.st_mtime;

	/* Get, display and validate internal valid time */
	valid_time = requested_time;
	if(!get_valid_time(root->children, &valid_time))
		rs_user_message(UM_ERROR, "The valid time was not found in the file: %s", fname);
	else if(requested_time != valid_time)
		rs_user_message(UM_ERROR, "File name timestamp and internal timestamp are different: %s", fname);
	strftime(buf, sizeof(buf), statcfg.user_time_format, gmtime(&valid_time));
	XuWidgetLabel(timeLabel, buf);

	get_thresholds(root->children);
	extract_storm_data(root->children, new_file);
	display_all_storms();
	if(sort_on_column >= 0)
		XbaeMatrixSortRows(statDisplay, row_sort_fcn, INT2PTR(sort_on_column));
	XbaeMatrixResizeColumnsToCells(statDisplay, True);
	resize_selected_storm_display();

	return True;
}


/************ Start of data manipulation section ****************/


/* If any values have changed between the xml file and the radar
 * matrix return True, else return False. Save the original of
 * any modified values.
 */
static void set_saveBtn_state(void)
{
	int ndx, n;
	Boolean state = False;

	if(!docp || current_is_fcst)
	{
		XtSetSensitive(saveBtn, False);
		return;
	}

	for(n = 0; n < storm_list_len; n++)
	{
		for(ndx = 0; ndx < statcfg.nelement; ndx++)
		{
			if(!storm_list[n].nodes[ndx] || same_storm_value(&storm_list[n], ndx)) continue;
			state = True;
			break;
		}
	}
	XtSetSensitive(saveBtn, state);
	XtSetSensitive(undoBtn, state);
}


/* Return the node pointer for the storm with the given id in the xml
 * tree defined by root.
 */
static xmlNodePtr ref_storm_node(xmlNodePtr root, String storm_id)
{
	Boolean found;
	String id;
	xmlNodePtr cur, data;

	for (cur = root->children; cur; cur = cur->next)
	{
		if(!KEYNODE(cur,STORM_KEY)) continue;
		for(data = cur->children; data; data = data->next)
		{
			if(!KEYNODE(data,STORM_NUMBER)) continue;
			id = xmlNodeGetContent(data);
			found = (xmlStrcmp(id, storm_id) == 0);
			xmlFree(id);
			if(found) return cur;
			break;
		}
	}
	return NULL;
}


/* Return the node pointer for the element found in the storm tree defined by
 * the parameter storm.
 */
static xmlNodePtr ref_element_node(xmlNodePtr storm, const xmlChar *element)
{
	xmlNodePtr data;
	for(data = storm->children; data; data = data->next)
	{
		if(KEYNODE(data,element)) return data;
	}
	return NULL;
}


/* This function sets off or on all elements in the given file fname
 * that are off or on in the currently active file and sets modified
 * storm environments. This operation is done on a save. If no changes
 * are made the return is False else the return is True;
 */
static Boolean propogate_element_state(String mod_file)
{
	Boolean rtn_status = False;
	xmlDocPtr  mod_doc = NULL;
	xmlNodePtr ref_root = NULL, mod_root = NULL;
	xmlNodePtr cur = NULL, data = NULL;
	xmlNodePtr ref_storm;

	if(!docp) return False;
	ref_root = xmlDocGetRootElement(docp);

	mod_doc = xmlReadFile(pathname(stat_data_dir,mod_file), NULL, 0);
	if(!mod_doc) return False;
	mod_root = xmlDocGetRootElement(mod_doc);

	if(ref_root && mod_root)
	{
		for (cur = mod_root->children; cur; cur = cur->next)
		{
			ref_storm = NULL;
			if(!KEYNODE(cur,STORM_KEY)) continue;
			/*
			 * Find the storm id and then find reference equivalent
			 */
			for(data = cur->children; data; data = data->next)
			{
				if(KEYNODE(data,STORM_NUMBER))
				{
					String id = xmlNodeGetContent(data);
					ref_storm = ref_storm_node(ref_root, id);
					xmlFree(id);
					break;
				}
			}

			if(!ref_storm) continue;

			/* Scan again through all of the elements for the storm
			 * and again find the reference equivalent
			 */
			for( data = cur->children; data; data = data->next)
			{
				xmlNodePtr node;

				if(data->type != XML_ELEMENT_NODE) continue;
				node = ref_element_node(ref_storm, data->name);
				if(!node) continue;
				/*
				 * There are two cases to propogate, storm environment
				 * changes and elements turned off or on.
				 */
				if(xmlStrcmp(data->name,STORM_ENV_ID) == 0)
				{
					String mod_env, ref_env;
					mod_env = xmlNodeGetContent(data);
					ref_env = xmlNodeGetContent(node);
					if(xmlStrcmp(mod_env, ref_env))
					{
						String recover_val = xmlGetProp(data, RECOVER_PROP);
						if(xmlStrcmp(recover_val, ref_env) == 0)
							xmlUnsetProp(data, RECOVER_PROP);
						xmlNodeSetContent(data, ref_env);
						rtn_status = True;
					}
					xmlFree(mod_env);
					xmlFree(ref_env);
				}
				else
				{
					String mod_offval, ref_offval;
					mod_offval = xmlGetProp(data, OFF_PROP);
					ref_offval = xmlGetProp(node, OFF_PROP);
					/*
					 * If the element is not off in either continue
					 */
					if(!mod_offval && !ref_offval) continue;
					/*
					 * If the element is not on in both process the file
					 */
					if(!(mod_offval && ref_offval))
					{
						String recover_val = xmlGetProp(data, RECOVER_PROP);
						rtn_status = True;
						if(mod_offval)
						{
							/* File is off and matrix is on so turn file on */
							xmlNodeSetContent(data, mod_offval);
							xmlUnsetProp(data, OFF_PROP);
							if(recover_val != NULL && same(recover_val,mod_offval))
								xmlUnsetProp(data, RECOVER_PROP);
						}
						else
						{
							/* Matrix is off and file is on so turn file off. The
							 * recovery value is only set on the first file change
							 */
							String mod_val;
							mod_val = xmlNodeGetContent(data);
							if(!recover_val) xmlSetProp(data, RECOVER_PROP, mod_val);
							xmlSetProp(data, OFF_PROP, mod_val);
							xmlNodeSetContent(data, DATA_NA);
							xmlFree(mod_val);
						}
						xmlFree(recover_val);
					}
					xmlFree(mod_offval);
					xmlFree(ref_offval);
				}
			}
		}
	}
	/* Save xml data if there were any changes to the data */
	if(rtn_status)
		xmlSaveFormatFile(pathname(stat_data_dir,mod_file), mod_doc, 1);

	xmlFreeDoc(mod_doc);
	return rtn_status;
}


/* Update the storm data to what is in storm->data
 * and save the data to the current file.
 */
static void update_xml_doc(void)
{
	int have_recover = False;
	int col, row, pos;

	for(row = 0; row < XbaeMatrixNumRows(statDisplay); row++)
	{
		STORM *storm = (STORM *) XbaeMatrixGetRowUserData(statDisplay, row);
		for(col = 0; col < statcfg.nelement; col++)
		{
			String xmlval, prop;
			String new_content = NULL;
			xmlNodePtr node = storm->nodes[col];

			if(!node) continue;
			if(statcfg.element[col].type == ET_VIEW) continue;
			if(statcfg.element[col].type == ET_STORM_NUMBER) continue;
			/*
			 * Get the current value of the node content before any
			 * changes are made. Used for setting the recovery proc.
			 */
			xmlval = xmlNodeGetContent(node);
			/*
			 * Check for a manual override of the value and
			 * set or remove the override property.
			 */
			prop = xmlGetProp(node, OVERRIDE);
			if(storm->data[col].usermod)
			{
				if(!prop) xmlSetProp(node,OVERRIDE,"forecaster");
			}
			else if(prop)
			{
				xmlUnsetProp(node, OVERRIDE);
			}
			xmlFree(prop);
			/*
			 * The environment column needs special processing and
			 * it can never be set to off.
			 */
			if(statcfg.element[col].type == ET_ENVIRON)
			{
				int n = NINT(storm->data[col].value);
				new_content = statcfg.environment[n].key;
				xmlNodeSetContent(node, new_content);
			}
			else 
			{
				if(storm->data[col].off)
				{
					char buf[32];
					if(storm->data[col].na)
						strcpy(buf, DATA_NA);
					else
						snprintf(buf, 32, "%.*f", statcfg.element[col].ndecimals, storm->data[col].value);
					xmlSetProp(node, OFF_PROP, buf);
					new_content = DATA_NA;
				}
				else
				{
					xmlUnsetProp(node, OFF_PROP);
					new_content = print_value(storm, col);
				}
				xmlNodeSetContent(node, new_content);
			}
			/*
			 * If a data recovery property does not exist and the data
			 * has changed, save the original value for recovery.
			 */
			if(statcfg.element[col].data_type == DT_STRING)
			{
				prop = xmlGetProp(node, RECOVER_PROP);
				if(!prop)
				{
					if(!same(xmlval,new_content))
					{
						xmlSetProp(node, RECOVER_PROP, xmlval);
						have_recover = True;
					}
				}
				else if(same(prop,new_content))
				{
					xmlUnsetProp(node,RECOVER_PROP);
				}
				else
				{
					have_recover = True;
				}
				xmlFree(prop);
			}
			else if(statcfg.element[col].data_type != DT_NONE)
			{
				if(storm->unmod[col].usermod)
				{
					if(same_values(storm->unmod[col].value, storm->data[col].value, col))
					{
						xmlUnsetProp(node,RECOVER_PROP);
						xmlUnsetProp(node, RATIO_PROP);
					}
					else
					{
						set_ratio_prop(storm, col);
						have_recover = True;
					}
				}
				else
				{
					if(same_values(storm->stat[col].value, storm->data[col].value, col))
					{
						xmlUnsetProp(node, RATIO_PROP);
					}
					else
					{
						xmlSetProp(node, RECOVER_PROP, xmlval);
						set_ratio_prop(storm, col);
						have_recover = True;
					}
				}
			}
			xmlFree(xmlval);
			/*
			 * Set the stat file values to the storm->data values
			 */
			storm->stat[col].value   = storm->data[col].value;
			storm->stat[col].off     = storm->data[col].off;
			storm->stat[col].na      = storm->data[col].na;
			storm->stat[col].usermod = storm->data[col].usermod;
		}
	}

	xmlSaveFormatFile(pathname(stat_data_dir,current_fname), docp, 1);

	/* Propogate any turned off/on elements or storm environment
	 * changes to all other files.
	 */
	if(InList(current_fname, nfilelist, filelist, &pos))
	{
		int n;
		for(n = pos-1; n >= 0; n--)
			if(!propogate_element_state(filelist[n])) break;
		for(n = pos+1; n < nfilelist; n++)
			if(!propogate_element_state(filelist[n])) break;
	}

	/* Reread the file so as to update the storm structure */
	read_stat_data(current_fname);
	activate_storm_trend_dialog(REDISPLAY);
	XtSetSensitive(recoverBtn, have_recover);
	set_saveBtn_state();
}


/************ End of data manipulation section ****************/


static void update_filelist(void)
{
	FREELIST(filelist, nfilelist);
	nfilelist = 0;
	dirlist_reuse(False);
	nfilelist = dirlist(stat_data_dir, statcfg.file_mask, &filelist);
	dirlist_reuse(True);
}


/* Free up the xml parser resources */
static void free_doc(void)
{
	if(!docp) return;
	xmlFreeDoc(docp);
	docp = NULL;
}


/* Exit the dialog and clean up
 */
static void exit_cb(Widget w, XtPointer un1, XtPointer un2)
{
	if(!dialog) return;
	DeleteObserver(OB_DEPICTION_CHANGE, depiction_change_observer);
	save_data_check();
	free_doc();
	FreeItem(current_fname);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}
