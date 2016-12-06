/***************************************************************************/
/*
 *  File: LoadColor.c
 *
 *  Purpose: Functions to return a pixel value given a named colour. The
 *           macros XtNforeground and XtNbackground are taken as a special
 *           case and the appropriate values are taken from the widget.
 *
 *  Note: Resources are read from the resource file associated with the
 *        display opened by the main program via VaAppInitialize.
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
/***************************************************************************/
#include <ctype.h>
#include "XuP.h"


/* Structure to allow us to keep track of our colour allocations.
 */
typedef struct _alloc_struct {
	Display *dpy;
	Colormap cmap;
	int npx;
	Pixel *px;
	int nsub;
	XColor *sub;
	struct _alloc_struct *next;
} ALLOC_STRUCT;

ALLOC_STRUCT *alloc_data = (ALLOC_STRUCT *)NULL;


/*================ PUBLIC FUNCTIONS ====================*/


Pixel XuLoadColor(Widget _w , String _colorName )
{
	int      depth;
	Colormap cmap;
	XColor   colour;
	Display *dpy = XtDisplayOfObject(_w);

	static String module = "XuLoadColor";

	if(same(_colorName,XtNbackground))
	{
		XtVaGetValues(_w, XmNbackground, &colour.pixel, NULL);
	}
	else if(same(_colorName,XtNforeground))
	{
		XtVaGetValues(_w, XmNforeground, &colour.pixel, NULL);
	}
	else
	{
		XtVaGetValues(_w, XmNcolormap, &cmap, XmNdepth, &depth, NULL);
		if(!XParseColor(dpy, cmap, _colorName, &colour))
		{
			(void) fprintf(stderr,"%s: Unrecognized colour name \"%s\" - using \"White\"\n", module, _colorName);
			colour.pixel = WhitePixelOfScreen(DefaultScreenOfDisplay(dpy));
		}
		(void)XuAllocColor(dpy, depth, cmap, &colour, True);
	}
	return (colour.pixel);
}


/* Load the given colour from the resource file or if not found use the default value.
 */
Pixel XuLoadColorResource(Widget _w, String _resName, String _defaultValue)
{
	return(XuLoadColor(_w, XuGetStringResource(_resName, _defaultValue)));
}



/* Try and allocate the given colour. If not successful we then find the closest
*  match in the colour table.
*/
Boolean XuAllocColor( Display *_display, int _depth, Colormap _colormap,
								XColor *_colour, const Boolean _do_closest_match )
{
	int          i;
	Status       allocated;
	ALLOC_STRUCT *alloc;

	allocated = XAllocColor(_display, _colormap, _colour);

	/* We can safely assume that if the depth is 16 bits or greater that the colormap
	 * is TrueColor and we do not need to store the pixel information to be freed later.
	 * There are some systems that use allocations at these depths, but very few people
	 * know how to use them and this library is not designed for this anyway.
	 */
	if( allocated && _depth >= 16 ) return True;

	/* Find data for the given display and colormap.
	*/
	alloc = alloc_data;
	while(NotNull(alloc) && (alloc->dpy != _display || alloc->cmap != _colormap))
	{
		alloc = alloc->next;
	}
	if(IsNull(alloc))
	{
		alloc = XTCALLOC(1, ALLOC_STRUCT);
		alloc->dpy  = _display;
		alloc->cmap = _colormap;
		alloc->next = alloc_data;
		alloc_data  = alloc;
	}

	if(allocated)
	{
		/* The pixel is stored so that we can free the allocation in xuFreeColors.
		 * For shallow display depths this is important so we can be a good
		 * resident of the display.
		 */
		for(i = 0; i < alloc->npx; i++) if(alloc->px[i] == _colour->pixel) break;
		if(i >= alloc->npx)
		{
			alloc->px = (Pixel *)XtRealloc((void *)alloc->px, (alloc->npx+1)*sizeof(Pixel));
			alloc->px[alloc->npx] = _colour->pixel;
			alloc->npx++;
		}
		return True;
	}
	else if(_do_closest_match)
	{
		/* Ok, our allocation failed so first we see if we already have the colour.
		 */
		for(i = 0; i < alloc->nsub; i++)
		{
			if(_colour->red   == alloc->sub[i].red   &&
			   _colour->green == alloc->sub[i].green &&
			   _colour->blue  == alloc->sub[i].blue)
				{
					_colour->pixel = alloc->sub[i].pixel;
					return False;
				}
		}

		/* The colour is a new one, so find the closest match.
		 */
		XuFindNearestColor(_display, _depth, _colormap, _colour, _colour);

		/* And again we see if we already have the closest match in our buffer.
		 */
		for(i = 0; i < alloc->nsub; i++)
		{
			if(_colour->red   == alloc->sub[i].red   &&
			   _colour->green == alloc->sub[i].green &&
			   _colour->blue  == alloc->sub[i].blue)
				{
					_colour->pixel = alloc->sub[i].pixel;
					return False;
				}
		}

		/* This is an entirely new entry so add it to our buffer.
		 */
		alloc->sub = (XColor *)XtRealloc((void *)alloc->sub, (alloc->nsub+1)*sizeof(XColor));
		(void) memcpy((void *)&alloc->sub[alloc->nsub], (void *)_colour, sizeof(XColor));
		alloc->sub[alloc->nsub].pixel = _colour->pixel;
		alloc->nsub++;
	}
	return False;
}


