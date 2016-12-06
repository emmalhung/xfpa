/****************************************************************************/
/*
*  File:	imagery_controlDialog.c
*
*  Purpose:	 Provides the dialog for controlling logic for imagery selection.
*
*  Note:     Be careful of the time lists. The times occur in reverse order
*            so that times[0] contains the most recent time.
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

#include "global.h"
#include <FpaXgl.h>
#include <graphics.h>
#include <ingred.h>
#include <Xm/ArrowB.h>
#include <Xm/CascadeB.h>
#include <Xm/ComboBox.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/XmpSpinBox.h>
#include <Xm/ToggleB.h>
#include <Xm/TabStack.h>
#include "depiction.h"
#include "editor.h"
#include "observer.h"
#include "resourceDefines.h"
#include "timelists.h"
#include "selector.h"
#include "source.h"
#include "contextMenu.h"
#include "imagery.h"


/* Keys for the state store
 */
#define IMKEY	"img"
#define MNKEY	"panel"
#define OPTKEY  "options"

/* General macros
 */
#define NONE -1


/* These apply to the arrows at the end of the animation window controls and
 * define the times for the looping when the arrows are held down by the user.
 */
#define LOOP_DELAY_BEGIN	500
#define	LOOP_DELAY_DECREASE	50
#define LOOP_DELAY_MIN		100

/* Required for the pulldown menus
 */
#define RadarStatTableId    920
#define SyncToggleId        921
#define TimeToggleId        922
#define BlendToggleId		923
#define BrightnessToggleId	924
#define LegendToggleId      925
#define NoRingsId			926
#define RangeRingOnlyId		927
#define AllRingsId			928
#define RingDeltaId			929


/* Forward function definitions
 */
static void    activate_sample_cursor    ( Boolean );
static void    add_site_to_context_menu  ( XmString, int );
static void    add_value_to_context_menu ( XmString, int );
static void    animate_cb                ( Widget, XtPointer, XtPointer );
static void    animation_lockout_observer( String*, int);
static void    animation_status_observer ( CAL, String*, int );
static void    blend_amount_cb           ( Widget, XtPointer, XtPointer );
static void    brightness_cb             ( Widget, XtPointer, XtPointer );
static void    clear_btn_cb              (Widget, XtPointer, XtPointer);
static void    create_context_menu       (void);
static void    do_blending_cb            ( Widget, XtPointer, XtPointer );
static void    do_brightness_cb          ( Widget, XtPointer, XtPointer );
static void    start_sampling_mode       ( void );
static void    reset_sampling_list       ( void );
static void    end_sampling_mode         ( void );
static void    exit_cb                   ( Widget, XtPointer, XtPointer );
static void    fill_in_lut_selections    ( IMDAT*, Boolean );
static void    generate_timelist         ( void );
static Boolean have_data_images          ( void );
static Boolean new_imagery_observer      ( Boolean );
static void    hold_time_cb              ( Widget, XtPointer, XtPointer );
static void    site_cb                   ( Widget, XtPointer, XtPointer );
static void    lut_cb                    ( Widget, XtPointer, XtPointer );
static void    on_off_cb                 ( Widget, XtPointer, XtPointer );
static void    prod_cb                   ( Widget, XtPointer, XtPointer );
static void    read_state_file           ( void );
static void    ready_imagery_observer    ( String*, int );
static void    ring_spacing_cb           ( Widget, XtPointer, XtPointer );
static void    sample_list_cb            ( Widget, XtPointer, XtPointer );
static void    sample_item_list_cb       ( Widget, XtPointer, XtPointer );
static void    sampling_lockout_observer ( String*, int );
static void    select_images_cb          ( Widget, XtPointer, XtPointer );
static void    select_all_of_list_cb     ( Widget, XtPointer, XtPointer );
static void    set_animation_time_window ( TimeWindowSelectorStruct* );
static void    set_font_attributes       ( FontSelectorStruct * );
static void    set_sample_tab_sensitivity( void );
static void    set_time_label            ( Widget, int );
static void    show_legend_cb            ( Widget, XtPointer, XtPointer );
static void    show_rings_cb             ( Widget, XtPointer, XtPointer );
static void    show_selections           ( void );
static Boolean site_wp                   ( XtPointer );
static void    speed_cb                  ( Widget, XtPointer, XtPointer );
static void    stat_table_cb             ( Widget, XtPointer, XtPointer );
static void    step_cb                   ( Widget, XtPointer, XtPointer );
static void    sync_cb                   ( Widget, XtPointer, XtPointer );
static void    sync_pushButton_cb        ( Widget, XtPointer, XtPointer );
static void    sync_selections           ( String );
static void    tabs_cb                   ( Widget, XtPointer, XtPointer );
static void    time_window_cb            ( Widget, XtPointer, XtPointer );
static void    imagery_update_observer   ( String*, int );
static void    update_time_labels        ( void );

/* Static variables
*/
static Widget  dialog              = NullWidget;
static Widget  menuBar             = NullWidget;
static Widget  syncPushButton      = NullWidget;
static Widget  tabStack            = NullWidget;
static Widget  selectTab           = NullWidget;
static Widget  optionsTab          = NullWidget;
static Widget  animateTab          = NullWidget;
static Widget  sampleTab           = NullWidget;
static Widget  animationTimeWindow = NullWidget;
static Widget  animateStartBtn     = NullWidget;
static Widget  animateStopBtn      = NullWidget;
static Widget  timeScale           = NullWidget;
static Widget  currentTimeLabel    = NullWidget;
static Widget  startTimeLabel      = NullWidget;
static Widget  endTimeLabel        = NullWidget;
static Widget  fontSetManager      = NullWidget;
static Widget  sampleList          = NullWidget;
static Widget  sampleItemList      = NullWidget;
static Widget  onOffGroup          = NullWidget;
static Widget  onBtn               = NullWidget;
static Widget  offBtn              = NullWidget;
static Widget  clearBtn            = NullWidget;
static Pixmap  syncPixmap[2];

/* context menu widgets
 */
static Widget  panelContextMenu     = NullWidget;
static Widget  contextSitePulldown  = NullWidget;
static int     ncontext_site_btns   = 0;
static Widget *contextSiteBtns      = NULL;
static Widget  contextValuePulldown = NullWidget;
static int     ncontext_value_btns  = 0;
static Widget *contextValueBtns     = NULL;

/* Common variables.
 */
static IMDAT   images[NIMAGES];
static int     active_time_ndx = NONE;

/* Option setting variables
 */
static Boolean sync_with_depictions   = True;
static Boolean use_time_window        = True;
static int     blend_amount           = 50;
static Boolean show_radar_range_rings = True;
static int     radar_ring_spacing     = 40;
static Boolean show_limit_ring_only   = False;
static Boolean show_legend            = False;
static Boolean blend_on               = False;
static int     blend_minimum          = 50;
static Boolean brightness_on          = False;

/* Structure to hold information required to create selection and control structures
 * for all of the image types. This allows the use of loops instead of a lot of
 * repeating code. Note that if any other image types are added in future just add
 * to the bottom of this structure and arrange the order with the order array. This
 * will ensure that the state store information is automatically backwards compatable.
 *
 * NOTE: Do not change the order of the items in this structure.
 *
 * The following macros are defined to index into the settings array for radar and
 * satellite images respectively.
 */
#define R 0
#define S 2
struct {
	String name;			/* Image name from images.h */
	int    type;			/* Image type from glib.h library header */
	int    show_always;		/* Always display the controls for this image type? */
	int    islay;			/* Is this an underlay or overlay? */
	int    max;				/* Maximum acceptance time (window in minutes) */
	int    interval;		/* Time increment interval */
	int    hold_default;	/* Default value for the time to hold the image during sync (minutes) */
	int    bright_min;		/* Minimum brightness setting */
	int    bright_max;		/* Maximum brightness setting */
	Widget w;				/* General use widget */
} settings[NIMAGES] = {
	{RADAR_NAME,     ImageTypeRadar,     True,  False, 15, 5,  5, 50, 100, NULL}, 
	{OVERLAY_NAME,   ImageTypeOverlay,   False, True,  30, 5, 15, 30, 100, NULL}, 
	{SATELLITE_NAME, ImageTypeSatellite, True,  False, 30, 5, 15, 30, 100, NULL}, 
	{UNDERLAY_NAME,  ImageTypeUnderlay,  False, True,  30, 5, 15, 30, 100, NULL} 
};

/* Order that the image types above will appear in selectors. The defined
 * macros are the element of the image_display array to be used to set
 * the appropriate control element.
 */
#define IMAGE_ORDER		0	/* Order of the image selection controls - left to right */
#define BRIGHT_ORDER	1	/* Order of the brightness sliders - bottom to top */
#define HOLD_ORDER		1	/* Order of the hold over time controls - bottom to top*/

struct {
	int order[NIMAGES];		/* Display order of settings */
	String resource_name;	/* Resource file identifier */
} image_display[2] = {
	{3, 2, 0, 1, RNimagerySelectionOrder},
	{1, 0, 2, 3, RNimagerySettingsOrder}
};

/* This is the list of range intervals that the rings around radar data images will be drawn.
 * It is done this way so that in future the possibility exists for these ranges to be set
 * in the setup or config file.
 */
static int default_radar_ring_ranges[] = {5,10,15,20,25,30,40,50,60};
static int *radar_ring_ranges = default_radar_ring_ranges;
static int nradar_ring_ranges = XtNumber(default_radar_ring_ranges);


/* animation variables
 */
static Boolean animation_running  = False;
static int     animate_speed      = 1;
static int     animate_begin_ndx  = 0;
static int     animate_end_ndx    = 0;
static TSTAMP  animate_begin_time = {0};
static TSTAMP  animate_end_time   = {0};

/* sampling related variables
 */
static Boolean    sampling_active      = False;
static Boolean    sampling_lockout     = False;
static Boolean    sample_cursor_on     = False;
static XmString   last_sample_select   = NullXmString;
static IMDAT     *sample_type          = (IMDAT*)0;
static int        sample_type_list_len = 0;
static IMDAT    **sample_type_list     = (IMDAT**)0;
static int       *sample_type_index    = (int*)0;

/* all_times is the composite time list of both radar and satellite times. Every entry
 * in this list must have a time, where as the radar and satellite lists can have
 * enries of no_time_match, not_available or NULL if a time has not been assigned yet
 */
static String *all_times = (String*)0;
static int     ntimes    = 0;
static int     max_times = 0;

/* First instance of this dialog?
 */
static Boolean first_creation = True;


/*========================= PUBLIC FUNCTIONS ===============================*/



/*=========================================================================*/
/*
 * InitializeImagery() - Initialize imagery data.
 */
