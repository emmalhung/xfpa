/***********************************************************************/
/*
*	File: glib_misc_fcns.c
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
/***********************************************************************/

#include <sys/stat.h>
#include <tools/tools.h>
#include "glib_private.h"


void glGetMapPixelSize( float *dx, float *dy )
{
	*dx = 1.0/W->Sx;
	*dy = 1.0/W->Sy;
}

void glVdcUnitPerMapUnit( float *dx, float *dy )
{
	*dx = W->Sx / (float)W->xm;
	*dy = W->Sy / (float)W->ym;
}

void glMapUnitPerVdcUnit( float *dx, float *dy )
{
	*dx = (float)W->xm / W->Sx;
	*dy = (float)W->ym / W->Sy;
}


void glSetWindow(int wid)
{
	static char *MyName = "glSetWindow";
	if(wid < 1 || wid >= Xgl.last_ndx)
	{
		pr_error(MyName, "invalid window id: %d\n", wid);
	}
	else
	{
		W = &Xgl.windows[Xgl.active_ndx = wid];
	}
}



static int stack_len = 0;
static int *window_stack = NULL;
static int spos = 0;

void glPushWindow(void)
{
	static char *MyName = "glPushWindow";
	int i;
	if(spos >= stack_len)
	{
		window_stack = GETMEM(window_stack, int, stack_len+10);
		if(!window_stack)
		{
			pr_error(MyName, "Memory allocation error\n");
			return;
		}
		stack_len += 10;
		for(i = spos; i < stack_len; i++) window_stack[i] = 0;
	}
	for(i = spos; i > 0; i--) window_stack[i] = window_stack[i-1];
	window_stack[0] = Xgl.active_ndx;
	spos++;
}

void glPopWindow(void)
{
	int i;
	if(window_stack[0] == 0) return;
	glSetWindow(window_stack[0]);
	for(i = 0; i < spos; i++) window_stack[i] = window_stack[i+1];
	spos--;
}

int glGetWindowId(void)
{
	return Xgl.active_ndx;
}


static void set_scales(XglWindow *w)
{
	w->Sx    = (w->vr - w->vl) / (w->or - w->ol);
	w->Sy    = (w->vt - w->vb) / (w->ot - w->ob);
	w->Tx    = w->vl - (w->ol * W->Sx);
	w->Ty    = w->vb - (w->ob * W->Sy);
	w->RxCos = w->Sx;
	w->RxSin = 0.0;
	w->RyCos = w->Sy;
	w->RySin = 0.0;
	w->ra    = 0.0;
	w->send  = 0;

	/* Pre-add in the factor required for the conversion to integer
	*  when the offset is truncated. We do it here as we do not want
	*  this factor in the matrix.
	*/
	w->Tx += 0.5;
	w->Ty += 0.5;
}

void glOrtho(Coord left, Coord right, Coord bottom, Coord top)
{
	static char *MyName = "glOrtho";
	if(left == right || bottom == top)
	{
		pr_error(MyName, "x-range or y-range is empty.\n");
	}
	else
	{
		W->ol = left;
		W->or = right;
		W->ob = bottom;
		W->ot = top;
		set_scales(W);
	}
}


void glGetOrtho(Coord *left, Coord *right, Coord *bottom, Coord *top)
{
	*left   = W->ol;
	*right  = W->or;
	*bottom = W->ob;
	*top    = W->ot;
}

/* Set viewport using the current world coordinates.
*/
void glMapViewport(float left, float right, float bottom, float top)
{
	Screencoord l, r, b, t;
	l = (Screencoord)XS(left,bottom);
	r = (Screencoord)XS(right,top);
	b = (Screencoord)((int)W->ym - YS(left,bottom) - 1);
	t = (Screencoord)((int)W->ym - YS(right,top) - 1);
	glViewport(l, r, b, t);
}

/* Set viewport using vdc.
*/
void glVdcViewport(float left, float right, float bottom, float top)
{
	Screencoord l, r, b, t;
	l = (int)(W->xm - 1) *left   + 0.5;
	r = (int)(W->xm - 1) *right  + 0.5;
	b = (int)(W->ym - 1) *bottom + 0.5;
	t = (int)(W->ym - 1) *top    + 0.5;
	glViewport(l, r, b, t);
}

/* Set viewport using screen coordinates.
*/
void glViewport(Screencoord left, Screencoord right, Screencoord bottom, Screencoord top)
{
	W->vl = MIN(left,right);
	W->vr = MAX(left,right);
	W->vb = MIN(top,bottom);
	W->vt = MAX(top,bottom);

	set_scales(W);

	if (W->x && Xgl.set_viewport) Xgl.set_viewport(W, top);
}

void glGetViewport(Screencoord *left, Screencoord *right, Screencoord *bottom, Screencoord *top)
{
	if (W)
	{
		*left   = (Screencoord) W->vl;
		*right	= (Screencoord) W->vr;
		*top    = (Screencoord) W->vt;
		*bottom = (Screencoord) W->vb;
	}
	else
	{
		*left   = 0;
		*right  = 1280;
		*top    = 0;
		*bottom = 1024;
	}
}

