/*----------------------------------------------------------------------------
 * SciPlot	A generalized plotting widget
 *
 * Copyright (c) 1996 Robert W. McMullen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Rob McMullen <rwmcm@mail.ae.utexas.edu>
 *         http://www.ae.utexas.edu/~rwmcm
 *
 *--------------------------------------------------------------------------
 *
 * July 2013 - Bob Paterson, MRD, Environment Canada
 *
 * - Modified to work only with Motif
 * - XmRenderTable used to handle fonts (compatible with XmFontList but only one
 *   or the other can be specified).
 * - XmString functions used for all text rendering and size information.
 * - Removed internal colour allocation functions and all colour handling done
 *   directly with externally provided pixel values.
 * - Widget creation functions added for convienience.
 * - Removed postscript generation code.
 * - Implemented additional resources but only for cartesian linear plots.
 * - Pixmap used for non-data items to speed up quick drawing.
 * - Added interactive graph modification
 *   See SciPlot.html for documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xm/XmP.h>
#include <Xm/TraitP.h>
#include <Xm/SpecRenderT.h>
#include "SciPlotP.h"


/*
 * Private function declarations
 */
static void    Redisplay       (Widget, XEvent*, Region);
static void    Resize          (Widget);
static Boolean SetValues       (Widget, Widget, Widget, ArgList, Cardinal*);
static void    GetValuesHook   (Widget, ArgList, Cardinal*);
static void    Initialize      (Widget, Widget, ArgList, Cardinal*);
static void    Realize         (Widget, XtValueMask *, XSetWindowAttributes *);
static void    Destroy         (Widget);
static void    ClassInitialize (void);

static void    ComputeAll       (SciPlotWidget);
static void    DrawAll          (SciPlotWidget);
static void    ItemDraw         (SciPlotWidget, SciPlotItem*);
static void    EraseAll         (SciPlotWidget);
static void    InitFonts        (SciPlotWidget);
static void    MotionAP         (SciPlotWidget, XEvent*, String, int);
static void    BtnUpAP          (SciPlotWidget, XEvent*, String, int);
static float   PlotToXValue     (SciPlotWidget, XEvent*);
static float   PlotToYValue     (SciPlotWidget, XEvent*);
static void    CheckRenderTable (Widget, int, XrmValue*); 
static void    PixmapUpdate     (SciPlotWidget);


#define offset(field) XtOffsetOf(SciPlotRec, plot.field)
static XtResource resources[] =
{
	{
		XmNchartType, XtCMargin, XtRInt, sizeof(int),
		offset(ChartType), XtRImmediate, (XtPointer) XmCARTESIAN
	},{
		XmNdegrees, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(Degrees), XtRImmediate, (XtPointer) True
	},{
		XmNdrawMajor, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMajor), XtRImmediate, (XtPointer) True
	},{
		XmNdrawMajorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMajorTics), XtRImmediate, (XtPointer) True
	},{
		XmNdrawMinor, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMinor), XtRImmediate, (XtPointer) True
	},{
		XmNdrawMinorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMinorTics), XtRImmediate, (XtPointer) True
	},{
		XmNshowLegend, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowLegend), XtRImmediate, (XtPointer) True
	},{
		XmNlegendAtBottom, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(LegendAtBottom), XtRImmediate, (XtPointer) False
	},{
		XmNshowTitle, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowTitle), XtRImmediate, (XtPointer) True
	},{
		XmNshowXLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowXLabel), XtRImmediate, (XtPointer) True
	},{
		XmNshowYLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowYLabel), XtRImmediate, (XtPointer) True
	},{
		XmNxLabel, XtCString, XtRString, sizeof(String),
		offset(TransientXLabel), XtRString, "X Axis"
	},{
		XmNyLabel, XtCString, XtRString, sizeof(String),
		offset(TransientYLabel), XtRString, "Y Axis"
	},{
		XmNplotTitle, XtCString, XtRString, sizeof(String),
		offset(TransientPlotTitle), XtRString, "Plot"
	},{
		XmNmarginWidth, XtCMargin, XtRInt, sizeof(int),
		offset(MarginWidth), XtRImmediate, (XtPointer) 5
	},{
		XmNmarginHeight, XtCMargin, XtRInt, sizeof(int),
		offset(MarginHeight), XtRImmediate, (XtPointer) 5
	},{
		XmNtitleMargin, XtCMargin, XtRInt, sizeof(int),
		offset(TitleMargin), XtRImmediate, (XtPointer) 16
	},{
		XmNlegendLineSize, XtCMargin, XtRInt, sizeof(int),
		offset(LegendLineSize), XtRImmediate, (XtPointer) 16
	},{
		XmNdefaultMarkerSize, XtCMargin, XtRInt, sizeof(int),
		offset(DefaultMarkerSize), XtRImmediate, (XtPointer) 3
	},{
		XmNlegendMargin, XtCMargin, XtRInt, sizeof(int),
		offset(LegendMargin), XtRImmediate, (XtPointer) 3
	},{
		XmNlegendThroughPlot, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(LegendThroughPlot), XtRImmediate, (XtPointer) False
	},{
		XmNtitleFontTag, XtCString, XtRString, sizeof(String),
		offset(titleFont.rtag), XtRString, "title"
	},{
		XmNlabelFontTag, XtCString, XtRString, sizeof(String),
		offset(labelFont.rtag), XtRString, "label"
	},{
		XmNaxisFontTag, XtCString, XtRString, sizeof(String),
		offset(axisFont.rtag), XtRString, "axis"
	},{
		XmNlegendFontTag, XtCString, XtRString, sizeof(String),
		offset(legendFont.rtag), XtRString, "legend"
	},{
		XmNxAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(XAutoScale), XtRImmediate, (XtPointer) True
	},{
		XmNyAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YAutoScale), XtRImmediate, (XtPointer) True
	},{
		XmNxAxisNumbers, XmCAxisNumbers, XtRInt, sizeof(int),
		offset(XAxisNumbers), XtRImmediate, (XtPointer) XmNUMBERS_SHOW
	},{
		XmNyAxisNumbers, XmCAxisNumbers, XtRInt, sizeof(int),
		offset(YAxisNumbers), XtRImmediate, (XtPointer) XmNUMBERS_SHOW
	},{
		XmNxAxisMaxNumbers, XtCMargin, XtRInt, sizeof(int),
		offset(XAxisMaxNumbers), XtRImmediate, (XtPointer) 0
	},{
		XmNyAxisMaxNumbers, XtCMargin, XtRInt, sizeof(int),
		offset(YAxisMaxNumbers), XtRImmediate, (XtPointer) 0
	},{
		XmNyAxisNumberNumDigits, XtCValue, XtRInt, sizeof(int),
		offset(YAxisNumberNumDigits), XtRImmediate, (XtPointer) 0
	},{
		XmNxLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(XLog), XtRImmediate, (XtPointer) False
	},{
		XmNyLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YLog), XtRImmediate, (XtPointer) False
	},{
		XmNxOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(XOrigin), XtRImmediate, (XtPointer) False
	},{
		XmNyOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YOrigin), XtRImmediate, (XtPointer) False
	},{
		XmNyNumbersHorizontal, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YNumHorz), XtRImmediate, (XtPointer) True
	},{
		"pri.vate", "Pri.vate", XmRBoolean, sizeof(Boolean),
		offset(check_set_render_table), XmRImmediate, (XtPointer) False
	},{
		XmNfontList, XmCFontList, XmRFontList, sizeof(XmRenderTable),
         offset(RenderTable), XmRCallProc, (XtPointer) CheckRenderTable
	},{
		XmNrenderTable, XmCRenderTable, XmRRenderTable, sizeof(XmRenderTable),
         offset(RenderTable), XmRCallProc, (XtPointer) CheckRenderTable
	},{
		XmNplotCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
         offset(PlotCallback), XmRCallback, (XtPointer) NULL
	}
};


static Boolean CvtStringToAxisNumbers(Display*, XrmValuePtr, Cardinal*, XrmValuePtr, XrmValuePtr, XtPointer*);

static char defaultTranslations[] =
	"<Btn1Down>  : Motion()\n"
	"<Btn1Motion>: Motion()\n"
	"<Btn1Up>    : BtnUp()\n";

static XtActionsRec actions[] = {
	{"Motion", (XtActionProc) MotionAP},
	{"BtnUp",  (XtActionProc) BtnUpAP}
};


SciPlotClassRec sciplotClassRec =
{
	{ /* core_class fields */
		/* superclass               */ (WidgetClass) & xmPrimitiveClassRec,
		/* class_name               */ "SciPlot",
		/* widget_size              */ sizeof(SciPlotRec),
		/* class_initialize         */ ClassInitialize,
		/* class_part_initialize    */ NULL,
		/* class_inited             */ False,
		/* initialize               */ Initialize,
		/* initialize_hook          */ NULL,
		/* realize                  */ Realize,
		/* actions                  */ actions,
		/* num_actions              */ XtNumber(actions),
		/* resources                */ resources,
		/* num_resources            */ XtNumber(resources),
		/* xrm_class                */ NULLQUARK,
		/* compress_motion          */ True,
		/* compress_exposure        */ XtExposeCompressMultiple,
		/* compress_enterleave      */ True,
		/* visible_interest         */ True,
		/* destroy                  */ Destroy,
		/* resize                   */ Resize,
		/* expose                   */ Redisplay,
		/* set_values               */ SetValues,
		/* set_values_hook          */ NULL,
		/* set_values_almost        */ XtInheritSetValuesAlmost,
		/* get_values_hook          */ GetValuesHook,
		/* accept_focus             */ NULL,
		/* version                  */ XtVersion,
		/* callback_private         */ NULL,
		/* tm_table                 */ defaultTranslations,
		/* query_geometry           */ NULL,
		/* display_accelerator      */ XtInheritDisplayAccelerator,
		/* extension                */ NULL
	},
	{ /* primitive_class fields */
		/* border_highlight         */ (XtWidgetProc) _XtInherit,
		/* border_unhighligh        */ (XtWidgetProc) _XtInherit,
		/* translations             */ XtInheritTranslations,
		/* arm_and_activate         */ (XtActionProc) MotionAP,
		/* syn_resources            */ NULL,
		/* num_syn_resources        */ 0,
		/* extension                */ NULL
	},
	{ /* plot_class fields */
		/* dummy                    */ 0
	}
};

WidgetClass sciplotWidgetClass = (WidgetClass) & sciplotClassRec;


static void Initialize(Widget treq, Widget tnew, ArgList args, Cardinal *num)
{
	SciPlotWidget new;

	new = (SciPlotWidget) tnew;

	new->plot.plotlist = NULL;
	new->plot.alloc_plotlist = 0;
	new->plot.num_plotlist = 0;

	new->plot.alloc_drawlist = NUMPLOTITEMALLOC;
	new->plot.drawlist = (SciPlotItem *) XtCalloc(new->plot.alloc_drawlist, sizeof(SciPlotItem));
	new->plot.num_drawlist = 0;

	new->plot.cmap = DefaultColormap(XtDisplay(new), DefaultScreen(XtDisplay(new)));

	new->plot.pix = (Pixmap)0;
	new->plot.pix_width = 0;
	new->plot.pix_height = 0;

	new->plot.xlabel = (char *) XtMalloc(strlen(new->plot.TransientXLabel) + 1);
	strcpy(new->plot.xlabel, new->plot.TransientXLabel);
	new->plot.ylabel = (char *) XtMalloc(strlen(new->plot.TransientYLabel) + 1);
	strcpy(new->plot.ylabel, new->plot.TransientYLabel);
	new->plot.plotTitle = (char *) XtMalloc(strlen(new->plot.TransientPlotTitle) + 1);
	strcpy(new->plot.plotTitle, new->plot.TransientPlotTitle);
	new->plot.TransientXLabel = NULL;
	new->plot.TransientYLabel = NULL;
	new->plot.TransientPlotTitle = NULL;

	new->plot.update = False;
	new->plot.move_on = False;
	new->plot.UserMin.x = new->plot.UserMin.y = 0.0;
	new->plot.UserMax.x = new->plot.UserMax.y = 10.0;

	new->plot.x.Fixed.set = False;
	new->plot.y.Fixed.set = False;

	if(!new->plot.RenderTable)
		new->plot.RenderTable = XmeGetDefaultRenderTable(tnew, XmTEXT_RENDER_TABLE);
	new->plot.RenderTable = XmRenderTableCopy(new->plot.RenderTable, NULL, 0);
	InitFonts(new);
}


static void ClassInitialize(void)
{
	XtSetTypeConverter(XmRString, XmRAxisNumbers, CvtStringToAxisNumbers,
			NULL, 0, XtCacheAll, NULL);
}


/* Note that the X drawing routines need the XCreateGC function but that the Motif
 * XmString functions require the use of XtAllocGC else things don't work.
 */
static void GCInitialize(SciPlotWidget new)
{
	XGCValues values;
	XtGCMask mask;

	values.line_style = LineSolid;
	values.line_width = 0;
	values.fill_style = FillSolid;
	values.background = new->core.background_pixel;
	new->plot.BackgroundColor = new->core.background_pixel;
	values.foreground = new->primitive.foreground;
	new->plot.ForegroundColor = values.foreground;

	mask = GCLineStyle | GCLineWidth | GCFillStyle | GCForeground | GCBackground;
	new->plot.defaultGC = XCreateGC(XtDisplay(new), XtWindow(new), mask, &values);

	values.line_style = LineOnOffDash;
	new->plot.dashGC = XCreateGC(XtDisplay(new), XtWindow(new), mask, &values);

	mask = GCForeground | GCBackground;
	new->plot.textGC = XtAllocateGC((Widget) new, 0, mask, &values, mask, 0);
}

static void Realize(Widget aw, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
	SciPlotWidget w = (SciPlotWidget) aw;
	(*(&widgetClassRec)->core_class.realize) (aw, value_mask, attributes);
	w->plot.draw = XtWindow(aw);
	GCInitialize(w);
	PixmapUpdate(w);
}

