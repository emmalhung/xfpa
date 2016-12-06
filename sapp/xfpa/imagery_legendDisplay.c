/****************************************************************************
*
*  File:     imagery_legendDisplay.c
*
*  Function: ShowImageLegend()
*
*  Purpose:  Displays the current radar and satellite colour tables.
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
*
****************************************************************************/

#include "global.h"
#include <Xm/MwmUtil.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <FpaXgl.h>
#include "observer.h"
#include "imagery.h"

/* The amount of space around the table layout */
#define BORDER_SIZE	10

/*  The layout logic is the same for radar, satellite, overlay and underlay so
 *  those items that need to be specified are stored in this structure. Once
 *  created these dialogs always exist and are just hidden or not.
 */
typedef struct _dlgdata {
	String   type;			/* Image type identifier */
	String   name;			/* Dialog identifier - used in resource file */
	String   deflab;		/* Used if label missing from resource file */
	String   title;			/* Map title */
	SITEINFO *site;			/* If applicable the site the colour map is for */
	glLUT    *lutdata;		/* Colour lookup table to display */
	TSTAMP   dt;			/* Current data time */
	Boolean  have_data;		/* Is there data? */
	Boolean  showing;		/* Is the legend showing? */
	Widget   dialog;		/* Generated dialog */
	int      dty;			/* y length */
	XtIntervalId tid;		/* timeout return */
} DLGDATA;

static DLGDATA dialogs[] = {
	{RADAR_NAME,     "radarLegend",    "Radar",    0,0,0,"",False,False,0,0,0},
	{SATELLITE_NAME, "satelliteLegend","Satellite",0,0,0,"",False,False,0,0,0},
	{OVERLAY_NAME,   "overlayLegend",  "Overlay",  0,0,0,"",False,False,0,0,0},
	{UNDERLAY_NAME,  "underlayLegend", "Underlay", 0,0,0,"",False,False,0,0,0}
};

/* This is for the image colour tables. They are different from the product
 * tables in that they are always transient and are created and destroyed
 * when the images are selected/deselected.
 */
static int     ncmap_image = 0;
static DLGDATA **cmap_image = NULL;


/*======================== Private Function =============================*/