void glSetMapClipRectangle( Coord left, Coord right, Coord bottom, Coord top )
{
	Screencoord l, r, b, t;

	if (!W) return;

	l = (Screencoord)XS(left,bottom);
	r = (Screencoord)XS(right,top);
	b = (Screencoord)((int)W->ym - YS(left,bottom) - 1);
	t = (Screencoord)((int)W->ym - YS(right,top) - 1);
	glSetClipRectangle(l,r,b,t);
}


void glSetVdcClipRectangle( Coord left, Coord right, Coord bottom, Coord top )
{
	Screencoord l, r, b, t;

	if (!W) return;

	l = (W->xm - 1) * left   + 0.5;
	r = (W->xm - 1) * right  + 0.5;
	b = (W->ym - 1) * bottom + 0.5;
	t = (W->ym - 1) * top    + 0.5;
	glSetClipRectangle( l, r, b, t );
}


void glSetClipRectangle( Screencoord left, Screencoord right, Screencoord bottom, Screencoord top )
{
	if (!W) return;
	/* The clip rectangle is constrained to be within the viewport.
	*/
	W->cl = MAX(MIN((int)left,(int)right),W->vl);
	W->cr = MIN(MAX((int)left,(int)right),W->vr);
	W->cb = MAX(MIN((int)top,(int)bottom),W->vb);
	W->ct = MIN(MAX((int)top,(int)bottom),W->vt);
}


void glGetClipRectangle( Screencoord *left, Screencoord *right, Screencoord *bottom, Screencoord *top )
{
	/* If W has not been assigned yet just return default values */
	if (W)
	{
		*left   = (Screencoord) W->cl;
		*right  = (Screencoord) W->cr;
		*top    = (Screencoord) W->ct;
		*bottom = (Screencoord) W->cb;
	}
	else
	{
		*left   = 0;
		*right  = 1280;
		*top    = 0;
		*bottom = 1024;
	}
}

void glHatchAngle(float angle)
{
	W->ha = angle;
}

void glHatchCrossAngle(float angle)
{
	W->hc = angle;
}


/* Hatch spacing in pixels.
*/
void glHatchSpacing(int space)
{
	static char *MyName = "glHatchSpacing";
	if(space < 1)
	{
		pr_error(MyName, "Attempt to set hatch spacing to < 1. Ignored.\n");
	}
	else
	{
		W->hs = (float)space;
	}
}

void glMapHatchSpacing(float space)
{
	static char *MyName = "glMapHatchSpacing";
	if(space <= 0.0)
	{
		pr_error(MyName, "Attempt to set hatch spacing to <= 0. Ignored.\n");
	}
	else
	{
		W->hs = space * W->Sy;
	}
}

void glVdcHatchSpacing(float space)
{
	static char *MyName = "glVdcHatchSpacing";
	if(space <= 0.0)
	{
		pr_error(MyName, "Attempt to set hatch spacing to <= 0. Ignored.\n");
	}
	else if(space > 1.0)
	{
		pr_error(MyName, "Attempt to set hatch spacing to > 1. Ignored.\n");
	}
	else
	{
		W->hs = (W->ym - 1) * space;
	}
}


/* This combination of functions will set the working directory for the library
 * in the directory supplied to the glSetWorkingDirectory() function. If the
 * directory does not exist it will attempt to create it. As the create is
 * recursive we keep a record of the "highest" level at which we create a
 * directory and destroy from there when removing the directory. Note that if
 * the calling program aborts, the directory will be left around.
 */
static LOGICAL make_work_directory( STRING dir )
{
	LOGICAL ok = TRUE;
	struct stat sb;

	if(blank(dir)) return FALSE;

	if( stat(dir,&sb) != 0 )
	{
		mode_t mode =  (mode_t) (S_IRWXU|S_IRWXG|S_IRWXO);
		if( mkdir(dir, mode) != 0)
		{
			STRING ptr, tmpdir;
			ok = FALSE;
			tmpdir = safe_strdup(dir);
			if((ptr = strrchr(tmpdir,'/')))
			{
				*ptr = '\0';
				if(make_work_directory(tmpdir))
				{
					ok = (mkdir(dir, mode) == 0);
				}
			}
			free(tmpdir);
		}
		else
		{
			FREEMEM(Xgl.work_directory_root);
			Xgl.work_directory_root = safe_strdup(dir);
		}
	}
	return ok;
}


void glSetWorkingDirectory( char *dir )
{
	char *d, *buf;
	static char *MyName = "glSetWorkingDirectory";

	if (!blank(Xgl.work_directory_root))
		(void) remove_directory(Xgl.work_directory_root, NULL);
	FREEMEM(Xgl.work_directory);
	FREEMEM(Xgl.work_directory_root);

	buf = safe_strdup(blank(dir)? "/tmp":dir);
	no_white(buf);
	d = tempnam(buf,"fpagl");
	if(!make_work_directory(d))
	{
		FREEMEM(d);
		pr_error(MyName, "Unable to use supplied directory: \"%s\" - using /tmp\n", dir);
		d = tempnam("/tmp","fpagl");
		if(!make_work_directory(d))
		{
			FREEMEM(d);
			d = safe_strdup("/tmp");
		}
	}
	Xgl.work_directory = d;
	FREEMEM(buf);
}
