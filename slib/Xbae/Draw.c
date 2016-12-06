/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2003, 2004 by the LessTif Developers.
 *
 *			All rights reserved
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Bellcore not be used in advertising
 * or publicity pertaining to this material without the specific,
 * prior written permission of an authorized representative of
 * Bellcore.
 *
 * BELLCORE MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES, EX-
 * PRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST IN-
 * FRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL BELLCORE OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELAT-
 * ING TO THE SOFTWARE.
 *
 * $Id: Draw.c,v 1.126 2005/07/31 18:35:32 tobiasoed Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/DrawP.h>
#include <Xbae/MatrixP.h>
#include <Xbae/Utils.h>
#include <Xbae/Shadow.h>
#include <Xbae/Draw.h>
#include <Xbae/Create.h>

#include "XbaeDebug.h"

/*
 * Width in pixels of a character in a given font
 */
#define charWidth(fs,c) ( \
    (fs)->per_char \
      ? (fs)->per_char[(((unsigned char) c) < (fs)->min_char_or_byte2 || ((unsigned char) c) >= (fs)->max_char_or_byte2\
			? (fs)->default_char - (fs)->min_char_or_byte2 \
			: ((unsigned char) c) - (fs)->min_char_or_byte2)].width \
      : (fs)->min_bounds.width)


/* This exists to avoid multiple creates and destroys to get
 * the XftDraw variable needed by the Xft functions.
 */
static XftDraw *xbaeXftDrawCreate(XbaeMatrixWidget mw, Window window)
{
	int i;
	XftDraw *draw;
	Display *display = XtDisplay(mw);

	for(i = 0; i < mw->matrix.xft_draw_cache_len; i++)
	{
		if(mw->matrix.xft_draw_cache[i].win == window)
			return mw->matrix.xft_draw_cache[i].draw;
	}

	if(!(draw = XftDrawCreate(display, window,
							  DefaultVisual(display, XScreenNumberOfScreen(mw->core.screen)),
							  mw->core.colormap)))
	{
		draw = XftDrawCreateBitmap(display, window);
	}

	i = mw->matrix.xft_draw_cache_len;	/* Next free index */
	mw->matrix.xft_draw_cache_len = mw->matrix.xft_draw_cache_len + 1;
	mw->matrix.xft_draw_cache =
		(XbaeXftDrawCache *) XtRealloc((void *) mw->matrix.xft_draw_cache,
									   sizeof(XbaeXftDrawCache) * mw->matrix.xft_draw_cache_len);
	mw->matrix.xft_draw_cache[i].win = window;
	mw->matrix.xft_draw_cache[i].draw = draw;

	return draw;
}


int xbaeXftStringWidth(XbaeMatrixWidget mw, XftFont *font, String string, int nc)
{
	XGlyphInfo ext;
	ext.xOff = 0;
	XftTextExtentsUtf8(XtDisplay(mw), font, (FcChar8 *) string, nc, &ext);
	return ext.xOff;
}


/* This was added to avoid multiple calls to the XQueryColor function.
 */
static XftColor *xftColorOfPixel(XbaeMatrixWidget mw, Pixel color)
{
	int n;
	XColor xcol;
	XRenderColor rcol;

	for(n = 0; n < mw->matrix.xft_color_cache_len; n++)
	{
		if(mw->matrix.xft_color_cache[n].pixel == color)
			return &mw->matrix.xft_color_cache[n].xftcolor;
	}

	n = mw->matrix.xft_color_cache_len;
	mw->matrix.xft_color_cache_len++;
	mw->matrix.xft_color_cache =
		(XbaeXftColorCache *) XtRealloc((void *) mw->matrix.xft_color_cache,
										mw->matrix.xft_color_cache_len * sizeof(XbaeXftColorCache));

	mw->matrix.xft_color_cache[n].pixel = color;
	xcol.pixel = color;
	XQueryColor(XtDisplay(mw), mw->core.colormap, &xcol);
	rcol.red = xcol.red;
	rcol.blue = xcol.blue;
	rcol.green = xcol.green;
	rcol.alpha = 0xFFFF;
	XftColorAllocValue(XtDisplay(mw), DefaultVisual(XtDisplay(mw), XScreenNumberOfScreen(mw->core.screen)),
					   mw->core.colormap, &rcol, &mw->matrix.xft_color_cache[n].xftcolor);

	return &mw->matrix.xft_color_cache[n].xftcolor;
}


/*
 * Draw a string with specified attributes. We want to avoid having to
 * use a GC clip_mask, so we clip by characters. This complicates the code.
 */
