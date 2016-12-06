/*******************************************************************************/
/*
*  File: windowToFile
*
*  Functions:
*
*   glWindowToFile()
*
*     Output the active window to a file. The format of the file is taken from
*     the type parameter.  The appearance of the output: colour, grayscale or
*     dithered black and white is determined by the fmt parameter.
*
*  glFileToWindow()
*
*     Output the named file to a window. There is no processing done (size,
*     translation, rotation, etc) and the image is output directly pixel by
*     pixel. If requested the function will centre the image in the window.
*
*  Notes:
*
*      The reduction to 256 colour function needs work to compact the colour
*      table if the number of colours exceeds 256.
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
/*******************************************************************************/

#include <unistd.h>
#ifdef MACHINE_PCLINUX
#include <png.h>
#include <tiffio.h>
#endif
#include <X11/XWDFile.h>
#include "FpaXglP.h"

/* Structure to use for colour conversions
*/
typedef struct {
	ColorIndex from_index;
	Pixel      from_pixel;
	Pixel      to_pixel;
} CTS;

/* This is the permanent colour conversion table.
*/
static int table_len = 0;
static CTS *table    = NULL;

/* Temporary colour conversion table.
*/
static size_t xt_len = 0;
static size_t xt_max = 0;
static CTS    *xt    = NULL;

#define clean_tables() {FREEMEM(xt);xt_len=xt_max=0;}

/* global variables
 */
static Visual  *visual   = NULL;
static LOGICAL no8visual = FALSE;

/* For error reports
 */
static String Module = "glWindowToFile";


/* Forward function declarations
*/
static void image_to_grayscale (XImage*, XColor*, int*);
static void image_to_bw        (XImage*, XColor*, int*, int);
static void image_to_depth8    (XImage*, XColor*, int*);
static void write_gif_file     (char*, XImage*, XColor*, int);
static void write_xwd_file     (char*, XImage*, XColor*, int);
static void write_xgl_file     (char*, XImage*, XColor*, int);
static void write_png_file     (char*, XImage*, XColor*, int);
static void write_tiff_file    (char*, XImage*, XColor*, int);




void glWindowToFile(String fname, int type, int fmt )
{
	int      ncolor = 0;
	XColor   color[256];
	XImage   *xi;

	/* These may be changed in the create_depth8_ximage function.
	 */
	visual    = WX->visual_info->visual;
	no8visual = FALSE;

	xi = XGetImage(D, WX->draw, 0, 0, (UNINT)W->xm,  (UNINT)W->ym, AllPlanes, ZPixmap);
	if(!xi)
	{
		pr_error(Module, "Can't allocate memory for XImage.\n", NULL);
		return;
	}

	/* If depth 8 get the colormap.
	*/
	if(xi->depth == 8)
		XQueryColors(D, WX->cmap, color, (ncolor = 256));

	switch(type)
	{
		case glGIF:
			switch(fmt)
			{
				case glGREYSCALE: image_to_grayscale (xi, color, &ncolor);    break;
				case glBW:        image_to_bw        (xi, color, &ncolor, 0); break;
				case glWB:        image_to_bw        (xi, color, &ncolor, 1); break;
				default:          image_to_depth8    (xi, color, &ncolor);    break;
			}
			write_gif_file(fname, xi, color, ncolor);
			break;

		case glXWD:
			switch(fmt)
			{
				case glGREYSCALE: image_to_grayscale (xi, color, &ncolor);    break;
				case glBW:        image_to_bw        (xi, color, &ncolor, 0); break;
				case glWB:        image_to_bw        (xi, color, &ncolor, 1); break;
			}
			write_xwd_file(fname, xi, color, ncolor);
			break;

		case glNATIVE:
		case glXGL:
			switch(fmt)
			{
				case glGREYSCALE: image_to_grayscale (xi, color, &ncolor);    break;
				case glBW:        image_to_bw        (xi, color, &ncolor, 0); break;
				case glWB:        image_to_bw        (xi, color, &ncolor, 1); break;
			}
			write_xgl_file(fname, xi, color, ncolor);
			break;

		case glPNG:
			switch(fmt)
			{
				case glGREYSCALE: image_to_grayscale (xi, color, &ncolor);    break;
				case glBW:        image_to_bw        (xi, color, &ncolor, 0); break;
				case glWB:        image_to_bw        (xi, color, &ncolor, 1); break;
			}
			write_png_file(fname, xi, color, ncolor);
			break;

		case glTIFF:
			switch(fmt)
			{
				case glGREYSCALE: image_to_grayscale (xi, color, &ncolor);    break;
				case glBW:        image_to_bw        (xi, color, &ncolor, 0); break;
				case glWB:        image_to_bw        (xi, color, &ncolor, 1); break;
			}
			write_tiff_file(fname, xi, color, ncolor);
			break;
	}
	FREE_XIMAGE(xi);
}


