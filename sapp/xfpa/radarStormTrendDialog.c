/*=========================================================================*/
/*
 *    File: radarStormElem_dialog.c
 *
 * Purpose: Displays the historical values for storm elements on a graph.
 *
 *------------------------------------------------------------------------------
 *
 *     Version 8 (c) Copyright 2013 Environment Canada
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
 *   You should have received a copy of the GNU General Public Li197Gcense
 *   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
 */
/*=========================================================================*/
#include "global.h"
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include "radarSTAT.h"

/* The storm elements are scaled to an arbitrary range and given
 * an interval in the y-axis where no plotting is done. This looks
 * better appearance than having the data go right to the edges of
 * the plot. This sets this margin.
 */
#define PLOT_MARGIN 0.2

/* Defined in radarSTAT_dialog.c */
extern STATCFG statcfg;

typedef unsigned int UINT;

/* Private variables */
static Widget  dialog = NullWidget;
static Widget  *elem_plot = NULL;
static Widget  storm_label;
static Widget  rank_plot;
static Boolean edit_fcst_rank_weight = False;

/************ Private Functions **************/


/* Return the value for the given storm element. Both the element being
 * turned off or being missing return the SciPlot value that results in
 * the data value being skipped. Also if the value is valid but very
 * "small" return 0. No storm elements have values this small.
#define VALUE(x) (x.off?0:(x.na?0:((fabsf(x.value)<.00001)?0:x.value)))
 */
static float VALUE(DELEM x)
{
	if(x.off || x.na) return SCIPLOT_SKIP_VAL;
	if(fabsf(x.value) < .00001) return 0;
	return x.value;
}


/* Draw the element on the graph, but only if its value and the value
 * of the previous point is != 0 and it is not flagged as to be skipped.
 */
static void draw_storm_element(Widget plot, STORM *storm, int elem_ndx)
{
	int n, i, list;
	time_t dt;
	SciPlotPoint *p;

	ELEMCFG *elem = &statcfg.element[elem_ndx];

	/* Just make sure that there is more than enough memory for plotting. */
	p = NewMem(SciPlotPoint, storm->nhist+statcfg.num_forecasts+2);

	/* The latest data is in storm->data while the latest plot point
	 * is in p[storm->nhist-1], thus the array position reversal below.
	 */
	dt = storm->vtime;
	for(i = 0, n = storm->nhist-1; n >= 0; n--, i++)
	{
		Boolean no_prev_value, no_next_value;
		p[i].x = (float) ((storm->hist[n].vtime - dt)/60);
		p[i].y = VALUE(storm->hist[n].elem[elem_ndx]);
		no_prev_value = (i == 0 || p[i-1].y == SCIPLOT_SKIP_VAL || VALUE(storm->hist[n+1].elem[elem_ndx]) == 0);
		no_next_value = (n == 0 || VALUE(storm->hist[n-1].elem[elem_ndx]) == 0); 
		if(p[i].y == 0 && no_next_value && no_prev_value)
			p[i].y = SCIPLOT_SKIP_VAL;
	}

	list = SciPlotListCreate(plot, storm->nhist, p, elem->trend_title);
	SciPlotListSetStyle(plot, list, elem->trend_line_color, XmMARKER_FCIRCLE,
			elem->trend_line_color, elem->trend_line_style);

	/* Output the forecast values */
	if(p[storm->nhist-1].y != SCIPLOT_SKIP_VAL)
	{
		p[0].x = 0;
		p[0].y = VALUE(storm->data[elem_ndx]);
		for(n = 0; n < statcfg.num_forecasts; n++)
		{
				p[n+1].x = (float)((storm->fcst[n].vtime - storm->vtime)/60);
				p[n+1].y = VALUE(storm->fcst[n].elem[elem_ndx]);
		}
		list = SciPlotListCreate(plot, statcfg.num_forecasts+1, p, XmNO_LEGEND_ENTRY);
		SciPlotListSetStyle(plot, list, elem->trend_line_color, XmMARKER_DIAMOND,
				elem->trend_line_color, elem->fcst_trend_style);
	}

	FreeItem(p);
}



/* Draw the given rank weight trend for the storm.
 */
