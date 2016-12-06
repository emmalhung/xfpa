/***********************************************************************
*                                                                      *
*     f p a g p g e n _ s t r u c t s . h                              *
*                                                                      *
*     Version 5 (c) Copyright 1997 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#ifndef FPAGPGEN_STRUCTS_DEFS
#define FPAGPGEN_STRUCTS_DEFS


/* We need FPA library definitions */
#include <fpa.h>


/***********************************************************************
*                                                                      *
*  Define FpaGPgen program types                                       *
*                                                                      *
***********************************************************************/

/* Define list of acceptable programs */
typedef enum { GPG_PSMet, GPG_SVGMet, GPG_CorMet, GPG_TexMet } GPGprogram;

/* Define structure to hold program information */
typedef struct
	{
	STRING			program;
	GPGprogram		macro;
	STRING			pdf_tag;
	STRING			out_tag;
	STRING			label;
	float			default_width_scale;
	float			default_height_scale;
	} PROGRAM_INFO;


/***********************************************************************
*                                                                      *
*  Define FpaGPgen string sizes and directive reading buffer           *
*                                                                      *
***********************************************************************/

#define GPGTiny		32
#define GPGShort	64
#define GPGMedium	256
#define GPGLong		512
#define GPGHuge		25600

typedef char DirectiveBuffer[GPGMedium];

/***********************************************************************
*                                                                      *
*  Define FpaGPgen parameters in various routines                      *
*                                                                      *
***********************************************************************/

#define GPGstart			"start"
#define GPGend				"end"

#define AnchorMap				"map"
#define AnchorMapLatLon			"map_latlon"
#define AnchorAbsolute			"absolute"
#define AnchorCurrent			"current"
#define AnchorLowerLeft			"lower_left"
#define AnchorCentreLeft		"centre_left"
#define AnchorCenterLeft		"center_left"
#define AnchorUpperLeft			"upper_left"
#define AnchorLowerCentre		"lower_centre"
#define AnchorLowerCenter		"lower_center"
#define AnchorCentre			"centre"
#define AnchorCenter			"center"
#define AnchorUpperCentre		"upper_centre"
#define AnchorUpperCenter		"upper_center"
#define AnchorLowerRight		"lower_right"
#define AnchorCentreRight		"centre_right"
#define AnchorCenterRight		"center_right"
#define AnchorUpperRight		"upper_right"
#define AnchorXsectStart		"xsect_"
#define AnchorXsectLowerLeft	"xsect_lower_left"
#define AnchorXsectCentreLeft	"xsect_centre_left"
#define AnchorXsectCenterLeft	"xsect_center_left"
#define AnchorXsectUpperLeft	"xsect_upper_left"
#define AnchorXsectLowerCentre	"xsect_lower_centre"
#define AnchorXsectLowerCenter	"xsect_lower_center"
#define AnchorXsectCentre		"xsect_centre"
#define AnchorXsectCenter		"xsect_center"
#define AnchorXsectUpperCentre	"xsect_upper_centre"
#define AnchorXsectUpperCenter	"xsect_upper_center"
#define AnchorXsectLowerRight	"xsect_lower_right"
#define AnchorXsectCentreRight	"xsect_centre_right"
#define AnchorXsectCenterRight	"xsect_center_right"
#define AnchorXsectUpperRight	"xsect_upper_right"

#define AreaTypeSubareas	"subareas"
#define AreaTypeBoundary	"boundary"
#define AreaTypeDivides		"divides"

#define ArrowHead			"head"
#define ArrowTail			"tail"
#define ArrowBoth			"both"
#define ArrowBothRev		"both_tail_reversed"

