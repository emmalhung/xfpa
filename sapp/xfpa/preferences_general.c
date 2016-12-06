/****************************************************************************
*
*  File:     preferences_general.c
*
*  Purpose:  Sets miscellaneous options.
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
#include <Xm/ComboBox.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <graphics.h>
#include "preferences.h"

#define KEY	    "prefgen"
#define MAP		"map"
#define LINK    "link"
#define PALETTE	"palette"
/*
 * Latitude-longitude display style.
 */
#define LL_DECIMAL	1
#define LL_DEG_MIN	2
#define MAP_COORD	3


/* Local static variables */
static Widget  w_dts, w_early_late, w_time, w_control, w_speed, w_restore, w_exit, w_delete_all;
static Widget  w_map_color, w_link_color, w_ll_display;
static int early_late_state     = True;
static int time_state           = True;
static int control_state        = True;
static int speed_state          = True;
static int map_color_index      = 0;
static int link_color_index     = 0;
/*
 * For the lat-long display.
 */
static float     ll_lat = 0;
static float     ll_lon = 0;
static POINT     ll_point = {0,0};
static MAP_PROJ *ll_proj = NULL;
static MAP_PROJ *ll_tproj = NULL;
static int       ll_display_style   = LL_DECIMAL;
static int       ll_style_original = LL_DECIMAL;
static Widget    ll_display_widget  = NullWidget;


/*========================== Private Functions ======================*/


/* Displays the latitude and longitude corresponding to the current
 * pointer location in the ll_display_widget label widget.
 */
static void display_pointer_location(void)
{
	char     lats, lons, buf[50];
	int      latd, latm, lond, lonm;
	double   dlat, dlon;
	XmString label;
	POINT    tpoint;

	static char       degsb[2] = {'\260','\0'};	/* Single byte degree symbol */
	static String     degmb = "°";				/* Multibyte degree symbol */
	static Widget	  last_display_label = NullWidget;
	static String     degstr = degsb;
	static XmTextType text_type = XmCHARSET_TEXT;

	if (!ll_display_widget) return;

	/* To show the degree symbol we need to know the font type we are dealing with.
	 * Determining if the font used by the lat-long display label is a type1 or multibyte
	 * (xft type) font is time consuming and for a fast update like the poisiton display
	 * causes too much delay. Thus the label is checked once as to type and the degree
	 * symbol chosen. After this just the string creation needs to be done.
	 */
	if(last_display_label != ll_display_widget)
	{
		int n;
		XmRenderTable rendertable = NULL;
		String tags[2] = {_MOTIF_DEFAULT_LOCALE,XmFONTLIST_DEFAULT_TAG};
		last_display_label = ll_display_widget;
		degstr = degsb;
		text_type = XmCHARSET_TEXT;
		XtVaGetValues(ll_display_widget, XmNrenderTable, &rendertable, NULL);
		if(rendertable)
		{
			for(n = 0; n < 2; n++)
			{
				XmRendition rendition = XmRenderTableGetRendition(rendertable, tags[n]);
				if(rendition)
				{
					Arg args[2];
					XmFontType font_type;
					XtSetArg(args[0], XmNfontType, &font_type);
					XmRenditionRetrieve(rendition, args, 1);
					XmRenditionFree(rendition);
					if(font_type == XmFONT_IS_XFT)
					{
						text_type = XmMULTIBYTE_TEXT;
						degstr = degmb;
					}
					break;
				}
			}
		}
	}

	/* Now process the position information and display
	 */
	lats = (ll_lat < 0)? 'S':'N';
	dlat = fabs((double)ll_lat);
	lons = (ll_lon < 0)? 'W':'E';
	dlon = fabs((double)ll_lon);

	switch(ll_display_style)
	{
		case LL_DEG_MIN:
			latd = (int)dlat;
			latm = NINT(60.0*fmod(dlat,1.0));
			lond = (int)dlon;
			lonm = NINT(60.0*fmod(dlon,1.0));
			(void) snprintf(buf, sizeof(buf), "%02d%s%02d\'%c  %03d%s%02d\'%c",
					latd, degstr, latm, lats, lond, degstr, lonm, lons);
			break;

		case MAP_COORD:
			(void) pos_to_pos(ll_proj, ll_point, ll_tproj, tpoint);
			(void) snprintf(buf, sizeof(buf), "x:%.1f  y:%.1f", tpoint[X],tpoint[Y]);
			break;

		default:
			(void) snprintf(buf, sizeof(buf), "%2.2f%s%c  %3.2f%s%c",
				dlat, degstr, lats, dlon, degstr, lons);
			break;
	}
	/*
	 * For some reason sending the alignment every time is necessary, otherwise
	 * the widget seems to "forget" the setting. Why? Who knows.
	 */
	label = XmStringGenerate((XtPointer)buf, NULL, text_type, NULL);
	XtVaSetValues(ll_display_widget,
		XmNlabelString, label,
		XmNalignment, XmALIGNMENT_CENTER,
		NULL);
	XmStringFree(label);
}


