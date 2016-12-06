/***********************************************************************************/
/**
*	@file	glib_image.c
*
*	Functions to deal with the manipulation of raster images. 
*/
/*
* This file is large so to aid in maintanance it is divided into
* functional sections.
*
* Some source images can come with a transparency mask. This is
* associated with images that are single band. Triple band images
* have one colour used for transparent so we loose one in 16M but
* greyscale only has 256 possible values and is usually pseudo-colour
* so that using one value could lead to colour table conflicts. In
* this case the mask is used. For single band images where there is
* an explicit transparent defined, a colour table entry is used to
* hold the transparent colour as there will be no conflict.
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
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#ifdef MACHINE_PCLINUX
#include <png.h>
#include <zlib.h>
#else
#include "glib_lzf.h"
#endif
#include <fpa_types.h>
#include <fpa_math.h>
#include <environ/environ.h>
#include <tools/tools.h>
#include "glib_private.h"


/* Module name stack.
*/
#define ModuleStackLen 10
static STRING ModuleStack[] = { 0,0,0,0,0,0,0,0,0,0 };

/* Masks that are used to set and clear bits in the blend_value variable
 */
#define NoBlendMask			0x100
#define NoBlendModifiedMask	0x200
#define NoBlendAllMask		0x300
#define BlendValueMask		0xff
#define DoBlending(p)		(p->info.synth->blend_ratio<100)

/*  Values used in overlay function
 */
static LOGICAL insert_range_rings   = FALSE;
static int     range_ring_delta     = 40;
static UNCHAR  ring_colour[3]       = { 220,220,220 };
static UNCHAR  limit_ring_colour[3] = { 220,  0,  0 };

/* Variables used in raster management
 */
static int      raster_queue_max = 0;
static int      raster_queue_len = 0;
static ImagePtr *raster_queue    = NULL;


/* Brigntness settings for image types. Note that if the list of image
 * types changes in glib_private.h this will have to change as well.
 */
static float image_type_bsf[6] = {
	glNORMAL_BRIGHTNESS, glNORMAL_BRIGHTNESS, glNORMAL_BRIGHTNESS,
	glNORMAL_BRIGHTNESS, glNORMAL_BRIGHTNESS, glNORMAL_BRIGHTNESS
};

/* Multiple occurance messages
 */
static STRING lut_change_msg  = "Look up color change is only valid for pseudo color images.\n";
static STRING invalid_im_msg  = "Invalid image pointer parameter.\n";
static STRING handling_msg    = "Problem handling image file: \"%s\"\n";
static STRING mem_error_msg   = "Memory allocation failure.\n";
static STRING no_file_access  = "Unable to access image file \"%s\"\n";
static STRING no_image_create = "Unable to create image.\n";


/* Forward internal function declarations.
*/
static void    add_image_to_synth    (ImagePtr, ImagePtr, MAP_PROJ*);
static void    affine_run            (int*,int*,const int,const int,const int,const float affine[6]);
static void    blend_image           (ImagePtr, const ImagePtr);
static void    calculate_geometry    (ImagePtr);
static void    clear_raster_queue    (void);
static void    combine_image         (ImagePtr, const ImagePtr);
static void    copy_image            (ImagePtr, const ImagePtr);
static LOGICAL create_image          (ImagePtr);
static LOGICAL create_raster         (ImagePtr);
static void    remove_store_file     (ImagePtr);
static STRING  file_encode_base64    (FILE*);
static float   get_brightness_factor (ImagePtr);
static float   radar_proj_lat_lon_val(ImagePtr, float, float, int, UNCHAR*);
static float   get_radar_value       (ImagePtr, int, int, int, UNCHAR*, float inv[6]);
static LOGICAL image_to_png          (Image, FILE*);
static LOGICAL invalid_image         (Image);
static LOGICAL invalid_lut           (ImageLUT);
static void    invert_affine         (float dst[6], const float src[6]);
static void    lat_lon_sample_image  (Image, STRING, LatLonPoint*, int, float*);
static void    lat_long_to_radar_grid(float, float, float, float, float*, float*);
static void    lat_long_to_RAZ       (float, float, float, float, float*, float*);
static FILE   *open_store_file       (ImagePtr, STRING);
static void    reset_module_name     (void);
static void    sample_image          (Image, STRING, Coord*, Coord*, int, float*);
static void    scale_cmap_brightness (glCOLOR*, const float);
static void    scale_pixel_brightness(UNCHAR*, UNCHAR*, UNCHAR*, const float);
static void    set_blend_ratio       (ImagePtr, int);
static void    set_module_name       (STRING);
static void    scan_image_tree       (ImagePtr);
static void    transform_image       (ImagePtr, const UNCHAR*, const UNCHAR*,
										const int, const int, const float d[6]);
static float   value_from_pixel      (ImagePtr, const UNCHAR*, const int, const int);



/*======================= Public Functions ===================================*/


/******************************************************************************/
/**
*	@brief Destroy the given image
*
*	@param[in] ndx The image index
*/
/******************************************************************************/
void glImageDestroy( Image ndx )
{
	int      i, n, k;
	ImagePtr im;
	
	if( !valid_image(ndx) ) return;

	set_module_name(NULL);

	im = Xgl.images[ndx];

	/* NULL array pointer */
	Xgl.images[ndx] = (ImagePtr)0;

	/* Remove it from any synthetic images */
	for(i = 0; i < Xgl.nimages; i++)
	{
		if(!Xgl.images[i]) continue;
		if(Xgl.images[i]->group != SyntheticGroup) continue;
		for(n = 0; n < Xgl.images[i]->info.synth->nsrc; n++ )
		{
			if( Xgl.images[i]->info.synth->src[n] != im) continue;
			Xgl.images[i]->info.synth->nsrc--;
			for( k = n; k < Xgl.images[i]->info.synth->nsrc; k++)
				Xgl.images[i]->info.synth->src[k] = Xgl.images[i]->info.synth->src[k+1];
		}
	}

	remove_store_file(im);

	if(im->group == SyntheticGroup)
	{
		if(im->type == CompositeImage)
		{
			for(i = 0; i < im->info.synth->nsrc; i++)
				glImageDestroy(im->info.synth->src[i]->info.synth->ndx);
		}
		FREEMEM(im->info.synth->src);
		FREEMEM(im->info.synth);
	}
	else if(im->group == DataGroup)
	{
		if(im->info.grid != NULL && im->info.grid != &im->imdef->grid)
		{
			if(im->info.grid->element != im->imdef->grid.element)
				FREEMEM(im->info.grid->element);
			FREEMEM(im->info.grid);
		}
	}

	FREEMEM(im->raster);
	FREEMEM(im->vtime);
	FREEMEM(im->lut_remap);
	FREEMEM(im);

	/* Strip trailing NULL pointers from the images array */
	while(Xgl.nimages > 1 && Xgl.images[Xgl.nimages-1] == (ImagePtr)0) Xgl.nimages--;
	
	reset_module_name();
}



/******************************************************************************/
/*
*	@brief Destroy the given list of images
*
*	@param[in] list The list of image indexes.
*	@param[in] nlist The number of elements in the list.
*/
/******************************************************************************/
void glImageDestroyList( ImageList list, int nlist )
{
	int n;
	for(n=0; n<nlist; n++) glImageDestroy(list[n]);
}



/******************************************************************************/
/*
*	@brief Destroy all images
*/
/******************************************************************************/
void glImageDestroyAll( void )
{
	int    i, nlist;
	char   pid[32];
	STRING *list;

	for( i = 1; i < Xgl.nimages; i++ )
	{
		if(!Xgl.images[i]) continue;

		FREEMEM(Xgl.images[i]->raster);

		if(Xgl.images[i]->group == SyntheticGroup)
		{
			FREEMEM(Xgl.images[i]->info.synth->src);
			FREEMEM(Xgl.images[i]->info.synth);
		}
		else if(Xgl.images[i]->group == DataGroup)
		{
			if(Xgl.images[i]->info.grid != NULL && Xgl.images[i]->info.grid != &Xgl.images[i]->imdef->grid)
			{
				if(Xgl.images[i]->info.grid->element != Xgl.images[i]->imdef->grid.element)
					FREEMEM(Xgl.images[i]->info.grid->element);
				FREEMEM(Xgl.images[i]->info.grid);
			}
		}

		FREEMEM(Xgl.images[i]->vtime);
		FREEMEM(Xgl.images[i]->lut_remap);
		FREEMEM(Xgl.images[i]);
	}
	FREEMEM(Xgl.images);
	Xgl.nimages   = 0;
	Xgl.maximages = 0;

	/* Free raster queue list */
	FREEMEM(raster_queue);
	raster_queue_max = 0;
	raster_queue_len = 0;

	/* Free reprojected image working directory list */
	_xgl_free_reprojection_data();

	/* Remove all image files */
	(void) snprintf(pid, 32, "^i%d-", getpid());
	nlist = dirlist(Xgl.work_directory, pid, &list);
	for(i = 0; i < nlist; i++)
		(void) unlink(pathname(Xgl.work_directory,list[i]));
}



/******************************************************************************/
/*
*    @brief Set the origin and size of the image in image coordinates.
*
*    @param[in] ndx The image index
*    @param[in] x The x coordinate of the origin
*    @param[in] y The y coordinate of the origin
*    @param[in] width The width of the image
*    @param[in] height The height of the image
*/
/******************************************************************************/
void glImageGeometry( Image ndx, Coord x, Coord y, Coord width, Coord height )
{
	ImagePtr im;

	set_module_name("glImageGeometry");
	if (invalid_image(ndx)) return;

	im = Xgl.images[ndx];
	im->mx = x;
	im->my = y;
	/* if width or height less than 1 use the original image size */
	im->mw = (width  > 0)? width  : (Coord) im->ow;
	im->mh = (height > 0)? height : (Coord) im->oh;
	
	reset_module_name();
}


/******************************************************************************/
/*
*    @brief Set the origin and size of the image in device dependent coordinates.
*
*    @param[in] ndx The image index
*    @param[in] x The x coordinate of the origin
*    @param[in] y The y coordinate of the origin
*    @param[in] width The width of the image
*    @param[in] height The height of the image
*/
/******************************************************************************/
void glImageVdcGeometry( Image ndx, float x, float y, float width, float height )
{
	float sx, sy;
	ImagePtr im;

	set_module_name("glImageVdcGeometry");
	if (invalid_image(ndx)) return;

	im = Xgl.images[ndx];

	sx = (float)W->xm / W->Sx;   /* map unit per vdc unit in x direction */
	sy = (float)W->ym / W->Sy;   /* map unit per vdc unit in y direction */

	im->mx = x * sx;
	im->my = y * sy;
	im->mw = ((width  > 0)? width  : (float) im->ow) * sx;
	im->mh = ((height > 0)? height : (float) im->oh) * sy;
	
	reset_module_name();
}


/******************************************************************************/
/*
*	@brief Set the amount to rotate the image around a given origin specified
*	       in image coordinates.
*
*	@param[in] ndx The image index
*	@param[in] angle The angle in degrees to rotate the image (+ve clockwise).
*	@param[in] x The x position of the rotation origin.
*	@param[in] y The y position of the rotation origin.
*/
/******************************************************************************/
void glImageRotation( Image ndx, Angle angle, Coord x, Coord y )
{
	ImagePtr im;

	set_module_name("glImageRotation");
	if( invalid_image(ndx) ) return;

	im = Xgl.images[ndx];

	if(im->group != SyntheticGroup)
	{
		if( im->rx != x || im->ry != y ) im->opstat = ImageGenerate;
		im->rx = x;
		im->ry = y;

		/* Rotation is restricted to plus and minus 180 degrees.
		*/
		while(angle >  180) angle -= 360;
		while(angle < -180) angle += 360;

		/* Set explicitly to zero if close ( < .0001 degree ) This avoids problems
		*  with roundoff in calculated angles as .0001 degree is less than a one
		*  pixel rotation with anywhere near a normal display.
		*/
		if( fabsf(angle) < 0.0001 ) angle = 0.0;

		/* If different from old angle 
		*/
		if( angle != im->ra ) im->opstat = ImageGenerate;
		im->ra = angle;
	}
	reset_module_name();
}


/******************************************************************************/
/*
*	@brief Set the amount to rotate the image around a given origin specified
*	       in device dependent coordinates.
*
*	@param[in] ndx The image index
*	@param[in] angle The angle in degrees to rotate the image (+ve clockwise).
*	@param[in] x The x position of the rotation origin.
*	@param[in] y The y position of the rotation origin.
*/
/******************************************************************************/
void glImageVdcRotation( Image ndx, Angle angle, float x, float y )
{
	set_module_name("glImageVdcRotation");
	if( invalid_image(ndx) ) return;

	glImageRotation( ndx, angle, (Coord)(x*W->xm/W->Sx), (Coord)(y*W->ym/W->Sy));

	reset_module_name();
}



/******************************************************************************/
/*
*   @brief Fetch an image.
*
*   @param[in] tag The image identification tag
*   @param[in] vtime The valid time of the image
*   @param[in] target The projection into which to reproject the image from
*                     its native projection.
*
*   @return The image index of the fetched image.
*
*   @note
*
*   The fetched image is not realized by the fetch operation. This is done
*   only on display.
*
*   The valid time (vtime) is not used if the image associated with the tag
*   is geographic type and can thus be set to NULL in this case.
*/
/******************************************************************************/
Image glImageFetch( STRING tag, STRING vtime, MAP_PROJ *target)
{
	int n;
	IMDEF *imdef;
	Image ndx = glNoImage;

	set_module_name("glImageFetch");

	if (blank(tag)) return glNoImage;

	for( n = 0; n < Xgl.nimages; n++ )
	{
		ImagePtr im = Xgl.images[n];
		if (!im) continue;
		if (!im->imdef) continue;
		if (!im->imdef->prod) continue;
		if (!same(tag, im->imdef->tag)) continue;
		if (im->imdef->prod->image_type == ImageTypeGeographic) return((Image)n);
		if (same(vtime, im->vtime)) return((Image)n);
	}

	if((imdef = _xgl_image_info_definition(tag)))
	{
		STRING fname = _xgl_make_image_file_path(tag, vtime);
		ndx = _xgl_get_image(fname, imdef->info.encoding, imdef);
		if(ndx == glNoImage)
		{
			if (fname) pr_error(ActiveModule, handling_msg, fname);
		}
		else
		{
			Xgl.images[ndx]->vtime = safe_strdup(vtime);
			if(!_xgl_remap_image(Xgl.images[ndx], target))
			{
				pr_error(ActiveModule, "Unable to remap image \"%s\" from %s to %s\n", fname,
					which_projection_name(Xgl.images[ndx]->mproj_org.projection.type),
					which_projection_name(target->projection.type));
			}
		}
		FREEMEM(fname);
	}
	return ndx;
}



