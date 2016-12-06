/*========================================================================*/
/*
*	File:		editor.h
*
*   Purpose:    Header file for edit related activities.
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

#ifndef _EDITOR_H
#define _EDITOR_H

/* I found this helps to make the code more understandable
 */
#define InEditMode(x)	same(GV_active_field->editor->active->cmd,x)

/* The editor panel widget can vary depending on how the program is
 * compiled and getting at it directly outside of panel_fieleEdit.c
 * the is not obvious. This define gets at the widget directly and
 * makes any changes to the names a single define change.
 */
#define EDITOR_PANEL XtNameToWidget(GW_tabFrame,"*.editorPanel")

/* State save keys. These are here so that they are all in one place.
 */
#define EDITOR_SAVE_STATE_KEY			"edssk"
#define DRAWING_MODE_STATE_KEY			"dmk"
#define SMOOTHING_MODE_STATE_KEY		"smk"
#define MODIFY_MODE_STATE_KEY			"mmk"
#define RADIUS_OF_INFLUENCE_STATE_KEY	"roik"
#define MOVE_MODE_STATE_KEY				"mmsk"

/* Define the available smoothing modes.
 */
#define DRAWING_MODES	"CONT","PPS"

/* menu entry attributes types. These can be or'ed
 * together to return combined states.
 */
#define ENTRY_ATTRIBUTES		(1)
#define MODIFY_ATTRIBUTES		(1<<1)
#define BACKGROUND_ATTRIBUTES	(1<<2)
#define NODE_ENTRY_ATTRIBUTES	(1<<3)

/* Define the edit key manager names.
*/
#define EM_POINT		"pointManager"
#define EM_LINE			"lineManager"
#define EM_DISCRETE		"discreteManager"
#define EM_VECTOR       "vectorManager"
#define EM_CONTINUOUS	"continuousManager"
#define EM_LCHAIN		"linkChainManager"

/* The editor keywords. Setting them as defines makes sure that
 * finger problems are miniminized.
 */
#define E_ADD            "ADD"
#define E_ADDING         "ADDING"
#define E_AREA           "AREA"
#define E_BREAK          "BREAK"
#define E_BUTTON         "BUTTON"
#define E_CANCEL         "CANCEL"
#define E_CLEAR          "CLEAR"
#define E_CHOOSE_CHAIN   "CHOOSE_CHAIN"
#define E_CONNECT        "CONNECT"
#define E_CONTINUOUS     "CONTINUOUS"
#define E_CONTOUR        "CONTOUR"
#define E_COPY           "COPY"
#define E_CREATE         "CREATE"
#define E_CUT            "CUT"
#define E_DELETE         "DELETE"
#define E_DELETED        "DELETED"
#define E_DELETE_HOLE    "DELETE_HOLE"
#define E_DESELECT       "DESELECT"
#define E_DIVIDE         "DIVIDE"
#define E_DRAG           "DRAG"
#define E_DRAW           "DRAW"
#define E_DRAWING        "DRAWING"
#define E_DRAW_DONE      "DRAW_DONE"
#define E_DRAW_HOLE      "DRAW_HOLE"
#define E_DRAW_OUTLINE   "DRAW_OUTLINE"
#define E_EDIT           "EDIT"
#define E_END_CHAIN      "END_CHAIN"
#define E_FIELD          "FIELD"
#define E_FLIP           "FLIP"
#define E_FLIP_REVERSE   "FLIP-REVERSE"
#define E_HOLE           "HOLE"
#define E_INTERPOLATE    "INTERPOLATE"
#define E_JOIN           "JOIN"
#define E_LABEL          "LABEL"
#define E_LINE           "LINE"
#define E_LCHAIN         "LCHAIN"
#define E_NEW_CHAIN      "NEW_CHAIN"
#define E_NODES          "NODES"
#define E_NODE_CAL       "NODE_CAL"
#define E_MENU           "MENU"
#define E_MERGE          "MERGE"
#define E_MODE           "MODE"
#define E_MODIFY         "MODIFY"
#define E_MODIFYING      "MODIFYING"
#define E_MODIFY_CONFIRM "MODIFY_CONFIRM"
#define E_MOVE           "MOVE"
#define E_NEW            "NEW"
#define E_NONE           "NONE"
#define E_OFF            "OFF"
#define E_ON             "ON"
#define E_OUTLINE        "OUTLINE"
#define E_PAN            "PAN"
#define E_PAN_DONE       "PAN_DONE"
#define E_PAN_MODE       "PAN_MODE"
#define E_PASTE          "PASTE"
#define E_PLACE          "PLACE"
#define E_POINT          "POINT"
#define E_POKE           "POKE"
#define E_PROCEED        "PROCEED"
#define E_PRESET_OUTLINE "PRESET_OUTLINE"
#define E_REJOIN         "REJOIN"
#define E_REMOVE         "REMOVE"
#define E_REVERSE        "REVERSE"
#define E_ROTATE         "ROTATE"
#define E_SAMPLE         "SAMPLE"
#define E_SELECT         "SELECT"
#define E_SELECT_ALL     "SELECT_ALL"
#define E_SET            "SET"
#define E_SHOW           "SHOW"
#define E_SMOOTH         "SMOOTH"
#define E_START          "START"
#define E_STATUS         "STATUS"
#define E_STOMP          "STOMP"
#define E_TRANSLATE      "TRANSLATE"
#define E_UNDO           "UNDO"
#define E_UPDATE         "UPDATE"
#define E_VECTOR         "VECTOR"
#define E_WIND           "WIND"
#define E_ZOOM           "ZOOM"