static void xbaeDrawString(XbaeMatrixWidget mw, Window win, GC gc,
						   int x, int y, int width, int height,
						   unsigned char alignment,
						   Boolean bold, Boolean arrows, Boolean underline,
						   Boolean multiline, unsigned char wrap,
						   Pixel color,
						   String string, XbaeMatrixFontInfo * font, int font_height, int baseline)
{

	int aw = mw->matrix.cell_font.width / 2;
	int ah = mw->matrix.cell_font.height / 2;

	int length = strlen(string);

	x += CELL_BORDER_WIDTH(mw);
	y += CELL_BORDER_HEIGHT(mw);
	width -= 2 * CELL_BORDER_WIDTH(mw);
	height -= 2 * CELL_BORDER_HEIGHT(mw);

	/*
	 * Don't worry, XSetForeground is smart about avoiding unnecessary
	 * protocol requests.
	 */
	XSetForeground(XtDisplay(mw), gc, color);

	if(font->type == XmFONT_IS_XFT)
	{
		int start, current, end;
		mbstate_t start_mbs, current_mbs, end_mbs;
		XftFont *xftFont = (XftFont *) font->fontp;
		XftDraw *draw = xbaeXftDrawCreate(mw, win);

		memset(&start_mbs, '\0', sizeof start_mbs);
		memset(&current_mbs, '\0', sizeof current_mbs);
		memset(&end_mbs, '\0', sizeof end_mbs);

		for(start = 0, end = 0; height > 0 && end < length;
			height -= font_height, y += font_height, start = end, start_mbs = end_mbs)
		{

			Boolean LeftArrow = False;
			Boolean RightArrow = False;
			Boolean doclip = False;

			int current_width, offset;

			/*
			 * If the font is too tall for the space we have to clip
			 */
			if(height < font_height)
			{
				XRectangle clipht;
				clipht.x = (short) x;
				clipht.y = (short) y;
				clipht.width = (unsigned short) width;
				clipht.height = (unsigned short) height;
				XftDrawSetClipRectangles(draw, x, y, &clipht, 1);
				doclip = True;
			}

			/*
			 * Find the end of the line and how wide it is
			 */
			if(multiline)
			{
				int space = 0;
				int space_width = 0;
				mbstate_t space_mbs;

				current_width = 0;

				switch (wrap)
				{
					case XbaeWrapNone:
						while(end < length && string[end] != '\n')
						{
							int cl = (int) mbrlen(string + end, length - end, &end_mbs);
							int cw = xbaeXftStringWidth(mw, xftFont, string + end, cl);
							current_width += cw;
							end += cl;
						}
						break;

					case XbaeWrapContinuous:
						while(end < length && string[end] != '\n')
						{
							int cl = (int) mbrlen(string + end, length - end, &end_mbs);
							int cw = xbaeXftStringWidth(mw, xftFont, string + end, cl);
							if(current_width + cw >= width) break;
							current_width += cw;
							end += cl;
						}
						break;

					case XbaeWrapWord:
						while(end < length && string[end] != '\n')
						{
							int cl = (int) mbrlen(string + end, length - end, &end_mbs);
							int cw = xbaeXftStringWidth(mw, xftFont, string + end, cl);
							if(current_width + cw >= width) break;
							if(isspace(string[end]))
							{
								space = end;
								space_mbs = end_mbs;
								space_width = current_width;
							}
							current_width += cw;
							end += cl;
						}
						break;
				}

				if(end == length)
				{
					current = end;
					current_mbs = end_mbs;
				}
				else if(string[end] == '\n')
				{
					current = end;
					current_mbs = end_mbs;
					end++;
				}
				else if(space_width > 0)
				{
					current_width = space_width;
					current = space;
					current_mbs = space_mbs;
					end = space + 1;
				}
				else
				{
					current = end;
					current_mbs = end_mbs;
				}
			}
			else
			{
				current_width = xbaeXftStringWidth(mw, xftFont, string, length);
				current = end = length;
			}

			switch (alignment)
			{
				case XmALIGNMENT_CENTER:
					offset = width / 2 - current_width / 2;
					break;

				case XmALIGNMENT_END:
					offset = width - current_width;
					break;

				case XmALIGNMENT_BEGINNING:
				default:
					offset = 0;
					break;
			}

			/*
			 * Trim the beginning of the string
			 */
			if(offset < 0)
			{
				int min;

				if(arrows)
				{
					LeftArrow = True;
					min = aw;
				}
				else
				{
					min = 0;
				}

				while(offset < min)
				{
					int cl = (int) mbrlen(string + start, length - start, &start_mbs);
					int cw = xbaeXftStringWidth(mw, xftFont, string + start, cl);
					current_width -= cw;
					offset += cw;
					start += cl;
				}
			}

			/* 
			 * Trim the end of the string
			 */
			if(offset + current_width > width)
			{
				int max;

				if(arrows)
				{
					RightArrow = True;
					max = width - aw;
				}
				else
				{
					max = width;
				}

				current_width = 0;
				current = start;
				current_mbs = start_mbs;

				for(;;)
				{
					int cl = (int) mbrlen(string + current, length - current, &current_mbs);
					int cw = xbaeXftStringWidth(mw, xftFont, string + current, cl);
					if(offset + current_width + cw > max) break;
					current_width += cw;
					current += cl;
				}
			}

			/*
			 * If we have arrows, draw them
			 */
			if(LeftArrow)
			{
				XPoint points[3];
				points[0].x = points[1].x = x + aw;
				points[0].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2;
				points[1].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah;
				points[2].x = x;
				points[2].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah / 2;
				XFillPolygon(XtDisplay(mw), win, gc, points, 3, Convex, CoordModeOrigin);
			}

			if(RightArrow)
			{
				XPoint points[3];
				points[0].x = points[1].x = x + width - 1 - aw;
				points[0].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2;
				points[1].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah;
				points[2].x = x + width - 1;
				points[2].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah / 2;
				XFillPolygon(XtDisplay(mw), win, gc, points, 3, Convex, CoordModeOrigin);
			}

			/*
			 * Draw the string
			 */
			XftDrawStringUtf8(draw, xftColorOfPixel(mw, color), xftFont, x + offset, y + baseline,
							  (XftChar8 *) (string + start), current - start);

			if(underline && current_width)
			{
				int i;
				for(i = 0; i < mw->matrix.underline_width; i++)
					XDrawLine(XtDisplay(mw), win, gc,
							  x + offset, y + baseline + mw->matrix.underline_position + i,
							  x + offset + current_width, y + baseline + mw->matrix.underline_position + i);
			}

			if(doclip)
			{
				XftDrawSetClipRectangles(draw, x, y, NULL, 0);
			}
		}
	}
	else if(font->type == XmFONT_IS_FONTSET)
	{
		XFontSet font_set = (XFontSet )font->fontp;

		XRectangle *ink_array = NULL;
		XRectangle *logical_array = NULL;
		XRectangle overall_logical;
		int num_chars;
		int start_char, current_char, end_char;
		int start, current, end;
		mbstate_t start_mbs, current_mbs, end_mbs;

		ink_array = (XRectangle *) XtMalloc(length * sizeof(XRectangle));
		logical_array = (XRectangle *) XtMalloc(length * sizeof(XRectangle));

		XmbTextPerCharExtents(font_set, string, length, ink_array, logical_array,
							  length, &num_chars, NULL, &overall_logical);

		memset(&start_mbs, '\0', sizeof start_mbs);
		memset(&current_mbs, '\0', sizeof current_mbs);
		memset(&end_mbs, '\0', sizeof end_mbs);

		for(start_char = 0, start = 0, end_char = 0, end = 0;
			height > 0 && end_char < num_chars && end < length;
			height -= font_height, y += font_height, start_char = end_char, start = end, start_mbs = end_mbs)
		{

			Boolean LeftArrow = False;
			Boolean RightArrow = False;
			Boolean doclip = False;

			int current_width, offset;

			/*
			 * If the font is too tall for the space we have to clip
			 */
			if(height < font_height)
			{
				XRectangle clipht;
				clipht.x = (short) x;
				clipht.y = (short) y;
				clipht.width = (unsigned short) width;
				clipht.height = (unsigned short) height;
				XSetClipRectangles(XtDisplay(mw), gc, 0, 0, &clipht, 1, Unsorted);
				doclip = True;
			}

			/*
			 * Find the dimensions of the line
			 */
			if(multiline)
			{
				int space_char = 0;
				int space = 0;
				int space_width = 0;
				mbstate_t space_mbs;

				current_width = 0;

				switch (wrap)
				{
					case XbaeWrapNone:
						while(end_char < num_chars && end < length && string[end] != '\n')
						{
							int cw = logical_array[end_char].width;
							int cl = (int) mbrlen(string + end, length - end, &end_mbs);
							current_width += cw;
							end += cl;
							end_char++;
						}
						break;

					case XbaeWrapContinuous:
						while(end_char < num_chars && end < length
							  && string[end] != '\n' && current_width + logical_array[end_char].width < width)
						{
							int cw = logical_array[end_char].width;
							int cl = (int) mbrlen(string + end, length - end, &end_mbs);
							current_width += cw;
							end += cl;
							end_char++;
						}
						break;

					case XbaeWrapWord:
						while(end_char < num_chars && end < length
							  && string[end] != '\n' && current_width + logical_array[end_char].width < width)
						{
							int cw = logical_array[end_char].width;
							int cl = (int) mbrlen(string + end, length - end, &end_mbs);
							if(isspace(string[end]))
							{
								space = end;
								space_char = end_char;
								space_mbs = end_mbs;
								space_width = current_width;
							}
							current_width += cw;
							end += cl;
							end_char++;
						}
						break;
				}

				if(end == length)
				{
					current = end;
					current_char = end_char;
					current_mbs = end_mbs;
				}
				else if(string[end] == '\n')
				{
					current = end;
					current_char = end_char;
					current_mbs = end_mbs;
					end++;
					end_char++;
				}
				else if(space_width > 0)
				{
					current_width = space_width;
					current = space;
					current_char = space_char;
					current_mbs = space_mbs;
					end = space + 1;
					end_char = space_char + 1;
				}
				else
				{
					current = end;
					current_char = end_char;
					current_mbs = end_mbs;
				}

			}
			else
			{
				current_width = overall_logical.width;
				current = end = length;
				current_char = end_char = num_chars;
			}

			switch (alignment)
			{
				case XmALIGNMENT_CENTER:
					offset = width / 2 - current_width / 2;
					break;

				case XmALIGNMENT_END:
					offset = width - current_width;
					break;

				case XmALIGNMENT_BEGINNING:
				default:
					offset = 0;
					break;
			}

			/*
			 * Trim the beginning of the string
			 */
			if(offset < 0)
			{
				int min;

				if(arrows)
				{
					LeftArrow = True;
					min = aw;
				}
				else
				{
					min = 0;
				}

				while(offset < min)
				{
					int cw = logical_array[start_char].width;
					int cl = (int) mbrlen(string + start, length - start, &start_mbs);
					current_width -= cw;
					offset += cw;
					start += cl;
					start_char++;
				}
			}

			/*
			 * Trim the end of the string
			 */
			if(offset + current_width > width)
			{
				int max;

				if(arrows)
				{
					RightArrow = True;
					max = width - aw;
				}
				else
				{
					max = width;
				}

				current_width = 0;
				current_char = start_char;
				current = start;
				current_mbs = start_mbs;

				while(offset + current_width + logical_array[current_char].width <= max)
				{
					int cw = logical_array[current_char].width;
					int cl = (int) mbrlen(string + current, length - current, &current_mbs);
					current_width += cw;
					current += cl;
					current_char++;
				}
			}

			/*
			 * If we have arrows, draw them
			 */
			if(LeftArrow)
			{
				XPoint points[3];
				points[0].x = points[1].x = x + aw;
				points[0].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2;
				points[1].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah;
				points[2].x = x;
				points[2].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah / 2;
				XFillPolygon(XtDisplay(mw), win, gc, points, 3, Convex, CoordModeOrigin);
			}

			if(RightArrow)
			{
				XPoint points[3];
				points[0].x = points[1].x = x + width - 1 - aw;
				points[0].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2;
				points[1].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah;
				points[2].x = x + width - 1;
				points[2].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah / 2;
				XFillPolygon(XtDisplay(mw), win, gc, points, 3, Convex, CoordModeOrigin);
			}

			/*
			 * Draw the string
			 */

			XmbDrawString(XtDisplay(mw), win, font_set, gc,
						  x + offset, y + baseline, string + start, current - start);

			if(bold)
				XmbDrawString(XtDisplay(mw), win, font_set, gc,
							  x + offset - 1, y + baseline, string + start, current - start);

			if(underline && current_width)
			{
				int i;
				for(i = 0; i < mw->matrix.underline_width; i++)
					XDrawLine(XtDisplay(mw), win, gc,
							  x + offset, y + baseline + mw->matrix.underline_position + i,
							  x + offset + current_width, y + baseline + mw->matrix.underline_position + i);
			}

			if(doclip)
			{
				XSetClipMask(XtDisplay(mw), gc, None);
			}
		}

		if(ink_array)
			XtFree((char *) ink_array);
		if(logical_array)
			XtFree((char *) logical_array);
	}
	else
	{
		XFontStruct *font_struct = (XFontStruct *) font->fontp;
		int start, current, end;

		for(start = 0, end = 0; height > 0 && end < length;
			height -= font_height, y += font_height, start = end)
		{

			Boolean LeftArrow = False;
			Boolean RightArrow = False;
			Boolean doclip = False;

			int current_width, offset;

			/*
			 * If the font is too tall for the space we have to clip
			 */
			if(height < font_height)
			{
				XRectangle clipht;
				clipht.x = (short) x;
				clipht.y = (short) y;
				clipht.width = (unsigned short) width;
				clipht.height = (unsigned short) height;
				XSetClipRectangles(XtDisplay(mw), gc, 0, 0, &clipht, 1, Unsorted);
				doclip = True;
			}

			/*
			 * Find the end of the line and how wide it is
			 */
			if(multiline)
			{
				int space = 0;
				int space_width = 0;

				current_width = 0;

				switch (wrap)
				{
					case XbaeWrapNone:
						while(end < length && string[end] != '\n')
						{
							int cw = charWidth(font_struct, string[end]);
							current_width += cw;
							end++;
						}
						break;

					case XbaeWrapContinuous:
						while(end < length
							  && string[end] != '\n'
							  && current_width + charWidth(font_struct, string[end]) < width)
						{
							int cw = charWidth(font_struct, string[end]);
							current_width += cw;
							end++;
						}
						break;

					case XbaeWrapWord:
						while(end < length
							  && string[end] != '\n'
							  && current_width + charWidth(font_struct, string[end]) < width)
						{
							int cw = charWidth(font_struct, string[end]);
							if(isspace(string[end]))
							{
								space = end;
								space_width = current_width;
							}
							current_width += cw;
							end++;
						}
						break;
				}

				if(end == length)
				{
					current = end;
				}
				else if(string[end] == '\n')
				{
					current = end;
					end++;
				}
				else if(space_width > 0)
				{
					current_width = space_width;
					current = space;
					end = space + 1;
				}
				else
				{
					current = end;
				}
			}
			else
			{
				current_width = XTextWidth(font_struct, string, length);
				current = end = length;
			}

			switch (alignment)
			{
				case XmALIGNMENT_CENTER:
					offset = width / 2 - current_width / 2;
					break;

				case XmALIGNMENT_END:
					offset = width - current_width;
					break;

				case XmALIGNMENT_BEGINNING:
				default:
					offset = 0;
					break;
			}

			/*
			 * Trim the beginning of the string
			 */
			if(offset < 0)
			{
				int min;

				if(arrows)
				{
					LeftArrow = True;
					min = aw;
				}
				else
				{
					min = 0;
				}

				while(offset < min)
				{
					int cw = charWidth(font_struct, string[start]);
					current_width -= cw;
					offset += cw;
					start++;
				}
			}

			/* 
			 * Trim the end of the string
			 */
			if(offset + current_width > width)
			{
				int max;

				if(arrows)
				{
					RightArrow = True;
					max = width - aw;
				}
				else
				{
					max = width;
				}

				current_width = 0;
				current = start;

				while(offset + current_width + charWidth(font_struct, string[current]) <= max)
				{
					int cw = charWidth(font_struct, string[current]);
					current_width += cw;
					current++;
				}
			}

			/*
			 * If we have arrows, draw them
			 */
			if(LeftArrow)
			{
				XPoint points[3];
				points[0].x = points[1].x = x + aw;
				points[0].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2;
				points[1].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah;
				points[2].x = x;
				points[2].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah / 2;
				XFillPolygon(XtDisplay(mw), win, gc, points, 3, Convex, CoordModeOrigin);
			}

			if(RightArrow)
			{
				XPoint points[3];
				points[0].x = points[1].x = x + width - 1 - aw;
				points[0].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2;
				points[1].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah;
				points[2].x = x + width - 1;
				points[2].y = y + TEXT_HEIGHT(mw) / 2 - font_height / 2 + ah / 2;
				XFillPolygon(XtDisplay(mw), win, gc, points, 3, Convex, CoordModeOrigin);
			}

			/*
			 * Draw the string
			 */
			XDrawString(XtDisplay(mw), win, gc, x + offset, y + baseline, string + start, current - start);

			if(bold)
				XDrawString(XtDisplay(mw), win, gc,
							x + offset, y + baseline, string + start, current - start);

			if(underline && current_width)
			{
				int i;
				for(i = 0; i < mw->matrix.underline_width; i++)
					XDrawLine(XtDisplay(mw), win, gc,
							  x + offset, y + baseline + mw->matrix.underline_position + i,
							  x + offset + current_width, y + baseline + mw->matrix.underline_position + i);
			}

			if(doclip)
			{
				XSetClipMask(XtDisplay(mw), gc, None);
			}
		}
	}
}

