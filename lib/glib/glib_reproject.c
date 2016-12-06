/***********************************************************************************/
/*
*	File:		glib_reproject.c
*
*	Purpose:	Contains functions to deal with the reprojection of raster
*	            images from some map projection to the active projection. 
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
#include <limits.h>
#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <fpa_macros.h>
#include "glib_private.h"


/* Working directory list used by open_reprojected_file.
 */
static int     nwdlist = 0;
static STRING *wdlist  = NULL;



/* Given a target projection, determine a set of new projection parameters
 * for a given source projection that will map into the area of the target. 
 * The new projection will be clipped to the target projection, but may be
 * smaller than the target if only a portion of the source projection maps
 * to the target projection.
 *
 * param[in]	src_proj    Source map projection
 * param[in]	target_proj Target map projection
 * param[out]	new_proj    New map projection
 *
 * return True if successful. False means that none of the source will map to the target.
 */
static LOGICAL define_reprojected_map_definition( MAP_PROJ *src_proj, MAP_PROJ *target_proj, MAP_PROJ *new_proj )
{
	int      n, ix, iy;
	float    lat, lon;
	POINT	 pos, srcpos, dstpos, minpos, maxpos;
	MAP_DEF  mdef;

	/* Find the box in the target projection that contains part of the source image.
	 * This is done by walking around the perimiter of the source image and finding
	 * the box in position units. Note that determining the exact size of the box
	 * is critical. The border walk around can lead to small errors in the box size
	 * which affects the scaling so we must clip to the target map and not just
	 * check if we are inside. Thus we convert back to the source map after clipping
	 * to the target map to catch any clipping results that are outside the source.
	 */
	minpos[X] = minpos[Y] = FPA_FLT_MAX;
	maxpos[X] = maxpos[Y] = FPA_FLT_MIN;

	for (ix = 0; ix < 2; ix++)
	{
		int   npt = ((ix)? src_proj->grid.nx:src_proj->grid.ny) * 2;
		float del = ((ix)? src_proj->definition.xlen:src_proj->definition.ylen)/(float)npt;

		for (iy = 0; iy < 2; iy++)
		{
			if (ix) pos[Y] = (iy)? src_proj->definition.ylen : 0;
			else    pos[X] = (iy)? src_proj->definition.xlen : 0;

			for (n = 0; n < npt; n++)
			{
				if (ix) pos[X] = (float)n * del;
				else    pos[Y] = (float)n * del;

				if (!clip_to_map_def(&src_proj->definition, pos, srcpos)      ) continue;
				if (!pos_to_pos(src_proj, srcpos, target_proj, dstpos)        ) continue;
				if (!clip_to_map_def(&target_proj->definition, dstpos, dstpos)) continue;
				if (!pos_to_pos(target_proj, dstpos, src_proj, srcpos)        ) continue;
				if (!inside_map_def(&src_proj->definition, srcpos)            ) continue;

				if (minpos[X] > dstpos[X]) minpos[X] = dstpos[X];
				if (minpos[Y] > dstpos[Y]) minpos[Y] = dstpos[Y];
				if (maxpos[X] < dstpos[X]) maxpos[X] = dstpos[X];
				if (maxpos[Y] < dstpos[Y]) maxpos[Y] = dstpos[Y];
			}
		}
	}

	/* This means that none of the source falls within the bounds of the target. */
	if (maxpos[X] <= minpos[X] || maxpos[Y] <= minpos[Y]) return FALSE;

	/*  Define the new map definition that is bounded by the above. It has its
	 *  origin in the lower left corner for simplicity.
	 */
	(void) pos_to_ll(target_proj, minpos, &lat, &lon);

	mdef.olat  = lat;
	mdef.olon  = lon;
	mdef.lref  = target_proj->definition.lref;
	mdef.xorg  = 0;
	mdef.yorg  = 0;
	mdef.xlen  = maxpos[X] - minpos[X];
	mdef.ylen  = maxpos[Y] - minpos[Y];
	mdef.units = target_proj->definition.units;

	define_map_projection(new_proj, &target_proj->projection, &mdef, &src_proj->grid);

	/* Calculate the width and height of the new image in pixels. The new pixel size is related
	 * to the size of the new image.
	 */
	if(src_proj->projection.type == ProjectLatLon || src_proj->projection.type == ProjectLatLonAng)
	{
		/* The lat-long projection is a special case as the axis lengths are in degrees not metres.
		 * Since the width and height of images are related to distances, the remapped image must
		 * have the number of pixels in both dimensions related to distance. Limit it to 1000km.
		 */
		float lx = new_proj->definition.xlen * new_proj->definition.units/1000.;
		float ly = new_proj->definition.ylen * new_proj->definition.units/1000.;
		while(lx > 1000. || ly > 1000.)
		{
			lx /= 2.;
			ly /= 2.;
		}
		new_proj->grid.nx = NINT(lx);
		new_proj->grid.ny = NINT(ly);
	}
	else
	{
		new_proj->grid.nx = (int)((float)src_proj->grid.nx *
			(new_proj->definition.xlen*new_proj->definition.units)/(src_proj->definition.xlen*src_proj->definition.units) + 0.5);
		new_proj->grid.ny = (int)((float)src_proj->grid.ny *
			(new_proj->definition.ylen*new_proj->definition.units)/(src_proj->definition.ylen*src_proj->definition.units) + 0.5);
	}
	return TRUE;
}