/*======================= Image Conversion Functions =======================*/


/* Colour name to grayscale conversion. The to  array is the grayscale ramp value
*  and must range from 0 to 255 entered as type Pixel.
*/
void glSetGrayscaleNameConversion(String *from, Pixel *to, int tlen)
{
	int i,j,k;
	ColorIndex idx;

	FREEMEM(table);
	table_len = 0;

	if(tlen < 1 || from == NULL || to == NULL) return;

	table = INITMEM(CTS, tlen);
	for(i = 0, k = 0; i < table_len; i++)
	{
		idx = _xgl_color_index_from_name("glSetGrayscaleNameConversion", from[i], FALSE);
		for( j = 0; j < k; j++ ) if( idx == table[j].from_index ) break;
		if( j >= k )
		{
			table[k].from_index = idx;
			table[k].to_pixel = to[i];
			k++;
		}
	}
	table_len = k;
}


/* Color index to grayscale conversion. The to  array is the grayscale ramp value
*  and must range from 0 to 255 entered as type Pixel.
*/
void glSetGrayscaleColorIndexConversion(ColorIndex *from, Pixel *to, int tlen)
{
	int i,j,k;

	FREEMEM(table);
	table_len = 0;

	if(tlen < 1 || from == NULL || to == NULL) return;

	table = INITMEM(CTS, tlen);

	for(i = 0, k = 0; i < tlen; i++)
	{
		for( j = 0; j < k; j++ ) if( from[i] == table[j].from_index ) break;
		if( j >= k )
		{
			table[k].from_index = from[i];
			table[k].to_pixel = to[i];
			k++;
		}
	}
	table_len = k;
}



/******************** PRIVATE FUNCTIONS **************************/



/* Function to use in sorting items in CTS arrays.
*/
static int ct_cmp( const void *a, const void *b )
{
	Pixel p1 = ((CTS *)a)->from_pixel;
	Pixel p2 = ((CTS *)b)->from_pixel;
	if( p1 > p2 ) return 1;
	if( p1 < p2 ) return -1;
	return 0;
}


/* Update the from pixel values from the from ColorIndex values
*  for the active window.
*/
static void update_tables(void)
{
	int i;

	xt_len = xt_max = (size_t) table_len;
	if(xt_max < 1) xt_max = 100;
	xt = INITMEM(CTS, xt_max);

	for(i = 0; i < table_len; i++)
	{
		table[i].from_pixel = _xgl_pixel_from_color_index(NULL, table[i].from_index);
		xt[i].from_pixel    = table[i].from_pixel;
		xt[i].to_pixel      = table[i].to_pixel;
	}
	qsort((void *)xt, xt_len, sizeof(CTS), ct_cmp);
}


/* This procedure goes through hoops because the XQueryColor function is slow
 * and doing a binary search on our own internal table is much faster. This
 * would be bad for photo type images, but for most weather images that have
 * a limited number of colours it is ok. The resulting pixel has a range of
 * 0 to 255.
 */
