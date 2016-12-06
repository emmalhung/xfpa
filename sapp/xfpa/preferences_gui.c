/*=========================================================================*/
/*
*	preferences_gui.c() - Contains functions, to be used in conjunction
*	                      with the preferences dialog, to change the
*	appearance of some of the graphical user interface elements. Only when
*	the apply button is activated will the changes be applied.
*
*      This resource gives the list of colours that are available 
*      to colour the cursor with, for both background and foreground.
*
*      cursorColors: colour1 colour2 ... colourn
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

#include "global.h"
#include <Xu.h>
#include <X11/cursorfont.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/Column.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/XmpSpinBox.h>
#include "resourceDefines.h"
#include "main_cursor_im.h"
#include "main_cursor_ularrow.h"
#include "preferences.h"

#define KEY		"prefgui"
#define ENVCUR	"FPA_ENVCUR"

/* Define struct to hold cursor information provided in the header files. Note
*  that we have the definition of both the main cursor and the cursor to be used
*  with pulldown menus. Since these will both be the same size we do not need
*  a width and height for the menu cursor.
*/
static struct {
	String name;			/* as seen in the resource file for the widget name */
	unsigned int width;
	unsigned int height;
	unsigned int x_hot;
	unsigned int y_hot;
	Byte *bits;
	Byte *mask;
	unsigned int menu_x_hot;
	unsigned int menu_y_hot;
	Byte *menu_bits;
	Byte *menu_mask;
} cursors[] = {
	{"default", 0, 0,				/* default is a special case */
		0, 0, NULL, NULL, 
		0, 0, NULL, NULL},
	{"im", im_width, im_height,
		im_x_hot, im_y_hot, im_bits, im_mask_bits, 
		im_menu_x_hot, im_menu_y_hot, im_menu_bits, im_menu_mask_bits},
	{"ularrow", ularrow_width, ularrow_height,
		ularrow_x_hot, ularrow_y_hot, ularrow_bits, ularrow_mask_bits, 
		ularrow_menu_x_hot, ularrow_menu_y_hot, ularrow_menu_bits, ularrow_menu_mask_bits},
};

/* Structure to hold information on the temporary cursor used while the user
*  selects which default cursor to display.
*/
static struct {
	String id;
	String fg;
	String bg;
	Cursor cursor;
} tc;

static Widget parent = NULL;
static int    iconbar_location = TB_TOP;
static int    timebar_location = TB_TOP;
static int    message_location = TB_TOP;
static char   default_name[16];
static char   default_fg[32];
static char   default_bg[32];
static int    ncolour = 0;
static String *colour = NULL;
static int    updating_flash_time;
static int    updating_flash_delay;

static void bookshelf_cb         (Widget, XtPointer, XtPointer);
static Cursor create_cursor	     (String, String, String);
static void delayed_cursor_set   (XtPointer, XtIntervalId*);
static void exit_cb			     (Widget, XtPointer, XtPointer);
static void cursor_type_cb	     (Widget, XtPointer, XtPointer);
static void fg_colour_cb	     (Widget, XtPointer, XtPointer);
static void bg_colour_cb	     (Widget, XtPointer, XtPointer);
static void iconbar_location_cb  (Widget, XtPointer, XtPointer);
static void timebar_location_cb  (Widget, XtPointer, XtPointer);
static void message_location_cb  (Widget, XtPointer, XtPointer);

static XuCursorDataStruct *set_cursor_data     (String, String, String);
static XuCursorDataStruct *set_menu_cursor_data(String, String, String);


