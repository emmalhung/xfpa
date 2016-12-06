/****************************************************************************
*
*  File:     field_stateDialog.c
*
*  Purpose:  Controls the depiction group and field visibilities.
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
*
****************************************************************************/

#include "global.h"
#include <ingred.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include "depiction.h"
#include "guidance.h"
#include "help.h"
#include "imagery.h"
#include "editor.h"
#include "menu.h"
#include "observer.h"
#include "resourceDefines.h"

/* defines
 */
#define ALL_ON		1
#define ALL_OFF		2
#define RESET		3
#define GROUP_KEY	"gds"
#define FIELD_KEY	"fds"
#define CONTROL_KEY	"cds"
#define CONTROL_LST	"grss"	/* The second key in the state store keys */
#define ACTIVE_ONLY	0
#define DEFVAL		0
#define CUSTOM		1


/* Local variables
 */
static Widget       dialog      = NullWidget;
static Widget       scrolledW   = NULL;
static Widget       scrollForm  = NULL;
static Widget       guidBtn     = NULL;
static Widget       radarBtn    = NULL;
static Widget       satBtn      = NULL;
static Widget       overlayBtn  = NULL;
static Widget       underlayBtn = NULL;
static Widget       scratchBtn  = NULL;
static Widget       defaultVis  = NULL;
static WidgetList   grpW        = NULL;
static WidgetList   fldW        = NULL;
static WidgetList   aOnW        = NULL;
static int          group_count = 0;
static int          field_count = 0;
static Boolean      doing_cb    = False;	/* used in case the callback causes update() to be called */
static Boolean      initialized = False;

/* These hold the default state and the custom state for the
 * various sub-system displays.
 */
static Boolean gid_state[2];
static Boolean rad_state[2];
static Boolean sat_state[2];
static Boolean scr_state[2];
static Boolean old_state[2];
static Boolean uld_state[2];


/* Local functions.
*/
static void cmd_button_cb            (Widget, XtPointer, XtPointer);
static void create_field_layout      (void);
static void option_menu_vis_change_cb(Widget, XtPointer, XtPointer);
static void exit_cb                  (Widget, XtPointer, XtPointer);
static void field_toggle_cb          (Widget, XtPointer, XtPointer);
static void field_always_on_cb       (Widget, XtPointer, XtPointer);
static void group_toggle_cb          (Widget, XtPointer, XtPointer);
static void general_cb               (Widget, XtPointer, XtPointer);
static void save_as_custom_cb        (Widget, XtPointer, XtPointer);
static void main_menu_vis_change_cb  (Widget, XtPointer, XtPointer);
static void field_change_observer    (String*, int);
static void field_update_observer    (String*, int);
static void guidance_update_observer (String*, int);
static void image_update_observer    (String*, int);
static void menu_activation_observer (String*, int);

/* Error name
 */
static String module = "VisibilityPreset";


/* Fill in the visibility state for an individual group.
*/
void SetGroupVisibility( GROUP *grp )
{
	int      i, count;
	char     stored;
	String   ptr, predef_list_filename;
	Boolean  use_saved_state, groups_visible;
	PARM     *setup;
	INFOFILE fd;

	/* Get the requested initial field visibility state from the resource file.
	*/
	ptr             = XuGetStringResource(RNsaveFieldVisibilityState, "GROUPS_OFF");
	use_saved_state = same_ic(ptr, "USE_SAVED_STATE");
	groups_visible  = same_ic(ptr, "ALL_ON");

	for( i = 0; i < MAXVIS; i++ ) grp->vis_state[i] = False;

	if (use_saved_state)
	{
		if(XuVaStateDataGet(GROUP_KEY, grp->name, NULL, "%c", &stored))
			grp->vis_state[CUSTOM] = (stored == 'y');
	}
	else
	{
		grp->vis_state[CUSTOM] = groups_visible;
	}

	/* Initialize the memory structure for the preset visibilities.
	*/
	setup = GetSetupKeyParms(PRESET_LISTS, FIELD_VIS_LIST_FILE);
	predef_list_filename = (setup) ? setup->parm[1] : FIELD_VIS_LIST_FILE;
	fd = info_file_open(get_file(PRESET_LISTS, predef_list_filename));
	if (!fd) return;

	count = CUSTOM + 1;
	while(count < MAXVIS && NotNull(info_file_find_next_block(fd)))
	{
		String  line;
		GROUP   *g;

		while(!blank(line = info_file_get_next_line(fd)))
		{
			FpaConfigElementStruct *edef;
			FpaConfigLevelStruct   *ldef;

			ptr = string_arg(line);
			edef = identify_element(ptr);
			if (!edef)
			{
				pr_error(module, "Unrecognized element \"%s\" in preset lists file \"%s\"\n",
							ptr, FIELD_VIS_LIST_FILE);
				continue;
			}

			ptr = string_arg(line);
			ldef = identify_level(ptr);
			if (!ldef)
			{
				pr_error(module, "Unrecognized level \"%s\" in preset lists file \"%s\"\n",
							ptr, FIELD_VIS_LIST_FILE);
				continue;
			}

			if(NotNull(FindField(edef->name, ldef->name))) continue;

			g = FindFieldGroup(edef->name);
			if( g == NULL || g != grp ) continue;
			grp->vis_state[count] = True;
			break;
		}
		count++;
	}
	info_file_close(fd);
}


