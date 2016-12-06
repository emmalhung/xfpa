/********************************************************************************/
/*
 *     File:  xlib_output.c
 *
 *  Purpose:  To output rgb rasters to X servers.
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
/********************************************************************************/

#include <stdio.h>
#include <math.h>
#include "FpaXglP.h"

/* Set the size of the image to be handled by each call to the converter functions.
 * Note that the memory for conversion in the XImage structure is allocated for all
 * time, so we do not want this to be too big.
 */
#define IMAGE_WIDTH  256	
#define IMAGE_HEIGHT 128


static void convert_555(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	UNCHAR r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNSHORT *)obuf)[x] = ((r & 0xf8) << 7) | ((g & 0xf8) << 2) | (b >> 3);
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_555_br(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	UNCHAR r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNSHORT *)obuf)[x] = ((r & 0xf8) >> 1) | ((g & 0xc0) >> 6) |
										((g & 0x18) << 10) | ((b & 0xf8) << 5);
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_565(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	UNCHAR r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNSHORT *)obuf)[x] = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_565_br(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	UNCHAR r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNSHORT *)obuf)[x] = (r & 0xf8) | ((g & 0xe0) >> 5) |
											((g & 0x1c) << 11) | ((b & 0xf8) << 5);
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_888(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    y;
	size_t w;
	UNCHAR *obuf, *bptr;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	w    = width * RASTER_BPP;
	for (y = 0; y < height; y++)
	{
		(void) memcpy((void *)obuf, (void *)bptr, w);
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_888_br(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *ob, *bptr, *bp2;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		ob  = obuf;
		for (x = 0; x < width; x++)
		{
			ob[2] = bp2[0];
			ob[1] = bp2[1];
			ob[0] = bp2[2];
			ob  += RASTER_BPP;
			bp2 += RASTER_BPP;
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_0888(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	int  r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNINT *)obuf)[x] = (r << 16) | (g << 8) | b;
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_0888_br(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	int  r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNINT *)obuf)[x] = (b << 16) | (g << 8) | r;
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_8880(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	int  r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNINT *)obuf)[x] = (r << 24) | (g << 16) | (b << 8);
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


static void convert_8880_br(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y;
	UNCHAR *obuf, *bptr, *bp2;
	int  r, g, b;

	bptr = buf;
	obuf = (UNCHAR *)image->data;
	for (y = 0; y < height; y++)
	{
		bp2 = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			((UNINT *)obuf)[x] = (b << 24) | (g << 16) | (r << 8);
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}


/* This should work for any truecolor not handled by the above, but it will be slow.
 */
static void convert_truecolor_msb(XImage *image, int width, int height, UNCHAR *buf, int rowstride)
{
	int    x, y, shift;
	int    r, g, b, r_right, r_left, g_right, g_left, b_right, b_left;
	UNCHAR *obuf, *obptr, *bptr, *bp2;
	UNINT  pixel;

	r_right = 8 - WX->outinfo->red_prec;
	r_left  = WX->outinfo->red_shift;
	g_right = 8 - WX->outinfo->green_prec;
	g_left  = WX->outinfo->green_shift;
	b_right = 8 - WX->outinfo->blue_prec;
	b_left  = WX->outinfo->blue_shift;
	shift   = WX->outinfo->bpp - 1;

	bptr = buf;
	obuf = (UNCHAR *)image->data;

	for (y = 0; y < height; y++)
	{
		obptr = obuf;
		bp2   = bptr;
		for (x = 0; x < width; x++)
		{
			r = *bp2++;
			g = *bp2++;
			b = *bp2++;
			pixel = ((r >> r_right) << r_left) | ((g >> g_right) << g_left) | ((b >> b_right) << b_left);
			switch(shift)
			{
				case 3: *obptr++ = (pixel >> 24) & 0xff;
				case 2: *obptr++ = (pixel >> 16) & 0xff;
				case 1: *obptr++ = (pixel >>  8) & 0xff;
				case 0: *obptr++ = (pixel      ) & 0xff;
			}
		}
		bptr += rowstride;
		obuf += image->bytes_per_line;
	}
}



/* Count the number of set bits in the mask value
 */
static int get_prec_from_mask(UNLONG val)
{
	int retval = 0;
	int cur_bit = 0;

	while (cur_bit < (sizeof(UNLONG) * 8))
	{
		if ((val >> cur_bit) & 0x1)
		{
			retval++;
		}
		cur_bit++;
	}
	return retval;
}

/* Find the position of the first non-zero bit
 */
static int get_shift_from_mask(UNLONG val)
{
	int cur_bit = 0;

	while (cur_bit < (sizeof(UNLONG) * 8))
	{
		if ((val >> cur_bit) & 0x1)
		{
			return cur_bit;
		}
		cur_bit++;
	}
	return cur_bit;
}


/* Xlib Output Library initialization function. This must be called before the output
 * function to initialize parameters for the particular window.
 */
static LOGICAL output_init (void)
{
	int      depth, bpp;
	UNINT    red_mask, green_mask, blue_mask;
	LOGICAL  byterev, mask_rgb, mask_bgr, mask_16, mask_15;

	const STRING MyName = "Graphics Initialize";

	byterev = (MACHINE_ENDIAN == IMAGE_LITTLE_ENDIAN);

	if(WX->outinfo) return TRUE;

	if(WX->visual_info->class != TrueColor || WX->visual_info->depth < 16)
	{
		pr_error(MyName,"Display is not 16, 24 or 32 bit TrueColor. Image functions not supported.\n");
		return FALSE;
	}

	WX->outinfo = ONEMEM(XOUTPUTINFO); 

   	WX->outinfo->red_shift   = get_shift_from_mask (WX->visual_info->red_mask);
   	WX->outinfo->red_prec    = get_prec_from_mask  (WX->visual_info->red_mask);
   	WX->outinfo->green_shift = get_shift_from_mask (WX->visual_info->green_mask);
   	WX->outinfo->green_prec  = get_prec_from_mask  (WX->visual_info->green_mask);
   	WX->outinfo->blue_shift  = get_shift_from_mask (WX->visual_info->blue_mask);
   	WX->outinfo->blue_prec   = get_prec_from_mask  (WX->visual_info->blue_mask);
	WX->outinfo->image       = XCreateImage(WX->display,
			 						WX->visual_info->visual, (UNINT)WX->visual_info->depth, ZPixmap,
			 						0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, 32, 0);

	switch (WX->outinfo->image->bits_per_pixel)
	{
		case  8: WX->outinfo->bpp = 1; break;
		case 16: WX->outinfo->bpp = 2; break;
		case 24: WX->outinfo->bpp = 3; break;
		case 32: WX->outinfo->bpp = 4; break;
		default: WX->outinfo->bpp = 4;
	}
	WX->outinfo->image->data             = MEM(char, IMAGE_WIDTH*IMAGE_HEIGHT*WX->outinfo->bpp);
	WX->outinfo->image->bitmap_bit_order = MSBFirst;
	WX->outinfo->image->byte_order       = MSBFirst;

	/* Select a conversion function based on the visual information. Note that the
	 * function convert_truecolor_msb is generic and will work for all visuals, but
	 * it is slow and is only used if all else fails.
	 */
	red_mask   = WX->visual_info->red_mask;
	green_mask = WX->visual_info->green_mask;
	blue_mask  = WX->visual_info->blue_mask;

	mask_rgb = ( red_mask == 0xff0000 && green_mask == 0xff00 && blue_mask == 0xff     );
	mask_bgr = ( red_mask == 0xff     && green_mask == 0xff00 && blue_mask == 0xff0000 );
	mask_16  = ( red_mask == 0xf800   && green_mask == 0x7e0  && blue_mask == 0x1f     );
	mask_15  = ( red_mask == 0x7c00   && green_mask == 0x3e0  && blue_mask == 0x1f     );

	depth = WX->visual_info->depth;
	bpp   = WX->outinfo->image->bits_per_pixel;

	if (!byterev && bpp == 16 && depth == 15 && mask_15)
	{
		WX->outinfo->conv = convert_555;
	}
	else if (byterev && bpp == 16 && depth == 15 && mask_15)
	{
		WX->outinfo->conv = convert_555_br;
	}
	else if (!byterev && bpp == 16 && depth == 16 && mask_16)
	{
		WX->outinfo->conv = convert_565;
	}
	else if (byterev && bpp == 16 && depth == 16 && mask_16)
	{
		WX->outinfo->conv = convert_565_br;
	}
	else if (bpp == 24 && depth == 24 && mask_bgr)
	{
		WX->outinfo->conv = convert_888_br;
	}
	else if (bpp == 24 && depth == 24 && mask_rgb)
	{
		WX->outinfo->conv = convert_888;
	}
	else if (!byterev && bpp == 32 && depth == 24 && mask_rgb)
	{
		WX->outinfo->conv = convert_0888;
	}
	else if (!byterev && bpp == 32 && depth == 24 && mask_bgr)
	{
		WX->outinfo->conv = convert_0888_br;
	}
	else if (byterev && bpp == 32 && depth == 24 && mask_rgb)
	{
		WX->outinfo->conv = convert_8880_br;
	}
	else if (byterev && bpp == 32 && depth == 24 && mask_bgr)
	{
		WX->outinfo->conv = convert_8880;
	}
	else
	{
		WX->outinfo->conv = convert_truecolor_msb;
	}

	return TRUE;
}



/*
 * This function requires rasters to be in pixel major format ( rgbrgbrgb... )
 */
void _xgl_image_xlib_output( ImagePtr im )
{
	int    x, y, xbgn, xlim, start, status, start_status;
  	int    ay, ax, width, height, rowstride;
  	UNCHAR *buf, *p;

	if (!im->raster   ) return;
	if (!output_init()) return;

	/* Create the image rendering cip mask. This is a pixmap of depth 1
	 * where a value of 0 will mask out the pixel. We default to masked
	 * out (transparent) and then draw when there are opaque pixels. This
	 * is a good compromise as most radar is mostly transparent and 
	 * satellite images will normally have one XDrawLine call per image
	 * scan line.
	 */
	XSetForeground(D, WX->dep1gc, 0);
	XFillRectangle(D, WX->mask, WX->dep1gc, 0, 0, (UNINT)W->xm, (UNINT)W->ym);
	XSetForeground(D, WX->dep1gc, 1);

	p    = im->raster;
	xbgn = im->dx + 1;		/* use variables to reduce loop calculations */
	xlim = im->dx + im->dw;

	for (y = im->dy; y < im->dy+im->dh; y++)
	{
		start = im->dx;
		start_status = OPAQUE_PIXEL(p);
		p += RASTER_BPP;
		for (x = xbgn; x < xlim; x++, p+=RASTER_BPP)
		{
			status = OPAQUE_PIXEL(p);
			if(status != start_status)
			{
				if (start_status)
					XDrawLine (D, WX->mask, WX->dep1gc, start, y, x-1, y);
				start = x;
				start_status = status;
			}
		}
		if (start_status)
			XDrawLine (D, WX->mask, WX->dep1gc, start, y, x-1, y);
	}

	XSetClipMask(D, WX->miscgc, WX->mask);
	XSetClipOrigin(D, WX->miscgc, 0, 0);

	/* Output the image itself using XImage functions.
	 */
	rowstride = im->dw * RASTER_BPP;
	for (ay = 0; ay < im->dh; ay += IMAGE_HEIGHT)
	{
		height = MIN(im->dh - ay, IMAGE_HEIGHT);
		for (ax = 0; ax < im->dw; ax += IMAGE_WIDTH)
		{
			width = MIN(im->dw - ax, IMAGE_WIDTH);
			buf   = im->raster + ay * rowstride + ax * RASTER_BPP;
			WX->outinfo->conv(WX->outinfo->image, width, height, buf, rowstride);
			XPutImage(WX->display, WX->draw, WX->miscgc, WX->outinfo->image,
				0, 0, im->dx + ax, im->dy + ay, (UNINT)width, (UNINT)height);
		}
	}

	XSetClipMask(D, WX->miscgc, None);
}