/* For the copy paste panel
 */
enum {
	NO_BUTTON_SELECT,
	ALL,
	SELECT_ALL_BUTTON_ON,
	SELECT_ALL_BUTTON_OFF,
	SELECT_ALL_BUTTON_ONLY,
	SHOW_COPY_PASTE_BUTTONS_ONLY
};


/* Define the enumerated types required by the editor.  Note that the first
*  14 entries are the editor types.  These must agree in order with the
*  editors as defined in field_edit.c or things will crash!
*/
enum {
	POINT_FIELD_NO_EDIT,
	POINT_FIELD_EDITOR,
	LINE_FIELD_NO_EDIT,
	LINE_FIELD_EDITOR,
	WIND_FIELD_NO_EDIT,
	WIND_FIELD_EDITOR,
	DISCRETE_FIELD_NO_EDIT,
	DISCRETE_FIELD_EDITOR,
	VECTOR_FIELD_NO_EDIT,
	VECTOR_FIELD_EDITOR,
	CONTINUOUS_FIELD_NO_EDIT,
	CONTINUOUS_FIELD_EDITOR,
	LCHAIN_FIELD_NO_EDIT,
	LCHAIN_FIELD_EDITOR,
	ACCEPT_MODE,
	SET_MODE,
	STACK_MODE
};

/* The maximum number of edit buttons. This allows for the edit functions to be
 * set in a data statement and makes for easier maintenance.
 */
#define NR_EDIT_BTNS 8


/*
 * Structure for drawing mode selection.
 */
typedef struct {
	String type;		/* key used to access state store info for this mode type */
	String draw;
	String modify;
	String smoothing;
} DRAWMODE;

/* Structure to hold edit button information.
*/
typedef struct _edit_btn {
	String contextBtnId;	/* resource reference for context button label */
	String cmd;             /* the edit command */
	String pixmapBaseName;  /* pixmap basename -> edit.<basename>.<type>.xpm */
	int    cursor;        	/* cursor to use when in drawing window */
	Widget w;               /* button widget */
} EDITBTN;

/* Structure to hold editor data.
*/
typedef struct _editor_info {
	int       type;                 /* field type editor is valid for */
	Boolean   stack_order_used;     /* Show the stacking order selector? */
	Boolean   spread_amount_used;   /* Show the spread amount selector? */
	Boolean   draw_modes_used;      /* Show the draw and smooth modes selectors? */
	DRAWMODE *draw_mode;            /* Draw mode selected for this editor */
	String    manager_name;         /* manager widget name */
	Widget    buttonManager;        /* manager for the edit function buttons */
	EDITBTN  *active;            	/* currently active edit command button */
	EDITBTN   btns[NR_EDIT_BTNS]; 	/* list of edit command buttons */
} EDITOR;

/* Structure to hold information pertaining to the use of CAL structures
*  in the various entry menus.
*/
typedef struct _cal_info {
	CAL     cal;				/* the CAL object */
	Boolean doing_mod_edit;		/* doing modification edit */
	Boolean replace_entry;		/* repalce existing entry */
} CALINFO;

/* attributeSampleDisplayDialog.c */
extern void ACTIVATE_attributeDisplayDialog    (String type, CAL cal);
extern void DEACTIVATE_attributeDisplayDialogs (void);
extern void ClearAttributeDisplayDialogs       (void);

/* dataEntryDialog_attributes.c */
#define POINT_ATTRIBUTES_DIALOG			"pointAttributesDialog"
#define LABEL_ATTRIBUTES_DIALOG			"labelAttributesDialog"
#define LINE_ATTRIBUTES_DIALOG			"lineAttributesDialog"
#define AREA_ATTRIBUTES_DIALOG			"areaAttributesDialog"
#define AREA_BKGND_ATTRIBUTES_DIALOG	"areaBkgndAttributesDialog"
#define LCHAIN_ATTRIBUTES_DIALOG		"linkChainAttributesDialog"
#define LNODE_ATTRIBUTES_DIALOG			"linkNodeAttributesDialog"