/* Reproject a raster.
 *
 * For the source:
 *
 * param[in]	src_proj   map projection
 * param[in]	src_raster raster as an unsigned char array
 * param[in]	src_mask   array of size one bit per pixel of the source raster
 * param[in]	src_bpp    bytes per pixel (value between 1 and 4)
 *
 * For the target:
 *
 * param[in]	target_proj          map projection
 * param[in]    target_pixel_init    value used to initialize the pixels.
 *									 This is a UNCHAR* array and is expected
 *									 to be src_bpp in length
 * param[out]	target_raster        raster to hold the output
 * param[out]	target_raster_size   size of the raster in bytes
 * param[out]	target_mask          raster sized as one bit per pixel
 * param[out]	target_mask_size     length of the mask in bytes
 *
 * Note 1. The target bpp will be the same as the source.
 *      2. The target_pixel_init array must be of length bpp and each
 *         byte will initialize a corresponding byte in each pixel.
 *      3. Both the src_mask and target_mask may be NULL. A src_mask
 *         of NULL means that all of the source pixels are available.
 *         A target_mask of NULL means that no target mask is to be
 *         generated.
 *
 */
static void reproject
	( 
	MAP_PROJ *src_proj, 
	UNCHAR *src_raster, 
	UNCHAR *src_mask,
	int src_bpp,
	MAP_PROJ *target_proj, 
	UNCHAR *target_pixel_init,
	UNCHAR **target_raster, 
	int    *target_raster_size,
	UNCHAR **target_mask,
	int    *target_mask_size
	)
{
	int      ix, iy, dpx, sx, sy, size;
	float    pux, puy, upx, upy;
	UNCHAR   *dp, *sp, *raster;
	POINT	 srcpos, dstpos;
	UNCHAR   *mask = (UNCHAR*)NULL;
	int      mask_len = 0;

	if (!target_raster) return;

	size = target_proj->grid.nx * target_proj->grid.ny * src_bpp;
	raster = INITMEM(UNCHAR, size);
	(void) memset(raster, 0, size);

	if (target_pixel_init)
	{
		sp = raster;
		dp = raster + size;
		switch(src_bpp)
		{
			case 1:
				while(sp < dp)
				{
					*sp++ = target_pixel_init[0];
				}
				break;

			case 2:
				while(sp < dp)
				{
					*sp++ = target_pixel_init[0];
					*sp++ = target_pixel_init[1];
				}
				break;

			case 3:
				while(sp < dp)
				{
					*sp++ = target_pixel_init[0];
					*sp++ = target_pixel_init[1];
					*sp++ = target_pixel_init[2];
				}
				break;

			case 4:
				while(sp < dp)
				{
					*sp++ = target_pixel_init[0];
					*sp++ = target_pixel_init[1];
					*sp++ = target_pixel_init[2];
					*sp++ = target_pixel_init[3];
				}
				break;
		}
	}

	if (target_mask)
	{
		mask_len = MASK_SIZE(target_proj->grid.nx, target_proj->grid.ny);
		mask     = INITMEM(UNCHAR, mask_len);
		(void) memset(mask, 0, mask_len);
	}

	/*  pixels per source length */
	pux = (float)src_proj->grid.nx  / src_proj->definition.xlen;
	puy = (float)src_proj->grid.ny / src_proj->definition.ylen;

	/* length per target pixel */
	upx = target_proj->definition.xlen / (float)target_proj->grid.nx;
	upy = target_proj->definition.ylen / (float)target_proj->grid.ny;


	/* Scan the new projected area and copy in whatever pixels
	 * are required from the source. Note that the map definition
	 * has its origin in the lower left corner while the raster
	 * is the upper left. Thus the reversal on the vertical axis.
	 */
	for ( iy = 0; iy < target_proj->grid.ny; iy++ )
	{
		int ndx = (target_proj->grid.ny - iy - 1) * target_proj->grid.nx;
		dpx = ndx * src_bpp;
		dstpos[Y] = (float)iy * upy;

		for ( ix = 0; ix < target_proj->grid.nx; ix++ )
		{
			dstpos[X] = (float)ix * upx;
			if (pos_to_pos(target_proj,dstpos,src_proj,srcpos))
			{
				sx = (int)(srcpos[X] * pux);
				sy = (int)(srcpos[Y] * puy);
				if (sx >= 0 && sx < src_proj->grid.nx && sy >= 0 && sy < src_proj->grid.ny)
				{
					int p = ((src_proj->grid.ny-sy-1)*src_proj->grid.nx + sx);
					dp  = raster + dpx;
					sp  = src_raster + p * src_bpp;
					switch (src_bpp)
					{
						case 4: *dp++ = *sp++;
						case 3: *dp++ = *sp++;
						case 2: *dp++ = *sp++;
						case 1: *dp++ = *sp++;
					}
					if (mask != NULL && (src_mask == NULL || MASK_BIT_SET(src_mask,p)))
					{
						p = ndx + ix;
						SET_MASK_BIT(mask,p);
					}
				}
			}
			dpx += src_bpp;
		}
	}

	if (target_raster)      *target_raster      = raster;
	if (target_raster_size) *target_raster_size = size;
	if (target_mask)        *target_mask        = mask;
	if (target_mask_size)   *target_mask_size   = mask_len;
}


