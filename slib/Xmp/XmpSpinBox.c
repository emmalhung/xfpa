/************************************************************************************
 *
 *  XmpSpinBox
 *
 *  Description:
 *
 *  XmpSpinBox is a manager that allows the user to cycle through sets of choices.
 *
 *  The associated values are specified through constraint resources. The value types
 *  are specified through the XmNspinBoxType resource. This widget provides for a far
 *  greater range of types than does the standard Motif SpinBox widget, in addition to
 *  allowing for external specification of the input and display functions.
 *
 *  Resources:
 *
 *  ---------------------------------------------------------------------------------
 *  XmNarrowOrientation        | Either XmVERTICAL or XmHORIZONTAL. Default XmVERTICAL
 *  ---------------------------------------------------------------------------------
 *  XmNcolumns                 | Number of columns in the value display. Default 8.
 *  ---------------------------------------------------------------------------------
 *  XmNbuttonSizeRatio         | The ratio of the arrow button size to the text box.
 *  ---------------------------------------------------------------------------------
 *  XmNeditable                | Can the value be edited or not? Default True.
 *  ---------------------------------------------------------------------------------
 *  XmNincrement               | The amount to increment the value every time one of
 *                             | the arrows is selected. Default 1.
 *  ---------------------------------------------------------------------------------
 *  XmNincrementLarge          |
 *  ---------------------------------------------------------------------------------
 *  XmNincrementByMultiples    | When the increment arrows are used, should the value
 *                             | be scaled to the increment. For example for an
 *                             | increment of 25, the values would be forced to a 
 *                             | multiple of 25. Default False.
 *  ---------------------------------------------------------------------------------
 *  XmNmaximum                 | The maximum alowable value of the SpinBox.
 *  ---------------------------------------------------------------------------------
 *  XmNminimum                 | The minimum alowable value of the SpinBox.
 *  ---------------------------------------------------------------------------------
 *  XmNspinBoxStyle            | XmSPINBOX_LEFT          - Both increment arrows are
 *                             |                           horizontal on left side.
 *                             | XmSPINBOX_RIGHT         - Both increment arrows are
 *                             |                           horizontal on right side.
 *                             | XmSPINBOX_SEPARATE      - Arrows are on left and right
 *                             |                           sides.
 *                             | XmSPINBOX_STACKED       -
 *                             | XmSPINBOX_STACKED_LEFT  -
 *                             | XmSPINBOX_STACKED_RIGHT - 
 *                             |
 *                             | Default is XmSPINBOX_STACKED_RIGHT
 *  ---------------------------------------------------------------------------------
 *  XmNspinBoxType             | XmSPINBOX_BEACONCODE    - Octal
 *                             | XmSPINBOX_CLOCK_HM      - Time in hours and minutes
 *                             | XmSPINBOX_CLOCK_HMS     - Time hours, minutes and sec
 *                             | XmSPINBOX_DATE          - Date (see below)
 *                             | XmSPINBOX_DOLLARS       - American style dollars
 *                             | XmSPINBOX_NUMBER        - Number (see below)
 *                             | XmSPINBOX_SIGNED_NUMBER - Signed number (see below)
 *                             | XmSPINBOX_STRINGS       - String array (see below)
 *                             | XmSPINBOX_USER_DEFINED  - A user defined type where
 *                             |                           functions must be supplied
 *                             |                           (see below).
 *                             |
 *                             | Default is XmSPINBOX_NUMBER
 *  ---------------------------------------------------------------------------------
 *  XmNspinBoxUseClosestValue  | If set True, SpinBox will set the value to one of
 *                             | the limits if the value is out of range.
 *                             | Default False.
 *  ---------------------------------------------------------------------------------
 *  XmNupdateText              |
 *  ---------------------------------------------------------------------------------
 *  XmNvalue                   | The current value of the SpinBox. Default minimum.
 *  ---------------------------------------------------------------------------------
 *
 *  This resource is only valid for a SpinBoxType of XmSPINBOX_DATE
 *  ---------------------------------------------------------------------------------
 *  XmNdateFormat              | Format, as per strftime, for the display of dates.
 *  ---------------------------------------------------------------------------------
 *
 *  Only valid for a SpinBoxType of XmSPINBOX_NUMBER or XmSPINBOX_SIGNED_NUMBER
 *  ---------------------------------------------------------------------------------
 *  XmNdecimalPoints           | The number of decimal places to display.
 *  ---------------------------------------------------------------------------------
 *
 *  These resources are only valid for a SpinBoxType of XmSPINBOX_STRINGS
 *  ---------------------------------------------------------------------------------
 *  XmNitemCount               | The number of items in the string list.
 *  ---------------------------------------------------------------------------------
 *  XmNitems                   | The list of strings.
 *  ---------------------------------------------------------------------------------
 *  XmNitemsAreSorted          | Are the items in the string list sorted?
 *  ---------------------------------------------------------------------------------
 *
 *  These resources are only valid for a SpinBoxType of XmSPINBOX_USER_DEFINED
 *  ---------------------------------------------------------------------------------
 *  XmNgetValueData            |
 *  ---------------------------------------------------------------------------------
 *  XmNgetValueProc            |
 *  ---------------------------------------------------------------------------------
 *  XmNshowValueData           |
 *  ---------------------------------------------------------------------------------
 *  XmNshowValueProc           |
 *
 ************************************************************************************
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
 *
 ************************************************************************************/
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Xmu/Converters.h>
#include <Xm/TextF.h>
#include <Xm/ArrowB.h>
#include "XmpSpinBoxP.h"

 /* The spacing between the stacked buttons and the text field.
  * Someday this could be a resource.
  */
#define STACKED_ARROW_OFFSET	2


/*
 *  When user presses arrowbutton down, the value will click
 *  up/down XmNincrement.  If the arrowbutton is held down, after 
 *  INITIAL_DELAY milliseconds the values will start scrolling
 *  at the rate of once per SECOND_DELAY milliseconds,
 *  speeding up until we are scrolling at the rate of
 *  once per MINIMUM_DELAY seconds.  Every time we speed up,
 *  we cut DELAY_INCREMENT ms off of our time.
 */

#define INITIAL_DELAY	500
#define SECOND_DELAY	300
#define MINIMUM_DELAY	5
#define DELAY_DECREMENT	20

 /*
  *  Used by activate_spinbox_callback
  */
#define CB_NOSEND	0
#define CB_SEND		1
#define CB_FORCE	2

/*
 *  min, max, and incrementLarge values are set depending upon
 *  the SpinBox type UNLESS the user has specfied them by
 *  hand.  Since a value of 0 from the resource file (`by hand')
 *  looks no different from a 0 as an XtResource default, we
 *  intentionally make the XtResource default values strange
 *  so that we can see if the user's overridden them or not.
 *
 *  I can't think of a good way around this as long as the
 *  SpinBox types all reside in the same widget.
 */

#define STRANGE_DEFAULT -2323

/*
 *  Convenience macros, mainly used when determining
 *  the layout of the textfield and arrowbuttons.
 */
#define int2ptr(a) ((void*)((int)(a)))
#define min2(a,b) ((a)<(b)?(a):(b))
#define max2(a,b) ((a)>(b)?(a):(b))
#define min3(a,b,c) (min2(min2((a),(b)),(c)))
#define max3(a,b,c) (max2(max2((a),(b)),(c)))
#ifndef XtWidth
#define XtWidth(w)	((w)->core.width)
#endif
#ifndef XtHeight
#define XtHeight(w)	((w)->core.height)
#endif


/* Instance field access macros */

#define SpinBoxArrowOrientation(w)		(w->spinbox.arrow_orientation)
#define SpinBoxButtonSizeRatio(w)		(w->spinbox.button_size_ratio)
#define SpinBoxContext(w)				(w->spinbox.context)
#define SpinBoxDateFormat(w)			(w->spinbox.date_format)
#define SpinBoxEditable(w)				(w->spinbox.editable)
#define SpinBoxCycle(w)					(w->spinbox.cycle)
#define SpinBoxDecimalPoints(w)			(w->spinbox.decimal_points)
#define SpinBoxDelay(w)					(w->spinbox.delay_ms)
#define SpinBoxDownBtn(w)				(w->spinbox.down_btn)
#define SpinBoxGetValue(w)				(w->spinbox.get_value_proc)
#define SpinBoxGetValueData(w)			(w->spinbox.get_value_data)
#define SpinBoxIncrement(w)				(w->spinbox.increment)
#define SpinBoxIncLarge(w)				(w->spinbox.increment_large)
#define SpinBoxIncByMultiples(w)		(w->spinbox.increment_by_multiples)
#define SpinBoxInterval(w)				(w->spinbox.interval)
#define SpinBoxItems(w)					(w->spinbox.items)
#define SpinBoxItemCount(w)				(w->spinbox.item_count)
#define SpinBoxItemsAreSorted(w)		(w->spinbox.items_are_sorted)
#define SpinBoxMaxValue(w)				(w->spinbox.val_max)
#define SpinBoxMinValue(w)				(w->spinbox.val_min)
#define SpinBoxOldValue(w)				(w->spinbox.val_old)
#define SpinBoxSensitive(w)				(w->core.sensitive)
#define SpinBoxShowValue(w)				(w->spinbox.show_value_proc)
#define SpinBoxShowValueData(w)			(w->spinbox.show_value_data)
#define SpinBoxStyle(w)					(w->spinbox.spinbox_style)
#define SpinBoxTextUpdateConstantly(w)	(w->spinbox.text_update_constantly)
#define SpinBoxTF(w)					(w->spinbox.tf)
#define SpinBoxTFColumns(w)				(w->spinbox.tf_columns)
#define SpinBoxTimeoutUseIncLarge(w)	(w->spinbox.timeout_use_inc_large)
#define SpinBoxType(w)					(w->spinbox.spinbox_type)
#define SpinBoxUpBtn(w)					(w->spinbox.up_btn)
#define SpinBoxUseClosestValue(w)		(w->spinbox.use_closest_value)
#define SpinBoxValue(w)					(w->spinbox.val_now)
#define SpinBoxValueChangedCBL(w)		(w->spinbox.ValueChangedCBL)

