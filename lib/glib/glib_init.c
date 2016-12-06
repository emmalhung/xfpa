/*************************************************************************/
/*
*   File:     glib_init.c
*
*   Purpose:  Contains functions required to initialize, reinitialize and
*             close the library and create windows.
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
/*************************************************************************/

#define GLIBINIT
#include <limits.h>
#include <tools/tools.h>
#include "glib_private.h"

/* Graphic library intialization
 */
void glVirtualInit(void)
{
	static int endian_test[1] = {1};

	if(Xgl.windows) return;

	/* Do the machine endian test. If the upper byte contains the 1
	 * then we are operating on a little endian machine.
	 */
	if( ((char*) endian_test)[0] == 1 )
		MACHINE_ENDIAN = IMAGE_LITTLE_ENDIAN;
	else
		MACHINE_ENDIAN = IMAGE_BIG_ENDIAN;

	/* Set the working maximum value of a short integer as a float.
	 * It is set to less than the actual max to account for roundoff.
	 */
	Xgl.short_limit = (float) SHRT_MAX * 0.95;

	/* The first window(0) is never used but we load default info into it.
	*/
	Xgl.windows    = INITMEM(XglWindow, INITIAL_WINDOW_COUNT);
	Xgl.active     = Xgl.windows;
	Xgl.active_ndx = 0;
	Xgl.last_ndx   = INITIAL_WINDOW_COUNT;

	/* If we don't have a working directory set to the default.
	 */
	if(!Xgl.work_directory)
		glSetWorkingDirectory(DEFAULT_WORKING_DIRECTORY);
}


int glCreateVirtualWindow(int width, int height)
{
	glVirtualInit();
	
	Xgl.active_ndx = 1;
	while(Xgl.active_ndx < Xgl.last_ndx && Xgl.windows[Xgl.active_ndx].inuse)
		Xgl.active_ndx++;

	if(Xgl.active_ndx >= Xgl.last_ndx)
	{
		Xgl.last_ndx++;
		Xgl.windows = GETMEM(Xgl.windows, XglWindow,  Xgl.last_ndx);
	}
	W = &Xgl.windows[Xgl.active_ndx];
	(void) memset((void *)W, 0, sizeof(XglWindow));
	
	W->inuse    = TRUE;
	W->xm       = (UNINT) width;
	W->ym       = (UNINT) height;
	W->xp	    = 0;
	W->yp       = 0;
	W->viewport = FALSE;
	W->clipping = FALSE;
	W->hs       = 10;
	W->ha       = 45.0;
	W->hc       = 90.0;
	W->slen     = 0;
	W->send     = 0;
	W->stack    = (MATRIXSTACK*)0;

	/* set default scale and viewport */
	glOrtho (0.0, (Coord)(W->xm - 1), 0.0, (Coord)(W->ym - 1));
	glViewport((Screencoord)0, (Screencoord)(W->xm - 1), (Screencoord)0, (Screencoord)(W->ym - 1));
	
	return Xgl.active_ndx;
}


void glCloseWindow(int wid)
{
	XglWindow *w = &Xgl.windows[wid];

	if(!w->inuse) return;   /* This window does not exist anymore */

	/* If the active window is this one reset it to somewhere else or
	*  to null if this is the last window.
	*/
	if(Xgl.active == w)
	{
		int i;
		Xgl.active = (XglWindow *)NULL;
		Xgl.active_ndx = 0;
		for(i = 1; i < Xgl.last_ndx; i++)
		{
			if(&Xgl.windows[i] == w) continue;
			Xgl.active     = &Xgl.windows[i];
			Xgl.active_ndx = i;
			break;
		}
	}

	if (Xgl.close_xstuff) Xgl.close_xstuff(w);

	FREEMEM(w->stack);
	(void) memset((void *)w, 0, sizeof(XglWindow));
}

/* Make sure that we release all server resources and remove
 * the working directory.
 */
void glExit(void)
{
	int n;

	if (Xgl.x_exit) Xgl.x_exit();

	glImageDestroyAll();

	for(n = 1; n < Xgl.last_ndx; n++)
	{
		if (Xgl.exit_xstuff) Xgl.exit_xstuff(n);
		glCloseWindow(n);
	}

	(void) remove_directory(Xgl.work_directory_root, NULL);

	FREEMEM(Xgl.work_directory);
	FREEMEM(Xgl.work_directory_root);
	FREEMEM(Xgl.windows);
}
