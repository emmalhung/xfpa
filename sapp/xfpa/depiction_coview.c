/*========================================================================*/
/*
*	File:		depiction_coview.c
*
*	Purpose:	Contains functions responsible for creating the
*               depiction viewing buttons and running the coview viewer.
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

#include <signal.h>
#include "global.h"
#include <Xm/PushB.h>

static String pgm;
static String key_string = NULL;

/*ARGSUSED*/
static void ForeignCB(Widget w, XtPointer client_data, XtPointer unused)
{
	int     ac, n;
	String  sfile, item, my_display, ddir, al[50];
	SETUP   *setup;
	Boolean have_display = False;

	int ndx = PTR2INT(client_data);

	/* Insert the manditory stuff into the run string.
	*/
	setup = GetSetup(key_string);
	sfile = SetupParm(setup, ndx, 1);
	ddir  = source_directory_by_name(DEPICT, NULL, NULL);

	ac = 0;
	al[ac] = pgm;           ac++;
	al[ac] = "+viewerMode"; ac++;
	al[ac] = "+iconic";     ac++;
	al[ac] = "-stateDir";   ac++;
	al[ac] = ddir;          ac++;
	al[ac] = "-setup";      ac++;
	al[ac] = sfile;         ac++;
	al[ac] = "-t0";         ac++;
	al[ac] = GV_T0_depict;  ac++;

	/* pick up any remaining X commands from the setup file and
	*  append them to the list.
	*/
	n = 2;
	while( (item = SetupParm(setup, ndx, n)) != NULL  && ac < 47 )
	{
		al[ac] = item; ac++;
		n++;
		if(same(item,"-display")) have_display = NotNull(SetupParm(setup, ndx, n));
	}

	/* If we don't have a display override put our own display into the run string.
	*/
    if(!have_display && NotNull(my_display = getenv("DISPLAY")))
    {
		al[ac] = "-display"; ac++;
		al[ac] = my_display; ac++;
    }
	al[ac] = NULL; ac++;

	(void) signal(SIGCHLD, SIG_IGN);
	switch((int)fork())
	{
		case 0: /* child */
			close(ConnectionNumber(XtDisplay(GW_mainWindow)));
			(void) signal(SIGINT, SIG_IGN);
			(void) signal(SIGQUIT, SIG_IGN);
			(void) signal(SIGHUP, SIG_IGN);
			(void) signal(SIGCHLD, SIG_IGN);
			(void) execvp(pgm, al);
			perror(pgm);
			exit(255);

		case -1:
			perror(pgm);
			printf("Fork failed for viewer\n");
	}
}


void CreateCoviewPulldown(int menu_id , String argv0 )
{
	int i;
	Widget pane, btn;
	SETUP *setup;

	/* The DEPICT_FOREIGN type is obsolete but is kept for
	*  backwards compatability.
	*/
	key_string = DEPICT_COVIEW;
	setup = GetSetup(DEPICT_COVIEW);
	if( setup->nentry < 1 )
	{
		key_string = DEPICT_FOREIGN;
		setup = GetSetup(DEPICT_FOREIGN);
	}

	if(!EntryExists(menu_id, key_string, False)) return;

	pgm = argv0;
	pane = XuMenuFind(GW_menuBar, menu_id);
	for( i = 0; i < setup->nentry; i++ )
	{
		btn = XmCreatePushButton(pane, SetupParm(setup,i,0), NULL, 0);
		XtAddCallback(btn, XmNactivateCallback, ForeignCB, INT2PTR(i));
		XtManageChild(btn);
	}
}
