/*
 * Geometry.c - XmpGeometry widget
 *
 * Motif XmManager subclass. Xt/Motif Geometry metaclass.
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
#include <stdio.h>
#include <Xm/RepType.h>
#include <Xm/GadgetP.h>
#include "XmpGeometryP.h"

static void ClassPartInitialize(WidgetClass);
static void Initialize(Widget, Widget, ArgList, Cardinal*);
static void Realize(Widget,XtValueMask*,XSetWindowAttributes*);
static void Resize(Widget);
static void Redisplay(Widget, XEvent*, Region);
static Boolean SetValues(Widget, Widget, Widget, ArgList, Cardinal*);
static XtGeometryResult QueryGeometry(Widget, XtWidgetGeometry*, XtWidgetGeometry*);
static XtGeometryResult GeometryManager(Widget, XtWidgetGeometry*, XtWidgetGeometry*);
static void ChangeManaged(Widget);
static Boolean ConstraintSetValues(Widget, Widget, Widget, ArgList, Cardinal*);
static void InitializePostHook(Widget, Widget, ArgList, Cardinal*);
static Boolean SetValuesPostHook(Widget, Widget, Widget, ArgList, Cardinal*);
static void ConstraintInitializePostHook(Widget, Widget, ArgList, Cardinal*);
static Boolean ConstraintSetValuesPostHook(Widget, Widget, Widget,	ArgList, Cardinal*);
static void Size(Widget,Dimension*,Dimension*);
static void ProcessSizeMethod(Widget,Dimension*,Dimension*);

/* Class method macros */

#define CallResize(w) (*GeometryClass(w)->core_class.resize)(w)
#define CallSize(w,wd,ht) (*GeometryClass(w)->geometry_class.size) (w,wd,ht)


/****************************************************************
 *
 * XmpGeometry class record
 *
 ****************************************************************/

externaldef(xmgeometryclassrec) XmpGeometryClassRec xmpGeometryClassRec = 
{
	{                                           /* core_class */
		(WidgetClass)&xmManagerClassRec,          /* superclass            */
		"XmpGeometry",                            /* class_name            */
		sizeof(XmpGeometryRec),                   /* widget_size           */
		NULL,                                     /* class_initialize      */
		ClassPartInitialize,                      /* class_part_initialize */
		FALSE,                                    /* class_inited          */
		Initialize,                               /* initialize            */
		NULL,                                     /* initialize_hook       */
		Realize,                                  /* realize               */
		NULL,                                     /* actions               */
		0,                                        /* num_actions           */
		NULL,                                     /* resources             */
		0,                                        /* num_resources         */
		NULLQUARK,                                /* xrm_class             */
		TRUE,                                     /* compress_motion       */
		XtExposeCompressMaximal,                  /* compress_exposure     */
		TRUE,                                     /* compress_enterleave   */
		FALSE,                                    /* visible_interest      */
		NULL,                                     /* destroy               */
		Resize,                                   /* resize                */
		Redisplay,                                /* expose                */
		SetValues,                                /* set_values            */
		NULL,                                     /* set_values_hook       */
		XtInheritSetValuesAlmost,                 /* set_values_almost     */
		NULL,                                     /* get_values_hook       */
		NULL,                                     /* accept_focus          */
		XtVersion,                                /* version               */
		NULL,                                     /* callback_private      */
		XtInheritTranslations,                    /* tm_table              */
		QueryGeometry,                            /* query_geometry        */
		NULL,                                     /* display_accelerator   */
		NULL,                                     /* extension             */
	},
	{                                           /* composite_class */
		GeometryManager,                          /* geometry_manager      */
		ChangeManaged,                            /* change_managed        */
		XtInheritInsertChild,                     /* insert_child          */
		XtInheritDeleteChild,                     /* delete_child          */
		NULL,                                     /* extension             */
	},
	{                                           /* constraint_class */
		NULL,                                     /* resources             */   
		0,                                        /* num_resources         */   
		sizeof(XmpGeometryConstraintRec),         /* constraint_size       */
		NULL,                                     /* initialize            */   
		NULL,                                     /* destroy               */   
		ConstraintSetValues,                      /* set_values            */   
		NULL,                                     /* extension             */
	},
	{                                           /* manager class */
		XtInheritTranslations,                    /* translations          */
		NULL,                                     /* syn_resources         */
		0,                                        /* num_syn_resources     */
		NULL,                                     /* syn_constraint_resources */
		0,                                        /* num_syn_constraint_resources */
		XmInheritParentProcess,                   /* parent_process        */
		NULL,                                     /* extension             */    
	},
	{                                           /* geometry class */
		ForgetGravity,                            /* bit_gravity           */
		InitializePostHook,                       /* initialize_post_hook  */
		SetValuesPostHook,                        /* set_values_post_hook  */
		ConstraintInitializePostHook,             /* constraint_initialize_post_hook*/
		ConstraintSetValuesPostHook,              /* constraint_set_values_post_hook*/
		Size,                                     /* size                  */
		NULL,                                     /* extension             */    
	}        
};