/* Fill in the visibility states for an individual field.
*/
void SetFieldVisibility( FIELD_INFO *fld )
{
	int      i, count;
	String   ptr, predef_list_filename;
	Boolean  use_saved_state;
	PARM     *setup;
	INFOFILE fd;

	/* Get the requested initial field visibility state from the resource file.
	*/
	ptr = XuGetStringResource(RNsaveFieldVisibilityState, "GROUPS_OFF");
	use_saved_state = same_ic(ptr, "USE_SAVED_STATE");

	for( i = 0; i < MAXVIS; i++ )
	{
		fld->vis_state[i] = VIS_OFF;
	}

	/* Initialize the memory structure for the custom visibilities. If we don't
	*  want the saved states according to the resource file, then initialize
	*  according to resource file requirements.
	*/
	fld->vis_state[CUSTOM] = VIS_ON;
	if (use_saved_state)
		(void) XuVaStateDataGet(FIELD_KEY, fld->info->element->name, fld->info->level->name,
						"%c", &fld->vis_state[CUSTOM]);

	/* Initialize the memory structure for the preset visibilities.
	*/
	setup = GetSetupKeyParms(PRESET_LISTS, FIELD_VIS_LIST_FILE);
	predef_list_filename = (setup) ? setup->parm[1] : FIELD_VIS_LIST_FILE;
	fd = info_file_open(get_file(PRESET_LISTS, predef_list_filename));
	if(!fd) return;

	count = CUSTOM + 1;
	while(count < MAXVIS && NotNull(info_file_find_next_block(fd)))
	{
		String     line;
		FIELD_INFO *f;

		while(!blank(line = info_file_get_next_line(fd)))
		{
			FpaConfigElementStruct *edef;
			FpaConfigLevelStruct   *ldef;

			ptr = string_arg(line);
			edef = identify_element(ptr);
			if (!edef)
			{
				pr_error(module, "Unrecognized element \"%s\" in preset lists file \"%s\"\n",
							ptr, FIELD_VIS_LIST_FILE);
				continue;
			}

			ptr = string_arg(line);
			ldef = identify_level(ptr);
			if (!ldef)
			{
				pr_error(module, "Unrecognized level \"%s\" in preset lists file \"%s\"\n",
							ptr, FIELD_VIS_LIST_FILE);
				continue;
			}

			if(NotNull(f = FindField(edef->name, ldef->name)) && f == fld)
			{
				ptr = string_arg(line);
				if (same_start_ic(ptr,"al"))
					fld->vis_state[count] = VIS_ALWAYS_ON;
				else if (same_start_ic(ptr,"of"))
					fld->vis_state[count] = VIS_OFF;
				else 
					fld->vis_state[count] = VIS_ON;
				break;
			}
		}
		count++;
	}
	info_file_close(fd);
}



