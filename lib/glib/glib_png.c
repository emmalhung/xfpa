/**********************************************************************/
/*
*   File:     glib_png.c
*
*   Purpose:  Functions to read and write PNG files.
*
*
*   Note:     The following data can be encoded into the png images
*             using the set_text function. 
*
*    Key               Value
*    ---               -----
*   "OriginLocation"  "UpperLeft" - y origin is in the upper left (default)
*                     "LowerLeft" - y origin is in the lower left
*   "Projection"      The map projection as an ascii string.
*   "MapDef"          The map definition of the image as an ascii string
*                     (same format found in the image definition file).
*
*   Image data can be encoded into the PNG file. If the file name ends
*   in "dpng" or "dpg" then a data png is assumed. Any other extension
*   requires the key-value pair "ImageType"-"DataImage" to be present
*   in order to be accepted as a data encoded PNG.
*
*   The functions ...DataPNG are for png files that carry image data. The
*   "image" is actually an array of encoded values, for example temperature,
*   and not pixels. The values must be stored as either 8 or 16 bit
*   greyscale pixels.  As on option, information about the data image may be
*   in the comment block of the image. The expected values (in addition to
*   the above) are:
*
*    Key               Value
*    ---               -----
*   "ImageType"       "DataImage"
*   "DataScale"       Scale as an ascii float  (default 1)
*   "DataOffset"      Offset as an ascii float (default 0)
*
*   The header file glib.h has these defined as the following macros
*   for avoiding finger trouble.
*
*		PNGTAG_ORIGINLOCATION	"OriginLocation"
*		PNGTAG_PROJECTION		"Projection"
*		PNGTAG_MAPDEF			"MapDef"
*		PNGTAG_IMAGETYPE		"ImageType"
*		PNGTAG_DATASCALE		"DataScale"
*		PNGTAG_DATAOFFSET		"DataOffset"
*
*		PNGVAL_UPPERLEFT		"UpperLeft"
*		PNGVAL_LOWERLEFT		"LowerLeft"
*		PNGVAL_DATAIMAGE		"DataImage"
*
*   See the glib_image_config.c file for a discussion of the
*   scale and offset values.
*
*------------------------------------------------------------------------
*
* Example code for creating a dataPNG file. We assume that the data is
* 8 bits per value and in a raster of size nx,ny. A bunch of error checks
* are left out.
*
*
* text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
* text_ptr[0].key = "ImageType";
* text_ptr[0].text = "DataImage";
* text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
* text_ptr[1].key = "ScaleValue";
* text_ptr[1].text = ".01";
* text_ptr[2].compression = PNG_TEXT_COMPRESSION_NONE;
* text_ptr[2].key = "OffsetValue";
* text_ptr[2].text = "0";
* text_ptr[3].compression = PNG_TEXT_COMPRESSION_NONE;
* text_ptr[3].key = "MapDef";
* text_ptr[3].text = "41.3 -83.1 0 0 0 72 51 0.1"
*
* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
* if(png_ptr)
* {
*     info_ptr = png_create_info_struct(png_ptr);
*     if(info_ptr)
*     {
*         if(setjmp(png_jmpbuf(png_ptr)) == 0)
*         {
*             fp = fopen(fname,"wb");
*             png_init_io(png_ptr, fp);
*             png_set_IHDR(png_ptr, info_ptr, nx, ny, 8, PNG_COLOR_TYPE_GRAY,
*                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
*             png_set_text(png_ptr, info_ptr, text_ptr, num_text);
*             png_write_info(png_ptr, info_ptr);
*             row_pointers = (png_bytep *) calloc(ny, sizeof(png_bytep));
*             for(n = 0; n < ny; n++) row_pointers[n] = raster + nx * n;
*             png_write_image(png_ptr, row_pointers);
*             png_write_end(png_ptr, NULL);
*             fclose(fp);
*             free(row_pointers);
*         }
*         png_destroy_write_struct(&png_ptr, &info_ptr);
*     }
* }
*
*------------------------------------------------------------------------
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
*/
/*************************************************************************/
#include <tools/tools.h>
#include "glib_private.h"