externaldef(xmgeometrywidgetclass) WidgetClass xmpGeometryWidgetClass = (WidgetClass) &xmpGeometryClassRec;


static void ClassPartInitialize (WidgetClass widgClass)
{
	XmpGeometryWidgetClass wc = (XmpGeometryWidgetClass)widgClass;
	XmpGeometryWidgetClass sc = (XmpGeometryWidgetClass)wc->core_class.superclass;

	 /* Process method inheritance for subclasses of XmpGeometry */

	if (wc->geometry_class.bit_gravity == XmInheritBitGravity)
		wc->geometry_class.bit_gravity = sc->geometry_class.bit_gravity;
	if (wc->geometry_class.initialize_post_hook == XmInheritInitializePostHook)
		wc->geometry_class.initialize_post_hook = sc->geometry_class.initialize_post_hook;
	if (wc->geometry_class.set_values_post_hook == XmInheritSetValuesPostHook)
		wc->geometry_class.set_values_post_hook = sc->geometry_class.set_values_post_hook;
	if (wc->geometry_class.constraint_initialize_post_hook ==	XmInheritConstraintInitializePostHook)
		wc->geometry_class.constraint_initialize_post_hook = sc->geometry_class.constraint_initialize_post_hook;
	if (wc->geometry_class.constraint_set_values_post_hook == XmInheritConstraintSetValuesPostHook)
		wc->geometry_class.constraint_set_values_post_hook = sc->geometry_class.constraint_set_values_post_hook;
	if (wc->geometry_class.size == XmInheritSize)
		wc->geometry_class.size = sc->geometry_class.size;
}


/*ARGSUSED*/
static void Initialize (Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	XmpGeometryWidget rw = (XmpGeometryWidget)request_w;
	XmpGeometryWidget nw = (XmpGeometryWidget)new_w;

	ConstraintReconfigure(nw) = False;
	Instigator(nw) = NULL;

	/* Remember application geometry settings */

	ComputeWidth(nw) = True;
	ComputeHeight(nw) = True;

	if (Width(rw) != 0) {
		ComputeWidth(nw) = False;
		PrefWidth(nw) = Width(rw);
		Width(nw) = Width(rw);
	}
	if (Height(rw) != 0) {
		ComputeHeight(nw) = False;
		PrefHeight(nw) = Height(rw);
		Height(nw) = Height(rw);
	}
}


static void Realize (Widget w, XtValueMask* mask, XSetWindowAttributes* wa)
{
	XSetWindowAttributes attr;

	(*xmManagerWidgetClass->core_class.realize)(w, mask, wa);

	attr.bit_gravity = BitGravity(w);
	XChangeWindowAttributes(XtDisplay(w), XtWindow(w), CWBitGravity, &attr);
}


static void Resize (Widget w)
{
	XmWarning(w, "XmpGeometry subclass didn't specify Resize method!");
}


static void Redisplay (Widget w, XEvent* event, Region region)
{
	/* Pass exposure down to gadget children */
	_XmRedisplayGadgets (w, event, region);
}