/* Follows the pointer motion and displays the corresponding latitude and longitude
 * in the ll_display label widget. The location of the pointer is verified before
 * displaying.
 */
static void pointer_motion_handler(Widget w , XtPointer client_data , XMotionEvent *event)
{
	float       lat, lon;
	POINT       point;
	Screencoord l, r, b, t;

	/* The current active zoomed projection should be returned by
	 * gxGetZoomMproj(), but in case there is a problem (or if the
	 * display has not yet been zoomed) default to the base projection.
	 */
	ll_proj = gxGetZoomMproj();
	if (!ll_proj || equivalent_map_projection(ll_proj, &NoMapProj))
		ll_proj = ll_tproj;

	glGetClipRectangle( &l, &r, &b, &t );

	/* Convert screen coordinates into projection coordinates. The assumption here
	 * is that the projection maps to the current clip rectangle.
	 */
	point[X] = ((float)(event->x - l) * ll_proj->definition.xlen) / (float)(r - l + 1);
	point[Y] = ((float)(event->y - t) * ll_proj->definition.ylen) / (float)(b - t + 1);

	/* Check that our point is valid for the projection.
	 */
	if (!inside_map_def(&(ll_proj->definition), point)) return;

	if(pos_to_ll( ll_proj, point, &lat, &lon))
	{
		ll_lat = lat;
		ll_lon = lon;
		ll_point[X] = point[X];
		ll_point[Y] = point[Y];
	}
	display_pointer_location();
}


/* Callback for the pointer location selection buttons. The callback sets the
 * display style.
 */
/*ARGSUSED*/
static void ll_display_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	if(XmToggleButtonGetState(w))
		ll_display_style = PTR2INT(client_data);

	/* Display the location on the map and in this dialog */
	ll_display_widget = w_ll_display;
	display_pointer_location();
    ll_display_widget = GW_latLongDisplay;
	display_pointer_location();
}



/*========================== Public Functions ========================*/

