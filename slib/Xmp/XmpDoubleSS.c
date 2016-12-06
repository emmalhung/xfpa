/************************************************************************ 
 *
 *    File: DoubleSS.c
 *
 * Purpose: Contains the code for the XmpDoubleSliderScaleWidget
 *
 * Comment: This widget provides two sliders on one bar, one setting a
 *          lower value and one an upper value on a range of numbers.
 *          This can then be used to provide a window within the range.
 *          The widget is a highly modified version based on an version
 *          whose notice is included below.
 *
 * Resource Notes:
 *
 * 	XmNsliderStyle - XmALONG_AXIS_RECTANGLE or XmCROSS_AXIS_RECTANGLE.
 *                   The slider is a rectangle with its major axis either
 *  along the scale major axis direction or cross wise to the major axis
 *  direction (along the minor axis). Default XmCROSS_AXIS_RECTANGLE.
 *
 *  XmNsnapToValue - True or False. After being moved with the mouse
 *                   the slider will "snap" to the position represented
 *  by the value returned. This will occur if there are fewer value
 *  positions than pixels on the slider path. Default True.
 *
 *  XmNwithinLimtsColor - The colour of the bar between the sliders.
 *                        Default is the manager foreground colour.
 *
 *  XmNoutsideLimitsColor - The colour of the bar not between the sliders.
 *                          Default is the manager foreground colour.
 *
 *  XmNvalueAlignment - Since there are two sliders, the optional value
 *                      display above each slider can be aligned in the
 *  standard ways, XmALIGNMENT_BEGINNING, XmALIGNMENT_CENTER, and
 *  XmALIGNMENT_END. Default XmALIGNMENT_END.
 *          
 ************************************************************************ 
 *
 * Version 1.4	on	18-Nov-1996
 * (c) 1996 Pralay Kanti Dakua (pralay@teil.soft.net)
 *		 Tata Elxsi India Ltd
 *		 
 * This is a free software and permission to use, modify, distribute,
 * selling and using for commercial purpose is hereby granted provided
 * that this permission notice shall be included in all copies and their
 * supporting documentations.
 *
 * There is no warranty for this software. In no event Pralay Kanti Dakua
 * or Tata Elxsi India Ltd will be liable for merchantability and
 * fitness of the software and damages due to this software.
 *
 * Author:
 * Pralay Kanti Dakua (pralay@teil.soft.net)
 * Tata Elxsi India Ltd.
 *
 ************************************************************************ 
 *
 * History: 2004.12.10
 *          Modified from the original to work properly with Motif 2.x,
 *          and much modified to fix many errors and provide more
 *          functionality and resources.
 *
 **************************************************************************/

#include <stdio.h>
#include <string.h>
#include <Xm/XmP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/CharSet.h>
#include "XmpDoubleSSP.h"

/****** For Convienience ******/

typedef unsigned int UINT;

/******** For easier understanding *********/

#define TROF(w) 			(w)->composite.children[0]
#define TROF_HEIGHT(w)		TROF(w)->core.height
#define TROF_WIDTH(w)		TROF(w)->core.width
#define TROF_X(w)			TROF(w)->core.x
#define TROF_Y(w)			TROF(w)->core.y
#define FOREGROUND_GC(w)	(w)->dsscale.foreground_GC
#define BETWEEN_GC(w) 		(w)->dsscale.within_limits_GC	/* GC to use for the slot between sliders */
#define OUTSIDE_GC(w)		(w)->dsscale.outside_limits_GC	/* GC for use in slot between the sliders */

/****** Keys for default_value() *******/

#define CORE_HEIGHT		1
#define CORE_LENGTH		2
#define SLIDER_HEIGHT	3
#define SLIDER_LENGTH	4
#define SLOT_RATIO		5

/****** Multi-use messages ******/
const String pdmsg = "Invalid processing direction";
const String shmsg = "The scale height is greater than window height";
const String swmsg = "The scale width is greater than window width";
const String atmsg = "Invalid alignment type";
const String ssmsg = "Invalid slider style";

/****** Widget Methods *************/

static void Initialize(Widget,Widget,ArgList,Cardinal *);
static void ClassInit(void);
static void ReDisplay(Widget,XExposeEvent *,Region );
static void Destroy(Widget);
static void Resize(Widget);
static Boolean SetValues(Widget,Widget, Widget,ArgList,Cardinal *);
static XtGeometryResult GeometryManager(Widget, XtWidgetGeometry*, XtWidgetGeometry*);
static Boolean cvt_string_to_slider_style(Display*, XrmValuePtr, Cardinal*,
			XrmValuePtr, XrmValuePtr, XtPointer*) ;


/******* Internal Functions *************/

static void redraw_all(XmpDoubleSliderScaleWidget);
static void set_initial_size(XmpDoubleSliderScaleWidget,XmpDoubleSliderScaleWidget,ArgList,Cardinal *);
static void set_hor_init_size(XmpDoubleSliderScaleWidget,ArgList,Cardinal *);
static void set_ver_init_size(XmpDoubleSliderScaleWidget,ArgList,Cardinal *);
static void create_children(XmpDoubleSliderScaleWidget);
static void create_slider_pixmap(XmpDoubleSliderScaleWidget);
static void create_GC(XmpDoubleSliderScaleWidget);
static void copy_manager_fg(XmpDoubleSliderScaleWidget, int, XrmValue*);
static void draw_hor_lower_slider_motion(Widget, XEvent *);
static void draw_ver_lower_slider_motion(Widget, XEvent *);
static void draw_hor_upper_slider_motion(Widget, XEvent *);
static void draw_ver_upper_slider_motion(Widget, XEvent *);
static void draw_slider(XmpDoubleSliderScaleWidget);
static void show_upper_value(XmpDoubleSliderScaleWidget); 
static void show_lower_value(XmpDoubleSliderScaleWidget); 
static void erase_prev_upper_value(XmpDoubleSliderScaleWidget); 
static void erase_prev_lower_value(XmpDoubleSliderScaleWidget); 
static void calculate_hor_slider_positions(XmpDoubleSliderScaleWidget);
static void calculate_ver_slider_positions(XmpDoubleSliderScaleWidget);
static void set_scale_values(XmpDoubleSliderScaleWidget,XmpDoubleSliderScaleWidget,ArgList,Cardinal *);
static void set_new_hor_orientation(XmpDoubleSliderScaleWidget,XmpDoubleSliderScaleWidget);
static void set_new_ver_orientation(XmpDoubleSliderScaleWidget,XmpDoubleSliderScaleWidget);
static Position get_ver_trough_x(XmpDoubleSliderScaleWidget);
static Position get_hor_trough_y(XmpDoubleSliderScaleWidget);
static Boolean if_set(ArgList,Cardinal *,char *);
static void configure_trough(XmpDoubleSliderScaleWidget, ArgList, Cardinal *);
static void change_background_GC(XmpDoubleSliderScaleWidget);
static void erase_hor_lower_slider(XmpDoubleSliderScaleWidget,int);
static void erase_ver_lower_slider(XmpDoubleSliderScaleWidget,int);
static void erase_hor_upper_slider(XmpDoubleSliderScaleWidget,int);
static void erase_ver_upper_slider(XmpDoubleSliderScaleWidget,int);
static void draw_hor_lower_slider(XmpDoubleSliderScaleWidget);
static void draw_ver_lower_slider(XmpDoubleSliderScaleWidget);
static void draw_hor_upper_slider(XmpDoubleSliderScaleWidget);
static void draw_ver_upper_slider(XmpDoubleSliderScaleWidget);
static void check_set_render_table(Widget, int, XrmValue*);

static void Btn1DownEvent   (Widget, XEvent*, String*, Cardinal*);
static void Btn1MotionEvent (Widget, XEvent*, String*, Cardinal*);
static void Btn1UpEvent     (Widget, XEvent*, String*, Cardinal*);

#define offset(field) XtOffsetOf(XmpDoubleSliderScaleRec, field)

static XtResource resources[]={
{
	XmNlowerDragCallback, XmCLowerDragCallback, XtRCallback,
	sizeof(XtPointer), offset(dsscale.lower_drag_callback),
	XtRCallback, NULL
},
{
	XmNupperDragCallback, XmCUpperDragCallback, XtRCallback,
	sizeof(XtPointer), offset(dsscale.upper_drag_callback),
	XtRCallback, NULL
},
{
	XmNlowerValueChangedCallback, XmCLowerValueChangedCallback, XtRCallback,
	sizeof(XtPointer), offset(dsscale.lower_value_changed_callback),
	XtRCallback, NULL
},
{
	XmNupperValueChangedCallback, XmCUpperValueChangedCallback, XtRCallback,
	sizeof(XtPointer), offset(dsscale.upper_value_changed_callback),
	XtRCallback, NULL
},
{
	XmNminimum, XmCMinimum, XmRInt,
	sizeof(int), offset(dsscale.minimum_value),
	XmRImmediate, (XtPointer)0
},
{
	XmNmaximum, XmCMaximum, XmRInt,
	sizeof(int), offset(dsscale.maximum_value),
	XmRImmediate, (XtPointer)100
},
{
	XmNlowerValue, XmCLowerValue, XmRInt,
	sizeof(int), offset(dsscale.lower_value),
	XmRImmediate, (XtPointer)0
},
{
	XmNupperValue, XmCUpperValue, XmRInt,
	sizeof(int), offset(dsscale.upper_value),
	XmRImmediate, (XtPointer)100
},
{
	XmNorientation, XmCOrientation, XmROrientation,
	sizeof(unsigned char), offset(dsscale.orientation),
	XmRImmediate, (XtPointer)XmHORIZONTAL
},
{
	XmNprocessingDirection, XmCProcessingDirection, XmRProcessingDirection,
	sizeof(unsigned char), offset(dsscale.processing_direction),
	XmRImmediate, (XtPointer)XmMAX_ON_RIGHT
},
{
	"pri.vate","Pri.vate", XmRInt,
	sizeof(int), offset(dsscale.font_last_value),
	XmRImmediate, (XtPointer) False
},
{
	XmNfontList, XmCFontList, XmRFontList, 
	sizeof(XmFontList), offset(dsscale.font_list), 
	XmRCallProc, (XtPointer)check_set_render_table
},
{
	XmNrenderTable, XmCRenderTable, XmRRenderTable, 
	sizeof(XmRenderTable), offset(dsscale.font_list), 
	XmRCallProc, (XtPointer)check_set_render_table
},
{
	XmNshowValues, XmCShowValues, XmRBoolean,
	sizeof(Boolean), offset(dsscale.show_values),
	XmRImmediate, (XtPointer)True
},
{
	XmNsnapToValue, XmCSnapToValue, XmRBoolean,
	sizeof(Boolean), offset(dsscale.snap_to_value),
	XmRImmediate, (XtPointer)True
},
{
	XmNvalueAlignment, XmCValueAlignment, XmRValueAlignment,
	sizeof(unsigned char), offset(dsscale.value_alignment),
	XmRImmediate, (XtPointer)XmALIGNMENT_END
},
{
	XmNscaleWidth, XmCScaleWidth, XmRDimension,
	sizeof(Dimension), offset(dsscale.scale_width),
	XmRImmediate, (XtPointer)0
},
{
	XmNscaleHeight, XmCScaleHeight, XmRDimension,
	sizeof(Dimension), offset(dsscale.scale_height),
	XmRImmediate, (XtPointer)0
},
{
	XmNwithinLimitsColor, XmCWithinLimitsColor, XmRPixel,
	sizeof(Pixel), offset(dsscale.within_limits_pixel),
	XmRCallProc, (XtPointer)copy_manager_fg
},
{
	XmNoutsideLimitsColor, XmCOutsideLimitsColor, XmRPixel,
	sizeof(Pixel), offset(dsscale.outside_limits_pixel),
	XmRCallProc, (XtPointer)copy_manager_fg
},
{
	XmNsliderStyle, XmCSliderStyle, XtRSliderStyle,
	sizeof(SliderStyle), offset(dsscale.slider_style),
	XmRImmediate, (XtPointer)XmCROSS_AXIS_RECTANGLE
}
};


/*  Definition for resources that need special processing in get values  */
static XmSyntheticResource syn_resources[] =
{
{ 
	XmNscaleWidth, sizeof(Dimension), offset(dsscale.scale_width), 
	XmeFromHorizontalPixels, XmeToHorizontalPixels 
},
{ 
	XmNscaleHeight, sizeof(Dimension), offset(dsscale.scale_height), 
	XmeFromVerticalPixels, XmeToVerticalPixels
}
};