static void Destroy(Widget wi)
{
	int i;
	SciPlotWidget w = (SciPlotWidget) wi;

	XtFree((char *) w->plot.xlabel);
	XtFree((char *) w->plot.ylabel);
	XtFree((char *) w->plot.plotTitle);

	for (i = 0; i < w->plot.num_plotlist; i++)
	{
		SciPlotList *p = w->plot.plotlist + i;
		if (p->allocated > 0)
			XtFree((char *) p->data);
		p->data = NULL;

		if (p->legend)
			XtFree(p->legend);
		p->legend = NULL;
	}

	if (w->plot.alloc_plotlist > 0)
		XtFree((char *) w->plot.plotlist);

	EraseAll(w);
	if(w->plot.RenderTable)
		XmRenderTableFree(w->plot.RenderTable);
	XtFree((char *) w->plot.drawlist);
	XFreePixmap(XtDisplay(w), w->plot.pix);
	XFreeGC(XtDisplay(w), w->plot.defaultGC);
	XFreeGC(XtDisplay(w), w->plot.dashGC);
	XtReleaseGC((Widget) w, w->plot.textGC);
}

static Boolean SetValues(Widget oldw, Widget reqw, Widget neww, ArgList args, Cardinal *nargs)
{
	SciPlotWidget old = (SciPlotWidget) oldw;
	SciPlotWidget request = (SciPlotWidget) reqw;
	SciPlotWidget new = (SciPlotWidget) neww;
	Boolean redisplay = False;

	if (old->plot.XLog != new->plot.XLog)
		redisplay = True;
	else if (old->plot.YLog != new->plot.YLog)
		redisplay = True;
	else if (old->plot.XOrigin != new->plot.XOrigin)
		redisplay = True;
	else if (old->plot.YOrigin != new->plot.YOrigin)
		redisplay = True;
	else if (old->plot.XAxisNumbers != new->plot.XAxisNumbers)
		redisplay = True;
	else if (old->plot.YAxisNumbers != new->plot.YAxisNumbers)
		redisplay = True;
	else if (old->plot.DrawMajor != new->plot.DrawMajor)
		redisplay = True;
	else if (old->plot.DrawMajorTics != new->plot.DrawMajorTics)
		redisplay = True;
	else if (old->plot.DrawMinor != new->plot.DrawMinor)
		redisplay = True;
	else if (old->plot.DrawMinorTics != new->plot.DrawMinorTics)
		redisplay = True;
	else if (old->plot.ChartType != new->plot.ChartType)
		redisplay = True;
	else if (old->plot.Degrees != new->plot.Degrees)
		redisplay = True;
	else if (old->plot.ShowLegend != new->plot.ShowLegend)
		redisplay = True;
	else if (old->plot.ShowTitle != new->plot.ShowTitle)
		redisplay = True;
	else if (old->plot.ShowXLabel != new->plot.ShowXLabel)
		redisplay = True;
	else if (old->plot.ShowYLabel != new->plot.ShowYLabel)
		redisplay = True;
	else if (old->plot.ShowTitle != new->plot.ShowTitle)
		redisplay = True;

	if (new->plot.TransientXLabel)
	{
		if (old->plot.TransientXLabel != new->plot.TransientXLabel ||
				strcmp(new->plot.TransientXLabel, old->plot.xlabel) != 0)
		{
			redisplay = True;
			XtFree(old->plot.xlabel);
			new->plot.xlabel = (char *) XtMalloc(strlen(new->plot.TransientXLabel) + 1);
			strcpy(new->plot.xlabel, new->plot.TransientXLabel);
			new->plot.TransientXLabel = NULL;
		}
	}
	if (new->plot.TransientYLabel)
	{
		if (old->plot.TransientYLabel != new->plot.TransientYLabel ||
				strcmp(new->plot.TransientYLabel, old->plot.ylabel) != 0)
		{
			redisplay = True;
			XtFree(old->plot.ylabel);
			new->plot.ylabel = (char *) XtMalloc(strlen(new->plot.TransientYLabel) + 1);
			strcpy(new->plot.ylabel, new->plot.TransientYLabel);
			new->plot.TransientYLabel = NULL;
		}
	}
	if (new->plot.TransientPlotTitle)
	{
		if (old->plot.TransientPlotTitle != new->plot.TransientPlotTitle ||
				strcmp(new->plot.TransientPlotTitle, old->plot.plotTitle) != 0)
		{
			redisplay = True;
			XtFree(old->plot.plotTitle);
			new->plot.plotTitle = (char *) XtMalloc(strlen(new->plot.TransientPlotTitle) + 1);
			strcpy(new->plot.plotTitle, new->plot.TransientPlotTitle);
			new->plot.TransientPlotTitle = NULL;
		}
	}

	if (new->plot.RenderTable != old->plot.RenderTable             ||
		strcmp(new->plot.titleFont.rtag, old->plot.titleFont.rtag) ||
		strcmp(new->plot.labelFont.rtag, old->plot.labelFont.rtag) ||
		strcmp(new->plot.axisFont.rtag, old->plot.axisFont.rtag)   ||
		strcmp(new->plot.legendFont.rtag, old->plot.legendFont.rtag))
	{
		redisplay = True;
		if(new->plot.RenderTable != old->plot.RenderTable)
		{
			if(old->plot.RenderTable)
				XmRenderTableFree(old->plot.RenderTable);
			if(new->plot.RenderTable == NULL)
				new->plot.RenderTable = XmeGetDefaultRenderTable(neww, XmTEXT_FONTLIST);
			new->plot.RenderTable = XmRenderTableCopy(new->plot.RenderTable, NULL, 0);
		}
		InitFonts(new);
	}

	new->plot.update = redisplay;

	return redisplay;
}

static void GetValuesHook(Widget wi, ArgList args, Cardinal *num_args)
{
	int i;
	String *loc;
	SciPlotWidget w = (SciPlotWidget) wi;

	for (i = 0; i < *num_args; i++)
	{
		loc = (String *)args[i].value;
		if (strcmp(args[i].name, XmNplotTitle) == 0)
			*loc = w->plot.plotTitle;
		else if (strcmp(args[i].name, XmNxLabel) == 0)
			*loc = w->plot.xlabel;
		else if (strcmp(args[i].name, XmNyLabel) == 0)
			*loc = w->plot.ylabel;
	}
}


/*ARGSUSED*/
static void Redisplay(Widget wi, XEvent *event, Region region)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XtIsRealized(wi)) return;

	if (w->plot.update)
	{
		Resize(wi);
		w->plot.update = False;
	}
	else
	{
		DrawAll(w);
	}
}

static void Resize(Widget wi)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XtIsRealized(wi)) return;

	PixmapUpdate(w);
	EraseAll(w);
	ComputeAll(w);
	DrawAll(w);
}



/*
 * Private SciPlot utility functions
 */


/* XmRCallProc routine for checking RenderTable. If "check_set_render_table"
 * is True, then function has been called twice on same widget, thus resource
 * needs to be set NULL, otherwise leave it alone.
 */
/*ARGSUSED*/
static void CheckRenderTable(Widget wi, int offset, XrmValue *value)
{
  SciPlotWidget w = (SciPlotWidget)wi;

  if (w->plot.check_set_render_table)
  {
      value->addr = NULL;
  }
  else
  {
      w->plot.check_set_render_table = True;
      value->addr = (char*)&(w->plot.RenderTable);
  }
}

/* Find the nearest plotlist point to the cursor. If there is more than
 * one plotlist find which plotlist the point is in. The point is found
 * through a least square of x*x + y*y.
 */
static Boolean FindLineAndPointIndex(SciPlotWidget w, int x, int y)
{
	int i, n, start, end;
	double minl, *minp;
	SciPlotList *list;

	w->plot.move_list = -1;
	if(w->plot.num_plotlist <= 0) return False;

	minp = (double *) XtCalloc(w->plot.num_plotlist, sizeof(double));
	for(i = 0; i < w->plot.num_plotlist; i++)
	{
		list = w->plot.plotlist + i;
		if(!list->used) continue;
		if(list->move.type == XmMOVE_NONE) continue;
		minp[i] = DBL_MAX;
		list->move.point = -1;
		if(list->move.type == XmMOVE_LINE)
		{
			start = 0;
			end = list->number;
		}
		else
		{
			start = (list->move.start_point < 0)? 0:list->move.start_point;
			end = (list->move.end_point <= 0)?  list->number : list->move.end_point+1;
			if(end > list->number) end = list->number;
		}
		
		for(n = start; n < end; n++)
		{
			double dx = (double) (x - list->data[n].x);
			double dy = (double) (y - list->data[n].y);
			double val = dx*dx + dy*dy;
			if(val < minp[i])
			{
				minp[i] = val;
				list->move.point = n;
			}
		}
	}

	minl = DBL_MAX;
	for(i = 0; i < w->plot.num_plotlist; i++)
	{
		if(!w->plot.plotlist[i].used) continue;
		if(w->plot.plotlist[i].move.type == XmMOVE_NONE) continue;
		if(w->plot.plotlist[i].number <= 0) continue;
		if(minp[i] >= minl) continue;
		minl = minp[i];
		w->plot.move_list = i;
	}

	if(w->plot.move_list >= 0)
	{
		SciPlotList *list = w->plot.plotlist + w->plot.move_list;
		if(list->move.type == XmMOVE_LINE)
			list->move.point =
				list->move.control_point_fcn(list->move.point, list->data, list->number);
	}

	XtFree((void *) minp);
	return (w->plot.move_list >= 0);
}


/*  Button press and button motion action proc. An offset is calculated from the
 *  cursor position value to the selected point and then applied after. This means
 *  that once the point is selected the relative location of the pointer will not
 *  affect the calculated value.
 */
static void MotionAP(SciPlotWidget w, XEvent *event, String args, int n_args)
{
	float x, y;

	if(!w->plot.move_on) return;

	x = PlotToXValue(w, event);
	y = PlotToYValue(w, event);

    if (event->type == ButtonPress)
	{
    	XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
		if(FindLineAndPointIndex(w, x, y))
		{
			SciPlotList *list = w->plot.plotlist + w->plot.move_list;
			list->move.offset.x = list->data[list->move.point].x - x;
			list->move.offset.y = list->data[list->move.point].y - y;
		}
	}
	else if(w->plot.move_list >= 0)
	{
		SciPlotList *list = w->plot.plotlist + w->plot.move_list;
		SciPlotPoint *point = list->data + list->move.point;

		if(list->move.type == XmMOVE_XY)
		{
			point->x = x + list->move.offset.x;
			point->y = y + list->move.offset.y;
		}
		else if(list->move.type == XmMOVE_X_ONLY)
		{
			point->x = x + list->move.offset.x;
		}
		else if(list->move.type == XmMOVE_Y_ONLY)
		{
			point->y = y + list->move.offset.y;
		}
		else if(list->move.type == XmMOVE_LINE)
		{
			SciPlotPoint p;
			p.x = x + list->move.offset.x;
			p.y = y + list->move.offset.y;
			list->move.line_move_fcn(p, list->data, list->number);
		}
		DrawAll(w);
	}
}


/* Button up action proc.
*/
static void BtnUpAP(SciPlotWidget w, XEvent *event, String args, int n_args)
{
	int lndx, pndx;
	SciPlotCallbackStruct call_data;

	if(!w->plot.move_on || w->plot.move_list < 0) return;

	lndx = w->plot.move_list;
	pndx = w->plot.plotlist[lndx].move.point;
	w->plot.move_list = -1;

	call_data.reason =
		(w->plot.plotlist[lndx].move.type == XmMOVE_LINE)? XmCR_MOVE_LINE : XmCR_MOVE_POINT;
	call_data.event = event;
	call_data.list_id = lndx;
	call_data.point_index = pndx;
	call_data.ndata = w->plot.plotlist[lndx].number;
	call_data.data  = w->plot.plotlist[lndx].data;

	XtCallCallbackList((Widget) w, w->plot.PlotCallback, (XtPointer) &call_data);
}


/* Check for the rendition in the render table for the requested tag
 * found in the parameter fs. If it exists assign the actual tag to
 * this. If no rendition is found leave the tag as NULL. This will
 * use the default renditon. Also calculate average font parameters
 * for the rendition.
 */
static void SetFontInfo(SciPlotWidget w, SciPlotFontStruct *fs)
{
	int i, n;
	char buf[128];
	XmString str;
	XmRendition rendition = NULL;

	for(n = 0, i = 0; i < 127; i++)
		if(isprint(i)) buf[n++] = (char) i;
	buf[n++] = '\0';

	fs->tag = NULL;
	rendition = XmRenderTableGetRendition(w->plot.RenderTable, fs->rtag);
	if(rendition)
	{
		fs->tag = fs->rtag;
		XmRenditionFree(rendition);
	}
	str = XmStringGenerate(buf, NULL, XmCHARSET_TEXT, fs->tag);
	fs->height = (int) XmStringHeight(w->plot.RenderTable, str);
	fs->ascent = (int) XmStringBaseline(w->plot.RenderTable, str);
	fs->descent = fs->height - fs->ascent;
	fs->width = (int) XmStringWidth(w->plot.RenderTable, str) / n;
	XmStringFree(str);
}


static void InitFonts(SciPlotWidget w)
{
	SetFontInfo(w, &w->plot.titleFont);
	SetFontInfo(w, &w->plot.labelFont);
	SetFontInfo(w, &w->plot.axisFont);
	SetFontInfo(w, &w->plot.legendFont);
}

static float TextWidth(SciPlotWidget w, XmStringTag tag, String c)
{
	Dimension width;
	XmString str = XmStringGenerate(c, NULL, XmCHARSET_TEXT, tag);
	width = XmStringWidth(w->plot.RenderTable, str);
	XmStringFree(str);
	return (float) width;
}