/*=========================================================================*/
void InitImagery(void)
{
	int       i, n;
	String    colour, ptr, *tags, *labels;
	Colormap  cmap;
	UNCHAR    ring_colour[3];
	PARM      *setup;

	/* Get the display order of the image types from the resource file. This is
	 * a space separated list of the display type identifiers.
	 */
	for(i = 0; i < XtNumber(image_display); i++)
	{
		if((ptr = XuGetStringResource(image_display[i].resource_name, NULL)))
		{
			int    k = 0;
			String buf = XtNewString(ptr);

			while((ptr = string_arg(buf)) && k < NIMAGES)
			{
				for(n = 0; n < NIMAGES; n++)
				{
					if(!same_ic(settings[n].name, ptr)) continue;
					image_display[i].order[k++] = n;
					break;
				}
				if(n >= NIMAGES)
					pr_error("InitImagery","Unrecognized image type \"%s\" associated with resource file entry \"%s\"\n",
					ptr, image_display[i].resource_name);
			}
			FreeItem(buf);
		}
	}


	/* Get the minimum amount of radar blending from the setup file. Note
	 * that the maximum value of the minimum is limited to 90%
	 */
	if((setup = GetSetupParms(IMAGERY_BLEND)))
	{
		blend_minimum = atoi(setup->parm[0]);
		if(blend_minimum < 0) blend_minimum = 0;
		if(blend_minimum > 90) blend_minimum = 90;
	}

	/* Get the ring spacing from the setup file. All distances must be in kilometres.
	 */
	if((setup = GetSetupParms(IMAGERY_RING_SPACE)) && setup->nparms > 1)
	{
		nradar_ring_ranges = setup->nparms;
		radar_ring_ranges = NewIntArray(nradar_ring_ranges);
		for(n = 1; n < nradar_ring_ranges; n++)
			radar_ring_ranges[n] = atoi(setup->parm[n+1]);
	}
			

	for(n = 0; n < NIMAGES; n++)
	{
		IMDAT *im = images+n;

		(void) memset((void*)im, 0, sizeof(IMDAT));

		im->name           = settings[n].name;
		im->display        = True;
		im->brightness     = 1.0;
		im->pre_hold_time  = -settings[n].max;
		im->post_hold_time = settings[n].max;

		im->nprod = glImageInfoGetProducts( settings[n].type, &tags, &labels );
		im->prod  = NewMem(PRODINFO, im->nprod);
		for( i = 0; i < im->nprod; i++ )
		{
			im->prod[i].tag    = tags[i];
			im->prod[i].label  = labels[i];
			im->prod[i].lutndx = NONE;
		}
		glFree(tags);
		glFree(labels);

		im->nsite = glImageInfoGetSites( settings[n].type, &labels );
		im->site = NewMem(SITEINFO, im->nsite);
		for(i = 0; i < im->nsite; i++)
		{
			im->site[i].label = labels[i];
		}
		glFree(labels);

		/* This allows us to display the data selectors only if there is data */
		im->exists = (settings[n].show_always || (im->nprod > 0 && im->nsite > 0));

		/* Check the setup file for an override of the ranges for the brightness
		*  sliders. Min must be >= 0 and max > min.
		*/
		if((setup = GetSetupKeyParms(IMAGERY_BRIGHTNESS, im->name)) && setup->nparms >= 3)
		{
			i = atoi(setup->parm[1]);
			if(i >= 0) settings[n].bright_min = i;
			i = atoi(setup->parm[2]);
			if(i > settings[n].bright_min) settings[n].bright_max = i;
		}

	}

	/* The state file can only be read after this point when things have
	 * been initialzed.
	 */
	read_state_file();

	/* Check some state variables
	 */
	for(n = 0; n < NIMAGES; n++)
	{
		IMDAT *im = images+n;

		/* Force the returned value to be an increment of the interval value
		 * then check the range for validity. We do this for all of the time
		 * window values.
		 */
		im->pre_hold_time = (im->pre_hold_time/settings[n].interval) * settings[n].interval;
		if(im->pre_hold_time > 0 || im->pre_hold_time < -settings[n].max)
			im->pre_hold_time = -settings[n].hold_default;

		im->post_hold_time = (im->post_hold_time/settings[n].interval) * settings[n].interval;
		if(im->post_hold_time < 0 || im->post_hold_time > settings[n].max)
			im->post_hold_time = settings[n].hold_default;

		if(brightness_on)
			glImageTypeSetBrightness(settings[n].type, im->brightness);
		else
			glImageTypeSetBrightness(settings[n].type, glNORMAL_BRIGHTNESS);
	}
	

	/* Check for resource file override of the radar range ring colour. We
	 * use GW_mainWindow here for reference as this is where the images go
	 * and who knows where this panel may be in future.
	 */
	XtVaGetValues(GW_mainWindow, XmNcolormap, &cmap, NULL);
	colour = XuGetStringResource(RNradarRangeRingColor, NULL);
	if(colour)
	{
		XColor xc, nc;
		XLookupColor(XtDisplay(GW_mainWindow), cmap, colour, &xc, &nc);
		ring_colour[0] = (UNCHAR)(nc.red   >> 8);
		ring_colour[1] = (UNCHAR)(nc.green >> 8);
		ring_colour[2] = (UNCHAR)(nc.blue  >> 8);
		glImageSetRadarRangeRingColor(ring_colour, NULL);
	}
	colour = XuGetStringResource(RNradarRangeLimitColor, NULL);
	if(colour)
	{
		XColor xc, nc;
		XLookupColor(XtDisplay(GW_mainWindow), cmap, colour, &xc, &nc);
		ring_colour[0] = (UNCHAR)(nc.red   >> 8);
		ring_colour[1] = (UNCHAR)(nc.green >> 8);
		ring_colour[2] = (UNCHAR)(nc.blue  >> 8);
		glImageSetRadarRangeRingColor(NULL, ring_colour);
	}
	
	animate_speed = GV_animation_max_delay/2 + 1;

	if (blend_on)
		(void) IngredVaCommand(GE_IMAGERY, "BLEND ON %d", blend_amount);

	create_context_menu();

	glImageShowRadarRangeRings(show_radar_range_rings, show_limit_ring_only? 0:radar_ring_spacing);

	AddObserver(OB_ANIMATION_RUNNING, animation_lockout_observer);
	AddObserver(OB_DEPICTION_CHANGE, imagery_update_observer);
	AddObserver(OB_DEPICTION_TZERO_CHANGE, imagery_update_observer);
	AddObserver(OB_DEPICTION_ABOUT_TO_CHANGE, ready_imagery_observer);
	AddObserver(OB_DIALOG_SAMPLING, sampling_lockout_observer);
	AddIngredObserver(animation_status_observer);
	AddSourceObserver(new_imagery_observer,"NewImagery");
}


/*=========================================================================*/
/*
 *  Create the dialog for controlling the imagery.
 */