#define AttribAnchorNone	"none"
#define AttribGPGenDefault			"GPGEN_default_attribute"
#define AttribGPGenIdent			"GPGEN_ident"
#define AttribGPGenLabel			"GPGEN_label"
#define AttribGPGenValue			"GPGEN_value"
#define AttribGPGenWind				"GPGEN_wind"
#define AttribGPGenFeatureAttrib	"GPGEN_feature_attribute"
#define AttribGPGenLat				"GPGEN_lat"
#define AttribGPGenLatDDMM			"GPGEN_lat_ddmm"
#define AttribGPGenLon				"GPGEN_lon"
#define AttribGPGenLonDDMM			"GPGEN_lon_ddmm"
#define AttribGPGenProximity		"GPGEN_proximity"
#define AttribGPGenNegProximity		"GPGEN_negative_proximity"
#define AttribGPGenBearing			"GPGEN_bearing"
#define AttribGPGenLineDirTo		"GPGEN_line_direction_to"
#define AttribGPGenLineDirFrom		"GPGEN_line_direction_from"
#define AttribGPGenLineLength		"GPGEN_line_length"
#define AttribGPGenLchainDir		"GPGEN_link_chain_direction"
#define AttribGPGenLchainSpd		"GPGEN_link_chain_speed"
#define AttribGPGenLchainVector		"GPGEN_link_chain_vector"
#define AttribGPGenLchainLength		"GPGEN_link_chain_length"
#define AttribGPGenXsectDir			"GPGEN_cross_section_direction"
#define AttribGPGenXsectSpd			"GPGEN_cross_section_speed"
#define AttribGPGenXsectVector		"GPGEN_cross_section_vector"
#define AttribGPGenXsectLength		"GPGEN_cross_section_length"
#define AttribGPGenProgTime			"GPGEN_prog_time"
#define AttribGPGenProgTimeHours	"GPGEN_prog_time_hours"
#define AttribGPGenProgTimeMinutes	"GPGEN_prog_time_minutes"
#define AttribGPGenGMTTime			"GPGEN_gmt_time"
#define AttribGPGenUTCTime			"GPGEN_utc_time"
#define AttribGPGenLocalTime		"GPGEN_local_time"
#define AttribGPGenT0Time			"GPGEN_T0_time"
#define AttribGPGenCreationTime		"GPGEN_creation_time"

#define AttribGPGenAll				"GPGEN_All"
#define AttribGPGenMissing			"GPGEN_Missing"
#define AttribGPGenDoNotMatch		"GPGEN_DoNotMatch"

#define CaseDay				"day"
#define CaseNight			"night"
#define CaseDusk			"dusk"
#define CaseNorthernHem		"northern_hemisphere"
#define CaseSouthernHem		"southern_hemisphere"

#define CatCascadeAnd		"and"
#define CatCascadeOr		"or"

#define ColourNone			"none"

#define DefProximity		   0.0
#define DefBearing			-999.0
#define DefLineLen			-999.0
#define DefSegDir			-999.0
#define DefSegSpd			-999.0

#define DisplayUnitsInches	"inches"
#define DisplayUnitsCM		"cm"
#define DisplayUnitsMM		"mm"
#define DisplayUnitsPicas	"picas"
#define DisplayUnitsPoints	"points"

#define DistanceScaleUnitsM	"m"

#define FeatureSpeedUnitsMS	"m/s"

#define FitToMapRefNone		"none"
#define FitToMapRefUpper	"upper"
#define FitToMapRefLower	"lower"
#define FitToMapRefLeft		"left"
#define FitToMapRefRight	"right"

#define FontWeightNone		"none"

#define FormatNone			"none"
#define FormatDirect		"direct"
#define FormatSymbol		"symbol"
#define FormatText			"text"
#define FormatWindBarb		"wind_barb"
#define FormatWindText		"wind_text"
#define FormatWindSymbol	"wind_symbol"
#define FormatVectorText	"vector_text"
#define FormatVectorSymbol	"vector_symbol"

#define FormatTailWindBarb		"tail_wind_barb"
#define FormatTailWindText		"tail_wind_text"
#define FormatTailWindSymbol	"tail_wind_symbol"
#define FormatCrossWindBarb		"cross_wind_barb"
#define FormatCrossWindText		"cross_wind_text"
#define FormatCrossWindSymbol	"cross_wind_symbol"

#define JustifyLeft			"left"
#define JustifyRight		"right"
#define JustifyCentre		"centre"
#define JustifyCenter		"center"

#define KeyNone				"none"