/* Class field access macros */

#define	SpinBoxStyleId			(xmpSpinBoxClassRec.spinbox_class.spinbox_style_id)
#define	SpinBoxTypeId			(xmpSpinBoxClassRec.spinbox_class.spinbox_type_id)


/*
 *  the *_show_value functions for the various XmNspinBoxTypes.
 */

static xmpSpinBoxShowValueProc
	alpha_show_value,
	bc_show_value,
	clock_hm_show_value,
	clock_hms_show_value,
	date_show_value,
	decimal_show_value,
	decimal_show_signed_value,
	dollar_show_value,
	int_show_value,
	int_show_signed_value;

/*
 *  the *_get_value functions for the various XmNspinBoxTypes.
 */

static xmpSpinBoxGetValueProc
	alpha_get_value,
	bc_get_value,
	clock_hm_get_value,
	clock_hms_get_value,
	date_get_value,
	decimal_get_value,
	dollar_get_value,
	int_get_value;

/*
 *  callbacks to keep the values spinning when the arrowbuttons
 *  are pressed and held down.
 */

static void up_arm_cb		( Widget, XtPointer, XtPointer );
static void up_disarm_cb	( Widget, XtPointer, XtPointer );
static void down_arm_cb		( Widget, XtPointer, XtPointer );
static void down_disarm_cb	( Widget, XtPointer, XtPointer );

/*
 *  Actions
 */

static void Action_Set_Large		( Widget, XEvent*, String*, Cardinal* );
static void Action_Decrement_Small	( Widget, XEvent*, String*, Cardinal* );
static void Action_Decrement_Large	( Widget, XEvent*, String*, Cardinal* );
static void Action_Increment_Small	( Widget, XEvent*, String*, Cardinal* );
static void Action_Increment_Large	( Widget, XEvent*, String*, Cardinal* );

/*
 *  Miscellaneous Prototypes
 */
static Boolean did_user_specify	              ( const char *res, const ArgList, Cardinal );
static void    set_tf_cb	                  ( XmpSpinBoxWidget, Boolean is_on );
static void    get_text_field	              ( XmpSpinBoxWidget );
static void    set_text_field	              ( XmpSpinBoxWidget );
static Boolean validate_new_inc_large	      ( XmpSpinBoxWidget, int value );
static Boolean validate_new_minimum	          ( XmpSpinBoxWidget, int value );
static Boolean validate_new_maximum	          ( XmpSpinBoxWidget, int value );
static Boolean validate_new_delay	          ( XmpSpinBoxWidget, int value );
static Boolean validate_new_type	          ( XmpSpinBoxWidget, int value );
static Boolean validate_new_style	          ( XmpSpinBoxWidget, int value );
static Boolean validate_new_value	          ( XmpSpinBoxWidget, int *value );
static Boolean validate_new_button_size_ratio ( XmpSpinBoxWidget, int ratio );
static void    activate_spinbox_callback      ( XmpSpinBoxWidget, int, char, int );
static void    change_spinbox_value	          ( XmpSpinBoxWidget, int );
static void    set_getshow_procs	          ( XmpSpinBoxWidget );
static void    set_default_min	              ( XmpSpinBoxWidget );
static void    set_default_max	              ( XmpSpinBoxWidget );
static void    set_default_inc_large	      ( XmpSpinBoxWidget );
static void    set_default_button_ratio	      ( XmpSpinBoxWidget );
static void    set_arrow_orientation	      ( XmpSpinBoxWidget );
static void    set_sensitive	              ( XmpSpinBoxWidget, Boolean sensitive );

/*
 *  Manditory Widget Functions
 */

static void	   ClassInitialize ( void );
static void	   Initialize      ( Widget, Widget, ArgList, Cardinal* );
static Boolean SetValues	   ( Widget, Widget, Widget, ArgList, Cardinal* );
static void	   Size            ( Widget, Dimension*, Dimension* );
static void	   Resize          ( Widget );

/*
 *  Translation Table
 */

static XtTranslations Translations;

static String translations =
	"<Key>osfUp:		IncrementSmall()	\n\
	 <Key>osfDown:	DecrementSmall()	\n\
	 <Key>osfPageUp:	IncrementLarge()	\n\
	 <Key>osfPageDown:	DecrementLarge()";

static String up_translation_string =
	"<Btn3Down>:		SetLarge() Arm()	\n\
	 <Btn3Up>:		IncrementLarge() Disarm()";

static String down_translation_string =
	"<Btn3Down>:		SetLarge() Arm()	\n\
	 <Btn3Up>:		DecrementLarge() Disarm()";

/*
 *  Action Table
 */

static XtActionsRec actions[] = {
	{ "SetLarge",       Action_Set_Large       },
	{ "DecrementSmall", Action_Decrement_Small },
	{ "DecrementLarge", Action_Decrement_Large },
	{ "IncrementSmall", Action_Increment_Small },
	{ "IncrementLarge", Action_Increment_Large },
	{ NULL, NULL }
};

/*
 *  Resources List
 */
static XtResource resources[] = 
{
	 {
	    XmNarrowOrientation, XmCArrowOrientation,
	    XmROrientation, sizeof (unsigned char),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.arrow_orientation),
	    XtRImmediate, int2ptr(XmVERTICAL),
	 },
	 {
	    XmNcolumns, XmCColumns,
	    XmRShort, sizeof(short),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.tf_columns),
	    XtRImmediate, int2ptr(8)
	 },
	 {
	    XmNbuttonSizeRatio, XmCButtonSizeRatio,
	    XmRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.button_size_ratio),
	    XtRImmediate, int2ptr(5)
	 },
	 {
	    XmNdateFormat, XmCDateFormat,
	    XmRString, sizeof (String),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.date_format),
	    XtRImmediate, "%a, %b %d, %Y",
	 },
	 {
	    XmNdecimalPoints, XmCDecimalPoints,
	    XmRShort, sizeof (short),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.decimal_points),
	    XtRImmediate, int2ptr(0),
	 },
	 {
	    XmNeditable, XmCEditable,
	    XmRBoolean, sizeof(Boolean),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.editable),
	    XtRImmediate, int2ptr(1)
	 },
	 {
	    XmNgetValueData, XmCGetValueData,
	    XtRPointer, sizeof(XtPointer),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.get_value_data),
	    XtRImmediate, (XtPointer)0
	 },
	 {
	    XmNgetValueProc, XmCGetValueProc,
	    XmRGetValueProc, sizeof(xmpSpinBoxGetValueProc*),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.get_value_proc),
	    XtRImmediate, (XtPointer)int_get_value
	 },
	 {
	    XmNincrement, XmCIncrement,
	    XmRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.increment),
	    XtRImmediate, int2ptr(1)
	 },
	 {
	    XmNincrementLarge, XmCIncrementLarge,
	    XmRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.increment_large),
	    XtRImmediate, int2ptr(STRANGE_DEFAULT)
	 },
	 {
	    XmNincrementByMultiples, XmCIncrementByMultiples,
	    XmRBoolean, sizeof(Boolean),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.increment_by_multiples),
	    XtRImmediate, (XtPointer) False,
	 },
	 {
	    XmNitemCount, XmCItemCount,
	    XtRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.item_count),
	    XtRImmediate, int2ptr(0)
	 },
	 {
	    XmNitems, XmCItems,
	    XtRStringArray, sizeof(String*),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.items),
	    XtRImmediate, (XtPointer)0
	 },
	 {
	    XmNitemsAreSorted, XmCItemsAreSorted,
	    XmRBoolean, sizeof(Boolean),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.items_are_sorted),
	    XtRImmediate, int2ptr(0)
	 },
	 {
	    XmNmaximum, XmCMaximum,
	    XmRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.val_max),
	    XtRImmediate, int2ptr(STRANGE_DEFAULT)
	 },
	 {
	    XmNminimum, XmCMinimum,
	    XmRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.val_min),
	    XtRImmediate, int2ptr(STRANGE_DEFAULT)
	 },
	 {
	    XmNshowValueData, XmCShowValueData,
	    XtRPointer, sizeof(XtPointer),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.show_value_data),
	    XtRImmediate, (XtPointer)0
	 },
	 {
	    XmNshowValueProc, XmCShowValueProc,
	    XmRShowValueProc, sizeof(xmpSpinBoxShowValueProc*),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.show_value_proc),
	    XtRImmediate, (XtPointer)int_show_value
	 },
	 {
	    XmNspinBoxCycle, XmCSpinBoxCycle,
	    XmRBoolean, sizeof(Boolean),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.cycle),
	    XtRImmediate, int2ptr(0)
	 },
	 {
	    XmNspinBoxStyle, XmCSpinBoxStyle,
	    XmRSpinBoxStyle, sizeof (unsigned char),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.spinbox_style),
	    XtRImmediate, int2ptr(XmSPINBOX_STACKED_RIGHT),
	 },
	 {
	    XmNspinBoxType, XmCSpinBoxType,
	    XmRSpinBoxType, sizeof (unsigned char),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.spinbox_type),
	    XtRImmediate, int2ptr(XmSPINBOX_NUMBER),
	 },
	 {
	    XmNspinBoxUseClosestValue, XmCSpinBoxUseClosestValue,
	    XmRBoolean, sizeof(Boolean),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.use_closest_value),
	    XtRImmediate, int2ptr(0)
	 },
	 {
	    XmNupdateText, XmCUpdateText,
	    XmRBoolean, sizeof(Boolean),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.text_update_constantly),
	    XtRImmediate, int2ptr(False)
	 },
	 {
	    XmNvalue, XmCValue,
	    XmRInt, sizeof(int),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.val_now),
	    XtRImmediate, int2ptr(0)
	 },
	 {
	    XmNvalueChangedCallback, XmCCallback,
	    XmRCallback, sizeof (XtCallbackList),
	    XtOffsetOf(struct _XmpSpinBoxRec, spinbox.ValueChangedCBL),
	    XtRCallback, (XtPointer)0,
	 }
};