#ifndef MACHINE_PCLINUX

/*ARGSUSED*/
LOGICAL _xgl_isPNG(STRING fname)
{
	pr_warning("isPNG", "PNG encoded files are not handled on this platform.\n");
	return FALSE;
}

/*ARGSUSED*/
LOGICAL _xgl_queryPNG(STRING fname, IMDEF *imdef, int *width, int *height, int *channels, glCOLOR *cmap)
{
	return FALSE;
}

/*ARGSUSED*/
LOGICAL _xgl_readPNG( ImagePtr im, UNCHAR **raster, UNCHAR **mask )
{
	return FALSE;
}


/*ARGSUSED*/
LOGICAL _xgl_isDataPNG(STRING fname)
{
	pr_warning("isPNG", "PNG encoded data files are not handled on this platform.\n");
	return FALSE;
}

/*ARGSUSED*/
LOGICAL _xgl_queryDataPNG(STRING fname, int *width, int *height, IMDEF *grid)
{
	return FALSE;
}

/*ARGSUSED*/
LOGICAL _xgl_readDataPNG( ImagePtr im, UNCHAR **raster )
{
	return FALSE;
}

#else

#include <png.h>

LOGICAL _xgl_isPNG(STRING fname)
{
	UNCHAR  header[8];
	FILE   *fp = fopen(fname, BINARY_READ);

	if (!fp) return FALSE;

	(void) fread(header, 1, 8, fp);
	(void) fclose(fp);
    return (png_sig_cmp(header, 0, 8) == 0);
}