/*=========================================================================*/
void ACTIVATE_imageryControlDialog(Widget reference_widget)
{
	int       i, n, nlist, ncols;
	Boolean   first, found;
	Cardinal  ac;
	Widget    topForm, menuBarForm;
	Widget    pulldown, brightFrame, blendFrame, holdFrame;
	Widget    form, form1, form2, rc, frame, label;
	Widget    arrow_right, arrow_left, slider;
	Widget    btn, ll, lr, spin1, spin2, sampleGrid;
	Widget    predefPoints;
	Dimension width, width1;
	XmString  item, *list;
	Arg       args[14];

	static XuMenuItemStruct list_menu[] = {
		{"exit", &xmPushButtonWidgetClass, 0, None, NoId,
			exit_cb, NULL, NULL },
		NULL
	};

	static XuMenuItemStruct rings_pullright_menu[] = {
		{"noRings", &xmToggleButtonWidgetClass, XuMENU_NONE, None, NoRingsId,
			show_rings_cb, (XtPointer)0, NULL },
		{"showLimitRingOnly", &xmToggleButtonWidgetClass, XuMENU_NONE, None, RangeRingOnlyId,
			show_rings_cb, (XtPointer)1, NULL },
		{"showRings", &xmToggleButtonWidgetClass, XuMENU_NONE, None, AllRingsId,
			show_rings_cb, (XtPointer)2, NULL },
		NULL
	};

	static XuMenuItemStruct ring_spacing_pullright_menu[] = {
		NULL
	};

	static XuMenuItemStruct option_menu[] = {
		{"showRadarStatTable", &xmPushButtonWidgetClass, 0, None, RadarStatTableId,
			stat_table_cb, NULL, NULL },
		{"sync", &xmToggleButtonWidgetClass, 0, None, SyncToggleId,
			sync_cb, NULL, NULL },
		{"useTimeWindow", &xmToggleButtonWidgetClass, 0, None, TimeToggleId,
			time_window_cb, NULL, NULL },
		{"blendBtn", &xmToggleButtonWidgetClass, 0, None, BlendToggleId,
			do_blending_cb, NULL, NULL },
		{"brightBtn", &xmToggleButtonWidgetClass, 0, None, BrightnessToggleId,
			do_brightness_cb, NULL, NULL },
		{"showLegend", &xmToggleButtonWidgetClass, 0, None, LegendToggleId,
			show_legend_cb, NULL, NULL },
		{"radarOptions", NULL, XuMENU_RADIO_LIST, None, NoId,
			show_rings_cb, NULL, rings_pullright_menu },
		{"ringDelta", NULL, XuMENU_RADIO_LIST, None, RingDeltaId,
			ring_spacing_cb, NULL, ring_spacing_pullright_menu },
		NULL
	};

	static XuMenuBarItemStruct menu_bar[] = {
		{"imagery", None, NoId, list_menu},
		{"viewBtn", None, NoId, option_menu},
		NULL
	};


	if(NotNull(dialog))
	{
		XuShowDialog(dialog);
		return;
	}


    dialog = XuCreateMainWindowDialog(reference_widget, "imageryControlDialog",
		XuNallowIconify, True,
		XuNmwmDeleteOverride, exit_cb,
        NULL);

	syncPixmap[0] = XuGetPixmap(dialog, "lock-off-24x24"),
	syncPixmap[1] = XuGetPixmap(dialog, "lock-on-24x24"),

	topForm = XmVaCreateForm(dialog, "topForm",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	/* The following complexity is due to the fact we want a status indicator for
	 * the sync with depictions toggle and it must be the same height as the menu
	 * bar for best appearance.
	 */
	menuBarForm = XmVaCreateForm(topForm, "form",
		XmNhorizontalSpacing, 0,
		XmNverticalSpacing, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);

	syncPushButton = XmVaCreateManagedPushButton(menuBarForm, "syncStatus",
		XmNlabelType, XmPIXMAP,
		XmNlabelPixmap, syncPixmap[sync_with_depictions?1:0],
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(syncPushButton, XmNactivateCallback, sync_pushButton_cb, NULL);

	menuBar = XuMenuBuildMenuBar(menuBarForm, "menuBar", menu_bar);
	XtVaSetValues(menuBar,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, syncPushButton,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	/* If the radar statistics table (SCIT) is not used turn off the launch button */
	if(!RadarStatSystemInitialized())
		XuMenuButtonSetVisibility(menuBar, RadarStatTableId, False);

	XtManageChild(menuBar);
	XtManageChild(menuBarForm);

	XuMenuToggleSetState(menuBar, SyncToggleId,       sync_with_depictions);
	XuMenuToggleSetState(menuBar, TimeToggleId,       use_time_window);
	XuMenuToggleSetState(menuBar, BlendToggleId,      blend_on);
	XuMenuToggleSetState(menuBar, BrightnessToggleId, brightness_on);
	XuMenuToggleSetState(menuBar, LegendToggleId,     show_legend);
	if(show_limit_ring_only)
		XuMenuToggleSetState(menuBar, RangeRingOnlyId, True);
	else if(show_radar_range_rings)
		XuMenuToggleSetState(menuBar, AllRingsId, True);
	else
		XuMenuToggleSetState(menuBar, NoRingsId, True);

	/* Create the list of ring ranges from our array. This allows for a possible future config setting. */
	pulldown = XuMenuFind(menuBar, RingDeltaId);
	for(found = False, n = 0; n < nradar_ring_ranges; n++)
	{
		char buf[10];
		if(radar_ring_spacing == radar_ring_ranges[n]) found = True;
		(void) snprintf(buf, 10, "%d", radar_ring_ranges[n]);
		btn = XmVaCreateManagedToggleButton(pulldown, buf,
			XmNset , (radar_ring_spacing == radar_ring_ranges[n]),
#ifdef INDICATOR_SIZE
			XmNindicatorSize, INDICATOR_SIZE,
#endif
			NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, ring_spacing_cb, INT2PTR(radar_ring_ranges[n]));
	}
	if(!found)
		XuMenuToggleSetState(menuBar, nradar_ring_ranges-1, True);

	/* Time display and image time selection block
	 */
	currentTimeLabel = XmVaCreateManagedLabel(topForm, "ctl",
		XmNalignment, XmALIGNMENT_CENTER,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menuBarForm,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	arrow_left = XmVaCreateManagedArrowButton(topForm, "al",
		XmNarrowDirection, XmARROW_LEFT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, currentTimeLabel,
		XmNtopOffset, 5,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(arrow_left, XmNactivateCallback, step_cb, (XtPointer)1);

	arrow_right = XmVaCreateManagedArrowButton(topForm, "ar",
		XmNarrowDirection, XmARROW_RIGHT,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, currentTimeLabel,
		XmNtopOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(arrow_right, XmNactivateCallback, step_cb, (XtPointer)-1);

	timeScale = XmVaCreateManagedScale( topForm, "rc",
		XmNorientation, XmHORIZONTAL,
		XmNminimum, 0,
		XmNmaximum, 100,
		XmNprocessingDirection, XmMAX_ON_LEFT,
		XmNshowValue, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, currentTimeLabel,
		XmNtopOffset, 5,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, arrow_right,
		XmNrightOffset, 3,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, arrow_left,
		XmNleftOffset, 3,
		NULL);
	XtAddCallback(timeScale, XmNvalueChangedCallback, select_images_cb, (XtPointer)0);
	XtAddCallback(timeScale, XmNdragCallback,         select_images_cb, (XtPointer)0);

	startTimeLabel = XmVaCreateManagedLabel(topForm, "stl",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, timeScale,
		XmNtopOffset, 5,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 50,
		NULL);

	endTimeLabel = XmVaCreateManagedLabel(topForm, "etl",
		XmNalignment, XmALIGNMENT_END,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, timeScale,
		XmNtopOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 50,
		NULL);


	/* Create the tabs for the various functions
	 */
	tabStack = XmVaCreateTabStack(topForm, "tabs",
		XmNtopAttachment,     XmATTACH_WIDGET,
		XmNtopWidget,         startTimeLabel,
		XmNleftAttachment,    XmATTACH_FORM,
		XmNrightAttachment,   XmATTACH_FORM,
		XmNbottomAttachment,  XmATTACH_FORM,
		NULL);
	XtAddCallback(tabStack, XmNtabSelectedCallback, tabs_cb, NULL);

	selectTab = XmVaCreateForm(tabStack, "selectTab",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	animateTab = XmVaCreateForm(tabStack, "animateTab",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	sampleTab = XmVaCreateForm(tabStack, "sampleTab",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	optionsTab = XmVaCreateForm(tabStack, "optionsTab",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);


	/*================= Image Selection Sub-panel =======================*/

	/* The widget for that part of the panel which holds all of the
	 * selection widgets. This is managed and unmanaged by the
	 * buttons in the edit functions block.
	 */

	/* This section of code finds the maximum number of columns needed to
	 * hold any of the labels in the selection widget set and then uses
	 * this to set the combo box column setting. The same setting is used
	 * for all of the selections as it looks better.
	 */
	for(ncols = 0, i = 0; i < NIMAGES; i++)
	{
		IMDAT *im = images+i;

		if(!im->exists) continue;

		for(n = 0; n < im->nprod; n++)
		{
			String *labels;
			int j, nlut, w = safe_strlen(im->prod[n].label);
			if(w > ncols) ncols = w;
			nlut = glImageInfoGetLuts(im->prod[n].tag, NULL, &labels, NULL);
			for(j = 0; j < nlut; j++)
			{
				w = safe_strlen(labels[j]);
				if(w > ncols) ncols = w;
			}
		}
		for(n = 0; n < im->nsite; n++)
		{
			int w = safe_strlen(im->site[n].label);
			if(w > ncols) ncols = w;
		}
	}
	/* 
	 * The comboBox assumes a fixed font and a porportional font is always
	 * narrower on average so an adjustment is made to the column count.
	 */
	ncols -= ncols/8;
	if(ncols < 12) ncols = 12;

	/* Images selection blocks
	 */
	first = True;
	for( i = 0; i < NIMAGES; i++ )
	{
		IMDAT *imd = images + image_display[IMAGE_ORDER].order[i];

		if (!imd->exists) continue;

		frame = XmVaCreateManagedFrame(selectTab, "frame",
			XmNshadowType, XmSHADOW_ETCHED_IN,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftAttachment, (first)? XmATTACH_FORM:XmATTACH_WIDGET,
			XmNleftWidget, frame,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		first = False;

		(void)XmVaCreateManagedLabel(frame, imd->name,
			XmNchildType, XmFRAME_TITLE_CHILD,
			NULL);

		form = XmVaCreateForm(frame, "form",
			XmNhorizontalSpacing, 5,
			XmNverticalSpacing, 5,
			NULL);

		/* Display type selection
		*/
		imd->prodSelect = XmVaCreateManagedComboBox(form, "type",
			XmNcolumns, ncols,
			XmNvisibleItemCount, MIN(imd->nprod,15),
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
		XtAddCallback(imd->prodSelect, XmNselectionCallback, prod_cb, (XtPointer)imd);

		if(	imd->nprod > 0 )
		{
			for( n = 0; n < imd->nprod; n++ )
				XuComboBoxAddItem(imd->prodSelect, imd->prod[n].label, 0);
			XuComboBoxSetString(imd->prodSelect, imd->prod[imd->prod_select].label);
		}

		/* LUT selection
		 */
		imd->lutW = XmVaCreateManagedComboBox(form, "enhanceList",
			XmNcolumns, ncols,
			XmNvisibleItemCount, 1,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);	
		XtAddCallback(imd->lutW, XmNselectionCallback, lut_cb, (XtPointer)imd);

		imd->lutW_label = XmVaCreateManagedLabel(form, "enhancement",
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, imd->lutW,
			XmNbottomOffset, 0,
			NULL);

		/* Select All and Deselect All buttons.
		 */
		btn = XmVaCreateManagedPushButton(form, "selectAll",
			XmNmarginWidth, 7,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, imd->lutW_label,
			NULL);
		XtAddCallback(btn, XmNactivateCallback, select_all_of_list_cb, (XtPointer) imd);

		btn = XmVaCreateManagedPushButton(form, "deselectAll",
			XmNmarginWidth, 7,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, imd->lutW_label,
			NULL);
		XtAddCallback(btn, XmNactivateCallback, select_all_of_list_cb, (XtPointer) imd);

		/* Site display and location selection
		*/
		imd->selectList = XmVaCreateManagedScrolledList(form, "selectSW",
			XmNselectionPolicy, XmEXTENDED_SELECT,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, imd->prodSelect,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, btn,
			NULL);	
		XtAddCallback(imd->selectList, XmNextendedSelectionCallback, site_cb, (XtPointer)imd);

		list = NewXmStringArray(imd->nsite);
		for( nlist = 0, n = 0; n < imd->nsite; n++ )
		{
			imd->site[n].tag = glImageInfoGetTag(imd->prod[imd->prod_select].tag, imd->site[n].label);
			if(blank(imd->site[n].tag)) continue;
			item = XmStringCreateLocalized(imd->site[n].label);
			XmListAddItem(imd->selectList, item, 0);
			if(imd->site[n].selected)
				list[nlist++] = item;
			else
				XmStringFree(item);
		}
		XtVaSetValues(imd->selectList, XmNselectedItems, list, XmNselectedItemCount, nlist, NULL);
		XmStringArrayFree(list, nlist);

		XtManageChild(form);
	}
	

	/*================= Options Selection Sub-panel =======================*/


	/* The widget for that part of the panel which holds all of the
	 * option setting widgets. This is managed and unmanaged by the
	 * buttons in the edit functions block.
	 */

	/*==== Brightness controls ====
	 */
	brightFrame = XmVaCreateManagedFrame(optionsTab, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginHeight, 5,
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(brightFrame, "imageBrightness",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(brightFrame, "form",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		NULL);

	width = 0;
	first = True;
	for(i = 0; i < NIMAGES; i++)
	{
		n = image_display[BRIGHT_ORDER].order[i];

		if (!images[n].exists) continue;

		form2 = XmVaCreateManagedForm(form, "form2",
			XmNtopAttachment, (first)? XmATTACH_FORM:XmATTACH_WIDGET,
			XmNtopWidget, form2,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		first = False;

		settings[n].w = XmVaCreateManagedLabel(form2, settings[n].name,
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		slider = XmVaCreateManagedScale(form2, "satB",
			XmNorientation, XmHORIZONTAL,
			XmNprocessingDirection, XmMAX_ON_RIGHT,
			XmNvalue, (int)(images[n].brightness * 100. + .5),
			XmNmaximum, settings[n].bright_max,
			XmNminimum, settings[n].bright_min,
			XmNdecimalPoints, 2,
			XmNshowValue, True,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, settings[n].w,
			XmNleftOffset, 5,
			NULL);

		XtAddCallback(slider, XmNvalueChangedCallback, brightness_cb, INT2PTR(settings[n].type));

		XtVaGetValues(settings[n].w, XmNwidth, &width1, NULL);
		if(width1 > width) width = width1;
	}
	XtVaSetValues(form2, XmNbottomAttachment, XmATTACH_FORM, NULL);

	/* Make the labels the same width so that the sliders are the same length
	 */
	for(n = 0; n < NIMAGES; n++)
	{
		if (images[n].exists) XtVaSetValues(settings[n].w, XmNwidth, width, NULL);
	}

	/*==== Blending Controls ====
	*/
	blendFrame = XmVaCreateManagedFrame(optionsTab, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginHeight, 5,
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, brightFrame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, brightFrame,
		XmNrightOffset, 0,
		NULL);

	(void)XmVaCreateManagedLabel(blendFrame, "blendLabel",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(blendFrame, "form",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		NULL);

	ll = XmVaCreateManagedLabel(form, "satellite",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	lr = XmVaCreateManagedLabel(form, "radar",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	slider = XmVaCreateManagedScale(form, "blendRatio",
		XmNorientation, XmHORIZONTAL,
		XmNprocessingDirection, XmMAX_ON_RIGHT,
		XmNminimum, blend_minimum,
		XmNvalue, blend_amount,
		XmNshowValue, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, ll,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(slider, XmNvalueChangedCallback, blend_amount_cb, (XtPointer)1);
	XtAddCallback(slider, XmNdragCallback, blend_amount_cb, (XtPointer)2);

	images[S].blendAmt = XmVaCreateManagedLabel(form, "satBlendAmt",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, slider,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XuWidgetPrint(images[S].blendAmt, "%d%%", 100 - blend_amount);

	images[R].blendAmt = XmVaCreateManagedLabel(form, "radBlendAmt",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, slider,
		XmNtopOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XuWidgetPrint(images[R].blendAmt, "%d%%", blend_amount);

	/*
	 * ==== Hold over times ====
	 */
	holdFrame = XmVaCreateManagedFrame(optionsTab, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginHeight, 5,
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, brightFrame,
		NULL);

	(void)XmVaCreateManagedLabel(holdFrame, "holdTime",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(holdFrame, "form",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		NULL);

	first = True;
	for(i = 0; i < NIMAGES; i++)
	{
		n = image_display[HOLD_ORDER].order[i];

		if (!images[n].exists) continue;

		form1 = XmVaCreateManagedForm(form, "form",
			XmNtopAttachment, (first)? XmATTACH_FORM:XmATTACH_WIDGET,
			XmNtopWidget, form1,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		first = False;

		spin1 = XmpVaCreateManagedSpinBox(form1, "spina",
			XmNspinBoxType, XmSPINBOX_SIGNED_NUMBER,
			XmNuserData, (XtPointer) &images[n],
			XmNvalue,   images[n].post_hold_time,
			XmNcolumns, 5,
			XmNminimum, 0,
			XmNmaximum, settings[n].max,
			XmNincrement, settings[n].interval,
			XmNspinBoxUseClosestValue, True,
			XmNeditable, False,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);
		XtAddCallback(spin1, XmNvalueChangedCallback, hold_time_cb, INT2PTR(1));

		spin2 = XmpVaCreateManagedSpinBox(form1, "spinb",
			XmNspinBoxType, XmSPINBOX_SIGNED_NUMBER,
			XmNuserData, (XtPointer) &images[n],
			XmNvalue, images[n].pre_hold_time,
			XmNcolumns, 5,
			XmNminimum, -settings[n].max,
			XmNmaximum, 0,
			XmNincrement, -settings[n].interval,
			XmNspinBoxUseClosestValue, True,
			XmNeditable, False,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_WIDGET,
			XmNrightWidget, spin1,
			XmNrightOffset, 5,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
		XtAddCallback(spin2, XmNvalueChangedCallback, hold_time_cb, INT2PTR(0));

		(void) XmVaCreateManagedLabel(form1, settings[n].name,
			XmNalignment, XmALIGNMENT_END,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_WIDGET,
			XmNrightWidget, spin2,
			XmNrightOffset, 5,
			NULL);
	}
	XtVaSetValues(form1, XmNbottomAttachment, XmATTACH_FORM, NULL);


	/*================= Animation Control Sub-panel =======================*/


	/* The widget for that part of the panel which holds all of the
	 * animation widgets. This is managed and unmanaged by the
	 * buttons in the edit functions block.
	 */

	animationTimeWindow = CreateTimeWindowSelector(animateTab,
		XmMAX_ON_LEFT, set_animation_time_window,
		XmNwidth, 500,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 50,
		NULL);

	/* Set animation speed
	*/
	frame = XmVaCreateManagedFrame(animateTab, "frame",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, animationTimeWindow,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, animationTimeWindow,
		XmNrightOffset, 0,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "animateSpeed",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(frame, "form",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 5,
		NULL);

	slider = XmVaCreateManagedScale(form, "speed",
		XmNorientation, XmHORIZONTAL,
		XmNprocessingDirection, XmMAX_ON_LEFT,
		XmNvalue, animate_speed,
		XmNminimum, 1,
		XmNmaximum, GV_animation_max_delay,
		XmNshowValue, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(slider, XmNvalueChangedCallback, speed_cb, NULL);

	(void) XmVaCreateManagedLabel(form, "minLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, slider,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(form, "maxLabel",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, slider,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	animateStartBtn = XmVaCreateManagedPushButton(animateTab, "startBtn",
		XmNshadowThickness, 2,
		XmNindicatorOn, False,
		XmNmarginWidth, 6,
		XmNmarginHeight, 6,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 30,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, animationTimeWindow,
		XmNrightOffset, 30,
		NULL);
	XtAddCallback(animateStartBtn, XmNactivateCallback, animate_cb, (XtPointer)True);

	animateStopBtn = XmVaCreatePushButton(animateTab, "stopBtn",
		XmNshadowThickness, 2,
		XmNmarginWidth, 6,
		XmNmarginHeight, 6,
		XmNbackground, XuLoadColorResource(animateTab, RNselectBgColor, "ForestGreen"),
		XmNforeground, XuLoadColorResource(animateTab, RNselectFgColor, "black"),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, animateStartBtn,
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget, animateStartBtn,
		XmNrightOffset, 0,
		NULL);
	XtAddCallback(animateStopBtn, XmNactivateCallback, animate_cb, (XtPointer)False);


	/*================= Sample Selection Sub-panel =======================*/

	/* The widget for that part of the panel which holds all of the
	 * sample widgets. This is managed and unmanaged by the
	 * buttons in the edit functions block.
	 */

	sampleGrid = CreateGridSelector(sampleTab, SELECT_IMAGE_SAMPLE,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	predefPoints = CreatePredefinedPointsSelector( sampleTab, SELECT_IMAGE_SAMPLE,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, sampleGrid,
		NULL);

	/* The controls for turning the sampling on and off from within
	 * the sample tab itself.
	 */
	onOffGroup = XmVaCreateManagedFrame(sampleTab, "onOffGroup",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, predefPoints,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, predefPoints,
		NULL);
	
	(void)XmVaCreateManagedLabel(onOffGroup, "label",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateForm(onOffGroup, "onOffForm",
		XmNhorizontalSpacing, 5,
		XmNverticalSpacing, 0,
		NULL);

	rc = XmVaCreateManagedRowColumn(form, "frame",
		XmNorientation, XmVERTICAL,
		XmNradioBehavior, True,
		XmNspacing, 0,
		XmNmarginWidth, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	onBtn = XmVaCreateManagedToggleButton(rc, "onBtn", NULL);
	XtAddCallback(onBtn, XmNvalueChangedCallback, on_off_cb, NULL);

	offBtn = XmVaCreateManagedToggleButton(rc, "offBtn", NULL);

	clearBtn = XmVaCreateManagedPushButton(form, "clearBtn",
		XmNmarginWidth, 15,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 15,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, rc,
		XmNleftOffset, 9,
		NULL);
	XtAddCallback(clearBtn, XmNactivateCallback, clear_btn_cb, NULL);

	XtManageChild(form);

	fontSetManager = CreateFontSelector(sampleTab, set_font_attributes,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, sampleGrid,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, onOffGroup,
		XmNleftOffset, 0,
		NULL);

	for(n = 0; n < NIMAGES; n++)
	{
		images[n].sample_display.font_type = GetFontSelectorValue(fontSetManager, SELECT_FONT_TYPE);
		images[n].sample_display.font_size = GetFontSelectorValue(fontSetManager, SELECT_FONT_SIZE);
		images[n].sample_display.colour    = GetFontSelectorValue(fontSetManager, SELECT_COLOUR);
	}

	label = XmVaCreateManagedLabel(sampleTab, "sampleSelect",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	ac = 0;
	XtSetArg(args[ac], XmNlistSizePolicy, XmCONSTANT); ac++;
	XtSetArg(args[ac], XmNselectionPolicy, XmSINGLE_SELECT); ac++;
	XtSetArg(args[ac], XmNwidth, 220); ac++;
	XtSetArg(args[ac], XmNvisibleItemCount, 4); ac++;
	XtSetArg(args[ac], XmNlistMarginHeight, 5); ac++;
	XtSetArg(args[ac], XmNlistMarginWidth, 5); ac++;
	XtSetArg(args[ac], XmNlistSpacing, 5); ac++;
	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(args[ac], XmNtopWidget, label); ac++;
	XtSetArg(args[ac], XmNtopOffset, 0); ac++;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
	XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(args[ac], XmNbottomWidget, predefPoints); ac++;

	sampleList = XmCreateScrolledList(sampleTab, "sampleList", args, ac);
	XtAddCallback(sampleList, XmNsingleSelectionCallback, sample_list_cb, NULL);
	XtManageChild(sampleList);

	label = XmVaCreateManagedLabel(sampleTab, "sampleItemLabel",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, sampleList,
		NULL);

	ac = 0;
	XtSetArg(args[ac], XmNlistSizePolicy, XmCONSTANT); ac++;
	XtSetArg(args[ac], XmNselectionPolicy, XmSINGLE_SELECT); ac++;
	XtSetArg(args[ac], XmNwidth, 220); ac++;
	XtSetArg(args[ac], XmNvisibleItemCount, 3); ac++;
	XtSetArg(args[ac], XmNlistMarginHeight, 5); ac++;
	XtSetArg(args[ac], XmNlistMarginWidth, 5); ac++;
	XtSetArg(args[ac], XmNlistSpacing, 5); ac++;
	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(args[ac], XmNtopWidget, label); ac++;
	XtSetArg(args[ac], XmNtopOffset, 0); ac++;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(args[ac], XmNleftWidget, sampleList); ac++;
	XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(args[ac], XmNbottomWidget, predefPoints); ac++;

	sampleItemList = XmCreateScrolledList(sampleTab, "sampleItemList", args, ac);
	XtAddCallback(sampleItemList, XmNsingleSelectionCallback, sample_item_list_cb, NULL);
	XtManageChild(sampleItemList);

	/*============ End sub-panel definitions - begin initialization  ================*/


	set_time_label(currentTimeLabel, NONE);
	set_time_label(startTimeLabel, NONE);
	set_time_label(endTimeLabel, NONE);

	set_sample_tab_sensitivity();

	generate_timelist();
	update_time_labels();

	/* Set the LUT list by setting the product for the first time
	 * in or just filling in the list if a later creation.
	 */
	for( i = 0; i < NIMAGES; i++ )
	{
		if(first_creation)
		{
			int    ndx = 1;
			String buf;

			if(!images[i].prodSelect) continue;
			if(XuStateDataGet(IMKEY, MNKEY, images[i].name, &buf))
			{
				for( n = 0; n < images[i].nprod; n++ )
				{
					if(same(buf, images[i].prod[n].tag))
					{
						ndx = n+1;
						break;
					}
				}
				FreeItem(buf);
			}
			XuComboBoxSelectPos(images[i].prodSelect, ndx, True);
			images[i].prodSelect = NullWidget;
		}
		else
		{
			fill_in_lut_selections(images+i, False);
		}
	}
	first_creation = False;

	XtManageChild(selectTab);
	XtManageChild(optionsTab);
	XtManageChild(animateTab);
	XtManageChild(sampleTab);
	XtManageChild(tabStack);
	XtManageChild(topForm);
	XuShowDialog(dialog);
}



/*=========================================================================*/
/*
 * SetImageryDisplayState() - Turn all satellite or radar images on or off.
 */
/*=========================================================================*/
void SetImageryDisplayState( String type, Boolean state )
{
	int n;
	for(n = 0; n < NIMAGES; n++)
	{
		if(blank(type) || same(images[n].name, type))
			images[n].display = state;
	}
	show_selections();
}



/*=========================================================================*/
/*
 * ImageryExists() - Does the given image type exist?
 */
/*=========================================================================*/
Boolean ImageryExists( String type )
{
	int n;
	for(n = 0; n < NIMAGES; n++)
	{
		if(same(images[n].name, type))
			return images[n].exists;
	}
	return True;
}


/*=========================================================================*/
/*
 *  Send the command to sample imagery.
 */
/*=========================================================================*/
/*ARGSUSED*/
void SendImageSampleCommand( String cmd )
{
	/* Sampling may not be active if we are in sampling lockout mode */
	if(!sampling_active || !sample_type) return;

	(void) IngredVaCommand(GE_IMAGERY, "SAMPLE %s Default \"%s\" %s %s%% %s",
				(blank(cmd))? "NORMAL":cmd,
				sample_type->sample_display.item,
				sample_type->sample_display.font_type,
				sample_type->sample_display.font_size,
				sample_type->sample_display.colour    );
}



/*======================= LOCAL FUNCTIONS ========================*/


/*ARGSUSED*/
static void exit_cb( Widget w, XtPointer cd, XtPointer xd )
{
	if(!dialog) return;
	end_sampling_mode();
	if (animation_running)
		animate_cb(NullWidget, NULL, NULL);
	XuFreePixmap(dialog, syncPixmap[0]);
	XuFreePixmap(dialog, syncPixmap[1]);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}


/* Process calls from Ingred that give the time of the of the depiction
 * currently visible in the animation.
 */
static void animation_status_observer( CAL cal, String *parms, int nparms )
{
	if(!dialog) return;
	if(!same_ic(parms[0], "ANIMATION")) return;

	if(same(parms[1],"SHOWING"))
	{
		sync_selections(parms[2]);
		show_selections();
	}
	else
	{
		sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
		show_selections();
		reset_sampling_list();
	}
}


/* This function will be called when another dialog goes into sampling
 * mode. This dialog must then lock itself out so that it cannot enter
 * into sampling mode. Note that OB_KEY_ON or OB_KEY_OFF is the state of the
 * dialog that sent the message.
 */
static void sampling_lockout_observer( String *parms, int nparms )
{
	if( !dialog ) return;						/* The dialog must exist */
	if( nparms < 2 ) return;					/* Illegal input */
	if( same(parms[0],OB_KEY_IMAGERY) ) return;	/* Ignore input from ourself */

	if(same_ic(parms[1],OB_KEY_ON))
	{
		sampling_lockout = True;
		if(sampling_active)
			XuToggleButtonSet(offBtn, True, True);
		set_sample_tab_sensitivity();
	}
	else
	{
		sampling_lockout = False;
		set_sample_tab_sensitivity();
	}
}


/* These are kept together so that any changes to the state store
 * retrevial process are kept in sync. A version number is the first
 * item stored so that problems will not happen if the following
 * functions are modified. Note that the boolean items are stored
 * as int and then converted into boolean after read. This turned out
 * to be more robust.
 */
#define CURRENT_STATE_VERSION	"v2"
#define STATE_VERSION_1			"v1"	/* 17 variables */
#define STATE_VERSION_2			"v2"	/* 22 variables */

static void save_state(void)
{
	int i;
	char buffer[1000];

	(void) snprintf(buffer, sizeof(buffer), "%s %d %d %d %d %d %d %d %d %d %d",
		CURRENT_STATE_VERSION,
		(int) sync_with_depictions,
		(int) use_time_window,
		(int) show_legend,
		(int) XuMenuToggleGetState(menuBar,BlendToggleId),
		blend_amount,
		animate_speed,
		(int) show_radar_range_rings,
		(int) show_limit_ring_only,
		radar_ring_spacing,
		(int) XuMenuToggleGetState(menuBar,BrightnessToggleId));

	for(i = 0; i < NIMAGES; i++)
	{
		size_t len = safe_strlen(buffer);
		(void) snprintf(buffer+len, sizeof(buffer)-len, " %d %d %f",
			images[i].pre_hold_time,
			images[i].post_hold_time,
			images[i].brightness);
	}

	XuStateDataSave(IMKEY, MNKEY, OPTKEY, buffer);
}


static void read_state_file(void)
{
	int     i, val;
	float   fval;
	String  buf, version;
	Boolean ok;

	if(!XuStateDataGet(IMKEY, MNKEY, OPTKEY, &buf)) return;

	version = string_arg(buf);

	if(same(version,STATE_VERSION_2))
	{
		val = int_arg(buf, &ok);    if (ok) sync_with_depictions = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) use_time_window = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) show_legend = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) blend_on = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) blend_amount = val;
		val = int_arg(buf, &ok);    if (ok) animate_speed = val;
		val = int_arg(buf, &ok);    if (ok) show_radar_range_rings = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) show_limit_ring_only = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) radar_ring_spacing = val;
		val = int_arg(buf, &ok);    if (ok) brightness_on = (Boolean) val;
		for(i = 0; i < NIMAGES; i++)
		{
			val = int_arg(buf, &ok);    if (ok) images[i].pre_hold_time = val;
			val = int_arg(buf, &ok);    if (ok) images[i].post_hold_time = val;
			fval = float_arg(buf, &ok); if (ok) images[i].brightness = fval;
		}
	}
	else if(same(version,STATE_VERSION_1))
	{
		val = int_arg(buf, &ok);    if (ok) sync_with_depictions = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) use_time_window = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) show_legend = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) blend_on = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) blend_amount = val;
		val = int_arg(buf, &ok);    if (ok) animate_speed = val;
		val = int_arg(buf, &ok);    if (ok) images[S].pre_hold_time = val;
		val = int_arg(buf, &ok);    if (ok) images[S].post_hold_time = val;
		val = int_arg(buf, &ok);    if (ok) images[R].pre_hold_time = val;
		val = int_arg(buf, &ok);    if (ok) images[R].post_hold_time = val;
		val = int_arg(buf, &ok);    if (ok) show_radar_range_rings = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) show_limit_ring_only = (Boolean) val;
		val = int_arg(buf, &ok);    if (ok) radar_ring_spacing = val;
		val = int_arg(buf, &ok);    if (ok) brightness_on = (Boolean) val;
		fval = float_arg(buf, &ok); if (ok) images[R].brightness = fval;
		fval = float_arg(buf, &ok); if (ok) images[S].brightness = fval;
	}
	FreeItem(buf);

	/* Validity checks
	 */
	if (blend_amount < 0 || blend_amount > 100) blend_amount = 100;
	if (animate_speed < 0 || animate_speed > GV_animation_max_delay) animate_speed = 0;

	for(i = 0; i < NIMAGES; i++)
	{
		if(images[i].brightness < 0 || images[i].brightness > 1.0)
			images[i].brightness = 1.0;

		if(images[i].pre_hold_time > 0 || images[i].pre_hold_time < -settings[i].max)
			images[i].pre_hold_time = -settings[i].hold_default;

		if(images[i].post_hold_time < 0 || images[i].post_hold_time > settings[i].max)
			images[i].post_hold_time = settings[i].hold_default;
	}
}


/* If any new imagery has arrived this function will be called by the
 * source checking proecdure and we must then update our lists.
 */
/*ARGSUSED*/
static Boolean new_imagery_observer(Boolean unused)
{
	int        n;
	SourceList slist;

	if(!dialog) return True;
	SourceListByType(SRC_IMAGERY, FpaC_TIMEDEP_ANY, &slist, &n);
	if(n > 0 && slist[0]->modified)
	{
		generate_timelist();
		update_time_labels();
	}
	return True;
}



/*  If the imagery mode is set to follow the depictions, this function
 *  turns off the existing imagery display. This allows the imagery to be
 *  hidden before the active field is changed so that the depiction and
 *  imagery appear to change together and we avoid display flashing.
 */
/*ARGSUSED*/
static void ready_imagery_observer(String *unused, int n)
{
    if(sync_with_depictions && active_time_ndx != NONE)
		(void) IngredCommand(GE_IMAGERY, "HIDE");
}


/*  Respond to a change in the active depiction.
*/
/* ARGSUSED */
static void imagery_update_observer( String *unused, int n )
{
	if(!dialog) return;
	sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
	show_selections();
	reset_sampling_list();
}


/*ARGSUSED*/
static void set_font_attributes(FontSelectorStruct *font)
{
	if (!sample_type) return;
	sample_type->sample_display.font_type = font->type;
	sample_type->sample_display.font_size = font->size;
	sample_type->sample_display.colour    = font->colour;

	if(font->reason != SELECT_NONE)
		SendImageSampleCommand(NULL);
}


/*ARGSUSED*/
static void tabs_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTabStackCallbackStruct *rtn = (XmTabStackCallbackStruct *)call_data;

	if(rtn->selected_child == sampleTab)
		start_sampling_mode();
	else
		end_sampling_mode();

	if (animation_running)
		animate_cb(NullWidget, NULL, NULL);
}


/* Callback function for the selectAll and deselectAll buttons associated with the
 * site list. The stupid list function does not have any way to select and deselect
 * multiple items and activate a callback, especially with the extended select mode.
 * This then is why the list is just selected and cleared in a visual sense and
 * the site_wp function is used directly,
 */
/* ARGSUSED */
static void select_all_of_list_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	int   n;
	IMDAT *im = (IMDAT *) client_data;
	/* 
	 * The selection array in the IMDAT structure is used to hold information on
	 * what sites are changing state. The array is freed in the site_wp function.
	 */
	im->nsel = 0;
	im->sel  = NewMem(int, im->nsite);
	/*
	 * It is easier to deselect the entire list first and then select all
	 * it that is our action.
	 */
	XmListDeselectAllItems(im->selectList);
	/* 
	 * I didn't want two functions for these activities so the switch is keyed
	 * to the button name.
	 */
	if(same(XtName(w),"selectAll"))
	{
		int len;
		/* 
		 * Switching to multiple select and back again is the only way to do this.
		 */
		XtVaSetValues(im->selectList, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
		XtVaGetValues(im->selectList, XmNitemCount, &len, NULL);
		for(n = 1; n <= len; n++)
			XmListSelectPos(im->selectList, n, False);
		XtVaSetValues(im->selectList, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
		im->selected = len;
		for(n = 0; n < im->nsite; n++)
		{
			if(blank(im->site[n].tag)) continue;
			if(im->site[n].selected) continue;
			im->sel[im->nsel++] = n;
			im->site[n].selected = True;
		}
	}
	else
	{
		im->selected = 0;
		for(n = 0; n < im->nsite; n++)
		{
			if(blank(im->site[n].tag)) continue;
			if(!im->site[n].selected) continue;
			im->sel[im->nsel++] = n;
			im->site[n].selected = False;
		}
	}
	(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc)site_wp, client_data);
}



/* On slow systems this can take a while so we do it as a work procedure to
*  provide a snappier feel to the selection.
*/
static Boolean time_select_wp(XtPointer indata)
{
	active_time_ndx = PTR2INT(indata);
	if(active_time_ndx >= 0)
	{
		int i, k;
		for( i = 0; i < NIMAGES; i++)
		{
			if(!images[i].selected) continue;
			for( k = 0; k < images[i].nsite; k++ )
			{
				if(!images[i].site[k].selected) continue;
				if(InTimeList(all_times[active_time_ndx], images[i].site[k].times, ntimes, NULL))
				{
					strcpy(images[i].selected_time, all_times[active_time_ndx]);
					break;
				}
			}
		}
	}
	show_selections();
	return True;
}


/* Function called from the time scale widget.
*/
/*ARGSUSED*/
static void select_images_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmScaleCallbackStruct *rtn = (XmScaleCallbackStruct *)call_data;

	if(ntimes < 1) return;

	switch(rtn->reason)
	{
		case XmCR_DRAG:
			set_time_label(currentTimeLabel, rtn->value);
			break;

		case XmCR_VALUE_CHANGED:
			(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc)time_select_wp, INT2PTR(rtn->value));
			break;
	}
}


/* Function called when the stepping arrows are selected */
/*ARGSUSED*/
static void step_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int ndx;
	int inc = PTR2INT(client_data);

	if(ntimes < 1) return;

	if(active_time_ndx == NONE)
	{
		/* Find the time closest to the active depiction. Note that the all_times array
		 * is in descending time order thus a positive inc results in an earlier time.
		 * If there is no image time in the requested direction do nothing.
		 */
		String   depict_time = ActiveDepictionTime(FIELD_DEPENDENT);
		MAP_PROJ *map_proj   = get_target_map();
		float    lref        = (map_proj)? map_proj->definition.lref : 0.0;

		ndx = closest_tstamp(ntimes, all_times, depict_time, lref, NULL, NULL, NULL, NULL);
		if(ndx < 0) return;
		/*
		 * The closest time may be on the "wrong" side of the depiction depending on
		 * which direction the requested image was in. This corrects the condition.
		 */
		if(inc > 0)
		{
			while(compare_tstamps(depict_time, all_times[ndx], lref) < 0)
				if(++ndx >= ntimes) return;
		}
		else
		{
			while(compare_tstamps(depict_time, all_times[ndx], lref) > 0)
				if(--ndx < 0) return;
		}
	}
	else
	{
		ndx = active_time_ndx + inc;
	}
	if(ndx < 0      ) ndx = 0;
	if(ndx >= ntimes) ndx = ntimes - 1;
	(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc)time_select_wp, INT2PTR(ndx));
}


static void fill_in_lut_selections(IMDAT *im, Boolean select)
{
	int i, nlut, defndx = -1, ndx = 1;
	String *labels;
	ImageLUT *luts, default_lut;

	if(!im->lutW) return;

	XuComboBoxDeleteAllItems(im->lutW);

	if(im->nprod < 1) return;	/* Nothing at all to process */

	if (select) im->luts_last_item = 0;

	nlut = glImageInfoGetLuts(im->prod[im->prod_select].tag, &luts, &labels, &default_lut);

	XtSetSensitive(im->lutW, nlut > 0);
	XtSetSensitive(im->lutW_label, nlut > 0);

	if(nlut < 1)
	{
		im->prod[im->prod_select].lutndx = NONE;
		(void) IngredVaCommand(GE_IMAGERY, "SETLUT %s DEFAULT", im->prod[im->prod_select].tag);
	}
	else
	{
		for(i = 0; i < nlut; i++)
		{
			XuComboBoxAddItem(im->lutW, labels[i], 0);
			if(default_lut == luts[i]) defndx = i;
		}

		ndx = 0;
		if(im->prod[im->prod_select].lutndx != NONE)
			ndx = im->prod[im->prod_select].lutndx;
		else if(defndx >= 0)
			ndx = defndx;

		XuComboBoxSetString(im->lutW, labels[ndx]);
		XuComboBoxSelectPos(im->lutW, ndx+1, select);
		XtVaSetValues(im->lutW, XmNvisibleItemCount, MIN(10,nlut), NULL);
	}
}


/* Workproc for the image product type selection. A workproc is used as on
 * slower systems the interface can seem to "stick" at this point.
 */
static Boolean prod_wp(XtPointer indata)
{
	int i;
	IMDAT *im = (IMDAT *)indata;
	/*
	 * The only way to select multiple items on the list is to set it to
	 * multiple select, deselect all of the items and then select as we
	 * add to the list then set it back to extended select.
	 */
	XuSetBusyCursor(ON);
	XtVaSetValues(im->selectList, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
	XmListDeselectAllItems(im->selectList);
	XuListEmpty(im->selectList);
	im->selected = 0;
	for( i = 0; i < im->nsite; i++)
	{
		if( im->site[i].selected && NotNull(im->site[i].tag) )
		{
			(void) IngredVaCommand(GE_IMAGERY, "REMOVE %s", im->site[i].tag);
		}

		im->site[i].tag = glImageInfoGetTag(im->prod[im->prod_select].tag, im->site[i].label);

		if(NotNull(im->site[i].tag))
		{
			XuListAddItem(im->selectList, im->site[i].label);
			if (im->site[i].selected)
			{
				XmListSelectPos(im->selectList, 0, False);
				im->selected++;
			}
		}
	}
	XtVaSetValues(im->selectList, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);

	generate_timelist();
	update_time_labels();
	fill_in_lut_selections(im, True);
	if(active_time_ndx == NONE) sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
	if(show_legend) ShowImageLegend(im, True);
	show_selections();
	XuSetBusyCursor(OFF);
	return True;
}


/*ARGSUSED*/
static void prod_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	IMDAT *im = (IMDAT *)client_data;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if( rtn->item_position < 1 || rtn->item_position == im->prod_last_item)  return;
	im->prod_last_item = rtn->item_position;
	im->prod_select = rtn->item_position - 1;
	(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc)prod_wp, client_data);
	XuStateDataSave(IMKEY, MNKEY, im->name, im->prod[im->prod_select].tag);
}


static Boolean site_wp(XtPointer indata)
{
	int n;
	String notify[3];
	IMDAT *im = (IMDAT *)indata;

	/* Override any display setting for this type of image and notify any
	 * interested observers of the change. We assume here that if the display
	 * state of the images if off, then it makes sense to turn them back on
	 * if a site selection is made.
	 */
	XuSetBusyCursor(ON);
	im->display = True;
	generate_timelist();
	update_time_labels();

	for( n = 0; n < im->nsel; n++ )
	{
		if(!im->site[im->sel[n]].selected)
			(void) IngredVaCommand(GE_IMAGERY, "REMOVE %s", im->site[im->sel[n]].tag);
	}

	if(active_time_ndx == NONE) sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
	if(show_legend) ShowImageLegend(im, False);

	notify[0] = im->name;
	notify[1] = (String) ((long) im->display);
	NotifyObservers(OB_IMAGE_SELECTED, notify, 2);

	show_selections();
	set_sample_tab_sensitivity();
	XuSetBusyCursor(OFF);
	FreeItem(im->sel);
	im->nsel = 0;
	return True;
}


/*ARGSUSED*/
static void site_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, n, k;
	XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;
	IMDAT *im = (IMDAT *)client_data;

	im->nsel = 0;
	im->sel  = NewMem(int, im->nsite);

	/* This convoluted selection procedure is needed as some of the sites
	 * in the list may not exist. Note the list has a one origin.
	 */
	for(k = 1, n = 0; n < im->nsite; n++)
	{
		if(blank(im->site[n].tag)) continue;
		/*
		 * If an item in the list is selected and the corresponding image
		 * is flagged as already selected and there is only one selection
		 * in the list, this is taken as a deselection action. We need to
		 * do it ourselves as the multiple selection list does not do this.
		 */
		for(i = 0; i < cbs->selected_item_count; i++)
		{
			if(k != cbs->selected_item_positions[i]) continue;
			if(im->site[n].selected)
			{
				if(cbs->selected_item_count == 1 && im->selected == 1)
				{
					im->sel[im->nsel++] = n;
					im->site[n].selected = False;
				}
			}
			else
			{
				im->sel[im->nsel++] = n;
				im->site[n].selected = True;
			}
			break;
		}
		/* If the site was not in the list and is selected then we must
		 * flag it for removal.
		 */
		if(i >= cbs->selected_item_count)
		{
			if(im->site[n].selected)
			{
				im->sel[im->nsel++] = n;
				im->site[n].selected = False;
			}
		}
		k++;
	}
	/*
	 * Get the total number selected and clear the list when the number
	 * drops to 0 as the multiple select list does not handle this itself.
	 */
	im->selected = 0;
	for(n = 0; n < im->nsite; n++)
	{
		if(im->site[n].selected) im->selected++;
	}
	if(im->selected ==  0)
		XmListDeselectAllItems(im->selectList);

	(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc)site_wp, client_data);
}


/*ARGSUSED*/
static void lut_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	int nlut;
	ImageLUT *luts;
	IMDAT *im = (IMDAT *)client_data;
	XmComboBoxCallbackStruct *rtn = (XmComboBoxCallbackStruct *) call_data;

	if( rtn->item_position < 1 ) return;
	if( rtn->item_position == im->luts_last_item ) return;

	im->luts_last_item = rtn->item_position;

	im->prod[im->prod_select].lutndx = rtn->item_position - 1;
	nlut = glImageInfoGetLuts(im->prod[im->prod_select].tag, &luts, NULL, NULL);
	if(im->prod[im->prod_select].lutndx < nlut)
	{
		(void) IngredVaCommand(GE_IMAGERY, "SETLUT %s %d",
					im->prod[im->prod_select].tag, luts[im->prod[im->prod_select].lutndx]);
		(void) IngredCommand(GE_IMAGERY, "SHOW");
	}
	if(show_legend) ShowImageLegend(im, True);
}


/* Check the length of the times arrays and extend if necessary.
 */
static void check_times_array_size(Boolean initialize)
{
	int i, j, n, start;

	if (initialize)
	{
		ntimes = 0;
		(void) memset((void *)all_times, 0, (size_t)max_times*sizeof(String*));
		for( i = 0; i < NIMAGES; i++ )
		{
			images[i].site_end = images[i].nsite;
			for( n = 0; n < images[i].nsite; n++ )
			{
				for( j = 0; j < max_times; j++ )
					images[i].site[n].times[j] = NULL;
			}
		}
	}
	if(ntimes >= max_times)
	{
		start = max_times;
		max_times = ntimes + 50;
		all_times = MoreStringArray(all_times, max_times);
		for( j = 0; j < NIMAGES; j++ )
		{
			for( n = 0; n < images[j].nsite; n++ )
			{
				images[j].site[n].times = MoreStringArray(images[j].site[n].times, max_times);
				for( i = start; i < max_times; i++ )
					images[j].site[n].times[i] = NULL;
			}
		}
	}
}


/* Generating the time list is somewhat messy. We want any given image to display with another
 * image if it is within a certain time of the base image valid time. Thus we first need to
 * create the base time list and then sync the other images with it.
 */
static void generate_timelist(void)
{
	int      i, j, k, m, n, pos;
	LOGICAL  found;
	TSTAMP   saved_time;

	/* Allocating and initializing memory right away for the times arrays
	 * is necessary to avoid problems in the code in other places.
	 */
	check_times_array_size(True);

	/* Save the previous active time for our fallback time reset */
	strcpy(saved_time, "");
	if(active_time_ndx >= 0 && max_times > 0)
		(void) safe_strcpy(saved_time, all_times[active_time_ndx]);
	active_time_ndx = NONE;

	/* Generate the time base. If selected, radar is used before satellite as it normally
	 * occurs at least every 10 minutes.
	 */
	for(i = 0; i < NIMAGES; i++ )
	{
		IMDAT *id = images+i;

		/* If none of these image types continue on to the next */
		if(!id->selected) continue;

		/* Second check to make sure that there really is an active image in the active group */
		for( n = 0; n < id->nsite; n++)
			if(id->site[n].selected && !blank(id->site[n].tag)) break;
		if(n >= id->nsite) continue;

		/* Start from the end of the list. For radar this makes little difference but the
		 * satellite at the end of the list will most probably be the one that underlies
		 * all of the others.
		 */
		for( n = id->nsite-1; n >= 0; n-- )
		{
			if(!id->site[n].selected) continue;
			id->site[n].nvtime = glImageInfoFindValidTimes(id->site[n].tag, glSORT_DESCENDING,
															&id->site[n].vtime, &id->site[n].atime);
			if(id->site[n].nvtime < 1) continue;
			ntimes = id->site[n].nvtime;
			check_times_array_size(False);
			for( j = 0; j < ntimes; j++ )
			{
				all_times[j]         = id->site[n].vtime[j];
				id->site[n].times[j] = id->site[n].atime[j];
			}
			glFree(id->site[n].vtime);
			glFree(id->site[n].atime);
			id->site_end = n;
			break;
		}
		break;
	}

	/* At this point it is possible that there are no selected times */
	if(ntimes < 1) return;

	/* Add in the rest of the images.
	 */
	for(i = 0; i < NIMAGES; i++ )
	{
		IMDAT *id = images+i;
		if(!id->selected) continue;

		for( k = 0; k < id->site_end; k++ )
		{
			if(!id->site[k].selected) continue;

			id->site[k].nvtime = glImageInfoFindValidTimes(id->site[k].tag, glSORT_DESCENDING,
															&id->site[k].vtime, &id->site[k].atime);
			if(id->site[k].nvtime < 1) continue;

			id->site[k].vused = NewBooleanArray(id->site[k].nvtime);

			/* For satellite, overlay and underlay add in if an image is within the time window limits.
			 * Radar is always added in with no window.
			 */
			if(use_time_window && id != &images[R])
			{
				char pre[16], post[16];
				/*
				 * The closest_tstamp function requires the before and after time delta to be
				 * in the format h:m. The ":" is required for proper intrepretation, otherwise
				 * the delta will be taken as hours.
				 */
				(void) snprintf(pre,  16, ":%d", abs(id->pre_hold_time));
				(void) snprintf(post, 16, ":%d", abs(id->post_hold_time));

				for(n = 0; n < ntimes; n++)
				{
					id->site[k].times[n]  = NULL;
					pos = closest_tstamp(id->site[k].nvtime, id->site[k].vtime, all_times[n], 0, pre, post, NULL, NULL);
					if(pos >= 0)
					{
						id->site[k].times[n]   = id->site[k].atime[pos];
						id->site[k].vused[pos] = True;
					}
				}
			}
			else
			{
				for(n = 0; n < ntimes; n++)
				{
					id->site[k].times[n] = NULL;
					if(InTimeList(all_times[n], id->site[k].vtime, id->site[k].nvtime, &pos))
					{
						id->site[k].times[n]   = id->site[k].atime[pos];
						id->site[k].vused[pos] = True;
					}
				}
			}
		}

		/* If we have any unused times we must add them to the list.
		*/
		for( k = 0; k < id->site_end; k++ )
		{
			if(!id->site[k].selected) continue;

			for(m = 0; m < id->site[k].nvtime; m++)
			{
				if(id->site[k].vused[m]) continue;

				/* The valid time might already be in the all_times array. */
				if(InTimeList(id->site[k].vtime[m], all_times, ntimes, &pos))
				{
					id->site[k].times[pos] = id->site[k].atime[m];
				}
				else
				{
					check_times_array_size(False);

					for(pos = 0; pos < ntimes; pos++)
					{
						if(MinuteDif(all_times[pos],id->site[k].vtime[m]) < 0) continue;

						/* Open up a hole in the main time array */
						for(j = ntimes; j > pos; j--)
						{
							all_times[j] = all_times[j-1];
						}

						/* Open up a hole in the times arrays of all sites */
						for( n = 0; n < NIMAGES; n++ )
						{
							for(j = 0; j < images[n].nsite; j++)
							{
								int mm;
								for(mm = ntimes; mm > pos; mm--)
								{
									images[n].site[j].times[mm] = images[n].site[j].times[mm-1];
								}
							}
						}
						break;
					}

					/* Null the resulting holes */
					for( n = 0; n < NIMAGES; n++ )
					{
						int mm;
						for(mm = 0; mm < images[n].nsite; mm++)
							images[n].site[mm].times[pos] = NULL;
					}

					/* Set the time entry to the global time array and to the site array */
					all_times[pos]         = id->site[k].vtime[m];
					id->site[k].times[pos] = id->site[k].atime[m];
					ntimes++;
				}
			}
			FreeItem(id->site[k].vtime);
			FreeItem(id->site[k].atime);
			FreeItem(id->site[k].vused);
		}
	}

	/* Reset the active time pointer. */
	active_time_ndx = 0;
	for(found = FALSE, k = 0; k < NIMAGES; k++)
	{
		if(images[k].selected && InTimeList(images[k].selected_time, all_times, ntimes, &n))
		{
			found = TRUE;
			active_time_ndx = n;
			break;
		}
	}
	if(!found && InTimeList(saved_time, all_times, ntimes, &n))
		active_time_ndx = n;

	sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
}


static void update_time_labels(void)
{
	int max;

	if (!dialog) return;

	if(ntimes > 0)
	{
		/* Careful with the begin and end times. array[0] element is the most recent
		 * and ntimes-1 the oldest, so the begin and end times need to reflect this
		 * reversal of index values.
		 */
		if (animation_running)
		{
			if(animate_begin_ndx >= ntimes) animate_begin_ndx = ntimes - 1;
		}
		else
		{
			/* We try and set the animation limits according to the actual times
			 * last recorded when we did animation. On failure we default to ends.
			 */
			int pos;
			animate_begin_ndx = ntimes - 1;
			animate_end_ndx   = 0;
			if(InTimeList(animate_begin_time, all_times, ntimes, &pos)) animate_begin_ndx = pos;
			if(InTimeList(animate_end_time,   all_times, ntimes, &pos)) animate_end_ndx   = pos;
		}

		/* Use a minumum of one here. If ntimes is 1 then we get a lot
		 * of complaints from the widget.
		 */
		max = MAX(ntimes-1,1);

		XtVaSetValues(timeScale,
			XmNvalue,   0,
			XmNminimum, 0,
			XmNmaximum, max,
			NULL);

		XtSetSensitive(animationTimeWindow, True);
		SetTimeWindowLimits(animationTimeWindow, all_times, ntimes, animate_begin_ndx, animate_end_ndx);

		set_time_label(currentTimeLabel, active_time_ndx);
		set_time_label(startTimeLabel, ntimes-1);
		set_time_label(endTimeLabel, 0);
	}
	else
	{
		set_time_label(currentTimeLabel, NONE);
		set_time_label(startTimeLabel, NONE);
		set_time_label(endTimeLabel, NONE);
		SetTimeWindowLimits(animationTimeWindow, NULL, 0, 0, 0);
		XtSetSensitive(animationTimeWindow, False);
	}
}


static void sync_selections(String active_depict)
{
	int    i, k, n, *xref, nxref;
	String *tl;

	if (!sync_with_depictions) return;

	if (use_time_window)
	{
		tl   = NewStringArray(ntimes);
		xref = NewIntArray(ntimes);
	}

	/* If we are to synchronize with the depiction sequence we need to find
	 * the image that is closest to the depiction time within the boundaries
	 * of the hold times.
	 */
	active_time_ndx = NONE;

	for( i = 0; i < NIMAGES; i++)
	{
		char pre[16], post[16];

		if(!images[i].selected) continue;

		/* The closest_tstamp function requires the before and after time delta to be
		 * in the format h:m. The ":" is required for proper intrepretation, otherwise
		 * the delta will be taken as hours.
		 */
		(void) snprintf(pre,  16, ":%d", abs(images[i].pre_hold_time));
		(void) snprintf(post, 16, ":%d", abs(images[i].post_hold_time));

		for( k = 0; k < images[i].nsite; k++ )
		{
			if(!images[i].site[k].selected) continue;

			if(use_time_window)
			{
				int    pos;
				/*
				 * The following procedure is required because closest_tstamp requires that
				 * there be no NULLS in the time array.
				 */
				for( nxref = 0, n = 0; n < ntimes; n++ )
				{
					if(blank(images[i].site[k].times[n])) continue;
					tl[nxref]   = images[i].site[k].times[n];
					xref[nxref] = n;
					nxref++;
				}
				pos = closest_tstamp(nxref, tl, active_depict, 0, pre, post, NULL, NULL);
				if(pos >= 0)
				{
					active_time_ndx = xref[pos];
					FreeItem(tl);
					FreeItem(xref);
					return;
				}
			}
			else
			{
				if(InTimeList(active_depict, images[i].site[k].times, ntimes, &n))
				{
					active_time_ndx = n;
					return;
				}
			}
		}
	}

	if (use_time_window)
	{
		FreeItem(tl);
		FreeItem(xref);
	}
}


static void show_selections(void)
{
	int i, n;

	set_time_label(currentTimeLabel, active_time_ndx);

	if(active_time_ndx == NONE)
	{
		activate_sample_cursor(False);
		(void) IngredCommand(GE_IMAGERY, "HIDE");
	}
	else
	{
		Boolean do_sample = False;
		if(ntimes > 0 && active_time_ndx >= 0 && active_time_ndx < ntimes)
		{
			for( i = 0; i < NIMAGES; i++ )
			{
				Boolean have_data = False;
				IMDAT  *imd       = images+i;

				/* The sites are done in reverse order so that the one at the top of the
				 * list appears on top of the displayed images.
				 */
				for( n = imd->nsite-1; n >= 0; n-- )
				{
					if(imd->site[n].selected && NotNull(imd->site[n].tag))
					{
						if(imd->display)
						{
							/* BUG 20080909: If there is not a valid time for an image give a time 
							 * at which will not be any images and ingred will not display anything.
							 */
							if(imd->site[n].times[active_time_ndx])
							{
								have_data = True;
								do_sample = True;
								(void) IngredVaCommand(GE_IMAGERY, "DISPLAY %s %s %s",
										IMAGERY_DISPLAY_PLANE, imd->site[n].tag, imd->site[n].times[active_time_ndx]);
							}
							else
							{
								(void) IngredVaCommand(GE_IMAGERY, "DISPLAY %s %s %s",
										IMAGERY_DISPLAY_PLANE, imd->site[n].tag, "1900:001:00:00");
							}
						}
						else
						{
							(void) IngredVaCommand(GE_IMAGERY, "REMOVE %s", imd->site[n].tag);
						}
					}
				}
				SetImageLegendTime(imd, all_times[active_time_ndx], have_data); 
			}
			/* If sampling we need to call the enter function as the list of items
			 * available for sampling may change with time.
			 */
			reset_sampling_list();
		}
		activate_sample_cursor(do_sample);
		(void) IngredCommand(GE_IMAGERY, "SHOW");
	}
}


static void set_time_label(Widget w, int ndx)
{
	int  yr, jd, hr, min, mn, dy, nt, max;
	char sn;

	if (!dialog) return;

	if(ndx >= 0 && ndx < ntimes && parse_tstamp(all_times[ndx],&yr,&jd,&hr,&min,NULL,NULL))
	{
		mdate(&yr, &jd, &mn, &dy);
		if(w == currentTimeLabel)
		{
			nt = MinuteDif(GV_T0_depict, all_times[ndx]);
			sn = (nt < 0)? '-':'+';
			nt = abs(nt);
			XuWidgetPrint(currentTimeLabel, "%.2d %.2d:%.2d (T%c%d:%.2d)", dy, hr, min, sn, nt/60, nt%60);

			/* The following is to avoid possible ndx out of range complaints */
			XtVaGetValues(timeScale, XmNminimum, &min, XmNmaximum, &max, NULL);
			if(ndx > max) ndx = max;
			if(ndx < min) ndx = min;
			XmScaleSetValue(timeScale, ndx);
			XtVaSetValues(timeScale, XmNeditable, True, NULL);
		}
		else
		{
			XuWidgetPrint(w, "%.2d %.2d:%.2d", dy, hr, min);
		}
	}
	else
	{
		XuWidgetLabel(w, "----");
		if(w == currentTimeLabel)
		{
			/* If no time center the scale */
			XtVaGetValues(timeScale, XmNminimum, &min, XmNmaximum, &max, NULL);
			XmScaleSetValue(timeScale, (max - min)/2);
			XtVaSetValues(timeScale, XmNeditable, False, NULL);
		}
	}
}


/*ARGSUSED*/
static void show_legend_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	if((show_legend = XmToggleButtonGetState(w)))
	{
		int i;
		for( i = 0; i < NIMAGES; i++ )
		{
			if(images[i].exists)
				ShowImageLegend(images+i, False);
		}
	}
	else
	{
		ShowImageLegend(NULL, False);
	}
	save_state();
}


static void sync_pushButton_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	sync_with_depictions = !sync_with_depictions;
	sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
	show_selections();
	save_state();
	XuMenuToggleSetState(menuBar, SyncToggleId, sync_with_depictions);
	XtVaSetValues(syncPushButton, XmNlabelPixmap, syncPixmap[sync_with_depictions?1:0], NULL);
}


/*ARGSUSED*/
static void stat_table_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	ACTIVATE_radarStatDialog(w);
}


/*ARGSUSED*/
static void sync_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	sync_with_depictions = XmToggleButtonGetState(w);
	sync_selections(ActiveDepictionTime(FIELD_INDEPENDENT));
	show_selections();
	save_state();
	XtVaSetValues(syncPushButton, XmNlabelPixmap, syncPixmap[sync_with_depictions?1:0], NULL);
}


/*ARGSUSED*/
static void time_window_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	XuSetBusyCursor(ON);
	use_time_window = XmToggleButtonGetState(w);
	generate_timelist();
	update_time_labels();
	show_selections();
	XuSetBusyCursor(OFF);
	save_state();
}


/*ARGSUSED*/
static void do_blending_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	if(XmToggleButtonGetState(w))
		(void) IngredVaCommand(GE_IMAGERY, "BLEND ON %d", blend_amount);
	else
		(void) IngredCommand(GE_IMAGERY, "BLEND OFF");
	(void) IngredCommand(GE_IMAGERY, "SHOW");
	save_state();
}


/*ARGSUSED*/
static void do_brightness_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	int n;
	for(n = 0; n < NIMAGES; n++)
	{
		if(!images[n].exists) continue;
		if(XmToggleButtonGetState(w))
			glImageTypeSetBrightness(settings[n].type, images[n].brightness);
		else
			glImageTypeSetBrightness(settings[n].type, glNORMAL_BRIGHTNESS);
	}
	(void) IngredCommand(GE_IMAGERY, "SHOW");
	save_state();
}



/*================ OPTIONS CALLBACKS ====================*/



/*ARGSUSED*/
static void brightness_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	int n;
	float scale;
	int scale_value = ((XmScaleCallbackStruct *) call_data)->value;
	int type = PTR2INT(client_data);


	if(scale_value == 100)
	{
		XuMenuToggleSetState(menuBar, BrightnessToggleId, False);
		for(n = 0; n < NIMAGES; n++)
		{
			if(type == settings[n].type)
			{
				images[n].brightness = glNORMAL_BRIGHTNESS;
				break;
			}
		}
		glImageTypeSetBrightness(type, glNORMAL_BRIGHTNESS);
	}
	else
	{
		XuMenuToggleSetState(menuBar, BrightnessToggleId, True);
		scale = (float)scale_value/100.0;
		for(n = 0; n < NIMAGES; n++)
		{
			if(type == settings[n].type)
			{
				images[n].brightness = scale;
				break;
			}
		}
		glImageTypeSetBrightness(type, scale);
	}
	(void) IngredCommand(GE_IMAGERY, "SHOW");
	save_state();
}


/*ARGSUSED*/
static void blend_amount_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	Boolean final = (PTR2INT(client_data) == 1);
	blend_amount = ((XmScaleCallbackStruct *) call_data)->value;

	/* Round to nearest 5%
	*/
	blend_amount = (blend_amount/5 + ((blend_amount%5)*2)/5) * 5;

	XuWidgetPrint(images[S].blendAmt, "%d%%", 100 - blend_amount);
	XuWidgetPrint(images[R].blendAmt, "%d%%", blend_amount);

	/* Send to Ingred only if toggle is on.
	*/
	if(final)
	{
		XuMenuToggleSetState(menuBar, BlendToggleId, True);
		DeactivateMenu();
		(void) IngredVaCommand(GE_IMAGERY, "BLEND ON %d", blend_amount);
		(void) IngredCommand(GE_IMAGERY, "SHOW");
		ActivateMenu();
	}
	save_state();
}


/*ARGSUSED*/
static void hold_time_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	XtPointer *ptr;
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;

	XtVaGetValues(w, XmNuserData, &ptr, NULL);

	if(PTR2INT(client_data) == 0)
		((IMDAT*) ptr)->pre_hold_time = (int) rtn->value;
	else
		((IMDAT*) ptr)->post_hold_time = (int) rtn->value;

	XuSetBusyCursor(ON);
	generate_timelist();
	update_time_labels();
	show_selections();
	save_state();
	XuSetBusyCursor(OFF);
}


