/*********************************************************************/
/**	@file raster.c
 *
 * Routines to handle operations of raster objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    r a s t e r . c                                                   *
*                                                                      *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                      *
***********************************************************************/

#define RAST_INIT
#include "raster.h"

#include <fpa_getmem.h>
#include <fpa_math.h>

int	RasterCount = 0;

/*******************************************************************************
*                                                                              *
*    c r e a t e _ r a s t e r _ o b j                                         *
*    c r e a t e _ r a s t e r _ m a s k                                       *
*                                                                              *
*******************************************************************************/
/******************************************************************************/
/**   Allocate memory and initialize a new raster object.
 *
 *  @return	New raster object or NullRaster if memory allocation failed.
 ******************************************************************************/

RASTER	create_raster_obj(void)

	{
	RASTER rast;

	/* Allocate memory for the principal structure */ 
	rast = INITMEM(struct RASTER_struct, 1);
	if (!rast) return NullRaster;

	/* Initialize the structure */
	rast->bpp       = 0;
	rast->grid      = (UNCHAR *) 0;
	rast->init      = (UNCHAR *) 0;
	rast->mask      = NullBitMask;
	rast->nrow      = 0;
	rast->ncol      = 0;
	rast->type      = RASTER_EMPTY;
	init_uspec(&rast->units);

	RasterCount++;
	return rast;
	}

/******************************************************************************/
/**   Allocate memory and initialize a new bitmask object.
 *
 *  @return	New bitmask object or NullBitMask if memory allocation failed.
 ******************************************************************************/
BITMASK	create_raster_mask(void)

	{
	BITMASK	mask;

	/* Allocate memory for the principal structure */
	mask = INITMEM(struct BITMASK_struct, 1);
	if (!mask)	return NullBitMask;
	
	/* Initialize the structure */
	mask->bits = (UNCHAR *) 0;
	mask->size = 0;
	return mask;
	}
/*******************************************************************************
*                                                                              *
*   d e f i n e _ r a s t e r                                                  *
*   d e f i n e _ r a s t e r _ m a s k                                        *
*                                                                              *
*******************************************************************************/
/******************************************************************************/
/**  Populate raster object with appropriate values 
 *
 *	@param[in/out]	rast	Value of this raster object will be set
 *	@param[in]		*grid	grid of values associated with raster object
 *	@param[in]		mask	bit mask for grid data
 *	@param[in]		nrow	Number of rows (in pixels)
 *	@param[in]		ncol	Number of columns (in pixels)
 *	@param[in]		bpp		Number of bytes per pixel
 *	@param[in]		init	The "No Value" value
 *	@param[in]		type	Type of raster being stored
 ******************************************************************************/
void	define_raster
	(
	RASTER		 rast,
	UNCHAR		*grid,
	BITMASK		 mask,
	int			 nrow,
	int			 ncol,
	int			 bpp,
	UNCHAR      *init,
	RASTER_TYPE  type
 	)
	{
	/* Do nothing if raster is not allocated */
	if (!rast) return;

	/* >>>>> DO I NEED TO USE MMM_[begin/report]_count? <<<<< */

	rast->nrow  = nrow;
	rast->ncol  = ncol;
	rast->bpp   = bpp;
	rast->type  = type;
	if (init) rast->init  = init;
	if (grid) rast->grid  = grid;
	if (mask) rast->mask  = mask;
	}

/******************************************************************************/
/**  Populate raster object with appropriate values 
 *
 *	@param[in/out]	mask	Value of this mask object will be set
 *	@param[in]		*bits	bit mask for grid data
 *	@param[in]		size	size of mask in bytes
 ******************************************************************************/
void define_raster_mask

	(
	 BITMASK	mask,
	 UNCHAR    *bits,
	 long		size
	)
	{
		if (!mask) return;
		mask->size  = size;
		mask->bits  = bits;
	}

/*******************************************************************************
*                                                                              *
*    d e s t r o y _ r a s t e r                                               *
*    d e s t r o y _ r a s t e r _ m a s k                                     *
*                                                                              *
*******************************************************************************/
/******************************************************************************/
/**    Free the frame buffer of the given raster.
 *
 *  @param[in]  *rast raster to free
 ******************************************************************************/

RASTER	destroy_raster
	(
	RASTER	rast
	)

	{
	if (IsNull(rast)) return NullRaster;

	destroy_raster_mask(rast->mask);
	FREEMEM(rast->grid);
	FREEMEM(rast);
	RasterCount--;
	return NullRaster;
	}

