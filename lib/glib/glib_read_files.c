/***********************************************************************************/
/*
*	File:		glib_read_files.c
*
*	Purpose:	Contains functions to read the various recognized image file
*	            formats.
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
/***********************************************************************************/
#include <stdio.h>
#include <environ/environ.h>
#include "glib_private.h"


/* Image information storage structure
 */
typedef struct {
	int        encoding;		/* ImageEncodingGIF ... etc */
	int        group;			/* RadarGroup, DataGroup, ... etc */
	int        width, height;   /* image width and height */
	int        bands;           /* number of bands (SingleBand or TripleBand) */
	int        rast_fmt;		/* raster data type */
	ImageLUT   lut;				/* colour lookup table index */
	UNCHAR    *lut_remap;		/* array to remap image native lut to lut above */
	IMGRIDDAT *grid;			/* gridded image data */
	IMRADDATA *table; 			/* radar data table */
	MAP_PROJ  *mproj;			/* map projection */
} IDS;


/* Messages */
static STRING read_file_msg   = "Read error on file: %s\n";
static STRING mem_error_msg   = "Memory allocation failure.\n";



/*------------- Utility functions for use within this file ----------------*/

static void file_read_error(ImagePtr im)
{
	if(im->imdef)
	{
		STRING fname = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
    	pr_error(ActiveModule, read_file_msg, fname);
		FREEMEM(fname);
	}
	else
	{
    	pr_error(ActiveModule, read_file_msg, im->vtime);
	}
}


/* Convert an image already in rgb pixel raster format into a single byte per pixel
 * greyscale single band image. Due to this we need to create a transparent pixel
 * mask.
 */
static void convert_to_greyscale(ImagePtr im, UNCHAR **pix, UNCHAR **mask)
{
	int    n, size;
	UNCHAR *p, *d, *m;

	if(*pix == NULL) return;

	size = im->ow*im->oh;
	p = INITMEM(UNCHAR, size);
	if(p)
	{
		size_t mask_size = (size_t) MASK_SIZE(im->ow,im->oh);
		m = INITMEM(UNCHAR, mask_size);
		if (m)
			(void) memset(m, 0xff, mask_size);
		else
		{
			FREEMEM(p);
    		pr_error(ActiveModule, mem_error_msg);
			return;
		}
	}
	else
	{
    	pr_error(ActiveModule, mem_error_msg);
		return;
	}

	for(d = p, n = 0; n < size; n+=RASTER_BPP, d++)
	{
		UNCHAR *s = *pix + n;
		if(OPAQUE_PIXEL(s))
		{
			*d = CONVERT_TO_GRAY_PIXEL(s[0], s[1], s[2]);
		}
		else
		{
			UNSET_MASK_BIT(m,n);
		}
	}
	FREEMEM(*pix);
	*pix  = p;
	*mask = m;
}


/* This function masks out that part of the image that is not contained within
 * the radar scan circle and for single band images masks out those parts of
 * the pixel that are unwanted (probably contain overlay information).
 */
static LOGICAL crop_radar_image( ImagePtr im, UNCHAR **raster, UNCHAR **mask )
{
	int  xo, yo, y, ra, a, n, nt, nb, vt, vb;
	UNCHAR *rmsk, *rast, bit_mask;
	UNCHAR *px  = NULL;
	UNCHAR *msk = NULL;

	if(!im->imdef) return TRUE;
	if(!(im->imdef->radar.options & RadarCrop)) return TRUE;

	if((px = MEM(UNCHAR, im->ow*im->oh*(int)im->bands)) == NULL) goto err5;

	rast = *raster;

	ra = im->imdef->radar.diam / 2;
	xo = im->imdef->radar.orgx - ra;
	yo = im->imdef->radar.orgy - ra;

	if(im->bands == SingleBand)
	{
		LOGICAL use_mask = im->imdef->radar.options & RadarStripOverlayBits;
		rmsk = *mask;
		if((msk = INITMEM(UNCHAR, MASK_SIZE(im->ow, im->oh))) == NULL) goto err5;
		bit_mask = (UNCHAR)(use_mask? im->imdef->radar.mask : 0xFF);

		for( y = 0; y < ra; y++ )
		{
			n = ra - y - 1;
			a = (int)(SQRT((ra*ra - n*n)) + 0.5);
			nt = ((yo + y) * im->imdef->info.width + xo + ra - a);
			nb = ((yo + im->imdef->radar.diam - y - 1) * im->imdef->info.width + xo + ra - a);
			vt = y * im->imdef->radar.diam + ra - a;
			vb = (im->imdef->radar.diam - y - 1) * im->imdef->radar.diam + ra - a;
			for( n = 0; n < 2*a; n++, nt++, nb++, vt++, vb++ )
			{
				if(IsNull(rmsk) || MASK_BIT_SET(rmsk,nt)) SET_MASK_BIT(msk,vt);
				if(IsNull(rmsk) || MASK_BIT_SET(rmsk,nb)) SET_MASK_BIT(msk,vb);
				px[vt] = rast[nt] & bit_mask;
				px[vb] = rast[nb] & bit_mask;
			}
		}
		FREEMEM(*mask);
		*mask = msk;
	}
	else
	{
		UNCHAR *p = px;
		UNCHAR *e = px + im->ow*im->oh*3;
		while(p < e)
		{
			*p++ = T_RED;
			*p++ = T_GREEN;
			*p++ = T_BLUE;
		}

		for( y = 0; y < ra; y++ )
		{
			n = ra - y - 1;
			a = (int)(SQRT((ra*ra - n*n)) + 0.5);
			nt = ((yo + y) * im->imdef->info.width + xo + ra - a) * 3;
			nb = ((yo + im->imdef->radar.diam - y - 1) * im->imdef->info.width + xo + ra - a) * 3;
			vt = (y * im->imdef->radar.diam + ra - a) * 3;
			vb = ((im->imdef->radar.diam - y - 1) * im->imdef->radar.diam + ra - a) * 3;
			for( n = 0; n < a*6; n++ )
			{
				px[vt++] = rast[nt++];
				px[vb++] = rast[nb++];
			}
		}
	}
	FREEMEM(*raster);
	*raster = px;
	return TRUE;

err5:
	FREEMEM(px);
	FREEMEM(msk);
    pr_error(ActiveModule, mem_error_msg);
	return FALSE;
}