/*ARGSUSED*/
static void show_rings_cb( Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2INT(client_data))
	{
		case 0:
			show_limit_ring_only   = False;
			show_radar_range_rings = False;
			break;

		case 1:
			show_limit_ring_only   = True;
			show_radar_range_rings = True;
			break;

		case 2:
			show_limit_ring_only   = False;
			show_radar_range_rings = True;
			break;
	}
	glImageShowRadarRangeRings(show_radar_range_rings, show_limit_ring_only? 0:radar_ring_spacing);
	(void) IngredCommand(GE_IMAGERY, "SHOW");
	save_state();
}


/*ARGSUSED*/
static void ring_spacing_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	radar_ring_spacing = PTR2INT(client_data);
	glImageShowRadarRangeRings(show_radar_range_rings, show_limit_ring_only? 0:radar_ring_spacing);
	(void) IngredCommand(GE_IMAGERY, "SHOW");
	save_state();
}


/*================ ANIMATION CALLBACKS ====================*/


static void animation_lockout_observer(String *parms, int nparms)
{
	if(dialog != NULL && nparms == 2 && !same(parms[0],OB_KEY_IMAGERY))
		XtSetSensitive(animateStartBtn, !same(parms[1],OB_KEY_ON));
}


/*ARGSUSED*/
static void animate_loop(XtPointer client_data, XtIntervalId *id)
{
	if(!animation_running || active_time_ndx < 0) return;
	active_time_ndx -= 1;
	if(active_time_ndx < animate_end_ndx) active_time_ndx = animate_begin_ndx;
	XtVaSetValues(timeScale, XmNvalue, active_time_ndx, NULL);
	show_selections();
	if( active_time_ndx == animate_end_ndx )
		(void) XtAppAddTimeOut(GV_app_context, (long) GV_animation_loop_delay, animate_loop, NULL);
	else
		(void) XtAppAddTimeOut(GV_app_context, (long) animate_speed, animate_loop, NULL);
}