/* ARGSUSED */
static Boolean CvtStringToAxisNumbers(Display *dpy, XrmValuePtr args, Cardinal *num_args,
		XrmValuePtr from, XrmValuePtr to, XtPointer *data)
{
        static int axis_numbers;
        String start = from->addr;

        if (*num_args != 0)
                XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "CvtStringToAxisNumbers",
                                "wrongParameters", "SciPlot",
                                "String to AxisNumbers conversion needs no extra arguments", NULL,
                                NULL);
        /*
         * User didn't provide enough space
         */
        if (to->addr != NULL && to->size < sizeof(int)) {
                to->size = sizeof(int);
                return False;
        }
        /*
         * Skip leading white space
         */
        while (isspace(*start)) start++;

        if (strncasecmp(start, "NUMBERS_NONE", 12))
                axis_numbers = XmNUMBERS_NONE;
        else if (strncasecmp(start, "NUMBERS_SHOW", 12))
                axis_numbers = XmNUMBERS_SHOW;
        else if (strncasecmp(start, "NUMBERS_SPACING_ALONG_AXIS", 26))
                axis_numbers = XmNUMBERS_SPACING_ALONG_AXIS;
        else
		{
                XtDisplayStringConversionWarning(dpy, from->addr, XmRAxisNumbers);
                return False;
        }
        /*
         * Store our return value
         */
        if (to->addr == NULL)
                to->addr = (XtPointer) &axis_numbers;
        else
                *(int *) to->addr = axis_numbers;
        to->size = sizeof(int);

        return True;
}


/*
 * Private List functions
 */

/* The following are the default actions for moving lines. The default selected point is
 * the last point in the array
 */
static int _DefaultControlPointFcn (int selected_point, SciPlotPoint *points, int npoints)
{
	return (npoints > 0)? npoints-1 : 0;
}


/* Grab the end point of the plot and move the entire line using a linear curve fit between
 * the first and last point of the curve. Only the y values change, x values stay the same.
 */
static void  _DefaultMoveListFcn (SciPlotPoint selected_point, SciPlotPoint *points, int npoints)
{
	float m, b;
	int n = npoints - 1;
	m = (selected_point.y - points[0].y)/(points[n].x - points[0].x);
	b = selected_point.y - m*points[n].x;
	for(n = 1; n < npoints; n++)
		points[n].y = m * points[n].x + b;
}


static int _ListNew (SciPlotWidget w)
{
	int index;
	SciPlotList *p;
	Boolean found;

	/* First check to see if there is any free space in the index */
	found = False;
	for (index = 0; index < w->plot.num_plotlist; index++)
	{
		p = w->plot.plotlist + index;
		if (!p->used)
		{
			found = True;
			break;
		}
	}

	/* If no space is found, increase the size of the index */
	if (!found)
	{
		w->plot.num_plotlist++;
		if (w->plot.alloc_plotlist == 0)
		{
			w->plot.alloc_plotlist = NUMPLOTLINEALLOC;
			w->plot.plotlist = (SciPlotList *) XtCalloc(w->plot.alloc_plotlist, sizeof(SciPlotList));
			if (!w->plot.plotlist)
			{
				printf("Can't calloc memory for SciPlotList\n");
				exit(1);
			}
			w->plot.alloc_plotlist = NUMPLOTLINEALLOC;
		}
		else if (w->plot.num_plotlist > w->plot.alloc_plotlist)
		{
			w->plot.alloc_plotlist += NUMPLOTLINEALLOC;
			w->plot.plotlist = (SciPlotList *) XtRealloc((char *) w->plot.plotlist,
							   w->plot.alloc_plotlist * sizeof(SciPlotList));
			if (!w->plot.plotlist)
			{
				printf("Can't realloc memory for SciPlotList\n");
				exit(1);
			}
		}
		index = w->plot.num_plotlist - 1;
		p = w->plot.plotlist + index;
	}

	p->LineStyle = p->LineColor = p->PointStyle = p->PointColor = 0;
	p->number = p->allocated = 0;
	p->data = NULL;
	p->legend = NULL;
	p->draw = p->used = True;
	p->markersize = (float) w->plot.DefaultMarkerSize;
	p->move.type = XmMOVE_NONE;
	p->move.control_point_fcn = _DefaultControlPointFcn;
	p->move.line_move_fcn = _DefaultMoveListFcn;
	return index;
}

static void _ListDelete (SciPlotList *p)
{
	p->draw = p->used = False;
	p->move.type = XmMOVE_NONE;
	p->number = p->allocated = 0;
	if (p->data)
		XtFree((char *) p->data);
	p->data = NULL;
	if (p->legend)
		XtFree((char *) p->legend);
	p->legend = NULL;
}

static SciPlotList *_ListFind (SciPlotWidget w, int id)
{
	SciPlotList *p;

	if ((id >= 0) && (id < w->plot.num_plotlist))
	{
		p = w->plot.plotlist + id;
		if (p->used)
			return p;
	}
	return NULL;
}

static void _ListSetStyle (Widget w, SciPlotList *p, Pixel pcolor, int pstyle, Pixel lcolor, int lstyle)
{
	switch(lstyle)
	{
		case XmLINE_NONE:
		case XmLINE_SOLID:
		case XmLINE_DOTTED:
		case XmLINE_WIDEDOT:
			p->LineStyle = lstyle;
			p->LineColor = lcolor;
			break;
		default:
			XtAppErrorMsg(XtWidgetToApplicationContext(w),
                "ListSetStyle", "badLineStyle", "sciplotWidgetClass",
                "SciPlot: Unrecognized line style specified",
                NULL, 0);
	}

	switch(pstyle)
	{
		case XmMARKER_NONE:
		case XmMARKER_CIRCLE:
		case XmMARKER_SQUARE:
		case XmMARKER_UTRIANGLE:
		case XmMARKER_DTRIANGLE:
		case XmMARKER_LTRIANGLE:
		case XmMARKER_RTRIANGLE:
		case XmMARKER_DIAMOND:
		case XmMARKER_HOURGLASS:
		case XmMARKER_BOWTIE:
		case XmMARKER_FCIRCLE:
		case XmMARKER_FSQUARE:
		case XmMARKER_FUTRIANGLE:
		case XmMARKER_FDTRIANGLE:
		case XmMARKER_FLTRIANGLE:
		case XmMARKER_FRTRIANGLE:
		case XmMARKER_FDIAMOND:
		case XmMARKER_FHOURGLASS:
		case XmMARKER_FBOWTIE:
		case XmMARKER_DOT:
			p->PointStyle = pstyle;
			p->PointColor = pcolor;
			break;

		default:
			XtAppErrorMsg(XtWidgetToApplicationContext(w),
                "ListSetStyle", "badMarkerStyle", "sciplotWidgetClass",
                "SciPlot: Unrecognized marker type specified",
                NULL, 0);
	}
}

static void _ListSetLegend (SciPlotList *p, char *legend)
{
	if(legend == XmNO_LEGEND_ENTRY) return;
	p->legend = (char *) XtMalloc(strlen(legend) + 1);
	strcpy(p->legend, legend);
}

static void _ListAllocData (SciPlotList *p, int num)
{
	if (p->data)
	{
		XtFree((char *) p->data);
		p->allocated = 0;
	}
	p->allocated = num + NUMPLOTDATAEXTRA;
	p->data = (SciPlotPoint *) XtCalloc(p->allocated, sizeof(SciPlotPoint));
	if (!p->data)
	{
		p->number = p->allocated = 0;
	}
}

static void _ListReallocData (SciPlotList *p, int more)
{
	if (!p->data)
	{
		_ListAllocData(p, more);
	}
	else if (p->number + more > p->allocated)
	{
		p->allocated += more + NUMPLOTDATAEXTRA;
		p->data = (SciPlotPoint *) XtRealloc((char *) p->data, p->allocated * sizeof(SciPlotPoint));
		if (!p->data)
		{
			p->number = p->allocated = 0;
		}
	}

}

static void _ListAddPoint (SciPlotList *p, int num, SciPlotPoint *list)
{
	int i;

	_ListReallocData(p, num);
	if (p->data)
	{
		for (i = 0; i < num; i++)
		{
			p->data[i + p->number].x = list[i].x;
			p->data[i + p->number].y = list[i].y;
		}
		p->number += num;
	}
}


static void _ListSetPoint(SciPlotList *p, int num, SciPlotPoint *list)
{
	if ((!p->data) || (p->allocated < num))
		_ListAllocData(p, num);
	p->number = 0;
	_ListAddPoint(p, num, list);
}


/*
 * Private SciPlot functions
 */

/* Draw a standard horizontal string. Note that XmStringDraw and XmStringDrawImage
 * both required a GC allocated by the XtAllocateGC function, thus the textGC.
 */
static void DrawHString(SciPlotWidget w, int x, int y, char *str, SciPlotFont font)
{
	Dimension width;
	XmString string = XmStringGenerate(str, NULL, XmCHARSET_TEXT, font->tag);
	width = XmStringWidth(w->plot.RenderTable, string);
	XmStringDraw(XtDisplay(w), w->plot.draw, w->plot.RenderTable, string, w->plot.textGC,
			x, y, width, XmALIGNMENT_BEGINNING, XmLEFT_TO_RIGHT, NULL);
	XmStringFree(string);
}


/* Draw a vertically oriented label. Motif and the XmString functions do not have a
 * method of creating a vertical label so this function creates a one for use as a
 * Y-axis label. Note that it will cover up anything underneath the entire area of
 * the label. For use as a Y-axis label this is not a problem in this application.
 */
static void DrawVString(SciPlotWidget w, int x, int y, char *str, SciPlotFont font)
{
	int i, j, xdest, ydest;
	Dimension width, height;
	XmString string;
	Pixmap pix;
	XImage *image1, *image2;
	Display *dpy;
	Window win;

	dpy = XtDisplay(w);
	win = w->plot.draw;

	string = XmStringGenerate(str, NULL, XmCHARSET_TEXT, font->tag);
	XmStringExtent(w->plot.RenderTable, string, &width, &height);

	/* I find it easier to create a pixmap and get the image rather than creating it */
	pix = XCreatePixmap(dpy, win, (UINT) height, (UINT) width, (UINT) w->core.depth);
	image2 = XGetImage(dpy, pix, 0, 0, (UINT) height, (UINT) width, AllPlanes, XYPixmap);
	XFreePixmap(dpy,pix);

	/* Create and write to the unrotated pixmap */
	pix = XCreatePixmap(dpy, win, (UINT) width, (UINT) height, (UINT) w->core.depth);
	XmStringDrawImage(dpy, pix, w->plot.RenderTable, string, w->plot.textGC,
			0, 0, width, XmALIGNMENT_BEGINNING, XmLEFT_TO_RIGHT, NULL);
	image1 = XGetImage(dpy, pix, 0, 0, (UINT) width, (UINT) height, AllPlanes, XYPixmap);
	XFreePixmap(dpy,pix);
	XmStringFree(string);

	/* 90 degree counter clockwise rotation */
	for(i = 0; i < width; i++)
	{
		int x0 = width - i - 1;
		for(j = 0; j < height; j++)
		{
			XPutPixel(image2, j, i, XGetPixel(image1, x0, j));
		}
	}

	/* Origin adjustment */
	xdest = x - font->ascent;
	if (xdest < 0)
		xdest = 0;
	ydest = y - width;

	/* Write out the image to the plot window */
	XPutImage(dpy, win, w->plot.textGC, image2, 0, 0, xdest, ydest, (UINT) height, (UINT) width);

	XDestroyImage(image1);
	XDestroyImage(image2);
}

static char dots[] = {2, 1, 1};
static char widedots[] = {2, 1, 4};

static GC ItemGetGC (SciPlotWidget w, SciPlotItem *item)
{
	GC gc;

	switch (item->kind.any.style)
	{
	case XmLINE_SOLID:
		gc = w->plot.defaultGC;
		break;
	case XmLINE_DOTTED:
		XSetDashes(XtDisplay(w), w->plot.dashGC, 0, &dots[1], (int) dots[0]);
		gc = w->plot.dashGC;
		break;
	case XmLINE_WIDEDOT:
		XSetDashes(XtDisplay(w), w->plot.dashGC, 0, &widedots[1], (int) widedots[0]);
		gc = w->plot.dashGC;
		break;
	default:
		gc = w->plot.defaultGC;
		break;
	}
	XSetForeground(XtDisplay(w), gc, item->kind.any.color);
	return gc;
}

static GC ItemGetFontGC (SciPlotWidget w, SciPlotItem *item)
{
	GC gc = w->plot.defaultGC;
	XSetForeground(XtDisplay(w), gc, item->kind.any.color);

	return gc;
}


/*
 * Private drawing functions
 */

