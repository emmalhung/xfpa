/*========================================================================*/
/*
*	File:		fcstText.h
*
*   Purpose:    Header file for fcstTextDialog.c
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

#ifndef _FCST_TEXT_H
#define _FCST_TEXT_H

#include <fpa.h>

/* Define the types of text forecast generators recognized.
*/
#define ID_FOG		"fog"
#define ID_TEXMET	"texmet"

/*
typedef enum {TYPE_FOG, TYPE_TEXMET};
*/

typedef enum {
    READY,
    GENERATING,
	AMENDING,
    UPDATING,
    PROCESSING,
    SPACER,
	HEADER
} TEXT_STATE;

typedef struct _fog_data {
	int type;
    String key;
    String label;
    int product_id;
    TEXT_STATE state;
    int order_no;
    String *element_order;
    Boolean *area_states;
} FOG_DATA, *FOG_DATA_PTR;

extern void ACTIVATE_fcstTextDialog			(Widget);
extern void ACTIVATE_fcstTextPriorityDialog	(Widget, int, FOG_DATA_PTR, FOG_DATA_PTR);
extern void ACTIVATE_fcstTextSetAreasDialog	(Widget, FOG_DATA_PTR);
extern void CreateBulletinDialog			(Widget, String, FOG_DATA_PTR, void(*)());
extern int  FcstAreaList					(FOG_DATA_PTR, String**, String**);
extern void InitFcstTextPriority			(FOG_DATA_PTR, INFOFILE);
extern void InitFcstTextAreas				(FOG_DATA_PTR, INFOFILE);
extern void InitFcstTextDialog				(void);
extern void WriteTextFcstInfoFile			(void);

#endif /* _FCST_TEXT_H */
