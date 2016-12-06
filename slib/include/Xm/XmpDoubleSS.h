/*
 *  File: DoubleSS.h
 *
 *  Purpose: Public header file for the XmpDoubleSliderScaleWidget class.
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
#ifndef __DOUBLESS_H__
#define __DOUBLESS_H__

#include <stdarg.h>
#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmNlowerDragCallback
#define XmNlowerDragCallback "lowerDragCallback"
#define XmCLowerDragCallback "LowerDragCallback"
#endif

#ifndef XmNupperDragCallback
#define XmNupperDragCallback "upperDragCallback"
#define XmCUpperDragCallback "UpperDragCallback"
#endif

#ifndef XmNlowerValueChangedCallback
#define XmNlowerValueChangedCallback "lowerValueChangedCallback"
#define XmCLowerValueChangedCallback "LowerValueChangedCallback"
#endif

#ifndef XmNupperValueChangedCallback
#define XmNupperValueChangedCallback "upperValueChangedCallback"
#define XmCUpperValueChangedCallback "UpperValueChangedCallback"
#endif

#ifndef XmNlowerValue
#define XmNlowerValue "lowerValue"
#define XmCLowerValue "LowerValue"
#endif

#ifndef XmNupperValue
#define XmNupperValue "upperValue"
#define XmCUpperValue "UpperValue"
#endif

#ifndef XmNvalueAlignment
#define XmNvalueAlignment "valueAlignment"
#define XmCValueAlignment "ValueAlignment"
#define XmRValueAlignment "ValueAlignment"
#endif

#ifndef XmNshowValues
#define XmNshowValues "showValues"
#define XmCShowValues "ShowValues"
#endif

#ifndef XmNwithinLimitsColor
#define XmNwithinLimitsColor	"withinLimitsColor"
#define XmCWithinLimitsColor	"WithinLimitsColor"
#endif

#ifndef XmNoutsideLimitsColor
#define XmNoutsideLimitsColor	"outsideLimitsColor"
#define XmCOutsideLimitsColor	"OutsideLimitsColor"
#endif

#ifndef XmNsnapToValue
#define XmNsnapToValue	"snapToValue"
#define XmCSnapToValue	"SnapToValue"
#endif

#ifndef XmNsliderStyle
#define XmNsliderStyle	"sliderStyle"
#define XmCSliderStyle	"SliderStyle"
#define XtRSliderStyle	"SliderStyle"
#endif

typedef unsigned char SliderStyle;
#define XmCROSS_AXIS_RECTANGLE	0
#define XmALONG_AXIS_RECTANGLE	1


extern WidgetClass xmpDoubleSliderScaleWidgetClass;

typedef struct _XmpDoubleSliderScaleClassRec *XmpDoubleSliderScaleWidgetClass;
typedef struct _XmpDoubleSliderScaleRec *XmpDoubleSliderScaleWidget;

enum {	XmCR_LOWER_VALUE_CHANGED,
		XmCR_UPPER_VALUE_CHANGED,
		XmCR_LOWER_DRAG,
		XmCR_UPPER_DRAG
};
 
typedef struct {
	int    reason;
	XEvent *event;
	int    value;
} XmpDoubleSliderScaleCallbackStruct;


extern Widget XmpCreateDoubleSliderScale          (Widget, char*, ArgList, int);
extern Widget XmpCreateManagedDoubleSliderScale   (Widget, char*, ArgList, int);
extern Widget XmpVaCreateDoubleSliderScale        (Widget, char*, ...);
extern Widget XmpVaCreateManagedDoubleSliderScale (Widget, char*, ...);
extern void   XmpDoubleSliderScaleSetUpperValue   (Widget, int);
extern void   XmpDoubleSliderScaleSetLowerValue   (Widget, int);
extern int    XmpDoubleSliderScaleGetUpperValue   (Widget);
extern int    XmpDoubleSliderScaleGetLowerValue   (Widget);

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /**  __DOUBLESS_H__  **/