static void print_cmap(STRING fname, glCOLOR *cmap)
{
	int n;
	pr_diag("print_cmap","Colormap for file: %s\n", fname);
	for(n = 0; n < 256; n++)
	{
		pr_diag("print_cmap", "    cmap[%.3d]  %d  %d  %d\n", n, cmap[n].red, cmap[n].green, cmap[n].blue);
	}
}


/* Set the colour in the colour map matching the transparent colour to
 * the transparent pixel value. If requested, by the 'flag' entry in the
 * transparent glCOLOR structure being non-zero, find the colour in the 
 * colour map that is closest to the requested transparent colour and
 * use this.
 */
static void set_transparent_colour( IMDEF *imdef, glCOLOR *cmap)
{
	int i, n;
	glCOLOR colour;

	for(n = 0; n < imdef->info.ntransparent; n++)
	{
		if(imdef->info.closest_rgb[n])
		{
			int   ndx = 0;
			float distance = 1000000.0;

			for( i = 0; i < 256; i++ )
			{
				float r, g, b, dist;

				r = (float) cmap[i].red   - (float) imdef->info.transparent[n].red;
				g = (float) cmap[i].green - (float) imdef->info.transparent[n].green;
				b = (float) cmap[i].blue  - (float) imdef->info.transparent[n].blue;
				dist = r*r + g*g + b*b;
				if(dist < distance)
				{
					distance = dist;
					ndx = i;
				}
			}
			colour.red   = cmap[ndx].red;
			colour.green = cmap[ndx].green;
			colour.blue  = cmap[ndx].blue;
		}
		else
		{
			colour.red   = imdef->info.transparent[n].red;
			colour.green = imdef->info.transparent[n].green;
			colour.blue  = imdef->info.transparent[n].blue;
		}

		for( i = 0; i < 256; i++ )
		{
			if( cmap[i].red   != colour.red   ) continue;
			if( cmap[i].green != colour.green ) continue;
			if( cmap[i].blue  != colour.blue  ) continue;
			SET_TO_TRANSPARENT_COLOR(cmap[i]);
		}
	}
}


/*======================= Image File Access Functions ===========================*/

/*
*	Contains functions to deal with raster images in various
*   formats. Each format has three associated functions:
*
*   Is...File()   - Determines if file is of the type.
*   Get...Info()  - Gets image information from file.
*   Read...File() - Returns a memory allocated raster.
*/


/*------------------ FPA metafile format ---------------------*/ 

/*ARGSUSED*/
static LOGICAL is_fmf_file(STRING fname, IMDEF *dummy)
{
	return (find_meta_revision(fname) != NULL);
}


static LOGICAL get_fmf_info(STRING fname, IDS *info, IMDEF *imdef)
{
	int      n;
	LOGICAL  rtn = FALSE;
	METAFILE mf;

	set_metafile_input_mode(MetaRasterReadMetadata);
	mf = read_metafile(fname, NULL);
	set_metafile_input_mode(MetaRasterReadAll);
	if (!mf) return FALSE;

	for(n = 0; n < mf->numfld; n++)
	{
		RASTER ras;
		if(mf->fields[n]->ftype != FtypeRaster) continue;
		ras = mf->fields[n]->data.raster;
		if(!ras) break;
		if(ras->type == RASTER_EMPTY) break;
		rtn = TRUE;

		info->encoding = ImageEncodingFpaMetafile;
		info->width    = ras->nrow;
		info->height   = ras->ncol;

		if(ras->type == RASTER_RGB)
		{
			info->bands = TripleBand;
		}
		else if(ras->type == RASTER_GRAY)
		{
			info->bands = SingleBand;
		}
		else if(ras->type == RASTER_DATA)
		{
			info->group = DataGroup;
			info->bands = SingleBand;
			info->lut   = imdef->info.lut;
			info->mproj = ONEMEM(MAP_PROJ);
			copy_map_projection(info->mproj, &(mf->mproj));
			info->grid  = ONEMEM(IMGRIDDAT);
			info->grid->bpp        = ras->bpp;
			info->grid->scale      = 1;
			info->grid->offset     = 0;
			info->grid->packed     = False;
			info->grid->byte_order = MACHINE_ENDIAN;
		}
		else
		{
			rtn = FALSE;
		}
		break;
	}
	(void) destroy_metafile(mf);
	return rtn;
}


