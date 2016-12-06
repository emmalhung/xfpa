/****************************************************************************/
/*
*  File:     main.c 
*
*  Purpose:  Provides the GUI program interface for FPA to
*            control the interactive graphics library (Ingred).
*
* Run Time Parameters:
*
*  -profile <key> - Start using the specified profile instead of putting
*                   up a dialog asking the user for a profile. "none" is
*                   a valid input.  XuVaAppInitialize() handles this so
*                   that there is no visible code in this file.
*
*    -askForTimes - Put up a dialog asking users for which range of depiction
*                   times they which to read into Ingred.
*
*  -debug <level> - Control the level state of the error fcns.
*                   The level consists of two digits, xy, where x and y
*                   range fro 0 to 5. x corresponds to the output level and
*                   y to the error level (see tools/message.c for details).
*                   If no level is given or the level is unrecognized the
*                   default is 55.
*
*  -s[etup] <setup_file> - Specifies the name of the file to use
*                   as the local setup file.  May be an absolute path name
*                   or relative to the default setup directories.
*
*       -stateDir - The directory to use to store the state store file. This
*                   is only used if in viewer mode.
*
* -t0 <date-time> - Specifies the time to use as T0 for the depiction sequence.
*
* -v[isible] <key> - Bring up FPA with fields visible as specified by the
*                   block of data indicated by the [key] in the preset  field
*                   visibility config file.
*
*        +iconic - Bring up in non-iconic mode.
*
*     +viewerMode - Run the Fpa in viewer mode.  All import, delete, save and
*                   edit commands are made inactive so only sequence viewing
*                   is available.
*
* Standard X program parameters are also supported. Notable are:
*
*  -xnllangage <language string> - sets directory where to find
*                   language resource files. Thus if we had a c-french resource
*                   directory under app-defaults and set xnllanguage to c-french
*                   all of those items that do a search path will find their
*                   resouces in the c-french directory.
*
*  Environment Variables:
*
*         FPA_LANG - has the same effect as the xnllanguage parameter.
*         LANG     - If FPA_LANG is not set then LANG is used to set the
*                    resource search path.
*
* ------------------------------------------------------------------------                   
*
*  Notes:
*
* 1. All times specified above must be in the formay yyyy:jjj:hh where yyyy
*    is the year, jjj the julian day and hh the hour.
*
* 2. This file contains the code to create the main window and all of the
*    main widgets assiciated with this window.  The action functions for
*    these widgets are also defined here.  This includes the main pulldown
*    menu, the depiction control button bar, and the drawing area manager
*    and drawing area widgets.
*
****************************************************************************
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/signal.h>

#define  FPA_MAIN
#include "global.h"
#undef  FPA_MAIN

#include <ingred.h>
#include <Xm/ArrowB.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawnB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <X11/xpm.h>
#include <Xm/ButtonBox.h>
#include <graphics.h>

/* Include all of the private header files.
*/
#include "alliedModel.h"
#include "contextMenu.h"
#include "depiction.h"
#include "editor.h"
#include "fcstText.h"
#include "fpapm.h"
#include "graphic.h"
#include "guidance.h"
#include "help.h"
#include "iconBar.h"
#include "imagery.h"
#include "radarSTAT.h"
#include "menu.h"
#include "observer.h"
#include "pointFcst.h"
#include "productStatus.h"
#include "resourceDefines.h"
#include "preferences.h"
#include "timelink.h"
#include "userReport.h"
#include "fallback.h"

/* The maximum value of the time scroll bar
 */
#define TSBMS 1000

/* Forward function declarations
 */
static void    colormap_change_handler (Widget, XtPointer, XEvent*);
static void    create_edit_window      (void);
static void    create_view_window      (void);
static void    fatal_error_handler     (int);
static void    ingred_commands         (String, CAL);
static void    ingred_set_cursor       (String, Boolean);
static void    init_products           (void);
static void    init_working_directory  (void);
static Boolean main_initialized_notify (XtPointer);
static void    post_message            (String, String);
static String  language_proc           (Display*, String, XtPointer);
/*
 * Local static variables
 */
static Widget   depictionTimeDisplay = NullWidget;
static Widget   timeScrollBar        = NullWidget;
static Widget   seqForm              = NullWidget;
static Widget   buttonBar            = NullWidget;
static Widget   iconBar              = NullWidget;
static Boolean  show_scratchpad      = False;
static String   my_argv0             = NULL;
static int      iconbar_location     = TB_TOP;
static int      timebar_location     = TB_TOP;
static int      message_location     = TB_TOP;
static Boolean  msg_show             = True;
static Boolean  allow_busy_cursor    = False;
static Boolean  initialized          = False;
static String   delayed_message      = NULL;


/* Set our command line options.
*/
static XrmOptionDescRec options[] = {
	{ "+iconic", ".iconic", XrmoptionNoArg, "off" }
};

/* Structure that defines the widgets, pixmaps and actions to be found on the icon bar.
 */
typedef struct _icon_data {
	String      setupName;	/* Identifier used in the setup file */
	String      icon_file;	/* file containing the icon image */
	String      name;		/* widget name */
	Widget      w;			/* created widget */
	int         action_id;	/* what to use as data in the callback function */
	Dimension   left_space;	/* space to left of button - used by xrt */	
	Dimension   margin;		/* margin width - used by ButtonBox */
	Boolean     sensitive;	/* sensitive on creation? */
	Boolean     manage;     /* managed on creation? */
} ICON_DATA;


/* These are used in a generic fashion in the code and are set to the appropriate
 * data in the interface generation functions.
 */
static ICON_DATA *icon_data = NULL;
static int       nicon_data = 0;


