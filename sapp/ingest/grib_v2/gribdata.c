/*******************************************************************************/
/** @file gribdata.c
 *
 * 	Routines to convert decoded GRIB data to FPA Grid Objects.
 *
 *******************************************************************************/
/***********************************************************************
*                                                                      *
*    g r i b d a t a . c                                               *
*                                                                      *
*     Version 6 (c) Copyright 2006 Environment Canada (MSC)            *
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

#define GRIBDATA_MAIN	/* To initialize defined constants and     */
						/*  internal structures in gribdata.h file */

/* We need FPA definitions */
#include <fpa.h>

/* We need definitions for GRIB data structures */
#include "rgrib.h"

#include "gribdata.h"

#include <math.h>
#include <stdio.h>

#ifdef DEBUG_GRIBMETA
	static int	DebugMode = TRUE;
#else
	static int	DebugMode = FALSE;
#endif /* DEBUG_GRIBMETA */

#define dprintf (!DebugMode)? (void) 0: (void) fprintf

/* Interface functions                  */
/*  ... these are defined in gribdata.h */

/* Internal static functions */
static LOGICAL	reorder_data ( DECODEDFIELD *, short **, float *, float, float);

/*******************************************************************************/
/** Translate from GRIB data to gridded data for FPA.
 *
 *  Since the GRIB data is converted to a short value you may need to scale
 *  the values to ensure they get store to the desired precision. 
 *  
 *  The range of short values (on a 32bit architecture) are -32767 to 32767
 *
 * 	short_value = (short)((value - offset)/precision)
 *
 * @param[in]	*gribfld	DECODEDFIELD Object with decoded GRIB data.
 * @param[in]	*fdesc		Output field descripter for data field.
 * @param[in]	precision	Amount to scale data values by.
 * @param[in]	offset		Amount to offset data values by.
 *******************************************************************************/
void gribfield_to_data
	(
	 DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	 FLD_DESCRIPT	*fdesc,		/* pointer to output field descripter */
	 STRING			units,
	 float			precision,
	 float			offset
	)
	{
	short 	*cfld;
	STRING  fname;
	FILE  	*DataFile;
	size_t	npts;

	if ( IsNull(gribfld) || IsNull(fdesc) ) return;

	if ( !reorder_data(gribfld, &cfld, gribfld->data, precision, offset) )
		{
		(void) fprintf(stderr, 
					   "[gribfield_to_data] Problem preparing field for datafile\n");
		return;
		}

	/* Get data file name */
	fname = construct_meta_filename(fdesc);
	if ( blank(fname) ) fname = build_meta_filename(fdesc);
	if ( blank(fname) )
		{
		(void) fprintf(stderr, "[gribfield_to_data] Cannot build datafile name ");
		(void) fprintf(stderr, "%s ", fdesc->sdef->name); 
		(void) fprintf(stderr, "%s ", fdesc->edef->name);
		(void) fprintf(stderr, "%s\n", fdesc->ldef->name);
		return;
		}
	/* Check if file is open */
	if ( !(DataFile = fopen(fname,"wb")) )
		{
		(void) fprintf(stderr, "Cannot open output file \"%s\"\n", fname);
		return;
		}

	/* Write data to file */
	npts = (gribfld->mproj->grid.nx * gribfld->mproj->grid.ny);
	(void) fwrite((void *)cfld, sizeof(short), npts, DataFile);

	/* Close File */
	(void) fclose(DataFile);
	
	}

/***********************************************************************
*                                                                      *
*    r e o r d e r _ d a t a                                           *
*                                                                      *
*	Orders data for image definition                                   *
***********************************************************************/
/*******************************************************************************/
/** Re-order grib data if required
 *
 * Preferred IMAGE order is:
 * - Points of first row scan in +i (+x) direction
 * - Points of first column scan in +j (+y) direction
 * - Adjacent points in i (x) direction are consecutive
 * - Adjacent rows in scan in the same direction
 *
 * @param[in]	*gribfld		DECODEDFIELD object with decoded GRIB data
 * @param[out]	**pshort_data	points to internal array of data, 
 * 								reordered and converted to short ints.
 * @param[in]	*pfloat_data	original array of data to be reordered and converted.
 * @param[in]	precision		value to scale data by.
 * @param[in]	offset			value to offset data by.
 *******************************************************************************/