static LOGICAL get_fmf_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	int      n;
	LOGICAL  rtn = FALSE;
	STRING   fname = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
	METAFILE mf    = read_metafile(fname, NULL);

	FREEMEM(fname);
	if (!mf) return FALSE;

	for(n = 0; n < mf->numfld; n++)
	{
		RASTER ras;
		if(mf->fields[n]->ftype != FtypeRaster) continue;
		ras = mf->fields[n]->data.raster;
		if(!ras || !ras->grid) break;
		*raster = ras->grid;
		ras->grid = NULL;				/* So destroy_metafile will not free memory */
		if(ras->mask)
		{
			*mask = ras->mask->bits;
			ras->mask->bits = NULL;		/* So destroy_metafile will not free memory */
		}
		else if(ras->init)
		{
		}
		rtn = TRUE;
		break;
	}
	(void) destroy_metafile(mf);
	return rtn;
}


/*---------------------------- GIF Format --------------------------*/

/*ARGSUSED*/
static LOGICAL is_gif_file(STRING fname, IMDEF *dummy)
{
	return _xgl_isGIF(fname);
}


/* Get image information.
*/
static LOGICAL get_gif_info(STRING fname, IDS *info, IMDEF *imdef)
{
	int     i, width, height, transparent = -1;
	glCOLOR cmap[256];

	if(!_xgl_queryGIF(fname, &width, &height, NULL, cmap, &transparent, NULL)) return FALSE;

	if(imdef->info.print_cmaps)
		print_cmap(fname, cmap);

	if( transparent >= 0 && transparent < 256 )
		SET_TO_TRANSPARENT_COLOR(cmap[transparent]);

	if(IS_RADAR(imdef))
	{
		info->group = RadarGroup;

		/* If we do not want an opaque image we must set all occurances
		*  of the no data rgb value to transparent.
		*/
		if(imdef->radar.options & RadarBackgroundTransparent)
		{
			for( i = 0; i < 256; i++ )
			{
				if( cmap[i].red   != imdef->radar.no_data_rgb.red   ) continue;
				if( cmap[i].green != imdef->radar.no_data_rgb.green ) continue;
				if( cmap[i].blue  != imdef->radar.no_data_rgb.blue  ) continue;
				SET_TO_TRANSPARENT_COLOR(cmap[i]);
			}
		}
		imdef->info.width  = width;
		imdef->info.height = height;
		if(imdef->radar.options & RadarCrop)
			width = height = imdef->radar.diam;
	}
	else if(imdef->info.ntransparent)
	{
		set_transparent_colour(imdef, cmap);
	}

	if(FORCE_GREYSCALE(imdef))
	{
		for( i = 0; i < 256; i++ )
		{
			if(OPAQUE_COLOR(cmap[i])) CONVERT_TO_GRAY_COLOR(cmap[i]);
		}
	}

	info->encoding = ImageEncodingGIF;
	info->bands    = SingleBand;
	info->width    = width;
	info->height   = height;
	info->lut      = _xgl_create_lut(cmap, &info->lut_remap, NULL);

	return TRUE;
}


/* Read any generic gif file
*/
static LOGICAL get_gif_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	LOGICAL status = FALSE;

	if(IS_RADAR(im->imdef))
	{
		if(_xgl_readGIF(im, raster))
			status = crop_radar_image(im, raster, mask);
	}
	else
	{
		status = _xgl_readGIF(im, raster);
	}

	return status;
}



/*----------------- Data Portable Network Graphics Files -------------------*/

/*ARGSUSED*/
static LOGICAL is_dpg_file(STRING fname, IMDEF *dummy)
{
	return  _xgl_isDataPNG(fname);
}


static LOGICAL get_dpg_info(STRING fname, IDS *info, IMDEF *imdef)
{
	int width, height;

	if(!_xgl_queryDataPNG(fname, &width, &height, imdef)) return FALSE;

	info->encoding = ImageEncodingDataPNG;
	info->group    = DataGroup;
	info->bands    = SingleBand;
	info->width    = width;
	info->height   = height;
	info->lut      = imdef->info.lut;
	info->grid     = &imdef->grid;

	return TRUE;
}


/*ARGSUSED*/
static LOGICAL get_dpg_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	return _xgl_readDataPNG(im, raster);
}


/*------------------- Portable Network Graphics Files -----------------------*/

/*ARGSUSED*/
static LOGICAL is_png_file(STRING fname, IMDEF *dummy)
{
	return  _xgl_isPNG(fname);
}


/* Get image information.
*/
static LOGICAL get_png_info(STRING fname, IDS *info, IMDEF *imdef)
{
	int     i, bands, width, height;
	glCOLOR cmap[256];

	if(!_xgl_queryPNG(fname, imdef, &width, &height, &bands, cmap)) return FALSE;

	if(imdef->info.print_cmaps && bands == SingleBand)
		print_cmap(fname, cmap);

	if(IS_RADAR(imdef))
	{
		info->group        = RadarGroup;
		imdef->info.width  = width;
		imdef->info.height = height;

		if(bands == SingleBand && imdef->radar.options & RadarBackgroundTransparent)
		{
			for( i = 0; i < 256; i++ )
			{
				if( cmap[i].red   != imdef->radar.no_data_rgb.red   ) continue;
				if( cmap[i].green != imdef->radar.no_data_rgb.green ) continue;
				if( cmap[i].blue  != imdef->radar.no_data_rgb.blue  ) continue;
				SET_TO_TRANSPARENT_COLOR(cmap[i]);
			}
		}
		if(imdef->radar.options & RadarCrop)
		{
			width = height = imdef->radar.diam;
		}
	}
	else if(bands == SingleBand && imdef->info.ntransparent)
	{
		set_transparent_colour(imdef, cmap);
	}


	if(FORCE_GREYSCALE(imdef))
	{
		int i;
		bands = SingleBand;
		for( i = 0; i < 256; i++ )
		{
			if(OPAQUE_COLOR(cmap[i])) CONVERT_TO_GRAY_COLOR(cmap[i]);
		}
	}

	info->encoding = ImageEncodingPNG;
	info->bands    = bands;
	info->width    = width;
	info->height   = height;
	info->lut      = _xgl_create_lut(cmap, &info->lut_remap, NULL);

	return TRUE;
}