/******************************************************************************/
/*
*   @brief Fetch an image from the specified file.
*
*   @param[in] fname The image file.
*
*   @return The image index
*
*   @note Since the file is fetched directly there is no way to know what its
*         projection and map definition is. Thus the image can not be remapped
*         to another projection.
*/
/******************************************************************************/
Image glImageFetchFile( STRING fname )
{
	Image ndx;

	set_module_name("glImageFetchFile");

	ndx = _xgl_get_image( fname, ImageEncodingAny, NULL );
	if(ndx == glNoImage) {
		if (fname) pr_error(ActiveModule, handling_msg, fname);
	} else {
		Xgl.images[ndx]->vtime = safe_strdup(fname);
	}
	reset_module_name();
	return ndx;
}


/******************************************************************************/
/*
*  @brief Combine a list of images into one image.
*
*  @param[in] orig If glNoImage a new combination image will be created,
*                  otherwise the given combination image will be redefined
*                  with the given list of images.
*  @param[in] target The map projection to display the images in.
*  @param[in] imlist The list of images to combine together.
*  @param[in] nlist The number of elements in imlist.
*
*  @return The image index of the combination of images.
*
*/
/******************************************************************************/

Image glImageCombine( Image orig, MAP_PROJ *target, ImageList imlist, int nlist)
{
	int		 n;
	Image    ndx;
	ImagePtr im;

	set_module_name("glImageCombine");

	if(valid_image(orig)) /* redefine an existing image */
	{
		ndx = orig;
		im = Xgl.images[ndx];
		if( im->type != CombinedImage )
		{
			pr_error(ActiveModule, "Parameter orig must be a combined image type.\n");
			ndx = 0;
		}
	}
	else if( orig == glNoImage ) /* Create a new image */
	{
		ndx = _xgl_make_image(CombinedImage);
		if( ndx )
		{
			im = Xgl.images[ndx];
			im->group  = SyntheticGroup;
			im->opstat = ImageNotVisible;
			im->info.synth = ONEMEM(IMSYNDATA); 
			im->info.synth->ndx = ndx;
			im->info.synth->image_type = ImageTypeUnknown;
		}
	}
	else
	{
		ndx = 0;
		pr_error(ActiveModule, invalid_im_msg);
	}

	if(ndx)
	{
		if(nlist > im->info.synth->maxsrc)
		{
			im->info.synth->maxsrc = nlist;
			im->info.synth->src = GETMEM(im->info.synth->src, ImagePtr, nlist);
		}
		im->info.synth->nsrc = 0;
		for( n = 0; n < nlist; n++ )
		{
			if(valid_image(imlist[n]))
				add_image_to_synth(im, Xgl.images[imlist[n]], target);
		}
	}

	reset_module_name();
	return ndx;
}


/******************************************************************************/
/*
*  @brief Combine two images and blend them into one another.
*
*  @param[in] orig If glNoImage a new blended image will be created, otherwise
*                  the given blended image will be redefined with the given
*                  images.
*  @param[in] target The map projection to display the images in.
*  @param[in] im1 The first image to blend.
*  @param[in] im2 The second image to blend.
*  @param[in] ratio The blending ratio of the two images in percent [0-100].
*
*  @return The image index of the blended image.
*/
/******************************************************************************/
Image glImageBlend( Image orig, MAP_PROJ *target, Image im1, Image im2, int ratio)
{
	int      ndx;
	ImagePtr im;

	set_module_name("glImageBlend");

	if( ratio < 0 || ratio > 100)
		pr_error(ActiveModule, "Blend ratio \"%d\" out of range 0-100.\n", ratio);

	if(valid_image(orig))	/* redefine an existing image */
	{
		ndx = orig;
		im = Xgl.images[ndx];
		if( im->type != BlendedImage )
		{
			ndx = 0;
			pr_error(ActiveModule, "Parameter orig must be a blended image type.\n");
		}
	}
	else if(orig == glNoImage)	/* create a new image */
	{
		ndx = _xgl_make_image(BlendedImage);
		if( ndx )
		{
			im = Xgl.images[ndx];
			im->group = SyntheticGroup;
			im->opstat = ImageNotVisible;
			im->info.synth = ONEMEM(IMSYNDATA);
			im->info.synth->ndx = ndx;
			im->info.synth->image_type = ImageTypeUnknown;
			im->info.synth->blend_ratio = 100;
			im->info.synth->src = INITMEM(ImagePtr, 2);
			im->info.synth->maxsrc = 2;
		}
	}
	else
	{
		ndx = 0;
		pr_error(ActiveModule, invalid_im_msg);
	}

	if(ndx)
	{
		set_blend_ratio(im, ratio);
		im->info.synth->nsrc = 0;
		if(valid_image(im1))
			add_image_to_synth(im,  Xgl.images[im1], target);
		if(im1 != im2 && valid_image(im2))
			add_image_to_synth(im,  Xgl.images[im2], target);
	}
	reset_module_name();
	return ndx;
}


/******************************************************************************/
/*
 *  @brief Create a composite of images such that different image types appear
 *         to be on different display planes. The geographic image plane is on
 *         on the bottom and is covered up by the underlay plane. The satellite
 *         plane covers up both the geographic and underlay plane, the radar
 *         plane covers up the geographic, underlay and satellite planes, and
 *         the overlay plane covers up everything. Within each plane individual
 *         images will cover up other images depending on their order as read
 *         from the image configuration file. Of course any transparency in
 *         an overlying plane will allow the underlying planes to show through.
 *
 *  @param[in] orig     If glNoImage, a new composite image will be created,
 *                      otherwise the given composite image will be redefined
 *                      with the given images.
 *  @param[in] map_proj Map projection of the final image.
 *  @param[in] imlist   The list of images to combine.
 *  @param[in] nlist    The number of images in imlist.
 *  @param[in] ratio    The blending ratio, in percent, for satellite and radar.
 *                      100% means that the radar will completely cover the
 *                      satellite.
 *
 *  @return The composite image identifier.
 *
 *  @note Any image not classified as overlay, radar, satellite or underlay
 *        is put into the geographic plane.
 */
/******************************************************************************/
Image glImageComposite(Image orig, MAP_PROJ *map_proj, ImageList imlist, int nlist, int ratio)
{
	int i;
	Image ndx;
	ImagePtr im;

	static int	 listmax[5] = {0,0,0,0,0};
	static Image *list[5]   = {NULL,NULL,NULL,NULL,NULL};

	set_module_name("glImageComposite");

	/* Note that the info.synth src arrays correspond to the image types as:
	 * src[0] - geographic
	 * src[1] - underlay
	 * src[2] - satellite
	 * src[3] - radar
	 * src[4] - overlay
	 * src[5] - an internal blend used for satellite and radar
	 *
	 * If this assignment changes then the function create_composite_image() needs
	 * to change as well as the values are hard coded.
	 */

	if(valid_image(orig)) /* redefine an existing image */
	{
		ndx = orig;
		im = Xgl.images[ndx];
		if( im->type != CompositeImage )
		{
			pr_error(ActiveModule, "Parameter orig must be a  composite type.\n");
			ndx = 0;
		}
	}
	else if( orig == glNoImage )
	{	/*
		 *  Create a new image. The sub-sources are predefined on creation as they
		 *  do not change during the life of this object.
		 */
		if((ndx = _xgl_make_image(CompositeImage)))
		{
			im = Xgl.images[ndx];
			im->group  = SyntheticGroup;
			im->opstat = ImageNotVisible;
			im->info.synth = ONEMEM(IMSYNDATA); 
			im->info.synth->ndx = ndx;
			im->info.synth->image_type = ImageTypeUnknown;
			im->info.synth->blend_ratio = 100; 
			im->info.synth->nsrc = 6;
			im->info.synth->maxsrc = 6;
			im->info.synth->src = INITMEM(ImagePtr,6);
			/* Create the component parts and assign source pointers */
			for(i = 0; i < 5; i++)
				im->info.synth->src[i] = Xgl.images[glImageCombine(glNoImage,map_proj,NULL,0)];
			/* The radar and satellite blending sources can be preset as they will never change */
			im->info.synth->src[5] = Xgl.images[glImageBlend(glNoImage,map_proj,glNoImage,glNoImage,100)];
			im->info.synth->src[5]->info.synth->nsrc = 2;
			im->info.synth->src[5]->info.synth->src[0] = im->info.synth->src[2];
			im->info.synth->src[5]->info.synth->src[1] = im->info.synth->src[3];
		}
	}
	else
	{
		ndx = 0;
		pr_error(ActiveModule, invalid_im_msg);
	}

	if(ndx)
	{
		ImagePtr p;
		int n;
		int listlen[5] = {0,0,0,0,0};

		set_blend_ratio(im, ratio);

		/* Assign the input images to the various "display planes" */
		for(i = 0; i < nlist; i++)
		{
			if(!valid_image(imlist[i])) continue;
			p = Xgl.images[imlist[i]];
			switch((p->group==SyntheticGroup)? p->info.synth->image_type : p->imdef->prod->image_type)
			{
				case ImageTypeUnderlay:  n = 1; break;
				case ImageTypeSatellite: n = 2; break;
				case ImageTypeRadar:     n = 3; break;
				case ImageTypeOverlay:   n = 4; break;
				default:                 n = 0;
			}
			if (listlen[n] >= listmax[n]) list[n] = GETMEM(list[n], Image, (listmax[n] = listlen[n]+1));
			list[n][listlen[n]] = imlist[i];
			listlen[n]++;
		}

		/* The src_used variable is used during image creation to avoid creating unnecessary rasters */
		for(i = 0; i < 5; i++)
		{
			im->info.synth->src_used[i] = (listlen[i] > 0);
			(void) glImageCombine(im->info.synth->src[i]->info.synth->ndx, map_proj, list[i], listlen[i]);
		}
	}
	reset_module_name();
	return ndx;
}


/******************************************************************************/
/*
*	@brief Change the blending ratio of a given blended image.
*
*	@param[in] The index of the blended image.
*	@param[in] The new blending ratio to apply to the child images.
*/
/******************************************************************************/
void glImageBlendRatio( Image ndx, int ratio )
{
	set_module_name("glImageBlendRatio");
	if( valid_image(ndx) )
		set_blend_ratio(Xgl.images[ndx], ratio);
	reset_module_name();
}


/******************************************************************************/
/*
*	@brief Allows blending to be turned on and off without resetting the
*	       existing blend ratio setting.
*
*	@param[in] The index of the blended image.
*	@param[in] The blending state: on = TRUE off = FALSE
*/
/******************************************************************************/
void glImageSetBlendingState(Image ndx, LOGICAL state)
{
	ImagePtr im;

	set_module_name("glImageSetBlendingState");
	if( invalid_image(ndx) ) return;

	if( im->type != BlendedImage && im->type != CompositeImage )
	{
		pr_error(ActiveModule, "Image must be a Blended or Composite image type.\n");
	}
	else if(state)
	{
		int regen = (im->info.synth->blend_ratio & NoBlendModifiedMask);
		im->info.synth->blend_ratio &= BlendValueMask;
		if(DoBlending(im) || regen)
		{
			im->opstat = ImageGenerate;
			remove_store_file(im);
		}
	}
	else
	{
		if(DoBlending(im))
		{
			im->opstat = ImageGenerate;
			remove_store_file(im);
		}
		im->info.synth->blend_ratio |= NoBlendMask;
	}

	reset_module_name();
}


/******************************************************************************/
/*
*	@brief Set the colour lookup table to use with the given image.
*
*	@param[in] ndx The image index
*	@param[in] lutdata The lookup table index
*/
/******************************************************************************/
void glImageSetLUT( Image ndx, ImageLUT lutdata )
{
	ImagePtr im;

	set_module_name("glImageSetLUT");

	if( invalid_image(ndx) || invalid_lut(lutdata)) return;

	im = Xgl.images[ndx];

	if( im->bands != SingleBand )
	{
		pr_warning(ActiveModule, lut_change_msg);
	}
	else
	{
		if(im->lut != lutdata)
		{
			if(lutdata == glNoLUT)
			{
				if(NotNull(im->imdef) && im->lut != im->imdef->info.dlut)
				{
					im->lut    = (im->imdef->info.dlut != glNoLUT) ? im->imdef->info.dlut:im->dlut;
					im->opstat = ImageGenerate;
				}
			}
			else
			{
				im->lut    = lutdata;
				im->opstat = ImageGenerate;
			}
		}
	}
	reset_module_name();
}



/******************************************************************************/
/*
*	@brief Set the colour lookup table to use with the given image as defined
*	       by its tag.
*
*	@param[in] tag The image tag
*	@param[in] lutdata The lookup table index
*/
/******************************************************************************/
void glImageSetTagLUT( STRING tag, ImageLUT lutdata )
{
	int      n;
	ImagePtr im;
	LOGICAL  colour_image = FALSE;

	set_module_name("glImageSetTagLUT");

	if(invalid_lut(lutdata)) return;

	glImageInfoSetLuts(tag, lutdata);

	for( n = 1; n < Xgl.nimages; n++ )
	{
		im = Xgl.images[n];
		if(!im) continue;
		if(!im->imdef) continue;
		if(im->bands != SingleBand )
		{
			colour_image = TRUE;
			continue;
		}
		if(im->lut == im->imdef->info.lut) continue;
		if(!same(im->imdef->prod->tag,tag) && !same(im->imdef->tag,tag)) continue;
		im->lut    = (im->imdef->info.lut != glNoLUT)? im->imdef->info.lut : im->dlut;
		im->opstat = ImageGenerate;
	}
	if(colour_image)
		pr_warning(ActiveModule, lut_change_msg);
	reset_module_name();
}


/******************************************************************************/
/*
*	@brief Set the image brightness.
*
*	@param[in] tag The image tag
*	@param[in] scale_factor The brightness of the image with 1.0 being normal.
*
*	@note Note that the scale factor can go from 0 to any positive number.
*	      Numbers beyond 1 will increase the brightness (within reason as high
*	      values can result in an all white image).
*/
/******************************************************************************/
void glImageTagSetBrightness( STRING tag, float scale_factor )
{
	int      n;
	ImagePtr im;

	if( scale_factor < 0 && scale_factor != glNORMAL_BRIGHTNESS )
		scale_factor = 1.0;

	for( n = 1; n < Xgl.nimages; n++ )
	{
		im = Xgl.images[n];
		if(!im) continue;
		if(!im->imdef) continue;
		if(same(im->imdef->tag,tag) && im->imdef->bsf != scale_factor)
		{
			im->imdef->bsf = scale_factor;
			im->opstat     = ImageGenerate;
			
		}
		if(!im->imdef->prod) continue;
		if(same(im->imdef->prod->tag,tag) && im->imdef->prod->bsf != scale_factor)
		{
			im->imdef->prod->bsf = scale_factor;
			im->opstat           = ImageGenerate;
		}
	}
}


/******************************************************************************/
/*
*	@brief Set the brightness of all images of a given type.
*
*	@param[in] type The image type
*	@param[in] scale_factor The brightness of the image with 1.0 being normal.
*
*	@note Note that the scale factor can go from 0 to any positive number.
*	      Numbers beyond 1 will increase the brightness (within reason as high
*	      values can result in an all white image).
*/
/******************************************************************************/
void glImageTypeSetBrightness(enum IMAGE_TYPE type, float scale_factor )
{
	int      n;
	ImagePtr im;

	set_module_name("glImageTypeSetBrightness");

	if( scale_factor < 0 && scale_factor != glNORMAL_BRIGHTNESS ) scale_factor = 1.0;

	if( image_type_bsf[type] != scale_factor )
	{
		image_type_bsf[type] = scale_factor;

		/* Flag all images of this type for redisplay */
		for( n = 1; n < Xgl.nimages; n++ )
		{
			im = Xgl.images[n];
			if(!im) continue;
			if(!im->imdef) continue;
			if(!im->imdef->prod) continue;
			if(im->imdef->prod->image_type != type) continue;
			im->opstat = ImageGenerate;
		}
	}
	reset_module_name();
}