/*ARGSUSED*/
static Boolean SetValues (Widget old_w, Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	XmpGeometryWidget cw = (XmpGeometryWidget)old_w;
	XmpGeometryWidget rw = (XmpGeometryWidget)request_w;
	XmpGeometryWidget nw = (XmpGeometryWidget)new_w;

	Reconfigure(nw) = False;

	/* Check for application geometry settings. '0' means 'ideal size' */

	if (Width(rw) == 0) {
		Width(nw) = 0;
	    ComputeWidth(nw) = True;
	} else if (Width(rw) != Width(cw)) {
	    ComputeWidth(nw) = False;
	    PrefWidth(nw) = Width(rw);
	    Width(nw) = Width(rw);
	}

	if (Height(rw) == 0) {
	    Height(nw) = 0;
	    ComputeHeight(nw) = True;
	} else if (Height(rw) != Height(cw)) {
	    ComputeHeight(nw) = False;
	    PrefHeight(nw) = Height(rw);
	    Height(nw) = Height(rw);
	}

	if (Width(nw) != Width(cw) || Height(nw) != Height(cw)) {
		/* Inform subclass that we need a reconfigure */
		Reconfigure(nw) = True;
	}

	return (False);
}


static XtGeometryResult QueryGeometry (Widget w, XtWidgetGeometry* request, XtWidgetGeometry* reply)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)w;

	/* Return our preferred size */

	if ((request->request_mode == (CWWidth | CWHeight)) &&
		request->width == PrefWidth(gw) &&
		request->height == PrefHeight(gw)) {
		return (XtGeometryYes);
	}

	if (Width(gw) == PrefWidth(gw) && Height(gw) == PrefHeight(gw)) {
		return (XtGeometryNo);
	}

	reply->request_mode = (CWWidth | CWHeight);
	reply->width = PrefWidth(gw);
	reply->height = PrefHeight(gw);

	return (XtGeometryAlmost);
}


/*ARGSUSED*/
static XtGeometryResult GeometryManager (Widget w, XtWidgetGeometry* request, XtWidgetGeometry* reply)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)XtParent(w);
	Dimension geometryWidth, geometryHeight;

	/* If the request was caused by ConstraintSetValues reset the flag */

	if (ConstraintReconfigure(gw)) {
		ConstraintReconfigure(gw) = False;
		request->border_width -= 1;
	}

	/* Query request: Say yes now, do the best we can when we're really asked */
	if (request->request_mode & XtCWQueryOnly)
		return (XtGeometryYes);

	/* X or Y request: Always say no */
	if ((request->request_mode & ~(CWX | CWY)) == 0)
		return (XtGeometryNo);

	/* Stacking request: Always say yes */
	if ((request->request_mode & ~(CWStackMode | CWSibling)) == 0)
		return (XtGeometryYes);

	/* Set up instigator to reflect its requested geometry */
	if (request->request_mode & CWX) X(w) = request->x;
	if (request->request_mode & CWY) Y(w) = request->y;
	if (request->request_mode & CWBorderWidth) BorderWidth(w) = request->border_width;
	if (request->request_mode & CWWidth) Width(w) = request->width;
	if (request->request_mode & CWHeight) Height(w) = request->height;

	/* Tag instigator for XmPreferredGeometry() and XmSetGeometry() */
	Instigator(gw) = w;

	/* Calculate new ideal size */
	ProcessSizeMethod((Widget)gw, &geometryWidth, &geometryHeight);

	/* Get closest to ideal size from parent */
	while (XtMakeResizeRequest ((Widget)gw, geometryWidth, geometryHeight,
		   &geometryWidth, &geometryHeight) == XtGeometryAlmost);

	/* Layout children with new size */
	CallResize((Widget)gw);

	/* Done */
	Instigator(gw) = NULL;

	return (XtGeometryYes);
}


static void ChangeManaged (Widget w)
{
	XmpGeometryWidget gw = (XmpGeometryWidget) w;
	Dimension geometryWidth, geometryHeight;

	/* Calculate ideal size */
	ProcessSizeMethod((Widget)gw, &geometryWidth, &geometryHeight);

	/* Get closest to ideal size from parent */
	while (XtMakeResizeRequest ((Widget)gw, geometryWidth, geometryHeight,
			   &geometryWidth, &geometryHeight) == XtGeometryAlmost);

	/* Layout children with new size */
	CallResize((Widget)gw);

	/* Update keyboard traversal */
	_XmNavigChangeManaged ((Widget)gw);
}