XmpDoubleSliderScaleClassRec xmpDoubleSliderScaleClassRec = {
{ /***** core class ******/
	(WidgetClass) &xmManagerClassRec,	/* super class */
	"XmpDoubleSliderScale",				/* class name */
	sizeof(XmpDoubleSliderScaleRec),		/* widget size */
	ClassInit,							/* class initialize */
	NULL, 								/* class part initialize */
	False,								/* class inited */
	(XtInitProc)Initialize,				/* initialize */
	NULL,								/* initialize hook */
	XtInheritRealize,					/* realize */
	NULL,								/* actions */
	0,									/* num actions */
	resources, 							/*	resources */
	XtNumber(resources),	 			/* num resources */
	NULLQUARK,							/* xrm class */
	True,								/* compress motion */
	XtExposeCompressMultiple,			/* compress exposure */
	True,								/* compress enter leave */
	False,								/* visible interest */
	(XtWidgetProc)Destroy,				/* destroy */
	(XtWidgetProc)Resize,				/* resize */
	(XtExposeProc)ReDisplay,			/* expose */
	(XtSetValuesFunc)SetValues,			/* set values */
	NULL,								/* set values hook */
	XtInheritSetValuesAlmost,			/* set values almost */
	NULL,								/* get values hook */
	XtInheritAcceptFocus,				/* accept focus */
	XtVersion,							/* version */
	NULL,								/* callback private */
	XtInheritTranslations,				/* tm table */
	XtInheritQueryGeometry,				/* query geometry */
	XtInheritDisplayAccelerator, 		/* display accelerator */
	NULL								/* extension */
},
{ /***** composite class part *****/
	GeometryManager, 					/* XtInheritGeometryManager, geomerty manager */
	XtInheritChangeManaged,				/* change managed */
	XtInheritInsertChild,				/* insert child */
	XtInheritDeleteChild,				/* delete child */
	NULL								/* extension */
},
{ /**** Constraint Class Part *****/
	NULL,								/* constraint resource list	 */
	0,			 						/* number of constraints in list */
	0,									/* size of constraint record		 */
	NULL,								/* constraint initialization	 */
	NULL,								/* constraint destroy proc		 */
	NULL,								/* constraint set_values proc	 */
	NULL								/* pointer to extension record	*/
},
{ /****** Manager Class Part *****/
	XtInheritTranslations,				/* translations */
	syn_resources,						/* syn_resources */
	XtNumber(syn_resources),			/* num_syn_resources */
	NULL,								/* syn_constraint_resources */
	0,									/* num_syn_constraint_resources */
	(XmParentProcessProc)NULL,			/* parent_process */
	NULL								/* extension */
},
{ /****** DoubleSliderScale class part ****/
	0,	 								/* extension */
},
};

WidgetClass xmpDoubleSliderScaleWidgetClass = (WidgetClass) &xmpDoubleSliderScaleClassRec;

static XtActionsRec actions[]={
	{"Btn1DownEvent",   Btn1DownEvent  },
	{"Btn1MotionEvent", Btn1MotionEvent},
	{"Btn1UpEvent",     Btn1UpEvent    }
};

static char translations[]=
"<Btn1Down>: Btn1DownEvent()	\n\
<Btn1Motion>: Btn1MotionEvent()	\n\
<Btn1Up>: Btn1UpEvent() 		\n";



/* Returns default values. It is convienient if all of the defaults are
 * held in one function and can be changed as a group if needed.
 */
static Dimension default_value(XmpDoubleSliderScaleWidget wid, int value_type)
{
	if(wid->dsscale.slider_style == XmALONG_AXIS_RECTANGLE)
	{
		switch(value_type)
		{
			case CORE_HEIGHT:   return 17;
			case CORE_LENGTH:   return 100;
			case SLIDER_HEIGHT: return 13;
			case SLIDER_LENGTH: return 30;
			case SLOT_RATIO:    return 5;
		}
	}
	else if(wid->dsscale.slider_style == XmCROSS_AXIS_RECTANGLE)
	{
		switch(value_type)
		{
			case CORE_HEIGHT:   return 24;
			case CORE_LENGTH:   return 100;
			case SLIDER_HEIGHT: return 20;
			case SLIDER_LENGTH: return 8;
			case SLOT_RATIO:    return 8;
		}
	}
	return 0;
}


/*
 * XmRCallProc routine for checking font_list before setting it to NULL
 * if no value is specified for both XmNrenderTable and XmNfontList.
 * If "last_value" is True, then function has been called twice on same 
 * widget, thus resource needs to be set NULL, otherwise leave it alone.
 */
/* ARGSUSED */
static void check_set_render_table(Widget wid, int offset, XrmValue *value )
{
	XmpDoubleSliderScaleWidget sw = (XmpDoubleSliderScaleWidget)wid;
  
	/* Check if been here before */
	if (sw->dsscale.font_last_value)
	{
		value->addr = NULL;
	}
	else
	{
		sw->dsscale.font_last_value = True;
		value->addr = (char*)&(sw->dsscale.font_list);
	}
}


/* As a default copy the foreground colour of the manager */
/*ARGSUSED*/
static void copy_manager_fg( XmpDoubleSliderScaleWidget sw, int offset, XrmValue *value )
{
	value->addr = (XtPointer) &sw->manager.foreground;
}


/* ARGSUSED */
static void Btn1DownEvent(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w->core.parent;
	int x,y;

	x = event->xbutton.x;
	y = event->xbutton.y;

	wid->dsscale.lower_slider_on = False;
	wid->dsscale.upper_slider_on = False;

	if(wid->dsscale.orientation == XmHORIZONTAL)
	{
		if(x >= wid->dsscale.lower_x && x < (wid->dsscale.lower_x + wid->dsscale.slider_size_x))
		{
			wid->dsscale.offset_x = x - wid->dsscale.lower_x ;
			wid->dsscale.lower_slider_on = True;
		}
		else if(x >= wid->dsscale.upper_x && x < (wid->dsscale.upper_x + wid->dsscale.slider_size_x))
		{
			wid->dsscale.offset_x = x - wid->dsscale.upper_x ;
			wid->dsscale.upper_slider_on = True;
		}
	}
	else if(wid->dsscale.orientation == XmVERTICAL)
	{
		if(y >= wid->dsscale.lower_y && y < (wid->dsscale.lower_y + wid->dsscale.slider_size_y))
		{
			wid->dsscale.offset_y = y - wid->dsscale.lower_y;
			wid->dsscale.lower_slider_on = True;
		}
		else if(y >= wid->dsscale.upper_y && y < (wid->dsscale.upper_y + wid->dsscale.slider_size_y))
		{
			wid->dsscale.offset_y = y - wid->dsscale.upper_y;
			wid->dsscale.upper_slider_on = True;
		}
	}
}



/* ARGSUSED */
static void Btn1MotionEvent(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w->core.parent;
	XmpDoubleSliderScaleCallbackStruct call_data;

	if(wid->dsscale.lower_slider_on)
	{
		if(wid->dsscale.show_values)
			erase_prev_lower_value(wid);

		if(wid->dsscale.orientation == XmHORIZONTAL) 
			draw_hor_lower_slider_motion(w,event);
		else if(wid->dsscale.orientation == XmVERTICAL) 
			draw_ver_lower_slider_motion(w,event);

		if(wid->dsscale.show_values)
			show_lower_value(wid);

		call_data.reason = XmCR_LOWER_DRAG;
		call_data.event  = event;
		call_data.value  = wid->dsscale.lower_value;
		XtCallCallbacks(XtParent(w),XmNlowerDragCallback,&call_data);
	}
	else if(wid->dsscale.upper_slider_on)
	{
		if(wid->dsscale.show_values)
			erase_prev_upper_value(wid);

		if(wid->dsscale.orientation == XmHORIZONTAL)
			draw_hor_upper_slider_motion(w,event);
		else if(wid->dsscale.orientation == XmVERTICAL)
			draw_ver_upper_slider_motion(w,event);

		if(wid->dsscale.show_values)
			show_upper_value(wid);

		call_data.reason = XmCR_UPPER_DRAG;
		call_data.event  = event;
		call_data.value  = wid->dsscale.upper_value;
		XtCallCallbacks(XtParent(w),XmNupperDragCallback,&call_data);
	}
}


/* ARGSUSED */
static void Btn1UpEvent(Widget w, XEvent *event, String *params, Cardinal *nparams)
{
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w->core.parent;
	XmpDoubleSliderScaleCallbackStruct call_data;

	if(wid->dsscale.lower_slider_on)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
			draw_hor_lower_slider_motion(w,event);
		else if(wid->dsscale.orientation == XmVERTICAL)
			draw_ver_lower_slider_motion(w,event);

		call_data.reason = XmCR_LOWER_VALUE_CHANGED;
		call_data.event  = event;
		call_data.value  = wid->dsscale.lower_value;
		XtCallCallbacks(XtParent(w), XmNlowerValueChangedCallback, &call_data);
	}
	else if(wid->dsscale.upper_slider_on)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
			draw_hor_upper_slider_motion(w,event);
		else if(wid->dsscale.orientation == XmVERTICAL)
			draw_ver_upper_slider_motion(w,event);

		call_data.reason = XmCR_UPPER_VALUE_CHANGED;
		call_data.event  = event;
		call_data.value  = wid->dsscale.upper_value;
		XtCallCallbacks(XtParent(w), XmNupperValueChangedCallback, &call_data);
	}

	wid->dsscale.lower_slider_on = False;
	wid->dsscale.upper_slider_on = False;

	if(wid->dsscale.snap_to_value)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
			calculate_hor_slider_positions(wid);
		else if(wid->dsscale.orientation == XmVERTICAL)
			calculate_ver_slider_positions(wid);
		redraw_all(wid);
	}
}



static void draw_hor_lower_slider_motion(Widget w, XEvent *event)
{
	int     x, lo_limit,up_limit, slot_y;
	GC      gc1, gc2;
	float   deno, nomi;
	Boolean recal = False;
	XmpDoubleSliderScaleWidget parent = (XmpDoubleSliderScaleWidget)w->core.parent;

	x = event->xmotion.x;

	if(parent->dsscale.processing_direction == XmMAX_ON_LEFT)
	{
		up_limit = w->core.width - parent->dsscale.slider_size_x + parent->dsscale.offset_x;
		lo_limit = parent->dsscale.upper_x + parent->dsscale.slider_size_x + parent->dsscale.offset_x;
		gc1      = OUTSIDE_GC(parent);
		gc2      = BETWEEN_GC(parent);
	}
	else
	{
		up_limit = parent->dsscale.upper_x - parent->dsscale.slider_size_x + parent->dsscale.offset_x;
		lo_limit = parent->dsscale.offset_x;
		gc1      = BETWEEN_GC(parent);
		gc2      = OUTSIDE_GC(parent);
	}

	slot_y = (int)((float)((int)(w->core.height) - (int)parent->dsscale.slot_thickness)/2.0 + 0.5);

	if(x <= up_limit && x >= lo_limit)
	{
		XCopyArea(XtDisplay(w),
			parent->dsscale.slider,XtWindow(w),
			parent->manager.background_GC,
			0,0,
			parent->dsscale.slider_size_x, parent->dsscale.slider_size_y,
			x - parent->dsscale.offset_x, 0);


		if((x - parent->dsscale.offset_x ) > parent->dsscale.lower_x)
		{
			XClearArea(XtDisplay(w), XtWindow(w), 
				parent->dsscale.lower_x ,0,
				(UINT)(x - parent->dsscale.offset_x - parent->dsscale.lower_x),
				(UINT)parent->dsscale.slider_size_y,
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc2,
				parent->dsscale.lower_x, slot_y,
				(UINT)(x - parent->dsscale.offset_x - parent->dsscale.lower_x),
				(UINT)parent->dsscale.slot_thickness);

		}
		else if((x - parent->dsscale.offset_x ) < parent->dsscale.lower_x)
		{
			XClearArea(XtDisplay(w), XtWindow(w), 
				x + parent->dsscale.slider_size_x - parent->dsscale.offset_x,0,
				(UINT)(parent->dsscale.lower_x - x + parent->dsscale.offset_x),
				(UINT)parent->dsscale.slider_size_y,
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc1,
				x + parent->dsscale.slider_size_x - parent->dsscale.offset_x,
				slot_y,
				(UINT)(parent->dsscale.lower_x - x + parent->dsscale.offset_x),
				(UINT)parent->dsscale.slot_thickness);
		}

		parent->dsscale.lower_x = x - parent->dsscale.offset_x ;
		recal = True;
	}
	else if(x > up_limit && (parent->dsscale.lower_x + parent->dsscale.offset_x) < up_limit )
	{
		parent->dsscale.lower_x = up_limit - parent->dsscale.offset_x;
		redraw_all(parent);
		recal = True;
	}
	else if(x < lo_limit && (parent->dsscale.lower_x + parent->dsscale.offset_x) > lo_limit)
	{
		parent->dsscale.lower_x = lo_limit - parent->dsscale.offset_x;
		redraw_all(parent);
		recal = True;
	}

	if(recal)
	{
		deno = (float)(TROF_WIDTH(parent) - 2*parent->dsscale.slider_size_x);

		if(parent->dsscale.processing_direction == XmMAX_ON_RIGHT)
		{
		 	nomi = (float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value) *
						parent->dsscale.lower_x);
		}
		else if(parent->dsscale.processing_direction == XmMAX_ON_LEFT)
		{
			nomi = (float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value)
				*(TROF_WIDTH(parent) - parent->dsscale.lower_x - parent->dsscale.slider_size_x));
		}
		parent->dsscale.lower_value = (int)(nomi/deno + 0.5) + parent->dsscale.minimum_value;
	}
}