int main( int argc, char **argv )
{ 
	int    i, olevel, elevel;
	char   mbuf[300];
	String tpath;
	String preset_vis_key, ptr, setupfile, state_dir, font_path, state_path;
	String rev_label;
	Boolean ask_for_times;
	Boolean no_title = True;
	Boolean diag_enable = False;
	int diag_level = 3;
	int diag_style = 0;
	
	/* Check for command line overrides
	*/
	ask_for_times   = False;
	my_argv0        = argv[0];
	GV_T0_depict[0] = '\0';
	preset_vis_key  = NULL;
	state_dir       = "/tmp";

	GV_app_name  = "xfpa";
	GV_app_class = "XFpa";
	rev_label    = FpaRevLabel;

	(void) signal(SIGCLD, SIG_IGN);
	diag_control(False, diag_level, diag_style);

	for( i = 1; i < argc; i++ )
	{
		if(same_ic(argv[i],"-debug"))
		{
			_Xdebug = 1;
			diag_enable = True;
			diag_level = -1;
			diag_style = -1;
			diag_control(False, diag_level, diag_style);
			if( i >= argc-1 ) break;
			if( argv[i+1][0] == '-' || argv[i+1][0] == '+') continue;
			i++;
			olevel = argv[i][0] - '0';
			if(olevel < 0 || olevel > 5) continue;
			elevel = argv[i][1] - '0';
			if(elevel < 0 || elevel > 5) continue;
			olevel = MAX(olevel, elevel);
			diag_enable = True;
			diag_level = olevel;
			diag_style = -1;
			diag_control(False, diag_level, diag_style);
		}
		else if(same_ic(argv[i],"-display"))
		{
			i++;
			if( i == argc ) break;
			strcpy(mbuf, "DISPLAY=");
			strcat(mbuf, argv[i]);
			ptr = XtNewString(mbuf);
			(void) putenv(ptr);
		}
		else if(same_ic(argv[i],"-askForTimes"))
		{
			ask_for_times = True;
		}
		else if(same_ic(argv[i],"-t0"))
		{
			i++;
			if( i == argc ) break;
			strcpy(GV_T0_depict, argv[i]);
			strcpy(mbuf, "FPA_DEMO_DATE=");
			strcat(mbuf, argv[i]);
			ptr = XtNewString(mbuf);
			(void) putenv(ptr);
		}
		else if(strncmp(argv[i],"-v",2) == 0)
		{
			i++;
			if( i == argc ) break;
			preset_vis_key = argv[i];
		}
		else if(same_ic(argv[i],"+viewerMode"))
		{
			GV_edit_mode = False;
			diag_enable = False;
			diag_level = 0;
			diag_style = 0;
			diag_control(False, diag_level, diag_style);
		}
		else if(same_ic(argv[i],"-stateDir"))
		{
			i++;
			if( i == argc ) break;
			state_dir = argv[i];
		}
		else if(same_ic(argv[i],"-title"))
		{
			i++;
			no_title = False;
		}
	}

	/* Set auto-flushing on, thus no need for fflush() calls.
	*/
	setvbuf(stdout, NULL,_IOLBF,0);
	setvbuf(stderr, NULL,_IOLBF,0);

	/* To trap errors create the message dialogs and then set the trap functions.
	*/
	set_error_trap(fatal_error_handler);

	(void) XtSetLanguageProc(NULL, language_proc, NULL);

	/* Note that font_path must not be null but blank if undefined
	 */
	tpath = getenv("FPA_FONT_PATH");
	if (!blank(tpath)) font_path = safe_strdup(tpath);
	else font_path = "";

	/* Make sure we have a license. This must be called before any other
	*  library functions.
	*/
	GetLicense(argc, argv, font_path);

	/* Read the FPA setup and config files. This needs to be done now as some
	 * information is requred to set the state file location.
	 */
	setupfile = GetSetupFile(argc,argv);
	if(!setupfile) exit(1);

	/* Determine the global (default) state store file location. If we are in viewer
	 * mode but have an explicit directory to store it in (state_dir) we can give
	 * write permission.
	 */
	if( !GV_edit_mode && state_dir )
	{
		(void) snprintf(mbuf, sizeof(mbuf), ".state_store_%s", base_name(setupfile,NULL));
		state_path = pathname(state_dir, mbuf);
	}
	else
	{
		state_path = source_path_by_name(DEPICT, NULL, NULL, ".state_store");
	}

	/* Create the shell. Note that the state store file location must have been
	 * determined before this point so that the profile information can be read.
	 */
	GW_topLevel = XuVaAppInitialize(&GV_app_context, GV_app_class,
		options, XtNumber(options),
		&argc, argv,
		fallback_resources,
		XuNiconPixmapFile, "fpalogo",
		XuNiconMaskFile, "fpalogo_m",
		XuNmwmDeleteOverride, MainExitCB,
		XuNallowProfileSelection, GV_edit_mode,
		XuNfontPath, font_path,
		XuNstateFile, state_path,
		XuNstateFileEditable, (GV_edit_mode || (!GV_edit_mode && state_dir)),
		XuNdefaultActionItemId, "cancelBtn,closeBtn",
		NULL);

	/* Handle the change of colormap event. This is only needed if we are in viewer mode.
	 */
	if(!GV_edit_mode)
	{
		XtAddEventHandler(GW_topLevel, ColormapChangeMask, False, (XtEventHandler)colormap_change_handler, NULL);
	}

	/* Set the default font from the resource file and override the map fonts.
	 */
	glSetDefaultFont(XuGetStringResource(RNdefaultFont,NULL));
	gxReplacePredefinedFonts(XuGetStringResource(RNdefaultMapFonts,NULL));

	/* Set diagnostic levels
	 */
	if(diag_enable)
	{
		diag_control(True, diag_level, diag_style);
	}

	/* We now read the complete config file to ensure that all is ok. We do it
	*  here rather than sooner to allow the interface to come up. Looks better.
	*/
	if(!read_complete_config_file())
	{
		XuShowError(GW_topLevel, "NoConfig", setupfile);
		exit(1);
	}

	/* Checks to make sure that our base directories exist as
	 * determined by the configuration.
	 */
	if(blank(source_directory_by_name(FpaDir_Depict,NULL,NULL)))
	{
		XuShowError(GW_topLevel, "NoDepict", NULL);
		exit(1);
	}
	if(blank(source_directory_by_name(FpaDir_Interp,NULL,NULL)))
	{
		XuShowError(GW_topLevel, "NoInterp", NULL);
		exit(1);
	}
	if(blank(source_directory_by_name(FpaDir_Backup,NULL,NULL)))
	{
		XuShowError(GW_topLevel, "NoBackup", NULL);
		exit(1);
	}


	/* Check to see that the use of minutes and file names is consistent.
	*/
	if(!check_depiction_minutes()) exit(1);
	if(!check_source_minutes_in_filenames(FpaDir_Depict)) exit(1);
	if(!check_source_minutes_in_filenames(FpaDir_Interp)) exit(1);
	if(!check_source_minutes_in_filenames(FpaDir_Backup)) exit(1);

	/* Put in our application name. This can come from either the directory
	 * name of the database or from an entry in the setup file or from the
	 * command line, which is why we need to wait until now to do this. The
	 * setup file entry name is defined in global.h as TITLE.
	 */
	if(no_title)
	{
		PARM *parm = GetSetupParms(TITLE);
		ptr = XuGetStringResource((GV_edit_mode)? RNtitle:RNviewerTitle,"");
		if(parm == NULL || parm->nparms < 1)
			snprintf(mbuf, sizeof(mbuf), "%s %s : %s", ptr, rev_label, base_name(get_directory("home"),NULL));
		else
			snprintf(mbuf, sizeof(mbuf), "%s %s : %s", ptr, rev_label, parm->parm[0]);
		XtVaSetValues(GW_topLevel, XmNtitle, mbuf, NULL);
	}

	/* Ensure that nothing in the interface creation procedures sends any
	 * commands to the Ingred functions before the connection is done.
	 */
	SendIngredCommands(False);

	/* Load the source data structure from the setup file entries.
	 */
	LoadSourceData();

	/* Create all of our main window structures.
	 */
	if (GV_edit_mode)
		create_edit_window();
	else
		create_view_window();

	ShowHelloMessage();
	XuSetBusyCursor(ON);
	XuDelay(GW_topLevel, 50);

	CheckPresetListFiles();
	CreatePrintPulldown(MENU_Depiction_print);

	(void) GEConnect(GV_app_context, GW_mapWindow, ingred_set_cursor, post_message, ingred_commands);

	/* Now commands to ingred can be permitted.
	 */
	SendIngredCommands(True);

	/* Initializations. Note that the order of these can be important so
	 * it must not be changed without checking for interactions.
	 */
	init_working_directory();
	InitFonts();
	InitPreferences();
	InitMap();
	InitDepictionSequence(ask_for_times);
	InitFields();
	InitFieldUpdateSystem();
	InitAutoImportSystem();
	InitGuidanceStatusSystem();
	InitGuidanceAvailabilitySystem();
	InitImagery();
	InitZoomFunctions();

	/* XXXX TEMPORARY SECTION XXXX */
	/* The idea is to put this information into the configuration system once
	 * Brian has figured out the best way to do it.
	 */
	{
		String units, label, max, min;

		min   = XuGetStringResource(".wind.speedMinimum", "-");
		max   = XuGetStringResource(".wind.speedMaximum", "-");
		units = XuGetStringResource(".wind.speedUnits",   "-");
		(void) IngredVaCommand(GE_ACTION, "DEFAULTS WINDS %s %s %s",  min, max, units);

		units = XuGetStringResource(".tl.speedUnits", "-");
		label = XuGetStringResource(".tl.speedLabel", "-");
		(void) IngredVaCommand(GE_ACTION, "DEFAULTS TIMELINK %s %s", units, label);
	}
	/* XXXX END TEMPORARY SECTION XXXX */

	if( GV_edit_mode )
	{
		InitFieldDisplayState(preset_vis_key);
		init_products();
		InitToActiveDepiction();
		(void) IngredCommand(GE_ACTION, "MODE NORMAL");
		InitGuidanceLists();
		InitToActiveGroup();
		InitTimelinkPanel();
		InitRadarStatSystem();
	}
	else
	{
		Dimension width, height;
		Position  x, y;

		InitFieldDisplayState(preset_vis_key);
		InitToActiveDepiction();
		(void) IngredCommand(GE_ACTION, "MODE NORMAL");
		/*
		 * If the setup file defines guidance fields initialize else grey out button
		 */
		if (HaveSetupEntry(NWP_MODELS)) {
			InitGuidanceLists();
		} else {
			XtSetSensitive(XuMenuFindButtonByName(GW_menuBar,"guidance"), False);
		}
		InitToActiveGroup();
		XuWidgetLabel(GW_mainMessageBar, " ");
		/*
		 * These need to be called explicitly at this point for sample context
		 */
		SetActiveContextMenu(FieldEditContextMenu);
		ConfigureMainContextMenuForField();
		/*
		 * Ok, this is strange, but in viewer mode the first time a menu item on the
		 * GW_menuBar was selected the entire program window would move and the menu
		 * would appear over the menu button! Also the first time in the zoom  would
		 * not work. I found that getting the shell position and size, setting the
		 * size 1 pixel smaller and then doing another set back to the original size
		 * cured both problems. No idea why but I suspect that there is some internal
		 * state that is not set properly and that doing this set it.
		 */
		XtVaGetValues(GW_topLevel, XmNx, &x, XmNy, &y, XmNwidth, &width, XmNheight, &height, NULL);
		XtVaSetValues(GW_topLevel, XmNwidth, width-1, XmNheight, height-1, NULL);
		XtVaSetValues(GW_topLevel, XmNx, x, XmNy, y, XmNwidth, width, XmNheight, height, NULL);
	}
	CheckTimeButtonLayout();
	(void) IngredCommand(GE_ACTION,"REDISPLAY");
	LogMsg("Start");
	RemoveHelloMessage();
	/*
	 * When the hello message went away part of one of the tabs would be
	 * "missing" and would not reapear until the tab was selected. Unmanaging
	 * and then managing the tab frame at this point cures the problem.
	 */
	XtUnmanageChild(GW_tabFrame);
	XuDelay(GW_tabFrame, 20);
	XtManageChild(GW_tabFrame);
	XuSetBusyCursor(OFF);
	(void) XtAppAddWorkProc(GV_app_context, main_initialized_notify, NULL);
	XtAppMainLoop(GV_app_context);
	return 0;
}



/*=========================================================================*/
/*
*	MainExitCB() - Popups up the yes-no dialog to ask if the user is sure
*	that exit is really meant.
*/
/*=========================================================================*/
/*ARGSUSED*/
void MainExitCB( Widget w, XtPointer client_data, XtPointer call_data)
{
	Boolean ok;

	if(GV_edit_mode && GV_pref.confirm_exit)
	{
		if(GV_pref.show_delete_all)
		{
			Boolean delete_all = False;
			if(XuAskUser(GW_mainWindow, "exit_fpa_confirm", XuNaskUserToggle, &delete_all, NULL) == XuNO ) return;
			if(delete_all)
				RemoveDepiction("all");
		}
		else
		{
			if(XuAskUser(GW_mainWindow, "exit_fpa_confirm", NULL) == XuNO ) return;
		}
	}

	/* Delay a bit for all end of application observers to clean up */
	XuDelay(GW_topLevel, 250);
	(void) release_license();
	(void) IngredCommand(GE_ACTION, "MODE SUSPEND");
	(void) GEDisconnect();
	(void) remove_directory(GV_working_directory, &ok);
	xmlCleanupParser();
	FreeItem(GV_working_directory);
	XuDestroyApplication(GW_topLevel);
}


/*===================================================================*/
/*
 * Post a message to the main message bar
 */
/*===================================================================*/
void ShowMessages(Boolean show)
{
	if((msg_show = show))
		post_message(NULL,NULL);
}


/*===================================================================*/
/*
 * Allow Ingred to put up a busy cursor.
 */
/*===================================================================*/
void AllowIngredBusyCursor(Boolean allow)
{
	allow_busy_cursor = allow;
}


/*===================================================================*/
/*
 *   SetIconBarButtonSensitivity() - Set the sensitivity of the
 *   icon button identified by its name. If name is NULL then set
 *   the state of all of the icon buttons to state.
 */
/*===================================================================*/
void SetIconBarButtonSensitivity(String name, Boolean state)
{
	int n;
	if(blank(name))
	{
		for(n = 0; n < nicon_data; n++)
		{
			if(icon_data[n].w)
				XtSetSensitive(icon_data[n].w, state);
		}
	}
	else
	{
		for(n = 0; n < nicon_data; n++)
		{
			if(same(name, icon_data[n].name)) 
			{
				if(icon_data[n].w)
					XtSetSensitive(icon_data[n].w, state);
				break;
			}
		}
	}
}


/*===================================================================*/
/*
 *   GetIconBarWidget() - Return the widget of an icon bar button
 *   given its *_ICON identifier as found in the iconBar.h header.
 */
/*===================================================================*/
Widget GetIconBarWidget(String name)
{
	int n;
	for(n = 0; n < nicon_data; n++)
	{
		if(same(name, icon_data[n].name)) return (icon_data[n].w);
	}
	return NullWidget;
}


/*===================================================================*/
/*
*	SetDepictionTimeDisplay() - Set a time into the depiction time
*	display widget.
*/
/*===================================================================*/
void SetDepictionTimeDisplay(String ptime)
{
	DATE_FORMAT fmt;
	String s;

	fmt = (minutes_in_depictions()) ? MINUTES:HOURS;

	if(IsNull(ptime) && GV_ndepict > 0)
		s = DateString(ActiveDepictionTime(FIELD_INDEPENDENT), fmt);
	else if(valid_tstamp(ptime))
		s = DateString(ptime, fmt);
	else
		s = "-----";

	XuWidgetLabel(depictionTimeDisplay, s);
}


/*=========================================================================*/
/*
*	GetDisplayState() - Return the display state of the given panel data.
*	The values are set in the View main menu pulldown.
*/
/*=========================================================================*/
Boolean GetDisplayState(PANEL_ID panel)
{
	switch (panel)
	{
		case SCRATCHPAD:   return show_scratchpad;
		default:           return False;
	}
}



/*=========================================================================*/
/* 
 * SetGUIComponentLocation() - Set the location of the icon bar, time bar
 * and message bar to either the top or bottom of the map area. This impacts
 * the animation time scale and its attachments as well so this gets somewhat
 * complicated. This function is in main.c as some of the widgets are local
 * to main and I did not want to make them golbal. The icon bar is not used
 * in viewer mode, where it is created but not managed, so we test to see
 * if it is managed for the attachments.
 */
