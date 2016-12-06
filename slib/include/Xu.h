/*========================================================================*/
/*
*	File: Xu.h - the header file for the Xu library.
*
*	Modifications:
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

#ifndef _XULIB_H
#define _XULIB_H

#include <X11/Intrinsic.h>
#ifdef MACHINE_PCLINUX
#	include <X11/xpm.h>
#else
#	include <xpm.h>
#endif
#include <Xm/Xm.h>

/* For the special separator character recognized as part of the XuNdialogID resource */
#define XuDIALOG_ID_PART_SEPARATOR	"~"

/* Define the cursor type macros.  Needed by XuSetCursor() function.
*
*  NOTE: Any new cursor added to here must be reflected in the static
*        data section in XuCursors.c and the appropriate cursor header
*        added.
*/
typedef enum {	XuBUSY_CURSOR, XuWINDOW_OBSCURED_CURSOR, XuSTOP_CURSOR, XuFINGER_CURSOR,
				XuKNIFE_CURSOR, XuMAGNIFY_CURSOR, XuPAN_CURSOR, XuPENCIL_CURSOR,
				XuSAMPLE_CURSOR, XuVALUEMOD_CURSOR, XuDEFAULT_CURSOR, XuMENU_CURSOR
} XuCURSOR_TYPE;

/* Cursor information structure
*/
typedef struct {
	unsigned int  width, height;	/* cursor width and height */
	unsigned char *bits;			/* cursor definition */
	unsigned char *mask;			/* masking bits */
	unsigned int  x_hot, y_hot;		/* the cursor hot spot location */
	char          *fg, *bg;			/* foreground and background colour names */
} XuCursorDataStruct;

/* Used by the XuMakeActionRequest function to indicate the type of request
 */
typedef enum { XuYNC, XuYYAN } XuREQUEST_TYPE;

/* These are used as returns by the prompt dialogs.
*/
typedef enum {	XuNO_RETURN, XuNO, XuYES, XuYES_ALL, XuCANCEL } XuRETURN;

/* Used by XuRunProgram
*/
typedef enum {	XuRUN_NONE, XuRUN_STATUS, XuRUN_DATA, XuRUN_ENDED, XuRUN_ERROR } XuRUN_RETURN;

/* For the XuNretainGeometry resource
*/
typedef enum {	XuRETAIN_NONE, XuRETAIN_POSN_ONLY, XuRETAIN_SIZE_ONLY, XuRETAIN_ALL } XuRETAIN;

/* For XuSaveDialogProfile
 */
#define XuActiveProfile	NULL
typedef enum { XuPIN_NONE, XuPIN_INITIAL, XuPIN_ALWAYS, XuPIN_RELATIVE } XuPIN_STYLE;

/* Structure to set arrow attributes in the XuCreateArrowPixmap()
*  function. The items to set are indicated by an or'ed set of
*  defines given below and set into flags.
*/
typedef enum { 	XuARROW_UP, XuARROW_DOWN, XuARROW_RIGHT, XuARROW_LEFT } XuARROWDIR;

typedef struct {
	long       flags;			/* which items are being set */
	long       appearance;
	XuARROWDIR direction;		/* arrow direction */
	int        height;
	int        width;
	int        margin_height;
	int        margin_width;
	int        outline_width;
	Pixel      foreground;		/* foreground colour */
	Pixel      background;		/* background colour */
	Boolean    text_under;		/* used by XuSetButtonArrow */
} XuArrowAttributes;

/* Flags (or'ed together)
*/
#define XuARROW_APPEARANCE		(1L)
#define XuARROW_DIRECTION		(1L<<1)
#define XuARROW_HEIGHT			(1L<<2)
#define XuARROW_WIDTH			(1L<<3)
#define XuARROW_MARGIN_HEIGHT	(1L<<4)
#define XuARROW_MARGIN_WIDTH	(1L<<5)
#define XuARROW_OUTLINE_WIDTH   (1L<<6)
#define XuARROW_FOREGROUND		(1L<<7)
#define XuARROW_BACKGROUND		(1L<<8)
#define XuARROW_TEXT_UNDER		(1L<<9)
#define XuARROW_ALL             (~0L)