/*
 *  The Widget CLASS RECORD
 */

externaldef(xmSpinBoxclassrec) XmpSpinBoxClassRec xmpSpinBoxClassRec = 
{
  {	                                        /* core_class            */
	 (WidgetClass)&xmpGeometryClassRec,         /* superclass            */
	 "XmpSpinBox",                              /* class_name            */
	 sizeof(XmpSpinBoxRec),                     /* widget_size           */
	 ClassInitialize,                          /* class_initialize      */
	 NULL,                                     /* class_part_initialize */
	 FALSE,                                    /* class_inited          */
	 Initialize,                               /* initialize            */
	 NULL,                                     /* initialize_hook       */
	 XtInheritRealize,                         /* realize               */
	 NULL,                                     /* actions               */
	 0,                                        /* num_actions           */
	 resources,                                /* resources             */
	 XtNumber(resources),                      /* num_resources         */
	 NULLQUARK,                                /* xrm_class             */
	 TRUE,                                     /* compress_motion       */
	 XtExposeCompressMaximal,                  /* compress_exposure     */
	 TRUE,                                     /* compress_enterleave   */
	 FALSE,                                    /* visible_interest      */
	 NULL,                                     /* destroy               */
	 Resize,                                   /* resize                */
	 XtInheritExpose,                          /* expose                */
	 SetValues,                                /* set_values            */
	 NULL,                                     /* set_values_hook       */
	 XtInheritSetValuesAlmost,                 /* set_values_almost     */
	 NULL,                                     /* get_values_hook       */
	 NULL,                                     /* accept_focus          */
	 XtVersion,                                /* version               */
	 NULL,                                     /* callback_private      */
	 XtInheritTranslations,                    /* tm_table              */
	 XtInheritQueryGeometry,                   /* query_geometry        */
	 NULL,                                     /* display_accelerator   */
	 NULL,                                     /* extension             */
  },

  {	                                        /* composite_class       */
	 XtInheritGeometryManager,                 /* geometry_manager      */
	 XtInheritChangeManaged,                   /* change_managed        */
	 XtInheritInsertChild,                     /* insert_child          */
	 XtInheritDeleteChild,                     /* delete_child          */
	 NULL,                                     /* extension             */
  },

  {	                                        /* constraint_class      */
	 NULL,                                     /* resources             */   
	 0,                                        /* num_resources         */   
	 sizeof(XmpSpinBoxConstraintRec),           /* constraint_size       */
	 NULL,                                     /* constraint_initialize */
	 NULL,                                     /* destroy               */   
	 NULL,                                     /* set_values            */   
	 NULL,                                     /* extension             */
  },

  {	                                        /* manager class         */
	 XtInheritTranslations,                    /* translations          */
	 NULL,                                     /* syn_resources         */
	 0,                                        /* num_syn_resources     */
	 NULL,                                     /* syn_constraint_resources */
	 0,                                        /* num_syn_constraint_resources */
	 XmInheritParentProcess,                   /* parent_process        */
	 NULL,                                     /* extension             */    
  },

  {	                                        /* geometry class        */
	/* This caused a compile failure with a complaint that it was not
	   a constant value. Changed to ForgetGravity and all seems ok.
	 XmInheritBitGravity,
	*/
	 ForgetGravity,                      /* bit_gravity           */
	 XmInheritInitializePostHook,              /* initialize_post_hook  */
	 XmInheritSetValuesPostHook,               /* set_values_post_hook  */
	 XmInheritConstraintInitializePostHook,    /* constraint_initialize_post_hook*/
	 XmInheritConstraintSetValuesPostHook,     /* constraint_set_values_post_hook*/
	 Size,                                     /* size                  */
	 NULL,                                     /* extension             */    
  },

  {	                                        /* SpinBox class         */
	 0,                                        /* spinbox_type_id       */
	 0,                                        /* spinbox_style_id      */
	 NULL,                                     /* extension             */    
  },	     
};

externaldef(xmSpinBoxwidgetclass) WidgetClass xmpSpinBoxWidgetClass = (WidgetClass) &xmpSpinBoxClassRec;

static String SpinBoxTypeNames[] = {
	"spinbox_number",
	"spinbox_clock_hm",
	"spinbox_clock_hms",
	"spinbox_date",
	"spinbox_beaconcode",
	"spinbox_dollars",
	"spinbox_strings",
	"spinbox_userdefined"
};
static String SpinBoxStyleNames[] = {
	"spinbox_stacked",
	"spinbox_stacked_left",
	"spinbox_stacked_right",
	"spinbox_left",
	"spinbox_right",
	"spinbox_separate"
};


static void ClassInitialize ( void )
{
	SpinBoxStyleId = XmRepTypeRegister ( XmRSpinBoxStyle,
	   SpinBoxStyleNames, NULL,
	   XtNumber(SpinBoxStyleNames));

	SpinBoxTypeId = XmRepTypeRegister ( XmRSpinBoxType,
	   SpinBoxTypeNames, NULL,
	   XtNumber(SpinBoxTypeNames));

	Translations = XtParseTranslationTable ( translations );
}


static Boolean did_user_specify( const char *resource_name, const ArgList args, Cardinal qty )
{
	Cardinal i;
	for ( i=0; i<qty; i++ )
	   if ( !strcmp ( args[i].name, resource_name ) )
	      return True;
	return False;
}


static void Initialize( Widget request_w, Widget new_w, ArgList args, Cardinal* num_args )
{
	Widget w;
	char tmpstr[256];
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)new_w;

	SpinBoxOldValue(cw) = INT_MAX;

	SpinBoxContext(cw) = XtWidgetToApplicationContext(new_w);
	XtAppAddActions ( SpinBoxContext(cw), actions, XtNumber(actions) );

	(void) validate_new_type ( cw, SpinBoxType(cw) );
	set_getshow_procs ( cw );

	SpinBoxTimeoutUseIncLarge(cw) = False;
	/* 
	 * did_user_specify() is better for SetValues than for Initialize
	 * because it doesn't take resource file settings into account.
	 * However, we can still use it here if we're a little more careful.
	 */
	if ( !did_user_specify( XmNminimum, args, *num_args ) && SpinBoxMinValue(cw)==STRANGE_DEFAULT )
	   set_default_min ( cw );
	if ( !did_user_specify ( XmNmaximum, args, *num_args ) && SpinBoxMaxValue(cw)==STRANGE_DEFAULT )
	   set_default_max ( cw );
	if ( !did_user_specify ( XmNincrementLarge, args, *num_args ) && SpinBoxIncLarge(cw)==STRANGE_DEFAULT )
	   set_default_inc_large ( cw );
	if ( !did_user_specify ( XmNbuttonSizeRatio, args, *num_args ) ) /* ccc */
	   set_default_button_ratio ( cw );
	/*
	 *   Make sure the values are all valid
	 */
	(void) validate_new_minimum ( cw, SpinBoxMinValue(cw) );
	(void) validate_new_maximum ( cw, SpinBoxMaxValue(cw) );
	(void) validate_new_inc_large ( cw, SpinBoxIncLarge(cw) );
	(void) validate_new_style ( cw, SpinBoxStyle(cw) );
	/* 
	 *   TextField
	 */
	(*SpinBoxShowValue(cw))((Widget)cw, SpinBoxShowValueData(cw), SpinBoxValue(cw), tmpstr, sizeof(tmpstr));

	SpinBoxTF(cw) = XtVaCreateManagedWidget ( "tf", xmTextFieldWidgetClass, new_w,
		XmNvalue, tmpstr,
		XmNcolumns, SpinBoxTFColumns(cw),
		XmNeditable, SpinBoxEditable(cw),
		XmNcursorPositionVisible, SpinBoxEditable(cw),
		NULL );

	set_tf_cb ( cw, True );
	XtOverrideTranslations ( SpinBoxTF(cw), Translations );
	/* 
	 *   Up ArrowButton
	 */
	SpinBoxUpBtn(cw) = w = XtVaCreateManagedWidget( "up_ab", xmArrowButtonWidgetClass, new_w, NULL );
	XtOverrideTranslations ( w, XtParseTranslationTable ( up_translation_string ) );

	XtAddCallback ( w, XmNarmCallback,    up_arm_cb,    (XtPointer)cw );
	XtAddCallback ( w, XmNdisarmCallback, up_disarm_cb, (XtPointer)cw );
	/* 
	 *   Down ArrowButton
	 */
	SpinBoxDownBtn(cw) = w = XtVaCreateManagedWidget( "down_ab", xmArrowButtonWidgetClass, new_w, NULL );
	XtOverrideTranslations ( w, XtParseTranslationTable ( down_translation_string ) );

	XtAddCallback ( w, XmNarmCallback,    down_arm_cb,    (XtPointer)cw );
	XtAddCallback ( w, XmNdisarmCallback, down_disarm_cb, (XtPointer)cw );

	set_arrow_orientation ( cw );
	if ( SpinBoxSensitive(cw) )
	   set_sensitive ( cw, True );
	   
	XmpGeometryInitialize ( xmpSpinBoxWidgetClass, request_w, new_w, args, num_args );
}