/*ARGSUSED*/
static void animate_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	String parms[2];

	static int old_active_time_ndx = 0;

	if((animation_running = PTR2BOOL(client_data)))
	{
		XtManageChild(animateStopBtn);
		XtUnmanageChild(animateStartBtn);
		old_active_time_ndx = active_time_ndx;
		active_time_ndx = animate_begin_ndx;
		if(animate_begin_ndx > animate_end_ndx)
		{
			animate_loop(NULL,NULL);
		}
		else
		{
			active_time_ndx = animate_begin_ndx;
			XtVaSetValues(timeScale, XmNvalue, active_time_ndx, NULL);
			show_selections();
		}
		DeactivatePanels();
	}
	else
	{
		ActivatePanels();
		XtManageChild(animateStartBtn);
		XtUnmanageChild(animateStopBtn);
		active_time_ndx = old_active_time_ndx;
		set_time_label(currentTimeLabel, active_time_ndx);
		show_selections();
	}
	parms[0] = OB_KEY_IMAGERY;
	parms[1] = (animation_running)? OB_KEY_ON:OB_KEY_OFF;
	NotifyObservers(OB_ANIMATION_RUNNING, parms, 2);
}


/*ARGSUSED*/
static void set_animation_time_window( TimeWindowSelectorStruct *tw )
{
	if(tw->start_ndx >= 0 && tw->start_ndx < ntimes)
	{
		animate_begin_ndx = tw->start_ndx;
		strcpy(animate_begin_time, all_times[tw->start_ndx]);
	}
	if(tw->end_ndx >= 0 && tw->end_ndx < ntimes)
	{
		animate_end_ndx = tw->end_ndx;
		strcpy(animate_end_time, all_times[tw->end_ndx]);
	}
}


