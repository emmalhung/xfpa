/***********************************************************************
*                                                                      *
*   r g r i b . h                                                      *
*                                                                      *
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
#ifndef RGRIB_DEFS
#define RGRIB_DEFS

/* Set definition for number of significant digits in writing metafiles */
#define MaxDigits   4

/* Set default size of STRING labels */
#define GRIB_LABEL_LEN		32

/* Processed Grib Field */
typedef struct
{
	STRING		model;			/* source           */
	STRING		rtime;			/* reference time   */
	STRING		vtimeb;			/* valid start time */
	STRING		vtimee;			/* valid end time   */
	STRING		units;			/* FPA unit name    */
	STRING		level;			/* FPA level name   */
	STRING		element;		/* FPA element name */
	MAP_PROJ	*mproj_orig;	/* original map projection         */
	MAP_PROJ	*mproj;			/* map projection after adjustment */
	float		*data_orig;		/* original Grib data              */
	float		*data;			/* Grib data after adjustment      */
	LOGICAL		*bmap;			/* Bitmap                          */
	int component_flag;			/* 0 grid in E/N coordinates
								   1 grid in x/y coordinates           */
	int projection;				/* Projection number from GRIB file    */
	LOGICAL		isweep;			/* data scans in the i direction first */
	LOGICAL		west;			/* data scans from west to east        */
	LOGICAL		north;			/* data scans from north to south      */
	LOGICAL		rsweep;			/* data scans in reverse order on
								   consecutive rows                    */
	LOGICAL		left;			/* origin on left side */
	LOGICAL		bottom;			/* origin at bottom    */

	LOGICAL		filled;		/* Grib data filled in where missing in bitmap */
	LOGICAL		reordered;	/* Grib data reordered to match FPA scanning   */
	LOGICAL		wrapped;	/* Grib data added for wrapped projections     */
} DECODEDFIELD;

/***********************************************************************
*                                                                      *
*  Declare external functions in rgrib_edition0.c                      *
*                                                                      *
***********************************************************************/

/* Declare interface functions in rgrib_edition0.c */
LOGICAL	open_gribfile_edition0(STRING);
LOGICAL	next_gribfield_edition0(DECODEDFIELD **);
LOGICAL	gribfield_identifiers_edition0(STRING *, STRING *, STRING *, STRING *,
										STRING *, STRING *, STRING *);
void	close_gribfile_edition0(void);

/* Gribtest functions */
void 	print_block0_edition0 ( void );
void 	print_block1_edition0 ( void );
void 	print_block2_edition0 ( void );
void 	print_block3_edition0 ( void );
void 	print_block4_edition0 ( void );
void 	print_block4_raw_edition0 ( void );
void 	print_block4_data_edition0 ( void );

/***********************************************************************
*                                                                      *
*  Declare external functions in rgrib_edition1.c                      *
*                                                                      *
***********************************************************************/

LOGICAL	open_gribfile_edition1(STRING);
LOGICAL	next_gribfield_edition1(DECODEDFIELD **);
LOGICAL	gribfield_identifiers_edition1(STRING *, STRING *, STRING *, STRING *,
										STRING *, STRING *, STRING *);
void	close_gribfile_edition1(void);

/* Gribtest functions */
void 	print_block0_edition1 ( void );
void 	print_block1_edition1 ( void );
void 	print_block2_edition1 ( void );
void 	print_block3_edition1 ( void );
void 	print_block4_edition1 ( void );
void 	print_block4_raw_edition1 ( void );
void 	print_block4_data_edition1 ( void );

/************************************************
*                                               *
* Declare external functionn in rgrib_edtion2.c *
*                                               *
************************************************/

LOGICAL	open_gribfile_edition2(STRING);
LOGICAL	next_gribfield_edition2(DECODEDFIELD **);
LOGICAL	gribfield_identifiers_edition2(STRING *, STRING *, STRING *, STRING *,
										STRING *, STRING *, STRING *);
void	close_gribfile_edition2(void);

/* Gribtest functions */
void 	print_block0_edition2 ( void );
void 	print_block1_edition2 ( void );
void 	print_block2_edition2 ( void );
void 	print_block3_edition2 ( void );
void 	print_block4_edition2 ( void );
void 	print_block5_edition2 ( void );
void 	print_block6_edition2 ( void );
void 	print_block7_edition2 ( void );

/* Now it has been included */
#endif	/* RGRIB_DEFS */
