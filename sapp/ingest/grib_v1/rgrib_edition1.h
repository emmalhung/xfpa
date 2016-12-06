/***********************************************************************
*                                                                      *
*   r g r i b _ e d i t i o n 1 . h                                    *
*                                                                      *
*   GRIB structure definitions and default grid definitions for        *
*   routines to decode GRIB Edition 1 format files (include file)      *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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
#ifndef RGRIBED1_DEFS
#define RGRIBED1_DEFS


/* We need FPA definitions */
#include <fpa.h>


/***********************************************************************
*                                                                      *
*  Define keywords and structures for GRIB data format                 *
*                                                                      *
***********************************************************************/

/* Set default size of STRING labels */
#define GRIB_LABEL_LEN		32

/* Set definitions for header and trailer format */
#define GRIB_HEADER				"GRIB"
#define GRIB_HEADER_LENGTH		4
#define GRIB_TRAILER			"7777"
#define GRIB_TRAILER_LENGTH		4

/* Set definitions for length of data blocks */
/*  ... Product Definition Block (PDB_LENGTH bytes decoded) */
#define PDB_LENGTH				28
#define PDB_MAX_LENGTH			250

/*  ... Grid Description Block (GDB_..._LENGTH bytes decoded) */
#define GDB_LATLON_LENGTH			28
#define GDB_GAUSS_LENGTH			28
#define GDB_PSTEREO_LENGTH			28
#define GDB_LAMBERTC_LENGTH			34
#define GDB_ROTATED_LATLON_LENGTH	42
#define GDB_MAX_LENGTH				250

/*  ... Bit Map Block (BMB_LENGTH bytes decoded from header) */
/*                    (Data length is variable)              */
#define BMB_LENGTH		6

/*  ... Binary Data Block (BDH_LENGTH bytes decoded from header) */
/*                        (Data length is variable)              */
#define BDH_LENGTH		11


/* Set definitions for types of recognized GRIB grids */
#define LATLON_GRID				0
#define MERCATOR_GRID			1
#define GNOMIC_GRID				2
#define LAMBERTC_GRID			3
#define GAUSS_GRID				4
#define PSTEREO_GRID			5
#define ROTATED_LATLON_GRID		10
#define ROTATED_LAMBERTC_GRID	13
#define ROTATED_GAUSS_GRID		14
#define STRETCHED_LATLON_GRID	20
#define STRETCHED_GAUSS_GRID	24


/* Set list of recognized GRIB grid labels */
typedef struct
	{
	int			ident;
	STRING		label;
	} GRIBGRID_LABELS;

static const GRIBGRID_LABELS GRIBGridLabels[] =
	{
		{ LATLON_GRID,				"Latitude/Longitude Grid" },
		{ MERCATOR_GRID,			"Mercator Projection" },
		{ GNOMIC_GRID,				"Gnomic Projection" },
		{ LAMBERTC_GRID,			"Lambert Conformal Projection" },
		{ GAUSS_GRID,				"Gaussian Grid" },
		{ PSTEREO_GRID,				"Polar Stereographic Projection" },
		{ ROTATED_LATLON_GRID,		"Rotated Latitude/Longitude Grid" },
		{ ROTATED_LAMBERTC_GRID,	"Rotated (Oblique) Lambert Conformal" },
		{ ROTATED_GAUSS_GRID,		"Rotated Gaussian Grid" },
		{ STRETCHED_LATLON_GRID,	"Stretched Latitude/Longitude Grid" },
		{ STRETCHED_GAUSS_GRID,		"Stretched Gaussian Grid" },
	};

/* Set number of recognized GRIB grids */
static const int NumGRIBGridLabels =
	(int) (sizeof(GRIBGridLabels) / sizeof(GRIBGRID_LABELS));


/* Define grib data structures */
typedef unsigned char Octet;

typedef struct
	{
	Octet octet1;
	Octet octet2;
	} Double_octet;

typedef struct
	{
	Octet octet1;
	Octet octet2;
	Octet octet3;
	} Triple_octet;

