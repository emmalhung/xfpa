/*========================================================================*/
/*
*	File:		depiction.h
*
*   Purpose:    Header for the ActiveDepictionTime() function and
*               the zoom functions.
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

#ifndef DEPICTION_H
#define DEPICTION_H

typedef enum { FIELD_DEPENDENT = 1, FIELD_INDEPENDENT } FIELD_DEPENDENCY;

/* T0 settings types */

#define T0_RESET					-1
#define T0_INITIALIZE				-2
#define T0_INITIALIZE_NEW_ONLY		-3
#define T0_TO_ACTIVE_DEPICTION		-4
#define T0_TO_SYSTEM_CLOCK			-5
#define T0_NEAREST_TO_SYSTEM_CLOCK	-6

extern void SetT0Depiction(int);


/* zoomControl.c function information */

typedef enum { ZOOM_CANCEL, ZOOM_IN, ZOOM_OUT, ZOOM_PAN, ZOOM_PAN_EXIT, ZOOM_EXIT } ZOOM_CMDS;

#define ZOOM_LINE_SIZE	5	/* Percent of the visible map window - must be int */

extern String ActiveDepictionTime		(FIELD_DEPENDENCY);
extern void   InitZoomFunctions         (void);
extern void   ZoomCommand				(ZOOM_CMDS);
extern void   ZoomStateCheck			(GE_CMD, String);
extern void   HorizontalZoomScrollBarCB	(Widget, XtPointer, XtPointer);
extern void   VerticalZoomScrollBarCB	(Widget, XtPointer, XtPointer);
extern void   ACTIVATE_zoomDialog       (Widget);
extern void   ResetDepictionBtnColour   (void);

#endif
