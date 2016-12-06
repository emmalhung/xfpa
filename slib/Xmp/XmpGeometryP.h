/*
 * GeometryP.h - XmpGeometry Private header
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
#ifndef XmpGeometryP_h
#define XmpGeometryP_h

#include <Xm/ManagerP.h>
#include "XmpGeometry.h"

typedef void (*XmSizeProc)(Widget,Dimension*,Dimension*);

#define XmInheritInitializePostHook ((XtInitProc) _XtInherit)
#define XmInheritSetValuesPostHook ((XtSetValuesFunc) _XtInherit)
#define XmInheritConstraintInitializePostHook ((XtInitProc) _XtInherit)
#define XmInheritConstraintSetValuesPostHook ((XtSetValuesFunc) _XtInherit)
#define XmInheritSize ((XmSizeProc) _XtInherit)
#define XmInheritBitGravity ((int)((long) _XtInherit) )

typedef struct
{
	int				bit_gravity;
	XtInitProc		initialize_post_hook;
	XtSetValuesFunc	set_values_post_hook;
	XtInitProc		constraint_initialize_post_hook;
	XtSetValuesFunc	constraint_set_values_post_hook;
	XmSizeProc		size;
	XtPointer		extension;
} XmpGeometryClassPart;

typedef struct XmpGeometryClassRec
{
	CoreClassPart			core_class;
	CompositeClassPart		composite_class;
	ConstraintClassPart		constraint_class;
	XmManagerClassPart		manager_class;
	XmpGeometryClassPart	geometry_class;
} XmpGeometryClassRec;

externalref XmpGeometryClassRec xmpGeometryClassRec;

typedef struct
{
	Dimension		pref_width;
	Dimension		pref_height;
	Boolean			compute_width;
	Boolean			compute_height;
	Boolean			reconfigure;
	Boolean			constraint_reconfigure;
	Widget			instigator;
} XmpGeometryPart, *XmpGeometryPartPtr;

typedef struct XmpGeometryRec
{
	CorePart		core;
	CompositePart	composite;
	ConstraintPart	constraint;
	XmManagerPart	manager;
	XmpGeometryPart	geometry;
} XmpGeometryRec;

typedef struct XmpGeometryConstraintPart
{
	XtPointer		reserved;
} XmpGeometryConstraintPart, *XmpGeometryConstraintPartPtr;

typedef struct XmpGeometryConstraintRec
{
	XmManagerConstraintPart	manager;
	XmpGeometryConstraintPart geometry;
} XmpGeometryConstraintRec, *XmpGeometryConstraint;



extern void XmPreferredGeometry(Widget,Dimension*,Dimension*);
extern void XmSetGeometry(Widget,Position,Position,Dimension,Dimension,Dimension);
extern void XmWarning(Widget,String);

/* Instance field access macros */

#define PrefWidth(w) (w->geometry.pref_width)
#define PrefHeight(w) (w->geometry.pref_height)
#define ComputeWidth(w) (w->geometry.compute_width)
#define ComputeHeight(w) (w->geometry.compute_height)
#define Reconfigure(w) (w->geometry.reconfigure)
#define ConstraintReconfigure(w) (w->geometry.constraint_reconfigure)
#define Instigator(w) (w->geometry.instigator)

#define Children(w) (w->composite.children)
#define NumChildren(w) (w->composite.num_children)

#define X(w) (w->core.x)
#define Y(w) (w->core.y)
#define Width(w) (w->core.width)
#define Height(w) (w->core.height)
#define BorderWidth(w) (w->core.border_width)

/* Constraint field access macros */

#define GeometryConstraint(w) ((XmpGeometryConstraint)(w->core.constraints)

#define GeometryReserved(w) (GeometryConstraint(w)->geometry.reserved)

/* Class method macros  - used by subclass to envelop XmpGeometry methods */

#define XmpGeometryRedisplay(w,ev,rg) \
		(*xmpGeometryWidgetClass->core_class.expose)(w,ev,rg)
#define XmpGeometryChangeManaged(w) \
	(*xmpGeometryWidgetClass->composite_class.change_managed)(w)
#define XmpGeometryRealize(w,m,a) \
		(*xmpGeometryWidgetClass->core_class.realize(w,m,a)

/* Class field access macros */

#define BitGravity(w) (GeometryClass(w)->geometry_class.bit_gravity)

/* XmpGeometry hook method support macros */

#define GeometryClass(w) ((XmpGeometryWidgetClass)XtClass(w))
#define GeometryClassPart(w) (GeometryClass(w)->geometry_class)

#define XmpGeometryInitialize(wc,rw,nw,a,na) \
    if (wc == XtClass(nw)) \
	(*GeometryClassPart(nw).initialize_post_hook)(rw,nw,a,na)

#define XmpGeometrySetValues(wc,ow,rw,nw,a,na) \
    (wc == XtClass(nw) ? \
	(*GeometryClassPart(nw).set_values_post_hook)(ow,rw,nw,a,na) \
	: False)

#define XmpGeometryConstraintInitialize(wc,rw,nw,a,na) \
    if (wc == XtClass(nw)) \
	(*GeometryClassPart(nw).constraint_initialize_post_hook)(rw,nw,a,na)

#define XmpGeometryConstraintSetValues(wc,ow,rw,nw,a,na) \
    (wc == XtClass(nw) ? \
	(*GeometryClassPart(nw).constraint_set_values_post_hook)(ow,rw,nw,a,na) \
	: False)

/* Useful Resize/Size macros */

/* Do the subtraction (a-b), but honor the specified minimum result (d) */
#define XmSubtract(a,b,d) ((a)>((b)+(d)) ? (a)-(b) : d)

#ifndef Max
#define Max(x, y) (((unsigned)(x) > (unsigned)(y)) ? (x) : (y))
#endif

#endif /* XmpGeometryP_h */