static Boolean SetValues (Widget old_w,Widget request_w,Widget new_w,ArgList args,Cardinal* num_args)
{
	XmpSpinBoxWidget old = (XmpSpinBoxWidget)old_w;
	XmpSpinBoxWidget new = (XmpSpinBoxWidget)new_w;
	Boolean needs_set_text_field = False;
	Boolean reconfigure = False;
	/*
	 *  See if type has changed..
	 */
	if ( SpinBoxType(new) != SpinBoxType(old) )
	{
	   if ( !validate_new_type ( new, SpinBoxType(new) ) ) {
	      SpinBoxType(new) = SpinBoxType(old);
	  } else {
	      set_getshow_procs ( new );
	      if ( !did_user_specify ( XmNminimum, args, *num_args ) ) set_default_min ( new );
	      if ( !did_user_specify ( XmNmaximum, args, *num_args ) ) set_default_max ( new );
	      if ( !did_user_specify ( XmNincrementLarge, args, *num_args ) ) set_default_inc_large ( new );
	      (void) validate_new_minimum ( new, SpinBoxMinValue(new) );
	      (void) validate_new_maximum ( new, SpinBoxMaxValue(new) );
	      (void) validate_new_inc_large ( new, SpinBoxIncLarge(new) );
	      needs_set_text_field = True;
	   }
	}
	/*
	 *  See size ratio has changed...
	 */
	if ( SpinBoxButtonSizeRatio(new) != SpinBoxButtonSizeRatio(old) )
	{
	   if ( !validate_new_button_size_ratio ( new, SpinBoxButtonSizeRatio(new) ) )
	      SpinBoxButtonSizeRatio(new) = SpinBoxButtonSizeRatio(old);
	   else
	      reconfigure = True;
	}
	/*
	 *  See style has changed...
	 */
	if ( SpinBoxStyle(new) != SpinBoxStyle(old) )
	{
	   if ( !validate_new_style ( new, SpinBoxStyle(new) ) ) {
	      SpinBoxStyle(new) = SpinBoxStyle(old); 
	  } else {
	      if ( !did_user_specify ( XmNbuttonSizeRatio, args, *num_args ) ) set_default_button_ratio ( new );
	      needs_set_text_field = True;
	      reconfigure = True;
	   }
	}
	/*
	 *  See delay has changed...
	 */
	if ( SpinBoxDelay(new) != SpinBoxDelay(old) )
	   if ( !validate_new_delay ( new, SpinBoxDelay(new) ) )
	      SpinBoxDelay(new) = SpinBoxDelay(old);
	/*
	 *  See if sensitive has changed...
	 */
	if ( SpinBoxSensitive(new) != SpinBoxSensitive(old) )
	   set_sensitive ( new, SpinBoxSensitive(new) );
	/*
	 *  See if min value has changed...
	 */
	if ( SpinBoxMinValue(new) != SpinBoxMinValue(old) )
	{
	   if ( !validate_new_minimum ( new, SpinBoxMinValue(new) ) )
	   {
	      SpinBoxMinValue(new) = SpinBoxMinValue(old);
	      if ( !did_user_specify ( XmNincrementLarge, args, *num_args ) )
	         set_default_inc_large ( new );
	      (void) validate_new_inc_large ( new, SpinBoxIncLarge(new) );
	   }
	}
	/*
	 *  See if maximum has changed...
	 */
	if ( SpinBoxMaxValue(new) != SpinBoxMaxValue(old) )
	{
	   if ( !validate_new_maximum ( new, SpinBoxMaxValue(new) ) )
	   {
	      SpinBoxMaxValue(new) = SpinBoxMaxValue(old);
	      if ( !did_user_specify ( XmNincrementLarge, args, *num_args ) )
	         set_default_inc_large ( new );
	      (void) validate_new_inc_large ( new, SpinBoxIncLarge(new) );
	   }
	}
	/*
	 *  See if the item list and/or count has changed ...
	 */
	if ( SpinBoxType(new) == XmSPINBOX_STRINGS && SpinBoxItemCount(new) != SpinBoxItemCount(old) )
	{
		SpinBoxMaxValue(new) = SpinBoxItemCount(new)-1;
	}
	/*
	 *  See if value has changed...
	 */
	if ( SpinBoxValue(new) != SpinBoxValue(old) )
	{
	   /* XmpSpinBoxSetValue wants to do the work itself, so take a step backwards */
	   int value = SpinBoxValue(new);
	   SpinBoxValue(new) = SpinBoxValue(old);
	   XmpSpinBoxSetValue ( (Widget)new, value, True );
	}
	/*
	 *  See if columns has changed... (todo: send these to tf)
	 */
	if ( SpinBoxTFColumns(new) != SpinBoxTFColumns(old) ) {
	   SpinBoxTFColumns(new) = SpinBoxTFColumns(old);
	   XtVaSetValues ( SpinBoxTF(new), XmNcolumns, SpinBoxTFColumns(new), NULL );
	}
	/*
	 *  See if decimal places has changed...
	 */
	if ( SpinBoxDecimalPoints(new) != SpinBoxDecimalPoints(old) ) {
	   if ( !did_user_specify ( XmNincrementLarge, args, *num_args ) ) set_default_inc_large ( new );
	   (void) validate_new_inc_large ( new, SpinBoxIncLarge(new) );
	   set_getshow_procs ( new ); /* to set the get/display procs if type is spinbox_NUMBER */
	   needs_set_text_field = True;
	}
	/*
	 *  See if arrow orientation has changed...
	 */
	if ( SpinBoxArrowOrientation(new) != SpinBoxArrowOrientation(old) )
	{
	   set_arrow_orientation ( new );
	}   
	/*
	 *  See if edit permission has changed ...
	 */
	if ( SpinBoxEditable(new) != SpinBoxEditable(old) ) {
	   SpinBoxEditable(new) = SpinBoxEditable(old);
	   XtVaSetValues ( SpinBoxTF(new),
	  	XmNeditable, SpinBoxEditable(new),
		XmNcursorPositionVisible, SpinBoxEditable(new),
		NULL );
	}

	Reconfigure(new) = reconfigure;
	reconfigure |= XmpGeometrySetValues ( xmpSpinBoxWidgetClass,
	   old_w, request_w, new_w,
	   args, num_args );

	if ( needs_set_text_field )
	   set_text_field ( new );

	/* just in case XmpGeometrySetValues is constrained by a higher parent.. */
	if ( reconfigure )
	   Resize ( new_w );

	return ( reconfigure );
}


static void Size ( Widget w, Dimension* SpinBoxWidth, Dimension* SpinBoxHeight )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;
	const double ratio = SpinBoxButtonSizeRatio(cw);
	Dimension width=0, height=0;
	Dimension ubw=0, ubh=0;
	Dimension dbw=0, dbh=0;
	Dimension tfw=0, tfh=0;

	XmPreferredGeometry ( SpinBoxUpBtn(cw), &ubw, &ubh );
	XmPreferredGeometry ( SpinBoxDownBtn(cw), &dbw, &dbh );
	XmPreferredGeometry ( SpinBoxTF(cw), &tfw, &tfh );

	switch ( SpinBoxStyle(cw) )
	{
	   case XmSPINBOX_STACKED :
	      width = tfw;
	      height = (Dimension)(tfh * 11.0 / 5.0);
	      break;

	  /* RDP 2006.09.18 - The ratio should not apply to the stacked buttons */	 
	   case XmSPINBOX_STACKED_LEFT :
	   case XmSPINBOX_STACKED_RIGHT :
	      height = tfh;
		  width = tfw + ubw + STACKED_ARROW_OFFSET;
	      break;

	   case XmSPINBOX_LEFT :
	   case XmSPINBOX_RIGHT :
	   case XmSPINBOX_SEPARATE :
	   default :
	      height = tfh;
	      width = (Dimension)(tfw * (2 + ratio)/ratio);
	      break;
	}

	*SpinBoxWidth = max2 ( 1, width );
	*SpinBoxHeight = max2 ( 1, height );
}