static LOGICAL	reorder_data
	(
	DECODEDFIELD	*gribfld,	/* DECODEDFIELD Object with decoded GRIB data */
	short 			**pshort_data,
	float 			*pfloat_data,
	float			precision,
	float			offset
	)
	{
	static short   	*fld_out=NullShort;
	float     		*value_in;
	short			*value_out, value_tmp;
	int     		jj, ii, nx, ny;
	LOGICAL 		left, bottom, isweep, rsweep;
	LOGICAL			DataValid = TRUE;

	if ( IsNull(gribfld) || IsNull(pshort_data) || IsNull(pfloat_data) ) return FALSE;

	isweep = gribfld->isweep;
	rsweep = gribfld->rsweep;

	/* Interpret scan_mode */
	left   = gribfld->left;
	bottom = gribfld->bottom;

	ny        = gribfld->mproj->grid.ny;
	nx        = gribfld->mproj->grid.nx;
	fld_out   = GETMEM(fld_out, short, (nx * ny) );
	value_out = fld_out;

	/******************************************************/
	/* Re-order grib data if required                     */
	/* Preferred IMAGE order is:                          */
	/* Points of first row scan in +i (+x) direction      */
	/* Points of first column scan in +j (+y) direction   */
	/* Adjacent points in i (x) direction are consecutive */
	/* Adjacent rows in scan in the same direction        */
	/******************************************************/
	for(jj=0; jj<ny; jj++)
		{
		for(ii=0; ii<nx; ii++)
			{
			/* Data with i'th direction incrementing first */
			if (!isweep)
				{
				/* Data ordered left to right */
				if ( left )
					{
					/* Data ordered top top bottom */
					if ( !bottom ) 
						{
						value_in = pfloat_data + (jj*nx) + ii;
						}
					/* Data ordered bottom to top */
					else
						{
						value_in = pfloat_data + ((ny-jj-1)*nx) + ii;
						}
					}
				/* Data ordered right to left */
				else
					{
					/* Data ordered top to bottom */
					if ( !bottom ) 
						{
						value_in = pfloat_data + (jj*nx) + (nx-ii-1);
						}
					/* Data ordered bottom to top */
					else
						{
						value_in = pfloat_data + ((ny-jj-1)*nx) + (nx-ii-1);
						}
					}
				}
			/* Data with j'th direction incrementing first */
			else
				{
				/* Data ordered left to right */
				if ( left )
					{
					/* Data ordered top to bottom */
					if ( !bottom ) 
						{
						value_in = pfloat_data + jj + (ii*ny);
						}
					/* Data ordered bottom to top */
					else
						{
						value_in = pfloat_data + (ny-jj-1) + (ii*ny);
						}
					}
				/* Data ordered right to left */
				else
					{
					/* Data ordered top top bottom */
					if ( !bottom ) 
						{
						value_in = pfloat_data + jj + ((nx-ii-1)*ny);
						}
					/* Data ordered bottom to top */
					else
						{
						value_in = pfloat_data + (ny-jj-1) + ((nx-ii-1)*ny);
						}
					}
				}
			/* Set value in output data array */
			value_tmp = (short) NINT(( *value_in - offset ) / precision);
			if (( (*value_in - offset) / precision) > SHRT_MAX) 
				{
				DataValid = FALSE;
				value_tmp = SHRT_MAX;
				}
			else if	(( (*value_in - offset) / precision) < SHRT_MIN)
				{
				DataValid = FALSE;
				value_tmp = SHRT_MIN;
				}
			*value_out++ = value_tmp;
			}
		/* If adjacent rows scan in opposite directions the reverse sense of "left" */
		if ( rsweep ) left = !left;
		}

		if (!DataValid)
			{
				(void) fprintf(stderr, 
					   "[gribfield_to_data] Problem preparing field for datafile\n");
				(void) fprintf(stderr, 
					   "	some data outside the valid range [%d,%d]\n", 
					   SHRT_MIN, SHRT_MAX);
			}
	*pshort_data = fld_out;

	dprintf(stderr, "  Completed reorder of data\n");
	return TRUE;
	}