static Pixel convert_pixel_to_grayscale(Pixel pix)
{
	int r, g, b;
	XColor xc;
	CTS    *ct, pt;

	pt.from_pixel = pix;
	ct = (CTS *)bsearch((void *)&pt, (void *)xt, xt_len, sizeof(CTS), ct_cmp);
	if (ct) return ct->to_pixel;
	
	xc.pixel = pt.from_pixel;
	xc.red   = 0;
	xc.green = 0;
	xc.blue  = 0;
	
	if(XQueryColor(D, WX->cmap, &xc) == BadValue)
	{
		pt.to_pixel = 0;
	}
	else if( xc.red == xc.green && xc.red == xc.blue )
	{
		pt.to_pixel = (Pixel)(xc.red >> 8);
	}
	else
	{
		r = xc.red   >> 8;
		g = xc.green >> 8;
		b = xc.blue  >> 8;
		pt.to_pixel = (Pixel)(CONVERT_TO_GRAY_PIXEL(r,g,b));
	}

	/* Put our new pixel in storted order.
	*/
	if(xt_len >= xt_max) xt = GETMEM(xt, CTS, xt_max += 100);
	xt[xt_len].from_pixel = pt.from_pixel;
	xt[xt_len].to_pixel   = pt.to_pixel;
	xt_len++;
	qsort((void *)xt, xt_len, sizeof(CTS), ct_cmp);
	
	return pt.to_pixel;
}


/* Create an XImage structure of depth 8 and allocate a data array.
 */
static XImage *create_depth8_ximage(XImage *xi)
{
	int         n;
	XImage      *ix;
	XVisualInfo *vis_info, template;

	no8visual = TRUE;
	template.depth = 8;
	template.class = PseudoColor;
	vis_info = XGetVisualInfo(D, VisualDepthMask|VisualClassMask, &template, &n);
	if(vis_info)
	{
		no8visual = FALSE;
		visual    = vis_info[0].visual;
		XFree(vis_info);
	}

	ix = XCreateImage(D, visual, 8, ZPixmap, 0, NULL, (UNINT)xi->width, (UNINT)xi->height, 32, 0);
	ix->data = INITMEM(char, ix->bytes_per_line * ix->height);

	return ix;
}



/* Convert the image to grayscale as defined by the conversion table.
*  Any colours not in the table default to black.
*/
static void image_to_grayscale(XImage *XI, XColor *color, int *ncolor)
{
	int    x, y, n;
	UNCHAR *buf, *b;
	XImage *xi;
	Pixel  px, gp = 0;
	Pixel  last = ~((Pixel)0);

	update_tables();

	/*  Create the gray scale colormap.
	*/
	for( n = 0; n < 256; n++ )
	{
		color[n].pixel = (Pixel)n;
		color[n].red   = (unsigned short)(n << 8);
		color[n].green = (unsigned short)(n << 8);
		color[n].blue  = (unsigned short)(n << 8);
		color[n].flags = DoRGB;
	}
	*ncolor = 256;

	xi = create_depth8_ximage(XI);

	/* Run through the raster changing the pixels into the colormap
	*  entry numbers as we just created above.
	*/
	buf = (UNCHAR *)xi->data;
	for(y = 0; y < XI->height; y++)
	{
		b = buf;
		for(x = 0; x < XI->width; x++)
		{
			px = XGetPixel(XI, x, y);
			if(px != last)
			{
				last = px;
				gp = convert_pixel_to_grayscale(px);
			}
			*b++ = (UNCHAR)gp;
		}
		buf += xi->bytes_per_line;
	}
	clean_tables();
	FREEMEM(XI->data);
	(void) memcpy((void*)XI, (void*)xi, sizeof(XImage));
	xi->data = NULL;
	FREE_XIMAGE(xi);
}