/*ARGSUSED*/
static void speed_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
	animate_speed = ((XmScaleCallbackStruct *) call_data)->value;
	save_state();
}


/*=============== SAMPLE SUB-PANEL FUNCTIONS ======================*/


 /* This function controls the sample cursor. The sample_cursor_on
  * state variable is used to ensure that we do not get multiple ON
  * or OFF commands to the cursor function which canconfuse it.
  */
static void activate_sample_cursor( Boolean state )
{
	set_sample_tab_sensitivity();
	if (sampling_active)
	{
		if (state)
		{
			if (!sample_cursor_on)
				XuSetCursor(GW_mapWindow, XuSAMPLE_CURSOR, ON);
		}
		else
		{
			if (sample_cursor_on)
				XuSetCursor(GW_mapWindow, XuSAMPLE_CURSOR, OFF);
		}
		sample_cursor_on = state;
	}
}


/* As it says, get a list of those things that can be sampled and put
 * it into the things to sample list in the sample tab.
 */
static void	make_sample_list ( void )
{
	int i, n;

	if (!dialog) return;

	/* Clear our list and set the default visibility of the selection lists/
	 */
	sample_type_list_len = 0;
	XmListDeleteAllItems(sampleList);
	XmListDeleteAllItems(sampleItemList);

	/* Clear the context menu
	 */
	XtUnmanageChildren(contextSiteBtns, ncontext_site_btns);

	if(active_time_ndx == NONE) return;

	if(!sample_type_list)
	{
		int total = 0;
		for(n = 0; n < NIMAGES; n++)
			total += images[n].nsite;

		sample_type_list  = NewMem(IMDAT*, total);
		sample_type_index = NewMem(int, total);
	}
	/* Create the list of images that can be sampled.
	 */
	for(i = 0; i < NIMAGES; i++)
	{
		IMDAT *im = images+i;
		for( n = 0; n < im->nsite; n++ )
		{
			if(im->site[n].selected && NotNull(im->site[n].tag) && NotNull(im->site[n].times[active_time_ndx]))
			{
				if(glImageIsDataImage(glImageFetch(im->site[n].tag, im->site[n].times[active_time_ndx], NULL)))
				{
					XmString xmlabel = XmStringCreateSimple(im->site[n].label);
					XmListAddItem(sampleList, xmlabel, 0);
					add_site_to_context_menu(xmlabel, sample_type_list_len);
					XmStringFree(xmlabel);
					sample_type_list[sample_type_list_len] = im;
					sample_type_index[sample_type_list_len] = n;
					sample_type_list_len++;
				}
			}
		}
	}
}


