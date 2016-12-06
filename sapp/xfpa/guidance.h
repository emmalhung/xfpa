/*========================================================================*/
/*
*	File:		guidance.h
*
*   Purpose:    Header file for all guidance related files.
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

#ifndef GUIDANCE_H
#define GUIDANCE_H

#include <FpaXgl.h>
#include "timelists.h"

#define GUID_NOT_DEFINED	'?'
#define GUID_NOT_AVAILABLE	'-'
#define GUID_DEPICT			'd'
#define GUID_CURRENT		'c'
#define GUID_PREVIOUS		'p'
#define GUID_ABSOLUTE		'a'


#define CURRENT_STRING		"current"
#define SHORT_CURR_STRING	"curr"
#define PREVIOUS_STRING		"previous"
#define SHORT_PREV_STRING	"prev"

#define SHOW_MINUTES(x)	(x->source->fd->sdef->minutes_rqd)

#define GUID_NO_SEL		-1

/* For those cases where the field list index and field index are encoded into
 * one number, this gives the amount to multiply the field index by for coding
 * purposes. Thus we encode <field index> * GUID_ENCODE_MULT + <field list index>
 */
#define GUID_ENCODE_MULT	200


typedef struct {				/* guidance field list element structure */
	char id[16];				/* field id as required by Ingred */
	int  id_key;				/* to use in constructing field id */
	FpaConfigFieldStruct *info; /* field information */
	struct _source *source;		/* field source */
	char sample_what_ndx;		/* index of last item type sampled */
	char rtype;					/* issue time type */
	char vtype;					/* valid time type */
	char dummy;					/* filler to align array boundary */
	RunTimeEntry run;			/* run time */
	ValidTimeEntry valid;		/* time structure pointer */
	int vsel;					/* selected valid time position */
	ColorIndex legend_colour;	/* colour to display the legend field label */
	ColorIndex legend_colour_default;/* legend field label colour as received from Ingred */
	Boolean diff;				/* is src, time-dep or run different from previous field? */
	Boolean show;				/* set the field to be shown */
	Boolean showing;			/* is the field now showing? */
	Boolean available;			/* is it available from the guidance database? */
	int retry_count;			/* used when doing availability lookup retry */
	int ndx;               		/* order number of the field in list */
	struct _GLS *list;			/* the list the field is in */
} GuidanceFieldStruct;

typedef struct _GLS {			/* guidance field list structure */
	String label;				/* list label */
	int id_key;                 /* to use in constructing field id */
	int ndx;               		/* index number of list */
	int nfield;					/* number of fields in the list */
	GuidanceFieldStruct **field;/* list of fields */
	Boolean showing;			/* True if at least one field is showing */
	Boolean fixed;				/* If True, list can not be modified */
} GuidlistStruct;

extern void    ACTIVATE_guidanceDialog                (Widget);
extern void    ACTIVATE_guidanceFieldSelectDialog     (Widget);
extern void    ACTIVATE_guidanceFieldAppearanceDialog (Widget);
extern void    ACTIVATE_guidanceFieldRemoveDialog     (Widget);
extern void    ACTIVATE_guidanceListAddItemDialog     (Widget, Boolean);
extern void    ACTIVATE_guidanceAvailabilityDialog    (Widget);
extern void    ACTIVATE_guidanceStatusDialog          (Widget);
extern void    ACTIVATE_processGuidanceUpdateDialog   (Widget);
extern void    ActivateGuidanceAnimateTab             (void);
extern void    ActivateGuidanceSampleTab              (void);
extern void    DeactivateGuidanceAnimateTab           (void);
extern void    DeactivateGuidanceSampleTab            (void);
extern void    ACTIVATE_guidanceLegendDialog          (Widget);
extern void    ACTIVATE_showGuidanceLegendDialog      (Widget);
extern void    DisplayGuidanceAtTime                  (String);
extern void    LayoutGuidanceAnimateTab               (void);
extern void    LayoutGuidanceSampleTab                (void);
extern Boolean DuplicateSourceDirectory               (FpaConfigSourceStruct*);
extern Boolean GetGuidanceDisplayState                (void);
extern FpaConfigFieldStruct *GetGuidanceSampleField   (void);
extern Boolean GuidanceListAddItem                    (String, Boolean);
extern Boolean GuidanceListAddField                   (GuidanceFieldStruct*, int);
extern void    GuidanceReset                          (void);
extern void    GuidanceLegendDialogSetValidTime       (String);
extern void    GuidanceListRemoveFields               (int*, int);
extern String  GuidFieldDateFormat                    (GuidanceFieldStruct*, String);
extern void    InitGuidanceLists                      (void);
extern void    InitSamplingPanel                      (void);
extern void    RestartGuidanceDialog                  (void);
extern void    SelectedFieldInfo                      (GuidanceFieldStruct*,Boolean,int*,int*);
extern void    SendGuidanceSampleCommand              (String);
extern void    SetGuidanceDialogSensitivity           (Boolean);
extern void    SetGuidanceDisplayState                (Boolean);
extern void    ShowGuidanceLegendCB                   (Widget, XtPointer, XtPointer);
extern void    StopGuidanceArrivedIndicator           (void);
extern void    UpdateAppearanceDialog                 (void);
extern Boolean GuidanceLegendDialogActivationState    (Boolean);
extern void    UpdateGuidanceSampleTab                (Boolean);
extern void    InitGuidanceAvailabilitySystem         (void);
extern void    InitGuidanceStatusSystem               (void);

extern void    LayoutGuidanceAnimationTab             (void);
extern void    ActivateGuidanceAnimationTab           (void);
extern void    DeactivateGuidanceAnimationTab         (void);

/* Define the globally available variables within the guidance subsystem.
*/
#ifdef GUIDANCE_MAIN
#    define GGVAR GLOBAL_INIT
#else
#    define GGVAR GLOBAL_EXTERN
#endif


GGVAR(Widget,           GVG_selectDialog,        NullWidget    );
GGVAR(Widget,           GVG_sampleTab,           NullWidget    );
GGVAR(Widget,           GVG_animateTab,          NullWidget    );
GGVAR(int,              GVG_nguidlist,           0             );
GGVAR(GuidlistStruct, **GVG_guidlist,            NULL          );
GGVAR(GuidlistStruct,  *GVG_active_guidlist,     NULL          );
GGVAR(TSTAMP,           GVG_active_time,         {'\0'}        );
GGVAR(int,              GVG_nvalid_seq,          0             );
GGVAR(String,          *GVG_valid_seq,           NULL          );
GGVAR(int,              GVG_nbtnBarBtns,         0             );
GGVAR(WidgetList,       GVG_btnBarBtns,          NullWidgetList);
GGVAR(Boolean,          GVG_option_full_display, True          );
GGVAR(Boolean,          GVG_option_synchro,      True          );
GGVAR(Boolean,          GVG_sample_tab_active ,  False         );


#endif /* GUIDANCE_H */