static void draw_time(DLGDATA *data)
{
	int           yr, jday, hr, min, height;
	long int      t;
	char          dbuf[50];
	Dimension     ww;
	XmRenderTable nrt;
	XmString      label;
	unsigned long mask;
	XGCValues     values;
	GC            gc;
	Display       *dpy = XtDisplay(data->dialog);
	Window        win  = XtWindow(data->dialog);

	XtVaGetValues(XtParent(data->dialog), XmNlabelRenderTable, &nrt, NULL);
	XtVaGetValues(data->dialog, XmNwidth, &ww, NULL);

	/* Create our date label. Check for the no time case */
	(void) parse_tstamp(data->dt, &yr, &jday, &hr, &min, NULL, NULL);
 	t = encode_clock(yr, jday, hr, min, 0);
	if(data->have_data)
	{
		(void) strftime(dbuf, sizeof(dbuf), "%a  %H:%M", gmtime((time_t*)&t));
	}
	else
	{
		(void) strftime(dbuf, sizeof(dbuf), "%H:%M - ", gmtime((time_t*)&t));
		(void) safe_strcat(dbuf, XuGetLabel("noData"));
	}
	label  = XmStringCreateSimple(dbuf);
	height = XmStringHeight(nrt,label);

	/* Clear the area where the data is written */
	mask = GCForeground;
	values.foreground = XuLoadColor(data->dialog, "black");
	gc = XCreateGC(dpy, win, mask, &values);
	XFillRectangle(dpy, win, gc, 0, data->dty, (unsigned int)ww, (unsigned int)height);

	/* Reset the colour */
	values.foreground = XuLoadColor(data->dialog, "white");
	XChangeGC(dpy, gc, mask, &values);

	/* Draw our date string */
	XmStringDraw(dpy, win, nrt, label, gc,
			(Position) BORDER_SIZE, (Position) data->dty, ww,
			XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
	XmStringFree(label);
	XFreeGC(dpy, gc);
}


static void draw_legend(DLGDATA *data)
{
	int           n, width, depth, x, y, yoff, xoff, inc;
	unsigned int  block_width, block_height;
	unsigned long mask;
	char          *p, buf[50];
	float         factor;
	Dimension     ww, wh, label_width, label_height;
	Boolean       sort;
	XGCValues     values;
	glLUTCOLOR    **colors;
	XColor        color;
	Colormap      cmap;
	GC            gc;
	XmString      label;
	XmRenderTable srt, nrt;
	Display       *dpy;
	Window        win;

	if(!data->dialog) return;
	if(!XtIsRealized(data->dialog)) return;

	dpy = XtDisplay(data->dialog);
	win = XtWindow(data->dialog);

	/* Extract required information from the drawing widget */
	XtVaGetValues(data->dialog,
		XmNdepth,    &depth,
		XmNcolormap, &cmap,
		XmNwidth,    &ww,
		XmNheight,   &wh,
		NULL);

	/* Get the render tables from the unmanaged label and the form parent */
	XtVaGetValues(XtNameToWidget(data->dialog,"font"), XmNrenderTable, &srt, NULL);
	XtVaGetValues(XtParent(data->dialog), XmNlabelRenderTable, &nrt, NULL);

	/* Copy the colour array and sort it in reverse order (biggest at index 0) */
	colors = NewMem(glLUTCOLOR *, data->lutdata->ncells);
	for( n = 0; n < data->lutdata->ncells; n++ )
		colors[n] = data->lutdata->cells + n;

	do
	{
		sort = False;
		for( n = 1; n < data->lutdata->ncells; n++ )
		{
			if( colors[n]->upper_bound > colors[n-1]->upper_bound )
			{
				glLUTCOLOR *lc = colors[n-1];
				colors[n-1] = colors[n];
				colors[n] = lc;
				sort = True;
			}
		}
	} while(sort);

	/* Set the size of our blocks of colour */
	(void) snprintf(buf, sizeof(buf), "*%s.scaleLength", data->name);
	block_height = XuGetIntResource(buf, 225) / data->lutdata->ncells;
	if(block_height < 1) block_height = 1;

	(void) snprintf(buf, sizeof(buf), "*%s.maxBlockHeight", data->name);
	n = XuGetIntResource(buf, 15);
	if( block_height > n ) block_height = n;

	(void) snprintf(buf, sizeof(buf), "*%s.blockWidth", data->name);
	block_width = XuGetIntResource(buf, 20);

	/* Set background of our drawing area */
	mask = GCForeground;
	values.foreground = XuLoadColor(data->dialog, "black");
	gc = XCreateGC(dpy, win, mask, &values);
	XFillRectangle(dpy, win, gc, 0, 0, (unsigned int)ww, (unsigned int)wh);

	/* Colour for general drawing */
	values.foreground = XuLoadColor(data->dialog, "white");
	XChangeGC(dpy, gc, mask, &values);

	/* Find the maximum size of the labels so we can do layout */
	label_width = 0;
	if(data->lutdata->item)
	{
		label = XmStringCreateSimple(data->lutdata->item);
		if(XmStringWidth(srt, label) > label_width)
			label_width = XmStringWidth(srt, label);
		XmStringFree(label);
	}

	/* Calculate the number of label jumps we need. This is the total height of the colour
	 * bar divided by <factor> times the space a label needs. Since all our colour bar
	 * labels are numbers we can find the height information here.
	 */
	label = XmStringCreateSimple("0");
	label_height = XmStringHeight(srt,label);
	XmStringFree(label);
	(void) snprintf(buf, sizeof(buf), "*%s.minLabelSpace", data->name);
	p = XuGetStringResource(buf,"2.0");
	if( sscanf(p, "%f", &factor) != 1 || factor < 0.1) factor = 2.0;
	inc = (int)((float)data->lutdata->ncells/ ((float)(block_height*data->lutdata->ncells)/(factor*label_height))+0.5);
	if(inc < 1) inc = 1;

	/* Now find the maximum size using the numeric labels */
	for( n = 0; n < data->lutdata->ncells; n+=inc )
	{
		if(colors[n]->lower_bound > 1.0)
			(void) snprintf(buf, sizeof(buf), "%.0f", colors[n]->lower_bound);
		else
			(void) snprintf(buf, sizeof(buf), "%.1f", colors[n]->lower_bound);
		label = XmStringCreateSimple(buf);
		if(XmStringWidth(srt, label) > label_width)
			label_width = XmStringWidth(srt, label);
		XmStringFree(label);
	}

	/* set starting location */
	x = BORDER_SIZE;
	y = BORDER_SIZE;

	/* Title */
	if(data->title)
	{
		label = XmStringCreateLocalized(data->title);
		ww = XmStringWidth(nrt, label);
		XmStringDraw(dpy, win, nrt, label, gc, (Position) x, (Position) y,
				ww, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
		y += XmStringHeight(nrt,label) + ((same(data->type, RADAR_NAME))?BORDER_SIZE:2*BORDER_SIZE);
		XmStringFree(label);
	}

	/* Label of image data field */
	if(data->lutdata->label)
	{
		label = XmStringCreateSimple(data->lutdata->label);
		width = XmStringWidth(srt, label);
		if( width > ww ) ww = (Dimension)width;
		XmStringDraw(dpy, win, srt, label, gc, (Position) x, (Position) y,
				(Dimension)width, XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);
		y += XmStringHeight(srt,label) + BORDER_SIZE/2;
		XmStringFree(label);
	}

	/* The colour bar and associated labels are to be centered under the title
	 * if the title is wider than the bar-label set.
	 */
	xoff = BORDER_SIZE;
	width = 2*label_width + 3*block_width/2;
	if( width  < ww)
		xoff += (ww - width)/2;
	else
		ww = (Dimension)width;

	/* Draw the name of the item the colour table is for */
	if(data->lutdata->item)
	{
		label = XmStringCreateSimple(data->lutdata->item);
		XmStringDraw(dpy, win, srt, label, gc, (Position) xoff, (Position) y,
				label_width, XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);
		y += (int)((3.0*(float)XmStringHeight(srt,label))/2.0 + 0.5);
		XmStringFree(label);
	}

	/* Put the numeric values on the scale */
	yoff = y;
	x    = xoff;
	y    = yoff + block_height - label_height;
	for( n = 0; n < data->lutdata->ncells; n+=inc )
	{
		if(colors[n]->lower_bound > 1.0)
			(void) snprintf(buf, sizeof(buf), "%.0f", colors[n]->lower_bound);
		else
			(void) snprintf(buf, sizeof(buf), "%.1f", colors[n]->lower_bound);
		label = XmStringCreateSimple(buf);
		XmStringDraw(dpy, win, srt, label, gc, (Position) x, (Position) y,
				label_width, XmALIGNMENT_END, XmSTRING_DIRECTION_L_TO_R, NULL);
		XmStringDraw(dpy, win, srt, label, gc,
				(Position)(x+label_width+3*block_width/2), (Position) y,
				label_width, XmALIGNMENT_BEGINNING, XmSTRING_DIRECTION_L_TO_R, NULL);
		XmStringFree(label);
		y += block_height*inc;
	}

	x += label_width;

	if(same(data->type, RADAR_NAME))
	{
		/* Put horizontal bars across the bottom of every colour block */
		y = yoff + block_height;
		for( n = 0; n < data->lutdata->ncells; n++ )
		{
			XFillRectangle(dpy, win, gc, x, y, 3*block_width/2, 1);
			y += block_height;
		}
	}
	else
	{
		/* Box the colour bar with a white rectangle as satellite is usually grayscale */
		y = yoff;
		values.line_width = 1;
		XChangeGC(dpy, gc, GCLineWidth, &values);
		XDrawRectangle(dpy, win, gc, x+block_width/4-1, y-1, block_width+1, data->lutdata->ncells*block_height+1);
	}

	/* Create the blocks of colour */
	x += block_width/4;
	y = yoff;
	for( n = 0; n < data->lutdata->ncells; n++ )
	{
		color.red   = (unsigned short)(colors[n]->red   << 8);
		color.green = (unsigned short)(colors[n]->green << 8);
		color.blue  = (unsigned short)(colors[n]->blue  << 8);
		color.flags = DoRed|DoGreen|DoBlue;

		(void) XuAllocColor(dpy, depth, cmap, &color, True);

		values.foreground = color.pixel;
		XChangeGC(dpy, gc, mask, &values);
		
		XFillRectangle(dpy, win, gc, x, y, block_width, block_height);
		y += block_height;
	}

	/* Write out the valid time */
	label = XmStringCreateSimple(" ");
	y += XmStringHeight(nrt,label);
	data->dty = y;
	draw_time(data);
	y += XmStringHeight(nrt,label) + BORDER_SIZE;
	XmStringFree(label);

	/* Clean up */
	FreeItem(colors);
	XFreeGC(dpy, gc);

	/* Set the dialog size to accomodate our colour map */
	XtVaSetValues(XuGetShell(data->dialog), XmNwidth, ww+2*BORDER_SIZE, XmNheight, y+BORDER_SIZE, NULL);

	if(!data->showing)
	{
		XuShowDialog(data->dialog);
		data->showing = True;
	}
}


/* The following two functions are for the expose and redraw callbacks. There can be
 * many of these generated if the window has another window move over it and we do
 * not want to redraw for every event. These two functions ensure that there is at
 * least 100 milliseconds between events before a redraw is done.
 */
static void redraw(XtPointer client_data, XtIntervalId *id)
{
	DLGDATA *data = (DLGDATA*) client_data;
	/*
	 * The id parameter will only be NULL if it was called from the function
	 * below and not from the exiration of the timeout. If called below we
	 * remove it from the list and add it back in, otherwise we do the redraw.
	 */
	if(!id)
	{
		if(data->tid) XtRemoveTimeOut(data->tid);
		data->tid = XtAppAddTimeOut(GV_app_context, 100, redraw, client_data);
	}
	else
	{
		data->tid = 0;
		draw_legend(data);
	}
}


/* ARGSUSED*/
static void expose_resize_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	redraw(client_data, NULL);
}


/* ARGSUSED*/
static void exit_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	((DLGDATA*)client_data)->dialog = NULL;
}


