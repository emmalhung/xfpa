/***********************************************************************
*                                                                      *
*   r g r i b _ e d i t i o n 2 . h                                    *
*                                                                      *
*   GRIB structure definitions and default grid definitions for        *
*   routines to decode GRIB Edition 2 format files (include file)      *
*                                                                      *
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

/* We need FPA definitions */
#include <fpa.h>
#include "rgrib.h"

/* See if already included */
#ifndef RGRIBED2_DEFS
#define RGRIBED2_DEFS

#ifdef RGRIBED2_MAIN

/* grib error messages */
static const STRING g2_infoErrors[] =
{   "No error",
	"Beginning characters \"GRIB\" not found.",
	"GRIB message is not Edition 2.",
	"Could not find section 1, where expected.",
	"End String \"7777\" found, but not where expected.",
	"End String \"7777\" not found at end of message.",
	"Invalid section number found.",
};

static const STRING g2_getfldErrors[] =
{   "No error", 
	"Beginning characters \"GRIB\" not found.",
	"GRIB message is not Edition 2.",
	"The data field request number was not positive.",
	"End string \"7777\" found, but not were expected.",
	"",
	"GRIB message did not contain the requested number of data fields.",
	"End string \"7777\" not found at end of message.",
	"Unrecognized Section encountered.",
	"Data Representation Template 5.NN not yet implemented",
	"Error unpacking Section 1.",
	"Error unpacking Section 2.",
	"Error unpacking Section 3.",
	"Error unpacking Section 4.",
	"Error unpacking Section 5.",
	"Error unpacking Section 6.",
	"Error unpacking Section 7.",
};  

/* Accepted Grid Templates */
#define GT_LATLON				0
#define GT_ROTATED_LATLON		1
#define GT_STRETCHED_LATLON		2
#define GT_MERCATOR				10
#define GT_PSTEREO				20
#define GT_LAMBERT				30
#define GT_GAUSS				40
#define GT_ROTATED_GAUSS		41
#define GT_STRETCHED_GAUSS		42

/* Scan mode flags */
#define E2_scan_flag_west   7
#define E2_scan_flag_north  6
#define E2_scan_flag_hsweep 5
#define E2_scan_flag_rsweep 4

/* Pole centre flags */
#define E2_pole_centre_south 	7
#define E2_pole_centre_bipolar	6

/* Accepted Int value for "missing" value in a template */
#define MISSING			   255

/* Convertion constants */
#define GribToDegrees  1e6
#define	GribToKMeters  1e6
#define MetersPerUnit  1e3

/* Set list of recognized GRIB grid labels */
typedef struct
	{
	int			ident;
	STRING		label;
	} GRIBGRID_LABELS;
/* Set list of recognized GRIB grid labels */
static const GRIBGRID_LABELS GRIBGridLabels[] =
	{
		{ GT_LATLON,			"Latitude/Longitude Grid" },
		{ GT_ROTATED_LATLON,	"Rotated Latitude/Longitude Grid" },
		{ GT_STRETCHED_LATLON,	"Stretched Latitude/Longitude Grid" },
		{ GT_MERCATOR , 		"Mercator Projection" },
		{ GT_PSTEREO , 			"Polar Stereographic Projection" },
		{ GT_LAMBERT , 			"Lambert Conformal Projection" },
		{ GT_GAUSS , 			"Gaussian Grid" },
		{ GT_ROTATED_GAUSS , 	"Rotated Gaussian Grid" },
		{ GT_STRETCHED_GAUSS , 	"Stretched Gaussian Grid" }
	};
static const int NumGRIBGridLabels = (int) (sizeof(GRIBGridLabels) / sizeof(GRIBGRID_LABELS));
static const GRIBGRID_LABELS FPAGridLabels[] =
	{
		{ GT_LATLON, 		 "lat_lon"},
		{ GT_ROTATED_LATLON, "rotated_lat_lon"},
		{ GT_PSTEREO, 		 "polar_stereographic"},
		{ GT_GAUSS , 		 "lat_lon" },
		{ GT_LAMBERT, 		 "lambert_conformal"}
	};
static const int NumFPAGridLabels = (int) (sizeof(FPAGridLabels) / sizeof(GRIBGRID_LABELS));

#endif /* RGRIBED2_MAIN */

/* Now it has been included */
#endif	/* RGRIBED2_DEFS */