/*ARGSUSED*/
static Boolean ConstraintSetValues(Widget old_w, Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)XtParent(new_w);

	ConstraintReconfigure(gw) = False;

	return False;
}


/*ARGSUSED*/
static void InitializePostHook(Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	XmpGeometryWidget nw = (XmpGeometryWidget)new_w;

	/* Calculate ideal size */
	ProcessSizeMethod((Widget)nw, &(Width(nw)), &(Height(nw)));

	/* Do initial layout */
	CallResize((Widget)nw);
}


/*ARGSUSED*/
static Boolean SetValuesPostHook(Widget old_w, Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	XmpGeometryWidget cw = (XmpGeometryWidget)old_w;
	XmpGeometryWidget nw = (XmpGeometryWidget)new_w;

	if (Reconfigure(nw) == True) {

		/* Calculate new size */
		ProcessSizeMethod((Widget)nw, &Width(nw), &Height(nw));

		if (Width(nw) == Width(cw) && Height(nw) == Height(cw) && BorderWidth(nw) == BorderWidth(cw)) {
			/* No geometry request will trigger a relayout, reconfigure now */
			CallResize((Widget)nw);
		}
	}

	return(False);
}

	 
/*ARGSUSED*/
static void ConstraintInitializePostHook(Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	/* NoOp */
}


/*ARGSUSED*/
static Boolean ConstraintSetValuesPostHook(Widget old_w, Widget request_w, Widget new_w, ArgList args, Cardinal* num_args)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)XtParent(old_w);

	if (ConstraintReconfigure(gw) == True)
		BorderWidth(new_w) += 1;

	return(False);
}


/*ARGSUSED*/
static void Size (Widget w, Dimension* geometryWidth, Dimension* geometryHeight)
{
	XmWarning(w, "XmpGeometry subclass didn't specify a Size method!");
}


/*ARGSUSED*/
static void ProcessSizeMethod(Widget w, Dimension* geometryWidth, Dimension* geometryHeight)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)(w);
	Dimension width;
	Dimension height;

	CallSize(w, &width, &height);

	if (ComputeWidth(gw) == True) PrefWidth(gw) = width;
	if (ComputeHeight(gw) == True) PrefHeight(gw) = height;

	*geometryWidth = PrefWidth(gw);
	*geometryHeight = PrefHeight(gw);
}


/************************************************************************
 *
 * XmpGeometry utilities
 *
 ************************************************************************/

/* Get child's preferred geometry. If child is involved in a 
 * geometry negotiation, use its requested size instead.
 */
void XmPreferredGeometry (Widget w, Dimension* width, Dimension* height)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)XtParent(w);
	Dimension pw;
	Dimension ph;

	if (w != Instigator(gw)) {
		XtWidgetGeometry reply;
		XtQueryGeometry (w, NULL, &reply);
		pw = (reply.request_mode & CWWidth) ? reply.width : Width(w);
		ph = (reply.request_mode & CWHeight) ? reply.height : Height(w);
	} else {
		pw = Width(w);
		ph = Height(w);
	}

	if (width != NULL) *width = pw;
	if (height != NULL) *height = ph;
}

/* Set child's geometry. If child is involved in a geometry request, just setup fields
 */
void XmSetGeometry(Widget w, Position x, Position y, Dimension width, Dimension height, Dimension borderWidth)
{
	XmpGeometryWidget gw = (XmpGeometryWidget)XtParent(w);

	if (w != Instigator(gw)) {
		XtConfigureWidget (w, x, y, width, height, borderWidth);
	} else {
		X(w) = x;
		Y(w) = y;
		Width(w) = width;
		Height(w) = height;
		BorderWidth(w) = borderWidth;
	}
}

void XmWarning(Widget w, String str)
{
	String message = XtMalloc(strlen(str)+strlen(XtName(w))+3);
	(void) sprintf(message, "%s: %s", XtName(w), str);
	XtAppWarning(XtWidgetToApplicationContext((Widget)w), message);
	XtFree(message);
}