typedef struct
	{
	Octet octet1;
	Octet octet2;
	Octet octet3;
	Octet octet4;
	} Quad_octet;



/**** GRIB FILE FORMAT ****/

/* GRID BLOCK INCLUSION AND BITMAP FLAG ... Bit locations */
/** NB: THESE FLAGS CONCUR WITH WMO FM 92-VIII ***/
#define E1_block_flag_grid_desc 7
#define E1_block_flag_bit_map   6

/* CENTRE OF PROJECTION FLAG ... Bit locations */
#define E1_proj_flag_pole       7
#define E1_proj_flag_bipolar    6

/* SCANNING DIRECTION FLAG ... Bit locations */
#define E1_scan_flag_west       7
#define E1_scan_flag_north      6
#define E1_scan_flag_hsweep     5

/* BITMAP FLAG ... bit locations */
#define E1_bitmap_flag_1        7
#define E1_bitmap_flag_2        6
#define E1_bitmap_flag_3        5
#define E1_bitmap_flag_4        4
#define E1_bitmap_flag_5        3
#define E1_bitmap_flag_6        2
#define E1_bitmap_flag_7        1
#define E1_bitmap_flag_8        0

/**** INDICATOR BLOCK ****/
typedef struct
	{
	long int length;	/* 1-3: LENGTH OF BLOCK IN OCTETS */
	short int edition;	/* 4: EDITION OF GRIB SPECIFICATION */

	} E1_Indicator_block;


/*** PRODUCT DEFINITION BLOCK ***/
typedef struct
	{
	long int length;		/* 1-3: LENGTH OF BLOCK IN OCTETS */
	short int edition;		/* 4: EDITION OF GRIB SPECIFICATION */
	short int centre_id;	/* 5: WMO NUMBER OF ORIGIN OF FILE */
	short int model_id;		/* 6: ORIGINATING CENTRE SPECIFIC ID */
	short int grid_defn;	/* 7: GRID CATALOGUE NUMBER */

	struct					/* 8: OPTIONAL BLOCKS FLAGS */
		{
		short int grid_description;	/* block 2 */
		short int bit_map;			/* block 3 */
		} block_flags;

	short int parameter;	/* 9: NUMERIC ID CODE OF DATA TYPE */

	struct				/* LAYER DESCRIPTOR: */
		{
		short int type;		/* 10: CODED TYPE	*/
		short int top;		/* 11: TYPE DEPENDANT	*/
		short int bottom;	/* 12: INFORMATION	*/
		} layer;

	struct				/* FORECAST DESCRIPTOR:	*/
		{
		struct			/* REFERENCE TIME OF DATA: */
			{
			short int year;		/* 13: YEAR OF CENTURY (2 DIGIT) */
			short int month;	/* 14: NUMERIC MONTH OF YEAR */
			short int day;		/* 15: DAY OF MONTH */
			short int hour;		/* 16: HOUR OF 24 HOUR DAY */
			short int minute;	/* 17: MINUTE OF HOUR */
			} reference;

		short int units;		/* 18: CODED TIME UNITS */
		short int time1;		/* 19: FORECAST PERIOD, ANALYSIS IS 0 */
		short int time2;		/* 20: INTERVAL */
		short int range_type;	/* 21: CODED RANGE TYPE */
		int nb_averaged;		/* 22-23: NUMBER AVERAGED */
		short int nb_missing;	/* 24: UNUSED EVEN BYTE FILLER */
		short int century;		/* 25: century of initial time */
		short int reserved;		/* 26: reserved - set to 0 */
		long int factor_d;		/* 27-28: decimal scale factor */

		} forecast;

	} E1_Product_definition_data;


/* CENTRE OF PROJECTION FLAGS */
typedef struct
	{
	short int pole;			/* 0: PROJECTION CENTRE IS NORTH POLE */
							/* 1: PROJECTION CENTRE IS SOUTH POLE */
	short int bipolar;		/* 0: ONE PROJECTION CENTRE */
							/* 1: BI-POLAR PROJECTION   */
	} proj_centre_ints;