static void ItemDraw (SciPlotWidget w, SciPlotItem *item)
{
	XmString xmstr;
	XPoint point[8];
	XSegment seg;
	XRectangle rect;
	int i;
	GC gc;

	if (!XtIsRealized((Widget) w))
		return;
	if ((item->type > SciPlotStartTextTypes) && (item->type < SciPlotEndTextTypes))
		gc = ItemGetFontGC(w, item);
	else
		gc = ItemGetGC(w, item);
	if (!gc)
		return;
	switch (item->type)
	{
	case SciPlotLine:
		seg.x1 = (short) item->kind.line.x1;
		seg.y1 = (short) item->kind.line.y1;
		seg.x2 = (short) item->kind.line.x2;
		seg.y2 = (short) item->kind.line.y2;
		XDrawSegments(XtDisplay(w), w->plot.draw, gc, &seg, 1);
		break;
	case SciPlotRect:
		XDrawRectangle(XtDisplay(w), w->plot.draw, gc,
					   (int) (item->kind.rect.x),
					   (int) (item->kind.rect.y),
					   (UINT) (item->kind.rect.w),
					   (UINT) (item->kind.rect.h));
		break;
	case SciPlotFRect:
		XFillRectangle(XtDisplay(w), w->plot.draw, gc,
					   (int) (item->kind.rect.x),
					   (int) (item->kind.rect.y),
					   (UINT) (item->kind.rect.w),
					   (UINT) (item->kind.rect.h));
		XDrawRectangle(XtDisplay(w), w->plot.draw, gc,
					   (int) (item->kind.rect.x),
					   (int) (item->kind.rect.y),
					   (UINT) (item->kind.rect.w),
					   (UINT) (item->kind.rect.h));
		break;
	case SciPlotPoly:
		i = 0;
		while (i < item->kind.poly.count)
		{
			point[i].x = (int) item->kind.poly.x[i];
			point[i].y = (int) item->kind.poly.y[i];
			i++;
		}
		point[i].x = (int) item->kind.poly.x[0];
		point[i].y = (int) item->kind.poly.y[0];
		XDrawLines(XtDisplay(w), w->plot.draw, gc,
				   point, i + 1, CoordModeOrigin);
		break;
	case SciPlotFPoly:
		i = 0;
		while (i < item->kind.poly.count)
		{
			point[i].x = (int) item->kind.poly.x[i];
			point[i].y = (int) item->kind.poly.y[i];
			i++;
		}
		point[i].x = (int) item->kind.poly.x[0];
		point[i].y = (int) item->kind.poly.y[0];
		XFillPolygon(XtDisplay(w), w->plot.draw, gc,
					 point, i + 1, Complex, CoordModeOrigin);
		XDrawLines(XtDisplay(w), w->plot.draw, gc,
				   point, i + 1, CoordModeOrigin);
		break;
	case SciPlotCircle:
		XDrawArc(XtDisplay(w), w->plot.draw, gc,
				 (int) (item->kind.circ.x - item->kind.circ.r),
				 (int) (item->kind.circ.y - item->kind.circ.r),
				 (UINT) (item->kind.circ.r * 2),
				 (UINT) (item->kind.circ.r * 2),
				 0 * 64, 360 * 64);
		break;
	case SciPlotFCircle:
		XFillArc(XtDisplay(w), w->plot.draw, gc,
				 (int) (item->kind.circ.x - item->kind.circ.r),
				 (int) (item->kind.circ.y - item->kind.circ.r),
				 (UINT) (item->kind.circ.r * 2),
				 (UINT) (item->kind.circ.r * 2),
				 0 * 64, 360 * 64);
		break;
	case SciPlotText:
		DrawHString(w, (int) (item->kind.text.x), (int) (item->kind.text.y),
					 item->kind.text.text, item->kind.text.font);
		break;
	case SciPlotVText:
		DrawVString(w, (int) (item->kind.text.x), (int) (item->kind.text.y),
					 item->kind.text.text, item->kind.text.font);
		break;
	case SciPlotClipRegion:
		rect.x = (short) item->kind.line.x1;
		rect.y = (short) item->kind.line.y1;
		rect.width = (short) item->kind.line.x2;
		rect.height = (short) item->kind.line.y2;
		XSetClipRectangles(XtDisplay(w), w->plot.dashGC, 0, 0, &rect, 1, Unsorted);
		XSetClipRectangles(XtDisplay(w), w->plot.defaultGC, 0, 0, &rect, 1, Unsorted);
		break;
	case SciPlotClipClear:
		XSetClipMask(XtDisplay(w), w->plot.dashGC, None);
		XSetClipMask(XtDisplay(w), w->plot.defaultGC, None);
		break;
	default:
		break;
	}
}


static void EraseAll (SciPlotWidget w)
{
	SciPlotItem *item;
	int i;

	w->plot.update = True;

	item = w->plot.drawlist;
	i = 0;
	while (i < w->plot.num_drawlist)
	{
		if ((item->type > SciPlotStartTextTypes) &&
				(item->type < SciPlotEndTextTypes))
			XtFree(item->kind.text.text);
		i++;
		item++;
	}
	w->plot.num_drawlist = 0;

	XSetForeground(XtDisplay(w), w->plot.defaultGC, w->plot.BackgroundColor);
	XFillRectangle(XtDisplay(w), w->plot.pix, w->plot.defaultGC,
		0, 0, w->plot.pix_width, w->plot.pix_height);
	XSetForeground(XtDisplay(w), w->plot.defaultGC, w->plot.ForegroundColor);
}

static SciPlotItem *ItemGetNew (SciPlotWidget w)
{
	SciPlotItem *item;

	w->plot.num_drawlist++;
	if (w->plot.num_drawlist >= w->plot.alloc_drawlist)
	{
		w->plot.alloc_drawlist += NUMPLOTITEMEXTRA;
		w->plot.drawlist = (SciPlotItem *) XtRealloc((char *) w->plot.drawlist,
						   w->plot.alloc_drawlist * sizeof(SciPlotItem));
		if (!w->plot.drawlist)
		{
			printf("Can't realloc memory for SciPlotItem list\n");
			exit(1);
		}
	}
	item = w->plot.drawlist + (w->plot.num_drawlist - 1);
	item->type = SciPlotFalse;
	item->drawing_class = w->plot.current_id;
	return item;
}


static void LineSet(SciPlotWidget w, float x1, float y1, float x2, float y2,
		Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.line.x1 = x1;
	item->kind.line.y1 = y1;
	item->kind.line.x2 = x2;
	item->kind.line.y2 = y2;
	item->type = SciPlotLine;
	ItemDraw(w, item);
}

static void RectSet(SciPlotWidget w, float x1, float y1, float x2, float y2,
		Pixel color, int style)
{
	SciPlotItem *item;
	float x, y, width, height;

	if (x1 < x2)
		x = x1, width = (x2 - x1 + 1);
	else
		x = x2, width = (x1 - x2 + 1);
	if (y1 < y2)
		y = y1, height = (y2 - y1 + 1);
	else
		y = y2, height = (y1 - y2 + 1);

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.rect.x = x;
	item->kind.rect.y = y;
	item->kind.rect.w = width;
	item->kind.rect.h = height;
	item->type = SciPlotRect;
	ItemDraw(w, item);
}

static void FilledRectSet (SciPlotWidget w, float x1, float y1, float x2, float y2, Pixel color, int style)
{
	SciPlotItem *item;
	float x, y, width, height;

	if (x1 < x2)
		x = x1, width = (x2 - x1 + 1);
	else
		x = x2, width = (x1 - x2 + 1);
	if (y1 < y2)
		y = y1, height = (y2 - y1 + 1);
	else
		y = y2, height = (y1 - y2 + 1);

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.rect.x = x;
	item->kind.rect.y = y;
	item->kind.rect.w = width;
	item->kind.rect.h = height;
	item->type = SciPlotFRect;
	ItemDraw(w, item);
}

static void TriSet (SciPlotWidget w, float x1, float y1, float x2, float y2, float x3, float y3, Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.poly.count = 3;
	item->kind.poly.x[0] = x1;
	item->kind.poly.y[0] = y1;
	item->kind.poly.x[1] = x2;
	item->kind.poly.y[1] = y2;
	item->kind.poly.x[2] = x3;
	item->kind.poly.y[2] = y3;
	item->type = SciPlotPoly;
	ItemDraw(w, item);
}

static void FilledTriSet (SciPlotWidget w, float x1, float y1, float x2, float y2, float x3, float y3, Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.poly.count = 3;
	item->kind.poly.x[0] = x1;
	item->kind.poly.y[0] = y1;
	item->kind.poly.x[1] = x2;
	item->kind.poly.y[1] = y2;
	item->kind.poly.x[2] = x3;
	item->kind.poly.y[2] = y3;
	item->type = SciPlotFPoly;
	ItemDraw(w, item);
}

static void QuadSet (SciPlotWidget w, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.poly.count = 4;
	item->kind.poly.x[0] = x1;
	item->kind.poly.y[0] = y1;
	item->kind.poly.x[1] = x2;
	item->kind.poly.y[1] = y2;
	item->kind.poly.x[2] = x3;
	item->kind.poly.y[2] = y3;
	item->kind.poly.x[3] = x4;
	item->kind.poly.y[3] = y4;
	item->type = SciPlotPoly;
	ItemDraw(w, item);
}

static void FilledQuadSet (SciPlotWidget w, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.poly.count = 4;
	item->kind.poly.x[0] = x1;
	item->kind.poly.y[0] = y1;
	item->kind.poly.x[1] = x2;
	item->kind.poly.y[1] = y2;
	item->kind.poly.x[2] = x3;
	item->kind.poly.y[2] = y3;
	item->kind.poly.x[3] = x4;
	item->kind.poly.y[3] = y4;
	item->type = SciPlotFPoly;
	ItemDraw(w, item);
}

static void CircleSet (SciPlotWidget w, float x, float y, float r, Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.circ.x = x;
	item->kind.circ.y = y;
	item->kind.circ.r = r;
	item->type = SciPlotCircle;
	ItemDraw(w, item);
}

static void FilledCircleSet (SciPlotWidget w, float x, float y, float r, Pixel color, int style)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = (short) style;
	item->kind.circ.x = x;
	item->kind.circ.y = y;
	item->kind.circ.r = r;
	item->type = SciPlotFCircle;
	ItemDraw(w, item);
}

static void TextSet (SciPlotWidget w, float x, float y, char *text, Pixel color, SciPlotFont font)
{
	SciPlotItem *item;

	if(!text) text = "???";
	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = 0;
	item->kind.text.x = x;
	item->kind.text.y = y - (float) font->ascent;
	item->kind.text.length = strlen(text);
	item->kind.text.text = XtMalloc((int) item->kind.text.length + 1);
	item->kind.text.font = font;
	strcpy(item->kind.text.text, text);
	item->type = SciPlotText;
	ItemDraw(w, item);
}

static void TextCenter (SciPlotWidget w, float x, float y, char *text, Pixel color, SciPlotFont font)
{
	x -= TextWidth(w, font->tag, text) / 2.0;
	y += (float) font->height / 2.0 - (float) font->descent;
	TextSet(w, x, y, text, color, font);
}

static void VTextSet (SciPlotWidget w, float x, float y, char *text, Pixel color, SciPlotFont font)
{
	SciPlotItem *item;

	item = ItemGetNew(w);
	item->kind.any.color = color;
	item->kind.any.style = 0;
	item->kind.text.x = x;
	item->kind.text.y = y - font->ascent;
	item->kind.text.length = strlen(text);
	item->kind.text.text = XtMalloc((int) item->kind.text.length + 1);
	item->kind.text.font = font;
	strcpy(item->kind.text.text, text);
	item->type = SciPlotVText;
	ItemDraw(w, item);
}

static void VTextCenter (SciPlotWidget w, float x, float y, char *text, Pixel color, SciPlotFont font)
{
	x += (float) font->height / 2.0 - (float) font->descent;
	y += TextWidth(w, font->tag, text) / 2.0;
	VTextSet(w, x, y, text, color, font);
}

static void ClipSet (SciPlotWidget w)
{
	SciPlotItem *item;

	if (w->plot.ChartType == XmCARTESIAN)
	{
		item = ItemGetNew(w);
		item->kind.any.style = XmLINE_SOLID;
		item->kind.any.color = 1;
		item->kind.line.x1 = w->plot.x.Origin;
		item->kind.line.x2 = w->plot.x.Size;
		item->kind.line.y1 = w->plot.y.Origin;
		item->kind.line.y2 = w->plot.y.Size;
		item->type = SciPlotClipRegion;
		ItemDraw(w, item);
	}
}

static void ClipClear (SciPlotWidget w)
{
	SciPlotItem *item;

	if (w->plot.ChartType == XmCARTESIAN)
	{
		item = ItemGetNew(w);
		item->kind.any.style = XmLINE_SOLID;
		item->kind.any.color = 1;
		item->type = SciPlotClipClear;
		ItemDraw(w, item);
	}
}


/*
 * Private data point to screen location converters
 */

static float PlotX (SciPlotWidget w, float xin)
{
	float xout;

	if (w->plot.XLog)
		xout = w->plot.x.Origin +
			   ((log10(xin) - log10(w->plot.x.DrawOrigin)) *
				(w->plot.x.Size / w->plot.x.DrawSize));
	else
		xout = w->plot.x.Origin +
			   ((xin - w->plot.x.DrawOrigin) *
				(w->plot.x.Size / w->plot.x.DrawSize));
	return xout;
}

static float PlotY (SciPlotWidget w, float yin)
{
	float yout;

	if (w->plot.YLog)
		yout = w->plot.y.Origin + w->plot.y.Size -
			   ((log10(yin) - log10(w->plot.y.DrawOrigin)) *
				(w->plot.y.Size / w->plot.y.DrawSize));
	else
		yout = w->plot.y.Origin + w->plot.y.Size -
			   ((yin - w->plot.y.DrawOrigin) *
				(w->plot.y.Size / w->plot.y.DrawSize));
	return yout;
}

static void PlotRTRadians (SciPlotWidget w, float r, float t, float *xout, float *yout)
{
	*xout = w->plot.x.Center + (r * (float) cos(t) /
								w->plot.PolarScale * w->plot.x.Size / 2.0);
	*yout = w->plot.y.Center + (-r * (float) sin(t) /
								w->plot.PolarScale * w->plot.x.Size / 2.0);
}

static void PlotRTDegrees (SciPlotWidget w, float r, float t, float *xout, float *yout)
{
	t *= DEG2RAD;
	PlotRTRadians(w, r, t, xout, yout);
}

static void PlotRT (SciPlotWidget w, float r, float t, float *xout, float *yout)
{
	if (w->plot.Degrees)
		t *= DEG2RAD;
	PlotRTRadians(w, r, t, xout, yout);
}

/*
 * Screen location to data point converters
 */

static float PlotToXValue (SciPlotWidget w, XEvent *event)
{
	float xout;
	XButtonEvent *be = (XButtonEvent *) event;

	if (w->plot.XLog)
	{
		float exp = log10(w->plot.x.DrawOrigin) + 
			((float) be->x - w->plot.x.Origin)*(w->plot.x.DrawSize/w->plot.x.Size);
		xout = powf(10.,exp);
	}
	else
	{
		xout = w->plot.x.DrawOrigin +
			(((float) be->x - w->plot.x.Origin)*(w->plot.x.DrawSize/w->plot.x.Size));
	}
	return xout;
}

static float PlotToYValue (SciPlotWidget w, XEvent *event)
{
	float yout;
	XButtonEvent *be = (XButtonEvent *) event;

	if (w->plot.YLog)
	{
		float exp = log10(w->plot.y.DrawOrigin) - 
			((w->plot.y.Origin + w->plot.y.Size - (float) be->y)*(w->plot.y.DrawSize/w->plot.y.Size));
		yout = powf(10.,exp);
	}
	else
	{
		yout = w->plot.y.DrawOrigin +
			((w->plot.y.Origin + w->plot.y.Size - (float) be->y)*(w->plot.y.DrawSize/w->plot.y.Size));
	}
	return yout;
}