static void draw_rankweight(STORM *storm)
{
	int i, n, ndx, list, count = 0;
	time_t dt;
	SciPlotPoint *p;
	ELEMCFG *elem;
	float yrange = statcfg.rankweight.min_max;

	if((ndx = rs_get_element_array_pos_from_id(statcfg.rankweight.element_id)) < 0) return;
	elem = &statcfg.element[ndx];

	p = NewMem(SciPlotPoint, storm->nhist+statcfg.num_forecasts+2);

	p[0].x = -600;
	p[1].x = 60;
	for(n = 0; n < MAXTHRESH; n++)
	{
		/* Test for a valid threshold and if not found go to fallbacks */
		THRESH *thresholds = rs_active_thresholds(elem, storm, n);
		if(thresholds->value[n] == NO_THRESHOLD) continue;

		p[0].y = p[1].y = thresholds->value[n];
		list = SciPlotListCreate(rank_plot, 2, p, XmNO_LEGEND_ENTRY);
		SciPlotListSetStyle(rank_plot, list, statcfg.threshold_bg[n], XmMARKER_NONE,
				statcfg.threshold_bg[n], XmLINE_SOLID);
	}

	dt = storm->vtime;
	for(i = 0, n = storm->nhist-1; n >= 0; n--, i++)
	{
		p[i].x = (float) ((storm->hist[n].vtime - dt)/60);
		p[i].y = VALUE(storm->hist[n].elem[ndx]);
		if(p[i].y > yrange) yrange = p[n].y;
	}

	yrange = MAX(ceilf(yrange),statcfg.rankweight.min_max);
	SciPlotSetYUserScale(rank_plot, 0, ceilf(yrange));

	list = SciPlotListCreate(rank_plot, storm->nhist, p, XmNO_LEGEND_ENTRY);
	SciPlotListSetStyle(rank_plot, list, elem->trend_line_color, XmMARKER_FCIRCLE,
			elem->trend_line_color, elem->trend_line_style);

	/* Output the forecast values */
	p[0].x = 0;
	p[0].y = VALUE(storm->data[ndx]);
	for(count = 1, n = 0; n < statcfg.num_forecasts; n++)
	{
		p[count].x = (float)((storm->fcst[n].vtime - storm->vtime)/60);
		p[count].y = VALUE(storm->fcst[n].elem[ndx]);
		count++;
	}
	if(count > 1)
	{
		list = SciPlotListCreate(rank_plot, count, p, XmNO_LEGEND_ENTRY);
		if(edit_fcst_rank_weight)
		{
			SciPlotListSetStyle(rank_plot, list, statcfg.rankweight.edit_color, XmMARKER_DIAMOND,
				statcfg.rankweight.edit_color, elem->fcst_trend_style);
			SciPlotListSetLineMove(rank_plot, list, NULL, NULL);
		}
		else
		{
			SciPlotListSetStyle(rank_plot, list, elem->trend_line_color, XmMARKER_DIAMOND,
				elem->trend_line_color, elem->fcst_trend_style);
		}
	}

	FreeItem(p);
}


/* Respond to a change in the forecast rank weights in the SciPlot graph.
 * The number of forecast times is limited to 10 as it makes the coding
 * easier and I do not forsee any case where this limit would be exceeded.
 */
static void plot_modify_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int n, vtimes[10];
	float  values[10];
	SciPlotCallbackStruct *cbs = (SciPlotCallbackStruct *) call_data;
	
	for(n = 1; n < cbs->ndata && n <= 10; n++)
	{
		vtimes[n-1] = NINT(cbs->data[n].x);
		values[n-1] = cbs->data[n].y;
	}
	rs_modify_forecast_rankweights(cbs->ndata-1, vtimes, values);
}


static void exit_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	FreeItem(elem_plot);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}


/*********** Public Functions **************/

