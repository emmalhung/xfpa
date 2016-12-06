/*
 *
 * XmpSpinBoxP.h - XmpSpinBox Private header
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

#ifndef _XmpSpinBoxP_h
#define _XmpSpinBoxP_h

#include <Xm/RepType.h>
#include "XmpGeometryP.h"
#include "XmpSpinBox.h"

typedef struct
{
    XmRepTypeId		spinbox_type_id;
    XmRepTypeId		spinbox_style_id;
	XtPointer		extension;
}
XmpSpinBoxClassPart;

typedef struct _XmpSpinBoxClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	XmpGeometryClassPart	geometry_class;
	XmpSpinBoxClassPart	spinbox_class;
}
XmpSpinBoxClassRec;

externalref XmpSpinBoxClassRec xmpSpinBoxClassRec;

typedef struct
{
   unsigned char arrow_orientation;
   unsigned char spinbox_style;
   unsigned char spinbox_type;
   String date_format;
   short tf_columns;
   short decimal_points;
   Boolean items_are_sorted;
   Boolean use_closest_value;
   Boolean cycle;
   Boolean text_update_constantly;
   Boolean editable;
   int delay_ms;
   int item_count;
   int button_size_ratio;
   int val_now;
   int val_old;
   int val_min;
   int val_max;
   int increment;
   int increment_large;
   Boolean increment_by_multiples;
   XtIntervalId interval;
   Widget down_btn;
   Widget up_btn;
   Widget tf;
   XtPointer show_value_data;
   XtPointer get_value_data;
   xmpSpinBoxShowValueProc *show_value_proc;
   xmpSpinBoxGetValueProc *get_value_proc;
   String* items;
   XtCallbackList ValueChangedCBL;
   XtAppContext context;
   Boolean timeout_use_inc_large;
}
XmpSpinBoxPart, *XmpSpinBoxPartPtr;

typedef struct _XmpSpinBoxRec
{
   CorePart			core;
   CompositePart	composite;
   ConstraintPart	constraint;
   XmManagerPart	manager;
   XmpGeometryPart	geometry;
   XmpSpinBoxPart	spinbox;
}
XmpSpinBoxRec;

typedef struct _XmpSpinBoxConstraintPart
{
   XtPointer		make_compiler_happy;
}
XmpSpinBoxConstraintPart, *XmpSpinBoxConstraintPartPtr;

typedef struct _XmpSpinBoxConstraintRec
{
   XmManagerConstraintPart	manager;
   XmpGeometryConstraintPart	geometry;
   XmpSpinBoxConstraintPart	spinbox;
}
XmpSpinBoxConstraintRec, *XmpSpinBoxConstraint;

#endif /* _XmpSpinBoxP_h */
