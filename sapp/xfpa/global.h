/*====================================================================*/
/*
 *	General header for the FPA interface program.
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
/*====================================================================*/

#ifndef _FPA_H
#define _FPA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <Xm/Xm.h>
#include <Xm/XmVaCreate.h>
#include <fpa.h>
#include <Xu.h>
#include "source.h"

/* Cast macros to avoid the compiler warning "cast from pointer to integer of different size"
 * when an integer is cast to XtPointer as in (XtPointer) ival and when a pointer is cast to
 * and integer. This normally only occurs in 64 bit compiles.
 */
#define INT2PTR(x)	(XtPointer)((long)x)
#define INT2STR(x)	(String)((long)x)
#define PTR2INT(x)	(int)((long)x)
#define PTR2CHAR(x)	(char)((long)x)
#define PTR2BOOL(x)	(Boolean)((long)x)

/* XRT widget tips group identifiers. This is done as a set of
 * defines so that we do not have to go searching for them.
 */
#define ICON_TIPS_GROUP		1
#define EDIT_ACTION_GROUP	2


#ifndef True
#	define True 1
#endif
#ifndef False
#	define False 0
#endif


/* Defines for compatability with Motif 1.2
*/
#ifndef XmSET
#	define XmSET   True
#	define XmUNSET False
#endif

/* If the default indicator size for toggle buttons in the menu bar
 * pulldowns is too small set this define for the size required
 */
/* #define INDICATOR_SIZE 12 */

/* The following group of ifndef statements is to maintain compatability
 * between any code that does not use the X header files and that which does.
 * This assumes that global.h is the last header file in the include set of
 * any particular function  (See <X11/Intrinsic.h>).
 */
#ifndef _XtIntrinsic_h
	typedef char Boolean;
	typedef char *String;
#endif

#ifndef XmINVALID_DIMENSION
#	define XmINVALID_DIMENSION 0xFFFF
#endif

#ifndef strcpy
#	define strcpy safe_strcpy
#endif
#ifndef strcat
#	define strcat safe_strcat
#endif

/* This one proves to be useful
*/
typedef unsigned char Byte;

#ifndef NullString
#define NullString		((String)0)
#endif
#define NullXmString	((XmString)0)
#define NullWidget		((Widget)0)
#define NullWidgetList	((WidgetList)0)

/* These are extensions of the Xt memory allocation functions.
*/
#define FreeList(var,len) \
{ \
	int ixtfree; \
	if(var) {\
	for (ixtfree=0; ixtfree<(len); ixtfree++) \
		XtFree((void*)((var)[ixtfree])); \
	XtFree((void*)var);} \
	var=NULL; \
}
#define XmStringArrayFree(xmstr,len) \
{ \
	int ixmstr; \
	if(xmstr) {\
	for (ixmstr=0; ixmstr<(len); ixmstr++) XmStringFree(xmstr[ixmstr]); \
	XtFree((void*)xmstr);} \
	xmstr=(XmString*)NULL; \
}
#define FreeItem(var)         {XtFree((void*)var);var=NULL;}
#define ZeroBuffer(b)         (void)memset((void*)b,0,(size_t)XtNumber(b)*sizeof(char))
#define BlankFill(v,n)        (void)memset((void*)v,' ',(size_t)(n))
#define CopyStruct(v1,v2,s,n) (void)memcpy((void*)(v1),(void*)(v2),(size_t)(n)*sizeof(s))

#define OneMem(a) \
	(a*)XtCalloc(1,(Cardinal)sizeof(a))
#define NewMem(a,n) \
	(((n)>0)?(a*)XtCalloc((Cardinal)(n),(Cardinal)sizeof(a)):NULL)
#define MoreMem(a,b,n) \
	(((n)>0)?(b*)XtRealloc((void*)a,(Cardinal)((n)*sizeof(b))):NULL)
#define NewWidgetArray(n) \
	(((n)>0)?(WidgetList)XtCalloc((Cardinal)(n),(Cardinal)sizeof(Widget)):NULL)
#define MoreWidgetArray(w,n) \
	(((n)>0)?(WidgetList)XtRealloc((void*)w,(Cardinal)((n)*sizeof(Widget))):NULL)
#define NewStringArray(n) \
	(((n)>0)?(String*)XtCalloc((Cardinal)(n),(Cardinal)sizeof(String)):NULL)