static void draw_ver_lower_slider_motion(Widget w, XEvent *event)
{
	XmpDoubleSliderScaleWidget parent=(XmpDoubleSliderScaleWidget)w->core.parent;
	int y;
	int lo_limit,up_limit;
	GC  gc1, gc2;
	float deno,nomi;
	Boolean recal = False;
	int slot_x;

	y = event->xmotion.y;

	if(parent->dsscale.processing_direction == XmMAX_ON_BOTTOM)
	{
		up_limit = parent->dsscale.upper_y - parent->dsscale.slider_size_y + parent->dsscale.offset_y;
		lo_limit = parent->dsscale.offset_y ;
		gc1      = OUTSIDE_GC(parent);
		gc2      = BETWEEN_GC(parent);
	}
	else
	{
		up_limit = w->core.height - parent->dsscale.slider_size_y + parent->dsscale.offset_y;
		lo_limit = parent->dsscale.upper_y + parent->dsscale.slider_size_y + parent->dsscale.offset_y;
		gc1      = BETWEEN_GC(parent);
		gc2      = OUTSIDE_GC(parent);
	}

	slot_x = (int)((float)((int)(w->core.width) - parent->dsscale.slot_thickness)/2.0 +0.5);

	if(y <= up_limit && y >= lo_limit)
	{
		XCopyArea(XtDisplay(w),
			parent->dsscale.slider,XtWindow(w),
			parent->manager.background_GC,
			0, 0,
			parent->dsscale.slider_size_x, parent->dsscale.slider_size_y,
			0, y - parent->dsscale.offset_y);

		if((y - parent->dsscale.offset_y ) > parent->dsscale.lower_y)
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				0,parent->dsscale.lower_y,
				(UINT)parent->dsscale.slider_size_x, 
				(UINT)(y - parent->dsscale.offset_y - parent->dsscale.lower_y),
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc1,
				slot_x,
				parent->dsscale.lower_y,
				(UINT)parent->dsscale.slot_thickness,
				(UINT)(y - parent->dsscale.offset_y - parent->dsscale.lower_y));
		}
		else if((y - parent->dsscale.offset_y ) < parent->dsscale.lower_y)
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				0,y + parent->dsscale.slider_size_y - parent->dsscale.offset_y,
				(UINT)parent->dsscale.slider_size_x, 
				(UINT)(parent->dsscale.lower_y - y + parent->dsscale.offset_y),
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc2,
				slot_x,
				y + parent->dsscale.slider_size_y - parent->dsscale.offset_y,
				(UINT)parent->dsscale.slot_thickness,
				(UINT)(parent->dsscale.lower_y - y + parent->dsscale.offset_y));
		}
		parent->dsscale.lower_y = y - parent->dsscale.offset_y ;
		recal = True;
	}
	else if(y < lo_limit && (parent->dsscale.lower_y+parent->dsscale.offset_y) > lo_limit)	
	{
		parent->dsscale.lower_y = lo_limit - parent->dsscale.offset_y ;
		redraw_all(parent);
		recal = True;
	}
	else if(y > up_limit && (parent->dsscale.lower_y+parent->dsscale.offset_y) < up_limit)
	{
		parent->dsscale.lower_y = up_limit - parent->dsscale.offset_y ;
		redraw_all(parent);
		recal = True;
	}


	if(recal)
	{
		deno = (float)(TROF_HEIGHT(parent) - 2*parent->dsscale.slider_size_y);

		if(parent->dsscale.processing_direction == XmMAX_ON_TOP)
		{
			nomi =(float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value)
			*(TROF_HEIGHT(parent) - parent->dsscale.lower_y - parent->dsscale.slider_size_y));
		}
		else if(parent->dsscale.processing_direction == XmMAX_ON_BOTTOM)
		{
			nomi = (float)((parent->dsscale.maximum_value 
				- parent->dsscale.minimum_value)*parent->dsscale.lower_y);
		}
		parent->dsscale.lower_value = (int)(nomi/deno +0.5) + parent->dsscale.minimum_value;
	}
}



static void draw_hor_upper_slider_motion(Widget w, XEvent *event)
{
	XmpDoubleSliderScaleWidget parent=(XmpDoubleSliderScaleWidget)w->core.parent;
	int x;
	int lo_limit,up_limit;
	GC  gc1, gc2;
	float nomi,deno;
	Boolean recal = False;
	int slot_y;

	x = event->xmotion.x;

	if(parent->dsscale.processing_direction == XmMAX_ON_LEFT)
	{
		up_limit = parent->dsscale.lower_x - parent->dsscale.slider_size_x + parent->dsscale.offset_x;
		lo_limit = parent->dsscale.offset_x;
		gc1      = OUTSIDE_GC(parent);
		gc2      = BETWEEN_GC(parent);
	}
	else
	{
		up_limit = w->core.width - parent->dsscale.slider_size_x + parent->dsscale.offset_x;
		lo_limit = parent->dsscale.lower_x + parent->dsscale.slider_size_x + parent->dsscale.offset_x;
		gc1      = BETWEEN_GC(parent);
		gc2      = OUTSIDE_GC(parent);
	}

	slot_y = (int)((float)((int)(w->core.height) - (int)parent->dsscale.slot_thickness)/2.0 +0.5);

	if(x <= up_limit && x >= lo_limit)
	{
		XCopyArea(XtDisplay(w),
			parent->dsscale.slider, XtWindow(w),
			parent->manager.background_GC,
			0, 0,
			parent->dsscale.slider_size_x, parent->dsscale.slider_size_y,
			x - parent->dsscale.offset_x, 0);

		if((x - parent->dsscale.offset_x ) > parent->dsscale.upper_x)
		{
		  	XClearArea(XtDisplay(w), XtWindow(w),
				parent->dsscale.upper_x ,0,
				(UINT)(x - parent->dsscale.offset_x - parent->dsscale.upper_x), 
				(UINT)parent->dsscale.slider_size_y,
				False);

		 	XFillRectangle(XtDisplay(w), XtWindow(w),
				gc1,
				parent->dsscale.upper_x, slot_y,
				(UINT)(x - parent->dsscale.offset_x - parent->dsscale.upper_x),
				(UINT)parent->dsscale.slot_thickness);
		}
		else if((x - parent->dsscale.offset_x ) < parent->dsscale.upper_x)
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				x + parent->dsscale.slider_size_x - parent->dsscale.offset_x,0,
				(UINT)(parent->dsscale.upper_x - x + parent->dsscale.offset_x), 
				(UINT)parent->dsscale.slider_size_y,
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc2,
				x + parent->dsscale.slider_size_x - parent->dsscale.offset_x,
				slot_y,
				(UINT)(parent->dsscale.upper_x - x + parent->dsscale.offset_x),
				(UINT)parent->dsscale.slot_thickness);
		}

		parent->dsscale.upper_x = x - parent->dsscale.offset_x ;
		recal = True;
	}
	else if( x > up_limit && (parent->dsscale.upper_x + parent->dsscale.offset_x) < up_limit)	
	{
		parent->dsscale.upper_x = up_limit - parent->dsscale.offset_x ;
		redraw_all(parent);
		recal = True;
	}
	else if( x < lo_limit && (parent->dsscale.upper_x + parent->dsscale.offset_x) > lo_limit)
	{
		parent->dsscale.upper_x = lo_limit - parent->dsscale.offset_x ;
		redraw_all(parent);
		recal = True;
	}

	if(recal)
	{
		deno = (float)(TROF_WIDTH(parent) - 2*parent->dsscale.slider_size_x);

		if(parent->dsscale.processing_direction == XmMAX_ON_RIGHT)
		{
			nomi = (float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value)
				*(parent->dsscale.upper_x - parent->dsscale.slider_size_x)); 

		}
		else if(parent->dsscale.processing_direction == XmMAX_ON_LEFT)
		{
			nomi = (float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value)
				*(TROF_WIDTH(parent) - 2*parent->dsscale.slider_size_x - parent->dsscale.upper_x));

		}
		parent->dsscale.upper_value = (int)(nomi/deno +0.5) + parent->dsscale.minimum_value;
	}
}



static void draw_ver_upper_slider_motion(Widget w, XEvent *event)
{
	XmpDoubleSliderScaleWidget parent = (XmpDoubleSliderScaleWidget)w->core.parent;
	int y;
	int lo_limit,up_limit;
	GC  gc1, gc2;
	float nomi,deno;
	Boolean recal=False;
	int slot_x;

	y = event->xmotion.y;

	if(parent->dsscale.processing_direction == XmMAX_ON_BOTTOM)
	{
		up_limit = w->core.height - parent->dsscale.slider_size_y + parent->dsscale.offset_y;
		lo_limit = parent->dsscale.lower_y + parent->dsscale.slider_size_y +  parent->dsscale.offset_y ;
		gc1      = OUTSIDE_GC(parent);
		gc2      = BETWEEN_GC(parent);
	}
	else
	{
		up_limit = parent->dsscale.lower_y - parent->dsscale.slider_size_y + parent->dsscale.offset_y;
		lo_limit = parent->dsscale.offset_y;
		gc1      = BETWEEN_GC(parent);
		gc2      = OUTSIDE_GC(parent);
	}

	slot_x = (int)((float)((int)(w->core.width) - parent->dsscale.slot_thickness)/2.0 +0.5);

	if(y <= up_limit && y >= lo_limit)
	{
		XCopyArea(XtDisplay(w),
			parent->dsscale.slider,XtWindow(w),
			parent->manager.background_GC,
			0,0,
			parent->dsscale.slider_size_x,parent->dsscale.slider_size_y,
			0,y - parent->dsscale.offset_y);

		if((y - parent->dsscale.offset_y )> parent->dsscale.upper_y)
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				0,parent->dsscale.upper_y,
				(UINT)parent->dsscale.slider_size_x, 
				(UINT)(y - parent->dsscale.offset_y - parent->dsscale.upper_y),
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc2,
				slot_x,
				parent->dsscale.upper_y,
				(UINT)parent->dsscale.slot_thickness,
				(UINT)(y - parent->dsscale.offset_y - parent->dsscale.upper_y));
		}
		else if((y - parent->dsscale.offset_y ) < parent->dsscale.upper_y)
		{
			XClearArea(XtDisplay(w), XtWindow(w),
				0, y + parent->dsscale.slider_size_y - parent->dsscale.offset_y,
				(UINT)parent->dsscale.slider_size_x, 
				(UINT)(parent->dsscale.upper_y - y + parent->dsscale.offset_y),
				False);

			XFillRectangle(XtDisplay(w), XtWindow(w),
				gc1,
				slot_x,
				y + parent->dsscale.slider_size_y - parent->dsscale.offset_y,
				(UINT)parent->dsscale.slot_thickness,
				(UINT)(parent->dsscale.upper_y - y + parent->dsscale.offset_y));

		}
		parent->dsscale.upper_y = y - parent->dsscale.offset_y ;
		recal = True;
	}
	else if(y < lo_limit && (parent->dsscale.upper_y + parent->dsscale.offset_y) > lo_limit)
	{
		parent->dsscale.upper_y = lo_limit - parent->dsscale.offset_y ;
		redraw_all(parent);
		recal = True;
	} 
	else if(y > up_limit && (parent->dsscale.upper_y + parent->dsscale.offset_y) < up_limit)
	{
		parent->dsscale.upper_y = up_limit - parent->dsscale.offset_y ;
		redraw_all(parent);
		recal = True;
	}

	if(recal)
	{
		deno = (float)(TROF_HEIGHT(parent) - 2*parent->dsscale.slider_size_y);

		if(parent->dsscale.processing_direction == XmMAX_ON_TOP)
		{
			nomi = (float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value)
				*(TROF_HEIGHT(parent) - parent->dsscale.upper_y - 2*parent->dsscale.slider_size_y));
		}
		else if(parent->dsscale.processing_direction == XmMAX_ON_BOTTOM)
		{
			nomi = (float)((parent->dsscale.maximum_value - parent->dsscale.minimum_value)
				*(parent->dsscale.upper_y - parent->dsscale.slider_size_y));
		}
		parent->dsscale.upper_value = (int)(nomi/deno +0.5) + parent->dsscale.minimum_value;
	}
}



static void set_font_list(XmpDoubleSliderScaleWidget wid)
{
	if(!wid->dsscale.font_list)
		wid->dsscale.font_list = XmeGetDefaultRenderTable((Widget)wid, XmLABEL_RENDER_TABLE);

	if(wid->dsscale.font_list)
	{
		wid->dsscale.font_list = XmFontListCopy(wid->dsscale.font_list);
	}
	else
	{
		XmFontListEntry entry;
		XFontStruct *fs = XLoadQueryFont(XtDisplay(wid), XmDEFAULT_FONT);
		if(!fs) fs = XLoadQueryFont(XtDisplay(wid), "*");
		entry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONT, fs);
		wid->dsscale.font_list = XmFontListAppendEntry(NULL, entry);
		XmFontListEntryFree(&entry);
		XFreeFont(XtDisplay(wid), fs);
	}
}


static void Initialize(Widget treq, Widget tnew, ArgList args, Cardinal *nargs)
{
	XmpDoubleSliderScaleWidget wid   = (XmpDoubleSliderScaleWidget)tnew;
	XmpDoubleSliderScaleWidget rqwid = (XmpDoubleSliderScaleWidget)treq;

	set_font_list(wid);
	create_children(wid);
	set_initial_size(wid, rqwid, args, nargs);
	create_GC(wid);
	change_background_GC(wid);
	create_slider_pixmap(wid);
}


static void ClassInit(void)
{
	 XtSetTypeConverter( XtRString, XtRSliderStyle, cvt_string_to_slider_style,
		NULL, 0, XtCacheNone, (XtDestructor)NULL);
}


static void create_children(XmpDoubleSliderScaleWidget wid)
{
	XtTranslations	child_translations;
	Arg arg[2];
	int i;

	i=0;
	XtSetArg(arg[i], XmNbackground,wid->core.background_pixel);i++;
	XtSetArg(arg[i], XmNhighlightThickness, 0);i++;
	XtCreateManagedWidget("child1", xmPrimitiveWidgetClass, (Widget)wid, arg, i);

	XtAppAddActions(XtWidgetToApplicationContext(TROF(wid)),actions,XtNumber(actions));
	child_translations = XtParseTranslationTable(translations);
	XtOverrideTranslations(TROF(wid), child_translations);
}