static void make_dialog(DLGDATA *data, int id_num, Boolean reprocess)
{
	/* Create the colour table display if it does not exist */
	if (!data->dialog)
	{
		char     buffer[100];
		XmString label;

		(void) snprintf(buffer, 100, "*.%s.labelString", data->name);
		label = XuGetXmStringResource(buffer, data->deflab);

		if(id_num < 0)
		{
			data->dialog = XuCreateToplevelDialog(GW_mainWindow, xmDrawingAreaWidgetClass, data->name,
				XuNdestroyCallback, exit_cb,
				XuNdestroyData, data,
				XmNdialogTitle, label,
				XmNmwmDecorations, MWM_DECOR_MENU,
				XmNnoResize, True,
				XmNminWidth, 50,
				XmNminHeight, 100,
				NULL);
		}
		else
		{
			(void) snprintf(buffer, 100, "%s%sN%d", data->name, XuDIALOG_ID_PART_SEPARATOR, id_num+1);
			data->dialog = XuCreateToplevelDialog(GW_mainWindow, xmDrawingAreaWidgetClass, data->name,
				XuNdestroyCallback, exit_cb,
				XuNdestroyData, data,
				XmNdialogTitle, label,
				XuNdialogID, buffer,
				XmNmwmDecorations, MWM_DECOR_MENU,
				XmNnoResize, True,
				XmNminWidth, 50,
				XmNminHeight, 100,
				NULL);
		}

		XtAddCallback(data->dialog, XmNexposeCallback, expose_resize_cb, (XtPointer)data);
		XtAddCallback(data->dialog, XmNresizeCallback, expose_resize_cb, (XtPointer)data);

		/* Used to obtain font information and never managed */
		(void) XmCreateLabel(data->dialog, "font", NULL, 0);
		XmStringFree(label);
		XuShowDialog(data->dialog);
		data->showing = True;
	}
	else if (reprocess)
	{
		draw_legend(data);
	}
}