#define LabelSizedBox		"sized_box"
#define LabelFixedBox		"fixed_box"
#define LabelSizedEllipse	"sized_ellipse"
#define LabelFixedEllipse	"fixed_ellipse"
#define LabelSizedUnderline	"sized_underline"
#define LabelFixedUnderline	"fixed_underline"

#define LabelDisplayUnitsKm	"km"

#define LineLengthUnitsKm	"km"

#define LnodeSpeedUnitsKnot	"knots"

#define LocationUnitsKm		"km"

#define LocIdentAll			"GPGEN_All"
#define LocIdentAllVtime	"GPGEN_All_at_vtime"

#define LogicalYes			"yes"
#define LogicalNo			"no"

#define LookupCategoryExt	".tab"
#define LookupWindExt		".wtab"
#define LookupVectorExt		".vtab"
#define LookupLocationExt	".ltab"
#define LookupVerticalExt	".ztab"

#define PatternSimple		"simple"

#define ProximityUnitsKm	"km"

#define RangeRingUnitsKm	"km"

#define ScaleXaxis			"x_axis"
#define ScaleYaxis			"y_axis"
#define ScaleLongest		"longest"
#define ScaleShortest		"shortest"

#define SymbolFillNone		"none"

#define TableCol			"column"
#define TableRow			"row"

#define VerticalBottom		"bottom"
#define VerticalCentre		"centre"
#define VerticalCenter		"center"
#define VerticalTop			"top"

#define VerAttribBase		"GPGEN_cross_section_base"
#define VerAttribTop		"GPGEN_cross_section_top"

#define WindCalm			"calm"
#define WindDirection		"direction"
#define WindSpeed			"speed"
#define WindGust			"gust"
#define VectorCalm			"calm"
#define VectorDirection		"direction"
#define VectorSpeed			"speed"
#define WVsubNone				"none"
#define WVsubValue				"value"
#define WVsubText				"text"
#define WVsubSymbol				"symbol"
#define WVsubUniform			"uniform"
#define WVsubProportional		"proportional"
#define WVsubEnd				"end"

#define WriteTimeIssue		"issue"
#define WriteTimeValid		"valid"
#define WriteTimeCreation	"creation"
#define WriteTimeGMT		"GMT"
#define WriteTimeUTC		"UTC"
#define WriteTimeLMT		"LMT"
#define WriteTimeLCL		"LCL"

#define XSectTime			"time"
#define XSectTimeRoute		"time_route"
#define XSectSpace			"space"
#define XSectSpaceRoute		"space_route"

#define XSectAxis			"axis"
#define XSectTicks			"ticks"
#define XSectHLines			"horizontal_lines"
#define XSectVLines			"vertical_lines"

#define XSectAxisLower		"lower"
#define XSectAxisUpper		"upper"
#define XSectAxisLeft		"left"
#define XSectAxisRight		"right"

#define XSectLineLinear		"linear"
#define XSectLineStepBefore	"step_before"
#define XSectLineStepCentre	"step_centre"
#define XSectLineStepCenter	"step_center"
#define XSectLineStepAfter	"step_after"
#define XSectLineBox		"box"

/* Define list of sampling types for list sampling */
typedef enum { GPG_LatLon, GPG_MapXY, GPG_Ident } GPGsample;

/* Define list of data file types */
typedef enum { GPG_None, GPG_Identifier, GPG_Latitude, GPG_Longitude,
				GPG_TimeStamp, GPG_Label, GPG_Value, GPG_Units,
				GPG_Wind, GPG_WindDirection, GPG_WindSpeed, GPG_WindGust,
				GPG_WindUnits } GPGdtype;

/* Define list of location types for cross sections */
typedef enum { GPG_LocNone, GPG_LocDistance, GPG_LocTime,
				GPG_LocFraction } GPGltype;


/***********************************************************************
*                                                                      *
*  Define FpaGPgen structures                                          *
*                                                                      *
***********************************************************************/

/* Structure for @display_units directive */
typedef struct
	{
	char	type[GPGShort];
	float	conversion;
	float	sfactor;
	} DISPLAY_UNITS;