/* Appearance (or'ed together)
 */
#define XuARROW_PLAIN			(0)
#define XuARROW_BARBED			(1L)
#define XuARROW_BAR				(1L<<1)
#define XuARROW_STEM			(1L<<2)
#define XuARROW_OUTLINED		(1L<<3)


/* Define additional resource macros used with the XuCreate...Dialog
*  and XuVaApp... functions.
*/
#define XuNactionAreaItems				"actionAreaRow1Items"
#define XuNaskUserToggle				"askUserToggle"
#define XuNnumActionAreaItems			"numActionAreaRow1Items"
#define XuNactionAreaRow1Items			"actionAreaRow1Items"
#define XuNnumActionAreaRow1Items		"numActionAreaRow1Items"
#define XuNactionAreaRow2Items			"actionAreaRow2Items"
#define XuNnumActionAreaRow2Items		"numActionAreaRow2Items"
#define XuNactionAreaButtonMarginHeight	"actionAreaButtonMarginHeight"
#define XuNactionAreaButtonTightness	"actionAreaButtonTightness"
#define XuNactionAreaMarginHeight		"actionAreaMarginHeight"
#define XuNallowIconify					"allowIconify"
#define XuNallowProfileSelection        "allowProfileSelection"
#define XuNapplicationDepth				"applicationDepth"
#define XuNcolormapType					"colormapType"
#define XuNdefaultActionItemId			"defaultActionItemId"
#define XuNdestroyCallback				"xuDestroyCB"
#define XuNdestroyData					"xuDestroyData"
#define XuNdialogDisplay				"dialogDisplay"
#define XuNdialogID						"dialogID"
#define XuNfontPath						"fontPath"
#define XuNiconPixmapFile				"iconPixmapFile"
#define XuNiconMaskFile					"iconMaskFile"
#define XuNisApplicationSuiteColormap 	"isApplicationSuiteColormap"
#define XuNminDialogSize				"minDialogSize"
#define XuNmwmDeleteOverride            "mwmDeleteOverride"
#define XuNmwmDeleteData				"mwmDeleteData"
#define XuNncopyColors					"ncopyColors"
#define XuNselectProfileDialogName		"selectProfileDialogName"
#define XuNrelativePosition				"relativePosition"
#define XuNretainGeometry				"retainGeometry"
#define XuNstateFile					"stateFile"
#define XuNstateFileEditable			"stateFileEditable"
#define XuNusePrivateColormap 			"usePrivateColormap"
#define XuNuseXCursor					"useXCursor"
#define XuNvisualClass					"visualClass"
#define XuNvisualID 					"visualID"



/* Define structure used by the action area in the standard dialog functions.
*/
typedef struct _xu_dialog_actions {
	String    id;                    /* PushButton's resource name */
	void      (*callback)();         /* pointer to callback routine */
	XtPointer data;                  /* client data for callback routine */
} XuDialogActionsStruct;



/*===================== FOR MENU FUNCTIONS ========================*/


/*  The id field in this structure is stored as XmNuserData for the created
 *  menu item. Setting it to less than 1 will make MenuBuild ignore it. The
 *  NoId define is for convienience.
 */
#define NoId -1


/* Options for the menu item. These may be or'ed together to provide more
 * than one type of funcionality for the menu item. 
 */
#define XuMENU_NONE			(0)		/* for when we have nothing                 */
#define XuMENU_INSENSITIVE	(1L)	/* initialize the menu item to insensitive  */
#define XuMENU_SET			(1L<<1)	/* set the item on - must be of type toggle */
#define XuMENU_RADIO_LIST	(1L<<3)	/* menu is a radio list type                */
#define XuMENU_TEAR_OFF		(1L<<4)	/* we want the menu to be tear off          */