/*
 * Private calculation utilities for axes
 */

#define NUMBER_MINOR	8
#define MAX_MAJOR	8
static float CAdeltas[8] =
{0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0};
static int CAdecimals[8] =
{0, 0, 1, 0, 0, 0, 1, 0};
static int CAminors[8] =
{4, 4, 4, 5, 4, 4, 4, 5};

static void ComputeAxis (SciPlotAxis *axis, float min, float max, Boolean log)
{
	float range, rnorm, delta, calcmin, calcmax;
	int nexp, majornum, minornum, majordecimals, decimals, i;

	range = max - min;
	if (log)
	{
		if (range == 0.0)
		{
			calcmin = powi(10.0, (int) floor(log10(min)));
			calcmax = 10.0 * calcmin;
		}
		else
		{
			calcmin = powi(10.0, (int) floor(log10(min)));
			calcmax = powi(10.0, (int) ceil(log10(max)));
		}

		delta = 10.0;

		axis->DrawOrigin = calcmin;
		axis->DrawMax = calcmax;
		axis->DrawSize = log10(calcmax) - log10(calcmin);
		axis->MajorInc = delta;
		axis->MajorNum = (int) (log10(calcmax) - log10(calcmin)) + 1;
		axis->MinorNum = 10;
		axis->Precision = -(int) (log10(calcmin) * 1.0001);
		if (axis->Precision < 0)
			axis->Precision = 0;
	}
	else
	{
		if (range == 0.0) nexp = 0;
		else nexp = (int) floor(log10(range));
		rnorm = range / powi(10.0, nexp);
		for (i = 0; i < NUMBER_MINOR; i++)
		{
			delta = CAdeltas[i];
			minornum = CAminors[i];
			majornum = (int) ((rnorm + 0.9999 * delta) / delta);
			majordecimals = CAdecimals[i];
			if (majornum <= MAX_MAJOR) break;
		}
		delta *= powi(10.0, nexp);

		if(axis->Fixed.set)
		{
			minornum = axis->Fixed.MinorNum;
			delta = axis->Fixed.MajorInc;
		}

		if (min < 0.0)
			calcmin = ((float) ((int) ((min - .9999 * delta) / delta))) * delta;
		else if ((min > 0.0) && (min < 1.0))
			calcmin = ((float) ((int) ((1.0001 * min) / delta))) * delta;
		else if (min >= 1.0)
			calcmin = ((float) ((int) ((.9999 * min) / delta))) * delta;
		else
			calcmin = min;
		if (max < 0.0)
			calcmax = ((float) ((int) ((.9999 * max) / delta))) * delta;
		else if (max > 0.0)
			calcmax = ((float) ((int) ((max + .9999 * delta) / delta))) * delta;
		else
			calcmax = max;

		axis->DrawOrigin = calcmin;
		axis->DrawMax = calcmax;
		axis->DrawSize = calcmax - calcmin;
		axis->MajorInc = delta;
		axis->MajorNum = majornum;
		axis->MinorNum = minornum;

		delta = log10(axis->MajorInc);
		if (delta > 0.0)
			decimals = -(int) floor(delta) + majordecimals;
		else
			decimals = (int) ceil(-delta) + majordecimals;
		if (decimals < 0)
			decimals = 0;
		axis->Precision = decimals;
	}
}

static void ComputeDrawingRange (SciPlotWidget w)
{
	if (w->plot.ChartType == XmCARTESIAN)
	{
		ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x, w->plot.XLog);
		ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y, w->plot.YLog);
	}
	else
	{
		ComputeAxis(&w->plot.x, (float) 0.0, w->plot.Max.x, (Boolean) False);
		w->plot.PolarScale = w->plot.x.DrawMax;
	}
}

static Boolean CheckMinMax (SciPlotWidget w)
{
	register int i, j;
	register SciPlotList *p;
	register float val;

	if (w->plot.ChartType == XmCARTESIAN)
	{
		for (i = 0; i < w->plot.num_plotlist; i++)
		{
			p = w->plot.plotlist + i;
			if (p->draw)
			{
				for (j = 0; j < p->number; j++)
				{

					/* Don't count the "break in line segment" flag for Min/Max */
					if (p->data[j].x > SCIPLOT_SKIP_VAL && p->data[j].y > SCIPLOT_SKIP_VAL)
					{
						val = p->data[j].x;
						if (val > w->plot.x.DrawMax || val < w->plot.x.DrawOrigin)
							return True;
						val = p->data[j].y;
						if (val > w->plot.y.DrawMax || val < w->plot.y.DrawOrigin)
							return True;
					}
				}
			}
		}
	}
	else
	{
		for (i = 0; i < w->plot.num_plotlist; i++)
		{
			p = w->plot.plotlist + i;
			if (p->draw)
			{
				for (j = 0; j < p->number; j++)
				{
					val = p->data[j].x;
					if (val > w->plot.Max.x || val < w->plot.Min.x)
						return True;
				}
			}
		}
	}
	return False;
}

static void ComputeMinMax (SciPlotWidget w)
{
	register int i, j;
	register SciPlotList *p;
	register float val;
	Boolean firstx, firsty;

	w->plot.Min.x = w->plot.Min.y = w->plot.Max.x = w->plot.Max.y = 1.0;
	firstx = True;
	firsty = True;

	for (i = 0; i < w->plot.num_plotlist; i++)
	{
		p = w->plot.plotlist + i;
		if (p->draw)
		{
			for (j = 0; j < p->number; j++)
			{

				/* Don't count the "break in line segment" flag for Min/Max */
				if (p->data[j].x > SCIPLOT_SKIP_VAL &&
						p->data[j].y > SCIPLOT_SKIP_VAL)
				{

					val = p->data[j].x;
					if (!w->plot.XLog || (w->plot.XLog && (val > 0.0)))
					{
						if (firstx)
						{
							w->plot.Min.x = w->plot.Max.x = val;
							firstx = False;
						}
						else
						{
							if (val > w->plot.Max.x)
								w->plot.Max.x = val;
							else if (val < w->plot.Min.x)
								w->plot.Min.x = val;
						}
					}

					val = p->data[j].y;
					if (!w->plot.YLog || (w->plot.YLog && (val > 0.0)))
					{
						if (firsty)
						{
							w->plot.Min.y = w->plot.Max.y = val;
							firsty = False;
						}
						else
						{
							if (val > w->plot.Max.y)
								w->plot.Max.y = val;
							else if (val < w->plot.Min.y)
								w->plot.Min.y = val;
						}
					}
				}

			}
		}
	}

	/* fix defaults if there is only one point. */
	if (firstx)
	{
		if (w->plot.XLog)
		{
			w->plot.Min.x = 1.0;
			w->plot.Max.x = 10.0;
		}
		else
		{
			w->plot.Min.x = 0.0;
			w->plot.Max.x = 10.0;
		}
	}
	if (firsty)
	{
		if (w->plot.YLog)
		{
			w->plot.Min.y = 1.0;
			w->plot.Max.y = 10.0;
		}
		else
		{
			w->plot.Min.y = 0.0;
			w->plot.Max.y = 10.0;
		}
	}
	if (w->plot.ChartType == XmCARTESIAN)
	{
		if (!w->plot.XLog)
		{
			if (!w->plot.XAutoScale)
			{
				w->plot.Min.x = w->plot.UserMin.x;
				w->plot.Max.x = w->plot.UserMax.x;
			}
			else if (w->plot.XOrigin)
			{
				if (w->plot.Min.x > 0.0)
					w->plot.Min.x = 0.0;
				if (w->plot.Max.x < 0.0)
					w->plot.Max.x = 0.0;
			}
			if (fabs(w->plot.Min.x - w->plot.Max.x) < 1.e-10)
			{
				w->plot.Min.x -= .5;
				w->plot.Max.x += .5;
			}
		}
		if (!w->plot.YLog)
		{
			if (!w->plot.YAutoScale)
			{
				w->plot.Min.y = w->plot.UserMin.y;
				w->plot.Max.y = w->plot.UserMax.y;
			}
			else if (w->plot.YOrigin)
			{
				if (w->plot.Min.y > 0.0)
					w->plot.Min.y = 0.0;
				if (w->plot.Max.y < 0.0)
					w->plot.Max.y = 0.0;
			}
			if (fabs(w->plot.Min.y - w->plot.Max.y) < 1.e-10)
			{
				w->plot.Min.y -= .5;
				w->plot.Max.y += .5;
			}
		}
	}
	else
	{
		if (fabs(w->plot.Min.x) > fabs(w->plot.Max.x))
			w->plot.Max.x = fabs(w->plot.Min.x);
	}
}

static void ComputeLegendDimensions (SciPlotWidget w)
{
	float current, xmax, ymax;
	int i;
	SciPlotList *p;

	if (w->plot.ShowLegend)
	{
		xmax = 0.0;
		ymax = 2.0 * (float) w->plot.LegendMargin;

		for (i = 0; i < w->plot.num_plotlist; i++)
		{
			p = w->plot.plotlist + i;
			if (p->draw && p->legend)
			{
				current = (float) (w->plot.MarginWidth + w->plot.LegendMargin * 3 + w->plot.LegendLineSize) +
						  TextWidth(w, w->plot.legendFont.tag, p->legend);
				if (current > xmax)
					xmax = current;
				ymax += (float) w->plot.legendFont.height;
			}
		}

		w->plot.x.LegendSize = xmax;
		w->plot.x.LegendPos = (float) w->plot.MarginWidth;
		w->plot.y.LegendSize = ymax;
		w->plot.y.LegendPos = 0.0;
	}
	else
	{
		w->plot.x.LegendSize = 0.0;
		w->plot.x.LegendPos = 0.0;
		w->plot.y.LegendSize = 0.0;
		w->plot.y.LegendPos = 0.0;
	}
}

static void ComputeDimensions (SciPlotWidget w)
{
	float x, y, width, height, axisnumbersize, axisXlabelsize, axisYlabelsize;

	/* x,y is the origin of the upper left corner of the drawing area inside
	 * the widget.  Doesn't necessarily have to be (Margin,Margin) as it is now.
	 */
	x = (float) w->plot.MarginWidth;
	y = (float) w->plot.MarginHeight;

	width = (float) w->core.width - (float) w->plot.MarginWidth - x;
	if (!w->plot.LegendThroughPlot)
		width -= w->plot.x.LegendSize;
	height = (float) w->core.height - (float) w->plot.MarginHeight - y;

	w->plot.x.Origin = x;
	w->plot.y.Origin = y;

	/* Adjust the size depending upon what sorts of text are visible. */
	if (w->plot.ShowTitle)
		height -= (float) (w->plot.TitleMargin + w->plot.titleFont.height);

	if (w->plot.ChartType == XmCARTESIAN)
	{
		if (w->plot.XAxisNumbers == XmNUMBERS_SHOW)
		{
			axisnumbersize = (float) (w->plot.MarginHeight + w->plot.axisFont.height);
			height -= axisnumbersize;
		}
		if (w->plot.YAxisNumbers == XmNUMBERS_SHOW)
		{
			axisnumbersize = (float) (w->plot.MarginWidth + w->plot.axisFont.width);
			width -= axisnumbersize;
			w->plot.x.Origin += axisnumbersize;
		}

		if (w->plot.ShowXLabel)
		{
			axisXlabelsize = (float) (w->plot.MarginHeight + w->plot.labelFont.height);
			height -= axisXlabelsize;
		}
		if (w->plot.ShowYLabel)
		{
			axisYlabelsize = (float) (w->plot.MarginWidth + w->plot.labelFont.width);
			width -= axisYlabelsize;
			w->plot.x.Origin += axisYlabelsize;
		}
	}

	w->plot.x.Size = width;
	w->plot.y.Size = height;

	/* Adjust parameters for polar plot */
	if (w->plot.ChartType == XmPOLAR)
	{
		if (height < width)
			w->plot.x.Size = height;
	}
	w->plot.x.Center = w->plot.x.Origin + (width / 2.0);
	w->plot.y.Center = w->plot.y.Origin + (height / 2.0);

}