/* DATA ORDERING FLAGS */
typedef struct
	{
	short int west;			/* 0: LEFTWARD (POSITIVE TO EAST)  */
							/* 1: RIGHTWARD (POSITIVE TO WEST) */
	short int north;		/* 0: DOWNWARD (POSITIVE TO SOUTH) */
							/* 1: UPWARD (POSITIVE TO NORTH)   */
	short int horz_sweep;	/* 0: LEFT-RIGHT (LONGITUDE) INCREMENTS FIRST */
							/* 1: UP-DOWN (LATITUDE) INCREMENTS FIRST     */
	} scan_mode_ints;


/*** GRID DESCRIPTION BLOCK ***/
typedef struct
	{
	long int length;		/* 1-3: LENGTH OF BLOCK IN OCTETS */
	short int nv;			/* 4: UNUSED BITS AT END OF BLOCK */
	short int pv_or_pl;		/* 5: QUASI-REGULAR GRID LOCATION */
	short int dat_rep;		/* 6: CODED TYPE */

	union		/** GRID DEFINITIONS **/
		{

		struct	/* REGULAR LATITUDE/LONGITUDE */
			{
			int Ni;				/* 7-8: POINTS ALONG LATITUDES (COLUMNS) */
			int Nj;				/* 9-10: POINTS ALONG MERIDIANS (ROWS) */
			long int La1;		/* 11-13: LATITUDE OF ORIGIN */
			long int Lo1;		/* 14-16: LONGITUDE OF ORIGIN */
			short int resltn;	/* 17: RESOLUTION AND COMPONENT FLAG */
			long int La2;		/* 18-20: LATITUDE OF EXTREME */
			long int Lo2;		/* 21-23: LONGITUDE OF EXTREME */
			int Di;				/* 24-25: DIRECTION INCREMENT */
			int Dj;				/* 26-27: DIRECTION INCREMENT */
			scan_mode_ints scan_mode;	/* 28: DATA ORDERING FLAGS */
			short int thin_mode;		/* THINNED ROW OR COLUMN     */
										/*  INDICATOR (0,1,2)        */
			short int *thin_pts;		/* NUMBER OF POINTS ON EACH  */
										/*  OF Nj ROWS OR Ni COLUMNS */
			short int pole_extra;		/* ADDITIONAL DATUM FOR POLE */
										/*  INDICATOR (-1,0,1)       */
			short int meridian_extra;	/* ADDITIONAL DATA FOR LAST  */
										/*  MERIDIAN INDICATOR (0,1) */
			} reg_ll;

		struct	/* GAUSSIAN LATITUDE/LONGITUDE */
			{
			int Ni;				/* 7-8: POINTS ALONG LATITUDES (COLUMNS) */
			int Nj;				/* 9-10: POINTS ALONG MERIDIANS (ROWS) */
			long int La1;		/* 11-13: LATITUDE OF ORIGIN */
			long int Lo1;		/* 14-16: LONGITUDE OF ORIGIN */
			short int resltn;	/* 17: RESOLUTION AND COMPONENT FLAG */
			long int La2;		/* 18-20: LATITUDE OF EXTREME */
			long int Lo2;		/* 21-23: LONGITUDE OF EXTREME */
			int Di;				/* 24-25: DIRECTION INCREMENT */
			int N;				/* 26-27: PARALLELS - EQUATOR TO POLE */
			scan_mode_ints scan_mode;	/* 28: DATA ORDERING FLAGS */
			short int thin_mode;		/* THINNED ROW INDICATOR (0,1) */
			short int *thin_pts;		/* NUMBER OF POINTS ON EACH OF */
										/*  Nj ROWS                    */
			} guas_ll;

		struct	/* POLAR STEREOGRAPHIC */
			{
			int Nx;				/* 7-8: POINTS ALONG X-AXIS (COLUMNS) */
			int Ny;				/* 9-10: POINTS ALONG Y-AXIS (ROWS) */
			long int La1;		/* 11-13: LATITUDE OF ORIGIN */
			long int Lo1;		/* 14-16: LONGITUDE OF ORIGIN */
			short int compnt;	/* 17: COMPONENT FLAG */
			long int LoV;		/* 18-20: ORIENTATION LONGITUDE */
			long int Dx;		/* 21-23: X INCREMENT METERS @ 60N */
			long int Dy;		/* 24-26: Y INCREMENT METERS @ 60N */
			proj_centre_ints proj_centre;	/* 27: PROJECTION CENTRE FLAGS */
			scan_mode_ints scan_mode;		/* 28: DATA ORDERING FLAGS */
			long int pole_i;		/* GRID CO-ORDINATES OF */
			long int pole_j;		/*  POLE OF PROJECTION */
			} ps;

		struct	/* LAMBERT CONFORMAL (tangent or secant) */
			{
			int Nx;				/* 7-8: POINTS ALONG X-AXIS (COLUMNS) */
			int Ny;				/* 9-10: POINTS ALONG Y-AXIS (ROWS) */
			long int La1;		/* 11-13: LATITUDE OF ORIGIN */
			long int Lo1;		/* 14-16: LONGITUDE OF ORIGIN */
			short int compnt;	/* 17: COMPONENT FLAG */
			long int LoV;		/* 18-20: ORIENTATION LONGITUDE */
			long int Dx;		/* 21-23: X INCREMENT METERS @ 60N */
			long int Dy;		/* 24-26: Y INCREMENT METERS @ 60N */
			proj_centre_ints proj_centre;	/* 27: PROJECTION CENTRE FLAGS */
			scan_mode_ints scan_mode;		/* 28: DATA ORDERING FLAGS */
			long int Latin1;	/* 29-31: LATITUDE OF SECANT (CLOSEST TO P) */
			long int Latin2;	/* 32-34: LATITUDE OF SECANT (FURTHEST TO P) */
			} lambert;

		struct	/* ROTATED LATITUDE/LONGITUDE */
			{
			int Ni;				/* 7-8: POINTS ALONG LATITUDES (COLUMNS) */
			int Nj;				/* 9-10: POINTS ALONG MERIDIANS (ROWS) */
			long int La1;		/* 11-13: LATITUDE OF ORIGIN */
			long int Lo1;		/* 14-16: LONGITUDE OF ORIGIN */
			short int resltn;	/* 17: RESOLUTION AND COMPONENT FLAG */
			long int La2;		/* 18-20: LATITUDE OF EXTREME */
			long int Lo2;		/* 21-23: LONGITUDE OF EXTREME */
			int Di;				/* 24-25: DIRECTION INCREMENT */
			int Dj;				/* 26-27: DIRECTION INCREMENT */
			scan_mode_ints scan_mode;	/* 28: DATA ORDERING FLAGS */
			long int LaP;		/* 33-35: LATITUDE OF SOUTHERN POLE */
			long int LoP;		/* 36-38: LONGITUDE OF SOUTHERN POLE */
			double AngR;		/* 39-42: ANGLE OF ROTATION */
			short int thin_mode;		/* THINNED ROW OR COLUMN     */
										/*  INDICATOR (0,1,2)        */
			short int *thin_pts;		/* NUMBER OF POINTS ON EACH  */
										/*  OF Nj ROWS OR Ni COLUMNS */
			short int pole_extra;		/* ADDITIONAL DATUM FOR POLE */
										/*  INDICATOR (-1,0,1)       */
			short int meridian_extra;	/* ADDITIONAL DATA FOR LAST  */
										/*  MERIDIAN INDICATOR (0,1) */
			} rotate_ll;

		} defn;

	} E1_Grid_description_data;


