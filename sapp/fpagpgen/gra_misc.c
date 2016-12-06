/***********************************************************************
*                                                                      *
*     g r a _ m i s c . c                                              *
*                                                                      *
*     Routines to handle miscellaneous processes                       *
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

#include "colour_utils.h"
#include "fpagpgen_routines.h"
#include "fpagpgen_structs.h"

/* We need FPA library definitions */
#include <fpa.h>

/* We need C standard library definitions */
#include <time.h>
#include <stdio.h>

/***********************************************************************
* Definitions for perspective adjustments                              *
***********************************************************************/

/* Storage for adjusting map perspective */
static	LOGICAL	PerspectiveView        = FALSE;
static	LOGICAL	PerspectiveViewScale   = FALSE;
static	float	PerspectiveScale       = 1.0;
static	float	PerspectiveYTiltOffSet = 0.0;
static	float	PerspectiveCosTilt     = 1.0;
static	float	PerspectiveSinTilt     = 1.0;
static	float	PerspectiveXEye        = 0.0;
static	float	PerspectiveYEye        = 0.0;
static	float	PerspectiveZEye        = 0.0;
static	float	PerspectiveXStretch    = 1.0;
static	float	PerspectiveYStretch    = 1.0;

/***********************************************************************
* End definitions for perspective adjustments                          *
***********************************************************************/

/***********************************************************************
* Definitions for determining positions on maps                        *
***********************************************************************/

/* Storage for map scaling parameters */
static	float	Uscale          =  1.0;
static	DBLPT	MapCentreOffSet = ZERO_POINT;
static	double	MapWidth        =  0.0;
static	double	MapHeight       =  0.0;
static	double	HalfMapWidth    =  0.0;
static	double	HalfMapHeight   =  0.0;
static	float	Xorigin         =  0.0;
static	float	Yorigin         =  0.0;
static	float	MinSize         =  1.0;				/* inches */
static	float	MaxSize         = 11.0;				/* inches */

/***********************************************************************
* End definitions for determining positions on maps                    *
***********************************************************************/

/* Placeholder for missing information in lookup tables and data files */
#define GPGplaceHolder	"-"

/***********************************************************************
* Definitions for category lookup tables                               *
***********************************************************************/

/* Structures for holding parameters from category lookup tables */
typedef struct
	{
	STRING			value;
	STRING			symbolfile;
	STRING			text;
	STRING			presentation;
	} CAT_LOOKUP_LINE;

typedef struct
	{
	STRING				label;
	int					numlines;
	CAT_LOOKUP_LINE		*lines;
	LOGICAL				ismiss;
	CAT_LOOKUP_LINE		mline;
	LOGICAL				isdef;
	CAT_LOOKUP_LINE		dline;
	} CAT_LOOKUP_TABLE;

/* Storage for named category lookup tables */
static	int					NumCatLookup = 0;
static	CAT_LOOKUP_TABLE	*CatLookups  = NullPtr(CAT_LOOKUP_TABLE *);

/***********************************************************************
* End definitions for category lookup tables                           *
***********************************************************************/

/***********************************************************************
* Definitions for wind and vector lookup tables                        *
***********************************************************************/

/* Structures for holding parameters from wind lookup tables */
typedef struct
	{
	int				nvalue;
	float			*minval;
	float			*maxval;
	float			*round;
	int				*factor;
	int				*ndigit;
	} WVLOOKUP_VALUE;

typedef struct
	{
	int				ntext;
	float			*minval;
	float			*maxval;
	STRING			*texts;
	} WVLOOKUP_TEXT;

typedef struct
	{
	int				nsymbol;
	float			*minval;
	float			*maxval;
	STRING			*symbols;
	} WVLOOKUP_SYMBOL;

typedef struct
	{
	int				nsymbol;
	float			*minval;
	float			*maxval;
	STRING			*symbols;
	float			*rotate;
	} WVLOOKUP_UNIFORM;

typedef struct
	{
	int				nsymbol;
	float			*minval;
	float			*maxval;
	STRING			*symbols;
	float			*minscl;
	float			*maxscl;
	float			*rotate;
	} WVLOOKUP_PROPORT;

typedef struct
	{
	WVLOOKUP_VALUE		wind_val;
	WVLOOKUP_TEXT		wind_txt;
	WVLOOKUP_SYMBOL		wind_sym;
	} WIND_LOOKUP_CALM;

typedef struct
	{
	WVLOOKUP_VALUE		wind_val;
	WVLOOKUP_TEXT		wind_txt;
	WVLOOKUP_UNIFORM	wind_uni;
	WVLOOKUP_PROPORT	wind_pro;
	} WIND_LOOKUP_DIRECTION;

typedef struct
	{
	WVLOOKUP_VALUE		wind_val;
	WVLOOKUP_TEXT		wind_txt;
	WVLOOKUP_SYMBOL		wind_sym;
	} WIND_LOOKUP_SPEED;

typedef struct
	{
	WVLOOKUP_VALUE		wind_val;
	WVLOOKUP_TEXT		wind_txt;
	WVLOOKUP_SYMBOL		wind_sym;
	} WIND_LOOKUP_GUST;

typedef struct
	{
	STRING					label;
	WIND_LOOKUP_CALM		calm;
	WIND_LOOKUP_DIRECTION	direction;
	WIND_LOOKUP_SPEED		speed;
	WIND_LOOKUP_GUST		gust;
	} WIND_LOOKUP_TABLE;

/* Storage for named wind lookup tables */
static	int					NumWindLookup = 0;
static	WIND_LOOKUP_TABLE	*WindLookups  = NullPtr(WIND_LOOKUP_TABLE *);

typedef struct
	{
	WVLOOKUP_VALUE		vector_val;
	WVLOOKUP_TEXT		vector_txt;
	WVLOOKUP_SYMBOL		vector_sym;
	} VECTOR_LOOKUP_CALM;

typedef struct
	{
	WVLOOKUP_VALUE		vector_val;
	WVLOOKUP_TEXT		vector_txt;
	WVLOOKUP_UNIFORM	vector_uni;
	WVLOOKUP_PROPORT	vector_pro;
	} VECTOR_LOOKUP_DIRECTION;

typedef struct
	{
	WVLOOKUP_VALUE		vector_val;
	WVLOOKUP_TEXT		vector_txt;
	WVLOOKUP_SYMBOL		vector_sym;
	} VECTOR_LOOKUP_SPEED;

typedef struct
	{
	STRING					label;
	VECTOR_LOOKUP_CALM		calm;
	VECTOR_LOOKUP_DIRECTION	direction;
	VECTOR_LOOKUP_SPEED		speed;
	} VECTOR_LOOKUP_TABLE;

/* Storage for named vector lookup tables */
static	int					NumVectorLookup = 0;
static	VECTOR_LOOKUP_TABLE	*VectorLookups  = NullPtr(VECTOR_LOOKUP_TABLE *);

/***********************************************************************
* End definitions for wind and vector lookup tables                    *
***********************************************************************/

/***********************************************************************
* Definitions for location look up tables                              *
***********************************************************************/

/* Structures for holding parameters from location look up tables */
typedef struct
	{
	STRING			ident;
	STRING			lat;
	STRING			lon;
	STRING			vstring;
	STRING			llab;
	} LOC_LOOKUP_LINE;

typedef struct
	{
	STRING				label;
	int					numlines;
	LOC_LOOKUP_LINE		*lines;
	LOGICAL				isdef;
	LOC_LOOKUP_LINE		dline;
	} LOC_LOOKUP_TABLE;

/* Storage for named location look up tables */
static	int					NumLocLookup = 0;
static	LOC_LOOKUP_TABLE	*LocLookups  = NullPtr(LOC_LOOKUP_TABLE *);

/***********************************************************************
* End definitions for location look up tables                          *
***********************************************************************/

/***********************************************************************
* Definitions for vertical lookup tables                               *
***********************************************************************/

/* Structures for holding parameters from vertical lookup tables */
typedef struct
	{
	STRING			ident;
	STRING			llab;
	float			yvalue;
	float			ylocation;
	} VERT_LOOKUP_LINE;

typedef struct
	{
	STRING				label;
	int					numlines;
	VERT_LOOKUP_LINE	*lines;
	} VERT_LOOKUP_TABLE;

/* Storage for named vertical lookup tables */
static	int					NumVertLookup = 0;
static	VERT_LOOKUP_TABLE	*VertLookups  = NullPtr(VERT_LOOKUP_TABLE *);

/***********************************************************************
* End definitions for vertical lookup tables                           *
***********************************************************************/

/***********************************************************************
* Definitions for data files                                           *
***********************************************************************/

/* Structures for holding parameters from data files */
typedef struct
	{
	int				nparms;
	STRING			*parms;
	} DATA_FILE_LINE;

typedef struct
	{
	STRING			label;
	int				numlines;
	DATA_FILE_LINE	*lines;
	} DATA_FILE;

/* Storage for named vertical lookup tables */
static	int			NumDataFile = 0;
static	DATA_FILE	*DataFiles  = NullPtr(DATA_FILE *);

/***********************************************************************
* End definitions for data files                                       *
***********************************************************************/

/* Local variables for reading lookup files */
static  const   STRING  Comment      = "!#";	/* comment line character(s) */
static  const   STRING  CommentOrEnd = "!#\n";	/* comment line character(s) */
												/*  or end of line           */

/* Local variables for wind direction and speed */
#define	DegreesTrue		"degrees_true"
#define	Knots			"knots"

/* >>>>> debug testing for data files <<<<< */
static	LOGICAL	DebugMode = FALSE;
/* >>>>> debug testing for data files <<<<< */

/* Internal static functions */
static	void				determine_text_size(STRING, float, float, float,
										float *, float *);
static	void				determine_attribute_offset(float, float, float,
										float, float, LOGICAL *, LOGICAL *,
										LOGICAL *, LOGICAL *, float *, float *);
static	void				perspective_map_to_page(const POINT, POINT, float *);
static	void				perspective_page_to_map(const POINT, POINT, float *);
static	CAT_LOOKUP_TABLE	*get_category_lookup_table(STRING);
static	WIND_LOOKUP_TABLE	*get_wind_lookup_table(STRING);
static	VECTOR_LOOKUP_TABLE	*get_vector_lookup_table(STRING);
static	LOC_LOOKUP_TABLE	*get_location_lookup_table(STRING);
static	VERT_LOOKUP_TABLE	*get_vertical_lookup_table(STRING);
static	DATA_FILE			*get_data_file(STRING, STRING);
static	GPGdtype			parse_data_format(STRING, STRING *);
static	LOGICAL				match_wvlookup_value(float, WVLOOKUP_VALUE *,
										STRING *);
static	LOGICAL				match_wvlookup_text(float, WVLOOKUP_TEXT *,
										STRING *);
static	LOGICAL				match_wvlookup_symbol(float, WVLOOKUP_SYMBOL *,
										STRING *);
static	LOGICAL				match_wvlookup_dirvalue(float, float,
										WVLOOKUP_VALUE *, STRING *);
static	LOGICAL				match_wvlookup_dirtext(float, WVLOOKUP_TEXT *,
										STRING *);
static	LOGICAL				match_wvlookup_diruniform(float, float, float,
										WVLOOKUP_UNIFORM *, STRING *, float *);
static	LOGICAL				match_wvlookup_dirproportional(float, float,
										float, float, WVLOOKUP_PROPORT *,
										STRING *, float *, float *);
static	LOGICAL				match_wvlookup_gustvalue(float, WVLOOKUP_VALUE *,
										STRING *);
static	PATTERN				*find_pattern(STRING);
static	PATTERN				*read_pattern(STRING);

/***********************************************************************
*                                                                      *
*    d e f a u l t _ p s m e t _ p r e s e n t a t i o n               *
*                                                                      *
*    d e f a u l t _ s v g m e t _ p r e s e n t a t i o n             *
*                                                                      *
*    d e f a u l t _ c o r m e t _ p r e s e n t a t i o n             *
*                                                                      *
*    d e f a u l t _ t e x m e t _ p r e s e n t a t i o n             *
*                                                                      *
***********************************************************************/

void		default_psmet_presentation

	(
	)

	{
	STRING		sym_file;

	/* Set default values for presentation */
	/* Note that line, pattern, and text sizes are in points (1/72 inch)! */
	(void) strcpy(PresDef.name,           "");
	(void) strcpy(PresDef.line_width,     "0.001");			/* points */
	(void) strcpy(PresDef.line_style,     "[] 0");
	(void) strcpy(PresDef.outline,        "0 0 0");
	(void) strcpy(PresDef.fill,           ColourNone);
	(void) strcpy(PresDef.interior_fill,  ColourNone);
	(void) strcpy(PresDef.sym_fill_name,  SymbolFillNone);
	(void) strcpy(PresDef.pattern,        PatternSimple);
	(void) strcpy(PresDef.pattern_width,  "7");				/* points */
	(void) strcpy(PresDef.pattern_length, "43");			/* points */
	(void) strcpy(PresDef.font,           "Times-Roman");
	(void) strcpy(PresDef.font_weight,    "");
	(void) strcpy(PresDef.italics,        "");
	(void) strcpy(PresDef.justified,      JustifyLeft);
	(void) strcpy(PresDef.text_size,      "36");			/* points */
	(void) strcpy(PresDef.char_space,     "0");
	(void) strcpy(PresDef.word_space,     "100");
	(void) strcpy(PresDef.line_space,     "100");
	PresDef.outline_first = FALSE;
	PresDef.num_comp      = 0;
	PresDef.comp_pres     = NullPtr(COMP_PRES *);

	/* Save the default presentation */
	(void) copy_presentation(&CurPres, &PresDef);

	/* Set default wind barb presentation                              */
	/* Note that shaft length and gust size are in points (1/72 inch)! */
	BarbDef.shaft_length  =    43;			/* points */
	BarbDef.barb_length   =  0.50;
	BarbDef.barb_width    =  0.20;
	BarbDef.barb_space    =  0.20;
	BarbDef.barb_angle    =    60;			/* degrees */
	BarbDef.speed_round   =   0.0;			/* wind units */
	BarbDef.gust_above    = 500.0;			/* wind units */
	BarbDef.gust_round    =   0.0;			/* wind units */
	BarbDef.gust_size     =    15;			/* points */
	BarbDef.gust_distance =  0.50;
	BarbDef.gust_angle    =    90;			/* degrees */
	(void) strcpy(BarbDef.gust_just,   JustifyCentre);
	(void) strcpy(BarbDef.gust_format, "G%.0f");
	BarbDef.calm_max      =   0.5;			/* wind units */
	sym_file = find_symbol_file("circle_mark");
	(void) strcpy(BarbDef.calm_symbol, sym_file);
	BarbDef.calm_scale    =    50;
	BarbDef.huge_min      = 250.0;			/* wind units */
	(void) strcpy(BarbDef.huge_symbol, sym_file);
	BarbDef.huge_scale    =   100;

	/* Set default wind presentation                                */
	/* Note that sizes and x/y locations are in points (1/72 inch)! */
	(void) strcpy(WindDef.wind_lookup, FpaCblank);
	(void) strcpy(WindDef.calm_type,   FpaCblank);
	(void) strcpy(WindDef.calm_just,   JustifyCentre);
	(void) strcpy(WindDef.calm_format, "%s");
	WindDef.calm_size       =   15;			/* points */
	WindDef.calm_scale      =  100;
	WindDef.x_calm          =    0;			/* points */
	WindDef.y_calm          =    0;			/* points */
	(void) strcpy(WindDef.direction_type,   FpaCblank);
	(void) strcpy(WindDef.direction_just,   JustifyRight);
	(void) strcpy(WindDef.direction_format, "%s");
	WindDef.direction_size  =    15;		/* points */
	WindDef.direction_scale =   100;
	WindDef.x_dir           =     0;		/* points */
	WindDef.y_dir           =     0;		/* points */
	(void) strcpy(WindDef.speed_type,   FpaCblank);
	(void) strcpy(WindDef.speed_just,   JustifyLeft);
	(void) strcpy(WindDef.speed_format, "%s");
	WindDef.speed_size      =    15;		/* points */
	WindDef.speed_scale     =   100;
	WindDef.x_spd           =     0;		/* points */
	WindDef.y_spd           =     0;		/* points */
	(void) strcpy(WindDef.gust_type,   FpaCblank);
	(void) strcpy(WindDef.gust_just,   JustifyLeft);
	(void) strcpy(WindDef.gust_format, "%s");
	WindDef.gust_above      = 500.0;		/* wind units */
	WindDef.gust_size       =    15;		/* points */
	WindDef.gust_scale      =   100;
	WindDef.x_gust          =     0;		/* points */
	WindDef.y_gust          =     0;		/* points */

	/* Set default vector presentation                              */
	/* Note that sizes and x/y locations are in points (1/72 inch)! */
	(void) strcpy(VectorDef.vector_lookup, FpaCblank);
	(void) strcpy(VectorDef.calm_type,     FpaCblank);
	(void) strcpy(VectorDef.calm_just,     JustifyCentre);
	(void) strcpy(VectorDef.calm_format,   "%s");
	VectorDef.calm_size       =   15;		/* points */
	VectorDef.calm_scale      =  100;
	VectorDef.x_calm          =    0;		/* points */
	VectorDef.y_calm          =    0;		/* points */
	(void) strcpy(VectorDef.direction_type,   FpaCblank);
	(void) strcpy(VectorDef.direction_just,   JustifyRight);
	(void) strcpy(VectorDef.direction_format, "%s");
	VectorDef.direction_size  =    15;		/* points */
	VectorDef.direction_scale =   100;
	VectorDef.x_dir           =     0;		/* points */
	VectorDef.y_dir           =     0;		/* points */
	(void) strcpy(VectorDef.speed_type,   FpaCblank);
	(void) strcpy(VectorDef.speed_just,   JustifyLeft);
	(void) strcpy(VectorDef.speed_format, "%s");
	VectorDef.speed_size      =    15;		/* points */
	VectorDef.speed_scale     =   100;
	VectorDef.x_spd           =     0;		/* points */
	VectorDef.y_spd           =     0;		/* points */
	}

void		default_svgmet_presentation

	(
	)

	{
	STRING		sym_file;

	/* Set default values for presentation */
	/* Note that line, pattern, and text sizes are in points (1/72 inch)! */
	(void) strcpy(PresDef.name,           "");
	(void) strcpy(PresDef.line_width,     "0.001");			/* points */
	(void) strcpy(PresDef.line_style,     "none");
	(void) strcpy(PresDef.outline,        "rgb(0,0,0)");
	(void) strcpy(PresDef.fill,           ColourNone);
	(void) strcpy(PresDef.interior_fill,  ColourNone);
	(void) strcpy(PresDef.sym_fill_name,  SymbolFillNone);
	(void) strcpy(PresDef.pattern,        PatternSimple);
	(void) strcpy(PresDef.pattern_width,  "7");				/* points */
	(void) strcpy(PresDef.pattern_length, "43");			/* points */
	(void) strcpy(PresDef.font,           "Times-Roman");
	(void) strcpy(PresDef.font_weight,    "");
	(void) strcpy(PresDef.italics,        "");
	(void) strcpy(PresDef.justified,      JustifyLeft);
	(void) strcpy(PresDef.text_size,      "36");			/* points */
	(void) strcpy(PresDef.char_space,     "0");
	(void) strcpy(PresDef.word_space,     "100");
	(void) strcpy(PresDef.line_space,     "100");
	PresDef.outline_first = FALSE;
	PresDef.num_comp      = 0;
	PresDef.comp_pres     = NullPtr(COMP_PRES *);

	/* Save the default presentation */
	(void) copy_presentation(&CurPres, &PresDef);

	/* Set default wind barb presentation                              */
	/* Note that shaft length and gust size are in points (1/72 inch)! */
	BarbDef.shaft_length  =    43;			/* points */
	BarbDef.barb_length   =  0.50;
	BarbDef.barb_width    =  0.20;
	BarbDef.barb_space    =  0.20;
	BarbDef.barb_angle    =    60;			/* degrees */
	BarbDef.speed_round   =   0.0;			/* wind units */
	BarbDef.gust_above    = 500.0;			/* wind units */
	BarbDef.gust_round    =   0.0;			/* wind units */
	BarbDef.gust_size     =    15;			/* points */
	BarbDef.gust_distance =  0.50;
	BarbDef.gust_angle    =    90;			/* degrees */
	(void) strcpy(BarbDef.gust_just,   JustifyCentre);
	(void) strcpy(BarbDef.gust_format, "G%.0f");
	BarbDef.calm_max      =   0.5;			/* wind units */
	sym_file = find_symbol_file("circle_mark");
	(void) strcpy(BarbDef.calm_symbol, sym_file);
	BarbDef.calm_scale    =    50;
	BarbDef.huge_min      = 250.0;			/* wind units */
	(void) strcpy(BarbDef.huge_symbol, sym_file);
	BarbDef.huge_scale    =   100;

	/* Set default wind presentation                                */
	/* Note that sizes and x/y locations are in points (1/72 inch)! */
	(void) strcpy(WindDef.wind_lookup, FpaCblank);
	(void) strcpy(WindDef.calm_type,   FpaCblank);
	(void) strcpy(WindDef.calm_just,   JustifyCentre);
	(void) strcpy(WindDef.calm_format, "%s");
	WindDef.calm_size       =   15;			/* points */
	WindDef.calm_scale      =  100;
	WindDef.x_calm          =    0;			/* points */
	WindDef.y_calm          =    0;			/* points */
	(void) strcpy(WindDef.direction_type,   FpaCblank);
	(void) strcpy(WindDef.direction_just,   JustifyRight);
	(void) strcpy(WindDef.direction_format, "%s");
	WindDef.direction_size  =    15;		/* points */
	WindDef.direction_scale =   100;
	WindDef.x_dir           =     0;		/* points */
	WindDef.y_dir           =     0;		/* points */
	(void) strcpy(WindDef.speed_type,   FpaCblank);
	(void) strcpy(WindDef.speed_just,   JustifyLeft);
	(void) strcpy(WindDef.speed_format, "%s");
	WindDef.speed_size      =    15;		/* points */
	WindDef.speed_scale     =   100;
	WindDef.x_spd           =     0;		/* points */
	WindDef.y_spd           =     0;		/* points */
	(void) strcpy(WindDef.gust_type,   FpaCblank);
	(void) strcpy(WindDef.gust_just,   JustifyLeft);
	(void) strcpy(WindDef.gust_format, "%s");
	WindDef.gust_above      = 500.0;		/* wind units */
	WindDef.gust_size       =    15;		/* points */
	WindDef.gust_scale      =   100;
	WindDef.x_gust          =     0;		/* points */
	WindDef.y_gust          =     0;		/* points */

	/* Set default vector presentation                              */
	/* Note that sizes and x/y locations are in points (1/72 inch)! */
	(void) strcpy(VectorDef.vector_lookup, FpaCblank);
	(void) strcpy(VectorDef.calm_type,     FpaCblank);
	(void) strcpy(VectorDef.calm_just,     JustifyCentre);
	(void) strcpy(VectorDef.calm_format,   "%s");
	VectorDef.calm_size       =   15;		/* points */
	VectorDef.calm_scale      =  100;
	VectorDef.x_calm          =    0;		/* points */
	VectorDef.y_calm          =    0;		/* points */
	(void) strcpy(VectorDef.direction_type,   FpaCblank);
	(void) strcpy(VectorDef.direction_just,   JustifyRight);
	(void) strcpy(VectorDef.direction_format, "%s");
	VectorDef.direction_size  =    15;		/* points */
	VectorDef.direction_scale =   100;
	VectorDef.x_dir           =     0;		/* points */
	VectorDef.y_dir           =     0;		/* points */
	(void) strcpy(VectorDef.speed_type,   FpaCblank);
	(void) strcpy(VectorDef.speed_just,   JustifyLeft);
	(void) strcpy(VectorDef.speed_format, "%s");
	VectorDef.speed_size      =    15;		/* points */
	VectorDef.speed_scale     =   100;
	VectorDef.x_spd           =     0;		/* points */
	VectorDef.y_spd           =     0;		/* points */
	}

void		default_cormet_presentation

	(
	)

	{
	STRING		sym_file;

	/* Set default values for presentation */
	/* Note that line, pattern, and text sizes are in 1000ths of an inch! */
	(void) strcpy(PresDef.name,           "");
	(void) strcpy(PresDef.line_width,     "@wd 1");			/* 1000ths */
	(void) strcpy(PresDef.line_style,     "@dt 0 0");
	(void) strcpy(PresDef.outline,        "@uO 0 0 0 255");
	(void) strcpy(PresDef.fill,           "@xF");
	(void) strcpy(PresDef.interior_fill,  "@xF");
	(void) strcpy(PresDef.sym_fill_name,  SymbolFillNone);
	(void) strcpy(PresDef.pattern,        PatternSimple);
	(void) strcpy(PresDef.pattern_width,  "100");			/* 1000ths */
	(void) strcpy(PresDef.pattern_length, "600");			/* 1000ths */
	(void) strcpy(PresDef.font,           "@f \"Times-Roman\" 0");
	(void) strcpy(PresDef.font_weight,    "400");
	(void) strcpy(PresDef.italics,        "0");
	(void) strcpy(PresDef.justified,      JustifyLeft);
	(void) strcpy(PresDef.text_size,      "500");			/* 1000ths */
	(void) strcpy(PresDef.char_space,     "0");
	(void) strcpy(PresDef.word_space,     "100");
	(void) strcpy(PresDef.line_space,     "100");
	PresDef.outline_first = FALSE;
	PresDef.num_comp      = 0;
	PresDef.comp_pres     = NullPtr(COMP_PRES *);

	/* Save the default presentation */
	(void) copy_presentation(&CurPres, &PresDef);

	/* Set default wind barb presentation                              */
	/* Note that shaft length and gust size are in 1000ths of an inch! */
	BarbDef.shaft_length  =   600;			/* 1000ths */
	BarbDef.barb_length   =  0.50;
	BarbDef.barb_width    =  0.20;
	BarbDef.barb_space    =  0.20;
	BarbDef.barb_angle    =    60;			/* degrees */
	BarbDef.speed_round   =   0.0;			/* wind units */
	BarbDef.gust_above    = 500.0;			/* wind units */
	BarbDef.gust_round    =   0.0;			/* wind units */
	BarbDef.gust_size     =   200;			/* 1000ths */
	BarbDef.gust_distance =  0.50;
	BarbDef.gust_angle    =    90;			/* degrees */
	(void) strcpy(BarbDef.gust_just,   JustifyCentre);
	(void) strcpy(BarbDef.gust_format, "G%.0f");
	BarbDef.calm_max      =   0.5;			/* wind units */
	sym_file = find_symbol_file("circle_mark");
	(void) strcpy(BarbDef.calm_symbol, sym_file);
	BarbDef.calm_scale    =    50;
	BarbDef.huge_min      = 250.0;			/* wind units */
	(void) strcpy(BarbDef.huge_symbol, sym_file);
	BarbDef.huge_scale    =   100;

	/* Set default wind presentation                                */
	/* Note that sizes and x/y locations are in 1000ths of an inch! */
	(void) strcpy(WindDef.wind_lookup, FpaCblank);
	(void) strcpy(WindDef.calm_type,   FpaCblank);
	(void) strcpy(WindDef.calm_just,   JustifyCentre);
	(void) strcpy(WindDef.calm_format, "%s");
	WindDef.calm_size       =  200;			/* 1000ths */
	WindDef.calm_scale      =  100;
	WindDef.x_calm          =    0;			/* 1000ths */
	WindDef.y_calm          =    0;			/* 1000ths */
	(void) strcpy(WindDef.direction_type,   FpaCblank);
	(void) strcpy(WindDef.direction_just,   JustifyRight);
	(void) strcpy(WindDef.direction_format, "%s");
	WindDef.direction_size  =   200;		/* 1000ths */
	WindDef.direction_scale =   100;
	WindDef.x_dir           =     0;		/* 1000ths */
	WindDef.y_dir           =     0;		/* 1000ths */
	(void) strcpy(WindDef.speed_type,   FpaCblank);
	(void) strcpy(WindDef.speed_just,   JustifyLeft);
	(void) strcpy(WindDef.speed_format, "%s");
	WindDef.speed_size      =   200;		/* 1000ths */
	WindDef.speed_scale     =   100;
	WindDef.x_spd           =     0;		/* 1000ths */
	WindDef.y_spd           =     0;		/* 1000ths */
	(void) strcpy(WindDef.gust_type,   FpaCblank);
	(void) strcpy(WindDef.gust_just,   JustifyLeft);
	(void) strcpy(WindDef.gust_format, "%s");
	WindDef.gust_above      = 500.0;		/* wind units */
	WindDef.gust_size       =   200;		/* 1000ths */
	WindDef.gust_scale      =   100;
	WindDef.x_gust          =     0;		/* 1000ths */
	WindDef.y_gust          =     0;		/* 1000ths */

	/* Set default vector presentation                              */
	/* Note that sizes and x/y locations are in 1000ths of an inch! */
	(void) strcpy(VectorDef.vector_lookup, FpaCblank);
	(void) strcpy(VectorDef.calm_type,     FpaCblank);
	(void) strcpy(VectorDef.calm_just,     JustifyCentre);
	(void) strcpy(VectorDef.calm_format,   "%s");
	VectorDef.calm_size       =  200;		/* 1000ths */
	VectorDef.calm_scale      =  100;
	VectorDef.x_calm          =    0;		/* 1000ths */
	VectorDef.y_calm          =    0;		/* 1000ths */
	(void) strcpy(VectorDef.direction_type,   FpaCblank);
	(void) strcpy(VectorDef.direction_just,   JustifyRight);
	(void) strcpy(VectorDef.direction_format, "%s");
	VectorDef.direction_size  =   200;		/* 1000ths */
	VectorDef.direction_scale =   100;
	VectorDef.x_dir           =     0;		/* 1000ths */
	VectorDef.y_dir           =     0;		/* 1000ths */
	(void) strcpy(VectorDef.speed_type,   FpaCblank);
	(void) strcpy(VectorDef.speed_just,   JustifyLeft);
	(void) strcpy(VectorDef.speed_format, "%s");
	VectorDef.speed_size      =   200;		/* 1000ths */
	VectorDef.speed_scale     =   100;
	VectorDef.x_spd           =     0;		/* 1000ths */
	VectorDef.y_spd           =     0;		/* 1000ths */
	}

void		default_texmet_presentation

	(
	)

	{

	/* Set default values for presentation */
	(void) strcpy(PresDef.name,       "");
	(void) strcpy(PresDef.justified,  JustifyLeft);

	/* Set default value ... for set_attribute_placement() routine */
	(void) strcpy(PresDef.text_size,  "1");

	/* Set default value ... for GRA_display_text() routine */
	(void) strcpy(PresDef.line_space, "-100");

	/* Save the default presentation */
	(void) copy_presentation(&CurPres, &PresDef);

	/* Set default wind presentation */
	(void) strcpy(WindDef.wind_lookup, FpaCblank);
	(void) strcpy(WindDef.calm_type,   FpaCblank);
	(void) strcpy(WindDef.calm_just,   JustifyCentre);
	(void) strcpy(WindDef.calm_format, "%s");
	WindDef.calm_size       =     1;
	WindDef.calm_scale      =   100;
	WindDef.x_calm          =     0;
	WindDef.y_calm          =     0;
	(void) strcpy(WindDef.direction_type,   FpaCblank);
	(void) strcpy(WindDef.direction_just,   JustifyRight);
	(void) strcpy(WindDef.direction_format, "%s");
	WindDef.direction_size  =     1;
	WindDef.direction_scale =   100;
	WindDef.x_dir           =    -1;
	WindDef.y_dir           =     0;
	(void) strcpy(WindDef.speed_type,   FpaCblank);
	(void) strcpy(WindDef.speed_just,   JustifyLeft);
	(void) strcpy(WindDef.speed_format, "%s");
	WindDef.speed_size      =     1;
	WindDef.speed_scale     =   100;
	WindDef.x_spd           =     0;
	WindDef.y_spd           =     0;
	(void) strcpy(WindDef.gust_type,   FpaCblank);
	(void) strcpy(WindDef.gust_just,   JustifyLeft);
	(void) strcpy(WindDef.gust_format, "%s");
	WindDef.gust_above      = 500.0;		/* wind units */
	WindDef.gust_size       =     1;
	WindDef.gust_scale      =   100;
	WindDef.x_gust          =     3;
	WindDef.y_gust          =     0;

	/* Set default vector presentation */
	(void) strcpy(VectorDef.vector_lookup, FpaCblank);
	(void) strcpy(VectorDef.calm_type,     FpaCblank);
	(void) strcpy(VectorDef.calm_just,     JustifyCentre);
	(void) strcpy(VectorDef.calm_format,   "%s");
	VectorDef.calm_size       =     1;
	VectorDef.calm_scale      =   100;
	VectorDef.x_calm          =     0;
	VectorDef.y_calm          =     0;
	(void) strcpy(VectorDef.direction_type,   FpaCblank);
	(void) strcpy(VectorDef.direction_just,   JustifyRight);
	(void) strcpy(VectorDef.direction_format, "%s");
	VectorDef.direction_size  =     1;
	VectorDef.direction_scale =   100;
	VectorDef.x_dir           =    -1;
	VectorDef.y_dir           =     0;
	(void) strcpy(VectorDef.speed_type,   FpaCblank);
	(void) strcpy(VectorDef.speed_just,   JustifyLeft);
	(void) strcpy(VectorDef.speed_format, "%s");
	VectorDef.speed_size      =     1;
	VectorDef.speed_scale     =   100;
	VectorDef.x_spd           =     0;
	VectorDef.y_spd           =     0;
	}

/***********************************************************************
*                                                                      *
*    c o p y _ p r e s e n t a t i o n                                 *
*    a d d _ p r e s e n t a t i o n                                   *
*    g e t _ p r e s e n t a t i o n                                   *
*                                                                      *
***********************************************************************/

/* Storage for named presentations */
static	PRES	*PresList   = NullPtr(PRES *);
static	int		NumPresList =  0;
static	int		CurPresList = -1;

void		copy_presentation

	(
	PRES		*outpres,
	PRES		*inpres
	)

	{

	/* Return immediately if missing input or output presentation */
	if ( IsNull(inpres) || IsNull(outpres) ) return;

	/* Copy the presentation parameters */
	(void) strcpy(outpres->name,           inpres->name);
	(void) strcpy(outpres->line_width,     inpres->line_width);
	(void) strcpy(outpres->line_style,     inpres->line_style);
	(void) strcpy(outpres->outline,        inpres->outline);
	(void) strcpy(outpres->fill,           inpres->fill);
	(void) strcpy(outpres->interior_fill,  inpres->interior_fill);
	(void) strcpy(outpres->sym_fill_name,  inpres->sym_fill_name);
	(void) strcpy(outpres->pattern,        inpres->pattern);
	(void) strcpy(outpres->pattern_width,  inpres->pattern_width);
	(void) strcpy(outpres->pattern_length, inpres->pattern_length);
	(void) strcpy(outpres->font,           inpres->font);
	(void) strcpy(outpres->font_weight,    inpres->font_weight);
	(void) strcpy(outpres->italics,        inpres->italics);
	(void) strcpy(outpres->justified,      inpres->justified);
	(void) strcpy(outpres->text_size,      inpres->text_size);
	(void) strcpy(outpres->char_space,     inpres->char_space);
	(void) strcpy(outpres->word_space,     inpres->word_space);
	(void) strcpy(outpres->line_space,     inpres->line_space);
	outpres->outline_first = inpres->outline_first;
	outpres->num_comp      = inpres->num_comp;
	outpres->comp_pres     = inpres->comp_pres;
	}

PRES		*add_presentation

	(
	STRING		name
	)

	{
	int			ii, np;

	/* Search the list for the named presentation */
	for ( ii=0; ii<NumPresList; ii++ )
		{

		/* Return the named presentation (if found) */
		if ( same(PresList[ii].name, name) ) return &PresList[ii];
		}

	/* Add another named presentation to the list */
	NumPresList++;
	PresList = GETMEM(PresList, PRES, NumPresList);
	np = NumPresList - 1;

	/* Set default presentation and name */
	(void) copy_presentation(&PresList[np], &PresDef);
	(void) strcpy(PresList[np].name, name);

	/* Return the new named presentation */
	return &PresList[np];
	}

PRES		get_presentation

	(
	STRING		name
	)

	{
	int			ii;
	char		err_buf[GPGLong];

	/* Check if the name corresponds to the current presentation */
	if ( CurPresList >= 0 && same(name, PresList[CurPresList].name) )
		return PresList[CurPresList];

	/* Use the presentation corresponding to this name */
	for ( ii=0; ii<NumPresList; ii++ )
		{
		if ( same(PresList[ii].name, name) )
			{
			CurPresList = ii;
			return PresList[CurPresList];
			}
		}

	/* Use current presentation if name not found */
	(void) sprintf(err_buf, "Missing named presentation for: %s", name);
	(void) warn_report(err_buf);
	return CurPres;
	}

/***********************************************************************
*                                                                      *
*    a d d _ c o m p _ p r e s                                         *
*    c o p y _ c o m p _ p r e s                                       *
*    c o m p l e t e _ c o m p _ p r e s                               *
*    f r e e _ c o m p _ p r e s                                       *
*    r e p l a c e _ p r e s e n t a t i o n _ c o m p _ p r e s       *
*    r e s e t _ p r e s e n t a t i o n _ b y _ c o m p _ p r e s     *
*                                                                      *
***********************************************************************/

/* Storage for special presentations */
static	int			NumCompPres = 0;
static	COMP_PRES	*CompPres   = NullPtr(COMP_PRES *);

int			add_comp_pres

	(
	COMP_PRES	**comp_pres,
	int			num_pres
	)

	{
	int			nc;

	/* Return if space is already available */
	if ( num_pres <= NumCompPres ) return NumCompPres;

	/* Add another component presentation buffer */
	nc = NumCompPres++;
	CompPres = GETMEM(CompPres, COMP_PRES, NumCompPres);

	/* Set first component presentation to default component presentation */
	if ( NumCompPres == 1 )
		{
		(void) copy_comp_pres(&CompPres[nc], &NoCompPres);
		}

	/* Set all other component presentations from previous component */
	else
		{
		(void) copy_comp_pres(&CompPres[nc], &CompPres[nc-1]);
		}

	/* Return the number of component presentation buffers */
	if ( NotNull(comp_pres) ) *comp_pres = CompPres;
	return NumCompPres;
	}

void		copy_comp_pres

	(
	COMP_PRES	*outcomp,
	COMP_PRES	*incomp
	)

	{

	/* Return immediately if missing input or output component presentation */
	if ( IsNull(incomp) || IsNull(outcomp) ) return;

	/* Copy the component presentation parameters */
	(void) strcpy(outcomp->line_width, incomp->line_width);
	(void) strcpy(outcomp->line_style, incomp->line_style);
	(void) strcpy(outcomp->outline,    incomp->outline);
	(void) strcpy(outcomp->fill,       incomp->fill);
	}

void		check_comp_pres

	(
	)

	{
	int			nc;
	COMP_PRES	cp, cprev;

	/* Return immediately if no need to complete */
	if ( NumCompPres <= 1 ) return;

	/* Copy the component presentation parameters (if required) */
	for ( nc=1; nc<NumCompPres; nc++ )
		{
		cp    = CompPres[nc];
		cprev = CompPres[nc-1];
		if ( blank(cp.line_width) && !blank(cprev.line_width) )
			(void) strcpy(cp.line_width, cprev.line_width);
		if ( blank(cp.line_style) && !blank(cprev.line_style) )
			(void) strcpy(cp.line_style, cprev.line_style);
		if ( blank(cp.outline) && !blank(cprev.outline) )
			(void) strcpy(cp.outline, cprev.outline);
		if ( blank(cp.fill) && !blank(cprev.fill) )
			(void) strcpy(cp.fill, cprev.fill);
		}
	}

void		free_comp_pres

	(
	)

	{

	/* Return if no component presentation buffers to free */
	if ( NumCompPres <= 0 ) return;

	/* Free space used by component presentation buffers */
	FREEMEM(CompPres);

	/* Reset the number of component presentation buffers */
	NumCompPres = 0;
	}

void		replace_presentation_comp_pres

	(
	PRES		*outpres,
	int			num_comp,
	COMP_PRES	*incomp
	)

	{
	int			nc;
	COMP_PRES	*cp;

	/* Return immediately if missing input component presentations */
	/*  or output presentation                                     */
	if ( num_comp <= 0 || IsNull(outpres) ) return;

	/* Remove the current component presentations */
	FREEMEM(outpres->comp_pres);
	outpres->num_comp = 0;

	/* Copy the component presentation parameters */
	outpres->num_comp  = num_comp;
	outpres->comp_pres = INITMEM(COMP_PRES, num_comp);
	for ( nc=0; nc<num_comp; nc++ )
		{
		cp = &outpres->comp_pres[nc];
		(void) safe_strcpy(cp->line_width, incomp[nc].line_width);
		(void) safe_strcpy(cp->line_style, incomp[nc].line_style);
		(void) safe_strcpy(cp->outline,    incomp[nc].outline);
		(void) safe_strcpy(cp->fill,       incomp[nc].fill);
		}
	}

void		reset_presentation_by_comp_pres

	(
	PRES		*outpres,
	COMP_PRES	*incomp
	)

	{

	/* Return immediately if missing input component presentation */
	/*  or output presentation                                    */
	if ( IsNull(incomp) || IsNull(outpres) ) return;

	/* Reset the component presentation parameters (if not blank) */
	if ( !blank(incomp->line_width) )
		(void) strcpy(outpres->line_width, incomp->line_width);
	if ( !blank(incomp->line_style) )
		(void) strcpy(outpres->line_style, incomp->line_style);
	if ( !blank(incomp->outline) )
		(void) strcpy(outpres->outline,    incomp->outline);
	if ( !blank(incomp->fill) )
		(void) strcpy(outpres->fill,       incomp->fill);
	}

/***********************************************************************
*                                                                      *
*    a d d _ c a t e g o r y _ a t t r i b s                           *
*    f r e e _ c a t e g o r y _ a t t r i b s                         *
*    a d d _ t r a c k _ c a t e g o r y _ a t t r i b s               *
*    f r e e _ t r a c k _ c a t e g o r y _ a t t r i b s             *
*                                                                      *
***********************************************************************/

/* Storage for multiple category attributes */
static	int			NumCatAttribs = 0;
static	CATATTRIB	*CatAttribs   = NullPtr(CATATTRIB *);

int			add_category_attribs

	(
	CATATTRIB	**cat_attrib,
	int			num_in
	)

	{
	int			nc;

	/* Return if space is already available */
	if ( num_in <= NumCatAttribs ) return NumCatAttribs;

	/* Add another category attribute to the list */
	nc = NumCatAttribs++;
	CatAttribs = GETMEM(CatAttribs, CATATTRIB, NumCatAttribs);

	/* Set defaults for category attributes */
	(void) strcpy(CatAttribs[nc].category_attribute, AttribCategory);
	(void) strcpy(CatAttribs[nc].category,           AttribCategoryAll);

	/* Return the number of category attributes in the list */
	if ( NotNull(cat_attrib) ) *cat_attrib = CatAttribs;
	return NumCatAttribs;
	}

void		free_category_attribs

	(
	)

	{

	/* Return if no category attributes to free */
	if ( NumCatAttribs <= 0 ) return;

	/* Free space used by category attributes */
	FREEMEM(CatAttribs);

	/* Reset the number of category attributes */
	NumCatAttribs = 0;
	}

/* Storage for multiple track category attributes */
static	int			NumTrackCatAttribs = 0;
static	CATATTRIB	*TrackCatAttribs   = NullPtr(CATATTRIB *);

int			add_track_category_attribs

	(
	CATATTRIB	**tcat_attrib,
	int			num_in
	)

	{
	int			nc;

	/* Return if space is already available */
	if ( num_in <= NumTrackCatAttribs ) return NumTrackCatAttribs;

	/* Add another track category attribute to the list */
	nc = NumTrackCatAttribs++;
	TrackCatAttribs = GETMEM(TrackCatAttribs, CATATTRIB, NumTrackCatAttribs);

	/* Set defaults for track category attributes */
	(void) strcpy(TrackCatAttribs[nc].category_attribute, AttribCategory);
	(void) strcpy(TrackCatAttribs[nc].category,           AttribCategoryAll);

	/* Return the number of track category attributes in the list */
	if ( NotNull(tcat_attrib) ) *tcat_attrib = TrackCatAttribs;
	return NumTrackCatAttribs;
	}

void		free_track_category_attribs

	(
	)

	{

	/* Return if no track category attributes to free */
	if ( NumTrackCatAttribs <= 0 ) return;

	/* Free space used by track category attributes */
	FREEMEM(TrackCatAttribs);

	/* Reset the number of track category attributes */
	NumTrackCatAttribs = 0;
	}

/***********************************************************************
*                                                                      *
*    a d d _ t i m e _ t y p e s                                       *
*    f r e e _ t i m e _ t y p e s                                     *
*                                                                      *
***********************************************************************/

/* Storage for multiple time parameters */
static	int			NumTTypFmts = 0;
static	TTYPFMT		*TTypFmts   = NullPtr(TTYPFMT *);

int			add_time_types

	(
	TTYPFMT		**type_fmt,
	int			num_in
	)

	{
	int			nt;

	/* Return if space is already available */
	if ( num_in <= NumTTypFmts ) return NumTTypFmts;

	/* Add another set of time parameters to the list */
	nt = NumTTypFmts++;
	TTypFmts = GETMEM(TTypFmts, TTYPFMT, NumTTypFmts);

	/* Set defaults for time parameters */
	(void) strcpy(TTypFmts[nt].time_type,   FpaCblank);
	(void) strcpy(TTypFmts[nt].zone_type,   FpaCblank);
	(void) strcpy(TTypFmts[nt].time_zone,   FpaCblank);
	(void) strcpy(TTypFmts[nt].language,    FpaCblank);
	(void) strcpy(TTypFmts[nt].time_format, FpaCblank);

	/* Return the number of time parameters in the list */
	if ( NotNull(type_fmt) ) *type_fmt = TTypFmts;
	return NumTTypFmts;
	}

void		free_time_types

	(
	)

	{

	/* Return if no time parameters to free */
	if ( NumTTypFmts <= 0 ) return;

	/* Free space used by time parameters */
	FREEMEM(TTypFmts);

	/* Reset the number of time parameters */
	NumTTypFmts = 0;
	}

/***********************************************************************
*                                                                      *
*    r e p l a c e _ d e f a u l t _ a t t r i b u t e                 *
*    m a g i c _ i s _ a t t r i b u t e                               *
*    m a g i c _ g e t _ a t t r i b u t e                             *
*                                                                      *
***********************************************************************/

STRING		replace_default_attribute

	(
	int			fkind,		/* Field type */
	STRING		name		/* Default attribute name */
	)

	{

	/* Return "default" attributes (based on type of field) */
	if ( same_ic(name, AttribGPGenDefault) )
		{
		switch ( fkind )
			{

			/* Return evaluation for continuous or vector fields */
			case FpaC_CONTINUOUS:
			case FpaC_VECTOR:
				return AttribEvalSpval;

			/* Return label for discrete fields */
			case FpaC_DISCRETE:
				return AttribAutolabel;

			/* Return category for line or scattered or link chain fields */
			case FpaC_LINE:
			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
				return AttribCategory;

			/* Return blank for other field types */
			default:
				return FpaCblank;
			}
		}

	/* Return non "default" attributes unchanged */
	return name;
	}

LOGICAL		magic_is_attribute

	(
	STRING		name
	)

	{

	/* Check for "magic" attributes */
	if      ( same_ic(name, AttribGPGenIdent) )           return TRUE;
	else if ( same_ic(name, AttribGPGenLabel) )           return TRUE;
	else if ( same_ic(name, AttribGPGenFeatureAttrib) )   return TRUE;
	else if ( same_ic(name, AttribGPGenLat) )             return TRUE;
	else if ( same_ic(name, AttribGPGenLatDDMM) )         return TRUE;
	else if ( same_ic(name, AttribGPGenLon) )             return TRUE;
	else if ( same_ic(name, AttribGPGenLonDDMM) )         return TRUE;
	else if ( same_ic(name, AttribGPGenProximity) )       return TRUE;
	else if ( same_ic(name, AttribGPGenNegProximity) )    return TRUE;
	else if ( same_ic(name, AttribGPGenBearing) )         return TRUE;
	else if ( same_ic(name, AttribGPGenLineDirTo) )       return TRUE;
	else if ( same_ic(name, AttribGPGenLineDirFrom) )     return TRUE;
	else if ( same_ic(name, AttribGPGenLineLength) )      return TRUE;
	else if ( same_ic(name, AttribGPGenLchainDir) )       return TRUE;
	else if ( same_ic(name, AttribGPGenLchainSpd) )       return TRUE;
	else if ( same_ic(name, AttribGPGenLchainVector) )    return TRUE;
	else if ( same_ic(name, AttribGPGenLchainLength) )    return TRUE;
	else if ( same_ic(name, AttribGPGenXsectDir) )        return TRUE;
	else if ( same_ic(name, AttribGPGenXsectSpd) )        return TRUE;
	else if ( same_ic(name, AttribGPGenXsectVector) )     return TRUE;
	else if ( same_ic(name, AttribGPGenXsectLength) )     return TRUE;
	else if ( same_ic(name, AttribGPGenProgTime) )        return TRUE;
	else if ( same_ic(name, AttribGPGenProgTimeHours) )   return TRUE;
	else if ( same_ic(name, AttribGPGenProgTimeMinutes) ) return TRUE;
	else if ( same_ic(name, AttribGPGenGMTTime) )         return TRUE;
	else if ( same_ic(name, AttribGPGenUTCTime) )         return TRUE;
	else if ( same_ic(name, AttribGPGenLocalTime) )       return TRUE;
	else if ( same_ic(name, AttribGPGenT0Time) )          return TRUE;
	else if ( same_ic(name, AttribGPGenCreationTime) )    return TRUE;

	/* Return FALSE if not found */
	return FALSE;
	}

STRING		magic_get_attribute

	(
	STRING		name,				/* Name of "magic" attribute */
	STRING		ident,				/* Current identifier */
	STRING		vtime,				/* Current valid time */
	STRING		time_zone,			/* Current time zone (used in strftime) */
	STRING		language,			/* Current language (used in strftime) */
	STRING		loclab,				/* Current label */
	POINT		pos,				/* Current position */
	float		fdist,				/* Distance to feature */
	float		fbear,				/* Bearing to feature */
	float		flen,				/* Length of current line */
	float		xdir,				/* Direction at current segment (deg True) */
	float		xspd,				/* Speed at current segment (m/s) */
	STRING		units,				/* Units for parameter display */
	STRING		format,				/* Format of "magic" attribute */
	STRING		conversion_format	/* Conversion format of "magic" attribute */
	)

	{
	LOGICAL		status;
	int			iangle, iang_deg, iang_min, pmin, year, jday, hour, minute;
	float		clon, flat, flon;
	double		dval;
	STRING		vt, wval;
	WIND_VAL	wv;
	time_t		itime;
	struct tm	*tm;
	char		env_tzone[GPGMedium], env_lang[GPGMedium];
	char		tbuf[GPGLong], err_buf[GPGLong];

	/* Static storage for return parameter */
	static	char	attribute[GPGLong];

	/* Static storage for environment variables */
	static	char	EnvTzone[GPGMedium], EnvLang[GPGLong];

	/* Initialize return parameter */
	(void) strcpy(attribute, FpaCblank);

	/* Check for proper format */
	if ( !same(format, FormatDirect)
			&& !same(format, FormatSymbol)
			&& !same(format, FormatText)
			&& !same(format, FormatWindBarb)
			&& !same(format, FormatWindText)
			&& !same(format, FormatWindSymbol)
			&& !same(format, FormatVectorText)
			&& !same(format, FormatVectorSymbol)
			&& !same(format, FormatNone) )
		{
		(void) sprintf(err_buf, "Incorrect format ... %s  for attribute ... %s",
				format, name);
		(void) error_report(err_buf);
		}

	/* Reset environment variables for time zone or language (for strftime) */
	if ( same_ic(name, AttribGPGenGMTTime)
			|| same_ic(name, AttribGPGenUTCTime)
			|| same_ic(name, AttribGPGenLocalTime)
			|| same_ic(name, AttribGPGenT0Time)
			|| same_ic(name, AttribGPGenCreationTime) )
		{

		/* Save current environment variables for time zone and language       */
		/* Note that once the environment variable has been stored with a call */
		/*  to putenv(), any change to the variable changes the environment!   */
		/* Note that changing these variables changes the environment! */
		(void) safe_strcpy(env_tzone, getenv("TZ"));
		(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
		(void) putenv(EnvTzone);
		(void) safe_strcpy(env_lang,  getenv("LANG"));
		(void) sprintf(EnvLang,  "LANG=%s", env_lang);
		(void) putenv(EnvLang);
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Original  Timezone ... %s  Language ... %s\n",
					env_tzone, env_lang);
			}

		/* Reset environment variables for time zone and language (if required) */
		if ( !blank(time_zone) ) (void) sprintf(EnvTzone, "TZ=%s",   time_zone);
		if ( !blank(language) )  (void) sprintf(EnvLang,  "LANG=%s", language);

		/* Set environment for call to strftime() */
		(void) set_locale_from_environment(LC_TIME);
		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Temporary Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}
		}

	/* Return "magic" identifier attribute */
	if ( same_ic(name, AttribGPGenIdent) )
		{

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strcpy(attribute, ident);
		else
			(void) sprintf(attribute, conversion_format, ident);

		/* Return the formatted identifier */
		return attribute;
		}

	/* Return "magic" label attribute */
	else if ( same_ic(name, AttribGPGenLabel) )
		{

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strcpy(attribute, loclab);
		else
			(void) sprintf(attribute, conversion_format, loclab);

		/* Return the formatted identifier */
		return attribute;
		}

	/* Return "magic" feature attribute (from looping) */
	else if ( same_ic(name, AttribGPGenFeatureAttrib) )
		{

		/* Apply the conversion format (if required) */
		if ( blank(CurAttribute) )
			(void) strcpy(attribute, FpaCblank);
		else if ( blank(conversion_format) )
			(void) strcpy(attribute, CurAttribute);
		else
			(void) sprintf(attribute, conversion_format, CurAttribute);

		/* Return the formatted identifier */
		return attribute;
		}

	/* Return "magic" latitude attribute */
	else if ( same_ic(name, AttribGPGenLat) )
		{

		/* Determine the latitude */
		(void) pos_to_ll(&BaseMap, pos, &flat, NullFloat);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			{
			if ( flat >= 0.0 ) (void) sprintf(attribute, "%.0fN",  flat);
			else               (void) sprintf(attribute, "%.0fS", -flat);
			}
		else
			{
			if ( flat >= 0.0 )
				{
				(void) sprintf(attribute, conversion_format,  flat);
				(void) strcat(attribute, "N");
				}
			else
				{
				(void) sprintf(attribute, conversion_format, -flat);
				(void) strcat(attribute, "S");
				}
			}

		/* Return the formatted latitude */
		return attribute;
		}

	/* Return "magic" latitude attribute (degrees and minutes) */
	else if ( same_ic(name, AttribGPGenLatDDMM) )
		{

		/* Determine the latitude */
		(void) pos_to_ll(&BaseMap, pos, &flat, NullFloat);
		iangle   = dddmm(flat);
		iang_deg = abs(iangle) / 100;
		iang_min = abs(iangle) - (iang_deg * 100);

		/* Format the latitude */
		if ( iangle >= 0.0 )
			(void) sprintf(tbuf, "%d:%.2dN", iang_deg, iang_min);
		else
			(void) sprintf(tbuf, "%d:%.2dS", iang_deg, iang_min);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strcpy(attribute, tbuf);
		else
			(void) sprintf(attribute, conversion_format, tbuf);

		/* Return the formatted latitude */
		return attribute;
		}

	/* Return "magic" longitude attribute */
	else if ( same_ic(name, AttribGPGenLon) )
		{

		/* Determine the longitude */
		(void) pos_to_ll(&BaseMap, pos, NullFloat, &flon);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			{
			if ( flon >= 0.0 ) (void) sprintf(attribute, "%.0fE",  flon);
			else               (void) sprintf(attribute, "%.0fW", -flon);
			}
		else
			{
			if ( flon >= 0.0 )
				{
				(void) sprintf(attribute, conversion_format,  flon);
				(void) strcat(attribute, "E");
				}
			else
				{
				(void) sprintf(attribute, conversion_format, -flon);
				(void) strcat(attribute, "W");
				}
			}

		/* Return the formatted longitude */
		return attribute;
		}

	/* Return "magic" longitude attribute (degrees and minutes) */
	else if ( same_ic(name, AttribGPGenLonDDMM) )
		{

		/* Determine the longitude */
		(void) pos_to_ll(&BaseMap, pos, NullFloat, &flon);
		iangle   = dddmm(flon);
		iang_deg = abs(iangle) / 100;
		iang_min = abs(iangle) - (iang_deg * 100);

		/* Format the longitude */
		if ( iangle >= 0.0 )
			(void) sprintf(tbuf, "%d:%.2dE", iang_deg, iang_min);
		else
			(void) sprintf(tbuf, "%d:%.2dW", iang_deg, iang_min);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strcpy(attribute, tbuf);
		else
			(void) sprintf(attribute, conversion_format, tbuf);

		/* Return the formatted longitude */
		return attribute;
		}

	/* Return "magic" proximity attribute (distance to feature) */
	else if ( same_ic(name, AttribGPGenProximity) )
		{

		/* Convert proximity to units for display */
		if ( !convert_value(ProximityUnitsKm, (double) fdist, units, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect proximity units: %s", units);
			(void) error_report(err_buf);
			}

		/* Format the distance to the feature */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.0f", dval);
		else
			(void) sprintf(attribute, conversion_format, dval);

		/* Return the formatted distance */
		return attribute;
		}

	/* Return "magic" proximity attribute (negative distance to feature) */
	else if ( same_ic(name, AttribGPGenNegProximity) )
		{

		/* Convert proximity to units for display */
		if ( !convert_value(ProximityUnitsKm, (double) fdist, units, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect proximity units: %s", units);
			(void) error_report(err_buf);
			}

		/* Format the distance to the feature */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.0f", -dval);
		else
			(void) sprintf(attribute, conversion_format, -dval);

		/* Return the formatted distance */
		return attribute;
		}

	/* Return "magic" bearing attribute (direction to feature) */
	else if ( same_ic(name, AttribGPGenBearing) )
		{

		/* Return if problem with direction to feature */
		if ( fbear == DefBearing ) return attribute;

		/* Format the direction to the feature */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.0f", fbear);
		else
			(void) sprintf(attribute, conversion_format, fbear);

		/* Return the formatted direction */
		return attribute;
		}

	/* Return "magic" line direction attribute (towards end of line) */
	/*  for line or link chain or cross section                      */
	else if ( same_ic(name, AttribGPGenLineDirTo)
				|| same_ic(name, AttribGPGenLchainDir)
				|| same_ic(name, AttribGPGenXsectDir) )
		{

		/* Return if problem with direction at line segment */
		if ( xdir == DefSegDir ) return attribute;

		/* Format the direction in degrees */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.2f", xdir);
		else
			(void) sprintf(attribute, conversion_format, xdir);

		/* Return the formatted line direction */
		return attribute;
		}

	/* Return "magic" line direction attribute (from end of line) */
	else if ( same_ic(name, AttribGPGenLineDirFrom) )
		{

		/* Return if problem with direction at line segment */
		if ( xdir == DefSegDir ) return attribute;

		/* Reverse the line direction */
		xdir = (xdir > 180.0)? xdir - 180.0: xdir + 180.0;

		/* Format the line direction in degrees */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.2f", xdir);
		else
			(void) sprintf(attribute, conversion_format, xdir);

		/* Return the formatted line direction */
		return attribute;
		}

	/* Return "magic" line length attribute     */
	/*  for line or link chain or cross section */
	else if ( same_ic(name, AttribGPGenLineLength)
				|| same_ic(name, AttribGPGenLchainLength)
				|| same_ic(name, AttribGPGenXsectLength) )
		{

		/* Return if problem with line length */
		if ( flen == DefLineLen ) return attribute;

		/* Convert line length to units for display */
		if ( !convert_value(LineLengthUnitsKm, (double) flen, units, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect line length units: %s", units);
			(void) error_report(err_buf);
			}

		/* Format the length of the line or link chain or cross section */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.0f", dval);
		else
			(void) sprintf(attribute, conversion_format, dval);

		/* Return the formatted line length */
		return attribute;
		}

	/* Return "magic" link chain or cross section speed attribute */
	/*  for link chain or cross section                           */
	else if ( same_ic(name, AttribGPGenLchainSpd)
				|| same_ic(name, AttribGPGenXsectSpd) )
		{

		/* Return if problem with link chain or cross section speed */
		if ( xspd == DefSegSpd ) return attribute;

		/* Convert speed to units for display */
		if ( !convert_value(FeatureSpeedUnitsMS, (double) xspd, units, &dval) )
			{
			(void) sprintf(err_buf,
					"Incorrect feature speed units: %s", units);
			(void) error_report(err_buf);
			}

		/* Format the link chain or cross section speed */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%.0f", dval);
		else
			(void) sprintf(attribute, conversion_format, dval);

		/* Return the formatted link chain or cross section speed */
		return attribute;
		}

	/* Return "magic" link chain or cross section vector attributes */
	else if ( same_ic(name, AttribGPGenLchainVector)
				|| same_ic(name, AttribGPGenXsectVector) )
		{

		/* Return if problem with link chain or cross section speed */
		if ( xdir == DefSegDir || xspd == DefSegSpd ) return attribute;

		/* Convert speed to knots for now */
		if ( !convert_value(FeatureSpeedUnitsMS, (double) xspd, Knots, &dval) )
			{
			(void) sprintf(err_buf,
					"Missing feature speed units: %s", FeatureSpeedUnitsMS);
			(void) error_report(err_buf);
			}

		/* Build wind value string from feature direction and speed */
		wv.dir   = xdir;
		wv.dunit = DegreesTrue;
		wv.speed = (float) dval;
		wv.gust  = 0.0;
		wv.sunit = Knots;
		wval     = build_wind_value_string(&wv);
		(void) safe_strcpy(attribute, wval);
		FREEMEM(wval);
		return attribute;
		}

	/* Return "magic" prog time attribute */
	else if ( same_ic(name, AttribGPGenProgTime) )
		{

		/* Determine the prog time from the current run and valid times */
		pmin = calc_prog_time_minutes(T0stamp, vtime, &status);
		if ( !status )
			{
			(void) sprintf(err_buf,
					"Cannot determine prog time from run time ... %s  and valid time ... %s",
					T0stamp, vtime);
			(void) error_report(err_buf);
			}

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "T%s", hour_minute_string(0, pmin));
		else
			(void) sprintf(attribute, conversion_format,
											hour_minute_string(0, pmin));

		/* Return the formatted prog time */
		return attribute;
		}

	/* Return "magic" prog time (as hours) attribute */
	else if ( same_ic(name, AttribGPGenProgTimeHours) )
		{

		/* Determine the prog time from the current run and valid times */
		pmin = calc_prog_time_minutes(T0stamp, vtime, &status);
		if ( !status )
			{
			(void) sprintf(err_buf,
					"Cannot determine prog time from run time ... %s  and valid time ... %s",
					T0stamp, vtime);
			(void) error_report(err_buf);
			}

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%d", pmin/60);
		else
			(void) sprintf(attribute, conversion_format, pmin/60);

		/* Return the formatted prog time */
		return attribute;
		}

	/* Return "magic" prog time (as minutes) attribute */
	else if ( same_ic(name, AttribGPGenProgTimeMinutes) )
		{

		/* Determine the prog time from the current run and valid times */
		pmin = calc_prog_time_minutes(T0stamp, vtime, &status);
		if ( !status )
			{
			(void) sprintf(err_buf,
					"Cannot determine prog time from run time ... %s  and valid time ... %s",
					T0stamp, vtime);
			(void) error_report(err_buf);
			}

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) sprintf(attribute, "%d", pmin);
		else
			(void) sprintf(attribute, conversion_format, pmin);

		/* Return the formatted prog time */
		return attribute;
		}

	/* Return "magic" GMT or UTC time attribute */
	else if ( same_ic(name, AttribGPGenGMTTime) || same_ic(name, AttribGPGenUTCTime) )
		{

		/* Set centre longitude from current map projection */
		(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

		if ( Verbose )
			{
			(void) fprintf(stdout, "   Convert time from ... %s at %.2f\n",
					vtime, clon);
			}

		/* Convert local times to GMT (if required) */
		vt = local_to_gmt(vtime, clon);
		if ( IsNull(vt) )
			{
			(void) sprintf(err_buf, "Error reading valid time ... %s", vtime);
			(void) error_report(err_buf);
			}
		if ( Verbose )
			{
			(void) fprintf(stdout, "   GMT valid time ... %s\n", vt);
			}

		/* Read the current valid time */
		(void) parse_tstamp(vt, &year, &jday, &hour, &minute,
											NullLogicalPtr, NullLogicalPtr);

		/* Set the GMT time */
		itime = (time_t) encode_clock(year, jday, hour, minute, 0);
		tm    = gmtime(&itime);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strftime(attribute, GPGLong, "%Y:%j:%H", tm);
		else
			(void) strftime(attribute, GPGLong, conversion_format, tm);

		/* Reset current environment variables for time zone and language */
		(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
		(void) sprintf(EnvLang,  "LANG=%s", env_lang);
		(void) reset_locale();

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Reset     Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}

		/* Return the formatted GMT time */
		return attribute;
		}

	/* Return "magic" local time attribute */
	else if ( same_ic(name, AttribGPGenLocalTime) )
		{

		/* Set centre longitude from current map projection */
		(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

		if ( Verbose )
			{
			(void) fprintf(stdout, "   Convert time from ... %s at %.2f\n",
					vtime, clon);
			}

		/* Convert local times to GMT (if required) */
		vt = local_to_gmt(vtime, clon);
		if ( IsNull(vt) )
			{
			(void) sprintf(err_buf, "Error reading valid time ... %s", vtime);
			(void) error_report(err_buf);
			}
		if ( Verbose )
			{
			(void) fprintf(stdout, "   GMT valid time ... %s\n", vt);
			}

		/* Read the current valid time */
		(void) parse_tstamp(vt, &year, &jday, &hour, &minute,
											NullLogicalPtr, NullLogicalPtr);

		/* Set the local time */
		itime = (time_t) encode_clock(year, jday, hour, minute, 0);
		tm    = localtime(&itime);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strftime(attribute, GPGLong, "%Y:%j:%HL", tm);
		else
			(void) strftime(attribute, GPGLong, conversion_format, tm);

		/* Reset current environment variables for time zone and language */
		(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
		(void) sprintf(EnvLang,  "LANG=%s", env_lang);
		(void) reset_locale();

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Reset     Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}

		/* Return the formatted local time */
		return attribute;
		}

	/* Return "magic" T0 time attribute */
	else if ( same_ic(name, AttribGPGenT0Time) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "   GMT T0 time ... %s\n", T0stamp);
			}

		/* Read the current T0 time */
		(void) parse_tstamp(T0stamp, &year, &jday, &hour, &minute,
											NullLogicalPtr, NullLogicalPtr);

		/* Set the GMT time */
		itime = (time_t) encode_clock(year, jday, hour, minute, 0);
		tm    = gmtime(&itime);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strftime(attribute, GPGLong, "%Y:%j:%H", tm);
		else
			(void) strftime(attribute, GPGLong, conversion_format, tm);

		/* Reset current environment variables for time zone and language */
		(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
		(void) sprintf(EnvLang,  "LANG=%s", env_lang);
		(void) reset_locale();

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Reset     Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}

		/* Return the formatted T0 time */
		return attribute;
		}

	/* Return "magic" creation time attribute */
	else if ( same_ic(name, AttribGPGenCreationTime) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "   GMT Creation time ... %s\n", TCstamp);
			}

		/* Read the current creation time */
		(void) parse_tstamp(TCstamp, &year, &jday, &hour, &minute,
											NullLogicalPtr, NullLogicalPtr);

		/* Set the GMT time */
		itime = (time_t) encode_clock(year, jday, hour, minute, 0);
		tm    = gmtime(&itime);

		/* Apply the conversion format (if required) */
		if ( blank(conversion_format) )
			(void) strftime(attribute, GPGLong, "%Y:%j:%H", tm);
		else
			(void) strftime(attribute, GPGLong, conversion_format, tm);

		/* Reset current environment variables for time zone and language */
		(void) sprintf(EnvTzone, "TZ=%s",   env_tzone);
		(void) sprintf(EnvLang,  "LANG=%s", env_lang);
		(void) reset_locale();

		if ( Verbose )
			{
			(void) fprintf(stdout,
					" Reset     Timezone ... %s  Language ... %s\n",
					getenv("TZ"), getenv("LANG"));
			}

		/* Return the formatted creation time */
		return attribute;
		}

	/* Error return if not a "magic" attribute */
	return NullString;
	}

/***********************************************************************
*                                                                      *
*    c h e c k _ l a b e l _ a t t r i b u t e s                       *
*    c h e c k _ v a l u e _ a t t r i b u t e s                       *
*    c h e c k _ v e c t o r _ a t t r i b u t e s                     *
*    m a t c h _ c a t e g o r y _ a t t r i b u t e s                 *
*                                                                      *
***********************************************************************/

LOGICAL		check_label_attributes

	(
	CAL				fld_attrib,		/* Sampled attributes */
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib		/* Number of attributes */
	)

	{
	int				nn;
	LOGICAL			att_match;

	/* Check for attribute(s) */
	att_match = TRUE;
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( !magic_is_attribute(attribs[nn].name)
				&& !CAL_has_attribute(fld_attrib, attribs[nn].name) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "   No attribute ... %s\n",
						attribs[nn].name);
				}
			att_match = FALSE;
			}
		}

	/* Return the result of checking */
	return att_match;
	}

LOGICAL		check_value_attributes

	(
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib		/* Number of attributes */
	)

	{
	int				nn;
	LOGICAL			att_match;

	/* Check for attribute(s) */
	att_match = TRUE;
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( !magic_is_attribute(attribs[nn].name)
				&& !same_ic(attribs[nn].name, AttribEvalSpval) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "   No attribute ... %s\n",
						attribs[nn].name);
				}
			att_match = FALSE;
			}
		}

	/* Return the result of checking */
	return att_match;
	}

LOGICAL		check_vector_attributes

	(
	ATTRIB_DISPLAY	*attribs,		/* Attributes to display */
	int				num_attrib		/* Number of attributes */
	)

	{
	int				nn;
	LOGICAL			att_match;

	/* Check for attribute(s) */
	att_match = TRUE;
	for ( nn=0; nn<num_attrib; nn++ )
		{
		if ( !magic_is_attribute(attribs[nn].name)
				&& !same_ic(attribs[nn].name, AttribEvalSpval)
				&& !same_ic(attribs[nn].name, AttribEvalVector) )
			{
			if ( Verbose )
				{
				(void) fprintf(stdout, "   No attribute ... %s\n",
						attribs[nn].name);
				}
			att_match = FALSE;
			}
		}

	/* Return the result of checking */
	return att_match;
	}

LOGICAL		match_category_attributes

	(
	CAL			cal,			/* Sampled attributes */
	STRING		cat_cascade,	/* And/Or for multiple CATATTRIB structs */
	CATATTRIB	*cat_attrib,	/* Structure containing categories */
	int			num_catatt		/* Number of categories */
	)

	{
	int			nn;
	LOGICAL		cat_match, check_missing, do_not_match, in_list;
	STRING		value, next;
	char		tbuf[GPGMedium];

	/* Return immediately if no categories to check */
	if ( num_catatt < 1 || IsNull(cat_attrib) ) return TRUE;

	/* Check that each "category_attribute" can be found */
	if ( same_ic(cat_cascade, CatCascadeAnd) )
		{
		cat_match = TRUE;
		for ( nn=0; nn<num_catatt; nn++ )
			{
			if ( !CAL_has_attribute(cal, cat_attrib[nn].category_attribute) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   No category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				cat_match = FALSE;
				}
			}
		if ( !cat_match ) return FALSE;
		}

	/* Or ... check that one "category_attribute" can be found */
	else if ( same_ic(cat_cascade, CatCascadeOr) )
		{
		cat_match = FALSE;
		for ( nn=0; nn<num_catatt; nn++ )
			{
			if ( CAL_has_attribute(cal, cat_attrib[nn].category_attribute) )
				{
				cat_match = TRUE;
				break;
				}
			else
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   No category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				}
			}
		if ( !cat_match ) return FALSE;
		}

	/* Now try to match each "category_attribute"             */
	/* Note that "category" may contain more than one member! */
	if ( same_ic(cat_cascade, CatCascadeAnd) )
		{
		cat_match = TRUE;
		for ( nn=0; nn<num_catatt; nn++ )
			{

			/* Check each member of "category" for special case of "Missing" */
			/* This avoids errors if the "category_attribute" value is blank */
			(void) strcpy(tbuf, cat_attrib[nn].category);
			check_missing = FALSE;
			while ( !blank( next = string_arg(tbuf) ) )
				{
				if ( same_ic(next, AttribGPGenMissing) )
					{
					check_missing = TRUE;
					break;
					}
				}

			/* Check each member of "category" for special case of "DoNotMatch" */
			/* This will reverse the logic to check that the value in           */
			/*  "category_attribute" does NOT match any value in "category"     */
			(void) strcpy(tbuf, cat_attrib[nn].category);
			do_not_match = FALSE;
			while ( !blank( next = string_arg(tbuf) ) )
				{
				if ( same_ic(next, AttribGPGenDoNotMatch) )
					{
					do_not_match = TRUE;
					break;
					}
				}

			/* Get the value of the "category_attribute" */
			value = CAL_get_attribute(cal, cat_attrib[nn].category_attribute);

			/* Special case searching for missing "category_attribute" */
			if ( check_missing && blank(value) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   No value for");
					(void) fprintf(stdout, " category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				continue;
				}

			/* Otherwise error if the "category_attribute" is blank */
			else if ( blank(value) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   No value for");
					(void) fprintf(stdout, " category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				cat_match = FALSE;
				break;
				}

			/* This "category" matches everything! */
			if ( same_ic(cat_attrib[nn].category, AttribCategoryAll)
					|| same_ic(cat_attrib[nn].category, AttribGPGenAll) )
				{

				/* Special case for NOT matching everything */
				if ( do_not_match )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout, "   Skipping all values");
						(void) fprintf(stdout, "  for category_attribute ... %s",
								cat_attrib[nn].category_attribute);
						(void) fprintf(stdout, "  found in category ... %s\n",
								cat_attrib[nn].category);
						}
					cat_match = FALSE;
					break;
					}

				/* Match found for this "category_attribute" */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout, "   Matching all values");
						(void) fprintf(stdout, "  for category_attribute ... %s",
								cat_attrib[nn].category_attribute);
						(void) fprintf(stdout, "  found in category ... %s\n",
								cat_attrib[nn].category);
						}
					continue;
					}
				}

			/* Check each member of "category" for a matching value */
			(void) strcpy(tbuf, cat_attrib[nn].category);
			in_list = FALSE;
			while ( !blank( next = string_arg(tbuf) ) )
				{
				if ( same(next, value) )
					{
					in_list = TRUE;
					break;
					}
				}

			/* Matching value found in "category"             */
			/*  and not checking for values that do not match */
			if ( in_list && !do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Matching value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				continue;
				}

			/* Error if matching value found in "category" */
			/*  and checking for values that do not match  */
			else if ( in_list && do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Skipping value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s",
							cat_attrib[nn].category_attribute);
					(void) fprintf(stdout, "  found in category ... %s\n",
							cat_attrib[nn].category);
					}
				cat_match = FALSE;
				break;
				}

			/* Error if there is no matching value in "category" */
			/*  and not checking for values that do not match    */
			else if ( !in_list && !do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s",
							cat_attrib[nn].category_attribute);
					(void) fprintf(stdout, "  does not match category ... %s\n",
							cat_attrib[nn].category);
					}
				cat_match = FALSE;
				break;
				}

			/* No matching value found in "category"       */
			/*  and checking for values that do not match  */
			else if ( !in_list && do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Accepting value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s",
							cat_attrib[nn].category_attribute);
					(void) fprintf(stdout, "  not found in category ... %s\n",
							cat_attrib[nn].category);
					}
				continue;
				}
			}

		/* Return the result of matching */
		return cat_match;
		}

	/* Or ... try to match at least one "category_attribute"  */
	/* Note that "category" may contain more than one member! */
	else if ( same_ic(cat_cascade, CatCascadeOr) )
		{
		cat_match = FALSE;
		for ( nn=0; nn<num_catatt; nn++ )
			{

			/* Check each member of "category" for special case of "missing" */
			/* This avoids errors if the "category_attribute" value is blank */
			(void) strcpy(tbuf, cat_attrib[nn].category);
			check_missing = FALSE;
			while ( !blank( next = string_arg(tbuf) ) )
				{
				if ( same_ic(next, AttribGPGenMissing) )
					{
					check_missing = TRUE;
					break;
					}
				}

			/* Check each member of "category" for special case of "DoNotMatch" */
			/* This will reverse the logic to check that the value in           */
			/*  "category_attribute" does NOT match any value in "category"     */
			(void) strcpy(tbuf, cat_attrib[nn].category);
			do_not_match = FALSE;
			while ( !blank( next = string_arg(tbuf) ) )
				{
				if ( same_ic(next, AttribGPGenDoNotMatch) )
					{
					do_not_match = TRUE;
					break;
					}
				}

			/* Get the value of the "category_attribute" */
			value = CAL_get_attribute(cal, cat_attrib[nn].category_attribute);

			/* Special case searching for missing "category_attribute" */
			if ( check_missing && blank(value) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   No value for");
					(void) fprintf(stdout, " category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				continue;
				}

			/* Otherwise warning if the "category_attribute" is missing */
			else if ( blank(value) )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   No value for");
					(void) fprintf(stdout, " category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				continue;
				}

			/* This "category" matches everything! */
			if ( same_ic(cat_attrib[nn].category, AttribCategoryAll)
					|| same_ic(cat_attrib[nn].category, AttribGPGenAll) )
				{

				/* Special case for NOT matching everything */
				if ( do_not_match )
					{
					if ( Verbose )
						{
						(void) fprintf(stdout, "   Skipping all values");
						(void) fprintf(stdout, "  for category_attribute ... %s",
								cat_attrib[nn].category_attribute);
						(void) fprintf(stdout, "  found in category ... %s\n",
								cat_attrib[nn].category);
						}
					continue;
					}

				/* Match found for this "category_attribute" */
				else
					{
					if ( Verbose )
						{
						(void) fprintf(stdout, "   Matching all values");
						(void) fprintf(stdout, "  for category_attribute ... %s",
								cat_attrib[nn].category_attribute);
						(void) fprintf(stdout, "  found in category ... %s\n",
								cat_attrib[nn].category);
						}
					cat_match = TRUE;
					break;
					}
				}

			/* Check each member of "category" for a matching value */
			(void) strcpy(tbuf, cat_attrib[nn].category);
			in_list = FALSE;
			while ( !blank( next = string_arg(tbuf) ) )
				{
				if ( same(next, value) )
					{
					in_list = TRUE;
					break;
					}
				}

			/* Matching value found in "category"             */
			/*  and not checking for values that do not match */
			if ( in_list && !do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Matching value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s\n",
							cat_attrib[nn].category_attribute);
					}
				cat_match = TRUE;
				break;
				}

			/* Warning if matching value found in "category" */
			/*  and checking for values that do not match    */
			else if ( in_list && do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Skipping value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s",
							cat_attrib[nn].category_attribute);
					(void) fprintf(stdout, "  found in category ... %s\n",
							cat_attrib[nn].category);
					}
				continue;
				}

			/* Warning if there is no matching value in "category" */
			/*  and not checking for values that do not match      */
			else if ( !in_list && !do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s",
							cat_attrib[nn].category_attribute);
					(void) fprintf(stdout, "  does not match category ... %s\n",
							cat_attrib[nn].category);
					}
				continue;
				}

			/* No matching value found in "category"       */
			/*  and checking for values that do not match  */
			else if ( !in_list && do_not_match )
				{
				if ( Verbose )
					{
					(void) fprintf(stdout, "   Accepting value ... %s", value);
					(void) fprintf(stdout, "  for category_attribute ... %s",
							cat_attrib[nn].category_attribute);
					(void) fprintf(stdout, "  not found in category ... %s\n",
							cat_attrib[nn].category);
					}
				cat_match = TRUE;
				break;
				}
			}

		/* Return the result of matching */
		return cat_match;
		}

	/* Error if unrecognized category cascade parameter */
	if ( Verbose )
		{
		(void) fprintf(stdout, "   Unrecognized category_cascade ... %s\n",
				cat_cascade);
		}
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    i n i t i a l i z e _ c o n t o u r _ p r e s e n t a t i o n     *
*    a d d _ c o n t o u r _ v a l u e s                               *
*    a d d _ c o n t o u r _ r a n g e s                               *
*    a d d _ c o n t o u r _ p r e s e n t a t i o n                   *
*    g e t _ c o n t o u r _ p r e s e n t a t i o n                   *
*                                                                      *
***********************************************************************/

/* Structure for holding parameters for contour presentations */
typedef struct
	{
	LOGICAL	old_version;
	RANGE	range;
	STRING	units;
	PRES	presentation;
	} CON_PRES_RANGE;

/* Storage for contour presentations */
static	CON_PRES_RANGE	*ContourValues   = NullPtr(CON_PRES_RANGE *);
static	int				NumContourValues = 0;
static	CON_PRES_RANGE	*ContourRanges   = NullPtr(CON_PRES_RANGE *);
static	int				NumContourRanges = 0;

void		initialize_contour_presentation

	(
	)

	{
	int			ii;

	/* Free space used by contour presentations */
	for ( ii=0; ii<NumContourValues; ii++ ) FREEMEM(ContourValues[ii].units);
	for ( ii=0; ii<NumContourRanges; ii++ ) FREEMEM(ContourRanges[ii].units);
	FREEMEM(ContourValues);
	FREEMEM(ContourRanges);
	NumContourValues = 0;
	NumContourRanges = 0;
	}

LOGICAL		add_contour_values

	(
	STRING		values,
	STRING		units,
	PRES		*inpres
	)

	{
	int			ii, nc;
	STRING		test, con_vals;
	float		val;
	double		dval;
	RANGE		xrange, trange;

	/* Loop to extract one or more contour values from list */
	con_vals = strdup(values);
	while ( NotNull( test = string_arg(con_vals) ) )
		{

		/* Extract contour value and set range about value */
		/*  adjusted by a fraction of units                */
		(void) sscanf(test, "%f", &val);
		xrange.minval = (double) val - 1.0E-06;
		xrange.mincon = RangeExcl;
		xrange.maxval = (double) val + 1.0E-06;
		xrange.maxcon = RangeExcl;

		/* Reset the presentation for contours already in list */
		for ( ii=0; ii<NumContourValues; ii++ )
			{

			/* Convert units to match value in list */
			if ( !convert_value(units, (double) val,
					ContourValues[ii].units, &dval) ) continue;

			/* Set test range with adjustment of a fraction of units */
			(void) copy_range(&trange, &xrange);
			trange.minval = dval - 1.0E-06;
			trange.maxval = dval + 1.0E-06;

			/* Check for matching range in list */
			if ( !same_range(&trange, &(ContourValues[ii].range)) ) continue;

			/* Reset contour presentation */
			(void) copy_presentation(&(ContourValues[ii].presentation), inpres);
			break;
			}

		/* Add another contour presentation if not found in values list */
		if ( ii >= NumContourValues )
			{

			/* Add another contour presentation to the values list */
			nc = NumContourValues++;
			ContourValues = GETMEM(ContourValues, CON_PRES_RANGE,
															NumContourValues);

			/* Set range and contour presentation for each value */
			ContourValues[nc].old_version = FALSE;
			(void) copy_range(&(ContourValues[nc].range), &xrange);
			ContourValues[nc].units       = strdup(units);
			(void) copy_presentation(&(ContourValues[nc].presentation), inpres);
			}
		}

	/* Return TRUE */
	FREEMEM(con_vals);
	return TRUE;
	}

LOGICAL		add_contour_ranges

	(
	STRING		range,
	STRING		units,
	PRES		*inpres
	)

	{
	int			ii, nc;
	double		dval;
	RANGE		xrange, trange;

	/* Read the contour range */
	if ( !read_range(range, &xrange) ) return FALSE;

	/* Reset the presentation for ranges already in list */
	for ( ii=0; ii<NumContourRanges; ii++ )
		{

		/* Convert units (where applicable) to match range in list */
		(void) copy_range(&trange, &xrange);
		if ( trange.mincon != RangeUnlim )
			{
			if ( !convert_value(units, trange.minval,
					ContourRanges[ii].units, &dval) ) continue;
			trange.minval = dval;
			}
		if ( trange.maxcon != RangeUnlim )
			{
			if ( !convert_value(units, trange.maxval,
					ContourRanges[ii].units, &dval) ) continue;
			trange.maxval = dval;
			}

		/* Check for matching range in list */
		if ( !same_range(&trange, &(ContourRanges[ii].range)) ) continue;

		/* Reset contour presentation */
		(void) copy_presentation(&(ContourRanges[ii].presentation), inpres);
		break;
		}

	/* Add another contour presentation if not found in ranges list */
	if ( ii >= NumContourRanges )
		{

		/* Add another contour presentation to the ranges list */
		nc = NumContourRanges++;
		ContourRanges = GETMEM(ContourRanges, CON_PRES_RANGE, NumContourRanges);

		/* Set range and contour presentation for each range */
		ContourRanges[nc].old_version = FALSE;
		(void) copy_range(&(ContourRanges[nc].range), &xrange);
		ContourRanges[nc].units       = strdup(units);
		(void) copy_presentation(&(ContourRanges[nc].presentation), inpres);
		}

	/* Return TRUE */
	return TRUE;
	}

PRES		get_contour_presentation

	(
	float		value,
	STRING		units
	)

	{
	int			ii;
	double		dval;
	char		err_buf[GPGLong];

	/* First check for individual contour values                */
	/* Note that values are saved as a range close to the value */
	/*  to allow for round off errors!                          */
	for ( ii=0; ii<NumContourValues; ii++ )
		{

		/* Convert value to matching units */
		if ( !convert_value(units, (double) value,
				ContourValues[ii].units, &dval) )
			{
			(void) sprintf(err_buf, "Cannot convert from \"%s\" to \"%s\"",
					units, ContourValues[ii].units);
			(void) error_report(err_buf);
			}

		/* Test for value outside contour value range */
		if ( !test_range(dval, &(ContourValues[ii].range)) ) continue;

		/* Return the contour presentation if value is within value range */
		return ContourValues[ii].presentation;
		}

	/* Then check for contour ranges */
	for ( ii=0; ii<NumContourRanges; ii++ )
		{

		/* Convert units (based on version) */
		if ( ContourRanges[ii].old_version )
			{
			dval = (double) value;
			}
		else
			{
			if ( !convert_value(units, (double) value,
					ContourRanges[ii].units, &dval) )
				{
				(void) sprintf(err_buf,
						"Cannot convert from \"%s\" to \"%s\"",
						units, ContourRanges[ii].units);
				(void) error_report(err_buf);
				}
			}

		/* Test for value outside contour range */
		if ( !test_range(dval, &(ContourRanges[ii].range)) ) continue;

		/* Return the contour presentation if value is within range */
		return ContourRanges[ii].presentation;
		}

	/* Return default presentation if contour value not found */
	return PresDef;
	}

/***********************************************************************
*                                                                      *
*    i n i t i a l i z e _ g e o _ p r e s e n t a t i o n             *
*    a d d _ g e o _ p r e s e n t a t i o n                           *
*    g e t _ g e o _ p r e s e n t a t i o n                           *
*                                                                      *
***********************************************************************/

/* Storage for geographical presentations */
static	PRES	*GeoPres   = NullPtr(PRES *);
static	int		NumGeoPres =  0;
static	int		CurGeoPres = -1;

void		initialize_geo_presentation

	(
	)

	{

	/* Free space used by geographic presentations */
	FREEMEM(GeoPres);
	NumGeoPres =  0;
	CurGeoPres = -1;
	}

PRES		*add_geo_presentation

	(
	STRING		name
	)

	{
	int			ii, ng;

	/* Search the list for the named geographical presentation */
	for ( ii=0; ii<NumGeoPres; ii++ )
		{

		/* Return the named geographical presentation (if found) */
		if ( same(GeoPres[ii].name, name) ) return &GeoPres[ii];
		}

	/* Add another geographical presentation to the list */
	NumGeoPres++;
	GeoPres = GETMEM(GeoPres, PRES, NumGeoPres);
	ng = NumGeoPres - 1;

	/* Set name and default geographical presentation                    */
	/* Note that default is set from the last geographical presentation! */
	if ( NumGeoPres == 1 )
		{
		(void) copy_presentation(&GeoPres[ng], &PresDef);
		}
	else
		{
		(void) copy_presentation(&GeoPres[ng], &GeoPres[ng-1]);
		}
	(void) strcpy(GeoPres[ng].name, name);

	/* Return the new geographical presentation */
	return &GeoPres[ng];
	}

PRES		get_geo_presentation

	(
	STRING		name
	)

	{
	int			ii;
	char		err_buf[GPGLong];

	/* Return default presentation if no geographic name passed */
	if ( blank(name) )
		{
		(void) sprintf(err_buf, "Using default geographic presentation");
		(void) warn_report(err_buf);
		return PresDef;
		}

	/* Check if the name corresponds to the current geographic presentation */
	if ( CurGeoPres >= 0 && same(name, GeoPres[CurGeoPres].name) )
		return GeoPres[CurGeoPres];

	/* Use the geographic presentation corresponding to this name */
	for ( ii=0; ii<NumGeoPres; ii++ )
		{
		if ( same(GeoPres[ii].name, name) )
			{
			CurGeoPres = ii;
			return GeoPres[CurGeoPres];
			}
		}

	/* Use default presentation if geographic name not found */
	(void) sprintf(err_buf, "Missing named geographic presentation for: %s",
			name);
	(void) warn_report(err_buf);
	return PresDef;
	}

/***********************************************************************
*                                                                      *
*    c o p y _ a r r o w _ d i s p l a y                               *
*    a d d _ a r r o w _ d i s p l a y                                 *
*    g e t _ a r r o w _ d i s p l a y                                 *
*                                                                      *
***********************************************************************/

/* Storage for parameters for displaying arrows on lines */
static	ARROW_DISPLAY		DefaultArrowDisplay = NO_ARROW_DISPLAY;
static	ARROW_DISPLAY		*ArrowDisplay       = NullPtr(ARROW_DISPLAY *);
static	int					NumArrowDisplay     =  0;
static	int					CurArrowDisplay     = -1;

void			copy_arrow_display

	(
	ARROW_DISPLAY		*outarrow,
	ARROW_DISPLAY		*inarrow
	)

	{

	/* Return immediately if missing input or output arrow display */
	if ( IsNull(inarrow) || IsNull(outarrow) ) return;

	/* Copy the arrow display parameters */
	(void) strcpy(outarrow->name,     inarrow->name);
	(void) strcpy(outarrow->features, inarrow->features);
	outarrow->length       = inarrow->length;
	outarrow->angle        = inarrow->angle;
	outarrow->return_angle = inarrow->return_angle;
	outarrow->length_off   = inarrow->length_off;
	outarrow->width_off    = inarrow->width_off;
	outarrow->head_length  = inarrow->head_length;
	outarrow->tail_length  = inarrow->tail_length;
	}

ARROW_DISPLAY	*add_arrow_display

	(
	STRING		name
	)

	{
	int			ii, ne;

	/* Search the list for the named arrow display */
	for ( ii=0; ii<NumArrowDisplay; ii++ )
		{

		/* Return the named arrow display (if found) */
		if ( same(ArrowDisplay[ii].name, name) ) return &ArrowDisplay[ii];
		}

	/* Add another arrow display to the list */
	NumArrowDisplay++;
	ArrowDisplay = GETMEM(ArrowDisplay, ARROW_DISPLAY, NumArrowDisplay);
	ne = NumArrowDisplay - 1;

	/* Set default arrow name and parameters */
	(void) copy_arrow_display(&ArrowDisplay[ne], &DefaultArrowDisplay);
	(void) strcpy(ArrowDisplay[ne].name, name);

	/* Return the new arrow display */
	return &ArrowDisplay[ne];
	}

ARROW_DISPLAY	get_arrow_display

	(
	STRING		name
	)

	{
	int			ii;
	char		err_buf[GPGLong];

	/* Check if the name corresponds to the current arrow display */
	if ( CurArrowDisplay >= 0
			&& same(name, ArrowDisplay[CurArrowDisplay].name) )
		return ArrowDisplay[CurArrowDisplay];

	/* Use the arrow display corresponding to this name */
	for ( ii=0; ii<NumArrowDisplay; ii++ )
		{
		if ( same(ArrowDisplay[ii].name, name) )
			{
			CurArrowDisplay = ii;
			return ArrowDisplay[CurArrowDisplay];
			}
		}

	/* Use default arrow display if name not found */
	(void) sprintf(err_buf, "Missing named arrow display for: %s", name);
	(void) warn_report(err_buf);
	return DefaultArrowDisplay;
	}

/***********************************************************************
*                                                                      *
*    c o p y _ l a b e l _ d i s p l a y                               *
*    a d d _ l a b e l _ d i s p l a y                                 *
*    g e t _ l a b e l _ d i s p l a y                                 *
*                                                                      *
***********************************************************************/

/* Storage for parameters for displaying outlined or underlined text */
static	LABEL_DISPLAY		DefaultLabelDisplay = NO_LABEL_DISPLAY;
static	LABEL_DISPLAY		*LabelDisplay       = NullPtr(LABEL_DISPLAY *);
static	int					NumLabelDisplay     =  0;
static	int					CurLabelDisplay     = -1;

void			copy_label_display

	(
	LABEL_DISPLAY		*outlabel,
	LABEL_DISPLAY		*inlabel
	)

	{

	/* Return immediately if missing input or output label display */
	if ( IsNull(inlabel) || IsNull(outlabel) ) return;

	/* Copy the label display parameters */
	(void) strcpy(outlabel->name,            inlabel->name);
	(void) strcpy(outlabel->width_attrib,    inlabel->width_attrib);
	(void) strcpy(outlabel->height_attrib,   inlabel->height_attrib);
	(void) strcpy(outlabel->diameter_attrib, inlabel->diameter_attrib);
	(void) strcpy(outlabel->radius_attrib,   inlabel->radius_attrib);
	(void) strcpy(outlabel->attrib_units,    inlabel->attrib_units);
	(void) strcpy(outlabel->rot_attrib,      inlabel->rot_attrib);
	outlabel->width         = inlabel->width;
	outlabel->height        = inlabel->height;
	outlabel->start_angle   = inlabel->start_angle;
	outlabel->end_angle     = inlabel->end_angle;
	outlabel->closed        = inlabel->closed;
	outlabel->rotation      = inlabel->rotation;
	outlabel->x_off         = inlabel->x_off;
	outlabel->y_off         = inlabel->y_off;
	outlabel->margin_top    = inlabel->margin_top;
	outlabel->margin_bottom = inlabel->margin_bottom;
	outlabel->margin_left   = inlabel->margin_left;
	outlabel->margin_right  = inlabel->margin_right;
	(void) copy_presentation(&outlabel->presentation, &inlabel->presentation);
	}

LABEL_DISPLAY	*add_label_display

	(
	STRING		name
	)

	{
	int			ii, ne;

	/* Search the list for the named label display */
	for ( ii=0; ii<NumLabelDisplay; ii++ )
		{

		/* Return the named label display (if found) */
		if ( same(LabelDisplay[ii].name, name) ) return &LabelDisplay[ii];
		}

	/* Add another label display to the list */
	NumLabelDisplay++;
	LabelDisplay = GETMEM(LabelDisplay, LABEL_DISPLAY, NumLabelDisplay);
	ne = NumLabelDisplay - 1;

	/* Set default label name and parameters */
	(void) copy_label_display(&LabelDisplay[ne], &DefaultLabelDisplay);
	(void) strcpy(LabelDisplay[ne].name, name);
	(void) copy_presentation(&(LabelDisplay[ne].presentation), &PresDef);

	/* Return the new label display */
	return &LabelDisplay[ne];
	}

LABEL_DISPLAY	get_label_display

	(
	STRING		name
	)

	{
	int				ii;
	LABEL_DISPLAY	temp_label;
	char			err_buf[GPGLong];

	/* Check if the name corresponds to the current label display */
	if ( CurLabelDisplay >= 0
			&& same(name, LabelDisplay[CurLabelDisplay].name) )
		return LabelDisplay[CurLabelDisplay];

	/* Use the label display corresponding to this name */
	for ( ii=0; ii<NumLabelDisplay; ii++ )
		{
		if ( same(LabelDisplay[ii].name, name) )
			{
			CurLabelDisplay = ii;
			return LabelDisplay[CurLabelDisplay];
			}
		}

	/* Use default label display if name not found */
	(void) sprintf(err_buf, "Missing named label display for: %s", name);
	(void) warn_report(err_buf);
	(void) copy_label_display(&temp_label, &DefaultLabelDisplay);
	(void) copy_presentation(&temp_label.presentation, &PresDef);
	return temp_label;
	}

/***********************************************************************
*                                                                      *
*    i n i t i a l i z e _ a t t r i b u t e _ d i s p l a y           *
*    c o p y _ a t t r i b u t e _ d i s p l a y                       *
*    a d d _ a t t r i b u t e _ d i s p l a y                         *
*    g e t _ a t t r i b u t e _ d i s p l a y                         *
*    s e t _ a t t r i b u t e _ p l a c e m e n t                     *
*    s e t _ w i n d _ b a r b _ p l a c e m e n t                     *
*    s e t _ w i n d _ p l a c e m e n t                               *
*    s e t _ v e c t o r _ p l a c e m e n t                           *
*    f u l l _ a t t r i b u t e _ p l a c e m e n t                   *
*    f i t _ a t t r i b u t e _ p l a c e m e n t                     *
*    r e t u r n _ a t t r i b u t e _ d i s p l a y                   *
*                                                                      *
***********************************************************************/

/* Storage for parameters for displaying attributes */
static	ATTRIB_DISPLAY		DefaultAttribDisplay = NO_ATTRIB_DISPLAY;
static	ATTRIB_DISPLAY		*AttribDisplay       = NullPtr(ATTRIB_DISPLAY *);
static	int					NumAttribDisplay     =  0;
static	int					CurAttribDisplay     = -1;
static	ATTRIB_DISPLAY		FullAttribDisplay    = NO_ATTRIB_DISPLAY;

void			initialize_attribute_display

	(
	)

	{

	/* Free space used by all current attribute displays */
	FREEMEM(AttribDisplay);

	/* Reset the attribute display counter */
	NumAttribDisplay =  0;
	CurAttribDisplay = -1;
	}

void			copy_attribute_display

	(
	ATTRIB_DISPLAY		*outattrib,
	ATTRIB_DISPLAY		*inattrib
	)

	{

	/* Return immediately if missing input or output attribute display */
	if ( IsNull(inattrib) || IsNull(outattrib) ) return;

	/* Copy the attribute display parameters */
	(void) strcpy(outattrib->name,              inattrib->name);
	(void) strcpy(outattrib->anchor,            inattrib->anchor);
	(void) strcpy(outattrib->ref,               inattrib->ref);
	(void) strcpy(outattrib->vertical_just,     inattrib->vertical_just);
	(void) strcpy(outattrib->units,             inattrib->units);
	(void) strcpy(outattrib->format,            inattrib->format);
	(void) strcpy(outattrib->conversion_format, inattrib->conversion_format);
	(void) strcpy(outattrib->look_up,           inattrib->look_up);
	(void) strcpy(outattrib->display_name,      inattrib->display_name);
	(void) strcpy(outattrib->display_type,      inattrib->display_type);
	outattrib->show_label   = inattrib->show_label;
	outattrib->symbol_scale = inattrib->symbol_scale;
	outattrib->txt_size     = inattrib->txt_size;
	outattrib->width_scale  = inattrib->width_scale;
	outattrib->height_scale = inattrib->height_scale;
	outattrib->x_off        = inattrib->x_off;
	outattrib->y_off        = inattrib->y_off;
	outattrib->x_attrib     = inattrib->x_attrib;
	outattrib->y_attrib     = inattrib->y_attrib;
	outattrib->x_left       = inattrib->x_left;
	outattrib->x_centre     = inattrib->x_centre;
	outattrib->x_right      = inattrib->x_right;
	outattrib->y_top        = inattrib->y_top;
	outattrib->y_centre     = inattrib->y_centre;
	outattrib->y_bottom     = inattrib->y_bottom;

	/* Copy the attribute presentation */
	(void) copy_presentation(&(outattrib->presentation),
								&(inattrib->presentation));
	}

ATTRIB_DISPLAY	*add_attribute_display

	(
	STRING		name,
	STRING		units,
	STRING		format,
	STRING		conversion_format,
	STRING		look_up,
	float		symbol_scale,
	float		txt_size,
	float		width_scale,
	float		height_scale,
	PRES		*inpres
	)

	{
	int			ne;

	/* Add another attribute display to the list */
	NumAttribDisplay++;
	AttribDisplay = GETMEM(AttribDisplay, ATTRIB_DISPLAY, NumAttribDisplay);
	ne = NumAttribDisplay - 1;

	/* Set default attribute display and name */
	(void) copy_attribute_display(&AttribDisplay[ne], &DefaultAttribDisplay);
	(void) strcpy(AttribDisplay[ne].name, name);

	/* Set default attribute parameters */
	(void) strcpy(AttribDisplay[ne].units,             units);
	(void) strcpy(AttribDisplay[ne].format,            format);
	(void) strcpy(AttribDisplay[ne].conversion_format, conversion_format);
	(void) strcpy(AttribDisplay[ne].look_up,           look_up);
	AttribDisplay[ne].symbol_scale = symbol_scale;
	AttribDisplay[ne].txt_size     = txt_size;
	AttribDisplay[ne].width_scale  = width_scale;
	AttribDisplay[ne].height_scale = height_scale;

	/* Set the default attribute presentation */
	(void) copy_presentation(&(AttribDisplay[ne].presentation), inpres);

	/* Return the new attribute display */
	return &AttribDisplay[ne];
	}

ATTRIB_DISPLAY	get_attribute_display

	(
	STRING		name
	)

	{
	int			ii;
	char		err_buf[GPGLong];

	/* Check if the name corresponds to the current attribute display */
	if ( CurAttribDisplay >= 0
			&& same(name, AttribDisplay[CurAttribDisplay].name) )
		return AttribDisplay[CurAttribDisplay];

	/* Use the attribute display corresponding to this name */
	for ( ii=0; ii<NumAttribDisplay; ii++ )
		{
		if ( same(AttribDisplay[ii].name, name) )
			{
			CurAttribDisplay = ii;
			return AttribDisplay[CurAttribDisplay];
			}
		}

	/* Return default attribute display if name not found */
	(void) sprintf(err_buf, "Missing attribute label display for: %s", name);
	(void) warn_report(err_buf);
	return DefaultAttribDisplay;
	}

void			set_attribute_placement

	(
	ATTRIB_DISPLAY	*attrib,
	STRING			text,
	STRING			symbol
	)

	{
	float			txt_size, sym_scale, pscale;
	float			width, height, xcenoff, ycenoff, halfwd, halfht, xx, yy;
	ATTRIB_DISPLAY	temp_attrib;

	/* Set attribute text size and symbol scale */
	txt_size  = attrib->txt_size;
	sym_scale = attrib->symbol_scale;

	/* Set width and height of attribute based on size of attribute text */
	if ( !blank(text) )
		{
		(void) determine_text_size(text, txt_size,
				attrib->width_scale, attrib->height_scale, &width, &height);
		}

	/* Set width and height of attribute based on size of attribute symbol */
	else if ( !blank(symbol) )
		{
		(void) graphics_symbol_size(symbol, sym_scale, &width, &height,
				&xcenoff, &ycenoff);
		}

	/* Set default size for unknown attribute types */
	else
		{
		width  = 0.0;
		height = 0.0;
		}

	/* Adjust size for perspective (if required) */
	if ( perspective_scale(&pscale) )
		{
		width  *= pscale;
		height *= pscale;
		}

	/* Set half sizes */
	halfwd = width / 2.0;
	halfht = height / 2.0;

	/* Set default attribute position */
	xx = attrib->x_off;
	yy = attrib->y_off;

	/* Use default position if attribute is not "anchored" to another */
	if ( same(attrib->anchor, AttribAnchorNone) )
		{
		}

	/* Adjust attribute position using "attribute_anchor" and "attribute_ref" */
	else
		{

		/* Get attribute to use as "anchor" location */
		temp_attrib = get_attribute_display(attrib->anchor);

		/* Adjust attribute position wrt position on "anchor" attribute */
		if ( same(attrib->ref, AnchorLowerLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_top;
			}
		}

	/* Set attribute placement for text strings */
	if ( !blank(text) )
		{

		/* Set horizontal attribute placement for left justfication   */
		/* Note that text is always aligned wrt current justification */
		if ( same(attrib->presentation.justified, JustifyLeft) )
			{
			attrib->x_attrib = xx;
			attrib->x_left   = xx;
			attrib->x_centre = xx + halfwd;
			attrib->x_right  = xx + width;
			}

		/* Set horizontal attribute placement for centre justfication */
		/* Note that text is always aligned wrt current justification */
		else if ( same(attrib->presentation.justified, JustifyCentre) )
			{
			attrib->x_attrib = xx;
			attrib->x_left   = xx - halfwd;
			attrib->x_centre = xx;
			attrib->x_right  = xx + halfwd;
			}

		/* Set horizontal attribute placement for right justfication  */
		/* Note that text is always aligned wrt current justification */
		else if ( same(attrib->presentation.justified, JustifyRight) )
			{
			attrib->x_attrib = xx;
			attrib->x_left   = xx - width;
			attrib->x_centre = xx - halfwd;
			attrib->x_right  = xx;
			}

		/* Default horizontal placement is same as left justfication  */
		/* Note that text is always aligned wrt current justification */
		else
			{
			attrib->x_attrib = xx;
			attrib->x_left   = xx;
			attrib->x_centre = xx + halfwd;
			attrib->x_right  = xx + width;
			}

		/* Set vertical attribute placement for bottom justfication */
		/* Note that text is always aligned along the bottom edge   */
		if ( same(attrib->vertical_just, VerticalBottom) )
			{
			attrib->y_attrib = yy;
			attrib->y_top    = yy + height;
			attrib->y_centre = yy + halfht;
			attrib->y_bottom = yy;
			}

		/* Set vertical attribute placement for centre justfication */
		/* Note that text is always aligned along the bottom edge   */
		else if ( same(attrib->vertical_just, VerticalCentre) )
			{
			attrib->y_attrib = yy - halfht;
			attrib->y_top    = yy + halfht;
			attrib->y_centre = yy;
			attrib->y_bottom = yy - halfht;
			}

		/* Set vertical attribute placement for top justfication  */
		/* Note that text is always aligned along the bottom edge */
		else if ( same(attrib->vertical_just, VerticalTop) )
			{
			attrib->y_attrib = yy - height;
			attrib->y_top    = yy;
			attrib->y_centre = yy - halfht;
			attrib->y_bottom = yy - height;
			}

		/* Default vertical placement is same as bottom justfication */
		/* Note that text is always aligned along the bottom edge    */
		else
			{
			attrib->y_attrib = yy;
			attrib->y_top    = yy + height;
			attrib->y_centre = yy + halfht;
			attrib->y_bottom = yy;
			}
		}

	/* Set attribute placement for symbols */
	else if ( !blank(symbol) )
		{

		/* Set horizontal attribute placement for left justfication */
		/* Note that symbols are always centred                     */
		if ( same(attrib->presentation.justified, JustifyLeft) )
			{
			attrib->x_attrib = xx + halfwd - xcenoff;
			attrib->x_left   = xx;
			attrib->x_centre = xx + halfwd - xcenoff;
			attrib->x_right  = xx + width;
			}

		/* Set horizontal attribute placement for centre justfication */
		/* Note that symbols are always centred                       */
		else if ( same(attrib->presentation.justified, JustifyCentre) )
			{
			attrib->x_attrib = xx;
			attrib->x_left   = xx - halfwd + xcenoff;
			attrib->x_centre = xx;
			attrib->x_right  = xx + halfwd + xcenoff;
			}

		/* Set horizontal attribute placement for right justfication */
		/* Note that symbols are always centred                      */
		else if ( same(attrib->presentation.justified, JustifyRight) )
			{
			attrib->x_attrib = xx - halfwd - xcenoff;
			attrib->x_left   = xx - width;
			attrib->x_centre = xx - halfwd - xcenoff;
			attrib->x_right  = xx;
			}

		/* Default horizontal placement is same as centre justfication */
		/* Note that symbols are always centred                        */
		else
			{
			attrib->x_attrib = xx;
			attrib->x_left   = xx - halfwd + xcenoff;
			attrib->x_centre = xx;
			attrib->x_right  = xx + halfwd + xcenoff;
			}

		/* Set vertical attribute placement for bottom justfication */
		/* Note that symbols are always centred                     */
		if ( same(attrib->vertical_just, VerticalBottom) )
			{
			attrib->y_attrib = yy + halfht - ycenoff;
			attrib->y_top    = yy + height;
			attrib->y_centre = yy + halfht - ycenoff;
			attrib->y_bottom = yy;
			}

		/* Set vertical attribute placement for centre justfication */
		/* Note that symbols are always centred                     */
		else if ( same(attrib->vertical_just, VerticalCentre) )
			{
			attrib->y_attrib = yy;
			attrib->y_top    = yy + halfht + ycenoff;
			attrib->y_centre = yy;
			attrib->y_bottom = yy - halfht + ycenoff;
			}

		/* Set vertical attribute placement for top justfication */
		/* Note that symbols are always centred                  */
		else if ( same(attrib->vertical_just, VerticalTop) )
			{
			attrib->y_attrib = yy - halfht - ycenoff;
			attrib->y_top    = yy;
			attrib->y_centre = yy - halfht - ycenoff;
			attrib->y_bottom = yy - height;
			}

		/* Default vertical placement is same as centre justfication */
		/* Note that symbols are always centred                      */
		else
			{
			attrib->y_attrib = yy;
			attrib->y_top    = yy + halfht + ycenoff;
			attrib->y_centre = yy;
			attrib->y_bottom = yy - halfht + ycenoff;
			}
		}

	/* Set default attribute placement for unknown types */
	else
		{

		/* Set horizontal attribute placement */
		attrib->x_attrib = xx;
		attrib->x_left   = xx;
		attrib->x_centre = xx;
		attrib->x_right  = xx;

		/* Set vertical attribute placement */
		attrib->y_attrib = yy;
		attrib->y_top    = yy;
		attrib->y_centre = yy;
		attrib->y_bottom = yy;
		}
	}

void			set_wind_barb_placement

	(
	ATTRIB_DISPLAY	*attrib,
	float			unused_dir,
	float			spd,
	float			gst
	)

	{
	int				num_50;
	float			shaftlen, barbspace, barbwidth, gustdist, pscale;
	float			width, height, halfwd, halfht, xx, yy;
	ATTRIB_DISPLAY	temp_attrib;

	/* Set size based on size of "calm" symbol if wind speed is very small */
	if ( spd < BarbDef.calm_max )
		{

		/* Set scaled size of symbol */
		(void) graphics_symbol_size(BarbDef.calm_symbol, BarbDef.calm_scale,
				&width, &height, NullFloat, NullFloat);
		}

	/* Set size based on size of "huge" symbol if wind speed is very large */
	else if ( spd > BarbDef.huge_min )
		{

		/* Set scaled size of symbol */
		(void) graphics_symbol_size(BarbDef.huge_symbol, BarbDef.huge_scale,
				&width, &height, NullFloat, NullFloat);
		}

	/* Set size based on barb length or gust location */
	else
		{

		/* Determine barb length based on wind speed */
		shaftlen  = BarbDef.shaft_length;
		barbspace = BarbDef.barb_space * shaftlen;
		barbwidth = BarbDef.barb_width * shaftlen;
		num_50   = NINT(spd) / 50;
		if ( num_50 > 1 ) shaftlen += (float) (num_50 - 1) * barbspace;
		if ( num_50 > 0 ) shaftlen += barbwidth;

		/* Determine gust location (if required) */
		gustdist = 0.0;
		if ( (gst - spd) >= BarbDef.gust_above )
			{
			gustdist = (BarbDef.gust_distance * shaftlen)
						+ (BarbDef.gust_size / 2.0);
			}

		/* Set size based on greater of barb length or gust location */
		width = height = MAX(shaftlen, gustdist) * 2.0;
		}

	/* Adjust size for perspective (if required) */
	if ( perspective_scale(&pscale) )
		{
		width  *= pscale;
		height *= pscale;
		}

	/* Set half sizes */
	halfwd = width / 2.0;
	halfht = height / 2.0;

	/* Set default attribute position */
	xx = attrib->x_off;
	yy = attrib->y_off;

	/* Use default position if attribute is not "anchored" to another */
	if ( same(attrib->anchor, AttribAnchorNone) )
		{
		}

	/* Adjust attribute position using "attribute_anchor" and "attribute_ref" */
	else
		{

		/* Get attribute to use as "anchor" location */
		temp_attrib = get_attribute_display(attrib->anchor);

		/* Adjust attribute position wrt position on "anchor" attribute */
		if ( same(attrib->ref, AnchorLowerLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_top;
			}
		}

	/* Set horizontal attribute placement (centred) */
	attrib->x_attrib = xx;
	attrib->x_left   = xx - halfwd;
	attrib->x_centre = xx;
	attrib->x_right  = xx + halfwd;

	/* Set vertical attribute placement (centred) */
	attrib->y_attrib = yy;
	attrib->y_top    = yy + halfht;
	attrib->y_centre = yy;
	attrib->y_bottom = yy - halfht;
	}

void			set_wind_placement

	(
	STRING			format,
	ATTRIB_DISPLAY	*attrib,
	float			dir,
	float			spd,
	float			gst,
	float			flat,
	float			flon
	)

	{
	STRING			wformat, text, symbol;
	float			sym_scale, width, height, pscale, xx, yy;
	POINT			llpoint, urpoint;
	POINT			dir_llpoint, dir_urpoint;
	POINT			spd_llpoint, spd_urpoint;
	POINT			gst_llpoint, gst_urpoint;
	LOGICAL			isdir, isspd, isgst;
	ATTRIB_DISPLAY	temp_attrib;

	/* Static buffer for formatted wind values */
	static	char	ftext[GPGMedium];

	/* Set locations based on calm text or symbol */
	wformat = WindDef.calm_type;
	if ( blank(wformat) )
		{
		if      ( same(format, FormatWindText))   wformat = WVsubText;
		else if ( same(format, FormatWindSymbol)) wformat = WVsubSymbol;
		}
	if ( !same(wformat, WVsubNone)
			&& match_wind_lookup(WindDef.wind_lookup, WindCalm,
					wformat, flat, flon, dir, spd, gst,
					&text, &symbol, &sym_scale, NullFloat) )
		{

		/* Set width and height of calm as a text string */
		if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
			{
			(void) sprintf(ftext, WindDef.calm_format, text);
			(void) determine_text_size(ftext, WindDef.calm_size,
					attrib->width_scale, attrib->height_scale,
					&width, &height);
			}

		/* Set width and height of calm as a symbol */
		else if ( same(wformat, WVsubSymbol) )
			{
			(void) graphics_symbol_size(symbol, WindDef.calm_scale,
					&width, &height, NullFloat, NullFloat);
			}

		/* Set default width and height for unknown calm types */
		else
			{
			width  = 0.0;
			height = 0.0;
			}

		/* Adjust size for perspective (if required) */
		if ( perspective_scale(&pscale) )
			{
			width  *= pscale;
			height *= pscale;
			}

		/* Set locations based on justification */
		if ( same(WindDef.calm_just, JustifyLeft) )
			{
			llpoint[X] = WindDef.x_calm;
			llpoint[Y] = WindDef.y_calm;
			urpoint[X] = WindDef.x_calm + width;
			urpoint[Y] = WindDef.y_calm + height;
			}
		else if ( same(WindDef.calm_just, JustifyCentre) )
			{
			llpoint[X] = WindDef.x_calm - width/2.0;
			llpoint[Y] = WindDef.y_calm;
			urpoint[X] = WindDef.x_calm + width/2.0;
			urpoint[Y] = WindDef.y_calm + height;
			}
		else if ( same(WindDef.calm_just, JustifyRight) )
			{
			llpoint[X] = WindDef.x_calm - width;
			llpoint[Y] = WindDef.y_calm;
			urpoint[X] = WindDef.x_calm;
			urpoint[Y] = WindDef.y_calm + height;
			}
		else
			{
			llpoint[X] = WindDef.x_calm;
			llpoint[Y] = WindDef.y_calm;
			urpoint[X] = WindDef.x_calm + width;
			urpoint[Y] = WindDef.y_calm + height;
			}
		}

	/* Set locations based on direction, speed, and gust text or symbols */
	else
		{

		/* Set locations based on direction text or symbol */
		isdir = FALSE;
		wformat = WindDef.direction_type;
		if ( blank(wformat) )
			{
			if      ( same(format, FormatWindText))   wformat = WVsubText;
			else if ( same(format, FormatWindSymbol)) wformat = WVsubUniform;
			}
		if ( !same(wformat, WVsubNone)
				&& match_wind_lookup(WindDef.wind_lookup, WindDirection,
						wformat, flat, flon, dir, spd, gst,
						&text, &symbol, &sym_scale, NullFloat) )
			{

			/* Set flag for direction used */
			isdir = TRUE;

			/* Set width and height of direction as a text string */
			if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
				{
				(void) sprintf(ftext, WindDef.direction_format, text);
				(void) determine_text_size(ftext, WindDef.direction_size,
						attrib->width_scale, attrib->height_scale,
						&width, &height);
				}

			/* Set width and height of direction as a uniform symbol */
			else if ( same(wformat, WVsubUniform) )
				{
				(void) graphics_symbol_size(symbol, WindDef.direction_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set width and height of direction as a proportional symbol */
			else if ( same(wformat, WVsubProportional) )
				{
				(void) graphics_symbol_size(symbol, sym_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set default width and height for unknown direction types */
			else
				{
				width  = 0.0;
				height = 0.0;
				}

			/* Adjust size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set locations based on justification */
			if ( same(WindDef.direction_just, JustifyLeft) )
				{
				dir_llpoint[X] = WindDef.x_dir;
				dir_llpoint[Y] = WindDef.y_dir;
				dir_urpoint[X] = WindDef.x_dir + width;
				dir_urpoint[Y] = WindDef.y_dir + height;
				}
			else if ( same(WindDef.direction_just, JustifyCentre) )
				{
				dir_llpoint[X] = WindDef.x_dir - width/2.0;
				dir_llpoint[Y] = WindDef.y_dir;
				dir_urpoint[X] = WindDef.x_dir + width/2.0;
				dir_urpoint[Y] = WindDef.y_dir + height;
				}
			else if ( same(WindDef.direction_just, JustifyRight) )
				{
				dir_llpoint[X] = WindDef.x_dir - width;
				dir_llpoint[Y] = WindDef.y_dir;
				dir_urpoint[X] = WindDef.x_dir;
				dir_urpoint[Y] = WindDef.y_dir + height;
				}
			else
				{
				dir_llpoint[X] = WindDef.x_dir;
				dir_llpoint[Y] = WindDef.y_dir;
				dir_urpoint[X] = WindDef.x_dir + width;
				dir_urpoint[Y] = WindDef.y_dir + height;
				}
			}

		/* Set locations based on speed text or symbol */
		isspd = FALSE;
		wformat = WindDef.speed_type;
		if ( blank(wformat) )
			{
			if      ( same(format, FormatWindText))   wformat = WVsubText;
			else if ( same(format, FormatWindSymbol)) wformat = WVsubSymbol;
			}
		if ( !same(wformat, WVsubNone)
				&& match_wind_lookup(WindDef.wind_lookup, WindSpeed,
						wformat, flat, flon, dir, spd, gst,
						&text, &symbol, &sym_scale, NullFloat) )
			{

			/* Set flag for speed used */
			isspd = TRUE;

			/* Set width and height of speed as a text string */
			if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
				{
				(void) sprintf(ftext, WindDef.speed_format, text);
				(void) determine_text_size(ftext, WindDef.speed_size,
						attrib->width_scale, attrib->height_scale,
						&width, &height);
				}

			/* Set width and height of speed as a symbol */
			else if ( same(wformat, WVsubSymbol) )
				{
				(void) graphics_symbol_size(symbol, WindDef.speed_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set default width and height for unknown speed types */
			else
				{
				width  = 0.0;
				height = 0.0;
				}

			/* Adjust size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set locations based on justification */
			if ( same(WindDef.speed_just, JustifyLeft) )
				{
				spd_llpoint[X] = WindDef.x_spd;
				spd_llpoint[Y] = WindDef.y_spd;
				spd_urpoint[X] = WindDef.x_spd + width;
				spd_urpoint[Y] = WindDef.y_spd + height;
				}
			else if ( same(WindDef.speed_just, JustifyCentre) )
				{
				spd_llpoint[X] = WindDef.x_spd - width/2.0;
				spd_llpoint[Y] = WindDef.y_spd;
				spd_urpoint[X] = WindDef.x_spd + width/2.0;
				spd_urpoint[Y] = WindDef.y_spd + height;
				}
			else if ( same(WindDef.speed_just, JustifyRight) )
				{
				spd_llpoint[X] = WindDef.x_spd - width;
				spd_llpoint[Y] = WindDef.y_spd;
				spd_urpoint[X] = WindDef.x_spd;
				spd_urpoint[Y] = WindDef.y_spd + height;
				}
			else
				{
				spd_llpoint[X] = WindDef.x_spd;
				spd_llpoint[Y] = WindDef.y_spd;
				spd_urpoint[X] = WindDef.x_spd + width;
				spd_urpoint[Y] = WindDef.y_spd + height;
				}
			}

		/* Set locations based on gust text or symbol */
		isgst = FALSE;
		wformat = WindDef.gust_type;
		if ( blank(wformat) )
			{
			if      ( same(format, FormatWindText))   wformat = WVsubText;
			else if ( same(format, FormatWindSymbol)) wformat = WVsubSymbol;
			}
		if ( (gst - spd) >= WindDef.gust_above
				&& !same(wformat, WVsubNone)
				&& match_wind_lookup(WindDef.wind_lookup, WindGust,
						wformat, flat, flon, dir, spd, gst,
						&text, &symbol, &sym_scale, NullFloat) )
			{

			/* Set flag for gust used */
			isgst = TRUE;

			/* Set width and height of gust as a text string */
			if ( same(wformat, WVsubValue) || same(wformat, WVsubText) )
				{
				(void) sprintf(ftext, WindDef.gust_format, text);
				(void) determine_text_size(ftext, WindDef.gust_size,
						attrib->width_scale, attrib->height_scale,
						&width, &height);
				}

			/* Set width and height of gust as a symbol */
			else if ( same(wformat, WVsubSymbol) )
				{
				(void) graphics_symbol_size(symbol, WindDef.gust_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set default width and height for unknown gust types */
			else
				{
				width  = 0.0;
				height = 0.0;
				}

			/* Adjust size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set locations based on justification */
			if ( same(WindDef.gust_just, JustifyLeft) )
				{
				gst_llpoint[X] = WindDef.x_gust;
				gst_llpoint[Y] = WindDef.y_gust;
				gst_urpoint[X] = WindDef.x_gust + width;
				gst_urpoint[Y] = WindDef.y_gust + height;
				}
			else if ( same(WindDef.gust_just, JustifyCentre) )
				{
				gst_llpoint[X] = WindDef.x_gust - width/2.0;
				gst_llpoint[Y] = WindDef.y_gust;
				gst_urpoint[X] = WindDef.x_gust + width/2.0;
				gst_urpoint[Y] = WindDef.y_gust + height;
				}
			else if ( same(WindDef.gust_just, JustifyRight) )
				{
				gst_llpoint[X] = WindDef.x_gust - width;
				gst_llpoint[Y] = WindDef.y_gust;
				gst_urpoint[X] = WindDef.x_gust;
				gst_urpoint[Y] = WindDef.y_gust + height;
				}
			else
				{
				gst_llpoint[X] = WindDef.x_gust;
				gst_llpoint[Y] = WindDef.y_gust;
				gst_urpoint[X] = WindDef.x_gust + width;
				gst_urpoint[Y] = WindDef.y_gust + height;
				}
			}

		/* Initialize locations from first parameter used */
		/*  ... direction, speed or gust                  */
		if ( isdir )
			{
			(void) copy_point(llpoint, dir_llpoint);
			(void) copy_point(urpoint, dir_urpoint);
			}
		else if ( isspd )
			{
			(void) copy_point(llpoint, spd_llpoint);
			(void) copy_point(urpoint, spd_urpoint);
			}
		else if ( isgst )
			{
			(void) copy_point(llpoint, gst_llpoint);
			(void) copy_point(urpoint, gst_urpoint);
			}
		else
			{
			(void) copy_point(llpoint, ZeroPoint);
			(void) copy_point(urpoint, ZeroPoint);
			}

		/* Reset locations to include all parameters */
		/*  ... direction, speed and gust            */
		if ( isspd )
			{
			llpoint[X] = MIN(llpoint[X], spd_llpoint[X]);
			llpoint[Y] = MIN(llpoint[Y], spd_llpoint[Y]);
			urpoint[X] = MAX(urpoint[X], spd_urpoint[X]);
			urpoint[Y] = MAX(urpoint[Y], spd_urpoint[Y]);
			}
		if ( isgst )
			{
			llpoint[X] = MIN(llpoint[X], gst_llpoint[X]);
			llpoint[Y] = MIN(llpoint[Y], gst_llpoint[Y]);
			urpoint[X] = MAX(urpoint[X], gst_urpoint[X]);
			urpoint[Y] = MAX(urpoint[Y], gst_urpoint[Y]);
			}
		}

	/* Set default attribute position */
	xx = attrib->x_off;
	yy = attrib->y_off;

	/* Use default position if attribute is not "anchored" to another */
	if ( same(attrib->anchor, AttribAnchorNone) )
		{
		}

	/* Adjust attribute position using "attribute_anchor" and "attribute_ref" */
	else
		{

		/* Get attribute to use as "anchor" location */
		temp_attrib = get_attribute_display(attrib->anchor);

		/* Adjust attribute position wrt position on "anchor" attribute */
		if ( same(attrib->ref, AnchorLowerLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_top;
			}
		}

	/* Set horizontal attribute placement */
	attrib->x_attrib = xx;
	attrib->x_left   = xx + llpoint[X];
	attrib->x_right  = xx + urpoint[X];
	attrib->x_centre = (attrib->x_left + attrib->x_right) / 2.0;

	/* Set vertical attribute placement (centred) */
	attrib->y_attrib = yy;
	attrib->y_top    = yy + urpoint[Y];
	attrib->y_bottom = yy + llpoint[Y];
	attrib->y_centre = (attrib->y_top + attrib->y_bottom) / 2.0;
	}

void			set_vector_placement

	(
	STRING			format,
	ATTRIB_DISPLAY	*attrib,
	float			dir,
	float			spd,
	float			flat,
	float			flon
	)

	{
	STRING			vformat, text, symbol;
	float			sym_scale, width, height, pscale, xx, yy;
	POINT			llpoint, urpoint;
	POINT			dir_llpoint, dir_urpoint;
	POINT			spd_llpoint, spd_urpoint;
	LOGICAL			isdir, isspd;
	ATTRIB_DISPLAY	temp_attrib;

	/* Static buffer for formatted vector values */
	static	char	ftext[GPGMedium];

	/* Set locations based on calm text or symbol */
	vformat = VectorDef.calm_type;
	if ( blank(vformat) )
		{
		if      ( same(format, FormatVectorText))   vformat = WVsubText;
		else if ( same(format, FormatVectorSymbol)) vformat = WVsubSymbol;
		}
	if ( !same(vformat, WVsubNone)
			&& match_vector_lookup(VectorDef.vector_lookup, VectorCalm,
					vformat, flat, flon, dir, spd,
					&text, &symbol, &sym_scale, NullFloat) )
		{

		/* Set width and height of calm as a text string */
		if ( same(vformat, WVsubValue) || same(vformat, WVsubText) )
			{
			(void) sprintf(ftext, VectorDef.calm_format, text);
			(void) determine_text_size(ftext, VectorDef.calm_size,
					attrib->width_scale, attrib->height_scale,
					&width, &height);
			}

		/* Set width and height of calm as a symbol */
		else if ( same(vformat, WVsubSymbol) )
			{
			(void) graphics_symbol_size(symbol, VectorDef.calm_scale,
					&width, &height, NullFloat, NullFloat);
			}

		/* Set default width and height for unknown calm types */
		else
			{
			width  = 0.0;
			height = 0.0;
			}

		/* Adjust size for perspective (if required) */
		if ( perspective_scale(&pscale) )
			{
			width  *= pscale;
			height *= pscale;
			}

		/* Set locations based on justification */
		if ( same(VectorDef.calm_just, JustifyLeft) )
			{
			llpoint[X] = VectorDef.x_calm;
			llpoint[Y] = VectorDef.y_calm;
			urpoint[X] = VectorDef.x_calm + width;
			urpoint[Y] = VectorDef.y_calm + height;
			}
		else if ( same(VectorDef.calm_just, JustifyCentre) )
			{
			llpoint[X] = VectorDef.x_calm - width/2.0;
			llpoint[Y] = VectorDef.y_calm;
			urpoint[X] = VectorDef.x_calm + width/2.0;
			urpoint[Y] = VectorDef.y_calm + height;
			}
		else if ( same(VectorDef.calm_just, JustifyRight) )
			{
			llpoint[X] = VectorDef.x_calm - width;
			llpoint[Y] = VectorDef.y_calm;
			urpoint[X] = VectorDef.x_calm;
			urpoint[Y] = VectorDef.y_calm + height;
			}
		else
			{
			llpoint[X] = VectorDef.x_calm;
			llpoint[Y] = VectorDef.y_calm;
			urpoint[X] = VectorDef.x_calm + width;
			urpoint[Y] = VectorDef.y_calm + height;
			}
		}

	/* Set locations based on direction, speed, and gust text or symbols */
	else
		{

		/* Set locations based on direction text or symbol */
		isdir = FALSE;
		vformat = VectorDef.direction_type;
		if ( blank(vformat) )
			{
			if      ( same(format, FormatVectorText))   vformat = WVsubText;
			else if ( same(format, FormatVectorSymbol)) vformat = WVsubUniform;
			}
		if ( !same(vformat, WVsubNone)
				&& match_vector_lookup(VectorDef.vector_lookup, VectorDirection,
						vformat, flat, flon, dir, spd,
						&text, &symbol, &sym_scale, NullFloat) )
			{

			/* Set flag for direction used */
			isdir = TRUE;

			/* Set width and height of direction as a text string */
			if ( same(vformat, WVsubValue) || same(vformat, WVsubText) )
				{
				(void) sprintf(ftext, VectorDef.direction_format, text);
				(void) determine_text_size(ftext, VectorDef.direction_size,
						attrib->width_scale, attrib->height_scale,
						&width, &height);
				}

			/* Set width and height of direction as a uniform symbol */
			else if ( same(vformat, WVsubUniform) )
				{
				(void) graphics_symbol_size(symbol, VectorDef.direction_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set width and height of direction as a proportional symbol */
			else if ( same(vformat, WVsubProportional) )
				{
				(void) graphics_symbol_size(symbol, sym_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set default width and height for unknown direction types */
			else
				{
				width  = 0.0;
				height = 0.0;
				}

			/* Adjust size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set locations based on justification */
			if ( same(VectorDef.direction_just, JustifyLeft) )
				{
				dir_llpoint[X] = VectorDef.x_dir;
				dir_llpoint[Y] = VectorDef.y_dir;
				dir_urpoint[X] = VectorDef.x_dir + width;
				dir_urpoint[Y] = VectorDef.y_dir + height;
				}
			else if ( same(VectorDef.direction_just, JustifyCentre) )
				{
				dir_llpoint[X] = VectorDef.x_dir - width/2.0;
				dir_llpoint[Y] = VectorDef.y_dir;
				dir_urpoint[X] = VectorDef.x_dir + width/2.0;
				dir_urpoint[Y] = VectorDef.y_dir + height;
				}
			else if ( same(VectorDef.direction_just, JustifyRight) )
				{
				dir_llpoint[X] = VectorDef.x_dir - width;
				dir_llpoint[Y] = VectorDef.y_dir;
				dir_urpoint[X] = VectorDef.x_dir;
				dir_urpoint[Y] = VectorDef.y_dir + height;
				}
			else
				{
				dir_llpoint[X] = VectorDef.x_dir;
				dir_llpoint[Y] = VectorDef.y_dir;
				dir_urpoint[X] = VectorDef.x_dir + width;
				dir_urpoint[Y] = VectorDef.y_dir + height;
				}
			}

		/* Set locations based on speed text or symbol */
		isspd = FALSE;
		vformat = VectorDef.speed_type;
		if ( blank(vformat) )
			{
			if      ( same(format, FormatVectorText))   vformat = WVsubText;
			else if ( same(format, FormatVectorSymbol)) vformat = WVsubSymbol;
			}
		if ( !same(vformat, WVsubNone)
				&& match_vector_lookup(VectorDef.vector_lookup, VectorSpeed,
						vformat, flat, flon, dir, spd,
						&text, &symbol, &sym_scale, NullFloat) )
			{

			/* Set flag for speed used */
			isspd = TRUE;

			/* Set width and height of speed as a text string */
			if ( same(vformat, WVsubValue) || same(vformat, WVsubText) )
				{
				(void) sprintf(ftext, VectorDef.speed_format, text);
				(void) determine_text_size(ftext, VectorDef.speed_size,
						attrib->width_scale, attrib->height_scale,
						&width, &height);
				}

			/* Set width and height of speed as a symbol */
			else if ( same(vformat, WVsubSymbol) )
				{
				(void) graphics_symbol_size(symbol, VectorDef.speed_scale,
						&width, &height, NullFloat, NullFloat);
				}

			/* Set default width and height for unknown speed types */
			else
				{
				width  = 0.0;
				height = 0.0;
				}

			/* Adjust size for perspective (if required) */
			if ( perspective_scale(&pscale) )
				{
				width  *= pscale;
				height *= pscale;
				}

			/* Set locations based on justification */
			if ( same(VectorDef.speed_just, JustifyLeft) )
				{
				spd_llpoint[X] = VectorDef.x_spd;
				spd_llpoint[Y] = VectorDef.y_spd;
				spd_urpoint[X] = VectorDef.x_spd + width;
				spd_urpoint[Y] = VectorDef.y_spd + height;
				}
			else if ( same(VectorDef.speed_just, JustifyCentre) )
				{
				spd_llpoint[X] = VectorDef.x_spd - width/2.0;
				spd_llpoint[Y] = VectorDef.y_spd;
				spd_urpoint[X] = VectorDef.x_spd + width/2.0;
				spd_urpoint[Y] = VectorDef.y_spd + height;
				}
			else if ( same(VectorDef.speed_just, JustifyRight) )
				{
				spd_llpoint[X] = VectorDef.x_spd - width;
				spd_llpoint[Y] = VectorDef.y_spd;
				spd_urpoint[X] = VectorDef.x_spd;
				spd_urpoint[Y] = VectorDef.y_spd + height;
				}
			else
				{
				spd_llpoint[X] = VectorDef.x_spd;
				spd_llpoint[Y] = VectorDef.y_spd;
				spd_urpoint[X] = VectorDef.x_spd + width;
				spd_urpoint[Y] = VectorDef.y_spd + height;
				}
			}

		/* Initialize locations from first parameter used */
		/*  ... direction, speed or gust                  */
		if ( isdir )
			{
			(void) copy_point(llpoint, dir_llpoint);
			(void) copy_point(urpoint, dir_urpoint);
			}
		else if ( isspd )
			{
			(void) copy_point(llpoint, spd_llpoint);
			(void) copy_point(urpoint, spd_urpoint);
			}
		else
			{
			(void) copy_point(llpoint, ZeroPoint);
			(void) copy_point(urpoint, ZeroPoint);
			}

		/* Reset locations to include direction and speed parameters */
		if ( isspd )
			{
			llpoint[X] = MIN(llpoint[X], spd_llpoint[X]);
			llpoint[Y] = MIN(llpoint[Y], spd_llpoint[Y]);
			urpoint[X] = MAX(urpoint[X], spd_urpoint[X]);
			urpoint[Y] = MAX(urpoint[Y], spd_urpoint[Y]);
			}
		}

	/* Set default attribute position */
	xx = attrib->x_off;
	yy = attrib->y_off;

	/* Use default position if attribute is not "anchored" to another */
	if ( same(attrib->anchor, AttribAnchorNone) )
		{
		}

	/* Adjust attribute position using "attribute_anchor" and "attribute_ref" */
	else
		{

		/* Get attribute to use as "anchor" location */
		temp_attrib = get_attribute_display(attrib->anchor);

		/* Adjust attribute position wrt position on "anchor" attribute */
		if ( same(attrib->ref, AnchorLowerLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperLeft) )
			{
			xx += temp_attrib.x_left;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperCentre) )
			{
			xx += temp_attrib.x_centre;
			yy += temp_attrib.y_top;
			}
		else if ( same(attrib->ref, AnchorLowerRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_bottom;
			}
		else if ( same(attrib->ref, AnchorCentreRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_centre;
			}
		else if ( same(attrib->ref, AnchorUpperRight) )
			{
			xx += temp_attrib.x_right;
			yy += temp_attrib.y_top;
			}
		}

	/* Set horizontal attribute placement */
	attrib->x_attrib = xx;
	attrib->x_left   = xx + llpoint[X];
	attrib->x_right  = xx + urpoint[X];
	attrib->x_centre = (attrib->x_left + attrib->x_right) / 2.0;

	/* Set vertical attribute placement (centred) */
	attrib->y_attrib = yy;
	attrib->y_top    = yy + urpoint[Y];
	attrib->y_bottom = yy + llpoint[Y];
	attrib->y_centre = (attrib->y_top + attrib->y_bottom) / 2.0;
	}

ATTRIB_DISPLAY	full_attribute_placement

	(
	)

	{
	int				ii;
	float			x_left, x_right, y_top, y_bottom;
	ATTRIB_DISPLAY	attrib;

	/* Initialize the full attribute display parameters */
	(void) copy_attribute_display(&FullAttribDisplay, &DefaultAttribDisplay);

	/* Determine the minimum and maximum dimensions from all the attributes */
	x_left   = 0.0;
	x_right  = 0.0;
	y_top    = 0.0;
	y_bottom = 0.0;
	for ( ii=0; ii<NumAttribDisplay; ii++ )
		{
		attrib = AttribDisplay[ii];
		if ( attrib.x_left   < x_left   ) x_left   = attrib.x_left;
		if ( attrib.x_right  > x_right  ) x_right  = attrib.x_right;
		if ( attrib.y_top    > y_top    ) y_top    = attrib.y_top;
		if ( attrib.y_bottom < y_bottom ) y_bottom = attrib.y_bottom;
		}

	/* Reset the full attribute display parameters */
	FullAttribDisplay.x_left   = x_left;
	FullAttribDisplay.x_centre = (x_left + x_right) / 2.0;
	FullAttribDisplay.x_right  = x_right;
	FullAttribDisplay.y_top    = y_top;
	FullAttribDisplay.y_centre = (y_top + y_bottom) / 2.0;
	FullAttribDisplay.y_bottom = y_bottom;

	/* Return the full attribute display */
	return FullAttribDisplay;
	}

LOGICAL			fit_attribute_placement

	(
	ATTRIB_DISPLAY	attrib,
	STRING			fit_to_map_ref,
	float			rotation,
	float			xorig,
	float			yorig,
	float			xpos,
	float			ypos,
	float			*xadj,
	float			*yadj,
	float			*rotadj
	)

	{
	LOGICAL			fit_left = TRUE, fit_right = TRUE;
	LOGICAL			fit_top = TRUE, fit_bottom = TRUE;
	float			xshft = 0.0, yshft = 0.0;
	float			xleft, xright, ytop, ybottom, xoff, yoff, width, height;
	LABEL_DISPLAY	label_display;
	char			err_buf[GPGLong];

	/* Initialize the return parameters */
	if ( NotNull(xadj) )   *xadj   = xpos;
	if ( NotNull(yadj) )   *yadj   = ypos;
	if ( NotNull(rotadj) ) *rotadj = 0.0;

	/* First check placement for four corners of rotated attribute */
	xleft   = attrib.x_left;
	xright  = attrib.x_right;
	ytop    = attrib.y_top;
	ybottom = attrib.y_bottom;
	(void) determine_attribute_offset(xpos, ypos, rotation, xleft, ybottom,
			&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
	(void) determine_attribute_offset(xpos, ypos, rotation, xleft, ytop,
			&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
	(void) determine_attribute_offset(xpos, ypos, rotation, xright, ytop,
			&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
	(void) determine_attribute_offset(xpos, ypos, rotation, xright, ybottom,
			&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);

	/* Next check placement for four corners of label display */
	if ( !blank(attrib.display_name) )
		{

		/* Get the named label display */
		label_display = get_label_display(attrib.display_name);

		/* Set the dimensions for sized box or ellipse or underline displays */
		if ( same(attrib.display_type, LabelSizedBox)
					|| same(attrib.display_type, LabelSizedEllipse)
					|| same(attrib.display_type, LabelSizedUnderline) )
			{
			xleft   = attrib.x_left
						+ label_display.x_off - label_display.margin_left;
			xright  = attrib.x_right
						+ label_display.x_off + label_display.margin_right;
			ytop    = attrib.y_top
						+ label_display.y_off + label_display.margin_top;
			ybottom = attrib.y_bottom
						+ label_display.y_off - label_display.margin_bottom;
			}

		/* Set the dimensions for fixed box or ellipse displays */
		else if ( same(attrib.display_type, LabelFixedBox)
					|| same(attrib.display_type, LabelFixedEllipse) )
			{
			xleft   = attrib.x_attrib
						+ label_display.x_off - label_display.width/2.0;
			xright  = attrib.x_attrib
						+ label_display.x_off + label_display.width/2.0;
			ytop    = attrib.y_attrib
						+ label_display.y_off + label_display.height/2.0;
			ybottom = attrib.y_attrib
						+ label_display.y_off - label_display.height/2.0;
			}

		/* Set the dimensions for fixed underline displays */
		else if ( same(attrib.display_type, LabelFixedUnderline) )
			{
			xleft   = attrib.x_attrib
						+ label_display.x_off - label_display.width/2.0;
			xright  = attrib.x_attrib
						+ label_display.x_off + label_display.width/2.0;
			ytop    = attrib.y_attrib + label_display.y_off;
			ybottom = attrib.y_attrib + label_display.y_off;
			}

		/* Check placement for four corners of label display */
		(void) determine_attribute_offset(xpos, ypos, rotation, xleft, ybottom,
				&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
		(void) determine_attribute_offset(xpos, ypos, rotation, xleft, ytop,
				&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
		(void) determine_attribute_offset(xpos, ypos, rotation, xright, ytop,
				&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
		(void) determine_attribute_offset(xpos, ypos, rotation, xright, ybottom,
				&fit_left, &fit_right, &fit_top, &fit_bottom, &xshft, &yshft);
		}

	/* Return TRUE if attribute fits map */
	if ( fit_left && fit_right && fit_top && fit_bottom ) return TRUE;

	/* Return FALSE if attribute cannot be fit to map */
	if ( !fit_left && !fit_right ) return FALSE;
	if ( !fit_top && !fit_bottom ) return FALSE;

	/* Set offset parameters based on attributes alone */
	xoff   = xpos - xorig;
	yoff   = ypos - yorig;
	width  = attrib.x_right - attrib.x_left;
	height = attrib.y_top - attrib.y_bottom;

	/* Adjust offset parameters for label displays             */
	/* Note that rotated attributes have no special placement! */
	if ( !blank(attrib.display_name) )
		{

		/* Reset offsets using label display offsets */
		xoff += label_display.x_off;
		yoff += label_display.y_off;

		/* Adjust width and height by margins              */
		/*  for sized box or ellipse or underline displays */
		if ( same(attrib.display_type, LabelSizedBox)
					|| same(attrib.display_type, LabelSizedEllipse)
					|| same(attrib.display_type, LabelSizedUnderline) )
			{
			width  += label_display.margin_left;
			width  += label_display.margin_right;
			height += label_display.margin_top;
			height += label_display.margin_bottom;
			}

		/* Set width and height for fixed box or ellipse displays */
		else if ( same(attrib.display_type, LabelFixedBox)
					|| same(attrib.display_type, LabelFixedEllipse) )
			{
			width  = label_display.width;
			height = label_display.height;
			}

		/* Set width for fixed underline displays */
		else if ( same(attrib.display_type, LabelFixedUnderline) )
			{
			width  = label_display.width;
			}
		}

	/* Reset attribute placement for no special placement */
	if ( same(fit_to_map_ref, FitToMapRefNone) )
		{

		/* Adjust x and y position ... rotation unchanged */
		if ( NotNull(xadj) )   *xadj   = xpos - xshft;
		if ( NotNull(yadj) )   *yadj   = ypos - yshft;
		return TRUE;
		}

	/* Reset attribute placement for "left" placement */
	else if ( same(fit_to_map_ref, FitToMapRefLeft) )
		{

		/* Error if attribute is off "left" of map! */
		if ( !fit_left ) return FALSE;

		/* Adjust y position (if required) */
		if ( !fit_top || !fit_bottom )
			{
			if ( NotNull(yadj) )   *yadj   = ypos - yshft;
			}

		/* Adjust x position and flip rotation (if required) */
		if ( !fit_right )
			{
			if ( NotNull(xadj) )   *xadj   = xorig - xoff - width;
			if ( NotNull(rotadj) ) *rotadj = rotation + 180.0;
			}

		/* Return TRUE */
		return TRUE;
		}

	/* Reset attribute placement for "right" placement */
	else if ( same(fit_to_map_ref, FitToMapRefRight) )
		{

		/* Error if attribute is off "right" of map! */
		if ( !fit_right ) return FALSE;

		/* Adjust y position (if required) */
		if ( !fit_top || !fit_bottom )
			{
			if ( NotNull(yadj) )   *yadj   = ypos - yshft;
			}

		/* Adjust x position and flip rotation (if required) */
		if ( !fit_left )
			{
			if ( NotNull(xadj) )   *xadj   = xorig - xoff + width;
			if ( NotNull(rotadj) ) *rotadj = rotation + 180.0;
			}

		/* Return TRUE */
		return TRUE;
		}

	/* Reset attribute placement for "upper" placement */
	else if ( same(fit_to_map_ref, FitToMapRefUpper) )
		{

		/* Error if attribute is off "top" of map! */
		if ( !fit_top ) return FALSE;

		/* Adjust x position (if required) */
		if ( !fit_left || !fit_right )
			{
			if ( NotNull(xadj) )   *xadj   = xpos - xshft;
			}

		/* Adjust y position and flip rotation (if required) */
		if ( !fit_bottom )
			{
			if ( NotNull(yadj) )   *yadj   = yorig - yoff + height;
			if ( NotNull(rotadj) ) *rotadj = rotation + 180.0;
			}

		/* Return TRUE */
		return TRUE;
		}

	/* Reset attribute placement for "lower" placement */
	else if ( same(fit_to_map_ref, FitToMapRefLower) )
		{

		/* Error if attribute is off "bottom" of map! */
		if ( !fit_bottom ) return FALSE;

		/* Adjust x position (if required) */
		if ( !fit_left || !fit_right )
			{
			if ( NotNull(xadj) )   *xadj   = xpos - xshft;
			}

		/* Adjust y position and flip rotation (if required) */
		if ( !fit_top )
			{
			if ( NotNull(yadj) )   *yadj   = yorig - yoff - height;
			if ( NotNull(rotadj) ) *rotadj = rotation + 180.0;
			}

		/* Return TRUE */
		return TRUE;
		}

	/* Error message for unrecognized fit to map type! */
	(void) sprintf(err_buf, "Unrecognized fit to map type ... %s",
			fit_to_map_ref);
	(void) error_report(err_buf);
	}

int				return_attribute_display

	(
	ATTRIB_DISPLAY	**attribs
	)

	{

	/* Return the current list of attribute displays */
	if ( NotNull(attribs) ) *attribs = AttribDisplay;
	return NumAttribDisplay;
	}

/***********************************************************************
*                                                                      *
*    a d d _ d i s t a n c e _ s c a l e                               *
*    g e t _ d i s t a n c e _ s c a l e                               *
*    d i s t a n c e _ s c a l e _ l i n e                             *
*    a d d _ d i s t a n c e _ s c a l e _ l o c a t i o n             *
*    f r e e _ d i s t a n c e _ s c a l e _ l o c a t i o n s         *
*                                                                      *
***********************************************************************/

/* Storage for parameters for displaying distance scales */
static	int				NumScaleDisplay =  0;
static	SCALE_DISPLAY	*ScaleDisplay   = NullPtr(SCALE_DISPLAY *);
static	int				NumScaleLctns   = 0;
static	SCALE_LCTNS		*ScaleLctns     = NullPtr(SCALE_LCTNS *);

void			add_distance_scale

	(
	STRING		scale_name,
	float		slength,
	STRING		sjust,
	float		srotate,
	float		xoff,
	float		yoff
	)

	{
	SCALE_DISPLAY	*Dscale;
	int				ii;
	float			xx, yy;
	LINE			line;
	char			err_buf[GPGLong];

	/* Check that distance scale has not already been defined */
	for ( ii=0; ii<NumScaleDisplay; ii++ )
		{
		if ( same(scale_name, ScaleDisplay[ii].name) ) break;
		}

	/* Re-define the distance scale parameters */
	if (ii < NumScaleDisplay)
		{
		Dscale = &ScaleDisplay[ii];
		(void) destroy_line(Dscale->sline);
		(void) sprintf(err_buf,
				"Re-defining parameters for distance scale: %s", scale_name);
		(void) warn_report(err_buf);
		}

	/* Add another distance scale to the list */
	else
		{
		NumScaleDisplay++;
		ScaleDisplay = GETMEM(ScaleDisplay, SCALE_DISPLAY, NumScaleDisplay);
		Dscale = &ScaleDisplay[NumScaleDisplay-1];
		}

	/* Set position for distance scale based on current anchor position */
	(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);

	/* Convert length of distance scale into an offset and rotated line */
	line = distance_scale_line(slength, sjust, srotate, xx, yy);

	/* Save the distance scale parameters */
	(void) strcpy(Dscale->name, scale_name);
	Dscale->slength = slength;
	Dscale->srotate = srotate;
	Dscale->sline   = line;
	return;
	}

SCALE_DISPLAY	*get_distance_scale

	(
	STRING		scale_name
	)

	{
	int				ii;

	/* Return distance scale with matching name */
	for ( ii=0; ii<NumScaleDisplay; ii++ )
		{
		if ( same(scale_name, ScaleDisplay[ii].name) )
			{

			/* Return the distance scale */
			return &ScaleDisplay[ii];
			}
		}

	/* Error return if distance scale not found */
	return NullPtr(SCALE_DISPLAY *);
	}

LINE			distance_scale_line

	(
	float		slength,
	STRING		sjust,
	float		srotate,
	float		xx,
	float		yy
	)

	{
	float			map_units, xbgn, ybgn, xend, yend;
	double			dlength, hlength;
	POINT			pos, bpos, epos;
	LINE			line;

	/* Convert length of distance scale to display units on current map */
	map_units = BaseMap.definition.units;
	dlength = (double) slength * (double) Uscale / (double) map_units;
	hlength = dlength / 2.0;

	/* Set distance scale start/end based on justification */
	if ( same(sjust, JustifyLeft) )
		{
		xbgn = xx;
		xend = xx + (float) dlength;
		ybgn = yend = yy;
		}
	else if ( same(sjust, JustifyCentre) )
		{
		xbgn = xx - (float) hlength;
		xend = xx + (float) hlength;
		ybgn = yend = yy;
		}
	else if ( same(sjust, JustifyRight) )
		{
		xbgn = xx - (float) dlength;
		xend = xx;
		ybgn = yend = yy;
		}
	else
		{
		xbgn = xx;
		xend = xx + (float) dlength;
		ybgn = yend = yy;
		}
	(void) set_point(pos,  xx,   yy);
	(void) set_point(bpos, xbgn, ybgn);
	(void) set_point(epos, xend, yend);

	/* Turn distance scale into a line and rotate it (if required) */
	line = create_line();
	(void) add_point_to_line(line, bpos);
	(void) add_point_to_line(line, epos);
	(void) rotate_line(line, pos, srotate);

	/* Return the offset and rotated line */
	return line;
	}

int			add_distance_scale_location

	(
	SCALE_LCTNS	**scale_lctns,
	int			num_lctns
	)

	{
	int			nl;

	/* Return if space is already available */
	if ( num_lctns <= NumScaleLctns ) return NumScaleLctns;

	/* Add another distance scale location buffer */
	nl = NumScaleLctns++;
	ScaleLctns = GETMEM(ScaleLctns, SCALE_LCTNS, NumScaleLctns);

	/* Set default parameters */
	ScaleLctns[nl].lctn = 0.0;
	(void) strcpy(ScaleLctns[nl].label, FpaCblank);

	/* Return the number of distance scale location buffers */
	if ( NotNull(scale_lctns) ) *scale_lctns = ScaleLctns;
	return NumScaleLctns;
	}

void		free_distance_scale_locations

	(
	)

	{

	/* Return if no distance scale location buffers to free */
	if ( NumScaleLctns <= 0 ) return;

	/* Free space used by distance scale location buffers */
	FREEMEM(ScaleLctns);

	/* Reset the number of distance scale location buffers */
	NumScaleLctns = 0;
	}

/***********************************************************************
*                                                                      *
*    c o p y _ s y m b o l _ f i l l                                   *
*    a d d _ s y m b o l _ f i l l                                     *
*    g e t _ s y m b o l _ f i l l                                     *
*                                                                      *
***********************************************************************/

/* Storage for parameters for filling areas with symbols */
static	SYMBOL_FILL		DefaultSymbolFill = NO_SYMBOL_FILL;
static	SYMBOL_FILL		*SymbolFill       = NullPtr(SYMBOL_FILL *);
static	int				NumSymbolFill     =  0;
static	int				CurSymbolFill     = -1;

void			copy_symbol_fill

	(
	SYMBOL_FILL		*outsfill,
	SYMBOL_FILL		*insfill
	)

	{

	/* Return immediately if missing input or output symbol fill */
	if ( IsNull(insfill) || IsNull(outsfill) ) return;

	/* Copy the symbol fill parameters */
	(void) strcpy(outsfill->name,   insfill->name);
	(void) copy_presentation(&outsfill->presentation, &insfill->presentation);
	(void) strcpy(outsfill->symbol, insfill->symbol);
	outsfill->sym_scale  = insfill->sym_scale;
	outsfill->sym_rotate = insfill->sym_rotate;
	outsfill->rep_rotate = insfill->rep_rotate;
	outsfill->x_off      = insfill->x_off;
	outsfill->y_off      = insfill->y_off;
	outsfill->x_repeat   = insfill->x_repeat;
	outsfill->y_repeat   = insfill->y_repeat;
	outsfill->x_shift    = insfill->x_shift;
	outsfill->y_shift    = insfill->y_shift;
	}

SYMBOL_FILL		*add_symbol_fill

	(
	STRING		name
	)

	{
	int			ii, ne;

	/* Search the list for the named symbol fill */
	for ( ii=0; ii<NumSymbolFill; ii++ )
		{

		/* Return the named symbol fill (if found) */
		if ( same(SymbolFill[ii].name, name) ) return &SymbolFill[ii];
		}

	/* Add another symbol fill to the list */
	NumSymbolFill++;
	SymbolFill = GETMEM(SymbolFill, SYMBOL_FILL, NumSymbolFill);
	ne = NumSymbolFill - 1;

	/* Set default symbol fill name and parameters */
	(void) copy_symbol_fill(&SymbolFill[ne], &DefaultSymbolFill);
	(void) strcpy(SymbolFill[ne].name, name);
	(void) copy_presentation(&(SymbolFill[ne].presentation), &PresDef);

	/* Return the new symbol fill */
	return &SymbolFill[ne];
	}

SYMBOL_FILL		get_symbol_fill

	(
	STRING		name
	)

	{
	int			ii;
	SYMBOL_FILL	temp_symfill;
	char		err_buf[GPGLong];

	/* Check if the name corresponds to the current symbol fill */
	if ( CurSymbolFill >= 0
			&& same(name, SymbolFill[CurSymbolFill].name) )
		return SymbolFill[CurSymbolFill];

	/* Use the symbol fill corresponding to this name */
	for ( ii=0; ii<NumSymbolFill; ii++ )
		{
		if ( same(SymbolFill[ii].name, name) )
			{
			CurSymbolFill = ii;
			return SymbolFill[CurSymbolFill];
			}
		}

	/* Use default symbol fill if name not found */
	(void) sprintf(err_buf, "Missing named symbol fill for: %s", name);
	(void) warn_report(err_buf);
	(void) copy_symbol_fill(&temp_symfill, &DefaultSymbolFill);
	(void) copy_presentation(&temp_symfill.presentation, &PresDef);
	return temp_symfill;
	}

/***********************************************************************
*                                                                      *
*    d e f i n e _ s o u r c e                                         *
*                                                                      *
***********************************************************************/

LOGICAL		define_source

	(
	STRING		sname,
	STRING		valid_time
	)

	{
	float					clon;
	STRING					vtime;
	FpaConfigSourceStruct	*sdef;

	static	char	CurSname[GPGMedium]   = FpaCblank;
	static	char	CurValidTime[GPGTiny] = FpaCblank;

	/* Check the source */
	sdef = identify_source(sname, FpaCblank);
	if ( IsNull(sdef) )
		{
		(void) error_report("Invalid data source");
		}

	/* Return now if source and valid time have not changed */
	if ( same_ic(CurSname, sname)
			&& same(CurValidTime, valid_time) ) return TRUE;

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Get the valid timestamp based on format of valid_time */
	vtime = interpret_timestring(valid_time, T0stamp, clon);
	if ( IsNull(vtime) )
		{
		(void) error_report("Invalid valid_time string");
		}

	/* Save the source name, valid time, and valid timestamp */
	(void) strcpy(CurSname,     sname);
	(void) strcpy(CurSource,    sname);
	(void) strcpy(CurValidTime, valid_time);
	(void) strcpy(TVstamp,      vtime);

	/* Reset the default field descriptor */
	/* Note that not setting the run timestamp will access */
	/*  data in the most recent guidance directory         */
	(void) set_fld_descript(&Fdesc, FpaF_DIRECTORY_PATH, FpaCblank,
									FpaF_SOURCE_NAME,    CurSource,
									FpaF_RUN_TIME,       FpaCblank,
									FpaF_ELEMENT_NAME,   FpaCblank,
									FpaF_LEVEL_NAME,     FpaCblank,
									FpaF_VALID_TIME,     TVstamp,
									FpaF_END_OF_LIST);

	/* Return TRUE if all went well */
	(void) fprintf(stdout, " Setting source to: \"%s\" at: \"%s\"\n",
			sname, vtime);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d e f i n e _ p s m e t _ u n i t s                               *
*                                                                      *
*    d e f i n e _ s v g m e t _ u n i t s                             *
*                                                                      *
*    d e f i n e _ c o r m e t _ u n i t s                             *
*                                                                      *
***********************************************************************/

LOGICAL		define_psmet_units

	(
	STRING		type,
	float		scale_factor
	)

	{
	float		sfactor;
	char		err_buf[GPGLong];

	/* Set the scale factor */
	sfactor = scale_factor / 100.0;

	/* Now set the postscript conversion factor based on the type of units */
	if ( same(type, DisplayUnitsInches) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsInches);
		DisplayUnits.conversion = 72.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsCM) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsCM);
		DisplayUnits.conversion = 72.0 / 2.54 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsMM) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsMM);
		DisplayUnits.conversion = 72.0 / 25.4 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsPicas) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsPicas);
		DisplayUnits.conversion = 72.0 / 6.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsPoints) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsPoints);
		DisplayUnits.conversion = 1.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	/* Error return for incorrect units type */
	else
		{
		(void) sprintf(err_buf, "Recognized units types are: %s %s %s %s %s",
				DisplayUnitsInches, DisplayUnitsCM, DisplayUnitsMM,
				DisplayUnitsPicas, DisplayUnitsPoints);
		(void) error_report(err_buf);
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, " Adjusted display units ... %s times %f",
				DisplayUnits.type, DisplayUnits.conversion);
		(void) fprintf(stdout, "  scaled by %f\n", DisplayUnits.sfactor);
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_svgmet_units

	(
	STRING		type,
	float		scale_factor
	)

	{
	float		sfactor;
	char		err_buf[GPGLong];

	/* Set the scale factor */
	sfactor = scale_factor / 100.0;

	/* Now set the svg conversion factor based on the type of units */
	if ( same(type, DisplayUnitsInches) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsInches);
		DisplayUnits.conversion = 72.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsCM) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsCM);
		DisplayUnits.conversion = 72.0 / 2.54 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsMM) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsMM);
		DisplayUnits.conversion = 72.0 / 25.4 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsPicas) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsPicas);
		DisplayUnits.conversion = 72.0 / 6.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsPoints) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsPoints);
		DisplayUnits.conversion = 1.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	/* Error return for incorrect units type */
	else
		{
		(void) sprintf(err_buf, "Recognized units types are: %s %s %s %s %s",
				DisplayUnitsInches, DisplayUnitsCM, DisplayUnitsMM,
				DisplayUnitsPicas, DisplayUnitsPoints);
		(void) error_report(err_buf);
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, " Adjusted display units ... %s times %f",
				DisplayUnits.type, DisplayUnits.conversion);
		(void) fprintf(stdout, "  scaled by %f\n", DisplayUnits.sfactor);
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_cormet_units

	(
	STRING		type,
	float		scale_factor
	)

	{
	float		sfactor;
	char		err_buf[GPGLong];

	/* Set the scale factor */
	sfactor = scale_factor / 100.0;

	/* Now set the conversion factor based on the type of units */
	if ( same(type, DisplayUnitsInches) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsInches);
		DisplayUnits.conversion = 1000 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsCM) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsCM);
		DisplayUnits.conversion = 1000 / 2.54 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsMM) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsMM);
		DisplayUnits.conversion = 1000 / 25.4 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsPicas) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsPicas);
		DisplayUnits.conversion = 1000 / 6.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	else if ( same(type, DisplayUnitsPoints) )
		{
		(void) strcpy(DisplayUnits.type, DisplayUnitsPoints);
		DisplayUnits.conversion = 1000 / 72.0 * sfactor;
		DisplayUnits.sfactor    = sfactor;
		}

	/* Error return for incorrect units type */
	else
		{
		(void) sprintf(err_buf, "Recognized units types are: %s %s %s %s %s",
				DisplayUnitsInches, DisplayUnitsCM, DisplayUnitsMM,
				DisplayUnitsPicas, DisplayUnitsPoints);
		(void) error_report(err_buf);
		}

	if ( Verbose )
		{
		(void) fprintf(stdout, " Adjusted display units ... %s times %f",
				DisplayUnits.type, DisplayUnits.conversion);
		(void) fprintf(stdout, "  scaled by %f\n", DisplayUnits.sfactor);
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d e f i n e _ p s m e t _ p l a c e m e n t                       *
*                                                                      *
*    d e f i n e _ s v g m e t _ p l a c e m e n t                     *
*                                                                      *
*    d e f i n e _ c o r m e t _ p l a c e m e n t                     *
*                                                                      *
***********************************************************************/

LOGICAL		define_psmet_placement

	(
	float		map_scale,
	float		size,
	STRING		axis,
	float		xoffset,
	float		yoffset
	)

	{
	float		xlen, ylen, map_units;
	char		err_buf[GPGLong];

	/* Set dimensions from current basemap */
	xlen      = BaseMap.definition.xlen;
	ylen      = BaseMap.definition.ylen;
	map_units = BaseMap.definition.units;
	if ( xlen <= 0.0 || ylen <= 0.0 || map_units <= 0.0 ) return FALSE;

	/* Return FALSE for missing map scale or size */
	if ( map_scale <= 0.0 && size <= 0.0 ) return FALSE;

	/* Set scaling factor for given map scale */
	if ( map_scale > 0.0 )
		{

		/* Set scaling factor for given map in inches per map unit */
		/* Note that "map_units" is in m                           */
		Uscale  = map_units / map_scale;
		Uscale /= 0.0254;

		/* Warning message for oversized maps */
		if ( Uscale*xlen > MaxSize || Uscale*ylen > MaxSize )
			{
			(void) sprintf(err_buf,
					"Map size larger than %f inches for map scale: %f",
					MaxSize, map_scale);
			(void) warn_report(err_buf);
			}

		/* Warning message for undersized maps */
		if ( Uscale*xlen < MinSize || Uscale*ylen < MinSize )
			{
			(void) sprintf(err_buf,
					"Map size smaller than %f inches for map scale: %f",
					MinSize, map_scale);
			(void) warn_report(err_buf);
			}

		/* Set scaling factor for given map in points per map unit */
		Uscale *= 72.0;
		}

	/* Set scaling factor for given map size and axis */
	else
		{

		/* Scale the map to the x-axis */
		if ( same(axis, ScaleXaxis) )
			{
			Uscale = size / xlen;
			}

		/* Scale the map to the y-axis */
		else if ( same(axis, ScaleYaxis) )
			{
			Uscale = size / ylen;
			}

		/* Scale the map to the longest axis */
		else if ( same(axis, ScaleLongest) )
			{
			if ( xlen >= ylen )
				Uscale = size / xlen;
			else
				Uscale = size / ylen;
			}

		/* Scale the map to the shortest axis */
		else if ( same(axis, ScaleShortest) )
			{
			if ( xlen <= ylen )
				Uscale = size / xlen;
			else
				Uscale = size / ylen;
			}

		/* Error return for incorrect axis to scale parameter */
		else
			{
			(void) sprintf(err_buf, "Recognized axis types are: %s %s %s %s",
					ScaleXaxis, ScaleYaxis, ScaleLongest, ScaleShortest);
			(void) error_report(err_buf);
			}
		}

	/* Set x and y map offsets of centre of map wrt centre of page */
	/*  based on current anchor location                           */
	MapCentreOffSet[X] = XYpoint[X] + (double) xoffset;
	MapCentreOffSet[Y] = XYpoint[Y] + (double) yoffset;

	/* Set width and height of map */
	MapWidth  = (double) xlen * (double) Uscale;
	MapHeight = (double) ylen * (double) Uscale;
	HalfMapWidth  = MapWidth  / 2.0;
	HalfMapHeight = MapHeight / 2.0;

	/* Set upper left corner of map wrt centre of page */
	ULpoint[X] = -HalfMapWidth  + MapCentreOffSet[X];
	ULpoint[Y] =  HalfMapHeight + MapCentreOffSet[Y];

	/* Set lower right corner of map wrt centre of page */
	LRpoint[X] =  HalfMapWidth  + MapCentreOffSet[X];
	LRpoint[Y] = -HalfMapHeight + MapCentreOffSet[Y];

	/* Set map origin from lower left corner of map */
	Xorigin = (float) ULpoint[X];
	Yorigin = (float) LRpoint[Y];

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_svgmet_placement

	(
	float		map_scale,
	float		size,
	STRING		axis,
	float		xoffset,
	float		yoffset
	)

	{
	float		xlen, ylen, map_units;
	char		err_buf[GPGLong];

	/* Set dimensions from current basemap */
	xlen      = BaseMap.definition.xlen;
	ylen      = BaseMap.definition.ylen;
	map_units = BaseMap.definition.units;
	if ( xlen <= 0.0 || ylen <= 0.0 || map_units <= 0.0 ) return FALSE;

	/* Return FALSE for missing map scale or size */
	if ( map_scale <= 0.0 && size <= 0.0 ) return FALSE;

	/* Set scaling factor for given map scale */
	if ( map_scale > 0.0 )
		{

		/* Set scaling factor for given map in inches per map unit */
		/* Note that "map_units" is in m                           */
		Uscale  = map_units / map_scale;
		Uscale /= 0.0254;

		/* Warning message for oversized maps */
		if ( Uscale*xlen > MaxSize || Uscale*ylen > MaxSize )
			{
			(void) sprintf(err_buf,
					"Map size larger than %f inches for map scale: %f",
					MaxSize, map_scale);
			(void) warn_report(err_buf);
			}

		/* Warning message for undersized maps */
		if ( Uscale*xlen < MinSize || Uscale*ylen < MinSize )
			{
			(void) sprintf(err_buf,
					"Map size smaller than %f inches for map scale: %f",
					MinSize, map_scale);
			(void) warn_report(err_buf);
			}

		/* Set scaling factor for given map in points per map unit */
		Uscale *= 72.0;
		}

	/* Set scaling factor for given map size and axis */
	else
		{

		/* Scale the map to the x-axis */
		if ( same(axis, ScaleXaxis) )
			{
			Uscale = size / xlen;
			}

		/* Scale the map to the y-axis */
		else if ( same(axis, ScaleYaxis) )
			{
			Uscale = size / ylen;
			}

		/* Scale the map to the longest axis */
		else if ( same(axis, ScaleLongest) )
			{
			if ( xlen >= ylen )
				Uscale = size / xlen;
			else
				Uscale = size / ylen;
			}

		/* Scale the map to the shortest axis */
		else if ( same(axis, ScaleShortest) )
			{
			if ( xlen <= ylen )
				Uscale = size / xlen;
			else
				Uscale = size / ylen;
			}

		/* Error return for incorrect axis to scale parameter */
		else
			{
			(void) sprintf(err_buf, "Recognized axis types are: %s %s %s %s",
					ScaleXaxis, ScaleYaxis, ScaleLongest, ScaleShortest);
			(void) error_report(err_buf);
			}
		}

	/* Set x and y map offsets of centre of map wrt centre of page */
	/*  based on current anchor location                           */
	MapCentreOffSet[X] = XYpoint[X] + (double) xoffset;
	MapCentreOffSet[Y] = XYpoint[Y] + (double) yoffset;

	/* Set width and height of map */
	MapWidth  = (double) xlen * (double) Uscale;
	MapHeight = (double) ylen * (double) Uscale;
	HalfMapWidth  = MapWidth  / 2.0;
	HalfMapHeight = MapHeight / 2.0;

	/* Set upper left corner of map wrt centre of page */
	ULpoint[X] = -HalfMapWidth  + MapCentreOffSet[X];
	ULpoint[Y] =  HalfMapHeight + MapCentreOffSet[Y];

	/* Set lower right corner of map wrt centre of page */
	LRpoint[X] =  HalfMapWidth  + MapCentreOffSet[X];
	LRpoint[Y] = -HalfMapHeight + MapCentreOffSet[Y];

	/* Set map origin from lower left corner of map */
	Xorigin = (float) ULpoint[X];
	Yorigin = (float) LRpoint[Y];

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_cormet_placement

	(
	float		map_scale,
	float		size,
	STRING		axis,
	float		xoffset,
	float		yoffset
	)

	{
	float		xlen, ylen, map_units;
	char		err_buf[GPGLong];

	/* Set dimensions from current basemap */
	xlen      = BaseMap.definition.xlen;
	ylen      = BaseMap.definition.ylen;
	map_units = BaseMap.definition.units;
	if ( xlen <= 0.0 || ylen <= 0.0 || map_units <= 0.0 ) return FALSE;

	/* Return FALSE for missing map scale or size */
	if ( map_scale <= 0.0 && size <= 0.0 ) return FALSE;

	/* Set scaling factor for given map scale */
	if ( map_scale > 0.0 )
		{

		/* Set scaling factor for given map in inches per map unit */
		/* Note that "map_units" is in m                           */
		Uscale  = map_units / map_scale;
		Uscale /= 0.0254;

		/* Warning message for oversized maps */
		if ( Uscale*xlen > MaxSize || Uscale*ylen > MaxSize )
			{
			(void) sprintf(err_buf,
					"Map size larger than %f inches for map scale: %f",
					MaxSize, map_scale);
			(void) warn_report(err_buf);
			}

		/* Warning message for undersized maps */
		if ( Uscale*xlen < MinSize || Uscale*ylen < MinSize )
			{
			(void) sprintf(err_buf,
					"Map size smaller than %f inches for map scale: %f",
					MinSize, map_scale);
			(void) warn_report(err_buf);
			}

		/* Set scaling factor for given map in 1000ths of inch per map unit */
		Uscale *= 1000.0;
		}

	/* Set scaling factor for given map size and axis */
	else
		{

		/* Scale the map to the x-axis */
		if ( same(axis, ScaleXaxis) )
			{
			Uscale = size / xlen;
			}

		/* Scale the map to the y-axis */
		else if ( same(axis, ScaleYaxis) )
			{
			Uscale = size / ylen;
			}

		/* Scale the map to the longest axis */
		else if ( same(axis, ScaleLongest) )
			{
			if ( xlen >= ylen )
				Uscale = size / xlen;
			else
				Uscale = size / ylen;
			}

		/* Scale the map to the shortest axis */
		else if ( same(axis, ScaleShortest) )
			{
			if ( xlen <= ylen )
				Uscale = size / xlen;
			else
				Uscale = size / ylen;
			}

		/* Error return for incorrect axis to scale parameter */
		else
			{
			(void) sprintf(err_buf, "Recognized axis types are: %s %s %s %s",
					ScaleXaxis, ScaleYaxis, ScaleLongest, ScaleShortest);
			(void) error_report(err_buf);
			}
		}

	/* Set x and y map offsets of centre of map wrt centre of page */
	/*  based on current anchor location                           */
	MapCentreOffSet[X] = XYpoint[X] + (double) xoffset;
	MapCentreOffSet[Y] = XYpoint[Y] + (double) yoffset;

	/* Set width and height of map */
	MapWidth  = (double) xlen * (double) Uscale;
	MapHeight = (double) ylen * (double) Uscale;
	HalfMapWidth  = MapWidth  / 2.0;
	HalfMapHeight = MapHeight / 2.0;

	/* Set upper left corner of map wrt centre of page */
	ULpoint[X] = -HalfMapWidth  + MapCentreOffSet[X];
	ULpoint[Y] =  HalfMapHeight + MapCentreOffSet[Y];

	/* Set lower right corner of map wrt centre of page */
	LRpoint[X] =  HalfMapWidth  + MapCentreOffSet[X];
	LRpoint[Y] = -HalfMapHeight + MapCentreOffSet[Y];

	/* Set map origin from lower left corner of map */
	Xorigin = (float) ULpoint[X];
	Yorigin = (float) LRpoint[Y];

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    m a p _ s c a l i n g                                             *
*    m a p _ d i m e n s i o n s                                       *
*                                                                      *
***********************************************************************/

float		map_scaling

	(
	)

	{

	/* Return map scaling factor */
	if ( AnchorToMap ) return Uscale;

	/* Return default scaling factor */
	else return 1.0;
	}

void		map_dimensions

	(
	POINT		map_centre,
	float		*map_width,
	float		*map_height
	)

	{

	/* Return dimensions of current map */
	if ( NotNull(map_centre) )
		{
		map_centre[X] = (float) MapCentreOffSet[X];
		map_centre[Y] = (float) MapCentreOffSet[Y];
		}
	if ( NotNull(map_width) )  *map_width  = (float) MapWidth;
	if ( NotNull(map_height) ) *map_height = (float) MapHeight;
	}

/***********************************************************************
*                                                                      *
*    d e f i n e _ p s m e t _ a n c h o r                             *
*                                                                      *
*    d e f i n e _ s v g m e t _ a n c h o r                           *
*                                                                      *
*    d e f i n e _ c o r m e t _ a n c h o r                           *
*                                                                      *
*    d e f i n e _ t e x m e t _ a n c h o r                           *
*                                                                      *
***********************************************************************/

LOGICAL		define_psmet_anchor

	(
	STRING		anchor,
	float		xoff,
	float		yoff,
	float		flat,
	float		flon,
	STRING		xsection_name
	)

	{
	float		pscale, xx, yy;
	POINT		pos;
	char		err_buf[GPGLong];

	/* Ensure that the named cross section has been defined (if required) */
	if ( same_start(anchor, AnchorXsectStart)
			&& IsNull(get_cross_section(xsection_name)) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Set anchor location and define anchor offset based on anchor type */
	/* Note that XYpoint is an offset from the centre of the display!    */
	if ( same(anchor, AnchorMap) )
		{
		(void) strcpy(Anchor, AnchorMap);
		XYpoint[X]  = (double) PageWidth  / 2.0;
		XYpoint[Y]  = (double) PageHeight / 2.0;
		AnchorToMap = TRUE;
		}
	else if ( same(anchor, AnchorMapLatLon) )
		{
		(void) strcpy(Anchor, AnchorMapLatLon);
		(void) ll_to_pos(&BaseMap, flat, flon, pos);
		if ( pos[X] < 0.0 || pos[Y] < 0.0
				|| pos[X] > BaseMap.definition.xlen
				|| pos[Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				(void) sprintf(err_buf, "Lat/Lon: %f %f  off map!", flat, flon);
				(void) warn_report(err_buf);
				}
			}
		/* Determine location on map */
		AnchorToMap = TRUE;
		(void) anchored_location(pos, xoff, yoff, &xx, &yy);
		XYpoint[X]  = (double) xx;
		XYpoint[Y]  = (double) yy;
		/* Save map scale if perspective adjustment used */
		if ( perspective_scale(&pscale) ) PerspectiveScale = pscale;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorAbsolute) )
		{
		(void) strcpy(Anchor, AnchorAbsolute);
		XYpoint[X]  = ((double) PageWidth  / 2.0) + (double) xoff;
		XYpoint[Y]  = ((double) PageHeight / 2.0) + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCurrent) )
		{
		(void) strcpy(Anchor, AnchorCurrent);
		XYpoint[X] += (double) xoff;
		XYpoint[Y] += (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Note that ULpoint/LRpoint are offsets of the UpperLeft/LowerRight */
	/*  corners of the map wrt the centre of the display!                */
	else if ( same(anchor, AnchorLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorLowerLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorCentreLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorUpperLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorLowerCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentre) )
		{
		(void) strcpy(Anchor, AnchorCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorUpperCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerRight) )
		{
		(void) strcpy(Anchor, AnchorLowerRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreRight) )
		{
		(void) strcpy(Anchor, AnchorCentreRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperRight) )
		{
		(void) strcpy(Anchor, AnchorUpperRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Note that ULpoint/LRpoint are offsets of the UpperLeft/LowerRight */
	/*  corners of the cross section wrt the LowerLeft corner!           */
	else if ( same(anchor, AnchorXsectLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectCentreLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectLowerRight) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentreRight) )
		{
		(void) strcpy(Anchor, AnchorXsectCentreRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperRight) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_svgmet_anchor

	(
	STRING		anchor,
	float		xoff,
	float		yoff,
	float		flat,
	float		flon,
	STRING		xsection_name
	)

	{
	float		pscale, xx, yy;
	POINT		pos;
	char		err_buf[GPGLong];

	/* Ensure that the named cross section has been defined (if required) */
	if ( same_start(anchor, AnchorXsectStart)
			&& IsNull(get_cross_section(xsection_name)) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Set anchor location and define anchor offset based on anchor type */
	/* Note that XYpoint is an offset from the centre of the display!    */
	if ( same(anchor, AnchorMap) )
		{
		(void) strcpy(Anchor, AnchorMap);
		XYpoint[X]  = (double) PageWidth  / 2.0;
		XYpoint[Y]  = (double) PageHeight / 2.0;
		AnchorToMap = TRUE;
		}
	else if ( same(anchor, AnchorMapLatLon) )
		{
		(void) strcpy(Anchor, AnchorMapLatLon);
		(void) ll_to_pos(&BaseMap, flat, flon, pos);
		if ( pos[X] < 0.0 || pos[Y] < 0.0
				|| pos[X] > BaseMap.definition.xlen
				|| pos[Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				(void) sprintf(err_buf, "Lat/Lon: %f %f  off map!", flat, flon);
				(void) warn_report(err_buf);
				}
			}
		/* Determine location on map */
		AnchorToMap = TRUE;
		(void) anchored_location(pos, xoff, yoff, &xx, &yy);
		XYpoint[X]  = (double) xx;
		XYpoint[Y]  = (double) yy;
		/* Save map scale if perspective adjustment used */
		if ( perspective_scale(&pscale) ) PerspectiveScale = pscale;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorAbsolute) )
		{
		(void) strcpy(Anchor, AnchorAbsolute);
		XYpoint[X]  = ((double) PageWidth  / 2.0) + (double) xoff;
		XYpoint[Y]  = ((double) PageHeight / 2.0) + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCurrent) )
		{
		(void) strcpy(Anchor, AnchorCurrent);
		XYpoint[X] += (double) xoff;
		XYpoint[Y] += (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Note that ULpoint/LRpoint are offsets of the UpperLeft/LowerRight */
	/*  corners of the map wrt the centre of the display!                */
	else if ( same(anchor, AnchorLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorLowerLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorCentreLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorUpperLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorLowerCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentre) )
		{
		(void) strcpy(Anchor, AnchorCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorUpperCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerRight) )
		{
		(void) strcpy(Anchor, AnchorLowerRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreRight) )
		{
		(void) strcpy(Anchor, AnchorCentreRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperRight) )
		{
		(void) strcpy(Anchor, AnchorUpperRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Note that ULpoint/LRpoint are offsets of the UpperLeft/LowerRight */
	/*  corners of the cross section wrt the LowerLeft corner!           */
	else if ( same(anchor, AnchorXsectLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectCentreLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectLowerRight) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentreRight) )
		{
		(void) strcpy(Anchor, AnchorXsectCentreRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperRight) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_cormet_anchor

	(
	STRING		anchor,
	float		xoff,
	float		yoff,
	float		flat,
	float		flon,
	STRING		xsection_name
	)

	{
	float		pscale, xx, yy;
	POINT		pos;
	char		err_buf[GPGLong];

	/* Ensure that the named cross section has been defined (if required) */
	if ( same_start(anchor, AnchorXsectStart)
			&& IsNull(get_cross_section(xsection_name)) )
		{
		(void) sprintf(err_buf, "Cross Section ... %s ... not yet defined",
				xsection_name);
		(void) error_report(err_buf);
		}

	/* Set anchor location and define anchor offset based on anchor type */
	/* Note that XYpoint is an offset from the centre of the display!    */
	if ( same(anchor, AnchorMap) )
		{
		(void) strcpy(Anchor, AnchorMap);
		XYpoint[X]  = 0.0;
		XYpoint[Y]  = 0.0;
		AnchorToMap = TRUE;
		}
	else if ( same(anchor, AnchorMapLatLon) )
		{
		(void) strcpy(Anchor, AnchorMapLatLon);
		(void) ll_to_pos(&BaseMap, flat, flon, pos);
		if ( pos[X] < 0.0 || pos[Y] < 0.0
				|| pos[X] > BaseMap.definition.xlen
				|| pos[Y] > BaseMap.definition.ylen )
			{
			if ( Verbose )
				{
				(void) sprintf(err_buf, "Lat/Lon: %f %f  off map!", flat, flon);
				(void) warn_report(err_buf);
				}
			}
		/* Determine location on map */
		AnchorToMap = TRUE;
		(void) anchored_location(pos, xoff, yoff, &xx, &yy);
		XYpoint[X]  = (double) xx;
		XYpoint[Y]  = (double) yy;
		/* Save map scale if perspective adjustment used */
		if ( perspective_scale(&pscale) ) PerspectiveScale = pscale;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorAbsolute) )
		{
		(void) strcpy(Anchor, AnchorAbsolute);
		XYpoint[X]  = (double) xoff;
		XYpoint[Y]  = (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCurrent) )
		{
		(void) strcpy(Anchor, AnchorCurrent);
		XYpoint[X] += (double) xoff;
		XYpoint[Y] += (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Note that ULpoint/LRpoint are offsets of the UpperLeft/LowerRight */
	/*  corners of the map wrt the centre of the display!                */
	else if ( same(anchor, AnchorLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorLowerLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorCentreLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorUpperLeft);
		XYpoint[X]  = ULpoint[X] + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorLowerCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentre) )
		{
		(void) strcpy(Anchor, AnchorCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorUpperCentre);
		XYpoint[X]  = (ULpoint[X] + LRpoint[X]) / 2.0 + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerRight) )
		{
		(void) strcpy(Anchor, AnchorLowerRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreRight) )
		{
		(void) strcpy(Anchor, AnchorCentreRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = (ULpoint[Y] + LRpoint[Y]) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperRight) )
		{
		(void) strcpy(Anchor, AnchorUpperRight);
		XYpoint[X]  = LRpoint[X] + (double) xoff;
		XYpoint[Y]  = ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Note that ULpoint/LRpoint are offsets of the UpperLeft/LowerRight */
	/*  corners of the cross section wrt the LowerLeft corner!           */
	else if ( same(anchor, AnchorXsectLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectCentreLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperLeft);
		XYpoint[X] += XSect_ULpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperCentre);
		XYpoint[X] += (XSect_ULpoint[X] + XSect_LRpoint[X]) / 2.0
															+ (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectLowerRight) )
		{
		(void) strcpy(Anchor, AnchorXsectLowerRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_LRpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectCentreRight) )
		{
		(void) strcpy(Anchor, AnchorXsectCentreRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += (XSect_ULpoint[Y] + XSect_LRpoint[Y]) / 2.0
															+ (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorXsectUpperRight) )
		{
		(void) strcpy(Anchor, AnchorXsectUpperRight);
		XYpoint[X] += XSect_LRpoint[X] + (double) xoff;
		XYpoint[Y] += XSect_ULpoint[Y] + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		define_texmet_anchor

	(
	STRING		anchor,
	float		xoff,
	float		yoff,
	float		unused_flat,
	float		unused_flon,
	STRING		unused_xsection_name
	)

	{

	/* Set anchor location and define anchor offset based on anchor type */
	/* Note that XYpoint is an offset from the left of the first line!   */
	if ( same(anchor, AnchorAbsolute) )
		{
		(void) strcpy(Anchor, AnchorAbsolute);
		XYpoint[X]  = 1.0 + (double) xoff;
		XYpoint[Y]  = 1.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCurrent) )
		{
		(void) strcpy(Anchor, AnchorCurrent);
		XYpoint[X] += (double) xoff;
		XYpoint[Y] += (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerLeft) )
		{
		(void) strcpy(Anchor, AnchorLowerLeft);
		XYpoint[X]  = 1.0 + (double) xoff;
		XYpoint[Y]  = (double) Tny + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreLeft) )
		{
		(void) strcpy(Anchor, AnchorCentreLeft);
		XYpoint[X]  = 1.0 + (double) xoff;
		XYpoint[Y]  = (double) (Tny + 1) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperLeft) )
		{
		(void) strcpy(Anchor, AnchorUpperLeft);
		XYpoint[X]  = 1.0 + (double) xoff;
		XYpoint[Y]  = 1.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerCentre) )
		{
		(void) strcpy(Anchor, AnchorLowerCentre);
		XYpoint[X]  = (double) (Tnx + 1) /2.0 + (double) xoff;
		XYpoint[Y]  = (double) Tny + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentre) )
		{
		(void) strcpy(Anchor, AnchorCentre);
		XYpoint[X]  = (double) (Tnx + 1) /2.0 + (double) xoff;
		XYpoint[Y]  = (double) (Tny + 1) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperCentre) )
		{
		(void) strcpy(Anchor, AnchorUpperCentre);
		XYpoint[X]  = (double) (Tnx + 1) /2.0 + (double) xoff;
		XYpoint[Y]  = 1.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorLowerRight) )
		{
		(void) strcpy(Anchor, AnchorLowerRight);
		XYpoint[X]  = (double) Tnx + (double) xoff;
		XYpoint[Y]  = (double) Tny + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorCentreRight) )
		{
		(void) strcpy(Anchor, AnchorCentreRight);
		XYpoint[X]  = (double) Tnx + (double) xoff;
		XYpoint[Y]  = (double) (Tny + 1) / 2.0 + (double) yoff;
		AnchorToMap = FALSE;
		}
	else if ( same(anchor, AnchorUpperRight) )
		{
		(void) strcpy(Anchor, AnchorUpperRight);
		XYpoint[X]  = (double) Tnx + (double) xoff;
		XYpoint[Y]  = 1.0 + (double) yoff;
		AnchorToMap = FALSE;
		}

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    d e f i n e _ p e r s p e c t i v e _ v i e w                     *
*    p e r s p e c t i v e _ l o c a t i o n                           *
*    p e r s p e c t i v e _ o r i g i n a l                           *
*    p e r s p e c t i v e _ s c a l e                                 *
*                                                                      *
***********************************************************************/

LOGICAL		define_perspective_view

	(
	LOGICAL		perspective_view,
	LOGICAL		scale_to_perspective,
	float		ytoff,
	float		tilt,
	float		xeye,
	float		yeye,
	float		zeye,
	float		xstretch,
	float		ystretch
	)

	{

	/* Set parameters to adjust map perspective */
	PerspectiveView = perspective_view;

	if ( PerspectiveView )
		{
		PerspectiveViewScale   = scale_to_perspective;
		PerspectiveYTiltOffSet = ytoff;
		PerspectiveCosTilt     = fpa_cosdeg(tilt);
		PerspectiveSinTilt     = fpa_sindeg(tilt);
		PerspectiveXEye        = xeye;
		PerspectiveYEye        = yeye;
		PerspectiveZEye        = zeye;
		PerspectiveXStretch    = xstretch / 100.0;
		PerspectiveYStretch    = ystretch / 100.0;
		}

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		perspective_location

	(
	const POINT	map,
	POINT		page,
	float		*scale
	)

	{
	float		pscale;
	POINT		pos, ppos;

	/* Initialize the return parameters */
	if ( NotNull(page) ) (void) copy_point(page, map);
	if ( NotNull(scale) ) *scale = 1.0;

	/* Return for all locations not anchored to map                      */
	/*  ... but make an exception for locations anchored to map lat/lon! */
	if ( !AnchorToMap && !same(Anchor, AnchorMapLatLon) ) return FALSE;

	/* Return if no perspective adjustment */
	if ( !PerspectiveView ) return FALSE;

	/* Determine new position based on perspective adjustment */
	pos[X] = map[X] - Xorigin;
	pos[Y] = map[Y] - Yorigin;
	(void) perspective_map_to_page(pos, ppos, &pscale);
	if ( NotNull(page) )
		{
		page[X] = ppos[X] + Xorigin;
		page[Y] = ppos[Y] + Yorigin;
		}

	/* Return if no perspective scaling */
	if ( !PerspectiveViewScale ) return FALSE;

	/* Return adjustment to perspective scaling */
	if ( NotNull(scale) ) *scale = pscale;
	return TRUE;
	}

LOGICAL		perspective_original

	(
	const POINT	page,
	POINT		map,
	float		*scale
	)

	{
	float		pscale;
	POINT		pos, ppos;

	/* Initialize the return parameters */
	if ( NotNull(map) ) (void) copy_point(map, page);
	if ( NotNull(scale) ) *scale = 1.0;

	/* Return for all locations not anchored to map                      */
	/*  ... but make an exception for locations anchored to map lat/lon! */
	if ( !AnchorToMap && !same(Anchor, AnchorMapLatLon) ) return FALSE;

	/* Return if no perspective adjustment */
	if ( !PerspectiveView ) return FALSE;

	/* Determine original position based on perspective adjustment */
	pos[X] = page[X] - Xorigin;
	pos[Y] = page[Y] - Yorigin;
	(void) perspective_page_to_map(pos, ppos, &pscale);
	if ( NotNull(map) )
		{
		map[X] = ppos[X] + Xorigin;
		map[Y] = ppos[Y] + Yorigin;
		}

	/* Return if no perspective scaling */
	if ( !PerspectiveViewScale ) return FALSE;

	/* Return adjustment to perspective scaling */
	if ( NotNull(scale) ) *scale = pscale;
	return TRUE;
	}

LOGICAL		perspective_scale

	(
	float		*scale
	)

	{

	/* Initialize the return parameters */
	if ( NotNull(scale) ) *scale = 1.0;

	/* Return for all locations not anchored to map                      */
	/*  ... but make an exception for locations anchored to map lat/lon! */
	if ( !AnchorToMap && !same(Anchor, AnchorMapLatLon) ) return FALSE;

	/* Return if no perspective adjustment or scaling */
	if ( !PerspectiveView )      return FALSE;
	if ( !PerspectiveViewScale ) return FALSE;

	/* Return current adjustment to perspective */
	if ( NotNull(scale) ) *scale = PerspectiveScale;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    a n c h o r e d _ l o c a t i o n                                 *
*    a n c h o r e d _ l i n e                                         *
*    a n c h o r e d _ b o u n d a r y                                 *
*    r o t a t e d _ l o c a t i o n                                   *
*    u n a n c h o r e d _ l o c a t i o n                             *
*    u n a n c h o r e d _ l i n e                                     *
*                                                                      *
***********************************************************************/

void		anchored_location

	(
	const POINT	pos,
	float		xoff,
	float		yoff,
	float		*xx,
	float		*yy
	)

	{
	float		pscale;
	POINT		pos_map, pos_pspct;

	/* Set location on current base map */
	if ( AnchorToMap )
		{
		if ( PerspectiveView )
			{
			pos_map[X] = pos[X]*Uscale + xoff;
			pos_map[Y] = pos[Y]*Uscale + yoff;
			(void) perspective_map_to_page(pos_map, pos_pspct, &pscale);
			PerspectiveScale = pscale;
			*xx = Xorigin + pos_pspct[X];
			*yy = Yorigin + pos_pspct[Y];
			}
		else
			{
			*xx = Xorigin + pos[X]*Uscale + xoff;
			*yy = Yorigin + pos[Y]*Uscale + yoff;
			}
		}

	/* Set location from current anchor location */
	else
		{
		if ( perspective_scale(NullFloat) )
			{
			*xx = (float) XYpoint[X] + (pos[X] + xoff) * PerspectiveScale;
			*yy = (float) XYpoint[Y] + (pos[Y] + yoff) * PerspectiveScale;
			}
		else
			{
			*xx = (float) XYpoint[X] + pos[X] + xoff;
			*yy = (float) XYpoint[Y] + pos[Y] + yoff;
			}
		}
	}

void		anchored_line

	(
	const LINE	line_in,
	float		xoff,
	float		yoff,
	LINE		*line_out
	)

	{
	int			ipts;
	POINT		pos;

	/* Storage for current anchored line */
	static	LINE	CurLine = NullLine;

	/* Initialize the return parameters */
	if ( NotNull(line_out) ) *line_out = NullLine;

	/* Return if no line passed */
	if ( IsNull(line_in) ) return;

	/* Initialize current anchored line */
	(void) destroy_line(CurLine);
	CurLine = create_line();

	/* Set anchored locations for each point in line */
	for ( ipts=0; ipts<line_in->numpts; ipts++ )
		{
		(void) anchored_location(line_in->points[ipts], xoff, yoff,
				&pos[X], &pos[Y]);
		(void) add_point_to_line(CurLine, pos);
		}

	/* Return the current anchored line */
	if ( NotNull(line_out) ) *line_out = CurLine;
	}

void		anchored_boundary

	(
	const BOUND	bound_in,
	float		xoff,
	float		yoff,
	BOUND		*bound_out
	)

	{
	int			ipts, ihole;
	LINE		line;
	POINT		pos;

	/* Storage for current anchored boundary */
	static	BOUND	CurBound = NullBound;

	/* Initialize the return parameters */
	if ( NotNull(bound_out) ) *bound_out = NullBound;

	/* Return if no boundary passed */
	if ( IsNull(bound_in) ) return;

	/* Initialize current anchored boundary */
	(void) destroy_bound(CurBound);
	CurBound = create_bound();

	/* Set anchored locations for each point in boundary */
	line = create_line();
	for ( ipts=0; ipts<bound_in->boundary->numpts; ipts++ )
		{
		(void) anchored_location(bound_in->boundary->points[ipts], xoff, yoff,
				&pos[X], &pos[Y]);
		(void) add_point_to_line(line, pos);
		}
	(void) define_bound_boundary(CurBound, line);

	/* Set anchored locations for each point in boundary holes */
	for ( ihole=0; ihole<bound_in->numhole; ihole++ )
		{
		line = create_line();
		for ( ipts=0; ipts<bound_in->holes[ihole]->numpts; ipts++ )
			{
			(void) anchored_location(bound_in->holes[ihole]->points[ipts],
					xoff, yoff, &pos[X], &pos[Y]);
			(void) add_point_to_line(line, pos);
			}
		(void) add_bound_hole(CurBound, line);
		}

	/* Return the current anchored boundary (including holes) */
	if ( NotNull(bound_out) ) *bound_out = CurBound;
	}

void		rotated_location

	(
	float		xoff,
	float		yoff,
	float		rotation,
	float		*xx,
	float		*yy
	)

	{
	double			rangle;

	/* Set the angle of rotation in radians */
	rangle = (double) (rotation * RAD);

	/* Convert the offset location to rotated coordinates */
	if ( NotNull(xx) )
		*xx = xoff * (float) cos(rangle) - yoff * (float) sin(rangle);
	if ( NotNull(yy) )
		*yy = xoff * (float) sin(rangle) + yoff * (float) cos(rangle);
	}

void		unanchored_location

	(
	const POINT	pos,
	float		xoff,
	float		yoff,
	float		*ux,
	float		*uy
	)

	{
	POINT		pos_pspct, pos_map;

	/* Set unanchored location on current base map */
	if ( AnchorToMap )
		{
		if ( PerspectiveView )
			{
			pos_pspct[X] = pos[X] - Xorigin;
			pos_pspct[Y] = pos[Y] - Yorigin;
			(void) perspective_page_to_map(pos_pspct, pos_map, NullFloat);
			*ux = (pos_map[X] - xoff) / Uscale;
			*uy = (pos_map[Y] - yoff) / Uscale;
			}
		else
			{
			*ux = (pos[X] - Xorigin - xoff) / Uscale;
			*uy = (pos[Y] - Yorigin - yoff) / Uscale;
			}
		}

	/* Set unanchored location from current anchor location */
	else
		{
		*ux = pos[X] - (float) XYpoint[X] - xoff;
		*uy = pos[Y] - (float) XYpoint[Y] - yoff;
		}
	}

void		unanchored_line

	(
	const LINE	line_in,
	float		xoff,
	float		yoff,
	LINE		*line_out
	)

	{
	int			ipts;
	POINT		pos;

	/* Storage for current unanchored line */
	static	LINE	CurLine = NullLine;

	/* Initialize the return parameters */
	if ( NotNull(line_out) ) *line_out = NullLine;

	/* Return if no line passed */
	if ( IsNull(line_in) ) return;

	/* Initialize current unanchored line */
	(void) destroy_line(CurLine);
	CurLine = create_line();

	/* Set unanchored locations for each point in line */
	for ( ipts=0; ipts<line_in->numpts; ipts++ )
		{
		(void) unanchored_location(line_in->points[ipts], xoff, yoff,
				&pos[X], &pos[Y]);
		(void) add_point_to_line(CurLine, pos);
		}

	/* Return the current unanchored line */
	if ( NotNull(line_out) ) *line_out = CurLine;
	}

/***********************************************************************
*                                                                      *
*    m a t c h _ c a t e g o r y _ l o o k u p                         *
*                                                                      *
***********************************************************************/

LOGICAL		match_category_lookup

	(
	STRING		lookup,			/* lookup file name */
	STRING		string,			/* string to match */
	STRING		*symbol,		/* matching symbol */
	STRING		*text,			/* matching text */
	STRING		*presentation	/* matching presentation */
	)

	{
	int					ii, hits, best, maxhits;
	STRING				test, wx_vals;
	CAT_LOOKUP_TABLE	*ctable;

	/* Initialize output buffers */
	if ( NotNull(symbol) )       *symbol       = FpaCblank;
	if ( NotNull(text) )         *text         = FpaCblank;
	if ( NotNull(presentation) ) *presentation = FpaCblank;

	/* Get category lookup table */
	ctable = get_category_lookup_table(lookup);
	if ( IsNull(ctable) ) return FALSE;

	/* No string to match ... so check for special missing parameters */
	if ( blank(string) && ctable->ismiss )
		{

		/* Set the missing parameters */
		if ( NotNull(symbol) )
				*symbol       = ctable->mline.symbolfile;
		if ( NotNull(text) )
				*text         = ctable->mline.text;
		if ( NotNull(presentation) )
				*presentation = ctable->mline.presentation;
		return TRUE;
		}

	/* Return if no string to match */
	else if ( blank(string) )
		{
		return FALSE;
		}

	/* Search each line of category lookup table for best match */
	for ( best=-1, maxhits=0, ii=0; ii<ctable->numlines; ii++ )
		{

		/* Match the parts of the "value" against the FPA "string" */
		wx_vals = strdup(ctable->lines[ii].value);
		hits    = 0;
		while ( NotNull( test = string_arg(wx_vals) ) )
			{
			if ( strstr(string, test) ) hits++;
			}
		FREEMEM(wx_vals);

		/* Save the match if it is the best one so far */
		if ( hits > maxhits )
			{
			maxhits = hits;
			best    = ii;
			}
		}

	/* Return parameters for best match */
	if ( maxhits >= 1 )
		{

		/* Set the matched parameters */
		if ( NotNull(symbol) )
				*symbol       = ctable->lines[best].symbolfile;
		if ( NotNull(text) )
				*text         = ctable->lines[best].text;
		if ( NotNull(presentation) )
				*presentation = ctable->lines[best].presentation;
		return TRUE;
		}

	/* No match found ... so check for special default parameters */
	else if ( ctable->isdef )
		{

		/* Set the default parameters */
		if ( NotNull(symbol) )
				*symbol       = ctable->dline.symbolfile;
		if ( NotNull(text) )
				*text         = ctable->dline.text;
		if ( NotNull(presentation) )
				*presentation = ctable->dline.presentation;
		return TRUE;
		}

	/* Return FALSE if no match found ... and no default */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    m a t c h _ w i n d _ l o o k u p                                 *
*                                                                      *
***********************************************************************/

LOGICAL		match_wind_lookup

	(
	STRING		lookup,			/* lookup file name */
	STRING		type,			/* type of wind to use */
	STRING		sub_type,		/* category of type to use */
	float		flat,			/* latitude for wind to match */
	float		flon,			/* longitude for wind to match */
	float		dir,			/* value of wind direction to match */
	float		spd,			/* value of wind speed to match */
	float		gst,			/* value of wind gust to match */
	STRING		*text,			/* matching text */
	STRING		*symbol,		/* matching symbol */
	float		*sym_scale,		/* scale for matching symbol */
	float		*sym_rotate		/* rotation for matching symbol */
	)

	{
	WIND_LOOKUP_TABLE	*wtable;

	/* Initialize output buffers */
	if ( NotNull(text) )       *text       = FpaCblank;
	if ( NotNull(symbol) )     *symbol     = FpaCblank;
	if ( NotNull(sym_scale) )  *sym_scale  = 100.0;
	if ( NotNull(sym_rotate) ) *sym_rotate =   0.0;

	/* Get wind lookup table */
	wtable = get_wind_lookup_table(lookup);
	if ( IsNull(wtable) ) return FALSE;

	/* Match for CALM wind types */
	if ( same(type, WindCalm) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_value(spd, &wtable->calm.wind_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_text(spd, &wtable->calm.wind_txt, text);
		else if ( same(sub_type, WVsubSymbol) )
			return match_wvlookup_symbol(spd, &wtable->calm.wind_sym, symbol);
		else
			return FALSE;
		}

	/* Match for DIRECTION wind types */
	else if ( same(type, WindDirection) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_dirvalue(dir, spd,
					&wtable->direction.wind_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_dirtext(dir,
					&wtable->direction.wind_txt, text);
		else if ( same(sub_type, WVsubUniform) )
			return match_wvlookup_diruniform(dir, flat, flon,
					&wtable->direction.wind_uni, symbol, sym_rotate);
		else if ( same(sub_type, WVsubProportional) )
			return match_wvlookup_dirproportional(dir, spd, flat, flon,
					&wtable->direction.wind_pro, symbol, sym_scale, sym_rotate);
		else
			return FALSE;
		}

	/* Match for SPEED wind types */
	else if ( same(type, WindSpeed) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_value(spd, &wtable->speed.wind_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_text(spd, &wtable->speed.wind_txt, text);
		else if ( same(sub_type, WVsubSymbol) )
			return match_wvlookup_symbol(spd, &wtable->speed.wind_sym, symbol);
		else
			return FALSE;
		}

	/* Match for GUST wind types */
	else if ( same(type, WindGust) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_gustvalue(gst, &wtable->gust.wind_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_text(gst, &wtable->gust.wind_txt, text);
		else if ( same(sub_type, WVsubSymbol) )
			return match_wvlookup_symbol(gst, &wtable->gust.wind_sym, symbol);
		else
			return FALSE;
		}

	/* Return FALSE if no match found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    m a t c h _ v e c t o r _ l o o k u p                             *
*                                                                      *
***********************************************************************/

LOGICAL		match_vector_lookup

	(
	STRING		lookup,			/* lookup file name */
	STRING		type,			/* type of vector to use */
	STRING		sub_type,		/* category of type to use */
	float		flat,			/* latitude for vector to match */
	float		flon,			/* longitude for vector to match */
	float		dir,			/* value of vector direction to match */
	float		spd,			/* value of vector speed to match */
	STRING		*text,			/* matching text */
	STRING		*symbol,		/* matching symbol */
	float		*sym_scale,		/* scale for matching symbol */
	float		*sym_rotate		/* rotation for matching symbol */
	)

	{
	VECTOR_LOOKUP_TABLE		*vtable;

	/* Initialize output buffers */
	if ( NotNull(text) )       *text       = FpaCblank;
	if ( NotNull(symbol) )     *symbol     = FpaCblank;
	if ( NotNull(sym_scale) )  *sym_scale  = 100.0;
	if ( NotNull(sym_rotate) ) *sym_rotate =   0.0;

	/* Get vector lookup table */
	vtable = get_vector_lookup_table(lookup);
	if ( IsNull(vtable) ) return FALSE;

	/* Match for CALM vector types */
	if ( same(type, VectorCalm) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_value(spd, &vtable->calm.vector_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_text(spd, &vtable->calm.vector_txt, text);
		else if ( same(sub_type, WVsubSymbol) )
			return match_wvlookup_symbol(spd, &vtable->calm.vector_sym, symbol);
		else
			return FALSE;
		}

	/* Match for DIRECTION vector types */
	else if ( same(type, VectorDirection) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_dirvalue(dir, spd,
					&vtable->direction.vector_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_dirtext(dir,
					&vtable->direction.vector_txt, text);
		else if ( same(sub_type, WVsubUniform) )
			return match_wvlookup_diruniform(dir, flat, flon,
					&vtable->direction.vector_uni, symbol, sym_rotate);
		else if ( same(sub_type, WVsubProportional) )
			return match_wvlookup_dirproportional(dir, spd, flat, flon,
					&vtable->direction.vector_pro, symbol,
					sym_scale, sym_rotate);
		else
			return FALSE;
		}

	/* Match for SPEED vector types */
	else if ( same(type, VectorSpeed) )
		{
		if ( same(sub_type, WVsubValue) )
			return match_wvlookup_value(spd, &vtable->speed.vector_val, text);
		else if ( same(sub_type, WVsubText) )
			return match_wvlookup_text(spd, &vtable->speed.vector_txt, text);
		else if ( same(sub_type, WVsubSymbol) )
			return match_wvlookup_symbol(spd, &vtable->speed.vector_sym,
					symbol);
		else
			return FALSE;
		}

	/* Return FALSE if no match found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    a d d _ l o o p _ l o c a t i o n _ l o o k u p                   *
*                                                                      *
***********************************************************************/

LOGICAL		add_loop_location_lookup

	(
	STRING		lookup,		/* lookup file name */
	STRING		stime,		/* start time for location lookup lines */
	STRING		etime,		/* end time for location lookup lines */
	STRING		*times,		/* times for location lookup lines */
	int			num_times,	/* number of times for location lookup lines */
	STRING		*labs,		/* matching labels for location lookup lines */
	int			num_labs,	/* number of labels for location lookup lines */
	float		xinterval	/* interval for location lookup lines (in km) */
	)

	{
	LOGICAL				status;
	int					ilook, iline, ipt, ilab, inode, itime, mplus;
	float				xlat, xlon, clon, xint, ids, idx, idl;
	float				slen;
	STRING				xtime, vtime;
	POINT				pos;
	char				slat[GPGShort], slon[GPGShort];
	LINE				line;
	LINTERP				linterp;
	LOC_LOOKUP_TABLE	*LocLookTable;
	LOC_LOOKUP_LINE		*LocLookLine;
	char				err_buf[GPGLong];

	/* Determine centre longitude for current map */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* First search the list for existing internal lookup files */
	for ( ilook=0; ilook<NumLocLookup; ilook++ )
		{
		if ( same(lookup, LocLookups[ilook].label) )
			{

			/* Empty the existing internal lookup file */
			LocLookTable = &LocLookups[ilook];
			for ( iline=0; iline<LocLookTable->numlines; iline++ )
				{
				LocLookLine = &(LocLookTable->lines[iline]);
				FREEMEM(LocLookLine->ident);
				FREEMEM(LocLookLine->lat);
				FREEMEM(LocLookLine->lon);
				FREEMEM(LocLookLine->vstring);
				FREEMEM(LocLookLine->llab);
				}
			FREEMEM(LocLookTable->lines);
			LocLookTable->numlines = 0;
			LocLookTable->lines    = NullPtr(LOC_LOOKUP_LINE *);
			break;
			}
		}

	/* Otherwise, add a new internal lookup file to the list */
	if ( ilook >= NumLocLookup )
		{
		NumLocLookup++;
		LocLookups   = GETMEM(LocLookups, LOC_LOOKUP_TABLE, NumLocLookup);
		LocLookTable = &LocLookups[ilook];

		/* Initialize the new lookup table */
		LocLookTable->label         = strdup(lookup);
		LocLookTable->numlines      = 0;
		LocLookTable->lines         = NullPtr(LOC_LOOKUP_LINE *);
		LocLookTable->isdef         = FALSE;
		LocLookTable->dline.ident   = NullString;
		LocLookTable->dline.lat     = NullString;
		LocLookTable->dline.lon     = NullString;
		LocLookTable->dline.vstring = NullString;
		LocLookTable->dline.llab    = NullString;
		}

	/* Create internal lookup file lines based on current CURVE object */
	if ( same(CurType, "curve") )
		{

		/* Error if no current CURVE object */
		if ( IsNull(CurCurve) )
			(void) error_report("Invalid CURVE object");

		/* Set line and determine line length (in map units) */
		line = CurCurve->line;
		condense_line(line);
		if ( IsNull(line) || line->numpts <= 0 )
			(void) error_report("Invalid line in CURVE object");
		(void) line_properties(line, NullChar, NullChar, NullFloat, &slen);

		/* Add lookup table lines at xinterval points along CURVE object */
		if ( xinterval > 0.0 )
			{

			/* Set interval (in map units) */
			xint = xinterval * 1000.0 / BaseMap.definition.units;

			if ( Verbose )
				{
				(void) fprintf(stdout,
						"  Extract points every %.2f  from line of length %.2f\n",
						xint, slen);
				}

			/* Set the latitude and longitude for the first line point */
			ipt  = 0;
			ilab = 0;
			(void) pos_to_ll(&BaseMap, line->points[ipt], &xlat, &xlon);
			(void) sprintf(slat, "%.4f", xlat);
			(void) sprintf(slon, "%.4f", xlon);

			/* Add lookup table line at default time */
			iline                = LocLookTable->numlines++;
			LocLookTable->lines  = GETMEM(LocLookTable->lines,
											LOC_LOOKUP_LINE,
											LocLookTable->numlines);
			LocLookLine          = &LocLookTable->lines[iline];

			/* Set the attribute, location and default valid time */
			LocLookLine->ident   = safe_strdup(CurAttribute);
			LocLookLine->lat     = safe_strdup(slat);
			LocLookLine->lon     = safe_strdup(slon);
			LocLookLine->vstring = safe_strdup(CurVtime);

			/* Add a matching label (if one exists) */
			if ( num_labs > ilab )
				LocLookLine->llab    = safe_strdup(labs[ilab]);
			else
				LocLookLine->llab    = safe_strdup(FpaCblank);
			ilab++;

			/* Interpolate positions by walking along line */
			ids = idx = 0.0;
			idl = (float) (line->numpts - 1);
			while ( idx < idl )
				{

				/* Determine next location on line */
				idx = line_walk(line, ids, xint);

				/* Add last location on the line */
				if ( idx >= idl )
					{

					/* Set the latitude and longitude for the last line point */
					ipt = line->numpts-1;
					(void) pos_to_ll(&BaseMap, line->points[ipt], &xlat, &xlon);
					(void) sprintf(slat, "%.4f", xlat);
					(void) sprintf(slon, "%.4f", xlon);

					/* Add lookup table line at default time */
					iline                = LocLookTable->numlines++;
					LocLookTable->lines  = GETMEM(LocLookTable->lines,
													LOC_LOOKUP_LINE,
													LocLookTable->numlines);
					LocLookLine          = &LocLookTable->lines[iline];

					/* Set the attribute, location and default valid time */
					LocLookLine->ident   = safe_strdup(CurAttribute);
					LocLookLine->lat     = safe_strdup(slat);
					LocLookLine->lon     = safe_strdup(slon);
					LocLookLine->vstring = safe_strdup(CurVtime);

					/* Do not add a label for the last point */
					LocLookLine->llab    = safe_strdup(FpaCblank);
					break;
					}

				/* Interpolate location on line */
				(void) copy_point(pos, line_pos(line, idx, NullInt, NullFloat));
				(void) pos_to_ll(&BaseMap, pos, &xlat, &xlon);
				(void) sprintf(slat, "%.4f", xlat);
				(void) sprintf(slon, "%.4f", xlon);

				/* Add lookup table line at default time */
				iline                = LocLookTable->numlines++;
				LocLookTable->lines  = GETMEM(LocLookTable->lines,
												LOC_LOOKUP_LINE,
												LocLookTable->numlines);
				LocLookLine          = &LocLookTable->lines[iline];

				/* Set the attribute, location and default valid time */
				LocLookLine->ident   = safe_strdup(CurAttribute);
				LocLookLine->lat     = safe_strdup(slat);
				LocLookLine->lon     = safe_strdup(slon);
				LocLookLine->vstring = safe_strdup(CurVtime);

				/* Add a matching label (if one exists) */
				if ( num_labs > ilab )
					LocLookLine->llab    = safe_strdup(labs[ilab]);
				else
					LocLookLine->llab    = safe_strdup(FpaCblank);
				ilab++;

				/* Reset start location for next point on line */
				ids = idx;
				}
			}

		/* Add lookup table lines at each point in CURVE object */
		else
			{

			if ( Verbose )
				{
				(void) fprintf(stdout,
						"  Extract %d points from line of length %.2f\n",
						line->numpts, slen);
				}

			/* Extract all points in line */
			for ( ipt=0; ipt<line->numpts; ipt++ )
				{

				/* Set the latitude and longitude for the current spot */
				(void) pos_to_ll(&BaseMap, line->points[ipt], &xlat, &xlon);
				(void) sprintf(slat, "%.4f", xlat);
				(void) sprintf(slon, "%.4f", xlon);

				/* Add lookup table line at default time */
				iline                = LocLookTable->numlines++;
				LocLookTable->lines  = GETMEM(LocLookTable->lines,
												LOC_LOOKUP_LINE,
												LocLookTable->numlines);
				LocLookLine          = &LocLookTable->lines[iline];

				/* Set the attribute, location and default valid time */
				LocLookLine->ident   = safe_strdup(CurAttribute);
				LocLookLine->lat     = safe_strdup(slat);
				LocLookLine->lon     = safe_strdup(slon);
				LocLookLine->vstring = safe_strdup(CurVtime);

				/* Add a matching label (if one exists) */
				if ( num_labs > ipt )
					LocLookLine->llab    = safe_strdup(labs[ipt]);
				else
					LocLookLine->llab    = safe_strdup(FpaCblank);
				}
			}

		if ( Verbose )
			{
			(void) fprintf(stdout,
					"  Location look up ... %s ... %d lines from CURVE feature\n",
					LocLookTable->label, LocLookTable->numlines);
			for ( iline=0; iline<LocLookTable->numlines; iline++ )
				{
				LocLookLine = &(LocLookTable->lines[iline]);
				(void) fprintf(stdout,
						"    %4d  Ident: %s  Lat/Lon: %s/%s  Vt: %s  Label: %s\n",
						iline, LocLookLine->ident,
						LocLookLine->lat, LocLookLine->lon,
						LocLookLine->vstring, LocLookLine->llab);
				}
			}
		}

	/* Create internal lookup file lines based on current SPOT object */
	else if ( same(CurType, "spot") )
		{

		/* Error if no current SPOT object */
		if ( IsNull(CurSpot) )
			(void) error_report("Invalid SPOT object");

		/* Set the latitude and longitude for the current spot */
		(void) pos_to_ll(&BaseMap, CurSpot->anchor, &xlat, &xlon);
		(void) sprintf(slat, "%.4f", xlat);
		(void) sprintf(slon, "%.4f", xlon);

		/* Add lookup table lines at each matching time */
		if ( num_times > 0 )
			{
			for ( itime=0; itime<num_times; itime++ )
				{

				/* Set valid time */
				vtime = interpret_timestring(times[itime], CurVtime, clon);
				if ( blank(vtime) )
					{
					(void) sprintf(err_buf, "Invalid time ... %s", times[itime]);
					(void) warn_report(err_buf);
					continue;
					}

				/* Add another line to the lookup table */
				iline                = LocLookTable->numlines++;
				LocLookTable->lines  = GETMEM(LocLookTable->lines,
												LOC_LOOKUP_LINE,
												LocLookTable->numlines);
				LocLookLine          = &LocLookTable->lines[iline];

				/* Set the attribute, location and valid time */
				LocLookLine->ident   = safe_strdup(CurAttribute);
				LocLookLine->lat     = safe_strdup(slat);
				LocLookLine->lon     = safe_strdup(slon);
				LocLookLine->vstring = safe_strdup(vtime);

				/* Add a matching label (if one exists) */
				if ( num_labs > itime )
					LocLookLine->llab    = safe_strdup(labs[itime]);
				else
					LocLookLine->llab    = safe_strdup(FpaCblank);
				}
			}

		/* Add lookup table line at default time */
		else
			{

			/* Add another line to the lookup table */
			iline                = LocLookTable->numlines++;
			LocLookTable->lines  = GETMEM(LocLookTable->lines,
											LOC_LOOKUP_LINE,
											LocLookTable->numlines);
			LocLookLine          = &LocLookTable->lines[iline];

			/* Set the attribute, location and default valid time */
			LocLookLine->ident   = safe_strdup(CurAttribute);
			LocLookLine->lat     = safe_strdup(slat);
			LocLookLine->lon     = safe_strdup(slon);
			LocLookLine->vstring = safe_strdup(CurVtime);

			/* Add a label (if one exists) */
			if ( num_labs > 0 )
				LocLookLine->llab    = safe_strdup(labs[0]);
			else
				LocLookLine->llab    = safe_strdup(FpaCblank);
			}

		if ( Verbose )
			{
			(void) fprintf(stdout,
					"  Location look up ... %s ... %d lines from SPOT feature\n",
					LocLookTable->label, LocLookTable->numlines);
			for ( iline=0; iline<LocLookTable->numlines; iline++ )
				{
				LocLookLine = &(LocLookTable->lines[iline]);
				(void) fprintf(stdout,
						"    %4d  Ident: %s  Lat/Lon: %s/%s  Vt: %s  Label: %s\n",
						iline, LocLookLine->ident,
						LocLookLine->lat, LocLookLine->lon,
						LocLookLine->vstring, LocLookLine->llab);
				}
			}
		}

	/* Create internal lookup file lines based on current LCHAIN object */
	else if ( same(CurType, "lchain") )
		{

		/* Error if no current LCHAIN object */
		if ( IsNull(CurLchain) || CurLchain->inum <= 0 )
			(void) error_report("Invalid LCHAIN object");

		/* Set the link chain reference time */
		xtime = CurLchain->xtime;

		/* Add lookup table lines by matching times to interpolated nodes */
		if ( num_times > 0 )
			{

			/* Match each time in list against the interpolated nodes */
			for ( itime=0; itime<num_times; itime++ )
				{

				/* Extract the current time */
				vtime = interpret_timestring(times[itime], CurVtime, clon);
				if ( blank(vtime) ) continue;
				mplus = calc_prog_time_minutes(xtime, vtime, &status);

				/* Add a duplicate location before the link chain start time */
				if ( mplus < CurLchain->splus )
					{
					inode   = 0;
					linterp = CurLchain->interps[inode];
					if ( !linterp->there ) continue;
					}

				/* Add a duplicate location after the link chain end time */
				else if ( mplus > CurLchain->eplus)
					{
					inode   = CurLchain->inum - 1;
					linterp = CurLchain->interps[inode];
					if ( !linterp->there ) continue;
					}

				/* Find the interpolated node at the matching time */
				else
					{
					for ( inode=0; inode<CurLchain->inum; inode++ )
						{
						linterp = CurLchain->interps[inode];
						if ( !linterp->there ) continue;
						if ( mplus == linterp->mplus ) break;
						}

					/* Continue if no match found */
					if ( inode >= CurLchain->inum ) continue;
					}

				/* Set the latitude and longitude for the current link node */
				(void) pos_to_ll(&BaseMap, linterp->node, &xlat, &xlon);
				(void) sprintf(slat, "%.4f", xlat);
				(void) sprintf(slon, "%.4f", xlon);

				/* Set the valid time for the current link node */
				vtime = calc_valid_time_minutes(xtime, 0, mplus);

				/* Add another line to the lookup table */
				iline                = LocLookTable->numlines++;
				LocLookTable->lines  = GETMEM(LocLookTable->lines,
												LOC_LOOKUP_LINE,
												LocLookTable->numlines);
				LocLookLine          = &LocLookTable->lines[iline];

				/* Set the attribute, location and valid time */
				LocLookLine->ident   = safe_strdup(CurAttribute);
				LocLookLine->lat     = safe_strdup(slat);
				LocLookLine->lon     = safe_strdup(slon);
				LocLookLine->vstring = safe_strdup(vtime);

				/* Add a matching label (if one exists) */
				if ( num_labs > itime )
					LocLookLine->llab    = safe_strdup(labs[itime]);
				else
					LocLookLine->llab    = safe_strdup(FpaCblank);
				}
			}

		/* Add lookup table lines at each interpolated node in LCHAIN object */
		else
			{

			/* Add a lookup table line at the start time (if required) */
			itime = 0;
			if ( !blank(stime) )
				{
				vtime = interpret_timestring(stime, CurVtime, clon);
				if ( !blank(vtime) )
					{
					mplus = calc_prog_time_minutes(xtime, vtime, &status);
					if ( mplus < CurLchain->splus )
						{
						inode   = 0;
						linterp = CurLchain->interps[inode];
						if ( linterp->there )
							{

							/* Set latitude/longitude for current link node */
							(void) pos_to_ll(&BaseMap, linterp->node,
																&xlat, &xlon);
							(void) sprintf(slat, "%.4f", xlat);
							(void) sprintf(slon, "%.4f", xlon);

							/* Set the valid time for the current link node */
							vtime = calc_valid_time_minutes(xtime, 0, mplus);

							/* Add another line to the lookup table */
							iline                = LocLookTable->numlines++;
							LocLookTable->lines  = GETMEM(LocLookTable->lines,
														LOC_LOOKUP_LINE,
														LocLookTable->numlines);
							LocLookLine          = &LocLookTable->lines[iline];

							/* Set the attribute, location and valid time */
							LocLookLine->ident   = safe_strdup(CurAttribute);
							LocLookLine->lat     = safe_strdup(slat);
							LocLookLine->lon     = safe_strdup(slon);
							LocLookLine->vstring = safe_strdup(vtime);

							/* Add a matching label (if one exists) */
							if ( num_labs > itime )
								LocLookLine->llab    = safe_strdup(labs[itime]);
							else
								LocLookLine->llab    = safe_strdup(FpaCblank);
							itime++;
							}
						}
					}
				}

			/* Add a lookup table line at each interpolated node */
			for ( inode=0; inode<CurLchain->inum; inode++ )
				{

				/* Define each interpolated node */
				linterp = CurLchain->interps[inode];
				if ( !linterp->there ) continue;

				/* Check interpolated node against start time (if required) */
				if ( !blank(stime) )
					{
					vtime = interpret_timestring(stime, CurVtime, clon);
					if ( !blank(vtime) )
						{
						mplus = calc_prog_time_minutes(xtime, vtime, &status);
						if ( linterp->mplus < mplus ) continue;
						}
					}

				/* Check interpolated node against end time (if required) */
				if ( !blank(etime) )
					{
					vtime = interpret_timestring(etime, CurVtime, clon);
					if ( !blank(vtime) )
						{
						mplus = calc_prog_time_minutes(xtime, vtime, &status);
						if ( linterp->mplus > mplus ) continue;
						}
					}

				/* Set the latitude and longitude for the current link node */
				(void) pos_to_ll(&BaseMap, linterp->node, &xlat, &xlon);
				(void) sprintf(slat, "%.4f", xlat);
				(void) sprintf(slon, "%.4f", xlon);

				/* Set the valid time for the current link node */
				vtime = calc_valid_time_minutes(xtime, 0, linterp->mplus);

				/* Add another line to the lookup table */
				iline                = LocLookTable->numlines++;
				LocLookTable->lines  = GETMEM(LocLookTable->lines,
												LOC_LOOKUP_LINE,
												LocLookTable->numlines);
				LocLookLine          = &LocLookTable->lines[iline];

				/* Set the attribute, location and valid time */
				LocLookLine->ident   = safe_strdup(CurAttribute);
				LocLookLine->lat     = safe_strdup(slat);
				LocLookLine->lon     = safe_strdup(slon);
				LocLookLine->vstring = safe_strdup(vtime);

				/* Add a matching label (if one exists) */
				if ( num_labs > itime )
					LocLookLine->llab    = safe_strdup(labs[itime]);
				else
					LocLookLine->llab    = safe_strdup(FpaCblank);
				itime++;
				}

			/* Add a lookup table line at the end time (if required) */
			if ( !blank(etime) )
				{
				vtime = interpret_timestring(etime, CurVtime, clon);
				if ( !blank(vtime) )
					{
					mplus = calc_prog_time_minutes(xtime, vtime, &status);
					if ( mplus > CurLchain->splus )
						{
						inode   = CurLchain->inum - 1;
						linterp = CurLchain->interps[inode];
						if ( linterp->there )
							{

							/* Set latitude/longitude for current link node */
							(void) pos_to_ll(&BaseMap, linterp->node,
																&xlat, &xlon);
							(void) sprintf(slat, "%.4f", xlat);
							(void) sprintf(slon, "%.4f", xlon);

							/* Set the valid time for the current link node */
							vtime = calc_valid_time_minutes(xtime, 0, mplus);

							/* Add another line to the lookup table */
							iline                = LocLookTable->numlines++;
							LocLookTable->lines  = GETMEM(LocLookTable->lines,
														LOC_LOOKUP_LINE,
														LocLookTable->numlines);
							LocLookLine          = &LocLookTable->lines[iline];

							/* Set the attribute, location and valid time */
							LocLookLine->ident   = safe_strdup(CurAttribute);
							LocLookLine->lat     = safe_strdup(slat);
							LocLookLine->lon     = safe_strdup(slon);
							LocLookLine->vstring = safe_strdup(vtime);

							/* Add a matching label (if one exists) */
							if ( num_labs > itime )
								LocLookLine->llab    = safe_strdup(labs[itime]);
							else
								LocLookLine->llab    = safe_strdup(FpaCblank);
							itime++;
							}
						}
					}
				}
			}

		if ( Verbose )
			{
			(void) fprintf(stdout,
					"  Location look up ... %s ... %d lines from LCHAIN feature\n",
					LocLookTable->label, LocLookTable->numlines);
			for ( iline=0; iline<LocLookTable->numlines; iline++ )
				{
				LocLookLine = &(LocLookTable->lines[iline]);
				(void) fprintf(stdout,
						"    %4d  Ident: %s  Lat/Lon: %s/%s  Vt: %s  Label: %s\n",
						iline, LocLookLine->ident,
						LocLookLine->lat, LocLookLine->lon,
						LocLookLine->vstring, LocLookLine->llab);
				}
			}
		}

	/* Return TRUE if all went well */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    m a t c h _ l o c a t i o n _ l o o k u p                         *
*                                                                      *
***********************************************************************/

LOGICAL		match_location_lookup

	(
	STRING		lookup,		/* lookup file name */
	STRING		ident,		/* identifier to match */
	STRING		vtime,		/* valid time to match (optional) */
	STRING		*flat,		/* matching latitude */
	STRING		*flon,		/* matching longitude */
	STRING		*llab		/* matching label */
	)

	{
	int					ii;
	float				clon;
	STRING				vt;
	LOC_LOOKUP_TABLE	*ltable;

	/* Initialize output buffers */
	if ( NotNull(flat) ) *flat = FpaCblank;
	if ( NotNull(flon) ) *flon = FpaCblank;
	if ( NotNull(llab) ) *llab = FpaCblank;

	/* Return if no identifier to match */
	if ( blank(ident) )
		{
		return FALSE;
		}

	/* Get location look up table */
	ltable = get_location_lookup_table(lookup);
	if ( IsNull(ltable) ) return FALSE;

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Search each line of location look up table for matching identifier */
	for ( ii=0; ii<ltable->numlines; ii++ )
		{

		/* Match valid time (if required) */
		if ( !blank(ltable->lines[ii].vstring) )
			{
			vt = interpret_timestring(ltable->lines[ii].vstring, T0stamp, clon);
			if ( !same(vt, vtime) ) continue;
			}

		/* Match identifier against identifier in location look up table line */
		if ( same(ltable->lines[ii].ident, ident) )
			{

			/* Return parameters for matching identifier */
			if ( NotNull(flat) ) *flat = ltable->lines[ii].lat;
			if ( NotNull(flon) ) *flon = ltable->lines[ii].lon;
			if ( NotNull(llab) ) *llab = ltable->lines[ii].llab;
			return TRUE;
			}
		}

	/* No match found ... so check for special default parameters */
	if ( ltable->isdef )
		{

		/* Set the default parameters */
		if ( NotNull(flat) ) *flat = ltable->dline.lat;
		if ( NotNull(flon) ) *flon = ltable->dline.lon;
		if ( NotNull(llab) ) *llab = ltable->dline.llab;
		return TRUE;
		}

	/* Return FALSE if no match found ... and no default */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*    n e x t _ l o c a t i o n _ l o o k u p _ l i n e                 *
*                                                                      *
***********************************************************************/

int			next_location_lookup_line

	(
	STRING		lookup,		/* lookup file name */
	int			icurr,		/* current line in lookup file */
	STRING		vtime,		/* valid time to match (optional) */
	STRING		*ident,		/* identifier for next line in lookup file */
	STRING		*flat,		/* latitude for next line in lookup file */
	STRING		*flon,		/* longitude for next line in lookup file */
	STRING		*llab		/* label for next line in lookup file */
	)

	{
	int					inext, ii;
	float				clon;
	STRING				vt;
	LOC_LOOKUP_TABLE	*ltable;

	/* Initialize output buffers */
	if ( NotNull(ident) ) *ident = FpaCblank;
	if ( NotNull(flat) )  *flat = FpaCblank;
	if ( NotNull(flon) )  *flon = FpaCblank;
	if ( NotNull(llab) )  *llab = FpaCblank;

	/* Get location look up table */
	ltable = get_location_lookup_table(lookup);
	if ( IsNull(ltable) ) return -1;

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Set start location for next line in lookup file */
	if ( icurr < 0 ) inext = 0;
	else             inext = icurr + 1;

	/* Return if end of lookup file has been reached */
	if ( inext >= ltable->numlines ) return -1;

	/* Return parameters for next line if no valid time to match */
	if ( blank(vtime) )
		{
		if ( NotNull(ident) ) *ident = ltable->lines[inext].ident;
		if ( NotNull(flat) )  *flat  = ltable->lines[inext].lat;
		if ( NotNull(flon) )  *flon  = ltable->lines[inext].lon;
		if ( NotNull(llab) )  *llab  = ltable->lines[inext].llab;
		return inext;
		}

	/* Search lines of location look up table for matching valid time */
	for ( ii=inext; ii<ltable->numlines; ii++ )
		{

		/* Match valid time (if required) */
		if ( !blank(ltable->lines[ii].vstring) )
			{
			vt = interpret_timestring(ltable->lines[ii].vstring, T0stamp, clon);
			if ( !same(vt, vtime) ) continue;
			}

		/* Return parameters for this line */
		if ( NotNull(ident) ) *ident = ltable->lines[ii].ident;
		if ( NotNull(flat) )  *flat  = ltable->lines[ii].lat;
		if ( NotNull(flon) )  *flon  = ltable->lines[ii].lon;
		if ( NotNull(llab) )  *llab  = ltable->lines[ii].llab;
		return ii;
		}

	/* Return if no matching valid time found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*    m a t c h _ d a t a _ f i l e                                     *
*                                                                      *
***********************************************************************/

LOGICAL		match_data_file

	(
	STRING		dname,		/* data file name */
	STRING		format,		/* format of data file */
	STRING		units,		/* units for values in data file */
	STRING		wind_units,	/* units for wind speeds in data file */
	STRING		ident,		/* identifier to match */
	STRING		lat,		/* latitude to match */
	STRING		lon,		/* longitude to match */
	STRING		vtime,		/* valid time to match */
	CAL			*cal		/* CAL structure containing matching parameters */
	)

	{
	LOGICAL				valid;
	int					ii, il, iid, ilat, ilon, ivt;
	int					ilab, ival, iunit, iwdir, iwspd, iwgst, iwunit;
	float				flat, flon, dlat, dlon, clon, fval;
	double				dval;
	char				tbuf[GPGLong], vbuf[GPGShort];
	STRING				delim, vt, vunit, wunit, wval;
	WIND_VAL			wv;
	GPGdtype			ftype;
	DATA_FILE			*dfile;
	char				err_buf[GPGLong];

	/* Storage for matched data file information */
	static	CAL		MCal = NullCal;

	/* Create or empty storage buffer */
	if ( IsNull(MCal) ) MCal = CAL_create_empty();
	else                (void) CAL_empty(MCal);

	/* Initialize output buffer */
	if ( NotNull(cal) )   *cal   = NullCal;

	/* Return if no identifier or latitude/longitude to match */
	if ( blank(ident) && (blank(lat) || blank(lon)))
		{
		return FALSE;
		}

	/* Return if problems with latitude/longitude to match */
	if ( !blank(lat) && !blank(lon) )
		{
		flat = read_lat(lat, &valid);
		if ( !valid ) return FALSE;
		flon = read_lon(lon, &valid);
		if ( !valid ) return FALSE;
		}

	/* Parse the first part of the data file format to find the delimiter */
	(void) strcpy(tbuf, format);
	ftype = parse_data_format(tbuf, &delim);

	/* Get data file structure containing information */
	dfile = get_data_file(dname, delim);
	if ( IsNull(dfile) ) return FALSE;

	/* Warning if no data in data file */
	if ( dfile->numlines <= 0 )
		{
		(void) sprintf(err_buf, "No data in data file: %s", dname);
		(void) warn_report(err_buf);
		return FALSE;
		}

	/* Set locations in data file for parameters */
	ii   = 0;
	iid  = ilat = ilon  = ivt   = -1;
	ilab = ival = iunit = iwdir = iwspd = iwgst = iwunit = -1;
	(void) strcpy(tbuf, format);
	do
		{

		/* Identify each portion of the data file format */
		ftype = parse_data_format(tbuf, NullStringPtr);
		switch (ftype)
			{
			case GPG_Identifier:		iid    = ii;	break;
			case GPG_Latitude:			ilat   = ii;	break;
			case GPG_Longitude:			ilon   = ii;	break;
			case GPG_TimeStamp:			ivt    = ii;	break;
			case GPG_Label:				ilab   = ii;	break;
			case GPG_Value:				ival   = ii;	break;
			case GPG_Units:				iunit  = ii;	break;
			case GPG_WindDirection:		iwdir  = ii;	break;
			case GPG_WindSpeed:			iwspd  = ii;	break;
			case GPG_WindGust:			iwgst  = ii;	break;
			case GPG_WindUnits:			iwunit = ii;	break;
			case GPG_None:				break;
			}

		/* Parse the next part of the data file format */
		ii++;
		}
		while ( !blank(tbuf) );

	/* Error message if format is inconsistent with data file paramters */
	if ( ii > dfile->lines[0].nparms )
		{
		(void) sprintf(err_buf,
			"Parameters (%d) in \"format\" inconsistent with (%d) in data file: %s",
			ii, dfile->lines[0].nparms, dname);
		(void) error_report(err_buf);
		}

	/* Error messages for missing data file formats for matching */
	if ( ( !blank(ident) && iid < 0 )
			&& ( (!blank(lat) && !blank(lon)) && (ilat < 0 || ilon < 0) ) )
		(void) error_report("Missing \"identifier\" or \"latitude\" and \"longitude\" in data file format");

	/* Error messages for missing data file formats data to extract */
	if ( ilab < 0 && ival < 0 && (iwspd < 0 || iwdir < 0) )
		(void) error_report("Missing \"label\" or \"value\" or \"wind_...\" in data file format");
	if ( iwspd >= 0 && iwdir < 0 )
		(void) error_report("Missing \"wind_direction\" in data file format");
	if ( iwdir >= 0 && iwspd < 0 )
		(void) error_report("Missing \"wind_speed\" in data file format");

	/* Warning message for inconsistent wind units */
	if ( iwunit >= 0 && !blank (wind_units) )
		{
		wunit = dfile->lines[0].parms[iwunit];
		if ( !same(wind_units, wunit) )
			{
			(void) sprintf(err_buf,
				"Inconsistent \"data_file_wind_units\": %s  and units in data file: %s",
					wind_units, wunit);
			(void) warn_report(err_buf);
			}
		}

	/* >>>>> debug testing for match_data_file() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout, "Data file format: %s\n", format);
		(void) fprintf(stdout, "  Ident: %d\n",     iid);
		(void) fprintf(stdout, "  Latitude: %d\n",  ilat);
		(void) fprintf(stdout, "  Longitude: %d\n", ilon);
		(void) fprintf(stdout, "  TimeStamp: %d\n", ivt);
		if ( ilab >= 0 )
			(void) fprintf(stdout, "  Label: %d\n", ilab);
		else if ( ival >= 0 )
			(void) fprintf(stdout, "  Value: %d\n", ival);
		else if ( iunit >= 0 )
			(void) fprintf(stdout, "  Units: %d\n", iunit);
		else if ( iwdir >= 0 )
			(void) fprintf(stdout, "  Wind Direction/Speed/Gust: %d/%d/%d\n",
														iwdir, iwspd, iwgst);
		else if ( iwunit >= 0 )
			(void) fprintf(stdout, "  Wind Units: %d\n", iwunit);
		}
	/* >>>>> debug testing for match_data_file() <<<<< */

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Search each line of data file for matching identifier or lat/lon */
	for ( il=0; il<dfile->numlines; il++ )
		{

		/* Match identifier against identifier in data file line */
		if ( !blank(ident) && iid >= 0 )
			if ( !same(dfile->lines[il].parms[iid], ident) ) continue;

		/* Match latitude/longitude against lat/lon in data file line */
		if ( !blank(lat) && !blank(lon) && ilat >= 0 && ilon >= 0 )
			{
			dlat = read_lat(dfile->lines[il].parms[ilat], &valid);
			if ( !valid ) continue;
			dlon = read_lon(dfile->lines[il].parms[ilon], &valid);
			if ( !valid ) continue;
			if ( !fcompare(flat, dlat, 90.0, 1e-5) ) continue;
			if ( !fcompare(flon, dlon, 90.0, 1e-5) ) continue;
			}

		/* Match valid time (if required) */
		if ( !blank(vtime) && ivt >= 0 )
			{
			vt = interpret_timestring(dfile->lines[il].parms[ivt],
					T0stamp, clon);
			if ( !same(vt, vtime) ) continue;
			}

		/* Found a match! */
		break;
		}

	/* Return FALSE if no match found */
	if ( il >= dfile->numlines ) return FALSE;

	/* Build data file value into a value string */
	if ( ival >= 0 )
		{

		/* Set data file value */
		(void) sscanf(dfile->lines[il].parms[ival], "%f", &fval);

		/* Convert data file value to MKS units */
		if ( !blank(units) )
			{
			if ( !convert_value(units, (double) fval, FpaCmksUnits, &dval) )
				{
				(void) sprintf(err_buf,
						"Unknown \"data_file_units\": %s", units);
				(void) error_report(err_buf);
				}
			fval = (float) dval;
			}
		else if ( iunit >= 0 )
			{
			vunit = dfile->lines[il].parms[iunit];
			if ( !convert_value(vunit, (double) fval, FpaCmksUnits, &dval) )
				{
				(void) sprintf(err_buf,
						"Unknown \"units\": %s  in data file: %s",
						vunit, dname);
				(void) error_report(err_buf);
				}
			fval = (float) dval;
			}

		/* Build the value string */
		(void) sprintf(vbuf, "%f", fval);
		}

	/* Build data file wind parameters into a wind value string */
	if ( iwdir >= 0 && iwspd >= 0 )
		{

		/* Set wind direction and units */
		(void) sscanf(dfile->lines[il].parms[iwdir], "%f", &fval);
		wv.dir   = fval;
		wv.dunit = DegreesTrue;

		/* Set wind speed and units ... converted to knots! */
		(void) sscanf(dfile->lines[il].parms[iwspd], "%f", &fval);
		if ( !blank(wind_units) )
			{
			if ( !convert_value(wind_units, (double) fval, Knots, &dval) )
				{
				(void) sprintf(err_buf,
						"Unknown \"data_file_wind_units\": %s", wind_units);
				(void) error_report(err_buf);
				}
			fval = (float) dval;
			}
		else if ( iwunit >= 0 )
			{
			wunit = dfile->lines[il].parms[iwunit];
			if ( !convert_value(wunit, (double) fval, Knots, &dval) )
				{
				(void) sprintf(err_buf,
						"Unknown \"wind_units\": %s  in data file: %s",
						wunit, dname);
				(void) error_report(err_buf);
				}
			fval = (float) dval;
			}
		wv.speed = fval;
		wv.sunit = Knots;

		/* Set wind gust speed ... converted to knots! */
		if ( iwgst >= 0 )
			{
			(void) sscanf(dfile->lines[il].parms[iwgst], "%f", &fval);
			if ( !blank(wind_units) )
				{
				if ( !convert_value(wind_units, (double) fval, Knots, &dval) )
					{
					(void) sprintf(err_buf,
							"Unknown \"data_file_wind_units\": %s",
							wind_units);
					(void) error_report(err_buf);
					}
				fval = (float) dval;
				}
			else if ( iwunit >= 0 )
				{
				wunit = dfile->lines[il].parms[iwunit];
				if ( !convert_value(wunit, (double) fval, Knots, &dval) )
					{
					(void) sprintf(err_buf,
							"Unknown \"wind_units\": %s  in data file: %s",
							wunit, dname);
					(void) error_report(err_buf);
					}
				fval = (float) dval;
				}
			wv.gust = fval;
			}

		/* Set wind gust speed to wind speed if not in data file */
		else
			{
			wv.gust  = wv.speed;
			}

		/* Build the wind value string */
		wval = build_wind_value_string(&wv);
		}

	/* Add parameters to the CAL structure */
	(void) CAL_add_attribute(MCal, AttribCategory, AttribCategoryDefault);
	if ( iid  >= 0 ) (void) CAL_add_attribute(MCal, AttribGPGenIdent,
													dfile->lines[il].parms[iid]);
	if ( ilat >= 0 )  (void) CAL_add_attribute(MCal, AttribGPGenLat,
													dfile->lines[il].parms[ilat]);
	if ( ilon >= 0 )  (void) CAL_add_attribute(MCal, AttribGPGenLon,
													dfile->lines[il].parms[ilon]);
	if ( ilab >= 0 )  (void) CAL_add_attribute(MCal, AttribGPGenLabel,
													dfile->lines[il].parms[ilab]);
	if ( ival >= 0 )  (void) CAL_add_attribute(MCal, AttribGPGenValue, vbuf);
	if ( iwdir >= 0 && iwspd >= 0 )
		{
		(void) CAL_add_attribute(MCal, AttribGPGenWind, wval);
		FREEMEM(wval);
		}

	/* Return the CAL structure */
	if ( NotNull(cal) )   *cal   = MCal;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    g e t _ p a t t e r n                                             *
*    g e t _ d e f a u l t _ b a s e l i n e _ p a t t e r n           *
*                                                                      *
* >>> Note that this code is adapted from graphics/pattern_line.c <<<  *
*                                                                      *
***********************************************************************/

PATTERN		*get_pattern

	(
	STRING		name,		/* pattern name              */
	float		width,		/* pattern width factor      */
	float		length		/* pattern repetition factor */
	)

	{
	int			ic;
	STRING		type;
	ITEM		item;
	PATTERN		*ptemp;

	/* Storage for current pattern */
	static	PATTERN	Pcur = { NullString, 0, NullItemPtr, NullStringList, \
								NullLogicalList };

	/* Re-initialize current pattern */
	if ( Pcur.num > 0 )
		{
		for ( ic=0; ic<Pcur.num; ic++ )
			{
			(void) destroy_item(Pcur.type[ic], Pcur.list[ic]);
			FREEMEM(Pcur.type[ic]);
			}
		Pcur.name = NullString;
		Pcur.num  = 0;
		}

	/* Find the pattern */
	ptemp = find_pattern(name);
	if ( IsNull(ptemp) ) return NullPtr(PATTERN *);

	/* Copy and scale the pattern to the specified width and length */
	Pcur.name   = ptemp->name;
	Pcur.num    = ptemp->num;
	Pcur.list   = GETMEM(Pcur.list,   ITEM,    Pcur.num);
	Pcur.type   = GETMEM(Pcur.type,   STRING,  Pcur.num);
	Pcur.contig = GETMEM(Pcur.contig, LOGICAL, Pcur.num);
	for ( ic=0; ic<Pcur.num; ic++ )
		{

		/* Scale the pattern wrt current map (if required) */
		type = ptemp->type[ic];
		item = copy_item(type, ptemp->list[ic]);
		(void) scale_item(type, item, (length / map_scaling()),
													(width / map_scaling()));

		/* Add the scaled pattern to the list */
		Pcur.list[ic]   = item;
		Pcur.type[ic]   = strdup(type);
		Pcur.contig[ic] = ptemp->contig[ic];
		}

	return &Pcur;
	}

PATTERN		*get_default_baseline_pattern

	(
	float		width,		/* pattern width factor      */
	float		length		/* pattern repetition factor */
	)

	{
	int			ic;
	STRING		type;
	ITEM		item;
	PATTERN		*ptemp;

	/* Storage for default baseline pattern */
	static	PATTERN	Pdbp = { NullString, 0, NullItemPtr, NullStringList, \
								NullLogicalList };

	/* Re-initialize default baseline pattern */
	if ( Pdbp.num > 0 )
		{
		for ( ic=0; ic<Pdbp.num; ic++ )
			{
			(void) destroy_item(Pdbp.type[ic], Pdbp.list[ic]);
			FREEMEM(Pdbp.type[ic]);
			}
		Pdbp.name = NullString;
		Pdbp.num  = 0;
		}

	/* Find the pattern */
	ptemp = find_pattern("$FPA/config/patterns/default_baseline");
	if ( IsNull(ptemp) ) return NullPtr(PATTERN *);

	/* Copy and scale the pattern to a default width and length */
	Pdbp.name   = ptemp->name;
	Pdbp.num    = ptemp->num;
	Pdbp.list   = GETMEM(Pdbp.list,   ITEM,    Pdbp.num);
	Pdbp.type   = GETMEM(Pdbp.type,   STRING,  Pdbp.num);
	Pdbp.contig = GETMEM(Pdbp.contig, LOGICAL, Pdbp.num);
	for ( ic=0; ic<Pdbp.num; ic++ )
		{

		/* Scale the pattern wrt current map (if required) */
		type = ptemp->type[ic];
		item = copy_item(type, ptemp->list[ic]);
		(void) scale_item(type, item, (length / map_scaling()),
													(width / map_scaling()));

		/* Add the scaled pattern to the list */
		Pdbp.list[ic]   = item;
		Pdbp.type[ic]   = strdup(type);
		Pdbp.contig[ic] = ptemp->contig[ic];
		}

	return &Pdbp;
	}

/***********************************************************************
*                                                                      *
*    a d d _ c r o s s _ s e c t i o n                                 *
*    g e t _ c r o s s _ s e c t i o n                                 *
*    f r e e _ c r o s s _ s e c t i o n _ a x e s                     *
*    d e f i n e _ c r o s s _ s e c t i o n _                         *
*                                       h o r i z o n t a l _ a x i s  *
*    c r o s s _ s e c t i o n _ h o r i z o n t a l _ a x i s         *
*    d e f i n e _ c r o s s _ s e c t i o n _                         *
*                                       v e r t i c a l _ a x i s      *
*    c r o s s _ s e c t i o n _ v e r t i c a l _ a x i s             *
*                                                                      *
***********************************************************************/

/* Storage for named cross sections */
static	int			NumXSections = 0;
static	GRA_XSECT	*XSections   = NullPtr(GRA_XSECT *);

LOGICAL			add_cross_section

	(
	STRING		xsection_name,	/* cross section name */
	STRING		type,			/* cross section type */
	float		width,			/* cross section horizontal width */
	float		map_scale,		/* cross section horizontal map scale */
	float		height,			/* cross section vertical height */
	STRING		loc_lookup,		/* cross section horizontal lookup table */
	STRING		ver_lookup,		/* cross section vertical lookup table */
	float		xoff,			/* cross section x offset */
	float		yoff			/* cross section y offset */
	)

	{
	int					ii;
	float				hwidth;
	double				hsize, vsize;
	GRA_XSECT			*CrossSect;
	LOC_LOOKUP_TABLE	*htable;
	VERT_LOOKUP_TABLE	*vtable;
	char				err_buf[GPGLong];

	/* Check that cross section has not already been defined */
	for ( ii=0; ii<NumXSections; ii++ )
		{
		if ( same(xsection_name, XSections[ii].label) ) break;
		}

	/* Re-define the cross section parameters */
	if (ii < NumXSections)
		{
		CrossSect = &XSections[ii];
		free_cross_section_axes(CrossSect);
		(void) sprintf(err_buf,
				"Re-defining parameters for cross section: %s", xsection_name);
		(void) warn_report(err_buf);
		}

	/* Add another cross section to the list */
	else
		{
		NumXSections++;
		XSections = GETMEM(XSections, GRA_XSECT, NumXSections);
		CrossSect = &XSections[NumXSections-1];
		}

	/* Initialize the cross section parameters */
	(void) strcpy(CrossSect->label, xsection_name);
	(void) strcpy(CrossSect->type,  type);
	CrossSect->width  = 0.0;
	CrossSect->height = 0.0;
	CrossSect->x_off  = xoff;
	CrossSect->y_off  = yoff;
	CrossSect->haxis  = NullPtr(XSECT_HOR_AXIS *);
	CrossSect->vaxis  = NullPtr(XSECT_VER_AXIS *);
	CrossSect->drange = 0.0;
	CrossSect->trange = 0.0;

	/* Set the horizontal lookup table name */
	htable = get_location_lookup_table(loc_lookup);
	if ( IsNull(htable) )
		{
		(void) sprintf(err_buf, "Cannot find horizontal lookup table ... %s",
				loc_lookup);
		(void) error_report(err_buf);
		}
	if ( htable->numlines < 2 )
		{
		(void) sprintf(err_buf, "Not enough lines in location look up ... %s",
				htable->label);
		(void) error_report(err_buf);
		}
	(void) strcpy(CrossSect->location_lookup, htable->label);

	/* Set the vertical lookup table name */
	vtable = get_vertical_lookup_table(ver_lookup);
	if ( IsNull(vtable) )
		{
		(void) sprintf(err_buf, "Cannot find vertical lookup table ... %s",
				ver_lookup);
		(void) error_report(err_buf);
		}
	if ( vtable->numlines < 2 )
		{
		(void) sprintf(err_buf, "Not enough lines in vertical lookup ... %s",
				vtable->label);
		(void) error_report(err_buf);
		}
	(void) strcpy(CrossSect->vertical_lookup, vtable->label);

	/* Set the cross section horizontal and vertical axis parameters         */
	/* Note that this initializes parameters in the cross section structure! */
	if ( !define_cross_section_horizontal_axis(CrossSect, loc_lookup, &hsize) )
		{
		(void) error_report("Error defining cross section horizontal axis");
		}
	if ( !define_cross_section_vertical_axis(CrossSect, ver_lookup, &vsize) )
		{
		(void) error_report("Error defining cross section vertical axis");
		}

	/* Set the horizontal width */
	if ( width > 0.0 )
		{
		hwidth = width;
		}

	/* Determine the horizontal width from the scaled horizontal azis */
	/* Note that BaseMap units are in m                               */
	else
		{
		hwidth  = hsize * BaseMap.definition.units / map_scale / 0.0254;
		hwidth *= DisplayUnits.conversion;
		}

	/* Define the cross section width and height */
	CrossSect->width  = hwidth;
	CrossSect->height = height;

	if ( Verbose )
		{
		(void) fprintf(stdout,
				"  Defining cross section ... %s of type ... %s\n",
				CrossSect->label, CrossSect->type);
		(void) fprintf(stdout,
				"   With parameters  width height ... %.5f %.5f  x_off/y_off ... %.2f/%.2f\n",
				CrossSect->width, CrossSect->height,
				CrossSect->x_off, CrossSect->y_off);
		(void) fprintf(stdout,
				"                    drange ... %.5f  trange ... %.5f\n",
				CrossSect->drange, CrossSect->trange);
		(void) fprintf(stdout,
				"   Horizontal axis from location look up ... %s\n",
				CrossSect->location_lookup);
		for ( ii=0; ii<CrossSect->haxis->num; ii++ )
			{
			(void) fprintf(stdout,
					"     Location %4d  ident ... %s  flat/flon ... %.2f/%.2f\n",
					ii, CrossSect->haxis->idents[ii],
					CrossSect->haxis->flats[ii], CrossSect->haxis->flons[ii]);
			(void) fprintf(stdout,
					"                    vtime ... %s  label ... %s  pstn ... %.1f\n",
					CrossSect->haxis->vtimes[ii],
					CrossSect->haxis->labels[ii], CrossSect->haxis->pstns[ii]);
			(void) fprintf(stdout,
					"                    dval ... %.5f  tval ... %.5f  loc ... %.5f\n",
					CrossSect->haxis->dvals[ii],
					CrossSect->haxis->tvals[ii], CrossSect->haxis->locs[ii]);
			(void) fprintf(stdout,
					"                    dir ... %.5f  spd ... %.5f\n",
					CrossSect->haxis->dirs[ii], CrossSect->haxis->spds[ii]);
			}
		(void) fprintf(stdout,
				"   Vertical axis from vertical lookup ... %s\n",
				CrossSect->vertical_lookup);
		for ( ii=0; ii<CrossSect->vaxis->num; ii++ )
			{
			(void) fprintf(stdout,
					"     Location %4d  ident ... %s  label ... %s  pstn ... %.1f\n",
					ii, CrossSect->vaxis->idents[ii],
					CrossSect->vaxis->labels[ii], CrossSect->vaxis->pstns[ii]);
			(void) fprintf(stdout,
					"                    val ... %.5f  loc ... %.5f\n",
					CrossSect->vaxis->vals[ii], CrossSect->vaxis->vals[ii]);
			}
		}

	/* Return TRUE */
	return TRUE;
	}

GRA_XSECT		*get_cross_section

	(
	STRING		xsection_name	/* cross section name */
	)

	{
	int			ii;

	/* Return cross section with matching name */
	for ( ii=0; ii<NumXSections; ii++ )
		{
		if ( same(xsection_name, XSections[ii].label) )
			{

			/* Set the coordinates for the current cross section */
			XSect_ULpoint[X] = XSections[ii].x_off;
			XSect_ULpoint[Y] = XSections[ii].y_off + XSections[ii].height;
			XSect_LRpoint[X] = XSections[ii].x_off + XSections[ii].width;
			XSect_LRpoint[Y] = XSections[ii].y_off;

			/* Return the cross section */
			return &XSections[ii];
			}
		}

	/* Error return if cross section not found */
	return NullPtr(GRA_XSECT *);
	}

void		free_cross_section_axes

	(
	GRA_XSECT		*cross			/* cross section structure */
	)

	{
	int					ii;

	/* Error return for missing parameters */
	if ( IsNull(cross) )        return;
	if ( IsNull(cross->haxis) ) return;
	if ( IsNull(cross->vaxis) ) return;

	/* Free space used by horizontal axis in cross section structure */
	if ( cross->haxis->num > 0 )
		{
		FREELIST(cross->haxis->idents, cross->haxis->num);
		FREELIST(cross->haxis->vtimes, cross->haxis->num);
		FREELIST(cross->haxis->labels, cross->haxis->num);
		FREEMEM(cross->haxis->flats);
		FREEMEM(cross->haxis->flons);
		FREEMEM(cross->haxis->pstns);
		FREEMEM(cross->haxis->dvals);
		FREEMEM(cross->haxis->tvals);
		FREEMEM(cross->haxis->dirs);
		FREEMEM(cross->haxis->spds);
		FREEMEM(cross->haxis->locs);
		}
	FREEMEM(cross->haxis);

	/* Free space used by vertical axis in cross section structure */
	if ( cross->vaxis->num > 0 )
		{
		FREELIST(cross->vaxis->idents, cross->vaxis->num);
		FREELIST(cross->vaxis->labels, cross->vaxis->num);
		FREEMEM(cross->vaxis->pstns);
		FREEMEM(cross->vaxis->vals);
		FREEMEM(cross->vaxis->locs);
		}
	FREEMEM(cross->vaxis);
	}

LOGICAL		define_cross_section_horizontal_axis

	(
	GRA_XSECT		*cross,			/* cross section structure */
	STRING			loc_lookup,		/* location look up table for axis */
	double			*horizontal		/* size of horizontal axis (in grid units) */
	)

	{
	int					ii, ib, ie, it, tdiff;
	LOGICAL				valid, first, vtinc;
	float				clon, flat, flon, flatx, flonx, xdir, xspd;
	double				drange, trange;
	POINT				posb, pose;
	STRING				vtime;
	int					byear, bjday, bhour, bminute;
	int					lyear, ljday, lhour, lminute;
	int					eyear, ejday, ehour, eminute;
	LOC_LOOKUP_TABLE	*htable;
	char				err_buf[GPGLong];

	/* Error return for missing parameters */
	if ( IsNull(cross) )     return FALSE;
	if ( blank(loc_lookup) ) return FALSE;

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Get the horizontal lookup table */
	htable = get_location_lookup_table(loc_lookup);
	if ( IsNull(htable) )
		{
		(void) sprintf(err_buf, "Cannot find horizontal lookup table ... %s",
				loc_lookup);
		(void) error_report(err_buf);
		}
	if ( htable->numlines < 1 )
		{
		(void) sprintf(err_buf, "Not enough lines in location look up ... %s",
				htable->label);
		(void) error_report(err_buf);
		}

	/* Initialize horizontal axis in cross section structure (if required) */
	if ( IsNull(cross->haxis) )
		{

		/* Initialize parameters in cross section structure */
		cross->haxis = INITMEM(XSECT_HOR_AXIS, 1);
		cross->haxis->num    = htable->numlines;
		cross->haxis->idents = INITMEM(STRING, cross->haxis->num);
		cross->haxis->flats  = INITMEM(float,  cross->haxis->num);
		cross->haxis->flons  = INITMEM(float,  cross->haxis->num);
		cross->haxis->vtimes = INITMEM(STRING, cross->haxis->num);
		cross->haxis->labels = INITMEM(STRING, cross->haxis->num);
		cross->haxis->pstns  = INITMEM(double, cross->haxis->num);
		cross->haxis->dvals  = INITMEM(double, cross->haxis->num);
		cross->haxis->tvals  = INITMEM(double, cross->haxis->num);
		cross->haxis->dirs   = INITMEM(double, cross->haxis->num);
		cross->haxis->spds   = INITMEM(double, cross->haxis->num);
		cross->haxis->locs   = INITMEM(double, cross->haxis->num);

		/* Set location from first latitude and longitude */
		flat = read_lat(htable->lines[0].lat, &valid);
		if ( !valid ) return FALSE;
		flon = read_lon(htable->lines[0].lon, &valid);
		if ( !valid ) return FALSE;
		(void) ll_to_pos(&BaseMap, flat, flon, posb);

		/* Set time from first location */
		if ( blank(htable->lines[0].vstring) )
			{
			vtime = interpret_timestring(TVstamp, T0stamp, clon);
			}
		else
			{
			vtime = interpret_timestring(htable->lines[0].vstring,
															T0stamp, clon);
			}
		if ( blank(vtime) ) return FALSE;
		(void) parse_tstamp(vtime, &byear, &bjday, &bhour, &bminute,
											NullLogicalPtr, NullLogicalPtr);

		/* Set axis parameters for first location */
		cross->haxis->idents[0] = safe_strdup(htable->lines[0].ident);
		cross->haxis->flats[0]  = flat;
		cross->haxis->flons[0]  = flon;
		cross->haxis->vtimes[0] = safe_strdup(vtime);
		cross->haxis->labels[0] = safe_strdup(htable->lines[0].llab);
		cross->haxis->pstns[0]  = 0.0;
		cross->haxis->dvals[0]  = 0.0;
		cross->haxis->tvals[0]  = 0.0;
		cross->haxis->dirs[0]   = 0.0;
		cross->haxis->spds[0]   = 0.0;
		cross->haxis->locs[0]   = 0.0;

		/* Set cumulative horizontal distance/time to subsequent locations */
		drange  = 0.0;
		trange  = 0.0;
		first   = TRUE;
		vtinc   = TRUE;
		lyear   = byear;
		ljday   = bjday;
		lhour   = bhour;
		lminute = bminute;
		for ( ii=1; ii<htable->numlines; ii++ )
			{

			/* Set location from next latitude and longitude */
			flat = read_lat(htable->lines[ii].lat, &valid);
			if ( !valid ) return FALSE;
			flon = read_lon(htable->lines[ii].lon, &valid);
			if ( !valid ) return FALSE;
			(void) ll_to_pos(&BaseMap, flat, flon, pose);

			/* Determine cumulative distance in space (in map units) */
			drange += hypot(pose[X]-posb[X], pose[Y]-posb[Y]);

			/* Set time from next location */
			if ( blank(htable->lines[ii].vstring) )
				{
				vtime = interpret_timestring(TVstamp, T0stamp, clon);
				}
			else
				{
				vtime = interpret_timestring(htable->lines[ii].vstring,
																T0stamp, clon);
				}
			if ( blank(vtime) ) return FALSE;
			(void) parse_tstamp(vtime, &eyear, &ejday, &ehour, &eminute,
											NullLogicalPtr, NullLogicalPtr);

			/* Check for consistent times (increasing or decreasing) */
			tdiff = mdif(lyear, ljday, lhour, lminute,
							eyear, ejday, ehour, eminute);
			if ( first && tdiff != 0 )
				{
				first = FALSE;
				vtinc = (tdiff > 0)? TRUE: FALSE;
				}
			else if ( vtinc && tdiff < 0 )
				{
				(void) sprintf(err_buf,
						"Inconsistent times (not increasing) in horizontal lookup table ... %s",
						loc_lookup);
				(void) error_report(err_buf);
				}
			else if ( !vtinc && tdiff > 0 )
				{
				(void) sprintf(err_buf,
						"Inconsistent times (not decreasing) in horizontal lookup table ... %s",
						loc_lookup);
				(void) error_report(err_buf);
				}

			/* Determine cumulative distance in time (in minutes) */
			trange = (double) mdif(byear, bjday, bhour, bminute,
									eyear, ejday, ehour, eminute);

			/* Set axis parameters for each location */
			cross->haxis->idents[ii] = safe_strdup(htable->lines[ii].ident);
			cross->haxis->flats[ii]  = flat;
			cross->haxis->flons[ii]  = flon;
			cross->haxis->vtimes[ii] = safe_strdup(vtime);
			cross->haxis->labels[ii] = safe_strdup(htable->lines[ii].llab);
			cross->haxis->pstns[ii]  = (double) ii;
			cross->haxis->dvals[ii]  = drange;
			cross->haxis->tvals[ii]  = trange;
			cross->haxis->dirs[ii]   = 0.0;
			cross->haxis->spds[ii]   = 0.0;

			/* Set axis location based on cross section type */
			if ( same(cross->type, XSectSpace)
					|| same(cross->type, XSectSpaceRoute) )
				cross->haxis->locs[ii] = drange;
			else if ( same(cross->type, XSectTime)
					|| same(cross->type, XSectTimeRoute) )
				cross->haxis->locs[ii] = trange;

			/* Reset the initial point */
			(void) copy_point(posb, pose);
			lyear   = eyear;
			ljday   = ejday;
			lhour   = ehour;
			lminute = eminute;
			}

		/* Scale axis locations based on cross section type */
		for ( ii=1; ii<htable->numlines; ii++ )
			{
			if ( same(cross->type, XSectSpace)
					|| same(cross->type, XSectSpaceRoute) )
				cross->haxis->locs[ii] /= drange;
			else if ( same(cross->type, XSectTime)
					|| same(cross->type, XSectTimeRoute) )
				cross->haxis->locs[ii] /= trange;
			}

		/* Set speed and direction for route cross sections */
		if ( same(cross->type, XSectSpaceRoute)
				|| same(cross->type, XSectTimeRoute) )
			{

			/* Determine speed and direction for non-coincident points */
			for ( ii=0; ii<htable->numlines; ii++ )
				{

				/* Set parameters for points to check */
				ib = (ii > 0)?                  ii - 1: ii;
				ie = (ii < htable->numlines-1)? ii + 1: ii;
				if ( ib == ie ) continue;

				/* Check for coincident points */
				while ( ie > ib )
					{
					flat  = cross->haxis->flats[ie];
					flon  = cross->haxis->flons[ie];
					flatx = cross->haxis->flats[ie-1];
					flonx = cross->haxis->flons[ie-1];
					if ( flat == flatx && flon == flonx ) ie--;
					else                                  break;
					}
				while ( ib < ie )
					{
					flat  = cross->haxis->flats[ib];
					flon  = cross->haxis->flons[ib];
					flatx = cross->haxis->flats[ib+1];
					flonx = cross->haxis->flons[ib+1];
					if ( flat == flatx && flon == flonx ) ib++;
					else                                  break;
					}
				if ( ib == ie ) continue;

				/* Check that position and time are incrementing */
				if ( cross->haxis->dvals[ib]
						== cross->haxis->dvals[ie] ) continue;
				if ( cross->haxis->tvals[ib]
						== cross->haxis->tvals[ie] ) continue;

				/* Reverse points if time on axis is decreasing */
				if ( cross->haxis->tvals[ie] < cross->haxis->tvals[ib] )
					{
					it = ib;
					ib = ie;
					ie = it;
					}

				/* Set start/end positions of this portion of cross section */
				(void) ll_to_pos(&BaseMap,
						cross->haxis->flats[ib], cross->haxis->flons[ib], posb);
				(void) ll_to_pos(&BaseMap,
						cross->haxis->flats[ie], cross->haxis->flons[ie], pose);

				/* Determine direction (in degrees true) */
				xdir = great_circle_bearing(&BaseMap, posb, pose);

				/* Determine speed from distance and time (in m/s) */
				xspd  = (cross->haxis->dvals[ie] - cross->haxis->dvals[ib]) /
							(cross->haxis->tvals[ie] - cross->haxis->tvals[ib]);
				xspd *= BaseMap.definition.units;
				xspd /= 60;

				/* Set direction and speed at this position */
				cross->haxis->dirs[ii] = xdir;
				cross->haxis->spds[ii] = (float) fabs( (double) xspd);
				}
			}

		/* Set parameters in cross section structure */
		cross->drange = drange;
		cross->trange = trange;

		/* Set return parameters */
		if ( NotNull(horizontal) ) *horizontal = cross->drange;
		return TRUE;
		}

	/* Error if cross section already initialized */
	else
		{
		(void) sprintf(err_buf,
				"Cross section %s horizontal already initialized!",
				cross->label);
		(void) error_report(err_buf);
		return FALSE;
		}
	}

XSECT_HOR_AXIS	*cross_section_horizontal_axis

	(
	GRA_XSECT		*cross,			/* cross section structure */
	STRING			loc_lookup,		/* location look up table for axis */
	GPGltype		ltype,			/* type of locations for axis */
	int				num_xloc,		/* number of locations for axis */
	XSECT_LOCATION	*xsect_locs,	/* locations for axis */
	double			*horizontal		/* size of horizontal axis (in grid units) */
	)

	{
	int					ii, jj, jb, je, nn, nx, numx;
	LOGICAL				stype, valid;
	float				clon, flat, flon;
	double				dval, tval, dbest, dnext, dloc;
	STRING				vtime;
	float				xfact, xdiff, xbear, xflat, xflon;
	double				xdval, xtval, xdir, xspd, xloc;
	STRING				xident, xvtime, xlabel;
	POINT				spos, xpos, epos;
	int					byear, bjday, bhour, bminute;
	int					xyear, xjday, xhour, xminute;
	LOC_LOOKUP_TABLE	*htable;
	char				err_buf[GPGLong];

	/* Storage for current horizontal axis positions */
	static	XSECT_HOR_AXIS	*Haxis  = NullPtr(XSECT_HOR_AXIS *);
	static	int				HMaxNum = 0;

	/* Initialize return parameters */
	if ( NotNull(horizontal) ) *horizontal = 0.0;

	/* Initialize structure for horizontal axis positions */
	if ( IsNull(Haxis) )
		{
		Haxis = INITMEM(XSECT_HOR_AXIS, 1);
		Haxis->num    = 0;
		Haxis->idents = NullStringList;
		Haxis->flats  = NullFloat;
		Haxis->flons  = NullFloat;
		Haxis->vtimes = NullStringList;
		Haxis->labels = NullStringList;
		Haxis->pstns  = NullDouble;
		Haxis->dvals  = NullDouble;
		Haxis->tvals  = NullDouble;
		Haxis->dirs   = NullDouble;
		Haxis->spds   = NullDouble;
		Haxis->locs   = NullDouble;
		}

	/* Free allocated memory in structure for horizontal axis positions */
	else
		{
		for ( ii=0; ii<Haxis->num; ii++ )
			{
			FREEMEM(Haxis->idents[ii]);
			FREEMEM(Haxis->vtimes[ii]);
			FREEMEM(Haxis->labels[ii]);
			}
		}

	/* Error return for missing parameters */
	if ( IsNull(cross) )        return NullPtr(XSECT_HOR_AXIS *);
	if ( IsNull(cross->haxis) ) return NullPtr(XSECT_HOR_AXIS *);

	/* Set centre longitude from current map projection */
	(void) grid_center(&BaseMap, NullPointPtr, NullFloat, &clon);

	/* Match horizontal axis parameters from locations */
	if ( num_xloc > 0 )
		{

		/* Ensure that the type of locations matches the cross section */
		if ( ltype == GPG_LocDistance )
			{
			if ( same(cross->type, XSectSpace)
				|| same(cross->type, XSectSpaceRoute) ) stype = TRUE;
			else
				{
				(void) sprintf(err_buf,
					"Cannot have keyword \"location_distances\" for cross section type %s",
					cross->type);
				(void) error_report(err_buf);
				}
			}
		else if (ltype == GPG_LocTime )
			{
			if ( same(cross->type, XSectTime)
				|| same(cross->type, XSectTimeRoute) ) stype = FALSE;
			else
				{
				(void) sprintf(err_buf,
					"Cannot have keyword \"location_times\" for cross section type %s",
					cross->type);
				(void) error_report(err_buf);
				}
			}
		else if (ltype == GPG_LocFraction )
			{
			if ( same(cross->type, XSectSpace)
				|| same(cross->type, XSectSpaceRoute) ) stype = TRUE;
			else if ( same(cross->type, XSectTime)
				|| same(cross->type, XSectTimeRoute) )  stype = FALSE;
			}

		/* Ensure that return structure is large enough */
		Haxis->num = num_xloc;
		if ( Haxis->num > HMaxNum )
			{
			Haxis->idents = GETMEM(Haxis->idents, STRING, Haxis->num);
			Haxis->flats  = GETMEM(Haxis->flats,  float,  Haxis->num);
			Haxis->flons  = GETMEM(Haxis->flons,  float,  Haxis->num);
			Haxis->vtimes = GETMEM(Haxis->vtimes, STRING, Haxis->num);
			Haxis->labels = GETMEM(Haxis->labels, STRING, Haxis->num);
			Haxis->pstns  = GETMEM(Haxis->pstns,  double, Haxis->num);
			Haxis->dvals  = GETMEM(Haxis->dvals,  double, Haxis->num);
			Haxis->tvals  = GETMEM(Haxis->tvals,  double, Haxis->num);
			Haxis->dirs   = GETMEM(Haxis->dirs,   double, Haxis->num);
			Haxis->spds   = GETMEM(Haxis->spds,   double, Haxis->num);
			Haxis->locs   = GETMEM(Haxis->locs,   double, Haxis->num);
			HMaxNum       = Haxis->num;
			}

		/* Set reference time from first line in cross section structure */
		(void) parse_tstamp(cross->haxis->vtimes[0], &byear, &bjday,
						&bhour, &bminute, NullLogicalPtr, NullLogicalPtr);

		/* Match locations with parameters in cross section structure */
		dval = tval = 0.0;
		for ( numx=0, ii=0; ii<num_xloc; ii++ )
			{

			/* Set distance along axis for matching         */
			/* Convert from km to units of current base map */
			if ( ltype == GPG_LocDistance )
				{
				dval = xsect_locs[ii].xdist * 1000.0 / BaseMap.definition.units;
				}

			/* Set time along axis for matching */
			else if ( ltype == GPG_LocTime )
				{
				vtime = interpret_timestring(xsect_locs[ii].vtime,
																T0stamp, clon);
				if ( blank(vtime) ) return NullPtr(XSECT_HOR_AXIS *);
				(void) parse_tstamp(vtime, &xyear, &xjday, &xhour, &xminute,
							NullLogicalPtr, NullLogicalPtr);
				tval = (double) mdif(byear, bjday, bhour, bminute,
									xyear, xjday, xhour, xminute);
				}

			/* Set distance or time along axis for matching */
			/*  ... depending on type of cross section      */
			else if ( ltype == GPG_LocFraction )
				{
				if (stype)
					dval = (double) xsect_locs[ii].xdist * cross->drange;
				else
					tval = (double) xsect_locs[ii].xdist * cross->trange;
				}

			/* Compare parameter with first line in default table */
			if (stype) dbest = dval - cross->haxis->dvals[0];
			else       dbest = tval - cross->haxis->tvals[0];

			/* Check if first check is a close enough match */
			if ( fabs(dbest) < 0.1 )
				{
				jb = je = 0;
				}

			/* Continue checking if not a close enough match */
			else
				{

				/* Find closest matching parameter in rest of default table */
				jb =  0;
				je = -1;
				for ( jj=1; jj<cross->haxis->num; jj++ )
					{

					/* Compare parameter with first line in default table */
					if (stype) dnext = dval - cross->haxis->dvals[jj];
					else       dnext = tval - cross->haxis->tvals[jj];

					/* End if this is a close enough match */
					if ( fabs(dnext) < 0.1 )
						{
						jb = je = jj;
						break;
						}

					/* End if getting further away   */
					/*  ... will need to extrapolate */
					else if ( (dbest < 0 && dnext < 0 && dnext < dbest)
							|| (dbest > 0 && dnext > 0 && dnext > dbest) )
						{
						break;
						}

					/* Reset and end if changing signs */
					/*  ... will need to interpolate   */
					else if ( (dbest < 0 && dnext > 0)
							|| (dbest > 0 && dnext < 0) )
						{
						je = jj;
						break;
						}

					/* Reset if getting closer to match */
					else
						{
						jb = jj;
						je = jj + 1;
						dbest = dnext;
						}
					}
				}

			if ( Verbose )
				{
				(void) fprintf(stdout,
						"  Matching cross section indicies ... %d and %d\n",
						jb, je);
				if (stype && jb == je)
					(void) fprintf(stdout,
							"  Matching cross section distance ... %.5f to %.5f\n",
							dval, cross->haxis->dvals[jb]);
				else if (stype && (je < 0 || je >= cross->haxis->num))
					(void) fprintf(stdout,
							"  Extrapolating cross section distance ... %.5f from %.5f\n",
							dval, cross->haxis->dvals[jb]);
				else if (stype)
					(void) fprintf(stdout,
							"  Interpolating cross section distance ... %.5f from %.5f and %.5f\n",
							dval, cross->haxis->dvals[jb],
							cross->haxis->dvals[je]);
				else if (jb == je)
					(void) fprintf(stdout,
							"  Matching cross section time ... %.5f to %.5f\n",
							tval, cross->haxis->tvals[jb]);
				else if (je < 0 || je >= cross->haxis->num)
					(void) fprintf(stdout,
							"  Extrapolating cross section time ... %.5f from %.5f\n",
							tval, cross->haxis->tvals[jb]);
				else
					(void) fprintf(stdout,
							"  Interpolating cross section time ... %.5f from %.5f and %.5f\n",
							tval, cross->haxis->tvals[jb],
							cross->haxis->tvals[je]);
				}

			/* Set parameters for exact match */
			if ( jb == je )
				{
				xident = cross->haxis->idents[jb];
				xflat  = cross->haxis->flats[jb];
				xflon  = cross->haxis->flons[jb];
				xvtime = cross->haxis->vtimes[jb];
				xlabel = cross->haxis->labels[jb];
				xdval  = cross->haxis->dvals[jb];
				xtval  = cross->haxis->tvals[jb];
				xdir   = cross->haxis->dirs[jb];
				xspd   = cross->haxis->spds[jb];
				xloc   = cross->haxis->locs[jb];
				}

			/* Extrapolate parameters from before start of cross section */
			else if ( je < 0 )
				{
				xident = FpaCblank;
				xlabel = FpaCblank;

				/* Extrapolate from first two locations of cross section */
				if (stype)
					{
					xdiff = cross->haxis->dvals[jb+1] - cross->haxis->dvals[jb];
					xfact = (cross->haxis->dvals[jb] - dval) / xdiff;
					}
				else
					{
					xdiff = cross->haxis->tvals[jb+1] - cross->haxis->tvals[jb];
					xfact = (cross->haxis->tvals[jb] - tval) / xdiff;
					}
				xdiff = cross->haxis->dvals[jb+1] - cross->haxis->dvals[jb];
				xdval = cross->haxis->dvals[jb] - (xfact * xdiff);
				xdiff = cross->haxis->tvals[jb+1] - cross->haxis->tvals[jb];
				xtval = cross->haxis->tvals[jb] - (xfact * xdiff);
				xdir  = cross->haxis->dirs[jb];
				xspd  = cross->haxis->spds[jb];
				xdiff = cross->haxis->locs[jb+1] - cross->haxis->locs[jb];
				xloc  = cross->haxis->locs[jb] - (xfact * xdiff);

				/* Set latitude and longitude from extrapolated location */
				(void) ll_to_pos(&BaseMap, cross->haxis->flats[jb],
						cross->haxis->flons[jb], spos);
				(void) ll_to_pos(&BaseMap, cross->haxis->flats[jb+1],
						cross->haxis->flons[jb+1], xpos);
				xbear = great_circle_bearing(&BaseMap, xpos, spos);
				xdiff = cross->haxis->dvals[jb] - xdval;
				(void) great_circle_span(&BaseMap, spos, xbear,
						(xdiff * BaseMap.definition.units), epos);
				(void) pos_to_ll(&BaseMap, epos, &xflat, &xflon);

				/* Set valid time from extrapolated time in minutes */
				xvtime = calc_valid_time_minutes(cross->haxis->vtimes[0],
																	0, xtval);
				}

			/* Extrapolate parameters from after end of cross section */
			else if ( je >= cross->haxis->num )
				{
				xident = FpaCblank;
				xlabel = FpaCblank;

				/* Extrapolate from last two locations of cross section */
				if (stype)
					{
					xdiff = cross->haxis->dvals[jb] - cross->haxis->dvals[jb-1];
					xfact = (dval - cross->haxis->dvals[jb]) / xdiff;
					}
				else
					{
					xdiff = cross->haxis->tvals[jb] - cross->haxis->tvals[jb-1];
					xfact = (tval - cross->haxis->tvals[jb]) / xdiff;
					}
				xdiff = cross->haxis->dvals[jb] - cross->haxis->dvals[jb-1];
				xdval = cross->haxis->dvals[jb] + (xfact * xdiff);
				xdiff = cross->haxis->tvals[jb] - cross->haxis->tvals[jb-1];
				xtval = cross->haxis->tvals[jb] + (xfact * xdiff);
				xdir  = cross->haxis->dirs[jb];
				xspd  = cross->haxis->spds[jb];
				xdiff = cross->haxis->locs[jb] - cross->haxis->locs[jb-1];
				xloc  = cross->haxis->locs[jb] + (xfact * xdiff);

				/* Set latitude and longitude from extrapolated location */
				(void) ll_to_pos(&BaseMap, cross->haxis->flats[jb],
						cross->haxis->flons[jb], spos);
				(void) ll_to_pos(&BaseMap, cross->haxis->flats[jb-1],
						cross->haxis->flons[jb-1], xpos);
				xbear = great_circle_bearing(&BaseMap, xpos, spos);
				xdiff = xdval - cross->haxis->dvals[jb];
				(void) great_circle_span(&BaseMap, spos, xbear,
						(xdiff * BaseMap.definition.units), epos);
				(void) pos_to_ll(&BaseMap, epos, &xflat, &xflon);

				/* Set valid time from extrapolated time in minutes */
				xvtime = calc_valid_time_minutes(cross->haxis->vtimes[0],
																	0, xtval);
				}

			/* Interpolate parameters from within cross section */
			else
				{
				xident = FpaCblank;
				xlabel = FpaCblank;

				/* Interpolate from two locations of cross section */
				if (stype)
					{
					xdiff = cross->haxis->dvals[je] - cross->haxis->dvals[jb];
					xfact = (dval - cross->haxis->dvals[jb]) / xdiff;
					}
				else
					{
					xdiff = cross->haxis->tvals[je] - cross->haxis->tvals[jb];
					xfact = (tval - cross->haxis->tvals[jb]) / xdiff;
					}
				xdiff = cross->haxis->dvals[je] - cross->haxis->dvals[jb];
				xdval = cross->haxis->dvals[jb] + (xfact * xdiff);
				xdiff = cross->haxis->tvals[je] - cross->haxis->tvals[jb];
				xtval = cross->haxis->tvals[jb] + (xfact * xdiff);
				xdir  = cross->haxis->dirs[jb];
				xspd  = cross->haxis->spds[jb];
				xdiff = cross->haxis->locs[je] - cross->haxis->locs[jb];
				xloc  = cross->haxis->locs[jb] + (xfact * xdiff);

				/* Set latitude and longitude from extrapolated location */
				(void) ll_to_pos(&BaseMap, cross->haxis->flats[jb],
						cross->haxis->flons[jb], spos);
				(void) ll_to_pos(&BaseMap, cross->haxis->flats[je],
						cross->haxis->flons[je], xpos);
				xbear = great_circle_bearing(&BaseMap, spos, xpos);
				xdiff = xdval - cross->haxis->dvals[jb];
				(void) great_circle_span(&BaseMap, spos, xbear,
						(xdiff * BaseMap.definition.units), epos);
				(void) pos_to_ll(&BaseMap, epos, &xflat, &xflon);

				/* Set valid time from interpolated time in minutes */
				xvtime = calc_valid_time_minutes(cross->haxis->vtimes[0],
																	0, xtval);
				}

			if ( Verbose )
				{
				(void) fprintf(stdout,
						"  Matched flat/flon ... %.2f/%.2f  vtime ... %s\n",
						xflat, xflon, xvtime);
				(void) fprintf(stdout,
						"  Matched dval ... %.5f  tval ... %.5f  loc ... %.5f\n",
						xdval, xtval, xloc);
				(void) fprintf(stdout,
						"  Matched dir ... %.5f  spd ... %.5f\n",
						xdir, xspd);
				}

			/* Determine order of closest match                 */
			/* Note that locations may not be in correct order! */
			for ( nx=0; nx<numx; nx++ )
				{
				if ( xloc > Haxis->locs[nx] ) continue;
				else                          break;
				}
			if ( nx < numx )
				{
				for ( nn=numx; nn>nx; nn-- )
					{
					Haxis->idents[nn] = safe_strdup(Haxis->idents[nn-1]);
					Haxis->flats[nn]  = Haxis->flats[nn-1];
					Haxis->flons[nn]  = Haxis->flons[nn-1];
					Haxis->vtimes[nn] = safe_strdup(Haxis->vtimes[nn-1]);
					Haxis->labels[nn] = safe_strdup(Haxis->labels[nn-1]);
					Haxis->pstns[nn]  = (double) nn;
					Haxis->dvals[nn]  = Haxis->dvals[nn-1];
					Haxis->tvals[nn]  = Haxis->tvals[nn-1];
					Haxis->dirs[nn]   = Haxis->dirs[nn-1];
					Haxis->spds[nn]   = Haxis->spds[nn-1];
					Haxis->locs[nn]   = Haxis->locs[nn-1];
					}
				}

			/* Set horizontal axis parameters based on closest match */
			Haxis->idents[nx] = safe_strdup(xident);
			Haxis->flats[nx]  = xflat;
			Haxis->flons[nx]  = xflon;
			Haxis->vtimes[nx] = safe_strdup(xvtime);
			Haxis->labels[nx] = safe_strdup(xlabel);
			Haxis->pstns[nx]  = (double) nx;
			Haxis->dvals[nx]  = xdval;
			Haxis->tvals[nx]  = xtval;
			Haxis->dirs[nx]   = xdir;
			Haxis->spds[nx]   = xspd;
			Haxis->locs[nx]   = xloc;
			numx++;
			}

		/* Set return parameters */
		if ( NotNull(horizontal) )
				*horizontal = Haxis->dvals[Haxis->num-1] - Haxis->dvals[0];
		return Haxis;
		}

	/* Match horizontal axis parameters from horizontal lookup table */
	else if ( !blank(loc_lookup) )
		{

		/* Get the location look up table */
		htable = get_location_lookup_table(loc_lookup);
		if ( IsNull(htable) )
			{
			(void) sprintf(err_buf,
					"Cannot find horizontal lookup table ... %s",
					loc_lookup);
			(void) error_report(err_buf);
			}
		if ( htable->numlines < 1 )
			{
			(void) sprintf(err_buf,
					"Not enough lines in location look up ... %s",
					htable->label);
			(void) error_report(err_buf);
			}

		/* Ensure that return structure is large enough */
		Haxis->num = htable->numlines;
		if ( Haxis->num > HMaxNum )
			{
			Haxis->idents = GETMEM(Haxis->idents, STRING, Haxis->num);
			Haxis->flats  = GETMEM(Haxis->flats,  float,  Haxis->num);
			Haxis->flons  = GETMEM(Haxis->flons,  float,  Haxis->num);
			Haxis->vtimes = GETMEM(Haxis->vtimes, STRING, Haxis->num);
			Haxis->labels = GETMEM(Haxis->labels, STRING, Haxis->num);
			Haxis->pstns  = GETMEM(Haxis->pstns,  double, Haxis->num);
			Haxis->dvals  = GETMEM(Haxis->dvals,  double, Haxis->num);
			Haxis->tvals  = GETMEM(Haxis->tvals,  double, Haxis->num);
			Haxis->dirs   = GETMEM(Haxis->dirs,   double, Haxis->num);
			Haxis->spds   = GETMEM(Haxis->spds,   double, Haxis->num);
			Haxis->locs   = GETMEM(Haxis->locs,   double, Haxis->num);
			HMaxNum       = Haxis->num;
			}

		/* Match parameters in lookup table with matching parameters */
		/*  in cross section structure                               */
		for ( ii=0; ii<htable->numlines; ii++ )
			{

			/* Set location from latitude and longitude */
			flat = read_lat(htable->lines[ii].lat, &valid);
			if ( !valid ) return NullPtr(XSECT_HOR_AXIS *);
			flon = read_lon(htable->lines[ii].lon, &valid);
			if ( !valid ) return NullPtr(XSECT_HOR_AXIS *);

			/* Set time */
			if ( blank(htable->lines[ii].vstring) )
				{
				vtime = interpret_timestring(TVstamp, T0stamp, clon);
				}
			else
				{
				vtime = interpret_timestring(htable->lines[ii].vstring,
																T0stamp, clon);
				}
			if ( blank(vtime) ) return NullPtr(XSECT_HOR_AXIS *);

			/* Set identifier and label */
			xident = htable->lines[ii].ident;
			xlabel = htable->lines[ii].llab;

			/* Find matching parameters in default table */
			for ( jj=0; jj<cross->haxis->num; jj++ )
				{

				if ( flat == cross->haxis->flats[jj]
						&& flon == cross->haxis->flons[jj]
						&& same(vtime, cross->haxis->vtimes[jj] ) )
					{
					if ( !blank(xident) )
						Haxis->idents[ii] = safe_strdup(xident);
					else
						Haxis->idents[ii] = safe_strdup(cross->haxis->idents[jj]);
					Haxis->flats[ii]  = cross->haxis->flats[jj];
					Haxis->flons[ii]  = cross->haxis->flons[jj];
					Haxis->vtimes[ii] = safe_strdup(vtime);
					if ( !blank(xlabel) )
						Haxis->labels[ii] = safe_strdup(xlabel);
					else
						Haxis->labels[ii] = safe_strdup(cross->haxis->labels[jj]);
					Haxis->pstns[ii]  = (double) ii;
					Haxis->dvals[ii]  = cross->haxis->dvals[jj];
					Haxis->tvals[ii]  = cross->haxis->tvals[jj];
					Haxis->dirs[ii]   = cross->haxis->dirs[jj];
					Haxis->spds[ii]   = cross->haxis->spds[jj];
					Haxis->locs[ii]   = cross->haxis->locs[jj];
					break;
					}
				}

			/* Error if no match found */
			if ( jj >= cross->haxis->num )
				{
				(void) sprintf(err_buf, "Mismatch in location look up ... %s",
						htable->label);
				(void) error_report(err_buf);
				}
			}

		/* Set return parameters */
		if ( NotNull(horizontal) )
				*horizontal = Haxis->dvals[Haxis->num-1] - Haxis->dvals[0];
		return Haxis;
		}

	/* Otherwise use default cross section horizontal axis */
	else
		{

		/* Ensure that return structure is large enough */
		Haxis->num = cross->haxis->num;
		if ( Haxis->num > HMaxNum )
			{
			Haxis->idents = GETMEM(Haxis->idents, STRING, Haxis->num);
			Haxis->flats  = GETMEM(Haxis->flats,  float,  Haxis->num);
			Haxis->flons  = GETMEM(Haxis->flons,  float,  Haxis->num);
			Haxis->vtimes = GETMEM(Haxis->vtimes, STRING, Haxis->num);
			Haxis->labels = GETMEM(Haxis->labels, STRING, Haxis->num);
			Haxis->pstns  = GETMEM(Haxis->pstns,  double, Haxis->num);
			Haxis->dvals  = GETMEM(Haxis->dvals,  double, Haxis->num);
			Haxis->tvals  = GETMEM(Haxis->tvals,  double, Haxis->num);
			Haxis->dirs   = GETMEM(Haxis->dirs,   double, Haxis->num);
			Haxis->spds   = GETMEM(Haxis->spds,   double, Haxis->num);
			Haxis->locs   = GETMEM(Haxis->locs,   double, Haxis->num);
			HMaxNum       = Haxis->num;
			}

		/* Set axis parameters from cross section structure */
		for ( ii=0; ii<Haxis->num; ii++ )
			{
			Haxis->idents[ii] = safe_strdup(cross->haxis->idents[ii]);
			Haxis->flats[ii]  = cross->haxis->flats[ii];
			Haxis->flons[ii]  = cross->haxis->flons[ii];
			Haxis->vtimes[ii] = safe_strdup(cross->haxis->vtimes[ii]);
			Haxis->labels[ii] = safe_strdup(cross->haxis->labels[ii]);
			Haxis->pstns[ii]  = cross->haxis->pstns[ii];
			Haxis->dvals[ii]  = cross->haxis->dvals[ii];
			Haxis->tvals[ii]  = cross->haxis->tvals[ii];
			Haxis->dirs[ii]   = cross->haxis->dirs[ii];
			Haxis->spds[ii]   = cross->haxis->spds[ii];
			Haxis->locs[ii]   = cross->haxis->locs[ii];
			}

		/* Set return parameters */
		if ( NotNull(horizontal) )
				*horizontal = Haxis->dvals[Haxis->num-1] - Haxis->dvals[0];
		return Haxis;
		}
	}

LOGICAL		define_cross_section_vertical_axis

	(
	GRA_XSECT	*cross,			/* cross section structure */
	STRING		ver_lookup,		/* vertical lookup table */
	double		*vertical		/* size of vertical axis */
	)

	{
	int					ii;
	VERT_LOOKUP_TABLE	*vtable;
	char				err_buf[GPGLong];

	/* Error return for missing parameters */
	if ( IsNull(cross) )     return FALSE;
	if ( blank(ver_lookup) ) return FALSE;

	/* Get the vertical lookup table */
	vtable = get_vertical_lookup_table(ver_lookup);
	if ( IsNull(vtable) )
		{
		(void) sprintf(err_buf, "Cannot find vertical lookup table ... %s",
				ver_lookup);
		(void) error_report(err_buf);
		}
	if ( vtable->numlines <= 1 )
		{
		(void) sprintf(err_buf, "Not enough lines in vertical lookup ... %s",
				vtable->label);
		(void) error_report(err_buf);
		}

	/* Initialize vertical axis in cross section structure (if required) */
	if ( IsNull(cross->vaxis) )
		{

		/* Initialize parameters in cross section structure */
		cross->vaxis = INITMEM(XSECT_VER_AXIS, 1);
		cross->vaxis->num    = vtable->numlines;
		cross->vaxis->idents = INITMEM(STRING, cross->vaxis->num);
		cross->vaxis->labels = INITMEM(STRING, cross->vaxis->num);
		cross->vaxis->pstns  = INITMEM(double, cross->vaxis->num);
		cross->vaxis->vals   = INITMEM(double, cross->vaxis->num);
		cross->vaxis->locs   = INITMEM(double, cross->vaxis->num);

		/* Set axis parameters from vertical locations */
		for ( ii=0; ii<vtable->numlines; ii++ )
			{
			cross->vaxis->idents[ii] = safe_strdup(vtable->lines[ii].ident);
			cross->vaxis->labels[ii] = safe_strdup(vtable->lines[ii].llab);
			cross->vaxis->pstns[ii]  = (double) ii;
			cross->vaxis->vals[ii]   = vtable->lines[ii].yvalue;
			cross->vaxis->locs[ii]   = vtable->lines[ii].ylocation / 100.0;
			}

		/* Set return parameters */
		if ( NotNull(vertical) ) *vertical = 1.0;
		return TRUE;
		}

	/* Error if cross section already initialized */
	else
		{
		(void) sprintf(err_buf,
				"Cross section %s vertical already initialized!",
				cross->label);
		(void) error_report(err_buf);
		return FALSE;
		}

	}

XSECT_VER_AXIS	*cross_section_vertical_axis

	(
	GRA_XSECT	*cross,			/* cross section structure */
	STRING		ver_lookup,		/* vertical lookup table */
	double		*vertical		/* size of vertical axis */
	)

	{
	int					ii;
	VERT_LOOKUP_TABLE	*vtable;
	char				err_buf[GPGLong];

	/* Storage for current vertical axis positions */
	static	XSECT_VER_AXIS	*Vaxis  = NullPtr(XSECT_VER_AXIS *);
	static	int				VMaxNum = 0;

	/* Initialize structure for vertical axis positions */
	if ( IsNull(Vaxis) )
		{
		Vaxis = INITMEM(XSECT_VER_AXIS, 1);
		Vaxis->num    = 0;
		Vaxis->idents = NullStringList;
		Vaxis->labels = NullStringList;
		Vaxis->pstns  = NullDouble;
		Vaxis->vals   = NullDouble;
		Vaxis->locs   = NullDouble;
		}

	/* Free allocated memory in structure for vertical axis positions */
	else
		{
		for ( ii=0; ii<Vaxis->num; ii++ )
			{
			FREEMEM(Vaxis->idents[ii]);
			FREEMEM(Vaxis->labels[ii]);
			}
		}

	/* Error return for missing parameters */
	if ( IsNull(cross) )        return NullPtr(XSECT_VER_AXIS *);
	if ( IsNull(cross->vaxis) ) return NullPtr(XSECT_VER_AXIS *);

	/* Get the vertical lookup table */
	if ( !blank(ver_lookup) )
		{
		vtable = get_vertical_lookup_table(ver_lookup);
		if ( IsNull(vtable) )
			{
			(void) sprintf(err_buf,
					"Cannot find vertical lookup table ... %s",
					ver_lookup);
			(void) error_report(err_buf);
			}
		}
	else
		{
		vtable = get_vertical_lookup_table(cross->vertical_lookup);
		}
	if ( vtable->numlines <= 1 )
		{
		(void) sprintf(err_buf, "Not enough lines in vertical lookup ... %s",
				vtable->label);
		(void) error_report(err_buf);
		}

	/* Set vertical axis parameters from cross section structure */
	if ( same(cross->vertical_lookup, vtable->label) )
		{

		/* Ensure that return structure is large enough */
		Vaxis->num = cross->vaxis->num;
		if ( Vaxis->num > VMaxNum )
			{
			Vaxis->idents = GETMEM(Vaxis->idents, STRING, Vaxis->num);
			Vaxis->labels = GETMEM(Vaxis->labels, STRING, Vaxis->num);
			Vaxis->pstns  = GETMEM(Vaxis->pstns,  double, Vaxis->num);
			Vaxis->vals   = GETMEM(Vaxis->vals,   double, Vaxis->num);
			Vaxis->locs   = GETMEM(Vaxis->locs,   double, Vaxis->num);
			VMaxNum       = Vaxis->num;
			}

		/* Set axis parameters from cross section structure */
		for ( ii=0; ii<Vaxis->num; ii++ )
			{
			Vaxis->idents[ii] = safe_strdup(cross->vaxis->idents[ii]);
			Vaxis->labels[ii] = safe_strdup(cross->vaxis->labels[ii]);
			Vaxis->pstns[ii]  = cross->vaxis->pstns[ii];
			Vaxis->vals[ii]   = cross->vaxis->vals[ii];
			Vaxis->locs[ii]   = cross->vaxis->locs[ii];
			}

		/* Set return parameters */
		if ( NotNull(vertical) ) *vertical = 1.0;
		return Vaxis;
		}

	/* Set vertical axis parameters from new vertical lookup table */
	else
		{

		/* Ensure that return structure is large enough */
		Vaxis->num = vtable->numlines;
		if ( Vaxis->num > VMaxNum )
			{
			Vaxis->idents = GETMEM(Vaxis->idents, STRING, Vaxis->num);
			Vaxis->labels = GETMEM(Vaxis->labels, STRING, Vaxis->num);
			Vaxis->pstns  = GETMEM(Vaxis->pstns,  double, Vaxis->num);
			Vaxis->vals   = GETMEM(Vaxis->vals,   double, Vaxis->num);
			Vaxis->locs   = GETMEM(Vaxis->locs,   double, Vaxis->num);
			VMaxNum       = Vaxis->num;
			}

		/* Set axis parameters from vertical locations */
		for ( ii=0; ii<vtable->numlines; ii++ )
			{
			Vaxis->idents[ii] = safe_strdup(vtable->lines[ii].ident);
			Vaxis->labels[ii] = safe_strdup(vtable->lines[ii].llab);
			Vaxis->pstns[ii]  = (double) ii;
			Vaxis->vals[ii]   = vtable->lines[ii].yvalue;
			Vaxis->locs[ii]   = vtable->lines[ii].ylocation / 100.0;
			}

		/* Set return parameters */
		if ( NotNull(vertical) ) *vertical = 1.0;
		return Vaxis;
		}
	}

/***********************************************************************
*                                                                      *
*    a d d _ s a m p l e _ g r i d                                     *
*    g e t _ s a m p l e _ g r i d                                     *
*    f r e e _ s a m p l e _ g r i d _ l o c a t i o n s               *
*                                                                      *
***********************************************************************/

/* Storage for named grids */
static	int			NumGrids = 0;
static	GRA_GRID	*Grids   = NullPtr(GRA_GRID *);

LOGICAL			add_sample_grid

	(
	STRING		grid_name,	/* grid name */
	STRING		lat_bgn,	/* begin latitude for sampling */
	STRING		lat_end,	/* end latitude for sampling */
	STRING		lat_int,	/* latitude interval for sampling */
	STRING		lon_bgn,	/* begin longitude for sampling */
	STRING		lon_end,	/* end longitude for sampling */
	STRING		lon_int,	/* longitude interval for sampling */
	STRING		mapx_bgn,	/* begin map x location for sampling */
	STRING		mapx_end,	/* end map x location for sampling */
	STRING		mapx_int,	/* map x interval for sampling */
	STRING		mapy_bgn,	/* begin map y location for sampling */
	STRING		mapy_end,	/* end map y location for sampling */
	STRING		mapy_int,	/* map y interval for sampling */
	float		map_units,	/* map units */
	float		xsoff,		/* grid offset in x direction for display */
	float		ysoff		/* grid offset in y direction for display */
	)

	{
	int			ii, ngrd, numx, numy, iix, iiy;
	LOGICAL		valid;
	float		flat, flatb, flate, flatx;
	float		flon, flonb, flone, flonx;
	float		mapxb, mapxe, mapxx;
	float		mapyb, mapye, mapyx;
	float		fact;
	POINT		pos;
	float		*plats, *plons;
	char		err_buf[GPGLong];

	/* Check that sample grid has not already been defined */
	for ( ii=0; ii<NumGrids; ii++ )
		{
		if ( same(grid_name, Grids[ii].label) ) break;
		}

	/* Re-define the sample grid parameters */
	if (ii < NumGrids)
		{
		ngrd = ii;
		free_sample_grid_locations(&Grids[ngrd]);
		(void) sprintf(err_buf,
				"Re-defining parameters for sample grid: %s", grid_name);
		(void) warn_report(err_buf);
		}

	/* Add another sampling grid to the list */
	else
		{
		NumGrids++;
		Grids = GETMEM(Grids, GRA_GRID, NumGrids);
		ngrd = NumGrids - 1;
		}

	/* Define the grid locations using latitude/longitude parameters */
	if ( ( !blank(lat_bgn) && !blank(lat_end) && !blank(lat_int)
			&& !blank(lon_bgn) && !blank(lon_end) && !blank(lon_int) ) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "  Defining sample grid ... %s\n",
					grid_name);
			(void) fprintf(stdout, "   With parameters  lat_begin/lat_end/lat_interval ... %s %s %s\n",
					lat_bgn, lat_end, lat_int);
			(void) fprintf(stdout, "               and  lon_begin/lon_end/lon_interval ... %s %s %s\n",
					lon_bgn, lon_end, lon_int);
			}

		/* Read the latitude parameters */
		flatb = read_lat(lat_bgn, &valid);
		if ( !valid )
			{
			(void) sprintf(err_buf, "Invalid lat_begin ... %s", lat_bgn);
			(void) error_report(err_buf);
			}
		flate = read_lat(lat_end, &valid);
		if ( !valid )
			{
			(void) sprintf(err_buf, "Invalid lat_end ... %s", lat_end);
			(void) error_report(err_buf);
			}
		flatx = read_lat(lat_int, &valid);
		if ( !valid || flatx == 0.0 )
			{
			(void) sprintf(err_buf, "Invalid lat_interval ... %s", lat_int);
			(void) error_report(err_buf);
			}

		/* Read the longitude parameters */
		flonb = read_lon(lon_bgn, &valid);
		if ( !valid )
			{
			(void) sprintf(err_buf, "Invalid lon_begin ... %s", lon_bgn);
			(void) error_report(err_buf);
			}
		flone = read_lon(lon_end, &valid);
		if ( !valid )
			{
			(void) sprintf(err_buf, "Invalid lon_end ... %s", lon_end);
			(void) error_report(err_buf);
			}
		flonx = read_lon(lon_int, &valid);
		if ( !valid || flonx == 0.0 )
			{
			(void) sprintf(err_buf, "Invalid lon_interval ... %s", lon_int);
			(void) error_report(err_buf);
			}

		/* Set the range for latitude (y axis) */
		numy = NINT( (flate - flatb) / flatx ) + 1;
		if ( numy < 1 )
			{
			(void) sprintf(err_buf,
					"Problem with latitude range from  lat_begin/lat_end/lat_interval ... %s %s %s",
					lat_bgn, lat_end, lat_int);
			(void) error_report(err_buf);
			}

		/* Adjust the longitudes (depending on sign of increment) */
		if      ( flonx > 0 && flone < flonb ) flone += 360.0;
		else if ( flonx < 0 && flonb < flone ) flonb += 360.0;

		/* Set the range for longitude (x axis) */
		numx = NINT( (flone - flonb) / flonx ) + 1;
		if ( numx < 1 )
			{
			(void) sprintf(err_buf,
					"Problem with longitude range from  lon_begin/lon_end/lon_interval ... %s %s %s",
					lon_bgn, lon_end, lon_int);
			(void) error_report(err_buf);
			}

		/* Define the initial grid parameters */
		(void) strcpy(Grids[ngrd].label, grid_name);
		Grids[ngrd].numx    = numx;
		Grids[ngrd].numy    = numy;
		Grids[ngrd].x_shift = xsoff;
		Grids[ngrd].y_shift = ysoff;

		/* Allocate space for arrays of latitude/longitude locations */
		plats = INITMEM(float, numx*numy);
		plons = INITMEM(float, numx*numy);
		Grids[ngrd].flats = INITMEM(float *, numy);
		Grids[ngrd].flons = INITMEM(float *, numy);

		/* Set the array of latitudes and longitudes */
		for ( iiy=0; iiy<numy; iiy++ )
			{
			flat = flatb + (float) iiy * flatx;
			Grids[ngrd].flats[iiy] = plats + iiy*numx;
			Grids[ngrd].flons[iiy] = plons + iiy*numx;
			for ( iix=0; iix<numx; iix++ )
				{
				flon = flonb + (float) iix * flonx;
				Grids[ngrd].flats[iiy][iix] = flat;
				Grids[ngrd].flons[iiy][iix] = flon;
				}
			}

		/* Return when grid latitudes/longitudes have been set */
		if ( Verbose )
			{
			(void) fprintf(stdout, "  Latitudes for sample grid ... %s\n",
					Grids[ngrd].label);
			for ( iiy=0; iiy<Grids[ngrd].numy; iiy++ )
				{
				(void) fprintf(stdout, "    Row: %d ", iiy);
				for ( iix=0; iix<Grids[ngrd].numx; iix++ )
					{
					(void) fprintf(stdout, " %.2f", Grids[ngrd].flats[iiy][iix]);
					}
				(void) fprintf(stdout, "\n");
				}
			(void) fprintf(stdout, "  Longitudes for sample grid ... %s\n",
					Grids[ngrd].label);
			for ( iiy=0; iiy<Grids[ngrd].numy; iiy++ )
				{
				(void) fprintf(stdout, "    Row: %d ", iiy);
				for ( iix=0; iix<Grids[ngrd].numx; iix++ )
					{
					(void) fprintf(stdout, " %.2f", Grids[ngrd].flons[iiy][iix]);
					}
				(void) fprintf(stdout, "\n");
				}
			}
		return TRUE;
		}

	/* Define the grid locations using map_x/map_y parameters */
	else if ( ( !blank(mapx_bgn) && !blank(mapx_end) && !blank(mapx_int)
			&& !blank(mapy_bgn) && !blank(mapy_end) && !blank(mapy_int) ) )
		{

		if ( Verbose )
			{
			(void) fprintf(stdout, "  Defining sample grid ... %s\n",
					grid_name);
			(void) fprintf(stdout, "   With parameters  map_x_begin/map_x_end/map_x_interval ... %s %s %s\n",
					mapx_bgn, mapx_end, mapx_int);
			(void) fprintf(stdout, "               and  map_y_begin/map_y_end/map_y_interval ... %s %s %s\n",
					mapy_bgn, mapy_end, mapy_int);
			(void) fprintf(stdout, "               and  map_units ... %.2f\n",
					map_units);
			}

		/* Read the map_x parameters */
		(void) sscanf(mapx_bgn, "%f", &mapxb);
		(void) sscanf(mapx_end, "%f", &mapxe);
		(void) sscanf(mapx_int, "%f", &mapxx);

		/* Read the map_y parameters */
		(void) sscanf(mapy_bgn, "%f", &mapyb);
		(void) sscanf(mapy_end, "%f", &mapye);
		(void) sscanf(mapy_int, "%f", &mapyx);

		/* Set the map adjustment factor */
		fact = map_units/ BaseMap.definition.units;

		/* Set the range for x axis */
		numx = NINT( (mapxe - mapxb) / mapxx ) + 1;
		if ( numx < 1 )
			{
			(void) sprintf(err_buf,
					"Problem with x axis range from  map_x_begin/map_x_end/map_x_interval ... %s %s %s",
					mapx_bgn, mapx_end, mapx_int);
			(void) error_report(err_buf);
			}

		/* Set the range for y axis */
		numy = NINT( (mapye - mapyb) / mapyx ) + 1;
		if ( numy < 1 )
			{
			(void) sprintf(err_buf,
					"Problem with y axis range from  map_y_begin/map_y_end/map_y_interval ... %s %s %s",
					mapy_bgn, mapy_end, mapy_int);
			(void) error_report(err_buf);
			}

		/* Define the initial grid parameters */
		(void) strcpy(Grids[ngrd].label, grid_name);
		Grids[ngrd].numx    = numx;
		Grids[ngrd].numy    = numy;
		Grids[ngrd].x_shift = xsoff;
		Grids[ngrd].y_shift = ysoff;

		/* Allocate space for arrays of latitude/longitude locations */
		plats = INITMEM(float, numx*numy);
		plons = INITMEM(float, numx*numy);
		Grids[ngrd].flats = INITMEM(float *, numy);
		Grids[ngrd].flons = INITMEM(float *, numy);

		/* Set the array of latitudes and longitudes */
		for ( iiy=0; iiy<numy; iiy++ )
			{
			pos[Y]  = mapyb + (float) iiy * mapyx;
			pos[Y] *= fact;
			Grids[ngrd].flats[iiy] = plats + iiy*numx;
			Grids[ngrd].flons[iiy] = plons + iiy*numx;
			for ( iix=0; iix<numx; iix++ )
				{
				pos[X]  = mapxb + (float) iix * mapxx;
				pos[X] *= fact;
				(void) pos_to_ll(&BaseMap, pos, &flat, &flon);
				Grids[ngrd].flats[iiy][iix] = flat;
				Grids[ngrd].flons[iiy][iix] = flon;
				}
			}

		/* Return when grid latitudes/longitudes have been set */
		if ( Verbose )
			{
			(void) fprintf(stdout, "  Latitudes for sample grid ... %s\n",
					Grids[ngrd].label);
			for ( iiy=0; iiy<Grids[ngrd].numy; iiy++ )
				{
				(void) fprintf(stdout, "    Row: %d ", iiy);
				for ( iix=0; iix<Grids[ngrd].numx; iix++ )
					{
					(void) fprintf(stdout, " %.2f", Grids[ngrd].flats[iiy][iix]);
					}
				(void) fprintf(stdout, "\n");
				}
			(void) fprintf(stdout, "  Longitudes for sample grid ... %s\n",
					Grids[ngrd].label);
			for ( iiy=0; iiy<Grids[ngrd].numy; iiy++ )
				{
				(void) fprintf(stdout, "    Row: %d ", iiy);
				for ( iix=0; iix<Grids[ngrd].numx; iix++ )
					{
					(void) fprintf(stdout, " %.2f", Grids[ngrd].flons[iiy][iix]);
					}
				(void) fprintf(stdout, "\n");
				}
			}
		return TRUE;
		}

	/* Error if missing parameters */
	else
		{
		(void) error_report("Missing lat/lon or map_x/map_y parameters");
		}
	}

GRA_GRID	*get_sample_grid

	(
	STRING		grid_name	/* grid name */
	)

	{
	int			ii;

	/* Return grid with matching name */
	for ( ii=0; ii<NumGrids; ii++ )
		{
		if ( same(grid_name, Grids[ii].label) ) return &Grids[ii];
		}

	/* Error return if grid not found */
	return NullPtr(GRA_GRID *);
	}

void		free_sample_grid_locations

	(
	GRA_GRID		*sgrd			/* sample grid structure */
	)

	{
	int					ii;

	/* Error return for missing parameters */
	if ( IsNull(sgrd) )                       return;
	if ( sgrd->numx == 0 && sgrd->numy == 0 ) return;

	/* Free space used by sample grid latitudes and longitudes */
	FREEMEM(*sgrd->flats);
	FREEMEM(sgrd->flats);
	FREEMEM(*sgrd->flons);
	FREEMEM(sgrd->flons);
	}

/***********************************************************************
*                                                                      *
*    a d d _ s a m p l e _ l i s t                                     *
*    g e t _ s a m p l e _ l i s t                                     *
*    f r e e _ s a m p l e _ l i s t _ l o c a t i o n s               *
*                                                                      *
***********************************************************************/

/* Storage for named lists */
static	int			NumLists = 0;
static	GRA_LIST	*Lists   = NullPtr(GRA_LIST *);

LOGICAL			add_sample_list

	(
	STRING		list_name,	/* list name */
	int			num_lctns,	/* number of sample locations */
	GRA_LCTN	*lctns,		/* sample location information */
	float		map_units,	/* map units */
	float		xsoff,		/* list offset in x direction for display */
	float		ysoff,		/* list offset in y direction for display */
	int			xwrap,		/* list count in x direction for display */
	int			ywrap		/* list count in y direction for display */
	)

	{
	int			ii, nlst, inum;
	float		fact, flat, flon;
	POINT		pos;
	char		err_buf[GPGLong];

	/* Check that sample list has not already been defined */
	for ( ii=0; ii<NumLists; ii++ )
		{
		if ( same(list_name, Lists[ii].label) ) break;
		}

	/* Re-define the sample grid parameters */
	if (ii < NumLists)
		{
		nlst = ii;
		free_sample_list_locations(&Lists[nlst]);
		(void) sprintf(err_buf,
				"Re-defining parameters for sample list: %s", list_name);
		(void) warn_report(err_buf);
		}

	/* Add another sampling grid to the list */
	else
		{
		NumLists++;
		Lists = GETMEM(Lists, GRA_LIST, NumLists);
		nlst = NumLists - 1;
		}

	/* Define the initial list parameters */
	(void) strcpy(Lists[nlst].label, list_name);
	Lists[nlst].num     = num_lctns;
	Lists[nlst].usell   = NullLogicalList;
	Lists[nlst].flats   = NullFloat;
	Lists[nlst].flons   = NullFloat;
	Lists[nlst].idents  = NullStringList;
	Lists[nlst].x_shift = xsoff;
	Lists[nlst].y_shift = ysoff;
	Lists[nlst].x_wrap  = xwrap;
	Lists[nlst].y_wrap  = ywrap;

	/* Allocate space for sample list locations */
	Lists[nlst].usell  = INITMEM(LOGICAL, num_lctns);
	Lists[nlst].flats  = INITMEM(float,   num_lctns);
	Lists[nlst].flons  = INITMEM(float,   num_lctns);
	Lists[nlst].idents = INITMEM(STRING,  num_lctns);

	/* Set the map adjustment factor */
	fact = map_units/ BaseMap.definition.units;

	if ( Verbose )
		{
		(void) fprintf(stdout, "  Setting locations for sample list ... %s\n",
				Lists[nlst].label);
		}

	/* Set sample list locations based on type of sampling */
	for ( inum=0; inum<num_lctns; inum++ )
		{

		/* Set sample list locations using latitude/longitude parameters */
		if ( lctns[inum].macro == GPG_LatLon )
			{
			Lists[nlst].usell[inum]  = TRUE;
			Lists[nlst].flats[inum]  = lctns[inum].xval;
			Lists[nlst].flons[inum]  = lctns[inum].yval;
			Lists[nlst].idents[inum] = safe_strdup(lctns[inum].ident);

			if ( Verbose )
				{
				(void) fprintf(stdout, "    Lat/Lon Location: %d ", inum);
				(void) fprintf(stdout, "  Lat/Lon: %.2f  %.2f  Ident: %s\n",
						Lists[nlst].flats[inum], Lists[nlst].flons[inum],
						Lists[nlst].idents[inum]);
				}
			}

		/* Set sample list locations using map_x/map_x parameters */
		else if ( lctns[inum].macro == GPG_MapXY )
			{
			Lists[nlst].usell[inum] = TRUE;
			pos[X] = lctns[inum].xval * fact;
			pos[Y] = lctns[inum].yval * fact;
			(void) pos_to_ll(&BaseMap, pos, &flat, &flon);
			Lists[nlst].flats[inum]  = flat;
			Lists[nlst].flons[inum]  = flon;
			Lists[nlst].idents[inum] = safe_strdup(lctns[inum].ident);

			if ( Verbose )
				{
				(void) fprintf(stdout, "    Map X/Y Location: %d ", inum);
				(void) fprintf(stdout, "  Lat/Lon: %.2f  %.2f  Ident: %s\n",
						Lists[nlst].flats[inum], Lists[nlst].flons[inum],
						Lists[nlst].idents[inum]);
				}
			}

		/* Set sample list locations using location identifiers */
		else if ( lctns[inum].macro == GPG_Ident )
			{
			Lists[nlst].usell[inum]  = FALSE;
			Lists[nlst].flats[inum]  = lctns[inum].xval;
			Lists[nlst].flons[inum]  = lctns[inum].yval;
			Lists[nlst].idents[inum] = safe_strdup(lctns[inum].ident);

			if ( Verbose )
				{
				(void) fprintf(stdout, "    Location: %d ", inum);
				(void) fprintf(stdout, "  Location identifier: %s\n",
						Lists[nlst].idents[inum]);
				}
			}
		}

	/* Return when list locations have been set */
	return TRUE;
	}

GRA_LIST	*get_sample_list

	(
	STRING		list_name	/* list name */
	)

	{
	int			ii;

	/* Return list with matching name */
	for ( ii=0; ii<NumLists; ii++ )
		{
		if ( same(list_name, Lists[ii].label) ) return &Lists[ii];
		}

	/* Error return if list not found */
	return NullPtr(GRA_LIST *);
	}

void		free_sample_list_locations

	(
	GRA_LIST		*slst			/* sample list structure */
	)

	{
	int					ii;

	/* Error return for missing parameters */
	if ( IsNull(slst) )   return;
	if ( slst->num == 0 ) return;

	/* Free space used by sample grid latitudes and longitudes */
	FREEMEM(slst->usell);
	FREEMEM(slst->flats);
	FREEMEM(slst->flons);
	FREELIST(slst->idents, slst->num);
	}

/***********************************************************************
*                                                                      *
*    a d d _ t a b l e                                                 *
*    g e t _ t a b l e                                                 *
*                                                                      *
***********************************************************************/

/* Storage for named tables */
static	int			NumTables = 0;
static	GRA_TABLE	*Tables   = NullPtr(GRA_TABLE *);

LOGICAL			add_table

	(
	STRING		table_name,	/* table name */
	STRING		type,		/* table type */
	float		xoff,		/* table x offset */
	float		yoff		/* table y offset */
	)

	{
	int			ii, ntbl;
	float		xx, yy;
	char		err_buf[GPGLong];

	/* Check that table has not already been defined */
	for ( ii=0; ii<NumTables; ii++ )
		{
		if ( same(table_name, Tables[ii].label) )
			{
			(void) sprintf(err_buf,
					"Attempting to re-define table: %s", table_name);
			(void) error_report(err_buf);
			}
		}

	/* Set table offset */
	/* >>> is this required??? xx and yy are not used!!! <<< */
	(void) anchored_location(ZeroPoint, xoff, yoff, &xx, &yy);

	/* Add another table to the list */
	NumTables++;
	Tables = GETMEM(Tables, GRA_TABLE, NumTables);
	ntbl = NumTables - 1;

	/* Define the table parameters */
	(void) strcpy(Tables[ntbl].label, table_name);
	(void) strcpy(Tables[ntbl].type,  type);
	Tables[ntbl].x_off  = xoff;
	Tables[ntbl].y_off  = yoff;
	Tables[ntbl].nsites = 0;
	Tables[ntbl].usell  = NullLogicalList;
	Tables[ntbl].flats  = NullFloat;
	Tables[ntbl].flons  = NullFloat;
	Tables[ntbl].idents = NullStringList;
	Tables[ntbl].offset = NullFloat;

	/* Return TRUE */
	return TRUE;
	}

GRA_TABLE	*get_table

	(
	STRING		table_name	/* table name */
	)

	{
	int			ii;

	/* Return table with matching name */
	for ( ii=0; ii<NumTables; ii++ )
		{
		if ( same(table_name, Tables[ii].label) ) return &Tables[ii];
		}

	/* Error return if table not found */
	return NullPtr(GRA_TABLE *);
	}

/***********************************************************************
*                                                                      *
*    a d d _ l e g e n d                                               *
*    d i s p l a y _ l e g e n d                                       *
*                                                                      *
***********************************************************************/

/* Structure for holding parameters from @legend directives */
typedef struct
	{
	int		num;
	STRING	*symbol;
	STRING	*string;
	float	*scale;
	float	*txt_size;
	float	*rotation;
	float	*xx;
	float	*yy;
	PRES	*presentation;
	} GRA_LEGEND;

/* Storage for @legend parameters */
#define	NO_LEGEND	{0, NullStringList, NullStringList, NullFloat, NullFloat, \
						NullFloat, NullFloat, NullFloat, NullPtr(PRES *)}
static	GRA_LEGEND	Legend = NO_LEGEND;

LOGICAL		add_legend

	(
	STRING		symbol,		/* legend symbol file name */
	STRING		string,		/* legend text string */
	float		scale,
	float		txt_size,
	float		rotation,
	float		xoff,
	float		yoff,
	PRES		*presentation
	)

	{
	int			nl;

	/* Add another legend to the list */
	nl = Legend.num++;
	Legend.symbol       = GETMEM(Legend.symbol,       STRING, Legend.num);
	Legend.string       = GETMEM(Legend.string,       STRING, Legend.num);
	Legend.scale        = GETMEM(Legend.scale,        float,  Legend.num);
	Legend.txt_size     = GETMEM(Legend.txt_size,     float,  Legend.num);
	Legend.rotation     = GETMEM(Legend.rotation,     float,  Legend.num);
	Legend.xx           = GETMEM(Legend.xx,           float,  Legend.num);
	Legend.yy           = GETMEM(Legend.yy,           float,  Legend.num);
	Legend.presentation = GETMEM(Legend.presentation, PRES,   Legend.num);

	/* Define the legend parameters */
	Legend.symbol[nl]   = safe_strdup(symbol);
	Legend.string[nl]   = safe_strdup(string);
	Legend.scale[nl]    = scale;
	Legend.txt_size[nl] = txt_size;
	Legend.rotation[nl] = rotation;
	Legend.xx[nl]       = xoff;
	Legend.yy[nl]       = yoff;

	/* Save the legend presentation */
	(void) copy_presentation(&(Legend.presentation[nl]), presentation);

	/* Return TRUE */
	return TRUE;
	}

LOGICAL		display_legend

	(
	)

	{
	int			ii;
	float		xx, yy;
	PRES		temp_pres;

	/* Return if no legend members to output */
	if ( Legend.num <= 0 ) return TRUE;

	/* Save the current presentation */
	(void) copy_presentation(&temp_pres, &CurPres);

	/* Reset anchor position to centre of page */
	(void) define_graphics_anchor(AnchorAbsolute, 0.0, 0.0, 0.0, 0.0,
																	FpaCblank);

	/* Output all members of list as grouped symbols and text strings */
	for ( ii=0; ii<Legend.num; ii++ )
		{

		/* Reset the current presentation to display the legend */
		(void) copy_presentation(&CurPres, &(Legend.presentation[ii]));

		/* Adjust the position wrt centre of page */
		(void) anchored_location(ZeroPoint, Legend.xx[ii], Legend.yy[ii],
																	&xx, &yy);

		/* Now display each symbol */
		(void) write_graphics_group(GPGstart, NullPointer, 0);
		(void) write_graphics_symbol(Legend.symbol[ii], xx, yy,
				Legend.scale[ii], Legend.rotation[ii]);

		/* Now display each text string */
		(void) write_graphics_text(Legend.string[ii], xx, yy,
				Legend.txt_size[ii], Legend.presentation[ii].justified,
				Legend.rotation[ii], TRUE);
		(void) write_graphics_group(GPGend, NullPointer, 0);
		}

	/* Restore the original presentation */
	(void) copy_presentation(&CurPres, &temp_pres);

	/* Return TRUE */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    c h e c k _ p s m e t _ k e y w o r d                             *
*                                                                      *
*    c h e c k _ s v g m e t _ k e y w o r d                           *
*                                                                      *
*    c h e c k _ c o r m e t _ k e y w o r d                           *
*                                                                      *
*    c h e c k _ t e x m e t _ k e y w o r d                           *
*                                                                      *
***********************************************************************/

#define LargestPSmetDistance		2160.0			/* points */
#define LargestSVGmetDistance		2160.0			/* points */
#define LargestCormetDistance		30000.0			/* 1000ths of inches */

void		check_psmet_keyword

	(
	STRING		key,
	STRING		action
	)

	{
	char		tbuf[GPGMedium], xbuf[GPGMedium];
	float		value;
	LOGICAL		status, even;
	char		err_buf[GPGLong];

	/* Check for @presentation keyword "line_width"            */
	/*  or @label/@sample_field keyword "attribute_line_width" */
	if ( same(key, "line_width")
			|| same(key, "attribute_line_width")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestPSmetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		if ( value < 0.001 )
			(void) sprintf(tbuf, "0.001");
		else
			(void) sprintf(tbuf, "%.3f", value);
		}

	/* Check for @presentation keyword "line_style"            */
	/*  or @label/@sample_field keyword "attribute_line_style" */
	else if ( same(key, "line_style")
			|| same(key, "attribute_line_style")
			)
		{
		if ( same_ic(action, "solid") )
			(void) sprintf(tbuf, "[] 0");
		else
			{
			/* Strip off the first parameter before copying! */
			(void) string_arg(action);
			(void) sprintf(tbuf, "[%s ] 0", action);
			}
		}

	/* Check for @presentation keywords "outline" "fill" "interior_fill"      */
	/*  or @label/@sample_field keywords "attribute_outline" "attribute_fill" */
	else if ( same(key, "outline")
			|| same(key, "fill")
			|| same(key, "interior_fill")
			|| same(key, "attribute_outline")
			|| same(key, "attribute_fill")
			)
		{

		/* Parameters for no outline or fill or interior_fill */
		if ( same_ic(action, ColourNone) )
			{
			(void) sprintf(tbuf, ColourNone);
			}

		/* Parameters for specified outline or fill or interior_fill */
		else
			{

			/* Strip off the type of specification and convert the remainder */
			(void) strcpy(xbuf, string_arg(action));
			if ( same_ic(xbuf, "CMYK") )
				{
				if ( !convert_cmyk_for_psmet(action) )
						(void) error_report("Improper CMYK parameters");
				}
			else if ( same_ic(xbuf, "RGB") )
				{
				if ( !convert_rgb_for_psmet(action) )
						(void) error_report("Improper RGB parameters");
				}
			else if ( same_ic(xbuf, "X11") )
				{
				if ( !convert_x11_for_psmet(action) )
						(void) error_report("Improper X11 Name parameters");
				}
			else
				(void) error_report("Invalid Colour Specification - Must begin with CMYK or RGB or X11");

			/* Copy the converted RGB colour specification to the buffer */
			(void) sprintf(tbuf, "%s", action);
			}
		}

	/* Check for @presentation keywords "pattern_width" "pattern_length" */
	else if ( same(key, "pattern_width")
			|| same(key, "pattern_length") )
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestPSmetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void) sprintf(tbuf, "%f", value);
		}

	/* Check for @presentation keyword "font"            */
	/*  or @label/@sample_field keyword "attribute_font" */
	else if ( same(key, "font")
			|| same(key, "attribute_font")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for @presentation keyword "font_weight"            */
	/*  or @label/@sample_field keyword "attribute_font_weight" */
	/* Note that these are presently ignored by PSMet!          */
	else if ( same(key, "font_weight")
			|| same(key, "attribute_font_weight")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for logical @presentation keyword "italics"    */
	/*  or @label/@sample_field keyword "attribute_italics" */
	/* Note that these are presently ignored by PSMet!      */
	else if ( same(key, "italics")
			|| same(key, "attribute_italics")
			)
		{
		if ( same_ic(action, LogicalYes) )
			(void) strcpy(tbuf, LogicalYes);
		else if ( same_ic(action, LogicalNo) )
			(void) strcpy(tbuf, LogicalNo);
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalYes);
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalNo);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalNo);
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for @presentation keyword "justification"            */
	/*  or @wind_presentation/@vector_presentation keywords       */
	/*      "calm_justification" or "direction_justification" or  */
	/*      "speed_justification" or "gust_justification"         */
	/*  or @label/@sample_field keyword "attribute_justification" */
	/*  or @draw_distance_scale keyword "scale_justification"     */
	/*  or @distance_scale_ticks keyword "tick_justification"     */
	/*  or @distance_scale_labels keyword "label_justification"   */
	else if ( same(key, "justification")
			|| same(key, "calm_justification")
			|| same(key, "direction_justification")
			|| same(key, "speed_justification")
			|| same(key, "gust_justification")
			|| same(key, "attribute_justification")
			|| same(key, "scale_justification")
			|| same(key, "tick_justification")
			|| same(key, "label_justification")
			)
		{
		if ( same_ic(action, JustifyLeft) )
											(void) strcpy(tbuf, JustifyLeft);
		else if ( same_ic(action, JustifyCentre) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyCenter) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyRight) )
											(void) strcpy(tbuf, JustifyRight);
		else
			{
			(void) sprintf(err_buf, "Recognized justifications are: %s %s %s",
					JustifyLeft, JustifyCentre, JustifyRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @label/@plot/@sample_field keyword "attribute_vertical_just" */
	else if ( same(key, "attribute_vertical_just") )
		{
		if ( same_ic(action, VerticalBottom) )
										(void) strcpy(tbuf, VerticalBottom);
		else if ( same_ic(action, VerticalCentre) )
										(void) strcpy(tbuf, VerticalCentre);
		else if ( same_ic(action, VerticalCenter) )
										(void) strcpy(tbuf, VerticalCentre);
		else if ( same_ic(action, VerticalTop) )
										(void) strcpy(tbuf, VerticalTop);
		else
			{
			(void) sprintf(err_buf,
					"Recognized vertical justifications are: %s %s %s",
					VerticalBottom, VerticalCentre, VerticalTop);
			(void) error_report(err_buf);
			}
		}

	/* Check for @anchor keyword "ref"                         */
	/*  and @label/@plot/@sample_field keyword "attribute_ref" */
	else if ( same(key, "ref"))
		{
		if ( same_ic(action, AnchorMap) )
			{
			(void) strcpy(tbuf, AnchorMap);
			}
		else if ( same_ic(action, AnchorMapLatLon) )
			{
			(void) strcpy(tbuf, AnchorMapLatLon);
			}
		else if ( same_ic(action, AnchorAbsolute) )
			{
			(void) strcpy(tbuf, AnchorAbsolute);
			}
		else if ( same_ic(action, AnchorCurrent) )
			{
			(void) strcpy(tbuf, AnchorCurrent);
			}
		else if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}
		else if ( same_ic(action, AnchorXsectLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerLeft);
			}
		else if ( same_ic(action, AnchorXsectCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreLeft);
			}
		else if ( same_ic(action, AnchorXsectCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreLeft);
			}
		else if ( same_ic(action, AnchorXsectUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperLeft);
			}
		else if ( same_ic(action, AnchorXsectLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerCentre);
			}
		else if ( same_ic(action, AnchorXsectLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerCentre);
			}
		else if ( same_ic(action, AnchorXsectCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectCentre);
			}
		else if ( same_ic(action, AnchorXsectCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectCentre);
			}
		else if ( same_ic(action, AnchorXsectUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperCentre);
			}
		else if ( same_ic(action, AnchorXsectUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperCentre);
			}
		else if ( same_ic(action, AnchorXsectLowerRight) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerRight);
			}
		else if ( same_ic(action, AnchorXsectCentreRight) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreRight);
			}
		else if ( same_ic(action, AnchorXsectCenterRight) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreRight);
			}
		else if ( same_ic(action, AnchorXsectUpperRight) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperRight);
			}

		/* Error message for incorrect keyword "ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized anchor refs are: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
					AnchorMap, AnchorMapLatLon, AnchorAbsolute, AnchorCurrent,
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight,
					AnchorXsectLowerLeft, AnchorXsectCentreLeft,
					AnchorXsectUpperLeft, AnchorXsectLowerCentre,
					AnchorXsectCentre, AnchorXsectUpperCentre,
					AnchorXsectLowerRight, AnchorXsectCentreRight,
					AnchorXsectUpperRight);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "attribute_ref"))
		{
		if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}

		/* Error message for incorrect keyword "attribute_ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized attribute refs are: %s %s %s %s %s %s %s %s %s",
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @arrow_display keyword "arrow_features" */
	else if ( same(key, "arrow_features") )
		{
		if ( same_ic(action, ArrowHead) )
										(void) strcpy(tbuf, ArrowHead);
		else if ( same_ic(action, ArrowTail) )
										(void) strcpy(tbuf, ArrowTail);
		else if ( same_ic(action, ArrowBoth) )
										(void) strcpy(tbuf, ArrowBoth);
		else if ( same_ic(action, ArrowBothRev) )
										(void) strcpy(tbuf, ArrowBothRev);
		else
			{
			(void) sprintf(err_buf,
					"Recognized arrow features are: %s %s %s %s",
					ArrowHead, ArrowTail, ArrowBoth, ArrowBothRev);
			(void) error_report(err_buf);
			}
		}

	/* Check for @wind_presentation/@vector_presentation keywords "..._type" */
	else if ( same(key, "calm_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized calm types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "direction_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubUniform) )
										(void) strcpy(tbuf, WVsubUniform);
		else if ( same_ic(action, WVsubProportional) )
										(void) strcpy(tbuf, WVsubProportional);
		else
			{
			(void) sprintf(err_buf,
					"Recognized direction types are: %s %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubUniform,
					WVsubProportional);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "speed_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized speed types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "gust_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized gust types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}

	/* Check for "category_cascade" keyword for directives                  */
	/*  @loop_begin/@areas/@cross_section_areas/@cross_section_curves       */
	/*  @geography/@label/@lchain_nodes/@lchain_tracks/@lines/@sample_field */
	else if ( same(key, "category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check for "track_category_cascade" keyword for directive @lchain_nodes */
	else if ( same(key, "track_category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "track_category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized track_category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check for @cross_section_areas/curves keyword "display_function" */
	else if ( same(key, "display_function"))
		{
		if ( same_ic(action, XSectLineLinear) )
									(void) strcpy(tbuf, XSectLineLinear);
		else if ( same_ic(action, XSectLineStepBefore) )
									(void) strcpy(tbuf, XSectLineStepBefore);
		else if ( same_ic(action, XSectLineStepCentre) )
									(void) strcpy(tbuf, XSectLineStepCentre);
		else if ( same_ic(action, XSectLineStepCenter) )
									(void) strcpy(tbuf, XSectLineStepCentre);
		else if ( same_ic(action, XSectLineStepAfter) )
									(void) strcpy(tbuf, XSectLineStepAfter);
		else if ( same_ic(action, XSectLineBox) )
									(void) strcpy(tbuf, XSectLineBox);
		else
			{
			(void) sprintf(err_buf,
					"Recognized display functions are: %s %s %s %s %s",
					XSectLineLinear, XSectLineStepBefore, XSectLineStepCentre,
					XSectLineStepAfter, XSectLineBox);
			(void) error_report(err_buf);
			}
		}

	/* Check all keywords expressed in terms of display units */
	else if ( same(key, "size")
			|| same(key, "width")
			|| same(key, "height")
			|| same(key, "diameter")
			|| same(key, "radius")
			|| same(key, "box_width")
			|| same(key, "text_size")
			|| same(key, "x")
			|| same(key, "y")
			|| same(key, "extra_x")
			|| same(key, "extra_y")
			|| same(key, "x_off")
			|| same(key, "y_off")
			|| same(key, "x_box_off")
			|| same(key, "y_box_off")
			|| same(key, "x_display_off")
			|| same(key, "y_display_off")
			|| same(key, "y_tilt_off")
			|| same(key, "x_eye")
			|| same(key, "y_eye")
			|| same(key, "z_eye")
			|| same(key, "x_label")
			|| same(key, "y_label")
			|| same(key, "x_val")
			|| same(key, "y_val")
			|| same(key, "calm_size")
			|| same(key, "x_calm")
			|| same(key, "y_calm")
			|| same(key, "direction_size")
			|| same(key, "x_dir")
			|| same(key, "y_dir")
			|| same(key, "speed_size")
			|| same(key, "x_spd")
			|| same(key, "y_spd")
			|| same(key, "gust_size")
			|| same(key, "x_gust")
			|| same(key, "y_gust")
			|| same(key, "shaft_length")
			|| same(key, "arrow_length")
			|| same(key, "length_offset")
			|| same(key, "width_offset")
			|| same(key, "attribute_text_size")
			|| same(key, "attribute_x_off")
			|| same(key, "attribute_y_off")
			|| same(key, "margin_left")
			|| same(key, "margin_right")
			|| same(key, "margin_top")
			|| same(key, "margin_bottom")
			|| same(key, "margin_width")
			|| same(key, "margin_height")
			|| same(key, "x_repeat")
			|| same(key, "y_repeat")
			|| same(key, "x_shift")
			|| same(key, "y_shift")
			|| same(key, "x_stationary")
			|| same(key, "y_stationary")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestPSmetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void ) sprintf(tbuf, "%f", value);
		}

	/* Check for @define_line keyword "line" ... a list of locations */
	/* Note that scaling line locations is done in set_define_line() */
	else if ( same(key, "line") )
		{
		(void) strcpy(xbuf, action);
		status = TRUE;
		even   = TRUE;
		while ( status )
			{
			value = float_arg(xbuf, &status);

			/* End of line reached on an even number of points */
			if ( !status && even )
				break;
			else if ( !status )
				{
				(void) sprintf(err_buf, "Odd number of points in %s = %s",
						key, action);
				(void) error_report(err_buf);
				}
			else
				even = !even;

			value *= DisplayUnits.conversion;
			if ( fabs((double) value) > LargestPSmetDistance )
				{
				(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
						DisplayUnits.type, key, action);
				(void) warn_report(err_buf);
				}
			}
		(void) strcpy(tbuf, action);
		}

	/* Check all keywords using symbol scaling */
	else if ( same(key, "scale")
			|| same(key, "symbol_scale")
			|| same(key, "mark_scale")
			|| same(key, "attribute_symbol_scale")
			|| same(key, "calm_scale")
			|| same(key, "huge_scale")
			|| same(key, "direction_scale")
			|| same(key, "speed_scale")
			|| same(key, "gust_scale")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.sfactor;
		(void ) sprintf(tbuf, "%f", value);
		}

	/* Check for logical keywords */
	else if ( same(key, "outline_first")
			|| same(key, "attribute_outline_first")
			|| same(key, "fit_to_map")
			|| same(key, "scale_to_perspective")
			|| same(key, "display_as_areas")
			|| same(key, "display_at_feature")
			|| same(key, "pattern_for_holes")
			|| same(key, "closed")
			|| same(key, "rotate_to_latitude")
			|| same(key, "rotate_to_longitude")
			|| same(key, "constrain_rotation")
			|| same(key, "blend_images")
			|| same(key, "radar_range_rings")
			|| same(key, "radar_limit_ring")
			)
		{
		if ( same_ic(action, LogicalYes) )
			(void) strcpy(tbuf, LogicalYes);
		else if ( same_ic(action, LogicalNo) )
			(void) strcpy(tbuf, LogicalNo);
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalYes);
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalNo);
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for common @(...)presentation keywords */
	else if ( same(key, "pattern")
			|| same(key, "rotation")
			|| same(key, "char_space")
			|| same(key, "word_space")
			|| same(key, "line_space")
			|| same(key, "start_angle")
			|| same(key, "end_angle")
			|| same(key, "barb_length")
			|| same(key, "barb_width")
			|| same(key, "barb_space")
			|| same(key, "barb_angle")
			|| same(key, "speed_round")
			|| same(key, "gust_above")
			|| same(key, "gust_round")
			|| same(key, "gust_distance")
			|| same(key, "gust_angle")
			|| same(key, "calm_max")
			|| same(key, "calm_symbol")
			|| same(key, "huge_min")
			|| same(key, "huge_symbol")
			|| same(key, "calm_format")
			|| same(key, "direction_format")
			|| same(key, "speed_format")
			|| same(key, "gust_format")
			|| same(key, "wind_look_up")
			|| same(key, "vector_look_up")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @(...)display keywords */
	else if ( same(key, "arrow_name")
			|| same(key, "arrow_angle")
			|| same(key, "return_angle")
			|| same(key, "head_length")
			|| same(key, "tail_length")
			|| same(key, "display_name")
			|| same(key, "display_type")
			|| same(key, "width_scale")
			|| same(key, "height_scale")
			|| same(key, "width_attribute")
			|| same(key, "height_attribute")
			|| same(key, "diameter_attribute")
			|| same(key, "radius_attribute")
			|| same(key, "symbol_fill_name")
			|| same(key, "symbol_rotation")
			|| same(key, "scale_name")
			|| same(key, "scale_length")
			|| same(key, "scale_units")
			|| same(key, "scale_rotation")
			|| same(key, "tick_location")
			|| same(key, "tick_length")
			|| same(key, "tick_units")
			|| same(key, "tick_rotation")
			|| same(key, "label_location")
			|| same(key, "label_string")
			|| same(key, "label_units")
			|| same(key, "label_rotation")
			|| same(key, "rotation_attribute")
			/* >>> "repeat_rotation" not used yet! <<< */
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @projection @mapdef @resolution keywords */
	else if ( same(key, "type")
			|| same(key, "ref1")
			|| same(key, "ref2")
			|| same(key, "ref3")
			|| same(key, "ref4")
			|| same(key, "ref5")
			|| same(key, "olat")
			|| same(key, "olon")
			|| same(key, "rlon")
			|| same(key, "xmin")
			|| same(key, "ymin")
			|| same(key, "xmax")
			|| same(key, "ymax")
			|| same(key, "map_units")
			|| same(key, "res")
			|| same(key, "map_x")
			|| same(key, "map_y")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for other common PSMet keywords */
	else if ( same(key, "dir")
			|| same(key, "name")
			|| same(key, "geo_name")
			|| same(key, "geo_file")
			|| same(key, "file")			/* >>> Version 6 obsolete <<< */
			|| same(key, "scale_factor")
			|| same(key, "number_of_iterations")
			|| same(key, "keyword_name")
			|| same(key, "keyword_value")
			|| same(key, "keyword_value_list")
			|| same(key, "group_name")
			|| same(key, "line_name")
			|| same(key, "table_name")
			|| same(key, "site_label")
			|| same(key, "last_site")
			|| same(key, "grid_name")
			|| same(key, "list_name")
			|| same(key, "look_up")
			|| same(key, "case")
			|| same(key, "case_look_up")
			|| same(key, "symbol")
			|| same(key, "mark")
			|| same(key, "map_scale")
			|| same(key, "axis_to_scale")
			|| same(key, "format")
			|| same(key, "time")
			|| same(key, "zone_type")
			|| same(key, "time_zone")
			|| same(key, "language")
			|| same(key, "text")
			|| same(key, "text_file")
			|| same(key, "string")
			|| same(key, "data_file")
			|| same(key, "data_file_format")
			|| same(key, "data_file_units")
			|| same(key, "data_file_wind_units")
			|| same(key, "lat")
			|| same(key, "lon")
			|| same(key, "location_ident")
			|| same(key, "location_look_up")
			|| same(key, "location_distances")
			|| same(key, "location_units")
			|| same(key, "location_times")
			|| same(key, "location_fractions")
			|| same(key, "location_interval")
			|| same(key, "start_time")
			|| same(key, "end_time")
			|| same(key, "times")
			|| same(key, "labels")
			|| same(key, "node_speed_units")
			|| same(key, "node_speed_round")
			|| same(key, "node_stationary_max")
			|| same(key, "node_stationary_label")
			|| same(key, "track_length_min")
			|| same(key, "track_length_units")
			|| same(key, "lat_begin")
			|| same(key, "lat_end")
			|| same(key, "lat_interval")
			|| same(key, "lon_begin")
			|| same(key, "lon_end")
			|| same(key, "lon_interval")
			|| same(key, "map_x_begin")
			|| same(key, "map_x_end")
			|| same(key, "map_x_interval")
			|| same(key, "map_y_begin")
			|| same(key, "map_y_end")
			|| same(key, "map_y_interval")
			|| same(key, "lat_lon_ident")
			|| same(key, "map_x_y_ident")
			|| same(key, "location_ident_list")
			|| same(key, "proximity")
			|| same(key, "proximity_units")
			|| same(key, "values")
			|| same(key, "range")
			|| same(key, "base")
			|| same(key, "min")
			|| same(key, "max")
			|| same(key, "interval")
			|| same(key, "area_type")
			|| same(key, "fit_to_map_ref")
			|| same(key, "tilt_angle")
			|| same(key, "show_perspective_view")
			|| same(key, "x_stretch")
			|| same(key, "y_stretch")
			|| same(key, "x_wrap")
			|| same(key, "y_wrap")
			|| same(key, "cross_section_name")
			|| same(key, "axis_for_display")
			|| same(key, "line_to_draw")
			|| same(key, "vertical_look_up")
			|| same(key, "vertical_data_file")
			|| same(key, "vertical_data_file_format")
			|| same(key, "vertical_data_file_units")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common configuration keywords */
	else if ( same(key, "element")
			|| same(key, "element_list")
			|| same(key, "subelement")		/* >>> Version 6 obsolete <<< */
			|| same(key, "subelements")		/* >>> Version 6 obsolete <<< */
			|| same(key, "level")
			|| same(key, "level_list")
			|| same(key, "equation")
			|| same(key, "units")
			|| same(key, "field_type")
			|| same(key, "category_attribute")
			|| same(key, "category")
			|| same(key, "track_category_attribute")
			|| same(key, "track_category")
			|| same(key, "attribute")
			|| same(key, "source")
			|| same(key, "valid_time")
			|| same(key, "wind_crossref")
			|| same(key, "vertical_element")
			|| same(key, "vertical_level")
			|| same(key, "vertical_equation")
			|| same(key, "vertical_units")
			|| same(key, "vertical_attribute")
			|| same(key, "vertical_attribute_upper")
			|| same(key, "vertical_attribute_lower")
			|| same(key, "vertical_field_type")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for "attribute" display keywords */
	else if ( same(key, "attribute_show")
			|| same(key, "attribute_anchor")
			|| same(key, "attribute_units")
			|| same(key, "attribute_format")
			|| same(key, "attribute_look_up")
			|| same(key, "attribute_display_name")
			|| same(key, "attribute_display_type")
			|| same(key, "attribute_width_scale")
			|| same(key, "attribute_height_scale")
			|| same(key, "attribute_char_space")
			|| same(key, "attribute_word_space")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for @images keywords for radar ring colours    */
	/*  "radar_range_ring_colour" "radar_limit_ring_colour" */
	else if ( same(key, "radar_range_ring_colour")
			|| same(key, "radar_limit_ring_colour")
			)
		{

		/* Parameters for no radar ring colour */
		if ( same_ic(action, ColourNone) )
			{
			(void) sprintf(tbuf, ColourNone);
			}

		/* Parameters for specified radar ring colours */
		else
			{

			/* Strip off the type of specification and convert the remainder */
			(void) strcpy(xbuf, string_arg(action));
			if ( same_ic(xbuf, "CMYK") )
				{
				if ( !convert_cmyk_for_radar(action) )
						(void) error_report("Improper CMYK parameters");
				}
			else if ( same_ic(xbuf, "RGB") )
				{
				if ( !convert_rgb_for_radar(action) )
						(void) error_report("Improper RGB parameters");
				}
			else if ( same_ic(xbuf, "X11") )
				{
				if ( !convert_x11_for_radar(action) )
						(void) error_report("Improper X11 Name parameters");
				}
			else
				(void) error_report("Invalid Colour Specification - Must begin with CMYK or RGB or X11");

			/* Copy the converted radar ring colour specification to the buffer */
			(void) sprintf(tbuf, "%s", action);
			}
		}

	/* Check for keywords for imagery */
	else if ( same(key, "image_tag")
			|| same(key, "image_tag_list")
			|| same(key, "colour_table")
			|| same(key, "colour_table_list")
			|| same(key, "brightness")
			|| same(key, "brightness_list")
			|| same(key, "satellite_brightness")
			|| same(key, "radar_brightness")
			|| same(key, "match_time_before")
			|| same(key, "match_time_after")
			|| same(key, "satellite_time_before")
			|| same(key, "satellite_time_after")
			|| same(key, "radar_time_before")
			|| same(key, "radar_time_after")
			|| same(key, "blend_ratio")
			|| same(key, "radar_range_ring_interval")
			|| same(key, "radar_range_ring_units")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Error message for unrecognized keywords! */
	else
		{
		(void) sprintf(err_buf, "Unrecognized keyword ... %s", key);
		(void) error_report(err_buf);
		}

	/* Overwrite the input buffer and return */
	(void) strcpy(action, tbuf);
	return;
	}

void		check_svgmet_keyword

	(
	STRING		key,
	STRING		action
	)

	{
	char		tbuf[GPGMedium], xbuf[GPGMedium];
	float		value;
	LOGICAL		status, even;
	char		err_buf[GPGLong];

	/* Check for @presentation keyword "line_width"            */
	/*  or @label/@sample_field keyword "attribute_line_width" */
	if ( same(key, "line_width")
			|| same(key, "attribute_line_width")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestSVGmetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		if ( value < 0.001 )
			(void) sprintf(tbuf, "0.001");
		else
			(void) sprintf(tbuf, "%.3f", value);
		}

	/* Check for @presentation keyword "line_style"            */
	/*  or @label/@sample_field keyword "attribute_line_style" */
	else if ( same(key, "line_style")
			|| same(key, "attribute_line_style")
			)
		{
		if ( same_ic(action, "solid") )
			(void) sprintf(tbuf, "none");
		else
			{
			/* Strip off the first parameter before copying! */
			(void) string_arg(action);
			(void) sprintf(tbuf, "%s", action);
			}
		}

	/* Check for @presentation keywords "outline" "fill" "interior_fill"      */
	/*  or @label/@sample_field keywords "attribute_outline" "attribute_fill" */
	else if ( same(key, "outline")
			|| same(key, "fill")
			|| same(key, "interior_fill")
			|| same(key, "attribute_outline")
			|| same(key, "attribute_fill")
			)
		{

		/* Parameters for no outline or fill or interior_fill */
		if ( same_ic(action, ColourNone) )
			{
			(void) sprintf(tbuf, ColourNone);
			}

		/* Parameters for specified outline or fill or interior_fill */
		else
			{

			/* Strip off the type of specification and convert the remainder */
			(void) strcpy(xbuf, string_arg(action));
			if ( same_ic(xbuf, "CMYK") )
				{
				if ( !convert_cmyk_for_svgmet(action) )
						(void) error_report("Improper CMYK parameters");
				}
			else if ( same_ic(xbuf, "RGB") )
				{
				if ( !convert_rgb_for_svgmet(action) )
						(void) error_report("Improper RGB parameters");
				}
			else if ( same_ic(xbuf, "X11") )
				{
				if ( !convert_x11_for_svgmet(action) )
						(void) error_report("Improper X11 Name parameters");
				}
			else
				(void) error_report("Invalid Colour Specification - Must begin with CMYK or RGB or X11");

			/* Copy the converted RGB colour specification to the buffer */
			(void) sprintf(tbuf, "%s", action);
			}
		}

	/* Check for @presentation keywords "pattern_width" "pattern_length" */
	else if ( same(key, "pattern_width")
			|| same(key, "pattern_length") )
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestSVGmetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void) sprintf(tbuf, "%f", value);
		}

	/* Check for @presentation keyword "font"            */
	/*  or @label/@sample_field keyword "attribute_font" */
	else if ( same(key, "font")
			|| same(key, "attribute_font")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for @presentation keyword "font_weight"            */
	/*  or @label/@sample_field keyword "attribute_font_weight" */
	/* Note that these are presently ignored by SVGMet!          */
	else if ( same(key, "font_weight")
			|| same(key, "attribute_font_weight")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for logical @presentation keyword "italics"    */
	/*  or @label/@sample_field keyword "attribute_italics" */
	/* Note that these are presently ignored by SVGMet!     */
	else if ( same(key, "italics")
			|| same(key, "attribute_italics")
			)
		{
		if ( same_ic(action, LogicalYes) )
			(void) strcpy(tbuf, LogicalYes);
		else if ( same_ic(action, LogicalNo) )
			(void) strcpy(tbuf, LogicalNo);
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalYes);
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalNo);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalNo);
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for @presentation keyword "justification"            */
	/*  or @wind_presentation/@vector_presentation keywords       */
	/*      "calm_justification" or "direction_justification" or  */
	/*      "speed_justification" or "gust_justification"         */
	/*  or @label/@sample_field keyword "attribute_justification" */
	/*  or @draw_distance_scale keyword "scale_justification"     */
	/*  or @distance_scale_ticks keyword "tick_justification"     */
	/*  or @distance_scale_labels keyword "label_justification"   */
	else if ( same(key, "justification")
			|| same(key, "calm_justification")
			|| same(key, "direction_justification")
			|| same(key, "speed_justification")
			|| same(key, "gust_justification")
			|| same(key, "attribute_justification")
			|| same(key, "scale_justification")
			|| same(key, "tick_justification")
			|| same(key, "label_justification")
			)
		{
		if ( same_ic(action, JustifyLeft) )
											(void) strcpy(tbuf, JustifyLeft);
		else if ( same_ic(action, JustifyCentre) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyCenter) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyRight) )
											(void) strcpy(tbuf, JustifyRight);
		else
			{
			(void) sprintf(err_buf, "Recognized justifications are: %s %s %s",
					JustifyLeft, JustifyCentre, JustifyRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @label/@plot/@sample_field keyword "attribute_vertical_just" */
	else if ( same(key, "attribute_vertical_just") )
		{
		if ( same_ic(action, VerticalBottom) )
										(void) strcpy(tbuf, VerticalBottom);
		else if ( same_ic(action, VerticalCentre) )
										(void) strcpy(tbuf, VerticalCentre);
		else if ( same_ic(action, VerticalCenter) )
										(void) strcpy(tbuf, VerticalCentre);
		else if ( same_ic(action, VerticalTop) )
										(void) strcpy(tbuf, VerticalTop);
		else
			{
			(void) sprintf(err_buf,
					"Recognized vertical justifications are: %s %s %s",
					VerticalBottom, VerticalCentre, VerticalTop);
			(void) error_report(err_buf);
			}
		}

	/* Check for @anchor keyword "ref"                         */
	/*  and @label/@plot/@sample_field keyword "attribute_ref" */
	else if ( same(key, "ref"))
		{
		if ( same_ic(action, AnchorMap) )
			{
			(void) strcpy(tbuf, AnchorMap);
			}
		else if ( same_ic(action, AnchorMapLatLon) )
			{
			(void) strcpy(tbuf, AnchorMapLatLon);
			}
		else if ( same_ic(action, AnchorAbsolute) )
			{
			(void) strcpy(tbuf, AnchorAbsolute);
			}
		else if ( same_ic(action, AnchorCurrent) )
			{
			(void) strcpy(tbuf, AnchorCurrent);
			}
		else if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}
		else if ( same_ic(action, AnchorXsectLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerLeft);
			}
		else if ( same_ic(action, AnchorXsectCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreLeft);
			}
		else if ( same_ic(action, AnchorXsectCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreLeft);
			}
		else if ( same_ic(action, AnchorXsectUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperLeft);
			}
		else if ( same_ic(action, AnchorXsectLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerCentre);
			}
		else if ( same_ic(action, AnchorXsectLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerCentre);
			}
		else if ( same_ic(action, AnchorXsectCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectCentre);
			}
		else if ( same_ic(action, AnchorXsectCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectCentre);
			}
		else if ( same_ic(action, AnchorXsectUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperCentre);
			}
		else if ( same_ic(action, AnchorXsectUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperCentre);
			}
		else if ( same_ic(action, AnchorXsectLowerRight) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerRight);
			}
		else if ( same_ic(action, AnchorXsectCentreRight) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreRight);
			}
		else if ( same_ic(action, AnchorXsectCenterRight) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreRight);
			}
		else if ( same_ic(action, AnchorXsectUpperRight) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperRight);
			}

		/* Error message for incorrect keyword "ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized anchor refs are: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
					AnchorMap, AnchorMapLatLon, AnchorAbsolute, AnchorCurrent,
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight,
					AnchorXsectLowerLeft, AnchorXsectCentreLeft,
					AnchorXsectUpperLeft, AnchorXsectLowerCentre,
					AnchorXsectCentre, AnchorXsectUpperCentre,
					AnchorXsectLowerRight, AnchorXsectCentreRight,
					AnchorXsectUpperRight);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "attribute_ref"))
		{
		if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}

		/* Error message for incorrect keyword "attribute_ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized attribute refs are: %s %s %s %s %s %s %s %s %s",
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @arrow_display keyword "arrow_features" */
	else if ( same(key, "arrow_features") )
		{
		if ( same_ic(action, ArrowHead) )
										(void) strcpy(tbuf, ArrowHead);
		else if ( same_ic(action, ArrowTail) )
										(void) strcpy(tbuf, ArrowTail);
		else if ( same_ic(action, ArrowBoth) )
										(void) strcpy(tbuf, ArrowBoth);
		else if ( same_ic(action, ArrowBothRev) )
										(void) strcpy(tbuf, ArrowBothRev);
		else
			{
			(void) sprintf(err_buf,
					"Recognized arrow features are: %s %s %s %s",
					ArrowHead, ArrowTail, ArrowBoth, ArrowBothRev);
			(void) error_report(err_buf);
			}
		}

	/* Check for @wind_presentation/@vector_presentation keywords "..._type" */
	else if ( same(key, "calm_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized calm types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "direction_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubUniform) )
										(void) strcpy(tbuf, WVsubUniform);
		else if ( same_ic(action, WVsubProportional) )
										(void) strcpy(tbuf, WVsubProportional);
		else
			{
			(void) sprintf(err_buf,
					"Recognized direction types are: %s %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubUniform,
					WVsubProportional);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "speed_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized speed types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "gust_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized gust types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}

	/* Check for "category_cascade" keyword for directives                 */
	/*  @loop_begin/@areas/@cross_section_areas/@cross_section_curves      */
	/*  @geography/@label/@lchain_nodes/lchain_tracks/@lines/@sample_field */
	else if ( same(key, "category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check for "track_category_cascade" keyword for directive @lchain_nodes */
	else if ( same(key, "track_category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "track_category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized track_category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check for @cross_section_areas/curves keyword "display_function" */
	else if ( same(key, "display_function"))
		{
		if ( same_ic(action, XSectLineLinear) )
									(void) strcpy(tbuf, XSectLineLinear);
		else if ( same_ic(action, XSectLineStepBefore) )
									(void) strcpy(tbuf, XSectLineStepBefore);
		else if ( same_ic(action, XSectLineStepCentre) )
									(void) strcpy(tbuf, XSectLineStepCentre);
		else if ( same_ic(action, XSectLineStepCenter) )
									(void) strcpy(tbuf, XSectLineStepCentre);
		else if ( same_ic(action, XSectLineStepAfter) )
									(void) strcpy(tbuf, XSectLineStepAfter);
		else if ( same_ic(action, XSectLineBox) )
									(void) strcpy(tbuf, XSectLineBox);
		else
			{
			(void) sprintf(err_buf,
					"Recognized display functions are: %s %s %s %s %s",
					XSectLineLinear, XSectLineStepBefore, XSectLineStepCentre,
					XSectLineStepAfter, XSectLineBox);
			(void) error_report(err_buf);
			}
		}

	/* Check all keywords expressed in terms of display units */
	else if ( same(key, "size")
			|| same(key, "width")
			|| same(key, "height")
			|| same(key, "diameter")
			|| same(key, "radius")
			|| same(key, "box_width")
			|| same(key, "text_size")
			|| same(key, "x")
			|| same(key, "y")
			|| same(key, "extra_x")
			|| same(key, "extra_y")
			|| same(key, "x_off")
			|| same(key, "y_off")
			|| same(key, "x_box_off")
			|| same(key, "y_box_off")
			|| same(key, "x_display_off")
			|| same(key, "y_display_off")
			|| same(key, "y_tilt_off")
			|| same(key, "x_eye")
			|| same(key, "y_eye")
			|| same(key, "z_eye")
			|| same(key, "x_label")
			|| same(key, "y_label")
			|| same(key, "x_val")
			|| same(key, "y_val")
			|| same(key, "calm_size")
			|| same(key, "x_calm")
			|| same(key, "y_calm")
			|| same(key, "direction_size")
			|| same(key, "x_dir")
			|| same(key, "y_dir")
			|| same(key, "speed_size")
			|| same(key, "x_spd")
			|| same(key, "y_spd")
			|| same(key, "gust_size")
			|| same(key, "x_gust")
			|| same(key, "y_gust")
			|| same(key, "shaft_length")
			|| same(key, "arrow_length")
			|| same(key, "length_offset")
			|| same(key, "width_offset")
			|| same(key, "attribute_text_size")
			|| same(key, "attribute_x_off")
			|| same(key, "attribute_y_off")
			|| same(key, "margin_left")
			|| same(key, "margin_right")
			|| same(key, "margin_top")
			|| same(key, "margin_bottom")
			|| same(key, "margin_width")
			|| same(key, "margin_height")
			|| same(key, "x_repeat")
			|| same(key, "y_repeat")
			|| same(key, "x_shift")
			|| same(key, "y_shift")
			|| same(key, "x_stationary")
			|| same(key, "y_stationary")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestSVGmetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void ) sprintf(tbuf, "%f", value);
		}

	/* Check for @define_line keyword "line" ... a list of locations */
	/* Note that scaling line locations is done in set_define_line() */
	else if ( same(key, "line") )
		{
		(void) strcpy(xbuf, action);
		status = TRUE;
		even   = TRUE;
		while ( status )
			{
			value = float_arg(xbuf, &status);

			/* End of line reached on an even number of points */
			if ( !status && even )
				break;
			else if ( !status )
				{
				(void) sprintf(err_buf, "Odd number of points in %s = %s",
						key, action);
				(void) error_report(err_buf);
				}
			else
				even = !even;

			value *= DisplayUnits.conversion;
			if ( fabs((double) value) > LargestSVGmetDistance )
				{
				(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
						DisplayUnits.type, key, action);
				(void) warn_report(err_buf);
				}
			}
		(void) strcpy(tbuf, action);
		}

	/* Check all keywords using symbol scaling */
	else if ( same(key, "scale")
			|| same(key, "symbol_scale")
			|| same(key, "mark_scale")
			|| same(key, "attribute_symbol_scale")
			|| same(key, "calm_scale")
			|| same(key, "huge_scale")
			|| same(key, "direction_scale")
			|| same(key, "speed_scale")
			|| same(key, "gust_scale")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.sfactor;
		(void ) sprintf(tbuf, "%f", value);
		}

	/* Check for logical keywords */
	else if ( same(key, "outline_first")
			|| same(key, "attribute_outline_first")
			|| same(key, "fit_to_map")
			|| same(key, "scale_to_perspective")
			|| same(key, "display_as_areas")
			|| same(key, "display_at_feature")
			|| same(key, "pattern_for_holes")
			|| same(key, "closed")
			|| same(key, "rotate_to_latitude")
			|| same(key, "rotate_to_longitude")
			|| same(key, "constrain_rotation")
			|| same(key, "blend_images")
			|| same(key, "radar_range_rings")
			|| same(key, "radar_limit_ring")
			)
		{
		if ( same_ic(action, LogicalYes) )
			(void) strcpy(tbuf, LogicalYes);
		else if ( same_ic(action, LogicalNo) )
			(void) strcpy(tbuf, LogicalNo);
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalYes);
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalNo);
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for common @(...)presentation keywords */
	else if ( same(key, "pattern")
			|| same(key, "rotation")
			|| same(key, "char_space")
			|| same(key, "word_space")
			|| same(key, "line_space")
			|| same(key, "start_angle")
			|| same(key, "end_angle")
			|| same(key, "barb_length")
			|| same(key, "barb_width")
			|| same(key, "barb_space")
			|| same(key, "barb_angle")
			|| same(key, "speed_round")
			|| same(key, "gust_above")
			|| same(key, "gust_round")
			|| same(key, "gust_distance")
			|| same(key, "gust_angle")
			|| same(key, "calm_max")
			|| same(key, "calm_symbol")
			|| same(key, "huge_min")
			|| same(key, "huge_symbol")
			|| same(key, "calm_format")
			|| same(key, "direction_format")
			|| same(key, "speed_format")
			|| same(key, "gust_format")
			|| same(key, "wind_look_up")
			|| same(key, "vector_look_up")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @(...)display keywords */
	else if ( same(key, "arrow_name")
			|| same(key, "arrow_angle")
			|| same(key, "return_angle")
			|| same(key, "head_length")
			|| same(key, "tail_length")
			|| same(key, "display_name")
			|| same(key, "display_type")
			|| same(key, "width_scale")
			|| same(key, "height_scale")
			|| same(key, "width_attribute")
			|| same(key, "height_attribute")
			|| same(key, "diameter_attribute")
			|| same(key, "radius_attribute")
			|| same(key, "symbol_fill_name")
			|| same(key, "symbol_rotation")
			|| same(key, "scale_name")
			|| same(key, "scale_length")
			|| same(key, "scale_units")
			|| same(key, "scale_rotation")
			|| same(key, "tick_location")
			|| same(key, "tick_length")
			|| same(key, "tick_units")
			|| same(key, "tick_rotation")
			|| same(key, "label_location")
			|| same(key, "label_string")
			|| same(key, "label_units")
			|| same(key, "label_rotation")
			|| same(key, "rotation_attribute")
			/* >>> "repeat_rotation" not used yet! <<< */
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @projection @mapdef @resolution keywords */
	else if ( same(key, "type")
			|| same(key, "ref1")
			|| same(key, "ref2")
			|| same(key, "ref3")
			|| same(key, "ref4")
			|| same(key, "ref5")
			|| same(key, "olat")
			|| same(key, "olon")
			|| same(key, "rlon")
			|| same(key, "xmin")
			|| same(key, "ymin")
			|| same(key, "xmax")
			|| same(key, "ymax")
			|| same(key, "map_units")
			|| same(key, "res")
			|| same(key, "map_x")
			|| same(key, "map_y")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for other common SVGMet keywords */
	else if ( same(key, "dir")
			|| same(key, "name")
			|| same(key, "geo_name")
			|| same(key, "geo_file")
			|| same(key, "scale_factor")
			|| same(key, "number_of_iterations")
			|| same(key, "keyword_name")
			|| same(key, "keyword_value")
			|| same(key, "keyword_value_list")
			|| same(key, "group_name")
			|| same(key, "line_name")
			|| same(key, "table_name")
			|| same(key, "site_label")
			|| same(key, "last_site")
			|| same(key, "grid_name")
			|| same(key, "list_name")
			|| same(key, "look_up")
			|| same(key, "case")
			|| same(key, "case_look_up")
			|| same(key, "symbol")
			|| same(key, "mark")
			|| same(key, "map_scale")
			|| same(key, "axis_to_scale")
			|| same(key, "format")
			|| same(key, "time")
			|| same(key, "zone_type")
			|| same(key, "time_zone")
			|| same(key, "language")
			|| same(key, "text")
			|| same(key, "text_file")
			|| same(key, "string")
			|| same(key, "data_file")
			|| same(key, "data_file_format")
			|| same(key, "data_file_units")
			|| same(key, "data_file_wind_units")
			|| same(key, "lat")
			|| same(key, "lon")
			|| same(key, "location_ident")
			|| same(key, "location_look_up")
			|| same(key, "location_distances")
			|| same(key, "location_units")
			|| same(key, "location_times")
			|| same(key, "location_fractions")
			|| same(key, "location_interval")
			|| same(key, "start_time")
			|| same(key, "end_time")
			|| same(key, "times")
			|| same(key, "labels")
			|| same(key, "node_speed_units")
			|| same(key, "node_speed_round")
			|| same(key, "node_stationary_max")
			|| same(key, "node_stationary_label")
			|| same(key, "track_length_min")
			|| same(key, "track_length_units")
			|| same(key, "lat_begin")
			|| same(key, "lat_end")
			|| same(key, "lat_interval")
			|| same(key, "lon_begin")
			|| same(key, "lon_end")
			|| same(key, "lon_interval")
			|| same(key, "map_x_begin")
			|| same(key, "map_x_end")
			|| same(key, "map_x_interval")
			|| same(key, "map_y_begin")
			|| same(key, "map_y_end")
			|| same(key, "map_y_interval")
			|| same(key, "lat_lon_ident")
			|| same(key, "map_x_y_ident")
			|| same(key, "location_ident_list")
			|| same(key, "proximity")
			|| same(key, "proximity_units")
			|| same(key, "values")
			|| same(key, "range")
			|| same(key, "base")
			|| same(key, "min")
			|| same(key, "max")
			|| same(key, "interval")
			|| same(key, "area_type")
			|| same(key, "fit_to_map_ref")
			|| same(key, "tilt_angle")
			|| same(key, "show_perspective_view")
			|| same(key, "x_stretch")
			|| same(key, "y_stretch")
			|| same(key, "x_wrap")
			|| same(key, "y_wrap")
			|| same(key, "cross_section_name")
			|| same(key, "axis_for_display")
			|| same(key, "line_to_draw")
			|| same(key, "vertical_look_up")
			|| same(key, "vertical_data_file")
			|| same(key, "vertical_data_file_format")
			|| same(key, "vertical_data_file_units")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common configuration keywords */
	else if ( same(key, "element")
			|| same(key, "element_list")
			|| same(key, "level")
			|| same(key, "level_list")
			|| same(key, "equation")
			|| same(key, "units")
			|| same(key, "field_type")
			|| same(key, "category_attribute")
			|| same(key, "category")
			|| same(key, "track_category_attribute")
			|| same(key, "track_category")
			|| same(key, "attribute")
			|| same(key, "source")
			|| same(key, "valid_time")
			|| same(key, "wind_crossref")
			|| same(key, "vertical_element")
			|| same(key, "vertical_level")
			|| same(key, "vertical_equation")
			|| same(key, "vertical_units")
			|| same(key, "vertical_attribute")
			|| same(key, "vertical_attribute_upper")
			|| same(key, "vertical_attribute_lower")
			|| same(key, "vertical_field_type")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for "attribute" display keywords */
	else if ( same(key, "attribute_show")
			|| same(key, "attribute_anchor")
			|| same(key, "attribute_units")
			|| same(key, "attribute_format")
			|| same(key, "attribute_look_up")
			|| same(key, "attribute_display_name")
			|| same(key, "attribute_display_type")
			|| same(key, "attribute_width_scale")
			|| same(key, "attribute_height_scale")
			|| same(key, "attribute_char_space")
			|| same(key, "attribute_word_space")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for @images keywords for radar ring colours    */
	/*  "radar_range_ring_colour" "radar_limit_ring_colour" */
	else if ( same(key, "radar_range_ring_colour")
			|| same(key, "radar_limit_ring_colour")
			)
		{

		/* Parameters for no radar ring colour */
		if ( same_ic(action, ColourNone) )
			{
			(void) sprintf(tbuf, ColourNone);
			}

		/* Parameters for specified radar ring colours */
		else
			{

			/* Strip off the type of specification and convert the remainder */
			(void) strcpy(xbuf, string_arg(action));
			if ( same_ic(xbuf, "CMYK") )
				{
				if ( !convert_cmyk_for_radar(action) )
						(void) error_report("Improper CMYK parameters");
				}
			else if ( same_ic(xbuf, "RGB") )
				{
				if ( !convert_rgb_for_radar(action) )
						(void) error_report("Improper RGB parameters");
				}
			else if ( same_ic(xbuf, "X11") )
				{
				if ( !convert_x11_for_radar(action) )
						(void) error_report("Improper X11 Name parameters");
				}
			else
				(void) error_report("Invalid Colour Specification - Must begin with CMYK or RGB or X11");

			/* Copy the converted radar ring colour specification to the buffer */
			(void) sprintf(tbuf, "%s", action);
			}
		}

	/* Check for keywords for imagery */
	else if ( same(key, "image_tag")
			|| same(key, "image_tag_list")
			|| same(key, "colour_table")
			|| same(key, "colour_table_list")
			|| same(key, "brightness")
			|| same(key, "brightness_list")
			|| same(key, "satellite_brightness")
			|| same(key, "radar_brightness")
			|| same(key, "match_time_before")
			|| same(key, "match_time_after")
			|| same(key, "satellite_time_before")
			|| same(key, "satellite_time_after")
			|| same(key, "radar_time_before")
			|| same(key, "radar_time_after")
			|| same(key, "blend_ratio")
			|| same(key, "radar_range_ring_interval")
			|| same(key, "radar_range_ring_units")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Error message for unrecognized keywords! */
	else
		{
		(void) sprintf(err_buf, "Unrecognized keyword ... %s", key);
		(void) error_report(err_buf);
		}

	/* Overwrite the input buffer and return */
	(void) strcpy(action, tbuf);
	return;
	}

void		check_cormet_keyword

	(
	STRING		key,
	STRING		action
	)

	{
	char		tbuf[GPGMedium], xbuf[GPGMedium];
	float		value;
	LOGICAL		status, even;
	char		err_buf[GPGLong];

	/* Check for @presentation keyword "line_width"            */
	/*  or @label/@sample_field keyword "attribute_line_width" */
	if ( same(key, "line_width")
			|| same(key, "attribute_line_width")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestCormetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void) sprintf(tbuf, "@wd %d", NINT(value));
		}

	/* Check for @presentation keyword "line_style"            */
	/*  or @label/@sample_field keyword "attribute_line_style" */
	else if ( same(key, "line_style")
			|| same(key, "attribute_line_style")
			)
		{
		if ( same_ic(action, "solid") )
			(void) sprintf(tbuf, "@dt 0 0");
		else
			(void) sprintf(tbuf, "@dt %s", action);
		}

	/* Check for @presentation keywords "outline" "fill" "interior_fill"      */
	/*  or @label/@sample_field keywords "attribute_outline" "attribute_fill" */
	else if ( same(key, "outline")
			|| same(key, "fill")
			|| same(key, "interior_fill")
			|| same(key, "attribute_outline")
			|| same(key, "attribute_fill")
			)
		{

		/* Parameters for no outline or fill or interior_fill */
		if ( same_ic(action, ColourNone) )
			{
			if ( same(key, "outline")
					|| same(key, "attribute_outline") )
				(void) sprintf(tbuf, "@xO");
			else if ( same(key, "fill")
					|| same(key, "interior_fill")
					|| same(key, "attribute_fill") )
				(void) sprintf(tbuf, "@xF");
			}

		/* Parameters for specified outline or fill or interior_fill */
		else
			{

			/* Initialize the specification */
			if ( same(key, "outline")
					|| same(key, "attribute_outline") )
				(void) sprintf(tbuf, "@uO ");
			else if ( same(key, "fill")
					|| same(key, "interior_fill")
					|| same(key, "attribute_fill") )
				(void) sprintf(tbuf, "@uF ");

			/* Strip off the type of specification and convert the remainder */
			(void) strcpy(xbuf, string_arg(action));
			if ( same_ic(xbuf, "CMYK") )
				{
				if ( !convert_cmyk_for_cormet(action) )
						(void) error_report("Improper CMYK parameters");
				}
			else if ( same_ic(xbuf, "RGB") )
				{
				if ( !convert_rgb_for_cormet(action) )
						(void) error_report("Improper RGB parameters");
				}
			else if ( same_ic(xbuf, "X11") )
				{
				if ( !convert_x11_for_cormet(action) )
						(void) error_report("Improper X11 Name parameters");
				}
			else
				(void) error_report("Invalid Colour Specification - Must begin with CMYK or RGB or X11");

			/* Add the converted CMYK colour specification to the buffer */
			(void) strcat(tbuf, action);
			}
		}

	/* Check for @presentation keywords "pattern_width" "pattern_length" */
	else if ( same(key, "pattern_width")
			|| same(key, "pattern_length") )
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestCormetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void) sprintf(tbuf, "%.0f", value);
		}

	/* Check for @presentation keyword "font"            */
	/*  or @label/@sample_field keyword "attribute_font" */
	else if ( same(key, "font")
			|| same(key, "attribute_font")
			)
		{
		(void) sprintf(tbuf, "@f \"%s\" 0", action);
		}

	/* Check for @presentation keyword "font_weight"            */
	/*  or @label/@sample_field keyword "attribute_font_weight" */
	else if ( same(key, "font_weight")
			|| same(key, "attribute_font_weight")
			)
		{
		if ( same_ic(action, FontWeightNone) )    (void) sprintf(tbuf, "0");
		else if ( same_ic(action, "thin") )       (void) sprintf(tbuf, "100");
		else if ( same_ic(action, "ultralight") ) (void) sprintf(tbuf, "200");
		else if ( same_ic(action, "light") )      (void) sprintf(tbuf, "300");
		else if ( same_ic(action, "normal") )     (void) sprintf(tbuf, "400");
		else if ( same_ic(action, "medium") )     (void) sprintf(tbuf, "500");
		else if ( same_ic(action, "demibold") )   (void) sprintf(tbuf, "600");
		else if ( same_ic(action, "bold") )       (void) sprintf(tbuf, "700");
		else if ( same_ic(action, "ultrabold") )  (void) sprintf(tbuf, "800");
		else if ( same_ic(action, "black") )      (void) sprintf(tbuf, "900");
		else
			{
			(void) warn_report("Invalid font_weight");
			(void) sprintf(tbuf, "0");
			}
		}

	/* Check for logical @presentation keyword "italics"    */
	/*  or @label/@sample_field keyword "attribute_italics" */
	else if ( same(key, "italics")
			|| same(key, "attribute_italics")
			)
		{
		if ( same_ic(action, LogicalYes) )
			(void) sprintf(tbuf, "1");
		else if ( same_ic(action, LogicalNo) )
			(void) sprintf(tbuf, "0");
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) sprintf(tbuf, "1");
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) sprintf(tbuf, "0");
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for @presentation keyword "justification"            */
	/*  or @wind_presentation/@vector_presentation keywords       */
	/*      "calm_justification" or "direction_justification" or  */
	/*      "speed_justification" or "gust_justification"         */
	/*  or @label/@sample_field keyword "attribute_justification" */
	/*  or @draw_distance_scale keyword "scale_justification"     */
	/*  or @distance_scale_ticks keyword "tick_justification"     */
	/*  or @distance_scale_labels keyword "label_justification"   */
	else if ( same(key, "justification")
			|| same(key, "calm_justification")
			|| same(key, "direction_justification")
			|| same(key, "speed_justification")
			|| same(key, "gust_justification")
			|| same(key, "attribute_justification")
			|| same(key, "scale_justification")
			|| same(key, "tick_justification")
			|| same(key, "label_justification")
			)
		{
		if ( same_ic(action, JustifyLeft) )
											(void) strcpy(tbuf, JustifyLeft);
		else if ( same_ic(action, JustifyCentre) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyCenter) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyRight) )
											(void) strcpy(tbuf, JustifyRight);
		else
			{
			(void) sprintf(err_buf, "Recognized justifications are: %s %s %s",
					JustifyLeft, JustifyCentre, JustifyRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @label/@plot/@sample_field keyword "attribute_vertical_just" */
	else if ( same(key, "attribute_vertical_just") )
		{
		if ( same_ic(action, VerticalBottom) )
										(void) strcpy(tbuf, VerticalBottom);
		else if ( same_ic(action, VerticalCentre) )
										(void) strcpy(tbuf, VerticalCentre);
		else if ( same_ic(action, VerticalCenter) )
										(void) strcpy(tbuf, VerticalCentre);
		else if ( same_ic(action, VerticalTop) )
										(void) strcpy(tbuf, VerticalTop);
		else
			{
			(void) sprintf(err_buf,
					"Recognized vertical justifications are: %s %s %s",
					VerticalBottom, VerticalCentre, VerticalTop);
			(void) error_report(err_buf);
			}
		}

	/* Check for @anchor keyword "ref"                         */
	/*  and @label/@plot/@sample_field keyword "attribute_ref" */
	else if ( same(key, "ref"))
		{
		if ( same_ic(action, AnchorMap) )
			{
			(void) strcpy(tbuf, AnchorMap);
			}
		else if ( same_ic(action, AnchorMapLatLon) )
			{
			(void) strcpy(tbuf, AnchorMapLatLon);
			}
		else if ( same_ic(action, AnchorAbsolute) )
			{
			(void) strcpy(tbuf, AnchorAbsolute);
			}
		else if ( same_ic(action, AnchorCurrent) )
			{
			(void) strcpy(tbuf, AnchorCurrent);
			}
		else if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}
		else if ( same_ic(action, AnchorXsectLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerLeft);
			}
		else if ( same_ic(action, AnchorXsectCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreLeft);
			}
		else if ( same_ic(action, AnchorXsectCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreLeft);
			}
		else if ( same_ic(action, AnchorXsectUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperLeft);
			}
		else if ( same_ic(action, AnchorXsectLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerCentre);
			}
		else if ( same_ic(action, AnchorXsectLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerCentre);
			}
		else if ( same_ic(action, AnchorXsectCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectCentre);
			}
		else if ( same_ic(action, AnchorXsectCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectCentre);
			}
		else if ( same_ic(action, AnchorXsectUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperCentre);
			}
		else if ( same_ic(action, AnchorXsectUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperCentre);
			}
		else if ( same_ic(action, AnchorXsectLowerRight) )
			{
			(void) strcpy(tbuf, AnchorXsectLowerRight);
			}
		else if ( same_ic(action, AnchorXsectCentreRight) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreRight);
			}
		else if ( same_ic(action, AnchorXsectCenterRight) )
			{
			(void) strcpy(tbuf, AnchorXsectCentreRight);
			}
		else if ( same_ic(action, AnchorXsectUpperRight) )
			{
			(void) strcpy(tbuf, AnchorXsectUpperRight);
			}

		/* Error message for incorrect keyword "ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized anchor refs are: %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
					AnchorMap, AnchorMapLatLon, AnchorAbsolute, AnchorCurrent,
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight,
					AnchorXsectLowerLeft, AnchorXsectCentreLeft,
					AnchorXsectUpperLeft, AnchorXsectLowerCentre,
					AnchorXsectCentre, AnchorXsectUpperCentre,
					AnchorXsectLowerRight, AnchorXsectCentreRight,
					AnchorXsectUpperRight);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "attribute_ref"))
		{
		if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}

		/* Error message for incorrect keyword "attribute_ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized attribute refs are: %s %s %s %s %s %s %s %s %s",
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @arrow_display keyword "arrow_features" */
	else if ( same(key, "arrow_features") )
		{
		if ( same_ic(action, ArrowHead) )
										(void) strcpy(tbuf, ArrowHead);
		else if ( same_ic(action, ArrowTail) )
										(void) strcpy(tbuf, ArrowTail);
		else if ( same_ic(action, ArrowBoth) )
										(void) strcpy(tbuf, ArrowBoth);
		else if ( same_ic(action, ArrowBothRev) )
										(void) strcpy(tbuf, ArrowBothRev);
		else
			{
			(void) sprintf(err_buf,
					"Recognized arrow features are: %s %s %s %s",
					ArrowHead, ArrowTail, ArrowBoth, ArrowBothRev);
			(void) error_report(err_buf);
			}
		}

	/* Check for @wind_presentation/@vector_presentation keywords "..._type" */
	else if ( same(key, "calm_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized calm types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "direction_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubUniform) )
										(void) strcpy(tbuf, WVsubUniform);
		else if ( same_ic(action, WVsubProportional) )
										(void) strcpy(tbuf, WVsubProportional);
		else
			{
			(void) sprintf(err_buf,
					"Recognized direction types are: %s %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubUniform,
					WVsubProportional);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "speed_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized speed types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "gust_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else if ( same_ic(action, WVsubSymbol) )
										(void) strcpy(tbuf, WVsubSymbol);
		else
			{
			(void) sprintf(err_buf,
					"Recognized gust types are: %s %s %s %s",
					WVsubNone, WVsubValue, WVsubText, WVsubSymbol);
			(void) error_report(err_buf);
			}
		}

	/* Check for "category_cascade" keyword for directives                 */
	/*  @loop_begin/@areas/@cross_section_areas/@cross_section_curves      */
	/*  @geography/@label/@lchain_nodes/lchain_tracks/@lines/@sample_field */
	else if ( same(key, "category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check for "track_category_cascade" keyword for directive @lchain_nodes */
	else if ( same(key, "track_category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "track_category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized track_category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check for @cross_section_areas/curves keyword "display_function" */
	else if ( same(key, "display_function"))
		{
		if ( same_ic(action, XSectLineLinear) )
									(void) strcpy(tbuf, XSectLineLinear);
		else if ( same_ic(action, XSectLineStepBefore) )
									(void) strcpy(tbuf, XSectLineStepBefore);
		else if ( same_ic(action, XSectLineStepCentre) )
									(void) strcpy(tbuf, XSectLineStepCentre);
		else if ( same_ic(action, XSectLineStepCenter) )
									(void) strcpy(tbuf, XSectLineStepCentre);
		else if ( same_ic(action, XSectLineStepAfter) )
									(void) strcpy(tbuf, XSectLineStepAfter);
		else if ( same_ic(action, XSectLineBox) )
									(void) strcpy(tbuf, XSectLineBox);
		else
			{
			(void) sprintf(err_buf,
					"Recognized display functions are: %s %s %s %s %s",
					XSectLineLinear, XSectLineStepBefore, XSectLineStepCentre,
					XSectLineStepAfter, XSectLineBox);
			(void) error_report(err_buf);
			}
		}

	/* Check all keywords expressed in terms of display units */
	else if ( same(key, "size")
			|| same(key, "width")
			|| same(key, "height")
			|| same(key, "diameter")
			|| same(key, "radius")
			|| same(key, "box_width")
			|| same(key, "text_size")
			|| same(key, "x")
			|| same(key, "y")
			|| same(key, "extra_x")
			|| same(key, "extra_y")
			|| same(key, "x_off")
			|| same(key, "y_off")
			|| same(key, "x_box_off")
			|| same(key, "y_box_off")
			|| same(key, "x_display_off")
			|| same(key, "y_display_off")
			|| same(key, "y_tilt_off")
			|| same(key, "x_eye")
			|| same(key, "y_eye")
			|| same(key, "z_eye")
			|| same(key, "x_label")
			|| same(key, "y_label")
			|| same(key, "x_val")
			|| same(key, "y_val")
			|| same(key, "calm_size")
			|| same(key, "x_calm")
			|| same(key, "y_calm")
			|| same(key, "direction_size")
			|| same(key, "x_dir")
			|| same(key, "y_dir")
			|| same(key, "speed_size")
			|| same(key, "x_spd")
			|| same(key, "y_spd")
			|| same(key, "gust_size")
			|| same(key, "x_gust")
			|| same(key, "y_gust")
			|| same(key, "shaft_length")
			|| same(key, "arrow_length")
			|| same(key, "length_offset")
			|| same(key, "width_offset")
			|| same(key, "attribute_text_size")
			|| same(key, "attribute_x_off")
			|| same(key, "attribute_y_off")
			|| same(key, "margin_left")
			|| same(key, "margin_right")
			|| same(key, "margin_top")
			|| same(key, "margin_bottom")
			|| same(key, "margin_width")
			|| same(key, "margin_height")
			|| same(key, "x_repeat")
			|| same(key, "y_repeat")
			|| same(key, "x_shift")
			|| same(key, "y_shift")
			|| same(key, "x_stationary")
			|| same(key, "y_stationary")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.conversion;
		if ( fabs((double) value) > LargestCormetDistance )
			{
			(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
					DisplayUnits.type, key, action);
			(void) warn_report(err_buf);
			}
		(void ) sprintf(tbuf, "%.0f", value);
		}

	/* Check for @define_line keyword "line" ... a list of locations */
	/* Note that scaling line locations is done in set_define_line() */
	else if ( same(key, "line") )
		{
		(void) strcpy(xbuf, action);
		status = TRUE;
		even   = TRUE;
		while ( status )
			{
			value = float_arg(xbuf, &status);

			/* End of line reached on an even number of points */
			if ( !status && even )
				break;
			else if ( !status )
				{
				(void) sprintf(err_buf, "Odd number of points in %s = %s",
						key, action);
				(void) error_report(err_buf);
				}
			else
				even = !even;

			value *= DisplayUnits.conversion;
			if ( fabs((double) value) > LargestCormetDistance )
				{
				(void) sprintf(err_buf, "Units: %s ... problem with %s = %s",
						DisplayUnits.type, key, action);
				(void) warn_report(err_buf);
				}
			}
		(void) strcpy(tbuf, action);
		}

	/* Check all keywords using symbol scaling */
	else if ( same(key, "scale")
			|| same(key, "symbol_scale")
			|| same(key, "mark_scale")
			|| same(key, "attribute_symbol_scale")
			|| same(key, "calm_scale")
			|| same(key, "huge_scale")
			|| same(key, "direction_scale")
			|| same(key, "speed_scale")
			|| same(key, "gust_scale")
			)
		{
		(void) sscanf(action, "%f", &value);
		value *= DisplayUnits.sfactor;
		(void ) sprintf(tbuf, "%f", value);
		}

	/* Check for logical keywords */
	else if ( same(key, "outline_first")
			|| same(key, "attribute_outline_first")
			|| same(key, "fit_to_map")
			|| same(key, "scale_to_perspective")
			|| same(key, "display_as_areas")
			|| same(key, "display_at_feature")
			|| same(key, "pattern_for_holes")
			|| same(key, "closed")
			|| same(key, "rotate_to_latitude")
			|| same(key, "rotate_to_longitude")
			|| same(key, "constrain_rotation")
			)
		{
		if ( same_ic(action, LogicalYes) )
			(void) strcpy(tbuf, LogicalYes);
		else if ( same_ic(action, LogicalNo) )
			(void) strcpy(tbuf, LogicalNo);
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalYes);
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalNo);
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for common @(...)presentation keywords */
	else if ( same(key, "pattern")
			|| same(key, "rotation")
			|| same(key, "char_space")
			|| same(key, "word_space")
			|| same(key, "line_space")
			|| same(key, "start_angle")
			|| same(key, "end_angle")
			|| same(key, "barb_length")
			|| same(key, "barb_width")
			|| same(key, "barb_space")
			|| same(key, "barb_angle")
			|| same(key, "speed_round")
			|| same(key, "gust_above")
			|| same(key, "gust_round")
			|| same(key, "gust_distance")
			|| same(key, "gust_angle")
			|| same(key, "calm_max")
			|| same(key, "calm_symbol")
			|| same(key, "huge_min")
			|| same(key, "huge_symbol")
			|| same(key, "calm_format")
			|| same(key, "direction_format")
			|| same(key, "speed_format")
			|| same(key, "gust_format")
			|| same(key, "wind_look_up")
			|| same(key, "vector_look_up")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @(...)display keywords */
	else if ( same(key, "arrow_name")
			|| same(key, "arrow_angle")
			|| same(key, "return_angle")
			|| same(key, "head_length")
			|| same(key, "tail_length")
			|| same(key, "display_name")
			|| same(key, "display_type")
			|| same(key, "width_scale")
			|| same(key, "height_scale")
			|| same(key, "width_attribute")
			|| same(key, "height_attribute")
			|| same(key, "diameter_attribute")
			|| same(key, "radius_attribute")
			|| same(key, "symbol_fill_name")
			|| same(key, "symbol_rotation")
			|| same(key, "scale_name")
			|| same(key, "scale_length")
			|| same(key, "scale_units")
			|| same(key, "scale_rotation")
			|| same(key, "tick_location")
			|| same(key, "tick_length")
			|| same(key, "tick_units")
			|| same(key, "tick_rotation")
			|| same(key, "label_location")
			|| same(key, "label_string")
			|| same(key, "label_units")
			|| same(key, "label_rotation")
			|| same(key, "rotation_attribute")
			/* >>> "repeat_rotation" not used yet! <<< */
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @projection @mapdef @resolution keywords */
	else if ( same(key, "type")
			|| same(key, "ref1")
			|| same(key, "ref2")
			|| same(key, "ref3")
			|| same(key, "ref4")
			|| same(key, "ref5")
			|| same(key, "olat")
			|| same(key, "olon")
			|| same(key, "rlon")
			|| same(key, "xmin")
			|| same(key, "ymin")
			|| same(key, "xmax")
			|| same(key, "ymax")
			|| same(key, "map_units")
			|| same(key, "res")
			|| same(key, "map_x")
			|| same(key, "map_y")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for other common CorMet keywords */
	else if ( same(key, "dir")
			|| same(key, "name")
			|| same(key, "geo_name")
			|| same(key, "geo_file")
			|| same(key, "file")			/* >>> Version 6 obsolete <<< */
			|| same(key, "scale_factor")
			|| same(key, "number_of_iterations")
			|| same(key, "keyword_name")
			|| same(key, "keyword_value")
			|| same(key, "keyword_value_list")
			|| same(key, "group_name")
			|| same(key, "line_name")
			|| same(key, "table_name")
			|| same(key, "site_label")
			|| same(key, "last_site")
			|| same(key, "grid_name")
			|| same(key, "list_name")
			|| same(key, "look_up")
			|| same(key, "case")
			|| same(key, "case_look_up")
			|| same(key, "symbol")
			|| same(key, "mark")
			|| same(key, "map_scale")
			|| same(key, "axis_to_scale")
			|| same(key, "format")
			|| same(key, "time")
			|| same(key, "zone_type")
			|| same(key, "time_zone")
			|| same(key, "language")
			|| same(key, "text")
			|| same(key, "text_file")
			|| same(key, "string")
			|| same(key, "data_file")
			|| same(key, "data_file_format")
			|| same(key, "data_file_units")
			|| same(key, "data_file_wind_units")
			|| same(key, "lat")
			|| same(key, "lon")
			|| same(key, "location_ident")
			|| same(key, "location_look_up")
			|| same(key, "location_distances")
			|| same(key, "location_units")
			|| same(key, "location_times")
			|| same(key, "location_fractions")
			|| same(key, "location_interval")
			|| same(key, "lat_begin")
			|| same(key, "lat_end")
			|| same(key, "lat_interval")
			|| same(key, "lon_begin")
			|| same(key, "lon_end")
			|| same(key, "lon_interval")
			|| same(key, "map_x_begin")
			|| same(key, "map_x_end")
			|| same(key, "map_x_interval")
			|| same(key, "map_y_begin")
			|| same(key, "map_y_end")
			|| same(key, "map_y_interval")
			|| same(key, "lat_lon_ident")
			|| same(key, "map_x_y_ident")
			|| same(key, "location_ident_list")
			|| same(key, "proximity")
			|| same(key, "proximity_units")
			|| same(key, "values")
			|| same(key, "range")
			|| same(key, "base")
			|| same(key, "min")
			|| same(key, "max")
			|| same(key, "interval")
			|| same(key, "area_type")
			|| same(key, "fit_to_map_ref")
			|| same(key, "tilt_angle")
			|| same(key, "show_perspective_view")
			|| same(key, "x_stretch")
			|| same(key, "y_stretch")
			|| same(key, "x_wrap")
			|| same(key, "y_wrap")
			|| same(key, "cross_section_name")
			|| same(key, "axis_for_display")
			|| same(key, "line_to_draw")
			|| same(key, "vertical_look_up")
			|| same(key, "vertical_data_file")
			|| same(key, "vertical_data_file_format")
			|| same(key, "vertical_data_file_units")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common configuration keywords */
	else if ( same(key, "element")
			|| same(key, "element_list")
			|| same(key, "subelement")		/* >>> Version 6 obsolete <<< */
			|| same(key, "subelements")		/* >>> Version 6 obsolete <<< */
			|| same(key, "level")
			|| same(key, "level_list")
			|| same(key, "equation")
			|| same(key, "units")
			|| same(key, "field_type")
			|| same(key, "category_attribute")
			|| same(key, "category")
			|| same(key, "track_category_attribute")
			|| same(key, "track_category")
			|| same(key, "attribute")
			|| same(key, "source")
			|| same(key, "valid_time")
			|| same(key, "wind_crossref")
			|| same(key, "vertical_element")
			|| same(key, "vertical_level")
			|| same(key, "vertical_equation")
			|| same(key, "vertical_units")
			|| same(key, "vertical_attribute")
			|| same(key, "vertical_attribute_upper")
			|| same(key, "vertical_attribute_lower")
			|| same(key, "vertical_field_type")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for "attribute" display keywords */
	else if ( same(key, "attribute_show")
			|| same(key, "attribute_anchor")
			|| same(key, "attribute_units")
			|| same(key, "attribute_format")
			|| same(key, "attribute_look_up")
			|| same(key, "attribute_display_name")
			|| same(key, "attribute_display_type")
			|| same(key, "attribute_width_scale")
			|| same(key, "attribute_height_scale")
			|| same(key, "attribute_char_space")
			|| same(key, "attribute_word_space")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Error message for unrecognized keywords! */
	else
		{
		(void) sprintf(err_buf, "Unrecognized keyword ... %s", key);
		(void) error_report(err_buf);
		}

	/* Overwrite the input buffer and return */
	(void) strcpy(action, tbuf);
	return;
	}

void		check_texmet_keyword

	(
	STRING		key,
	STRING		action
	)

	{
	char		tbuf[GPGMedium];
	char		err_buf[GPGLong];

	/* Check for @presentation keyword "justification"           */
	/*  or @wind_presentation/@vector_presentation keywords      */
	/*      "calm_justification" or "direction_justification" or */
	/*      "speed_justification" or "gust_justification"        */
	/*  or @sample_field keyword "attribute_justification"       */
	if ( same(key, "justification")
			|| same(key, "calm_justification")
			|| same(key, "direction_justification")
			|| same(key, "speed_justification")
			|| same(key, "gust_justification")
			|| same(key, "attribute_justification")
			)
		{
		if ( same_ic(action, JustifyLeft) )
											(void) strcpy(tbuf, JustifyLeft);
		else if ( same_ic(action, JustifyCentre) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyCenter) )
											(void) strcpy(tbuf, JustifyCentre);
		else if ( same_ic(action, JustifyRight) )
											(void) strcpy(tbuf, JustifyRight);
		else
			{
			(void) sprintf(err_buf, "Recognized justifications are: %s %s %s",
					JustifyLeft, JustifyCentre, JustifyRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for @sample_field keyword "attribute_vertical_just" */
	else if ( same(key, "attribute_vertical_just") )
		{
		if ( same_ic(action, VerticalBottom) )
										(void) strcpy(tbuf, VerticalBottom);
		else if ( same_ic(action, VerticalTop) )
										(void) strcpy(tbuf, VerticalTop);
		else
			{
			(void) sprintf(err_buf,
					"Recognized vertical justifications are: %s %s",
					VerticalBottom, VerticalTop);
			(void) error_report(err_buf);
			}
		}

	/* Check for @anchor keyword "ref"                         */
	/*  and @label/@plot/@sample_field keyword "attribute_ref" */
	else if ( same(key, "ref"))
		{
		if ( same_ic(action, AnchorAbsolute) )
			{
			(void) strcpy(tbuf, AnchorAbsolute);
			}
		else if ( same_ic(action, AnchorCurrent) )
			{
			(void) strcpy(tbuf, AnchorCurrent);
			}
		else if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorCentreLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorCenterLeft) )
			{
			(void) strcpy(tbuf, AnchorCentreLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorCentre) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorCenter) )
			{
			(void) strcpy(tbuf, AnchorCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorCentreRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorCenterRight) )
			{
			(void) strcpy(tbuf, AnchorCentreRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}

		/* Error message for incorrect keyword "ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized anchor refs are: %s %s %s %s %s %s %s %s %s %s %s",
					AnchorAbsolute, AnchorCurrent,
					AnchorLowerLeft, AnchorCentreLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorCentreRight, AnchorUpperRight);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "attribute_ref"))
		{
		if ( same_ic(action, AnchorLowerLeft) )
			{
			(void) strcpy(tbuf, AnchorLowerLeft);
			}
		else if ( same_ic(action, AnchorUpperLeft) )
			{
			(void) strcpy(tbuf, AnchorUpperLeft);
			}
		else if ( same_ic(action, AnchorLowerCentre) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorLowerCenter) )
			{
			(void) strcpy(tbuf, AnchorLowerCentre);
			}
		else if ( same_ic(action, AnchorUpperCentre) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorUpperCenter) )
			{
			(void) strcpy(tbuf, AnchorUpperCentre);
			}
		else if ( same_ic(action, AnchorLowerRight) )
			{
			(void) strcpy(tbuf, AnchorLowerRight);
			}
		else if ( same_ic(action, AnchorUpperRight) )
			{
			(void) strcpy(tbuf, AnchorUpperRight);
			}

		/* Error message for incorrect keyword "attribute_ref" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized attribute refs are: %s %s %s %s %s %s",
					AnchorLowerLeft, AnchorUpperLeft,
					AnchorLowerCentre, AnchorUpperCentre,
					AnchorLowerRight, AnchorUpperRight);
			(void) error_report(err_buf);
			}
		}

	/* Check for "category_cascade" keyword for directives */
	/*  @loop_begin/@label/@sample_field                   */
	else if ( same(key, "category_cascade"))
		{
		if ( same_ic(action, CatCascadeAnd) )
			{
			(void) strcpy(tbuf, CatCascadeAnd);
			}
		else if ( same_ic(action, CatCascadeOr) )
			{
			(void) strcpy(tbuf, CatCascadeOr);
			}

		/* Error message for incorrect keyword "category_cascade" */
		else
			{
			(void) sprintf(err_buf,
					"Recognized category_cascade values are: %s %s",
					CatCascadeAnd, CatCascadeOr);
			(void) error_report(err_buf);
			}
		}

	/* Check all keywords expressed in terms of file position */
	else if ( same(key, "columns")
			|| same(key, "rows")
			|| same(key, "column")
			|| same(key, "row")
			|| same(key, "x_off")
			|| same(key, "y_off")
			|| same(key, "x_label")
			|| same(key, "y_label")
			|| same(key, "x_calm")
			|| same(key, "y_calm")
			|| same(key, "x_dir")
			|| same(key, "y_dir")
			|| same(key, "x_spd")
			|| same(key, "y_spd")
			|| same(key, "x_gust")
			|| same(key, "y_gust")
			|| same(key, "attribute_x_off")
			|| same(key, "attribute_y_off")
			|| same(key, "x_shift")
			|| same(key, "y_shift")
			|| same(key, "x_stationary")
			|| same(key, "y_stationary")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common @projection @mapdef @resolution keywords */
	else if ( same(key, "type")
			|| same(key, "ref1")
			|| same(key, "ref2")
			|| same(key, "ref3")
			|| same(key, "ref4")
			|| same(key, "ref5")
			|| same(key, "olat")
			|| same(key, "olon")
			|| same(key, "rlon")
			|| same(key, "xmin")
			|| same(key, "ymin")
			|| same(key, "xmax")
			|| same(key, "ymax")
			|| same(key, "map_units")
			|| same(key, "res")
			|| same(key, "map_x")
			|| same(key, "map_y")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for @wind_presentation/@vector_presentation keywords "..._type" */
	else if ( same(key, "calm_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else
			{
			(void) sprintf(err_buf,
					"Recognized calm types are: %s %s %s",
					WVsubNone, WVsubValue, WVsubText);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "direction_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else
			{
			(void) sprintf(err_buf,
					"Recognized direction types are: %s %s %s",
					WVsubNone, WVsubValue, WVsubText);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "speed_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else
			{
			(void) sprintf(err_buf,
					"Recognized speed types are: %s %s %s",
					WVsubNone, WVsubValue, WVsubText);
			(void) error_report(err_buf);
			}
		}
	else if ( same(key, "gust_type") )
		{
		if ( same_ic(action, WVsubNone) )
										(void) strcpy(tbuf, WVsubNone);
		else if ( same_ic(action, WVsubValue) )
										(void) strcpy(tbuf, WVsubValue);
		else if ( same_ic(action, WVsubText) )
										(void) strcpy(tbuf, WVsubText);
		else
			{
			(void) sprintf(err_buf,
					"Recognized gust types are: %s %s %s",
					WVsubNone, WVsubValue, WVsubText);
			(void) error_report(err_buf);
			}
		}

	/* Check for logical keywords */
	else if ( same(key, "fit_to_map") )
		{
		if ( same_ic(action, LogicalYes) )
			(void) strcpy(tbuf, LogicalYes);
		else if ( same_ic(action, LogicalNo) )
			(void) strcpy(tbuf, LogicalNo);
		else if ( same_ic(action, "true") || same_ic(action, "t")
					|| same_ic(action, "on") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalYes);
			}
		else if ( same_ic(action, "false") || same_ic(action, "f")
					|| same_ic(action, "off") )
			{
			(void) sprintf(err_buf, "Parameter should be ... %s = %s",
					key, LogicalYes);
			(void) warn_report(err_buf);
			(void) strcpy(tbuf, LogicalNo);
			}
		else
			{
			(void) sprintf(err_buf, "Recognized values for %s are: %s %s",
					key, LogicalYes, LogicalNo);
			(void) error_report(err_buf);
			}
		}

	/* Check for other common TexMet keywords */
	else if ( same(key, "dir")
			|| same(key, "name")
			|| same(key, "geo_file")
			|| same(key, "file")			/* >>> Version 6 obsolete <<< */
			|| same(key, "number_of_iterations")
			|| same(key, "keyword_name")
			|| same(key, "keyword_value")
			|| same(key, "keyword_value_list")
			|| same(key, "group_name")
			|| same(key, "table_name")
			|| same(key, "site_label")
			|| same(key, "look_up")
			|| same(key, "grid_name")
			|| same(key, "list_name")
			|| same(key, "case")
			|| same(key, "case_look_up")
			|| same(key, "format")
			|| same(key, "time")
			|| same(key, "zone_type")
			|| same(key, "time_zone")
			|| same(key, "language")
			|| same(key, "text_file")
			|| same(key, "string")
			|| same(key, "data_file")
			|| same(key, "data_file_format")
			|| same(key, "data_file_units")
			|| same(key, "data_file_wind_units")
			|| same(key, "lat")
			|| same(key, "lon")
			|| same(key, "gust_above")
			|| same(key, "calm_format")
			|| same(key, "direction_format")
			|| same(key, "speed_format")
			|| same(key, "gust_format")
			|| same(key, "wind_look_up")
			|| same(key, "vector_look_up")
			|| same(key, "location_ident")
			|| same(key, "location_look_up")
			|| same(key, "location_distances")
			|| same(key, "location_units")
			|| same(key, "location_times")
			|| same(key, "location_fractions")
			|| same(key, "location_interval")
			|| same(key, "start_time")
			|| same(key, "end_time")
			|| same(key, "times")
			|| same(key, "labels")
			|| same(key, "node_speed_units")
			|| same(key, "node_speed_round")
			|| same(key, "node_stationary_max")
			|| same(key, "node_stationary_label")
			|| same(key, "track_length_min")
			|| same(key, "track_length_units")
			|| same(key, "lat_begin")
			|| same(key, "lat_end")
			|| same(key, "lat_interval")
			|| same(key, "lon_begin")
			|| same(key, "lon_end")
			|| same(key, "lon_interval")
			|| same(key, "map_x_begin")
			|| same(key, "map_x_end")
			|| same(key, "map_x_interval")
			|| same(key, "map_y_begin")
			|| same(key, "map_y_end")
			|| same(key, "map_y_interval")
			|| same(key, "lat_lon_ident")
			|| same(key, "map_x_y_ident")
			|| same(key, "location_ident_list")
			|| same(key, "proximity")
			|| same(key, "proximity_units")
			|| same(key, "x_wrap")
			|| same(key, "y_wrap")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for common configuration keywords */
	else if ( same(key, "element")
			|| same(key, "subelement")		/* >>> Version 6 obsolete <<< */
			|| same(key, "subelements")		/* >>> Version 6 obsolete <<< */
			|| same(key, "level")
			|| same(key, "equation")
			|| same(key, "units")
			|| same(key, "field_type")
			|| same(key, "category_attribute")
			|| same(key, "category")
			|| same(key, "attribute")
			|| same(key, "source")
			|| same(key, "valid_time")
			|| same(key, "wind_crossref")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Check for "attribute" display keywords */
	else if ( same(key, "attribute_show")
			|| same(key, "attribute_anchor")
			|| same(key, "attribute_units")
			|| same(key, "attribute_format")
			|| same(key, "attribute_look_up")
			)
		{
		(void) sprintf(tbuf, "%s", action);
		}

	/* Error message for unrecognized keywords! */
	else
		{
		(void) sprintf(err_buf, "Unrecognized keyword ... %s", key);
		(void) error_report(err_buf);
		}

	/* Overwrite the input buffer and return */
	(void) strcpy(action, tbuf);
	return;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES                                          *
*                                                                      *
*     All the routines after this point are available only within      *
*     this source file.                                                *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*    d e t e r m i n e _ t e x t _ s i z e                             *
*    d e t e r m i n e _ a t t r i b u t e _ o f f s e t               *
*                                                                      *
***********************************************************************/

static	void				determine_text_size

	(
	STRING		text,			/* text string */
	float		txt_size,		/* text size */
	float		width_scale,	/* scaling for width of text characters */
	float		height_scale,	/* scaling for height of text characters */
	float		*width,			/* returned width of text string */
	float		*height			/* returned height of text string */
	)

	{
	size_t		nlen;
	char		tbuf[GPGLong];

	/* Initialize the return parameters */
	if ( NotNull(width) )  *width  = 0.0;
	if ( NotNull(height) ) *height = 0.0;

	/* Strip newline character off the end of text string */
	(void) strcpy(tbuf, text);
	nlen = strlen(tbuf);
	if ( nlen > (size_t) 0 )
		{
		if ( tbuf[nlen-1] == '\n' )
			{
			tbuf[nlen-1] = '\0';
			nlen--;
			}
		}

	/* Return width and height of text string */
	if ( nlen > (size_t) 0 )
		{
		if ( NotNull(width) )  *width  = txt_size * (float) nlen
													* width_scale  / 100.0;
		if ( NotNull(height) ) *height = txt_size   * height_scale / 100.0;
		}
	else
		{
		if ( NotNull(width) )  *width  = 0.0;
		if ( NotNull(height) ) *height = 0.0;
		}
	}

/**********************************************************************/

static	void				determine_attribute_offset
	(
	float		xpos,
	float		ypos,
	float		rotation,
	float		xloc,
	float		yloc,
	LOGICAL		*fit_left,
	LOGICAL		*fit_right,
	LOGICAL		*fit_top,
	LOGICAL		*fit_bottom,
	float		*xshft,
	float		*yshft
	)

	{
	float		xx, yy, diff;

	/* Determine rotated location */
	(void) rotated_location(xloc, yloc, rotation, &xx, &yy);
	xx += xpos;
	yy += ypos;

	/* Determine adjustments on current base map */
	if ( AnchorToMap )
		{

		/* Check for location to left of map */
		diff = xx - (float) ULpoint[X];
		if ( diff < 0.0 )
			{
			if ( NotNull(fit_left) ) *fit_left = FALSE;
			if ( NotNull(xshft) )
				{
				if ( diff < *xshft ) *xshft = diff;
				}
			}

		/* Check for location to right of map */
		diff = xx - (float) LRpoint[X];
		if ( diff > 0.0 )
			{
			if ( NotNull(fit_right) ) *fit_right = FALSE;
			if ( NotNull(xshft) )
				{
				if ( diff > *xshft ) *xshft = diff;
				}
			}

		/* Check for location above top of map */
		diff = yy - (float) ULpoint[Y];
		if ( diff > 0.0 )
			{
			if ( NotNull(fit_top) ) *fit_top = FALSE;
			if ( NotNull(yshft) )
				{
				if ( diff > *yshft ) *yshft = diff;
				}
			}

		/* Check for location below bottom of map */
		diff = yy - (float) LRpoint[Y];
		if ( diff < 0.0 )
			{
			if ( NotNull(fit_bottom) ) *fit_bottom = FALSE;
			if ( NotNull(yshft) )
				{
				if ( diff < *yshft ) *yshft = diff;
				}
			}
		}

	/* Determine adjustments on current cross section */
	else if ( AnchorToCrossSection )
		{

		/* Check for location to left of cross section */
		diff = xx - (float) (XSect_ULpoint[X] + XYpoint[X]);
		if ( diff < 0.0 )
			{
			if ( NotNull(fit_left) ) *fit_left = FALSE;
			if ( NotNull(xshft) )
				{
				if ( diff < *xshft ) *xshft = diff;
				}
			}

		/* Check for location to right of cross section */
		diff = xx - (float) (XSect_LRpoint[X] + XYpoint[X]);
		if ( diff > 0.0 )
			{
			if ( NotNull(fit_right) ) *fit_right = FALSE;
			if ( NotNull(xshft) )
				{
				if ( diff > *xshft ) *xshft = diff;
				}
			}

		/* Check for location above top of cross section */
		diff = yy - (float) (XSect_ULpoint[Y] + XYpoint[Y]);
		if ( diff > 0.0 )
			{
			if ( NotNull(fit_top) ) *fit_top = FALSE;
			if ( NotNull(yshft) )
				{
				if ( diff > *yshft ) *yshft = diff;
				}
			}

		/* Check for location below bottom of cross section */
		diff = yy - (float) (XSect_LRpoint[Y] + XYpoint[Y]);
		if ( diff < 0.0 )
			{
			if ( NotNull(fit_bottom) ) *fit_bottom = FALSE;
			if ( NotNull(yshft) )
				{
				if ( diff < *yshft ) *yshft = diff;
				}
			}
		}
	}

/***********************************************************************
*                                                                      *
*    p e r s p e c t i v e _ m a p _ t o _ p a g e                     *
*    p e r s p e c t i v e _ p a g e _ t o _ m a p                     *
*                                                                      *
***********************************************************************/

static	void				perspective_map_to_page

	(
	const POINT	map,
	POINT		page,
	float		*scale
	)

	{
	float	ex, ey, ez, xx, yy, qx, qy, qz, ss, ssx, ssy;

	/* Initialize the return parameters */
	if ( NotNull(scale) ) *scale = 1.0;

	/* Set the perspective "eye" location from the centre of the current map */
	/*  and located 2*MapHeight back from the centre                         */
	ex =  PerspectiveXEye;
	ey =  PerspectiveYEye + (float) HalfMapHeight;
	ez = -PerspectiveZEye - (float) (MapHeight * 2.0);

	/* Set the translated location on the map          */
	/*  ... X location is wrt center of map            */
	/*  ... Y location is wrt bottom of map (adjusted) */
	xx = map[X] - (float) HalfMapWidth;
	yy = map[Y] - PerspectiveYTiltOffSet;
	qx = xx;
	qy = yy * PerspectiveCosTilt;
	qz = yy * PerspectiveSinTilt;

	/* Determine the scaling factor */
	ss = -ez / (qz - ez);
	if ( NotNull(scale) ) *scale = ss;

	/* Apply stretching to x and y scaling factors */
	ssx = ss * PerspectiveXStretch;
	ssy = ss * PerspectiveYStretch;

	/* Set the perspective location */
	if ( NotNull(page) )
		{
		page[X] = ex + (ssx * (qx - ex)) + (float) HalfMapWidth;
		page[Y] = ey + (ssy * (qy - ey)) + PerspectiveYTiltOffSet;
		}
	}

/**********************************************************************/

static	void				perspective_page_to_map

	(
	const POINT	page,
	POINT		map,
	float		*scale
	)

	{
	float	ex, ey, ez, xx, yy, qx, qy, qz, ss, ssx, ssy;

	/* Initialize the return parameters */
	if ( NotNull(scale) ) *scale = 1.0;

	/* Set the perspective "eye" location from the centre of the current map */
	/*  and located 2*MapHeight back from the centre                         */
	ex =  PerspectiveXEye;
	ey =  PerspectiveYEye + (float) HalfMapHeight;
	ez = -PerspectiveZEye - (float) (MapHeight * 2.0);

	/* Determine the scaling factor                    */
	/*  ... X location is wrt center of map            */
	/*  ... Y location is wrt bottom of map (adjusted) */
	xx = page[X] - (float) HalfMapWidth;
	yy = page[Y] - PerspectiveYTiltOffSet;
	if ( PerspectiveSinTilt == 0.0 ) ss = 1.0;
	else
		{
		qz = ez * yy / (yy - ey + (ez * PerspectiveCosTilt/PerspectiveSinTilt));
		ss = -ez / (qz - ez);
		}
	if ( NotNull(scale) ) *scale = ss;

	/* Apply stretching to x and y scaling factors */
	ssx = ss * PerspectiveXStretch;
	ssy = ss * PerspectiveYStretch;

	/* Set the perspective location on the map */
	qx = ex + ((xx - ex) / ssx);
	qy = ey + ((yy - ey) / ssy);
	if ( NotNull(map) )
		{
		map[X] = qx + (float) HalfMapWidth;
		if ( PerspectiveCosTilt == 0.0 ) map[Y] = PerspectiveYTiltOffSet;
		else
			{
			map[Y] = (qy / PerspectiveCosTilt) + PerspectiveYTiltOffSet;
			}
		}
	}

/***********************************************************************
*                                                                      *
*    g e t _ c a t e g o r y _ l o o k u p _ t a b l e                 *
*                                                                      *
***********************************************************************/

static	CAT_LOOKUP_TABLE	*get_category_lookup_table

	(
	STRING		lookup	/* category lookup table name */
	)

	{
	int					ilook, iline;
	STRING				table_file, sym_file;
	char				tbuf[GPGLong];
	char				*token1, *token2, *token3, *token4;
	FILE				*table_fp;
	CAT_LOOKUP_TABLE	*CatLookTable;
	CAT_LOOKUP_LINE		*CatLookLine;
	char				err_buf[GPGLong];

	/* First find the full path of the lookup file */
	table_file = find_lookup_file(lookup, LookupCategoryExt);
	if ( blank(table_file) ) return NullPtr(CAT_LOOKUP_TABLE *);

	/* Search the list for the lookup file name */
	for ( ilook=0; ilook<NumCatLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(table_file, CatLookups[ilook].label) )
			{
			return &CatLookups[ilook];
			}
		}

	/* Otherwise, add another lookup table to the list */
	ilook        = NumCatLookup++;
	CatLookups   = GETMEM(CatLookups, CAT_LOOKUP_TABLE, NumCatLookup);
	CatLookTable = &CatLookups[ilook];

	/* Initialize the new lookup table */
	CatLookTable->label              = strdup(table_file);
	CatLookTable->numlines           = 0;
	CatLookTable->lines              = NullPtr(CAT_LOOKUP_LINE *);
	CatLookTable->ismiss             = FALSE;
	CatLookTable->mline.value        = NullString;
	CatLookTable->mline.symbolfile   = NullString;
	CatLookTable->mline.text         = NullString;
	CatLookTable->mline.presentation = NullString;
	CatLookTable->isdef              = FALSE;
	CatLookTable->dline.value        = NullString;
	CatLookTable->dline.symbolfile   = NullString;
	CatLookTable->dline.text         = NullString;
	CatLookTable->dline.presentation = NullString;

	/* Open the lookup file */
	if ( IsNull(table_fp = fopen(table_file, "r")) )
		{
		(void) sprintf(err_buf, "Cannot find category lookup table ... %s",
				table_file);
		(void) error_report(err_buf);
		}

	/* Read the lookup file line by line */
	if ( Verbose )
		{
		(void) fprintf(stdout, "  Reading category lookup table ... %s\n",
				table_file);
		}
	while ( NotNull( getvalidline(table_fp, tbuf, (size_t) GPGLong, Comment) ) )
		{

		/* Extract the category lookup table parameters */
		token1 = strtok(tbuf, ":");
		(void) no_white(token1);

		token2 = strtok('\0', ":");
		(void) no_white(token2);

		token3 = strtok('\0', ":");
		(void) no_white(token3);

		token4 = strtok('\0', CommentOrEnd);
		(void) no_white(token4);

		/* Skip lines that contain no parameters ... comment lines! */
		if ( blank(token2) && blank(token3) && blank(token4) ) continue;

		/* Get the full path of the symbol file */
		sym_file = FpaCblank;
		if ( !blank(token2) )
			{
			sym_file = find_symbol_file(token2);
			}

		/* Add the category lookup table parameters to the default list */
		if ( same(token1, "*missing*") )
			{
			CatLookTable->ismiss = TRUE;
			CatLookLine          = &CatLookTable->mline;
			CatLookLine->value        = safe_strdup(FpaCblank);
			CatLookLine->symbolfile   = safe_strdup(sym_file);
			CatLookLine->text         = safe_strdup(token3);
			CatLookLine->presentation = safe_strdup(token4);
			}

		/* Add the category lookup table parameters to the default list */
		else if ( same(token1, "*default*") )
			{
			CatLookTable->isdef  = TRUE;
			CatLookLine          = &CatLookTable->dline;
			CatLookLine->value        = safe_strdup(FpaCblank);
			CatLookLine->symbolfile   = safe_strdup(sym_file);
			CatLookLine->text         = safe_strdup(token3);
			CatLookLine->presentation = safe_strdup(token4);
			}

		/* Add the category lookup table parameters to the table list */
		else
			{
			iline                = CatLookTable->numlines++;
			CatLookTable->lines  = GETMEM(CatLookTable->lines,
											CAT_LOOKUP_LINE,
											CatLookTable->numlines);
			CatLookLine          = &CatLookTable->lines[iline];
			CatLookLine->value        = safe_strdup(token1);
			CatLookLine->symbolfile   = safe_strdup(sym_file);
			CatLookLine->text         = safe_strdup(token3);
			CatLookLine->presentation = safe_strdup(token4);
			}
		}

	/* Close the category lookup table and return the current pointer */
	(void) fclose(table_fp);
	return &CatLookups[ilook];
	}

/***********************************************************************
*                                                                      *
*    g e t _ w i n d _ l o o k u p _ t a b l e                         *
*    g e t _ v e c t o r _ l o o k u p _ t a b l e                     *
*                                                                      *
***********************************************************************/

/* >>> add warning messages for improper lines <<< */
/* >>> improper  wtype                         <<< */
/* >>> improper  wcat/wcatx  for given  wtype  <<< */

static	WIND_LOOKUP_TABLE	*get_wind_lookup_table

	(
	STRING		lookup	/* wind lookup table name */
	)

	{
	int					ilook, ncnt;
	LOGICAL				valid;
	STRING				table_file, sym_file;
	char				tbuf[GPGLong], xbuf[GPGLong];
	FILE				*table_fp;
	WIND_LOOKUP_TABLE	*WindLookTable;
	WVLOOKUP_VALUE		*WVLookValue;
	WVLOOKUP_TEXT		*WVLookText;
	WVLOOKUP_SYMBOL		*WVLookSymbol;
	WVLOOKUP_UNIFORM	*WVLookUniform;
	WVLOOKUP_PROPORT	*WVLookProport;
	char				err_buf[GPGLong];

	/* Static buffers for wind type and category */
	static	char	wtype[GPGShort];
	static	char	wcat[GPGShort];
	static	char	wcatx[GPGShort];

	/* First find the full path of the lookup file */
	table_file = find_lookup_file(lookup, LookupWindExt);
	if ( blank(table_file) ) return NullPtr(WIND_LOOKUP_TABLE *);

	/* Search the list for the lookup file name */
	for ( ilook=0; ilook<NumWindLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(table_file, WindLookups[ilook].label) )
			{
			return &WindLookups[ilook];
			}
		}

	/* Otherwise, add another lookup table to the list */
	ilook         = NumWindLookup++;
	WindLookups   = GETMEM(WindLookups, WIND_LOOKUP_TABLE, NumWindLookup);
	WindLookTable = &WindLookups[ilook];

	/* Initialize the new lookup table */
	WindLookTable->label = strdup(table_file);

	WindLookTable->calm.wind_val.nvalue       = 0;
	WindLookTable->calm.wind_val.minval       = NullFloat;
	WindLookTable->calm.wind_val.maxval       = NullFloat;
	WindLookTable->calm.wind_val.round        = NullFloat;
	WindLookTable->calm.wind_val.factor       = NullInt;
	WindLookTable->calm.wind_val.ndigit       = NullInt;
	WindLookTable->calm.wind_txt.ntext        = 0;
	WindLookTable->calm.wind_txt.minval       = NullFloat;
	WindLookTable->calm.wind_txt.maxval       = NullFloat;
	WindLookTable->calm.wind_txt.texts        = NullStringList;
	WindLookTable->calm.wind_sym.nsymbol      = 0;
	WindLookTable->calm.wind_sym.minval       = NullFloat;
	WindLookTable->calm.wind_sym.maxval       = NullFloat;
	WindLookTable->calm.wind_sym.symbols      = NullStringList;

	WindLookTable->direction.wind_val.nvalue  = 0;
	WindLookTable->direction.wind_val.minval  = NullFloat;
	WindLookTable->direction.wind_val.maxval  = NullFloat;
	WindLookTable->direction.wind_val.round   = NullFloat;
	WindLookTable->direction.wind_val.factor  = NullInt;
	WindLookTable->direction.wind_val.ndigit  = NullInt;
	WindLookTable->direction.wind_txt.ntext   = 0;
	WindLookTable->direction.wind_txt.minval  = NullFloat;
	WindLookTable->direction.wind_txt.maxval  = NullFloat;
	WindLookTable->direction.wind_txt.texts   = NullStringList;
	WindLookTable->direction.wind_uni.nsymbol = 0;
	WindLookTable->direction.wind_uni.minval  = NullFloat;
	WindLookTable->direction.wind_uni.maxval  = NullFloat;
	WindLookTable->direction.wind_uni.symbols = NullStringList;
	WindLookTable->direction.wind_uni.rotate  = NullFloat;
	WindLookTable->direction.wind_pro.nsymbol = 0;
	WindLookTable->direction.wind_pro.minval  = NullFloat;
	WindLookTable->direction.wind_pro.maxval  = NullFloat;
	WindLookTable->direction.wind_pro.symbols = NullStringList;
	WindLookTable->direction.wind_pro.minscl  = NullFloat;
	WindLookTable->direction.wind_pro.maxscl  = NullFloat;
	WindLookTable->direction.wind_pro.rotate  = NullFloat;

	WindLookTable->speed.wind_val.nvalue      = 0;
	WindLookTable->speed.wind_val.minval      = NullFloat;
	WindLookTable->speed.wind_val.maxval      = NullFloat;
	WindLookTable->speed.wind_val.round       = NullFloat;
	WindLookTable->speed.wind_val.factor      = NullInt;
	WindLookTable->speed.wind_val.ndigit      = NullInt;
	WindLookTable->speed.wind_txt.ntext       = 0;
	WindLookTable->speed.wind_txt.minval      = NullFloat;
	WindLookTable->speed.wind_txt.maxval      = NullFloat;
	WindLookTable->speed.wind_txt.texts       = NullStringList;
	WindLookTable->speed.wind_sym.nsymbol     = 0;
	WindLookTable->speed.wind_sym.minval      = NullFloat;
	WindLookTable->speed.wind_sym.maxval      = NullFloat;
	WindLookTable->speed.wind_sym.symbols     = NullStringList;

	WindLookTable->gust.wind_val.nvalue       = 0;
	WindLookTable->gust.wind_val.minval       = NullFloat;
	WindLookTable->gust.wind_val.maxval       = NullFloat;
	WindLookTable->gust.wind_val.round        = NullFloat;
	WindLookTable->gust.wind_val.factor       = NullInt;
	WindLookTable->gust.wind_val.ndigit       = NullInt;
	WindLookTable->gust.wind_txt.ntext        = 0;
	WindLookTable->gust.wind_txt.minval       = NullFloat;
	WindLookTable->gust.wind_txt.maxval       = NullFloat;
	WindLookTable->gust.wind_txt.texts        = NullStringList;
	WindLookTable->gust.wind_sym.nsymbol      = 0;
	WindLookTable->gust.wind_sym.minval       = NullFloat;
	WindLookTable->gust.wind_sym.maxval       = NullFloat;
	WindLookTable->gust.wind_sym.symbols      = NullStringList;

	/* Open the lookup file */
	if ( IsNull(table_fp = fopen(table_file, "r")) )
		{
		(void) sprintf(err_buf, "Cannot find wind lookup table ... %s",
				table_file);
		(void) error_report(err_buf);
		}

	/* Read the lookup file line by line */
	if ( Verbose )
		{
		(void) fprintf(stdout, "  Reading wind lookup table ... %s\n",
				table_file);
		}
	while ( NotNull( getvalidline(table_fp, tbuf, (size_t) GPGLong, Comment) ) )
		{

		/* Skip blank lines */
		if ( blank(tbuf) ) continue;

		/* Branch to the appropriate sections */
		(void) strcpy(xbuf, tbuf);
		(void) safe_strcpy(wtype, string_arg(tbuf));
		(void) safe_strcpy(wcat,  string_arg(tbuf));
		(void) safe_strcpy(wcatx, string_arg(tbuf));

		/* Read the CALM sections */
		if ( same(wtype, WindCalm) )
			{

			/* Read the CALM "value" section */
			if ( same(wcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(WindLookTable->calm.wind_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the CALM "text" section */
			else if ( same(wcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(WindLookTable->calm.wind_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));
					}
				}

			/* Read the CALM "symbol" section */
			else if ( same(wcat, WVsubSymbol) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookSymbol = &(WindLookTable->calm.wind_sym);
					ncnt = WVLookSymbol->nsymbol++;
					WVLookSymbol->minval  = GETMEM(WVLookSymbol->minval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->maxval  = GETMEM(WVLookSymbol->maxval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->symbols = GETMEM(WVLookSymbol->symbols,
												STRING, WVLookSymbol->nsymbol);

					/* Set the parameter values */
					WVLookSymbol->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookSymbol->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookSymbol->symbols[ncnt] = safe_strdup(sym_file);
					}
				}
			}

		/* Read the DIRECTION sections */
		else if ( same(wtype, WindDirection) )
			{

			/* Read the DIRECTION "value" section */
			if ( same(wcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(WindLookTable->direction.wind_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the DIRECTION "text" section */
			else if ( same(wcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(WindLookTable->direction.wind_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));

					/* Ensure that parameter values cover an acceptable range */
					if ( WVLookText->minval[ncnt] < 0.0 )
						{
						WVLookText->minval[ncnt] += 360.0;
						}
					if ( WVLookText->maxval[ncnt] < WVLookText->minval[ncnt] )
						{
						WVLookText->maxval[ncnt] += 360.0;
						}
					}
				}

			/* Read the DIRECTION "symbol uniform" section */
			else if ( same(wcat, WVsubSymbol) && same(wcatx, WVsubUniform) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookUniform = &(WindLookTable->direction.wind_uni);
					ncnt = WVLookUniform->nsymbol++;
					WVLookUniform->minval  = GETMEM(WVLookUniform->minval,
												float, WVLookUniform->nsymbol);
					WVLookUniform->maxval  = GETMEM(WVLookUniform->maxval,
												float, WVLookUniform->nsymbol);
					WVLookUniform->symbols = GETMEM(WVLookUniform->symbols,
												STRING, WVLookUniform->nsymbol);
					WVLookUniform->rotate  = GETMEM(WVLookUniform->rotate,
												float, WVLookUniform->nsymbol);

					/* Set the parameter values */
					WVLookUniform->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookUniform->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookUniform->symbols[ncnt] = safe_strdup(sym_file);
					WVLookUniform->rotate[ncnt]  = float_arg(tbuf, &valid);

					/* Ensure that parameter values cover an acceptable range */
					if ( WVLookUniform->minval[ncnt] < 0.0 )
						{
						WVLookUniform->minval[ncnt] += 360.0;
						}
					if ( WVLookUniform->maxval[ncnt]
							< WVLookUniform->minval[ncnt] )
						{
						WVLookUniform->maxval[ncnt] += 360.0;
						}
					}
				}

			/* Read the DIRECTION "symbol proportional" section */
			else if ( same(wcat, WVsubSymbol)
					&& same(wcatx, WVsubProportional) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookProport = &(WindLookTable->direction.wind_pro);
					ncnt = WVLookProport->nsymbol++;
					WVLookProport->minval  = GETMEM(WVLookProport->minval,
												float, WVLookProport->nsymbol);
					WVLookProport->maxval  = GETMEM(WVLookProport->maxval,
												float, WVLookProport->nsymbol);
					WVLookProport->symbols = GETMEM(WVLookProport->symbols,
												STRING, WVLookProport->nsymbol);
					WVLookProport->maxscl  = GETMEM(WVLookProport->maxscl,
												float, WVLookProport->nsymbol);
					WVLookProport->minscl  = GETMEM(WVLookProport->minscl,
												float, WVLookProport->nsymbol);
					WVLookProport->rotate  = GETMEM(WVLookProport->rotate,
												float, WVLookProport->nsymbol);

					/* Set the parameter values */
					WVLookProport->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookProport->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookProport->symbols[ncnt] = safe_strdup(sym_file);
					WVLookProport->minscl[ncnt]  = float_arg(tbuf, &valid);
					WVLookProport->maxscl[ncnt]  = float_arg(tbuf, &valid);
					WVLookProport->rotate[ncnt]  = float_arg(tbuf, &valid);
					}
				}
			}

		/* Read the SPEED sections */
		else if ( same(wtype, WindSpeed) )
			{

			/* Read the SPEED "value" section */
			if ( same(wcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(WindLookTable->speed.wind_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the SPEED "text" section */
			else if ( same(wcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(WindLookTable->speed.wind_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));
					}
				}

			/* Read the SPEED "symbol" section */
			else if ( same(wcat, WVsubSymbol) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookSymbol = &(WindLookTable->speed.wind_sym);
					ncnt = WVLookSymbol->nsymbol++;
					WVLookSymbol->minval  = GETMEM(WVLookSymbol->minval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->maxval  = GETMEM(WVLookSymbol->maxval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->symbols = GETMEM(WVLookSymbol->symbols,
												STRING, WVLookSymbol->nsymbol);

					/* Set the parameter values */
					WVLookSymbol->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookSymbol->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookSymbol->symbols[ncnt] = safe_strdup(sym_file);
					}
				}
			}

		/* Read the GUST sections */
		else if ( same(wtype, WindGust) )
			{

			/* Read the GUST "value" section */
			if ( same(wcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(WindLookTable->gust.wind_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the GUST "text" section */
			else if ( same(wcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(WindLookTable->gust.wind_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));
					}
				}

			/* Read the GUST "symbol" section */
			else if ( same(wcat, WVsubSymbol) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookSymbol = &(WindLookTable->gust.wind_sym);
					ncnt = WVLookSymbol->nsymbol++;
					WVLookSymbol->minval  = GETMEM(WVLookSymbol->minval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->maxval  = GETMEM(WVLookSymbol->maxval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->symbols = GETMEM(WVLookSymbol->symbols,
												STRING, WVLookSymbol->nsymbol);

					/* Set the parameter values */
					WVLookSymbol->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookSymbol->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookSymbol->symbols[ncnt] = safe_strdup(sym_file);
					}
				}
			}
		}

	/* Close the wind lookup table and return the current pointer */
	(void) fclose(table_fp);
	return &WindLookups[ilook];
	}

/* >>> add warning messages for improper lines <<< */
/* >>> improper  vtype                         <<< */
/* >>> improper  vcat/vcatx  for given  vtype  <<< */

static	VECTOR_LOOKUP_TABLE	*get_vector_lookup_table

	(
	STRING		lookup	/* vector lookup table name */
	)

	{
	int					ilook, ncnt;
	LOGICAL				valid;
	STRING				table_file, sym_file;
	char				tbuf[GPGLong], xbuf[GPGLong];
	FILE				*table_fp;
	VECTOR_LOOKUP_TABLE	*VectorLookTable;
	WVLOOKUP_VALUE		*WVLookValue;
	WVLOOKUP_TEXT		*WVLookText;
	WVLOOKUP_SYMBOL		*WVLookSymbol;
	WVLOOKUP_UNIFORM	*WVLookUniform;
	WVLOOKUP_PROPORT	*WVLookProport;
	char				err_buf[GPGLong];

	/* Static buffers for vector type and category */
	static	char	vtype[GPGShort];
	static	char	vcat[GPGShort];
	static	char	vcatx[GPGShort];

	/* First find the full path of the lookup file */
	table_file = find_lookup_file(lookup, LookupVectorExt);
	if ( blank(table_file) ) return NullPtr(VECTOR_LOOKUP_TABLE *);

	/* Search the list for the lookup file name */
	for ( ilook=0; ilook<NumVectorLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(table_file, VectorLookups[ilook].label) )
			{
			return &VectorLookups[ilook];
			}
		}

	/* Otherwise, add another lookup table to the list */
	ilook           = NumVectorLookup++;
	VectorLookups   = GETMEM(VectorLookups, VECTOR_LOOKUP_TABLE,
															NumVectorLookup);
	VectorLookTable = &VectorLookups[ilook];

	/* Initialize the new lookup table */
	VectorLookTable->label = strdup(table_file);

	VectorLookTable->calm.vector_val.nvalue       = 0;
	VectorLookTable->calm.vector_val.minval       = NullFloat;
	VectorLookTable->calm.vector_val.maxval       = NullFloat;
	VectorLookTable->calm.vector_val.round        = NullFloat;
	VectorLookTable->calm.vector_val.factor       = NullInt;
	VectorLookTable->calm.vector_val.ndigit       = NullInt;
	VectorLookTable->calm.vector_txt.ntext        = 0;
	VectorLookTable->calm.vector_txt.minval       = NullFloat;
	VectorLookTable->calm.vector_txt.maxval       = NullFloat;
	VectorLookTable->calm.vector_txt.texts        = NullStringList;
	VectorLookTable->calm.vector_sym.nsymbol      = 0;
	VectorLookTable->calm.vector_sym.minval       = NullFloat;
	VectorLookTable->calm.vector_sym.maxval       = NullFloat;
	VectorLookTable->calm.vector_sym.symbols      = NullStringList;

	VectorLookTable->direction.vector_val.nvalue  = 0;
	VectorLookTable->direction.vector_val.minval  = NullFloat;
	VectorLookTable->direction.vector_val.maxval  = NullFloat;
	VectorLookTable->direction.vector_val.round   = NullFloat;
	VectorLookTable->direction.vector_val.factor  = NullInt;
	VectorLookTable->direction.vector_val.ndigit  = NullInt;
	VectorLookTable->direction.vector_txt.ntext   = 0;
	VectorLookTable->direction.vector_txt.minval  = NullFloat;
	VectorLookTable->direction.vector_txt.maxval  = NullFloat;
	VectorLookTable->direction.vector_txt.texts   = NullStringList;
	VectorLookTable->direction.vector_uni.nsymbol = 0;
	VectorLookTable->direction.vector_uni.minval  = NullFloat;
	VectorLookTable->direction.vector_uni.maxval  = NullFloat;
	VectorLookTable->direction.vector_uni.symbols = NullStringList;
	VectorLookTable->direction.vector_uni.rotate  = NullFloat;
	VectorLookTable->direction.vector_pro.nsymbol = 0;
	VectorLookTable->direction.vector_pro.minval  = NullFloat;
	VectorLookTable->direction.vector_pro.maxval  = NullFloat;
	VectorLookTable->direction.vector_pro.symbols = NullStringList;
	VectorLookTable->direction.vector_pro.minscl  = NullFloat;
	VectorLookTable->direction.vector_pro.maxscl  = NullFloat;
	VectorLookTable->direction.vector_pro.rotate  = NullFloat;

	VectorLookTable->speed.vector_val.nvalue      = 0;
	VectorLookTable->speed.vector_val.minval      = NullFloat;
	VectorLookTable->speed.vector_val.maxval      = NullFloat;
	VectorLookTable->speed.vector_val.round       = NullFloat;
	VectorLookTable->speed.vector_val.factor      = NullInt;
	VectorLookTable->speed.vector_val.ndigit      = NullInt;
	VectorLookTable->speed.vector_txt.ntext       = 0;
	VectorLookTable->speed.vector_txt.minval      = NullFloat;
	VectorLookTable->speed.vector_txt.maxval      = NullFloat;
	VectorLookTable->speed.vector_txt.texts       = NullStringList;
	VectorLookTable->speed.vector_sym.nsymbol     = 0;
	VectorLookTable->speed.vector_sym.minval      = NullFloat;
	VectorLookTable->speed.vector_sym.maxval      = NullFloat;
	VectorLookTable->speed.vector_sym.symbols     = NullStringList;

	if ( IsNull(table_fp = fopen(table_file, "r")) )
		{
		(void) sprintf(err_buf, "Cannot find vector lookup table ... %s",
				table_file);
		(void) error_report(err_buf);
		}

	/* Read the lookup file line by line */
	if ( Verbose )
		{
		(void) fprintf(stdout, "  Reading vector lookup table ... %s\n",
				table_file);
		}
	while ( NotNull( getvalidline(table_fp, tbuf, (size_t) GPGLong, Comment) ) )
		{

		/* Skip blank lines */
		if ( blank(tbuf) ) continue;

		/* Branch to the appropriate sections */
		(void) strcpy(xbuf, tbuf);
		(void) safe_strcpy(vtype, string_arg(tbuf));
		(void) safe_strcpy(vcat,  string_arg(tbuf));
		(void) safe_strcpy(vcatx, string_arg(tbuf));

		/* Read the CALM sections */
		if ( same(vtype, VectorCalm) )
			{

			/* Read the CALM "value" section */
			if ( same(vcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(VectorLookTable->calm.vector_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the CALM "text" section */
			else if ( same(vcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(VectorLookTable->calm.vector_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));
					}
				}

			/* Read the CALM "symbol" section */
			else if ( same(vcat, WVsubSymbol) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookSymbol = &(VectorLookTable->calm.vector_sym);
					ncnt = WVLookSymbol->nsymbol++;
					WVLookSymbol->minval  = GETMEM(WVLookSymbol->minval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->maxval  = GETMEM(WVLookSymbol->maxval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->symbols = GETMEM(WVLookSymbol->symbols,
												STRING, WVLookSymbol->nsymbol);

					/* Set the parameter values */
					WVLookSymbol->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookSymbol->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookSymbol->symbols[ncnt] = safe_strdup(sym_file);
					}
				}
			}

		/* Read the DIRECTION sections */
		else if ( same(vtype, VectorDirection) )
			{

			/* Read the DIRECTION "value" section */
			if ( same(vcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(VectorLookTable->direction.vector_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the DIRECTION "text" section */
			else if ( same(vcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(VectorLookTable->direction.vector_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));

					/* Ensure that parameter values cover an acceptable range */
					if ( WVLookText->minval[ncnt] < 0.0 )
						{
						WVLookText->minval[ncnt] += 360.0;
						}
					if ( WVLookText->maxval[ncnt] < WVLookText->minval[ncnt] )
						{
						WVLookText->maxval[ncnt] += 360.0;
						}
					}
				}

			/* Read the DIRECTION "symbol uniform" section */
			else if ( same(vcat, WVsubSymbol) && same(vcatx, WVsubUniform) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookUniform = &(VectorLookTable->direction.vector_uni);
					ncnt = WVLookUniform->nsymbol++;
					WVLookUniform->minval  = GETMEM(WVLookUniform->minval,
												float, WVLookUniform->nsymbol);
					WVLookUniform->maxval  = GETMEM(WVLookUniform->maxval,
												float, WVLookUniform->nsymbol);
					WVLookUniform->symbols = GETMEM(WVLookUniform->symbols,
												STRING, WVLookUniform->nsymbol);
					WVLookUniform->rotate  = GETMEM(WVLookUniform->rotate,
												float, WVLookUniform->nsymbol);

					/* Set the parameter values */
					WVLookUniform->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookUniform->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookUniform->symbols[ncnt] = safe_strdup(sym_file);
					WVLookUniform->rotate[ncnt]  = float_arg(tbuf, &valid);

					/* Ensure that parameter values cover an acceptable range */
					if ( WVLookUniform->minval[ncnt] < 0.0 )
						{
						WVLookUniform->minval[ncnt] += 360.0;
						}
					if ( WVLookUniform->maxval[ncnt]
							< WVLookUniform->minval[ncnt] )
						{
						WVLookUniform->maxval[ncnt] += 360.0;
						}
					}
				}

			/* Read the DIRECTION "symbol proportional" section */
			else if ( same(vcat, WVsubSymbol)
					&& same(vcatx, WVsubProportional) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookProport = &(VectorLookTable->direction.vector_pro);
					ncnt = WVLookProport->nsymbol++;
					WVLookProport->minval  = GETMEM(WVLookProport->minval,
												float, WVLookProport->nsymbol);
					WVLookProport->maxval  = GETMEM(WVLookProport->maxval,
												float, WVLookProport->nsymbol);
					WVLookProport->symbols = GETMEM(WVLookProport->symbols,
												STRING, WVLookProport->nsymbol);
					WVLookProport->maxscl  = GETMEM(WVLookProport->maxscl,
												float, WVLookProport->nsymbol);
					WVLookProport->minscl  = GETMEM(WVLookProport->minscl,
												float, WVLookProport->nsymbol);
					WVLookProport->rotate  = GETMEM(WVLookProport->rotate,
												float, WVLookProport->nsymbol);

					/* Set the parameter values */
					WVLookProport->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookProport->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookProport->symbols[ncnt] = safe_strdup(sym_file);
					WVLookProport->maxscl[ncnt]  = float_arg(tbuf, &valid);
					WVLookProport->minscl[ncnt]  = float_arg(tbuf, &valid);
					WVLookProport->rotate[ncnt]  = float_arg(tbuf, &valid);
					}
				}
			}

		/* Read the SPEED sections */
		else if ( same(vtype, VectorSpeed) )
			{

			/* Read the SPEED "value" section */
			if ( same(vcat, WVsubValue) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookValue = &(VectorLookTable->speed.vector_val);
					ncnt = WVLookValue->nvalue++;
					WVLookValue->minval = GETMEM(WVLookValue->minval,
												float, WVLookValue->nvalue);
					WVLookValue->maxval = GETMEM(WVLookValue->maxval,
												float, WVLookValue->nvalue);
					WVLookValue->round  = GETMEM(WVLookValue->round,
												float, WVLookValue->nvalue);
					WVLookValue->factor = GETMEM(WVLookValue->factor,
												int, WVLookValue->nvalue);
					WVLookValue->ndigit = GETMEM(WVLookValue->ndigit,
												int, WVLookValue->nvalue);

					/* Set the parameter values */
					WVLookValue->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookValue->round[ncnt]  = float_arg(tbuf, &valid);
					WVLookValue->factor[ncnt] = int_arg(tbuf, &valid);
					WVLookValue->ndigit[ncnt] = int_arg(tbuf, &valid);
					}
				}

			/* Read the SPEED "text" section */
			else if ( same(vcat, WVsubText) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookText = &(VectorLookTable->speed.vector_txt);
					ncnt = WVLookText->ntext++;
					WVLookText->minval = GETMEM(WVLookText->minval,
												float, WVLookText->ntext);
					WVLookText->maxval = GETMEM(WVLookText->maxval,
												float, WVLookText->ntext);
					WVLookText->texts  = GETMEM(WVLookText->texts,
												STRING, WVLookText->ntext);

					/* Set the parameter values */
					WVLookText->minval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->maxval[ncnt] = float_arg(tbuf, &valid);
					WVLookText->texts[ncnt]  = safe_strdup(string_arg(tbuf));
					}
				}

			/* Read the SPEED "symbol" section */
			else if ( same(vcat, WVsubSymbol) )
				{
				while ( NotNull( getvalidline(table_fp, tbuf,
												(size_t) GPGLong, Comment) ) )
					{

					/* Continue reading lines until WVsubEnd found */
					if ( same_start(tbuf, WVsubEnd) ) break;

					/* Add another parameter to the list */
					WVLookSymbol = &(VectorLookTable->speed.vector_sym);
					ncnt = WVLookSymbol->nsymbol++;
					WVLookSymbol->minval  = GETMEM(WVLookSymbol->minval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->maxval  = GETMEM(WVLookSymbol->maxval,
												float, WVLookSymbol->nsymbol);
					WVLookSymbol->symbols = GETMEM(WVLookSymbol->symbols,
												STRING, WVLookSymbol->nsymbol);

					/* Set the parameter values */
					WVLookSymbol->minval[ncnt]  = float_arg(tbuf, &valid);
					WVLookSymbol->maxval[ncnt]  = float_arg(tbuf, &valid);
					sym_file = find_symbol_file(string_arg(tbuf));
					WVLookSymbol->symbols[ncnt] = safe_strdup(sym_file);
					}
				}
			}
		}

	/* Close the vector lookup table and return the current pointer */
	(void) fclose(table_fp);
	return &VectorLookups[ilook];
	}

/***********************************************************************
*                                                                      *
*    g e t _ l o c a t i o n _ l o o k u p _ t a b l e                 *
*                                                                      *
***********************************************************************/

static	LOC_LOOKUP_TABLE	*get_location_lookup_table

	(
	STRING		lookup	/* location look up table name */
	)

	{
	int					ilook, iline;
	LOGICAL				valid;
	STRING				table_file;
	char				tbuf[GPGLong];
	char				*token;
	FILE				*table_fp;
	LOC_LOOKUP_TABLE	*LocLookTable;
	LOC_LOOKUP_LINE		*LocLookLine;
	char				err_buf[GPGLong];

	/* Static buffers for location parameters */
	static	char	ident[GPGMedium];
	static	char	lat[GPGMedium];
	static	char	lon[GPGMedium];
	static	char	vstring[GPGMedium];
	static	char	llab[GPGMedium];

	/* First search the list for internal lookup files */
	for ( ilook=0; ilook<NumLocLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(lookup, LocLookups[ilook].label) )
			{
			return &LocLookups[ilook];
			}
		}

	/* Next find the full path of the lookup file */
	table_file = find_lookup_file(lookup, LookupLocationExt);
	if ( blank(table_file) ) return NullPtr(LOC_LOOKUP_TABLE *);

	/* Search the list for the lookup file name */
	for ( ilook=0; ilook<NumLocLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(table_file, LocLookups[ilook].label) )
			{
			return &LocLookups[ilook];
			}
		}

	/* Otherwise, add another lookup table to the list */
	ilook        = NumLocLookup++;
	LocLookups   = GETMEM(LocLookups, LOC_LOOKUP_TABLE, NumLocLookup);
	LocLookTable = &LocLookups[ilook];

	/* Initialize the new lookup table */
	LocLookTable->label         = strdup(table_file);
	LocLookTable->numlines      = 0;
	LocLookTable->lines         = NullPtr(LOC_LOOKUP_LINE *);
	LocLookTable->isdef         = FALSE;
	LocLookTable->dline.ident   = NullString;
	LocLookTable->dline.lat     = NullString;
	LocLookTable->dline.lon     = NullString;
	LocLookTable->dline.vstring = NullString;
	LocLookTable->dline.llab    = NullString;

	/* Open the lookup file */
	if ( IsNull(table_fp = fopen(table_file, "r")) )
		{
		(void) sprintf(err_buf, "Cannot find location look up table ... %s",
				table_file);
		(void) error_report(err_buf);
		}

	/* Read the lookup file line by line */
	if ( Verbose )
		{
		(void) fprintf(stdout, "  Reading location look up table ... %s\n",
				table_file);
		}
	while ( NotNull( getvalidline(table_fp, tbuf, (size_t) GPGLong, Comment) ) )
		{

		/* Extract the location parameters */
		(void) safe_strcpy(ident,   string_arg(tbuf));
		(void) safe_strcpy(lat,     string_arg(tbuf));
		(void) safe_strcpy(lon,     string_arg(tbuf));

		/* Skip lines with missing parameters */
		if ( blank(ident) || blank(lat) || blank(lon) ) continue;

		/* Strip comments off the ends of lines */
		if ( NotNull( token = strpbrk(tbuf, Comment) ) )
			{
			*token = '\0';
			(void) no_white(tbuf);
			}

		/* Extract the optional valid time and label */
		(void) safe_strcpy(vstring, string_arg(tbuf));
		(void) safe_strcpy(llab,    string_arg(tbuf));

		/* Replace placeholders */
		if ( same(ident,   GPGplaceHolder) ) (void) strcpy(ident,   FpaCblank);
		if ( same(lat,     GPGplaceHolder) ) (void) strcpy(lat,     FpaCblank);
		if ( same(lon,     GPGplaceHolder) ) (void) strcpy(lon,     FpaCblank);
		if ( same(vstring, GPGplaceHolder) ) (void) strcpy(vstring, FpaCblank);
		if ( same(llab,    GPGplaceHolder) ) (void) strcpy(llab,    FpaCblank);

		/* Ensure that the parameters are correctly specified */
		if ( !blank(lat) )
			{
			(void) read_lat(lat, &valid);
			if ( !valid )
				{
				(void) sprintf(err_buf,
						"Invalid lat ... %s  in location look up table ... %s",
						lat, table_file);
				(void) error_report(err_buf);
				}
			}
		if ( !blank(lon) )
			{
			(void) read_lon(lon, &valid);
			if ( !valid )
				{
				(void) sprintf(err_buf,
						"Invalid lon ... %s  in location look up table ... %s",
						lon, table_file);
				(void) error_report(err_buf);
				}
			}
		if ( !blank(vstring) )
			{
			if ( blank(interpret_timestring(vstring, T0stamp, 0.0) ) )
				{
				(void) sprintf(err_buf,
						"Invalid valid_time ... %s  in location look up table ... %s",
						vstring, table_file);
				(void) error_report(err_buf);
				}
			}

		/* Add the location look up table parameters to the default list */
		if ( same(ident, "*default*") )
			{
			LocLookTable->isdef  = TRUE;
			LocLookLine          = &LocLookTable->dline;
			LocLookLine->ident       = safe_strdup(FpaCblank);
			LocLookLine->lat         = safe_strdup(lat);
			LocLookLine->lon         = safe_strdup(lon);
			LocLookLine->vstring     = safe_strdup(vstring);
			LocLookLine->llab        = safe_strdup(llab);
			}

		/* Add the location look up table parameters to the table list */
		else
			{
			iline                = LocLookTable->numlines++;
			LocLookTable->lines  = GETMEM(LocLookTable->lines,
											LOC_LOOKUP_LINE,
											LocLookTable->numlines);
			LocLookLine          = &LocLookTable->lines[iline];
			LocLookLine->ident       = safe_strdup(ident);
			LocLookLine->lat         = safe_strdup(lat);
			LocLookLine->lon         = safe_strdup(lon);
			LocLookLine->vstring     = safe_strdup(vstring);
			LocLookLine->llab        = safe_strdup(llab);
			}
		}

	/* Close the location look up table and return the current pointer */
	(void) fclose(table_fp);
	return &LocLookups[ilook];
	}

/***********************************************************************
*                                                                      *
*    g e t _ v e r t i c a l _ l o o k u p _ t a b l e                 *
*                                                                      *
***********************************************************************/

static	VERT_LOOKUP_TABLE	*get_vertical_lookup_table

	(
	STRING		lookup	/* vertical lookup table name */
	)

	{
	int					ilook, iline;
	STRING				table_file;
	float				yvalue, ylocation;
	LOGICAL				validv, validl;
	char				tbuf[GPGLong];
	char				*token;
	FILE				*table_fp;
	VERT_LOOKUP_TABLE	*VertLookTable;
	VERT_LOOKUP_LINE	*VertLookLine;
	char				err_buf[GPGLong];

	/* Static buffer for identifier */
	static	char	ident[GPGMedium];
	static	char	llab[GPGMedium];

	/* First find the full path of the lookup file */
	table_file = find_lookup_file(lookup, LookupVerticalExt);
	if ( blank(table_file) ) return NullPtr(VERT_LOOKUP_TABLE *);

	/* Search the list for the lookup file name */
	for ( ilook=0; ilook<NumVertLookup; ilook++ )
		{

		/* Return the lookup table from the list */
		if ( same(table_file, VertLookups[ilook].label) )
			{
			return &VertLookups[ilook];
			}
		}

	/* Otherwise, add another lookup table to the list */
	ilook         = NumVertLookup++;
	VertLookups   = GETMEM(VertLookups, VERT_LOOKUP_TABLE, NumVertLookup);
	VertLookTable = &VertLookups[ilook];

	/* Initialize the new lookup table */
	VertLookTable->label       = strdup(table_file);
	VertLookTable->numlines    = 0;
	VertLookTable->lines       = NullPtr(VERT_LOOKUP_LINE *);

	/* Open the lookup file */
	if ( IsNull(table_fp = fopen(table_file, "r")) )
		{
		(void) sprintf(err_buf, "Cannot find vertical lookup table ... %s",
				table_file);
		(void) error_report(err_buf);
		}

	/* Read the lookup file line by line */
	if ( Verbose )
		{
		(void) fprintf(stdout, "  Reading vertical lookup table ... %s\n",
				table_file);
		}
	while ( NotNull( getvalidline(table_fp, tbuf, (size_t) GPGLong, Comment) ) )
		{

		/* Extract the vertical parameters   */
		(void) safe_strcpy(ident, string_arg(tbuf));
		yvalue    = float_arg(tbuf, &validv);
		ylocation = float_arg(tbuf, &validl);

		/* Skip lines with missing parameters */
		if ( blank(ident) || !validv || !validl ) continue;

		/* Strip comments off the ends of lines */
		if ( NotNull( token = strpbrk(tbuf, Comment) ) )
			{
			*token = '\0';
			(void) no_white(tbuf);
			}

		/* Extract the optional label */
		(void) safe_strcpy(llab, string_arg(tbuf));

		/* Add the vertical lookup table parameters to the table list */
		iline                = VertLookTable->numlines++;
		VertLookTable->lines = GETMEM(VertLookTable->lines,
										VERT_LOOKUP_LINE,
										VertLookTable->numlines);
		VertLookLine         = &VertLookTable->lines[iline];
		VertLookLine->ident       = safe_strdup(ident);
		VertLookLine->yvalue      = yvalue;
		VertLookLine->ylocation   = ylocation;
		VertLookLine->llab        = safe_strdup(llab);
		}

	/* Close the vertical lookup table and return the current pointer */
	(void) fclose(table_fp);
	return &VertLookups[ilook];
	}

/***********************************************************************
*                                                                      *
*    g e t _ d a t a _ f i l e                                         *
*    p a r s e _ d a t a _ f o r m a t                                 *
*                                                                      *
***********************************************************************/

static	DATA_FILE			*get_data_file

	(
	STRING		dname,		/* data file name */
	STRING		delim		/* delimiter for data file parameters */
	)

	{
	int					idat, iline, ip, maxp;
	STRING				data_file;
	char				tbuf[GPGLong];
	char				*token;
	FILE				*data_fp;
	DATA_FILE			*CurDataFile;
	DATA_FILE_LINE		*CurDataLine;
	char				err_buf[GPGLong];

	/* First find the full path of the data file */
	data_file = find_data_file(dname);
	if ( blank(data_file) ) return NullPtr(DATA_FILE *);

	/* Search the list for the data file name */
	for ( idat=0; idat<NumDataFile; idat++ )
		{

		/* Return the data file from the list */
		if ( same(data_file, DataFiles[idat].label) )
			{
			return &DataFiles[idat];
			}
		}

	/* Otherwise, add another data file to the list */
	idat        = NumDataFile++;
	DataFiles   = GETMEM(DataFiles, DATA_FILE, NumDataFile);
	CurDataFile = &DataFiles[idat];

	/* Initialize the new data file */
	CurDataFile->label    = strdup(data_file);
	CurDataFile->numlines = 0;
	CurDataFile->lines    = NullPtr(DATA_FILE_LINE *);

	/* Open the data file */
	if ( IsNull(data_fp = fopen(data_file, "r")) )
		{
		(void) sprintf(err_buf, "Cannot find data file ... %s", data_file);
		(void) error_report(err_buf);
		}

	/* Read the data file line by line */
	if ( Verbose )
		{
		(void) fprintf(stdout, "  Reading data file ... %s\n", data_file);
		}
	while ( NotNull( getvalidline(data_fp, tbuf, (size_t) GPGLong, Comment) ) )
		{

		/* Strip comments off the ends of lines */
		if ( NotNull( token = strpbrk(tbuf, Comment) ) )
			{
			*token = '\0';
			(void) no_white(tbuf);
			}

		/* Skip blank lines */
		if ( blank(tbuf) ) continue;

		/* Add another line to the data file structure */
		iline               = CurDataFile->numlines++;
		CurDataFile->lines  = GETMEM(CurDataFile->lines,
										DATA_FILE_LINE, CurDataFile->numlines);
		CurDataLine         = &CurDataFile->lines[iline];
		CurDataLine->nparms = 0;
		CurDataLine->parms  = NullStringList;

		/* Read the parameters for the first line */
		if ( iline == 0 )
			{

			/* Keep count of parameters for space delimited parameters */
			if ( blank(delim) )
				{
				while ( !blank(tbuf) )
					{
					ip                 = CurDataLine->nparms++;
					CurDataLine->parms = GETMEM(CurDataLine->parms,
												STRING, CurDataLine->nparms);
					CurDataLine->parms[ip] = safe_strdup(string_arg(tbuf));
					}
				}

			/* Keep count of parameters for character delimited parameters */
			else
				{
				if ( NotNull(token = strtok(tbuf, delim)) )
					{
					ip                 = CurDataLine->nparms++;
					CurDataLine->parms = GETMEM(CurDataLine->parms,
												STRING, CurDataLine->nparms);
					(void) no_white(token);
					CurDataLine->parms[ip] = safe_strdup(token);
					}
				while ( NotNull(token = strtok(NULL, delim)) )
					{
					ip                 = CurDataLine->nparms++;
					CurDataLine->parms = GETMEM(CurDataLine->parms,
												STRING, CurDataLine->nparms);
					(void) no_white(token);
					CurDataLine->parms[ip] = safe_strdup(token);
					}
				}

			/* Set maximum number of parameters for checking */
			maxp = CurDataLine->nparms;
			}

		/* Read the parameters for subsequent lines */
		else
			{

			/* Keep count of parameters for space delimited parameters */
			if ( blank(delim) )
				{
				while ( !blank(tbuf) )
					{
					ip                 = CurDataLine->nparms++;
					CurDataLine->parms = GETMEM(CurDataLine->parms,
												STRING, CurDataLine->nparms);
					CurDataLine->parms[ip] = safe_strdup(string_arg(tbuf));
					}
				}

			/* Keep count of parameters for character delimited parameters */
			else
				{
				if ( NotNull(token = strtok(tbuf, delim)) )
					{
					ip                 = CurDataLine->nparms++;
					CurDataLine->parms = GETMEM(CurDataLine->parms,
												STRING, CurDataLine->nparms);
					(void) no_white(token);
					CurDataLine->parms[ip] = safe_strdup(token);
					}
				while ( NotNull(token = strtok(NULL, delim)) )
					{
					ip                 = CurDataLine->nparms++;
					CurDataLine->parms = GETMEM(CurDataLine->parms,
												STRING, CurDataLine->nparms);
					(void) no_white(token);
					CurDataLine->parms[ip] = safe_strdup(token);
					}
				}

			/* Check number of parameters */
			if ( CurDataLine->nparms != maxp )
				{
				(void) sprintf(err_buf,
						"Inconsistent parameters at line ... %d  of data file ... %s",
						CurDataFile->numlines, data_file);
				(void) error_report(err_buf);
				}
			}
		}

	/* >>>>> debug testing for get_data_file() <<<<< */
	if ( DebugMode )
		{
		(void) fprintf(stdout, "Data file: \"%s\"\n", data_file);
		for (iline=0; iline< CurDataFile->numlines; iline++)
			{
			(void) fprintf(stdout,
					"  \"%s\"", CurDataFile->lines[iline].parms[0]);
			for (ip=1; ip<maxp; ip++)
				{
				if (!blank(delim)) (void) fprintf(stdout, "%s", delim);
				(void) fprintf(stdout,
						" \"%s\"", CurDataFile->lines[iline].parms[ip]);
				}
			(void) fprintf(stdout, "\n");
			}
		}
	/* >>>>> debug testing for get_data_file() <<<<< */

	/* Close the data file and return the current pointer */
	(void) fclose(data_fp);
	return &DataFiles[idat];
	}

static	GPGdtype			parse_data_format

	(
	STRING		format,		/* format of data file */
	STRING		*delim		/* delimiter after next parameter */
	)

	{
	size_t				nc;
	STRING				ll;
	GPGdtype			ftype;
	char				err_buf[GPGLong];

	static	char		fdelim[1];

	/* Initialize output buffer */
	if ( NotNull(delim) ) *delim = FpaCblank;

	/* Read the format for the next recognized parameter */
	if ( same_start_ic(format, "-") )
		{
		nc    = strlen("-");
		ftype = GPG_None;
		}
	else if ( same_start_ic(format, "identifier") )
		{
		nc    = strlen("identifier");
		ftype = GPG_Identifier;
		}
	else if ( same_start_ic(format, "latitude") )
		{
		nc = strlen("latitude");
		ftype = GPG_Latitude;
		}
	else if ( same_start_ic(format, "longitude") )
		{
		nc    = strlen("longitude");
		ftype = GPG_Longitude;
		}
	else if ( same_start_ic(format, "timestamp") )
		{
		nc    = strlen("timestamp");
		ftype = GPG_TimeStamp;
		}
	else if ( same_start_ic(format, "label") )
		{
		nc    = strlen("label");
		ftype = GPG_Label;
		}
	else if ( same_start_ic(format, "value") )
		{
		nc    = strlen("value");
		ftype = GPG_Value;
		}
	else if ( same_start_ic(format, "units") )
		{
		nc    = strlen("units");
		ftype = GPG_Units;
		}
	else if ( same_start_ic(format, "wind_direction") )
		{
		nc    = strlen("wind_direction");
		ftype = GPG_WindDirection;
		}
	else if ( same_start_ic(format, "wind_speed") )
		{
		nc    = strlen("wind_speed");
		ftype = GPG_WindSpeed;
		}
	else if ( same_start_ic(format, "wind_gust") )
		{
		nc    = strlen("wind_gust");
		ftype = GPG_WindGust;
		}
	else if ( same_start_ic(format, "wind_units") )
		{
		nc    = strlen("wind_units");
		ftype = GPG_WindUnits;
		}
	else
		{
		(void) sprintf(err_buf, "Unknown data file format \"%s\"", format);
		(void) error_report(err_buf);
		}

	/* The following character will be the delimiter */
	ll = format + nc;
	if ( !blank(ll) )
		{
		(void) strncpy(fdelim, ll, 1);
		if ( NotNull(delim) ) *delim = fdelim;
		ll++;
		}

	/* Strip off the parameter and delimiter from the format string */
	if ( !blank(ll) ) (void) strcpy(format, ll);
	else              (void) strcpy(format, "");

	/* Return the parameter type */
	return ftype;
	}

/***********************************************************************
*                                                                      *
*    m a t c h _ w v l o o k _ v a l u e                               *
*    m a t c h _ w v l o o k _ t e x t                                 *
*    m a t c h _ w v l o o k _ s y m b o l                             *
*    m a t c h _ w v l o o k _ d i r v a l u e                         *
*    m a t c h _ w v l o o k _ d i r t e x t                           *
*    m a t c h _ w v l o o k _ d i r u n i f o r m                     *
*    m a t c h _ w v l o o k _ d i r p r o p o r t i o n a l           *
*    m a t c h _ w v l o o k _ g u s t v a l u e                       *
*                                                                      *
***********************************************************************/

static	LOGICAL				match_wvlookup_value

	(
	float			value,		/* value to match */
	WVLOOKUP_VALUE	*wvlook,	/* structure with parameters to match */
	STRING			*text		/* formatted value to return */
	)

	{
	int			nn;
	float		diff, val;

	/* Static buffers for formatted value to return */
	static	char	format[GPGShort];
	static	char	ftext[GPGShort];

	/* Initialize output buffer */
	if ( NotNull(text) ) *text = FpaCblank;

	/* Check the list for matching values */
	for ( nn=0; nn<wvlook->nvalue; nn++ )
		{
		if ( value >= wvlook->minval[nn] && value < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching value found */
	if ( nn >= wvlook->nvalue ) return FALSE;

	/* Round the value based on the matched parameters */
	if ( wvlook->round[nn] != 0.0 )
		{
		diff = (float) fmod((double) value, (double) wvlook->round[nn]);
		val  = value - diff;
		if      ( diff >=  wvlook->round[nn]/2.0 ) val += wvlook->round[nn];
		else if ( diff <= -wvlook->round[nn]/2.0 ) val -= wvlook->round[nn];
		}
	else
		{
		val = value;
		}

	/* Format the value based on the matched parameters */
	val /= pow(10.0, wvlook->factor[nn]);
	if ( wvlook->ndigit[nn] > 0 )
		{
		(void) sprintf(format, "%%.%dd", wvlook->ndigit[nn]);
		(void) sprintf(ftext, format, NINT(val));
		}
	else if ( wvlook->ndigit[nn] < 0 )
		{
		(void) sprintf(format, "%%.%df", -wvlook->ndigit[nn]);
		(void) sprintf(ftext, format, val);
		}
	else
		{
		(void) sprintf(ftext, "%d", NINT(val));
		}

	/* Return the formatted value */
	if ( NotNull(text) ) *text = ftext;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_text

	(
	float			value,		/* value to match */
	WVLOOKUP_TEXT	*wvlook,	/* structure with parameters to match */
	STRING			*text		/* matched text to return */
	)

	{
	int			nn;

	/* Initialize output buffer */
	if ( NotNull(text) ) *text = FpaCblank;

	/* Check the list for matching values */
	for ( nn=0; nn<wvlook->ntext; nn++ )
		{
		if ( value >= wvlook->minval[nn] && value < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching value found */
	if ( nn >= wvlook->ntext ) return FALSE;

	/* Return the matched text string */
	if ( NotNull(text) ) *text = wvlook->texts[nn];
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_symbol

	(
	float			value,		/* value to match */
	WVLOOKUP_SYMBOL	*wvlook,	/* structure with parameters to match */
	STRING			*symbol		/* matched symbol to return */
	)

	{
	int			nn;

	/* Initialize output buffer */
	if ( NotNull(symbol) ) *symbol = FpaCblank;

	/* Check the list for matching values */
	for ( nn=0; nn<wvlook->nsymbol; nn++ )
		{
		if ( value >= wvlook->minval[nn] && value < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching value found */
	if ( nn >= wvlook->nsymbol ) return FALSE;

	/* Return the matched symbol */
	if ( NotNull(symbol) ) *symbol = wvlook->symbols[nn];
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_dirvalue

	(
	float			dir,		/* wind direction */
	float			spd,		/* wind speed to match */
	WVLOOKUP_VALUE	*wvlook,	/* structure with parameters to match */
	STRING			*text		/* formatted value to return */
	)

	{
	int			nn;
	float		diff, val;

	/* Static buffers for formatted wind direction to return */
	static	char	format[GPGShort];
	static	char	ftext[GPGShort];

	/* Initialize output buffer */
	if ( NotNull(text) ) *text = FpaCblank;

	/* Check the list for matching wind speeds */
	for ( nn=0; nn<wvlook->nvalue; nn++ )
		{
		if ( spd >= wvlook->minval[nn] && spd < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching wind speed found */
	if ( nn >= wvlook->nvalue ) return FALSE;

	/* Round the wind direction based on the matched parameters */
	if ( wvlook->round[nn] != 0.0 )
		{
		diff = (float) fmod((double) dir, (double) wvlook->round[nn]);
		val  = dir - diff;
		if      ( diff >=  wvlook->round[nn]/2.0 ) val += wvlook->round[nn];
		else if ( diff <= -wvlook->round[nn]/2.0 ) val -= wvlook->round[nn];
		}
	else
		{
		val = dir;
		}

	/* Format the wind direction based on the matched parameters */
	val /= pow(10.0, wvlook->factor[nn]);
	if ( wvlook->ndigit[nn] > 0 )
		{
		(void) sprintf(format, "%%.%dd", wvlook->ndigit[nn]);
		(void) sprintf(ftext, format, NINT(val));
		}
	else if ( wvlook->ndigit[nn] < 0 )
		{
		(void) sprintf(format, "%%.%df", -wvlook->ndigit[nn]);
		(void) sprintf(ftext, format, val);
		}
	else
		{
		(void) sprintf(ftext, "%d", NINT(val));
		}

	/* Return the formatted wind direction */
	if ( NotNull(text) ) *text = ftext;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_dirtext

	(
	float			dir,		/* wind direction to match */
	WVLOOKUP_TEXT	*wvlook,	/* structure with parameters to match */
	STRING			*text		/* matched text to return */
	)

	{
	int			nn;
	float		dirp;

	/* Initialize output buffer */
	if ( NotNull(text) ) *text = FpaCblank;

	/* Check the list for matching wind directions */
	dirp = dir + 360.0;
	for ( nn=0; nn<wvlook->ntext; nn++ )
		{
		if ( dir  >= wvlook->minval[nn] && dir  < wvlook->maxval[nn] ) break;
		if ( dirp >= wvlook->minval[nn] && dirp < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching wind direction found */
	if ( nn >= wvlook->ntext ) return FALSE;

	/* Return the matched text string */
	if ( NotNull(text) ) *text = wvlook->texts[nn];
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_diruniform

	(
	float				dir,		/* wind direction to match */
	float				flat,		/* latitude for wind to match */
	float				flon,		/* longitude for wind to match */
	WVLOOKUP_UNIFORM	*wvlook,	/* structure with parameters to match */
	STRING				*symbol,	/* matched symbol to return */
	float				*sym_rotate	/* rotation for matching symbol */
	)

	{
	int			nn;
	float		dirp, cart_dir;

	/* Initialize output buffers */
	if ( NotNull(symbol) )     *symbol     = FpaCblank;
	if ( NotNull(sym_rotate) ) *sym_rotate = 0.0;

	/* Check the list for matching wind directions */
	dirp = dir + 360.0;
	for ( nn=0; nn<wvlook->nsymbol; nn++ )
		{
		if ( dir  >= wvlook->minval[nn] && dir  < wvlook->maxval[nn] ) break;
		if ( dirp >= wvlook->minval[nn] && dirp < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching wind direction found */
	if ( nn >= wvlook->nsymbol ) return FALSE;

	/* Set the symbol rotation based on the matched wind direction */
	if ( wvlook->rotate[nn] >= 0.0 )
		{
		if ( AnchorToMap )
			cart_dir = wind_dir_xy(&BaseMap, flat, flon, wvlook->rotate[nn]);
		else
			cart_dir = 90.0 - wvlook->rotate[nn];
		}

	/* Set the symbol rotation based on the actual wind direction */
	else
		{
		if ( AnchorToMap )
			cart_dir = wind_dir_xy(&BaseMap, flat, flon, dir);
		else
			cart_dir = 90.0 - dir;
		}

	/* Return the matched symbol and rotation */
	if ( NotNull(symbol) )     *symbol     = wvlook->symbols[nn];
	if ( NotNull(sym_rotate) ) *sym_rotate = cart_dir;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_dirproportional

	(
	float				dir,		/* wind direction */
	float				spd,		/* wind speed to match */
	float				flat,		/* latitude for wind to match */
	float				flon,		/* longitude for wind to match */
	WVLOOKUP_PROPORT	*wvlook,	/* structure with parameters to match */
	STRING				*symbol,	/* matched symbol to return */
	float				*sym_scale,	/* scale for matching symbol */
	float				*sym_rotate	/* rotation for matching symbol */
	)

	{
	int			nn;
	float		minval, maxval, minscl, maxscl, scaling, cart_dir;

	/* Initialize output buffers */
	if ( NotNull(symbol) )     *symbol     = FpaCblank;
	if ( NotNull(sym_scale) )  *sym_scale  = 100.0;
	if ( NotNull(sym_rotate) ) *sym_rotate =   0.0;

	/* Check the list for matching wind speeds */
	for ( nn=0; nn<wvlook->nsymbol; nn++ )
		{
		if ( spd >= wvlook->minval[nn] && spd < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching wind speed found */
	if ( nn >= wvlook->nsymbol ) return FALSE;

	/* Scale the symbol proportional to the wind speed */
	minval   = wvlook->minval[nn];
	maxval   = wvlook->maxval[nn];
	minscl   = wvlook->minscl[nn];
	maxscl   = wvlook->maxscl[nn];
	scaling  = minscl + ((maxscl-minscl) * (spd-minval) / (maxval-minval));
	if ( AnchorToMap )
		{
		scaling *= DisplayUnits.sfactor;
		}

	/* Set the symbol rotation based on the actual wind direction */
	if ( AnchorToMap )
		cart_dir = wind_dir_xy(&BaseMap, flat, flon, dir);
	else
		cart_dir = 90.0 - dir;

	/* Return the matched symbol, scale and rotation */
	if ( NotNull(symbol) )     *symbol     = wvlook->symbols[nn];
	if ( NotNull(sym_scale) )  *sym_scale  = scaling;
	if ( NotNull(sym_rotate) ) *sym_rotate = cart_dir;
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL				match_wvlookup_gustvalue

	(
	float			gst,		/* wind gust to match */
	WVLOOKUP_VALUE	*wvlook,	/* structure with parameters to match */
	STRING			*text		/* formatted wind gust to return */
	)

	{
	int			nn;
	float		diff, val;

	/* Static buffers for formatted wind gust to return */
	static	char	format[GPGShort];
	static	char	ftext[GPGShort];

	/* Initialize output buffer */
	if ( NotNull(text) ) *text = FpaCblank;

	/* Check the list for matching wind gusts */
	for ( nn=0; nn<wvlook->nvalue; nn++ )
		{
		if ( gst >= wvlook->minval[nn] && gst < wvlook->maxval[nn] ) break;
		}

	/* Return FALSE if no matching wind gust found */
	if ( nn >= wvlook->nvalue ) return FALSE;

	/* Round the wind gust based on the matched parameters */
	if ( wvlook->round[nn] != 0.0 )
		{
		diff = (float) fmod((double) gst, (double) wvlook->round[nn]);
		val  = gst - diff;
		if      ( diff >=  wvlook->round[nn]/2.0 ) val += wvlook->round[nn];
		else if ( diff <= -wvlook->round[nn]/2.0 ) val -= wvlook->round[nn];
		}
	else
		{
		val = gst;
		}

	/* Format the wind gust based on the matched parameters     */
	/* Note that all wind gusts are preceded by a "G" character */
	val /= pow(10.0, wvlook->factor[nn]);
	if ( wvlook->ndigit[nn] > 0 )
		{
		(void) sprintf(format, "G%%.%dd", wvlook->ndigit[nn]);
		(void) sprintf(ftext, format, NINT(val));
		}
	else if ( wvlook->ndigit[nn] < 0 )
		{
		(void) sprintf(format, "G%%.%df", -wvlook->ndigit[nn]);
		(void) sprintf(ftext, format, val);
		}
	else
		{
		(void) sprintf(ftext, "G%d", NINT(val));
		}

	/* Return the formatted wind gust */
	if ( NotNull(text) ) *text = ftext;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*    f i n d _ p a t t e r n                                           *
*    r e a d _ p a t t e r n                                           *
*                                                                      *
* >>> Note that this code is adapted from graphics/pattern_line.c <<<  *
*                                                                      *
***********************************************************************/

/* Storage for named patterns */
static	int			NumPatterns  = 0;
static	PATTERN		*PatternList = NullPtr(PATTERN *);

static	PATTERN		*find_pattern

	(
	STRING		name	/* pattern name */
	)

	{
	int			ip;

	/* Look for pattern in current list */
	for ( ip=0; ip<NumPatterns; ip++ )
		{
		if ( same(PatternList[ip].name, name) ) return &PatternList[ip];
		}

	/* Read the pattern if not yet encountered */
	return read_pattern(name);
	}

/**********************************************************************/

static	PATTERN		*read_pattern

	(
	STRING		name	/* pattern name */
	)

	{
	METAFILE	meta;
	FIELD		fld;
	int			ifld, ilist, np, ic;
	SET			set;

	STRING		type;
	int			ncspec;
	CATSPEC		*cspecs;
	ITEM		item;
	LOGICAL		contig;
	float		width, length, lscale, wscale;

	float		xb, yb, xe, ye, xlen;
	CURVE		pcurve = NullCurve;
	LINE		pline;

	/* Try to read the metafile from the "patterns" directory */
	meta  = read_metafile(get_file("patterns", env_sub(name)), NullMapProj);

	/* Return if pattern cannot be found */
	if ( IsNull(meta) ) return NullPtr(PATTERN *);

	/* Set scaling */
	width  = meta->mproj.definition.ylen;
	length = meta->mproj.definition.xlen;
	if ( width  <= 0 ) width  = 100;
	if ( length <= 0 ) length = 100;
	wscale = 1.0 / width;
	lscale = 1.0 / length;

	/* Expand the pattern list */
	NumPatterns++;
	PatternList = GETMEM(PatternList, PATTERN, NumPatterns);
	np = NumPatterns - 1;
	PatternList[np].name   = safe_strdup(name);
	PatternList[np].num    = 0;
	PatternList[np].list   = NullItemPtr;
	PatternList[np].type   = NullStringList;
	PatternList[np].contig = NullLogicalList;

	/* Extract the components */
	for ( ifld=0; ifld<meta->numfld; ifld++ )
		{

		fld = meta->fields[ifld];
		if ( IsNull(fld) ) continue;

		/* Use the item list from set fields */
		if ( fld->ftype == FtypeSet )
			{

			set = fld->data.set;
			if ( IsNull(set) )   continue;
			if ( set->num <= 0 ) continue;

			/* Expand the pattern item list */
			ic = PatternList[np].num;
			PatternList[np].num   += set->num;
			PatternList[np].list   = GETMEM(PatternList[np].list, ITEM,
														PatternList[np].num);
			PatternList[np].type   = GETMEM(PatternList[np].type, STRING,
														PatternList[np].num);
			PatternList[np].contig = GETMEM(PatternList[np].contig, LOGICAL,
														PatternList[np].num);

			/* Add the items from the SET Object to the list */
			type   = set->type;
			ncspec = set->ncspec;
			cspecs = set->cspecs;
			for ( ilist=0; ilist<set->num; ilist++, ic++ )
				{

				/* Get the pattern */
				item   = copy_item(type, set->list[ilist]);
				contig = FALSE;

				/* Determine if a "curve" type pattern is contiguous        */
				/* Pattern is contiguous if it covers the full length       */
				/*  of the metafile, and begins and ends at the same height */
				if ( same(type, "curve") )
					{
					pcurve = (CURVE) item;
					pline  = pcurve->line;
					xb     = pline->points[0][X];
					yb     = pline->points[0][Y];
					xe     = pline->points[pline->numpts-1][X];
					ye     = pline->points[pline->numpts-1][Y];
					xlen   = (float) fabs((double) (xe - xb));
					if ( xlen == length && ye == yb ) contig = TRUE;
					}

				/* Scale the pattern */
				(void) scale_item(type, item, lscale, wscale);
				(void) invoke_item_catspec(type, item, ncspec, cspecs);

				/* Add the pattern to the list */
				PatternList[np].list[ic]   = item;
				PatternList[np].type[ic]   = safe_strdup(type);
				PatternList[np].contig[ic] = contig;
				}
			}
		}

	/* Clean up and return the pattern info */
	meta = destroy_metafile(meta);
	return &PatternList[np];
	}