typedef struct _xu_menu_item{
	char                 *name;        /* menu item name		                */
	WidgetClass          *class;       /* menu item class		                */
	unsigned long        options;      /* XuMENU_... (see above)                */
	KeySym               mnemonic;     /* menu item mnemonic (pulldowns only)   */
	int                  id;           /* menu button id (must be > 0)          */
	XtCallbackProc       callback;     /* menu callback                         */
	XtPointer            client_data;  /* menu callback data                    */
	struct _xu_menu_item *subitems;    /* pullright or menu bar menu items      */
} XuMenuItemStruct, *XuMenuItem; 

typedef struct _xu_menu_bar_item {
	char          *name;               /* menu bar item name                     */
	KeySym        mnemonic;            /* menu item mnemonic                     */
	int           id;                  /* menu button id (must be > 0)           */
	XuMenuItem    subitems;            /* pulldown menu items                    */
} XuMenuBarItemStruct, *XuMenuBarItem;

/* Required for XuVaMenuBuildOption. XmNmanageChild is for compatability with the
 * XmVa functions and is here in case the XmVa header is not included.
 */
#define XuNmanageChild "manageChild"
#ifndef XmNmanageChild
#define XmNmanageChild	XuNmanageChild
#endif

/* The function XuGetActionAreaBtn() takes a XuMenuItemStruct element as the second
 * argument to get the button widget. All we need is a define to do this as the id
 * part of the structure will contain the widget name that we are interested in.
 */
#define XuGetActionAreaBtn(w,s)	XuGetActionAreaBtnByName(w, s.id)


/*===================== FUNCTION PROTOTYPES =======================*/

/* Public functions */
extern void         XuWidgetLabel                (Widget, String);
extern void         XuWidgetPrint                (Widget, String, ...);
extern Boolean      XuAllocColor                 (Display*, int, Colormap, XColor*, const Boolean);
extern Widget       XuAppInitialize              (XtAppContext*, String, XrmOptionDescRec*,
                                                    Cardinal, int*, String*, String*, ArgList, int);
extern XuRETURN     XuAskUser                    (Widget, String, ...);
extern String       XuAssignLabel                (String);
extern Widget       XuBuildMenu                  (Widget, long, String, XuMenuItemStruct*);
extern void         XuClearBusyCursor            (void);
extern void         XuClearButtonArrow           (Widget);
extern void         XuComboBoxAddItem            (Widget, String, int);
extern void         XuComboBoxAddItems           (Widget, String*, int, int);
extern void         XuComboBoxDeleteAllItems     (Widget);
extern int          XuComboBoxGetSelectedPos     (Widget);
extern int          XuComboBoxItemPos            (Widget, String);
extern void         XuComboBoxSelectItem         (Widget, String, const Boolean);
extern void         XuComboBoxSelectPos          (Widget, int, Boolean);
extern void         XuComboBoxSetString          (Widget, String);
extern void         XuConductSyncInteraction     (Widget*);
extern Widget       XuCreateApplicationShell     (XtAppContext*, String, XrmOptionDescRec*, Cardinal,
		                                            int*, String*, String*);