/*** BITMAP BLOCK HEADER ***/
typedef struct
	{
	long int length;	/* 1-3: LENGTH OF BLOCK IN OCTETS */
	Octet    unused;	/* 4:   NUMBER OF UNUSED BITS AT END */
	int      ntable;	/* 5-6: PRE-DEFINED BITMAP (if non 0) */
	} E1_Bit_map_header;


/*** BINARY DATA BLOCK HEADER ***/
#define E1_shift_r4(a) ( (a)>>4 )
#define E1_mask_l4(a)  ( (a)&15 )
typedef struct
	{
	long int length;		/* 1-3: LENGTH OF BLOCK IN OCTETS */

	Octet flags;			/* 4: BIT-MODE FLAGS                    */
	Octet unused;			/*     AND NUMBER OF UNUSED BITS AT END */

	int scale;				/* 5-6: SCALE FACTOR */
	double reference;		/* 7-10: MIN DATA VALUE (IBM 360 CODED) */
	short int bits_per_val;	/* 11: NUMBER OF BITS FOR EACH VALUE */

} E1_Binary_data_header;


/*** GRIB DATA ***/
typedef struct
	{
	E1_Indicator_block			Isb;
	E1_Product_definition_data	Pdd;
	E1_Grid_description_data	Gdd;
	unsigned int				NumGrid;
	E1_Bit_map_header			Bmhd;
	unsigned int				NumBit;
	LOGICAL						*PBit;
	LOGICAL						PoleBit;
	E1_Binary_data_header		Bdhd;
	unsigned int				NumRaw;
	float						*PRaw;
	float						PoleDatum;
	int							Nii, Njj;
	int							Dii, Djj;
	float						*PData;

	} GRIBFIELD;