void InitGeneralOptions(void)
{
	int     i;
	int     val, confirm, delete;
	int     restore_edit = 0; 
	int     lat_long_style = LL_DECIMAL;
	char    mbuf[500];
	String  last_palette, ptr;
	Boolean ok;
	SETUP   *setup;

	
	ptr = XuGetStringResource(".confirmExit","y");
	GV_pref.confirm_exit = (strchr("YyTt", (int) ptr[0]) != NULL);
	confirm = (int) GV_pref.confirm_exit;
	delete = 0;

	if(XuVaStateDataGet(KEY, NULL, NULL, "%d %d %d %d %d %d %d %d %d",
		&val,
		&control_state,
		&early_late_state,
		&speed_state,
		&time_state,
		&restore_edit,
		&lat_long_style,
		&confirm,
		&delete))
	{
		GV_pref.daily_date_format  = (val)? SHORT_DAY_NAME_NR_OF_MONTH : DAY_NR;
		GV_pref.restore_edit_state = (Boolean) restore_edit;
		GV_pref.confirm_exit       = (Boolean) confirm;
		GV_pref.show_delete_all    = (Boolean) delete;
		ll_display_style      = lat_long_style;
	}

	/* Check the validity of the display style */
	switch(ll_display_style)
	{
		case LL_DECIMAL:
		case LL_DEG_MIN:
		case MAP_COORD:
			break;

		default:
			ll_display_style = LL_DEG_MIN;
	}

    ll_display_widget  = GW_latLongDisplay;
	ll_style_original = ll_display_style;

	/* Set the default display of the pointer position to be the map origin */
	ll_proj = ll_tproj = get_target_map();
	(void) pos_to_ll( ll_proj, ll_point, &ll_lat, &ll_lon);
	display_pointer_location();

	(void) IngredVaCommand(GE_TIMELINK, "SHOW CONTROL %s", (control_state)? "ON":"OFF");
	(void) IngredVaCommand(GE_TIMELINK, "SHOW EARLY_LATE %s", (early_late_state)? "ON":"OFF");
	(void) IngredVaCommand(GE_TIMELINK, "SHOW SPEED %s", (speed_state)? "ON":"OFF");
	(void) IngredVaCommand(GE_TIMELINK, "SHOW TIME %s", (time_state)? "ON":"OFF");

	/* Initialize the map colour scheme
	 */
    setup = GetSetup(MAP_PALETTE);
	if(setup != NULL && setup->nentry > 0)
	{
		if(XuStateDataGet(MAP, PALETTE, NULL, &last_palette))
		{
			map_color_index = int_arg(last_palette, &ok);
			if(map_color_index < 0) map_color_index = 0;
			XtFree(last_palette);
		}
		map_color_index = (map_color_index < setup->nentry) ? map_color_index : 0;

		strcpy(mbuf,"MAP PALETTE");
		for( i = 1; i < setup->entry[map_color_index].nparms; i++ )
		{
			strcat(mbuf," '");
			strcat(mbuf, SetupParm(setup, map_color_index, i));
			strcat(mbuf,"'");
		}
		(void) IngredCommand(GE_ACTION, mbuf);	
	}
	/*
	 * Initialize the timelink link colour scheme
	 */
	(void) strcpy(mbuf,"LINK PALETTE");
    setup = GetSetup(TIMELINK_PALETTE);
	if(setup != NULL && setup->nentry > 0)
	{
		if(XuStateDataGet(LINK, PALETTE, NULL, &last_palette))
		{
			link_color_index = int_arg(last_palette, &ok);
			if(link_color_index < 0) link_color_index = 0;
			XtFree(last_palette);
		}
		link_color_index = (link_color_index < setup->nentry) ? link_color_index : 0;
		for( i = 1; i < setup->entry[link_color_index].nparms; i++ )
		{
			(void) strcat(mbuf, " '");
			(void) strcat(mbuf, SetupParm(setup, link_color_index, i));
			(void) strcat(mbuf, "'");
		}
	}
	else
	{
		/* Set some default link colours */
		(void) strcat(mbuf, " Cyan CadetBlue Magenta");
	}
	(void) IngredCommand(GE_ACTION, mbuf);
	/*
	 * Attach the pointer motion tracker for the display of pointer
	 * position as lat-long and such. The display must exist.
	 */
	if(GW_latLongDisplay)
		XtAddEventHandler(GW_mapWindow,PointerMotionMask,False,(XtEventHandler)pointer_motion_handler,0);
}