/*=========================================================================*/
/*
*   InitMainGui() - Initilization
*
*   1. Sets the standard cursor to be used by all windows using these
*      functions. 
*   2. Sets the Motif menu cursor.
*   3. Initializes other preference settings.
*/
/*=========================================================================*/
void InitMainGui(void)
{
	char *ptr, *env, mbuf[500];

	tc.id = default_name;
	tc.fg = default_fg;
	tc.bg = default_bg;

	strcpy(default_name, cursors[0].name);
	strcpy(default_fg,   "white");
	strcpy(default_bg,   "black");

	/* If there is a cursor defined in the environment we will use this
	*  as this means we were launched by another version of the FPA and
	*  this cursor gets priority.
	*/
	ptr = NULL;
	env = getenv(ENVCUR);
	if(!blank(env))
		ptr = XtNewString(env);
	else if(!XuStateDataGet("xu", "cursor", "default", &ptr))
		return;

	strcpy(default_name, strtok_arg(ptr));
	strcpy(default_fg,   strtok_arg(NULL));
	strcpy(default_bg,   strtok_arg(NULL));
	FreeItem(ptr);

	XuSetDefaultCursor(set_cursor_data(default_name, default_fg, default_bg));
	XuSetDefaultMenuCursor(set_menu_cursor_data(default_name, default_fg, default_bg));

	if(blank(env))
	{
		(void) snprintf(mbuf, sizeof(mbuf), "%s=%s \"%s\" \"%s\"", ENVCUR, default_name, default_fg, default_bg);
		(void) putenv(strdup(mbuf));
	}

	/* updating_flash_time is the length of time to let the indicator flash after the source
	 * has finished updating.  updating_flash_delay is the length of time after the user has
	 * looked at the status in the dialog when the indicator will not show even if the source
	 * updates.
	 */
	updating_flash_time  = MAX(1,XuGetIntResource(RNsourceUpdatingFlashTime, 5));
	updating_flash_delay = XuGetIntResource(RNsourceUpdatingIndicatorDelay, 15);

	(void) XuVaStateDataGet(KEY,NULL,NULL, "%d %d %d %d %d",
		&timebar_location, 
		&message_location,
		&iconbar_location,
		&updating_flash_time,
		&updating_flash_delay);

	SetGUIComponentLocation(iconbar_location, timebar_location, message_location);

	/* The global preferences are in seconds while the internal ones are in minutes */
	GV_pref.updating_flash_time  = updating_flash_time * 60;
	GV_pref.updating_flash_delay = updating_flash_delay * 60;
}