/* Put Ingred into image sampling mode on the image to be sampled.
 */
static void activate_sample_item (void)
{
	int      pos = 1;
	XmString xmlabel;

	if (!dialog) return;

	if(sample_type_list_len > 0)
	{
		activate_sample_cursor(True);
		if(last_sample_select)
		{
			pos = XmListItemPos(sampleList, last_sample_select);
			if (!pos) pos = 1;
		}
		XmListSelectPos(sampleList, pos, True);
		XtManageChildren(contextSiteBtns, sample_type_list_len);
		SetActiveContextMenu(panelContextMenu);
	}
	else
	{
		activate_sample_cursor(False);
		xmlabel = XmStringCreateSimple( "No data images");
		XmListAddItem(sampleList, xmlabel, 0);
		XmStringFree(xmlabel);
		xmlabel = XmStringCreateSimple( "selected");
		XmListAddItem(sampleList, xmlabel, 0);
		XmStringFree(xmlabel);
	}
}



/* A wapper for the SendImageSampleCommand that has no arguments so
 * that can be used with the NotifyObservers call below. This allows
 * the ActivateMenu function to send the SAMPLE command to ingred when
 * required.
 */
static void send_sampling_command(void)
{
	SendImageSampleCommand(NULL);
}


/* The check for sampling_active is required as setting the onBtn
 * results in a recursive call to this function.
 */