static void set_initial_size(XmpDoubleSliderScaleWidget nw, XmpDoubleSliderScaleWidget rw,
								ArgList args, Cardinal *nargs)
{
	if(	nw->dsscale.orientation != XmHORIZONTAL && 
		nw->dsscale.orientation != XmVERTICAL     )
	{
		XtWarning("Invalid orientation-type");
		nw->dsscale.orientation = XmHORIZONTAL;
	} 

	if (rw->core.width == 0)
	{
	    if (nw->dsscale.orientation == XmHORIZONTAL)
		{
			nw->core.width += default_value(nw,CORE_LENGTH);
		}
	    else
		{
			nw->core.width += default_value(nw,CORE_HEIGHT);
			if(nw->dsscale.show_values)
			{
				XmString xms;
				char buf[40];
				(void) sprintf(buf,"%d",nw->dsscale.maximum_value);
				xms = XmStringCreateLocalized(buf);
				nw->core.width += XmStringWidth(nw->dsscale.font_list, xms);
				XmStringFree(xms);
			}
		}
	}

    if (rw->core.height == 0)
	{
	    if (nw->dsscale.orientation == XmHORIZONTAL)
		{
			nw->core.height += default_value(nw,CORE_HEIGHT);
			if(nw->dsscale.show_values)
			{
				XmString xms = XmStringCreateLocalized("1");
				nw->core.height += XmStringHeight(nw->dsscale.font_list, xms);
				XmStringFree(xms);
			}
		}
	    else
		{
			nw->core.height += default_value(nw,CORE_LENGTH);
		}
	}

	set_scale_values(nw, rw, args, nargs);

	switch(nw->dsscale.orientation)
	{
		case XmHORIZONTAL: set_hor_init_size(nw,args,nargs); break;
		case XmVERTICAL:   set_ver_init_size(nw,args,nargs); break;
	}
}



static void set_hor_init_size(XmpDoubleSliderScaleWidget nw, ArgList args, Cardinal *nargs)
{
	Position y_co;

	/* processing direction */ 
	if(	nw->dsscale.processing_direction != XmMAX_ON_RIGHT && 
		nw->dsscale.processing_direction != XmMAX_ON_LEFT     )
	{
		if(if_set(args, nargs, XmNprocessingDirection)) XtWarning(pdmsg);
		nw->dsscale.processing_direction = XmMAX_ON_RIGHT;
	}

	/* scale */
	if(nw->dsscale.scale_height <= 0)
	{
		nw->dsscale.scale_height = default_value(nw, SLIDER_HEIGHT);
	}
	else if(nw->dsscale.scale_height > nw->core.height)
	{
		XtWarning(shmsg);
		nw->dsscale.scale_height = nw->core.height;
	}

	nw->dsscale.slider_thickness = nw->dsscale.scale_height;
	nw->dsscale.slot_thickness   =
		(int)((float)nw->dsscale.slider_thickness/(float)default_value(nw,SLOT_RATIO) + 0.5);

	if(nw->dsscale.slot_thickness == 0) nw->dsscale.slot_thickness = 1;

	/* trough */
	TROF_HEIGHT(nw) = nw->dsscale.slider_thickness;

	if(	if_set(args,nargs,XmNscaleWidth) &&
		nw->dsscale.scale_width <= nw->core.width)
			TROF_WIDTH(nw) = nw->dsscale.scale_width;
	else 
			TROF_WIDTH(nw) = nw->core.width;

	TROF_X(nw) = (Position)( (nw->core.width - TROF_WIDTH(nw))/2.0 + 0.5) ;

	nw->dsscale.slider_length = default_value(nw,SLIDER_LENGTH);

	if(	nw->dsscale.value_alignment != XmALIGNMENT_BEGINNING &&
		nw->dsscale.value_alignment != XmALIGNMENT_CENTER    &&
		nw->dsscale.value_alignment != XmALIGNMENT_END         )
	{
		XtWarning(atmsg);
		nw->dsscale.value_alignment = XmALIGNMENT_END;
	}

	/* must be after setting alignment */
	y_co = get_hor_trough_y(nw);
	TROF_Y(nw) = y_co;

	/* end trough */
	nw->dsscale.slider_size_x = nw->dsscale.slider_length;
	nw->dsscale.slider_size_y = nw->dsscale.slider_thickness;

	calculate_hor_slider_positions(nw);
}



static Position get_hor_trough_y(XmpDoubleSliderScaleWidget nw)
{
	Position y_co;
	Dimension font_height;
	XmString xmstring;

	if(nw->dsscale.value_alignment == XmALIGNMENT_BEGINNING)
	{
		xmstring = XmStringCreateLocalized("1");
		font_height = XmStringHeight(nw->dsscale.font_list, xmstring);
		XmStringFree(xmstring);

		y_co = (Position)(font_height + 5);
		if(y_co < 0) y_co = 0;
	}
	else if(nw->dsscale.value_alignment == XmALIGNMENT_CENTER)
	{
		y_co = (Position)((nw->core.height - nw->dsscale.slider_thickness)/2.0 + 0.5);
	}
	else if(nw->dsscale.value_alignment == XmALIGNMENT_END)
	{
		y_co = (Position)(nw->core.height - nw->dsscale.slider_thickness - 5);
		if(y_co < 0) y_co = 0;
	}

	return(y_co);
}



static void calculate_hor_slider_positions(XmpDoubleSliderScaleWidget nw)
{
	float value_span;
	float pixel_span;
	float l_nomi,u_nomi;

	value_span = (float)(nw->dsscale.maximum_value - nw->dsscale.minimum_value);
	pixel_span = (float)(TROF_WIDTH(nw) - 2*nw->dsscale.slider_size_x);

	if(nw->dsscale.processing_direction == XmMAX_ON_RIGHT)
	{
		l_nomi = pixel_span * (nw->dsscale.lower_value - nw->dsscale.minimum_value);
		nw->dsscale.lower_x = (int)(l_nomi/value_span +0.5);
	
		u_nomi = pixel_span*(nw->dsscale.upper_value - nw->dsscale.minimum_value);
		nw->dsscale.upper_x = (int)(u_nomi/value_span +0.5) + nw->dsscale.slider_size_x;
	}
	else if(nw->dsscale.processing_direction == XmMAX_ON_LEFT)
	{
		l_nomi = pixel_span*(nw->dsscale.lower_value - nw->dsscale.minimum_value);
		nw->dsscale.lower_x = TROF_WIDTH(nw) -
								(int)(l_nomi/value_span +0.5) - nw->dsscale.slider_size_x;			

		u_nomi = pixel_span*(nw->dsscale.upper_value - nw->dsscale.minimum_value);
		nw->dsscale.upper_x = TROF_WIDTH(nw) -
								(int)(u_nomi/value_span+0.5) - 2*nw->dsscale.slider_size_x;			
	}
}



static void set_ver_init_size(XmpDoubleSliderScaleWidget nw,ArgList args,Cardinal *nargs)
{
	Position x_co;

	/* processing direction */
	if(	nw->dsscale.processing_direction != XmMAX_ON_TOP    &&
		nw->dsscale.processing_direction != XmMAX_ON_BOTTOM   )
	{
		if(if_set(args, nargs, XmNprocessingDirection)) XtWarning(pdmsg);
		nw->dsscale.processing_direction=XmMAX_ON_TOP;
	}

	/* scale */
	if(nw->dsscale.scale_width <= 0)
	{
		nw->dsscale.scale_width = default_value(nw, SLIDER_HEIGHT);
	}
	else if(nw->dsscale.scale_width > nw->core.width)
	{
		XtWarning(swmsg);
		nw->dsscale.scale_width = nw->core.width;
	}

	nw->dsscale.slider_thickness = nw->dsscale.scale_width;
	nw->dsscale.slot_thickness   =
		(int)((float)nw->dsscale.slider_thickness/(float)default_value(nw,SLOT_RATIO) + 0.5);

	if(nw->dsscale.slot_thickness == 0) nw->dsscale.slot_thickness = 1;

	/* trough */
	TROF_WIDTH(nw) = nw->dsscale.slider_thickness;

	if(	nw->dsscale.value_alignment != XmALIGNMENT_BEGINNING && 
		nw->dsscale.value_alignment != XmALIGNMENT_CENTER    &&
		nw->dsscale.value_alignment != XmALIGNMENT_END         )
	{
		XtWarning(atmsg);
		nw->dsscale.value_alignment = XmALIGNMENT_END;
	}
	
	x_co = get_ver_trough_x(nw);

	TROF_X(nw) = x_co;

	if(if_set(args,nargs,XmNscaleHeight) && nw->dsscale.scale_height <= nw->core.height)
		TROF_HEIGHT(nw) = nw->dsscale.scale_height;
	else
		TROF_HEIGHT(nw) = nw->core.height;

	TROF_Y(nw) = (Position)((nw->core.height - TROF_HEIGHT(nw))/2.0 + 0.5);

	nw->dsscale.slider_length = default_value(nw,SLIDER_LENGTH);

	nw->dsscale.slider_size_x = nw->dsscale.slider_thickness;
	nw->dsscale.slider_size_y = nw->dsscale.slider_length;

	calculate_ver_slider_positions(nw);
}



static Position get_ver_trough_x(XmpDoubleSliderScaleWidget nw)
{
	Position x_co;
	Dimension font_width;
	XmString xmstring;
	char string[10];


	if(nw->dsscale.value_alignment == XmALIGNMENT_BEGINNING)
	{
		(void) sprintf(string,"%d",nw->dsscale.maximum_value);
		xmstring = XmStringCreateLocalized(string);
		font_width = XmStringWidth(nw->dsscale.font_list, xmstring);
		XmStringFree(xmstring);

		x_co = (Position)(font_width + 5);
		if(x_co > nw->core.width - nw->dsscale.slider_thickness)
			x_co = (Position)(nw->core.width - nw->dsscale.slider_thickness);
	}
	else if(nw->dsscale.value_alignment == XmALIGNMENT_CENTER)
	{
		x_co = (Position)((nw->core.width - nw->dsscale.slider_thickness)/2.0 +0.5);
	}
	else if(nw->dsscale.value_alignment == XmALIGNMENT_END)
	{
		x_co = (Position)(nw->core.width - nw->dsscale.slider_thickness - 5);
	}

	return(x_co); 
}



static void calculate_ver_slider_positions(XmpDoubleSliderScaleWidget nw)
{
	float value_span;
	float pixel_span;
	float l_nomi,u_nomi;

	value_span = (float)(nw->dsscale.maximum_value - nw->dsscale.minimum_value);
	pixel_span = (float)(TROF_HEIGHT(nw) - 2*nw->dsscale.slider_length);


	if(nw->dsscale.processing_direction == XmMAX_ON_TOP)
	{
		l_nomi = pixel_span*(nw->dsscale.lower_value - nw->dsscale.minimum_value);
		nw->dsscale.lower_y = TROF_HEIGHT(nw) -
								(int)(l_nomi/value_span +0.5) - nw->dsscale.slider_size_y;
	
		u_nomi = pixel_span*(nw->dsscale.upper_value - nw->dsscale.minimum_value);
		nw->dsscale.upper_y = TROF_HEIGHT(nw) -
								(int)(u_nomi/value_span +0.5) - 2*nw->dsscale.slider_size_y;
	}
	else if(nw->dsscale.processing_direction == XmMAX_ON_BOTTOM)
	{
		l_nomi = pixel_span*(nw->dsscale.lower_value - nw->dsscale.minimum_value);
		nw->dsscale.lower_y = (int)(l_nomi/value_span +0.5);

		u_nomi = pixel_span*(nw->dsscale.upper_value - nw->dsscale.minimum_value);
		nw->dsscale.upper_y = (int)(u_nomi/value_span +0.5) + nw->dsscale.slider_size_y ;
	}
}



static void show_lower_value(XmpDoubleSliderScaleWidget nw)
{
	Position x_lo,y_lo;
	Dimension height, width_lo;
	XmString val_lo;
	char str_lo[10];

	if(!(XtIsRealized((Widget)nw))) return;

	(void) sprintf(str_lo,"%d",nw->dsscale.lower_value);

	val_lo = XmStringCreateLocalized(str_lo);

	height = XmStringHeight(nw->dsscale.font_list, val_lo);

	width_lo = XmStringWidth(nw->dsscale.font_list, val_lo); 

	if(nw->dsscale.orientation == XmHORIZONTAL)
	{
		if(nw->dsscale.processing_direction == XmMAX_ON_RIGHT)
		{
			x_lo = (Position)(TROF_X(nw) + nw->dsscale.lower_x + nw->dsscale.slider_size_x - width_lo);
		}
		else if(nw->dsscale.processing_direction == XmMAX_ON_LEFT)
		{
			x_lo = (Position)(TROF_X(nw) + nw->dsscale.lower_x);
		}

		y_lo = (Position)(TROF_Y(nw) - height);
	}
	else if(nw->dsscale.orientation == XmVERTICAL)
	{
		if(nw->dsscale.processing_direction == XmMAX_ON_TOP)
		{
			y_lo = (Position)(TROF_Y(nw) + nw->dsscale.lower_y);
		}

		else if(nw->dsscale.processing_direction == XmMAX_ON_BOTTOM)
		{
			y_lo = (Position)(TROF_Y(nw) + nw->dsscale.lower_y + nw->dsscale.slider_size_y - height);
		}

		x_lo = (Position)(TROF_X(nw) - width_lo);
	}

	XmStringDraw(XtDisplay((Widget)nw), XtWindow((Widget)nw),
		nw->dsscale.font_list, 
		val_lo,
		FOREGROUND_GC(nw), 
		x_lo, y_lo, width_lo, 
		XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);

	XmStringFree(val_lo);

	nw->dsscale.show_lower_value_x = x_lo;
	nw->dsscale.show_lower_value_y = y_lo;
}