#define MoreStringArray(w,n) \
	(((n)>0)?(String*)XtRealloc((void*)w,(Cardinal)((n)*sizeof(String))):NULL)
#define NewXmStringArray(n) \
	(((n)>0)?(XmString*)XtCalloc((Cardinal)(n),(Cardinal)sizeof(XmString)):NULL)
#define MoreXmStringArray(w,n) \
	(((n)>0)?(XmString*)XtRealloc((void*)w,(Cardinal)((n)*sizeof(XmString))):NULL)
#define NewBooleanArray(n) \
	(((n)>0)?(Boolean*)XtCalloc((Cardinal)(n),(Cardinal)sizeof(Boolean)):NULL)
#define MoreBooleanArray(w,n) \
	(((n)>0)?(Boolean*)XtRealloc((void*)w,(Cardinal)((n)*sizeof(Boolean))):NULL)
#define NewIntArray(n) \
	(((n)>0)?(int*)XtCalloc((Cardinal)(n),(Cardinal)sizeof(int)):NULL)
#define MoreIntArray(w,n) \
	(((n)>0)?(int*)XtRealloc((void*)w,(Cardinal)((n)*sizeof(int))):NULL)
#define NewCalArray(n) \
	(((n)>0)?(CAL*)XtCalloc((Cardinal)(n),(Cardinal)sizeof(CAL)):NULL)
#define MoreCalArray(w,n) \
	(((n)>0)?(CAL*)XtRealloc((void*)w,(Cardinal)((n)*sizeof(CAL))):NULL)


/* This sets the width of the panels in the tabbed control area to the right of
*  the map area. This must not be resized and is set here.
*/
#define PANEL_WIDTH	180

																   
/* This defines the maximum number of pre-defined visibility selections available
*  in the pulldown menu under visibility. Note that the custom settings count as one
*  so that we actually can have only 23 settings from a file.
*/
#define MAXVIS   24

/* Miscellaneous
*/
#define ACTIVE      	"active"
#define BKGND_KEY   	"bgv"
#define EXIT			"EXIT"

/* Language strings
*/
#define ENGLISH "english"
#define FRENCH  "french"

/* Define extra logical terms.
*/
#define ON  True
#define OFF False
/*
 * Time bar location options
 */
#define TB_NONE		0
#define TB_TOP		1
#define TB_BOTTOM	2
/*
 * Field types as used in the FieldTypeID function found in utilities.c
 */
#define ContinuousFieldTypeID	"c"
#define VectorFieldTypeID		"v"
#define DiscreteFieldTypeID		"d"
#define WindFieldTypeID			"w"
#define LineFieldTypeID			"l"
#define ScatteredFieldTypeID	"s"
#define LinkChainFieldTypeID	"n"

/* This is required for the GEC() function for the type
*  of GE* function to use.
*/
typedef enum {
	GE_ACTION,
	GE_ANIMATE,
	GE_CONNECT,
	GE_DEPICTION,
	GE_EDIT,
	GE_GUIDANCE,
	GE_IMAGERY,
	GE_SEQUENCE,
	GE_SCRATCHPAD,
	GE_TIMELINK,
	GE_ZOOM
} GE_CMD;

/* Form keys
*/
typedef enum {
	DEFAULT_FORMAT,
	MINUTES,
	HOURS,
	DAYS,
	DAY_NR,
	SHORT_DAY_NAME,
	SHORT_DAY_NAME_NR_OF_MONTH,
	LONG_DAY_NAME,
	LONG_DAY_NAME_NR_OF_MONTH
} DATE_FORMAT;

typedef enum { EXACT, PREV, NEXT } DATE_MATCH;


/* Set fonts as defined in the resource file.
*/
#define NORMAL_FONT				XmFONTLIST_DEFAULT_TAG
#define ITALIC_FONT				"italic"
#define SMALL_FONT				"small"
#define SMALL_BOLD_FONT			"small_bold"
#define SMALL_ITALIC_FONT		"small_italic"
#define LARGE_FIXED_FONT		"large"
#define LARGE_BOLD_FIXED_FONT	"large_bold"
#define VERY_SMALL_FONT			"very_small"
#define SEQUENCE_HOUR_FONT		"hour_font"
#define SEQUENCE_MINUTE_FONT	"minute_font"

/* Selection keys for the SetSequenceBtnTime() function
*/
typedef enum { SPECIAL_SEQUENCE, MAIN_SEQUENCE } SEQUENCE_TYPE;