/* Here we first apply the grayscale conversion according to the conversion table
*  then use Floyd dithering to create the final black and white image. If reverse
*  is TRUE then the order of the 0's and 1's are reversed.
*/
static void image_to_bw(XImage *XI, XColor *color, int *ncolor, int reverse)
{
	int    x, y, *errvec;
	UNCHAR *buf, *b, p0, p1;
	XImage *xi;
	Pixel  px, last = ~((Pixel)0);
	
	update_tables();

	xi = create_depth8_ximage(XI);

	/* Create a two entry colormap.
	*/
	color[0].pixel = 0;
	color[0].red   = 0;
	color[0].green = 0;
	color[0].blue  = 0;
	color[0].flags = DoRGB;

	color[1].pixel = 1;
	color[1].red   = 65535;
	color[1].green = 65535;
	color[1].blue  = 65535;
	color[1].flags = DoRGB;

	*ncolor = 2;

	errvec = INITMEM(int, XI->width+1);

	p0 = (UNCHAR)((reverse)? 1:0);
	p1 = (UNCHAR)((reverse)? 0:1);

	/* Do the colour replacement pixel-by-pixel
	*/
	buf = (UNCHAR *)xi->data;
	for(y = 0; y < XI->height; y++)
	{
		int		inval, error, error4, error16;
		long	diag, down, maxval, maxval2;

		error4  = 0;
		error16 = 0;
		diag    = 0;
		down    = 0;
		maxval  = 255;
		maxval2 = 127;

		b = buf;
		for(x = 0; x < XI->width; x++)
		{
			px = XGetPixel(XI, x, y);
			if(px != last)
			{
				last  = px;
				inval = (int)convert_pixel_to_grayscale(px);
			}
			error = inval + errvec[x+1] + error4 + error4 - error16;
			if (error >= (int) maxval2)
			{
				*b++ = p1;
				error -= maxval;
			}
			else
			{
				*b++ = p0;
			}
			error4    = error/4;
			error16   = error/16;
			errvec[x] = (int)(down + error4 - error16);
			down      = diag + error4 + error16;
			diag      = error16;
		}
		errvec[XI->width] = (int) down;
		buf += xi->bytes_per_line;
	}
	FREEMEM(errvec);
	clean_tables();
	FREEMEM(XI->data);
	(void) memcpy((void*)XI, (void*)xi, sizeof(XImage));
	xi->data = NULL;
	FREE_XIMAGE(xi);
}

/* Cross reference structure for the internal 8 bit colormap.
*/
static size_t ncxmax = 0;
static size_t ncxref = 0;
static CTS    *cxref = NULL;

static void add_color_256( XColor in, XColor *color, int *ncolor )
{
	int i, n, diff, dmin, rd, gd, bd;
	
	/* cxref is the colour cross reference table. This cross references pixel values
	*  in an image to a colour in our local colour table. If there are more than 256
	*  colours in the original image then more then one image pixel will point to the
	*  same local colormap entry.
	*/
	if(*ncolor < 256)
	{
		n = *ncolor;
		*ncolor = n+1;
		color[n].pixel = (Pixel) n;
		color[n].red   = in.red;
		color[n].green = in.green;
		color[n].blue  = in.blue;
		color[n].flags = DoRGB;		
	}
	else
	{
		/* Find closest match and compact table.
		*/
		n = 0;
		dmin = 256*256 * RASTER_BPP;
		for(i = 0; i < 256; i++)
		{
			rd = (int)(in.red   - color[i].red  ) >> 8;
			gd = (int)(in.green - color[i].green) >> 8;
			bd = (int)(in.blue  - color[i].blue ) >> 8;
			diff = rd*rd + gd*gd + bd*bd;
			if(diff < dmin)
			{
				dmin = diff;
				n = i;
			}
		}
	}

	if(ncxref >= ncxmax) cxref = GETMEM(cxref, CTS, ncxmax += 256);
	cxref[ncxref].from_pixel = in.pixel;
	cxref[ncxref].to_pixel   = (Pixel) n;
	ncxref++;
	qsort((void *)cxref, ncxref, sizeof(CTS), ct_cmp);
}


/* If the image depth is > 8 we need to convert the image to depth
*  8. We do this by replacing all pixel references with ColorIndex 
*  values which index into a local colormap which holds no more than
*  256 values.
*/
static void image_to_depth8(XImage *XI, XColor *color, int *ncolor)
{
	int    x, y, n;
	UNCHAR *buf, *b;
	XImage *xi;
	Pixel  last = ~((Pixel)0);
	CTS    *ct, pt;

	xi = create_depth8_ximage(XI);

	pt.from_pixel = 0;
	*ncolor = 0;

	for(n = 0; n < WX->ncolors; n++)
	{
		if(WX->colors[n].flags != 0) add_color_256(WX->colors[n], color, ncolor);
	}

	buf = (UNCHAR *)xi->data;
	for(y = 0; y < XI->height; y++)
	{
		b = buf;
		for(x = 0; x < XI->width; x++)
		{
			pt.from_pixel = XGetPixel(XI, x, y);
			if(pt.from_pixel != last)
			{
				last = pt.from_pixel;
				ct = (CTS *)bsearch((void *)&pt, (void *)cxref, ncxref, sizeof(CTS), ct_cmp);
				if(!ct)
				{
					XColor xc;
					xc.pixel = pt.from_pixel;
					XQueryColor(D, WX->cmap, &xc);
					add_color_256(xc, color, ncolor);
					ct = (CTS *)bsearch((void *)&pt, (void *)cxref, ncxref, sizeof(CTS), ct_cmp);
				}
			}
			*b++ = (UNCHAR)ct->to_pixel;
		}
		buf += xi->bytes_per_line;
	}
	FREEMEM(cxref);
	ncxref = 0;
	ncxmax = 0;
	FREEMEM(XI->data);
	(void) memcpy((void*)XI, (void*)xi, sizeof(XImage));
	xi->data = NULL;
	FREE_XIMAGE(xi);
}


