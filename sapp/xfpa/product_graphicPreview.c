/*========================================================================*/
/*
*  File:    product_graphicPreview.c.c
*
*  Purpose: Provides a preview function which displays the direct output
*           products as specified in the graphicDialog.
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
/*========================================================================*/
#include "global.h"
#include "graphic.h"
#include <FpaXgl.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>

static Widget     dialog = NULL;
static Widget     prev, next;
static int        wid    = 0;
static String     fname  = NULL;
static int        ndx    = 0;
static int        nimages = 0;
static String     *image_list = NULL;
GraphicProdStruct *prod = NULL;


static void DrawDepiction(int width, int height)
{
	char mode[30], olay[256];
	String ptr, fmt;
	Boolean pad, ok;
	INFOFILE fh;

	fh = info_file_open(get_file(prod->program, prod->pdf));

	ptr = info_file_get_data(fh, "MODE");
	strcpy(mode, ptr);

	ptr = info_file_get_data(fh, "PAD");
	pad = same_start_ic(ptr, "T");

	ptr = info_file_get_data(fh, "OVERLAY");
	ptr = get_file(prod->program, ptr);
	strcpy(olay, blank(ptr) ? "":ptr);

	fmt = GraphicFileTypeByExtent(info_file_get_data(fh,"OUTPUT"));

	ptr = tempnam(GV_working_directory,NULL);
	fname = malloc(safe_strlen(ptr)+10);
	strcpy(fname, ptr);
	strcat(fname, ".");
	strcat(fname, fmt);
	FreeItem(ptr);

	ok = IngredVaCommand(GE_ACTION,
			"RASTER_DUMP %s %s %s %d %d %s %s %s",
			fmt, mode, image_list[ndx], width, height, fname, pad ? "PAD" : "NOPAD", olay);

	info_file_close(fh);
	XSync(XtDisplay(dialog), 0);

	if(ok) glFileToWindow(fname, True);
}


/*ARGSUSED*/
static void CreateFpaMetImage(int width, int height)
{
}


/* This function switches between drawing functions depending on the mode
*  the viewer is working in.
*/
static void DrawImages(void)
{
	int width, height;

	if(!wid) return;

	glPushWindow();
	glSetWindow(wid);
	(void) glSetColor("black");
	glClear();
	glSwapBuffers();
	glPopWindow();

	XuSetBusyCursor(ON);
	glGetWindowSize(&width, &height);

	/* Decide which type of graphic we are drawing.
	*/
	switch(prod->type)
	{
		case GRAPHICS: DrawDepiction(width, height);     break;
		case FPAMET:   CreateFpaMetImage(width, height); break;
	}
	glSwapBuffers();
	XuSetBusyCursor(OFF);
}


/*ARGSUSED*/
static void MoveCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	switch(PTR2BOOL(client_data))
	{
		case 'p': ndx--; break;
		case 'n': ndx++; break;
	}
	XtSetSensitive(prev, ndx > 0);
	XtSetSensitive(next, ndx <  nimages-1);
	glPushWindow();
	glSetWindow(wid);
	glResetViewport();
	glFileToWindow(fname, True);
	DrawImages();
	glSwapBuffers();
	glPopWindow();
}


/* The following two functions are a bit of a work around for a Linux problem
*  (Redhat 7.1). When the resize happens there seem to be many resize callbacks
*  made. These two functions in combination ensure that only the last callback
*  in the event stack will activate the actual drawing procedure.
*/
/*ARGSUSED*/
static Boolean ResizeWP( XtPointer client_data )
{
	glPushWindow();
	glSetWindow(wid);
	glResetViewport();
	DrawImages();
	glSwapBuffers();
	glPopWindow();
	*((Boolean *)client_data) = True;
	return True;
}


/*ARGSUSED*/
static void ResizeCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	static Boolean do_resize = True;

	if (!wid) return;
	if (!do_resize) return;
	do_resize = False;
	(void) XtAppAddWorkProc(GV_app_context, (XtWorkProc)ResizeWP, &do_resize);
}