/* This function initializes the field visibility states, creates the list of
*  predefined states, creates the toggle buttons in the pulldown menu bar of
*  the main window and informs Ingred of the current choice of state.
*/
void InitFieldDisplayState(String start_list )
{
	int	    count;
	String  p, ptr, data, predef_list_filename = NULL;
	Widget  btn, pulldown, defaultBtn;
	Widget  selBtn = NullWidget;
	PARM    *setup;
	INFOFILE fd;

	pulldown = XuMenuFind(GW_menuBar, MENU_View_presetField);

	/* The first pulldown toggle sets only the active field visible.
	*/
	btn = XmVaCreateManagedToggleButton(pulldown, "activeOnlyBtn",
#ifdef INDICATOR_SIZE
		XmNindicatorSize, INDICATOR_SIZE,
#endif
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, main_menu_vis_change_cb, INT2PTR(ACTIVE_ONLY));

	/* The second selects the custom setup from the dialog below.
	*/
	defaultBtn = XmVaCreateManagedToggleButton(pulldown, "defaultBtn",
#ifdef INDICATOR_SIZE
		XmNindicatorSize, INDICATOR_SIZE,
#endif
		NULL);
	XtAddCallback(defaultBtn, XmNvalueChangedCallback, main_menu_vis_change_cb, INT2PTR(CUSTOM));
	selBtn = defaultBtn;

	/* The next we do is to create the pre-defined field visibliity groups
	*  as found in the field visibility setup file. If the start_list key
	*  is set we initialize the with that list.
	*/
	setup = GetSetupKeyParms(PRESET_LISTS, FIELD_VIS_LIST_FILE);
	predef_list_filename = (setup) ? setup->parm[1] : FIELD_VIS_LIST_FILE;
	fd = info_file_open(get_file(PRESET_LISTS, predef_list_filename));
	count = CUSTOM + 1;
	while(count < MAXVIS && NotNull(ptr = info_file_find_next_block(fd)))
	{
		p = info_file_get_block_label(fd);
		btn = XmVaCreateManagedToggleButton(pulldown, p,
#ifdef INDICATOR_SIZE
			XmNindicatorSize, INDICATOR_SIZE,
#endif
			NULL);
		XtAddCallback(btn, XmNvalueChangedCallback, main_menu_vis_change_cb, INT2PTR(count));
		if(same_ic(ptr, start_list)) selBtn = btn;
		count++;
	}
	info_file_close(fd);

	XuToggleButtonSet(selBtn, True, True);

	/* Set the initial visibility state of the guidance, radar, etc controls.
	 */
	gid_state[CUSTOM] = gid_state[DEFVAL] = GetGuidanceDisplayState();
	rad_state[CUSTOM] = rad_state[DEFVAL] = True;
	sat_state[CUSTOM] = sat_state[DEFVAL] = True;
	old_state[CUSTOM] = old_state[DEFVAL] = True;
	uld_state[CUSTOM] = uld_state[DEFVAL] = True;
	scr_state[CUSTOM] = scr_state[DEFVAL] = GetDisplayState(SCRATCHPAD);

	if(XuStateDataGet(CONTROL_KEY,CONTROL_LST,NULL, &data))
	{
		Boolean ok;
		Boolean val = (Boolean) int_arg(data, &ok);
		if (ok) gid_state[CUSTOM] = gid_state[DEFVAL] = val;
		val = (Boolean) int_arg(data, &ok);
		if (ok) rad_state[CUSTOM] = rad_state[DEFVAL] = val;
		val = (Boolean) int_arg(data, &ok);
		if (ok) sat_state[CUSTOM] = sat_state[DEFVAL] = val;
		val = (Boolean) int_arg(data, &ok);
		if (ok) scr_state[CUSTOM] = scr_state[DEFVAL] = val;
		val = (Boolean) int_arg(data, &ok);
		if (ok) old_state[CUSTOM] = old_state[DEFVAL] = val;
		val = (Boolean) int_arg(data, &ok);
		if (ok) uld_state[CUSTOM] = uld_state[DEFVAL] = val;
		FreeItem(data);
	}

	SetGuidanceDisplayState(gid_state[CUSTOM]);
	SetImageryDisplayState(RADAR_NAME, rad_state[CUSTOM]);
	SetImageryDisplayState(SATELLITE_NAME, sat_state[CUSTOM]);
	SetImageryDisplayState(OVERLAY_NAME, sat_state[CUSTOM]);
	SetImageryDisplayState(UNDERLAY_NAME, sat_state[CUSTOM]);
	selBtn = XtNameToWidget(GW_menuBar, "*scratchpadShow");
	if (selBtn) XmToggleButtonSetState(selBtn, scr_state[CUSTOM], True);

	initialized = True;
}