/* Define the strings used for extracting setup and configuration information.
*  The first group is for the directories block of the setup file. The second
*  is for the interface block.
*/

#define ALLIED          	"amodels.data"
#define AUTOIMPORT			"autoimport"
#define BACKUP				"backup"
#define CONFIG				"config"
#define DEPICT  			"depict"
#define FCST_RELEASE		"fcst.release"
#define FCST_WORK			"fcst.work"
#define FPA_PROG  			"fpa_prog"
#define HELP_SOURCE			"help.source"
#define HELP_VIEWER			"help.viewer"
#define INTERP  			"interp"
#define MAPS				"Maps"
#define MENUS_CFG			"menus.cfg"
#define MEMORY_CFG			"memory.cfg"
#define NWP  				"guidance"
#define POINT_FCST			"point_fcst"
#define PRESET_LISTS		"preset_lists"
#define WORKING_DIRECTORY	"scratch_files"

#define ALLIED_MODELS		"allied.model"
#define ANIM_DELAY			"animation.delay"
#define BULLET_NODE			"bullet.node"
#define DEMO_DATE			"demo.date"
#define DEPICT_COVIEW		"depiction.coview"
#define DEPICT_EXTERNAL		"depiction.external"
#define DEPICT_PRINT		"depiction.print"
#define DEPICT_SAVE			"depiction.savetime"
#define DEPICT_TIME_STEPS	"depiction.timeSteps"
#define FIELD_AUTOIMPORT	"field.autoimport"
#define FIELD_SMOOTHING		"field.smoothing"
#define GUID_TIME_STEPS		"guidance.animationTimeSteps"
#define IMAGERY_BRIGHTNESS	"imagery.brightness"
#define IMAGERY_BLEND		"imagery.blend"
#define IMAGERY_RING_SPACE	"imagery.radarRingSpacing"
#define INTERFACE			"interface"
#define INTERP_DELTA		"interpolation.delta"
#define MAP_BASE			"map.base"
#define MAP_EDITOR			"map.editor"
#define MAP_HOLES			"map.holes"
#define MAP_OVERLAY			"map.overlay"
#define MAP_PALETTE			"map.palette"
#define NWP_MODELS			"guidance.model"
#define PROD_GRAPHIC		"product.graphic"
#define PROD_LANGUAGES		"product.languages"
#define PROD_POINT			"product.point"
#define PROD_TEXT			"product.text"
#define PROD_TIMEZONES		"product.timezones"
#define TIMELINK_PALETTE	"link.palette"
#define TITLE				"title"

/* Obsolete but kept for backwards compatability
*/
#define DEPICT_FOREIGN	"depiction.foreign"     /* same as depiction.coview */


/* Define the predefined list file names.
*/
#define FIELD_VIS_LIST_FILE					"field_visibility"
#define GUIDANCE_LIST_FILE					"guidance"
#define POINTS_FILE							"point_lists"
#define AREA_SAMPLE_FILTERS_FILE			"area_sample_filters"
#define LABEL_SAMPLE_FILTERS_FILE			"label_sample_filters"
#define UPDATE_FIELDS_OFFICE_DEFAULT_FILE	"field_update_office_default"

/* Now the config/Forecasts file.
*/
#define FOG_AREAS			"areas"
#define FOG_EDITOR			"editor"
#define FOG_ELEMENT_ORDER	"element_order"
#define FOG_REQ_FIELDS		"required_fields"
#define FOG_SELECT_AREAS	"select_areas"

/* Define enumerated types for the panel identifiers.
*/
typedef enum {
	ANIMATION,
	CONNECT,
	ELEMENT_EDIT,
	GUIDANCE,
	GUIDANCE_STATUS,
	SCRATCHPAD,
	TIMELINK,
	INTERPOLATE,
	VIEWER,
	IMAGERY
} PANEL_ID;

/* Generic structure to hold label-key information as usually read
*  from the setup file.
*/
typedef struct _KeyInfo {
	String key;				/* item key as understood by processes */
	String label;			/* label that the user sees */
} KEYINFO;

/* Structure to hold setup information.
*/
typedef struct {
	int    nparms;		/* number of parameters associated with each entry line */
	String *parm;		/* the entry parameters */
} PARM;

typedef struct {
	String key_id;		/* key identifier from interface block of setup file */
	int nentry;			/* number of entry lines for this key */
	PARM *entry;		/* entry line parameters */
} SETUP;