/* Get image information.
*/
LOGICAL _xgl_queryPNG(STRING fname, IMDEF *imdef, int *width, int *height, int *bands, glCOLOR *cmap)
{
	int         n, nt, bit_depth, color_type;
	png_uint_32 w, h;
	png_structp png_ptr  = NULL;
	png_infop   info_ptr = NULL;
	png_text    *text_ptr = NULL;
	FILE        *fp;

	fp = fopen(fname, BINARY_READ);
	if (!fp) return FALSE;

	/* Bug fix 20121220: The colormap must be initialized to the gray scale.
	 *                   Previously it was initialized to 0.
	 */
	for( n = 0; n < 256; n++ )
		cmap[n].red = cmap[n].green = cmap[n].blue = (UNCHAR) n;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) goto queryerr;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		goto queryerr;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        goto queryerr;
    }

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);
	(void) png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);

    if(bit_depth <= 8 && color_type == PNG_COLOR_TYPE_PALETTE)
	{
		int           num_palette, num_trans;
		png_color_16p values;
		png_color     *palette;
		png_bytep     trans;

		*bands = SingleBand;

		if(bit_depth < 8) png_set_packing(png_ptr);
		
		(void) png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);

		/* Use MIN here just to be paranoid as the bit_depth restriction
		 * should eliminate any palette over 256 in size
		 */
		for(n = 0; n < MIN(256,num_palette); n++)
		{
			cmap[n].red   = (UNCHAR)palette[n].red;
			cmap[n].green = (UNCHAR)palette[n].green;
			cmap[n].blue  = (UNCHAR)palette[n].blue;
		}

		/* If there is transparency information modify the colour table
		 * assuming that any non-zero alpha value is not transparent
		 */
		if(png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &values))
		{
			for(n = 0;n < MIN(256,num_trans); n++)
			{
				if(trans[n] != 0) continue;
				cmap[n].red   = T_RED;
				cmap[n].green = T_GREEN;
				cmap[n].blue  = T_BLUE;
			}
		}
	}
	else
	{
		if(bit_depth == 16)
			png_set_strip_16(png_ptr);
		if(bit_depth < 8)
			png_set_packing(png_ptr);
		if(color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png_ptr);
		if(bit_depth < 8 && color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
#ifndef PNG_1_0_X
			png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
			png_set_gray_1_2_4_to_8(png_ptr);
#endif
	}

	png_read_update_info(png_ptr, info_ptr);

	/* Channels 1 & 2 are g & ga or palette images, channels 3 & 4 are rgb & rgba */
	if(png_get_channels(png_ptr, info_ptr) < 3)
		*bands = SingleBand;
	else
		*bands = TripleBand;

	*width  = (int) w;
	*height = (int) h;

	/* check for scale and offset values stored as comments */
	nt = png_get_text(png_ptr, info_ptr, &text_ptr, NULL);
	for(n = 0; n < nt; n++)
	{
		if(same(text_ptr[n].key, "Projection"))
		{
			if(!_xgl_parse_projection(text_ptr[n].text, &imdef->mproj))
				pr_error(ActiveModule, "Invalid map projection in PNG file \"%s\".\n", fname);
		}
		else if(same_ic(text_ptr[n].key, "MapDef"))
		{
			if(!_xgl_parse_map_def(text_ptr[n].text, &imdef->mproj))
				pr_error(ActiveModule, "Invalid map definition in PNG file \"%s\".\n", fname);
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	(void) fclose(fp);
	return TRUE;

queryerr:
	(void) fclose(fp);
	return FALSE;
}


/* Read the file but map the png return to our internal format of either single
 * band with mask or triple band with transparent pixel. It is expected that the
 * info function above has been called so that we know how many bands our internal
 * image will be.
 */
LOGICAL _xgl_readPNG( ImagePtr im, UNCHAR **raster, UNCHAR **mask )
{
	int           nt, bit_depth, color_type, num_trans;
	UNCHAR        *p, *b;
	UNCHAR        *array = NULL;
	UNCHAR        *ras   = NULL;
	UNCHAR        *msk   = NULL;
	LOGICAL       lower_left = FALSE;
	FILE          *fp;
	png_color_16p values;
	png_uint_32   n, width, height, rowbytes;
	png_byte      channels;
	png_bytep     trans, *row_pointers = NULL;
	png_structp   png_ptr  = NULL;
	png_infop     info_ptr = NULL;
	png_text      *text_ptr = NULL;
	

	fp = _xgl_open_image_file(im);
	if(!fp) return FALSE;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) goto readerr;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) goto readerr;

	if(setjmp(png_jmpbuf(png_ptr))) goto readerr;

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);
	(void) png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    if(bit_depth <= 8 && color_type == PNG_COLOR_TYPE_PALETTE)
	{
		if(bit_depth < 8) png_set_packing(png_ptr);
	}
	else
	{
		if(bit_depth == 16)
			png_set_strip_16(png_ptr);
		if(bit_depth < 8)
			png_set_packing(png_ptr);
		if(color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png_ptr);
		if(bit_depth < 8 && color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
#ifndef PNG_1_0_X
			png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
			png_set_gray_1_2_4_to_8(png_ptr);
#endif
	}

	png_read_update_info(png_ptr, info_ptr);

	channels = png_get_channels(png_ptr, info_ptr);
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	if((array = MEM(UNCHAR, height*rowbytes))  == NULL) goto rasterr;
	if((row_pointers = MEM(png_bytep, height)) == NULL) goto rasterr;

	/* If the origin is in the lower left we must reverse the extraction */
	nt = png_get_text(png_ptr, info_ptr, &text_ptr, NULL);
	for(n = 0; n < nt; n++)
	{
		if(!same_ic(text_ptr[n].key,"OriginLocation")) continue;
		lower_left = same_ic(text_ptr[n].text,"LowerLeft");
		break;
	}
	if(lower_left)
	{
		for(n = 0; n < height; n++)
			row_pointers[n] = array + rowbytes*(height-1-n);
	}
	else
	{
		for(n = 0; n < height; n++)
			row_pointers[n] = array + rowbytes*n;
	}

	png_read_image(png_ptr, row_pointers);

	FREEMEM(row_pointers);

	/* SingleBand images can be produced from multiband original images if the
	 * force to greyscale flag was set. In all cases where there is an alpha
	 * channel any non-zero alpha is assumed to be not transparent.
	 */
	if(im->bands == SingleBand && color_type == PNG_COLOR_TYPE_PALETTE)
	{
		/* palette images should only occur on channel 1 as transparency is
		 * handled through the palette and we limited the bit depth to <= 8.
		 */
		if(channels != 1) goto decode_err;
		ras = array;
	}
	else if(png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &values))
	{
		UNCHAR rt, gt, bt;
		rt = (UNCHAR)values->red;
		gt = (UNCHAR)values->green;
		bt = (UNCHAR)values->blue;

		/* If we have a transparent pixel value specified then we should not get
		 * channels 2 or 4 as there should not be an alpha channel.
		 */
		if(im->bands == SingleBand)
		{
			switch(channels)
			{
				case 1:
					ras = array;
					if((msk = INITMEM(UNCHAR, MASK_SIZE(width,height))) == NULL) goto rasterr;
					for(b = array, n = 0; n < width*height; n++, b++)
					{
						if((*b = rt)) SET_MASK_BIT(msk, n);
					}
					break;

				case 3:
					if((ras = INITMEM(UNCHAR, width*height)) == NULL) goto rasterr;
					if((msk = INITMEM(UNCHAR, MASK_SIZE(width,height))) == NULL) goto rasterr;
					for(b = array, p = ras, n = 0; n < width*height; n++, b+=3)
					{
						*p++ = CONVERT_TO_GRAY_PIXEL(b[0], b[1], b[2]);
						if(b[0] == rt && b[1] == gt && b[2] == bt) SET_MASK_BIT(msk, n);
					}
					FREEMEM(array);
					break;

				default:
					goto decode_err;
					break;
			}
		}
		else
		{
			/* If this is a triple band image with a transparent pixel then the data
			 * must only be on channel 3 (no alpha channel).
			 */
			if(channels != 3) goto decode_err;

			ras = array;

			/* only process the array if the transparent pixel is different from
			 * the library one (it might be one of our own png images ;-)
			 */
			if(rt != T_RED || gt != T_GREEN || bt != T_BLUE)
			{
				for(b = array, n = 0; n < width*height; n++, b+=3)
				{
					if(b[0] == rt && b[1] == gt && b[2] == bt)
					{
						b[0] = T_RED;
						b[1] = T_GREEN;
						b[2] = T_BLUE;
					}
				}
			}
		}
	}
	else if(im->bands == SingleBand)
	{
		/* Check for all channels as a force to greyscale flag setting means that an
		 * rgb image might be converted to greyscale
		 */
		switch(channels)
		{
			case 1:
				ras = array;
				break;

			case 2:
				if((ras = INITMEM(UNCHAR, width*height)) == NULL) goto rasterr;
				if((msk = INITMEM(UNCHAR, MASK_SIZE(width,height))) == NULL) goto rasterr;
				for(b = array, p = ras, n = 0; n < width*height; n++)
				{
					*p++ = *b++;
					if (*b++) SET_MASK_BIT(msk, n);
				}
				FREEMEM(array);
				break;

			case 3:
				if((ras = INITMEM(UNCHAR, width*height)) == NULL) goto rasterr;
				for(b = array, p = ras, n = 0; n < width*height; n++, b+=3)
				{
					*p++ = CONVERT_TO_GRAY_PIXEL(b[0], b[1], b[2]);
				}
				FREEMEM(array);
				break;

			case 4:
				if((ras = INITMEM(UNCHAR, width*height)) == NULL) goto rasterr;
				if((msk = INITMEM(UNCHAR, MASK_SIZE(width,height))) == NULL) goto rasterr;
				for(b = array, p = ras, n = 0; n < width*height; n++, b+=4)
				{
					*p++ = CONVERT_TO_GRAY_PIXEL(b[0], b[1], b[2]);
					if (b[3]) SET_MASK_BIT(msk, n);
				}
				FREEMEM(array);
				break;

			default:
				goto decode_err;
				break;
		}
	}
	else
	{
		/* Only channels 3 and 4 correspond to a triple band image
		 */
		switch(channels)
		{
			case 3:
				ras = array;
				break;

			case 4:
				if((ras = INITMEM(UNCHAR, width*height*3)) == NULL) goto rasterr;
				for(b = array, p = ras, n = 0; n < width*height; n++)
				{
					if(b[3])
					{
						*p++ = b[0];
						*p++ = b[1];
						*p++ = b[2];
					}
					else
					{
						*p++ = T_RED;
						*p++ = T_BLUE;
						*p++ = T_GREEN;
					}
					b += 4;
				}
				FREEMEM(array);
				break;

			default:
				goto decode_err;
				break;
		}
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	*raster = ras;
	*mask   = msk;
	return TRUE;

readerr:
	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	FREEMEM(row_pointers);
	FREEMEM(array);
	return FALSE;

rasterr:
	(void) fprintf(stderr, "Memory allocation failure\n");
decode_err:
	(void) fprintf(stderr, "Problem decoding PNG image\n");
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	FREEMEM(msk);
	FREEMEM(ras);
	FREEMEM(array);
	return FALSE;
}