void ACTIVATE_stormTrendDialog(STORM *storm, Boolean editable)
{
	int n;
	Dimension width;

	edit_fcst_rank_weight = editable;

	if(!storm)
	{
		if(dialog) exit_cb(dialog, NULL, NULL);
		return;
	}

	/* Now that there is a storm create the dialog if necessary */
	if(!dialog)
	{
		int range, end;
		Widget frame, form, form2, rt;
		Pixel fg, bg;

		dialog = XuCreateToplevelFormDialog(GW_mainWindow, "stormTrendDialog",
			XuNmwmDeleteOverride, exit_cb,
			XuNmwmDeleteData, INT2PTR(True),
			XmNhorizontalSpacing, 9,
			XmNverticalSpacing, 9,
			NULL);

		frame = XmVaCreateManagedFrame(dialog, "plotFrame",
			XmNshadowType, XmSHADOW_IN,
			XmNmarginWidth, 9,
			XmNmarginHeight, 9,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtVaGetValues(frame, XmNforeground, &fg, XmNbackground, &bg, NULL);

		form = XmVaCreateForm(frame, "plotForm",
			XmNforeground, fg,
			XmNbackground, bg,
			XmNhorizontalSpacing, 0,
			XmNverticalSpacing, 0,
			NULL);

		storm_label = XmVaCreateManagedLabel(form, "stormLabel",
			XmNforeground, fg,
			XmNbackground, bg,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 50,
			NULL);

		rank_plot = XmVaCreateManagedSciPlot(form, "plot", 
			XmNxLabel, "Minutes",
			XmNshowYLabel, False,
			XmNshowTitle, False,
			XmNdrawMajorTics, False,
			XmNdrawMinor, False,
			XmNdrawMinorTics, False,
			XmNdefaultMarkerSize, 4,
			XmNyAxisNumberNumDigits, 4,
			XmNshowLegend, False,
			XmNheight, statcfg.rankweight.trend_graph_height,
			XmNmarginWidth, 3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, storm_label,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		XtAddCallback(rank_plot, XmNplotCallback, plot_modify_cb, NULL);

		SciPlotSetXUserIncrement(rank_plot, 10, 0);

		form2 = XmVaCreateForm(form, "elemPlotForm",
			XmNfractionBase, 1000,
			XmNhorizontalSpacing, 0,
			XmNverticalSpacing, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, rank_plot,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		elem_plot = NewMem(Widget, statcfg.rankweight.nelem);

		range = 1000/statcfg.rankweight.nelem;
		end = statcfg.rankweight.nelem - 1;

		for(n = 0; n < statcfg.rankweight.nelem; n++)
		{
			elem_plot[n] = XmVaCreateManagedSciPlot(form2, "plot", 
				XmNshowXLabel, False,
				XmNxAxisNumbers, XmNUMBERS_SPACING_ALONG_AXIS,
				XmNyAxisMaxNumbers, 1,
				XmNshowYLabel, False,
				XmNshowTitle, False,
				XmNdrawMajorTics, False,
				XmNdrawMinor, False,
				XmNdrawMinorTics, False,
				XmNdefaultMarkerSize, 4,
				XmNyAxisNumberNumDigits, 4,
				XmNlegendThroughPlot, True,
				XmNlegendAtBottom, True,
				XmNlegendLineSize, 0,
				XmNmarginWidth, 3,
				XmNtopAttachment, XmATTACH_POSITION,
				XmNtopPosition, range*n,
				XmNrightAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_POSITION,
				XmNbottomPosition, range*(n+1),
				NULL);
		}

		XtManageChild(form);
		XtManageChild(form2);
		XuShowDialog(dialog);
	}


	XuWidgetLabel(storm_label, "Rank Weight Storm N/A");
	if(storm)
	{
		int   minor_num;
		float yval, minx, maxx, increment;

		/* The maximum and minimum time values for the x-axis */
		maxx = (statcfg.num_forecasts > 0)?((storm->fcst[statcfg.num_forecasts-1].vtime - storm->vtime)/60):30;
		maxx = MAX(maxx, 0);
		minx = (storm->hist[storm->nhist-1].vtime - storm->vtime)/60;
		minx = MIN(minx, -statcfg.min_trend_time);

		/* How often are numbers printed in the x-axis and the number of minor grid lines */
		minor_num = (int)((maxx - minx)/statcfg.time_compress_val);
		increment = (float)(statcfg.time_interval * (minor_num + 1));

		SciPlotListDeleteAll(rank_plot);
		SciPlotSetXUserScale(rank_plot, minx, maxx);
		SciPlotSetXUserIncrement(rank_plot, increment, minor_num);

		/* In order for the graphs to line up vertically the y-axis scale must be the same
		 * for both graphs due to the layout depending on the size of the y-axis values.
		 * Thus the rank weight graph maximum value is used for the element display.
		 */
		draw_rankweight(storm);
		SciPlotUpdate(rank_plot);

		for(n = 0; n < statcfg.rankweight.nelem; n++)
		{
			SciPlotListDeleteAll(elem_plot[n]);
			SciPlotSetXUserScale(elem_plot[n], minx, maxx);
			SciPlotSetXUserIncrement(elem_plot[n], increment, minor_num);
			SciPlotSetYAutoScale(elem_plot[n]);
			draw_storm_element(elem_plot[n], storm, statcfg.rankweight.elem[n]->ndx);
			SciPlotUpdate(elem_plot[n]);
		}

		XuWidgetPrint(storm_label, "Rank Weight Storm %d", storm->num);
	}
	else
	{
		SciPlotListDeleteAll(rank_plot);
		SciPlotSetXUserScale(rank_plot, -statcfg.min_trend_time, 30);
		SciPlotSetXUserIncrement(rank_plot, (float) statcfg.time_interval, 0);
		SciPlotSetYUserScale(rank_plot, 0, statcfg.rankweight.min_max);
		SciPlotUpdate(rank_plot);
		for(n = 0; n < statcfg.rankweight.nelem; n++)
		{
			SciPlotListDeleteAll(elem_plot[n]);
			SciPlotSetXUserScale(elem_plot[n], -statcfg.min_trend_time, 30);
			SciPlotSetYUserScale(elem_plot[n], 0, 1);
			SciPlotUpdate(elem_plot[n]);
		}
	}

	XtVaGetValues(storm_label, XmNwidth, &width, NULL);
	XtVaSetValues(storm_label, XmNleftOffset, -(width/2), NULL);
}