/* Structure for component presentations */
typedef struct
	{
	char	line_width[GPGMedium];
	char	line_style[GPGMedium];
	char	outline[GPGMedium];
	char	fill[GPGMedium];
	} COMP_PRES;

/* Structure for @presentation directive ... and named presentations */
typedef struct
	{
	char	name[GPGMedium];
	char	line_width[GPGMedium];
	char	line_style[GPGMedium];
	char	outline[GPGMedium];
	char	fill[GPGMedium];
	LOGICAL	outline_first;
	char	interior_fill[GPGMedium];
	char	sym_fill_name[GPGMedium];
	char	pattern[GPGMedium];
	char	pattern_width[GPGMedium];
	char	pattern_length[GPGMedium];
	char	font[GPGMedium];
	char	font_weight[GPGMedium];
	char	italics[GPGMedium];
	char	text_size[GPGMedium];
	char	justified[GPGMedium];
	char	char_space[GPGMedium];
	char	word_space[GPGMedium];
	char	line_space[GPGMedium];
	int			num_comp;
	COMP_PRES	*comp_pres;
	} PRES;

/* Structure for case presentations */
typedef struct
	{
	char	spcase[GPGMedium];
	char	lookup[GPGMedium];
	PRES	pres;
	} SPCASE;

/* Structure for category matching */
typedef struct
	{
	char	category_attribute[GPGMedium];
	char	category[GPGMedium];
	} CATATTRIB;

/* Structure for time display */
typedef struct
	{
	char	time_type[GPGMedium];
	char	zone_type[GPGMedium];
	char	time_zone[GPGMedium];
	char	language[GPGMedium];
	char	time_format[GPGMedium];
	} TTYPFMT;

/* Structure for multiple fields */
typedef struct
	{
	char	element[GPGMedium];
	char	level[GPGMedium];
	char	equation[GPGMedium];
	} GRA_FLD_INFO;

/* Structure for multiple images */
typedef struct
	{
	char	image_tag[GPGMedium];
	char	ctable[GPGMedium];
	char	brightness[GPGMedium];
	} GRA_IMAGE_INFO;

/* Structure for named lines */
typedef struct
	{
	char	name[GPGMedium];
	LINE	line;
	} GRA_LINE;

/* Structures for named cross sections */
typedef struct
	{
	float	xdist;
	char	vtime[GPGMedium];
	} XSECT_LOCATION;
typedef struct
	{
	int		num;
	STRING	*idents;
	float	*flats;
	float	*flons;
	STRING	*vtimes;
	STRING	*labels;
	double	*pstns;
	double	*dvals;
	double	*tvals;
	double	*locs;
	double	*dirs;
	double	*spds;
	} XSECT_HOR_AXIS;
typedef struct
	{
	int		num;
	STRING	*idents;
	STRING	*labels;
	double	*pstns;
	double	*vals;
	double	*locs;
	} XSECT_VER_AXIS;
typedef struct
	{
	char	label[GPGShort];
	char	type[GPGShort];
	float	width;
	float	height;
	float	x_off;
	float	y_off;
	char	location_lookup[GPGMedium];
	char	vertical_lookup[GPGMedium];
	XSECT_HOR_AXIS	*haxis;
	XSECT_VER_AXIS	*vaxis;
	double	drange;
	double	trange;
	} GRA_XSECT;

/* Structure for named sample grids */
typedef struct
	{
	char	label[GPGShort];
	int		numx;
	int		numy;
	float	**flats;
	float	**flons;
	float	x_shift;
	float	y_shift;
	} GRA_GRID;

/* Structure for sample location for list sampling */
typedef struct
	{
	GPGsample	macro;
	float		xval;
	float		yval;
	char		ident[GPGMedium];
	} GRA_LCTN;

/* Structure for named sample lists */
typedef struct
	{
	char	label[GPGShort];
	int		num;
	LOGICAL	*usell;
	float	*flats;
	float	*flons;
	STRING	*idents;
	float	x_shift;
	float	y_shift;
	int		x_wrap;
	int		y_wrap;
	} GRA_LIST;

