/*
 *
 * XmpSpinBox.h - XmpSpinBox Public header
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

#ifndef _XmpSpinBox_h
#define _XmpSpinBox_h

#include <stdarg.h>
#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmpIsSpinBox
#define XmpIsSpinBox(w) XtIsSubclass(w, xmpSpinBoxWidgetClass)
#endif


/**
***   RESOURCES
**/

#ifndef XmNdateFormat
#define XmNdateFormat "dateFormat"
#define XmCDateFormat "DateFormat"
#endif

#ifndef XmNspinBoxDelay
#define XmNspinBoxDelay "spinBoxDelay"
#define XmCSpinBoxDelay "SpinBoxDelay"
#endif

#ifndef XmNspinBoxCycle
#define XmNspinBoxCycle "spinBoxCycle"
#define XmCSpinBoxCycle "SpinBoxCycle"
#endif

#ifndef XmNincrementLarge
#define XmNincrementLarge "incrementLarge"
#define XmCIncrementLarge "IncrementLarge"
#endif

#ifndef XmNincrementByMultiples
#define XmNincrementByMultiples	"incrementByMultiples"
#define XmCIncrementByMultiples	"IncrementByMultiples"
#endif

#ifndef XmNupdateText
#define XmNupdateText "updateText"
#define XmCUpdateText "UpdateText"
#endif

#ifndef XmNarrowOrientation
#define XmNarrowOrientation "arrowOrientation"
#define XmCArrowOrientation "ArrowOrientation"
#endif

#ifndef XmNspinBoxType
#define XmNspinBoxType "spinBoxType"
#define XmCSpinBoxType "SpinBoxType"
#define XmRSpinBoxType "SpinBoxType"
#endif

enum
{
   XmSPINBOX_NUMBER,
   XmSPINBOX_SIGNED_NUMBER,
   XmSPINBOX_CLOCK_HM,
   XmSPINBOX_CLOCK_HMS,
   XmSPINBOX_DATE,
   XmSPINBOX_BEACONCODE,
   XmSPINBOX_DOLLARS,
   XmSPINBOX_STRINGS,
   XmSPINBOX_USER_DEFINED
};

#ifndef XmNspinBoxStyle
#define XmNspinBoxStyle "spinBoxStyle"
#define XmCSpinBoxStyle "SpinBoxStyle"
#define XmRSpinBoxStyle "SpinBoxStyle"
#endif

enum
{
   XmSPINBOX_STACKED,
   XmSPINBOX_STACKED_LEFT,
   XmSPINBOX_STACKED_RIGHT, 
   XmSPINBOX_LEFT,
   XmSPINBOX_RIGHT,
   XmSPINBOX_SEPARATE
};

#define SPINBOX_BUTTON_SIZE_RATIO_DECIMAL_PLACES 2

#ifndef XmNbuttonSizeRatio
#define XmNbuttonSizeRatio "buttonSizeRatio"
#define XmCButtonSizeRatio "ButtonSizeRatio"
#endif

#ifndef XmNshowValueProc
#define XmNshowValueProc   "showValueProc"
#define XmCShowValueProc   "ShowValueProc"
#define XmRShowValueProc   "ShowValueProc"
#endif

#ifndef XmNshowValueData
#define XmNshowValueData   "showValueData"
#define XmCShowValueData   "ShowValueData"
#endif

#ifndef XmNgetValueData
#define XmNgetValueData	   "getValueData"
#define XmCGetValueData	   "GetValueData"
#endif

#ifndef XmNgetValueProc
#define XmNgetValueProc	   "getValueProc"
#define XmCGetValueProc	   "GetValueProc"
#define XmRGetValueProc	   "GetValueProc"
#endif

#ifndef XmNitemsAreSorted
#define XmNitemsAreSorted  "itemsAreSorted"
#define XmCItemsAreSorted  "ItemsAreSorted"
#endif

#ifndef XmNspinBoxUseClosestValue
#define XmNspinBoxUseClosestValue	"spinBoxUseClosestValue"
#define XmCSpinBoxUseClosestValue	"SpinBoxUseClosestValue"
#endif

externalref WidgetClass xmpSpinBoxWidgetClass;

typedef struct _XmpSpinBoxClassRec *XmpSpinBoxWidgetClass;
typedef struct _XmpSpinBoxRec *XmpSpinBoxWidget;

typedef struct
{
   const char *str;
   size_t str_len;
   int reason;
   int value;
}
XmpSpinBoxCallbackStruct;

typedef void (xmpSpinBoxShowValueProc) (
   Widget spinBox,
   XtPointer client_data,
   int num,
   char *buffer,
   size_t buflen_max );

typedef Boolean (xmpSpinBoxGetValueProc) (
   Widget spinBox,
   XtPointer client_data,
   const char *buffer,
   size_t buflen,
   int *num );


/**
***  Convenience functions
**/

extern Widget XmpCreateSpinBox          ( Widget parent, char *name, Arg *args, int arg_qty );
extern Widget XmpCreateManagedSpinBox   ( Widget parent, char *name, Arg *args, int arg_qty );
extern Widget XmpVaCreateSpinBox        ( Widget parent, char *name, ... );
extern Widget XmpVaCreateManagedSpinBox ( Widget parent, char *name, ... );
extern void   XmpSpinBoxSetValue        ( Widget spinBox, int value, Boolean notify ); 
extern int    XmpSpinBoxGetValue        ( Widget spinBox );
extern void   XmpSpinBoxSetReal         ( Widget spinBox, double value, Boolean notify );
extern double XmpSpinBoxGetReal         ( Widget spinBox );
extern void   XmpSpinBoxSetItem         ( Widget spinBox, char *item, Boolean notify );
extern Widget XmpSpinBoxGetTextField    ( Widget spinBox );


#ifdef __cplusplus
}
#endif

#endif /* _XmpSpinBox_h */
