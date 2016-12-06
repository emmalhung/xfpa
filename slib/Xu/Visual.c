/*==========================================================================*/
/*
*	Visual.c - Function that determines the visual information.
*
*   If no visual information is specified in the resource file then this
*   function will attempt to find the "best" visual from those available.
*   Best is normally considered to be the greatest depth and/or a non-
*   default pseudocolor visual.
*
*   The following new resources are defined to set the visual for a
*   given screen. Note that these are only valid if found in the resource
*   database and will not be recognized if input as hard coded inputs.
*
*      XmNvisualID         - the visual id to use.
*      XmNvisualClass      - the required visual class.
*      XmNapplicationDepth - display depth, usually 8, 12, 16 or 24
*
*   Note that the above are in priority order and any input near the top
*   of the list will override all others. The resources are grouped into
*   those that are used together.
*
*   For multiple screens, the information for XmNvisualID, XmNvisualClass
*   and XmNapplicationDepth may be specified on a per screen basis by using
*   <screen number>:<value>. For example *visualID: 0:0x20  1:0x25
*
*   The following resources work in conjunction with the above only if
*   the visual is a PseudoColor type (normally 8 bits deep):
*
*      XmNusePrivateColormap          - True or False. Default False.
*      XmNisApplicationSuiteColormap  - check environment for an already
*                                       existing colormap. Default True.
*      XmNncopyColors                 - number of colours to copy from the
*                                       default colormap into our private
*                                       colormap.
***************************************************************************
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
*==========================================================================*/
/* $Header: /home/bob/fpa/slib/Xu/RCS/Visual.c,v 1.2 2007/10/01 11:01:49 bob Exp bob $ */
#include <stdlib.h>
#include <string.h>
#include "XuP.h"

#define DEFAULT 	-1

typedef struct {
	Display  *dpy;
	int       screen;
	Visual   *visual;
	Visual   *default_visual;
	VisualID  id;
	int       class;
	int       depth;
	Boolean   use_private_colormap;
	int       n_copy_colors;
	Boolean   is_suite_colormap;
} VisInfoStruct;


/* This function returns a "score" based on the following criteria (in hex):
 *
 *  x00 is the quality - 1 is 1bpp,
 *						 2 is 4bpp,
 *						 3 is 8bpp greycolor,
 *						 4 is 8bpp,
 *						 5 is 12bpp
 *						 6 is 15bpp
 *						 7 is 16bpp
 *						 8 is 32bpp
 *						 9 is 25bpp
 *
 *
 *	0x0  gets a point for being pseudocolor for depth <= 8
 *	     a point for being directcolor for depths > 8.
 *	     two points for being truecolor for depths > 8.
 *
 *	00x  gets a point for not being the default visual if the visual is
 *	     pseudocolour. In pseudocolour this gives us access to a visual
 *	     probably not used by many other apps. It gets a point for truecolor
 *	     if it is the default visual as we prefer this.
 */
static unsigned int score_visual (Visual *default_visual, XVisualInfo *vi)
{
	unsigned int quality = 0, class = 0, sys = 0;
  
	if (vi->class == TrueColor || vi->class == DirectColor)
	{
		switch(vi->depth)
		{
			case 24: quality = 9; break;
			case 32: quality = 8; break;
			case 16: quality = 7; break;
			case 15: quality = 6; break;
			case 12: quality = 5; break;
			case  8: quality = 4; break;
		}
	}
	else if (vi->class == PseudoColor || vi->class == StaticColor)
	{
		switch(vi->depth)
		{
			case 16: quality = 8; break;
			case 12: quality = 6; break;
			case  8: quality = 4; break;
			case  4: quality = 2; break;
			case  1: quality = 1; break;
		}
	}
	else if (vi->class == StaticGray || vi->class == GrayScale)
	{
		switch(vi->depth)
		{
			case 8: quality = 3; break;
			case 4: quality = 2; break;
			case 1: quality = 1; break;
		}
	}

	if (quality == 0) return 0;

	if(vi->depth < 9 && vi->class == PseudoColor) class = 1;
	if(vi->depth > 8 && vi->class == DirectColor) class = 1;
	if(vi->depth > 8 && vi->class == TrueColor  ) class = 2;

	if(vi->class == PseudoColor && vi->visual != default_visual) sys = 1;
	if(vi->class == TrueColor   && vi->visual == default_visual) sys = 1;

	return ((quality << 8) | (class << 4) | sys);
}


/* Choose the visual with the best score
 */