/* Structure for named tables */
typedef struct
	{
	char	label[GPGShort];
	char	type[GPGShort];
	float	x_off;
	float	y_off;
	int		nsites;
	LOGICAL	*usell;
	float	*flats;
	float	*flons;
	STRING	*idents;
	float	*offset;
	} GRA_TABLE;

/* Structure for named patterns */
typedef struct
	{
	STRING	name;		/* pattern name */
	int		num;		/* number of pattern components */
	ITEM	*list;		/* list of pattern components */
	STRING	*type;		/* list of component types */
	LOGICAL	*contig;	/* list of contiguous line flags */
	} PATTERN;

/* Structure for wind barb presentation */
typedef struct
	{
	float	shaft_length;
	float	barb_length;
	float	barb_width;
	float	barb_space;
	float	barb_angle;
	float	speed_round;
	float	gust_above;
	float	gust_round;
	float	gust_size;
	float	gust_distance;
	float	gust_angle;
	char	gust_just[GPGMedium];
	char	gust_format[GPGMedium];
	float	calm_max;
	char	calm_symbol[GPGMedium];
	float	calm_scale;
	float	huge_min;
	char	huge_symbol[GPGMedium];
	float	huge_scale;
	} BARB_PRES;

/* Structure for wind text or symbol presentation */
typedef struct
	{
	char	wind_lookup[GPGMedium];
	char	calm_type[GPGMedium];
	char	calm_just[GPGMedium];
	char	calm_format[GPGMedium];
	float	calm_size;
	float	calm_scale;
	float	x_calm;
	float	y_calm;
	char	direction_type[GPGMedium];
	char	direction_just[GPGMedium];
	char	direction_format[GPGMedium];
	float	direction_size;
	float	direction_scale;
	float	x_dir;
	float	y_dir;
	char	speed_type[GPGMedium];
	char	speed_just[GPGMedium];
	char	speed_format[GPGMedium];
	float	speed_size;
	float	speed_scale;
	float	x_spd;
	float	y_spd;
	char	gust_type[GPGMedium];
	char	gust_just[GPGMedium];
	char	gust_format[GPGMedium];
	float	gust_above;
	float	gust_size;
	float	gust_scale;
	float	x_gust;
	float	y_gust;
	} WIND_PRES;

/* Structure for vector text or symbol presentation */
typedef struct
	{
	char	vector_lookup[GPGMedium];
	char	calm_type[GPGMedium];
	char	calm_just[GPGMedium];
	char	calm_format[GPGMedium];
	float	calm_size;
	float	calm_scale;
	float	x_calm;
	float	y_calm;
	char	direction_type[GPGMedium];
	char	direction_just[GPGMedium];
	char	direction_format[GPGMedium];
	float	direction_size;
	float	direction_scale;
	float	x_dir;
	float	y_dir;
	char	speed_type[GPGMedium];
	char	speed_just[GPGMedium];
	char	speed_format[GPGMedium];
	float	speed_size;
	float	speed_scale;
	float	x_spd;
	float	y_spd;
	} VECTOR_PRES;

/* Structure for arrow display */
typedef struct
	{
	char	name[GPGMedium];
	char	features[GPGMedium];
	float	length;
	float	angle;
	float	return_angle;
	float	length_off;
	float	width_off;
	float	head_length;
	float	tail_length;
	} ARROW_DISPLAY;

/* Structure for label display */
typedef struct
	{
	char	name[GPGMedium];
	PRES	presentation;
	float	width;
	float	height;
	char	width_attrib[GPGMedium];
	char	height_attrib[GPGMedium];
	char	diameter_attrib[GPGMedium];
	char	radius_attrib[GPGMedium];
	char	attrib_units[GPGMedium];
	float	start_angle;
	float	end_angle;
	LOGICAL	closed;
	float	rotation;
	char	rot_attrib[GPGMedium];
	float	x_off;
	float	y_off;
	float	margin_left;
	float	margin_right;
	float	margin_top;
	float	margin_bottom;
	} LABEL_DISPLAY;

