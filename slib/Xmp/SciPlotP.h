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
 *----------------------------------------------------------------------------
 *
 * May 2013 - Bob Paterson: Modified for changes required for Motif only
 *                          updates.
 */

#ifndef _SCIPLOTP_H
#define _SCIPLOTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <X11/CoreP.h>
#include <Xm/PrimitiveP.h>
#include <X11/Xcms.h>
#include "SciPlot.h"

#define powi(a,i)	(float)pow(a,(double)((int)i))

#define NUMPLOTLINEALLOC	5
#define NUMPLOTDATAEXTRA	25
#define NUMPLOTITEMALLOC	256
#define NUMPLOTITEMEXTRA	64
#define DEG2RAD				(3.1415926535897931160E0/180.0)

#define DRAW_MINOR(zz)	(w->plot.DrawMinor || (w->plot.zz.Fixed.set && w->plot.zz.Fixed.MinorNum > 0))

typedef unsigned int UINT;

typedef struct
{
	int dummy;			/* keep compiler happy with dummy field */
} SciPlotClassPart;


typedef struct _SciPlotClassRec
{
	CoreClassPart core_class;
	XmPrimitiveClassPart primitive_class;
	SciPlotClassPart plot_class;
} SciPlotClassRec;

extern SciPlotClassRec sciplotClassRec;

typedef enum
{
	SciPlotFalse,
	SciPlotLine,
	SciPlotRect,
	SciPlotFRect,
	SciPlotCircle,
	SciPlotFCircle,
	SciPlotStartTextTypes,
	SciPlotText,
	SciPlotVText,
	SciPlotEndTextTypes,
	SciPlotPoly,
	SciPlotFPoly,
	SciPlotClipRegion,
	SciPlotClipClear,
	SciPlotENDOFLIST
} SciPlotTypeEnum;

typedef enum
{
	SciPlotDrawingAny,
	SciPlotDrawingAxis,
	SciPlotDrawingLegend,
	SciPlotDrawingLine
} SciPlotDrawingEnum;


/* Structure to hold render table font tags and
 * basic general font information.
 */
typedef struct {
	String rtag;		/* requested XmStringTag */
	String tag;			/* assigned XmStringTag */
	int    ascent;		/* font ascent */
	int    descent;		/* font descent */
	int    height;		/* font max height */
	int    width;		/* font max width */
} SciPlotFontStruct, *SciPlotFont;


typedef struct _SciPlotItem
{
	SciPlotTypeEnum type;
	SciPlotDrawingEnum drawing_class;
	union
	{
		struct
		{
			Pixel color;
			short style;
			float x, y;
		} pt;
		struct
		{
			Pixel color;
			short style;
			float x1, y1, x2, y2;
		} line;
		struct
		{
			Pixel color;
			short style;
			float x, y, w, h;
		} rect;
		struct
		{
			Pixel color;
			short style;
			float x, y, r;
		} circ;
		struct
		{
			Pixel color;
			short style;
			short count;
			float x[4], y[4];
		} poly;
		struct
		{
			SciPlotFont font;
			Pixel color;
			short style;
			short length;
			float x, y;
			char *text;
		} text;
		struct
		{
			Pixel color;
			short style;
		} any;
	} kind;
	short individually_allocated;
	struct _SciPlotItem *next;
} SciPlotItem;

typedef struct
{
	int LineStyle;
	Pixel LineColor;
	int PointStyle;
	Pixel PointColor;
	int number;
	int allocated;
	SciPlotPoint *data;
	String legend;
	float markersize;
	SciPlotPoint min, max;
	Boolean draw, used;
	struct {
		int point;				/* what point, by array index, was selected by mouse */
		SciPlotPoint offset;	/* used during mouse movement */
		int     type;			/* enumerated type of the move */
		int     start_point;	/* start of range for point moves */
		int     end_point;		/* end of range for point moves */
		SciPlotCPF control_point_fcn;
		SciPlotMLF line_move_fcn;
	} move;
} SciPlotList;

typedef struct
{
	float Origin;
	float Size;
	float Center;
	float TitlePos;
	float AxisPos;
	float LabelPos;
	float LegendPos;
	float LegendSize;
	float DrawOrigin;
	float DrawSize;
	float DrawMax;
	float MajorInc;
	int   MajorNum;
	int   MinorNum;
	int   Precision;
	struct {
		Boolean set;
		float   MajorInc;
		int     MinorNum;
	} Fixed;

} SciPlotAxis;

typedef struct
{
	/* Public stuff ... */
	String  TransientPlotTitle;
	String  TransientXLabel;
	String  TransientYLabel;
	int     MarginWidth;
	int     MarginHeight;
	int     TitleMargin;
	int     LegendMargin;
	int     LegendLineSize;
	int     MajorTicSize;
	int     DefaultMarkerSize;
	int     ChartType;
	int     XAxisNumbers;
	int     YAxisNumbers;
	int     YAxisNumberNumDigits;
	int     XAxisMaxNumbers;
	int     YAxisMaxNumbers;
	Boolean ScaleToFit;
	Boolean Degrees;
	Boolean XLog;
	Boolean YLog;
	Boolean XAutoScale;
	Boolean YAutoScale;
	Boolean XOrigin;
	Boolean YOrigin;
	Boolean DrawMajor;
	Boolean DrawMinor;
	Boolean DrawMajorTics;
	Boolean DrawMinorTics;
	Boolean ShowLegend;
	Boolean ShowTitle;
	Boolean ShowXLabel;
	Boolean ShowYLabel;
	Boolean YNumHorz;
	Boolean LegendThroughPlot;
	Boolean LegendAtBottom;
	Pixel   ForegroundColor;
	Pixel   BackgroundColor;
	XmRenderTable RenderTable;

	XtCallbackList PlotCallback;

	/* Private stuff ... */
	String plotTitle;
	String xlabel;
	String ylabel;
	SciPlotPoint Min, Max;
	SciPlotPoint UserMin, UserMax;
	float PolarScale;
	SciPlotAxis x, y;
	SciPlotFontStruct titleFont;
	SciPlotFontStruct labelFont;
	SciPlotFontStruct axisFont;
	SciPlotFontStruct legendFont;

	Drawable draw;
	Pixmap pix;						/* SciPlot pixmap */
	UINT pix_width, pix_height;
	GC defaultGC;
	GC dashGC;
	GC textGC;
	Colormap cmap;

	int move_list;					/* which list was selected by the mouse */
	int alloc_plotlist;
	int num_plotlist;
	SciPlotList *plotlist;
	int alloc_drawlist;
	int num_drawlist;
	SciPlotItem *drawlist;
	SciPlotDrawingEnum current_id;  /* id of current item.  Used for erasing */
	Boolean update;
	Boolean move_on;
	Boolean check_set_render_table;
} SciPlotPart;

typedef struct _SciPlotRec
{
	CorePart core;
	XmPrimitivePart primitive;
	SciPlotPart plot;
} SciPlotRec;

#ifdef __cplusplus
};
#endif

#endif /* _SCIPLOTP_H */
