/*
 *  File: DoubleSSP.h
 *
 *  Purpose: Private header file for the XmpDoubleSliderScaleWidget class.
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
#ifndef __DOUBLESSP_H__
#define __DOUBLESSP_H__

#include <X11/ConstrainP.h>
#include <X11/CompositeP.h>
#include <Xm/ManagerP.h>
#include "XmpDoubleSS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XmpDoubleSliderScaleClassPart {
	int i;
} XmpDoubleSliderScaleClassPart;

typedef struct _XmpDoubleSliderScaleClassRec{
	CoreClassPart core_class;
	CompositeClassPart composite_class;
	ConstraintClassPart constraint_class;
	XmManagerClassPart  manager_class;
	XmpDoubleSliderScaleClassPart dsscale_class;
}XmpDoubleSliderScaleClassRec;

typedef struct _XmpDoubleSliderScalePart {

	XtCallbackList lower_drag_callback;
	XtCallbackList upper_drag_callback;
	XtCallbackList lower_value_changed_callback;
	XtCallbackList upper_value_changed_callback;

	Pixmap slider;
	
	Pixel within_limits_pixel;
	Pixel outside_limits_pixel;

	GC foreground_GC;
	GC within_limits_GC;
	GC outside_limits_GC;

	Dimension slot_thickness;

	Dimension     slider_thickness;
	Dimension     slider_length;
	unsigned char slider_style;

	unsigned char orientation;
	unsigned char processing_direction;

	Boolean show_values;
	Boolean snap_to_value;

	int minimum_value;
	int maximum_value;

	int lower_value;
	int upper_value;

	int lower_x;
	int lower_y;
	int upper_x;
	int upper_y;

	Position show_lower_value_x;
	Position show_lower_value_y;
	Position show_upper_value_x;
	Position show_upper_value_y;

	Boolean lower_slider_on;
	Boolean upper_slider_on;

	int offset_x;
	int offset_y;

	Dimension scale_width;
	Dimension scale_height;

	Dimension slider_size_x;
	Dimension slider_size_y;

	XmFontList  font_list;
	int         font_last_value; /* used to resolve between XmRenderTable & XmFontList
									when setting up the resource table */
	unsigned char value_alignment;

} XmpDoubleSliderScalePart;

typedef struct _XmpDoubleSliderScaleRec {
	CorePart   core;
	CompositePart  composite;
	ConstraintPart constraint; 	
  	XmManagerPart   manager;
	XmpDoubleSliderScalePart	dsscale; 
} XmpDoubleSliderScaleRec;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* __DOUBLESSP_H__ */