/*ARGSUSED*/
static LOGICAL get_png_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	UNCHAR *r;
	LOGICAL status = FALSE;

	if(IS_RADAR(im->imdef))
	{
		if(_xgl_readPNG(im, raster, mask) && crop_radar_image(im, raster, mask))
		{
			status = TRUE;
			if(im->bands == TripleBand && im->imdef->radar.options & RadarBackgroundTransparent)
			{
				/* Convert background value to transparent */
				for(r = *raster; r < *raster + im->ow*im->oh*3; r+=3)
				{
					if(r[0] != im->imdef->radar.no_data_rgb.red  ) continue;
					if(r[1] != im->imdef->radar.no_data_rgb.green) continue;
					if(r[2] != im->imdef->radar.no_data_rgb.blue ) continue;
					r[0] = T_RED;
					r[1] = T_GREEN;
					r[2] = T_BLUE;
				}
			}
		}
	}
	else if(im->bands == TripleBand && im->imdef->info.ntransparent)
	{
		if(_xgl_readPNG(im, raster, mask))
		{
			int n;
			status = TRUE;
			/* Convert background value to transparent */
			for(n = 0; n < im->imdef->info.ntransparent; n++)
			{
				for(r = *raster; r < *raster + im->ow*im->oh*3; r+=3)
				{
					if(r[0] != im->imdef->info.transparent[n].red  ) continue;
					if(r[1] != im->imdef->info.transparent[n].green) continue;
					if(r[2] != im->imdef->info.transparent[n].blue ) continue;
					r[0] = T_RED;
					r[1] = T_GREEN;
					r[2] = T_BLUE;
				}
			}
		}
	}
	else
	{
		status = _xgl_readPNG(im, raster, mask);
	}
	return status;
}


/*------------------- Tagged Information Format Files ------------------*/

/*ARGSUSED*/
static LOGICAL is_tif_file(STRING fname, IMDEF *dummy)
{
	return _xgl_isTIFF(fname);
}


static LOGICAL get_tif_info(STRING fname, IDS *info, IMDEF *imdef)
{
	int     i, bands, width, height;
	glCOLOR cmap[256];

	if(!_xgl_queryTIFF(fname, &width, &height, &bands, cmap)) return FALSE;

	if(imdef->info.print_cmaps && bands == SingleBand)
		print_cmap(fname, cmap);

	if(IS_RADAR(imdef))
	{
		info->group        = RadarGroup;
		imdef->info.width  = width;
		imdef->info.height = height;

		if(bands == SingleBand && imdef->radar.options & RadarBackgroundTransparent)
		{
			for( i = 0; i < 256; i++ )
			{
				if( cmap[i].red   != imdef->radar.no_data_rgb.red   ) continue;
				if( cmap[i].green != imdef->radar.no_data_rgb.green ) continue;
				if( cmap[i].blue  != imdef->radar.no_data_rgb.blue  ) continue;
				SET_TO_TRANSPARENT_COLOR(cmap[i]);
			}
		}
		if(imdef->radar.options & RadarCrop)
		{
			width = height = imdef->radar.diam;
		}
	}
	else if(bands == SingleBand && imdef->info.ntransparent)
	{
		set_transparent_colour(imdef, cmap);
	}

	if(FORCE_GREYSCALE(imdef))
	{
		int    i;
		bands = SingleBand;
		for( i = 0; i < 256; i++ )
		{
			if(OPAQUE_COLOR(cmap[i])) CONVERT_TO_GRAY_COLOR(cmap[i]);
		}
	}

	info->encoding = ImageEncodingTIFF;
	info->bands    = bands;
	info->width    = width;
	info->height   = height;
	info->lut      = _xgl_create_lut(cmap, &info->lut_remap, NULL);

	return TRUE;
}



/*ARGSUSED*/
static LOGICAL get_tif_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	UNCHAR *r;
	LOGICAL status = FALSE;

	if(IS_RADAR(im->imdef))
	{
		if(_xgl_readTIFF(im, raster, mask) && crop_radar_image(im, raster, mask))
		{
			status = TRUE;
			if(im->bands == TripleBand && im->imdef->radar.options & RadarBackgroundTransparent)
			{
				/* Convert background value to transparent */
				for(r = *raster; r < *raster + im->ow*im->oh*3; r+=3)
				{
					if(r[0] != im->imdef->radar.no_data_rgb.red  ) continue;
					if(r[1] != im->imdef->radar.no_data_rgb.green) continue;
					if(r[2] != im->imdef->radar.no_data_rgb.blue ) continue;
					r[0] = T_RED;
					r[1] = T_GREEN;
					r[2] = T_BLUE;
				}
			}
		}
	}
	else if(im->bands == TripleBand && im->imdef->info.ntransparent)
	{
		if(_xgl_readTIFF(im, raster, mask))
		{
			int n;
			status = TRUE;
			/* Convert background value to transparent */
			for(n = 0; n < im->imdef->info.ntransparent; n++)
			{
				for(r = *raster; r < *raster + im->ow*im->oh*3; r+=3)
				{
					if(r[0] != im->imdef->info.transparent[n].red) continue;
					if(r[1] != im->imdef->info.transparent[n].green) continue;
					if(r[2] != im->imdef->info.transparent[n].blue) continue;
					r[0] = T_RED;
					r[1] = T_GREEN;
					r[2] = T_BLUE;
				}
			}
		}
	}
	else
	{
		status = _xgl_readTIFF( im, raster, mask);
	}
	return status;
}