extern Pixmap       XuCreateArrowPixmap          (Widget, XuArrowAttributes*);
extern Pixmap       XuCreateColoredPixmap        (Widget, String, int, int);
extern Widget       XuCreateDialog               (Widget, WidgetClass, String, ...); 
extern Widget       XuCreateFormDialog           (Widget, String, ...);
extern Widget       XuCreateMainWindowDialog     (Widget refw, String id, ...);
extern Boolean      XuCreateProfile              (String);
extern Widget       XuCreateScrolledWindowDialog (Widget, String, ...);
extern Widget       XuCreatePopupShell           (String, WidgetClass, Widget, ArgList, int);
extern Widget       XuCreateToplevelDialog       (Widget, WidgetClass, String, ...); 
extern Widget       XuCreateToplevelFormDialog   (Widget, String, ...);
extern Widget       XuCreateToplevelScrolledWindowDialog (Widget, String, ...);
extern void         XuDelay                      (Widget, long);
extern void         XuDestroyApplication         (Widget);
extern void         XuDestroyDialog              (Widget);
extern void         XuDestroyDialogCB            (Widget, XtPointer, XtPointer);
extern void         XuFreePixmap                 (Widget, Pixmap);
extern void         XuDestroyProfile             (String);
extern void         XuExitOnDestroyCallback      (Widget, XtPointer, XtPointer);
extern XmFontList   XuExtendFontList             (XmFontList, String, String);
extern String       XuFindDirectory              (String);
extern String       XuFindFile                   (String);
extern String       XuFindKeyLine                (String, String, String);
extern void         XuFindNearestColor           (Display*, int, Colormap, XColor*, XColor*);
extern String       XuFindTypeFile               (String, String, String);
extern void         XuFree                       (void*);
extern Widget       XuGetActionAreaBtnByName     (Widget, String);
extern String       XuGetActiveProfile           (void);
extern Boolean      XuGetBooleanResource         (String, Boolean);
extern void         XuGetDialogGeometry          (String, int*, int*, int*, int*);
extern Pixmap       XuGetPixmap                  (Widget, String);
extern Pixmap       XuCreateInsensitivePixmap    (Widget, Pixmap);
extern int          XuGetIntResource             (String, int);
extern String       XuGetLabel                   (String);
extern String       XuGetLabelLc                 (String);
extern String       XuGetLabelUc                 (String);
extern String       XuGetLabelResource           (String, String);
extern String       XuGetMdbLine                 (String, String);
extern int          XuGetProfiles                (String**);
extern Boolean      XuGetProfileStateData        (String, Boolean*, String*);
extern Widget       XuGetShell                   (Widget);
extern Widget       XuGetShellChild              (Widget);
extern String       XuGetStringResource          (String, String);
extern XtPointer    XuGetSyncReturnValue         (void);
extern int          XuGetWidgetFontHeight        (Widget);
extern XFontStruct *XuGetWidgetFontStruct        (Widget);
extern XmString     XuGetXmLabel                 (String);
extern XmString     XuGetXmStringResource        (String, String);
extern void         XuHideDialog                 (Widget);
extern Boolean      XuIsActiveProfile            (String);
extern Boolean      XuIsProfiledDialog           (String);
extern Display     *XuIsValidDisplayString       (String);
extern void         XuListAddItem                (Widget, String);
extern void         XuListEmpty                  (Widget);
extern void         XuListLoad                   (Widget, String*, int, int);
extern void         XuListMakePosVisible         (Widget, int);
extern void         XuListSetToItem              (Widget, String);
extern Pixel        XuLoadColor                  (Widget, String);
extern Pixel        XuLoadColorResource          (Widget, String, String);
extern XuRETURN     XuMakeActionRequest          (Widget, XuREQUEST_TYPE, String, ...);
extern Widget       XuMenuAddButton              (Widget, String, String, int, XtCallbackProc,XtPointer);
extern Widget       XuMenuAddCascadeButton       (Widget, String, String, int, Widget);
extern Widget       XuMenuAddPixmapButton        (Widget, String, int, Pixmap, Pixmap, XtCallbackProc, XtPointer);
extern Widget       XuMenuAddSeparator           (Widget, unsigned char);
extern Widget       XuMenuAddShared              (Widget, String, KeySym, Widget);
extern Widget       XuMenuBuild                  (Widget, String, KeySym, XuMenuItem);
extern Widget       XuMenuBuildMenuBar           (Widget, String, XuMenuBarItem);
extern Widget       XuMenuBuildOption            (Widget, String, XuMenuItem, ArgList, int);
extern Widget       XuMenuBuildPopup             (Widget, String, XuMenuItem);
extern Widget       XuMenuBuildShared            (Widget, String, XuMenuItem);
extern int          XuMenuButtonGetId            (Widget);
extern void         XuMenuButtonSetSensitivity   (Widget, int, Boolean);
extern void         XuMenuButtonSetVisiblity     (Widget, int, Boolean);
extern void         XuMenuClear                  (Widget);
extern Widget       XuMenuFind                   (Widget, int);
extern Widget       XuMenuFindByName             (Widget, String);
extern Widget       XuMenuFindButton             (Widget, int);
extern Widget       XuMenuFindButtonByName       (Widget, String);
extern int          XuMenuGetSelected            (Widget);
extern void         XuMenuMakeToggle             (Widget);
extern void         XuMenuMakeTearOff            (Widget);
extern void         XuMenuMakeHelp               (Widget);
extern void         XuMenuSelectItem             (Widget, int);
extern void         XuMenuSelectItemByName       (Widget, String);
extern Boolean      XuMenuToggleGetState         (Widget, int);
extern void         XuMenuToggleSetState         (Widget, int, Boolean);
extern XmString     XuNewXmString                (String);
extern XmString     XuNewXmStringFmt             (String, ...);
extern void         XuPrintWidget                (Widget);
extern void         XuPrintWidgetAgain           (void);
extern void         XuProcessEventLoopWhile      (Widget, long, Boolean*);
extern void         XuSaveDialogProfile          (XuPIN_STYLE);
extern void         XuPutStringResource          (String, String);
extern Boolean      XuRunProgram                 (String, String*);
extern int          XuRunReceiveProgram          (String, String*);
extern Boolean      XuRunSendingProgram          (String, String*, void(*callfcn)(), XtPointer);
extern void         XuRunSendingConfig           (int, Boolean);
extern int          XuRunSendReceiveProgram      (String, String*, void(*callfcn)(), XtPointer);
extern void         XuSavePositionOfDialogs      (void);
extern void         XuSaveProfileStateData       (Boolean, String);
extern Boolean      XuSendToProgram              (int, String);
extern void         XuSetBusyCursor              (const Boolean);
extern void         XuSetButtonArrow             (Widget, XuArrowAttributes*);
extern void         XuSetCursor                  (Widget, XuCURSOR_TYPE, const Boolean);
extern void         XuSetDefaultCursor           (XuCursorDataStruct*);
extern void         XuSetDefaultMenuCursor       (XuCursorDataStruct*);
extern void         XuSetDialogCursor            (Widget, XuCURSOR_TYPE, const Boolean);
extern void         XuSetDialogCursorDirect      (Widget, Cursor);
extern void         XuSetProfile                 (String);
extern int          XuSetStateFile               (String, const Boolean);
extern void         XuSetStateStore              (int);
extern void         XuSetSyncReturnValue         (XtPointer);
extern void         XuShellAllowResize           (Widget, int);
extern void         XuShellCenter                (Widget);
extern void         XuShellRestrictResize        (Widget, int);
extern Boolean      XuShellVisible               (Widget);
extern void         XuRealizeApplication         (Widget);
extern void         XuShowDialog                 (Widget);
extern void         XuShowError                  (Widget, String, ...);
extern void         XuShowMessage                (Widget, String, ...);
extern Widget       XuShowNamedDialog            (String);
extern String       XuStandardDisplayString      (String);
extern Boolean      XuStateDataExists            (String, String, String);
extern Boolean      XuStateDataGet               (String, String, String, String*);
extern void         XuStateDataRemove            (String, String, String);
extern void         XuStateDataSave              (String, String, String, String);
extern void         XuToggleButtonSet            (Widget, const Boolean, const Boolean);
extern void         XuUpdateDisplay              (Widget);
extern int          XuWidgetToScreenNumber       (Widget);
extern void         XuXrmUpdate                  (String);
extern Widget       XuVaAppInitialize            (XtAppContext*, String, XrmOptionDescRec*,
                                                    Cardinal, int*, String*, String *, ...);
extern Widget       XuVaCreatePopupShell         (String, WidgetClass, Widget, ...);
extern Boolean      XuVaGetBooleanResource       (Boolean, String, ...);
extern int          XuVaGetIntResource           (int, String, ...);
extern String       XuVaGetStringResource        (String, String, ...);
extern XmString     XuVaGetXmStringResource      (String, String, ...);
extern Widget       XuVaMenuBuildOption          (Widget, String, XuMenuItem, ...);
extern Widget       XuVaMenuBuildSharedOption    (Widget, String, Widget, ...);
extern int          XuVaStateDataGet             (String, String, String, String, ...);
extern void         XuVaStateDataSave            (String, String, String, String, ...);

#endif /* _XULIB_H */