/*
 * Draw an xmstring with (some of the) specified attributes.
 */
static void xbaeDrawXmString(XbaeMatrixWidget mw, Window win, GC gc,
							 int x, int y, int width, int height,
							 unsigned char alignment,
							 Boolean bold, Boolean arrows, Boolean underline,
							 Pixel color, XmString xms, XmRenderTable render_table)
{
	XRectangle clipht;

	x += CELL_BORDER_WIDTH(mw);
	y += CELL_BORDER_HEIGHT(mw) + 1;
	width -= 2 * CELL_BORDER_WIDTH(mw);
	height -= 2 * CELL_BORDER_HEIGHT(mw);

	XSetForeground(XtDisplay(mw), gc, color);

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "%s(%s) x %d y %d, clip w %d h %d\n", __FUNCTION__,
			  "(some xmstring)", x, y, width, height));

	clipht.x = x;
	clipht.y = y;
	clipht.width = width;
	clipht.height = height;

	XmStringDraw(XtDisplay(mw), win, render_table, xms, gc, x, y, width, alignment,
				 XmSTRING_DIRECTION_L_TO_R, &clipht);
}

/*
 * Draw a cell's pixmap. The coordinates are calculated relative
 * to the correct window and pixmap is copied to that window.
 */
static void
xbaeDrawCellPixmap(XbaeMatrixWidget mw, Window win,
				   int x, int y, int width, int height,
				   unsigned char alignment,
				   Pixmap pixmap, Pixmap mask, int pixmap_width, int pixmap_height, Pixel bg, Pixel fg,
				   int depth)
{
	int src_x, src_y, dest_x, dest_y;
	int copy_width, copy_height;
	int space = mw->matrix.cell_shadow_thickness + mw->matrix.cell_highlight_thickness;

	Display *display = XtDisplay(mw);
	GC gc = mw->matrix.pixmap_gc;

	x += space;
	y += space;
	width -= 2 * space;
	height -= 2 * space;

	XSetForeground(display, gc, fg);
	XSetBackground(display, gc, bg);

	/*
	 * Adjust y source and destinations.
	 */
	if(pixmap_height > height)
	{
		copy_height = height;
		src_y = (pixmap_height - height) / 2;
		dest_y = 0;
	}
	else
	{
		copy_height = pixmap_height;
		src_y = 0;
		dest_y = (height - pixmap_height) / 2;
	}

	/*
	 * Adjust the x source and destination paying attention to the alignment.
	 */
	if(pixmap_width > width)
	{
		copy_width = width;
	}
	else
	{
		copy_width = pixmap_width;
	}

	switch (alignment)
	{
		case XmALIGNMENT_CENTER:
			if(pixmap_width > width)
			{
				src_x = (pixmap_width - width) / 2;
				dest_x = 0;
			}
			else
			{
				src_x = 0;
				dest_x = (width - pixmap_width) / 2;
			}
			break;
		case XmALIGNMENT_END:
			if(pixmap_width > width)
			{
				src_x = pixmap_width - width;
				dest_x = 0;
			}
			else
			{
				src_x = 0;
				dest_x = width - pixmap_width;
			}
			break;
		case XmALIGNMENT_BEGINNING:
		default:
			src_x = 0;
			dest_x = 0;
			break;
	}

	dest_y += y;
	dest_x += x;

	/*
	 * Draw the pixmap.  Clip it, if necessary
	 */
	if(pixmap && pixmap != XmUNSPECIFIED_PIXMAP)
	{
		if(depth > 1)
		{
			/* A pixmap using xpm */
			if(mask && mask != XmUNSPECIFIED_PIXMAP)
			{
				XSetClipMask(display, gc, mask);
				XSetClipOrigin(display, gc, dest_x - src_x, dest_y - src_y);
			}
			XCopyArea(display, pixmap, win, gc, src_x, src_y, copy_width, copy_height, dest_x, dest_y);
			if(mask && mask != XmUNSPECIFIED_PIXMAP)
			{
				XSetClipMask(display, gc, None);
			}
		}
		else
		{
			/* A plain old bitmap */
			XCopyPlane(display, pixmap, win, gc, src_x, src_y, copy_width, copy_height, dest_x, dest_y, 1L);
		}
	}
}