/* Determine if the given file is a data png type.
 */
LOGICAL _xgl_isDataPNG(STRING fname)
{
	UNCHAR      header[8];
	png_structp png_ptr  = NULL;
	png_infop   info_ptr = NULL;
	LOGICAL     status   = FALSE;
	FILE       *fp;

	fp = fopen(fname, BINARY_READ);
	if(!fp) return FALSE;

	(void) fread(header, 1, 8, fp);
    if(png_sig_cmp(header, 0, 8) == 0)
	{
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr)
		{
			if ((info_ptr=png_create_info_struct(png_ptr)) != NULL && setjmp(png_jmpbuf(png_ptr)) == 0)
			{
				png_uint_32 n, nc;
				png_text    *text_ptr;

				png_init_io(png_ptr, fp);
				png_set_sig_bytes(png_ptr, 8);
				png_read_info(png_ptr, info_ptr);
				nc = png_get_text(png_ptr, info_ptr, &text_ptr, NULL);
				for(n = 0; n < nc && !status; n++)
				{
					if(same_ic(text_ptr[n].key,PNGTAG_IMAGETYPE) && same_ic(text_ptr[n].text,PNGVAL_DATAIMAGE))
						status = TRUE;
				}
			}
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		}
	}
	(void) fclose(fp);
	return status;
}