/*=========================================================================*/
void SetGUIComponentLocation(int iconbar_lcn, int timebar_lcn, int message_lcn)
{
	Boolean   time_scroll_active = XtIsManaged(timeScrollBar);
	
	if( iconbar_lcn != TB_NONE ) iconbar_location = iconbar_lcn;
	if( timebar_lcn != TB_NONE ) timebar_location = timebar_lcn;
	if( message_lcn != TB_NONE ) message_location = message_lcn;

	XtUnmanageChild(GW_mainManager);
	XtUnmanageChild(GW_drawingWindow);
	XtUnmanageChild(timeScrollBar);
	XtUnmanageChild(GW_tabFrame);
	XtUnmanageChild(buttonBar);
	XtUnmanageChild(GW_secSeqManager);
	
	/* The icon bar can be by itself as it only attaches to the frame */
	if(iconbar_location == TB_TOP)
	{
		XtVaSetValues(iconBar,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_NONE,
			NULL);
	}
	else
	{
		XtVaSetValues(iconBar,
			XmNtopAttachment, XmATTACH_NONE,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	}

	if(timebar_location == TB_TOP && message_location == TB_TOP)
	{
		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, iconBar,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}
		else
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}

		XtVaSetValues(GW_tabFrame,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, buttonBar,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtVaSetValues(timeScrollBar,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, buttonBar,
			XmNbottomAttachment, XmATTACH_NONE,
			NULL);

		if (time_scroll_active)
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, timeScrollBar,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, buttonBar,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}
	}
	else if(timebar_location == TB_BOTTOM && message_location == TB_TOP)
	{
		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);

			XtVaSetValues(GW_tabFrame,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, iconBar,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, buttonBar,
				NULL);
		}
		else
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, iconBar,
				NULL);

			XtVaSetValues(GW_tabFrame,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, buttonBar,
				NULL);
		}

		XtVaSetValues(timeScrollBar,
			XmNtopAttachment, XmATTACH_NONE,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, buttonBar,
			NULL);

		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, iconBar,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}
	}
	else if(timebar_location == TB_TOP && message_location == TB_BOTTOM)
	{
		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, iconBar,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}
		else
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
		}

		XtVaSetValues(GW_tabFrame,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, buttonBar,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtVaSetValues(timeScrollBar,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, buttonBar,
			XmNbottomAttachment, XmATTACH_NONE,
			NULL);

		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, iconBar,
				NULL);
		}
	}
	else if(timebar_location == TB_BOTTOM && message_location == TB_BOTTOM)
	{
		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);

			XtVaSetValues(GW_tabFrame,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, iconBar,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, buttonBar,
				NULL);
		}
		else
		{
			XtVaSetValues(buttonBar,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, iconBar,
				NULL);

			XtVaSetValues(GW_tabFrame,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, buttonBar,
				NULL);
		}

		XtVaSetValues(timeScrollBar,
			XmNtopAttachment, XmATTACH_NONE,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, buttonBar,
			NULL);

		if (time_scroll_active)
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, timeScrollBar,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_secSeqManager,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, buttonBar,
				NULL);
		}
	}

	/* If the animation scale exists then we must position it */
	if (GW_animationScaleManager)
	{
		int       offset;
		Dimension time_height, msg_height;

		/* The offset is only used when the time and message bars are not
		 * together to set the height of the animation time control. For some
		 * reason if the buttonBar is at the bottom the returned size is smaller
		 * than if it is at the top although the size looks the same. If it starts
		 * out at the top and is moved to the bottom the returned size is the same
		 * as at the top! Why - someday I may find out. Anyway the size is
		 * determined by taking the maximum height of either the message or
		 * time bar. A work around but it seems to do the trick for now.
		 */
		XtVaGetValues(buttonBar, XmNheight, &time_height, NULL);
		XtVaGetValues(GW_secSeqManager, XmNheight, &msg_height, NULL);
		offset = -(int)(MAX(time_height,msg_height));

		if(timebar_location == TB_TOP && message_location == TB_TOP)
		{
			XtVaSetValues(GW_animationScaleManager,
				XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNtopWidget, buttonBar,
				XmNtopOffset, 0,
				XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNbottomWidget, GW_secSeqManager,
				XmNbottomOffset, 0,
				NULL);
		}
		else if(timebar_location == TB_BOTTOM && message_location == TB_TOP)
		{
			XtVaSetValues(GW_animationScaleManager,
				XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNtopWidget, buttonBar,
				XmNtopOffset, offset,
				XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNbottomWidget, buttonBar,
				XmNbottomOffset, 0,
				NULL);
		}
		else if(timebar_location == TB_TOP && message_location == TB_BOTTOM)
		{
			XtVaSetValues(GW_animationScaleManager,
				XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNtopWidget, buttonBar,
				XmNtopOffset, 0,
				XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNbottomWidget, buttonBar,
				XmNbottomOffset, offset,
				NULL);
		}
		else if(timebar_location == TB_BOTTOM && message_location == TB_BOTTOM)
		{
			XtVaSetValues(GW_animationScaleManager,
				XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNtopWidget, GW_secSeqManager,
				XmNtopOffset, 0,
				XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
				XmNbottomWidget, buttonBar,
				XmNbottomOffset, 0,
				NULL);
		}
	}

	XtManageChild(buttonBar);
	XtManageChild(GW_secSeqManager);
	XtManageChild(GW_tabFrame);
	Manage(timeScrollBar, time_scroll_active);

	/* This must come after the possible management of timeScrollBar */
	SetDrawingWindowAttachment();

	XtManageChild(GW_drawingWindow);
	XtManageChild(GW_mainManager);
}


/*=========================================================================*/
/* 
 * SetDrawingWindowAttachment() - For the same reasons as the above this
 * function is in main.c as I did not want to make some of the widgets
 * global. This function ensures that the drawing window is properly
 * attached depending on the state of the time and message widgets
 * (including the animation panel widget).
 */
/*=========================================================================*/
void SetDrawingWindowAttachment( void )
{
	if(timebar_location == TB_TOP && message_location == TB_TOP)
	{
		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, GW_secSeqManager,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, GW_secSeqManager,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, iconBar,
				NULL);
		}
	}
	else if(timebar_location == TB_BOTTOM && message_location == TB_TOP)
	{
		if( PanelIsActive(ANIMATION) )
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, GW_secSeqManager,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, GW_animationScaleManager,
				NULL);
		}
		else if (XtIsManaged(timeScrollBar))
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, GW_secSeqManager,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget,timeScrollBar,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, GW_secSeqManager,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, buttonBar,
				NULL);
		}
	}
	else if(timebar_location == TB_TOP && message_location == TB_BOTTOM)
	{
		if( PanelIsActive(ANIMATION) )
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, GW_animationScaleManager,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, GW_secSeqManager,
				NULL);
		}
		else if (XtIsManaged(timeScrollBar))
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, timeScrollBar,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget,GW_secSeqManager,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, buttonBar,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, GW_secSeqManager,
				NULL);
		}
	}
	else if(timebar_location == TB_BOTTOM && message_location == TB_BOTTOM)
	{
		if(iconbar_location == TB_TOP)
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, iconBar,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, GW_secSeqManager,
				NULL);
		}
		else
		{
			XtVaSetValues(GW_drawingWindow,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, GW_secSeqManager,
				NULL);
		}
	}
}


/*========================= LOCAL FUNCTIONS ==============================*/


/*  fatal_error_handler() - Called by the signal trap function for all
*	fatal errors;
*/
static void fatal_error_handler(int sig)
{
	String sname;
	(void) signal(sig,SIG_IGN);
	sname = signal_name(sig);
	printf("[Fpa] FATAL PROGRAM ERROR: Aborted on %s.\n", sname);
	exit(1);
}


/* Notify all interested parties that the mainline has finished its
 * initialization and is ready for business. This is done as a work
 * proc so that any events generated during initializing are done
 * before the notification is sent.
 */
static Boolean main_initialized_notify(XtPointer unused)
{
	initialized = True;

	/* 2012.12.14 - In Ubuntu Linux, for some reason, when the drawing
	 * window first comes up it shows as obscured. Resizing the window
	 * by even one pixel jogs things to reattach thus eliminating the
	 * problem. Must be done after everyting else is ready which is why
	 * the code is here. Crude but it works ;-)
	 */
	XtVaSetValues(GW_drawingWindow, XmNtopOffset, 1, NULL);
	XtVaSetValues(GW_drawingWindow, XmNtopOffset, 0, NULL);

	/* Show any delayed special dialog style messages from Ingred.
	 * See post_message() function for details.
	 */
	if(delayed_message)
	{
		String type = string_arg(delayed_message);
		post_message(type, delayed_message);
		FreeItem(delayed_message);
	}

	NotifyObservers(OB_MAIN_INITIALIZED, NULL, 0);
	return True;
}


/* If the process which spawned this program exits it will take its colormap with it.
 * If this happens we must exit as well as we do not have a valid colormap.  This
 * will only occur when we are in viewer mode and only on older systems with 8 bit
 * deep maps.
 */
/*ARGSUSED*/
static void colormap_change_handler( Widget w, XtPointer client_data, XEvent *event)
{
	if(event->type != ColormapNotify ) return;
	if(event->xcolormap.colormap != None) return;
	MainExitCB(w, NULL, NULL);
}


/* The utime() function updates the last modified time of the directory to
*  the current time.
*/
static void touch_work_dir( XtPointer client_data , XtIntervalId *id )
{
	(void) utime(GV_working_directory, NULL);
	(void) XtAppAddTimeOut(GV_app_context, 300000, touch_work_dir, NULL);
}


/* Create the working directory to be used for any processes which
 * use temporary files.
 */
static void init_working_directory(void)
{
	int	    i, ndir;
	char    name[100];
	String  *dirs, dir;
	Boolean ok;

	dir = get_directory(WORKING_DIRECTORY);
	if (!dir) dir = "/tmp";

	/* Remove any inactive directories (caused by aborts?). Since the directory is
	 * touched by the function above every 5 minutes, anything over this age
	 * is not attached to an active program. We will give it an hour just to be
	 * really safe.
	 */
	(void) snprintf(name, sizeof(name), "%s_twkdr*", GV_app_name);
	ndir = dirlist(dir, name, &dirs);
	for( i = 0; i < ndir; i++ )
	{
		String dbuf = AllocPrint("%s/%s", dir, dirs[i]);
		if(find_directory(dbuf))
		{
			struct stat sd;
			time_t curtime = time(NULL);
			(void)stat(dbuf, &sd);
			if( curtime - sd.st_mtime  > 3600 )
			{
				(void) remove_directory(dbuf, &ok);
				if (!ok)
					pr_warning(GV_app_name, "Unable to remove old working directory: %s\n", dbuf);
			}
		}
		FreeItem(dbuf);
	}

	GV_working_directory = AllocPrint("%s/%s_twkdrXXXXXX", dir, GV_app_name);
	ok = (mkdtemp(GV_working_directory) != NULL);

	/* Create our working directory
	 */
	if (ok)
	{
		(void) XtAppAddTimeOut(GV_app_context, 300000, touch_work_dir, NULL);
		glSetWorkingDirectory(GV_working_directory);
	}
	else
	{
		pr_warning(GV_app_name, "Unable to create working directory: %s\n", GV_working_directory);
		FreeItem(GV_working_directory);
	}
}