/*
 * Draw a cell's string. The coordinates are calculated relative
 * to the correct window and the cell is drawn in that window.
 */
static void xbaeDrawCellString(XbaeMatrixWidget mw, Window win,
				   int x, int y, int width, int height,
				   unsigned char alignment,
				   Boolean bold, Boolean arrows, Boolean underline, Pixel fg, String string, XrmQuark qtag)
{
	xbaeSetDrawFont(mw, qtag, True);
	xbaeDrawString(mw, win, mw->matrix.draw_gc,
				   x, y, width, height,
				   alignment, bold, arrows, underline,
				   mw->matrix.multi_line_cell, mw->matrix.wrap_type,
				   fg, string, &mw->matrix.draw_font, CELL_FONT_HEIGHT(mw), CELL_BASELINE(mw));
}



static void DrawCellFill(XbaeMatrixWidget mw, Window win, int row, int column, int x, int y,
						 int column_width, int row_height,
						 int cell_width, int cell_height, int clear_width, int clear_height)
{
	Display *display = XtDisplay(mw);
	int space = mw->matrix.cell_shadow_thickness;

	if(clear_width)
	{
		if(IN_GRID_ROW_MODE(mw))
		{
			/* Don't erase the shadow */
			clear_width -= (column == mw->matrix.columns - 1) ? space : 0;
			XClearArea(display, win, x + column_width, y + space, clear_width, cell_height - 2 * space,
					   False);
		}
		else
		{
			XClearArea(display, win, x + column_width, y, clear_width, cell_height, False);
		}
	}
	if(clear_height)
	{
		if(IN_GRID_COLUMN_MODE(mw))
		{
			/* Don't erase the shadow */
			clear_height -= (row == mw->matrix.rows - 1) ? space : 0;
			XClearArea(display, win, x + space, y + row_height, cell_width - 2 * space, clear_height, False);
		}
		else
		{
			XClearArea(display, win, x, y + row_height, cell_width, clear_height, False);
		}
	}
}