/* Find the nearest colour in the colormap to the given colour and 
*  return the match in the colour parameter. _out_colour contains
*  the nearest colour match.
*/
void XuFindNearestColor(Display *_display, int _depth, Colormap _colormap,
							XColor *_in_colour, XColor *_out_colour )
{
	int    i, diff, dmin, rv, gv, bv, rd, gd, bd;
	Cardinal ncolours;
	XColor *color_array;

	/* This condition should not happen, but just in case we return black.
	 * At bit depths more than this our allocation could easily take up all
	 * of memory!
	 */
	if(_depth > 16)
	{
		(void) memset((void*)_out_colour, 0, sizeof(XColor));
	}
	else
	{
		ncolours = (1 << _depth) - 1;
		color_array = XTCALLOC(ncolours, XColor);
		for(i = 0; i < ncolours; i++) color_array[i].pixel = (Pixel)i;
		XQueryColors(_display, _colormap, color_array, ncolours);
		dmin = 256*256 * 3;
		rv = ((int)_in_colour->red   >> 8);
		gv = ((int)_in_colour->green >> 8);
		bv = ((int)_in_colour->blue  >> 8);
		(void) memcpy((void *)_out_colour, (void *)color_array, sizeof(XColor));
		for(i = 0; i < ncolours; i++)
		{
			rd = rv - ((int)color_array[i].red   >> 8);
			gd = gv - ((int)color_array[i].green >> 8);
			bd = bv - ((int)color_array[i].blue  >> 8);
			diff = rd*rd + gd*gd + bd*bd;
			if(diff < dmin)
			{
				dmin = diff;
				(void) memcpy((void *)_out_colour, (void *)&color_array[i], sizeof(XColor));
			}
		}
		XtFree((void *)color_array);
	}
}


/* Free any allocated colours.
*/
void _xu_free_colours(Widget _w)
{
	Colormap colormap;
	ALLOC_STRUCT *alloc, *prev;

	if(_w)
	{
		XtVaGetValues(CW(_w), XmNcolormap, &colormap, NULL);
		alloc = prev = alloc_data;
		while(alloc)
		{
			if(alloc->dpy == XtDisplayOfObject(_w) && alloc->cmap == colormap)
			{
				XFreeColors(alloc->dpy, alloc->cmap, alloc->px, alloc->npx, 0);
				XtFree((void *)alloc->px);
				XtFree((void *)alloc->sub);
				if(alloc == alloc_data)
					alloc_data = alloc->next;
				else
					prev->next = alloc->next;
				XtFree((void *)alloc);
				break;
			}
			prev = alloc;
			alloc = alloc->next;
		}
		XFreeColormap(XtDisplayOfObject(_w), colormap);
	}
	else
	{
		alloc = alloc_data;
		while(alloc)
		{
			XFreeColors(alloc->dpy, alloc->cmap, alloc->px, alloc->npx, 0);
			XtFree((void *)alloc->px);
			XtFree((void *)alloc->sub);
			XFreeColormap(alloc->dpy, alloc->cmap);
			prev = alloc;
			alloc = alloc->next;
			XtFree((void *)prev);
		}
		alloc_data = NULL;
	}
}