/*-------------------- X Window Dump Format ------------------------*/

/*ARGSUSED*/
static LOGICAL is_xwd_file( STRING fname, IMDEF *dummy )
{
	return _xgl_isXWDFile(fname);
}


static LOGICAL get_xwd_info( STRING fname, IDS *info, IMDEF *imdef )
{
	int bands;

	if(!_xgl_queryXWD(fname, &info->width, &info->height, &bands, NULL)) return FALSE;

	info->encoding = ImageEncodingXWD;
	info->bands    = (char) bands;
	if(FORCE_GREYSCALE(imdef)) info->bands = SingleBand;
	return TRUE;
}


static LOGICAL get_xwd_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	int     width, height, bands;
	UNCHAR  *pix = NULL;

	if(_xgl_readXWD(im, &width, &height, &bands, &pix ))
	{
		if(FORCE_GREYSCALE(im->imdef))
		{
			if(im->bands == SingleBand && bands != SingleBand)
			{
				convert_to_greyscale(im, &pix, mask);
			}
		}
		*raster = pix;
	}
	return NotNull(pix);
}


/*------------------- Xgl Library Format -------------------*/


/* Read in Xgl Library format. This consists of a 24 byte header block followed by the
*  image data. The file starts with "xglraster". This is followed by a byte that is the
*  type of image storage. The options are 'G','R','P' and are:
*
*     'G' = 1 byte per pixel (greyscale)
*     'R' = 3 bytes per pixel stored as RGB triplets: RGBRGBRGB....
*     'P' = 3 bytes per pixel stored as RGB stored in plane order: RRRR....GGGG....BBBB
*
*  This is followed by a block that holds the image size stored as width x height.
*  Note that the number of bytes in the image will be width x height x bytes per pixel
*
*  For example: "xglrasterR1024x768      "
*/

/*ARGSUSED*/
static LOGICAL is_xgl_file(STRING fname, IMDEF *dummy)
{
	int  w,h;
	char type;
	LOGICAL ok;
	FILE *fp = fopen(fname, BINARY_READ);

	if (!fp) return FALSE;

	ok = (fscanf(fp, "xglraster%c%dx%d", &type, &w, &h) == 3);
	if (ok)
	{
		type = (char)toupper((int)type);
		if( type != 'G' && type != 'R' && type != 'P' ) return FALSE;
	}
	(void) fclose(fp);
	return ok;
}



static LOGICAL get_xgl_info( STRING fname, IDS *info, IMDEF *imdef )
{
	LOGICAL ok;
	char type;
	FILE *fp = fopen(fname, BINARY_READ);
	
	if (!fp) return FALSE;

	ok = (fscanf(fp, "xglraster%c%dx%d", &type, &info->width, &info->height) == 3 );
	(void) fclose(fp);
	if (!ok) return FALSE;

	type = (char)toupper((int)type);
	if( type != 'G' && type != 'R' && type != 'P' ) return FALSE;
	
	info->encoding = ImageEncodingXGL;

	if(FORCE_GREYSCALE(imdef))
		info->bands = SingleBand;
	else
		info->bands = (type == 'G') ? SingleBand:TripleBand;

	return TRUE;
}


/* Return an XGL style raster in RGB triple format
*/
static LOGICAL get_xgl_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	int     bands;
	size_t  size;
	char    type, buf[25];
	UNCHAR  *pix = NULL;
	FILE    *fp  = _xgl_open_image_file(im);

	if (!fp) return FALSE;

	buf[24] = '\0';
	(void)fread((void*)buf, 1, 24, fp);
	type = (char)toupper((int)buf[9]);
	bands = (type == 'G')? SingleBand : TripleBand;
	size = (size_t)(im->ow*im->oh*bands);
	pix = MEM(UNCHAR, size);
	if (pix)
	{
		if(type == 'P')
		{
			int  i,j;
			for(i = 0; i < RASTER_BPP; i++)
			{
				for(j = i; j < (int) size; j+=RASTER_BPP) pix[j] = (UNCHAR)getc(fp);
			}
		}
		else
		{
			if( fread(pix, 1, size, fp) != size )
			{
				FREEMEM(pix);
				file_read_error(im);
				(void) fclose(fp);
				return FALSE;
			}
		}
	}
	else
	{
    	pr_error(ActiveModule, mem_error_msg);
		(void) fclose(fp);
		return FALSE;
	}

	if(FORCE_GREYSCALE(im->imdef))
	{
		/* original image must not be greyscale to call convert */
		if(im->bands == SingleBand && type != 'G')
		{
			convert_to_greyscale(im, &pix, mask);
		}
	}
	(void) fclose(fp);

	*raster = pix;
	return NotNull(pix);
}


/*------------------ Raw Unencoded Graphics Rasters --------------------*/