/* We only need one error message location by having this function.
 */
static FILE *open_file(char *fname)
{
	FILE *fp = fopen(fname, "wb");
	if(!fp) pr_error(Module, "Unable to open file \"%s\" for writing.\n", fname);
	return fp;
}


static void swapshort(register char *bp, register size_t n)
{
    register char c;
    register char *ep = bp + n;

    while (bp < ep)
	{
		c = *bp;
		*bp = *(bp + 1);
		bp++;
		*bp++ = c;
    }
}


static void swaplong(register char *bp, register size_t n)
{
    register char c;
    register char *ep = bp + n;
    register char *sp;

    while (bp < ep)
	{
		sp = bp + 3;
		c = *sp;
		*sp = *bp;
		*bp++ = c;
		sp = bp + 1;
		c = *sp;
		*sp = *bp;
		*bp++ = c;
		bp += 2;
    }
}


/* 
 *  The function write_msb_int() is used here to write out the header information
 *  rather than just writing out the structure directly is based on the need
 *  to cater to LSBFirst and MSBFirst machines. As well some system store the
 *  elements of structures in a different order than one would expect (back to
 *  front for example). Using the write function overcomes this problem.
 */
static void write_xwd_file( char *fname, XImage *xi, XColor *color, int ncolor )
{
	unsigned long swaptest = 1;
	int           n;
	XWDFileHeader xwd;
	XWDColor      xwdcolor;
	FILE          *fp;


	fp = open_file(fname);
	if(!fp) return;


	/* The visual overrides below are in case the system does not have an
	 * 8 bit pseudo color visual.
	 */
	xwd.header_size      = sizeof(xwd);
	xwd.file_version     = XWD_FILE_VERSION;
	xwd.pixmap_format    = (CARD32) xi->format;
	xwd.pixmap_depth     = (CARD32) xi->depth;
	xwd.pixmap_width     = (CARD32) xi->width;
	xwd.pixmap_height    = (CARD32) xi->height;
	xwd.xoffset          = (CARD32) xi->xoffset;
	xwd.byte_order       = (CARD32) xi->byte_order;
	xwd.bitmap_unit      = (CARD32) xi->bitmap_unit;
	xwd.bitmap_bit_order = (CARD32) xi->bitmap_bit_order;
	xwd.bitmap_pad       = (CARD32) xi->bitmap_pad;
	xwd.bits_per_pixel   = (CARD32) xi->bits_per_pixel;
	xwd.bytes_per_line   = (CARD32) xi->bytes_per_line;
	xwd.visual_class     = (CARD32) (no8visual? PseudoColor : visual->class);
    xwd.red_mask         = no8visual? 0 : visual->red_mask;
    xwd.green_mask       = no8visual? 0 : visual->green_mask;
    xwd.blue_mask        = no8visual? 0 : visual->blue_mask;
    xwd.bits_per_rgb     = (CARD32) visual->bits_per_rgb;
    xwd.colormap_entries = (CARD32) (no8visual? 256 : visual->map_entries);
	xwd.ncolors          = (CARD32) ncolor;
	xwd.window_width     = (CARD32) xi->width;
	xwd.window_height    = (CARD32) xi->height;
	xwd.window_x         = 0;
	xwd.window_y         = 0;
	xwd.window_bdrwidth  = 0;

	/* The standard is for the header to be in MSBFirst order.
	 */
	if(*((char *)&swaptest))
	{
		swaplong((char *)&xwd, sizeof(XWDFileHeader));
		for (n = 0; n < ncolor; n++)
		{
			swaplong((char *)&color[n].pixel, sizeof(long));
			swapshort((char *)&color[n].red,  sizeof(short) * 3);
		}
    }
    (void) fwrite((char *)&xwd, sizeof(XWDFileHeader), 1, fp);

    /* Write out the color maps, if any
     */
    for (n = 0; n < ncolor; n++)
	{
		xwdcolor.pixel = color[n].pixel;
		xwdcolor.red   = color[n].red;
		xwdcolor.green = color[n].green;
		xwdcolor.blue  = color[n].blue;
		xwdcolor.flags = (CARD8) color[n].flags;
		(void) fwrite((void *)&xwdcolor, sizeof(XWDColor), 1, fp);
    }

	(void) fwrite(xi->data, (size_t)(xi->bytes_per_line*xi->height), 1, fp);
	(void) fclose(fp);
}


