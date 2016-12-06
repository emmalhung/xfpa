/**********************************************************************/
/*
*   File:     glib_tiff.c
*
*   Purpose:  Functions to read and write TIFF files.
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

#include <tools/tools.h>
#include "glib_private.h"



#ifndef MACHINE_PCLINUX


/*ARGSUSED*/
LOGICAL _xgl_isTIFF(STRING fname)
{
	pr_warning(NULL, "TIFF encoded files are not handled on this platform.\n");
	return FALSE;
}

/*ARGSUSED*/
LOGICAL _xgl_queryTIFF(STRING fname, int *width, int *height, int *bands, glCOLOR *cmap )
{
	return FALSE;
}

/*ARGSUSED*/
LOGICAL _xgl_readTIFF( ImagePtr im, UNCHAR **raster, UNCHAR **mask )
{
	return FALSE;
}

#else

#include <tiffio.h>


/*************** Private Functions *******************/

/* convert 16 bit colour to 8 bit colour */
#define	CVT(x)	(UNCHAR)(((x) * 255) / ((1L<<16)-1))

static void copy_colours( int nc, uint16 *rmap, uint16 *gmap, uint16 *bmap, glCOLOR *cmap)
{
	int n;

	/* Does the colormap range from 0-255 or from 0-65535 */
	for(n = 0; n < nc; n++)
	{
		if(rmap[n] > 255) break;
		if(gmap[n] > 255) break;
		if(bmap[n] > 255) break;
	}
	if( n < nc )
	{
		for (n = 0; n < nc; n++)
		{
			cmap[n].red   = CVT(rmap[n]);
			cmap[n].green = CVT(gmap[n]);
			cmap[n].blue  = CVT(bmap[n]);
		}
	}
	else
	{
		for (n = 0; n < nc; n++)
		{
			cmap[n].red   = (UNCHAR)rmap[n];
			cmap[n].green = (UNCHAR)gmap[n];
			cmap[n].blue  = (UNCHAR)bmap[n];
		}
	}
}


/************** Library Only External Functions *****************/


/*ARGSUSED*/
LOGICAL _xgl_isTIFF(STRING fname)
{
	char    errmsg[1024];
	LOGICAL status = FALSE;
	TIFF    *fd;

	fd = TIFFOpen(fname, "r");
	if (fd)
	{
		status = (TIFFRGBAImageOK(fd, errmsg) != 0);
		TIFFClose(fd);
	}
	return status;
}