/*ARGSUSED*/
void ACTIVATE_fieldDisplayStateDialog(Widget w)
{
	int     i, nchild, count;
	String  default_name = NULL;
	Widget  btn, rc, sep, pulldown, *childs;

	static XuDialogActionsStruct action_items_1[] = {
		{ "resetBtn",     cmd_button_cb,     (XtPointer)RESET },
		{ "setAsDefault", save_as_custom_cb, NULL             }
	};
	static XuDialogActionsStruct action_items_2[] = {
		{ "closeBtn", exit_cb, NULL                    },
		{ "helpBtn",  HelpCB,  HELP_FIELD_DISPLAY_STATE}		
	};

	if (dialog)
	{
		XuShowDialog(dialog);
		return;
	}

	dialog = XuCreateToplevelFormDialog(GW_mainWindow, "fieldDisplayState",
		XuNretainGeometry, XuRETAIN_ALL,
		XuNmwmDeleteOverride, exit_cb,
		XuNactionAreaRow1Items, action_items_1,
		XuNnumActionAreaRow1Items, XtNumber(action_items_1),
		XuNactionAreaRow2Items, action_items_2,
		XuNnumActionAreaRow2Items, XtNumber(action_items_2),
		XmNminWidth, 100,
		XmNminHeight, 100,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	rc = XmVaCreateManagedRowColumn(dialog, "rc",
		XmNorientation, XmHORIZONTAL,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, 3,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	guidBtn = XmVaCreateManagedToggleButton(rc, "guidBtn",
		XmNset, gid_state[CUSTOM],
		NULL);
	XtAddCallback(guidBtn, XmNvalueChangedCallback, general_cb, (XtPointer)'g');

	scratchBtn = XmVaCreateManagedToggleButton(rc, "scratchpad",
		XmNset, scr_state[CUSTOM],
		NULL);
	XtAddCallback(scratchBtn, XmNvalueChangedCallback, general_cb, (XtPointer)'p');

	radarBtn = XmVaCreateManagedToggleButton(rc, RADAR_NAME,
		XmNset, rad_state[CUSTOM],
		NULL);
	XtAddCallback(radarBtn, XmNvalueChangedCallback, general_cb, (XtPointer)'r');

	satBtn = XmVaCreateManagedToggleButton(rc, SATELLITE_NAME,
		XmNset, sat_state[CUSTOM],
		NULL);
	XtAddCallback(satBtn, XmNvalueChangedCallback, general_cb, (XtPointer)'s');

	if(ImageryExists(OVERLAY_NAME))
	{
		overlayBtn = XmVaCreateManagedToggleButton(rc, OVERLAY_NAME,
			XmNset, old_state[CUSTOM],
			NULL);
		XtAddCallback(overlayBtn, XmNvalueChangedCallback, general_cb, (XtPointer)'o');
	}

	if(ImageryExists(UNDERLAY_NAME))
	{
		underlayBtn = XmVaCreateManagedToggleButton(rc, UNDERLAY_NAME,
			XmNset, uld_state[CUSTOM],
			NULL);
		XtAddCallback(underlayBtn, XmNvalueChangedCallback, general_cb, (XtPointer)'u');
	}

	sep = XmVaCreateManagedSeparator(dialog, "sep",
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, rc,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);

	btn = XmVaCreateManagedPushButton(dialog, "allOnBtn",
		XmNmarginWidth, 7,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, sep,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 12,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, cmd_button_cb, (XtPointer)ALL_ON);

	btn = XmVaCreateManagedPushButton(dialog, "allOffBtn",
		XmNmarginWidth, 7,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, sep,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, btn,
		NULL);
	XtAddCallback(btn, XmNactivateCallback, cmd_button_cb, (XtPointer)ALL_OFF);

	/* This option menu duplicates the entries in the main window menu bar view
	 * pulldown that gives the preset visibility options. It is easier just to
	 * go through the button list in the pulldown to get the active buttons.
	 */
	defaultVis = XuVaMenuBuildOption(dialog, "presetVis", NULL,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, btn,
		NULL);

	pulldown = XuMenuFind(GW_menuBar, MENU_View_presetField);
	XtVaGetValues(pulldown, XmNnumChildren, &nchild, XmNchildren, &childs, NULL);
	count = ACTIVE_ONLY;
	for( i = 0; i < nchild; i++, count++ )
	{
		if(XmToggleButtonGetState(childs[i])) default_name = XtName(childs[i]);
		(void) XuMenuAddButton(defaultVis, XtName(childs[i]), NULL, count, option_menu_vis_change_cb, INT2PTR(count));
	}
	XuMenuSelectItemByName(defaultVis, default_name);

	scrolledW = XmVaCreateManagedScrolledWindow(dialog, "sw",
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, defaultVis,
		NULL);

	create_field_layout();
	XuShowDialog(dialog);

	AddObserver(OB_FIELD_CHANGE,        field_change_observer);
	AddObserver(OB_FIELD_AVAILABLE,     field_update_observer);
	AddObserver(OB_GUIDANCE_VISIBILITY, guidance_update_observer);
	AddObserver(OB_IMAGE_SELECTED,      image_update_observer);
	AddObserver(OB_MAIN_MENU_ACTIVATION,menu_activation_observer);
}


/*=========================== LOCAL FUNCTIONS =========================*/


/* Just a utility function to set the field visibility selection 
 * buttons so changes can be done in one place.
 */
static String set_field_buttons(int ndx)
{
	if(GV_field[ndx]->visible == VIS_ALWAYS_ON)
	{
		XmToggleButtonSetState(fldW[ndx], True, False);
		XmToggleButtonSetState(aOnW[ndx], True, False);
		return "ON";
	}
	else if(GV_field[ndx]->visible == VIS_ON)
	{
		XmToggleButtonSetState(fldW[ndx], True,  False);
		XmToggleButtonSetState(aOnW[ndx], False, False);
		return "ON_WHEN_GROUP_VISIBLE";
	}
	else
	{
		XmToggleButtonSetState(fldW[ndx], False, False);
		XmToggleButtonSetState(aOnW[ndx], False, False);
		return "OFF";
	}
}


/* Create the group and field button layout in the scrolled window.
 */
static void create_field_layout(void)
{
	int      i, j, nfields, ndx;
	String   *element, *level;
	XmString xmlabel, spaceLabel;
	Widget   aOnManager, manager, label;

	/* Destroy any existing form along with all associated resources. */
	if (scrollForm) XtDestroyWidget(scrollForm);
	FreeItem(grpW);
	FreeItem(fldW);
	FreeItem(aOnW);

	/* Allocate memory for the selection toggle buttons */
	grpW = NewWidgetArray(GV_ngroups);
	fldW = NewWidgetArray(GV_nfield);
	aOnW = NewWidgetArray(GV_nfield);

	/* Save the current counts */
	group_count = GV_ngroups;
	field_count = GV_nfield;

	/* This is the form within the scrolled window */
	scrollForm = XmVaCreateForm(scrolledW, "form",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	label = XmVaCreateManagedLabel(scrollForm, "group",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNmarginBottom, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	label = XmVaCreateManagedLabel(scrollForm, "fieldHeader",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNmarginHeight, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 30,
		NULL);

	(void) XmVaCreateManagedLabel(scrollForm, "alwaysOn",
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);

	manager = XmVaCreateManagedSeparator(scrollForm, "sep",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);

	/* Create the group state selection buttons and the associated field state managers */
	for( i = 0, ndx = 0; i < GV_ngroups; i++ )
	{
		xmlabel = XmStringCreateLocalized(GV_groups[i]->label);
		grpW[i] = XmVaCreateManagedToggleButton(scrollForm, "groupState",
			XmNlabelString, xmlabel,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, manager,
			XmNleftAttachment, XmATTACH_FORM,
			XmNset, GV_groups[i]->visible,
			NULL);
		XtAddCallback(grpW[i], XmNvalueChangedCallback, group_toggle_cb, INT2PTR(i));
		XmStringFree(xmlabel);

		aOnManager = XmVaCreateRowColumn(scrollForm, "faomManager",
			XmNmarginHeight, 0,
			XmNmarginWidth, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 3,
			XmNtopWidget, grpW[i],
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		manager = XmVaCreateRowColumn(scrollForm, "fbmManager",
			XmNmarginHeight, 0,
			XmNmarginWidth, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, grpW[i],
			XmNtopOffset, 3,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 30,
			XmNrightAttachment, XmATTACH_WIDGET,
			XmNrightWidget, aOnManager,
			XmNbottomAttachment, (i == GV_ngroups-1)? XmATTACH_FORM:XmATTACH_NONE,
			NULL);

		/* Create the group-field state buttons. */
		spaceLabel = XmStringCreateLocalized(" ");
		for ( j = 0; j < GV_groups[i]->nfield; j++, ndx++ )
		{
			FIELD_INFO *fld = GV_groups[i]->field[j];

			aOnW[ndx] = XmVaCreateToggleButton(aOnManager, "alwaysOnState",
				XmNlabelString, spaceLabel,
				NULL);
			XtAddCallback(aOnW[ndx], XmNvalueChangedCallback, field_always_on_cb, INT2PTR(ndx));

			xmlabel = XmStringCreateLocalized(fld->info->label);
			fldW[ndx] = XmVaCreateToggleButton(manager, "fieldState",
				XmNlabelString, xmlabel,
				NULL);
			XtAddCallback(fldW[ndx], XmNvalueChangedCallback, field_toggle_cb, INT2PTR(ndx));
			XmStringFree(xmlabel);
		}
		XmStringFree(spaceLabel);
		XtManageChild(aOnManager);
		XtManageChild(manager);
	}

	/* Manage valid buttons */
	if(GEStatus("FIELDS", &nfields, &element, &level, NULL) == GE_VALID)
	{
		for(i = 0; i < GV_nfield; i++)
		{
			if(InFieldList(GV_field[i], nfields, element, level, NULL) )
			{
				XtManageChild(aOnW[i]);
				XtManageChild(fldW[i]);
				(void) set_field_buttons(i);
			}
		}
	}

	XtManageChild(scrollForm);
}


/* The callback function for the visibility option menu.
 */
/*ARGSUSED*/
static void main_menu_vis_change_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int        i, j, ndx = PTR2INT(client_data);
	FIELD_INFO *fld;

	XuSetBusyCursor(ON);
	if (initialized) DeactivateMenu();
	GV_selected_vis_setting = ndx;

	/* 20100709: Special processing for the active field only visible option.
	 * It was easier to do this at this point before the more complex loop.
	 */
	if(GV_active_field)
	{
		for(i = 0; i < GV_nfield; i++)
			GV_field[i]->vis_state[ACTIVE_ONLY] = VIS_OFF;
		GV_active_field->vis_state[ACTIVE_ONLY] = VIS_ALWAYS_ON;
	}

	for( i = 0; i < GV_ngroups; i++ )
	{
		if(!initialized || GV_groups[i]->visible != GV_groups[i]->vis_state[ndx])
		{
			(void) IngredVaCommand(GE_DEPICTION, "GROUP_VISIBILITY %s %s",
				GV_groups[i]->name,
				GV_groups[i]->vis_state[ndx] ? "ON":"OFF");
		}
		GV_groups[i]->visible = GV_groups[i]->vis_state[ndx];

		for ( j = 0; j < GV_groups[i]->nfield; j++)
		{
			fld = GV_groups[i]->field[j];
			if(!initialized || fld->visible != fld->vis_state[ndx])
			{
				String  state;
				switch(fld->vis_state[ndx])
				{
					case VIS_ON: 		state = "ON_WHEN_GROUP_VISIBLE"; break;
					case VIS_ALWAYS_ON: state = "ON";                    break;
					default: 			state = "OFF";                   break;
				}
				(void) IngredVaCommand(GE_DEPICTION, "FIELD_VISIBILITY %s %s %s",
					fld->info->element->name,
					fld->info->level->name,
					state);
			}
			fld->visible = fld->vis_state[ndx];
		}
	}

	(void) IngredCommand(GE_DEPICTION, "SHOW");
	if (initialized) ActivateMenu();

	XuSetBusyCursor(OFF);

	if (dialog)
	{
		if (!doing_cb) XuMenuSelectItem(defaultVis, ndx);
		for( i = 0; i < GV_ngroups; i++ )
			XmToggleButtonSetState(grpW[i], GV_groups[i]->vis_state[GV_selected_vis_setting], False);
		for(i = 0; i < GV_nfield; i++)
			(void) set_field_buttons(i);
	}
}


/* 20100709 - Responds to a change in the active file, and then only in the show
 * active field only visibility mode. This ensures that the proper field state is
 * maintained when the active field is changed via the main editor panel or the
 * mouse context buttons.
 */
/*ARGSUSED*/
static void field_change_observer(String *unused, int n)
{
	/* In coview mode there is a positive feedback loop and I did not think that
	 * the effort in chasing it down was worth it. Thus the fco_active lockout.
	 */
	static Boolean fco_active = False;
	if(fco_active) return;
	fco_active = True;
	if(GV_selected_vis_setting == ACTIVE_ONLY)
		main_menu_vis_change_cb(NULL, INT2PTR(ACTIVE_ONLY), NULL);
	fco_active = False;
}


/* Either the number of groups or fields may have changed or there
 * may have been an instance of a field created
 */
/*ARGSUSED*/
static void field_update_observer(String *unused, int n)
{
	int     i, nfields;
	String *element, *level;

	if (!dialog || doing_cb)  return;

	if (group_count != GV_ngroups || field_count != GV_nfield)
	{
		create_field_layout();

		for(i = 0; i < GV_ngroups; i++)
			XmToggleButtonSetState(grpW[i], GV_groups[i]->visible, False);

		for(i = 0; i < GV_nfield; i++)
			(void) set_field_buttons(i);
	}
	else if(GEStatus("FIELDS", &nfields, &element, &level, NULL) == GE_VALID)
	{
		for(i = 0; i < GV_nfield; i++)
		{
			if( InFieldList(GV_field[i], nfields, element, level, NULL) )
			{
				XtManageChild(fldW[i]);
				XtManageChild(aOnW[i]);
				(void) set_field_buttons(i);
			}
			else
			{
				XtUnmanageChild(fldW[i]);
				XtUnmanageChild(aOnW[i]);
			}
		}
	}
}


/* Upon notification set the guidance button state to that of the
 * current guidance display state.
 */
/*ARGSUSED*/
static void guidance_update_observer(String *parms, int nparms)
{
	if(nparms > 0)
	{
		gid_state[CUSTOM] = PTR2BOOL(parms[0]);
		XmToggleButtonSetState(guidBtn, gid_state[CUSTOM], False);
	}
}


/* Upon notification set the radar and satellite buttons to that of
 * the image display.
 */
/*ARGSUSED*/
static void image_update_observer(String *parms, int nparms)
{
	if( nparms >= 2 )
	{
		String name = parms[0];
		if(same(name,RADAR_NAME))
		{
			rad_state[CUSTOM] = PTR2BOOL(parms[1]);
			XmToggleButtonSetState(radarBtn, rad_state[CUSTOM], False);
		}
		else if(same(name,SATELLITE_NAME))
		{
			sat_state[CUSTOM] = PTR2BOOL(parms[1]);
			XmToggleButtonSetState(satBtn, sat_state[CUSTOM], False);
		}
		else if(same(name,OVERLAY_NAME))
		{
			old_state[CUSTOM] = PTR2BOOL(parms[1]);
			if (overlayBtn) XmToggleButtonSetState(overlayBtn, old_state[CUSTOM], False);
		}
		else if(same(name,UNDERLAY_NAME))
		{
			uld_state[CUSTOM] = PTR2BOOL(parms[1]);
			if (underlayBtn) XmToggleButtonSetState(underlayBtn, uld_state[CUSTOM], False);
		}
	}
	return;
}


/* On notification that the menus have been activated. Check the state
 * of the scratchpad.
 */
/*ARGSUSED*/
static void menu_activation_observer(String *parms, int nparms)
{
	scr_state[CUSTOM] = GetDisplayState(SCRATCHPAD);
	XmToggleButtonSetState(scratchBtn, scr_state[CUSTOM], False);
}


/* Select the button in the main window menu bar in the view preset field display
 * toggle button list to activate.
 */
/*ARGSUSED*/
static void option_menu_vis_change_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int    nchild;
	Widget pulldown, *childs;

	doing_cb = True;
	pulldown = XuMenuFind(GW_menuBar, MENU_View_presetField);
	XtVaGetValues(pulldown, XmNnumChildren, &nchild, XmNchildren, &childs, NULL);
	XmToggleButtonSetState(childs[PTR2INT(client_data)], True, True);
	doing_cb = False;
}


/*ARGSUSED*/
static void general_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	Boolean state = ((XmToggleButtonCallbackStruct *)call_data)->set;

	switch(PTR2CHAR(client_data))
	{
		case 'g':
			gid_state[CUSTOM] = state;
			SetGuidanceDisplayState(state);
			break;

		case 'r':
			rad_state[CUSTOM] = state;
			SetImageryDisplayState(RADAR_NAME, state);
			break;

		case 's':
			sat_state[CUSTOM] = state;
			SetImageryDisplayState(SATELLITE_NAME, state);
			break;

		case 'p':
			scr_state[CUSTOM] = state;
			w = XtNameToWidget(GW_menuBar, "*scratchpadShow");
			if (w) XmToggleButtonSetState(w, state, True);
			break;

		case 'o':
			old_state[CUSTOM] = state;
			SetImageryDisplayState(OVERLAY_NAME, state);
			break;

		case 'u':
			uld_state[CUSTOM] = state;
			SetImageryDisplayState(UNDERLAY_NAME, state);
			break;
	}
}


/*ARGSUSED*/
static void group_toggle_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	GROUP *group = GV_groups[PTR2INT(client_data)];
	group->visible = ((XmToggleButtonCallbackStruct *)call_data)->set;
	(void) IngredVaCommand(GE_DEPICTION, "GROUP_VISIBILITY %s %s",
		group->name,
		group->visible? "ON":"OFF");
}


/* If the field is turned off then the field always on button is turned off as well.
 */
/*ARGSUSED*/
static void field_toggle_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int        ndx  = PTR2INT(client_data);
	Boolean    set  = ((XmToggleButtonCallbackStruct *)call_data)->set;
	FIELD_INFO *fld = GV_field[ndx];

	doing_cb = True;
	if (!set) XmToggleButtonSetState(aOnW[ndx], False, False);
	fld->visible = (set)? VIS_ON:VIS_OFF;
	(void) set_field_buttons(ndx);
	DeactivateMenu();
	(void) IngredVaCommand(GE_DEPICTION, "FIELD_VISIBILITY %s %s %s",
		fld->info->element->name,
		fld->info->level->name,
		(fld->visible == VIS_ON)? "ON_WHEN_GROUP_VISIBLE":"OFF");
	(void) IngredCommand(GE_DEPICTION, "SHOW");
	ActivateMenu();
	doing_cb = False;
}