/***********************************************************************
*                                                                      *
*  Initialize defined constants for rgrib_edition1 routines            *
*                                                                      *
***********************************************************************/

#ifdef RGRIBED1_MAIN


/**** PREDEFINED MODEL, ELEMENT AND LEVEL IDENTIFIERS ****/


/* Define default model identifiers */
typedef struct
	{
	short int	centre_id;			/* ID for issuing centre */
	short int	model_id;			/* ID for output model type */
	STRING		model_label;		/* FPA model label */
	} ModelDefinitions;


/* Define default models */
static const ModelDefinitions ModelDefs[] =
	{
		/*** CMC STANDARD ***/

		{ 54, 30, "FEM_Analysis_30" },
		{ 54, 31, "FEM_Analysis_31" },
		{ 54, 32, "FEM_Analysis_32" },
		{ 54, 33, "FEM_Analysis_33" },
		{ 54, 34, "FEM_Analysis_34" },
		{ 54, 35, "FEM_35" },
		{ 54, 36, "FEM_36" },
		{ 54, 37, "FEM_37" },
		{ 54, 38, "FEM_38" },
		{ 54, 39, "FEM_39" },
		{ 54, 40, "Spectral_Analysis_40" },
		{ 54, 41, "Spectral_Analysis_41" },
		{ 54, 42, "Spectral_Analysis_42" },
		{ 54, 43, "Spectral_Analysis_43" },
		{ 54, 44, "Spectral_Analysis_44" },	/* Used for sea surface temperature */
		{ 54, 45, "Spectral_45" },
		{ 54, 46, "Spectral_46" },
		{ 54, 47, "Spectral_47" },
		{ 54, 48, "Spectral_48" },
		{ 54, 49, "Spectral_49" },

		/*** NMC STANDARD ***/

		{ 07, 77, "NMC_77" },
		{ 07, 78, "NMC_78" },
		{ 07, 79, "NMC_79" },
		{ 07, 80, "NMC_80" },

		/*** ECMWF STANDARD ***/

		{ 98, 40, "ECMWF_40" },

	};


/* Set number of predefined model labels */
static const int NumModelDefs =
	(int) (sizeof(ModelDefs) / sizeof(ModelDefinitions));



/* Define default element identifiers */
typedef struct
	{
	short int	parameter;			/* identification number */
	STRING		element_label;		/* FPA element label */
	STRING		units_label;		/* FPA units label */
	} ElementDefinitions;