/* Return information from a PNG file that contains a grid of data values. The
 * data must be stored as greyscale with no alpha channel. The comment section
 * in the file must have the scaling and offset factors included.
 */
LOGICAL _xgl_queryDataPNG(STRING fname, int *width, int *height, IMDEF *imdef)
{
	int         bit_depth, color_type;
	LOGICAL     status = FALSE;
	png_uint_32 w, h, n, ncomment;
	png_structp png_ptr  = NULL;
	png_infop   info_ptr = NULL;
	png_text   *text_ptr = NULL;
	FILE       *fp;

	fp = fopen(fname, BINARY_READ);
	if (!fp) return FALSE;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr)
	{
		if ((info_ptr = png_create_info_struct(png_ptr))!= NULL && setjmp(png_jmpbuf(png_ptr)) == 0)
		{
			png_init_io(png_ptr, fp);
			png_read_info(png_ptr, info_ptr);
			(void) png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, NULL, NULL, NULL);
			if(color_type != PNG_COLOR_TYPE_GRAY)
			{
				pr_error(ActiveModule, "Color type not PNG_COLOR_TYPE_GRAY in PNG data file \"%s\".\n", fname);
			}
			else if(bit_depth != 8 && bit_depth != 16)
			{
				pr_error(ActiveModule, "Invalid bit depth of %d in PNG data file \"%s\".\n", bit_depth,fname);
			}
			else
			{
				*width  = (int) w;
				*height = (int) h;
				if(png_get_channels(png_ptr, info_ptr) == 1)
				{
					status = TRUE;

					/* The data image will be returned in the native endian order - see readDataPNG */
					imdef->grid.byte_order = MACHINE_ENDIAN;
					imdef->grid.bpp        = (bit_depth == 16)? 2:1;

					/* check for scale and offset values stored as comments */
					ncomment = png_get_text(png_ptr, info_ptr, &text_ptr, NULL);
					for(n = 0; n < ncomment; n++)
					{
						float val;
						if(same_ic(text_ptr[n].key, PNGTAG_DATASCALE))
						{
							if(sscanf(text_ptr[n].text, "%f", &val) == 1)
								imdef->grid.scale = val;
							else
								pr_error(ActiveModule, "Invalid scale value in PNG data file \"%s\".\n", fname);
						}
						else if(same_ic(text_ptr[n].key, PNGTAG_DATAOFFSET))
						{
							if(sscanf(text_ptr[n].text, "%f", &val) == 1)
								imdef->grid.offset = val;
							else
								pr_error(ActiveModule, "Invalid offset value in PNG data file \"%s\".\n", fname);
						}
						else if(same(text_ptr[n].key, PNGTAG_PROJECTION))
						{
							if(!_xgl_parse_projection(text_ptr[n].text, &imdef->mproj))
								pr_error(ActiveModule, "Invalid map projection in PNG data file \"%s\".\n", fname);
						}
						else if(same_ic(text_ptr[n].key, PNGTAG_MAPDEF))
						{
							if(!_xgl_parse_map_def(text_ptr[n].text, &imdef->mproj))
								pr_error(ActiveModule, "Invalid map definition in PNG data file \"%s\".\n", fname);
						}
					}
					imdef->grid.packed = (imdef->grid.scale != 1.0 || imdef->grid.offset != 0.0);
				}
			}
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	}
	(void) fclose(fp);
	return status;
}