void xbaeChangeHighlight(XbaeMatrixWidget mw, int row, int column, unsigned char new_hl)
{
	XbaeMatrixCellValuesStruct cell_values;

	unsigned char old_hl;

	int column_width, row_height;
	int cell_width, cell_height;
	int clear_width, clear_height;
	int new_hl_width, new_hl_height;
	int old_hl_width, old_hl_height;
	int x, y;
	Widget w = xbaeRowColToClipXY(mw, row, column, &x, &y);
	Window win = XtWindow(w);
	Display *display = XtDisplay(mw);

	assert(row >= 0 && row < mw->matrix.rows && column >= 0 && column < mw->matrix.columns);

	if(!win || mw->matrix.disable_redisplay)
		return;

	old_hl = mw->matrix.per_cell[row][column].highlighted;

	clear_width = 0;
	column_width = cell_width = new_hl_width = old_hl_width = COLUMN_WIDTH(mw, column);
	if(IS_FILL_COLUMN(mw, column))
	{
		int empty_width = EMPTY_WIDTH(mw);
		if(mw->matrix.horz_fill)
		{
			cell_width += empty_width;
			new_hl_width += empty_width;
			old_hl_width += empty_width;
		}
		else if(IN_GRID_ROW_MODE(mw))
		{
			clear_width = empty_width;
			new_hl_width += (new_hl & HighlightRow) ? empty_width : 0;
			old_hl_width += (old_hl & HighlightRow) ? empty_width : 0;
		}
	}

	clear_height = 0;
	row_height = cell_height = new_hl_height = old_hl_height = ROW_HEIGHT(mw, row);
	if(IS_FILL_ROW(mw, row))
	{
		int empty_height = EMPTY_HEIGHT(mw);
		if(mw->matrix.vert_fill)
		{
			cell_height += empty_height;
			new_hl_height += empty_height;
			old_hl_height += empty_height;
		}
		else if(IN_GRID_COLUMN_MODE(mw))
		{
			clear_height = empty_height;
			new_hl_height += (new_hl & HighlightColumn) ? empty_height : 0;
			old_hl_height += (old_hl & HighlightColumn) ? empty_height : 0;
		}
	}

	/*
	 * In order to erase the old highlight, we redraw it with the background color. 
	 * Then, if there is some filled space we clear it as there is no background color there
	 */
	xbaeGetCellValues(mw, row, column, False, &cell_values);

	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtFree((XtPointer) cell_values.drawCB.string);
	}

	XSetForeground(display, mw->matrix.draw_gc, cell_values.drawCB.background);
	xbaeDrawCellHighlight(mw, win, mw->matrix.draw_gc, row, column, x, y, old_hl_width, old_hl_height,
						  old_hl);
	DrawCellFill(mw, win, row, column, x, y, column_width, row_height, cell_width, cell_height, clear_width,
				 clear_height);

	/*
	 * Draw the highlight
	 */
	mw->matrix.per_cell[row][column].highlighted = new_hl;
	xbaeDrawCellHighlight(mw, win, mw->manager.highlight_GC, row, column, x, y, new_hl_width, new_hl_height,
						  new_hl);
}