/* State store keys for the sequence delta value */
#define SEQ_DELTA_STATE_KEYS	"main","seq","del"

/* Structure and data definitions for the depiction arrow sequence selector
 * increment values.
 */
typedef struct {
	XmString step_label;	/* what appears between the arrows */
	int      value;			/* time increment in minutes */
} SEQDELTA;


/* Structure to hold sample list information.
*/
typedef struct {
	String type;
	String name;
	String label;
	String sh_label;
} SampleListStruct;

/* Define field visibility values.
*/
#define VIS_ON			'y'
#define VIS_OFF			'n'
#define VIS_ALWAYS_ON	'a'

/* Sample information structure.
*/
typedef struct _sample {
	String process;      		/* how to sample NORMAL|GRID|LIST*/
	String item;         		/* what sort of thing is to be sampled */
	String type;				/* type of thing to be sampled */
	String colour;       		/* the colour to display the thing sampled in */
	String font_type;    		/* font to use when displaying sample string */
	String font_size;    		/* size of the font */
} SAMPLE;


/* Fields are placed into groups.
*/
typedef struct _group {
	String name;          		/* group name (as known to the system) */
	String label;         		/* group label (that which the user sees) */
	char visible;				/* visibility state of the group */
	char vis_state[MAXVIS];		/* used in field_stateDialog.c to store vis states */
	int nfield;           		/* number of fields */
	struct _fldinfo **field; 	/* pointers to field data structures */
	struct _fldinfo *afield;   	/* field currently active */
	int ntlfld;					/* number of time-linkable fields */
	struct _fldinfo **tlfld;	/* time-linkable field list */
	struct _fldinfo *atlfld;	/* time linked field currently active */
	int atlfldno;               /* list order number of active linkable field */
	int fields_link_status;    	/* field status rollup (lowest field status in group) */
} GROUP;

/* Define items associated with the fields.
*/
typedef struct _fldinfo {
	FpaConfigFieldStruct *info;	    /* field config info */
	Boolean manditory;              /* if true then the field must exist */
	Boolean exists;    				/* True if at least one of this field exists */
	char visible;               	/* visibility state of the field in its group */
	char vis_state[MAXVIS];			/* used in field_stateDialog.c to store vis states */
	Boolean animate_vis;			/* visibility state of field for animation purposes */
	int link_state;                 /* macro key for link state (see timelink.h) */
	int link_status;                /* macro key for link status (see timelink.h) */
	CAL cal;    					/* CAL data structure now active */
	CAL bkgnd_cal;    				/* background CAL data structure now active */
	struct _group *group;           /* pointer to the group it is in */
	struct _fldinfo *link_fld;      /* points to field if linked to another field */
	struct _editor_info *editor;    /* editor for this field */
	struct _sample sample;    		/* sample information structure */
	struct _memory *memory;         /* memory data pointer used by field panels */
	void (*entryFcn)(void);         /* call to activate the field editor */
	void (*showFcn)(Boolean);       /* call to show or hide field edit panel */
	void (*sendEditCmdFcn)(int);    /* call to send the current edit configuration */
	void (*changeEditFcn)(String);  /* call when edit function is changed */
	void (*depictFcn)(void);        /* call when changing depictions */
	void (*setBkgndFcn)(struct _fldinfo*, CAL);
									/* call to set the field background value */
	void (*destroyFcn)(struct _fldinfo*);
									/* call to destroy field */
} FIELD_INFO;


/* Structure for holding values set through the preferences dialog and used in other
 * parts of the program. A structure is used here to group all of the preferences
 * for code clarity and the initialization is done here as well go keep everything
 * together.
 */
struct PREFDATA {
	DATE_FORMAT daily_date_format;
	Boolean     restore_edit_state;
	Boolean     confirm_exit;				/* confirm exit with user */
	Boolean     ask_to_import_allied_models;
	Boolean     show_delete_all;			/* show button asking if user wants to delete all depictions */
	int         updating_flash_time;		/* max time to flash after source update finished */
	int         updating_flash_delay;		/* min time after forecaster checked status to flash again*/
};

#ifdef FPA_MAIN
	struct PREFDATA GV_pref =
		{
			SHORT_DAY_NAME_NR_OF_MONTH,
			False,
			False,
			False,
			False,
			10,
			15
		};
#else
	extern struct PREFDATA GV_pref;