/*=========================================================================*/
/*
*	setCursor() - Creates the cursor selection widgets.
*/
/*=========================================================================*/
void GuiOptions(Widget parent_widget )
{
	int	   i;
	String ptr, colours;
	Widget w, cursorFrame, cursorSelect, frame, form;
	Widget bgSelect, fgSelect;
	Widget label, timer;

	parent = parent_widget;

	/*============= Cursor Setting ================*/

	/* Get the list of cursor colours from the resource file.
	*/
	ptr = XuGetStringResource(RNcursorColors, "");
	colours = XtNewString(ptr);
	ptr = strtok_arg(colours);
	while(ptr)
	{
		colour = MoreStringArray(colour, ncolour+1);
		colour[ncolour] = XtNewString(ptr);
		/*
		 * Sometimes the colour assigned to the default values at this point
		 * can differ only in the case of the name ("White" vs "white" due to
		 * resource file changes), so the case is checked. This avoids error
		 * messages from the XuMenu functions if this occurs.
		 */
		if(same_ic(default_fg, colour[ncolour]))
			(void) strcpy(default_fg, colour[ncolour]);
		if(same_ic(default_bg, colour[ncolour]))
			(void) strcpy(default_bg, colour[ncolour]);
		ncolour++;
		ptr = strtok_arg(NULL);
	}

	XtAddCallback(parent, XmNdestroyCallback, exit_cb, NULL);

	cursorFrame = XmVaCreateManagedFrame(parent, "frame",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(cursorFrame, "cursorLabel",
		XmNframeChildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateColumn(cursorFrame, "form",
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNlabelSpacing, 9,
		XmNitemSpacing,  0,
		XmNmarginWidth,  9,
		XmNmarginHeight, 9,
		NULL);

	cursorSelect = XuVaMenuBuildOption(form, "cursorSelect", NULL, NULL);
	for( i = 0; i < XtNumber(cursors); i++ )
	{
		(void) XuMenuAddButton(cursorSelect, cursors[i].name, NULL, NoId, cursor_type_cb, cursors[i].name);
	}
	XuMenuSelectItemByName(cursorSelect, default_name);

	fgSelect = XuVaMenuBuildOption(form, "fgSelect", NULL, NULL);
    for( i = 0; i < ncolour; i++ )
	{
		Pixmap ins, pix;
		pix = XuCreateColoredPixmap(fgSelect, colour[i], 32, 12);
		ins = XuCreateInsensitivePixmap(fgSelect, pix);
		(void) XuMenuAddPixmapButton(fgSelect, colour[i], NoId, pix, ins, fg_colour_cb, (XtPointer)colour[i]);
	}
	XuMenuSelectItemByName(fgSelect, default_fg);

	bgSelect = XuVaMenuBuildOption(form, "bgSelect", NULL, NULL);
    for( i = 0; i < ncolour; i++ )
	{
		Pixmap ins, pix;
		pix = XuCreateColoredPixmap(bgSelect, colour[i], 32, 12);
		ins = XuCreateInsensitivePixmap(bgSelect, pix);
		(void) XuMenuAddPixmapButton(bgSelect, colour[i], NoId, pix, ins, bg_colour_cb, (XtPointer)colour[i]);
	}
	XuMenuSelectItemByName(bgSelect, default_bg);

	XtManageChild(form);

	/* Copy the current default cursor data. We then need to set it directly
	*  into the dialog replacing the real default as we remove the cursor when
	*  we exit the dialog.
	*/
	tc.id     = default_name;
	tc.fg     = default_fg;
	tc.bg     = default_bg;
	tc.cursor = None;

	for( i = 0; i < XtNumber(cursors); i++ )
	{
		if(!same(default_name, cursors[i].name)) continue;
		w = XtNameToWidget(cursorSelect,cursors[i].name);
		(void) XtAppAddTimeOut(GV_app_context, 500, delayed_cursor_set, (XtPointer)w);
		break;
	}

	/*=============== Time and Message Bar Location =================*/

	frame = XmVaCreateManagedFrame(parent, "frame",
		XmNmarginWidth, 9,
		XmNmarginHeight, 0,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, cursorFrame,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "locations",
		XmNframeChildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateColumn(frame, "aligner",
		XmNdefaultEntryLabelAlignment, XmALIGNMENT_END,
		XmNlabelSpacing, 9,
		XmNitemSpacing,  0,
		XmNmarginWidth,  9,
		XmNmarginHeight, 9,
		NULL);

	/* The preferences can be set in viewer mode.
	 */
	w = XuVaMenuBuildOption(form, "iconbarPos", NULL, NULL);
	(void) XuMenuAddButton(w, "top",    NULL, TB_TOP,    iconbar_location_cb, (XtPointer)TB_TOP   );
	(void) XuMenuAddButton(w, "bottom", NULL, TB_BOTTOM, iconbar_location_cb, (XtPointer)TB_BOTTOM);
	XuMenuSelectItem(w, iconbar_location);

	w = XuVaMenuBuildOption(form, "timebarPos", NULL, NULL);
	(void) XuMenuAddButton(w, "top",    NULL, TB_TOP,    timebar_location_cb, (XtPointer)TB_TOP   );
	(void) XuMenuAddButton(w, "bottom", NULL, TB_BOTTOM, timebar_location_cb, (XtPointer)TB_BOTTOM);
	XuMenuSelectItem(w, timebar_location);

	w = XuVaMenuBuildOption(form, "messagePos", NULL, NULL);
	(void) XuMenuAddButton(w, "top",    NULL, TB_TOP,    message_location_cb, (XtPointer)TB_TOP   );
	(void) XuMenuAddButton(w, "bottom", NULL, TB_BOTTOM, message_location_cb, (XtPointer)TB_BOTTOM);
	XuMenuSelectItem(w, message_location);

	XtManageChild(form);

	/*=============== BOOKSHELF TIME SETTINGS ===================*/

	/* The global preferences are in seconds while the internal ones are in minutes */
	updating_flash_time  = GV_pref.updating_flash_time/60;
	updating_flash_delay = GV_pref.updating_flash_delay/60;

	frame = XmVaCreateManagedFrame(parent, "frame",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, cursorFrame,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	(void) XmVaCreateManagedLabel(frame, "bookshelfOptionsLabel",
		XmNframeChildType, XmFRAME_TITLE_CHILD,
		NULL);

	form = XmVaCreateForm(frame, "form",
		XmNverticalSpacing, 15,
		XmNhorizontalSpacing, 15,
		NULL);

	label = XmVaCreateManagedLabel(form, "beginFlashTime",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	timer = XmpVaCreateManagedSpinBox(form, "timer1",
		XmNspinBoxType, XmSPINBOX_NUMBER,
		XmNminimum, 0,
		XmNmaximum, 60,
		XmNvalue, updating_flash_delay,
		XmNcolumns, 2,
		XmNeditable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 50,
		NULL);

	XtAddCallback(timer, XmNvalueChangedCallback, bookshelf_cb, (XtPointer) &updating_flash_delay);

	(void) XmVaCreateManagedLabel(form, "minutesBtn",
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, timer,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, timer,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, timer,
		XmNbottomOffset, 0,
		NULL);

	label = XmVaCreateManagedLabel(form, "endFlashTime",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, timer,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	timer = XmpVaCreateManagedSpinBox(form, "timer2",
		XmNspinBoxType, XmSPINBOX_NUMBER,
		XmNminimum, 1,
		XmNmaximum, 60,
		XmNvalue, updating_flash_time,
		XmNcolumns, 2,
		XmNeditable, False,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 50,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XtAddCallback(timer, XmNvalueChangedCallback, bookshelf_cb, (XtPointer) &updating_flash_time);

	(void) XmVaCreateManagedLabel(form, "minutesBtn",
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, timer,
		XmNtopOffset, 0,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, timer,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, timer,
		XmNbottomOffset, 0,
		NULL);

	XtManageChild(form);
}


void SetGuiOptions(void)
{
	char  mbuf[500];

	if(!blank(tc.id) && !blank(tc.fg) && !blank(tc.bg))
	{
		(void) snprintf(mbuf, sizeof(mbuf), "%s=%s \"%s\" \"%s\"", ENVCUR, tc.id, tc.fg, tc.bg);
		(void) putenv(strdup(mbuf));

		XuVaStateDataSave("xu", "cursor", "default",
			"%s \"%s\" \"%s\"", tc.id, tc.fg, tc.bg, NULL);
		if(tc.id != default_name) strcpy(default_name, tc.id);
		if(tc.fg != default_fg)   strcpy(default_fg, tc.fg);
		if(tc.bg != default_bg)   strcpy(default_bg, tc.bg);

		XuSetDefaultCursor(set_cursor_data(default_name, default_fg, default_bg));
		XuSetDefaultMenuCursor(set_menu_cursor_data(default_name, default_fg, default_bg));
		XFlush(XtDisplay(parent));
	}

	/* Set bar locations */
	(void) XuVaStateDataSave(KEY,NULL,NULL, "%d %d %d %d %d",
		timebar_location,
		message_location,
		iconbar_location,
		updating_flash_time,
		updating_flash_delay);

	SetGUIComponentLocation(iconbar_location, timebar_location, message_location);

	/* The global preferences are in seconds while the internal ones are in minutes. */
	GV_pref.updating_flash_time  = updating_flash_time * 60;
	GV_pref.updating_flash_delay = updating_flash_delay * 60;
}


/* ================= LOCAL FUNCTIONS =====================*/


/* This is called in a time out so that the cursor will be set into the dialog
*  properly. If not done this way things do not work as one would expect!
*/
/*ARGSUSED*/
static void delayed_cursor_set(XtPointer client_data, XtIntervalId *id)
{
	XuToggleButtonSet((Widget)client_data, False, False);
	XuToggleButtonSet((Widget)client_data, True,  True );
}


/*ARGSUSED*/
static void cursor_type_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	Cursor cursor_old = tc.cursor;
	tc.id = (String) client_data;
	tc.cursor = create_cursor(tc.id, tc.fg, tc.bg);
	XuSetDialogCursorDirect(parent, tc.cursor);
	if(cursor_old != None) XFreeCursor(XtDisplay(parent), cursor_old);
}


/*ARGSUSED*/
static void fg_colour_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	Cursor cursor_old = tc.cursor;
	tc.fg = (String)client_data;
	tc.cursor = create_cursor(tc.id, tc.fg, tc.bg);
	XuSetDialogCursorDirect(parent, tc.cursor);
	if(cursor_old != None) XFreeCursor(XtDisplay(parent), cursor_old);
}


/*ARGSUSED*/
static void bg_colour_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	Cursor cursor_old = tc.cursor;
	tc.bg = (String)client_data;
	tc.cursor = create_cursor(tc.id, tc.fg, tc.bg);
	XuSetDialogCursorDirect(parent, tc.cursor);
	if(cursor_old != None) XFreeCursor(XtDisplay(parent), cursor_old);
}


static XuCursorDataStruct *set_cursor_data( String cursor_name, String fg, String bg )
{ 
	int i;
	static XuCursorDataStruct dc;

	if(same(cursor_name, cursors[0].name)) return NULL;

	for(i = 0; i < XtNumber(cursors); i++)
	{
		if(!same(cursor_name, cursors[i].name)) continue;
		dc.width  = cursors[i].width;
		dc.height = cursors[i].height;
		dc.bits   = cursors[i].bits;
		dc.mask   = cursors[i].mask;
		dc.x_hot  = cursors[i].x_hot;
		dc.y_hot  = cursors[i].y_hot;
		dc.fg     = fg;
		dc.bg     = bg;
		return &dc;
	}
	return NULL;
}


static XuCursorDataStruct *set_menu_cursor_data( String cursor_name, String fg, String bg )
{ 
	int i;
	static XuCursorDataStruct dc;

	if(same(cursor_name, cursors[0].name)) return NULL;

	for(i = 0; i < XtNumber(cursors); i++)
	{
		if(!same(cursor_name, cursors[i].name)) continue;
		dc.width  = cursors[i].width;
		dc.height = cursors[i].height;
		dc.bits   = cursors[i].menu_bits;
		dc.mask   = cursors[i].menu_mask;
		dc.x_hot  = cursors[i].menu_x_hot;
		dc.y_hot  = cursors[i].menu_y_hot;
		dc.fg     = fg;
		dc.bg     = bg;
		return &dc;
	}
	return NULL;
}


static Cursor create_cursor( String cursor_name , String fg , String bg )
{
	int i;
	Cursor cursor;
	Pixmap pixmap, mask;
	XColor color[2];
	Colormap colormap;

	if(same(cursor_name, cursors[0].name)) return None;

	for(i = 0; i < XtNumber(cursors); i++)
	{
		if(same(cursor_name, cursors[i].name)) break;
	}
	if(i >= XtNumber(cursors)) return None;

	XtVaGetValues(GW_mainWindow, XmNcolormap, &colormap, NULL);
	color[0].pixel = XuLoadColor(GW_mainWindow,fg);
	color[1].pixel = XuLoadColor(GW_mainWindow,bg);
	XQueryColors(XtDisplay(GW_mainWindow), colormap, color, 2);

	pixmap = XCreateBitmapFromData(XtDisplay(GW_mainWindow),
		XRootWindowOfScreen(XtScreen(GW_mainWindow)),
		(char *)cursors[i].bits,
		cursors[i].width,
		cursors[i].height);
	mask = XCreateBitmapFromData(XtDisplay(GW_mainWindow),
		XRootWindowOfScreen(XtScreen(GW_mainWindow)),
		(char *)cursors[i].mask,
		cursors[i].width,
		cursors[i].height);
	cursor = XCreatePixmapCursor(XtDisplay(GW_mainWindow),
		pixmap, mask,
		&color[0], &color[1],
		cursors[i].x_hot, cursors[i].y_hot);

	XFreePixmap(XtDisplay(GW_mainWindow), pixmap);
	XFreePixmap(XtDisplay(GW_mainWindow), mask);
	return cursor;
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i;
	for(i = 0; i < ncolour; i++) FreeItem(colour[i]);
	FreeItem(colour);
	ncolour = 0;
	colour = NULL;
	if(tc.cursor != None) XFreeCursor(XtDisplay(parent), tc.cursor);
}


/*ARGSUSED*/
static void iconbar_location_cb(Widget w , XtPointer client_data , XtPointer call_data)
{
	iconbar_location = PTR2INT(client_data);
}


/*ARGSUSED*/
static void timebar_location_cb(Widget w , XtPointer client_data , XtPointer call_data)
{
	timebar_location = PTR2INT(client_data);
}


/*ARGSUSED*/
static void message_location_cb(Widget w , XtPointer client_data , XtPointer call_data)
{
	message_location = PTR2INT(client_data);
}


static void bookshelf_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	XmpSpinBoxCallbackStruct *rtn = (XmpSpinBoxCallbackStruct *)call_data;
	*((int*) client_data) = (int) rtn->value;
}