void xbaeDrawCell(XbaeMatrixWidget mw, int row, int column)
{
	XbaeMatrixCellValuesStruct cell_values;

	unsigned char highlight = HighlightNone;
	unsigned char alignment = XmALIGNMENT_BEGINNING;
	Boolean arrows = False;
	Boolean bold = False;
	Boolean underline = False;

	int column_width, row_height;
	int cell_width, cell_height;
	int clear_width, clear_height;
	int shadow_width, shadow_height;
	int highlight_width, highlight_height;
	int x, y;
	Widget w = xbaeRowColToClipXY(mw, row, column, &x, &y);
	Window win = XtWindow(w);
	Display *display = XtDisplay(mw);

	assert(row >= 0 && row < mw->matrix.rows && column >= 0 && column < mw->matrix.columns);

	if(!win || mw->matrix.disable_redisplay)
		return;

	if(mw->matrix.column_alignments)
	{
		alignment = mw->matrix.column_alignments[column];
	}
	if(mw->matrix.show_arrows
	   && (mw->matrix.show_column_arrows == NULL || mw->matrix.show_column_arrows[column]))
	{
		arrows = True;
	}
	if(mw->matrix.column_font_bold)
	{
		bold = mw->matrix.column_font_bold[column];
	}
	if(mw->matrix.per_cell)
	{
		highlight = mw->matrix.per_cell[row][column].highlighted;
		underline = mw->matrix.per_cell[row][column].underlined;
	}

	clear_width = 0;
	column_width = cell_width = shadow_width = highlight_width = COLUMN_WIDTH(mw, column);
	if(IS_FILL_COLUMN(mw, column))
	{
		int empty_width = EMPTY_WIDTH(mw);
		if(mw->matrix.horz_fill)
		{
			cell_width += empty_width;
			shadow_width += empty_width;
			highlight_width += empty_width;
		}
		else if(IN_GRID_ROW_MODE(mw))
		{
			clear_width = empty_width;
			shadow_width += empty_width;
			highlight_width += (highlight & HighlightRow) ? empty_width : 0;
		}
	}

	clear_height = 0;
	row_height = cell_height = shadow_height = highlight_height = ROW_HEIGHT(mw, row);
	if(IS_FILL_ROW(mw, row))
	{
		int empty_height = EMPTY_HEIGHT(mw);
		if(mw->matrix.vert_fill)
		{
			cell_height += empty_height;
			shadow_height += empty_height;
			highlight_height += empty_height;
		}
		else if(IN_GRID_COLUMN_MODE(mw))
		{
			clear_height = empty_height;
			shadow_height += empty_height;
			highlight_height += (highlight & HighlightColumn) ? empty_height : 0;
		}
	}

	/*
	 * Get the cell contents and colors
	 */

	xbaeGetCellValues(mw, row, column, False, &cell_values);

	/*
	 * Fill the cell's background
	 */
	XSetForeground(display, mw->matrix.draw_gc, cell_values.drawCB.background);
	XFillRectangle(XtDisplay(mw), win, mw->matrix.draw_gc, x, y, cell_width, cell_height);

	if(mw->matrix.per_cell && mw->matrix.per_cell[row][column].widget)
	{
		/* There is nothing to do for cells with widgets */
	}
	else
	{
		if(cell_values.drawCB.type & XbaePixmap)
			xbaeDrawCellPixmap(mw, win, x, y, cell_width, cell_height,
							   alignment,
							   cell_values.drawCB.pixmap,
							   cell_values.drawCB.mask,
							   cell_values.drawCB.width,
							   cell_values.drawCB.height,
							   cell_values.drawCB.background,
							   cell_values.drawCB.foreground, cell_values.drawCB.depth);

		if(cell_values.drawCB.type & XbaeString)
			xbaeDrawCellString(mw, win,
							   x, y, cell_width, cell_height,
							   alignment, bold, arrows, underline,
							   cell_values.drawCB.foreground, cell_values.drawCB.string, cell_values.qtag);
	}

	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtFree((XtPointer) cell_values.drawCB.string);
	}

	/* 
	 * Draw the cell highlight and the cell shadow
	 */

	xbaeDrawCellShadow(mw, win, row, column, x, y, shadow_width, shadow_height);
	DrawCellFill(mw, win, row, column, x, y,
				 column_width, row_height, cell_width, cell_height, clear_width, clear_height);

	if(highlight)
	{
		xbaeDrawCellHighlight(mw, win, mw->manager.highlight_GC, row, column, x, y,
							  highlight_width, highlight_height, highlight);
	}
}

