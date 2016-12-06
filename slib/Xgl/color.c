/*======================================================================*/
/*
*	File:  color.c
*
*   Purpose: Handles all colour related assignment activity. Colours are
*            specified globally and then defined locally so that multiple
*            drawables can be used without them all having to have the
*            same depth and type.
*
*            Reserved colours are defined by their colour index only and
*            can be redefined on the fly so that the same index will then
*            point to another colour. This is useful when the colour
*            index values are embedded into a complex structure and you
*            want to change colours without changing the referencing
*            index value.
*
*            Note that these functions are not called by the image
*            functions as it is assumed that the program will be run
*            in 16 or 24 bit TrueColor when using the image processing.
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
/*======================================================================*/

#include "FpaXglP.h"


static int cnamecmp(const void *a, const void *b)
{
	return strcmp(((XglColor*)a)->name, ((XglColor*)b)->name);
}


/* Assign a colour index value to the given name. If the reserved parameter
*  is True then no search is done for an existing name in our list.
*/
ColorIndex _xgl_color_index_from_name(String module, String name, int reserved)
{
	int    i, ndx;
	XColor xc;

	/* Do we have the colour already?
	*/
	if(!reserved && Xgl.Colors)
	{
		XglColor c, *p;
		c.name = name;
		p = (XglColor*)bsearch((void*)&c, (void*)&Xgl.Colors[Xgl.ReservedColors],
								(size_t)(Xgl.LastColor - Xgl.ReservedColors),
								sizeof(XglColor), cnamecmp);
		if(p) return p->ndx;
	}

	/* Check for a recognized colour name.
	 */
	xc.flags = DoRGB;
	if(!XParseColor(D, WX->cmap ? WX->cmap:Xgl.windows->x->cmap, name, &xc))
	{
		if(reserved)
		{
			pr_warning(module, "Unrecognized Colour name \"%s\" - black assigned.\n", name);
			(void) memset((void*) &xc, 0, sizeof(XColor));
			xc.flags = DoRGB;
		}
		else
		{
			pr_warning(module, "Unrecognized Colour name \"%s\".\n", name);
			return UnallocatedColorIndex;
		}
	}

	/* Allocate more memory if necessary
	 */
	if(Xgl.LastColor >= Xgl.ColorArrayLen)
	{
		Xgl.ColorArrayLen += 50;
		Xgl.Colors    = GETMEM(Xgl.Colors,    XglColor, Xgl.ColorArrayLen);
		Xgl.ColorXRef = GETMEM(Xgl.ColorXRef, int,      Xgl.ColorArrayLen);

		for(i = 1; i < Xgl.last_ndx; i++)
		{
			XglWindow *gw = &Xgl.windows[i];
			if(IsNull(gw->x) || IsNull(gw->x->front)) continue;
			gw->x->colorXRef = GETMEM(gw->x->colorXRef, int, Xgl.ColorArrayLen);
			for(ndx = Xgl.LastColor; ndx < Xgl.ColorArrayLen; ndx++)
				gw->x->colorXRef[ndx] = UnallocatedColorIndex;
		}
	}

	/* If reserved we add it to the start of the list. If not we put
	*  it in binary search order after the reserved section.
	*/
	if(reserved)
	{
		for(ndx = Xgl.LastColor; ndx > Xgl.ReservedColors; ndx--)
		{
			Xgl.Colors[ndx].name     = Xgl.Colors[ndx-1].name;
			Xgl.Colors[ndx].ndx      = Xgl.Colors[ndx-1].ndx;
			Xgl.Colors[ndx].xc.pixel = Xgl.Colors[ndx-1].xc.pixel;
			Xgl.Colors[ndx].xc.red   = Xgl.Colors[ndx-1].xc.red;
			Xgl.Colors[ndx].xc.green = Xgl.Colors[ndx-1].xc.green;
			Xgl.Colors[ndx].xc.blue  = Xgl.Colors[ndx-1].xc.blue;
			Xgl.Colors[ndx].xc.flags = DoRGB;
		}
		ndx = Xgl.ReservedColors;
		Xgl.ReservedColors++;
	}
	else
	{
		for(ndx = Xgl.LastColor; ndx > Xgl.ReservedColors; ndx--)
		{
			if(strcmp(name, Xgl.Colors[ndx-1].name) > 0) break;
			Xgl.Colors[ndx].name     = Xgl.Colors[ndx-1].name;
			Xgl.Colors[ndx].ndx      = Xgl.Colors[ndx-1].ndx;
			Xgl.Colors[ndx].xc.pixel = Xgl.Colors[ndx-1].xc.pixel;
			Xgl.Colors[ndx].xc.red   = Xgl.Colors[ndx-1].xc.red;
			Xgl.Colors[ndx].xc.green = Xgl.Colors[ndx-1].xc.green;
			Xgl.Colors[ndx].xc.blue  = Xgl.Colors[ndx-1].xc.blue;
			Xgl.Colors[ndx].xc.flags = DoRGB;
		}
	}

	Xgl.Colors[ndx].name = safe_strdup(name);
	Xgl.Colors[ndx].ndx  = (ColorIndex) Xgl.LastColor;
	(void) memcpy((void*) &Xgl.Colors[ndx].xc, (void*) &xc, sizeof(XColor));
	Xgl.ColorXRef[Xgl.LastColor] = ndx;

	/* All x refs which reference an entry >= than i must be incremented by one
	*  as their corresponding colour name has been bumped up the list.
	*/
	for(i = 0; i < Xgl.LastColor; i++)
	{
		if(Xgl.ColorXRef[i] >= ndx) Xgl.ColorXRef[i]++;
	}
	Xgl.LastColor++;

	return (ColorIndex) (Xgl.LastColor - 1);
}