static void init_products(void)
{
	int i;
	Boolean some;
	SETUP *setup;

	static String default_lang[] = {ENGLISH,FRENCH};
	static String default_tz[] = {
		"pacific","mountain","central","eastern","atlantic","newfoundland"};

	/* Set up the product language list.
	*/
	setup = GetSetup(PROD_LANGUAGES);
	if(setup->nentry > 0)
	{
		GV_nlanguages = setup->nentry;
		GV_language = NewMem(KEYINFO, setup->nentry);
		for(i = 0; i < setup->nentry; i++)
		{
			GV_language[i].key = SetupParm(setup,i,1);
			GV_language[i].label = SetupParm(setup,i,0);
		}
	}
	else
	{
		GV_nlanguages = XtNumber(default_lang);;
		GV_language = NewMem(KEYINFO, GV_nlanguages);
		for(i = 0; i < GV_nlanguages; i++)
		{
			GV_language[i].key = default_lang[i];
			GV_language[i].label = XuGetLabel(default_lang[i]);
		}
	}
	/* Set up the product timezone list.
	*/
	setup = GetSetup(PROD_TIMEZONES);
	if(setup->nentry > 0)
	{
		GV_ntimezones = setup->nentry;
		GV_timezone = NewMem(KEYINFO, setup->nentry);
		for(i = 0; i < setup->nentry; i++)
		{
			GV_timezone[i].key = SetupParm(setup,i,1);
			GV_timezone[i].label = SetupParm(setup,i,0);
		}
	}
	else
	{
		GV_ntimezones = XtNumber(default_tz);;
		GV_timezone = NewMem(KEYINFO, GV_ntimezones);
		for(i = 0; i < GV_ntimezones; i++)
		{
			GV_timezone[i].key   = default_tz[i];
			GV_timezone[i].label = XuGetLabel(default_tz[i]);
		}
	}


	/* Now initialize the product interfaces.
	*/
	some = False;

	if(EntryExists(MENU_Products_alliedModels, ALLIED_MODELS, True))
	{
		InitAlliedModels();
		some = True;
	}
	if(EntryExists(MENU_Products_regularText, PROD_TEXT, True))
	{
		InitFcstTextDialog();
		some = True;
	}
	if(EntryExists(MENU_Products_graphics, PROD_GRAPHIC, True))
	{
		InitGraphicProductsDialog();
		some = True;
	}
	if(EntryExists(MENU_Products_pointFcst, PROD_POINT, True))
	{
		InitPointFcstDialog();
		some = True;
	}
	if(!some)
	{
		Widget w = XuMenuFindButton(GW_menuBar, MENU_Bar_products);
		XtSetSensitive(w,False);
	}
}


/*
*	Write a text message of the given type into the main message bar. This
*   function is required for Ingred. The type specifications are expected
*   to be found in the resource file as:
*
*      .ingredMessage.<type>.fg: colour
*      .ingredMessage.<type>.bg: colour
*
*   This function and ShowMessages work together to enable message blocking in
*   those cases where many messages would come out together too quickly
*   for a user to read. In this case ShowMessages(False) would be called
*   before the sequence of commands and ShowMessages(True) after the commands.
*   The last command received from ingred would then be shown.
*
*   There is a special type that indicates that Ingred wants the interface
*   to put up an error dialog. This has the specific key of "ErrorDialog"
*   and the text is then the CommonMdb keyword that specifies the parameters
*   for the error dialog.
*/
static void post_message(String type, String text)
{
	static XmString msg_label     = (XmString)NULL;
	static Pixel    msg_bg        = 0;
	static Pixel    msg_fg        = 0;
	static char     last_type[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	if (!GV_edit_mode) return;

	if(same(type,"ErrorDialog"))
	{
		/* Showing the dialog before the interface has finished initializing
		 * can cause problems. Thus during initialization delay the display
		 * to the end.
		 */
		if(initialized)
		{
			no_white(text);
			XuShowError(GW_mainWindow, text, NULL);
		}
		else
		{
			FreeItem(delayed_message)
			delayed_message = AllocPrint("%s %s", type, text);
		}
	}
	else
	{
		if (text)
		{
			if (msg_label) XmStringFree(msg_label);
			msg_label = XmStringCreateLocalized(blank(text)? " ":text);
		}

		if (type && !same(type,last_type))
		{
			char mbuf[128];
			(void) strncpy(last_type, type, 15);
			(void) snprintf(mbuf, sizeof(mbuf), ".ingredMessage.%s.fg", type);
			msg_fg = XuLoadColorResource(GW_topLevel, mbuf, "foreground");

			(void) snprintf(mbuf, sizeof(mbuf), ".ingredMessage.%s.bg", type);
			msg_bg = XuLoadColorResource(GW_topLevel, mbuf, "background");
		}

		if (msg_show && msg_label)
		{
			XtVaSetValues(GW_mainMessageBar,
				XmNlabelString, msg_label,
				XmNforeground, msg_fg,
				XmNbackground, msg_bg,
				NULL );
			XmUpdateDisplay(GW_mainMessageBar);
		}
	}
}


/* Function to return the language string of our choice when registered with
 * the XtSetLanguageProc() function. The language is determined in decending
 * order of importance by:
 *
 * - the command line entry -xnllangage <language string>
 * - the FPA_LANG environment variable
 */
static String language_proc( Display *dpy, String xnl, XtPointer client_data )
{
	String lang;

	if(!blank(xnl))
	{
		lang = xnl;
	}
	else
	{
		lang = getenv("FPA_LANG");
		if(blank(lang)) lang = getenv("LANG");
		if(blank(lang)) lang = "C";
	}
	set_language_token(lang);
	return lang;
}



/* Cursor display specific to Ingred.
*/
static void ingred_set_cursor(String cmd, Boolean state)
{
	if(same_ic(cmd, "busy"))
	{
		if(allow_busy_cursor)
			XuSetBusyCursor(state);
	}
	else if(same_ic(cmd, "obscured"))
	{
		if(GV_edit_mode)
			XuSetCursor(GW_mapWindow, XuWINDOW_OBSCURED_CURSOR, state);
	}
	else if(same_ic(cmd, "finger"))
	{
		XuSetDialogCursor(GW_topLevel, XuFINGER_CURSOR, state);
	}
	else if(same_ic(cmd, "pen"))
	{
		XuSetDialogCursor(GW_topLevel, XuPENCIL_CURSOR, state);
	}
	else if(same_ic(cmd, "stop"))
	{
		XuSetDialogCursor(GW_topLevel, XuSTOP_CURSOR, state);
	}
	else
	{
		XuSetDialogCursor(GW_topLevel, XuDEFAULT_CURSOR, state);
	}
}


/* Process status messages from Ingred and send them on to all interested
 * observers. The input string is parsed into a string array which is then
 * sent so that the individual observers do not have to parse the string
 * themselves. The CAL structure is sent as the first parameter, so that
 * the command parameters will start at position 1. 
 */
static void ingred_commands(String cmd, CAL cal)
{
	int           count = 1;
	static int    nkeys = 50;
	static String *keys = NULL;

	if (!keys) keys = NewStringArray(nkeys);

	(void) memset((void*)keys, 0, nkeys*sizeof(String));

	keys[0] = strtok_arg(cmd);
	while((keys[count] = strtok_arg(NULL)) != NULL)
	{
		count++;
		if(count >= nkeys)
		{
			keys = MoreStringArray(keys, (nkeys + 10));
			(void) memset((void*)(keys+nkeys), 0, 10*sizeof(String));
			nkeys += 10;
		}
	}

	if(same_ic(keys[0],"BACKGROUND"))
	{
		FIELD_INFO *fld = FindField(keys[1], keys[2]);
		if(fld && fld->setBkgndFcn) fld->setBkgndFcn(fld, cal);
	}
	else
	{
		NotifyIngredObservers(cal, keys, count);
	}
}


/*  menu_cb() - Handles all of the callbacks from the pulldown selection
*	buttons.  The keys are defined in the menu.h header file.
*/
/*ARGSUSED*/
static void menu_cb( Widget w, XtPointer key, XtPointer unused)
{
	String notify[1];
	String rev_label = FpaRevLabel;

	switch(PTR2INT(key))
	{
		case MENU_Create_fields:
			ACTIVATE_createFieldsDialog(GW_mainWindow);
			break;

		case MENU_Load_fields:
			ACTIVATE_loadFieldsDialog(GW_mainWindow);
			break;

		case MENU_Update_fields:
			ACTIVATE_updateFieldsDialog(w);
			break;

		case MENU_Import_fields:
			ACTIVATE_autoImportDialog(w);
			break;

		case MENU_Depiction_save:
			SaveDepiction(ACTIVE);
			break;

		case MENU_Depiction_saveAll:
			SaveDepiction("all");
			break;

		case MENU_Depiction_delete:
			ACTIVATE_deleteFieldsDialog(w);
			break;

		case MENU_Depiction_deleteAll:
			RemoveDepiction("all");
			break;

		case MENU_Option_t0_active:
			DeactivateMenu();
			SetT0Depiction(T0_TO_ACTIVE_DEPICTION);
			ActivateMenu();
			break;

		case MENU_Option_nearest_t0_clock:
			DeactivateMenu();
			SetT0Depiction(T0_NEAREST_TO_SYSTEM_CLOCK);
			ActivateMenu();
			break;

		case MENU_Option_t0_clock:
			DeactivateMenu();
			SetT0Depiction(T0_TO_SYSTEM_CLOCK);
			ActivateMenu();
			break;

		case MENU_Option_preferences:
			ACTIVATE_preferencesDialog(w);
			break;

		case MENU_Option_save_profile:
			ACTIVATE_saveProfileDialog(w);
			break;

		case MENU_Option_manage_profile:
			ACTIVATE_manageProfileDialog(w);
			break;

		case MENU_Status_depiction:
			ACTIVATE_depictionStatusDialog(w);
			break;

		case MENU_Status_products:
			ACTIVATE_productStatusDialog(w);
			break;

		case MENU_Products_regularText:
			ACTIVATE_fcstTextDialog(w);
			break;

		case MENU_Products_graphics:
			ACTIVATE_graphicProductsDialog(w);
			break;

		case MENU_Products_pointFcst:
			ACTIVATE_pointFcstSelectDialog(w);
			break;

		case MENU_Products_alliedModels:
			ACTIVATE_alliedModelSelectDialog(w);
			break;

		case MENU_Actions_Zoom_tear:
			ACTIVATE_zoomDialog(w);
			break;

		case MENU_Actions_Zoom_in:
			ZoomCommand(ZOOM_IN);
			break;

		case MENU_Actions_Zoom_out:
			ZoomCommand(ZOOM_OUT);
			break;

		case MENU_Actions_Zoom_pan:
			ZoomCommand(ZOOM_PAN);
			break;

		case MENU_Actions_Zoom_exit:
			ZoomCommand(ZOOM_EXIT);
			break;

		case MENU_View_scratchpadShow:
			show_scratchpad  = XmToggleButtonGetState(w);
			break;

		case MENU_Guidance_status:
			ACTIVATE_guidanceStatusDialog(w);
			break;

		case MENU_Guidance_availability:
			ACTIVATE_guidanceAvailabilityDialog(w);
			break;

		case MENU_Guidance_displayed:
			ACTIVATE_showGuidanceLegendDialog(w);
			break;

		case MENU_Guidance_select:
			ACTIVATE_showGuidanceLegendDialog(w);
			ACTIVATE_guidanceDialog(w);
			break;

		case MENU_Guidance_hide:
			SetGuidanceDisplayState(False);
			break;

		case MENU_Guidance_show:
			SetGuidanceDisplayState(True);
			break;

		case MENU_Image_select:
			ACTIVATE_imageryControlDialog(w);
			break;

		case MENU_Image_show:
			SetImageryDisplayState(NULL, True);
			break;

		case MENU_Image_hide:
			SetImageryDisplayState(NULL, False);
			break;

		case MENU_View_fieldVisibility:
			ACTIVATE_fieldDisplayStateDialog(w);
			break;

		case MENU_View_mapOverlays:
			ACTIVATE_mapOverlayDialog(w);
			break;

		case MENU_Help_problems:
			ACTIVATE_problemReportingDialog(w);
			break;

		case MENU_Help_about:
			XuShowMessage(GW_mainWindow, "about_fpa", rev_label, "\251", NULL);
			break;

		case MENU_Bar_guidance_update:
			ACTIVATE_processGuidanceUpdateDialog(w);
			break;

		default:
			pr_diag(GV_app_name,"menu_cb - Unrecognized key!\n");
			break;
	}

	notify[0] = (String) key;
	NotifyObservers(OB_MAIN_MENU_ACTIVATION, notify, 1);
}


/* This callback is separate from the general menu_cb function so that the
 * highlight around the icon buttons can be managed. It turns out that the
 * highlight will not go away when a dialog is popped up. This function gets
 * around this problem by turning it off before a delay loop for processing
 * and then turning it back on again.
 */
/*ARGSUSED*/
static void icon_button_cb(Widget w, XtPointer key, XtPointer unused)
{
	XtVaSetValues(w, XmNhighlightOnEnter, False, NULL);
	XuDelay(w, 10);
	XtVaSetValues(w, XmNhighlightOnEnter, True, NULL);
	menu_cb(w, key, unused);
}

/* Function called by the arrow buttons defined below for moving through
*  the depiction sequence (normal or animation).
*/
static void arrow_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int increment = PTR2INT(client_data);

	if(PanelIsActive(ANIMATION))
		AnimationStep(increment, GV_increment_step);
	else
		DepictionStep(increment, GV_increment_step);
}


