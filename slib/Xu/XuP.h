/*========================================================================*/
/*
*	File: XuP.h - the private header file for the Xu library.
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

#ifndef _XULIBP_H
#define _XULIBP_H

#include <stdio.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <fpa.h>
#include "Xu.h"


/* Cast macros to avoid the compiler warning "cast from pointer to integer of different size"
 * when an integer is cast to XtPointer as in (XtPointer) ival and when a pointer is cast to
 * and integer. Generally applies to 64 bit systems.
 */
#define INT2PTR(x)	(XtPointer)((long)x)
#define PTR2INT(x)	(int)((long)x)


/* Set the suffix used to create the database of prompt dialog information
*  and labels, errors and warnings that are specific to the application
*  using the Xu Library.  The name is made up of the application class name
*  plus this define (ie. XFpaMdb, XMapMdb, etc.).
*/
#define XuMESSAGE_DB_SUFFIX "Mdb"

/* Default state directory to be found in $HOME directory
*/
#define XURESDIR	".xures"

/* profile state file keys */
#define PROFILED_DIALOGS "PvDl"
#define PINSTATE         "PiN"

/* The X environment strings */
#define XAPPLRESDIR	 "XAPPLRESDIR"
#define XENVIRONMENT "XENVIRONMENT"

/* Key for use by toplevel in the state and profile files.
 */
#define TOP_LEVEL_ID  "ToPlEvEl"

/* For interning the atom */
#define WM_DELETE_WINDOW "WM_DELETE_WINDOW"


/* Dialog type enumerations.
 */
enum { STANDARD, TOPLEVEL };

/* I found that on RedHat Linus Enterprise version 3 the top level shell
 * would come back with a size one pixel larger than specified at start
 * time when the system did an exit. The following sets the minimum size
 * change that will be accepted as real.
 */
#define MIN_SIZE_CHANGE	2


/* Memory defines
 */
#define XTCALLOC(NUMBER,TYPE) (TYPE *)XtCalloc((Cardinal)(NUMBER),(Cardinal)(sizeof(TYPE)))
#define XTREALLOC(VAR,NUMBER,TYPE) (TYPE *)XtRealloc((void*)VAR,((Cardinal)(NUMBER)*(Cardinal)(sizeof(TYPE))))
#define XTFREELIST(var,len) \
{ \
	int ixtfree; \
	if(var) {\
		for (ixtfree=0; ixtfree<(len); ixtfree++) \
			XtFree((void*)((var)[ixtfree])); \
		XtFree((void*)var);} \
	var=NULL; \
}


/* Defines to make the code easier to read.
 */
#define NullWidget		(Widget)0;
#define RES             _xu_make_resource_id

/* Macro to find widget of core type.
*/
#define CW(w) (XtIsWidget(w)?w:XtParent(w))

/* The file CursorControl.c contains the cursor definitions. This define is the
*  number of cursors in that file. If this changes then this define must change.
*/
#define NCURSORS 12

/* The following structures define the only externally visible variable.
*/

/* Display information structure. The visual, colormap and depth need to be specified on
 * displays that support more than one depth and visual as the application shell may have
 * been told to use a specific one on the screen on which it was created.
 */
typedef struct {
	String      name;				/* full name of display wanted (example: kong:0.0) */
	Display     *display;			/* Display pointer */
	Visual      *visual;			/* Visual associated with this display */
	Colormap    cmap;				/* associated colormap */
	int         depth;				/* depth used */
	String      id;			        /* name without any non-alphanumeric characters (example: kong00) */
	int         width;				/* width of display screen */
	int         height;				/* height of display screen */
	Cursor      cursor[NCURSORS];	/* Cursors used in this display */
} DPYDATA, *DPYPTR;


/* resource information structure used during dialog construction
 */
typedef struct {
	Boolean        toplevel_type;
	DPYPTR         dpyinfo;
	String         dialog_id;
	int            decorations;
	int            functions;
	int            min_width;
	int            min_height;
	int            width_inc;
	int            height_inc;
	int            base_width;
	int            base_height;
	String         icon_file;
	String         mask_file;
	Boolean        destroy;
	Boolean        no_resize;
	Boolean        use_xu_cursor;
	Boolean        default_pos;
	Boolean        relative_pos;
	Boolean        is_kb_focus_policy;
	int            kb_focus_policy;
	int            resize_policy;
	XtCallbackProc CBFcn;
	XtPointer      CBFcn_data;
	XtCallbackProc delete_fcn;
	XtPointer      delete_data;
	int            ac;
	int            mode;
	XmString       dialogTitle;
	XuRETAIN       retain;
	int            num_actions[2];
	XuDialogActionsStruct *actions[2];
	XuDialogActionsStruct *default_action;
} ResInfoStruct;


