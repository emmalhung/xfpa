/****************************************************************************
*                                                                           *
*  File:        ingred.h                                                    *
*                                                                           *
*  Purpose:     Header file for FPA Graphics Editor Library (ingred.a)      *
*                                                                           *
*     Version 8 (c) Copyright 2011 Environment Canada                       *
*                                                                           *
*   This file is part of the Forecast Production Assistant (FPA).           *
*   The FPA is free software: you can redistribute it and/or modify it      *
*   under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation, either version 3 of the License, or       *
*   any later version.                                                      *
*                                                                           *
*   The FPA is distributed in the hope that it will be useful, but          *
*   WITHOUT ANY WARRANTY; without even the implied warranty of              *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
*   See the GNU General Public License for more details.                    *
*                                                                           *
*   You should have received a copy of the GNU General Public License       *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.        *
*                                                                           *
*****************************************************************************/

#ifndef INGREDDEFS
#define INGREDDEFS

#include <X11/Intrinsic.h>
#include <fpa.h>

/* These macros should be used when using the GEImagery function to specify the
 * display plane. Only the satellite and radar display planes can be blended.
 */
#define IMAGERY_DISPLAY_PLANE		"PLANE1"
#define GEOGRAPHIC_DISPLAY_PLANE	"PLANE0"

typedef enum
	{
		GE_INVALID,
		GE_VALID
	} GEREPLY;

GEREPLY GEConnect(XtAppContext, Widget,
					void (*)(STRING, LOGICAL),
					void (*)(STRING, STRING),
					void (*)(STRING, CAL));
GEREPLY GEDisconnect(void);
GEREPLY GEStatus(STRING, int *, STRING **, STRING **, CAL *);
GEREPLY	GEAction(STRING);
GEREPLY	GEZoom(STRING);
GEREPLY	GEAnimate(STRING);
GEREPLY	GEDepiction(STRING, CAL, CAL);
GEREPLY	GEGuidance(STRING);
GEREPLY	GEImagery(STRING);
GEREPLY	GESequence(STRING);
GEREPLY	GEScratchpad(STRING);
GEREPLY	GETimelink(STRING);
GEREPLY	GEEdit(STRING);

#endif