static void erase_prev_lower_value(XmpDoubleSliderScaleWidget nw)
{
	Dimension width_lo;
	XmString val_lo;
	char str_lo[10];

	if(!(XtIsRealized((Widget)nw))) return;

	(void) sprintf(str_lo,"%d",nw->dsscale.lower_value);

	val_lo = XmStringCreateLocalized(str_lo);

	width_lo = XmStringWidth(nw->dsscale.font_list, val_lo); 

	XmStringDraw(XtDisplay((Widget)nw), XtWindow((Widget)nw),
		nw->dsscale.font_list, 
		val_lo,
		nw->manager.background_GC, 
		nw->dsscale.show_lower_value_x, nw->dsscale.show_lower_value_y, width_lo,
		XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);

	XmStringFree(val_lo);
}



static void show_upper_value(XmpDoubleSliderScaleWidget nw)
{
	Position x_up,y_up;
	Dimension height, width_up;
	XmString val_up;
	char str_up[10];

	if(!(XtIsRealized((Widget)nw))) return;

	(void) sprintf(str_up,"%d",nw->dsscale.upper_value);

	val_up = XmStringCreateLocalized(str_up);

	height = XmStringHeight(nw->dsscale.font_list, val_up);

	width_up = XmStringWidth(nw->dsscale.font_list, val_up); 

	if(nw->dsscale.orientation == XmHORIZONTAL)
	{
		if(nw->dsscale.processing_direction == XmMAX_ON_RIGHT)
		{
			x_up = (Position)(TROF_X(nw) + nw->dsscale.upper_x);
		}
		else if(nw->dsscale.processing_direction == XmMAX_ON_LEFT)
		{
			x_up = (Position)(TROF_X(nw) + nw->dsscale.upper_x + nw->dsscale.slider_size_x - width_up);
		}

		y_up = (Position)(TROF_Y(nw) - height);
	}
	else if(nw->dsscale.orientation == XmVERTICAL)
	{
		if(nw->dsscale.processing_direction == XmMAX_ON_TOP)
		{
			y_up = (Position)(TROF_Y(nw) + nw->dsscale.upper_y + nw->dsscale.slider_size_y - height);
		}

		else if(nw->dsscale.processing_direction == XmMAX_ON_BOTTOM)
		{
			y_up = (Position)(TROF_Y(nw) + nw->dsscale.upper_y);
		}

		x_up = (Position)(TROF_X(nw) - width_up);
	}

	XmStringDraw(XtDisplay((Widget)nw), XtWindow((Widget)nw),
		nw->dsscale.font_list, 
		val_up,
		FOREGROUND_GC(nw), 
		x_up, y_up, width_up, 
		XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);

	XmStringFree(val_up);

	nw->dsscale.show_upper_value_x = x_up;
	nw->dsscale.show_upper_value_y = y_up;
}



/* An erase is done by overwriting the string background colour */
static void erase_prev_upper_value(XmpDoubleSliderScaleWidget nw)
{
	Dimension width_up;
	XmString val_up;
	char str_up[10];

	if(!(XtIsRealized((Widget)nw))) return;

	(void) sprintf(str_up,"%d",nw->dsscale.upper_value);

	val_up = XmStringCreateLocalized(str_up);

	width_up = XmStringWidth(nw->dsscale.font_list, val_up); 

	XmStringDraw(XtDisplay((Widget)nw), XtWindow((Widget)nw),
		nw->dsscale.font_list, 
		val_up,
		nw->manager.background_GC, 
		nw->dsscale.show_upper_value_x, nw->dsscale.show_upper_value_y,width_up,
		XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);

	XmStringFree(val_up);
}



static void create_slider_pixmap(XmpDoubleSliderScaleWidget wid)
{
	int screen_number;

	screen_number = DefaultScreen(XtDisplay((Widget)wid));

	wid->dsscale.slider = XCreatePixmap(XtDisplay((Widget)wid),
							RootWindow(XtDisplay((Widget)wid),screen_number),
							wid->dsscale.slider_size_x,
							wid->dsscale.slider_size_y,
							(int)wid->core.depth);
	draw_slider(wid);
}


static void draw_vertical_stripes(XmpDoubleSliderScaleWidget wid, int number)
{
	int line_pos, ii;
	Dimension shadow_thickness;

	shadow_thickness = (Dimension)((wid->manager.shadow_thickness > 0)?wid->manager.shadow_thickness:1);

	line_pos = (int)((float)(wid->dsscale.slider_size_x - 5*number + 3)/2.0 + 0.5);
		
	for(ii = 0; ii < number; ii++)
	{
		XDrawLine(XtDisplay((Widget)wid),wid->dsscale.slider,
			wid->manager.top_shadow_GC,
			line_pos,(int)shadow_thickness,
			line_pos,(int)(wid->dsscale.slider_size_y - 2*shadow_thickness));
		line_pos++;

		XDrawLine(XtDisplay((Widget)wid),wid->dsscale.slider,
			wid->manager.bottom_shadow_GC,
			line_pos,(int)shadow_thickness,
			line_pos,(int)(wid->dsscale.slider_size_y - 2*shadow_thickness));
		line_pos+=3;
	}
}


static void draw_horizontal_stripes(XmpDoubleSliderScaleWidget wid, int number)
{
	int line_pos, ii;
	Dimension shadow_thickness;

	shadow_thickness = (Dimension)((wid->manager.shadow_thickness > 0)?wid->manager.shadow_thickness:1);

	line_pos = (int)((float)(wid->dsscale.slider_size_y - 5*number + 3)/2.0 + 0.5);
		
	for(ii = 0;ii < number; ii++)
	{
		XDrawLine(XtDisplay((Widget)wid),wid->dsscale.slider,
			wid->manager.top_shadow_GC,
			(int)shadow_thickness, line_pos,
			(int)(wid->dsscale.slider_size_x - 2*shadow_thickness), line_pos);
		line_pos++;

		XDrawLine(XtDisplay((Widget)wid),wid->dsscale.slider,
			wid->manager.bottom_shadow_GC,
			(int)shadow_thickness, line_pos,
			(int)(wid->dsscale.slider_size_x - 2*shadow_thickness), line_pos);
		line_pos+=3;
	}
}


static void draw_slider(XmpDoubleSliderScaleWidget wid)
{
	XPoint point[5];
	Dimension shadow_thickness;

	shadow_thickness = (Dimension)((wid->manager.shadow_thickness > 0)?wid->manager.shadow_thickness:1);

	XFillRectangle(XtDisplay((Widget)wid),wid->dsscale.slider,
		wid->manager.background_GC, 0,0,
		(UINT)wid->dsscale.slider_size_x,
		(UINT)wid->dsscale.slider_size_y);

	point[0].x = 0;
	point[0].y = 0;
	point[1].x = wid->dsscale.slider_size_x;
	point[1].y = 0;
	point[2].x = wid->dsscale.slider_size_x - shadow_thickness; 
	point[2].y = shadow_thickness;
	point[3].x = shadow_thickness; 
	point[3].y = shadow_thickness;
	point[4].x = 0;
	point[4].y = 0;

	XFillPolygon(XtDisplay((Widget)wid),wid->dsscale.slider,
		wid->manager.top_shadow_GC,point,5,Convex,CoordModeOrigin);

	point[0].x = 0;
	point[0].y = 0	;
	point[1].x = shadow_thickness ; 
	point[1].y = shadow_thickness ;
	point[2].x = shadow_thickness ; 
	point[2].y = wid->dsscale.slider_size_y - shadow_thickness;
	point[3].x = 0;
	point[3].y = wid->dsscale.slider_size_y;
	point[4].x = 0;
	point[4].y = 0 ;

	XFillPolygon(XtDisplay((Widget)wid),wid->dsscale.slider,
		wid->manager.top_shadow_GC,point,5,Convex,CoordModeOrigin);


	point[0].x = wid->dsscale.slider_size_x;
	point[0].y = 0;
	point[1].x = wid->dsscale.slider_size_x;
	point[1].y = wid->dsscale.slider_size_y;
	point[2].x = wid->dsscale.slider_size_x - shadow_thickness; 
	point[2].y = wid->dsscale.slider_size_y - shadow_thickness;
	point[3].x = wid->dsscale.slider_size_x - shadow_thickness;	
	point[3].y = shadow_thickness;
	point[4].x = wid->dsscale.slider_size_x;
	point[4].y = 0;

	XFillPolygon(XtDisplay((Widget)wid),wid->dsscale.slider,
		wid->manager.bottom_shadow_GC,point,5,Convex,CoordModeOrigin);


	point[0].x = 0;
	point[0].y = wid->dsscale.slider_size_y;
	point[1].x = wid->dsscale.slider_size_x;
	point[1].y = wid->dsscale.slider_size_y;
	point[2].x = wid->dsscale.slider_size_x - shadow_thickness ; 
	point[2].y = wid->dsscale.slider_size_y - shadow_thickness;
	point[3].x = shadow_thickness ; 
	point[3].y = wid->dsscale.slider_size_y - shadow_thickness;
	point[4].x = 0;
	point[4].y = wid->dsscale.slider_size_y ;

	XFillPolygon(XtDisplay((Widget)wid),wid->dsscale.slider,
		wid->manager.bottom_shadow_GC,point,5,Convex,CoordModeOrigin);

	/* fancy lines on slider */
	if(wid->dsscale.slider_style == XmALONG_AXIS_RECTANGLE)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
			draw_vertical_stripes(wid, 3);
		else if(wid->dsscale.orientation == XmVERTICAL)
			draw_horizontal_stripes(wid, 3);
	}
	else if(wid->dsscale.slider_style == XmCROSS_AXIS_RECTANGLE)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
			draw_vertical_stripes(wid, 1);
		else if(wid->dsscale.orientation == XmVERTICAL)
			draw_horizontal_stripes(wid, 1);
	}
}


static void create_GC(XmpDoubleSliderScaleWidget wid)
{
	XtGCMask    mask;
	XGCValues   values;
	XFontStruct *font_struct;

	mask = GCForeground|GCBackground;

	values.background = TROF(wid)->core.background_pixel;

	values.foreground = wid->dsscale.within_limits_pixel;
	BETWEEN_GC(wid)   = XtGetGC((Widget)wid, mask, &values);

	values.foreground = wid->dsscale.outside_limits_pixel;
	OUTSIDE_GC(wid)   = XtGetGC((Widget)wid, mask, &values);

	if( XmeRenderTableGetDefaultFont(wid->dsscale.font_list, &font_struct) )
	{
		mask       |= GCFont;
		values.font = font_struct->fid;
	}

	values.foreground  = wid->manager.foreground;
	FOREGROUND_GC(wid) = XtGetGC((Widget)wid, mask, &values);
}



static void change_background_GC(XmpDoubleSliderScaleWidget wid)
{
	XGCValues   value;
	XFontStruct *font_struct;

	if( XmeRenderTableGetDefaultFont(wid->dsscale.font_list, &font_struct) )
	{
		value.font = font_struct->fid;
		XChangeGC(XtDisplay(wid), wid->manager.background_GC, GCFont, &value);
	}
}


/*ARGSUSED*/
static void ReDisplay(Widget w, XExposeEvent *event, Region region)
{
	if(XtIsRealized(w)) redraw_all((XmpDoubleSliderScaleWidget)w);
}



static void draw_hor_lower_slider(XmpDoubleSliderScaleWidget wid)
{
	XCopyArea(XtDisplay(wid),
		wid->dsscale.slider, XtWindow(TROF(wid)),
		wid->manager.background_GC,
		0, 0,
		wid->dsscale.slider_size_x, wid->dsscale.slider_size_y,
		wid->dsscale.lower_x, 0);
}



static void draw_ver_lower_slider(XmpDoubleSliderScaleWidget wid)
{
	XCopyArea(XtDisplay(wid),
		wid->dsscale.slider, XtWindow(TROF(wid)),
		wid->manager.background_GC,
		0, 0,
		wid->dsscale.slider_size_x, wid->dsscale.slider_size_y,
		0, wid->dsscale.lower_y);
}



static void draw_hor_upper_slider(XmpDoubleSliderScaleWidget wid)
{
	XCopyArea(XtDisplay(wid),
		wid->dsscale.slider, XtWindow(TROF(wid)),
		wid->manager.background_GC,
		0, 0,
		wid->dsscale.slider_size_x, wid->dsscale.slider_size_y,
		wid->dsscale.upper_x, 0);
}



static void draw_ver_upper_slider(XmpDoubleSliderScaleWidget wid)
{
	XCopyArea(XtDisplay(wid),
		wid->dsscale.slider, XtWindow(TROF(wid)),
		wid->manager.background_GC,
		0, 0,
		wid->dsscale.slider_size_x, wid->dsscale.slider_size_y,
		0, wid->dsscale.upper_y);
}