/* Find the pixel value corresponding to the given colour index for the active window.
*/
Pixel _xgl_pixel_from_color_index(String module, ColorIndex ndx)
{
	XglWindow  *w = Xgl.active;

	/*
	 * 2007.02.26 RDP: If the index is unallocated return without issuing a message.
	 */
	if(ndx == UnallocatedColorIndex) return (Pixel)0;

	/*
	 * Make sure we have a valid active window.
	 */
	if(Xgl.active_ndx < 1 || Xgl.active_ndx >= Xgl.last_ndx)
	{
		if (module) pr_error(module, "Active window not valid.\n", NULL);
		return (Pixel)0;
	}

	if(ndx < 0 || ndx > (ColorIndex) Xgl.LastColor)
	{
		if (module) pr_error(module, "Invalid colour index: %d\n", ndx); 
		return (Pixel)0;
	}


	if(w->x->colorXRef[ndx] == UnallocatedColorIndex)
	{
		int i;
		XColor colour;

		/* A pixel value for the given global colour index has not been
		*  assigned yet. We do this now for this window.
		*/
		colour.red   = Xgl.Colors[Xgl.ColorXRef[ndx]].xc.red;
		colour.green = Xgl.Colors[Xgl.ColorXRef[ndx]].xc.green;
		colour.blue  = Xgl.Colors[Xgl.ColorXRef[ndx]].xc.blue;
		colour.flags = DoRGB;

		/* Try and allocate the colour, but do not let the XuAllocColor
		 * function do a closest match as we need to store the colour
		 * information in this library and not in XuLib.
		 */
		if(!XuAllocColor(D, w->x->depth, w->x->cmap, &colour, FALSE))
		{
			/* Do we already have this colour in our table?
			*/
			for(i = 0; i < w->x->ncolors; i++)
			{
				if(colour.red   != w->x->colors[i].red  ) continue;
				if(colour.green != w->x->colors[i].green) continue;
				if(colour.blue  != w->x->colors[i].blue ) continue;
				w->x->colorXRef[ndx] = i;
				return w->x->colors[i].pixel;
			}

			/* Search for the closest match.
			*/
			XuFindNearestColor(D, w->x->depth, w->x->cmap, &colour, &colour);

			/* Do we already have this pixel value in our array? This can happen
			*  if the colour table is full and we used the closest match. We don't
			*  want more than one rbg value for any given pixel value.
			*/
			for(i = 0; i < w->x->ncolors; i++)
			{
				if(colour.pixel != w->x->colors[i].pixel) continue;
				w->x->colorXRef[ndx] = i;
				return w->x->colors[i].pixel;
			}
		}

		/* Passed all checks so add this colour to our table. Check to see if
		*  we have any existing array entry available for reuse.
		*/
		for(i = 0; i < w->x->ncolors; i++)
		{
			if(w->x->colors[i].flags != 0) continue;
			w->x->colorXRef[ndx] = i;
			w->x->colors[i].pixel = colour.pixel;
			w->x->colors[i].red   = colour.red;
			w->x->colors[i].green = colour.green;
			w->x->colors[i].blue  = colour.blue;
			w->x->colors[i].flags = DoRGB;
			return w->x->colors[i].pixel;
		}

		/* None free so we will have to extend our list.
		*/
		w->x->colorXRef[ndx] = w->x->ncolors;
		w->x->colors = GETMEM(w->x->colors, XColor, w->x->ncolors+1);
		w->x->colors[w->x->ncolors].pixel = colour.pixel;
		w->x->colors[w->x->ncolors].red   = colour.red;
		w->x->colors[w->x->ncolors].green = colour.green;
		w->x->colors[w->x->ncolors].blue  = colour.blue;
		w->x->colors[w->x->ncolors].flags = DoRGB;
		w->x->ncolors++;
	}
	return (w->x->colors[w->x->colorXRef[ndx]].pixel);
}