/* Info structure associated with each state file */
typedef struct {
	String       fname;		/* path name of state file */
	String       profile;	/* profile associated with this file */
	Boolean      writable;	/* are we allowed to write to the file? */
	XtIntervalId id;		/* for use with timeout function call */
	int          nkd;		/* number of array elements in use */
	String       *key;		/* array of keys */
	size_t       *keylen;	/* array of key lengths */
	String       *data;		/* array of data associated with the keys */
} SFIS;


/* Dialog information structure */
typedef struct {
	String      name;			/* dialog name */
	Widget      dialog;			/* top level form (child of shell) */
	Widget      mainForm;		/* form where dialog elements are added */
	Widget      stopw;			/* widget on which to put a stop sign cursor in modal mode */
	struct {
		Widget w;				/* reference widget */
		int    x, y;			/* reference widget location */
	}           ref;			/* reference widget information */
	int         type;			/* TOPLEVEL or STANDARD */
	XtCallbackProc delete_fcn;	/* mwm close override function */
	XtPointer   delete_data;	/* data for the mwm overrride */
	Boolean     modal;			/* dialog modal state */
	Boolean     default_pos;	/* force the default position? */
	Boolean     relative_pos;	/* is the dialog to always be positioned relative to its parent? */
	Boolean     resize;			/* is there resize permission? */
	DPYPTR      dd;				/* display information */
	XuPIN_STYLE pin;			/* dialog pin type */
	int         x, y;			/* screen position */
	int         width, height;	/* dialog size */
	char        geometry[32];	/* dialog geometry as a string */
	Pixmap      icon_pixmap;	/* upper left icon and iconify image */
	Pixmap      icon_mask;		/* mask for the icon pixmap */
	struct {
	  int doit;					/* check the frame size? */
	  int x, y;					/* offsets */
	}           offset;			/* position offset required due to window manager frame */
} XuDStruct, *XuDSP;


/* Library variable structure */
typedef	struct {
	String         app_name;			/* application name */
	String         app_class;			/* application class */
	XtAppContext   app_context;			/* application context */
	Widget         top_level;			/* application top level shell */
	String         top_icon_file;		/* application icon pixmap file */
	String         top_mask_file;		/* mask for application icon pixmap */
	String         home_dir;			/* applicaiton home directory */
	FILE           *mdbfp;				/* pointer to messsage database file */
	int            ndd;					/* number of dd elements */
	DPYPTR         *dd;					/* display information */
	int            pixmap_cache_len;	/* how many entries in cache */
	struct _xu_pmc *pixmap_cache;		/* the pixmap information cache */
	int            bndx;				/* base state store file index */
	int            gndx;				/* general state store index */
	int            andx;				/* active state store index */
    int            nsf;					/* number of SFIS structures */
    SFIS           *sf;					/* state store data structures */
    int            dlalloc;				/* allocated dl pointers */
    int            ndl;					/* number of active dialogs */
    XuDStruct      **dl;				/* active dialog information */
    int            tightness;			/* action area tightness */
    int            margins;				/* action area margins */
	int            button_margins;		/* action area button margins */
	String         default_action_id;	/* item id of action item to be shown as default */
} XuControl;

/* Handy defined macros. Note that the display as opened by XuVaAppInitialize
 * is forced to reside in dd[0] and thus becomes the default display for this
 * library.
 */
#define Fxu                     _Xu_Control_
#define DefaultAppDisplayInfo	_Xu_Control_.dd[0]

#ifndef XULIBMAIN
extern XuControl Fxu;   /* Variable initialization done in GeneralFcns.c */
#endif

/* Internal library functions.
 */
extern void     _xu_check_frame_size              (XuDSP, unsigned long);
extern XuDSP    _xu_create_data_structure         (String, Widget, ResInfoStruct*);
extern XuDSP    _xu_dialog_data                   (Widget);
extern DPYPTR   _xu_find_display_info_from_widget (Widget);
extern void     _xu_free_colours                  (Widget);
extern void     _xu_get_minimum_size              (String, XuDSP, ResInfoStruct*);
extern String   _xu_make_resource_id              (String, String);
extern DPYPTR   _xu_open_display                  (String, XrmOptionDescRec*, Cardinal, String*, int*);
extern Boolean  _xu_in_comma_separated_list       (String, String);
extern int      _xu_parse_comma_separated_list    (String,String**);
extern void     _xu_parse_geometry_string         (String, int*, int*, int*, int*);
extern void     _xu_scan_resource_strings         (String, Widget, Boolean, ResInfoStruct*, ArgList*, int*, va_list);
extern void     _xu_state_data_save               (String, String, String, String);
extern void     _xu_set_geometry                  (XuDSP, int, int);
extern void     _xu_set_visual                    (Display*, int, int*, Visual**, Colormap*);
extern XmString _xu_xmstring_create               (Widget, String, String);

#endif /* _XULIBP_H */