/*ARGSUSED*/
static LOGICAL is_raw_file(STRING fname, IMDEF *imdef)
{
	int    bands;
	struct stat sb;

	if(!imdef)
		return FALSE;
	if(imdef->info.encoding != ImageEncodingNone && imdef->info.encoding != ImageEncodingRGB)
		return FALSE;
	if(imdef->info.width < 1 || imdef->info.height < 1)
	{
		pr_warning(ActiveModule, "Width or Height < 1 for unencoded image file\n");
		return FALSE;
	}
	bands = (imdef->info.encoding == ImageEncodingRGB)? TripleBand : SingleBand;
	if(stat(fname,&sb) != 0 || sb.st_size != imdef->info.width*imdef->info.height*bands)
	{
		pr_error(ActiveModule,
			"File size (%d) and given image size %d x %d x %d (width,height,depth) inconsistent.\n",
			sb.st_size, imdef->info.width, imdef->info.height, bands);
		return FALSE;
	}
	return TRUE;
}


/*ARGSUSED*/
static LOGICAL get_raw_info( STRING fname, IDS *info, IMDEF *imdef )
{
	if(!imdef) return FALSE;
	if(imdef->info.encoding != ImageEncodingNone && imdef->info.encoding != ImageEncodingRGB)
		return FALSE;
	if(imdef->info.width < 1 || imdef->info.height < 1) return FALSE;

	info->encoding = imdef->info.encoding;
	info->bands    = (info->encoding == ImageEncodingRGB)? TripleBand : SingleBand;
	info->lut      = imdef->info.lut;
	info->width    = imdef->info.width;
	info->height   = imdef->info.height;

	if(IS_RADAR(imdef))
	{
		if(info->lut == glNoLUT)
		{
			int i;
			glCOLOR cmap[256];
			for(i=0; i<256; i++) cmap[i].red = cmap[i].green = cmap[i].blue =(UNCHAR)i;
			info->lut = _xgl_create_lut(cmap, NULL, NULL);
		}
		if(!imdef->radar.options & RadarCrop)
		{
			info->width = info->height = imdef->radar.diam;
		}
	}

	return TRUE;
}


static LOGICAL get_raw_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	size_t size;
	UNCHAR *pix = NULL;
	UNCHAR *msk = NULL;
	FILE    *fp = _xgl_open_image_file(im);

	if (!fp) return FALSE;

	size = (size_t)(im->ow*im->oh*(int)im->bands);
	pix = MEM(UNCHAR, size);
	if (pix)
	{
		if( fread((void*)pix, BYTESIZE, size, fp) != size )
		{
			FREEMEM(pix);
			file_read_error(im);
		}
		else if(IS_RADAR(im->imdef))
		{
			if(!crop_radar_image(im, &pix, &msk))
			{
				FREEMEM(pix);
				FREEMEM(msk);
			}
		}
	}
	else
	{
    	pr_error(ActiveModule, mem_error_msg);
	}
	*raster = pix;
	*mask   = msk;
	(void) fclose(fp);

	return NotNull(pix);
}


/*------------------- Raw Unencoded Data Raster Files -------------------*/

/*ARGSUSED*/
static LOGICAL is_dat_file(STRING fname, IMDEF *imdef)
{
	struct stat sb;

	if(!imdef) return FALSE;
	if(imdef->info.encoding != ImageEncodingGridded) return FALSE;
	if(imdef->info.width < 1 || imdef->info.height < 1)
	{
		pr_warning(ActiveModule, "Width or Height < 1 for unencoded image file\n");
		return FALSE;
	}

	/* only 1 to 4 bytes per pixel is recognized. We check for bits per pixel
	 * just in case someone mistook the bpp
	 */
	if(imdef->grid.bpp%8 == 0) imdef->grid.bpp /= 8;

	if(imdef->grid.bpp < 1 || imdef->grid.bpp > 4)
	{
		pr_error(ActiveModule, "bytes_per_pixel is %d. Only 1 to 4 is valid.\n", imdef->grid.bpp);
		return FALSE;
	}

	if(stat(fname,&sb) != 0)
	{
		pr_error(ActiveModule,"Image file cannot be accessed.\n");
		return FALSE;
	}
	if(sb.st_size != imdef->info.width*imdef->info.height*imdef->grid.bpp)
	{
		pr_error(ActiveModule,
			"File size (%d) and specified size (%d x %d x (%d bytes/pixel)) not consistent.\n",
			sb.st_size, imdef->info.width, imdef->info.height, imdef->grid.bpp);
		return FALSE;
	}
	return TRUE;
}


/*ARGSUSED*/
static LOGICAL get_dat_info( STRING fname, IDS *info, IMDEF *imdef )
{
	if(!imdef) return FALSE;
	if(imdef->info.encoding != ImageEncodingGridded) return FALSE;
	if(imdef->info.width < 1 || imdef->info.height < 1) return FALSE;

	info->encoding = ImageEncodingGridded;
	info->group    = DataGroup;
	info->bands    = SingleBand;
	info->lut      = imdef->info.lut;
	info->width    = imdef->info.width;
	info->height   = imdef->info.height;
	info->grid     = &imdef->grid;

	return TRUE;
}


/*ARGSUSED*/
static LOGICAL get_dat_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	size_t size;
	UNCHAR *pix = NULL;
	FILE    *fp = _xgl_open_image_file(im);

	if (!fp) return FALSE;

	size = (size_t)(im->ow*im->oh*im->info.grid->bpp);
	pix = MEM(UNCHAR, size);
	if (pix)
	{
		if( fread((void*)pix, BYTESIZE, size, fp) != size )
		{
			FREEMEM(pix);
			file_read_error(im);
		}
	}
	else
	{
    	pr_error(ActiveModule, mem_error_msg);
	}
	*raster = pix;
	(void) fclose(fp);
	return NotNull(pix);
}


/*--------------- Gridded URP Radar Data -----------------------*/