/* Open a file for a reprojected image in the working directory. We do this
 * so that the reprojection only has to be done once. The code requires some
 * explanation. The reprojected image is stored in the current working
 * directory. If the process is forked the working directory will change, but
 * the image is still stored in the one in force when the image was reprojected.
 * Thus we need to keep a directory list so we can find the original.
 */
static FILE *open_reprojected_file(ImagePtr im, STRING mode, LOGICAL complain)
{
	int    n;
	STRING impath, path, fname;
	FILE   *fp;

	if(!im) return NULL;
	if(!im->imdef) return NULL;

	/* Have we already been here and assigned a directory? */
	if(im->reprowd < 1)
	{
		for( n = 0; n < nwdlist; n++ )
		{
			if(same(wdlist[n], Xgl.work_directory)) break;
		}
		if( n >= nwdlist )
		{
			wdlist = GETMEM(wdlist, STRING, n+1);
			wdlist[nwdlist++] = INITSTR(Xgl.work_directory);
		}
		im->reprowd = (char)(n + 1);
	}

	impath = _xgl_make_image_file_path(im->imdef->tag, im->vtime);
	fname = base_name(impath, NULL);
	path = pathname(wdlist[(int)im->reprowd-1], fname);
	fp = fopen(path, mode);
	if(IsNull(fp))
	{
		if(complain) pr_error(ActiveModule, "Unable to open file \"%s\" for %s\n",
								path, (*mode == 'r')?"reading":"writing");
		FREEMEM(impath);
		return NULL;
	}


	/* If the library is active for a long time our reprojected files could
	 * consume a coniderable amount of disk space. Thus we will record the
	 * files and then check to see if we can remove some if the original
	 * source is gone. This is only done after a minimum of 30 minutes has
	 * ellapsed since our last check.
	 */
	if( *mode == 'w' )
	{
		/* Append our current image paths to our recording file */
		FILE *rpf;
		fname = safe_strdup(path);
		path  = pathname(wdlist[(int)im->reprowd-1], "rfl1");
		if((rpf = fopen(path, "a")))
		{
			(void) fprintf(rpf, "%s %s\n", impath, fname);
			(void) fclose(rpf);
		}
		FREEMEM(fname);
	}
	else
	{
		static time_t ltime = 0;

		/* Check the file list for removals after 30 minutes has passed */
		if(time(NULL) - ltime > 1800)
		{
			char f1[2000], f2[2000];
			FILE *fp1, *fp2;

			/* Rename our file and read and write back valid entries */
			(void) safe_strcpy(f1, pathname(wdlist[(int)im->reprowd-1], "rfl1"));
			(void) safe_strcpy(f2, pathname(wdlist[(int)im->reprowd-1], "rfl2"));
			(void) rename(f1, f2);
			if((fp2 = fopen(f2, "r")))
			{
				if((fp1 = fopen(f1, "w")))
				{
					/* Update the time only if both opens are ok */
					ltime = time(NULL);
					while(fscanf(fp2, "%s %s", f1, f2) == 2)
					{
						if(access(f1, F_OK))
							(void) unlink(f2);
						else
							(void) fprintf(fp1, "%s %s\n", f1, f2);
					}
					(void) fclose(fp1);
				}
				(void) fclose(fp2);
			}
		}
	}
	FREEMEM(impath);
	return fp;
}