/* The following four functions are used to control the position of the
 * time buttons with the slider bar. This will only come into play when
 * the length of the time buttons exceeds the available display space. We
 * need to keep track of the scroll value so that the scroll position
 * can to be set back to the last position that the user set it to.
 */
static int scroll_value = TSBMS;

/*ARGSUSED*/
static void drawing_resize_cb( Widget w, XtPointer na, XtPointer unused )
{
	Dimension w1, w2;
	int       range, offset;

	XtVaGetValues(seqForm,             XmNwidth, &w1, NULL);
	XtVaGetValues(GW_sequenceSelector, XmNwidth, &w2, NULL);

	if(w2 <= w1)
	{
		if(XtIsManaged(timeScrollBar))
		{
			if(timebar_location == TB_TOP && message_location == TB_TOP)
			{
				XtVaSetValues(GW_secSeqManager, XmNtopWidget, buttonBar, NULL);
			}
			else if(timebar_location == TB_BOTTOM && message_location == TB_TOP)
			{
				XtVaSetValues(GW_drawingWindow, XmNbottomWidget, buttonBar, NULL);
				XtVaSetValues(GW_secSeqManager, XmNbottomWidget, buttonBar, NULL);
			}
			else if(timebar_location == TB_TOP && message_location == TB_BOTTOM)
			{
				XtVaSetValues(GW_secSeqManager, XmNtopWidget, buttonBar, NULL);
			}
			else if(timebar_location == TB_BOTTOM && message_location == TB_BOTTOM)
			{
				XtVaSetValues(GW_secSeqManager, XmNbottomWidget, buttonBar, NULL);
			}
			XtVaSetValues(GW_sequenceSelector, XmNrightOffset, 0, NULL);
			XtUnmanageChild(timeScrollBar);
			scroll_value = TSBMS;
		}
	}
	else
	{
		range = (int)(MAX((((double)(w2-w1)/(double)w2) * TSBMS),0));
		if(range > 0)
		{
			if( scroll_value > range ) scroll_value = range;
			offset = (range - scroll_value) * (int) MAX(w2-w1,0) /  range;
		}
		else
		{
			scroll_value = 0;
			offset = (int) MAX(w2-w1,0);
		}

		XtVaSetValues(GW_sequenceSelector,
			XmNrightOffset, -offset,
			NULL);

		XtVaSetValues(timeScrollBar,
			XmNvalue, scroll_value,
			XmNpageIncrement, TSBMS - range,
			XmNsliderSize, TSBMS - range,
			NULL);

		if(!XtIsManaged(timeScrollBar))
		{
			XtManageChild(timeScrollBar);
			if(timebar_location == TB_TOP && message_location == TB_TOP)
			{
				XtVaSetValues(GW_secSeqManager, XmNtopWidget, timeScrollBar, NULL);
			}
			else if(timebar_location == TB_BOTTOM && message_location == TB_TOP)
			{
				XtVaSetValues(GW_drawingWindow, XmNbottomWidget, timeScrollBar, NULL);
				XtVaSetValues(GW_secSeqManager, XmNbottomWidget, timeScrollBar, NULL);
			}
			else if(timebar_location == TB_TOP && message_location == TB_BOTTOM)
			{
				XtVaSetValues(GW_secSeqManager, XmNtopWidget, timeScrollBar, NULL);
			}
			else if(timebar_location == TB_BOTTOM && message_location == TB_BOTTOM)
			{
				XtVaSetValues(GW_secSeqManager, XmNbottomWidget, timeScrollBar, NULL);
			}
		}
	}
}


/* Public function call for above */
void CheckTimeButtonLayout(void)
{
	/* When the depiction sequence buttons are created and labeled and the
	 * length of the button set is greater than seqForm, the buttons do not
	 * space out properly. This also happens if buttons are added to the
	 * row column and the button length exceeds the size of seqForm.
	 * Experience has shown that if one then changes the width of the FPA
	 * window the buttons then display correctly. To do the eqivalent in a
	 * code sense, the buttonBar leftOffset is changed to "kick" the widget
	 * and its children.
	 */
	XtVaSetValues(buttonBar, XmNleftOffset, 1, NULL);
	XuDelay(GW_topLevel, 10);
	XtVaSetValues(buttonBar, XmNleftOffset, 0, NULL);
	XuDelay(GW_topLevel, 10);
	drawing_resize_cb(NULL,NULL,NULL);
}


/* Time buttons scroll bar callback. This positions the buttons by setting
 * the offset of the row-column parent. If the offset is negative as used
 * here, then the parent will be visually cropped.
 */
/*ARGSUSED*/
static void time_scroll_cb( Widget w, XtPointer na, XtPointer client_data )
{
	XmScrollBarCallbackStruct *rtn = (XmScrollBarCallbackStruct *)client_data;

	if(rtn->reason == XmCR_VALUE_CHANGED)
	{
		int       offset, range;
		Dimension w1, w2;

		XtVaGetValues(seqForm, XmNwidth, &w1, NULL);
		XtVaGetValues(GW_sequenceSelector, XmNwidth, &w2, NULL);

		scroll_value = rtn->value;
		range        = MAX(((double)(w2 - w1) / (double)w2) * TSBMS, 0);
		if(range > 0)
			offset = (range - scroll_value) * (int) MAX(w2-w1,0) / range;
		else
			offset = (int) MAX(w2-w1,0);

		XtVaSetValues(GW_sequenceSelector,
			XmNrightOffset, -offset,
			NULL);
	}
}


static void create_icon_bar(ICON_DATA *icons, int nicons)
{
	int       n;
	Dimension height;
	Widget    w;
	Pixmap    px, ipx;

	/* Set the global icon variables to the specific instance created */
	icon_data  = icons;
	nicon_data = nicons;

	iconBar = XmVaCreateManagedFrame(GW_mainManager, "ibf",
		XmNshadowType, XmSHADOW_OUT,
		XmNshadowThickness, 2,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		NULL);

	w = XmVaCreateManagedForm(iconBar, "iconBarForm", NULL);
	GW_iconButtonBar = XmVaCreateButtonBox(w, "iconBar",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 0,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 0,
		NULL);

	for( height = 0, n = 0; n < nicon_data; n++ )
	{
		/* Separators are a special case */
		if(same(icon_data[n].name, SEPARATOR_ICON))
		{
			icon_data[n].w = XmVaCreateManagedSeparator(GW_iconButtonBar, icon_data[n].name,
				XmNorientation, XmVERTICAL,
				XmNseparatorType, XmSHADOW_ETCHED_IN,
				XmNmarginLeft, icon_data[n].margin,
				XmNmarginRight, icon_data[n].margin,
				NULL);
		}
		else
		{
			px  = XuGetPixmap(GW_iconButtonBar, icon_data[n].icon_file);
			ipx = XuCreateInsensitivePixmap(GW_iconButtonBar, px);
			
			icon_data[n].w = XmVaCreatePushButton(GW_iconButtonBar, icon_data[n].name,
				XmNlabelType, XmPIXMAP,
				XmNlabelPixmap, px,
				XmNlabelInsensitivePixmap, ipx,
				XmNshadowThickness, 0,
				XmNhighlightOnEnter, True,
				XmNhighlightThickness, 1,
				XmNsensitive, icon_data[n].sensitive,
				XmNmarginLeft, icon_data[n].margin,
				XmNmarginRight, icon_data[n].margin,
				NULL);

			XtAddCallback(icon_data[n].w, XmNactivateCallback, icon_button_cb, INT2PTR(icon_data[n].action_id));

			if(icon_data[n].manage)
			{
				Dimension h;
				XtManageChild(icon_data[n].w);
				XtVaGetValues(icon_data[n].w, XmNheight, &h, NULL);
				if(h > height) height = h;
			}
		}
	}

	/* Set the separator height to be all the same. This is in case some of the icons
	 * are of different sizes. I try for this to not happen - but just in case. The
	 * -6 for the hight is to ballance the top space of 3 above.
	 */
	for( n = 0; n < nicon_data; n++ )
	{
		if(same(icon_data[n].name, SEPARATOR_ICON))
			XtVaSetValues(icon_data[n].w, XmNheight, height - 6, NULL);
	}

	XtManageChild(GW_iconButtonBar);
}