#endif


/* Define the globally available variables.
*/
#ifdef FPA_MAIN
#    define GVAR GLOBAL_INIT
#else
#    define GVAR GLOBAL_EXTERN
#endif


GVAR(Widget, GW_animationPanel,          NullWidget);
GVAR(Widget, GW_animationScaleManager,	 NullWidget);
GVAR(Widget, GW_mainManager,             NullWidget);
GVAR(Widget, GW_iconButtonBar,           NullWidget);
GVAR(Widget, GW_drawingWindow,           NullWidget);
GVAR(Widget, GW_editorAcceptBtn,         NullWidget);
GVAR(Widget, GW_editorCancelBtn,         NullWidget);
GVAR(Widget, GW_editorUndoBtn,           NullWidget);
GVAR(Widget, GW_guidance,                NullWidget);
GVAR(Widget, GW_imageryPanel,            NullWidget);
GVAR(Widget, GW_interpolatePanel,        NullWidget);
GVAR(Widget, GW_mainMessageBar,          NullWidget);
GVAR(Widget, GW_mainWindow,              NullWidget);
GVAR(Widget, GW_mapWindow,               NullWidget);
GVAR(Widget, GW_menuBar,                 NullWidget);
GVAR(Widget, GW_tabFrame,                NullWidget);
GVAR(Widget, GW_tabFrame2,               NullWidget);
GVAR(Widget, GW_topLevel,                NullWidget);
GVAR(Widget, GW_depictMinTimeStepLabel,  NullWidget);
GVAR(Widget, GW_latLongDisplay,          NullWidget);	/* lat-long cursor position display */
GVAR(Widget, GW_scratchPanel,            NullWidget);
GVAR(Widget, GW_sequenceSelector,        NullWidget);
GVAR(Widget, GW_secSeqManager,           NullWidget);
GVAR(Widget, GW_secSeqLabel,             NullWidget);
GVAR(Widget, GW_secSeqBtns,              NullWidget);
GVAR(Widget, GW_timelinkPanel,           NullWidget);
GVAR(Widget, GW_connectPanel ,           NullWidget);


GVAR(int,          GV_animation_max_delay,  1000 );
GVAR(int,          GV_animation_loop_delay, 2000 );
GVAR(XtAppContext, GV_app_context,          NULL );
GVAR(String,       GV_app_name,             NULL );
GVAR(String,       GV_app_class,            NULL );
GVAR(String,       GV_working_directory,    NULL );  /* program working directory */
GVAR(TSTAMP,       GV_T0_depict,            ""   );  /* the T0 depiction time */
GVAR(Boolean,      GV_edit_mode,            True );  /* field edit mode? */
GVAR(int,          GV_nlanguages,           0    );  /* number of product languages */
GVAR(KEYINFO,     *GV_language,             NULL );  /* list of product languages */
GVAR(int,          GV_ntimezones,           0    );  /* number of product timezones */
GVAR(KEYINFO,     *GV_timezone,             NULL );  /* list of product timezones */
GVAR(int,          GV_ndepict,              0    );  /* number of depictions */
GVAR(String,      *GV_depict,               NULL );  /* depictions */
GVAR(int,          GV_ngroups,              0    );  /* number of groups */
GVAR(GROUP,      **GV_groups,               NULL );  /* pointers to group structures */
GVAR(GROUP,       *GV_active_group,         NULL );  /* pointer to active group */
GVAR(int,          GV_ntlgrps,              0    );  /* number of groups plus  master */
GVAR(GROUP,      **GV_tlgrps,               NULL );  /* full timelink group list */
GVAR(GROUP,       *GV_atlgrp,               NULL );  /* active timelink group */
GVAR(int,          GV_nfield,               0    );  /* total number of fields */
GVAR(FIELD_INFO, **GV_field,                NULL );  /* pointer to all fields */
GVAR(FIELD_INFO,  *GV_active_field,         NULL );  /* pointer to active field */
GVAR(int,          GV_selected_vis_setting, 0    );	 /* index into the vis_state array */
GVAR(String,       GV_stacking_order,       NULL );  /* area stacking order mode */
GVAR(int,          GV_interp_time_delta,    60   );  /* interval between interpolations */
GVAR(int,          GV_nseq_delta,           0    );  /* number of depiction step sequence delta values */
GVAR(SEQDELTA,    *GV_seq_delta,            NULL );  /* depiction step sequence delta values */
GVAR(int,          GV_increment_step,       0    );  /* current depiction increment step value */
GVAR(Boolean,      GV_inotify_process_used, False);  /* The inotify process is being used for source checking */