static void redraw_all(XmpDoubleSliderScaleWidget wid)
{
	int x1, x2, x3, y1, y2, y3, w1, w2, w3, h1, h2, h3;

	Widget w = (Widget)wid;

	/* Clear the widget */
	XClearWindow(XtDisplay(w), XtWindow(wid));
	/* Clear the slot */
	XClearWindow(XtDisplay(w), XtWindow(TROF(wid)));

	if( wid->dsscale.orientation == XmVERTICAL )
	{
		if( wid->dsscale.processing_direction == XmMAX_ON_TOP )
		{
			x1 = (int)((float)((int)TROF_WIDTH(wid) - (int)wid->dsscale.slot_thickness)/2.0 + 0.5);
			y1 = 0;
			w1 = wid->dsscale.slot_thickness;
			h1 = wid->dsscale.upper_y + 1;
			x2 = x1;
			y2 = wid->dsscale.upper_y;
			w2 = w1;
			h2 = wid->dsscale.lower_y - wid->dsscale.upper_y + 1;
			x3 = x1;
			y3 = wid->dsscale.lower_y;
			w3 = w1;
			h3 = TROF_HEIGHT(wid) - wid->dsscale.lower_x;
		}
		else
		{
			x1 = (int)((float)((int)TROF_WIDTH(wid) - (int)wid->dsscale.slot_thickness)/2.0 + 0.5);
			y1 = 0;
			w1 = wid->dsscale.slot_thickness;
			h1 = wid->dsscale.lower_y + 1;
			x2 = x1;
			y2 = wid->dsscale.lower_y;
			w2 = w1;
			h2 = wid->dsscale.upper_y - wid->dsscale.lower_y + 1;
			x3 = x1;
			y3 = wid->dsscale.upper_y;
			w3 = w1;
			h3 = TROF_HEIGHT(wid) - wid->dsscale.upper_y;
		}
	}
	else
	{
		if( wid->dsscale.processing_direction == XmMAX_ON_LEFT )
		{
			x1 = 0;
			y1 = (int)((float)((int)TROF_HEIGHT(wid) - (int)wid->dsscale.slot_thickness)/2.0 + 0.5);
			w1 = wid->dsscale.upper_x + 1;
			h1 = wid->dsscale.slot_thickness;
			x2 = wid->dsscale.upper_x;
			y2 = y1;
			w2 = wid->dsscale.lower_x - wid->dsscale.upper_x + 1;
			h2 = h1;
			x3 = wid->dsscale.lower_x;
			y3 = y1;
			w3 = TROF_WIDTH(wid) - wid->dsscale.lower_x;
			h3 = h1;
		}
		else
		{
			x1 = 0;
			y1 = (int)((float)((int)TROF_HEIGHT(wid) - (int)wid->dsscale.slot_thickness)/2.0 + 0.5);
			w1 = wid->dsscale.lower_x + 1;
			h1 = wid->dsscale.slot_thickness;
			x2 = wid->dsscale.lower_x;
			y2 = y1;
			w2 = wid->dsscale.upper_x - wid->dsscale.lower_x + 1;
			h2 = h1;
			x3 = wid->dsscale.upper_x;
			y3 = y1;
			w3 = TROF_WIDTH(wid) - wid->dsscale.upper_x;
			h3 = h1;
		}
	}

	XFillRectangle(XtDisplay(w), XtWindow(TROF(wid)), OUTSIDE_GC(wid), x1, y1, (UINT)w1, (UINT)h1);
	XFillRectangle(XtDisplay(w), XtWindow(TROF(wid)), BETWEEN_GC(wid), x2, y2, (UINT)w2, (UINT)h2);
	XFillRectangle(XtDisplay(w), XtWindow(TROF(wid)), OUTSIDE_GC(wid), x3, y3, (UINT)w3, (UINT)h3);

	/* copy the slider */
	switch(wid->dsscale.orientation)
	{
		case XmHORIZONTAL:
			draw_hor_upper_slider(wid);
			draw_hor_lower_slider(wid);
			break;

		case XmVERTICAL:
			draw_ver_upper_slider(wid);
			draw_ver_lower_slider(wid);
			break;
	}

	if(wid->dsscale.show_values) 
	{
		show_lower_value(wid);
		show_upper_value(wid);
	}
}



static void Destroy(Widget w)
{
	XmpDoubleSliderScaleWidget wid=(XmpDoubleSliderScaleWidget)w;

	XmFontListFree(wid->dsscale.font_list);
	XtRemoveAllCallbacks(w,XmNupperValueChangedCallback);
	XtRemoveAllCallbacks(w,XmNlowerValueChangedCallback);
	XtRemoveAllCallbacks(w,XmNupperDragCallback);
	XtRemoveAllCallbacks(w,XmNlowerDragCallback);
	XFreePixmap(XtDisplay(w), wid->dsscale.slider);
	XtReleaseGC(w, FOREGROUND_GC(wid));
	XtReleaseGC(w, OUTSIDE_GC(wid));
	XtReleaseGC(w, BETWEEN_GC(wid));
	XtDestroyWidget(TROF(wid));
}



static void Resize(Widget w)
{
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w;
	Position y_co,x_co;
	Dimension width, height, slider_length;

	if(wid->dsscale.orientation == XmHORIZONTAL)
	{
		/* trough */
		if(wid->core.width > wid->dsscale.scale_width && wid->dsscale.scale_width != 0)
		{
			width = wid->dsscale.scale_width;	
		}
		else
		{
			width = wid->core.width;
		}

		height = wid->dsscale.slider_thickness;

		x_co = (Position)((wid->core.width - width)/2.0 + 0.5);
		if(x_co < 0) x_co = 0;

		y_co = get_hor_trough_y(wid);
	}
	else if(wid->dsscale.orientation == XmVERTICAL)
	{
		 /* trough */
		if(wid->core.height > wid->dsscale.scale_height && wid->dsscale.scale_height != 0)
		{
			height = wid->dsscale.scale_height;
		}
		else
		{
			height = wid->core.height;
		}

		width = wid->dsscale.slider_thickness;

		x_co = get_ver_trough_x(wid);

		y_co = (Position)((wid->core.height - height)/2.0 + 0.5);
		if(y_co < 0) y_co = 0;

	}

	XtConfigureWidget(TROF(wid), x_co, y_co, width, height, 0);

	if(wid->dsscale.orientation == XmHORIZONTAL)
	{
		slider_length = default_value(wid,SLIDER_LENGTH);
		if( wid->dsscale.slider_length != slider_length)
		{
			wid->dsscale.slider_length = slider_length;
			wid->dsscale.slider_size_x = wid->dsscale.slider_length;
			XFreePixmap(XtDisplay(wid), wid->dsscale.slider);
			create_slider_pixmap(wid);
		}
		calculate_hor_slider_positions(wid);
	}
	else if(wid->dsscale.orientation == XmVERTICAL)
	{
		slider_length = default_value(wid,SLIDER_LENGTH);
		if( wid->dsscale.slider_length != slider_length)
		{
			wid->dsscale.slider_length = slider_length;
			wid->dsscale.slider_size_y = wid->dsscale.slider_length;
			XFreePixmap(XtDisplay(wid), wid->dsscale.slider);
			create_slider_pixmap(wid);
		}
		calculate_ver_slider_positions(wid);
	}

	if(XtIsRealized(w)) redraw_all(wid);
}



/*ARGSUSED*/
static Boolean SetValues(Widget current,Widget request, Widget new,ArgList args,Cardinal *nargs)
{
	XmpDoubleSliderScaleWidget curw = (XmpDoubleSliderScaleWidget)current;
	XmpDoubleSliderScaleWidget neww = (XmpDoubleSliderScaleWidget)new;
	Arg arg[1];
	Position x_co,y_co;
	Boolean redraw;


	/* orientation */
	if(	curw->dsscale.orientation == XmVERTICAL && 
		neww->dsscale.orientation == XmHORIZONTAL)
	{
		set_new_hor_orientation(curw,neww);
		XFreePixmap(XtDisplay((Widget)neww), neww->dsscale.slider);
		create_slider_pixmap(neww);
	}
	else if(curw->dsscale.orientation == XmHORIZONTAL &&
			neww->dsscale.orientation == XmVERTICAL)
	{
		set_new_ver_orientation(curw,neww);
		XFreePixmap(XtDisplay((Widget)neww), neww->dsscale.slider);
		create_slider_pixmap(neww);
	}
	else if(curw->dsscale.orientation == XmVERTICAL && 
			neww->dsscale.orientation != XmVERTICAL && 
			neww->dsscale.orientation != XmHORIZONTAL)
	{
		neww->dsscale.orientation = XmVERTICAL;
	}
	else if(curw->dsscale.orientation == XmHORIZONTAL && 
			neww->dsscale.orientation != XmVERTICAL && 
			neww->dsscale.orientation != XmHORIZONTAL)
	{
		neww->dsscale.orientation = XmHORIZONTAL;
	}

	/* slider style */
	if(neww->dsscale.slider_style != curw->dsscale.slider_style)
	{
		if(	neww->dsscale.slider_style != XmCROSS_AXIS_RECTANGLE && 
			neww->dsscale.slider_style != XmALONG_AXIS_RECTANGLE)
		{
			XtWarning(ssmsg);
			neww->dsscale.slider_style = curw->dsscale.slider_style;
		}
		else
		{
			XFreePixmap(XtDisplay((Widget)neww), neww->dsscale.slider);
			create_slider_pixmap(neww);
		}
	}

	/* scale alignment */
	if(curw->dsscale.value_alignment != neww->dsscale.value_alignment)
	{
		if(	neww->dsscale.value_alignment != XmALIGNMENT_BEGINNING &&
			neww->dsscale.value_alignment != XmALIGNMENT_CENTER    &&
			neww->dsscale.value_alignment != XmALIGNMENT_END)
	  	{
			XtWarning(atmsg);
			neww->dsscale.value_alignment = curw->dsscale.value_alignment;
		}
	
		if(neww->dsscale.orientation == XmHORIZONTAL)
		{
			y_co = get_hor_trough_y(neww);
			XtMoveWidget(TROF(neww), TROF_X(neww), y_co);
		}
		else if(neww->dsscale.orientation == XmVERTICAL)
		{
			x_co = get_ver_trough_x(neww);
			XtMoveWidget(TROF(neww), x_co, TROF_Y(neww));
		}
	}

	/* processing direction */
	if(	neww->dsscale.orientation == XmHORIZONTAL &&
		neww->dsscale.processing_direction != XmMAX_ON_RIGHT && 
		neww->dsscale.processing_direction != XmMAX_ON_LEFT)
	 {
		XtWarning(pdmsg);
		if(curw->dsscale.orientation == XmHORIZONTAL)
			neww->dsscale.processing_direction = curw->dsscale.processing_direction;
		else if(curw->dsscale.orientation == XmVERTICAL)
			neww->dsscale.processing_direction = XmMAX_ON_RIGHT;
	 }
	else if(neww->dsscale.orientation == XmVERTICAL && 
			neww->dsscale.processing_direction != XmMAX_ON_TOP && 
			neww->dsscale.processing_direction != XmMAX_ON_BOTTOM)
	 {
		XtWarning(pdmsg);
		if(curw->dsscale.orientation == XmVERTICAL)
			neww->dsscale.processing_direction = curw->dsscale.processing_direction;
		else if(curw->dsscale.orientation == XmHORIZONTAL)
			neww->dsscale.processing_direction = XmMAX_ON_TOP;
	 }

	/* foreground GC */
	if(neww->manager.foreground != curw->manager.foreground )
	{
		XGCValues   values;
		XFontStruct *font_struct;
		XtGCMask    mask = GCForeground|GCBackground;

		XtReleaseGC((Widget)curw, FOREGROUND_GC(curw));

		values.foreground = neww->manager.foreground;
		values.background = TROF(neww)->core.background_pixel;

		if( XmeRenderTableGetDefaultFont(neww->dsscale.font_list, &font_struct) )
		{
			mask       |= GCFont;
			values.font = font_struct->fid;
		}
		FOREGROUND_GC(neww) = XtGetGC((Widget)neww, mask, &values);
		draw_slider(neww);
	}

	/* background */
	if(neww->core.background_pixel != curw->core.background_pixel)
	{
		XtSetArg(arg[0], XmNbackground, neww->core.background_pixel);
		XtSetValues(TROF(neww),arg,1);
		draw_slider(neww);
	}

	/* outside of slider range GC */
	if(neww->dsscale.outside_limits_pixel != curw->dsscale.outside_limits_pixel )
	{
		XGCValues value;
		value.foreground = neww->dsscale.outside_limits_pixel;
		XChangeGC(XtDisplay(neww), OUTSIDE_GC(neww), GCForeground, &value);
	}

	/* outside of slider range GC */
	if(neww->dsscale.within_limits_pixel != curw->dsscale.within_limits_pixel )
	{
		XGCValues value;
		value.foreground = neww->dsscale.within_limits_pixel;
		XChangeGC(XtDisplay(neww), BETWEEN_GC(neww), GCForeground, &value);
	}


	/* font list */
	if(neww->dsscale.font_list != curw->dsscale.font_list)
	{
		XGCValues   value;
		XFontStruct *font_struct;

		XmFontListFree(curw->dsscale.font_list);
		set_font_list(neww);
		if( XmeRenderTableGetDefaultFont(neww->dsscale.font_list, &font_struct) )
		{
			value.font = font_struct->fid;
			XChangeGC(XtDisplay(neww), neww->manager.background_GC, GCFont, &value);
			XChangeGC(XtDisplay(neww), FOREGROUND_GC(neww), GCFont, &value);
		}
	}

	/* values */
	if(neww->dsscale.minimum_value >= neww->dsscale.maximum_value)
	{
		char buf[80];
		(void) sprintf(buf,
			"The minimum value (%d) is greater than or equal to maximum value (%d)",
			neww->dsscale.minimum_value, neww->dsscale.maximum_value);
		XtWarning(buf);
		neww->dsscale.minimum_value = neww->dsscale.maximum_value -1;
	}

	if(neww->dsscale.upper_value > neww->dsscale.maximum_value)
	{
		char buf[80];
		(void) sprintf(buf,
			"The upper value (%d) is greater than the maximum value (%d)",
			neww->dsscale.upper_value, neww->dsscale.maximum_value);
		XtWarning(buf);
		neww->dsscale.upper_value = neww->dsscale.maximum_value;
	}	

	if(neww->dsscale.upper_value < neww->dsscale.minimum_value)
	{
		char buf[80];
		(void) sprintf(buf,
			"The upper value (%d) is less than the minimum value (%d)",
			neww->dsscale.upper_value, neww->dsscale.minimum_value);
		XtWarning(buf);
		neww->dsscale.upper_value = neww->dsscale.minimum_value;
	}

	if(neww->dsscale.lower_value < neww->dsscale.minimum_value)
	{
		char buf[80];
		(void) sprintf(buf,
			"The lower value (%d) is less than the minimum value (%d)",
			neww->dsscale.lower_value, neww->dsscale.minimum_value);
		XtWarning(buf);
		neww->dsscale.lower_value = neww->dsscale.minimum_value;		
	}		

	if(neww->dsscale.upper_value < neww->dsscale.lower_value)
	{
		char buf[80];
		(void) sprintf(buf,
			"The upper value (%d) is less than the lower value (%d)",
			neww->dsscale.upper_value, neww->dsscale.lower_value);
		XtWarning(buf);
		neww->dsscale.lower_value = neww->dsscale.upper_value;
	}

	/* scale width and height */
	if((neww->dsscale.scale_width != curw->dsscale.scale_width ) 
		|| (neww->dsscale.scale_height != curw->dsscale.scale_height) 
		|| (neww->dsscale.scale_width != curw->dsscale.scale_width 
				&& neww->dsscale.scale_height != curw->dsscale.scale_height))
	{
		configure_trough(neww, args, nargs);
	}

	if(	curw->core.width != neww->core.width  &&
		neww->dsscale.orientation == XmHORIZONTAL)
	{
		calculate_hor_slider_positions(neww);
	}

	if(	curw->core.height != neww->core.height &&
		neww->dsscale.orientation == XmVERTICAL)
	{
		calculate_ver_slider_positions(neww);
	}


	if(XtIsRealized(current)) redraw = True;
	else redraw = False;

	return(redraw); 
}