Pixel _xgl_pixel_from_name(String module, String name)
{
	ColorIndex ndx = _xgl_color_index_from_name(module, name, FALSE);
	return _xgl_pixel_from_color_index(module, ndx);
}


/* Find the pixel value associated with the given RGB value. To stay compatable
*  with the rest of the colour scheme this is done by creating a name as per
*  the XParseColor() specification.
*/
Pixel _xgl_pixel_from_XColor(String module, XColor color)
{
	char mbuf[32];
	(void) snprintf(mbuf, sizeof(mbuf), "rgb:%.4x/%.4x/%.4x", color.red, color.green, color.blue);
	return _xgl_pixel_from_name(module, mbuf);
}

void _xgl_set_xft_color(Pixel px, XftColor *color)
{
	XColor xcol;
	XRenderColor rcol;

	XftColorFree(WX->display, WX->visual_info->visual, WX->cmap, color);
	xcol.pixel = px;
	XQueryColor(WX->display, WX->cmap, &xcol);
	rcol.red = xcol.red;
	rcol.green = xcol.green;
	rcol.blue = xcol.blue;
	rcol.alpha = 0xffff;
	XftColorAllocValue( WX->display, WX->visual_info->visual, WX->cmap, &rcol, color);
}


/* Assign a reserved colour index.
*/
ColorIndex glReservedColorIndex(char *name)
{
	return (_xgl_color_index_from_name("glReservedColorIndex", name, TRUE));
}


/* Reassign the given colour index to another colour given its name. This may
*  only be done with those colours which have been reserved.
*/
void glLoadColor(ColorIndex ndx, char *name)
{
	int i, idx;
	static char *MyName = "glLoadColor";

	/* We must use the cross referenced index.
	 */
	idx = Xgl.ColorXRef[ndx];

	if(idx >= Xgl.ReservedColors)
	{
		pr_error(MyName, "The given ColorIndex is not a reserved index.\n", NULL);
	}
	else if(!XParseColor(D, WX->cmap ? WX->cmap:Xgl.windows->x->cmap, name, &Xgl.Colors[idx].xc))
	{
		pr_warning(MyName, "Unrecognized Colour name \"%s\".\n", name);
	}
	else
	{
		FREEMEM(Xgl.Colors[idx].name);
		Xgl.Colors[idx].name = safe_strdup(name);
		Xgl.Colors[idx].xc.flags = DoRGB;

		for(i = 1; i < Xgl.last_ndx; i++)
		{
			int n, count;
			XglWindow *w = &Xgl.windows[i];

			if(!w->x->front) continue;

			/* If the XColor pointed to by the colour index is only referenced
			*  by this index we make the XColor available for reuse.
			*/
			for(count = 0, n = 0; n < Xgl.LastColor; n++)
			{
				if(w->x->colorXRef[n] == w->x->colorXRef[ndx]) count++;
			}
			if(count == 1) w->x->colors[w->x->colorXRef[ndx]].flags = 0;
			w->x->colorXRef[ndx] = UnallocatedColorIndex;
		}
	}
}


ColorIndex glSetColor(char *name)
{
	static String MyName = "glSetColor";

	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W)
	{
		Pixel px  = _xgl_pixel_from_color_index(MyName, ndx);
		XSetForeground(D, WX->linegc, px);
		XSetForeground(D, WX->fillgc, px);
		_xgl_set_xft_color(px, WX->xftfg);
	}
	return ndx;
}

Pixel glGetPixelFromColorIndex( ColorIndex ndx )
{
	return _xgl_pixel_from_color_index("glGetPixelFromColorIndex", ndx);
}

ColorIndex glSetLineColor(char *name)
{
	static String MyName = "glSetLineColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W) XSetForeground(D, WX->linegc, _xgl_pixel_from_color_index(MyName, ndx));
	return ndx;
}

ColorIndex glSetFillColor(char *name)
{
	static String MyName = "glSetFillColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W) XSetForeground(D, WX->fillgc, _xgl_pixel_from_color_index(MyName, ndx));
	return ndx;
}