/* The remove is done and checked in multiple stages to ensure that the dialog is
 * actually gone before we remove the associated data structure.
 */
static void remove_site_dialogs(IMDAT *imdat)
{
	int i, n;

	for(i = 0; i < ncmap_image; i++)
	{
		if(!cmap_image[i]) continue;
		if(!cmap_image[i]->dialog)
		{
			FreeItem(cmap_image[i]);
		}
		else if(imdat == NULL)
		{
			XuDestroyDialog(cmap_image[i]->dialog);
		}
		else
		{
			for(n = 0; n < imdat->nsite; n++)
			{
				SITEINFO *site = imdat->site + n;
				if(cmap_image[i]->site == site && !site->selected)
					XuDestroyDialog(cmap_image[i]->dialog);
			}
		}
	}
	XuDelay(GW_mainWindow, 50);
}


/* Create a data structure and dialog specific to a site colour map.
 */
static void make_site_dialog(IMDAT *imdat, DLGDATA *base_data, Boolean reprocess)
{
	int      i, n;
	DLGDATA  *data;
	SITEINFO *site;
	ImageLUT lut;

	/* First remove any non-selected site dialogs */
	remove_site_dialogs(imdat);

	/* Add in any selected but not existing dialogs */
	for(n = 0; n < imdat->nsite; n++)
	{
		site = imdat->site + n;
		if(!site->selected) continue;

		lut = glImageInfoGetDefaultLut(site->tag);
		if(!lut || !glImageIsDataLUT(lut)) continue;

		/* Test for existing valid dialog */
		for(i = 0; i < ncmap_image; i++)
			if(cmap_image[i] && cmap_image[i]->dialog && cmap_image[i]->site == site) break;
		if(i < ncmap_image) continue;

		data = OneMem(DLGDATA);
		data->type    = base_data->type;
		data->name    = base_data->name;
		data->deflab  = base_data->deflab;
		data->title   = site->label;
		data->site    = site;
		data->lutdata = glImageGetLUTInfo(lut);

		for(i = 0; i < ncmap_image; i++)
			if(!cmap_image[i]) break;
		if(i >= ncmap_image)
		{
			cmap_image = MoreMem(cmap_image, DLGDATA*, ncmap_image+10);
			(void) memset((void*) &cmap_image[ncmap_image], 0, 10*sizeof(DLGDATA*));
			ncmap_image += 10;
		}
		cmap_image[i] = data;
		make_dialog(data, i, reprocess);
	}
}


