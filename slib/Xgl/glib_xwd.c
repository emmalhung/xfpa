/**********************************************************************/
/*
*   File:     glib_xwd.c
*
*   Purpose:  Functions to read and write XWD files.
*
*   Note:     This function does use the X header file to obtain
*             some macro definitions, but does not require any X
*             functions to operate.
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
/**********************************************************************/

#include <X11/X.h>
#include <X11/XWDFile.h>
#include <sys/stat.h>
#include <tools/tools.h>
#include "glib_private.h"


static UNINT getpix    (XWDFileHeader *xwd, FILE *);
static void  swapshort (register char *bp, register size_t n);
static void  swaplong  (register char *bp, register size_t n);

static int    bits_used;
static UNINT  pixel_mask;
static unsigned long swaptest = 1;


LOGICAL _xgl_isXWDFile(STRING fname)
{
	XWDFileHeader xwd;
	LOGICAL       ok;
	FILE          *fp = fopen(fname, BINARY_READ);

	if (!fp) return FALSE;

	ok = (fread((void *)&xwd, sizeof(XWDFileHeader), 1, fp) == 1);
	(void) fclose(fp);
	if (!ok) return FALSE;

	if(*((char *)&swaptest))
		swaplong((char *)&xwd, sizeof(XWDFileHeader));

	if( xwd.file_version != XWD_FILE_VERSION ) return FALSE;

	return TRUE;
}


LOGICAL _xgl_queryXWD(STRING fname, int *w, int *h, int *bands, char **comm)
{
	int            i, len;
	XWDFileHeader  xwd;
	XWDColor       xcolor;
	STRING         comment = NULL;
	FILE           *fp;

	static STRING Module = "queryXWD";

	fp = fopen(fname, BINARY_READ);
	if (!fp) return FALSE;

	*w = *h = 0;
	*bands = 1;
	if (comm) *comm = NULL;

	if(fread((void *)&xwd, sizeof(XWDFileHeader), 1, fp) != 1) goto generr;

	if(*((char *)&swaptest))
		swaplong((char *)&xwd, sizeof(XWDFileHeader));

	if (xwd.pixmap_depth > 24)
	{
		pr_error(Module, "Can't handle X11 pixmap_depth > 24.\n");
		goto generr;
	}

	if (xwd.bits_per_rgb > 24)
	{
		pr_error(Module, "Can't handle X11 bits_per_rgb > 24.\n");
		goto generr;
	}

	if (xwd.pixmap_format != ZPixmap && xwd.pixmap_depth != 1)
	{
		pr_error(Module, "Can't handle X11 pixmap_format %d with depth != 1\n", xwd.pixmap_format);
		goto generr;
	}

	if (xwd.bitmap_unit != 8 && xwd.bitmap_unit != 16 && xwd.bitmap_unit != 32)
	{
		pr_error(Module, "X11 bitmap_unit (%d) is non-standard - can't handle\n", xwd.bitmap_unit);
		goto generr;
	}

	len = (int) xwd.header_size - (int) sizeof(xwd);
	if(len > 0)
	{
		comment = INITMEM(char, len+1);
		if(!comment) goto memerr;
		comment[len] = '\0';
		if(fread((void*)comment, (size_t)len, 1, fp) != 1) goto readerr;
	}

	if( xwd.ncolors > 0 )
	{
		for(i = 0; i < (int) xwd.ncolors; i++)
		{
			if(fread((void *)&xcolor, sizeof(XWDColor), 1, fp) != 1) goto readerr;
			if(*((char *)&swaptest))
			{
				swaplong((char *)&xcolor.pixel, sizeof(long));
				swapshort((char *)&xcolor.red,  sizeof(short) * 3);
			}
			if( xcolor.red != xcolor.green || xcolor.green != xcolor.blue ) break;
		}
		*bands = ( i < (int) xwd.ncolors )? 3 : 1;
	}
	else
	{
		*bands = (xwd.pixmap_depth == 8) ? 1:3;
	}

	*w = (int) xwd.pixmap_width;
	*h = (int) xwd.pixmap_height;

	if (comm)
		*comm = comment;
	else
		FREEMEM(comment);

	(void) fclose(fp);
	return TRUE;

memerr:
	pr_error(Module, "Unable to allocate memory.\n");
readerr:
	pr_error(Module, "Read error on XImage file.\n");
	FREEMEM(comment);
generr:
	(void) fclose(fp);
	return FALSE;
	
}