static void write_gif_file(char *fname, XImage *xi, XColor *color, int ncolor)
{
	int  n, y;
	UNCHAR *buf, *p, *xib, r[256], g[256], b[256];

	for( n = 0; n < ncolor; n++ )
	{
		r[n] = (UNCHAR) (color[n].red   >> 8);
		g[n] = (UNCHAR) (color[n].green >> 8);
		b[n] = (UNCHAR) (color[n].blue  >> 8);
	}
	buf = INITMEM(UNCHAR, xi->width*xi->height);
	xib = (UNCHAR *)xi->data;
	for(p = buf, y = 0; y < xi->height; y++)
	{
		(void) memcpy(p, xib, (size_t)xi->width);
		p += xi->width;
		xib += xi->bytes_per_line;
	}
	_xgl_writeGIF(fname, xi->width, xi->height, buf, r, g, b, ncolor, NULL);
	FREEMEM(buf);
}


static int pixel_cmp( const void *a, const void *b)
{
	Pixel p1 = ((XColor *)a)->pixel;
	Pixel p2 = ((XColor *)b)->pixel;
	if( p1 > p2 ) return  1;
	if( p1 < p2 ) return -1;
	return 0;
}

static void get_rgb_raster(XImage *xi, XColor *color, int ncolor, UNCHAR **raster, int *bpp)
{
	int     n, x, y, size;
	UNCHAR  r, g, b, *p, *buf;
	XColor  *cp, xc;
	Pixel   last_pixel = ~0;

	*raster = NULL;

	*bpp = (ncolor > 0)? 1:3;
	for(n = 0; n < ncolor; n++)
	{
		if(color[n].red != color[n].green || color[n].red != color[n].blue)
		{
			*bpp = 3;
			break;
		}
	}

	size = xi->width * xi->height * (*bpp);
	buf = p = INITMEM(UNCHAR, size);
	if (!buf) return;

	if(*bpp == 1)
	{
		for(y = 0; y < xi->height; y++)
		{
			for(x = 0; x < xi->width; x++)
			{
				xc.pixel = XGetPixel(xi,x,y);
				if(xc.pixel < (Pixel) ncolor)
					*p++ = (UNCHAR)(color[xc.pixel].red >> 8);
			}
		}
	}
	else if(ncolor > 0)
	{
		for(y = 0; y < xi->height; y++)
		{
			for(x = 0; x < xi->width; x++)
			{
				xc.pixel = XGetPixel(xi,x,y);
				if( xc.pixel != last_pixel )
				{
					if(xc.pixel < (Pixel) ncolor)
					{
						r = (UNCHAR)(color[xc.pixel].red   >> 8);
						g = (UNCHAR)(color[xc.pixel].green >> 8);
						b = (UNCHAR)(color[xc.pixel].blue  >> 8);
					}
					else
					{
						r = g = b = 0;
					}
					last_pixel = xc.pixel;
				}
				*p++ = r;
				*p++ = g;
				*p++ = b;
			}
		}
	}
	else
	{
		size_t nxcl       = 0;
		size_t max_xcl    = 256;
		XColor *xcl       = NULL;

		/* First thing we need to turn the pixel values into an RGB array. The
		 * procedure followed here may seem over complicated, but the XQueryColor()
		 * function is slow and putting things into a XColor array speeds things up
		 * considerably.
		 */
		xcl = GETMEM(xcl, XColor, max_xcl);
		if (!xcl)
		{
			FREEMEM(buf);
			return;
		}
		(void) memset((void *)&xc, 0, sizeof(XColor));
		xc.flags = DoRed | DoGreen | DoBlue;
		(void) XQueryColor(D, WX->cmap, &xc);

		for(y = 0; y < xi->height; y++)
		{
			for(x = 0; x < xi->width; x++)
			{
				xc.pixel = XGetPixel(xi,x,y);
				if( xc.pixel != last_pixel )
				{
					cp = (XColor *)bsearch((void *)&xc, (void *)xcl, nxcl, sizeof(XColor), pixel_cmp);
					if(!cp)
					{
						cp = &xc;
						if(ncolor > 0)
						{
							for( n = 0; n < ncolor; n++ )
								if(xc.pixel == color[n].pixel) { cp = &color[n]; break; }
							if( n >= ncolor )
								(void) XQueryColor(D, WX->cmap, cp);
						}
						else
						{
							(void) XQueryColor(D, WX->cmap, cp);
						}
						/* Insert in sorted order
						*/
						if( nxcl >= max_xcl) xcl = GETMEM(xcl, XColor, max_xcl += 256);
						(void) memcpy((void *)&xcl[nxcl], cp, sizeof(XColor));
						nxcl++;
						qsort((void *)xcl, nxcl, sizeof(XColor), pixel_cmp);
					}
					r = (UNCHAR)(cp->red   >> 8);
					g = (UNCHAR)(cp->green >> 8);
					b = (UNCHAR)(cp->blue  >> 8);
					last_pixel = cp->pixel;
				}
				*p++ = r;
				*p++ = g;
				*p++ = b;
			}
		}
		FREEMEM(xcl);
	}

	*raster = buf;
}


