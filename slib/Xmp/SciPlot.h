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
 */

#ifndef _SCIPLOT_H
#define _SCIPLOT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <X11/Core.h>
#include <math.h>
#include <float.h> /* NOTE:  float.h is required by POSIX */

#define _SCIPLOT_WIDGET_VERSION	2.0

#ifndef XmIsSciPlot
#define XmIsSciPlot(w) XtIsSubclass((Widget)w, sciplotWidgetClass)
#endif
#define SCIPLOT_SKIP_VAL (-FLT_MAX)

typedef struct
{
	float x, y;
} SciPlotPoint;


typedef struct {
	int    reason;
	XEvent *event;
	int    list_id;
	int    point_index;
	int    ndata;
	SciPlotPoint *data;
} SciPlotCallbackStruct;


typedef int  (*SciPlotCPF) (int pndx, SciPlotPoint *points, int npoints);
typedef void (*SciPlotMLF) (SciPlotPoint p, SciPlotPoint *points, int npoints);


#define XmNchartType			"chartType"
#define XmNdegrees				"degrees"
#define XmNdefaultMarkerSize	"defaultMarkerSize"
#define XmNdrawMajor			"drawMajor"
#define XmNdrawMajorTics		"drawMajorTics"
#define XmNdrawMinor			"drawMinor"
#define XmNdrawMinorTics		"drawMinorTics"
#define XmNxAutoScale			"xAutoScale"
#define XmNyAutoScale			"yAutoScale"
#define XmNxAxisNumbers			"xAxisNumbers"
#define XmNyAxisNumbers			"yAxisNumbers"
#define XmCAxisNumbers			"AxisNumbers"
#define XmRAxisNumbers			"AxisNumbers"
#define XmNyAxisNumberNumDigits	"yAxisNumberNumDigits"
#define XmNxAxisMaxNumbers		"xAxisMaxNumbers"
#define XmNyAxisMaxNumbers		"yAxisMaxNumbers"
#define XmNxLog					"xLog"
#define XmNyLog					"yLog"
#define XmNxOrigin				"xOrigin"
#define XmNyOrigin				"yOrigin"
#define XmNxLabel				"xLabel"
#define XmNyLabel				"yLabel"
#define XmNplotTitle			"plotTitle"
#define XmNtitleMargin			"titleMargin"
#define XmNshowLegend			"showLegend"
#define XmNshowTitle			"showTitle"
#define XmNshowXLabel			"showXLabel"
#define XmNshowYLabel			"showYLabel"
#define XmNlegendLineSize		"legendLineSize"
#define XmNlegendMargin			"legendMargin"
#define XmNlegendThroughPlot	"legendThroughPlot"
#define XmNlegendAtBottom		"legendAtBottom"
#define XmNtitleFont			"titleFont"
#define XmNlabelFont			"labelFont"
#define XmNaxisFont				"axisFont"
#define XmNtitleFontTag			"titleFontTag"
#define XmNlabelFontTag			"labelFontTag"
#define XmNaxisFontTag			"axisFontTag"
#define XmNlegendFontTag		"legendFontTag"
#define XmNyNumbersHorizontal	"yNumbersHorizontal"
#define XmNplotCallback			"plotCallback"

#define XmPOLAR		0
#define XmCARTESIAN	1

#define XmMARKER_NONE		0
#define XmMARKER_CIRCLE		1
#define XmMARKER_SQUARE		2
#define XmMARKER_UTRIANGLE	3
#define XmMARKER_DTRIANGLE	4
#define XmMARKER_LTRIANGLE	5
#define XmMARKER_RTRIANGLE	6
#define XmMARKER_DIAMOND	7
#define XmMARKER_HOURGLASS	8
#define XmMARKER_BOWTIE		9
#define XmMARKER_FCIRCLE	10
#define XmMARKER_FSQUARE	11
#define XmMARKER_FUTRIANGLE	12
#define XmMARKER_FDTRIANGLE	13
#define XmMARKER_FLTRIANGLE	14
#define XmMARKER_FRTRIANGLE	15
#define XmMARKER_FDIAMOND	16
#define XmMARKER_FHOURGLASS	17
#define XmMARKER_FBOWTIE	18
#define XmMARKER_DOT		19

#define XmLINE_NONE		0
#define XmLINE_SOLID	1
#define XmLINE_DOTTED	2
#define XmLINE_WIDEDOT	3
#define XmLINE_USERDASH	4	/* Not implemented - here as a to do reminder */

#define XmNO_LEGEND_ENTRY	NULL

/* For display of values along the axis */
enum { XmNUMBERS_NONE, XmNUMBERS_SHOW, XmNUMBERS_SPACING_ALONG_AXIS };

/* For move action */
enum { XmMOVE_NONE, XmMOVE_XY, XmMOVE_X_ONLY, XmMOVE_Y_ONLY, XmMOVE_LINE };

/* SciPlotCallbackStruct reason values */
enum { XmCR_MOVE_POINT = 1, XmCR_MOVE_LINE };


extern WidgetClass sciplotWidgetClass;

typedef struct _SciPlotClassRec *SciPlotWidgetClass;
typedef struct _SciPlotRec      *SciPlotWidget;


/*
** Public function declarations
*/
void    SciPlotSetBackgroundColor (Widget wi, Pixel color);
void    SciPlotSetForegroundColor (Widget wi, Pixel color);
void    SciPlotListDelete (Widget wi, int idnum);
void    SciPlotListDeleteAll (Widget wi);
int     SciPlotListCreate (Widget wi, int num, SciPlotPoint *list, char *legend);
void    SciPlotListUpdate (Widget wi, int idnum, int num, SciPlotPoint *list);
void    SciPlotListAdd (Widget wi, int idnum, int num, SciPlotPoint *list);
void    SciPlotListSetStyle (Widget wi, int idnum, Pixel pcolor, int pstyle, Pixel lcolor, int lstyle);
void    SciPlotListSetMarkerSize (Widget wi, int idnum, float size);
void    SciPlotListSetLineMove (Widget wi, int idnum, SciPlotCPF sel, SciPlotMLF move);
void    SciPlotListSetPointMove (Widget wi, int idnum, int type, int start, int end);
void    SciPlotListUnsetMove (Widget wi, int idnum);
void    SciPlotSetXUserIncrement(Widget wi, float inc, int minor_num);
void    SciPlotSetXAutoScale (Widget wi);
void    SciPlotSetXUserScale (Widget wi, float min, float max);
void    SciPlotSetYUserIncrement(Widget wi, float inc, int minor_num);
void    SciPlotSetYAutoScale (Widget wi);
void    SciPlotSetYUserScale (Widget wi, float min, float max);
void    SciPlotUpdate (Widget wi);
Boolean SciPlotQuickUpdate (Widget wi);
Widget  XmCreateSciPlot(Widget parent, String name, ArgList args, int nargs);
Widget  XmCreateManagedSciPlot(Widget parent, String name, ArgList args, int nargs);
Widget  XmVaCreateSciPlot( Widget parent, char* name, ... );
Widget  XmVaCreateManagedSciPlot( Widget parent, char* name, ... );

#ifdef __cplusplus
};
#endif

#endif /* _SCIPLOT_H */