/* Called to create the background colour and map overlay selection buttons.
*/
void GeneralOptions(Widget parent )
{
	int    i, ncols;
	Widget btn, frame, form, rc, linkrc;
	SETUP  *setup;

	/*===== Misc options ======*/

	rc = XmVaCreateManagedRowColumn(parent, "form",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	w_exit = XmVaCreateManagedToggleButton(rc, "programExit",
		XmNset, GV_pref.confirm_exit? XmSET:XmUNSET,
		NULL);

	w_delete_all = XmVaCreateManagedToggleButton(rc, "deleteDepict",
		XmNset, GV_pref.show_delete_all? XmSET:XmUNSET,
		NULL);

	w_restore = XmVaCreateManagedToggleButton(rc, "editStateSave",
		XmNset, GV_pref.restore_edit_state? XmSET:XmUNSET,
		NULL);

	w_dts = XmVaCreateManagedToggleButton(rc, "dailyTimeOption",
		XmNset, (GV_pref.daily_date_format == SHORT_DAY_NAME_NR_OF_MONTH)? XmSET:XmUNSET,
		NULL);

	/*=============== Map Colours ========================*/

	frame = XmVaCreateManagedFrame(parent, "frame",
		XmNmarginHeight, 9,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, rc,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "MapSettings",
		XmNframeChildType, XmFRAME_TITLE_CHILD,
		NULL);

	/* Scan the palette labels so that we can set the combobox
	 * to a reasonable size for the selection list.
	 */
	setup = GetSetup(MAP_PALETTE);
	if(setup != NULL && setup->nentry > 0)
	{
		for( ncols = 0, i = 0; i < setup->nentry; i++ )
		{
			String label = SetupParm(setup,i,0);
			if(safe_strlen(label) > ncols) ncols = safe_strlen(label);
		}

		w_map_color = XmVaCreateManagedComboBox(frame, "mapSelect",
			XmNcolumns, ncols,
			XmNvisibleItemCount, MIN(10,setup->nentry),
			NULL);

		for( ncols = 0, i = 0; i < setup->nentry; i++ )
		{
			XuComboBoxAddItem(w_map_color, SetupParm(setup,i,0), 0);
		}
	}
	else
	{
		w_map_color = XmVaCreateManagedComboBox(frame, "mapSelect",
			XmNcolumns, safe_strlen(XuGetLabel("default")),
			XmNvisibleItemCount, 1,
			NULL);
		XuComboBoxAddItem(w_map_color, XuGetLabel("default"), 0);
	}
	/* Note that combobox is defaulted to one origin in fallback.h */
	XuComboBoxSelectPos(w_map_color, map_color_index + 1, False);

	/*============ Lat-Long display selection ==============*/

	frame = XmVaCreateManagedFrame(parent, "lld",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, frame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "latLongDisplayStyle",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateManagedForm(frame, "form",
		XmNverticalSpacing, 5,
		NULL);

	w_ll_display = XmVaCreateManagedLabel(form, "latLongDisplay",
		XmNborderWidth, 1,
		XmNmarginHeight, 5,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	rc = XmVaCreateManagedRowColumn(form, "rc",
		XmNradioBehavior, True,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, w_ll_display,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	btn = XmVaCreateManagedToggleButton(rc, "decimalDegrees",
		XmNset, (ll_display_style == LL_DECIMAL),
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, ll_display_cb, INT2PTR(LL_DECIMAL));

	btn = XmVaCreateManagedToggleButton(rc, "degreesMinutes",
		XmNset, (ll_display_style == LL_DEG_MIN),
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, ll_display_cb, INT2PTR(LL_DEG_MIN));

	btn = XmVaCreateManagedToggleButton(rc, "Map Coordinates",
		XmNset, (ll_display_style == MAP_COORD),
		NULL);
	XtAddCallback(btn, XmNvalueChangedCallback, ll_display_cb, INT2PTR(MAP_COORD));

	/*=============== Timelink options ==================*/

	frame = XmVaCreateManagedFrame(parent, "timelink",
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNmarginWidth, 9,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, w_dts,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, frame,
		NULL);

	(void)XmVaCreateManagedLabel(frame, "optTimelink",
		XmNchildType, XmFRAME_TITLE_CHILD,
		NULL);

	rc = XmVaCreateManagedRowColumn(frame, "form",
		NULL);

	w_control = XmVaCreateManagedToggleButton(rc, "optControl",
		XmNset, (Boolean)control_state,
		NULL);

	w_early_late = XmVaCreateManagedToggleButton(rc, "optEarlyLate",
		XmNset, (Boolean)early_late_state,
		NULL);

	w_speed = XmVaCreateManagedToggleButton(rc, "optSpeed",
		XmNset, (Boolean)speed_state,
		NULL);

	w_time = XmVaCreateManagedToggleButton(rc, "optTime",
		XmNset, (Boolean)time_state,
		NULL);

	/* Timelink colour selection */
	linkrc = XmVaCreateRowColumn(rc, "frame",
		XmNspacing,  0,
		XmNmarginWidth,  0,
		XmNmarginHeight, 5,
		XmNadjustLast, False,
		NULL);

	(void) XmVaCreateManagedLabel(linkrc, "linkColor", NULL);

	setup = GetSetup(TIMELINK_PALETTE);
	if(setup != NULL && setup->nentry > 0)
	{
		for( ncols = 0, i = 0; i < setup->nentry; i++ )
		{
			String label = SetupParm(setup,i,0);
			if(safe_strlen(label) > ncols) ncols = safe_strlen(label);
		}

		w_link_color = XmVaCreateManagedComboBox(linkrc, "linkColor",
			XmNcolumns, ncols,
			XmNvisibleItemCount, MIN(10,setup->nentry),
			NULL);

		for( i = 0; i < setup->nentry; i++ )
		{
			XuComboBoxAddItem(w_link_color, SetupParm(setup,i,0), 0);
		}
	}
	else
	{
		w_link_color = XmVaCreateManagedComboBox(linkrc, "linkColor",
			XmNcolumns, safe_strlen(XuGetLabel("default")),
			XmNvisibleItemCount, 1,
			NULL);
		XuComboBoxAddItem(w_link_color, XuGetLabel("default"), 0);
	}
	/* Note that combobox is defaulted to one origin in fallback.h */
	XuComboBoxSelectPos(w_link_color, link_color_index + 1, False);
	XtManageChild(linkrc);
	
	/* Display the pointer location for the lat-long selector */
	ll_display_widget = w_ll_display;
	display_pointer_location();
}


/* Called to activate those items which take effect when accepted.
*/
void SetGeneralOptions(void)
{
	int   n, date_state, state;
	SETUP *setup;

	GV_pref.confirm_exit       = XmToggleButtonGetState(w_exit);
	GV_pref.restore_edit_state = XmToggleButtonGetState(w_restore);
	GV_pref.show_delete_all    = XmToggleButtonGetState(w_delete_all);

	date_state = (int)XmToggleButtonGetState(w_dts);
	GV_pref.daily_date_format = (date_state)? SHORT_DAY_NAME_NR_OF_MONTH : DAY_NR;

	state = (int)XmToggleButtonGetState(w_control);
	if( state != control_state )
	{
		control_state = state;
		(void) IngredVaCommand(GE_TIMELINK, "SHOW CONTROL %s", (control_state)? "ON":"OFF");
	}

	state = (int)XmToggleButtonGetState(w_early_late);
	if( state != early_late_state )
	{
		early_late_state = state;
		(void) IngredVaCommand(GE_TIMELINK, "SHOW EARLY_LATE %s", (early_late_state)? "ON":"OFF");
	}

	state = (int)XmToggleButtonGetState(w_speed);
	if( state != speed_state )
	{
		speed_state = state;
		(void) IngredVaCommand(GE_TIMELINK, "SHOW SPEED %s", (speed_state)? "ON":"OFF");
	}

	state = (int)XmToggleButtonGetState(w_time);
	if( state != time_state )
	{
		time_state = state;
		(void) IngredVaCommand(GE_TIMELINK, "SHOW TIME %s", (time_state)? "ON":"OFF");
	}

	/* The original location display is now the
	 * set style as it has been accepted 
	 */
	ll_style_original = ll_display_style;

	XuVaStateDataSave(KEY, NULL, NULL, "%d %d %d %d %d %d %d %d %d",
		date_state,
		control_state,
		early_late_state,
		speed_state,
		time_state,
		(int) GV_pref.restore_edit_state,
		ll_display_style,
		(int) GV_pref.confirm_exit,
		(int) GV_pref.show_delete_all);

	/* Set the map colour */
	setup = GetSetup(MAP_PALETTE);
	if(setup != NULL && setup->nentry > 0)
	{
		char mbuf[500];
		map_color_index = XuComboBoxGetSelectedPos(w_map_color) - 1;
		XuVaStateDataSave(MAP, PALETTE, NULL, "%d", map_color_index, NULL);
		strcpy(mbuf,"MAP PALETTE");
		for( n = 1; n < setup->entry[map_color_index].nparms; n++ )
		{
			strcat(mbuf," '");
			strcat(mbuf, SetupParm(setup, map_color_index, n));
			strcat(mbuf,"'");
		}
		(void) IngredCommand(GE_ACTION, mbuf);
	}

	/* Set the link colour */
	setup = GetSetup(TIMELINK_PALETTE);
	if(setup != NULL && setup->nentry > 0)
	{
		char mbuf[500];
		link_color_index = XuComboBoxGetSelectedPos(w_link_color) - 1;
		XuVaStateDataSave(LINK, PALETTE, NULL, "%d", link_color_index, NULL);
		strcpy(mbuf,"LINK PALETTE");
		for( n = 1; n < setup->entry[link_color_index].nparms; n++ )
		{
			strcat(mbuf," '");
			strcat(mbuf, SetupParm(setup, link_color_index, n));
			strcat(mbuf,"'");
		}
		(void) IngredCommand(GE_ACTION, mbuf);
	}
}


void ExitGeneralOptions(void)
{
	/* If the display style was not accepted this will return it
	 * to its original state.
	 */
	ll_display_style  = ll_style_original;
    ll_display_widget = GW_latLongDisplay;
	display_pointer_location();
}