static void Resize ( Widget w )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;
	const double ratio = SpinBoxButtonSizeRatio(cw);
	const Dimension width = XtWidth ( cw );
	const Dimension height = XtHeight ( cw );
	Dimension ubx=0, uby=0, ubw=0, ubh=0;
	Dimension dbx=0, dby=0, dbw=0, dbh=0;
	Dimension tfx=0, tfy=0, tfw=0, tfh=0;

	switch ( SpinBoxStyle(cw) )
	{
	   case XmSPINBOX_STACKED :
	      ubw = dbw = tfw = width;
	      tfh = (Dimension)(height * 5.0/11.0);
	      ubh = dbh = (Dimension)(height * 3.0/11.0);
	      ubx = uby = tfx = dbx = 0;
	      tfy = ubh;
	      dby = tfy + tfh;
	      break;

	   case XmSPINBOX_STACKED_LEFT :
	      tfh = height;
	      tfw = (Dimension)(ratio*width)/(ratio + 1);
	      ubh = dbh = height / 2;
	      ubw = dbw = ubh;
	      uby = tfy = ubx = dbx = 0;
	      tfx = ubw + STACKED_ARROW_OFFSET;
	      dby = ubh;
	      break;

	   case XmSPINBOX_STACKED_RIGHT :
	      tfh = height;
	      tfw = (Dimension)(ratio*width)/(ratio + 1);
	      ubh = dbh = height / 2;
	      ubw = dbw = ubh;
	      tfx = tfy = uby = 0;
	      ubx = dbx = tfw + STACKED_ARROW_OFFSET;
	      dby = ubh;
	      break;

	   case XmSPINBOX_LEFT :
	      tfh = ubh = dbh = height;
	      ubw = dbw = (Dimension)(width / (2 + ratio));
	      tfw = width - ubw - dbw;
	      uby = dby = dby = tfx = 0;
	      ubx = dbw;
	      tfx = ubx + ubw;
	      break;

	   case XmSPINBOX_RIGHT :
	      tfh = ubh = dbh = height;
	      ubw = dbw = (Dimension)(width / (2 + ratio));
	      tfw = width - ubw - dbw;
	      tfx = tfy = dby = uby = 0;
	      dbx = tfw;
	      ubx = dbx + dbw;
	      break;

	   case XmSPINBOX_SEPARATE :
	      tfh = ubh = dbh = height;
	      ubw = dbw = (Dimension)(width / (2 + ratio));
	      tfw = width - ubw - dbw;
	      tfy = dby = uby = dbx = 0;
	      tfx = dbw;
	      ubx = tfx + tfw;
	      break;
	}

	XmSetGeometry ( SpinBoxUpBtn(cw), ubx, uby, max2(ubw,1), max2(ubh,1), 0 );
	XmSetGeometry ( SpinBoxDownBtn(cw), dbx, dby, max2(dbw,1), max2(dbh,1), 0 );
	XmSetGeometry ( SpinBoxTF(cw), tfx, tfy, max2(tfw,1), max2(tfh,1), 0 );
}


/*
 *	Default get/set textfield functions
 */
static void set_text_field ( XmpSpinBoxWidget cw )
{
	char buffer[128];

	(*SpinBoxShowValue(cw))(
	   (Widget)cw,
	   SpinBoxShowValueData(cw),
	   SpinBoxValue(cw),
	   buffer, sizeof(buffer));

	XmTextFieldSetString ( SpinBoxTF(cw), buffer );
}

static void get_text_field ( XmpSpinBoxWidget cw )
{
	Boolean result=0;
	int val=0;
	char *pchar = XmTextFieldGetString(SpinBoxTF(cw));
	result = (*SpinBoxGetValue(cw))(
	   (Widget)cw, SpinBoxGetValueData(cw),
	   pchar, strlen(pchar),
	   &val );
	XtFree ( pchar );
	/*
	 *  If error, reset text field.
	 */
	if ( !result )
	   set_text_field ( cw );
	else if ( validate_new_value ( cw, &val ) ) 
	   XmpSpinBoxSetValue ( (Widget)cw, val, True );
}


/*
 *  TextField / ArrowButton Callbacks
 */
/*ARGSUSED*/
static void tf_valchange_cb ( Widget w, XtPointer client, XtPointer call )
{
	get_text_field ( (XmpSpinBoxWidget)client );
	set_text_field ( (XmpSpinBoxWidget)client );
}

static void set_tf_cb ( XmpSpinBoxWidget cw, Boolean is_on )
{
	Widget tf = SpinBoxTF(cw);

	/* we don't want error checking when arrowbuttons are on,
	   so this func can turn tf error checking on/off */
	if ( is_on ) {
	   if ( SpinBoxTextUpdateConstantly(cw) ) {
	      XtAddCallback ( tf, XmNvalueChangedCallback, tf_valchange_cb, (XtPointer)cw );
	   } else {
	      XtAddCallback ( tf, XmNactivateCallback, tf_valchange_cb, (XtPointer)cw );
	      XtAddCallback ( tf, XmNlosingFocusCallback, tf_valchange_cb, (XtPointer)cw );
	      XtAddCallback ( tf, XmNlosePrimaryCallback, tf_valchange_cb, (XtPointer)cw );
	   }
	} else {
	   if ( SpinBoxTextUpdateConstantly(cw) ) {
	      XtRemoveCallback ( tf, XmNvalueChangedCallback, tf_valchange_cb, (XtPointer)cw );
	   } else {
	      XtRemoveCallback ( tf, XmNactivateCallback, tf_valchange_cb, (XtPointer)cw );
	      XtRemoveCallback ( tf, XmNlosingFocusCallback, tf_valchange_cb, (XtPointer)cw );
	      XtRemoveCallback ( tf, XmNlosePrimaryCallback, tf_valchange_cb, (XtPointer)cw );
	   }
	}
}

/*ARGSUSED*/
static void up_armloop_cb ( XtPointer client, XtIntervalId *pId )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)client;
	int inc = SpinBoxTimeoutUseIncLarge(cw) ? SpinBoxIncLarge(cw) : SpinBoxIncrement(cw);
	change_spinbox_value ( cw, SpinBoxValue(cw) + inc );
	if ( SpinBoxDelay(cw)==INITIAL_DELAY ) SpinBoxDelay(cw) = SECOND_DELAY;
	else if ( SpinBoxDelay(cw)-DELAY_DECREMENT >= MINIMUM_DELAY )
	   SpinBoxDelay(cw) -= DELAY_DECREMENT;
	SpinBoxInterval(cw) = XtAppAddTimeOut ( SpinBoxContext(cw), SpinBoxDelay(cw), up_armloop_cb, client );
}

/*ARGSUSED*/
static void up_arm_cb ( Widget w, XtPointer client, XtPointer call )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)client;
	SpinBoxDelay(cw) = INITIAL_DELAY;
	set_tf_cb ( cw, False );
	if(SpinBoxIncByMultiples(cw)) {
		int val, inc = SpinBoxTimeoutUseIncLarge(cw) ? SpinBoxIncLarge(cw) : SpinBoxIncrement(cw);
		val = ((int) floor((double)(SpinBoxValue(cw) + inc)/(double)inc)) * inc;
		change_spinbox_value(cw, val);
	}
	SpinBoxInterval(cw) = XtAppAddTimeOut ( SpinBoxContext(cw), SpinBoxDelay(cw), up_armloop_cb, client );
}

/*ARGSUSED*/
static void up_disarm_cb ( Widget w, XtPointer client, XtPointer call )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)client;
	int inc = SpinBoxTimeoutUseIncLarge(cw) ? SpinBoxIncLarge(cw) : SpinBoxIncrement(cw);
	XtRemoveTimeOut ( SpinBoxInterval(cw) );
	SpinBoxTimeoutUseIncLarge(cw) = False;
	/* 2007.02.21
	* If we are at the limit already force the callback. Note that this check allows for the increment
	* value to be negative (the up arrow goes to larger negative values). Note that depending on the
	* increment and the limits, the displayed value may not be the same as the min or max values.
	*/
	if(inc > 0 && (SpinBoxValue(cw) > (SpinBoxMaxValue(cw)-inc)) || (inc < 0 && SpinBoxValue(cw) < (SpinBoxMinValue(cw)-inc)))
		activate_spinbox_callback ( cw, SpinBoxValue(cw), CB_FORCE, XmCR_OK );
	else
		XmpSpinBoxSetValue ( (Widget)cw, SpinBoxValue(cw) + inc, True );
	set_tf_cb ( cw, True );
}

/*ARGSUSED*/
static void down_armloop_cb ( XtPointer client, XtIntervalId *pId )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)client;
	int inc = SpinBoxTimeoutUseIncLarge(cw) ? SpinBoxIncLarge(cw) : SpinBoxIncrement(cw);
	change_spinbox_value ( cw, SpinBoxValue(cw) - inc );
	if ( SpinBoxDelay(cw)==INITIAL_DELAY ) SpinBoxDelay(cw) = SECOND_DELAY;
	else if ( SpinBoxDelay(cw)-DELAY_DECREMENT >= MINIMUM_DELAY )
	   SpinBoxDelay(cw) -= DELAY_DECREMENT;
	SpinBoxInterval(cw) = XtAppAddTimeOut ( SpinBoxContext(cw), SpinBoxDelay(cw), down_armloop_cb, client );
}

/*ARGSUSED*/
static void down_arm_cb ( Widget w, XtPointer client, XtPointer call )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)client;
	SpinBoxDelay(cw) = INITIAL_DELAY;
	set_tf_cb ( cw, False );
	if(SpinBoxIncByMultiples(cw)) {
		int val, inc = SpinBoxTimeoutUseIncLarge(cw) ? SpinBoxIncLarge(cw) : SpinBoxIncrement(cw);
		val = ((int) ceil((double)(SpinBoxValue(cw) - inc)/(double)inc)) * inc;
		change_spinbox_value(cw, val);
	}
	SpinBoxInterval(cw) = XtAppAddTimeOut ( SpinBoxContext(cw), SpinBoxDelay(cw), down_armloop_cb, client );
}