/* The field button is turned on when the always on button is set and left in the
 * on state when the always on button is turned off.
 */
/*ARGSUSED*/
static void field_always_on_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	String      state;
	int         ndx = PTR2INT(client_data);
	FIELD_INFO *fld = GV_field[ndx];

	if(((XmToggleButtonCallbackStruct *)call_data)->set)
	{
		fld->visible = VIS_ALWAYS_ON;
		state = "ON";
		XmToggleButtonSetState(fldW[ndx], True, False);
	}
	else 
	{
		fld->visible = VIS_ON;
		state = "ON_WHEN_GROUP_VISIBLE";
	}

	doing_cb = True;
	DeactivateMenu();
	(void) IngredVaCommand(GE_DEPICTION, "FIELD_VISIBILITY %s %s %s",
		fld->info->element->name,
		fld->info->level->name,
		state);
	(void) IngredCommand(GE_DEPICTION, "SHOW");
	ActivateMenu();
	doing_cb = False;

}


/*ARGSUSED*/
static void cmd_button_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int  i;

	DeactivateMenu();

	if(PTR2INT(client_data) == RESET)
	{
		XmToggleButtonSetState(guidBtn,    (gid_state[CUSTOM] = gid_state[DEFVAL]), True);
		XmToggleButtonSetState(radarBtn,   (rad_state[CUSTOM] = rad_state[DEFVAL]), True);
		XmToggleButtonSetState(satBtn,     (sat_state[CUSTOM] = sat_state[DEFVAL]), True);
		XmToggleButtonSetState(scratchBtn, (scr_state[CUSTOM] = scr_state[DEFVAL]), True);
		old_state[CUSTOM] = old_state[DEFVAL];
		if (overlayBtn)  XmToggleButtonSetState(overlayBtn, old_state[CUSTOM], True);
		uld_state[CUSTOM] = uld_state[DEFVAL];
		if (underlayBtn) XmToggleButtonSetState(underlayBtn,uld_state[CUSTOM], True);
	}

	for( i = 0; i < GV_ngroups; i++ )
	{
		GROUP *grp = GV_groups[i];

		switch(PTR2INT(client_data))
		{
			case ALL_ON:  grp->visible = True; break;
			case ALL_OFF: grp->visible = False; break;
			case RESET:   grp->visible = grp->vis_state[GV_selected_vis_setting]; break;
		}
		XmToggleButtonSetState(grpW[i], grp->visible, False);
		(void) IngredVaCommand(GE_DEPICTION, "GROUP_VISIBILITY %s %s",
			grp->name,
			grp->visible ? "ON":"OFF");
	}
	for( i = 0; i < GV_nfield; i++ )
	{
		String state;
		FIELD_INFO *fld = GV_field[i];

		switch(PTR2INT(client_data))
		{
			case ALL_ON:  fld->visible = VIS_ON; break;
			case ALL_OFF: fld->visible = VIS_OFF; break;
			case RESET:   fld->visible = fld->vis_state[GV_selected_vis_setting]; break;
		}
		
		state = set_field_buttons(i);
		if(XtIsManaged(fldW[i]))
		{
			(void) IngredVaCommand(GE_DEPICTION, "FIELD_VISIBILITY %s %s %s",
				GV_field[i]->info->element->name,
				GV_field[i]->info->level->name,
				state);
		}
	}
	(void) IngredCommand(GE_DEPICTION, "SHOW");
	ActivateMenu();
}