/* Structure for attribute display */
typedef struct
	{
	char	name[GPGMedium];
	LOGICAL	show_label;
	char	anchor[GPGMedium];
	char	ref[GPGMedium];
	PRES	presentation;
	char	vertical_just[GPGMedium];
	char	units[GPGMedium];
	char	format[GPGMedium];
	char	conversion_format[GPGMedium];
	char	look_up[GPGMedium];
	float	symbol_scale;
	float	txt_size;
	char	display_name[GPGMedium];
	char	display_type[GPGMedium];
	float	width_scale;
	float	height_scale;
	float	x_off;
	float	y_off;
	float	x_attrib;
	float	y_attrib;
	float	x_left;
	float	x_centre;
	float	x_right;
	float	y_top;
	float	y_centre;
	float	y_bottom;
	} ATTRIB_DISPLAY;

/* Structure for distance scale display */
typedef struct
	{
	char	name[GPGMedium];
	float	slength;
	float	srotate;
	LINE	sline;
	} SCALE_DISPLAY;

/* Structure for distance scale locations */
typedef struct
	{
	float	lctn;
	char	label[GPGMedium];
	} SCALE_LCTNS;

/* Structure for symbol fill display */
typedef struct
	{
	char	name[GPGMedium];
	PRES	presentation;
	char	symbol[GPGMedium];
	float	sym_scale;
	float	sym_rotate;
	float	rep_rotate;
	float	x_off;
	float	y_off;
	float	x_repeat;
	float	y_repeat;
	float	x_shift;
	float	y_shift;
	} SYMBOL_FILL;


/***********************************************************************
*                                                                      *
*  Define FpaGPgen GLOBALs  (Bruno says sorry Ross!)                   *
*                                                                      *
***********************************************************************/

#undef  GLOBAL
#ifdef FPAGPGEN_MAIN
#	define GLOBAL GLOBAL_INIT
#else
#	define GLOBAL GLOBAL_EXTERN
#endif

/* Current program information and version number */
#define NO_PROGRAM	{FpaCblank, FpaCnoMacro, FpaCblank, FpaCblank, FpaCblank}
GLOBAL(PROGRAM_INFO,	Program,					NO_PROGRAM);
GLOBAL(char,			Version[GPGTiny],			FpaCblank);
GLOBAL(LOGICAL,			OldVersion,					FALSE);
GLOBAL(LOGICAL,			ObsoleteVersion,			FALSE);

/* Debugging flag for verbose comments */
GLOBAL(LOGICAL,			Verbose,					FALSE);

/* Current setup file, source, creation time, run time, valid time */
GLOBAL(char,			SetupFile[GPGMedium],		FpaCblank);
GLOBAL(char,			CurSource[GPGMedium],		FpaCblank);
GLOBAL(char,			CurSubSource[GPGMedium],	FpaCblank);
GLOBAL(char,			TCstamp[GPGTiny],			FpaCblank);
GLOBAL(char,			T0stamp[GPGTiny],			FpaCblank);
GLOBAL(char,			TVstamp[GPGTiny],			FpaCblank);

/* Current object and attribute for loop based on field features */
GLOBAL(char,			CurType[GPGTiny],			FpaCblank);
GLOBAL(AREA,			CurArea,					NullArea);
GLOBAL(CURVE,			CurCurve,					NullCurve);
GLOBAL(SPOT,			CurSpot,					NullSpot);
GLOBAL(LCHAIN,			CurLchain,					NullLchain);
GLOBAL(char,			CurAttribute[GPGMedium],	FpaCblank);
GLOBAL(char,			CurVtime[GPGTiny],			FpaCblank);

/* Display units */
#define NO_DISPLAY_UNITS	{FpaCblank, 0.0, 1.0}
GLOBAL(DISPLAY_UNITS,	DisplayUnits,				NO_DISPLAY_UNITS);

/* Map projection information */
GLOBAL(MAP_PROJ,		BaseMap,					NO_MAPPROJ);

/* Global field descriptor */
GLOBAL(FLD_DESCRIPT,	Fdesc,						FpaNO_FDESC);