/*ARGSUSED*/
static void down_disarm_cb ( Widget w, XtPointer client, XtPointer call )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)client;
	int inc = SpinBoxTimeoutUseIncLarge(cw) ? SpinBoxIncLarge(cw) : SpinBoxIncrement(cw);
	XtRemoveTimeOut ( SpinBoxInterval(cw) );
	SpinBoxTimeoutUseIncLarge(cw) = False;
	/* 2007.02.21
	* If we are at the limit already force the callback. Note that this check allows for the increment
	* value to be negative (the up arrow goes to larger negative values) Note that depending on the
	* increment and the limits, the displayed limit value may not be the same as the min or max values.
	*/
	if((inc > 0 && SpinBoxValue(cw) < (SpinBoxMinValue(cw)+inc)) || (inc < 0 && SpinBoxValue(cw) > (SpinBoxMaxValue(cw)+inc)))
		activate_spinbox_callback ( cw, SpinBoxValue(cw), CB_FORCE, XmCR_OK );
	else
		XmpSpinBoxSetValue ( (Widget)cw, SpinBoxValue(cw) - inc, True );
	set_tf_cb ( cw, True );
}

/*
 *	SET routines...
 */
static void set_getshow_procs ( XmpSpinBoxWidget cw )
{
	switch ( SpinBoxType(cw) ) {

	   case XmSPINBOX_CLOCK_HM :
	      SpinBoxShowValue(cw) = clock_hm_show_value;
	      SpinBoxGetValue(cw) = clock_hm_get_value;
	      break;

	   case XmSPINBOX_CLOCK_HMS :
	      SpinBoxShowValue(cw) = clock_hms_show_value;
	      SpinBoxGetValue(cw) = clock_hms_get_value;
	      break;

	   case XmSPINBOX_DATE :
	      SpinBoxShowValue(cw) = date_show_value;
	      SpinBoxGetValue(cw) = date_get_value;
	      break;

	   case XmSPINBOX_BEACONCODE :
	      SpinBoxShowValue(cw) = bc_show_value;
	      SpinBoxGetValue(cw) = bc_get_value;
	      break;

	   case XmSPINBOX_DOLLARS :
	      SpinBoxShowValue(cw) = dollar_show_value;
	      SpinBoxGetValue(cw) = dollar_get_value;
	      break;

	   case XmSPINBOX_STRINGS :
	      SpinBoxShowValue(cw) = alpha_show_value;
	      SpinBoxGetValue(cw) = alpha_get_value;
	      break;

	   case XmSPINBOX_USER_DEFINED :
	      break;

	   case XmSPINBOX_NUMBER :
	      if ( SpinBoxDecimalPoints(cw) ) {
	         SpinBoxShowValue(cw) = decimal_show_value;
	         SpinBoxGetValue(cw) = decimal_get_value;
	      } else {
	         SpinBoxShowValue(cw) = int_show_value;
	         SpinBoxGetValue(cw) = int_get_value;
	      }
	      break;

	   case XmSPINBOX_SIGNED_NUMBER :
	      if ( SpinBoxDecimalPoints(cw) ) {
	         SpinBoxShowValue(cw) = decimal_show_signed_value;
	         SpinBoxGetValue(cw) = decimal_get_value;
	      } else {
	         SpinBoxShowValue(cw) = int_show_signed_value;
	         SpinBoxGetValue(cw) = int_get_value;
	      }
	      break;

	   default :
	      assert ( 0 );
	      break;
	}
}


static void set_default_inc_large ( XmpSpinBoxWidget cw  )
{
	switch ( SpinBoxType(cw) ) {
	   case XmSPINBOX_CLOCK_HM :
	   case XmSPINBOX_CLOCK_HMS :
	      SpinBoxIncLarge(cw) = 60;
	      break;
	  case XmSPINBOX_DATE:
	  	SpinBoxIncLarge(cw) = SpinBoxIncrement(cw)*10;
		break;
	   case XmSPINBOX_DOLLARS :
	      SpinBoxIncLarge(cw) = 100;
	      break;
	   case XmSPINBOX_NUMBER :
	  case XmSPINBOX_SIGNED_NUMBER:
	      if ( SpinBoxDecimalPoints(cw) ) {
	         int i, inc;
	         for ( i=0, inc=10; i<SpinBoxDecimalPoints(cw); i++ )
	            inc *= 10;
	         SpinBoxIncLarge(cw) = inc;
	      } else {
	         SpinBoxIncLarge(cw) = (SpinBoxMaxValue(cw) - SpinBoxMinValue(cw))/10;
	      }
	      break;
	   case XmSPINBOX_STRINGS :
	   case XmSPINBOX_BEACONCODE :
	      SpinBoxIncLarge(cw) = (SpinBoxMaxValue(cw) - SpinBoxMinValue(cw))/10;
	      break;
	   case XmSPINBOX_USER_DEFINED :
	      break;
	   default :
	      assert ( 0 );
	      break;
	}
}

static void set_default_min ( XmpSpinBoxWidget cw )
{
	switch ( SpinBoxType(cw) ) {
	   case XmSPINBOX_CLOCK_HM :     SpinBoxMinValue(cw) = 0; break;
	   case XmSPINBOX_CLOCK_HMS :    SpinBoxMinValue(cw) = 0; break;
	   case XmSPINBOX_DATE :         SpinBoxMinValue(cw) = 0; break;
	   case XmSPINBOX_BEACONCODE :   SpinBoxMinValue(cw) = 0; break;
	   case XmSPINBOX_DOLLARS :      SpinBoxMinValue(cw) = INT_MIN+1; break;
	   case XmSPINBOX_STRINGS :      SpinBoxMinValue(cw) = 0; break;
	   case XmSPINBOX_SIGNED_NUMBER:
	   case XmSPINBOX_NUMBER :       SpinBoxMinValue(cw) = INT_MIN+1; break;
	   case XmSPINBOX_USER_DEFINED : break;
	   default : assert ( 0 ); break;
	}
}

static void set_default_button_ratio ( XmpSpinBoxWidget cw )
{
	switch ( SpinBoxStyle(cw) ) {
	   case XmSPINBOX_STACKED_LEFT:
	   case XmSPINBOX_STACKED_RIGHT:
	  	SpinBoxButtonSizeRatio(cw) = 2;
		break;

	   case XmSPINBOX_LEFT:
	   case XmSPINBOX_RIGHT:
	   case XmSPINBOX_STACKED:
	   case XmSPINBOX_SEPARATE:
	  	SpinBoxButtonSizeRatio(cw) = 5;
		break;

	   default:
	  	assert ( 0 );
		break;
	}
}

static void set_default_max ( XmpSpinBoxWidget cw )
{
	switch ( SpinBoxType(cw) ) {
	   case XmSPINBOX_CLOCK_HM :     SpinBoxMaxValue(cw) = 24*60-1; break;
	   case XmSPINBOX_CLOCK_HMS :    SpinBoxMaxValue(cw) = 24*60*60-1; break;
	   case XmSPINBOX_DATE :         SpinBoxMaxValue(cw) = INT_MAX-1; break;
	   case XmSPINBOX_BEACONCODE :   SpinBoxMaxValue(cw) = 010000-1; break;
	   case XmSPINBOX_DOLLARS :      SpinBoxMaxValue(cw) = INT_MAX-1; break;
	   case XmSPINBOX_STRINGS :      SpinBoxMaxValue(cw) = SpinBoxItemCount(cw)-1; break;
	   case XmSPINBOX_SIGNED_NUMBER:
	   case XmSPINBOX_NUMBER :       SpinBoxMaxValue(cw) = INT_MAX-1; break;
	   case XmSPINBOX_USER_DEFINED : break;
	   default : assert ( 0 ); break;
	}
}

static void set_sensitive ( XmpSpinBoxWidget cw, Boolean sensitive )
{
	XtVaSetValues ( SpinBoxUpBtn(cw),    XmNsensitive, sensitive, NULL );
	XtVaSetValues ( SpinBoxDownBtn(cw),  XmNsensitive, sensitive, NULL );
	XtVaSetValues ( SpinBoxTF(cw),       XmNsensitive, sensitive, NULL );
}

static void set_arrow_orientation ( XmpSpinBoxWidget nw )
{
	XtVaSetValues ( SpinBoxUpBtn(nw),
	   XmNarrowDirection, SpinBoxArrowOrientation(nw)==XmVERTICAL ? XmARROW_UP : XmARROW_RIGHT,
	   NULL );
	XtVaSetValues ( SpinBoxDownBtn(nw),
	   XmNarrowDirection, SpinBoxArrowOrientation(nw)==XmVERTICAL ? XmARROW_DOWN : XmARROW_LEFT,
	   NULL );
}


/*
 *  Validation Routines...
 */
/*ARGSUSED*/
static Boolean validate_new_button_size_ratio ( XmpSpinBoxWidget cw, int ratio )
{
	return ( ratio > 0 );
}