/******************************************************************************/
/*
*	@brief Display a given image.
*
*	@param[in] ndx The image index.
*/
/******************************************************************************/
void glImageDisplay( Image ndx )
{
	ImagePtr im;

	set_module_name("glImageDisplay");

	if(invalid_image(ndx)) return;

	if(Xgl.image_xlib_output)
	{
		im = Xgl.images[ndx];
		scan_image_tree(im);
		if(create_image(im))
		{
			Xgl.image_xlib_output(im);
		}
		clear_raster_queue();
	}
	else
	{
		pr_error(ActiveModule, "Library is not initialized for X display.\n");
	}
	reset_module_name();
}


/******************************************************************************/
/*
*	g l I m a g e C r e a t e R a s t e r
*
*	The type parameter can be one of:
*
*	glImagePixelMajor       - rgbrgbrgb containing transparent pixels if any
*	glImagePixelMajorOpaque - as above with black replacing transparent pixels
*	glImagePlaneMajor       - rrr...ggg...bbb
*	glImagePlaneMajorOpaque - black replaces transparent
*	glAlphaPixelMajor       - rgbargba...
*	glAlphaPlaneMajor       - rrr...ggg...bbb...aaa
*
*	xorg and yorg are the offset of the image from the drawing window origin
*	(lower left in all cases)
*
*	Note: Use the glImageTransparentPixel() function to obtain the definition
*	      of a transparent pixel.
*/
/******************************************************************************/
LOGICAL glImageCreateRaster( Image ndx, enum IMAGE_RASTER_TYPE type, UNCHAR **raster,
								int *xorg, int *yorg, int *width, int *height )
{
	int     bpp = 3;
	UNCHAR  *ras, *b, *e, *rp, *gp, *bp, *ap;
	ImagePtr im;

	set_module_name("glImageCreateRaster");
	if(invalid_image(ndx)) return FALSE;

	im = Xgl.images[ndx];

	scan_image_tree(im);

	if(!create_image(im))
	{
		pr_error(ActiveModule, no_image_create);
		reset_module_name();
		return FALSE;
	}

	switch(type)
	{
		case glAlphaPlaneMajor:       bpp = 4; break;
		case glAlphaPixelMajor:       bpp = 4; break;
		case glImagePlaneMajor:       bpp = 3; break;
		case glImagePixelMajor:       bpp = 3; break;
		case glImagePlaneMajorOpaque: bpp = 3; break;
		case glImagePixelMajorOpaque: bpp = 3; break;
	}

	ras = INITMEM(UNCHAR,im->dw*im->dh*bpp);
	if(!ras)
	{
		pr_error(ActiveModule, mem_error_msg);
		reset_module_name();
		return FALSE;
	}

	rp = ras;
	b  = im->raster;
	e  = im->raster + im->dw*im->dh*RASTER_BPP;

	if(type == glAlphaPlaneMajor)
	{
		gp = ras + im->dw*im->dh;
		bp = ras + im->dw*im->dh*2;
		ap = ras + im->dw*im->dh*3;
		while(b < e)
		{
			if(OPAQUE_PIXEL(b))
			{
				*rp = b[0];
				*gp = b[1];
				*bp = b[2];
				*ap = 255;
			}
			rp++;
			gp++;
			bp++;
			ap++;
			b += RASTER_BPP;
		}
	}
	else if(type ==  glAlphaPixelMajor)
	{
		while(b < e)
		{
			if(OPAQUE_PIXEL(b))
			{
				rp[0] = b[0];
				rp[1] = b[1];
				rp[2] = b[2];
				rp[3] = 255;
			}
			rp += bpp;
			b  += RASTER_BPP;
		}
	}
	else if(type == glImagePlaneMajor)
	{
		gp = ras + im->dw*im->dh;
		bp = ras + im->dw*im->dh*2;
		ap = ras + im->dw*im->dh*3;
		while(b < e)
		{
			if(OPAQUE_PIXEL(b))
			{
				*rp++ = b[0];
				*gp++ = b[1];
				*bp++ = b[2];
			}
			else
			{
				*rp++ = T_RED;
				*gp++ = T_GREEN;
				*bp++ = T_BLUE;
			}
			b += RASTER_BPP;
		}
	}
	else if(type == glImagePlaneMajorOpaque)
	{
		gp = ras + im->dw*im->dh;
		bp = ras + im->dw*im->dh*2;
		ap = ras + im->dw*im->dh*3;
		while(b < e)
		{
			if(OPAQUE_PIXEL(b))
			{
				*rp = b[0];
				*gp = b[1];
				*bp = b[2];
			}
			rp++;
			gp++;
			bp++;
			b += RASTER_BPP;
		}
	}
	else if(type == glImagePixelMajor)
	{
		while(b < e)
		{
			*rp++ = *b++;
		}
	}
	else if(type == glImagePixelMajorOpaque)
	{
		while(b < e)
		{
			if(OPAQUE_PIXEL(b))
			{
				rp[0] = b[0];
				rp[1] = b[1];
				rp[2] = b[2];
			}
			rp += bpp;
			b  += RASTER_BPP;
		}
	}
	else
	{
		/* The compiler should not allow this */
		pr_error(ActiveModule, "Unrecognized image raster type\n");
		reset_module_name();
		FREEMEM(ras);
		return FALSE;
	}

	if (raster) *raster = ras;   else FREEMEM(ras);

	(void) glImageGetGeometry(ndx, xorg, yorg, width, height);

	clear_raster_queue();
	reset_module_name();
	return TRUE;
}


/******************************************************************************/
/*
 *	@brief Creates a png (portable network graphics) image encoded in base64.
 *
 *	@param[in] ndx The image index.
 *
 *	@return An allocated string that contains the image encoded image.
 *
 *	@note The calling routine is responsible for freeing the returned string.
 *	      No array length is given as the string is in standard null
 *	      terminated format with all printable characters and can be obtained
 *	      with the strlen() function.
 */
/******************************************************************************/
STRING glImageToBase64PNG( Image ndx )
{
	FILE   *fp;
	STRING raster = NULL;

	set_module_name("glImageToBase64PNG");
	if(invalid_image(ndx)) return NULL;
#ifdef MACHINE_PCLINUX
	if((fp = tmpfile()))
	{
		if(image_to_png(ndx, fp))
			raster = file_encode_base64(fp);
		(void) fclose(fp);
	}
#else
	pr_error(ActiveModule, "The creation of png images is not available on this machine.\n");
#endif
	reset_module_name();
	return raster;
}


/******************************************************************************/
/*
*	@brief Writes the image to a file in png (portable network graphics) format.
*
*	@param[in] ndx The image index.
*	@param[in] file_path The complete path and file name of the output file.
*
*	@return TRUE if the write was successful or FALSE if not.
*/
/******************************************************************************/
LOGICAL glImageToPNG( Image ndx, STRING file_path )
{
	FILE     *fp;
	LOGICAL  png_ok = FALSE;

	set_module_name("glImageToPNG");
	if(invalid_image(ndx)) return FALSE;
#ifdef MACHINE_PCLINUX
	if((fp = fopen(file_path,"w")))
	{
		png_ok = image_to_png(ndx, fp);
		(void) fclose(fp);
	}
#else
	pr_error(ActiveModule, "The creation of png images is not available on this machine.\n");
#endif
	reset_module_name();
	return png_ok;
}


/******************************************************************************/
/*
*	g l I m a g e T r a n s p a r e n t P i x e l
*/
/******************************************************************************/
glCOLOR *glImageTransparentPixel(void)
{
	static glCOLOR trans_pixel;

	/* Setting the values directly ensures that if the calling routine
	 * modified the contents that the return will be correct.
	 */
	SET_TO_TRANSPARENT_COLOR(trans_pixel);

	return &trans_pixel;
}


/******************************************************************************/
/*
*	g l I m a g e G e t G e o m e t r y
*
*	Return the image size and its location wrt the output window in pixels.
*	The image origin is specified with respect to the output window and is in
*	the	library coordinate system of a lower left corner origin.
*/
/******************************************************************************/
LOGICAL glImageGetGeometry( Image ndx, int *xorg, int *yorg, int *width, int *height )
{
	ImagePtr im;

	set_module_name("glImageGetGeometry");
	if(invalid_image(ndx)) return FALSE;

	im = Xgl.images[ndx];

	if (xorg)   *xorg   = im->dx;
	if (yorg)   *yorg   = W->ym - (im->dh + im->dy);
	if (width)  *width  = im->dw;
	if (height) *height = im->dh;

	reset_module_name();
	return TRUE;
}



/******************************************************************************/
/*
*	g l I m a g e I s G r e y s c a l e
*/
/******************************************************************************/
LOGICAL glImageIsGreyscale( Image ndx )
{
	set_module_name("glImageIsGreyscale");
	if( invalid_image(ndx) ) return FALSE;
	reset_module_name();
	return( Xgl.images[ndx]->bands == SingleBand );
}



/******************************************************************************/
/*
 * g l I m a g e I s D a t a
 */
/******************************************************************************/
LOGICAL glImageIsDataImage( Image ndx )
{
	ImagePtr im;
	LOGICAL  rtn = FALSE;

	set_module_name("glImageIsDataImage");
	if(invalid_image(ndx)) return FALSE;
	im = Xgl.images[ndx];

	if(im->group == RadarGroup)
	{
		if(im->encoding == ImageEncodingGriddedURP || im->encoding == ImageEncodingPolarURP)
			rtn = (NotNull(im->info.radar) && im->info.radar->nitems > 0);
	}
	else if(im->group == DataGroup)
	{
		rtn = NotNull(im->info.grid);
	}
	reset_module_name();
	return rtn;
}



/******************************************************************************/
/*
 *
 *   g l I m a g e G e t D a t a I t e m s
 *
 *   IMPORTANT: Do not free the items paramter as it points to an internal
 *              static.
 */
/******************************************************************************/
int glImageGetDataItems( Image ndx, char ***items )
{
	int      n = 0;
	ImagePtr im;

	set_module_name("glImageGetDataItems");

	if (items) *items = (STRING *)0;
	if(invalid_image(ndx)) return 0;

	im = Xgl.images[ndx];

	if(im->group == RadarGroup)
	{
		if (items) *items = im->info.radar->item_id;
		n = im->info.radar->nitems;
	}
	else if(im->group == DataGroup)
	{
		if (im->info.grid->element)
		{
			if (items) *items = &im->info.grid->element;
			n = 1;
		}
	}

	reset_module_name();
	return n;
}



/******************************************************************************/
/*
*	g l I m a g e S a m p l e P o i n t
*/
/******************************************************************************/
float glImageSamplePoint( Image idx, STRING item, Coord x, Coord y )
{
	float vals[1] = {glDATA_MISSING};

	set_module_name("glImageSamplePoint");

	if(!invalid_image(idx))
	{
		sample_image(idx, item, &x, &y, 1, vals);
		reset_module_name();
	}
	return vals[0];
}



/******************************************************************************/
/*
*	g l I m a g e S a m p l e A r r a y
*/
/******************************************************************************/
float *glImageSampleArray( Image idx, STRING item, Coord *x, Coord *y, int np )
{
	int   n;
	float *vals = NULL;

	set_module_name("glImageSampleArray");

	if(np > 0 && !invalid_image(idx))
	{
		vals = MEM(float, np);
		if (vals)
		{
			for(n=0; n<np; n++) vals[n] = glDATA_MISSING;
			sample_image(idx, item, x, y, np, vals);
		}
		reset_module_name();
	}
	return vals;
}



/******************************************************************************/
/*
*	g l I m a g e S a m p l e L a t L o n P o i n t
*/
/******************************************************************************/
float glImageSampleLatLonPoint( Image idx, STRING item, LatLonPoint p )
{
	float vals[1] = {glDATA_MISSING};

	set_module_name("glImageSampleLatLonPoint");

	if(!invalid_image(idx))
	{
		lat_lon_sample_image(idx, item, &p, 1, vals);
		reset_module_name();
	}
	return vals[0];
}



/******************************************************************************/
/*
*	g l I m a g e S a m p l e L a t L o n A r r a y
*/
/******************************************************************************/
float *glImageSampleLatLonArray( Image idx, STRING item, LatLonPoint *p, int np )
{
	int   n;
	float *vals = NULL;

	set_module_name("glImageSampleLatLonArray");

	if(np > 0 && !invalid_image(idx))
	{
		vals = MEM(float, np);
		if (vals)
		{
			for(n=0; n<np; n++) vals[n] = glDATA_MISSING;
			lat_lon_sample_image(idx, item, p, np, vals);
		}
		reset_module_name();
	}
	return vals;
}



/******************************************************************************/
/*
 *   g l I m a g e S h o w R a d a r R a n g e R i n g s
 */
/******************************************************************************/
void glImageShowRadarRangeRings( LOGICAL show, int delta )
{
	int      n;
	ImagePtr im;

	/* delta is the distance the range rings are apart in km - default 40 */
	if(show == insert_range_rings && delta == range_ring_delta)
		return;

	insert_range_rings = show;
	range_ring_delta   = delta;

	/* Regenerate all radar images affected by this change. Note that only
	 * radar images with a radar projection (handled as no projection) are
	 * affected by the change
	 */
	for( n = 1; n < Xgl.nimages; n++ )
	{
		im = Xgl.images[n];
		if(im->group != RadarGroup) continue;
		if(!im->imdef) continue;
		if(im->imdef->mproj.projection.type != ProjectNone) continue;
		if(im->imdef->radar.options & RadarRangeRings ) im->opstat = ImageGenerate;
	}
}


/******************************************************************************/
/*
 *   g l I m a g e S e t R a d a r R a n g e R i n g C o l o r
 */
/******************************************************************************/
void glImageSetRadarRangeRingColor( UNCHAR *rings, UNCHAR *limit_ring )
{
	if (rings)
	{
		ring_colour[0] = rings[0];
		ring_colour[1] = rings[1];
		ring_colour[2] = rings[2];
	}
	if (limit_ring)
	{
		limit_ring_colour[0] = limit_ring[0];
		limit_ring_colour[1] = limit_ring[1];
		limit_ring_colour[2] = limit_ring[2];
	}
}



/*=============== Internal Library Functions ======================*/