/* Re-project an image from one projection to another projection. As the conversion
 * is slow the re-projection is done when the image information is retrieved and the
 * image is cached.
 */
static LOGICAL reproject_image( ImagePtr im, MAP_PROJ *target_proj)
{
	float    scale;
	POINT	 corner, origin;
	MAP_PROJ new, mp;
	FILE     *fp;

	/* Find the box in the target projection that contains part of the source image.
	 * The grid structure is used to pass through the image size.
	 */
	im->mproj_org.grid.nx = im->ow;
	im->mproj_org.grid.ny = im->oh;
	if (!define_reprojected_map_definition(&im->mproj_org, target_proj, &new)) return FALSE;

	/* If we have a previously reprojected file cached check the map projection. If it is the
	 * same then we just set the parameters as the file contents will still be ok.
	 */
	fp = open_reprojected_file(im, BINARY_READ, FALSE);

	if(IsNull(fp) || fread((void*)&mp,sizeof(MAP_PROJ),1,fp)!=1 || !same_map_projection(&mp,&new))
	{
		int    bpp;
		UNCHAR *src_raster = NULL;
		UNCHAR *src_mask   = NULL;

		if (fp) (void) fclose(fp);
		fp = (FILE*) NULL;

		/* We don't have anything yet so we must create it. Triple band grids do not use a 
		 * mask as the value of any pixel will determine if the pixel is transparent or not.
		 */
		if(_xgl_get_source_image(im, &src_raster, &src_mask))
		{
			UNCHAR *raster;
			UNCHAR *mask     = NULL;
			int    mask_size = 0;

			if(im->group == DataGroup)
			{
				bpp = im->info.grid->bpp;
				reproject(&im->mproj_org, src_raster, src_mask, bpp, &new, NULL, &raster, NULL, &mask, &mask_size);
			}
			else if(im->bands == TripleBand)
			{
				UNCHAR pixel_init[] = {T_RED, T_GREEN, T_BLUE};
				bpp = TripleBand;
				reproject(&im->mproj_org, src_raster, src_mask, bpp, &new, pixel_init, &raster, NULL, NULL, NULL);
			}
			else
			{
				bpp = im->bands;
				reproject(&im->mproj_org, src_raster, src_mask, bpp, &new, NULL, &raster, NULL, &mask, &mask_size);
			}
			FREEMEM(src_raster);
			FREEMEM(src_mask);

			/* Save in a scratch file so we don't have to do this again */
			fp = open_reprojected_file(im, "w", TRUE);
			if(fp)
			{
				size_t mask_t_size = (size_t) mask_size;
				(void)fwrite((void *)&new, sizeof(MAP_PROJ), 1, fp);
				(void)fwrite((void *)&mask_t_size, sizeof(size_t), 1, fp);
				if(mask_size > 0)
					(void)fwrite((void *)mask, 1, mask_t_size, fp);
				if(!_xgl_write_image_file( fp, raster, new.grid.nx, new.grid.ny, bpp ))
				{
					(void) fclose(fp);
					fp = (FILE*) NULL;
				}
			}
			FREEMEM(raster);
			FREEMEM(mask);
		}
	}

	/* If fp is NULL at this point we encountred an error
	 */
	if (!fp) return FALSE;

	(void) fclose(fp);

	/* Reset the image parameters to the reprojection.
	 */
	scale = new.definition.units / target_proj->definition.units;

	copy_point(corner, ZeroPoint);
	(void) pos_to_pos(&new, corner, target_proj, origin);
	im->mx  = origin[X];
	im->my  = origin[Y];
	im->mw  = new.definition.xlen * scale;
	im->mh  = new.definition.ylen * scale;
	im->rx  = origin[X];
	im->ry  = origin[Y];
	im->ra  = 0.;
	im->ow  = new.grid.nx;
	im->oh  = new.grid.ny;
	im->ort = ORT_RPG;

	new.grid.nx = 0;
	new.grid.ny = 0;

	/* Here we redefine the original map projection as well as our base projection
	 * as the reprojected image becomes our "original" image from now on.
	 */
	copy_map_projection(&(im->mproj),     &new);
	copy_map_projection(&(im->mproj_org), &new);

	return TRUE;
}