/* Define default elements */
static const ElementDefinitions ElementDefs[] =
	{

		{   1, "real_pressure",              "Pa" },
		{   2, "pressure",                   "Pa" },
		{   3, "pressure_tendency",          "Pa/s" },
		{   6, "geopotential",               "m2/s/s" },
		{   7, "height",                     "m" },
		{  11, "temperature",                "degreesK" },
		{  12, "virtual_temperature",        "degreesK" },
		{  13, "potential_temperature",      "degreesK" },
		{  15, "maximum_temperature_gmt",    "degreesK" },
		{  16, "minimum_temperature_gmt",    "degreesK" },
		{  17, "dewpoint",                   "degreesK" },
		{  18, "dewpoint_depression",        "Kdegrees" },
		{  19, "lapse_rate",                 "Kdegrees/m" },
		{  31, "wind_direction",             "degrees_true" },
		{  32, "wind_speed",                 "m/s" },
		{  33, "uu_wind",                    "m/s" },
		{  34, "vv_wind",                    "m/s" },
		{  39, "vertical_velocity",          "Pa/s" },
		{  40, "real_vertical_velocity",     "m/s" },
		{  41, "vorticity",                  "1/s" },
		{  42, "divergence",                 "1/s" },
		{  43, "relative_vorticity",         "1/s" },
		{  44, "relative_divergence",        "1/s" },
		{  51, "specific_humidity",          "kg/kg" },
		{  52, "relative_humidity",          "percent" },
		{  59, "rainfall_rate",              "kg/m2/s" },
		{  61, "precipitation",              "kg/m2" },
		{  63, "convective_precip",          "kg/m2" },
		{  71, "total_cloud",                "percent" },
		{  72, "convective_cloud",           "percent" },
		{  73, "low_cloud",                  "percent" },
		{  74, "mid_cloud",                  "percent" },
		{  75, "high_cloud",                 "percent" },
		{  80, "sea_temperature",            "degreesK" },
		{  91, "ice_cover",                  "ratio" },
		{ 214, "convective_precip_rate",     "kg/m2/s" },

	};


/* Set number of predefined element labels */
static const int NumElementDefs =
	(int) (sizeof(ElementDefs) / sizeof(ElementDefinitions));



/* Define default level identifiers */
typedef struct
	{
	short int	type;		/* level identifier */
	short int	nval;		/* how many values in next 2 octets (0/1/2) */
	float		scale1;		/* scale factor for 1st value */
	float		off1;		/* offset for 1st value */
	float		scale2;		/* scale factor for 2nd value */
	float		off2;		/* offset for 2nd value */
	STRING		tag;		/* terminating string */
	} LevelDefinitions;