static void choose_visual(VisInfoStruct *vi, XVisualInfo *rtnVis)
{
	int          i, num_vinfo;
	unsigned int score, best_score;
	XVisualInfo  *best_visual;
	XVisualInfo  *vinfo;
	XVisualInfo  template;
	
	template.screen = vi->screen;
	vinfo = XGetVisualInfo(vi->dpy, VisualScreenMask, &template, &num_vinfo);
	
	best_visual = vinfo;
	best_score  = score_visual(vi->default_visual, vinfo);

	for (i = 1; i < num_vinfo; i++)
	{
		score = score_visual(vi->default_visual, &vinfo[i]);
		if (score > best_score)
		{
			best_score  = score;
			best_visual = &vinfo[i];
		}
	}
	(void) memcpy((void*)rtnVis, (void*)best_visual, sizeof(XVisualInfo));
	XFree((char*)vinfo);
}


static String parse_line(VisInfoStruct *vi, String resource, String *input)
{
	long    sn;
	String  data, s, p, buf;

	*input = XuGetStringResource(resource, NULL);
	if (!(*input)) return NULL;

	/* Look for either a single specifier or screen_num:info */
	buf = XtNewString(*input);
	data = s = strtok(buf," ");
	while(s)
	{
		sn = strtol(s, &p, 0);
		if (p == s) break;
		if (*p == ':' && sn == (long) vi->screen)
		{
			data = ++p;
			break;
		}
		s = strtok(NULL," ");
	}
	s = XtNewString(data);
	XtFree(buf);
	return s;
}


static void print_error(VisInfoStruct *vi, String resource, String dataline)
{
	pr_error("X Resource", "Value \"%s\" given for resource \"%s\" on screen \"%d\" is not available on this system.\n",
			dataline, resource, vi->screen);
}


/* Get depth from the resource database
 */
static void get_depth(VisInfoStruct *vi)
{
	int         vitems, depth;
	String      line, value;
	XVisualInfo *vinfos, vtemp;

	value = parse_line(vi, XuNapplicationDepth, &line);
	if (!value) return;

	depth = (int) strtol(value, NULL, 0);
	XtFree(value);

	/* Check to see that the depth is valid */
	vtemp.screen = vi->screen;
	vtemp.depth  = depth;
	vinfos = XGetVisualInfo(vi->dpy, VisualDepthMask|VisualScreenMask, &vtemp, &vitems);
	if (!vinfos)
	{
		print_error(vi, XuNapplicationDepth, line);
	}
	else
	{
		vi->depth = PTR2INT(value);
		XFree((char *)vinfos);
	}
}


/* Get the visual class from the resource database
 */
static void get_visual_class(VisInfoStruct *vi)
{
	int         vitems;
	String      vc, line;
	XVisualInfo vtemp, *vinfos = NULL;

	vc = parse_line(vi, XuNvisualClass, &line);
	if (!vc) return;

	     if (!strncasecmp(vc, "staticgray", 10)  ) vi->class = StaticGray;
	else if (!strncasecmp(vc, "staticcolor", 11) ) vi->class = StaticColor;
	else if (!strncasecmp(vc, "staticcolour", 12)) vi->class = StaticColor;
	else if (!strncasecmp(vc, "pseudocolor", 11) ) vi->class = PseudoColor;
	else if (!strncasecmp(vc, "pseudocolour", 12)) vi->class = PseudoColor;
	else if (!strncasecmp(vc, "grayscale", 9)    ) vi->class = GrayScale;
	else if (!strncasecmp(vc, "truecolor", 9)    ) vi->class = TrueColor;
	else if (!strncasecmp(vc, "truecolour", 10)  ) vi->class = TrueColor;
	else if (!strncasecmp(vc, "directcolor", 11) ) vi->class = DirectColor;
	else if (!strncasecmp(vc, "directcolour", 12)) vi->class = DirectColor;

	XtFree(vc);

	if(vi->class != DEFAULT)
	{
		/* Check to see that the id is valid */
		vtemp.screen = vi->screen;
		vtemp.class  = vi->class;
		vinfos = XGetVisualInfo(vi->dpy, VisualClassMask|VisualScreenMask, &vtemp, &vitems);
		if(vinfos)
			XFree((char *)vinfos);
		else
			vi->class = DEFAULT;
	}

	if(vi->class == DEFAULT)
		print_error(vi, XuNvisualClass, line);
}


/* Get the visual id from the resource database
 */
static Boolean get_visual_id(VisInfoStruct *vi, XVisualInfo *vinf)
{
	int         vitems;
	String      input, line;
	VisualID    value;
	XVisualInfo vtemp, *vinfos = NULL;

	input = parse_line(vi, XuNvisualID, &line);
	if (!input) return False;

	value = (VisualID) strtol(input, NULL, 0);
	XtFree(input);

	if (value)
	{
		/* Check to see that the id is valid */
		vtemp.screen   = vi->screen;
		vtemp.visualid = value;
		vinfos = XGetVisualInfo(vi->dpy, VisualIDMask|VisualScreenMask, &vtemp, &vitems);
	}
	if (!vinfos)
	{
		print_error(vi, XuNvisualID, line);
		return False;
	}
	else
	{
		(void) memcpy((void *)vinf, (void *)vinfos, sizeof(XVisualInfo));
		XFree((char *)vinfos);
		return True;
	}
}