LOGICAL _xgl_readXWD( ImagePtr im, int *w, int *h, int *d, UNCHAR **image)
{
	UNCHAR        *pic = NULL, *p = NULL, *r = NULL, *g = NULL, *b = NULL;
	int           i, bands, len, col, rows, cols, padright, row;
	XWDFileHeader xwd;
	XWDColor      xcolor;
	FILE          *fp;

	static STRING Module = "readXWD";

	*w     = 0;
	*h     = 0;
	*d     = 0;
	*image = (UNCHAR *) NULL;

	fp = _xgl_open_image_file(im);
	if (!fp) return FALSE;

	if(fread((void *)&xwd, sizeof(XWDFileHeader), 1, fp) != 1) goto generr;

	if(*((char *)&swaptest))
		swaplong((char *)&xwd, sizeof(XWDFileHeader));

	len = (int) xwd.header_size - (int) sizeof(xwd);
	if(len > 0) (void) fseek( fp, (long)len, SEEK_CUR );

	bands = 3;
	if( xwd.ncolors > 0 )
	{
		bands = 1;
		if(IsNull(r = MEM(UNCHAR, xwd.ncolors))) goto memerr;
		if(IsNull(g = MEM(UNCHAR, xwd.ncolors))) goto memerr;
		if(IsNull(b = MEM(UNCHAR, xwd.ncolors))) goto memerr;

		for( i = 0; i < (int) xwd.ncolors; i++ )
		{
			(void) fread((void *)&xcolor, sizeof(XWDColor), 1, fp);
			if(*((char *)&swaptest))
			{
				swaplong((char *)&xcolor.pixel, sizeof(long));
				swapshort((char *)&xcolor.red,  sizeof(short) * 3);
			}
			r[i] = (UNCHAR) (xcolor.red   >> 8);
			g[i] = (UNCHAR) (xcolor.green >> 8);
			b[i] = (UNCHAR) (xcolor.blue  >> 8);

			/* If by chance the colormap entry is transparent change it to black */
			if(r[i] == T_RED && g[i] == T_GREEN && b[i] == T_BLUE)
			{
				r[i] = g[i] = b[i] = 0;
			}
			if(r[i] != g[i] || r[i] != b[i]) bands = 3;
		}
	}
	
	cols = (int) xwd.pixmap_width;
	rows = (int) xwd.pixmap_height;
	padright = xwd.bytes_per_line * 8 / xwd.bits_per_pixel - xwd.pixmap_width;
	bits_used = (int) xwd.bitmap_unit;
	
	if (xwd.bits_per_pixel == sizeof(pixel_mask) * 8)
		pixel_mask = (UNINT) -1;
	else
		pixel_mask = (UNINT) ((1 << xwd.bits_per_pixel) - 1);
	
	switch (xwd.visual_class)
	{
 		case StaticGray:
			bands = 1;
			pic = MEM(UNCHAR, cols*rows);
			if (!pic) goto memerr;
			for (row=0; row<rows; row++)
			{
				for (col=0, p=pic+(row*cols); col<cols; col++, p++)
					*p = (UNCHAR)(getpix(&xwd, fp) * 255);
				for (col=0; col<padright; col++) (void) getpix(&xwd, fp);
			}
			break;      

		case GrayScale:
			bands = 1;
			pic = MEM(UNCHAR, cols*rows);
			if (!pic) goto memerr;
			for (row=0; row<rows; row++)
			{
				for (col=0, p=pic+(row*cols); col<cols; col++, p++)
				{
					if (r) *p = r[getpix(&xwd, fp)];
					else   *p = (UNCHAR)getpix(&xwd, fp);
				}
				for (col=0; col<padright; col++) (void) getpix(&xwd, fp);
			}
			break;      

		case StaticColor:
		case PseudoColor:
			pic = MEM(UNCHAR, cols*rows*bands);
			if (!pic) goto memerr;
			if(bands == 1)
			{
				for (row=0; row<rows; row++)
				{
					for (col=0, p=pic+(row*cols); col<cols; col++, p++)
						*p = r[getpix(&xwd, fp)];
					for (col=0; col<padright; col++) (void) getpix(&xwd, fp);
				}
			}
			else
			{
				for (row=0; row<rows; row++)
				{
					for (col=0, p=pic+(row*cols*RASTER_BPP); col<cols; col++)
					{
						UNINT pn = getpix(&xwd, fp);
						*p++ = r[pn];
						*p++ = g[pn];
						*p++ = b[pn];
					}
					for (col=0; col<padright; col++) (void) getpix(&xwd, fp);
				}
			}
			break;      

		case TrueColor:
		case DirectColor:
			bands = 3;
			pic = MEM(UNCHAR, cols*rows*RASTER_BPP);
			if (!pic) goto memerr;
			for (row=0; row<rows; row++)
			{
				for (col=0, p=pic+(row*cols*RASTER_BPP); col<cols; col++)
				{
					UNINT pn = getpix(&xwd, fp);
					if(xwd.red_mask > xwd.blue_mask)
					{
						switch (xwd.bits_per_pixel)
						{
							case 16:
								*p++ = ((pn & xwd.red_mask)   >> 10);
								*p++ = ((pn & xwd.green_mask) >> 5);
								*p++ =  (pn & xwd.blue_mask);
								break;
	  
							case 24:
							case 32:
								*p++ = (pn>>16) & 0xff;
								*p++ = (pn>> 8) & 0xff;
								*p++ = (pn    ) & 0xff;
								break;

							default:
								pr_error(Module, "True/Direct only supports 16, 24, and 32 bits\n");
								goto generr;
						}
					}
					else
					{
						switch (xwd.bits_per_pixel)
						{
							case 16:
								*p++ =  (pn & xwd.red_mask);
								*p++ = ((pn & xwd.green_mask) >> 5);
								*p++ = ((pn & xwd.blue_mask)  >> 10);
								break;
	  
							case 24:
							case 32:
								*p++ = (pn    ) & 0xff;
								*p++ = (pn>> 8) & 0xff;
								*p++ = (pn>>16) & 0xff;
								break;

							default:
								pr_error(Module, "True/Direct only supports 16, 24, and 32 bits\n");
								goto generr;
						}
					}
				}
				for (col=0; col<padright; col++) (void) getpix(&xwd, fp);
			}
    		break;
    
  		default:
			pr_error(Module, "Unknown visual class in image\n");
    		goto generr;
	}
	FREEMEM(r);
	FREEMEM(g);
	FREEMEM(b);

	*w     = cols;
	*h     = rows;
	*d     = bands;
   	*image = pic;

	(void) fclose(fp);
	return TRUE;

memerr:
	pr_error(Module, "Unable to allocate memory.\n");
generr:
	(void) fclose(fp);
	FREEMEM(r);
	FREEMEM(g);
	FREEMEM(b);
	FREEMEM(pic);
	return FALSE;
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


static UNINT getpix(XWDFileHeader *xwd, FILE *fp)
{
	UNINT          n;
	static int     bit_shift;
	static UNCHAR  byteP;
	static UNSHORT shortP;
	static UNINT   longP;
	
	if (bits_used == (int) xwd->bitmap_unit)
	{
		switch (xwd->bitmap_unit)
		{
			case 8:
		  		byteP = (UNCHAR)getc(fp);
		  		break;
		  
			case 16:

		  		if (xwd->byte_order == MSBFirst)
				{
					shortP  = (UNSHORT)((getc(fp) & 0xff) << 8);
					shortP |= (UNSHORT)( getc(fp) & 0xff);
				}
		  		else
				{
					shortP  = (UNSHORT)( getc(fp) & 0xff);
					shortP |= (UNSHORT)((getc(fp) & 0xff) << 8);
				}
		  		break;
		  
			case 32:
		  		if (xwd->byte_order == MSBFirst)
				{
					longP  = (UNINT)((getc(fp) & 0xff) << 24);
					longP |= (UNINT)((getc(fp) & 0xff) << 16);
					longP |= (UNINT)((getc(fp) & 0xff) << 8);
					longP |= (UNINT)( getc(fp) & 0xff);
				}
		  		else
				{
					longP  = (UNINT)( getc(fp) & 0xff);
					longP |= (UNINT)((getc(fp) & 0xff) << 8);
					longP |= (UNINT)((getc(fp) & 0xff) << 16);
					longP |= (UNINT)((getc(fp) & 0xff) << 24);
				}
		  		break;
		}
		bits_used = 0;
		
		if (xwd->bitmap_bit_order == MSBFirst)
			bit_shift = (int) (xwd->bitmap_unit - xwd->bits_per_pixel);
		else
			bit_shift = 0;
	}
	
	switch (xwd->bitmap_unit)
	{
		case 8:
			n = (UNINT) ((byteP >> bit_shift) & pixel_mask);
			break;
			
		case 16:
			n = (UNINT) ((shortP >> bit_shift) & pixel_mask);
			break;
			
		case 32:
			n = (UNINT) ((longP >> bit_shift) & pixel_mask);
			break;
			
		default:
			n = 0;
	}
	
	if (xwd->bitmap_bit_order == MSBFirst)
		bit_shift -= xwd->bits_per_pixel;
	else
		bit_shift += xwd->bits_per_pixel;

	bits_used += xwd->bits_per_pixel;
	
	return n;
}
