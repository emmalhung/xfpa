/***********************************************************************
*                                                                      *
*   r g r i b _ e d i t i o n 0 . h                                    *
*                                                                      *
*   Routines to decode GRIB Edition 0 format files (include file)      *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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
#ifndef RGRIBED0_DEFS
#define RGRIBED0_DEFS


/* We need FPA definitions */
#include <fpa.h>
#include "rgrib.h"
/* We need definitions for GRIB data structures */
#include "rgrib_edition1.h"


/***********************************************************************
*                                                                      *
*  Define keywords and structures for GRIB data format                 *
*                                                                      *
***********************************************************************/

/* Set definitions for length of data blocks */
/*  ... Product Definition Block (PDBED0_LENGTH bytes decoded) */
#define PDBED0_LENGTH		24


/***********************************************************************
*                                                                      *
*  Initialize defined constants for rgrib_edition0 routines            *
*                                                                      *
***********************************************************************/

#ifdef RGRIBED0_MAIN


/**** PREDEFINED LATITUDE/LONGITUDE GRIDS ****/


/* Define default latitude-longitude grid definition structure */
typedef struct
	{
	short int grid_defn;		/* GRID CATALOGUE NUMBER */

	short int dat_rep;			/* 6: CODED TYPE */
	int Ni;						/* 7-8: POINTS ALONG LATITUDES */
	int Nj;						/* 9-10: POINTS ALONG MERIDIANS */
	long int La1;				/* 11-13: LATITUDE OF ORIGIN */
	long int Lo1;				/* 14-16: LONGITUDE OF ORIGIN */
	short int resltn;			/* 17: RESOLUTION FLAG */
	long int La2;				/* 18-20: LATITUDE OF EXTREME */
	long int Lo2;				/* 21-23: LONGITUDE OF EXTREME */
	int Di;						/* 24-25: LATITUDNAL DIRECTION INCREMENT */
	int Dj;						/* 26-27: LONGITUDNAL DIRECTION INCREMENT */
	scan_mode_ints scan_mode;	/* 28: DATA ORDERING FLAGS */
								/* 29-32: RESERVED */
	short int pole_extra;		/* FLAG INDICATING EXTRA DATUM FOR POLE */

	} E1_ll_grid_predefinition;


/* Define default latitude-longitude grid definitions */
static const E1_ll_grid_predefinition E1_predef_ll_grids[] =
	{
		/*** INTERNATIONAL EXCHANGE ***/

		/** 5 deg x 2.5 deg RESOLUTION 1/2 HEMISHERES **/

		{21, 0, 37,36,  0,0, 128, 90000,180000, 5000,2500, {0,1,0},  1}, /*NE*/
		{22, 0, 37,36,  0,180000, 128, 90000,0, 5000,2500, {0,1,0},  1}, /*NW*/
		{23, 0, 37,36, -90000,0, 128, 0,180000, 5000,2500, {0,1,0}, -1}, /*SE*/
		{24, 0, 37,36, -90000,180000, 128, 0,0, 5000,2500, {0,1,0}, -1}, /*SW*/

		/** 5 deg x 5 deg RESOLUTION HEMISHERES **/

		{25, 0, 72,18,  0,0, 128, 90000,355000, 5000,5000, {0,1,0},  1}, /*N*/
		{26, 0, 72,18, -90000,0, 128, 0,355000, 5000,5000, {0,1,0}, -1}, /*S*/

		/** 2 deg x 2 deg RESOLUTION 1/2 HEMISHERES **/

		{61, 0, 91,45,  0,0, 128, 90000,180000, 2000,2000, {0,1,0},  1}, /*NE*/
		{62, 0, 91,45,  0,180000, 128, 90000,0, 2000,2000, {0,1,0},  1}, /*NW*/
		{63, 0, 91,45, -90000,0, 128, 0,180000, 2000,2000, {0,1,0}, -1}, /*SE*/
		{64, 0, 91,45, -90000,180000, 128, 0,0, 2000,2000, {0,1,0}, -1}, /*SW*/

		/*** NMC HEMISHERES - 2.5 deg RESOLUTION **/

		{29, 0, 145,37,  0,0, 128, 90000,360000, 2500,2500, {0,1,0}, 0}, /*N*/
		{30, 0, 145,37, -90000,0, 128, 0,360000, 2500,2500, {0,1,0}, 0}, /*S*/

		/*** NMC HEMISHERES - 2 deg RESOLUTION **/

		{33, 0, 181,46,  0,0, 128, 90000,360000, 2000,2000, {0,1,0}, 0}, /*N*/
		{34, 0, 181,46, -90000,0, 128, 0,360000, 2000,2000, {0,1,0}, 0}, /*S*/
	};