/* General presentation */
#define NO_PRES	{FpaCblank, FpaCblank, FpaCblank, FpaCblank, FpaCblank,    \
					FALSE, FpaCblank, FpaCblank, FpaCblank, FpaCblank,     \
					FpaCblank, FpaCblank, FpaCblank, FpaCblank, FpaCblank, \
					FpaCblank, FpaCblank, FpaCblank, FpaCblank,            \
					0, NullPtr(COMP_PRES *) }
GLOBAL(PRES,			NoPresDef,					NO_PRES);
GLOBAL(PRES,			PresDef,					NO_PRES);
GLOBAL(PRES,			CurPres,					NO_PRES);

/* Component presentation */
#define NO_COMP_PRES	{FpaCblank, FpaCblank, FpaCblank, FpaCblank}
GLOBAL(COMP_PRES,		NoCompPres,					NO_COMP_PRES);

/* Wind presentation */
#define NO_BARB_PRES		{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, \
								0.0, 0.0, 0.0, FpaCblank, FpaCblank, \
								0.0, FpaCblank, 0.0, 0.0, FpaCblank, 0.0}
#define NO_WIND_PRES		{FpaCblank, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0, 0.0}
GLOBAL(BARB_PRES,		BarbDef,					NO_BARB_PRES);
GLOBAL(WIND_PRES,		WindDef,					NO_WIND_PRES);

/* Vector presentation */
#define NO_VECTOR_PRES	{FpaCblank, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0, \
								FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, 0.0, 0.0}
GLOBAL(VECTOR_PRES,		VectorDef,					NO_VECTOR_PRES);

/* Arrow display */
#define NO_ARROW_DISPLAY	{FpaCblank, ArrowHead, 0.0, 0.0, 0.0, \
								0.0, 0.0, 0.0, 0.0}

/* Label display */
#define NO_LABEL_DISPLAY	{FpaCblank, NO_PRES, 0.0, 0.0, FpaCblank,       \
								FpaCblank, FpaCblank, FpaCblank, FpaCblank, \
								0.0, 0.0, TRUE, 0.0, FpaCblank, 0.0, 0.0,   \
								0.0, 0.0, 0.0, 0.0}

/* Attribute display */
#define NO_ATTRIB_DISPLAY	{FpaCblank, TRUE, AttribAnchorNone,     \
								AnchorLowerCentre, NO_PRES,         \
								FpaCblank, FpaCmksUnits,            \
								FormatDirect, FpaCblank, FpaCblank, \
								100.0, 0.0, FpaCblank, FpaCblank,   \
								60.0, 100.0, 0.0, 0.0,              \
								0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}

/* Symbol fill */
#define NO_SYMBOL_FILL		{FpaCblank, NO_PRES, FpaCblank, \
								100.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}

/* Structures for multiple fields */
#define NO_GRA_FLD_INFO		{FpaCblank, FpaCblank, FpaCblank}
GLOBAL(GRA_FLD_INFO,	NoGraFldInfo,				NO_GRA_FLD_INFO);

/* Structures for multiple images */
#define NO_GRA_IMAGE_INFO		{FpaCblank, FpaCblank, FpaCblank}
GLOBAL(GRA_IMAGE_INFO,	NoGraImageInfo,				NO_GRA_IMAGE_INFO);

/* Output positioning */
GLOBAL(char,			Anchor[GPGTiny],			FpaCblank);
GLOBAL(LOGICAL,			AnchorToMap,				TRUE);
GLOBAL(LOGICAL,			AnchorToCrossSection,		FALSE);
GLOBAL(DBLPT,			ULpoint,					ZERO_POINT);
GLOBAL(DBLPT,			LRpoint,					ZERO_POINT);
GLOBAL(DBLPT,			XYpoint,					ZERO_POINT);
GLOBAL(DBLPT,			XSect_ULpoint,				ZERO_POINT);
GLOBAL(DBLPT,			XSect_LRpoint,				ZERO_POINT);
GLOBAL(float,			PageWidth,					0.0);
GLOBAL(float,			PageHeight,					0.0);
GLOBAL(int,				Tnx,						0);
GLOBAL(int,				Tny,						0);

/* Line and outline drawing filter */
GLOBAL(float,			PolyFilter,					0.0);


/* Now it has been included */
#endif
