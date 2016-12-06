/*********************************************************************/
/** @file raster.h
 *
 * RASTER object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    r a s t e r . h                                                   *
*                                                                      *
*    RASTER object definitions (include file)                          *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

/* See if already included */
#ifndef RASTER_DEFS
#define RASTER_DEFS

/* Need types */
#include <stdio.h>
#include <fpa_types.h>
#include "pspec.h"

/* Define valid RASTER types */
typedef enum	{ RASTER_EMPTY, RASTER_RGB, RASTER_DATA, RASTER_GRAY } RASTER_TYPE;

/** Define BITMASK Object for RASTER */
typedef struct BITMASK_struct
	{
	UNCHAR	*bits;	    /**< bit mask */
	long 	size;		/**< Size of mask in bytes */
	} *BITMASK;

/** Define RASTER Object - an array of pixel values */
typedef struct RASTER_struct
	{
	USPEC		units;	/**< units spec */
	int		 	 ncol;	/**< Width in pixels */
	int		 	 nrow;	/**< Height in pixels */
	int		 	 bpp;	/**< bytes per pixel */
	RASTER_TYPE	 type;	/**< type of compressed grid either RGB or DATA */
	UNCHAR    	*init;	/**< "No Value" value */
	UNCHAR		*grid;	/**< raster */
	BITMASK		 mask;	/**< bit mask object */
	} *RASTER;

#define NullRaster    NullPtr(RASTER)
#define NullRasterPtr NullPtr(RASTER *)
#define NullBitMask    NullPtr(BITMASK)
#define NullBitMaskPtr NullPtr(BITMASK *)
#define NullRasterGrid NullPtr(UNCHAR *)

/* Declare functions in raster.c */
RASTER	create_raster_obj(void);
RASTER	copy_raster( const RASTER );
RASTER	destroy_raster( RASTER );
void	define_raster( RASTER, UNCHAR *, BITMASK, int, int, int, UNCHAR *, RASTER_TYPE );
LOGICAL recall_raster_info(RASTER, long *, long *, long *, RASTER_TYPE *, UNCHAR **, UNCHAR **, BITMASK *);
LOGICAL recall_raster_size(RASTER, long *);
void	define_raster_units(RASTER, const USPEC *);
void	change_raster_units(RASTER, const USPEC *);
void	recall_raster_units(RASTER, USPEC **);

BITMASK create_raster_mask(void);
BITMASK copy_raster_mask(const BITMASK);
BITMASK destroy_raster_mask( BITMASK );
void	define_raster_mask(BITMASK, UNCHAR *, long);

/* Helpful debug functions */
void	debug_print_raster( FILE *, RASTER);
void	debug_print_raster_mask( FILE *, BITMASK);
/* Now it has been included */
#endif