static void start_sampling_mode ( void )
{
	String on_parms[] = {OB_KEY_IMAGERY, OB_KEY_ON, (String) send_sampling_command};

	if (!dialog) return;
	if (sampling_active) return;

	sampling_active = !sampling_lockout;
	XtVaSetValues(onBtn,  XmNset, sampling_active, NULL);
	XtVaSetValues(offBtn, XmNset, !sampling_active, NULL);

	make_sample_list();
	if(sample_type_list_len > 0)
	{
		if(sampling_active)
		{
			NotifyObservers(OB_DIALOG_SAMPLING, on_parms, 3);
			XuToggleButtonSet(onBtn, True, True);
		}
	}
	else
	{
		XuToggleButtonSet(onBtn, False, True);
		sampling_active = False;
	}
	activate_sample_item();
	set_sample_tab_sensitivity();
}


static void	reset_sampling_list ( void )
{
	if (!dialog) return;
	if (!sampling_active) return;

	make_sample_list();
	activate_sample_item();
}


/* If no_sampling is true, then sampling_active will be set to
 * false. This is done only if the tabs in the sample panel change
 * and not if we exit the image panel through the panel tabs. In
 * this case if we re-enter the panel we want the sampling to resume.
 */
static void end_sampling_mode()
{
	String parms[] = {OB_KEY_IMAGERY,"off"};

	if(!sampling_active) return;

	SetActiveContextMenu(None);
	activate_sample_cursor(False);
	sampling_active = False;
	set_sample_tab_sensitivity();
	sample_type_list_len = 0;
	FreeItem(sample_type_list);
	FreeItem(sample_type_index);
	(void) IngredCommand(GE_IMAGERY, "SAMPLE OFF");
	/*
	 * Only send the off notification if we are not locked out as
	 * if we are another dialog will be sending notifications.
	 */
	if(!sampling_lockout)
		NotifyObservers(OB_DIALOG_SAMPLING, parms, 2);
}


/*ARGSUSED*/
static void sample_list_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int  i, nitems, ndx, pos = 1;
	String *items, vtime;
	XmString xms;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	if(sample_type_list_len < 1 || rtn->item_position > sample_type_list_len) return;

	/* This is a single selection widget and one item should always appear selected.
	 * If the selection count is 0 then reselect the item.
	 */
	if(rtn->selected_item_count < 1 && rtn->item_position >= 1)
	{
		XmListSelectPos(w, rtn->item_position, False);
		return;
	}

	if (last_sample_select) XmStringFree(last_sample_select);
	last_sample_select = XmStringCopy(rtn->item);

	sample_type = sample_type_list[rtn->item_position - 1];
	ndx         = sample_type_index[rtn->item_position - 1];
	vtime       = sample_type->site[ndx].times[active_time_ndx];

	SetFontSelector(fontSetManager, sample_type->sample_display.font_type,
			sample_type->sample_display.font_size, sample_type->sample_display.colour);

	/* Set the active image to the one just selected */
	if(sampling_active)
		(void) IngredVaCommand(GE_IMAGERY, "ACTIVE %s %s", sample_type->site[ndx].tag, vtime);
	
	/* Create our item list for this image */
	sample_type->sample_image = glImageFetch(sample_type->site[ndx].tag, vtime, NULL);
	nitems = glImageGetDataItems(sample_type->sample_image, &items);
	XmListDeleteAllItems(sampleItemList);
	XtUnmanageChildren(contextValueBtns, ncontext_value_btns);
	for(i=0; i<nitems; i++)
	{
		xms = XmStringCreateSimple(items[i]);
		XmListAddItem(sampleItemList, xms, 0);
		add_value_to_context_menu(xms, i);
		XmStringFree(xms);
	}

	/* Now select our item */
	if(sample_type->last_sample_item)
	{
		pos = XmListItemPos(sampleItemList, sample_type->last_sample_item);
		if (!pos) pos = 1;
	}
	XmListSelectPos(sampleItemList, pos, True);
	if (sampling_active)
		XtManageChildren(contextValueBtns, nitems);
}


/*ARGSUSED*/
static void sample_item_list_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	int nitems, pos;
	String *items;
	XmListCallbackStruct *rtn = (XmListCallbackStruct *)call_data;

	/* This is a single selection widget and one item should always appear selected.
	 * If the selection count is 0 then reselect the item.
	 */
	if(rtn->selected_item_count < 1 && rtn->item_position >= 1)
	{
		XmListSelectPos(w, rtn->item_position, False);
		return;
	}

	if (sample_type->last_sample_item) XmStringFree(sample_type->last_sample_item);
	sample_type->last_sample_item = XmStringCopy(rtn->item);

	pos = rtn->item_position - 1;
	nitems = glImageGetDataItems(sample_type->sample_image, &items);
	if(pos < nitems) sample_type->sample_display.item = items[pos];
	else             sample_type->sample_display.item = (String)0;

	SendImageSampleCommand(NULL);
}

/* Se the sensitivity of the sampling tab widgets. Note that the on/off
 * block always stays sensitive.
 */
static void set_sample_tab_sensitivity(void)
{
	int        n, numKids;
	WidgetList kids;

	if(!dialog || !sampleTab) return;

	XtVaGetValues(sampleTab, XtNnumChildren, &numKids, XtNchildren, &kids, NULL);

	if(have_data_images())
	{
		if(sampling_active)
		{
			for(n = 0; n < numKids; n++)
				XtSetSensitive(kids[n], True);
		}
		else
		{
			for(n = 0; n < numKids; n++)
			{
				if( kids[n] != onOffGroup)
					XtSetSensitive(kids[n], False);
				else
					XtSetSensitive(kids[n], True);
			}
		}
	}
	else
	{
		for(n = 0; n < numKids; n++)
			XtSetSensitive(kids[n], False);
	}
}


/*ARGSUSED*/
static void on_off_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(XmToggleButtonGetState(w))
	{
		sampling_lockout = False;
		start_sampling_mode();
	}
	else
	{
		end_sampling_mode();
	}
}



/*ARGSUSED*/
static void clear_btn_cb (Widget w, XtPointer client_data, XtPointer call_data)
{
	if (sampling_active)
		(void) IngredCommand(GE_EDIT, E_CLEAR);
}


static Boolean have_data_images(void)
{
	int i, n;

	if(active_time_ndx == NONE) return False;

	for(i = 0; i < NIMAGES; i++)
	{
		for( n = 0; n < images[i].nsite; n++ )
		{
			if(!images[i].site[n].selected) continue;
			if(glImageInfoIsDataType(images[i].site[n].tag)) return True;
		}
	}
	return False;
}


/*============= Context Menu ====================*/


/* The fact that the list is one based is used to decide what we are doing here by
 * making the clear zero, the site list positive and the item list index negative
 * so that we get three choices.
 */
static void context_menu_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int ndx = PTR2INT(client_data);

	if(ndx == 0)
	{
		(void) IngredCommand(GE_EDIT, E_CLEAR);
	}
	else if(ndx < 0)
	{
		XmListSelectPos(sampleItemList, abs(ndx), True);
	}
	else
	{
		XmListSelectPos(sampleList, ndx, True);
	}
}


/* The image context popup menu consists of a clear button and two pull-right
 * popup menus that allow the site and the sampling value to be chosen from
 * the context menu.
 */
static void create_context_menu(void)
{
	Widget btn;

	panelContextMenu = CreatePanelContextMenu("imageryContextMenu");

	btn = XmVaCreateManagedPushButton(panelContextMenu, "clearBtn", NULL);
	XtAddCallback(btn, XmNactivateCallback, context_menu_cb, (XtPointer) 0);

	(void) XmVaCreateManagedSeparator(panelContextMenu, "sep", NULL);

	contextSitePulldown = XmCreatePulldownMenu(panelContextMenu, "cm_fieldSelect", NULL, 0);

	btn = XmVaCreateManagedCascadeButton(panelContextMenu, "siteBtn",
		XmNsubMenuId, contextSitePulldown,
		NULL);

	contextValuePulldown = XmCreatePulldownMenu(panelContextMenu, "cm_fieldSelect", NULL, 0);

	btn = XmVaCreateManagedCascadeButton(panelContextMenu, "valueBtn",
		XmNsubMenuId, contextValuePulldown,
		NULL);
}


/* Add the sample list item to our context menu and extend the button widget array
 * if necessary. Note that the list is one origin, thus the client_data is set as
 * n+1 to activate the proper list item. This applies to the value list a well.
 */
static void add_site_to_context_menu(XmString label, int list_len)
{
	if(list_len >= ncontext_site_btns)
	{
		ncontext_site_btns++;
		contextSiteBtns = MoreWidgetArray(contextSiteBtns, ncontext_site_btns);
		contextSiteBtns[list_len] = XmVaCreatePushButton(contextSitePulldown, "btn", NULL);
		XtAddCallback(contextSiteBtns[list_len], XmNactivateCallback, context_menu_cb, INT2PTR(ncontext_site_btns));
	}
	XtVaSetValues(contextSiteBtns[list_len], XmNlabelString, label, NULL);
}


static void add_value_to_context_menu(XmString label, int list_len)
{
	if(list_len >= ncontext_value_btns)
	{
		ncontext_value_btns++;
		contextValueBtns = MoreWidgetArray(contextValueBtns, ncontext_value_btns);
		contextValueBtns[list_len] = XmVaCreatePushButton(contextValuePulldown, "btn", NULL);
		XtAddCallback(contextValueBtns[list_len], XmNactivateCallback, context_menu_cb, INT2PTR(-(ncontext_value_btns)));
	}
	XtVaSetValues(contextValueBtns[list_len], XmNlabelString, label, NULL);
}