/*  create_edit_window() - Creates the main window using XmCreateMainWindow
*	function.  The three main sections of this window are assigned to the
*	pulldown menu, the depiction selection button bar, and the drawing
*	window manager which parents the side bar controls and the drawing
*	window itself.
*/
static void create_edit_window(void)
{
	int       n;
	Dimension width;
	Widget    w, arrowPlus, arrowMinus, secSeqRC;
	Widget    topBar, timeForm, messageFrame;
	XmString  time_step_label;
	Widget    latLongFrame;
	XmString  label;
	Arg       args[1];

	/* Widgets, pixmaps and actions to be found on the editor icon bar.
	 */
	static ICON_DATA edit_icon_data[] = {
		{"load_fields",     "load-fields",    LOAD_FIELDS_ICON,       0,MENU_Load_fields,          3,2,True, True },
		{"update_fields",   "update-fields",  UPDATE_FIELDS_ICON,     0,MENU_Update_fields,        3,2,True, True },
		{"import_fields",   "import-fields",  IMPORT_FIELDS_ICON,     0,MENU_Import_fields,        3,2,True, True },
		{"delete_fields",   "delete",         DELETE_FIELDS_ICON,     0,MENU_Depiction_delete,     3,2,True, True },
		{ NULL,             NULL,             SEPARATOR_ICON,         0,0,                         3,2,True, True },
		{"graphic_products","graphic-product",GRAPHIC_PRODUCTION_ICON,0,MENU_Products_graphics,    3,2,True, True },
		{"allied_models",   "allied-model",   ALLIED_MODELS_ICON,     0,MENU_Products_alliedModels,3,2,True, True },
		{ NULL,             "Info", PRODUCT_STATUS_ICON,    0,MENU_Status_products,      3,2,True, True },
		{ NULL,             NULL,             SEPARATOR_ICON,         0,0,                         3,2,True, True },
		{"guidance_select", "guidance-select",GUIDANCE_SELECT_ICON,   0,MENU_Guidance_select,      3,2,False,True },
		{NULL,              "bookshelf-full", GUIDANCE_STATUS_ICON,   0,MENU_Guidance_status,      3,2,True, True },
		{NULL,              "redflag",        GUID_SELECT_STATUS_ICON,0,MENU_Bar_guidance_update,  3,2,True, False},
		{"image_select",    "image-select",   IMAGE_SELECT_ICON,      0,MENU_Image_select,         3,2,True, True },
		{ NULL,             NULL,             SEPARATOR_ICON,         0,0,                         3,2,True, True },
		{"map_overlays",    "map-overlay",    MAP_OVERLAYS_ICON,      0,MENU_View_mapOverlays,     3,2,True, True },
		{"field_display",   "field-control",  FIELD_DISPLAY_ICON,     0,MENU_View_fieldVisibility, 3,2,True, True },
		{ NULL,             NULL,             SEPARATOR_ICON,         0,0,                         3,2,True, True },
		{"zoom_in",         "zoom-in",        ZOOM_IN_ICON,           0,MENU_Actions_Zoom_in,      3,2,True, True },
		{"zoom_out",        "zoom-out",       ZOOM_OUT_ICON,          0,MENU_Actions_Zoom_out,     3,2,False,True },
		{"zoom_pan",        "zoom-pan",       ZOOM_PAN_ICON,          0,MENU_Actions_Zoom_pan,     3,2,False,True }
	};


	/* Define the data structures used to create the main pulldown menu and
	*  all of its pull right menus.  See the library header Xu.h for
	*  details as to the content of this structure.
	*/
	static XuMenuItemStruct pullright_print_menu[] = {
		NULL
	};

	static XuMenuItemStruct depict_menu[] = {
		{"status", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Status_depiction,
			menu_cb, (XtPointer)MENU_Status_depiction, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"create", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Create_fields,
			menu_cb, (XtPointer)MENU_Create_fields, NULL },
		{"load", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Load_fields,
			menu_cb, (XtPointer)MENU_Load_fields, NULL },
		{"update", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Update_fields,
			menu_cb, (XtPointer)MENU_Update_fields, NULL },
		{"import", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Import_fields,
			menu_cb, (XtPointer)MENU_Import_fields, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"save", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Depiction_save,
			menu_cb, (XtPointer)MENU_Depiction_save, NULL },
		{"saveAll", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Depiction_saveAll,
			menu_cb, (XtPointer)MENU_Depiction_saveAll, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"delete", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Depiction_delete,
			menu_cb, (XtPointer)MENU_Depiction_delete, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"print", NULL, XuMENU_NONE, None, MENU_Depiction_print,
			NULL, (XtPointer)0, pullright_print_menu },
		{"sepDbl",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"quit", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			MainExitCB, NULL, NULL },
		NULL
	};


	static XuMenuItemStruct setT0Depiction_pullright_menu[] = {
		{"to_active", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_t0_active,
			menu_cb, (XtPointer)MENU_Option_t0_active, NULL },
		{"nearest_to_clock", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_nearest_t0_clock,
			menu_cb, (XtPointer)MENU_Option_nearest_t0_clock, NULL },
		{"to_clock", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_t0_clock,
			menu_cb, (XtPointer)MENU_Option_t0_clock, NULL },
		NULL
	};

	static XuMenuItemStruct timeStep_pullright_menu[] = {
		NULL
	};

	static XuMenuItemStruct option_menu[] = {
		{"t0", NULL, XuMENU_TEAR_OFF, None, NoId,
			NULL, (XtPointer)0, setT0Depiction_pullright_menu },
		{"timeStep", NULL, XuMENU_TEAR_OFF|XuMENU_RADIO_LIST, None, MENU_Option_timeStep,
			NULL, (XtPointer)0, timeStep_pullright_menu },
		{"preferences",  &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_preferences,
			menu_cb, (XtPointer)MENU_Option_preferences, NULL },
		{"saveProfile",  &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_save_profile,
			menu_cb, (XtPointer)MENU_Option_save_profile, NULL },
		{"manageProfile",  &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_manage_profile,
			menu_cb, (XtPointer)MENU_Option_manage_profile, NULL },
		NULL
	};

	static XuMenuItemStruct product_menu[] = {
		{"status", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Status_products,
			menu_cb, (XtPointer)MENU_Status_products, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"textFcsts", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Products_regularText,
			menu_cb, (XtPointer)MENU_Products_regularText, NULL },
		{"graphic", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Products_graphics,
			menu_cb, (XtPointer)MENU_Products_graphics, NULL },
		{"pointFcsts", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Products_pointFcst,
			menu_cb, (XtPointer)MENU_Products_pointFcst, NULL },
		{"alliedModels", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Products_alliedModels,
			menu_cb, (XtPointer)MENU_Products_alliedModels, NULL },
		NULL
	};

	static XuMenuItemStruct guidance_menu[] = {
		{"status", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_status,
			menu_cb, (XtPointer)MENU_Guidance_status, NULL },
		{"availability", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_availability,
			menu_cb, (XtPointer)MENU_Guidance_availability, NULL },
		{"guidanceDisplayed", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_displayed,
			menu_cb, (XtPointer)MENU_Guidance_displayed, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"guidanceSelect", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_select,
			menu_cb, (XtPointer)MENU_Guidance_select, NULL },
		{"guidanceHide", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_hide,
			menu_cb, (XtPointer)MENU_Guidance_hide, NULL },
		{"guidanceShow", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_show,
			menu_cb, (XtPointer)MENU_Guidance_show, NULL },
		NULL
	};

	static XuMenuItemStruct image_menu[] = {
		{"imagerySelect", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Image_select,
			menu_cb, (XtPointer)MENU_Image_select, NULL },
		{"imageryHide", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Image_hide,
			menu_cb, (XtPointer)MENU_Image_hide, NULL },
		{"imageryShow", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Image_show,
			menu_cb, (XtPointer)MENU_Image_show, NULL },
		NULL
	};

	static XuMenuItemStruct zoom_pullright_menu[] = {
		{"db", &xmDrawnButtonWidgetClass, XuMENU_NONE, None, MENU_Actions_Zoom_tear,
			menu_cb, (XtPointer)MENU_Actions_Zoom_tear, NULL },
		{"zoomIn", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Actions_Zoom_in,
			menu_cb, (XtPointer)MENU_Actions_Zoom_in, NULL },
		{"zoomOut", &xmPushButtonWidgetClass, XuMENU_INSENSITIVE, None, MENU_Actions_Zoom_out,
			menu_cb, (XtPointer)MENU_Actions_Zoom_out, NULL },
		{"zoomPan", &xmPushButtonWidgetClass, XuMENU_INSENSITIVE, None, MENU_Actions_Zoom_pan,
			menu_cb, (XtPointer)MENU_Actions_Zoom_pan, NULL },
		{"zoomExit", &xmPushButtonWidgetClass, XuMENU_INSENSITIVE, None, MENU_Actions_Zoom_exit,
			menu_cb, (XtPointer)MENU_Actions_Zoom_exit, NULL },
		NULL
	};

	static XuMenuItemStruct select_field_display_menu[] = {
		NULL
	};

	static XuMenuItemStruct coview_menu[] = {
		NULL
	};

	static XuMenuItemStruct view_menu[] = {
		{"scratchpadShow", &xmToggleButtonWidgetClass, XuMENU_NONE, None, MENU_View_scratchpadShow,
			menu_cb, (XtPointer)MENU_View_scratchpadShow, NULL },
		{"display_sep1",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"mapOverlays", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_View_mapOverlays,
			menu_cb, (XtPointer)MENU_View_mapOverlays, NULL },
		{"fieldOnOff", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_View_fieldVisibility,
			menu_cb, (XtPointer)MENU_View_fieldVisibility, NULL },
		{"presetFieldDisplay", NULL, XuMENU_RADIO_LIST, None, MENU_View_presetField,
			NULL, (XtPointer)0, select_field_display_menu },
		NULL
	};

	static XuMenuItemStruct actions_menu[] = {
		{"coview", NULL, XuMENU_TEAR_OFF, None, MENU_Actions_coview,
			NULL, (XtPointer)0, coview_menu },
		{"zoom", NULL, XuMENU_NONE, None, NoId,
			menu_cb, (XtPointer)0, zoom_pullright_menu },
		NULL
	};

	static XuMenuItemStruct help_menu[] = {
		{"helpHelp", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_MAIN, NULL },
		{"helpAnimation", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_ANIMATION, NULL },
		{"helpEditAreas", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_EDIT_AREAS, NULL },
		{"helpEditLines", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_EDIT_LINES, NULL },
		{"helpEditSfc", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_EDIT_SFC, NULL },
		{"helpScratchpad", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_SCRATCHPAD, NULL },
		{"helpTimelink", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			HelpCB, HELP_TIMELINK, NULL },
		{"helpSep", &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"helpProblems", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			menu_cb, (XtPointer)MENU_Help_problems, NULL },
		{"helpAbout", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			menu_cb, (XtPointer)MENU_Help_about, NULL },
		NULL
	};

	static XuMenuBarItemStruct menu_bar[] = {
		{"depiction", None, NoId,              depict_menu  },
		{"options",   None, NoId,              option_menu  },
		{"products",  None, MENU_Bar_products, product_menu },
		{"guidance",  None, NoId,              guidance_menu},
		{"imagery",   None, NoId,              image_menu   },
		{"view",      None, NoId,              view_menu    },
		{"actions",   None, NoId,              actions_menu },
		{"help",      None, NoId,              help_menu    },
		NULL
	};

	/*=================== Create the main window ========================*/

	GW_mainWindow = XmCreateMainWindow(GW_topLevel,"mainWindow", NULL, 0);
	XtAddCallback(GW_mainWindow, XmNhelpCallback, HelpCB, HELP_MAIN);

	/*================== Create Menu Bar and all its children =======================*/


	topBar = XmVaCreateForm(GW_mainWindow, "topBar", NULL);

	/* Display of the cursor location on the map
	 */
	latLongFrame = XmVaCreateManagedFrame(topBar, "pf",
		XmNshadowType, XmSHADOW_OUT,
		XmNshadowThickness, 2,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* This string is just to set the size of the widget */
	label = XmStringCreateLocalized("000000N  0000000W");
	GW_latLongDisplay = XmVaCreateManagedLabel(latLongFrame, "latLongDisplay",
		XmNlabelString, label,
		XmNmarginWidth, 9,
		XmNalignment, XmALIGNMENT_CENTER,
		XmNrecomputeSize, False,
		NULL);
	XmStringFree(label);


	/* The help menu items for sending automatic mail bug reports will only
	*  be active if the mail address is in the resource file.  If it is not
	*  we eliminate these items from the help data structure.
	*/
	if(blank(XuGetStringResource(RNmailAddress, NULL)))
	{
		CopyStruct(&help_menu[7], &help_menu[9], XuMenuItemStruct, 2);
	}

	GW_menuBar = XuMenuBuildMenuBar(topBar, "menuBar", menu_bar);
	XtVaSetValues(GW_menuBar,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, latLongFrame,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	CreateCoviewPulldown(MENU_Actions_coview, my_argv0);
	CreateTimeStepPulldown(&time_step_label);

	/*================ Main Manager for all other activity ================*/

	GW_mainManager = XmVaCreateForm(GW_mainWindow, "mainManager",
		XmNshadowThickness, 0,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);

	/*======================= Icon Button Bar ==============================*/

	create_icon_bar(edit_icon_data, XtNumber(edit_icon_data));

	/*=============== Animation Control Form Manager =================*/

	GW_animationScaleManager = XmVaCreateFrame(GW_mainManager, "animationScaleManager",
		XmNshadowThickness, 2,
		XmNshadowType, XmSHADOW_OUT,
		XmNmarginWidth, 5,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, iconBar,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		NULL);

	/*================ Create the button bar and its children ==============*/

	buttonBar = XmVaCreateManagedFrame(GW_mainManager, "buttonBar",
		XmNmarginHeight, 2,
		XmNshadowType, XmSHADOW_OUT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, iconBar,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomWidget, iconBar,
		NULL);

	timeForm = XmVaCreateForm(buttonBar, "buttonBar",
		NULL);

	seqForm = XmVaCreateManagedForm(timeForm, "seqForm",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 200,				/* rough guess - see below for actual */
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	GW_sequenceSelector = XmVaCreateManagedRowColumn(seqForm, "sequenceSelector",
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		XmNorientation, XmHORIZONTAL,
		XmNspacing, 0,
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	arrowMinus = XmVaCreateManagedArrowButton(timeForm, "arrowMinus",
		XmNwidth, 22,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 1,
		XmNshadowThickness, 0,
		XmNarrowDirection, XmARROW_LEFT,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, seqForm,
		XmNleftOffset, 5,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
    XtAddCallback(arrowMinus, XmNactivateCallback, arrow_cb, (XtPointer)-1);

	/*
	 * This next label shows the depiction time step setting. The function
	 * CreateTimeStepPulldown must have been called before this point.
	 */
		GW_depictMinTimeStepLabel = XmVaCreateManagedLabel(timeForm, "arrowDelta",
		XmNshadowThickness, 0,
		XmNlabelString, time_step_label,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, arrowMinus,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	arrowPlus = XmVaCreateManagedArrowButton(timeForm, "arrowPlus",
		XmNwidth, 22,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 1,
		XmNshadowThickness, 0,
		XmNarrowDirection, XmARROW_RIGHT,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, GW_depictMinTimeStepLabel,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
    XtAddCallback(arrowPlus, XmNactivateCallback, arrow_cb, (XtPointer)1);

	depictionTimeDisplay = XmVaCreateManagedLabel(timeForm, "depictionTimeDisplay",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNrecomputeSize, False,
		XmNmarginHeight, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, arrowPlus,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	
	/*===================== Create the edit controls  ===================*/

	/* When using freetype2 fonts there was a problem in the editor panel
	 * that caused an XQueryColor error when the entire panel was set
	 * insensitive and then sensitive. Thus tabFrame2 is used to cover up
	 * the entire editor tab panel if input is to be restricted. This
	 * mainly happens when in guidance sampling mode.
	 */
	GW_tabFrame2 = XmVaCreateFrame(GW_mainManager, "tabFrame2",
		XmNshadowThickness, 2,
		XmNshadowType, XmSHADOW_OUT,
		XmNmarginHeight, 4,
		XmNmarginWidth, 1,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	w = XmVaCreateManagedForm(GW_tabFrame2, "tf2f", NULL);

	(void) XmVaCreateManagedLabel(w, "samplingInEffect", 
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 40,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	GW_tabFrame = XmVaCreateManagedFrame(GW_mainManager, "tabFrame",
		XmNshadowThickness, 2,
		XmNshadowType, XmSHADOW_OUT,
		XmNmarginHeight, 4,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, buttonBar,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* tabFrame2 must be created before tabFrame so that it will cover
	 * up tabFrame when managed but as it is attached to tabFrame the
	 * attachment must be done now.
	 */
	XtVaSetValues(GW_tabFrame2,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, GW_tabFrame,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, GW_tabFrame,
		XmNleftOffset, 0,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, GW_tabFrame,
		XmNbottomOffset, 0,
		NULL);

	/*=========== Sequence Selector Buttons Scroll bar =================*/

	timeScrollBar = XmVaCreateScrollBar(GW_mainManager, "timeScrollBar",
		XmNorientation, XmHORIZONTAL,
		XmNmaximum, TSBMS,
		XmNminimum, 0,
		XmNsliderSize, TSBMS,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, buttonBar,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(timeScrollBar, XmNvalueChangedCallback, time_scroll_cb, NULL);

	/*=============== Secondary field control ========================*/

	GW_secSeqManager = XmVaCreateManagedFrame(GW_mainManager, "secSeqManager",
		XmNmappedWhenManaged, False,
		XmNshadowType, XmSHADOW_OUT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, buttonBar,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		NULL);

	secSeqRC = XmVaCreateManagedRowColumn(GW_secSeqManager, "secSeqRC",
		XmNorientation, XmHORIZONTAL,
		XmNspacing, 2,
		NULL);

	GW_secSeqLabel = XmVaCreateManagedLabel(secSeqRC, "secSeqLabel", NULL);

	GW_secSeqBtns = XmVaCreateManagedRowColumn(secSeqRC, "secSeqBtns",
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		XmNspacing, 0,
		XmNorientation, XmHORIZONTAL,
		XmNpacking, XmPACK_COLUMN,
		XmNradioBehavior, True,
		NULL);

	w = XmVaCreateManagedPushButton(secSeqRC, "?", NULL);
	XtAddCallback(w, XmNactivateCallback, SecondaryBtnResetCB, NULL);

	/*=============== Create Ingred message display  ===================*/

	messageFrame = XmVaCreateManagedFrame(GW_mainManager, "messageFrame",
		XmNshadowType, XmSHADOW_OUT,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, GW_secSeqManager,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, GW_secSeqManager,
		NULL);

	GW_mainMessageBar = XmVaCreateManagedLabel(messageFrame, "messageBar",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNmarginWidth, 12,
		NULL);

	/*============ Reattachment Action ===============================*/

	/* This manager had to be created before the others so that it would
	*  appear on top when mapped, but now we need to set the attachments.
	*/
	XtVaSetValues(GW_animationScaleManager,
		XmNrightWidget, GW_tabFrame,
		XmNbottomWidget, GW_secSeqManager,
		NULL);

	/*============= Now the Ingred Drawing Window =====================*/

	GW_drawingWindow = XmVaCreateManagedScrolledWindow(GW_mainManager, "drawingWindow",
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, GW_secSeqManager,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	GW_mapWindow = XmVaCreateManagedDrawingArea(GW_drawingWindow, "mapWindow", NULL);

	CreateContextMenus();

	w = XmVaCreateScrollBar(GW_drawingWindow, "zoom_scroll_right",
		XmNorientation, XmVERTICAL,
		XmNmaximum, 100/ZOOM_LINE_SIZE,
		XmNpageIncrement, 100/ZOOM_LINE_SIZE,
		XmNsliderSize, 100/ZOOM_LINE_SIZE,
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, VerticalZoomScrollBarCB, NULL);

	w = XmVaCreateScrollBar(GW_drawingWindow, "zoom_scroll_bottom",
		XmNorientation, XmHORIZONTAL,
		XmNmaximum, 100/ZOOM_LINE_SIZE,
		XmNpageIncrement, 100/ZOOM_LINE_SIZE,
		XmNsliderSize, 100/ZOOM_LINE_SIZE,
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, HorizontalZoomScrollBarCB, NULL);

	/*=================== Set the areas of the main window =====================*/

	XmMainWindowSetAreas(GW_mainWindow,topBar,NULL,NULL,NULL,GW_mainManager);

	/* This must wait until after all else has been created */
	CreateTabPanels();

	XtManageChild(GW_menuBar);
	XtManageChild(topBar);
	XtManageChild(timeForm);
	XtManageChild(GW_mainManager);
	XtManageChild(GW_mainWindow);
	XuRealizeApplication(GW_topLevel);
	XuSetDialogCursor(GW_topLevel, XuDEFAULT_CURSOR, ON);

	/* Here we align the end of the depiction sequence selection buttons
	*  with the left edge of the editor panel.
	*/
	XuDelay(GW_tabFrame, 100);
	XtVaGetValues(GW_tabFrame, XmNwidth, &width, NULL);
	XtVaSetValues(seqForm, XmNrightOffset, (int)width, NULL);

	/* Finally we add in the map window resize callback to allow us to handle a
	 * series of time buttons which take up more than the allowed space.
	 */
	XtAddCallback(GW_mapWindow, XmNresizeCallback, drawing_resize_cb, NULL);
}


/*  Create as much of the interface as is required for viewing of depictions only.
*/
static void create_view_window(void)
{
	Widget w, arrowPlus, arrowMinus, secSeqRC, timeForm, messageFrame;
	Widget topBar, latLongFrame;
	XmString label;

	/* Widgets, pixmaps and actions to be found on the coview icon bar.
	 */
	static ICON_DATA coview_icon_data[] = {
		{"guidance_select", "guidance-select",GUIDANCE_SELECT_ICON,   0,MENU_Guidance_select,      3,2,False,True },
		{NULL,              "bookshelf-full", GUIDANCE_STATUS_ICON,   0,MENU_Guidance_status,      3,2,True, True },
		{NULL,              "redflag",        GUID_SELECT_STATUS_ICON,0,MENU_Bar_guidance_update,  3,2,True, False},
		{NULL,              NULL,             SEPARATOR_ICON,         0,0,                         3,2,True, True },
		{"map_overlays",    "map-overlay",    MAP_OVERLAYS_ICON,      0,MENU_View_mapOverlays,     3,2,True, True },
		{"field_display",   "field-control",  FIELD_DISPLAY_ICON,     0,MENU_View_fieldVisibility, 3,2,True, True },
		{ NULL,             NULL,             SEPARATOR_ICON,         0,0,                         3,2,True, True },
		{"zoom_in",         "zoom-in",        ZOOM_IN_ICON,           0,MENU_Actions_Zoom_in,      3,2,True, True },
		{"zoom_out",        "zoom-out",       ZOOM_OUT_ICON,          0,MENU_Actions_Zoom_out,     3,2,False,True },
		{"zoom_pan",        "zoom-pan",       ZOOM_PAN_ICON,          0,MENU_Actions_Zoom_pan,     3,2,False,True }
	};


	/* Define the data structures used to create the main pulldown menu and
	*  all of its pull right menus.  See the library header Xu.h for
	*  details as to the content of this structure.
	*/

	static XuMenuItemStruct pullright_print_menu[] = {
		NULL
	};

	static XuMenuItemStruct depict_menu[] = {
		{"print", NULL, XuMENU_NONE, None, MENU_Depiction_print,
			NULL, (XtPointer)0, pullright_print_menu },
		{"sep4", &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"quit", &xmPushButtonWidgetClass, XuMENU_NONE, None, NoId,
			MainExitCB, NULL, NULL },
		NULL
	};

	static XuMenuItemStruct option_menu[] = {
		{"preferences", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Option_preferences,
			menu_cb, (XtPointer)MENU_Option_preferences, NULL },
		NULL
	};

	static XuMenuItemStruct guidance_menu[] = {
		{"status", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_status,
			menu_cb, (XtPointer)MENU_Guidance_status, NULL },
		{"availability", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_availability,
			menu_cb, (XtPointer)MENU_Guidance_availability, NULL },
		{"guidanceDisplayed", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_displayed,
			menu_cb, (XtPointer)MENU_Guidance_displayed, NULL },
		{"sep",  &xmSeparatorWidgetClass, XuMENU_NONE, None, NoId,
			NULL, (XtPointer)0, NULL },
		{"guidanceSelect", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_select,
			menu_cb, (XtPointer)MENU_Guidance_select, NULL },
		{"guidanceHide", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_hide,
			menu_cb, (XtPointer)MENU_Guidance_hide, NULL },
		{"guidanceShow", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Guidance_show,
			menu_cb, (XtPointer)MENU_Guidance_show, NULL },
		NULL
	};


	static XuMenuItemStruct select_field_display_menu[] = {
		NULL
	};

	static XuMenuItemStruct view_menu[] = {
		{"mapOverlays", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_View_mapOverlays,
			menu_cb, (XtPointer)MENU_View_mapOverlays, NULL },
		{"fieldOnOff", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_View_fieldVisibility,
			menu_cb, (XtPointer)MENU_View_fieldVisibility, NULL },
		{"presetFieldDisplay", NULL, XuMENU_RADIO_LIST, None, MENU_View_presetField,
			NULL, (XtPointer)0, select_field_display_menu },
		NULL
	};

	static XuMenuItemStruct actions_menu[] = {
		{"zoomIn", &xmPushButtonWidgetClass, XuMENU_NONE, None, MENU_Actions_Zoom_in,
			menu_cb, (XtPointer)MENU_Actions_Zoom_in, NULL },
		{"zoomOut", &xmPushButtonWidgetClass, XuMENU_INSENSITIVE, None, MENU_Actions_Zoom_out,
			menu_cb, (XtPointer)MENU_Actions_Zoom_out, NULL },
		{"zoomPan", &xmPushButtonWidgetClass, XuMENU_INSENSITIVE, None, MENU_Actions_Zoom_pan,
			menu_cb, (XtPointer)MENU_Actions_Zoom_pan, NULL },
		{"zoomExit", &xmPushButtonWidgetClass, XuMENU_INSENSITIVE, None, MENU_Actions_Zoom_exit,
			menu_cb, (XtPointer)MENU_Actions_Zoom_exit, NULL },
		NULL
	};

	static XuMenuBarItemStruct menu_bar[] = {
		{"depiction", None, NoId, depict_menu  },
		{"options",   None, NoId, option_menu  },
		{"guidance",  None, NoId, guidance_menu},
		{"view",      None, NoId, view_menu    },
		{"actions",   None, NoId, actions_menu },
		NULL
	};


	/*=================== Create the main window ========================*/

	GW_mainWindow = XmCreateMainWindow(GW_topLevel,"mainWindow", NULL, 0);

	/*================== Create Menu Bar and all its children =======================*/

	topBar = XmVaCreateForm(GW_mainWindow, "topBar", NULL);

	/* Display of the cursor location on the map
	 */
	latLongFrame = XmVaCreateManagedFrame(topBar, "pf",
		XmNshadowType, XmSHADOW_OUT,
		XmNshadowThickness, 2,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	label = XmStringCreateLocalized("000000N  0000000W");
	GW_latLongDisplay = XmVaCreateManagedLabel(latLongFrame, "latLongDisplay",
		XmNlabelString, label,
		XmNalignment, XmALIGNMENT_CENTER,
		XmNrecomputeSize, False,
		NULL);
	XmStringFree(label);

	GW_menuBar = XuMenuBuildMenuBar(topBar, "menuBar", menu_bar);
	XtVaSetValues(GW_menuBar,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, latLongFrame,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/*================ Main Manager for all other activity ================*/

	GW_mainManager = XmVaCreateForm(GW_mainWindow, "mainManager",
		XmNshadowThickness, 0,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);

	/*======================= Icon Button Bar ==============================*/

	create_icon_bar(coview_icon_data, XtNumber(coview_icon_data));
	
	/*================ Create the button bar and its children ==============*/

	buttonBar = XmVaCreateManagedFrame(GW_mainManager, "buttonBar",
		XmNmarginHeight, 2,
		XmNshadowType, XmSHADOW_OUT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, iconBar,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomWidget, iconBar,
		NULL);

	timeForm = XmVaCreateForm(buttonBar, "buttonBar",
		NULL);

	depictionTimeDisplay = XmVaCreateManagedLabel(timeForm, "depictionTimeDisplay",
		XmNresizable, False,
		XmNwidth, PANEL_WIDTH,
		XmNalignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 6,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	arrowPlus = XmVaCreateManagedArrowButton(timeForm, "arrowPlus",
		XmNwidth, 22,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNshadowThickness, 0,
		XmNarrowDirection, XmARROW_RIGHT,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, depictionTimeDisplay,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
    XtAddCallback(arrowPlus, XmNactivateCallback, arrow_cb, (XtPointer)1);

	arrowMinus = XmVaCreateManagedArrowButton(timeForm, "arrowMinus",
		XmNwidth, 22,
		XmNhighlightOnEnter, True,
		XmNhighlightThickness, 2,
		XmNshadowThickness, 0,
		XmNarrowDirection, XmARROW_LEFT,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, arrowPlus,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
    XtAddCallback(arrowMinus, XmNactivateCallback, arrow_cb, (XtPointer)-1);

	seqForm = XmVaCreateManagedForm(timeForm, "seqForm",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, arrowMinus,
		XmNrightOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	GW_sequenceSelector = XmVaCreateManagedRowColumn(seqForm, "sequenceSelector",
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		XmNorientation, XmHORIZONTAL,
		XmNspacing, 0,
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	
	/*============= Group and Field Control Panel =====================*/

	GW_tabFrame = XmVaCreateManagedFrame(GW_mainManager, "tabFrame",
		XmNresizable, False,
		XmNshadowThickness, 2,
		XmNshadowType, XmSHADOW_OUT,
		XmNmarginHeight, 4,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, buttonBar,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);


	/*=========== Sequence Selector Buttons Scroll bar =================*/

	timeScrollBar = XmVaCreateScrollBar(GW_mainManager, "timeScrollBar",
		XmNorientation, XmHORIZONTAL,
		XmNmaximum, TSBMS,
		XmNminimum, 0,
		XmNsliderSize, TSBMS,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, buttonBar,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(timeScrollBar, XmNvalueChangedCallback, time_scroll_cb, NULL);

	/*=============== Secondary field control ========================*/

	GW_secSeqManager = XmVaCreateManagedFrame(GW_mainManager, "secSeqManager",
		XmNmappedWhenManaged, False,
		XmNshadowType, XmSHADOW_OUT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, buttonBar,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		NULL);

	secSeqRC = XmVaCreateManagedRowColumn(GW_secSeqManager, "secSeqRC",
		XmNorientation, XmHORIZONTAL,
		XmNspacing, 2,
		NULL);

	GW_secSeqLabel = XmVaCreateManagedLabel(secSeqRC, "secSeqLabel", NULL);

	GW_secSeqBtns = XmVaCreateManagedRowColumn(secSeqRC, "secSeqBtns",
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		XmNspacing, 0,
		XmNorientation, XmHORIZONTAL,
		XmNpacking, XmPACK_COLUMN,
		XmNradioBehavior, True,
		NULL);

	w = XmVaCreateManagedPushButton(secSeqRC, "?", NULL);
	XtAddCallback(w, XmNactivateCallback, SecondaryBtnResetCB, NULL);


	/*=============== Create Ingred message display  ===================*/

	messageFrame = XmVaCreateManagedFrame(GW_mainManager, "messageFrame",
		XmNshadowType, XmSHADOW_OUT,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, GW_secSeqManager,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, GW_secSeqManager,
		NULL);

	GW_mainMessageBar = XmVaCreateManagedLabel(messageFrame, "messageBar",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNmarginWidth, 12,
		NULL);


	/*============= Now the Ingred Drawing Window =====================*/

	GW_drawingWindow = XmVaCreateManagedScrolledWindow(GW_mainManager, "drawingWindow",
		XmNscrollingPolicy, XmAPPLICATION_DEFINED,
		XmNvisualPolicy, XmVARIABLE,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, GW_secSeqManager,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, GW_tabFrame,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	GW_mapWindow = XmVaCreateManagedDrawingArea(GW_drawingWindow, "mapWindow", NULL);

	CreateContextMenus();

	w = XmVaCreateScrollBar(GW_drawingWindow, "zoom_scroll_right",
		XmNorientation, XmVERTICAL,
		XmNmaximum, 100/ZOOM_LINE_SIZE,
		XmNpageIncrement, 100/ZOOM_LINE_SIZE,
		XmNsliderSize, 100/ZOOM_LINE_SIZE,
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, VerticalZoomScrollBarCB, NULL);

	w = XmVaCreateScrollBar(GW_drawingWindow, "zoom_scroll_bottom",
		XmNorientation, XmHORIZONTAL,
		XmNmaximum, 100/ZOOM_LINE_SIZE,
		XmNpageIncrement, 100/ZOOM_LINE_SIZE,
		XmNsliderSize, 100/ZOOM_LINE_SIZE,
		NULL);
	XtAddCallback(w, XmNvalueChangedCallback, HorizontalZoomScrollBarCB, NULL);

	/*=================== Set the areas of the main window =====================*/

	XmMainWindowSetAreas(GW_mainWindow,topBar,NULL,NULL,NULL,GW_mainManager);

	/* This needs to be done now as we need the mapWindow to exist
	 */
	w = XmVaCreateForm(GW_tabFrame, "form",
		XmNresizable, False,
		XmNwidth, PANEL_WIDTH,
		NULL);

	CreateViewerModeControlPanel(w);
	XtManageChild(w);

	XtManageChild(GW_menuBar);
	XtManageChild(topBar);
	XtManageChild(timeForm);
	XtManageChild(GW_mainManager);
	XtManageChild(GW_mainWindow);
	XtRealizeWidget(GW_topLevel);
	XuSetDialogCursor(GW_topLevel, XuDEFAULT_CURSOR, ON);

	/* Finally we add in the map window resize callback to allow us to handle a
	 * series of time buttons which take up more than the allowed space.
	 */
	XtAddCallback(GW_mapWindow, XmNresizeCallback, drawing_resize_cb, NULL);
}