/*ARGSUSED*/
static void ExposeCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	if(!wid) return;

	glPushWindow();
	glSetWindow(wid);
	glSwapBuffers();
	glPopWindow();
}


/*ARGSUSED*/
static void ExitCB(Widget w , XtPointer cld , XtPointer cd )
{
	if(IsNull(dialog)) return;

	XuDestroyDialog(dialog);
	(void) unlink(fname);
	XtFree(fname);
	glCloseWindow(wid);
	dialog = NULL;
	wid = 0;
	ndx = 0;
	FreeList(image_list, nimages);
	nimages = 0;
}


void GraphicPreviewPopdown(void)
{
	ExitCB(NULL, NULL, NULL);
}


static void GetDepictionImageSequence(void)
{
	String   p, dt, toff;
	Source   src;
	INFOFILE fh;

	/* Find the requested source. If none found default to
	*  the depiction sequence.
	*/
	fh  = info_file_open(get_file(prod->program, prod->pdf));
	p   = string_arg(info_file_get_data(fh, "SOURCE"));
	src = FindSourceByName(blank(p) ? DEPICT:p, NULL);
	if(IsNull(src)) src = FindSourceByName(DEPICT, NULL);

	/* Create the time offset list and verify that there are
	*  actually depictions at the given times.
	*/
	toff = info_file_get_data(fh, "TIME");
	if(blank(toff)) toff = info_file_get_data(fh, "OFFSET");
	while(NotNull(dt = string_arg(toff)))
	{
		if(!GetDepictionTimeFromOffset(src, dt, EXACT, &p)) continue;
		image_list = MoreMem(image_list, String, nimages+1);
		image_list[nimages] = XtNewString(p);
		nimages++;
	}
	info_file_close(fh);
}


static void GetFpaMetImageSequence(void)
{
}


void GraphicPreviewShow(GraphicProdStruct *in_prod )
{
	static Widget drawing;

	static XuDialogActionsStruct action_items[] = {
		{ "prevBtn",  MoveCB,            (XtPointer)'p' },
		{ "nextBtn",  MoveCB,            (XtPointer)'n' },
		{ "closeBtn", XuDestroyDialogCB, NULL           }
	};

	FreeList(image_list, nimages);

	nimages = 0;
	prod    = in_prod;
	ndx     = 0;

	switch(prod->type)
	{
		case GRAPHICS:  GetDepictionImageSequence(); break;
		case FPAMET:    GetFpaMetImageSequence();    break;
	}
	if(nimages == 0) return;

	if(dialog)
	{
		XtSetSensitive(prev, ndx > 0);
		XtSetSensitive(next, ndx <  nimages-1);
		XuShowDialog(dialog);
	}
	else
	{
		XmString title = XmStringCreateLocalized(prod->label);

		dialog = XuCreateToplevelFormDialog(GW_mainWindow, "graphicPreview",
			XmNdialogTitle, title,
			XuNmwmDeleteOverride, ExitCB,
			XuNactionAreaItems, action_items,
			XuNnumActionAreaItems, XtNumber(action_items),
			NULL);

		XmStringFree(title);

		prev = XuGetActionAreaBtn(dialog, action_items[0]);
		next = XuGetActionAreaBtn(dialog, action_items[1]);

		drawing = XmVaCreateManagedDrawingArea(dialog, "da",
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		XtAddCallback(drawing, XmNresizeCallback, ResizeCB, NULL);
		XtAddCallback(drawing, XmNexposeCallback, ExposeCB, NULL);

		XtSetSensitive(prev, ndx > 0);
		XtSetSensitive(next, ndx <  nimages-1);

		XuShowDialog(dialog);

		glInit();
		glPushWindow();
		wid = glInitWidgetWindow(drawing);
		glDoubleBuffer();
		DrawImages();
		glSwapBuffers();
		glPopWindow();
	}
}