static LOGICAL is_gur_file(STRING fname, IMDEF *imdef)
{
	if (!imdef) return FALSE;	/* Radar data files must have an associated imdef */
	return _xgl_isGridURP(fname);
}


static LOGICAL get_gur_info( STRING fname, IDS *info, IMDEF *imdef )
{
	glCOLOR   cmap[256];
	IMRADDATA *table;

	if(!imdef) return FALSE;

	if(!_xgl_queryGridURP(fname, cmap, imdef, &table)) return FALSE;

	info->encoding = ImageEncodingGriddedURP;
	info->group    = RadarGroup;
	info->bands    = SingleBand;
	info->width    = imdef->info.width;
	info->height   = imdef->info.height;
	info->table    = table;

	if(imdef->info.lut != glNoLUT)
		info->lut = imdef->info.lut;
	else
		info->lut = _xgl_create_lut(cmap, NULL, NULL);

	return TRUE;
}


/*ARGSUSED*/
static LOGICAL get_gur_raster(ImagePtr im, UNCHAR **rast, UNCHAR **msk)
{
	return _xgl_get_gridded_urp_raster(im, rast);
}



/* pur = Polar URP Radar Data */

static LOGICAL is_pur_file(STRING fname, IMDEF *imdef)
{
	if (!imdef) return FALSE;	/* Radar data files must have an associated imdef */
	return _xgl_isPolarURP(fname);
}


/*---------------------- Polar Format URP data files ---------------------------*/

static LOGICAL get_pur_info( STRING fname, IDS *info, IMDEF *imdef )
{
	glCOLOR   cmap[256];
	IMRADDATA *table;

	if(!imdef) return FALSE;

	if(!_xgl_queryPolarURP(fname, cmap, imdef, &table)) return FALSE;

	info->encoding = ImageEncodingPolarURP;
	info->group    = RadarGroup;
	info->bands    = SingleBand;
	info->width    = imdef->info.width;
	info->height   = imdef->info.height;
	info->table    = table;

	if(NotNull(table) && table->encode == AsciiFloat)
		info->rast_fmt = FloatType;

	if(imdef->info.lut != glNoLUT)
		info->lut = imdef->info.lut;
	else
		info->lut = _xgl_create_lut(cmap, NULL, NULL);

	return TRUE;
}


/*ARGSUSED*/
static LOGICAL get_pur_raster(ImagePtr im, UNCHAR **rast, UNCHAR **msk)
{
	return _xgl_get_polar_urp_raster(im, rast);
}


/*========================= End of image decode section ===========================*/



/* Here we define the graphics extensions we will recognize, the function used
*  to determine if the file is actually what we expect, the base image data
*  extraction function and the function which returns the image raster from
*  the image file.
*/
static struct {
	enum IMAGE_ENCODING encoding;			/* from glib.h */
	STRING  label;							/* to be used for error messages */
	LOGICAL detectable;						/* can type be determined from the test_fcn? */
	STRING  ext;							/* comma separated list of recognized extensions */
	int     ort;							/* original raster retrevial type */
	LOGICAL (*test_fcn)(STRING,IMDEF*);		/* test to see if it really is the image type */
	LOGICAL (*info_fcn)(STRING,IDS*,IMDEF*);/* get information about the image */
} im_type[] = {
	{ ImageEncodingFpaMetafile, "FPA Metafile", TRUE,  "",              ORT_FMF, is_fmf_file, get_fmf_info },
	{ ImageEncodingGIF,         "GIF",          TRUE,  "gif",           ORT_GIF, is_gif_file, get_gif_info },
	{ ImageEncodingXWD,         "XWD",          TRUE,  "xwd",           ORT_XWD, is_xwd_file, get_xwd_info },
	{ ImageEncodingXGL,         "XGL",          TRUE,  "xgl",           ORT_XGL, is_xgl_file, get_xgl_info },
	{ ImageEncodingNone,        "None",         FALSE, "im",            ORT_RSB, is_raw_file, get_raw_info },
	{ ImageEncodingRGB,         "RGB",          FALSE, "rgb",           ORT_RGB, is_raw_file, get_raw_info },
	{ ImageEncodingGridded,     "Gridded",      FALSE, "dat,data,grid", ORT_DAT, is_dat_file, get_dat_info },
	{ ImageEncodingGriddedURP,  "Gridded URP",  TRUE,  "urp",           ORT_GUR, is_gur_file, get_gur_info },
	{ ImageEncodingPolarURP,    "R-Theta URP",  TRUE,  "urp",           ORT_PUR, is_pur_file, get_pur_info },
	{ ImageEncodingPNG,         "PNG",          TRUE,  "png",           ORT_PNG, is_png_file, get_png_info },
	{ ImageEncodingDataPNG,     "Data PNG",     TRUE,  "dpng,dpg,png",  ORT_DPG, is_dpg_file, get_dpg_info },
	{ ImageEncodingTIFF,        "TIFF",         TRUE,  "tiff,tif",      ORT_TIF, is_tif_file, get_tif_info }
};

#define N_IM_TYPE     (int)(sizeof(im_type)/sizeof(im_type[0]))

/*========== Functions visible within the library ===============*/