/*ARGSUSED*/
static Boolean validate_new_inc_large ( XmpSpinBoxWidget cw, int value )
{
	/* Note that the max and min values can run from MAX_INT to MIN_INT so that
	 * the range can overrun an int, thus the use of float as a range check.
	 */
	float range = (float) SpinBoxMaxValue(cw) - (float) SpinBoxMinValue(cw);

	if ( value < 0 ) {
	   XmWarning ( (Widget)cw, "\nRequested SpinBox large increment must be greater than zero." );
	   return False;
	}
	if ( (float) value > range ) {
	   char tmpstr[150];
	   (void) snprintf ( tmpstr, sizeof(tmpstr),
	      "Requested Large Increment (%d) is larger than the range of the SpinBox (%d ... %d).",
	      value, SpinBoxMinValue(cw), SpinBoxMaxValue(cw) );
	   XmWarning ( (Widget)cw, tmpstr );
	   return False;
	}
	return True;
}

static Boolean validate_new_type ( XmpSpinBoxWidget cw, int value )
{
	if ( value != XmSPINBOX_NUMBER &&
		 value != XmSPINBOX_SIGNED_NUMBER &&
	     value != XmSPINBOX_CLOCK_HM && 
	     value != XmSPINBOX_CLOCK_HMS && 
		 value != XmSPINBOX_DATE &&
	     value != XmSPINBOX_BEACONCODE &&
	     value != XmSPINBOX_DOLLARS &&
	     value != XmSPINBOX_STRINGS &&
	     value != XmSPINBOX_USER_DEFINED ) {
			XmWarning ( (Widget)cw, "\nRequested SpinBox type not recognized!" );
			return False;
	}
	return True;
}

static Boolean validate_new_style ( XmpSpinBoxWidget cw, int value )
{
	if ( value != XmSPINBOX_STACKED &&
	     value != XmSPINBOX_STACKED_LEFT &&
	     value != XmSPINBOX_STACKED_RIGHT &&
	     value != XmSPINBOX_LEFT &&
	     value != XmSPINBOX_RIGHT &&
	     value != XmSPINBOX_SEPARATE ) {
			XmWarning ( (Widget)cw, "\nRequested SpinBox style not recognized!" );
			return False;
	}
	return True;
}

static Boolean validate_new_minimum ( XmpSpinBoxWidget cw, int value )
{
	if ( value > SpinBoxValue(cw) ) {
	   XmWarning ( (Widget)cw, "\nRequested minimum is greater than current SpinBox value!" );
	   return False;
	}
	return True;
}

static Boolean validate_new_maximum ( XmpSpinBoxWidget cw, int value )
{
	if ( value < SpinBoxValue(cw) ) {
	   XmWarning ( (Widget)cw, "\nRequested maximum is less than current SpinBox value!" );
	   return False;
	}
	return True;
}

static Boolean validate_new_delay ( XmpSpinBoxWidget cw, int value )
{
	if ( value<1 ) {
	   XmWarning ( (Widget)cw, "\nNew delay is smaller than minimum SpinBox delay!" );
	   return False;
	}
	return True;
}

static Boolean validate_new_value ( XmpSpinBoxWidget cw, int *value )
{
	if ( SpinBoxCycle(cw) )
	   return True;

	if ( *value < SpinBoxMinValue(cw) )
	{
	   if ( SpinBoxUseClosestValue(cw) )
	   {
	      *value = SpinBoxMinValue(cw);
	      return ( True );
	   }
	   return False;
	}

	if ( *value > SpinBoxMaxValue(cw) )
	{
	   if ( SpinBoxUseClosestValue(cw) )
	   {
	      *value = SpinBoxMaxValue(cw);
	      return ( True );
	   }
	   return False;
	}

	return True;
}


/*
 *  SET SpinBox && CALL VALUE CHANGED CALLBACKS
 */
static void activate_spinbox_callback ( XmpSpinBoxWidget cw, int value, char notify, int reason )
{
	if ( value > SpinBoxMaxValue(cw) && SpinBoxCycle(cw) ) value = SpinBoxMinValue(cw);
	if ( value < SpinBoxMinValue(cw) && SpinBoxCycle(cw) ) value = SpinBoxMaxValue(cw);

	SpinBoxValue(cw) = value;

	if ( notify != CB_SEND && notify != CB_FORCE )
	   return;

	if ( ( value != SpinBoxOldValue(cw) || notify == CB_FORCE ) &&
	     ( XtHasCallbacks((Widget)cw, XmNvalueChangedCallback ) == XtCallbackHasSome ) )
	{
	   char *pchar = XmTextFieldGetString(SpinBoxTF(cw));
	   XmpSpinBoxCallbackStruct data;
	   data.reason = reason;
	   data.value = value;
	   data.str = pchar;
	   data.str_len = strlen(pchar);
	   XtCallCallbacks ((Widget)cw, XmNvalueChangedCallback, (XtPointer)&data );
	   XtFree ( pchar );
	}
}

/* This is like XmpSpinBoxSetValue except that the reason sent in the callback
 * will be XmCR_VALUE_CHANGED and notification is always true. Used while the
 * arrows buttons are held down and values are "spinning".
 */
static void change_spinbox_value ( XmpSpinBoxWidget cw, int value )
{
	if ( SpinBoxValue(cw) == value )
	   return;
	if ( validate_new_value ( cw, &value ) )
	{
	   activate_spinbox_callback ( cw, value, CB_SEND, XmCR_VALUE_CHANGED );
	   set_text_field ( cw );
	   SpinBoxOldValue(cw) = SpinBoxValue(cw);
	}
}

/*
 *	 ACTIONS
 */
/*ARGSUSED*/
static void Action_Set_Large ( Widget w, XEvent *event, String *strings, Cardinal *string_qty )
{
	XmpSpinBoxWidget sb = (XmpSpinBoxWidget)XtParent(w);
	SpinBoxTimeoutUseIncLarge(sb) = True;
}

/*ARGSUSED*/
static void Action_Increment_Small ( Widget w, XEvent *event, String *strings, Cardinal *string_qty )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)XtParent(w);
	XmpSpinBoxSetValue ( (Widget)cw, SpinBoxValue(cw)+SpinBoxIncrement(cw), True );
}

/*ARGSUSED*/
static void Action_Increment_Large ( Widget w, XEvent * event, String * strings, Cardinal * string_qty )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)XtParent(w);
	XmpSpinBoxSetValue ( (Widget)cw, SpinBoxValue(cw)+SpinBoxIncLarge(cw), True );
}

/*ARGSUSED*/
static void Action_Decrement_Small ( Widget w, XEvent * event, String * strings, Cardinal * string_qty )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)XtParent(w);
	XmpSpinBoxSetValue ( (Widget)cw, SpinBoxValue(cw)-SpinBoxIncrement(cw), True );
}

/*ARGSUSED*/
static void Action_Decrement_Large ( Widget w, XEvent * event, String * strings, Cardinal * string_qty )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)XtParent(w);
	XmpSpinBoxSetValue ( (Widget)cw, SpinBoxValue(cw)-SpinBoxIncLarge(cw), True );
}

/*
 *	SHOW_VALUE and GET_VALUE functions
 */
/*ARGSUSED*/
static void decimal_show ( Widget w, int value, char *buffer, size_t maxlen, Boolean show_sign)
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;
	int i, left, right, magnitude;
	char *pchar = buffer;

	for ( i=0,magnitude=1; i<SpinBoxDecimalPoints(cw); i++ ) magnitude *= 10;

	/* show the sign */
	if ( value<0 ) {
	   *pchar++ = '-';
	   value = -value;
	}
	else if ( show_sign ) {
		*pchar++ = '+';
	}

	/* print lhs of the decimal point */
	left = value / magnitude;
	pchar += snprintf ( pchar, sizeof(pchar), "%d", left );

	right = value % magnitude;
	if ( !right ) return;
	if ( right<0 ) right = -right;

	pchar += snprintf ( pchar, sizeof(pchar), ".%0*d", SpinBoxDecimalPoints(cw), right );
	
	while ( *(--pchar)=='0' ) *pchar = '\0'; /* strip trailing 0s */
}


/*ARGSUSED*/
static void decimal_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	decimal_show(w, value, buffer, maxlen, False);
}


/*ARGSUSED*/
static void decimal_show_signed_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	decimal_show(w, value, buffer, maxlen, True);
}


/*ARGSUSED*/
static Boolean decimal_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;
	int left=0, right=0;
	int i, magnitude;
	char *pchar, *pchar2;

	for ( i=0,magnitude=1; i<SpinBoxDecimalPoints(cw); i++ )     
	   magnitude *= 10;

	errno = 0;
	left = strtol ( buffer, &pchar, 10 );
	if ( errno ) return ( False );
	left *= magnitude;

	if ( *pchar == '.' )
	{
	   int len;

	   errno = 0;
	   right = strtol ( ++pchar, &pchar2, 10 );
	   if ( errno ) return ( False );

	   if ( pchar!=pchar2 )
	   {
	      if ( *pchar2 ) return False;
	      len = SpinBoxDecimalPoints(cw) - (pchar2-pchar);
	      while ( len > 0 ) { right *= 10; len--; }
	      while ( len < 0 ) { right /= 10; len++; }
	   }
	}

	*value = left + right;
	return True;
}