void
xbaeGetCellValues(XbaeMatrixWidget mw, int row, int column, Boolean textChild,
				  XbaeMatrixCellValuesStruct * cell_values)
{
	assert(row >= 0 && row < mw->matrix.rows && column >= 0 && column < mw->matrix.columns);

	cell_values->drawCB.reason = XbaeDrawCellReason;
	cell_values->drawCB.event = (XEvent *) NULL;
	cell_values->drawCB.row = row;
	cell_values->drawCB.column = column;
	cell_values->drawCB.width = COLUMN_WIDTH(mw, column) - 2 * CELL_BORDER_WIDTH(mw);
	cell_values->drawCB.height = ROW_HEIGHT(mw, row) - 2 * CELL_BORDER_HEIGHT(mw);
	cell_values->drawCB.depth = 0;

	/*
	 * If we have per_cell data initialize cell_values from it
	 */
	if(mw->matrix.per_cell)
	{
		cell_values->drawCB.foreground = mw->matrix.per_cell[row][column].color;
		cell_values->drawCB.background = mw->matrix.per_cell[row][column].background;
		cell_values->drawCB.pixmap = mw->matrix.per_cell[row][column].pixmap;
		cell_values->drawCB.mask = mw->matrix.per_cell[row][column].mask;
		cell_values->drawCB.string = mw->matrix.per_cell[row][column].CellValue;
		cell_values->drawCB.type =
			XbaeString | ((cell_values->drawCB.pixmap != XmUNSPECIFIED_PIXMAP) ? XbaePixmap : 0);
		cell_values->qtag = mw->matrix.per_cell[row][column].qtag;
	}
	else
	{
		cell_values->drawCB.foreground = XmUNSPECIFIED_PIXEL;
		cell_values->drawCB.background = XmUNSPECIFIED_PIXEL;
		cell_values->drawCB.pixmap = XmUNSPECIFIED_PIXMAP;
		cell_values->drawCB.mask = XmUNSPECIFIED_PIXMAP;
		cell_values->drawCB.string = NULL;
		cell_values->drawCB.type = XbaeString;
		cell_values->qtag = NULLQUARK;
	}

	/*
	 * Calculate the colors ignoring selection
	 */
	if(cell_values->drawCB.background == XmUNSPECIFIED_PIXEL)
	{
		if(mw->matrix.alt_row_count)
		{
			if((row / mw->matrix.alt_row_count) % 2)
			{
				cell_values->drawCB.background = mw->matrix.odd_row_background;
			}
			else
			{
				cell_values->drawCB.background = mw->matrix.even_row_background;
			}
		}
		else
		{
			cell_values->drawCB.background = mw->matrix.even_row_background;
		}
		if(cell_values->drawCB.background == XmUNSPECIFIED_PIXEL)
		{
			cell_values->drawCB.background = mw->core.background_pixel;
		}
	}

	if(cell_values->drawCB.foreground == XmUNSPECIFIED_PIXEL)
	{
		cell_values->drawCB.foreground = mw->manager.foreground;
	}

	/*
	 * Call the drawCellCB if there is one
	 */
	if(mw->matrix.draw_cell_callback)
	{
		XtCallCallbackList((Widget) mw, mw->matrix.draw_cell_callback, (XtPointer) & cell_values->drawCB);
	}

	/*
	 * Now adjust colors for selection or textChild
	 */
	if(textChild)
	{
		if(mw->matrix.text_background != XmUNSPECIFIED_PIXEL)
		{
			cell_values->drawCB.background = mw->matrix.text_background;
		}
		else if(!mw->matrix.text_background_is_cell)
		{
			cell_values->drawCB.background = mw->core.background_pixel;
		}
	}
	else
	{
		if(mw->matrix.per_cell && mw->matrix.per_cell[row][column].selected)
		{
			if(mw->matrix.reverse_select)
			{
				Pixel tmp = cell_values->drawCB.foreground;
				cell_values->drawCB.foreground = cell_values->drawCB.background;
				cell_values->drawCB.background = tmp;
			}
			else
			{
				cell_values->drawCB.background = (mw->matrix.selected_background != XmUNSPECIFIED_PIXEL)
					? mw->matrix.selected_background : mw->manager.foreground;
				cell_values->drawCB.foreground = (mw->matrix.selected_foreground != XmUNSPECIFIED_PIXEL)
					? mw->matrix.selected_foreground : mw->core.background_pixel;
			}
		}
	}

	/*
	 * Return "" instead of NULL for the string and make sure we don't free ""
	 */
	if(cell_values->drawCB.type & XbaeString && cell_values->drawCB.string == NULL)
	{
		cell_values->drawCB.string = "";
		cell_values->drawCB.type &= ~(XbaeStringFree ^ XbaeString);
	}

	/*
	 * Calculate pixmap parameters
	 */
	if(cell_values->drawCB.type & XbaePixmap)
	{
		if(cell_values->drawCB.mask == XmUNSPECIFIED_PIXMAP || cell_values->drawCB.mask == BadPixmap)
			cell_values->drawCB.mask = 0;

		if(cell_values->drawCB.pixmap == XmUNSPECIFIED_PIXMAP || cell_values->drawCB.pixmap == BadPixmap)
		{
			XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
							"drawCellCallback", "Pixmap", "XbaeMatrix",
							"XbaeMatrix: Bad pixmap passed from drawCellCallback", NULL, 0);
			cell_values->drawCB.type &= ~XbaePixmap;
		}
		else if(cell_values->drawCB.depth == 0)
		{
			/*
			 * If we know the depth, width and height don't do a round
			 * trip to find the geometry
			 */
			Window root_return;
			int x_return, y_return;
			unsigned int pixmap_width, pixmap_height;
			unsigned int border_width_return;
			unsigned int depth;

			if(XGetGeometry(XtDisplay(mw), cell_values->drawCB.pixmap, &root_return,
							&x_return, &y_return,
							&pixmap_width, &pixmap_height, &border_width_return, &depth))
			{
				cell_values->drawCB.width = pixmap_width;
				cell_values->drawCB.height = pixmap_height;
				cell_values->drawCB.depth = depth;
			}
		}
	}
}

