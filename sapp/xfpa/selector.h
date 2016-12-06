/*========================================================================*/
/*
*	File:		selector.h
*
*   Purpose:    Header for selector routines
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
#ifndef SELECTOR_H
#define SELECTOR_H

/* Enumerated type for determining which actions are to be taken by
*  some of the selectors.
*/
typedef enum {
	SELECT_NONE,
	SELECT_EDIT_LABEL,
	SELECT_EDIT_SAMPLE,
	SELECT_GUID_LABEL,
	SELECT_GUID_SAMPLE,
	SELECT_IMAGE_LABEL,
	SELECT_IMAGE_SAMPLE,
	SELECT_COLOUR,
	SELECT_STYLE,
	SELECT_TEXT_MODS,
	SELECT_TEXT_FOCUS,
	SELECT_FONT_TYPE,
	SELECT_FONT_SIZE,
	SELECT_FONT_COLOUR
} SELECT_TYPE;
/*
 * Structure and enumerated keys used for obtaining data from the
 * colour and line style setting "widget" structure contained in
 * createColorSelector.c.
 */
typedef struct _css {
	SELECT_TYPE reason;
	String      colour;
	String      style;
} ColorSelectorStruct;
/*
 * Structure and enumerated keys used for obtaining data from the
 *  text, font and size  setting "widget" structure contained in
 *  createTextSelector.c.
 */
typedef struct _tss {
	SELECT_TYPE reason;
	String      text;
	String      font;
	String      size;
	String      colour;
} TextSelectorStruct;
/*
 * Structure used by selector_displayFont.c
 */
typedef struct _fss {
	SELECT_TYPE reason;
	String      type;
	String      size;
	String      colour;
} FontSelectorStruct;
/*
 * Resources for targetTime. The no label versions remove the label that is normally put to the
 * left of the spinbox. Was the easiest way to do this without a major rewrite.
 */
typedef enum { DATE_TO_MINUTE, DATE_TO_HOUR, DATE_TO_DAY, DATE_TO_MINUTE_NO_LABEL, DATE_TO_HOUR_NO_LABEL } DATE_DISPLAY;
/*
 * Structure used to pass back data from the time selector.
 */
typedef struct _tws {
	int    start_ndx;
	int    end_ndx;
	TSTAMP start_time;
	TSTAMP end_time;
} TimeWindowSelectorStruct;

extern Widget CreateColorSelector            (Widget, PANEL_ID, Boolean, void(*)(), ...);
extern void   SetColorSelector               (Widget, String, String);
extern String GetColorSelectorValue          (Widget, SELECT_TYPE);

extern Widget CreateFontSelector             (Widget, void(*)(), ...);
extern void   SetFontSelector                (Widget, String, String, String);
extern String GetFontSelectorValue           (Widget, SELECT_TYPE);

extern Widget CreateGridSelector             (Widget, SELECT_TYPE, ... );
extern Widget CreatePredefinedPointsSelector (Widget, SELECT_TYPE, ... );

extern Widget CreateTextSelector             (Widget, PANEL_ID, void(*)(), ...);
extern void   SetTextSelector                (Widget, String, String, String);

extern Widget CreateTargetTimeControl        (Widget, DATE_DISPLAY, void(*)(), ...);
extern void   TargetTimeAddCallback          (Widget, void(), XtPointer);
extern void   TargetTimeSetStrTime           (Widget, String, Boolean);
extern String TargetTimeGetStrTime           (Widget);
extern void   TargetTimeSetFormatType        (Widget, DATE_DISPLAY);

/* selector_timeWindow.c */
extern Widget CreateTimeWindowSelector (Widget, int, void (*)(TimeWindowSelectorStruct*), ...);
extern void   SetTimeWindowLimits      (Widget, String*, int, int, int);

#endif  /*SELECTOR_H*/