/*==================== Public Function ======================*/


/* Show the legend associated with the given image type as defined in the data
 * structure. If the parameter reprocess is True, then the dialog is forced
 * to regenerate itself.
 */
void ShowImageLegend(IMDAT *imdat, const Boolean reprocess)
{
	int      n, nlut;
	ImageLUT *luts;
	PRODINFO *prod;
	DLGDATA  *data = NULL;

	/* If imdat is NULL then hide and destroy all dialogs */
	if(!imdat)
	{
		for( n = 0; n < XtNumber(dialogs); n++ )
		{
			XuHideDialog(dialogs[n].dialog);
			dialogs[n].showing = False;
		}
		remove_site_dialogs(NULL);
		return;
	}

	/* Are we processing radar, satellite or data? */
	for( n = 0; n < XtNumber(dialogs); n++ )
		if( same(imdat->name, dialogs[n].type) ) data = dialogs + n;
	if (!data) return;

	/* If nothing selected disappear */
	if(imdat->selected < 1)
	{
		XuHideDialog(data->dialog);
		data->showing = False;
		remove_site_dialogs(imdat);
		return;
	}

	prod = &imdat->prod[imdat->prod_select];
	nlut = glImageInfoGetLuts(prod->tag, &luts, NULL, NULL);

	/* At this point we may have no product colour table but have tables
	 * for the images themselves.
	 */
	if(nlut < 1 || IsNull(luts) || prod->lutndx >= nlut)
	{
		XuHideDialog(data->dialog);
		data->showing = False;
		make_site_dialog(imdat, data, reprocess);
	}
	else if(glImageIsDataLUT(luts[prod->lutndx]))
	{
		/* Get our product colour lookup table information */
		data->title   = prod->label;
		data->lutdata = glImageGetLUTInfo(luts[prod->lutndx]);
		data->dty     = 0;
		make_dialog(data, -1, reprocess);
	}
	XuDelay(GW_mainWindow, 50);
}

/* The time parameter dt can be NULL or blank to indicate no time match, IMAGE_NA a
 * macro defined in the imagery.h header file and meaning no image is available or
 * dt can be the time of the displayed image.
 */
void SetImageLegendTime(IMDAT *imdat, String dt, Boolean have_data)
{
	int  n;

	for( n = 0; n < XtNumber(dialogs); n++ )
	{
		if(!same(imdat->name, dialogs[n].type)) continue;

		if(IsNull(dialogs[n].dialog)) break;
		if(!XtIsRealized(dialogs[n].dialog)) break;
		if(!XtIsManaged(dialogs[n].dialog)) break;

		dialogs[n].have_data = have_data;
		(void) safe_strcpy(dialogs[n].dt, dt);

		/* Just draw the date if possible */
		if(dialogs[n].dty > 0)
			draw_time(&dialogs[n]);
		else
			ShowImageLegend(imdat, True);

		break;
	}

	for( n = 0; n < ncmap_image; n++ )
	{
		if(!cmap_image[n]) continue;
		if(!same(imdat->name, cmap_image[n]->type)) continue;
		if(IsNull(cmap_image[n]->dialog)) continue;

		cmap_image[n]->have_data = have_data;
		(void) safe_strcpy(cmap_image[n]->dt, dt);
		draw_time(cmap_image[n]);
	}
	XuDelay(GW_mainWindow, 50);
}