static void write_xgl_file(char *fname, XImage *xi, XColor *color, int ncolor)
{
	int         bpp;
	size_t      size;
	char        type, hbuf[25];
	UNCHAR      *buf;
	FILE        *fp;

	fp = open_file(fname);
	if(!fp) return;

	get_rgb_raster(xi, color, ncolor, &buf, &bpp);
	if (buf)
	{
		if(bpp == 1)
			type = 'G';
		else
			type = 'R';

		size = (size_t) (xi->width * xi->height * bpp);
		(void) snprintf(hbuf, sizeof(hbuf), "xglraster%c%dx%d", type, xi->width, xi->height);
		(void) fwrite(hbuf, 1, 24, fp);
		(void) fwrite(buf, 1, size, fp);
	}
	FREEMEM(buf);
	(void) fclose(fp);
}


/*ARGSUSED*/
static void write_png_file(char *fname, XImage *xi, XColor *color, int ncolor)
{
#ifdef MACHINE_PCLINUX
	int         n, bpp, color_type;
	UNCHAR      *buf = NULL;
	png_structp png_ptr = NULL;
    png_infop   info_ptr = NULL;
	png_bytep   *row_pointers = NULL;
	FILE        *fp;

	fp = open_file(fname);
	if(!fp) return;

	get_rgb_raster(xi, color, ncolor, &buf, &bpp);
	if (!buf) goto err1;

	row_pointers = MEM(png_bytep, xi->height);
	if(!row_pointers) goto err1;

	for(n = 0; n < xi->height; n++)
		row_pointers[n] = buf + xi->width * n * bpp;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) goto err1;

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) goto err1;

	if(setjmp(png_jmpbuf(png_ptr))) goto err1;

	color_type = (bpp == 1)? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB;

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, xi->width, xi->height, 8, color_type,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING, NULL);

err1:
	png_destroy_write_struct(&png_ptr, &info_ptr);
	FREEMEM(buf);
	FREEMEM(row_pointers);
	(void) fclose(fp);
#endif
}