/*
*	Copy colours from the default colormap into the given colormap.
*/
static void copy_colormap(VisInfoStruct *vi, Colormap cm1)
{
	int            i, nc;
	unsigned short del, inc, r, g, b;
	Colormap       default_cmap;
	XColor         colour;

	if(vi->n_copy_colors < 1) return;

	/* The following bit is tricky.  I wanted to copy the first block of
	*  colours from the default colour table as read only cells to avoid
	*  having to play with the resource converter in the Xt Intrinsics.
	*  Since XAllocColor will not allocate if the color already has been
	*  allocated and since the default colormap may have duplicate entries
	*  and since I do want a copy this is what you get.  Any duplicate
	*  entries are colour shifted slightly in my local map to get unique
	*  entries but hopefully they are not shifted enough so that it will
	*  be obvious to the user when they switch back to the default map.
	*  This is only a problem with duplicate entries and can be avoided
	*  by not having duplicate entries in the default colormap.
	*/
	default_cmap = DefaultColormap(vi->dpy,vi->screen);
	del = USHRT_MAX/256;
	inc = del;

	nc = (int)pow(2.0, (double)((vi->depth < 12)? vi->depth:12)) - 1;
	if(vi->n_copy_colors > nc) vi->n_copy_colors = nc;

	for( i = 0; i < vi->n_copy_colors; i++ )
	{
		colour.pixel = (Pixel) i;
		XQueryColor(vi->dpy, default_cmap, &colour);
		XAllocColor(vi->dpy, cm1, &colour);
		r = colour.red;
		g = colour.green;
		b = colour.blue;
		while(colour.pixel < (Pixel) i && inc < USHRT_MAX)
		{
			if(r == USHRT_MAX && g == USHRT_MAX && b == USHRT_MAX )
			{
				r -= inc;
				g -= inc;
				b -= inc;
				inc += del;
			}
			else
			{
				if(r < USHRT_MAX) r += del;
				if(g < USHRT_MAX) g += del;
				if(b < USHRT_MAX) b += del;
			}
			colour.red = r;
			colour.green = g;
			colour.blue = b;
			XAllocColor(vi->dpy, cm1, &colour);
		}
	}
}


/*
*   An application suite colormap is a colormap that is shared by
*   applications that are run in the same environment. That is one
*   is forked from the other. If requested, this function creates
*   such by putting the colormap information into the environment.
*   If the visual id is TrueColor this is not done.
*
*   The "is_application_suite_colormap" parameter determines the
*   operation of this function.
*
*   A colormap is created according to the workstation project
*   concept of colormap allocation.  In this scheme the first
*   ncopyColors colours are copied from the default colormap and
*   the rest are then assigned as required by the application. 
*
*   If is_application_suite_colormap = True and if no colormap is
*   found in the environment the default colormap is copied and
*   the resulting colormap is put into the environment. If a
*   colormap is found in the environment it is used directly.
*
*   If False, the environment is not looked at and a private
*   colormap is created directly.
*
*   In both of the above cases the first ncopyColors is copied
*   from the default visual.
*/
static Colormap create_colormap(VisInfoStruct *vi)
{
	unsigned long index;
	char          dpy[256];
	String        s, p, envstr, getenv();
	Colormap      new_cmap;
	Window        root_window;

	root_window  = RootWindow(vi->dpy,vi->screen);

	if (vi->visual->class == TrueColor)
	{
		return XCreateColormap(vi->dpy, root_window, vi->visual, AllocNone);
	}

	if (vi->is_suite_colormap)
	{
		char mbuf[256];
		(void) strcpy(mbuf, XDisplayString(vi->dpy));
		for(s = mbuf, p = mbuf; *s; s++) if(isalnum((int)*s)) *p++ = *s;
		*p = '\0';
		(void) snprintf(dpy, sizeof(dpy), "CMAP_%s_%lu", mbuf, (unsigned long) vi->visual->visualid);
		if((envstr = getenv(dpy)) != NULL && sscanf(envstr,"%lu",&index) == 1)
			return ((Colormap) index);
	}

	new_cmap = XCreateColormap(vi->dpy, root_window, vi->visual, AllocNone);

	/* If the new map is the same as the default map then we have an immutable colormap. */
	if (new_cmap != DefaultColormap(vi->dpy,vi->screen))
	{
		copy_colormap(vi, new_cmap);
		if(vi->is_suite_colormap)
		{
			char mbuf[256];
			(void) snprintf(mbuf, sizeof(mbuf), "%s=%lu", dpy, (unsigned long)new_cmap);
			(void) putenv(XtNewString(mbuf));
		}
	}
	return new_cmap;
}