/*ARGSUSED*/
static void save_as_custom_cb(Widget w, XtPointer unused, XtPointer notused)
{
	int  i, j, ndx;

	gid_state[DEFVAL] = gid_state[CUSTOM];
	rad_state[DEFVAL] = rad_state[CUSTOM];
	sat_state[DEFVAL] = sat_state[CUSTOM];
	old_state[DEFVAL] = old_state[CUSTOM];
	uld_state[DEFVAL] = uld_state[CUSTOM];
	scr_state[DEFVAL] = scr_state[CUSTOM];
	XuVaStateDataSave(CONTROL_KEY,CONTROL_LST,NULL, "%d %d %d %d %d %d",
		gid_state[DEFVAL], rad_state[DEFVAL], sat_state[DEFVAL], scr_state[DEFVAL], old_state[DEFVAL], uld_state[DEFVAL]);

	for(i = 0, ndx = 0; i < GV_ngroups; i++)
	{
		GV_groups[i]->vis_state[CUSTOM] = GV_groups[i]->visible;
		XuStateDataSave(GROUP_KEY, GV_groups[i]->name, NULL, GV_groups[i]->vis_state[CUSTOM] ? "y":"n");
		for ( j = 0; j < GV_groups[i]->nfield; j++, ndx++ )
		{
			FIELD_INFO *fld = GV_groups[i]->field[j];
			fld->vis_state[CUSTOM] = fld->visible;
			XuVaStateDataSave(FIELD_KEY, fld->info->element->name, fld->info->level->name, "%c", fld->visible);
		}
	}
}


/*ARGSUSED*/
static void exit_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (!dialog) return;
	DeleteObserver(OB_FIELD_CHANGE, field_change_observer);
	DeleteObserver(OB_FIELD_AVAILABLE, field_update_observer);
	DeleteObserver(OB_GUIDANCE_VISIBILITY, guidance_update_observer);
	DeleteObserver(OB_IMAGE_SELECTED, image_update_observer);
	DeleteObserver(OB_MAIN_MENU_ACTIVATION, menu_activation_observer);
	FreeItem(grpW);
	FreeItem(fldW);
	FreeItem(aOnW);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
	scrollForm = NullWidget;
}