/*================== Function definitions =========================*/


/* panel_animation.c */
extern void CreateAnimationPanel(Widget);
extern Boolean AnimationStartup(void);
extern void AnimationStep(int, int);
extern void AnimationExit(String);

/* create_fieldsDialog.c */
extern void ACTIVATE_createFieldsDialog (Widget);

/* deleteFieldsDialog.c */
extern void ACTIVATE_deleteFieldsDialog (Widget);

/* depictModifiedCheck.c */
extern void StartDepictModifiedChecking (void);

/* depictionStatusDialog.c */
extern void ACTIVATE_depictionStatusDialog(Widget w);

/* depictions.c */
extern void    AllowLimitedDepictionSequence  (Boolean);
extern void    InitDepictionSequence          (Boolean);
extern void    InitToActiveDepiction          (void);
extern Boolean CreateDepiction                (String);
extern Boolean DepictionAtTime                (String);
extern void    MakeActiveDepiction            (String dt );
extern void    SaveDepiction                  (String cmd );
extern void    RemoveDepiction                (String cmd );
extern Boolean IsActiveDepiction              (String date_time );
extern void    SetActiveTimelinkDepiction     (String, Boolean);
extern void    DepictionStep                  (int, int);
extern Boolean HaveFieldToEdit                (void);
extern void    HideSecondarySequenceBar       (Boolean);
extern void    ResetDepictionSelection        (Boolean);
extern void    SecondaryBtnResetCB            (Widget, XtPointer, XtPointer);

/* field_autoImport.c */
extern void ACTIVATE_autoImportDialog (Widget);
extern void InitAutoImportSystem      (void);

/* field_updateDialog.c */
extern void ACTIVATE_updateFieldsDialog(Widget ref_widget );
extern void InitFieldUpdateSystem(void);

/* depiction_coview.c */
extern void CreateCoviewPulldown(int menu_id , String argv0 );

/* getDepictionRangeDialog.c */
extern void ACTIVATE_getDepictionRangeDialog(String cmd_buffer );

/* ingredCommand.c */
extern void    SendIngredCommands (Boolean state );
extern Boolean IngredCommand      (GE_CMD type , String cmd );
extern Boolean IngredVaCommand    (GE_CMD type, String fmt, ...);
extern Boolean IngredEditCommand  (String, CAL, CAL);
extern void    IngredVaEditCommand(CAL, CAL, String, ...);

/* panel_connect.c */
extern void CreateConnectPanel(Widget);
extern void ConnectStartup(void);
extern void ConnectExit(String);

/* panel_interpolate.c */
extern void CreateInterpolatePanel             (Widget parent);
extern void InterpolateStartup                 (void);
extern void InterpolateExit                    (void);

/* loadFieldsDialog.c */
extern void ACTIVATE_loadFieldsDialog(Widget parent );

/* main.c */
extern void    AllowIngredBusyCursor      (Boolean);
extern Boolean GetDisplayState            (PANEL_ID);
extern void    MainExitCB                 (Widget, XtPointer, XtPointer);
extern void    SetDepictionTimeDisplay    (String);
extern void    SetGUIComponentLocation    (int, int, int);
extern void    ShowMessages               (Boolean);
extern void    CheckTimeButtonLayout      (void);
extern void    SetDrawingWindowAttachment (void);

/* mapOverlayDialog.c */
extern void    InitMap                   (void);
extern void    ACTIVATE_mapOverlayDialog (Widget);

/* panel_control.c */
extern void    CreateTabPanels        (void);
extern Boolean SwitchPanel            (PANEL_ID panel );
extern void    DeactivateMenu         (void);
extern void    DeactivateMenuForZoom  (void);
extern void    ActivateMenu           (void);
extern Boolean PanelIsActive          (PANEL_ID panel );
extern void    ActivateMenu           (void);
extern Boolean PanelIsActive          (PANEL_ID panel );
extern void    SetDepictionVisibility (Boolean);
extern void    DeactivatePanels       (void);
extern void    ActivatePanels         (void);

/* printFunction.c */
extern void CreatePrintPulldown(int menu_id );

/* profile_manageDialog.c */
extern void ACTIVATE_manageProfileDialog(Widget);