/* determine the visual for the given display
 */
void _xu_set_visual( Display *dpy, int screen_num, int *depth, Visual **visual, Colormap *cmap)
{
	int           numVis, n;
	long          mask;
	XVisualInfo   visTemplate, *visReturn, visualInfo, bestVisual;
	VisInfoStruct vi;

	(void) memset((void*)&vi, 0, sizeof(VisInfoStruct));

	vi.dpy                  = dpy;
	vi.screen               = screen_num;
	vi.depth                = DEFAULT;
	vi.class                = DEFAULT;
	vi.use_private_colormap = XuGetBooleanResource(XuNusePrivateColormap, False);
	vi.n_copy_colors        = XuGetIntResource(XuNncopyColors, 0);
	vi.is_suite_colormap    = XuGetBooleanResource(XuNisApplicationSuiteColormap, False);
	vi.default_visual       = DefaultVisual(dpy,screen_num);

	/* Find our idea of the best visual */
	choose_visual(&vi, &bestVisual);

	/* See if depth or class has been specified */
	get_depth(&vi);
	get_visual_class(&vi);

	/* Highest priority is given to a specified visual id.
	*/
	if(get_visual_id(&vi, &visualInfo))
	{
		vi.visual = visualInfo.visual;
		vi.depth  = visualInfo.depth;
		vi.class  = visualInfo.class;
	}
	else if(vi.depth == DEFAULT && vi.class == DEFAULT)
	{
		vi.depth  = bestVisual.depth;
		vi.visual = bestVisual.visual;
		vi.class  = bestVisual.class;
	}
	else
	{
		if (vi.depth == DEFAULT)
		{
			vi.depth = bestVisual.depth;
		}

		/* Depth 8 is a special case and we prefer PseudoColor */
		if (vi.class == DEFAULT)
		{
			if(vi.depth == 8)
				vi.class = PseudoColor;
			else
				vi.class = bestVisual.class;
		}

		/* Try for match.
		 */
		if (XMatchVisualInfo(vi.dpy, vi.screen, vi.depth, vi.class, &visualInfo) != 0)
		{
			vi.visual = visualInfo.visual;
		}
		else
		{
			n = 0;
			/* Next see if we can find a depth that they have set.
			*/
			mask = VisualDepthMask|VisualScreenMask;
			visTemplate.depth  = vi.depth;
			visTemplate.screen = vi.screen;
			visReturn = XGetVisualInfo(vi.dpy, mask, &visTemplate, &numVis);

			if (visReturn)
			{
				/* If more than one visual let us pick our favourite class.
				*/
				int i;
				for( i = 1; i < numVis; i++ )
				{
					/* Depth 8 is a special case - prefer pseudo colour */
					if (visReturn[i].depth == 8) {
						if (visReturn[i].class == PseudoColor) n = i;
					} else {
						if (visReturn[i].class == TrueColor) n = i;
					}
				}
			}
			else
			{
				/* Nope.  Now find the best depth at this visual class
				*/
				mask = VisualClassMask|VisualScreenMask;
				visTemplate.class  = vi.class;
				visTemplate.screen = vi.screen;
				visReturn = XGetVisualInfo(vi.dpy, mask, &visTemplate, &numVis);

				if (visReturn)
				{
					int i;
					for ( i = 1; i < numVis; i++ )
						if (visReturn[i].depth > visReturn[n].depth) n = i;

					/* Depth 8 is a special case. We prefer Pseudo to True Color.
					*/
					if(	visReturn[n].depth == 8 && vi.class != PseudoColor)
					{
						if(XMatchVisualInfo(vi.dpy, vi.screen, 8, PseudoColor, &visualInfo) != 0)
						{
							visReturn[n].visual = visualInfo.visual;
							visReturn[n].class  = visualInfo.class;
						}
					}
				}
			}

			if (visReturn)
			{
				vi.visual = visReturn[n].visual;
				vi.depth  = visReturn[n].depth;
				vi.class  = visReturn[n].class;
				XtFree((char *)visReturn);
			}
			else
			{
				/* Found nothing so use defaults */
				vi.depth  = bestVisual.depth;
				vi.visual = bestVisual.visual;
				vi.class  = bestVisual.class;
			}
		}
	}

	if(!vi.use_private_colormap && vi.visual->visualid == vi.default_visual->visualid)
		*cmap = DefaultColormap(vi.dpy, vi.screen);
	else
		*cmap = create_colormap(&vi);

	*depth  = vi.depth;
	*visual = vi.visual;
}