/*ARGSUSED*/
static void configure_trough(XmpDoubleSliderScaleWidget wid, ArgList args, Cardinal *nargs)
{
	Position y_co,x_co;
	Dimension width, height;

	if(wid->dsscale.orientation == XmHORIZONTAL)
	{
		/* trough */
		if(wid->core.width > wid->dsscale.scale_width && wid->dsscale.scale_width != 0)
			width = wid->dsscale.scale_width;	
		else
			width = wid->core.width;

		if( wid->dsscale.scale_height != 0)
		{
			if(wid->core.height >= wid->dsscale.scale_height)
			{
				height = wid->dsscale.scale_height;
			}
			else
			{
				XtWarning(shmsg);
				height = (Dimension)(wid->core.height);
			}
			wid->dsscale.slider_thickness = height;
		}
		else height = wid->dsscale.slider_thickness;


		x_co = (Position)((wid->core.width - width)/2.0 + 0.5);
		if(x_co < 0) x_co =0;

		y_co = get_hor_trough_y(wid);

		wid->dsscale.slider_size_x = wid->dsscale.slider_length;
		wid->dsscale.slider_size_y = wid->dsscale.slider_thickness;
	}
	else if(wid->dsscale.orientation == XmVERTICAL)
	{
		/* trough */
		if(wid->core.height > wid->dsscale.scale_height && wid->dsscale.scale_height != 0)
			height = wid->dsscale.scale_height;
		else
			height = wid->core.height;

		if(wid->dsscale.scale_width != 0)
		{
			if(wid->core.height >= wid->dsscale.scale_width)
			{
				width = wid->dsscale.scale_width;
			}
			else
			{
				XtWarning(swmsg);
				width = wid->core.width;
			}
			wid->dsscale.slider_thickness = width;
		}
		else width = wid->dsscale.slider_thickness;

		x_co = get_ver_trough_x(wid);

		y_co = (Position)((wid->core.height - height)/2.0 + 0.5);
		if(y_co < 0) y_co =0;

		wid->dsscale.slider_size_x = wid->dsscale.slider_thickness;
		wid->dsscale.slider_size_y = wid->dsscale.slider_length;
	}

	XtConfigureWidget(TROF(wid), x_co, y_co, width, height, 0);

	wid->dsscale.slot_thickness = (int)(wid->dsscale.slider_thickness/2.0 +0.5);
	if(wid->dsscale.slot_thickness == 0) wid->dsscale.slot_thickness =1;


	XFreePixmap(XtDisplay((Widget)wid), wid->dsscale.slider);
	create_slider_pixmap(wid);
}



/*ARGSUSED*/
static void set_scale_values(XmpDoubleSliderScaleWidget nw, XmpDoubleSliderScaleWidget rw,
								ArgList args, Cardinal *nargs)
{
	int i;
	Boolean assigned;

	if(nw->dsscale.minimum_value >= nw->dsscale.maximum_value)
	{
		char buf[80];
		(void) sprintf(buf,
			"The minimum value (%d) is greater than or equal to maximum value (%d)",
			nw->dsscale.minimum_value, nw->dsscale.maximum_value);
		XtWarning(buf);
		nw->dsscale.minimum_value = nw->dsscale.maximum_value - 1;
		nw->dsscale.lower_value   = nw->dsscale.minimum_value;
	}

	/* check whether the values have been set */
	i=0;
	assigned=False;
	while(i < (int)(*nargs) &&  !assigned )
	{
		if(strcmp(args[i].name,XmNupperValue) == 0)
		{
			if(nw->dsscale.upper_value > nw->dsscale.maximum_value)
			{
				char buf[80];
				(void) sprintf(buf,
					"The upper value (%d) is greater than maximum value (%d)",
					nw->dsscale.upper_value, nw->dsscale.maximum_value);
				XtWarning(buf);
				nw->dsscale.upper_value = nw->dsscale.maximum_value;
			}
			else if(nw->dsscale.upper_value < nw->dsscale.minimum_value)
			{
				char buf[80];
				(void) sprintf(buf,
					"The upper value (%d) is less than minimum value (%d)",
					nw->dsscale.upper_value, nw->dsscale.minimum_value);
				XtWarning(buf);
				nw->dsscale.upper_value = nw->dsscale.minimum_value;
			}
			assigned = True;
		}
		i++;
	}
	if(!assigned) nw->dsscale.upper_value = nw->dsscale.maximum_value;


	i=0;
	assigned=False;
	while(i < (int)(*nargs) && !assigned)
	{
		if(strcmp(args[i].name,XmNlowerValue) == 0)
		{
			if(nw->dsscale.lower_value < nw->dsscale.minimum_value)
			{
				char buf[80];
				(void) sprintf(buf,
					"The lower value (%d) is less than minimum value (%d)",
					nw->dsscale.lower_value, nw->dsscale.minimum_value);
				XtWarning(buf);
				nw->dsscale.lower_value = nw->dsscale.minimum_value;
			}
			else if(nw->dsscale.lower_value > nw->dsscale.upper_value)
			{
				char buf[80];
				(void) sprintf(buf,
					"The lower value (%d) is greater than upper value (%d)",
					nw->dsscale.lower_value, nw->dsscale.upper_value);
				XtWarning(buf);
				nw->dsscale.lower_value = nw->dsscale.upper_value;
							}
				assigned = True;
			}
			i++;
	}

	if(!assigned) nw->dsscale.lower_value = nw->dsscale.minimum_value;
}



/*ARGSUSED*/
static void set_new_hor_orientation(XmpDoubleSliderScaleWidget cw,XmpDoubleSliderScaleWidget nw)
{
	Position y_co,x_co;
	Dimension width,height;


	/* processing direction */
	if(	nw->dsscale.processing_direction != XmMAX_ON_RIGHT && 
		nw->dsscale.processing_direction != XmMAX_ON_LEFT)
	{
		nw->dsscale.processing_direction=XmMAX_ON_RIGHT;
	}

	/* width */
	if(nw->core.width < 2*nw->dsscale.slider_length)
	{
		nw->core.width = 2*nw->dsscale.slider_length + 10;
	}

	/* height */
	if(nw->core.height < nw->dsscale.slider_thickness)
	{
		nw->core.height = nw->dsscale.slider_thickness;
	}

	/* trough */
	height = nw->dsscale.slider_thickness;
	width  = nw->core.width;
	x_co   = 0;
	y_co   = get_hor_trough_y(nw);

	XtConfigureWidget(TROF(nw), x_co, y_co, width, height, 0);
			
	/* end trough */
	nw->dsscale.slider_size_x = nw->dsscale.slider_length;
	nw->dsscale.slider_size_y = nw->dsscale.slider_thickness;
}



/*ARGSUSED*/
static void set_new_ver_orientation(XmpDoubleSliderScaleWidget cw,XmpDoubleSliderScaleWidget nw)
{
	Position y_co,x_co;
	Dimension width,height;

	if(	nw->dsscale.processing_direction != XmMAX_ON_TOP && 
		nw->dsscale.processing_direction != XmMAX_ON_BOTTOM)
	{
		nw->dsscale.processing_direction=XmMAX_ON_TOP;
	}

	/* width */
	if(nw->core.width < nw->dsscale.slider_thickness)
	{
		nw->core.width = nw->dsscale.slider_thickness;
	}

	/* height */
	if(nw->core.height < 2*nw->dsscale.slider_length)
	{
		nw->core.height = 2*nw->dsscale.slider_length + 10;
	}

	/* trough */
	width = nw->dsscale.slider_thickness;

	x_co = get_ver_trough_x(nw);

	height = nw->core.height;
	y_co = 5;

	XtConfigureWidget(TROF(nw), x_co, y_co, width, height, 0);

	nw->dsscale.slider_size_x = nw->dsscale.slider_thickness;
	nw->dsscale.slider_size_y = nw->dsscale.slider_length;
}



static Boolean if_set(ArgList args, Cardinal *nargs, char *res)
{
	int i;

	for(i=0; i<(int)(*nargs); i++)
	{
		if(strcmp(args[i].name,res) == 0) return(True);
	}
	return(False);
}



/*ARGSUSED*/
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *prefer)
{

	if(request->request_mode & CWX)
		w->core.x = request->x;

	if(request->request_mode & CWY)
		w->core.y = request->y;

	if(request->request_mode & CWHeight)
		w->core.height = request->height;

	if(request->request_mode & CWWidth)
		w->core.width = request->width;

	if(request->request_mode & CWBorderWidth)
		w->core.border_width = request->border_width;

	return(XtGeometryYes);
}



/* RDP: Note that in the following erase functions the difference between
 * newval and and the current value needs to be done using the slider position
 * and not the upper_value or lower_value values as the actual position of the
 * slider is required, not the corresponding value, as the two can be quite
 * different. Usually there are quite a number of pixel locations between the
 * value positions.
 */