LOGICAL _xgl_queryTIFF(STRING fname, int *w, int *h, int *bands, glCOLOR *cmap )
{
	int      n;
	LOGICAL  status = FALSE;
	TIFF    *fd;
	uint32   length = 0;
	uint32   width = 0;
	uint16   bits_per_sample = 8;
	uint16   samples_per_pixel = 3;
	uint16  *rmap, *gmap, *bmap;


	for( n = 0; n < 256; n++ )
		cmap[n].red = cmap[n].green = cmap[n].blue = 0;

	fd = TIFFOpen(fname, "r");
	if (!fd) return FALSE;

	(void) TIFFGetField(fd, TIFFTAG_IMAGEWIDTH, &width);
	(void) TIFFGetField(fd, TIFFTAG_IMAGELENGTH, &length);
	(void) TIFFGetField(fd, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
	(void) TIFFGetField(fd, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);

	*w     = (int) width;
	*h     = (int) length;
	*bands = (samples_per_pixel == 1 && bits_per_sample <= 8)? SingleBand : TripleBand;
	status = ( width > 0 && length > 0 );

	/* If an indexed image get the colormap */
	if(*bands == SingleBand && TIFFGetField(fd, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap) == 1)
	{
		copy_colours(1<<bits_per_sample,rmap, gmap, bmap, cmap);
	}
	else /* If we do not have a colormap handle as a triple band image */
	{
		*bands = TripleBand;
	}
	TIFFClose(fd);

	return TRUE;
}


/* Read the file but map the tiff return to our internal format of either single
 * band with mask or triple band with transparent pixel. It is expected that the
 * info function above has been called so that we know how many bands our internal
 * image will be.
 */
LOGICAL _xgl_readTIFF( ImagePtr im, UNCHAR **inrast, UNCHAR **inmask )
{
	int      i, n, x, y, ncmap = 0;
	char     errmsg[1024];
	LOGICAL  ok;
	STRING   fname = NULL;
	UNCHAR  *p;
	UNCHAR   black_ndx = 0;
	UNCHAR  *ras = NULL;
	UNCHAR  *msk = NULL;
	glCOLOR  cmap[256];
	TIFF    *fd = 0;
	size_t   npixels;
	uint16   orientation       = ORIENTATION_TOPLEFT;
   	uint16   bits_per_sample   = 8;
   	uint16   samples_per_pixel = 3;
	uint16  *rmap, *gmap, *bmap;
	uint32   height = 0;
	uint32   width = 0;
	uint32  *b, *raster;


	/* See _xgl_open_image_file() in glib_image.c for the reason for
	 * the following file name determination.
	 */
	if(im->imdef)
	{
		fname = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
		fd = TIFFOpen(fname, "r");
	}
	else if(im->vtime)
	{
		fname = safe_strdup(im->vtime);
		fd = TIFFOpen(fname, "r");
	}
	if (!fd)
	{
		pr_error("TIFF Decode","Unable to open file: \"%s\"\n", fname);
		FREEMEM(fname);
		return FALSE;
	}

	/* Make sure the image is a TIFF */
	if(!TIFFRGBAImageOK(fd, errmsg)) goto err1;

	(void) TIFFGetField(fd, TIFFTAG_IMAGEWIDTH, &width);
	(void) TIFFGetField(fd, TIFFTAG_IMAGELENGTH, &height);
    (void) TIFFGetField(fd, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    (void) TIFFGetField(fd, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);

	/* If we have an indexed image get the colour table and save it */
	if(bits_per_sample <= 8 && samples_per_pixel == 1 &&
		TIFFGetField(fd, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap) == 1)
	{
		ncmap = (1 << bits_per_sample);
		copy_colours(ncmap, rmap, gmap, bmap, cmap);
		for(n = 0; n < ncmap; n++)
		{
			if(cmap[n].red   != 0) continue;
			if(cmap[n].green != 0) continue;
			if(cmap[n].blue  != 0) continue;
			black_ndx = (UNCHAR)n;
			break;
		}
	}

	if(TIFFGetField(fd, TIFFTAG_ORIENTATION, &orientation) == 0)
	{
		pr_warning("TIFF Decode","No orientation tag in \"%s\". Top left is assumed.\n", fname);
	}
	else if(orientation != ORIENTATION_TOPLEFT && orientation != ORIENTATION_BOTLEFT)
	{
		pr_error("TIFF Decode","TIFF image orientation unsupported: \"%s\"\n", fname);
		TIFFClose(fd);
		FREEMEM(fname);
		return FALSE;
	}

	npixels = width * height;
	raster = (uint32*) _TIFFmalloc((tsize_t)(npixels * sizeof (uint32)));
	if (!raster) goto err1;

	ok = TIFFReadRGBAImage(fd, width, height, raster, 0);
	TIFFClose(fd);
	fd = 0;
	if (!ok)
	{
		pr_error("TIFF Decode","Read failure on tiff image file: \"%s\"\n", fname);
		FREEMEM(fname);
		return FALSE;
	}
	FREEMEM(fname);

	/*  For simplicity the image is returned in rgba format and is then
	 *  converted to our internal format. We also have to account for the
	 *  storage method (orientation) of the raster.
	 */
	if(im->bands == SingleBand)
	{
		if((ras = INITMEM(UNCHAR, width*height)) == NULL) goto err1;
		if((msk = INITMEM(UNCHAR, MASK_SIZE(width,height))) == NULL) goto err1;

		if(ncmap)
		{
			/* The image came from an indexed colormap and we wish to retain this mode */
			for( n = 0, b = raster, y = 0; y < (int) height; y++ )
			{
				if(orientation == ORIENTATION_TOPLEFT)
					p = ras + width*(height - 1 - y);
				else
					p = ras + width*y;

				for( x = 0; x < (int) width; x++, n++, b++ )
				{
					*p = black_ndx;
					if(TIFFGetA(*b))
					{
						UNCHAR rc, gc, bc;
						rc = (UNCHAR) TIFFGetR(*b);
						gc = (UNCHAR) TIFFGetG(*b);
						bc = (UNCHAR) TIFFGetB(*b);
						for(i=0; i<ncmap; i++)
						{
							if(rc != cmap[i].red  ) continue;
							if(gc != cmap[i].green) continue;
							if(bc != cmap[i].blue ) continue;
							*p = (UNCHAR)i;
							break;
						}
						SET_MASK_BIT(msk, n);
					}
					p++;
				}
			}
		}
		else if(FORCE_GREYSCALE(im->imdef))
		{
			for( n = 0, b = raster, y = 0; y < (int) height; y++ )
			{
				if(orientation == ORIENTATION_TOPLEFT)
					p = ras + width*(height - 1 - y);
				else
					p = ras + width*y;

				for( x = 0; x < (int) width; x++, n++, b++ )
				{
					*p++ = CONVERT_TO_GRAY_PIXEL(TIFFGetR(*b), TIFFGetG(*b), TIFFGetB(*b));
					if(TIFFGetA(*b)) SET_MASK_BIT(msk, n);
				}
			}
		}
		else
		{
			for( n = 0, b = raster, y = 0; y < (int) height; y++ )
			{
				if(orientation == ORIENTATION_TOPLEFT)
					p = ras + width*(height - 1 - y);
				else
					p = ras + width*y;

				for( x = 0; x < (int) width; x++, n++, b++ )
				{
					*p++ = (UNCHAR) TIFFGetR(*b);
					if(TIFFGetA(*b)) SET_MASK_BIT(msk, n);
				}
			}
		}
	}
	else
	{
		/* We could still have a greyscale raster here if the info function
		 * did not find a colormap. Try greyscale first and if colour is 
		 * found switch to full colour. If the raster is colour we should
		 * jump out of the first look quickly.
		 */
		LOGICAL greyscale = TRUE;

		if((ras = INITMEM(UNCHAR, width*height)) == NULL) goto err1;
		if((msk = INITMEM(UNCHAR, MASK_SIZE(width,height))) == NULL) goto err1;
		for( n = 0, b = raster, y = 0; y < (int) height && greyscale; y++ )
		{
			if(orientation == ORIENTATION_TOPLEFT)
				p = ras + width*(height - 1 - y);
			else
				p = ras + width*y;

			for( x = 0; x < (int) width && greyscale; x++, n++, b++ )
			{
				uint32  red    = TIFFGetR(*b);
				LOGICAL opaque = (TIFFGetA(*b) != 0);
				*p++ = (UNCHAR)red;
				if (opaque) SET_MASK_BIT(msk, n);
				greyscale = ( opaque && (red == TIFFGetG(*b) && red == TIFFGetB(*b)));
			}
		}
		if(greyscale)
		{
			im->bands = SingleBand;
		}
		else
		{
			FREEMEM(ras);
			FREEMEM(msk);
			if((ras = INITMEM(UNCHAR, width*height*3)) == NULL) goto err1;
			for( b = raster, y = 0; y < (int) height; y++ )
			{
				if(orientation == ORIENTATION_TOPLEFT)
					p = ras + width*3*(height - 1 - y);
				else
					p = ras + width*y*3;

				for( x = 0; x < (int) width; x++, b++ )
				{
					if(TIFFGetA(*b))
					{
						*p++ = (UNCHAR) TIFFGetR(*b);
						*p++ = (UNCHAR) TIFFGetG(*b);
						*p++ = (UNCHAR) TIFFGetB(*b);
					}
					else
					{
						*p++ = T_RED;
						*p++ = T_GREEN;
						*p++ = T_BLUE;
					}
				}
			}
		}
	}
   _TIFFfree(raster);

	*inrast = ras;
	*inmask = msk;
	return TRUE;

err1:
	if (fd) TIFFClose(fd);
	if (raster) _TIFFfree(raster);
	FREEMEM(fname);
	FREEMEM(msk);
	FREEMEM(ras);
	pr_error("TIFF Decode","Memory allocation failure\n");
	return FALSE;
}

#endif