/*  Creates a standard image structure and initializes it.
*/
int _xgl_make_image(int type )
{
	int       i, fid = 0;
	ImagePtr  im;
	MAP_PROJ  *proj;

	im = ONEMEM(ImageStruct);

	/* Set default values
	*/
	im->type   = type;
	im->bands  = TripleBand;
	im->ow     = 1;
	im->oh     = 1;
	im->rx     = 0.0;
	im->ry     = 0.0;
	im->lut    = glNoLUT;
	im->opstat = ImageGenerate;
	im->fid[0] = '\0';
	im->mx     = 0.0;
	im->my     = 0.0;

	/* fix #20050726
	 * mw and mh are the width and height in map units of the base map
	 * projection. If not available we determine this from the window
	 * and scaling factors. Note that this latter action can result in
	 * strange behaviour when creating the image when zoomed.
	 */
	if(NotNull(proj = get_target_map()))
	{
		im->mw = (Coord) proj->definition.xlen;
		im->mh = (Coord) proj->definition.ylen;
	}
	else
	{
		im->mw = (Coord) ((float)W->xm / W->Sx);   /* width in map units */
		im->mh = (Coord) ((float)W->ym / W->Sy);   /* height in map units */
	}

	define_map_projection(&(im->mproj),     &NoProjDef, &NoMapDef, &NoGridDef);
	define_map_projection(&(im->mproj_org), &NoProjDef, &NoMapDef, &NoGridDef);


	/* Add to image list. Check to see if there are any null pointers avaliable
	 * for use. Since we want Image value 0 to be unused we start the indexing
	 * at 1. So for simplicity the first element of the array is always ignored.
	 */
	for(i=1; i < Xgl.nimages; i++)
	{
		if(Xgl.images[i]) continue;
		Xgl.images[i] = im;
		fid = i;
		break;

	}
	if(!fid)
	{
		if(Xgl.nimages >= Xgl.maximages)
		{
			Xgl.images = GETMEM(Xgl.images, ImagePtr, Xgl.maximages+=100);
			if(!Xgl.nimages) Xgl.nimages++;
		}
		fid = Xgl.nimages;
		Xgl.images[Xgl.nimages] = im;
		Xgl.nimages++;
	}
	(void) snprintf(im->fid, FIDLEN, "%d-", fid);

	return fid;
}


/* If the file is associated with an image definition read from a config
 * file then the file will be created from this information. If the image
 * is not associated with a definition as would happen with a call to
 * glImageFetchFile() then the name of the file is stored in the vtime
 * variable.
 */
FILE *_xgl_open_image_file(ImagePtr im)
{
	FILE *fp = (FILE*)0;

	if(im->imdef)
	{
		STRING fname = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
		if(fname)
		{
			fp = fopen(fname, BINARY_READ);
			if (!fp) pr_error(ActiveModule, no_file_access, fname);
			FREEMEM(fname);
		}
	}
	else if(im->vtime)
	{
		fp = fopen(im->vtime, BINARY_READ);
		if (!fp) pr_error(ActiveModule, no_file_access, im->vtime);
	}
	else
	{
		pr_error(ActiveModule, "Unable to determine image file name.\n");
	}

	return fp;
}



/*  Save a raster as a compressed file. Disk reading is much slower than memory
 *  operations so this should result in smaller files and faster performance. For
 *  Linux zlib is used for compression. For others where the zilb library is not
 *  normally available, the zlf functions are used.
 */
LOGICAL _xgl_write_image_file( FILE *fp, UNCHAR *raster, int width, int height, int bpp )
{
	LOGICAL rtn = FALSE;

#ifdef MACHINE_PCLINUX

	uLongf length;
	uLong  srclen;
	UNCHAR *dst;

	srclen = (uLong) (width * height * bpp);
	/* zlib needs the buffer to be at least 0.1% bytes bigger */
	length = (uLongf) (srclen + srclen/500 + 20);
	dst    = MEM(UNCHAR, length);
	if(IsNull(dst))
	{
    	pr_error(ActiveModule, mem_error_msg);
	}
	else if(compress(dst, &length, raster, srclen) == Z_OK)
	{
		size_t dstlen = length;
		if( fwrite((void *)&width,  sizeof(int), 1, fp) == 1 &&
			fwrite((void *)&height, sizeof(int), 1, fp) == 1 &&
			fwrite((void *)&bpp,    sizeof(int), 1, fp) == 1 &&
			fwrite((void *)&dstlen, sizeof(size_t), 1, fp) == 1 &&
			fwrite((void *)dst, 1, dstlen, fp) == dstlen) rtn = TRUE;
	}

#else

	UNINT length, srclen;
	void  *dst;

	srclen = width * height * bpp;
	length = srclen - 1;
	dst    = MEM(UNCHAR, length);
	if(IsNull(dst))
	{
    	pr_error(ActiveModule, mem_error_msg);
	}
	else if((length = lzf_compress((void*) raster, srclen, dst, length)) > 0)
	{
		size_t dstlen = length;
		if( fwrite((void *)&width,  sizeof(int),   1, fp) == 1 &&
			fwrite((void *)&height, sizeof(int),   1, fp) == 1 &&
			fwrite((void *)&bpp,    sizeof(int),   1, fp) == 1 &&
			fwrite((void *)&length, sizeof(UNINT), 1, fp) == 1 &&
			fwrite(dst, 1, dstlen, fp) == dstlen) rtn = TRUE;
	}

#endif

	FREEMEM(dst);
	return rtn;
}


/*  Read encoded raster file. We don't have to be fancy here as only
 *  files created by the above function are read.
 */
LOGICAL  _xgl_read_image_file(FILE *fp, UNCHAR **raster, int w, int h)
{
	int     width, height, bpp;

#ifdef MACHINE_PCLINUX
	size_t  src_size;
	UNCHAR  *srcbuf;
	uLongf  dstlen;

	/* find the bytes per pixel and the size of the encoded file */
	if( fread((void *)&width,    sizeof(int),    1, fp) != 1 ) return FALSE;
	if( fread((void *)&height,   sizeof(int),    1, fp) != 1 ) return FALSE;
	if( fread((void *)&bpp,      sizeof(int),    1, fp) != 1 ) return FALSE;
	if( fread((void *)&src_size, sizeof(size_t), 1, fp) != 1 ) return FALSE;

	/* size check */
	if( width != w || height != h ) return FALSE;

	/* Allocate the buffer to read our compressed file into */
	srcbuf = MEM(UNCHAR,src_size);
	if(IsNull(srcbuf)) goto err1;
	if(fread(srcbuf, 1, src_size, fp) != src_size) goto err2;

	/* Allocate the raster if required */
	dstlen = (uLongf) (width * height * bpp);
	if(IsNull(*raster)) *raster = MEM(UNCHAR,dstlen);
	if(IsNull(*raster)) goto err1;
	if(uncompress(*raster, &dstlen, srcbuf, (uLong)src_size) != Z_OK) goto err2;

#else

	UNINT src_size, dstlen;
	void  *srcbuf;

	/* find the bytes per pixel and the size of the encoded file */
	if( fread((void *)&width,    sizeof(int),   1, fp) != 1 ) return FALSE;
	if( fread((void *)&height,   sizeof(int),   1, fp) != 1 ) return FALSE;
	if( fread((void *)&bpp,      sizeof(int),   1, fp) != 1 ) return FALSE;
	if( fread((void *)&src_size, sizeof(UNINT), 1, fp) != 1 ) return FALSE;

	/* size check */
	if( width != w || height != h ) return FALSE;

	/* Allocate the buffer to read our compressed file into */
	srcbuf = MEM(UNCHAR,src_size);
	if(IsNull(srcbuf)) goto err1;
	if(fread(srcbuf, 1, (size_t)src_size, fp) != (size_t)src_size) goto err2;

	/* Allocate the raster if required */
	dstlen = width * height * bpp;
	if(IsNull(*raster)) *raster = MEM(UNCHAR,dstlen);
	if(IsNull(*raster)) goto err1;

	if(lzf_decompress(srcbuf, src_size, (void*)(*raster), dstlen) == 0) goto err2;

#endif

	FREEMEM(srcbuf);
	return TRUE;

err1:
    pr_error(ActiveModule, mem_error_msg);
err2:
	FREEMEM(srcbuf);
	return FALSE;
}



/*=============== Private Functions ======================*/


/*  If blending has been turned off reset the blending amount but set the upper bits if
 *  the ratio changed. If blending is on and the ratio changes mark for regeneration.
 */
static void set_blend_ratio(ImagePtr im, int ratio)
{
	if( im->type != BlendedImage && im->type != CompositeImage )
	{
		pr_error(ActiveModule, "Image must be a Blended or Composite image type.\n");
	}
	else
	{
		int bits;
		int r = MIN(100,MAX(0,ratio));

		if( ratio < 0 || ratio > 100)
			pr_error(ActiveModule, "Blend ratio \"%d\" out of range 0-100.\n", ratio);

		if((bits = im->info.synth->blend_ratio & NoBlendAllMask))
		{
			if( r != (im->info.synth->blend_ratio & BlendValueMask) )
				im->info.synth->blend_ratio = r | (bits | NoBlendModifiedMask);
			else
				im->info.synth->blend_ratio = r | bits;
		}
		else if( r != im->info.synth->blend_ratio )
		{
			/* blend ratio changed so remove all store files */
			im->info.synth->blend_ratio = r;
			remove_store_file(im);
		}
	}
}


/* Add the image ims to the synthetic image im after remaping. The image_type
 * of im is set to that of the source image. If not all of the source images
 * are of the same type then the image type of im is set to ImageTypeUnknown.
 */
static void add_image_to_synth(ImagePtr im, ImagePtr ims, MAP_PROJ *map_proj)
{
	enum IMAGE_TYPE type;

	if(ims->group == SyntheticGroup)
		type = ims->info.synth->image_type;
	else
		type = ims->imdef->prod->image_type;

	if(im->info.synth->nsrc == 0)
		im->info.synth->image_type = type;
	else if(im->info.synth->image_type != type)
		im->info.synth->image_type = ImageTypeUnknown;

	(void) _xgl_remap_image(ims, map_proj);
	im->info.synth->src[im->info.synth->nsrc] = ims;
	im->info.synth->nsrc++;
}



/* Create a png encoded image file. Make sure that the file
 * pointer is for a writable file type.
 */
static LOGICAL image_to_png( Image ndx, FILE *fp )
{
	LOGICAL     png_ok = FALSE;
#ifdef MACHINE_PCLINUX
	ImagePtr    im;
	png_structp png_ptr;
	png_infop   info_ptr;
	png_bytep  *row_pointers;

	im = Xgl.images[ndx];

	scan_image_tree(im);

	if(!create_image(im))
	{
		pr_error(ActiveModule, no_image_create);
		return png_ok;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr)
	{
		info_ptr = png_create_info_struct(png_ptr);
		if(info_ptr)
		{
			/* For some reason setjmp needs to be on its own line or a sigseg results! */
			if(!setjmp(png_jmpbuf(png_ptr)))
			{
				row_pointers = INITMEM(png_bytep, im->dh);
				if(row_pointers)
				{
					int n;
					png_color_16 trans_values;

					/* set the transparent pixel information */
					(void) memset((void*)&trans_values, 0, sizeof(png_color_16));
					SET_TO_TRANSPARENT_COLOR(trans_values);

					/* assign the row pointers to the raster row locations */
					for(n = 0; n < im->dh; n++)
						row_pointers[n] = im->raster + im->dw*RASTER_BPP*n;

					/* create the png image */
					png_init_io(png_ptr, fp);
					png_set_IHDR(png_ptr, info_ptr,
						im->dw, im->dh, 8,
						PNG_COLOR_TYPE_RGB,
						PNG_INTERLACE_NONE,
						PNG_COMPRESSION_TYPE_DEFAULT,
						PNG_FILTER_TYPE_DEFAULT);
					png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans_values);
					png_write_info(png_ptr, info_ptr);
					png_write_image(png_ptr, row_pointers);
					png_write_end(png_ptr, info_ptr);
					FREEMEM(row_pointers);
					png_ok = TRUE;
				}
				else
				{
					pr_error(ActiveModule, mem_error_msg);
				}
			}
		}
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
	clear_raster_queue();

	if (!png_ok)
		pr_error(ActiveModule, "PNG image creation failure.\n");
#endif
	return png_ok;
}


/* Encode a binary file into base64 format returned as an allocated string.
 * This is a simple encoding scheme that has not been coded for speed. If
 * more speed is needed then a different scheme will be needed.
 */