ColorIndex glSetTextColor(char *name)
{
	static String MyName = "glSetTextColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W)
	{
		Pixel px = _xgl_pixel_from_color_index(MyName, ndx);
		_xgl_set_xft_color(px, WX->xftfg);
	}
	return ndx;
}

ColorIndex glSetBgColor(char *name)
{
	static String MyName = "glSetBgColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W)
	{
		Pixel px  = _xgl_pixel_from_color_index(MyName, ndx);
		XSetBackground(D, WX->linegc, px);
		XSetBackground(D, WX->fillgc, px);
		_xgl_set_xft_color(px, WX->xftbg);
	}
	return ndx;
}

ColorIndex glSetLineBgColor(char *name)
{
	static String MyName = "glSetLineBgColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W) XSetBackground(D, WX->linegc, _xgl_pixel_from_color_index(MyName, ndx));
	return ndx;
}

ColorIndex glSetFillBgColor(char *name)
{
	static String MyName = "glSetFillBgColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W) XSetBackground(D, WX->fillgc, _xgl_pixel_from_color_index(MyName, ndx));
	return ndx;
}

ColorIndex glSetTextBgColor(char *name)
{
	static String MyName = "glSetTextBgColor";
	ColorIndex ndx = _xgl_color_index_from_name(MyName, name, FALSE);
	if(W)
	{
		Pixel px = _xgl_pixel_from_color_index(MyName, ndx);
		_xgl_set_xft_color(px, WX->xftbg);
	}
	return ndx;
}

void glSetColorIndex(ColorIndex ndx)
{
	Pixel px;
	if (!W) return;
	px = _xgl_pixel_from_color_index("glSetColorIndex", ndx);
	XSetForeground(D, WX->linegc, px);
	XSetForeground(D, WX->fillgc, px);
	_xgl_set_xft_color(px, WX->xftfg);
}

void glSetLineColorIndex(ColorIndex ndx)
{
	if (!W) return;
	XSetForeground(D, WX->linegc, _xgl_pixel_from_color_index("glSetLineColorIndex", ndx));
}

void glSetFillColorIndex(ColorIndex ndx)
{
	if (!W) return;
	XSetForeground(D, WX->fillgc, _xgl_pixel_from_color_index("glSetFilColorIndex", ndx));
}

void glSetTextColorIndex(ColorIndex ndx)
{
	Pixel px;
	if (!W) return;
	px = _xgl_pixel_from_color_index("glSetTextColorIndex", ndx);
	_xgl_set_xft_color(px, WX->xftfg);
}

void glSetBgColorIndex(ColorIndex ndx)
{
	Pixel px;
	if (!W) return;
	px = _xgl_pixel_from_color_index("glSetBgColorIndex", ndx);
	XSetBackground(D, WX->linegc, px);
	XSetBackground(D, WX->fillgc, px);
	_xgl_set_xft_color(px, WX->xftbg);
}

void glSetLineBgColorIndex(ColorIndex ndx)
{
	if (!W) return;
	XSetBackground(D, WX->linegc, _xgl_pixel_from_color_index("glSetLineBgColorIndex", ndx));
}

void glSetFillBgColorIndex(ColorIndex ndx)
{
	if (!W) return;
	XSetBackground(D, WX->fillgc, _xgl_pixel_from_color_index("glSetFillBgColorIndex", ndx));
}

void glSetTextBgColorIndex(ColorIndex ndx)
{
	Pixel px;
	if (!W) return;
	px = _xgl_pixel_from_color_index("glSetTextBgColorIndex", ndx);
	_xgl_set_xft_color(px, WX->xftbg);
}


/* Returns a colormap of the colours actually used by the active
*  window.  If the colour parameter is NULL then only the size
*  is returned.
*
*  Note: Three functions are assigned to the Xgl structure here.
*        This is strictly a cludge as this function is called
*        from glInit and in this case does nothing.
*/
void glGetColormap(XColor **colour, int *size)
{
	int i, n;
	XColor *color;

	if( !size ) return;

	for(n = 0, i = 0; i < WX->ncolors; i++)
	{
		if(WX->colors[i].flags != 0) n++;
	}
	*size = n;
	if(!colour) return;

	color = MEM(XColor, n);
	for(n = 0, i = 0; i < WX->ncolors; i++)
	{
		if (WX->colors[i].flags == 0) continue;
		color[n].pixel = WX->colors[i].pixel;
		color[n].red   = WX->colors[i].red;
		color[n].green = WX->colors[i].green;
		color[n].blue  = WX->colors[i].blue;
		color[n].flags = DoRGB;
		n++;
	}
	*colour = color;
}
