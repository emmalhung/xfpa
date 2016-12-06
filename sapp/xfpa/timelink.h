/*========================================================================*/
/*
*	File:		timelink.h
*
*   Purpose:    Header file for the timelink macros.
*
*   Note:       The enumerated types NOT_LINKABLE to INTERPOLATED
*               must be in ascending order although it does not matter
*               what their individual values are and the LINKED_TO_
*               variables must have value < 0.
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

#ifndef _TIMELINK_H
#define _TIMELINK_H

/* Define the difference in size between the status indicator and the
*  associaged button in pixels and the margin width, in pixels, to go
*  around the arrow.
*/
#define INDICATOR_DIFF      10
#define INDICATOR_MARGIN    0

typedef enum {
	LINKED_TO_SELF = -4,	/* has its own links */
	LINKED_TO_MASTER,		/* is uses master links */
	LINKED_TO_FIELD,		/* uses links of another field */
	LINKED_TO_SPECIAL,		/* has special links (daily or static field) */
	NOT_LINKABLE,			/* cannot be linked */
	NOLINKS,				/* does not any links */
	SOME_LINKS,				/* has some links but not enough for interpolation */
	LINKED,					/* is completely linked */
	FIELD_INTERP,           /* field but not labels interpolated */
	INTERPOLATED			/* is linked and interpolated */
} LINK_STATUS;

extern void CreateTimelinkPanel(Widget parent);
extern void InitTimelinkPanel(void);
extern void SetFieldTimelinkState(FIELD_INFO *eptr );
extern void SetTimelinkCancel(Boolean);
extern Boolean TimelinkStartup(Boolean);
extern void TimelinkExit(String);
extern int LinkStatusStringToIndex(String status_string , Boolean is_master);
extern void LinkStatusStringToColour(String status_string , Pixel *fg, Pixel *bg);
extern void SetGroupLinkStatus(GROUP *grp );

#endif