/* Define default levels descriptions */
/*  100  (Isobaric level in hPa) (set to mb)                          */
/*  101  (Layer between two isobaric levels in kPa) (set to mb)       */
/*  102  (Mean sea level)                                             */
/*  105  (Altitude above sea level in m) (set to m)                   */
/*  106  (Layer between two altitudes in hm) (set to m)               */
/*  105  (Height above ground in m) (set to m)                        */
/*  106  (Layer between two heights in hm) (set to m)                 */
/*  107  (Sigma level in 1/10000) (set to sigma*100)                  */
/*  108  (Layer between two sigma levels in 1/100) (set to sigma*100) */
/*  200  (Entire atmosphere - taken as a single layer)                */
/*  214  (Low cloud)                                                  */
/*  224  (Mid cloud)                                                  */
/*  234  (High cloud)                                                 */
static const LevelDefinitions LevelDefs[] =
	{

		{   1, 0,   0,   0,   0,   0, "surface" },
		{   2, 0,   0,   0,   0,   0, "cloud_base" },
		{   3, 0,   0,   0,   0,   0, "cloud_top" },
		{   4, 0,   0,   0,   0,   0, "zero_degree" },
		{   5, 0,   0,   0,   0,   0, "lifted_cond" },
		{   6, 0,   0,   0,   0,   0, "max_wind" },
		{   7, 0,   0,   0,   0,   0, "tropopause" },
		{   8, 0,   0,   0,   0,   0, "atm_top" },
		{   9, 0,   0,   0,   0,   0, "sea_bottom" },
		{  20, 1, .01,   0,   0,   0, "isothermal" },
		{ 100, 1,   1,   0,   0,   0, "mb" },
		{ 101, 2,  10,   0,  10,   0, "mb" },
		{ 102, 0,   0,   0,   0,   0, "msl" },
		{ 103, 1,   1,   0,   0,   0, "altitude" },
		{ 104, 2, 100,   0, 100,   0, "altitude" },
		{ 105, 1,   1,   0,   0,   0, "height" },
		{ 106, 2, 100,   0, 100,   0, "height" },
		{ 107, 1, .01,   0,   0,   0, "sigma" },
		{ 108, 2,   1,   0,   1,   0, "sigma" },
		{ 109, 1,   1,   0,   0,   0, "hybrid" },
		{ 110, 2,   1,   0,   1,   0, "hybrid" },
		{ 111, 1,   1,   0,   0,   0, "cm_below_sfc" },
		{ 112, 2,   1,   0,   1,   0, "cm_below_sfc" },
		{ 113, 1,   1,   0,   0,   0, "theta" },
		{ 114, 2,  -1, 475,  -1, 475, "theta" },
		{ 115, 1,   1,   0,   0,   0, "mb_agl" },
		{ 116, 2,   1,   0,   1,   0, "mb_agl" },
		{ 117, 1,   1,   0,   0,   0, "PV" },
		{ 119, 1, .01,   0,   0,   0, "ETA" },
		{ 120, 2,   1,   0,   1,   0, "ETA" },
		{ 121, 2,  -1,1100,  -1,1100, "mb" },
		{ 125, 1, .01,   0,   0,   0, "height" },
		{ 128, 2, -.1,  11, -.1,  11, "sigma" },
		{ 141, 2,   1,   0,  -1,1100, "mb" },
		{ 160, 1,   1,   0,   0,   0, "below_sea_level" },
		{ 200, 0,   0,   0,   0,   0, "entire_atmosphere" },
		{ 201, 0,   0,   0,   0,   0, "entire_ocean" },
		{ 204, 0,   0,   0,   0,   0, "trop_freeze" },
		{ 209, 0,   0,   0,   0,   0, "bl_cloud_bottom" },
		{ 210, 0,   0,   0,   0,   0, "bl_cloud_top" },
		{ 211, 0,   0,   0,   0,   0, "bl_cloud" },
		{ 212, 0,   0,   0,   0,   0, "low_cloud_bottom" },
		{ 213, 0,   0,   0,   0,   0, "low_cloud_top" },
		{ 214, 0,   0,   0,   0,   0, "low_cloud" },
		{ 222, 0,   0,   0,   0,   0, "mid_cloud_bottom" },
		{ 223, 0,   0,   0,   0,   0, "mid_cloud_top" },
		{ 224, 0,   0,   0,   0,   0, "mid_cloud" },
		{ 232, 0,   0,   0,   0,   0, "high_cloud_bottom" },
		{ 233, 0,   0,   0,   0,   0, "high_cloud_top" },
		{ 234, 0,   0,   0,   0,   0, "high_cloud" },
		{ 242, 0,   0,   0,   0,   0, "conv_cloud_bottom" },
		{ 243, 0,   0,   0,   0,   0, "conv_cloud_top" },
		{ 244, 0,   0,   0,   0,   0, "conv_cloud" },

	};


/* Set number of predefined level labels */
static const int NumLevelDefs =
	(int) (sizeof(LevelDefs) / sizeof(LevelDefinitions));



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
	short int resltn;			/* 17: RESOLUTION AND COMPONENT FLAG */
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


/***********************************************************************
*                                                                      *
*  Declare external functions in rgrib_edition1.c                      *
*                                                                      *
***********************************************************************/

LOGICAL	open_gribfile_edition1(STRING);
LOGICAL	next_gribfield_edition1(GRIBFIELD **);
LOGICAL	gribfield_identifiers_edition1(STRING *, STRING *, STRING *, STRING *,
										STRING *, STRING *, STRING *);
void	close_gribfile_edition1(void);


/* Now it has been included */
#endif