/*ARGSUSED*/
static void dollar_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	register char *src, *tgt;
	int i, len, commas;
	char tmpbuf[32];
	Boolean neg;

	if (( neg = value < 0 ))
	   value = -value;

	src = tmpbuf;
	tgt = buffer;
	len = snprintf ( src, sizeof(tmpbuf), "%01d", value/100 );

	if ( neg )
	   *tgt++ = '(';

	*tgt++ = '$';

	commas = (len-1)/3;
	i = len - commas*3;
	while ( i-- )
	   *tgt++ = *src++;
	while ( commas-- ) {
	   *tgt++ = ',';
	   *tgt++ = *src++;
	   *tgt++ = *src++;
	   *tgt++ = *src++;
	}

	if (( i = value % 100 )) /* cents */
	   tgt += snprintf ( tgt, sizeof(tmpbuf), ".%02d", i );

	if ( neg )
	   *tgt++ = ')';

	*tgt = '\0';
}

/*ARGSUSED*/
static Boolean dollar_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	register char ch;
	Boolean dot = False;
	Boolean neg = False;
	int val = 0;

	while ( buflen-- ) {
	   ch = *buffer++;
	   if ( isdigit ( (int)ch ) ) val = (val*10) + (ch-'0');
	   else if ( ch==',' || ch=='$' ) continue;
	   else if ( ch=='.' ) { buflen=2; dot=True; }
	   else if ( ch=='(' || ch=='-' || ch==')' ) neg = True;
	   else return False;
	}

	if ( !dot ) val *= 100;
	if ( neg ) val = -val;
	*value = val;
	return True;
}


/*ARGSUSED*/
static void int_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	(void) snprintf ( buffer, maxlen, "%d", value );
}


/*ARGSUSED*/
static void int_show_signed_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	if(value > 0)
			(void) snprintf ( buffer, maxlen, "%+d", value );
	else
			(void) snprintf ( buffer, maxlen, "%d", value );
}

/*ARGSUSED*/
static Boolean int_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	errno = 0;
	*value = strtol ( buffer, NULL, 10 );
	return ( !errno );
}


/*ARGSUSED*/
static void clock_hms_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	int sec = value % 60;
	value /= 60;
	(void) snprintf ( buffer, maxlen, "%01d:%02d:%02d", value/60, value%60, sec );
}

/*ARGSUSED*/
static void clock_hm_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	(void) snprintf ( buffer, maxlen, "%01d:%02d", value/60, value%60 );
}

/*ARGSUSED*/
static Boolean clock_hms_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	int hr=0, min=0, sec=0;
	char *pchar;

	/* get hour */
	errno = 0;
	hr = strtol ( buffer, &pchar, 10 );
	if ( errno ) return ( False );

	/* get minute */
	if ( *pchar ) min = strtol ( ++pchar, &pchar, 10 );
	if ( errno ) return ( False );

	/* get seconds */
	if ( *pchar ) sec = strtol ( ++pchar, &pchar, 10 );
	if ( errno ) return ( False );

	*value = hr*3600 + min*60 + sec;
	return True;
}

/*ARGSUSED*/
static Boolean clock_hm_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	int hr, min=0;
	char *pchar;

	/* get hour */
	errno = 0;
	hr = strtol ( buffer, &pchar, 10 );
	if ( errno ) return ( False );

	/* get minute */
	if ( *pchar ) min = strtol ( ++pchar, &pchar, 10 );
	if ( errno ) return ( False );

	/* set value */
	*value = hr*60 + min;
	return True;
}


/*ARGSUSED*/
static void date_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	time_t tm = (time_t) value;
	(void)strftime(buffer, maxlen, ((XmpSpinBoxWidget)w)->spinbox.date_format, gmtime(&tm));
}

/*ARGSUSED*/
static Boolean date_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	return False;
}

/*ARGSUSED*/
static void bc_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	(void) snprintf ( buffer, maxlen, "%04o", (unsigned int)value );
}

/*ARGSUSED*/
static Boolean bc_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	char *pchar;
	int val;

	/* get number */
	errno = 0;
	val = strtol ( buffer, &pchar, 8 );
	if ( errno ) return ( False );

	/* we only want digits, nothing else. */
	if ( *pchar ) return False;

	*value = val;
	return True;
}

/*ARGSUSED*/
static void alpha_show_value ( Widget w, XtPointer client, int value, char *buffer, size_t maxlen )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;
	assert ( value >= 0 );
	assert ( value < SpinBoxItemCount(cw) );
	(void) strcpy ( buffer, ((char**)SpinBoxItems(cw))[value] );
}

static int alpha_cmp ( const void *a, const void *b )
{
	const String sa = *(const String *)a;
	const String sb = *(const String *)b;
	assert ( sa && (isprint((int)*sa)||!*sa) );
	assert ( sb && (isprint((int)*sb)||!*sb) );
	return ( strcmp ( sa, sb ) );
}

static String* alpha_search ( const char *findme, const String *base, int qty, Boolean are_sorted )
{
	int i;

	if ( are_sorted )
	   return ( (String*) bsearch ( &findme, base, qty, sizeof(String), alpha_cmp ) );

	for ( i=0; i<qty; i++ )
	   if ( !strcmp ( findme, base[i] ) )
	      break;
	if ( i==qty )
	   return (String*)0;

	return ((String*)(base + i));
}
	      
/*ARGSUSED*/
static Boolean alpha_get_value ( Widget w, XtPointer client, const char *buffer, size_t buflen, int *value )
{
	const XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;
	const String *pStr;
	const String *base = SpinBoxItems(cw);

	assert ( buffer );
	assert ( isprint((int)*buffer) || !*buffer );

	pStr = alpha_search ( buffer, base, SpinBoxItemCount(cw), SpinBoxItemsAreSorted(cw) );
	if ( !pStr ) return False;

	*value = (int)(pStr - base);
	return True;
}

/*==================== Public Functions ==============================*/

extern Widget XmpCreateSpinBox(Widget parent, String name, ArgList args, int nargs)
{
	return XtCreateWidget(name,xmpSpinBoxWidgetClass,parent,args,nargs);
}


extern Widget XmpCreateManagedSpinBox(Widget parent, String name, ArgList args, int nargs)
{
	return XtCreateManagedWidget(name,xmpSpinBoxWidgetClass,parent,args,nargs);
}


Widget XmpVaCreateSpinBox( Widget parent, char* name, ... )
{
	Widget w;
	va_list ap;
	int count;

	va_start(ap, name);
	count = XmeCountVaListSimple(ap);
	va_end(ap);

	va_start(ap, name);
	w = XmeVLCreateWidget(name, xmpSpinBoxWidgetClass, parent, False, ap, count);
	va_end(ap);
	return w;
}


Widget XmpVaCreateManagedSpinBox( Widget parent, char* name, ... )
{
	Widget w;
	va_list ap;
	int count;

	va_start(ap, name);
	count = XmeCountVaListSimple(ap);
	va_end(ap);

	va_start(ap, name);
	w = XmeVLCreateWidget(name, xmpSpinBoxWidgetClass, parent, True, ap, count);
	va_end(ap);
	return w;
}


void XmpSpinBoxSetValue ( Widget w, int value, Boolean notify )
{
	XmpSpinBoxWidget cw = (XmpSpinBoxWidget)w;

	if ( SpinBoxValue(cw) == value )
	   return;
	if ( validate_new_value ( cw, &value ) )
	{
	   activate_spinbox_callback ( cw, value, notify? CB_SEND:CB_NOSEND, XmCR_OK );
	   set_text_field ( cw );
	   SpinBoxOldValue(cw) = SpinBoxValue(cw);
	}
}

int XmpSpinBoxGetValue ( Widget w )
{
	XmpSpinBoxWidget sb = (XmpSpinBoxWidget)w;
	return ( SpinBoxValue(sb) );
}

void XmpSpinBoxSetReal ( Widget w, double value, Boolean notify )
{
	XmpSpinBoxWidget sb = (XmpSpinBoxWidget)w;
	int magnitude;
	int i;

	for ( i=0,magnitude=1; i<SpinBoxDecimalPoints(sb); i++ ) magnitude *= 10;
	XmpSpinBoxSetValue ( w, (int)(value*magnitude), notify );
}

double XmpSpinBoxGetReal ( Widget w )
{
	XmpSpinBoxWidget sb = (XmpSpinBoxWidget)w;
	double value = (double) SpinBoxValue ( sb );
	int magnitude;
	int i;

	for ( i=0,magnitude=1; i<SpinBoxDecimalPoints(sb); i++ ) magnitude *= 10;
	return ( value / magnitude );
}


/* Only valid if the SpinBox type is XmSPINBOX_STRINGS */
void XmpSpinBoxSetItem ( Widget w, String instring, Boolean notify )
{
	XmpSpinBoxWidget sb = (XmpSpinBoxWidget)w;
	const String *pStr;
	const String *base = SpinBoxItems(sb);

	if (SpinBoxType(sb) != XmSPINBOX_STRINGS) return;
	if (instring == NULL || *instring == '\0') return;

	pStr = alpha_search ( instring, base, SpinBoxItemCount(sb), SpinBoxItemsAreSorted(sb) );
	if (pStr )
	{
	  int value = (int)(pStr - base);
	  XmTextFieldSetString ( SpinBoxTF(sb), SpinBoxItems(sb)[value] );
	   activate_spinbox_callback ( sb, value, notify? CB_SEND:CB_NOSEND, XmCR_OK );
	   SpinBoxOldValue(sb) = SpinBoxValue(sb) = value;
	}
}


Widget XmpSpinBoxGetTextField(Widget w)
{
	XmpSpinBoxWidget sb = (XmpSpinBoxWidget)w;
	return SpinBoxTF(sb);
}