/* profile_saveDialog.c */
extern void ACTIVATE_saveProfileDialog(Widget);

/* scratchpadPanel.c */
extern void CreateScratchpadPanel(Widget);
extern void ScratchpadStartup(void);
extern void ScratchpadExit(String);

/* time_fcns.c */
extern Boolean  InTimeList                (String, String*, int, int*);
extern void     InTimeListResolution      (DATE_FORMAT);
extern void     GetDateFormats            (String*, String*, String*);
extern int      HourDif                   (String, String);
extern int      MinuteDif                 (String, String);
extern String   DepictFieldDateFormat     (FIELD_INFO*, String);
extern String   TimeDiffFormat            (String, String, Boolean);
extern String   UnixSecToMinuteDateFormat (long);
extern void     AppDateTime               (String, int);
extern String   sysClockFmt               (void);
extern String   DateString                (String, DATE_FORMAT);
extern void     SetSequenceBtnTime        (Widget, String, SEQUENCE_TYPE);
extern XmString XmStringSequenceTime      (String, String, String);
extern String   TimeOffsetString          (String, String);
extern int      FilteredValidTimeList     (FLD_DESCRIPT*, int, String**);
extern int      FilteredValidTimeListFree (String**, int);


/* utilities.c */
extern String      AllocatedPrint               (String, va_list);
extern String      AllocPrint                   (String, ...);
extern void        CheckPresetListFiles         (void);
extern Widget      CreateXbaeMatrix             (Widget, String, ...);
extern void        DisplayAttributesPopup       (Widget, Boolean, CAL);
extern Boolean     EntryExists                  (int, String, Boolean);
extern Boolean     FieldInDepictSequence        (FpaConfigFieldStruct *field );
extern FIELD_INFO *FindField                    (String elem, String level );
extern GROUP      *FindFieldGroup               (String name );
extern Boolean     GetDepictionTimeFromOffset   (Source, String, DATE_MATCH, String*);
extern void        GetMapFontInfo               (String**, int*, String**, String**, int*);
extern SETUP      *GetSetup                     (String key );
extern String      GetSetupFile                 (int, String*);
extern PARM       *GetSetupKeyParms             (String block_key, String parm_key );
extern PARM       *GetSetupParms                (String key );
extern Boolean     HaveSetupEntry               (String);
extern String      FieldTypeID                  (FIELD_INFO*);
extern String      GraphicFileTypeByExtent      (String);
extern Boolean     HaveField                    (FIELD_INFO*, int, FpaConfigFieldStruct**, int*);
extern Boolean     InFieldList                  (FIELD_INFO*, int, String*, String*, int*);
extern Boolean     IsPointInsideMap             (float, float);
extern Boolean     InList                       (String, int, String*, int*);
extern Boolean     InterpFieldsAvailable        (String, String, Widget, int, String*);
extern void        InitFonts                    (void);
extern void        LogMsg                       (String key );
extern String      LineBreak                    (String, int, int);
extern void        MakeSampleItemList           (FLD_DESCRIPT*, SampleListStruct**, int*);
extern void        MakeTaperPixmaps             (Widget, int, int, String, Pixmap*, Pixmap*, Pixmap*);
extern void        Manage                       (Widget, Boolean);
extern void        Map                          (Widget, Boolean);
extern String      ParseTimeDeltaString         (String intime);
extern void        PushButtonPixmaps            (Widget, String, String, Pixmap*, Pixmap*);
extern String      ReadLine                     (FILE*);
extern void        SetReadLineComment           (String);
extern String      ReplaceKeyword			    (String, String, CAL, String);
extern String      NewStringReplaceKeyword      (String, String, CAL, String);
extern Boolean     RunProgramManager            (void (*rtn_fcn)(), XtPointer, ArgList, int);
extern void        GetSequenceBtnColor          (String, Pixel*, Pixel*);
extern void        SetFieldExistance            (FIELD_INFO *fptr );
extern String      SetupParm                    (SETUP *setup, int nrow, int ncol );
extern void        SetFieldBackground           (FIELD_INFO *field );
extern void        ShowFieldStatusLegendCB      (Widget, XtPointer, XtPointer);
extern String      TimeKeyReplace               (String, String);
extern void        ToggleButtonPixmaps          (Widget, String, String, Pixmap*, Pixmap*, Pixmap*);
extern void        Warning                      (String module, String key, ...);

#endif /* _FPA_H */