static STRING file_encode_base64(FILE *fp)
{
	LOGICAL h1, h2;
	int     b1, b2, b3;
	int     c1, c2, c3, c4;
	long    i, size;
	size_t  len;
	STRING  p, out;

	static char encode_vals[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	if (!fp) return NULL;

	(void) fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	len = (size*4)/3+5;
	out = p = INITMEM(char, len);
	(void) memset((void*) out, 0, len);
	  
	for(i = 0; i < size; i += 3)
	{
		h1 = (i+1 < size);
		h2 = (i+2 < size);
		  
		b1 = fgetc(fp);
		b2 = (h1)? fgetc(fp):0;
		b3 = (h2)? fgetc(fp):0;
		  
		c1 = (b1 >> 2) & 0x3f;
		c2 = ((b1 & 0x3) << 4) | ((b2 >> 4) & 0xf);
		c3 = ((b2 & 0xf) << 2) | ((b3 >> 6) & 0x3);
		c4 = b3 & 0x3f;
		  
		*p++ = encode_vals[c1];
		*p++ = encode_vals[c2];
		*p++ = (h1)? encode_vals[c3]:'=';
		*p++ = (h2)? encode_vals[c4]:'=';
	}
	return out;
}




/*
*  Function: calculate_geometry
*
*   Purpose: To determine the geometry of the actual displayed image given the
*            size and position requirements as set by the image geometry setting
*
*/
static void calculate_geometry( ImagePtr im )
{
	int  ix, iy, iw, ih, dx, dy, dw, dh, vl, vr, vt, vb;

	/* determine the image screen size
	*/
	iw = XR(im->mw);
	ih = YR(im->mh);

	/* Maintain image aspect ratio but only for non-synthetic images and if
	 * we are not set to adjust the ratio.
	 */
	if(im->group != SyntheticGroup && !(im->imdef != NULL && im->imdef->options & AdjustAspect))
	{
		float r0 = (float)im->ow / (float)im->oh;
		float r1 = (float)iw / (float)ih;
		if( r1 > r0 ) iw = ih * r0 + 0.5;
		else if( r1 < r0 ) ih = iw / r0 + 0.5;
	}

	/* Set image scale factors required to fill given size (im->mw * im->mh)
	*/
	im->sx = ((float)iw)/((float)im->ow);
	im->sy = ((float)ih)/((float)im->oh);

	/* Determine origin in X coords (upper left corner origin)
	*/
	ix = XS(im->mx,im->my);
	iy = YS(im->mx,im->my) - ih + 1;

	/* Set new flag if any changes.
	*/
	if( im->ix != ix || im->iy != iy || im->iw != iw || im->ih != ih ) im->opstat = ImageGenerate;

	/* get the location and size of the final image to be drawn (X coords)
	*  cropped to the viewport. We must also take rotation into consideration.
	*  First find our unbounded and unrotated location and size.
	*/
	dx = im->ix = ix;
	dy = im->iy = iy;
	dw = im->iw = iw;
	dh = im->ih = ih;

	/* Clipping limits. We use these directly instead of relying on masking
	*  in the GC because the amount of raster processing is dependent on the
	*  size of the clipping area. If we process directly into the clipping
	*  area we increase our efficiency and speed. The limits are set in terms
	*  of X coordinates (origin at upper left).
	*/
	if(W->clipping)
	{
		vl = W->cl;
		vr = W->cr;
		vt = (int)W->ym - W->ct - 1;
		vb = (int)W->ym - W->cb - 1;
	}
	else if(W->viewport)
	{
		vl = W->vl;
		vr = W->vr;
		vt = (int)W->ym - W->vt - 1;
		vb = (int)W->ym - W->vb - 1;
	}
	else
	{
		vl = 0;
		vr = W->xm - 1;
		vt = 0;
		vb = (int)W->ym - 1;
	}

	/* The clipping limits can be outside of the drawing window. This is ok
	*  for X drawing calls, but wasteful for raster images. Constrain to the
	*  drawing window.
	*/
	vl = MAX(vl,0);
	vr = MIN(vr,W->xm - 1);
	vt = MAX(vt,0);
	vb = MIN(vb,W->ym - 1);

	/* Check to see if angle is (near) zero or the clipping width or height is 1 or
	*  less pixels so as to avoid all the trig calls. 
	*/
	if( fabsf(im->ra) < 0.001 || vr - vl < 2 || vb - vt < 2 )
	{
		im->tx = MIN(dx-vl,0);
		im->ty = MIN(dy-vt,0);
	}
	else
	{
		/* Precompute these to save time
		*/
		float cosra = COSDEG(im->ra);
		float sinra = SINDEG(im->ra);

		/* Amount to translate the image to get the rotation point back to its
		*  original position. Our library coordinate system rotates about the
		*  lower left corner but the X geometry function actually rotates
		*  about the upper left corner.
		*/
		int x  = XS(im->rx,im->ry) - im->ix;
		int y  = YS(im->rx,im->ry) - im->iy;
		float xp = y*sinra + x*cosra;
		float yp = y*cosra - x*sinra;
		int tx = x - (int)((xp >= 0)? (xp+.5):(xp-.5)); /* roundoff */
		int ty = y - (int)((yp >= 0)? (yp+.5):(yp-.5)); /* roundoff */

		/* Size of containing rectangle of the rotated image.
		*/
		dw = (int)(im->ih * fabsf(sinra) + im->iw * fabsf(cosra) + 0.5);
		dh = (int)(im->ih * fabsf(cosra) + im->iw * fabsf(sinra) + 0.5);

		/* Now figure out the location of our containing rectangle and the amount to
		*  translate our image within that rectangle to appear as we want it. Note
		*  that our angle is constrained to plus/minus 180 degrees.
		*/
		if( im->ra > 0.0 && im->ra <= 90.0 )
		{
			dx = im->ix + tx;
			dy = im->iy - (int)(im->iw * sinra + 0.5) + ty;
			im->tx = MIN(dx - vl, 0);
			im->ty = MIN(dy - vt, 0) + (int)(im->iw * sinra + 0.5);
		}
		else if( im->ra > 0.0 )
		{
			dx = im->ix + (int)(im->iw * cosra - 0.5) + tx;
			dy = im->iy + (int)(im->ih * cosra - im->iw * sinra - 0.5) + ty;
			im->tx = MIN(dx - vl, 0) - (int)(im->iw * cosra - 0.5);
			im->ty = MIN(dy - vt, 0) + dh;
		}
		else if( im->ra >= -90.0 )
		{
			dx = im->ix + (int)(im->ih * sinra - 0.5) + tx;
			dy = im->iy + ty;
			im->tx = MIN(dx - vl, 0) - (int)(im->ih * sinra - 0.5);
			im->ty = MIN(dy - vt, 0);
		}
		else
		{
			dx = im->ix + (int)(im->ih * sinra + im->iw * cosra - 0.5) + tx;
			dy = im->iy + (int)(im->ih * cosra - 0.5) + ty;
			im->tx = MIN(dx - vl, 0) + dw;
			im->ty = MIN(dy - vt, 0) - (int)(im->ih * cosra - 0.5);
		}
	}

	/* Now clip to current clip window.
	*/
	dw = MIN(dx + dw, vr+1) - MAX(dx, vl);
	dh = MIN(dy + dh, vb+1) - MAX(dy, vt);
	dw = MAX(dw,0);
	dh = MAX(dh,0);
	dx = MAX(dx, vl);
	dy = MAX(dy, vt);

	/* Set new flag if any changes to target image position or size.
	*/
	if( im->dx != dx || im->dy != dy || im->dw != dw || im->dh != dh ) im->opstat = ImageGenerate;

	im->dx = dx;
	im->dy = dy;
	im->dw = dw;
	im->dh = dh;

	/* We will assume that images less then 3 pixels wide or high
	 * are due to roundoff error and we do not have a valid image.
	*/
	if( im->dw < 3 || im->dh < 3 )
	{
		im->opstat = ImageNotVisible;
		FREEMEM(im->raster);
	}
}


/* Structure used in the following 2 functions for passing data */
struct STORE_FILE {
	size_t datalen;
	size_t baselen;
	size_t namelen;
	STRING name;
	STRING data;
};


/* Re-entrant function to add the children of synthetic images to a file name. */
static void add_image_to_name(ImagePtr im, struct STORE_FILE *store_file)
{
	int n;

	(void) strcat(store_file->data, im->fid);

	if(im->group != SyntheticGroup) return;
	if(!im->info.synth) return;
	if(im->info.synth->nsrc < 1) return;
	/*
	 * Make sure the static buffer is big enough. If not add more
	 * memory and reset store_file->data.
	 */
	store_file->datalen += (size_t) im->info.synth->nsrc*FIDLEN;
	if(store_file->datalen > store_file->namelen)
	{
		store_file->namelen = store_file->datalen+1;
		store_file->name = GETMEM(store_file->name, char, store_file->namelen);
		if(!store_file->name)
		{
			store_file->namelen = 0;
			pr_error(ActiveModule, mem_error_msg);
			return;
		}
		store_file->data = store_file->name + store_file->baselen;
	}
	/*
	 * Add in the children file identifiers
	 */
	for( n = 0; n < im->info.synth->nsrc; n++ )
	{
		add_image_to_name(im->info.synth->src[n], store_file);
	}
}


/* Create the data file name and open the file in the appropriate mode.
 * The file name has the process pid added at the beginning in case there
 * is another child process runnng that also uses the same work directory.
 * This function is called many times, especially during animation so it
 * has been coded to speed things up where ever possible.
 */
static STRING make_store_file_name( ImagePtr im )
{
	static struct STORE_FILE store_file = {0,0,0,NULL,NULL};

	if (!store_file.name)
	{
		/* Allocate the store_file_name giving enough space so reallocation should
		 * not really every be necessary and copy in the directory and pid info.
		 */
		store_file.name = MEM(char, 300);
		if (!store_file.name)
		{
			pr_error(ActiveModule, mem_error_msg);
			return NULL;
		}
		store_file.namelen = 300;
		(void) snprintf(store_file.name, 300, "%s/i%d-", Xgl.work_directory, getpid());
		store_file.baselen = safe_strlen(store_file.name);
		store_file.data = store_file.name + store_file.baselen;
	}

	store_file.datalen = store_file.baselen;
	strcpy(store_file.data, "");
	add_image_to_name(im, &store_file);
	return store_file.name;
}


static FILE *open_store_file( ImagePtr im, STRING mode )
{
	FILE *fp = (FILE *)0;
	STRING fname = make_store_file_name(im);
	if (fname) fp = fopen(fname, mode);
	return fp;
}


/* Delete any store files with this image in them. This will of
 * course include the image store file itself plus any synthetic
 * files that include this image.
 */
static void remove_store_file( ImagePtr im )
{
	int    n, nlist;
	char   fid[12], pid[32];
	STRING *list;

	(void) strcpy(fid, "-");
	(void) strcat(fid, im->fid);
	/*
	 * i<pid> will start the file name.
	 */
	(void) snprintf(pid, 32, "^i%d-", getpid());

	nlist = dirlist(Xgl.work_directory, pid, &list);
	for(n = 0; n < nlist; n++)
	{
		if(strstr(list[n],fid))
			(void) unlink(pathname(Xgl.work_directory,list[n]));
	}
}


static LOGICAL write_store_file( ImagePtr im )
{
	LOGICAL rtn = FALSE;
	FILE *fp = open_store_file(im,"w");
	if(fp)
	{
		rtn = _xgl_write_image_file(fp, im->raster, im->dw, im->dh, RASTER_BPP);
		(void) fclose(fp);
	}
	return rtn;
}


static LOGICAL read_store_file( ImagePtr im )
{
	LOGICAL rtn = FALSE;
	FILE *fp = open_store_file(im, "r");
	if(fp)
	{
		rtn = _xgl_read_image_file(fp, &im->raster, im->dw, im->dh);
		(void) fclose(fp);
	}
	return rtn;
}



/*  Scan the image tree and check status. If the image is found to not be
 *  visible we just return. For synthetic images we assume not visible and
 *  change this if any of the child images are visible. The im->changed
 *  variable is used by synthetic images to determine if any of the sub
 *  images have changed, and thus there is the requirement to regenerate
 *  the image.
 */
static void scan_image_tree( ImagePtr im )
{
	if(im->group == SyntheticGroup)
	{
		int n;
		ImagePtr imp;

		calculate_geometry(im);
		im->opstat  = ImageNotVisible;
		im->changed = FALSE;
		for(n = 0; n < im->info.synth->nsrc; n++)
		{
			if((imp = im->info.synth->src[n]))
			{
				scan_image_tree(imp);
				if(imp->opstat != ImageNotVisible) im->opstat = ImageExists;
				if(imp->changed) im->changed = TRUE;
			}
		}
	}
	else
	{
		char opstat = im->opstat;
		calculate_geometry(im);
		im->changed = (im->opstat != opstat || im->opstat == ImageGenerate);
	}
}



/*  Add any overlays to the image after conversion to our internal raster format.
 */
static void add_overlays( ImagePtr im )
{
	if(im->group == RadarGroup)
	{
		if(insert_range_rings)
		{
			/* The following case is only for images with no projection */
			if( NotNull(im->imdef) &&
				im->imdef->mproj.projection.type == ProjectNone &&
				im->imdef->radar.options & RadarRangeRings)
			{
				int     xoff, yoff;
				float   coef[6], dx, dy, limit;	/* diamater of the radar coverage */

				if(im->encoding == ImageEncodingGriddedURP)
				{
					dx    = im->imdef->mproj.definition.xorg;
					dy    = im->imdef->mproj.definition.yorg;
					limit = im->imdef->info.width;
				}
				else if(im->encoding == ImageEncodingPolarURP)
				{
					dx    = im->imdef->mproj.definition.xorg;
					dy    = im->imdef->mproj.definition.yorg;
					limit = im->imdef->info.width;
				}
				else
				{
					dx    = im->imdef->radar.orgx;
					dy    = im->imdef->radar.orgy;
					limit = im->imdef->radar.diam;
				}

				/* Find the location of the radar in the image raster */
				_xgl_affine_coef(coef, im->sx, im->sy, im->ra, im->tx, im->ty );
				xoff = (int) floorf(dx * coef[0] + dy * coef[2] + coef[4]);
				yoff = (int) floorf(dx * coef[1] + dy * coef[3] + coef[5]);

				/* we need limit as a radius */
				limit /= 2.0;

				/* Draw the outer limit ring. */
				_xgl_draw_ellipse(im->raster, im->dw, im->dh, xoff, yoff,
					(int)floorf(limit*im->sx), (int)floorf(limit*im->sy),
					limit_ring_colour);

				if(range_ring_delta > 0)
				{
					float range, rrd = (float)range_ring_delta;
					for(range = rrd; range < limit; range += rrd)
					{
						_xgl_draw_ellipse(im->raster, im->dw, im->dh, xoff, yoff,
							(int)floorf(range*im->sx), (int)floorf(range*im->sy),
							ring_colour);
					}
				}
			}
		}
	}
}



/*  Create an image from some sort of image file stored on disk. The
 *  processing to determine what sort of image it is and how to read
 *  it is done by the functions found in image_fetch.c
 */
static LOGICAL create_file_image( ImagePtr im )
{
	float   coef[6];
	UNCHAR  *data, *mask;

	if(!create_raster(im)) return FALSE;

	/* If the image exists already on disk just read it from there
	 */
	if(im->opstat == ImageOnDisk)
	{
		if(read_store_file(im)) return TRUE;
		im->opstat = ImageGenerate;
	}

	/* Image is not on disk so create it from the original source
	 */
	if(!_xgl_get_source_image(im, &data, &mask)) return FALSE;

	/* Process the raw image into its final projection and colour map
	*/
	_xgl_affine_coef(coef, im->sx, im->sy, im->ra, im->tx, im->ty );
	transform_image( im, data, mask, im->ow, im->oh, coef );

	FREEMEM(data);
	FREEMEM(mask);

	/* Overlays are things added to the image that are not part of the
	 * original image (like radar range rings).
	 */
	add_overlays(im);

	/* Save the image */
	if(write_store_file(im))
		im->opstat = ImageOnDisk;

	return TRUE;
}


/* Blended and Combined images are synthetic images composed
 * of other synthetic images and file images.
 */
static LOGICAL create_blended_image( ImagePtr im )
{
	int      n;
	int      count = 0;
	ImagePtr imf;

	if(!create_raster(im)) return FALSE;

	/* If the image exists already on disk just read it from there */
	if(!im->changed)
	{
		if(read_store_file(im)) return TRUE;
		im->changed = TRUE;
	}

	for( n = 0; n < im->info.synth->nsrc; n++ )
	{
		imf = im->info.synth->src[n];
		if(create_image(imf))
		{
			if(count)
			{
				if(DoBlending(im))
					blend_image( im, imf );
				else
					combine_image( im, imf );
			}
			else
				copy_image( im, imf );
			count++;
		}
	}

	if(count > 0 && write_store_file(im))
		im->changed = FALSE;

	return (count > 0);
}


static LOGICAL create_combined_image( ImagePtr im )
{
	int      n;
	int      count = 0;
	ImagePtr imf;

	if(!create_raster(im)) return FALSE;

	/* If the image exists already on disk just read it from there */
	if(!im->changed)
	{
		if(read_store_file(im)) return TRUE;
		im->changed = TRUE;
	}

	for( n = 0; n < im->info.synth->nsrc; n++ )
	{
		imf = im->info.synth->src[n];
		if(create_image(imf))
		{
			if(count) combine_image( im, imf );
			else      copy_image( im, imf );
			count++;
		}
	}

	if(count > 0 && write_store_file(im))
		im->changed = FALSE;

	return (count > 0);
}


static LOGICAL create_composite_image( ImagePtr im )
{
	int	     i;
	int      count = 0;
	int      ndx = 5;
	ImagePtr imp[6];
	LOGICAL  used[6];

	if(!create_raster(im)) return FALSE;

	/* If the image exists already on disk just read it from there */
	if(!im->changed)
	{
		if(read_store_file(im)) return TRUE;
		im->changed = TRUE;
	}

	/* Use temporary image pointers as the assignment can be modified by blending. Note
	 * that src[5] is the blending operator and is not included in the ndx setting. The
	 * used variable is to avoid creating rasters that are not necessary.
	 */
	for(i = 0; i < 6; i++)
	{
		imp[i]  = im->info.synth->src[i];
		used[i] = im->info.synth->src_used[i];
	}

	/* index 2 is satellite and 3 is radar */
	if(DoBlending(im) && used[2] && used[3])
	{
		imp[5]->info.synth->blend_ratio = im->info.synth->blend_ratio;
		if(create_blended_image(imp[5]))
		{
			imp[2]  = imp[5];
			used[2] = TRUE;
			imp[3]  = imp[4];
			used[3] = used[4];
			ndx = 4;
		}
	}
	for(i = 0; i < ndx; i++)
	{
		if(used[i] && create_image(imp[i]))
		{
			if(count) combine_image( im, imp[i] );
			else      copy_image( im, imp[i] );
			count++;
		}
	}
	reset_module_name();

	if(count > 0 && write_store_file(im))
		im->changed = FALSE;

	return (count > 0);
}


/*  Call the function appropriate to the type of image being generated.
 */
static LOGICAL create_image( ImagePtr im )
{
	if(im->opstat == ImageNotVisible) return FALSE;

	/* If true we must already have an image as all of the rasters
	 * are created fresh on every display cycle.
	 */
	if(im->raster) return TRUE;

	switch(im->type)
	{
		case FileImage:      return create_file_image( im );
		case CombinedImage:  return create_combined_image( im );
		case BlendedImage:   return create_blended_image( im );
		case CompositeImage: return create_composite_image( im );
	}
	return FALSE;
}



/*================== Image geometry processing functions ====================*/




/* For pixels that represent encoded data values we must decode the value. Pixels
 * may be 8, 16 or 24 bit values and how we decode them depends on the endian type
 * of the computer. We make the assumption that the images are always delivered
 * as most significant byte first. If this is a problem then a new parameter giving
 * the endian order of the image data array will be needed.
 */
static float value_from_pixel(ImagePtr im, const UNCHAR *ptr, const int x, const int y)
{
	UNCHAR *p08, *c;
	short  *p16;
	int    *p32, v32;
	float  vf;

	if (MACHINE_ENDIAN == IMAGE_LITTLE_ENDIAN)
	{
		if(im->info.grid->byte_order == IMAGE_BIG_ENDIAN)
		{
			p08 = (UNCHAR*) ptr;
			switch(im->info.grid->bpp)
			{
				case 1:
					c   = p08 + y * im->ow + x;
					v32 = (int) *c;
					break;
				case 2:
					c   = p08 + y * im->ow * 2 + x * 2;
					v32 = c[0] | c[1] << 8;
					break;
				case 3:
					c   = p08 + y * im->ow * 3 + x * 3;
					v32 = c[0] | c[1] << 8 | c[2] << 16;
					break;
				case 4:
					c   = p08 + y * im->ow * 4 + x * 4;
					v32 = c[0] | c[1] << 8 | c[2] << 16 | c[3] << 24;
					break;
			}
			vf  = (float) v32;
		}
		else
		{
			switch(im->info.grid->bpp)
			{
				case 1:
					p08 = (UNCHAR*) ptr;
					vf  = (float) *(p08 + y * im->ow + x);
					break;
				case 2:
					p16 = (short*) ptr;
					vf  = (float) *(p16 + y * im->ow + x);
					break;
				case 3:
					p08 = (UNCHAR*) ptr;
					c   = p08 + y * im->ow * 3 + x * 3;
					v32 = c[0] << 16 | c[1] << 8 | c[2];
					vf  = (float)v32;
					break;
				case 4:
					p32 = (int*) ptr;
					vf = (float) *(p32 + y * im->ow + x);
					break;
			}
		}
	}
	else	/* we are running on a big endian machine */
	{
		if(im->info.grid->byte_order == IMAGE_BIG_ENDIAN)
		{
			switch(im->info.grid->bpp)
			{
				case 1:
					p08 = (UNCHAR*) ptr;
					vf  = (float) *(p08 + y * im->ow + x);
					break;
				case 2:
					p16 = (short*) ptr;
					vf  = (float) *(p16 + y * im->ow + x);
					break;
				case 3:
					p08 = (UNCHAR*) ptr;
					c   = p08 + y * im->ow * 3 + x * 3;
					v32 = c[0] << 16 | c[1] << 8 | c[2];
					vf  = (float) v32;
					break;
				case 4:
					p32 = (int*) ptr;
					vf  = (float) *(p32 + y * im->ow + x);
					break;
			}
		}
		else
		{
			p08 = (UNCHAR *)ptr;
			switch(im->info.grid->bpp)
			{
				case 1:
					c   = p08 + y * im->ow + x;
					v32 = (int) *c;
					break;
				case 2:
					c   = p08 + y * im->ow * 2 + x * 2;
					v32 = c[0] | c[1] << 8;
					break;
				case 3:
					c   = p08 + y * im->ow * 3 + x * 3;
					v32 = c[0] | c[1] << 8 | c[2] << 16;
					break;
				case 4:
					c   = p08 + y * im->ow * 4 + x * 4;
					v32 = c[0] | c[1] << 8 | c[2] << 16 | c[3] << 24;
					break;
			}
			vf = (float) v32;
		}
	}
	return ((im->info.grid->packed)? (vf * im->info.grid->scale + im->info.grid->offset) : vf);
}


static LOGICAL get_data_lut(ImagePtr im, glLUTCOLOR **dlut, size_t *nluts, float bfactor)
{
	int        n;
	size_t     nlut;
	glLUTCOLOR *lut;

	if(!_xgl_get_image_data_lut(im, &lut, &nlut))
	{
		if(im->imdef)
		{
			STRING path = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
			pr_error(ActiveModule,"Did not find look up table for image \"%s\".\n",
						base_name(path, NULL));
			FREEMEM(path);
		}
		else
		{
			pr_error(ActiveModule,"Did not find look up table for image.\n");
		}
		return FALSE;
	}

	for(n = 0; n < (int) nlut; n++)
		scale_pixel_brightness(&lut[n].red, &lut[n].green, &lut[n].blue, bfactor);

	*nluts = nlut;
	*dlut  = lut;
	return TRUE;
}



/* For speed the lut value tables are put in order so we can do a binary
 * search (see image_lut.c) This is the binary search comparison function.
 */
static int lutcmp(const void *a1, const void *a2)
{
	float      val = *((float *)a1);
	glLUTCOLOR *ls = (glLUTCOLOR *)a2;
	
	if(val <  ls->lower_bound) return -1;
	if(val >= ls->upper_bound) return 1;
	return 0;
}



/*  Affine transform source RGB image. If there is colour mapping to be done we do it
 *  here as it saves another run through the raster.
 *
 *  The geometry function uses an affine transformation with a nearest neighbour
 *  assignment method. This works best for meteorological images as we do not
 *  want to modify any pixel values as they have physical meaning.
 *
 *  This function also handles gridded pixel data rasters where the pixels can
 *  consist of more than one byte and are encoded as
 *
 *      value = pixel * scale + offset
 *
 *  For radar data the original source may be either raster or range-theta data.
 *  If range-theta then the width and height used to determine the coefficients
 *  is 2*range of the data in km. Thus we fake a square original source like it
 *  was a raster and get a source x,y location and transform this into radial
 *  coordinates. If the data is sub-km this is taken care of in the transform.
 *
 *  NOTE: A raster of the appropriate size must have been allocated and
 *        initialized to transparent in im->raster prior to calling this
 *        function.
 *
 *  im:         Image into which to do the affine transformation.
 *  src:        Source image buffer.
 *  mask:       Bit mask for source. Any bit not set indicates the corresponding
 *              source pixel is transparent.
 *  src_width:  Width of source image.
 *  src_height: Height of source image.
 *  affine:     Affine transform matrix to apply to the image.
 */
static void transform_image( ImagePtr im, const UNCHAR *src, const UNCHAR *mask, 
								const int src_width, const int src_height, const float affine[6])
{
	int     x, y, src_x, src_y, run_x0, run_x1, radius, theta, pos_p;
	int     src_rowstride, dst_rowstride;
	float   dx, dy, inv[6], dist, bear, offset;
	glCOLOR cmap[256];
	UNCHAR  *dst_p, *dst_linestart;
	const UNCHAR *src_p;

	float bfactor = get_brightness_factor(im);

	/* Polar radar images are a special case
	 */
	if(im->group == RadarGroup && im->encoding == ImageEncodingPolarURP)
	{
		if(im->rast_fmt == FloatType)
		{
			float      val, *farray;
			size_t     nluts, lutsize;
			glLUTCOLOR *dlut, *lptr;

			if(!get_data_lut(im, &dlut, &nluts, bfactor)) return;

			invert_affine(inv, affine);

			dst_rowstride = im->dw * RASTER_BPP;
			dst_linestart = im->raster;

			lutsize = sizeof(glLUTCOLOR);
			offset  = im->info.radar->range * im->info.radar->rscale;
			farray  = (float *)src;

			for (y = 0; y < im->dh; y++)
			{
				run_x0 = 0;
				run_x1 = im->dw;
				affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
				dst_p = dst_linestart + run_x0 * RASTER_BPP;
				for (x = run_x0; x < run_x1; x++)
				{
					dx = x * inv[0] + y * inv[2] + inv[4] - offset;
					dy = x * inv[1] + y * inv[3] + inv[5] - offset;
					if(fabsf(dx) < 0.000001)
					{
						dist = fabsf(dy);
						bear = (dy<0)? 180.:0.;
					}
					else if(dx < 0)
					{
						dist = SQRT(dx*dx + dy*dy);
						bear = 270. - ATAN(-dy/dx)*180./M_PI;
					}
					else
					{
						dist = SQRT(dx*dx + dy*dy);
						bear = 90. - ATAN(-dy/dx)*180./M_PI;
					}
					radius = (int)floorf(dist/im->info.radar->rscale + .5);
					if(radius < im->info.radar->range)
					{
						theta  = (int)floorf(bear/im->info.radar->tscale + .5) % im->info.radar->theta;
						val = farray[(theta * im->info.radar->range) + radius];
						lptr = (glLUTCOLOR *)bsearch((void*)&val,(void*)dlut,nluts,lutsize,lutcmp);
						if(lptr)
						{
							dst_p[0] = lptr->red;
							dst_p[1] = lptr->green;
							dst_p[2] = lptr->blue;
						}
					}
					dst_p += RASTER_BPP;
				}
				dst_linestart += dst_rowstride;
			}
			FREEMEM(dlut);
		}
		else
		{
			invert_affine(inv, affine);

			dst_rowstride = im->dw * RASTER_BPP;
			dst_linestart = im->raster;

			_xgl_get_image_cmap(im, cmap);
			scale_cmap_brightness(cmap, bfactor);

			offset = im->info.radar->range * im->info.radar->rscale;
			for (y = 0; y < im->dh; y++)
			{
				run_x0 = 0;
				run_x1 = im->dw;
				affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
				dst_p = dst_linestart + run_x0 * RASTER_BPP;
				for (x = run_x0; x < run_x1; x++)
				{
					dx = x * inv[0] + y * inv[2] + inv[4] - offset;
					dy = x * inv[1] + y * inv[3] + inv[5] - offset;
					if(fabsf(dx) < 0.000001)
					{
						dist = fabsf(dy);
						bear = (dy<0)? 180.:0.;
					}
					else if(dx < 0)
					{
						dist = SQRT(dx*dx + dy*dy);
						bear = 270. - ATAN(-dy/dx)*180./M_PI;
					}
					else
					{
						dist = SQRT(dx*dx + dy*dy);
						bear = 90. - ATAN(-dy/dx)*180./M_PI;
					}
					radius = (int)floorf(dist/im->info.radar->rscale + .5);
					if(radius < im->info.radar->range)
					{
						theta  = (int)floorf(bear/im->info.radar->tscale + .5) % im->info.radar->theta;
						src_p = src + (theta * im->info.radar->range) + radius;
						dst_p[0] = cmap[*src_p].red;
						dst_p[1] = cmap[*src_p].green;
						dst_p[2] = cmap[*src_p].blue;
					}
					dst_p += RASTER_BPP;
				}
				dst_linestart += dst_rowstride;
			}
		}
	}
	else if(im->group == DataGroup)
	{
		LOGICAL    put_pixel = TRUE;
		float      val;
		size_t     nluts, lutsize;
		glLUTCOLOR *dlut, *lptr;

		if(!get_data_lut(im, &dlut, &nluts, bfactor)) return;

		invert_affine(inv, affine);
		dst_rowstride = im->dw * RASTER_BPP;
		dst_linestart = im->raster;
		src_rowstride = src_width * RASTER_BPP;
		lutsize       = sizeof(glLUTCOLOR);

		for (y = 0; y < im->dh; y++)
		{
			dy = y + 0.5;
			run_x0 = 0;
			run_x1 = im->dw;
			affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
			dst_p = dst_linestart + run_x0 * RASTER_BPP;
			for (x = run_x0; x < run_x1; x++)
			{
				dx = x + 0.5;
				src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
				src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
				if(mask)
				{
					pos_p = (src_y * src_width) + src_x;
					put_pixel = (LOGICAL)MASK_BIT_SET(mask,pos_p);
				}
				if(put_pixel)
				{
					val = value_from_pixel(im, src, src_x, src_y);
					lptr = (glLUTCOLOR *)bsearch((void*)&val, (void*)dlut, nluts, lutsize, lutcmp);
					if(lptr)
					{
						dst_p[0] = lptr->red;
						dst_p[1] = lptr->green;
						dst_p[2] = lptr->blue;
					}
				}
				dst_p += RASTER_BPP;
			}
			dst_linestart += dst_rowstride;
		}
		FREEMEM(dlut);
	}
	else if(im->bands == TripleBand)
	{
		invert_affine(inv, affine);
		dst_rowstride = im->dw * RASTER_BPP;
		dst_linestart = im->raster;
		src_rowstride = src_width * RASTER_BPP;

		for (y = 0; y < im->dh; y++)
		{
			dy = y + 0.5;
			run_x0 = 0;
			run_x1 = im->dw;
			affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
			dst_p = dst_linestart + run_x0 * RASTER_BPP;
			for (x = run_x0; x < run_x1; x++)
			{
				dx = x + 0.5;
				src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
				src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
				src_p = src + (src_y * src_rowstride) + src_x * RASTER_BPP;
				dst_p[0] = *src_p++;
				dst_p[1] = *src_p++;
				dst_p[2] = *src_p++;
				scale_pixel_brightness(dst_p, dst_p+1, dst_p+2, bfactor);
				dst_p += RASTER_BPP;
			}
			dst_linestart += dst_rowstride;
		}
	}
	else if(mask)
	{
		_xgl_get_image_cmap(im, cmap);
		scale_cmap_brightness(cmap, bfactor);
		invert_affine(inv, affine);
		dst_rowstride = im->dw * RASTER_BPP;
		dst_linestart = im->raster;

		/* The lut_remap variable will exist if we did not find an exact match
		 * for the native colourmap of the original image but one that contained
		 * the colours of the original. The lut_remap variable then maps the
		 * original image to the assigned colormap. This is only done for the
		 * internal colormap not for any other assigned colormap.
		 */
		if(im->lut_remap != NULL && im->lut == im->dlut)
		{
			for (y = 0; y < im->dh; y++)
			{
				dy = y + 0.5;
				run_x0 = 0;
				run_x1 = im->dw;
				affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
				dst_p = dst_linestart + run_x0 * RASTER_BPP;
				for (x = run_x0; x < run_x1; x++)
				{
					dx = x + 0.5;
					src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
					src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
					pos_p = (src_y * src_width) + src_x;
					if(MASK_BIT_SET(mask,pos_p))
					{
						src_p = src + pos_p;
						dst_p[0] = cmap[im->lut_remap[*src_p]].red;
						dst_p[1] = cmap[im->lut_remap[*src_p]].green;
						dst_p[2] = cmap[im->lut_remap[*src_p]].blue;
					}
					dst_p += RASTER_BPP;
				}
				dst_linestart += dst_rowstride;
			}
		}
		else
		{
			for (y = 0; y < im->dh; y++)
			{
				dy = y + 0.5;
				run_x0 = 0;
				run_x1 = im->dw;
				affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
				dst_p = dst_linestart + run_x0 * RASTER_BPP;
				for (x = run_x0; x < run_x1; x++)
				{
					dx = x + 0.5;
					src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
					src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
					pos_p = (src_y * src_width) + src_x;
					if(MASK_BIT_SET(mask,pos_p))
					{
						src_p = src + pos_p;
						dst_p[0] = cmap[*src_p].red;
						dst_p[1] = cmap[*src_p].green;
						dst_p[2] = cmap[*src_p].blue;
					}
					dst_p += RASTER_BPP;
				}
				dst_linestart += dst_rowstride;
			}
		}
	}
	else
	{
		_xgl_get_image_cmap(im, cmap);
		scale_cmap_brightness(cmap, bfactor);
		invert_affine(inv, affine);
		dst_rowstride = im->dw * RASTER_BPP;
		dst_linestart = im->raster;

		/* See note above for explanation */
		if(im->lut_remap != NULL && im->lut == im->dlut)
		{
			for (y = 0; y < im->dh; y++)
			{
				dy = y + 0.5;
				run_x0 = 0;
				run_x1 = im->dw;
				affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
				dst_p = dst_linestart + run_x0 * RASTER_BPP;
				for (x = run_x0; x < run_x1; x++)
				{
					dx = x + 0.5;
					src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
					src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
					src_p = src + (src_y * src_width) + src_x;
					*dst_p++ = cmap[im->lut_remap[*src_p]].red;
					*dst_p++ = cmap[im->lut_remap[*src_p]].green;
					*dst_p++ = cmap[im->lut_remap[*src_p]].blue;
				}
				dst_linestart += dst_rowstride;
			}
		}
		else
		{
			for (y = 0; y < im->dh; y++)
			{
				dy = y + 0.5;
				run_x0 = 0;
				run_x1 = im->dw;
				affine_run (&run_x0, &run_x1, y, src_width, src_height, inv);
				dst_p = dst_linestart + run_x0 * RASTER_BPP;
				for (x = run_x0; x < run_x1; x++)
				{
					dx = x + 0.5;
					src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
					src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
					src_p = src + (src_y * src_width) + src_x;
					*dst_p++ = cmap[*src_p].red;
					*dst_p++ = cmap[*src_p].green;
					*dst_p++ = cmap[*src_p].blue;
				}
				dst_linestart += dst_rowstride;
			}
		}
	}
}


/*================== copy, combine and blend functions ==================*/


/*  Calculate the paramters required for the copy, combine and blend fcns.
 *  The parameters eliminate multiplication within the image processing loop.
 *  This is important when you are dealing with 1M or so operations.
 */
static void calc_parms( ImagePtr dst, ImagePtr src, 
						int *run_x0, int *run_x1, int *run_y0, int *run_y1,
						UNCHAR **dst_linestart, int *dst_rowstride, int *dst_run_x,
						UNCHAR **src_linestart, int *src_rowstride, int *src_run_x )
{
	int  tx, ty, xi, yi;

	tx = src->dx - dst->dx;
	ty = src->dy - dst->dy;

	*run_x0 = 0;
	*run_x1 = dst->dw;
	if (tx > *run_x0) *run_x0 = tx;
	xi = src->dw + tx;
	if (xi < *run_x1) *run_x1 = xi;

	*run_y0 = 0;
	*run_y1 = dst->dh;
	if (ty > *run_y0) *run_y0 = ty;
	yi = src->dh + ty;
	if (yi < *run_y1) *run_y1 = yi;

	*dst_rowstride = dst->dw * RASTER_BPP;
	*dst_run_x     = (*run_x0) * RASTER_BPP;
	*dst_linestart = dst->raster + (*run_y0) * (*dst_rowstride);

	*src_rowstride = src->dw * RASTER_BPP;
	*src_run_x     = ((*run_x0) - tx) * RASTER_BPP;
	*src_linestart = src->raster + ((*run_y0) - ty)*(*src_rowstride);
}



/* Copy a source image to a destination image. The combine will do the
 * same thing, but for just one image where we do not have to worry about
 * transparency this should be  faster.
 */
static void copy_image( ImagePtr dst, const ImagePtr src )
{
	int     y;
	size_t  ncpy;
	int     run_x0, run_x1, run_y0, run_y1;
	int     src_rowstride, src_run_x, dst_rowstride, dst_run_x;
	UNCHAR *src_linestart, *dst_linestart;
	UNCHAR *dst_p, *src_p;

	if(IsNull(src->raster)) return;

	calc_parms(dst, src,
				&run_x0, &run_x1, &run_y0, &run_y1,
				&dst_linestart, &dst_rowstride, &dst_run_x,
				&src_linestart, &src_rowstride, &src_run_x );

	src_p = src_linestart + src_run_x;
	dst_p = dst_linestart + dst_run_x;
	ncpy  = (size_t)(run_x1 - run_x0) * RASTER_BPP;

	for (y = run_y0; y < run_y1; y++)
	{
		(void) memcpy((void *)dst_p, (void *)src_p, ncpy);
		src_p += src_rowstride;
		dst_p += dst_rowstride;
	}
}


static void combine_image( ImagePtr dst, const ImagePtr src )
{
	int     x, y;
	UNCHAR *dst_p, *src_p;
	int     run_x0, run_x1, run_y0, run_y1;
	int     src_rowstride, src_run_x, dst_rowstride, dst_run_x;
	UNCHAR *src_linestart, *dst_linestart;

	if(IsNull(src->raster)) return;

	calc_parms(dst, src,
				&run_x0, &run_x1, &run_y0, &run_y1,
				&dst_linestart, &dst_rowstride, &dst_run_x,
				&src_linestart, &src_rowstride, &src_run_x );

	for (y = run_y0; y < run_y1; y++)
	{
		src_p = src_linestart + src_run_x;
		dst_p = dst_linestart + dst_run_x;
		for (x = run_x0; x < run_x1; x++)
		{
			if(OPAQUE_PIXEL(src_p))
			{
				*dst_p++ = *src_p++;
				*dst_p++ = *src_p++;
				*dst_p++ = *src_p++;
			}
			else
			{
				src_p += RASTER_BPP;
				dst_p += RASTER_BPP;
			}
		}
		src_linestart += src_rowstride;
		dst_linestart += dst_rowstride;
	}
}


static void blend_image( ImagePtr dst, const ImagePtr src )
{
	int     x, y, ratio, d, s;
	UNCHAR *dst_p, *src_p;
	int     run_x0, run_x1, run_y0, run_y1;
	int     src_rowstride, src_run_x, dst_rowstride, dst_run_x;
	UNCHAR *src_linestart, *dst_linestart;

	if(IsNull(src->raster)) return;

	calc_parms(dst, src,
				&run_x0, &run_x1, &run_y0, &run_y1,
				&dst_linestart, &dst_rowstride, &dst_run_x,
				&src_linestart, &src_rowstride, &src_run_x );

	/* Convert ratio from percent into parts per 256. Some compilers can speed
	 * up the math by using bit shifts in this case instead of divides.
	 */
	ratio = (dst->info.synth->blend_ratio * 256) / 100;

	for (y = run_y0; y < run_y1; y++)
	{
		dst_p = dst_linestart + dst_run_x;
		src_p = src_linestart + src_run_x;
		for (x = run_x0; x < run_x1; x++)
		{
			if(OPAQUE_PIXEL(src_p))
			{
				if(OPAQUE_PIXEL(dst_p))
				{
					d = (int)*dst_p;
					s = (int)*src_p++;
					*dst_p++ = (UNCHAR)(d + ((s - d)*ratio)/256);
					d = (int)*dst_p;
					s = (int)*src_p++;
					*dst_p++ = (UNCHAR)(d + ((s - d)*ratio)/256);
					d = (int)*dst_p;
					s = (int)*src_p++;
					*dst_p++ = (UNCHAR)(d + ((s - d)*ratio)/256);
				}
				else
				{
					*dst_p++ = *src_p++;
					*dst_p++ = *src_p++;
					*dst_p++ = *src_p++;
				}
			}
			else
			{
				dst_p += RASTER_BPP;
				src_p += RASTER_BPP;
			}
		}
		dst_linestart += dst_rowstride;
		src_linestart += src_rowstride;
	}
}


/*================= Small local utility functions ===============*/


static LOGICAL invalid_lut( ImageLUT lut )
{
	if(glImageLUTisValid(lut)) return FALSE;
	pr_error(ActiveModule, "Invalid ImageLUT passed to function.");
	reset_module_name();
	return TRUE;
}


static void set_module_name( STRING id )
{
	int i;
	for( i = ModuleStackLen - 1; i > 0; i-- ) ModuleStack[i] = ModuleStack[i-1];
	ModuleStack[0] = ActiveModule;
	ActiveModule = id;
}


static void reset_module_name( void )
{
	int i;
	ActiveModule = ModuleStack[0];
	for( i = 1; i < ModuleStackLen; i++ ) ModuleStack[i-1] = ModuleStack[i];
}


/* This function will call the reset_module_name() function if the return is TRUE.
 * Thus the calling function should return when this function returns true and
 * not execute the reset function.
 */
static LOGICAL invalid_image( Image ndx )
{
	if(valid_image(ndx)) return FALSE;
	pr_error(ActiveModule, "Invalid Image pointer.");
	reset_module_name();
	return TRUE;
}



/*==================== Affine calculation functions ===============*/


/*
 * Find the inverse of an affine transformation.
 * dst: The resulting affine transformation
 * src: The original affine transformation.
 */
static void invert_affine (float dst[6], const float src[6])
{
	float r_det;

	r_det = 1.0 / (src[0] * src[3] - src[1] * src[2]);
	dst[0] = src[3] * r_det;
	dst[1] = -src[1] * r_det;
	dst[2] = -src[2] * r_det;
	dst[3] = src[0] * r_det;
	dst[4] = -src[4] * dst[0] - src[5] * dst[2];
	dst[5] = -src[4] * dst[1] - src[5] * dst[3];
}



/* Determine a "run", such that the inverse affine of all pixels from (x0, y)
 * inclusive to (x1, y) exclusive fit within the bounds of the source image.
 *
 * Initial values of x0, x1, and result values stored in first two pointer arguments.
 */
static void affine_run (int *p_x0, int *p_x1, const int y, const int width, const int height,
						const float affine[6])
{
	int    x0, x1, xi;
	float  z, x_intercept;

	x0 = *p_x0;
	x1 = *p_x1;

	/* do left and right edges */
	if (affine[0] > FLT_EPSILON)
	{
		z = affine[2] * (y + 0.5) + affine[4];
		x_intercept = -z / affine[0];
		xi = (int) ceilf (x_intercept + FLT_EPSILON - 0.5);
		if (xi > x0) x0 = xi;
		x_intercept = (-z + width) / affine[0];
		xi = (int) ceilf (x_intercept - FLT_EPSILON - 0.5);
		if (xi < x1) x1 = xi;
	}
	else if (affine[0] < -FLT_EPSILON)
	{
		z = affine[2] * (y + 0.5) + affine[4];
		x_intercept = (-z + width) / affine[0];
		xi = (int) ceilf (x_intercept + FLT_EPSILON - 0.5);
		if (xi > x0) x0 = xi;
		x_intercept = -z / affine[0];
		xi = (int) ceilf (x_intercept - FLT_EPSILON - 0.5);
		if (xi < x1) x1 = xi;
	}
	else
	{
		z = affine[2] * (y + 0.5) + affine[4];
		if (z < 0 || z >= (float) width)
		{
			*p_x1 = *p_x0;
			return;
		}
	}

	/* do top and bottom edges */
	if (affine[1] > FLT_EPSILON)
	{
		z = affine[3] * (y + 0.5) + affine[5];
		x_intercept = -z / affine[1];
		xi = (int) ceilf (x_intercept + FLT_EPSILON - 0.5);
		if (xi > x0) x0 = xi;
		x_intercept = (-z + height) / affine[1];
		xi = (int) ceilf (x_intercept - FLT_EPSILON - 0.5);
		if (xi < x1) x1 = xi;
	}
	else if (affine[1] < -FLT_EPSILON)
	{
		z = affine[3] * (y + 0.5) + affine[5];
		x_intercept = (-z + height) / affine[1];
		xi = (int) ceilf (x_intercept + FLT_EPSILON - 0.5);
		if (xi > x0) x0 = xi;
		x_intercept = -z / affine[1];
		xi = (int) ceilf (x_intercept - FLT_EPSILON - 0.5);
		if (xi < x1) x1 = xi;
	}
	else
	{
		z = affine[3] * (y + 0.5) + affine[5];
		if (z < 0 || z >= (float) height)
		{
			*p_x1 = *p_x0;
			return;
		}
	}
	*p_x0 = x0;
	*p_x1 = x1;
}


/*================ Data Image Sampling ======================*/


/* sample an image given an array of latitude-longitude coordinates. Note that since we read
 * up the source image it is important to use the original map projection as found in 
 * mproj_org structure and not anything that has been done to display it on the screen. We
 * can not use the projection in the imdef structure as the projection can change from image
 * to image (polar orbiting satellites).
 */
static void lat_lon_sample_image( Image ndx, STRING item, LatLonPoint *p, int np, float *vals )
{
	int      n, ix, iy;
	float    xpkm, ypkm;
	UNCHAR   *src, *mask;
	POINT    point;
	ImagePtr im;

	im  = Xgl.images[ndx];
	src = (UNCHAR *)0;
	for(n=0; n < np; n++) vals[n] = FLT_MIN;

	if (im->group == DataGroup && NotNull(im->info.grid))
	{
		/* x and y pixels/km */
		xpkm = (float)im->ow/(im->mproj_org.definition.xlen * im->mproj_org.definition.units / 1000.);
		ypkm = (float)im->oh/(im->mproj_org.definition.ylen * im->mproj_org.definition.units / 1000.);
		for(n=0; n < np; n++)
		{
			(void) ll_to_pos(&im->mproj_org, p[n].lat, p[n].lon, point);
			ix = (int)(point[X]*xpkm + .5);
			iy = (int)(point[Y]*ypkm + .5);
			if (ix >= 0 && ix < im->ow && iy >= 0 && iy < im->oh)
			{
				if(IsNull(src) && !_xgl_get_source_image(im, &src, &mask)) break;
				vals[n] = value_from_pixel(im, src, ix, iy);
			}
		}
		FREEMEM(src);
		FREEMEM(mask);
	}
	else if (im->group == RadarGroup && NotNull(im->info.radar))
	{
		src = (UNCHAR*)0;
		if (im->imdef->mproj.projection.type == ProjectNone)
		{
			for(ndx=0; ndx<im->info.radar->nitems; ndx++)
			{
				if(!same_ic(item, im->info.radar->item_id[ndx])) continue;
				if(IsNull(src) && !_xgl_get_source_image(im, &src, &mask)) break;
				for(n=0; n < np; n++)
				{
					vals[n] = radar_proj_lat_lon_val(im, p[n].lat, p[n].lon, ndx, src);
				}
				FREEMEM(src);
				FREEMEM(mask);
				break;
			}
		}
	}
}


/* sample an image given an array of map coordinates
 */
static void sample_image( Image ndx, STRING item, Coord *cx, Coord *cy, int ncp, float *vals )
{
	int      x, y, n, run_x0, run_x1, src_x, src_y;
	UNCHAR   *src, *mask;
	float    dx, dy, coef[6], inv[6];
	ImagePtr im;

	im   = Xgl.images[ndx];
	src  = (UNCHAR *)0;
	mask = (UNCHAR *)0;

	if (im->group == DataGroup && NotNull(im->info.grid))
	{
		calculate_geometry( im );
		_xgl_affine_coef(coef, im->sx, im->sy, im->ra, im->tx, im->ty );
		invert_affine(inv, coef);

		/* is our data item valid for this particular image?
		 */
		for(n = 0; n < ncp; n++ )
		{
			x = XS(cx[n],cy[n]) - im->dx;
			y = YS(cx[n],cy[n]) - im->dy;

			/* Is our point within the bounds of the image?
			 */
			if (x < 0 || x >= im->dw || y < 0 || y >= im->dh) continue;

			/* Do the affine_run to get the range of x that will contain the
			 * source image. x must be within this range.
			 */
			run_x0 = 0;
			run_x1 = im->dw;
			affine_run (&run_x0, &run_x1, y, im->ow, im->oh, inv);
			if( x < run_x0 || x >= run_x1) continue;

			/* From the screen position find the source image point that maps to our
			 * screen position and return the data value that corresponds to that point.
			 */
			dx = x + 0.5;
			dy = y + 0.5;
			src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
			src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
			if(IsNull(src) && !_xgl_get_source_image(im, &src, &mask)) break;
			vals[n] = value_from_pixel(im, src, src_x, src_y);
		}
		FREEMEM(src);
		FREEMEM(mask);
	}
	else if(im->group == RadarGroup && NotNull(im->info.radar))
	{
		calculate_geometry( im );
		_xgl_affine_coef(coef, im->sx, im->sy, im->ra, im->tx, im->ty );
		invert_affine(inv, coef);

		/* is our data item valid for this particular image?
		 */
		for(ndx=0; ndx<im->info.radar->nitems; ndx++)
		{
			if(same(item, im->info.radar->item_id[ndx]))
			{
				for(n = 0; n < ncp; n++ )
				{
					x = XS(cx[n],cy[n]) - im->dx;
					y = YS(cx[n],cy[n]) - im->dy;

					if (x < 0 || x >= im->dw || y < 0 || y >= im->dh) continue;

					/* Do the affine_run to get the range of x that will contain the
					 * source image. x must be within this range.
					 */
					run_x0 = 0;
					run_x1 = im->dw;
					affine_run (&run_x0, &run_x1, y, im->ow, im->oh, inv);
					if( x < run_x0 || x >= run_x1) continue;

					if(IsNull(src) && !_xgl_get_source_image(im, &src, &mask)) break;

					vals[n] = get_radar_value(im, x, y, ndx, src, inv);
				}
				FREEMEM(src);
				FREEMEM(mask);
				break;
			}
		}
	}
}


/*==================== Raster Management ==========================*/


/* Allocate an image raster,
 */
static LOGICAL create_raster( ImagePtr im )
{
	UNCHAR *p, *e;
	int    size;

	if (!im) return FALSE;
	if (im->raster) return TRUE;

	size = im->dw * im->dh * RASTER_BPP;
	im->raster = MEM(UNCHAR, size);
	if(!im->raster)
	{
		pr_error(ActiveModule, mem_error_msg);
		return FALSE;
	}

	/* Initialize raster to transparent */
	p = im->raster;
	e = im->raster + size;
	while(p < e)
	{
		*p++ = T_RED;
		*p++ = T_GREEN;
		*p++ = T_BLUE;
	}

	/* add to raster queue */
	if(raster_queue_len >= raster_queue_max)
		raster_queue = GETMEM(raster_queue, ImagePtr, ++raster_queue_max);
	raster_queue[raster_queue_len] = im;
	raster_queue_len++;

	return TRUE;
}


/* Free all entries in the raster queue */
static void clear_raster_queue(void)
{
	int n;
	for(n = 0; n < raster_queue_len; n++)
	{
		FREEMEM(raster_queue[n]->raster);
	}
	raster_queue_len = 0;
}


/*=============== Color Brightness Manipulation  =================*/


static void scale_pixel_brightness(UNCHAR *r, UNCHAR *g, UNCHAR *b, const float scale)
{
	if( scale == 1.0 || scale == glNORMAL_BRIGHTNESS) return;

	if( *r != T_RED || *g != T_GREEN || *b != T_BLUE)
	{
		float pv = (float)*r * scale;
		if( pv <   0 ) pv = 0;
		if( pv > 255 ) pv = 255;
		*r = (UNCHAR)(pv + 0.5);

		pv = (float)*g * scale;
		if( pv <   0 ) pv = 0;
		if( pv > 255 ) pv = 255;
		*g = (UNCHAR)(pv + 0.5);

		pv = (float)*b * scale;
		if( pv <   0 ) pv = 0;
		if( pv > 255 ) pv = 255;
		*b = (UNCHAR)(pv + 0.5);
	}
}


static void scale_cmap_brightness(glCOLOR *cmap, const float scale)
{
	int n;
	if( scale == 1.0 || scale == glNORMAL_BRIGHTNESS) return;
	for( n = 0; n < 256; n++ )
		scale_pixel_brightness(&cmap[n].red, &cmap[n].green, &cmap[n].blue, scale);
}


/* 
 * Get the brightness scale factor for the images. This allows images to
 * be darkened or brightened depending on the factor. For useful results
 * it normally runs between about .3 and 1.3.
 *
 * Setting priority: 1. the image
 *                   2. image definition group
 *                   3. image product group
 *                   4. image type
 */
static float get_brightness_factor(ImagePtr im)
{
	if(NotNull(im->imdef))
	{
		if(im->imdef->bsf != glNORMAL_BRIGHTNESS)
			return im->imdef->bsf;

		if(NotNull(im->imdef->prod))
		{
			if(im->imdef->prod->bsf != glNORMAL_BRIGHTNESS)
				return im->imdef->prod->bsf;

			if(image_type_bsf[im->imdef->prod->image_type] != glNORMAL_BRIGHTNESS)
				return image_type_bsf[im->imdef->prod->image_type];
		}
	}
	return glNORMAL_BRIGHTNESS;
}


/*================== Radar Geometry Processing Routines ==================*/



const float  rad    = M_PI/180.0;  /* degrees to radians */
const float  pid    = 180.0/M_PI;  /* radians to degrees */
const float  rEarth = 6367.65;     /* mean radius of the earth in km */


/*
 *  Given an origin lat-long point and a destination lat-long point, return the range and
 *  bearing of the destination point from the origin point. All lat-long in decimal degrees.
 *
 *  lat0,lon0  Origin
 *  lat1,lon1  Destination
 *
 *  Development Note:
 *
 *  The great circle distance between two points with coordinates (lat1,lon1) and (lat2,lon2)
 *  is given by:
 *
 *  	d = acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon1-lon2))
 *
 *  The problem with this formula is that when the longitude difference DL is small then cos(DL)
 *  is essentially 1, and arccos(1) = 0. We loose all accuracy in the case where DL and
 *  |lat1-lat2| are both small. The problem is that cosines and arccosines do not have the
 *  property that f(0)=0. What is really wanted is a formula based on sines/arcsines or
 *  tangents/arctangents which do have this property. We thus use the old relation
 *
 *    sin(dL/2)^2 = (1-cos(dl))/2  or cos(dl) = 1-2sin(dl/2)^2
 *
 *  This gives us the mathematically equivalent formula
 *
 *    d = 2 asin( sqrt( sin(dl/2)^2 + cos(l1)cos(l2)sin(dL/2)^2 ) )
 *
 *  where dl is the latitude difference and dL the longitude difference. This gives much
 *  better results at all distances.
 */
static void lat_long_to_RAZ(float lat1, float lon1, float lat0, float lon0, float *range, float *bearing)
{
	float  sa, slat, slon, slat2, slon2, dlon, dist, azmet, rlat0, rlat1, clat0;

	rlat0 = lat0 * rad;
	rlat1 = lat1 * rad;
	slat  = SIN((rlat1-rlat0)/2.);
	slat2 = slat*slat;
	dlon  = (lon1-lon0)*rad;
	slon  = SIN(dlon/2.);
	slon2 = slon*slon;
	clat0 = COS(rlat0);
	sa    = SQRT(slat2 + clat0*COS(rlat1)*slon2);
	dist  = 2. * ASIN(MIN(1.,sa));

	azmet = ACOS((SIN(rlat1)-SIN(rlat0)*COS(dist))/(SIN(dist)*clat0));
	if(dlon < 0) azmet = 2.*M_PI - azmet;

	if (range)   *range   = (float)(dist * rEarth);
	if (bearing) *bearing = (float)(azmet * pid);
}



/*  Given an origin lat-long point, assumed to be at the center of a radar grid, and a destination
 *  lat-long point, return the grid coordinates of the destination point. The coordinates are in km.
 *
 *  lat1,lon1 - destination latitude-longitude
 *  lat0,lon0 - radar grid center location
 *  x,y       - grid coordinates
 */
static void lat_long_to_radar_grid(float lat1, float lon1, float lat0, float lon0, float *x, float *y)
{
	float  sa, slat, slon, slat2, slon2, dlon, dist, bearing, rlat0, rlat1, clat0;

	rlat0 = lat0 * rad;
	rlat1 = lat1 * rad;
	slat  = SIN((rlat1-rlat0)/2.);
	slat2 = slat*slat;
	dlon  = (lon1-lon0)*rad;
	slon  = SIN(dlon/2.);
	slon2 = slon*slon;
	clat0 = COS(rlat0);
	sa    = SQRT(slat2 + clat0*COS(rlat1)*slon2);
	dist  = 2. * ASIN(MIN(1.,sa));

	bearing = ACOS((SIN(rlat1)-SIN(rlat0)*COS(dist))/(SIN(dist)*clat0));
	if(dlon < 0) bearing = 2.*M_PI - bearing;

	dist *= rEarth;
	*x = (float)(dist * SIN(bearing));
	*y = (float)(dist * COS(bearing));
}



/* From the screen position find the source image point that maps to our
 * screen position and return the data value that corresponds to that point.
 */
static float get_radar_value(ImagePtr im, int x, int y, int ndx, UNCHAR *src, float inv[6])
{
	int    src_x, src_y, ival, radius, theta;
	float  val = glDATA_MISSING;
	UNCHAR   *src_p;
	float  dx, dy, dist, bear;

	if(im->encoding == ImageEncodingGriddedURP)
	{
		dx = x + 0.5;
		dy = y + 0.5;
		src_x = (int)floorf(dx * inv[0] + dy * inv[2] + inv[4]);
		src_y = (int)floorf(dx * inv[1] + dy * inv[3] + inv[5]);
		src_p = src + (src_y * im->ow) + src_x;
		ival = (int)*src_p;
		val = im->info.radar->item_values[ival*im->info.radar->nitems+ndx];
	}
	else if(im->encoding == ImageEncodingPolarURP)
	{
		float  offset = im->info.radar->range * im->info.radar->rscale;
		dx = x * inv[0] + y * inv[2] + inv[4] - offset;
		dy = x * inv[1] + y * inv[3] + inv[5] - offset;
		dist = SQRT(dx*dx + dy*dy);
		bear = (dx==0) ? ((dy<0)? 180.:0.) : (((dx<0)? 270.:90.) - ATAN(-dy/dx)*180./M_PI);
		radius = (int)floorf(dist/im->info.radar->rscale + .5);
		theta  = (int)floorf(bear/im->info.radar->tscale + .5);
		if(radius < im->info.radar->range)
		{
			src_p = src + (theta * im->info.radar->range) + radius;
			ival = (int)*src_p;
			val = im->info.radar->item_values[ival*im->info.radar->nitems+ndx];
		}
	}
	return val;
}



/* Find the radar value on a radar projection at a given latitude-longitude point
 */
static float radar_proj_lat_lon_val( ImagePtr im, float lat, float lon, int ndx, UNCHAR *src )
{
	int      ix, iy, ival;
	float    x, y, del, dist, bear, units;
	float    val = glDATA_MISSING;
	UNCHAR   *src_p;
	POINT    point;

	if(im->encoding == ImageEncodingGriddedURP)
	{
		if(im->imdef->mproj.projection.type == ProjectNone)
		{
			lat_long_to_radar_grid(lat, lon, im->imdef->mproj.definition.olat, im->imdef->mproj.definition.olon, &x, &y);
			del = (float)im->ow/2.0;
			ix = (int)(x/im->info.radar->rscale + del);
			iy = (int)(y/im->info.radar->rscale + del);
		}
		else
		{
			(void) ll_to_pos(&im->imdef->mproj, lat, lon, point);
			units = im->imdef->mproj.definition.units / 1000.;
			ix = (int)(point[X] * (float)im->ow / (im->imdef->mproj.definition.xlen*units) + .5);
			iy = (int)(point[Y] * (float)im->oh / (im->imdef->mproj.definition.ylen*units) + .5);
		}
		if (ix >= 0 && ix < im->ow && iy >= 0 && iy < im->oh)
		{
			src_p = src + iy * im->ow + ix;
			ival = (int)*src_p;
			val = im->info.radar->item_values[ival*im->info.radar->nitems+ndx];
		}
	}
	else if(im->encoding == ImageEncodingPolarURP)
	{
		lat_long_to_RAZ(lat, lon, im->imdef->mproj.definition.olat, im->imdef->mproj.definition.olon, &dist, &bear);
		ix = (int)floorf(dist/im->info.radar->rscale + .5);
		iy = (int)floorf(bear/im->info.radar->tscale + .5) % im->info.radar->theta;
		if( ix < im->info.radar->range )
		{
			src_p = src + iy * im->info.radar->range + ix;
			ival = (int)*src_p;
			val = im->info.radar->item_values[ival*im->info.radar->nitems+ndx];
		}
	}
	return val;
}
