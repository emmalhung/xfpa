/*========================================================================*/
/*
*	File:		depiction_print.c
*
*	Purpose:	Contains functions responsible for creating the depiction
*               printing buttons and for actioning the print event. Note
*               that only XWD output is supported.
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
#include <Xm/PushB.h>
#include "fpapm.h"

static char module[] = "printFunction";

/*ARGSUSED*/
static void PrintCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int ac, index;
	String  arg, p, parm, printer, outfile, encoding, dfile;
	Arg al[7];
	char dbuf[16], xbuf[16], ybuf[16];
	SETUP *setup;
	float val;
	int ival;
	double strtod();

	String	mode = "portrait";
	float	xlen = 7.9;
	float	ylen = 10.0;
	float	xoff = 0;
	float	yoff = 0;
	int		dpi  = 150;
	int		nx, ny;

	ac       = 1;
	printer  = NULL;
	outfile  = NULL;
	index    = PTR2INT(client_data);
	setup    = GetSetup(DEPICT_PRINT);
	encoding = GraphicFileTypeByExtent(NULL);

	/* Get override information from the setup file */
	if( setup )
	{
		parm = SetupParm(setup, index, ac); ac++;
		if (same(parm,"printer"))
		{
			printer = SetupParm(setup, index, ac); ac++;
			parm = SetupParm(setup, index, ac); ac++;
		}
		else if (same(parm,"file"))
		{
			outfile = SetupParm(setup, index, ac); ac++;
			parm = SetupParm(setup, index, ac); ac++;
			encoding = GraphicFileTypeByExtent(outfile);
		}
		if (!blank(parm)) mode = parm;
		if (same(mode,"portrait"))
		{
			xlen = 7.9;
			ylen = 10.0;
		}
		else if (same(mode,"landscape"))
		{
			xlen = 10.0;
			ylen = 7.9;
		}
		else
		{
			Warning(module, "PrintOrientation", mode);
			return;
		}
		do
		{
			arg = SetupParm(setup, index, ac); ac++;
			if (!arg) break;
			val = (float) strtod(arg,&p);
			if (p != arg)
			{
				xoff = val;
				xlen -= val;
			}

			arg = SetupParm(setup, index, ac); ac++;
			if (!arg) break;
			val = (float) strtod(arg,&p);
			if (p != arg)
			{
				yoff = val;
				ylen -= val;
			}

			arg = SetupParm(setup, index, ac); ac++;
			if (!arg) break;
			val = (float) strtod(arg,&p);
			if (p != arg) xlen = val;

			arg = SetupParm(setup, index, ac); ac++;
			if (!arg) break;
			val = (float) strtod(arg,&p);
			if (p != arg) ylen = val;

			arg = SetupParm(setup, index, ac); ac++;
			if (!arg) break;
			ival = (int) strtol(arg,&p,10);
			if (p != arg) dpi = ival;
		} while (0);	/* do the above once (allows use of break) */
	}
	if (xlen<=0 || ylen<=0)
	{
		char mbuf[50];
		(void) snprintf(mbuf, sizeof(mbuf), "%g X %g", xlen, ylen);
		Warning(module, "PrintSize", mbuf);
		return;
	}

	/* Generate the dump file */
	dfile = tempnam(GV_working_directory,NULL);
	nx = (int) ceil(xlen*dpi);
	ny = (int) ceil(ylen*dpi);
	(void) IngredVaCommand(GE_ACTION, "RASTER_DUMP %s BW ACTIVE %d %d %s NOPAD", encoding, nx,ny,dfile);

	/* Output the dump file to the printer */
	ac = 0;
	XtSetArg(al[ac], PmNprogram, PmSCREEN_PRINT); ac++;
	XtSetArg(al[ac], PmNinfoFile, dfile); ac++;
	XtSetArg(al[ac], PmNpageMode, mode); ac++;
	if (printer)
	{
		XtSetArg(al[ac], PmNprinter, printer); ac++;
	}
	if (outfile)
	{
		XtSetArg(al[ac], PmNfileName, outfile); ac++;
	}
	if (xoff > 0)
	{
		(void) snprintf(xbuf, sizeof(xbuf), "%g", xoff);
		XtSetArg(al[ac], PmNxOffset, xbuf); ac++;
	}
	if (yoff > 0)
	{
		(void) snprintf(ybuf, sizeof(ybuf), "%g", yoff);
		XtSetArg(al[ac], PmNyOffset, ybuf); ac++;
	}
	(void) snprintf(dbuf, sizeof(dbuf), "%d", dpi);
	XtSetArg(al[ac], PmNdpi, dbuf); ac++;
	(void)RunProgramManager(NULL, NULL, al, ac);

	FreeItem(dfile);
}

void CreatePrintPulldown(int menu_id )
{
	int i;
	Widget pane, btn;
	SETUP *setup;

	if(!EntryExists(menu_id, DEPICT_PRINT, False)) return;

	setup = GetSetup(DEPICT_PRINT);
	if( setup->nentry < 1 ) return;

	pane = XuMenuFind(GW_menuBar, menu_id);
	for( i = 0; i < setup->nentry; i++ )
	{
		btn = XmCreatePushButton(pane, SetupParm(setup,i,0), NULL, 0);
		XtAddCallback(btn, XmNactivateCallback, PrintCB, INT2PTR(i));
		XtManageChild(btn);
	}
}