/*ARGSUSED*/
static void write_tiff_file(char *fname, XImage *xi, XColor *color, int ncolor)
{
#ifdef MACHINE_PCLINUX
	int    bpp;
	UNCHAR *buf = NULL;
	TIFF   *fp;

	fp = TIFFOpen(fname,"w");
	if(!fp)
	{
		pr_error(Module, "Unable to open file \"%s\" for writing.\n", fname);
		return;
	}

	get_rgb_raster(xi, color, ncolor, &buf, &bpp);
	if (buf)
	{
		TIFFSetField(fp, TIFFTAG_IMAGEWIDTH,      (uint32)xi->width);
		TIFFSetField(fp, TIFFTAG_IMAGELENGTH,     (uint32)xi->height);
		TIFFSetField(fp, TIFFTAG_BITSPERSAMPLE,   8);
		TIFFSetField(fp, TIFFTAG_SAMPLESPERPIXEL, (uint32)bpp);
		TIFFSetField(fp, TIFFTAG_COMPRESSION,     COMPRESSION_DEFLATE);
		TIFFSetField(fp, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
		TIFFSetField(fp, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB);

		if(TIFFWriteEncodedStrip(fp, 0, (char*)buf, (tsize_t)(xi->width * xi->height * bpp)) == 0)
		{
			pr_error(Module, "Unable to write to file \"%s\".\n", fname);
		}
				
	}
	FREEMEM(buf);
	TIFFClose(fp);
#endif
}



/*******************************************************************************/
/*
*  glFileToWindow
*/
/*******************************************************************************/

static int xcolor_cmp( const void *a, const void *b)
{
	UNSHORT r1, g1, b1, r2, g2, b2;

	r1 = ((XColor *)a)->red;
	g1 = ((XColor *)a)->green;
	b1 = ((XColor *)a)->blue;

	r2 = ((XColor *)b)->red;
	g2 = ((XColor *)b)->green;
	b2 = ((XColor *)b)->blue;

	if(r1 > r2) return 1;
	if(r1 < r2) return -1;
	if(g1 > g2) return 1;
	if(g1 < g2) return -1;
	if(b1 > b2) return 1;
	if(b1 < b2) return -1;
	return 0;
}

void glFileToWindow( String fname, LOGICAL center )
{
	int       x, y;
	UNCHAR   *data, *mask, *p;
	Image     ndx;
	ImagePtr  im;
	GC        gc;
	XColor    *cp, xc;
	XImage    *xi;

	size_t nxcl    = 0;
	size_t max_xcl = 256;
	XColor *xcl    = NULL;

	static String MyName = "glFileToWindow";

	ndx = glImageFetchFile(fname);
	if (!ndx) return;

	im = Xgl.images[ndx];

	if(!_xgl_get_source_image(im, &data, &mask)) return;

	p = data;

	xc.flags = DoRed | DoGreen | DoBlue;
	xi = XGetImage(D, WX->draw, 0, 0, (UNINT)im->ow,  (UNINT)im->oh, AllPlanes, ZPixmap);

	xcl = GETMEM(xcl, XColor, max_xcl);
	for( y = 0; y < im->oh; y++ )
	{
		for( x = 0; x < im->ow; x++ )
		{
			xc.red = p[0] << 8;
			if(im->bands == SingleBand)
			{
				xc.green = xc.red;
				xc.blue  = xc.red;
				p++;
			}
			else
			{
				xc.green = p[1] << 8;
				xc.blue  = p[2] << 8;
				p+=3;
			}
			cp = (XColor *)bsearch((void *)&xc, (void *)xcl, nxcl, sizeof(XColor), xcolor_cmp);
			if(!cp)
			{
				cp = &xc;
				xc.pixel = _xgl_pixel_from_XColor(MyName, xc);
				if( nxcl >= max_xcl) xcl = GETMEM(xcl, XColor, max_xcl += 256);
				(void) memcpy((void *)&xcl[nxcl++], cp, sizeof(XColor));
				qsort((void *)xcl, nxcl, sizeof(XColor), xcolor_cmp);
			}
			XPutPixel(xi, x, y, cp->pixel);
		}
	}
	FREEMEM(xcl);
	FREEMEM(data);
	FREEMEM(mask);

	x = y = 0;
	if(center)
	{
		x = (W->xm - im->ow)/2;
		if(x < 1) x = 0;
		y = (W->ym - im->oh)/2;
		if(y < 1) y = 0;
	}
	gc = XCreateGC(D, WX->draw, 0, NULL);
	XPutImage(D, WX->draw, gc, xi, 0, 0, x, y, (UNINT)im->ow, (UNINT)im->oh);
	XDestroyImage(xi);
	XFreeGC(D, gc);
	glImageDestroy(ndx);
}