Image _xgl_get_image( STRING fname, enum IMAGE_ENCODING encoding, IMDEF *imdef )
{
	int      i, ort = -1;
	STRING   p;
	ImagePtr im;
	Image    ndx;
	IDS      info;

	/* Initialize information structure */
	info.encoding  = ImageEncodingAny;
	info.group     = GenericGroup;
	info.width     = 0;
	info.height    = 0;
	info.bands     = TripleBand;
	info.rast_fmt  = ByteType; 
	info.lut       = glNoLUT;
	info.lut_remap = (UNCHAR *)0;
	info.grid      = (IMGRIDDAT *)0;
	info.table     = (IMRADDATA *)0;
	info.mproj     = (MAP_PROJ *)0;


	/* Does file exist and is it readable? */
	if(access(fname, R_OK)) return glNoImage;

	if(encoding != ImageEncodingAny)
	{
		for( i = 0; i < N_IM_TYPE; i++ )
		{
			if(im_type[i].encoding != encoding) continue;
			if(!im_type[i].test_fcn(fname, imdef))
			{
				pr_error(ActiveModule, "Specified encoding \"%s\" and file contents do not match.\n",
						im_type[i].label);
				return glNoImage;
			}
			if(!im_type[i].info_fcn(fname, &info, imdef)) return glNoImage;
			ort = im_type[i].ort;
			break;
		}
	}
	else
	{
		/* Check the file extension first.
		 */
		if((p = strrchr(fname,'.')) != NULL)
		{
			p++;
			for( i = 0; i < N_IM_TYPE; i++ )
			{
				char buf[24], *ptr;
				(void) safe_strcpy(buf,im_type[i].ext);
				ptr = strtok(buf,",");
				while(ptr)
				{
					if(same_ic(p,ptr))
					{
						if(im_type[i].test_fcn(fname, imdef) && im_type[i].info_fcn(fname, &info, imdef))
							ort = im_type[i].ort;
						i = N_IM_TYPE;
						break;
					}
					ptr = strtok(NULL,",");
				}
			}
		}

		/* If unrecognized run through all our file type functions.
		*/
		if( ort < 0 )
		{
			for( i = 0; i < N_IM_TYPE; i++ )
			{
				if(!im_type[i].detectable) continue;
				if(!im_type[i].test_fcn(fname, imdef)) continue;
				if(!im_type[i].info_fcn(fname, &info, imdef)) return glNoImage;
				ort = im_type[i].ort;
				break;
			}
		}
	}

	if(ort < 0)
	{
		pr_error(ActiveModule, "Unable to determine the image file type\n");
		return glNoImage;
	}

	ndx = _xgl_make_image(FileImage);
	if (!ndx) return glNoImage;

	im = Xgl.images[ndx];

	/* Transfer the image information to our image structure.
	*/
	im->group     = info.group;
	im->ow        = info.width;
	im->oh        = info.height;
	im->encoding  = info.encoding;
	im->bands     = info.bands;
	im->rast_fmt  = info.rast_fmt;
	im->imdef     = imdef;
	im->ort       = ort;
	im->dlut      = info.lut;
	im->lut       = info.lut;
	im->lut_remap = info.lut_remap;

	if (info.grid ) im->info.grid  = info.grid;
	if (info.table) im->info.radar = _xgl_create_radar_data_table(info.table);

	/* Only valid if we have an image definition that exists */
	if(imdef)
	{
		MAP_PROJ mproj;

		im->dlut = (info.lut == glNoLUT)? imdef->info.dlut : info.lut;
		/* 20100407: Changed to add the info.lut check */
		im->lut  = (imdef->info.lut != glNoLUT)? imdef->info.lut : ((info.lut != glNoLUT)? info.lut:im->dlut);

		/* The map projection has a pecking order. First is anything in the image
		 * file itself, second is a projection information file and last is the
		 * image configuration file.
		 */
		if(info.mproj)
		{
			copy_map_projection(&(im->mproj),     info.mproj);
			copy_map_projection(&(im->mproj_org), info.mproj);
			FREEMEM(info.mproj);
		}
		else if(_xgl_read_image_map_projection_file(ActiveModule, im, &mproj))
		{
			copy_map_projection(&(im->mproj),     &mproj);
			copy_map_projection(&(im->mproj_org), &mproj);
		}
		else
		{
			copy_map_projection(&(im->mproj),     &(imdef->mproj));
			copy_map_projection(&(im->mproj_org), &(imdef->mproj));
		}
	}
	else if(info.mproj)
	{
		copy_map_projection(&(im->mproj),     info.mproj);
		copy_map_projection(&(im->mproj_org), info.mproj);
	}

	return (ndx);
}



/* Read and return the raster and mask from the original source image.
 */
LOGICAL _xgl_get_source_image( ImagePtr im, UNCHAR **raster, UNCHAR **mask )
{
	*raster = (UNCHAR*)0;
	*mask   = (UNCHAR*)0;

	switch(im->ort)
	{
		case ORT_FMF: return  get_fmf_raster (im, raster, mask);
		case ORT_GIF: return  get_gif_raster (im, raster, mask);
		case ORT_PNG: return  get_png_raster (im, raster, mask);
		case ORT_DPG: return  get_dpg_raster (im, raster, mask);
		case ORT_TIF: return  get_tif_raster (im, raster, mask);
		case ORT_XWD: return  get_xwd_raster (im, raster, mask);
		case ORT_XGL: return  get_xgl_raster (im, raster, mask);
		case ORT_RSB: return  get_raw_raster (im, raster, mask);
		case ORT_RGB: return  get_raw_raster (im, raster, mask);
		case ORT_DAT: return  get_dat_raster (im, raster, mask);
		case ORT_GUR: return  get_gur_raster (im, raster, mask);
		case ORT_PUR: return  get_pur_raster (im, raster, mask);
		case ORT_RPG: return _xgl_rpg_raster (im, raster, mask);
	}
	return FALSE;
}