/************ Visible internally to library functions *****************8*/



/* Remapping is done when the images have no projection or the same
 * projection as the target. In this case we simply need to scale and
 * rotate to align the images.
 */
LOGICAL _xgl_remap_image( ImagePtr im, MAP_PROJ *target)
{
	POINT corner, origin;
	float hfact, vfact, lfact;

	if (im->group == SyntheticGroup || IsNull(target)) return TRUE;

	/* We should never get here it this is true */
	if (!im->imdef) return TRUE;

	/* If the image has already been set to this projection then we return */
	if (same_map_projection(target, &(im->mproj))) return TRUE;

	/* Transform image LL-corner into target map co-ordinates */
	/* (dx,dy do not need to include rotation resize) */
	lfact = im->mproj_org.definition.units / target->definition.units;

	if (im->mproj_org.projection.type == ProjectNone)
	{
		/* Treat ProjectNone as having units in metres instead of degrees */
		/* Add scaling due to latitude distortion */
		(void) ll_distort(target, im->mproj_org.definition.olat, im->mproj_org.definition.olon, &hfact, &vfact);
		(void) ll_to_pos(target, im->mproj_org.definition.olat, im->mproj_org.definition.olon, origin);
		im->mx = origin[X] - lfact * hfact * im->mproj_org.definition.xorg;
		im->my = origin[Y] - lfact * vfact * im->mproj_org.definition.yorg;
		im->mw = lfact * hfact * im->mproj_org.definition.xlen;
		im->mh = lfact * vfact * im->mproj_org.definition.ylen;
	}
	/* Make sure the image projection is compatible with the target map */
	else if (same_projection(&(target->projection), &(im->mproj_org.projection)))
	{
		copy_point(corner, ZeroPoint);
		(void) pos_to_pos(&im->mproj_org, corner, target, origin);
		im->mx = origin[X];
		im->my = origin[Y];
		im->mw = lfact * im->mproj_org.definition.xlen;
		im->mh = lfact * im->mproj_org.definition.ylen;
	}
	/* The source projection is different so we need to do a full reprojection */
	else
	{
		return reproject_image(im, target);
	}

	/* Determine rotation to line up image with target map */
	im->rx = origin[X];
	im->ry = origin[Y];
	im->ra = im->mproj_org.definition.lref - target->definition.lref;
	if( fabsf(im->ra) < 0.0001 ) im->ra = 0.0;

	/* Record the new projection information */
	copy_map_projection(&(im->mproj), target);

	return TRUE;
}



/* function to get the reprojected raster. 
 */
LOGICAL _xgl_rpg_raster(ImagePtr im, UNCHAR **raster, UNCHAR **mask)
{
	size_t   size = 0;
	LOGICAL  rtn  = FALSE;
	FILE     *fp  = open_reprojected_file(im, BINARY_READ, TRUE);

	if (!fp) return FALSE;

	(void) fseek(fp, (long int) sizeof(MAP_PROJ), SEEK_SET);
	(void) fread((void *)&size, sizeof(size_t), 1, fp);
	if(size > 0)
	{
		*mask = MEM(UNCHAR, size);
		(void) fread(*mask, 1, size, fp);
	}
	rtn = _xgl_read_image_file(fp, raster, im->ow, im->oh);
	if (!rtn) FREEMEM(*mask);
	(void) fclose(fp);

	return rtn;
}



void _xgl_free_reprojection_data(void)
{
	FREELIST(wdlist, nwdlist);
	nwdlist = 0;
}