static void erase_hor_lower_slider(XmpDoubleSliderScaleWidget wid, int newval)
{
	int   pixel, slot_y;
	float pixel_range, value_range, val_diff;

	if(!XtIsRealized((Widget)wid)) return;

	if(newval < wid->dsscale.minimum_value) newval = wid->dsscale.minimum_value;
	if(newval > wid->dsscale.upper_value  ) newval = wid->dsscale.upper_value;

	slot_y = (int)((float)((int)TROF_HEIGHT(wid) - (int)wid->dsscale.slot_thickness)/2.0 + 0.5);

	pixel_range = (float)(TROF_WIDTH(wid) - 2*wid->dsscale.slider_size_x);
	value_range = (float)(wid->dsscale.maximum_value - wid->dsscale.minimum_value);
 
	 if(wid->dsscale.processing_direction == XmMAX_ON_RIGHT)
	 {
		val_diff = (float)newval - ((float)wid->dsscale.lower_x * value_range / pixel_range +
					(float)wid->dsscale.minimum_value);

		if(val_diff >= 0.0)
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.lower_x,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				wid->dsscale.lower_x,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
		else 
		{
			pixel = (int)(0.5 + (-val_diff)* pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.lower_x + wid->dsscale.slider_size_x - pixel - 1,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				wid->dsscale.lower_x + wid->dsscale.slider_size_x - pixel - 1,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
	 }
	 else if(wid->dsscale.processing_direction == XmMAX_ON_LEFT)
	 {
		float pos = (float)(TROF_WIDTH(wid) - wid->dsscale.slider_size_x - wid->dsscale.lower_x);
		val_diff = (float)newval - (pos * value_range / pixel_range + (float)wid->dsscale.minimum_value);

		if(val_diff < 0.0)
		{
			pixel = (int)(0.5 + (-val_diff)* pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.lower_x ,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				wid->dsscale.lower_x,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
		else
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.lower_x + wid->dsscale.slider_size_x - pixel - 1,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				wid->dsscale.lower_x + wid->dsscale.slider_size_x - pixel - 1,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
	}
}



static void erase_ver_lower_slider(XmpDoubleSliderScaleWidget wid,int newval)
{
	int   pixel, slot_x;
	float pixel_range, value_range, val_diff;

	if(!XtIsRealized((Widget)wid)) return;

	if(newval < wid->dsscale.minimum_value) newval = wid->dsscale.minimum_value;
	if(newval > wid->dsscale.upper_value  ) newval = wid->dsscale.upper_value;

	slot_x = (int)((float)((int)(TROF_WIDTH(wid)) - wid->dsscale.slot_thickness)/2.0 +0.5);

	pixel_range = (float)(TROF_HEIGHT(wid) - 2*wid->dsscale.slider_size_y);
	value_range = (float)(wid->dsscale.maximum_value - wid->dsscale.minimum_value);

	 if(wid->dsscale.processing_direction == XmMAX_ON_BOTTOM)
	 {
		val_diff = (float)newval - ((float)wid->dsscale.lower_y * value_range / pixel_range +
					(float)wid->dsscale.minimum_value);

		if(val_diff >= 0)
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.lower_y,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				slot_x,
				wid->dsscale.lower_y,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
		else
		{
			pixel = (int)(0.5 + (-val_diff) * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.lower_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				slot_x,
				wid->dsscale.lower_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
	 }
	 else if(wid->dsscale.processing_direction == XmMAX_ON_TOP)
	 {
		float pos = (float)(TROF_HEIGHT(wid) - wid->dsscale.slider_size_y - wid->dsscale.lower_y);
		val_diff = (float)newval - (pos * value_range / pixel_range + (float)wid->dsscale.minimum_value);

		if(val_diff < 0)
		{
			pixel = (int)(0.5 + (-val_diff) * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.lower_y,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				slot_x,
				wid->dsscale.lower_y,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
		else
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.lower_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				slot_x,
				wid->dsscale.lower_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
	}
}



static void erase_hor_upper_slider(XmpDoubleSliderScaleWidget wid, int newval)
{
	int   pixel, slot_y;
	float pixel_range, value_range, val_diff;

	if(!XtIsRealized((Widget)wid)) return;

	if(newval > wid->dsscale.maximum_value) newval = wid->dsscale.maximum_value;
	if(newval < wid->dsscale.lower_value  ) newval = wid->dsscale.lower_value;

	slot_y = (int)((float)((int)(TROF_HEIGHT(wid)) - (int)wid->dsscale.slot_thickness)/2.0 +0.5);

	pixel_range = (float)(TROF_WIDTH(wid) - 2*wid->dsscale.slider_size_x);
	value_range = (float)(wid->dsscale.maximum_value - wid->dsscale.minimum_value);

	 if(wid->dsscale.processing_direction == XmMAX_ON_RIGHT)
	 {
		val_diff = (float)newval - ((float)(wid->dsscale.upper_x - wid->dsscale.slider_size_x) *
					value_range / pixel_range + (float)wid->dsscale.minimum_value);

		if(val_diff >= 0.0)
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.upper_x ,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				wid->dsscale.upper_x,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
		else
		{
			pixel = (int)(0.5 + (-val_diff)* pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.upper_x + wid->dsscale.slider_size_x - pixel - 1, 0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				wid->dsscale.upper_x + wid->dsscale.slider_size_x - pixel - 1,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
	 }
	 else if(wid->dsscale.processing_direction == XmMAX_ON_LEFT)
	 {
		float pos = (float)(TROF_WIDTH(wid) - 2*wid->dsscale.slider_size_x - wid->dsscale.upper_x);
		val_diff = (float)newval - (pos * value_range / pixel_range + (float)wid->dsscale.minimum_value);

		if(val_diff < 0.0)
		{
			pixel = (int)(0.5 + (-val_diff)* pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.upper_x,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				wid->dsscale.upper_x,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);

		}
		else
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				wid->dsscale.upper_x + wid->dsscale.slider_size_x - pixel - 1,0,
				(UINT)pixel+1, (UINT)TROF_HEIGHT(wid),
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				wid->dsscale.upper_x + wid->dsscale.slider_size_x - pixel - 1,
				slot_y,
				(UINT)pixel+1,
				(UINT)wid->dsscale.slot_thickness);
		}
	}
}



static void erase_ver_upper_slider(XmpDoubleSliderScaleWidget wid, int newval)
{
	int   pixel, slot_x;
	float pixel_range, value_range, val_diff;

	if(!XtIsRealized((Widget)wid)) return;

	if(newval > wid->dsscale.maximum_value) newval = wid->dsscale.maximum_value;
	if(newval < wid->dsscale.lower_value  ) newval = wid->dsscale.lower_value;

	slot_x = (int)((float)((int)(TROF_WIDTH(wid)) - wid->dsscale.slot_thickness)/2.0 +0.5);

	pixel_range = (float)(TROF_HEIGHT(wid) - 2*wid->dsscale.slider_size_y);
	value_range = (float)(wid->dsscale.maximum_value - wid->dsscale.minimum_value);

	 if(wid->dsscale.processing_direction == XmMAX_ON_BOTTOM)
	 {
		val_diff = (float)newval - ((float)(wid->dsscale.upper_y - wid->dsscale.slider_size_y) * 
						value_range / pixel_range + (float)wid->dsscale.minimum_value);

		if(val_diff >= 0)
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.upper_y,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				slot_x,
				wid->dsscale.upper_y,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
		else
		{
			pixel = (int)(0.5 + (-val_diff) * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.upper_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				slot_x,
				wid->dsscale.upper_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
	}
	else if(wid->dsscale.processing_direction == XmMAX_ON_TOP)
	{
		float pos = (float)(TROF_HEIGHT(wid) - 2*wid->dsscale.slider_size_y - wid->dsscale.upper_y);
		val_diff = (float)newval - (pos * value_range / pixel_range + (float)wid->dsscale.minimum_value);

		if(val_diff < 0)
		{
			pixel = (int)(0.5 + (-val_diff) * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.upper_y,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				OUTSIDE_GC(wid),
				slot_x,
				wid->dsscale.upper_y,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);

		}
		else
		{
			pixel = (int)(0.5 + val_diff * pixel_range / value_range);

			XClearArea(XtDisplay(wid),XtWindow(TROF(wid)),
				0, wid->dsscale.upper_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)TROF_WIDTH(wid), (UINT)pixel+1,
				False);

			XFillRectangle(XtDisplay(wid), XtWindow(TROF(wid)),
				BETWEEN_GC(wid),
				slot_x,
				wid->dsscale.upper_y + wid->dsscale.slider_size_y - pixel - 1,
				(UINT)wid->dsscale.slot_thickness,
				(UINT)pixel+1);
		}
	}
}


/*
 * Return a non-zero value if the new position of the slider in pixels will
 * be different from the current actual position.
 */
static Boolean have_new_lower_slider_position( XmpDoubleSliderScaleWidget wid, int value )
{
	int  diff, newpos, value_span, pixel_span;

	value_span = wid->dsscale.maximum_value - wid->dsscale.minimum_value;

	if(value > wid->dsscale.upper_value)
		value = wid->dsscale.upper_value;
	else if(value < wid->dsscale.minimum_value)
		value = wid->dsscale.minimum_value;

	if(wid->dsscale.orientation == XmHORIZONTAL)
	{
		pixel_span = TROF_WIDTH(wid) - 2*wid->dsscale.slider_size_x;

		if(wid->dsscale.processing_direction == XmMAX_ON_RIGHT)
			newpos = (value * pixel_span) / value_span;
		else if(wid->dsscale.processing_direction == XmMAX_ON_LEFT)
			newpos = ((wid->dsscale.maximum_value - value) * pixel_span) / value_span +
						wid->dsscale.slider_size_x;

		diff = wid->dsscale.lower_x - newpos;
	}
	else if(wid->dsscale.orientation == XmVERTICAL)
	{
		pixel_span = TROF_HEIGHT(wid) - 2*wid->dsscale.slider_size_y;

		if(wid->dsscale.processing_direction == XmMAX_ON_TOP)
			newpos = ((wid->dsscale.maximum_value - value) * pixel_span) / value_span +
						wid->dsscale.slider_size_y;
		else if(wid->dsscale.processing_direction == XmMAX_ON_BOTTOM)
			newpos = (value * pixel_span) / value_span;

		diff = wid->dsscale.lower_y - newpos;
	}

	return (diff != 0);
}


static Boolean have_new_upper_slider_position( XmpDoubleSliderScaleWidget wid, int value )
{
	int  diff, newpos, value_span, pixel_span;

	if(value > wid->dsscale.maximum_value)
		value = wid->dsscale.maximum_value;
	else if(value < wid->dsscale.lower_value)
		value = wid->dsscale.lower_value;

	value_span = wid->dsscale.maximum_value - wid->dsscale.minimum_value;

	if(wid->dsscale.orientation == XmHORIZONTAL)
	{
		pixel_span = (float)(TROF_WIDTH(wid) - 2*wid->dsscale.slider_size_x);

		if(wid->dsscale.processing_direction == XmMAX_ON_RIGHT)
			newpos = (value * pixel_span) / value_span + wid->dsscale.slider_size_x;
		else if(wid->dsscale.processing_direction == XmMAX_ON_LEFT)
			newpos = ((wid->dsscale.maximum_value - value) * pixel_span) / value_span;

		diff = wid->dsscale.upper_x  - newpos;
	}
	else if(wid->dsscale.orientation == XmVERTICAL)
	{
		pixel_span = TROF_HEIGHT(wid) - 2*wid->dsscale.slider_size_y;

		if(wid->dsscale.processing_direction == XmMAX_ON_TOP)
			newpos = ((wid->dsscale.maximum_value - value) * pixel_span) / value_span;
		else if(wid->dsscale.processing_direction == XmMAX_ON_BOTTOM)
			newpos = (value * pixel_span) / value_span + wid->dsscale.slider_size_y;

		diff = wid->dsscale.upper_y  - newpos;
	}

	return (diff != 0);
}


/******* RESOURCE CONVERSION ************/

/* ARGSUSED */
static Boolean cvt_string_to_slider_style(Display *dpy, XrmValuePtr args, Cardinal *num_args,
					XrmValuePtr fromVal, XrmValuePtr toVal, XtPointer *data)
{
    String str = (String)fromVal->addr ;
    static int slider_style;

	if( XmuCompareISOLatin1(str, "alongAxisRectangle") == 0 )
		slider_style = XmALONG_AXIS_RECTANGLE;
	else if( XmuCompareISOLatin1(str, "crossAxisRectangle") == 0 )
		slider_style = XmCROSS_AXIS_RECTANGLE;
	else {
		XtStringConversionWarning(fromVal->addr, XtRSliderStyle);
		return False ;
	}

	toVal->size = sizeof(slider_style) ;
	if( toVal->addr )
	{
		if( toVal->size < sizeof(slider_style) )
			return False ;
		else
			*((int *)toVal->addr) = slider_style ;
	}
	else
		toVal->addr = (XPointer) &slider_style;

	return True ;
}




/******** PUBLIC FUNCTIONS ****************/

extern void XmpDoubleSliderScaleSetLowerValue(Widget w, int value)
{
	Boolean newpos;
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w; 

	/* Will the new value result in a slider position change? */
	newpos = have_new_lower_slider_position(wid, value);
	
	if(XtIsRealized(w) && newpos)
	{
		if(wid->dsscale.show_values)
			erase_prev_lower_value(wid);

		if(wid->dsscale.orientation == XmHORIZONTAL)
			erase_hor_lower_slider(wid,value);
		else if(wid->dsscale.orientation == XmVERTICAL)
			erase_ver_lower_slider(wid,value);
	}

	if(value < wid->dsscale.minimum_value)
		value = wid->dsscale.minimum_value;
	else if(value > wid->dsscale.upper_value)
		value = wid->dsscale.upper_value;

	wid->dsscale.lower_value = value;

	if(XtIsRealized(w) && newpos)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
		{
			calculate_hor_slider_positions(wid);
			draw_hor_lower_slider(wid);
		}
		else if(wid->dsscale.orientation == XmVERTICAL)
		{
			calculate_ver_slider_positions(wid);
			draw_ver_lower_slider(wid);
		}

		if(wid->dsscale.show_values)
			show_lower_value(wid);
	}
}



extern void XmpDoubleSliderScaleSetUpperValue(Widget w, int value)
{
	Boolean newpos;
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w; 

	/* Will the new value result in a slider position change? */
	newpos = have_new_upper_slider_position(wid, value);

	if(XtIsRealized(w) && newpos)
	{
		if(wid->dsscale.show_values)
			erase_prev_upper_value(wid);

		if(wid->dsscale.orientation == XmHORIZONTAL)
			erase_hor_upper_slider(wid,value);
		else if(wid->dsscale.orientation == XmVERTICAL)
			erase_ver_upper_slider(wid,value);
	}

	if(value > wid->dsscale.maximum_value)
		value = wid->dsscale.maximum_value;
	else if(value < wid->dsscale.lower_value)
		value = wid->dsscale.lower_value;

	wid->dsscale.upper_value = value;

	if(XtIsRealized(w) && newpos)
	{
		if(wid->dsscale.orientation == XmHORIZONTAL)
		{
			calculate_hor_slider_positions(wid);
			draw_hor_upper_slider(wid);
		}
		else if(wid->dsscale.orientation == XmVERTICAL)
		{
			calculate_ver_slider_positions(wid);
			draw_ver_upper_slider(wid);
		}

		if(wid->dsscale.show_values)
			show_upper_value(wid);
	}
}



extern int XmpDoubleSliderScaleGetLowerValue(Widget w)
{
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w; 

	return	wid->dsscale.lower_value;
}



extern int XmpDoubleSliderScaleGetUpperValue(Widget w)
{
	XmpDoubleSliderScaleWidget wid = (XmpDoubleSliderScaleWidget)w; 

	return wid->dsscale.upper_value;
}



extern Widget XmpCreateDoubleSliderScale(Widget parent, String name, ArgList args, int nargs)
{
	return XtCreateWidget(name,xmpDoubleSliderScaleWidgetClass,parent,args,nargs);
}


extern Widget XmpCreateManagedDoubleSliderScale(Widget parent, String name, ArgList args, int nargs)
{
	return XtCreateManagedWidget(name,xmpDoubleSliderScaleWidgetClass,parent,args,nargs);
}


Widget XmpVaCreateDoubleSliderScale( Widget parent, char* name, ... )
{
	Widget w;
	va_list ap;
	int count;

	va_start(ap, name);
	count = XmeCountVaListSimple(ap);
	va_end(ap);

	va_start(ap, name);
	w = XmeVLCreateWidget(name, xmpDoubleSliderScaleWidgetClass, parent, False, ap, count);
	va_end(ap);
	return w;
}


Widget XmpVaCreateManagedDoubleSliderScale( Widget parent, char* name, ... )
{
	Widget w;
	va_list ap;
	int count;

	va_start(ap, name);
	count = XmeCountVaListSimple(ap);
	va_end(ap);

	va_start(ap, name);
	w = XmeVLCreateWidget(name, xmpDoubleSliderScaleWidgetClass, parent, True, ap, count);
	va_end(ap);

	return w;
}

/********************* END OF FILE *********************/