/* Set number of default latitude-longitude grid definitions */
static const int E1_nb_predef_ll_grids =
	(int) (sizeof(E1_predef_ll_grids) / sizeof(E1_ll_grid_predefinition));



/**** PREDEFINED POLAR STEREOGRAPHIC GRIDS ****/

/* Define default polar stereographic grid definition structure */
typedef struct
	{
	short int grid_defn;		/* GRID CATALOGUE NUMBER */

	short int dat_rep;			/* 6: CODED TYPE */
	int Nx;						/* 7-8: POINTS ALONG X-AXIS */
	int Ny;						/* 9-10: POINTS ALONG Y-AXIS */
	long int La1;				/* 11-13: LATITUDE OF ORIGIN */
	long int Lo1;				/* 14-16: LONGITUDE OF ORIGIN */
	short int compnt;			/* 17: COMPONENT FLAG */
	long int LoV;				/* 18-20: ORIENTATION LONGITUDE */
	long int Dx;				/* 21-23: X INCREMENT METERS @ 60N */
	long int Dy;				/* 24-26: Y INCREMENT METERS @ 60N */
	proj_centre_ints proj_centre;	/* 27: PROJECTION CENTRE FLAGS */
	scan_mode_ints scan_mode;		/* 28: DATA ORDERING FLAGS */
								/* 29-32: RESERVED */

	long int pole_i;			/* GRID CO-ORDINATES OF POLE */
	long int pole_j;			/*  OF PROJECTION            */

	} E1_ps_grid_predefinition;


/* Define default polar stereographic grid definitions   */
/*  Note that grid co-ordinates of pole of projection    */
/*  are one less than given in NMC documents ... since   */
/*  C counts from [0][0] while FORTRAN counts from (1,1) */
static const E1_ps_grid_predefinition E1_predef_ps_grids[] =
	{
		/*** NMC POLAR STEREOGRAPHIC ***/

		{  5, 5,  53,57, -99999,-999999, 136, -105000, 190500,190500, {0},
			{0,1,0}, 26000, 48000},
		{  6, 5,  53,45, -99999,-999999, 136, -105000, 190500,190500, {0},
			{0,1,0}, 26000, 48000},
		{ 27, 5,  65,65, -99999,-999999, 136,  -80000, 381000,381000, {0},
			{0,1,0}, 32000, 32000},
		{ 28, 5,  65,65, -99999,-999999, 136,  100000, 381000,381000, {1},
			{0,1,0}, 32000, 32000},
		{100, 5,  83,83, -99999,-999999, 136, -105000,  91452, 91452, {0},
			{0,1,0}, 39500, 87500},
		{101, 5, 113,91, -99999,-999999, 136, -105000,  91452, 91452, {0},
			{0,1,0}, 57500, 91500},
		{103, 5,  65,56, -99999,-999999, 136, -105000,  91452, 91452, {0},
			{0,1,0}, 24500, 83500},

	};


/* Set number of default polar stereographic grid definitions */
static const int E1_nb_predef_ps_grids =
	(int) (sizeof(E1_predef_ps_grids) / sizeof(E1_ps_grid_predefinition));


#endif

/* Now it has been included */
#endif