/*
 * Draw the column label for the specified column.  
 */
void xbaeDrawColumnLabel(XbaeMatrixWidget mw, int column, Boolean pressed)
{
	int width, height;
	int x, y;
	Widget w = xbaeRowColToClipXY(mw, -1, column, &x, &y);
	Window win = XtWindow(w);
	GC gc = mw->matrix.label_gc;
	Boolean button;
	Boolean alignment;

	assert(column >= 0 && column < mw->matrix.columns);

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "%s(%d)\n", __FUNCTION__, column));

	height = COLUMN_LABEL_HEIGHT(mw);
	width = COLUMN_WIDTH(mw, column);
	if(IS_FILL_COLUMN(mw, column) && mw->matrix.horz_fill)
	{
		width += EMPTY_WIDTH(mw);
	}

	button = mw->matrix.button_labels || (mw->matrix.column_button_labels
										  && mw->matrix.column_button_labels[column]);

	alignment = (mw->matrix.column_label_alignments)
		? mw->matrix.column_label_alignments[column] : XmALIGNMENT_BEGINNING;

	if(mw->matrix.column_label_backgrounds)
	{
		XSetForeground(XtDisplay(mw), gc, mw->matrix.column_label_backgrounds[column]);
		XFillRectangle(XtDisplay(mw), win, gc, x, y, width, height);
	}
	else if(button)
	{
		XSetForeground(XtDisplay(mw), gc, mw->matrix.button_label_background);
		XFillRectangle(XtDisplay(mw), win, gc, x, y, width, height);
	}
	else
	{
		XClearArea(XtDisplay(mw), win, x, y, width, height, False);
	}

	if(mw->matrix.xmcolumn_labels && mw->matrix.xmcolumn_labels[column])
	{

		xbaeDrawXmString(mw, win, gc,
						 x, y, width, height,
						 alignment, mw->matrix.bold_labels, False, False,
						 (mw->matrix.column_label_foregrounds) ?
						 mw->matrix.column_label_foregrounds[column] : mw->matrix.column_label_color,
						 mw->matrix.xmcolumn_labels[column],
						 mw->matrix.render_table);

	}
	else if(mw->matrix.column_labels && mw->matrix.column_labels[column]
			&& mw->matrix.column_labels[column][0] != '\0')
	{

		xbaeDrawString(mw, win, gc,
					   x, y, width, height,
					   alignment, mw->matrix.bold_labels, False, False,
					   True, XbaeWrapNone,
					   (mw->matrix.column_label_foregrounds) ?
					   mw->matrix.column_label_foregrounds[column] : mw->matrix.column_label_color,
					   mw->matrix.column_labels[column],
					   &mw->matrix.label_font, LABEL_FONT_HEIGHT(mw), COLUMN_LABEL_BASELINE(mw));
	}

	if(button)
		xbaeDrawLabelShadow(mw, win, x, y, width, height, pressed);
}

/*
 * Draw the row label for the specified row.
 */
void xbaeDrawRowLabel(XbaeMatrixWidget mw, int row, Boolean pressed)
{
	int width, height;
	int x, y;
	Widget w = xbaeRowColToClipXY(mw, row, -1, &x, &y);
	Window win = XtWindow(w);
	GC gc = mw->matrix.label_gc;
	Boolean button;

	assert(row >= 0 && row < mw->matrix.rows);

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "%s(%d)\n", __FUNCTION__, row));

	width = ROW_LABEL_WIDTH(mw);
	height = ROW_HEIGHT(mw, row);
	if(IS_FILL_ROW(mw, row) && mw->matrix.vert_fill)
	{
		height += EMPTY_HEIGHT(mw);
	}

	button = mw->matrix.button_labels || (mw->matrix.row_button_labels && mw->matrix.row_button_labels[row]);

	if(mw->matrix.row_label_backgrounds)
	{
		XSetForeground(XtDisplay(mw), gc, mw->matrix.row_label_backgrounds[row]);
		XFillRectangle(XtDisplay(mw), win, gc, x, y, width, height);
	}
	else if(button)
	{
		XSetForeground(XtDisplay(mw), gc, mw->matrix.button_label_background);
		XFillRectangle(XtDisplay(mw), win, gc, x, y, width, height);
	}
	else
	{
		XClearArea(XtDisplay(mw), win, x, y, width, height, False);
	}

	if(mw->matrix.xmrow_labels && mw->matrix.xmrow_labels[row])
	{

		xbaeDrawXmString(mw, win, gc,
						 x, y, width, height,
						 mw->matrix.row_label_alignment, mw->matrix.bold_labels, False, False,
						 (mw->matrix.row_label_foregrounds) ?
						 mw->matrix.row_label_foregrounds[row] : mw->matrix.row_label_color,
						 mw->matrix.xmrow_labels[row],
						 mw->matrix.render_table);

	}
	else if(mw->matrix.row_labels && mw->matrix.row_labels[row] && mw->matrix.row_labels[row][0] != '\0')
	{

		xbaeDrawString(mw, win, gc,
					   x, y, width, height,
					   mw->matrix.row_label_alignment, mw->matrix.bold_labels, False, False,
					   True, XbaeWrapNone,
					   (mw->matrix.row_label_foregrounds) ?
					   mw->matrix.row_label_foregrounds[row] : mw->matrix.row_label_color,
					   mw->matrix.row_labels[row],
					   &mw->matrix.label_font, LABEL_FONT_HEIGHT(mw), ROW_LABEL_BASELINE(mw));
	}

	if(button)
		xbaeDrawLabelShadow(mw, win, x, y, width, height, pressed);
}