/* Read a PNG file containing a grid of data values. If the bytes per pixel is
 * greater then 1, then the array will be returned in the native format of
 * the machine.
 */
LOGICAL _xgl_readDataPNG( ImagePtr im, UNCHAR **raster )
{
	int           nt, bit_depth, color_type;
	UNCHAR        *array = NULL;
	LOGICAL       lower_left = FALSE;
	FILE          *fp;
	png_uint_32   n, width, height, rowbytes;
	png_structp   png_ptr  = NULL;
	png_infop     info_ptr = NULL;
	png_text      *text_ptr = NULL;
	png_bytep     *row_pointers = NULL;
	
	fp = _xgl_open_image_file(im);
	if(!fp) return FALSE;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) goto readerr;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) goto readerr;

	if(setjmp(png_jmpbuf(png_ptr))) goto readerr;

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);
	(void) png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	/* Return the data in the native machine byte order */
	if(bit_depth > 8 && MACHINE_ENDIAN == IMAGE_LITTLE_ENDIAN)
	{
		png_set_swap(png_ptr);
		png_read_update_info(png_ptr, info_ptr);
	}

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	if((array = MEM(UNCHAR, height*rowbytes))  == NULL) goto rasterr;
	if((row_pointers = MEM(png_bytep, height)) == NULL) goto rasterr;

	/* If the origin is in the lower left we must reverse the extraction */
	nt = png_get_text(png_ptr, info_ptr, &text_ptr, NULL);
	for(n = 0; n < nt; n++)
	{
		if(!same_ic(text_ptr[n].key,PNGTAG_ORIGINLOCATION)) continue;
		lower_left = same_ic(text_ptr[n].text,PNGVAL_LOWERLEFT);
		break;
	}
	if(lower_left)
	{
		for(n = 0; n < height; n++)
			row_pointers[n] = array + rowbytes*(height-1-n);
	}
	else
	{
		for(n = 0; n < height; n++)
			row_pointers[n] = array + rowbytes*n;
	}

	png_read_image(png_ptr, row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	FREEMEM(row_pointers);

	*raster = array;
	return TRUE;

rasterr:
	(void) fprintf(stderr, "Memory allocation failure\n");
readerr:
	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	FREEMEM(row_pointers);
	FREEMEM(array);
	return FALSE;
}

#endif