extern Boolean MenuFileExists                     (int, FIELD_INFO*);
extern void    ACTIVATE_pointAttributesDialog     (Widget, CAL, void(*)() );
extern void    ACTIVATE_labelAttributesDialog     (Widget, CAL, String, void(*)() );
extern void    ACTIVATE_lineAttributesDialog      (Widget, CAL, void(*)() );
extern void    ACTIVATE_areaAttributesDialog      (Widget, CAL, void(*)(), void(*)() );
extern void    ACTIVATE_areaBkgndAttributesDialog (Widget, CAL, void(*)() );
extern void    ACTIVATE_linkChainAttributesDialog (Widget, CAL, void(*)() );
extern void    ACTIVATE_linkNodeAttributesDialog  (Widget, CAL, void(*)() );
extern void    UpdateAttributesEntryDialog        (CAL);
extern void    DestroyAttributesEntryDialog       (void);

/* dataEntryDialog_weather.c */
extern void ACTIVATE_weatherEntryDialog (Widget, CAL, void (*)(), void (*)());

/* field_stateDialog.c */
extern void SetFieldVisibility               (FIELD_INFO*);
extern void SetGroupVisibility               (GROUP*);
extern void InitFieldDisplayState            (String);
extern void ACTIVATE_fieldDisplayStateDialog (Widget);

/* field_linkChain.c */
extern void AddLinkChainField			(FIELD_INFO*);
extern void CreateLinkChainFieldPanel	(Widget, Widget);

/* field_continuous.c */
extern void   AddContinuousField         (FIELD_INFO *field );
extern void   CreateContinuousFieldPanel (Widget parent, Widget top);

/* field_copyPaste.c */
extern void   ConfigureCopyPastePanel (void);
extern void   CreateCopyPasteSubpanel (Widget, Widget);
extern void   ShowCopyPasteSubpanel   (int);
extern void   HideCopyPasteSubpanel   (void);

/* field_vector.c */
extern void   AddVectorField         (FIELD_INFO *field );
extern void   CreateVectorFieldPanel (Widget parent, Widget top);

/* field_discrete.c */
extern void CreateDiscreteFieldPanel        (Widget parent ,Widget top);
extern void AddDiscreteField                (FIELD_INFO  *field );

/* In panel_fieldEdit.c */
extern void    AddField                        (FpaConfigFieldStruct*, const Boolean);
extern void    CreateEditControlPanel          (Widget);
extern void    CreateViewerModeControlPanel    (Widget);
extern void    FieldEditorExit                 (String);
extern void    FieldEditorStartup              (void);
extern EDITOR  *GetEditor                      (int);
extern Boolean IsActiveEditor                  (int);
extern EDITOR  *GetKey                         (int);
extern void    InitFields                      (void);
extern void    InitToActiveGroup               (void);
extern void    RemoveField                     (FIELD_INFO*);
extern void    ResetActiveField                (void);
extern void    SetActiveFieldByIndex           (int);
extern Boolean ValidEditField                  (void);


/* field_label.c */
extern void CreateLabelFieldPanel    (Widget, Widget);
extern void ConfigureLabelFieldPanel (void);
extern void HideLabelFieldPanel      (void);
extern void ShowLabelFieldPanel      (void);
extern void SendEditLabelCommand     (String);

/* field_line.c */
extern void AddLineField			(FIELD_INFO*);
extern void CreateLineFieldPanel	(Widget, Widget);


/* field_merge.c */
extern void CreateMergeFieldPanel    (Widget, Widget);
extern void ConfigureMergeFieldPanel (void);
extern void HideMergeFieldPanel      (void);
extern void ShowMergeFieldPanel      (void);

/* field_point.c */
extern void AddPointField			   (FIELD_INFO*);
extern void CreatePointFieldPanel	   (Widget, Widget);

/* field_sample.c */
extern void CreateSampleFieldPanel (Widget, Widget);
extern void SetFieldSampleInfo     (FIELD_INFO*, String, String);
extern void ConfigureSamplingPanel (void);
extern void HideSamplingPanel      (void);
extern void SampleDsplayControl    (String, CAL);
extern void ShowSamplingPanel      (void);
extern void SendEditSampleCommand  (String);

/* field_smoothing.c */
extern void CreateSmoothingFieldPanel (Widget, Widget);
extern void SetSmoothingValue         (float*);
extern void ShowSmoothingPanel        (void);
extern void HideSmoothingPanel        (void);

/* field_wind.c */
extern void CreateWindFieldPanel        (Widget parent , Widget top);
extern void AddWindField                (FIELD_INFO  *field );

#endif /* EDITOR_H */