static void AdjustDimensionsCartesian (SciPlotWidget w)
{
	float xextra, yextra, val, xhorz;
	float x, y, width, height, axisnumbersize, axisXlabelsize, axisYlabelsize;
	char numberformat[16], label[16];
	int precision;

	/* Compute xextra and yextra, which are the extra distances that the text
	 * labels on the axes stick outside of the graph. This is done when the
	 * numbers are actually showing and when they are hidded so as to get the
	 * same axis length in both cases. Useful when displaying multiple vertically
	 * stacked graphs.
	 */
	xextra = yextra = 0.0;
	if (w->plot.XAxisNumbers)
	{
		precision = w->plot.x.Precision;
		if (w->plot.XLog)
		{
			val = w->plot.x.DrawMax;
			precision -= w->plot.x.MajorNum;
			if (precision < 0)
				precision = 0;
		}
		else
			val = w->plot.x.DrawOrigin + floor(w->plot.x.DrawSize /
											   w->plot.x.MajorInc) * w->plot.x.MajorInc;
		x = PlotX(w, val);
		sprintf(numberformat, "%%.%df", precision);
		sprintf(label, numberformat, val);
		x += TextWidth(w, w->plot.axisFont.tag, label);
		if ((int) x > w->core.width)
		{
			xextra = ceil(x - w->core.width + w->plot.MarginWidth);
			if (xextra < 0.0)
				xextra = 0.0;
		}
	}

	yextra = xhorz = 0.0;
	if (w->plot.YAxisNumbers)
	{
		precision = w->plot.y.Precision;
		if (w->plot.YLog)
		{
			int p1, p2;

			p1 = precision;
			val = w->plot.y.DrawOrigin;
			if (p1 > 0)
				p1--;

			val = w->plot.y.DrawMax;
			p2 = precision - w->plot.y.MajorNum;
			if (p2 < 0)
				p2 = 0;

			if (p1 > p2) precision = p1;
			else precision = p2;
		}
		else
		{
			val = w->plot.y.DrawOrigin + floor(w->plot.y.DrawSize /
											   w->plot.y.MajorInc * 1.0001) * w->plot.y.MajorInc;
		}
		y = PlotY(w, val);
		sprintf(numberformat, "%%.%df", precision);
		sprintf(label, numberformat, val);
		if (w->plot.YNumHorz)
		{
			yextra = (float) w->plot.axisFont.height / 2.0;
			xhorz = TextWidth(w, w->plot.axisFont.tag, label);
			/*
			 * If YAxisNumberNumDigits has a value it is assumed that the y-axis numbers
			 * will have this minimum number of digits and this is then applied to the
			 * y-axis numbers offset. This is useful for vertically aligning graphs.
			 */
			if(w->plot.YAxisNumberNumDigits > 0)
			{
				int nc, fill = 15;
				char buf[16];
				memset(buf, 0, 16);
				if (w->plot.YAxisNumberNumDigits < 15) fill = w->plot.YAxisNumberNumDigits;
				memset(buf, '0', fill);
				nc = TextWidth(w, w->plot.axisFont.tag, buf);
				if(nc > xhorz) xhorz = nc;
			}
		}
		else
		{
			y -= TextWidth(w, w->plot.axisFont.tag, label);
			if ((int) y <= 0)
			{
				yextra = ceil(w->plot.MarginHeight - y);
				if (yextra < 0.0)
					yextra = 0.0;
			}
		}
	}

	/* x,y is the origin of the upper left corner of the drawing area inside
	 * the widget.  Doesn't necessarily have to be (Margin,Margin) as it is now.
	 */
	x = (float) w->plot.MarginWidth + xhorz;
	y = (float) w->plot.MarginHeight + yextra;
	width = (float) w->core.width - (float) w->plot.MarginWidth - x - xextra;
	height = (float) w->core.height - (float) w->plot.MarginHeight - y;

	w->plot.x.Origin = x;
	w->plot.y.Origin = y;

	/* Adjust the size depending upon what sorts of text are visible. */
	if (w->plot.ShowTitle)
		height -= (float) (w->plot.TitleMargin + w->plot.titleFont.height);

	axisXlabelsize = 0.0;
	axisYlabelsize = 0.0;
	if (w->plot.XAxisNumbers == XmNUMBERS_SHOW)
	{
		axisnumbersize = (float) (w->plot.MarginHeight + w->plot.axisFont.height);
		height -= axisnumbersize;
	}
	if (w->plot.YAxisNumbers == XmNUMBERS_SHOW && !w->plot.YNumHorz)
	{
		axisnumbersize = (float) (w->plot.MarginWidth + w->plot.axisFont.height);
		width -= axisnumbersize;
		w->plot.x.Origin += axisnumbersize;
	}

	if (w->plot.ShowXLabel)
	{
		axisXlabelsize = (float) (w->plot.MarginHeight + w->plot.labelFont.height);
		height -= axisXlabelsize;
	}
	if (w->plot.ShowYLabel)
	{
		axisYlabelsize = (float) (w->plot.MarginWidth + w->plot.labelFont.height);
		width -= axisYlabelsize;
		w->plot.x.Origin += axisYlabelsize;
	}

	/* Move legend position to the right of the plot and optionally at bottom */
	if (w->plot.LegendThroughPlot)
	{
		w->plot.x.LegendPos += w->plot.x.Origin + width - w->plot.x.LegendSize;
		w->plot.y.LegendPos += w->plot.y.Origin;
		if(w->plot.LegendAtBottom)
			w->plot.y.LegendPos += height - w->plot.y.LegendSize;
	}
	else
	{
		width -= w->plot.x.LegendSize;
		w->plot.x.LegendPos += w->plot.x.Origin + width;
		w->plot.y.LegendPos += w->plot.y.Origin;
		if(w->plot.LegendAtBottom)
			w->plot.y.LegendPos += height - w->plot.y.LegendSize;
	}

	w->plot.x.Size = width;
	w->plot.y.Size = height;

	w->plot.y.AxisPos = w->plot.y.Origin + w->plot.y.Size + (float) (w->plot.MarginHeight + w->plot.axisFont.ascent);
	if (w->plot.YNumHorz)
	{
		w->plot.x.AxisPos = w->plot.x.Origin - (float) w->plot.MarginWidth;
	}
	else
	{
		w->plot.x.AxisPos = w->plot.x.Origin - (float) (w->plot.MarginWidth - w->plot.axisFont.descent);
	}

	w->plot.y.LabelPos = w->plot.y.Origin + w->plot.y.Size + (float) w->plot.MarginHeight +
		((float) w->plot.labelFont.height / 2.0);
	if (w->plot.XAxisNumbers == XmNUMBERS_SHOW)
		w->plot.y.LabelPos += axisnumbersize;
	if (w->plot.YAxisNumbers == XmNUMBERS_SHOW)
	{
		if (w->plot.YNumHorz)
		{
			w->plot.x.LabelPos = w->plot.x.Origin - xhorz - (float) w->plot.MarginWidth -
				((float) w->plot.labelFont.height / 2.0);
		}
		else
		{
			w->plot.x.LabelPos = w->plot.x.Origin - axisnumbersize - (float) w->plot.MarginWidth -
				((float) w->plot.labelFont.height / 2.0);
		}
	}
	else
	{
		w->plot.x.LabelPos = w->plot.x.Origin - (float) w->plot.MarginWidth -
			((float) w->plot.labelFont.height / 2.0);
	}

	w->plot.y.TitlePos = (float) (w->core.height - w->plot.MarginHeight);
	w->plot.x.TitlePos = (float) w->plot.MarginWidth;
}

static void AdjustDimensionsPolar (SciPlotWidget w)
{
	float x, y, xextra, yextra, val;
	float width, height, size;
	char numberformat[16], label[16];

	/* Compute xextra and yextra, which are the extra distances that the text
	 * labels on the axes stick outside of the graph.
	 */
	xextra = yextra = 0.0;
	val = w->plot.PolarScale;
	PlotRTDegrees(w, val, 0.0, &x, &y);
	sprintf(numberformat, "%%.%df", w->plot.x.Precision);
	sprintf(label, numberformat, val);
	x += TextWidth(w, w->plot.axisFont.tag, label);
	if ((int) x > w->core.width)
	{
		xextra = x - w->core.width + w->plot.MarginWidth;
		if (xextra < 0.0)
			xextra = 0.0;
	}
	yextra = 0.0;


	/* x,y is the origin of the upper left corner of the drawing area inside
	 * the widget.  Doesn't necessarily have to be (Margin,Margin) as it is now.
	 */
	w->plot.x.Origin = (float) w->plot.MarginWidth;
	w->plot.y.Origin = (float) w->plot.MarginHeight;

	/* width = (float)w->core.width - (float)w->plot.Margin - x -
	 *          legendwidth - AxisFontHeight
	 */
	width = (float) w->core.width - (float) w->plot.MarginWidth - w->plot.x.Origin - xextra;

	/* height = (float)w->core.height - (float)w->plot.Margin - y
	 *           - Height of axis numbers (including margin)
	 *           - Height of axis label (including margin)
	 *           - Height of Title (including margin)
	 */
	height = (float) w->core.height - (float) w->plot.MarginHeight - w->plot.y.Origin - yextra;

	/* Adjust the size depending upon what sorts of text are visible. */
	if (w->plot.ShowTitle)
		height -= (float) (w->plot.TitleMargin + w->plot.titleFont.height);

	/* Only need to carry one number for the size, (since it is a circle!) */
	if (height < width)
		size = height;
	else
		size = width;

	/* Assign some preliminary values */
	w->plot.x.Center = w->plot.x.Origin + (width / 2.0);
	w->plot.y.Center = w->plot.y.Origin + (height / 2.0);
	w->plot.x.LegendPos += width - w->plot.x.LegendSize;
	w->plot.y.LegendPos += w->plot.y.Origin;

	/*
	 * Check and see if the legend can fit in the blank space in the upper right
	 *
	 * To fit, the legend must:
	 *   1) be less than half the width/height of the plot
	 *   2) hmmm.
	 */
	if (!w->plot.LegendThroughPlot)
	{
		float radius = size / 2.0;
		float dist;

		x = w->plot.x.LegendPos - w->plot.x.Center;
		y = (w->plot.y.LegendPos + w->plot.y.LegendSize) - w->plot.y.Center;

		dist = sqrt(x * x + y * y);
		/*       printf("rad=%f dist=%f: legend=(%f,%f) center=(%f,%f)\n", */
		/*              radius,dist,w->plot.x.LegendPos,w->plot.y.LegendPos, */
		/*              w->plot.x.Center,w->plot.y.Center); */

		/* It doesn't fit if this check is true.  Make the plot smaller */

		/* This is a first cut horrible algorithm.  My calculus is a bit
		 * rusty tonight--can't seem to figure out how to maximize a circle
		 * in a rectangle with a rectangular chunk out of it. */
		if (dist < radius)
		{
			width -= w->plot.x.LegendSize;
			height -= w->plot.y.LegendSize;

			/* readjust some parameters */
			w->plot.x.Center = w->plot.x.Origin + width / 2.0;
			w->plot.y.Center = w->plot.y.Origin + w->plot.y.LegendSize + height / 2.0;
			if (height < width)
				size = height;
			else
				size = width;
		}
	}


	/* OK, customization is finished when we reach here. */
	w->plot.x.Size = w->plot.y.Size = size;

	w->plot.y.TitlePos = w->plot.y.Center + w->plot.y.Size / 2.0 +
		(float) (w->plot.TitleMargin + w->plot.titleFont.ascent);
	w->plot.x.TitlePos = w->plot.x.Origin;
}

static void AdjustDimensions (SciPlotWidget w)
{
	if (w->plot.ChartType == XmCARTESIAN)
	{
		AdjustDimensionsCartesian(w);
	}
	else
	{
		AdjustDimensionsPolar(w);
	}
}


static void ComputeAll (SciPlotWidget w)
{
	ComputeMinMax(w);
	ComputeLegendDimensions(w);
	ComputeDimensions(w);
	ComputeDrawingRange(w);
	AdjustDimensions(w);
}


/*
 * Private drawing routines
 */

static void DrawMarker (SciPlotWidget w, float xpaper, float ypaper, float size, Pixel color, int style)
{
	float sizex, sizey;

	switch (style)
	{
	case XmMARKER_CIRCLE:
		CircleSet(w, xpaper, ypaper, size, color, XmLINE_SOLID);
		break;
	case XmMARKER_FCIRCLE:
		FilledCircleSet(w, xpaper, ypaper, size, color, XmLINE_SOLID);
		break;
	case XmMARKER_SQUARE:
		size -= .5;
		RectSet(w, xpaper - size, ypaper - size,
				xpaper + size, ypaper + size,
				color, XmLINE_SOLID);
		break;
	case XmMARKER_FSQUARE:
		size -= .5;
		FilledRectSet(w, xpaper - size, ypaper - size,
					  xpaper + size, ypaper + size,
					  color, XmLINE_SOLID);
		break;
	case XmMARKER_UTRIANGLE:
		sizex = size * .866;
		sizey = size / 2.0;
		TriSet(w, xpaper, ypaper - size,
			   xpaper + sizex, ypaper + sizey,
			   xpaper - sizex, ypaper + sizey,
			   color, XmLINE_SOLID);
		break;
	case XmMARKER_FUTRIANGLE:
		sizex = size * .866;
		sizey = size / 2.0;
		FilledTriSet(w, xpaper, ypaper - size,
					 xpaper + sizex, ypaper + sizey,
					 xpaper - sizex, ypaper + sizey,
					 color, XmLINE_SOLID);
		break;
	case XmMARKER_DTRIANGLE:
		sizex = size * .866;
		sizey = size / 2.0;
		TriSet(w, xpaper, ypaper + size,
			   xpaper + sizex, ypaper - sizey,
			   xpaper - sizex, ypaper - sizey,
			   color, XmLINE_SOLID);
		break;
	case XmMARKER_FDTRIANGLE:
		sizex = size * .866;
		sizey = size / 2.0;
		FilledTriSet(w, xpaper, ypaper + size,
					 xpaper + sizex, ypaper - sizey,
					 xpaper - sizex, ypaper - sizey,
					 color, XmLINE_SOLID);
		break;
	case XmMARKER_RTRIANGLE:
		sizey = size * .866;
		sizex = size / 2.0;
		TriSet(w, xpaper + size, ypaper,
			   xpaper - sizex, ypaper + sizey,
			   xpaper - sizex, ypaper - sizey,
			   color, XmLINE_SOLID);
		break;
	case XmMARKER_FRTRIANGLE:
		sizey = size * .866;
		sizex = size / 2.0;
		FilledTriSet(w, xpaper + size, ypaper,
					 xpaper - sizex, ypaper + sizey,
					 xpaper - sizex, ypaper - sizey,
					 color, XmLINE_SOLID);
		break;
	case XmMARKER_LTRIANGLE:
		sizey = size * .866;
		sizex = size / 2.0;
		TriSet(w, xpaper - size, ypaper,
			   xpaper + sizex, ypaper + sizey,
			   xpaper + sizex, ypaper - sizey,
			   color, XmLINE_SOLID);
		break;
	case XmMARKER_FLTRIANGLE:
		sizey = size * .866;
		sizex = size / 2.0;
		FilledTriSet(w, xpaper - size, ypaper,
					 xpaper + sizex, ypaper + sizey,
					 xpaper + sizex, ypaper - sizey,
					 color, XmLINE_SOLID);
		break;
	case XmMARKER_DIAMOND:
		QuadSet(w, xpaper, ypaper - size,
				xpaper + size, ypaper,
				xpaper, ypaper + size,
				xpaper - size, ypaper,
				color, XmLINE_SOLID);
		break;
	case XmMARKER_FDIAMOND:
		FilledQuadSet(w, xpaper, ypaper - size,
					  xpaper + size, ypaper,
					  xpaper, ypaper + size,
					  xpaper - size, ypaper,
					  color, XmLINE_SOLID);
		break;
	case XmMARKER_HOURGLASS:
		QuadSet(w, xpaper - size, ypaper - size,
				xpaper + size, ypaper - size,
				xpaper - size, ypaper + size,
				xpaper + size, ypaper + size,
				color, XmLINE_SOLID);
		break;
	case XmMARKER_FHOURGLASS:
		FilledQuadSet(w, xpaper - size, ypaper - size,
					  xpaper + size, ypaper - size,
					  xpaper - size, ypaper + size,
					  xpaper + size, ypaper + size,
					  color, XmLINE_SOLID);
		break;
	case XmMARKER_BOWTIE:
		QuadSet(w, xpaper - size, ypaper - size,
				xpaper - size, ypaper + size,
				xpaper + size, ypaper - size,
				xpaper + size, ypaper + size,
				color, XmLINE_SOLID);
		break;
	case XmMARKER_FBOWTIE:
		FilledQuadSet(w, xpaper - size, ypaper - size,
					  xpaper - size, ypaper + size,
					  xpaper + size, ypaper - size,
					  xpaper + size, ypaper + size,
					  color, XmLINE_SOLID);
		break;
	case XmMARKER_DOT:
		FilledCircleSet(w, xpaper, ypaper, 1.5, color, XmLINE_SOLID);
		break;

	default:
		break;
	}
}