/******************************************************************************/
/**    Free memory of the given bitmask.
 *
 *  @param[in]  *mask bitmask to free
 ******************************************************************************/

BITMASK	destroy_raster_mask
	(
	BITMASK	mask
	)

	{
	if (IsNull(mask)) return NullBitMask;

	FREEMEM(mask->bits);
	FREEMEM(mask);
	return NullBitMask;
	}
/*******************************************************************************
*                                                                              *
*    c o p y _ r a s t e r                                                     *
*    c o p y _ r a s t e r _ m a s k                                           *
*                                                                              *
*******************************************************************************/
/******************************************************************************/
/**    Copy the given raster.
 *
 *  @param[in]	rast
 *  @return		copy of RASTER object.
 ******************************************************************************/

RASTER	copy_raster
	(
	const RASTER rast
	)

	{
	long    size;
	UNCHAR *grid;
	UNCHAR *init;
	RASTER rnew = NullRaster;

	/* Make sure we have something to copy */
	if (!rast) return NullRaster;

	recall_raster_size(rast, &size);
	grid = INITMEM(UNCHAR, size);
	grid = memcpy(grid, rast->grid, size);

	init = INITMEM(UNCHAR, rast->bpp);
	init = memcpy(init, rast->init, rast->bpp);

	rnew = create_raster_obj();
	define_raster(rnew, grid, copy_raster_mask(rast->mask), rast->ncol, 
			rast->nrow, rast->bpp, init, rast->type);	

	return rnew;
	}
/******************************************************************************/
/**    Copy the given bitmask.
 *
 *  @param[in]	rast
 *  @return		copy of BITMASK object.
 ******************************************************************************/

BITMASK	copy_raster_mask
	(
	const BITMASK mask
	)

	{
	UNCHAR *bits;
	BITMASK bnew = NullBitMask;

	/* Make sure we have something to copy */
	if (!mask) return NullBitMask;

	bits = INITMEM(UNCHAR, mask->size);
	bits = memcpy(bits, mask->bits, mask->size);

	bnew = create_raster_mask();
	define_raster_mask(bnew, bits, mask->size);

	return bnew;
	}
/*******************************************************************************
*                                                                              *
*    r e c a l l _ r a s t e r _ s i z e                                       *
*    r e c a l l _ r a s t e r _ i n f o                                       *
*                                                                              *
*******************************************************************************/
/******************************************************************************/
/**   Calculate the size in bytes of the raster.
 *
 *  @param[in]	*rast	raster to examine
 *  @param[out]	*size	size of grid in bytes
 *  @return		True if calculation was successful. False otherwise.
 ******************************************************************************/
LOGICAL recall_raster_size
	(
	 RASTER	rast,
	 long	*size
	)
	{
	long nrow, ncol, bpp;
	if (!recall_raster_info(rast, &nrow, &ncol, &bpp, NULL, NULL, NULL, NULL)) 
		return FALSE;
	if (size) *size = nrow * ncol * bpp;
	return TRUE;
	}
/******************************************************************************/
/**   Calculate the size in bytes of the raster.
 *
 *  @param[in]	*rast	raster to examine
 *  @param[out]	*nrow	number of rows in grid
 *  @param[out]	*ncol	number of columns in grid
 *  @param[out]	*bpp	number of bytes per pixel
 *  @param[out]	*type	type of image data or RGB
 *  @param[out]	**init  initial value for masked out pixels
 *  @param[out]	**grid	grid of pixels
 *  @param[out]	*mask	bitmask
 *  @return		True if successful. False otherwise.
 ******************************************************************************/
LOGICAL recall_raster_info
	(
	 RASTER      rast, 
	 long        *nrow, 
	 long        *ncol, 
	 long        *bpp, 
	 RASTER_TYPE *type,
	 UNCHAR      **init, 
	 UNCHAR      **grid,
	 BITMASK	 *mask
	)
	{
	/* Return an invalid number if raster not given */
	if (!rast) return FALSE;

	if (nrow)	*nrow = rast->nrow;
	if (ncol)	*ncol = rast->ncol;
	if (bpp)	*bpp  = rast->bpp;
	if (type)   *type = rast->type;
	if (init)	*init = rast->init;
	if (grid)	*grid = rast->grid;
	if (mask)	*mask = rast->mask;
	return TRUE;
	}