static void DrawLegend (SciPlotWidget w)
{
	float x, y, len, height, height2, len2, ascent;
	int i;
	SciPlotList *p;

	w->plot.current_id = SciPlotDrawingLegend;
	if (w->plot.ShowLegend)
	{
		x = w->plot.x.LegendPos;
		y = w->plot.y.LegendPos;
		len = (float) w->plot.LegendLineSize;
		len2 = len / 2.0;
		height = (float) w->plot.legendFont.height;
		height2 = height / 2.0;
		ascent = (float) w->plot.legendFont.ascent;
		RectSet(w, x, y,
				x + w->plot.x.LegendSize - 1.0 - (float) w->plot.MarginWidth,
				y + w->plot.y.LegendSize - 1.0,
				w->plot.ForegroundColor, XmLINE_SOLID);
		x += (float) w->plot.LegendMargin;
		y += (float) w->plot.LegendMargin;

		for (i = 0; i < w->plot.num_plotlist; i++)
		{
			p = w->plot.plotlist + i;
			if (p->draw && p->legend)
			{
				/* If the line size is 0 then do not output the marker symbol */
				if(w->plot.LegendLineSize > 0)
				{
					LineSet(w, x, y + height2, x + len, y + height2,
						p->LineColor, p->LineStyle);
					DrawMarker(w, x + len2, y + height2, p->markersize,
						   p->PointColor, p->PointStyle);
				}
				TextSet(w, x + len + (float) w->plot.LegendMargin,
						y + ascent,
						p->legend, w->plot.ForegroundColor,
						&w->plot.legendFont);
				y += height;
			}
		}
	}
}

static void DrawCartesianAxes (SciPlotWidget w)
{
	Boolean constrain;
	float x, y, x1, y1, x2, y2, tic, val, height, majorval, xoff;
	float increment = 0, threshold = 0;
	int j, precision;
	char numberformat[16], label[16];

	w->plot.current_id = SciPlotDrawingAxis;
	height = (float) w->plot.labelFont.height;
	x1 = PlotX(w, w->plot.x.DrawOrigin);
	y1 = PlotY(w, w->plot.y.DrawOrigin);
	x2 = PlotX(w, w->plot.x.DrawMax);
	y2 = PlotY(w, w->plot.y.DrawMax);
	LineSet(w, x1, y1, x2, y1, w->plot.ForegroundColor, XmLINE_SOLID);
	LineSet(w, x1, y1, x1, y2, w->plot.ForegroundColor, XmLINE_SOLID);

	precision = w->plot.x.Precision;
	sprintf(numberformat, "%%.%df", precision);
	if (w->plot.XLog)
	{
		val = w->plot.x.DrawOrigin;
		if (precision > 0)
			precision--;
	}
	else
	{
		val = w->plot.x.DrawOrigin;
	}
	x = PlotX(w, val);
	if (w->plot.DrawMajorTics)
		LineSet(w, x, y1 + 5, x, y1 - 5, w->plot.ForegroundColor, XmLINE_SOLID);
	if (w->plot.XAxisNumbers == XmNUMBERS_SHOW)
	{
		sprintf(label, numberformat, val);
		xoff = TextWidth(w, w->plot.axisFont.tag, label)/2.;
		TextSet(w, x - xoff, w->plot.y.AxisPos, label, w->plot.ForegroundColor,
				&w->plot.axisFont);
	}

	if((constrain = (w->plot.XAxisMaxNumbers > 0)))
	{
		increment = (w->plot.x.DrawMax - val)/(float) w->plot.XAxisMaxNumbers;
		threshold = val + increment;
	}
	else
	{
		threshold = increment = 0;
	}

	majorval = val;
	while ((majorval * 1.0001) < w->plot.x.DrawMax)
	{
		if (w->plot.XLog)
		{

			/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
			if (majorval * 1.1 > w->plot.x.DrawMax)
				break;
			tic = majorval;
			if (w->plot.DrawMinor || w->plot.DrawMinorTics)
			{
				for (j = 2; j < w->plot.x.MinorNum; j++)
				{
					val = tic * (float) j;
					x = PlotX(w, val);
					if (w->plot.DrawMinor)
						LineSet(w, x, y1, x, y2,
								w->plot.ForegroundColor,
								XmLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w, x, y1, x, y1 - 3,
								w->plot.ForegroundColor,
								XmLINE_SOLID);
				}
			}
			val = tic * (float) w->plot.x.MinorNum;
			sprintf(numberformat, "%%.%df", precision);
			if (precision > 0)
				precision--;
		}
		else
		{
			tic = majorval;
			if (DRAW_MINOR(x) || w->plot.DrawMinorTics)
			{
				for (j = 1; j < w->plot.x.MinorNum; j++)
				{
					val = tic + w->plot.x.MajorInc * (float) j / (float) w->plot.x.MinorNum;
					x = PlotX(w, val);
					if (DRAW_MINOR(x))
						LineSet(w, x, y1, x, y2,
								w->plot.ForegroundColor,
								XmLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w, x, y1, x, y1 - 3,
								w->plot.ForegroundColor,
								XmLINE_SOLID);
				}
			}
			val = tic + w->plot.x.MajorInc;
		}
		x = PlotX(w, val);
		if (w->plot.DrawMajor)
			LineSet(w, x, y1, x, y2, w->plot.ForegroundColor, XmLINE_DOTTED);
		else if (DRAW_MINOR(x))
			LineSet(w, x, y1, x, y2, w->plot.ForegroundColor, XmLINE_WIDEDOT);
		if (w->plot.DrawMajorTics)
			LineSet(w, x, y1 + 5, x, y1 - 5, w->plot.ForegroundColor, XmLINE_SOLID);
		if (w->plot.XAxisNumbers == XmNUMBERS_SHOW)
		{
			/* roundoff problems again */
			if(constrain && val < (threshold-.0001) && (val * 1.0001) < w->plot.x.DrawMax)
			{
				majorval = val;
				continue;
			}
			else
			{
				sprintf(label, numberformat, val);
				xoff = TextWidth(w, w->plot.axisFont.tag, label)/2.;
				TextSet(w, x - xoff, w->plot.y.AxisPos, label, w->plot.ForegroundColor,
					&w->plot.axisFont);
				threshold += increment;
			}
		}
		majorval = val;
	}

	precision = w->plot.y.Precision;
	sprintf(numberformat, "%%.%df", precision);
	if (w->plot.YLog)
	{
		val = w->plot.y.DrawOrigin;
		if (precision > 0)
			precision--;
	}
	else
	{
		val = w->plot.y.DrawOrigin;
	}
	y = PlotY(w, val);
	if (w->plot.DrawMajorTics)
		LineSet(w, x1 + 5, y, x1 - 5, y, w->plot.ForegroundColor, XmLINE_SOLID);
	if (w->plot.YAxisNumbers == XmNUMBERS_SHOW)
	{
		sprintf(label, numberformat, val);
		if (w->plot.YNumHorz)
		{
			y += ((float) w->plot.axisFont.height / 2.0) - (float) w->plot.axisFont.descent;
			TextSet(w,
					w->plot.x.AxisPos - TextWidth(w, w->plot.axisFont.tag, label),
					y, label, w->plot.ForegroundColor,
					&w->plot.axisFont);
		}
		else
		{
			VTextSet(w, w->plot.x.AxisPos, y, label, w->plot.ForegroundColor,
					 &w->plot.axisFont);
		}
	}
	majorval = val;

	if((constrain = (w->plot.YAxisMaxNumbers > 0)))
	{
		increment = (w->plot.y.DrawMax - val)/(float) w->plot.YAxisMaxNumbers;
		threshold = val + increment;
	}
	else
	{
		threshold = increment = 0;
	}

	/* majorval*1.0001 is a fudge to get rid of rounding errors that seem to
	 * occur when continuing to add the major axis increment.
	 */
	while ((majorval * 1.0001) < w->plot.y.DrawMax)
	{
		if (w->plot.YLog)
		{

			/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
			if (majorval * 1.1 > w->plot.y.DrawMax)
				break;
			tic = majorval;
			if (w->plot.DrawMinor || w->plot.DrawMinorTics)
			{
				for (j = 2; j < w->plot.y.MinorNum; j++)
				{
					val = tic * (float) j;
					y = PlotY(w, val);
					if (w->plot.DrawMinor)
						LineSet(w, x1, y, x2, y,
								w->plot.ForegroundColor,
								XmLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w, x1, y, x1 + 3, y,
								w->plot.ForegroundColor,
								XmLINE_SOLID);
				}
			}
			val = tic * (float) w->plot.y.MinorNum;
			sprintf(numberformat, "%%.%df", precision);
			if (precision > 0)
				precision--;
		}
		else
		{
			tic = majorval;
			if (DRAW_MINOR(y) || w->plot.DrawMinorTics)
			{
				for (j = 1; j < w->plot.y.MinorNum; j++)
				{
					val = tic + w->plot.y.MajorInc * (float) j /
						  w->plot.y.MinorNum;
					y = PlotY(w, val);
					if (DRAW_MINOR(y))
						LineSet(w, x1, y, x2, y,
								w->plot.ForegroundColor,
								XmLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w, x1, y, x1 + 3, y,
								w->plot.ForegroundColor,
								XmLINE_SOLID);
				}
			}
			val = tic + w->plot.y.MajorInc;
		}
		y = PlotY(w, val);
		if (w->plot.DrawMajor)
			LineSet(w, x1, y, x2, y, w->plot.ForegroundColor, XmLINE_DOTTED);
		else if (DRAW_MINOR(y))
			LineSet(w, x1, y, x2, y, w->plot.ForegroundColor, XmLINE_WIDEDOT);
		if (w->plot.DrawMajorTics)
			LineSet(w, x1 - 5, y, x1 + 5, y, w->plot.ForegroundColor, XmLINE_SOLID);
		if (w->plot.YAxisNumbers == XmNUMBERS_SHOW)
		{
			/* roundoff problems again */
			if(constrain && val < (threshold-.0001) && (val * 1.0001) < w->plot.y.DrawMax)
			{
				majorval = val;
				continue;
			}
			else
			{
				sprintf(label, numberformat, val);
				if (w->plot.YNumHorz)
				{
					y += ((float) w->plot.axisFont.height / 2.0) - (float) w->plot.axisFont.descent;
					TextSet(w,
							w->plot.x.AxisPos - TextWidth(w, w->plot.axisFont.tag, label),
							y, label, w->plot.ForegroundColor,
							&w->plot.axisFont);
				}
				else
				{
					VTextSet(w, w->plot.x.AxisPos, y, label, w->plot.ForegroundColor,
							 &w->plot.axisFont);
				}
				threshold += increment;
			}
		}
		majorval = val;
	}

	if (w->plot.ShowTitle)
		TextSet(w, w->plot.x.TitlePos, w->plot.y.TitlePos,
				w->plot.plotTitle, w->plot.ForegroundColor,
				&w->plot.titleFont);
	if (w->plot.ShowXLabel)
		TextCenter(w, w->plot.x.Origin + (w->plot.x.Size / 2.0),
				   w->plot.y.LabelPos, w->plot.xlabel,
				   w->plot.ForegroundColor, &w->plot.labelFont);
	if (w->plot.ShowYLabel)
		VTextCenter(w, w->plot.x.LabelPos,
					w->plot.y.Origin + (w->plot.y.Size / 2.0),
					w->plot.ylabel, w->plot.ForegroundColor,
					&w->plot.labelFont);
}


static void DrawCartesianPlot (SciPlotWidget w)
{
	int i, j, jstart;
	SciPlotList *p;

	w->plot.current_id = SciPlotDrawingAny;
	ClipSet(w);
	w->plot.current_id = SciPlotDrawingLine;
	for (i = 0; i < w->plot.num_plotlist; i++)
	{
		p = w->plot.plotlist + i;
		if (p->draw)
		{
			float x1, y1, x2, y2;
			Boolean skipnext = False;

			jstart = 0;
			while ((jstart < p->number) &&
					(((p->data[jstart].x <= SCIPLOT_SKIP_VAL ||
					   p->data[jstart].y <= SCIPLOT_SKIP_VAL) ||
					  (w->plot.XLog && (p->data[jstart].x <= 0.0)) ||
					  (w->plot.YLog && (p->data[jstart].y <= 0.0)))))
				jstart++;
			if (jstart < p->number)
			{
				x1 = PlotX(w, p->data[jstart].x);
				y1 = PlotY(w, p->data[jstart].y);
			}
			for (j = jstart; j < p->number; j++)
			{
				if (p->data[j].x <= SCIPLOT_SKIP_VAL ||
						p->data[j].y <= SCIPLOT_SKIP_VAL)
				{
					skipnext = True;
					continue;
				}

				if (!((w->plot.XLog && (p->data[j].x <= 0.0)) ||
						(w->plot.YLog && (p->data[j].y <= 0.0))))
				{
					x2 = PlotX(w, p->data[j].x);
					y2 = PlotY(w, p->data[j].y);
					if (!skipnext)
						LineSet(w, x1, y1, x2, y2, p->LineColor, p->LineStyle);
					x1 = x2;
					y1 = y2;
				}

				skipnext = False;
			}
		}
	}
	w->plot.current_id = SciPlotDrawingAny;
	ClipClear(w);
	w->plot.current_id = SciPlotDrawingLine;
	for (i = 0; i < w->plot.num_plotlist; i++)
	{
		p = w->plot.plotlist + i;
		if (p->draw)
		{
			float x2, y2;

			for (j = 0; j < p->number; j++)
			{
				if (!((w->plot.XLog && (p->data[j].x <= 0.0)) ||
						(w->plot.YLog && (p->data[j].y <= 0.0)) ||
						p->data[j].x <= SCIPLOT_SKIP_VAL ||
						p->data[j].y <= SCIPLOT_SKIP_VAL ))
				{
					x2 = PlotX(w, p->data[j].x);
					y2 = PlotY(w, p->data[j].y);
					if ((x2 >= w->plot.x.Origin) &&
							(x2 <= w->plot.x.Origin + w->plot.x.Size) &&
							(y2 >= w->plot.y.Origin) &&
							(y2 <= w->plot.y.Origin + w->plot.y.Size))
					{

						DrawMarker(w, x2, y2, p->markersize, p->PointColor, p->PointStyle);
					}

				}
			}
		}
	}
}