/*********************************************************************/
/** Set the raster units.
 *
 *	@param[in]  rast		given surface
 *	@param[in]  *units	new units
 *********************************************************************/
void	define_raster_units

	(
	RASTER		rast,
	const USPEC	*units
	)

	{
	/* Do nothing if no surface */
	if (!rast) return;
	if (rast->type != RASTER_DATA) return;

	/* Define the units (current units are forgotten) */
	free_uspec(&rast->units);
	if (units) copy_uspec(&rast->units, units);
	}
/**********************************************************************/

/*********************************************************************/
/** Reset the raster units. Make the needed changes.
 *
 *	@param[in] 	rast	given raster
 *	@param[in] 	*units	new units
 *********************************************************************/
void	change_raster_units

	(
	RASTER		rast,
	const USPEC	*units
	)

	{
	long	ii, size;
	double	old_factor, old_offset;
	double	efactor, eoffset;

	/* Do nothing if no raster or raster is not a data grid */
	if (!rast)  return;
	if (!units) return;
	if (rast->type != RASTER_DATA) return;

	/* Remember the original units */
	old_factor = rast->units.factor;
	old_offset = rast->units.offset;

	/* Return if no change */
	if ((units->factor == old_factor) && (units->offset == old_offset))
		return;

	/* Construct effective factor and offset to convert spline */
	/* coefficients directly (i.e. new = old*efactor + eoffset) */
	efactor = units->factor / old_factor;
	eoffset = units->offset - old_offset*efactor;

	/* Save the new units definition */
	copy_uspec(&rast->units, units);

	/* Calculate new raster values */
	recall_raster_size(rast, &size);
	for(ii=0; ii<size; ii++)	
		{
		if (MASK_BIT_SET(rast->mask->bits,ii))
			{
			rast->grid[ii] *= efactor;
			rast->grid[ii] += eoffset;
			}
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the units of the given raster.
 *
 *	@param[in] 	rast	given raster
 *	@param[out]	**units	units returned
 *********************************************************************/
void	recall_raster_units

	(
	RASTER	rast,
	USPEC	**units
	)

	{
	if (units) *units = (rast) ? &rast->units : (USPEC *) 0;
	}

/**********************************************************************/
/** Test function to print out bit mask to file.
 *
 * 	@param[in]	*fp		Outbut stream
 * 	@param[in]	mask	Object to print
**********************************************************************/
void debug_print_raster_mask

	(
	 FILE	*fp,
	 BITMASK mask
	)

	{   
	int ii;                 
	if ( !mask ) return;
	fprintf(fp, "Begin DEBUG printing of bit mask: %ld", mask->size);	
	for (ii = 0; ii < mask->size*8; ii++)
		{
		if ( ii%80 == 0 ) 					fprintf(fp, "\n");
		if ( ii%8 == 0 ) 					fprintf(fp, " ");
		if ( MASK_BIT_SET(mask->bits,ii) )	fprintf(fp, "1");
		else                            	fprintf(fp, "0");
		}
	fprintf(fp, "\n");
	}    

/**********************************************************************/

/**********************************************************************/
/** Test function to print out raster to file.
 *
 * 	@param[in]	*fp		Outbut stream
 * 	@param[in]	rast	Object to print
**********************************************************************/
void debug_print_raster

	(
	 FILE	*fp,
	 RASTER rast
	)

	{   
	long ii, size;
	float *fgrid;
	if ( !rast ) return;
	size = rast->ncol*rast->nrow;

	fprintf(fp, "Begin DEBUG printing of raster: %ld\n", size);	
	switch(rast->type) 
		{
		case RASTER_DATA:
			fgrid = (float *)(rast->grid);
			for (ii = 0; ii < size; ii++)
				{
				if ( ii%20 == 0 ) fprintf(fp, "\n");
				fprintf(fp, " %3.0f", fgrid[ii]);
				}
			fprintf(fp, "\n");
			break;
		case RASTER_RGB:
			/* Do something appropriate here */
			/* break; */
		case RASTER_GRAY:
			/* Do something appropriate here */
			/* break; */
		default:
			for (ii = 0; ii < (size * rast->bpp); ii++)
				{
				if ( ii%20 == 0 )  fprintf(fp, "\n");
				fprintf(fp, " %X", rast->grid[ii]);	
				}
			fprintf(fp, "\n");
		}
	}    