static void DrawPolarAxes (SciPlotWidget w)
{
	float x1, y1, x2, y2, max, tic, val, height;
	int i, j;
	char numberformat[16], label[16];

	w->plot.current_id = SciPlotDrawingAxis;
	sprintf(numberformat, "%%.%df", w->plot.x.Precision);
	height = (float) w->plot.labelFont.height;
	max = w->plot.PolarScale;
	PlotRTDegrees(w, 0.0, 0.0, &x1, &y1);
	PlotRTDegrees(w, max, 0.0, &x2, &y2);
	LineSet(w, x1, y1, x2, y2, 1, XmLINE_SOLID);
	for (i = 45; i < 360; i += 45)
	{
		PlotRTDegrees(w, max, (float) i, &x2, &y2);
		LineSet(w, x1, y1, x2, y2, w->plot.ForegroundColor, XmLINE_DOTTED);
	}
	for (i = 1; i <= w->plot.x.MajorNum; i++)
	{
		tic = w->plot.PolarScale * (float) i / (float) w->plot.x.MajorNum;
		if (w->plot.DrawMinor || w->plot.DrawMinorTics)
		{
			for (j = 1; j < w->plot.x.MinorNum; j++)
			{
				val = tic - w->plot.x.MajorInc * (float) j /
					  w->plot.x.MinorNum;
				PlotRTDegrees(w, val, 0.0, &x2, &y2);
				if (w->plot.DrawMinor)
					CircleSet(w, x1, y1, x2 - x1,
							  w->plot.ForegroundColor, XmLINE_WIDEDOT);
				if (w->plot.DrawMinorTics)
					LineSet(w, x2, y2 - 2.5, x2, y2 + 2.5,
							w->plot.ForegroundColor, XmLINE_SOLID);
			}
		}
		PlotRTDegrees(w, tic, 0.0, &x2, &y2);
		if (w->plot.DrawMajor)
			CircleSet(w, x1, y1, x2 - x1, w->plot.ForegroundColor, XmLINE_DOTTED);
		if (w->plot.DrawMajorTics)
			LineSet(w, x2, y2 - 5.0, x2, y2 + 5.0, w->plot.ForegroundColor, XmLINE_SOLID);
		if (w->plot.XAxisNumbers == XmNUMBERS_SHOW)
		{
			sprintf(label, numberformat, tic);
			TextSet(w, x2, y2 + height, label, w->plot.ForegroundColor, &w->plot.axisFont);
		}
	}

	if (w->plot.ShowTitle)
		TextSet(w, w->plot.x.TitlePos, w->plot.y.TitlePos,
				w->plot.plotTitle, w->plot.ForegroundColor, &w->plot.titleFont);
}

static void DrawPolarPlot (SciPlotWidget w)
{
	int i, j;
	SciPlotList *p;

	w->plot.current_id = SciPlotDrawingLine;
	for (i = 0; i < w->plot.num_plotlist; i++)
	{
		p = w->plot.plotlist + i;
		if (p->draw)
		{
			int jstart;
			float x1, y1, x2, y2;
			Boolean skipnext = False;

			jstart = 0;
			while ((jstart < p->number) &&
					(p->data[jstart].x <= SCIPLOT_SKIP_VAL ||
					 p->data[jstart].y <= SCIPLOT_SKIP_VAL))
				jstart++;
			if (jstart < p->number)
			{
				PlotRT(w, p->data[0].x, p->data[0].y, &x1, &y1);
			}
			for (j = jstart; j < p->number; j++)
			{
				if (p->data[j].x <= SCIPLOT_SKIP_VAL ||
						p->data[j].y <= SCIPLOT_SKIP_VAL)
				{
					skipnext = True;
					continue;
				}

				PlotRT(w, p->data[j].x, p->data[j].y, &x2, &y2);
				if (!skipnext)
				{
					LineSet(w, x1, y1, x2, y2,
							p->LineColor, p->LineStyle);
					DrawMarker(w, x1, y1, p->markersize,
							   p->PointColor, p->PointStyle);
					DrawMarker(w, x2, y2, p->markersize,
							   p->PointColor, p->PointStyle);
				}
				x1 = x2;
				y1 = y2;

				skipnext = False;
			}
		}
	}
}


/* If an update is required draw the axes and legend into the pixmap. The data
 * plot is always done to the window after the pixmap is copied.
 */
static void DrawAll (SciPlotWidget w)
{
	if (w->plot.ChartType == XmCARTESIAN)
	{
		if(w->plot.update)
		{
			DrawCartesianAxes(w);
			DrawLegend(w);
		}
		XCopyArea(XtDisplay(w), w->plot.pix, XtWindow(w), w->plot.defaultGC,
			0, 0, w->plot.pix_width, w->plot.pix_height, 0, 0);
		w->plot.draw = XtWindow(w);
		DrawCartesianPlot(w);
		w->plot.draw = w->plot.pix;
	}
	else
	{
		if(w->plot.update)
		{
			DrawPolarAxes(w);
			DrawLegend(w);
		}
		XCopyArea(XtDisplay(w), w->plot.pix, XtWindow(w), w->plot.defaultGC,
			0, 0, w->plot.pix_width, w->plot.pix_height, 0, 0);
		w->plot.draw = XtWindow(w);
		DrawPolarPlot(w);
		w->plot.draw = w->plot.pix;
	}
	w->plot.update = False;
}


/* Create a pixmap with the same size and depth as the plot window.
 */
static void PixmapUpdate (SciPlotWidget w)
{
	if(w->plot.pix_width == (UINT) w->core.width && w->plot.pix_height == (UINT) w->core.height) return;

	w->plot.pix_width = (UINT) w->core.width;
	w->plot.pix_height = (UINT) w->core.height;
	if(w->plot.pix != (Pixmap)0)
		XFreePixmap(XtDisplay(w), w->plot.pix);
	w->plot.pix = XCreatePixmap(XtDisplay(w), XtWindow(w),
		w->plot.pix_width, w->plot.pix_height, (UINT) w->core.depth);
	w->plot.draw = w->plot.pix;
}


/*
 * Public Plot functions
 */

void SciPlotSetBackgroundColor (Widget wi, Pixel color)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	w->core.background_pixel = color;
	w->plot.BackgroundColor = color;
	XSetWindowBackground( XtDisplay(w), XtWindow(w), color);
}

void SciPlotSetForegroundColor (Widget wi, Pixel color)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	w->plot.ForegroundColor = color;
}

void SciPlotListDelete (Widget wi, int idnum)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
		_ListDelete(p);
}

void SciPlotListDeleteAll(Widget wi)
{
	int n;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	for(n = 0; n < w->plot.num_plotlist; n++)
	{
		SciPlotList *p = w->plot.plotlist + n;
		if(p->used) _ListDelete(p);
	}
}


int SciPlotListCreate (Widget wi, int numPoints, SciPlotPoint *points, char *legend)
{
	int idnum;
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return -1;

	idnum = _ListNew(w);
	p = w->plot.plotlist + idnum;
	_ListSetPoint(p, numPoints, points);
	_ListSetLegend(p, legend);
	_ListSetStyle(wi, p, 1, XmMARKER_CIRCLE, 1, XmLINE_SOLID);
	return idnum;
}

void SciPlotListUpdate (Widget wi, int idnum, int numPoints, SciPlotPoint *points)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
		_ListSetPoint(p, numPoints, points);
}


void SciPlotListAdd (Widget wi, int idnum, int numPoints, SciPlotPoint *points)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
		_ListAddPoint(p, numPoints, points);
}


void SciPlotListSetStyle (Widget wi, int idnum, Pixel pcolor, int pstyle, Pixel lcolor, int lstyle)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
		_ListSetStyle(wi, p, pcolor, pstyle, lcolor, lstyle);
}


void SciPlotListSetMarkerSize (Widget wi, int idnum, float size)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
		p->markersize = size;
}


/* Sets the type of move to do for a point in the list. The start and end
 * parameters limit the range of the points that are selected for a move.
 * If end == 0 then a search is done from start to the end of the list.
 */
void SciPlotListSetPointMove (Widget wi, int idnum, int type, int start, int end)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	switch(type)
	{
		case XmMOVE_XY:
		case XmMOVE_X_ONLY:
		case XmMOVE_Y_ONLY:
			break;
		default:
			XtAppErrorMsg(XtWidgetToApplicationContext(wi),
                "SciPlotListSetPointMove", "badMoveType", "sciplotWidgetClass",
                "SciPlot: Unrecognized point move type specified",
                NULL, 0);
			return;
	}

	p = _ListFind(w, idnum);
	if (p)
	{
		p->move.type = type;
		p->move.start_point = start;
		p->move.end_point = end;
		w->plot.move_on = True;
	}
}



/* Sets the list line move on and sets the move control functions. If the functions
 * are set NULL then the defaults are used.
 */
void SciPlotListSetLineMove (Widget wi, int idnum, SciPlotCPF sel, SciPlotMLF move)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
	{
		p->move.type = XmMOVE_LINE;
		w->plot.move_on = True;
		if (sel) p->move.control_point_fcn = sel;
		if (move) p->move.line_move_fcn = move;
	}
}


void SciPlotListUnsetMove (Widget wi, int idnum)
{
	SciPlotList *p;
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	p = _ListFind(w, idnum);
	if (p)
	{
		int n;
		p->move.type = XmMOVE_NONE;
		p->move.start_point = 0;
		p->move.end_point = 0;
		p->move.control_point_fcn = NULL;
		p->move.line_move_fcn = NULL;
		w->plot.move_on = False;
		for (n = 0; n < w->plot.num_plotlist; n++)
		{
			if(w->plot.plotlist[n].move.type == XmMOVE_NONE) continue;
			w->plot.move_on = True;
			break;
		}
	}
}


void SciPlotSetXAutoScale (Widget wi)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	w->plot.XAutoScale = True;
	w->plot.y.Fixed.set = False;
}


void SciPlotSetXUserScale (Widget wi, float min, float max)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	if (min < max)
	{
		w->plot.XAutoScale = False;
		w->plot.UserMin.x = (float) min;
		w->plot.UserMax.x = (float) max;
	}
}


void SciPlotSetYAutoScale (Widget wi)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	w->plot.YAutoScale = True;
	w->plot.y.Fixed.set = False;
}


void SciPlotSetYUserScale (Widget wi, float min, float max)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	if (min < max)
	{
		w->plot.YAutoScale = False;
		w->plot.UserMin.y = (float) min;
		w->plot.UserMax.y = (float) max;
	}
}


void SciPlotSetXUserIncrement (Widget wi, float inc, int minor_num)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	if((w->plot.x.Fixed.set = (inc > 0)))
	{
		w->plot.x.Fixed.MajorInc = inc;
		if(minor_num > 0)
			w->plot.x.Fixed.MinorNum = minor_num+1;
		else
			w->plot.x.Fixed.MinorNum = 0;
	}
}


void SciPlotSetYUserIncrement (Widget wi, float inc, int minor_num)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	if((w->plot.y.Fixed.set = (inc > 0)))
	{
		w->plot.y.Fixed.MajorInc = inc;
		if(minor_num > 0)
			w->plot.y.Fixed.MinorNum = minor_num+1;
		else
			w->plot.y.Fixed.MinorNum = 0;
	}
}


/* Update the plot with all of the changes made with the public functions.
 * No assumptions are made and all of the axis parameters and legend layout
 * are calculated.
 */
void SciPlotUpdate (Widget wi)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return;

	EraseAll(w);
	ComputeAll(w);
	DrawAll(w);
}


/* The quick update assumes that the axis values and legend do not change
 * when the data is modified. If the data does go out of range the return
 * is False, else it is True.
 */
Boolean SciPlotQuickUpdate (Widget wi)
{
	SciPlotWidget w = (SciPlotWidget) wi;

	if (!XmIsSciPlot(wi)) return False;

	DrawAll(w);
	return CheckMinMax(w);
}

extern Widget XmCreateSciPlot(Widget parent, String name, ArgList args, int nargs)
{
	return XtCreateWidget(name,sciplotWidgetClass,parent,args,nargs);
}


extern Widget XmCreateManagedSciPlot(Widget parent, String name, ArgList args, int nargs)
{
	return XtCreateManagedWidget(name,sciplotWidgetClass,parent,args,nargs);
}


Widget XmVaCreateSciPlot( Widget parent, char* name, ... )
{
	Widget w;
	va_list ap;
	int count;

	va_start(ap, name);
	count = XmeCountVaListSimple(ap);
	va_end(ap);

	va_start(ap, name);
	w = XmeVLCreateWidget(name, sciplotWidgetClass, parent, False, ap, count);
	va_end(ap);
	return w;
}


Widget XmVaCreateManagedSciPlot( Widget parent, char* name, ... )
{
	Widget w;
	va_list ap;
	int count;

	va_start(ap, name);
	count = XmeCountVaListSimple(ap);
	va_end(ap);

	va_start(ap, name);
	w = XmeVLCreateWidget(name, sciplotWidgetClass, parent, True, ap, count);
	va_end(ap);
	return w;
}

