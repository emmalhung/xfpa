/*********************************************************************/
/** @file pspec.h
 *
 * Presentation spec definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    p s p e c . h                                                     *
*                                                                      *
*    Presentation spec definitions (include file)                      *
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
#ifndef PSPEC_DEFS
#define PSPEC_DEFS

/* Set debug parameter (if required) */
#undef DEBUG_PSPEC

/* Need other objects */
#include "misc.h"

/* Define assorted types */
typedef	short	COLOUR;
typedef	short	LSTYLE;
typedef	short	FSTYLE;
typedef	short	MTYPE;
typedef	short	FONT;
typedef	short	HILITE;
typedef	enum	{ BarbNone=0, BarbWind='w', BarbArrow='a' } BTYPE;
typedef	enum	{ Hr='r', Hc='c', Hl='l' }                  HJUST;
typedef	enum	{ VT='T', Vt='t', Vc='c', Vb='b', VB='B' }  VJUST;

#undef GLOBAL
#ifdef PSPEC_INIT
#	define GLOBAL GLOBAL_INIT
#else
#	define GLOBAL GLOBAL_EXTERN
#endif

/* Values used to leave the current value alone */
GLOBAL(const COLOUR,	SkipColour,		-99);
GLOBAL(const COLOUR,	SkipShadow,		-99);
GLOBAL(const LSTYLE,	SkipLstyle,		-99);
GLOBAL(const FSTYLE,	SkipFstyle,		-99);
GLOBAL(const MTYPE,		SkipMtype,		-99);
GLOBAL(const FONT,		SkipFont,		-99);
GLOBAL(const HILITE,	SkipHilite,		-99);
GLOBAL(const BTYPE,		SkipBtype,		-99);
GLOBAL(const HJUST,		SkipHjust,		-99);
GLOBAL(const VJUST,		SkipVjust,		-99);
GLOBAL(const LOGICAL,	SkipSense,		-99);
GLOBAL(const LOGICAL,	SkipValue,		-99);
GLOBAL(const LOGICAL,	SkipScale,		-99);
GLOBAL(const LOGICAL,	SkipCross,		-99);
GLOBAL(const float,		SkipWidth,		-99);
GLOBAL(const float,		SkipLength,		-99);
GLOBAL(const float,		SkipSpace,		-99);
GLOBAL(const float,		SkipTsize,		-99);
GLOBAL(const float,		SkipMsize,		-99);
GLOBAL(const float,		SkipAngle,		-99);
GLOBAL(const float,		SkipVoff,		-99);
GLOBAL(const STRING,	SkipPattern,	0);
GLOBAL(const STRING,	SkipSymbol,		0);
GLOBAL(const STRING,	SkipUname,		0);
GLOBAL(const double,	SkipUfactor,	-99);
GLOBAL(const double,	SkipUoffset,	-99);

/* Appropriate default values */
GLOBAL(const COLOUR,	SafeColour,		0);
GLOBAL(const COLOUR,	SafeShadow,		-1);
GLOBAL(const LSTYLE,	SafeLstyle,		0);
GLOBAL(const FSTYLE,	SafeFstyle,		0);
GLOBAL(const MTYPE,		SafeMtype,		0);
GLOBAL(const FONT,		SafeFont,		4);
GLOBAL(const HILITE,	SafeHilite,		0);
GLOBAL(const BTYPE,		SafeBtype,		BarbWind);
GLOBAL(const HJUST,		SafeHjust,		Hc);
GLOBAL(const VJUST,		SafeVjust,		Vc);
GLOBAL(const LOGICAL,	SafeSense,		False);
GLOBAL(const LOGICAL,	SafeValue,		False);
GLOBAL(const LOGICAL,	SafeScale,		False);
GLOBAL(const LOGICAL,	SafeCross,		False);
GLOBAL(const float,		SafeWidth,		1.0);
GLOBAL(const float,		SafeLength,		10.0);
GLOBAL(const float,		SafeSpace,		1.0);
GLOBAL(const float,		SafeTsize,		1.0);
GLOBAL(const float,		SafeMsize,		1.0);
GLOBAL(const float,		SafeAngle,		0.0);
GLOBAL(const float,		SafeVoff,		0.0);
GLOBAL(const STRING,	SafePattern,	0);
GLOBAL(const STRING,	SafeSymbol,		0);
GLOBAL(const STRING,	SafeUname,		0);
GLOBAL(const double,	SafeUfactor,	1.0);
GLOBAL(const double,	SafeUoffset,	0.0);


typedef	void	*PSPEC;
typedef	enum	{ PSPEC_LINE, PSPEC_FILL, PSPEC_TEXT, PSPEC_MARK, PSPEC_BARB }
				PTYPE;
typedef	enum	{
				/* End of attributes */
				PP_END,

				/* Line attributes */
				LINE_COLOUR, LINE_STYLE, LINE_PATTERN, LINE_SCALE, LINE_WIDTH,
				LINE_LENGTH, LINE_SENSE, LINE_HILITE,

				/* Fill attributes */
				FILL_COLOUR, FILL_STYLE, FILL_PATTERN, FILL_CROSS, FILL_SCALE,
				FILL_SPACE, FILL_ANGLE, FILL_HILITE,

				/* Text attributes */
				TEXT_COLOUR, TEXT_TCOLOUR, TEXT_BCOLOUR, TEXT_FONT, TEXT_SCALE,
				TEXT_SIZE, TEXT_ANGLE, TEXT_XOFF, TEXT_YOFF, TEXT_HJUST,
				TEXT_VJUST, TEXT_HILITE,

				/* Mark attributes */
				MARK_COLOUR, MARK_TCOLOUR, MARK_BCOLOUR, MARK_TYPE, MARK_SYMBOL,
				MARK_FONT, MARK_SCALE, MARK_SIZE, MARK_ANGLE, MARK_XOFF,
				MARK_YOFF, MARK_HJUST, MARK_VJUST, MARK_HILITE,

				/* Barb attributes */
				BARB_COLOUR, BARB_TYPE, BARB_SCALE, BARB_WIDTH, BARB_LENGTH,
				BARB_ANGLE, BARB_XOFF, BARB_YOFF, BARB_SENSE, BARB_VALUE,
				BARB_XVOFF, BARB_YVOFF, BARB_UNAME, BARB_HILITE,

				/* Unit attributes */
				UNIT_NAME, UNIT_FACTOR, UNIT_OFFSET
				} PPARAM;

/** Define line presentation spec */
typedef	struct
		{
		COLOUR	colour;		/**< line/perimeter colour */
		LSTYLE	style;		/**< line/perimeter style */
		STRING	pattern;	/**< line/perimeter pattern name */
		LOGICAL	scale;		/**< scale to map (True) or VDC (False) */
		float	width;		/**< line/perimeter width / pattern amplitude */
		float	length;		/**< pattern repeat length / barb length */
		HILITE	hilite;		/**< line/perimeter hilite mode */
		} LSPEC;

/** Define fill presentation spec */
typedef	struct
		{
		COLOUR	colour;		/**< fill colour */
		FSTYLE	style;		/**< fill style */
		STRING	pattern;	/**< fill pattern for certain styles */
		LOGICAL	cross;		/**< use cross-hatch (True) or parallel (False) */
		LOGICAL	scale;		/**< scale to map (True) or VDC (False) */
		float	space;		/**< hatch spacing */
		float	angle;		/**< hatch orientation */
		HILITE	hilite;		/**< fill hilite mode */
		} FSPEC;

/** Define text presentation spec */
typedef	struct
		{
		COLOUR	colour;		/**< text colour */
		COLOUR	tcolour;	/**< text top shadow colour (negative ignored) */
		COLOUR	bcolour;	/**< text bottom shadow colour (negative ignored) */
		FONT	font;		/**< text font name */
		LOGICAL	scale;		/**< scale to map (True) or VDC (False) */
		float	size;		/**< text size */
		float	angle;		/**< text relative orientation */
		float	xoff;		/**< text x-offset */
		float	yoff;		/**< text y-offset */
		HJUST	hjust;		/**< text horizontal justification code */
		VJUST	vjust;		/**< text vertical justification code */
		HILITE	hilite;		/**< text hilite mode */
		} TSPEC;

/** Define mark presentation spec */
typedef	struct
		{
		COLOUR	colour;		/**< mark colour */
		COLOUR	tcolour;	/**< mark top shadow colour (negative ignored) */
		COLOUR	bcolour;	/**< mark bottom shadow colour (negative ignored) */
		MTYPE	type;		/**< mark type  (negative means use symbol) */
		STRING	symbol;		/**< mark alphanumeric symbol */
		FONT	font;		/**< symbol font */
		LOGICAL	scale;		/**< scale to map (True) or VDC (False) */
		float	size;		/**< mark size */
		float	angle;		/**< mark relative orientation */
		float	xoff;		/**< mark x-offset */
		float	yoff;		/**< mark y-offset */
		HJUST	hjust;		/**< symbol horizontal justification code */
		VJUST	vjust;		/**< symbol vertical justification code */
		HILITE	hilite;		/**< mark hilite mode */
		} MSPEC;

/** Define barb/arrow presentation spec */
typedef	struct
		{
		COLOUR	colour;		/**< barb colour */
		BTYPE	type;		/**< barb type (negative means label only) */
		LOGICAL	scale;		/**< scale to map (True) or VDC (False) */
		float	width;		/**< barb line width */
		float	length;		/**< barb length */
		float	angle;		/**< barb relative orientation */
		float	xoff;		/**< barb x-offset */
		float	yoff;		/**< barb y-offset */
		LOGICAL	sense;		/**< direction towards (True) or from (False) */
		LOGICAL	value;		/**< include a value (True) or not (False) */
		float	xvoff;		/**< x-offset for value */
		float	yvoff;		/**< y-offset for value */
		STRING	uname;		/**< units label */
		HILITE	hilite;		/**< barb hilite mode */
		} BSPEC;

/** Define units spec for continuous fields and barbs/arrows */
typedef	struct
		{
		STRING	name;		/**< units label */
		double	factor;		/**< scale factor */
		double	offset;		/**< offset (added after scaling) */
		} USPEC;

/** Define contour spec for continuous fields */
typedef struct
		{
		LSPEC	lspec;		/**< presentation spec for contour lines */
		FSPEC	fspec;		/**< presentation spec for filled items */
		TSPEC	tspec;		/**< presentation spec for contour labels */
		BSPEC	bspec;		/**< presentation spec for vectors */
		MSPEC	mspec;		/**< presentation spec for max/min/saddle marks */
		STRING	type;		/**< contour spec type */
		int		vmult;		/**< multiplier for vector locations */
		float	cmin;		/**< minimum contour value to show */
		float	cmax;		/**< maximum contour value to show */
		float	cstd;		/**< starting contour value for regular interval */
		float	cint;		/**< regular interval between contour values */
		short	nval;		/**< number of explicit contour values */
		float	*cvals;		/**< list of explicit contour values */
		STRING	*clabs;		/**< optional explicit label override */
		} CONSPEC;

/** Define category spec for discrete and scattered fields */
typedef struct
		{
		LSPEC	lspec;		/**< presentation spec for lines and  boundaries */
		TSPEC	tspec;		/**< presentation spec for labels */
		FSPEC	fspec;		/**< presentation spec for filled areas */
		MSPEC	mspec;		/**< presentation spec for markers */
		BSPEC	bspec;		/**< presentation spec for barbs */
		STRING	mclass;		/**< member class (for label types) */
		STRING	name;		/**< member name (for label types) */
		STRING	type;		/**< member type (for label types) */
		STRING	cat;		/**< member category type */
		STRING	val;		/**< member category value */
		STRING	attrib;		/**< member attribute to use */
		POINT	offset;		/**< member offset */
		float	angle;		/**< member orientation */
		} CATSPEC;

/** Define plot spec for plot fields */
typedef	struct
		{
		LSPEC	lspec;		/**< presentation spec for lines and boundaries */
		TSPEC	tspec;		/**< presentation spec for labels */
		FSPEC	fspec;		/**< presentation spec for filled areas */
		MSPEC	mspec;		/**< presentation spec for markers */
		BSPEC	bspec;		/**< presentation spec for barbs */
		STRING	type;		/**< subfield type */
		STRING	name;		/**< subfield name */
		POINT	offset;		/**< subfield offset */
		float	angle;		/**< subfield orientation */
		} PLTSPEC;

/* Provide a few constants */
#define MKS_INIT { "MKS", 1.0, 0.0 }
GLOBAL(const USPEC,	MKS_UNITS, MKS_INIT);

/* Declare functions in pspec_disp.c */
void	init_pspec(PTYPE ptpe, PSPEC pspec);
void	free_pspec(PTYPE ptpe, PSPEC pspec);
void	copy_pspec(PTYPE ptpe, PSPEC pnew, const PSPEC pspec);
void	define_pspec_value(PSPEC pspec, PPARAM param, const POINTER value);
void	recall_pspec_value(PSPEC pspec, PPARAM param, POINTER value);

void	init_lspec(LSPEC *lspec);
void	skip_lspec(LSPEC *lspec);
void	free_lspec(LSPEC *lspec);
void	copy_lspec(LSPEC *lnew, const LSPEC *lspec);
void	string_lspec(LSPEC *lspec, STRING string);
void	define_lspec(LSPEC *lspec, COLOUR colour, LSTYLE style,
						STRING pattern, LOGICAL scale,
						float width, float length, HILITE hilite);
void	recall_lspec(LSPEC *lspec, COLOUR *colour, LSTYLE *style,
						STRING *pattern, LOGICAL *scale,
						float *width, float *length, HILITE *hilite);
void	define_lspec_value(LSPEC *lspec, PPARAM param, const POINTER value);
void	recall_lspec_value(LSPEC *lspec, PPARAM param, POINTER value);

void	init_fspec(FSPEC *fspec);
void	skip_fspec(FSPEC *fspec);
void	free_fspec(FSPEC *fspec);
void	copy_fspec(FSPEC *fnew, const FSPEC *fspec);
void	string_fspec(FSPEC *fspec, STRING string);
void	define_fspec(FSPEC *fspec, COLOUR colour, FSTYLE style,
						STRING pattern, LOGICAL cross, LOGICAL scale,
						float space, float angle, HILITE hilite);
void	recall_fspec(FSPEC *fspec, COLOUR *colour, FSTYLE *style,
						STRING *pattern, LOGICAL *cross, LOGICAL *scale,
						float *space, float *angle, HILITE *hilite);
void	define_fspec_value(FSPEC *fspec, PPARAM param, const POINTER value);
void	recall_fspec_value(FSPEC *fspec, PPARAM param, POINTER value);

void	init_tspec(TSPEC *tspec);
void	skip_tspec(TSPEC *tspec);
void	free_tspec(TSPEC *tspec);
void	copy_tspec(TSPEC *tnew, const TSPEC *tspec);
void	string_tspec(TSPEC *tspec, STRING string);
void	define_tspec(TSPEC *tspec, COLOUR colour,
						FONT font, LOGICAL scale, float size, float angle,
						HJUST hjust, VJUST vjust, HILITE hilite);
void	recall_tspec(TSPEC *tspec, COLOUR *colour,
						FONT *font, LOGICAL *scale, float *size, float *angle,
						HJUST *hjust, VJUST *vjust, HILITE *hilite);
void	define_tspec_value(TSPEC *tspec, PPARAM param, const POINTER value);
void	recall_tspec_value(TSPEC *tspec, PPARAM param, POINTER value);

void	init_mspec(MSPEC *mspec);
void	skip_mspec(MSPEC *mspec);
void	free_mspec(MSPEC *mspec);
void	copy_mspec(MSPEC *mnew, const MSPEC *mspec);
void	string_mspec(MSPEC *mspec, STRING string);
void	define_mspec(MSPEC *mspec, COLOUR colour, MTYPE type,
						STRING symbol, FONT font, LOGICAL scale,
						float size, float angle, HILITE hilite);
void	recall_mspec(MSPEC *mspec, COLOUR *colour, MTYPE *type,
						STRING *symbol, FONT *font, LOGICAL *scale,
						float *size, float *angle, HILITE *hilite);
void	define_mspec_value(MSPEC *mspec, PPARAM param, const POINTER value);
void	recall_mspec_value(MSPEC *mspec, PPARAM param, POINTER value);

void	init_bspec(BSPEC *bspec);
void	skip_bspec(BSPEC *bspec);
void	free_bspec(BSPEC *bspec);
void	copy_bspec(BSPEC *bnew, const BSPEC *bspec);
void	string_bspec(BSPEC *bspec, STRING string);
void	define_bspec(BSPEC *bspec, COLOUR colour, BTYPE type, LOGICAL scale,
						float width, float length, LOGICAL sense,
						LOGICAL value, float xvoff, float yvoff,
						STRING uname, HILITE hilite);
void	recall_bspec(BSPEC *bspec, COLOUR *colour, BTYPE *type, LOGICAL *scale,
						float *width, float *length, LOGICAL *sense,
						LOGICAL *value, float *xvoff, float *yvoff,
						STRING *uname, HILITE *hilite);
void	define_bspec_value(BSPEC *bspec, PPARAM param, const POINTER value);
void	recall_bspec_value(BSPEC *bspec, PPARAM param, POINTER value);

void	init_conspec(CONSPEC *cspec);
void	free_conspec(CONSPEC *cspec);
void	copy_conspec(CONSPEC *cnew, const CONSPEC *cspec);
void	define_conspec_range(CONSPEC *cspec,
						float cmin, float cmax, float cbase, float cinterval);
void	define_conspec_list(CONSPEC *cspec,
						int nval, const float *cvals, const STRING *clabs);
void	define_conspec_vector(CONSPEC *cspec, int vmult);
void	define_conspec_special(CONSPEC *cspec,
						STRING type, float cmin, float cmax);
void	add_cval_to_conspec(CONSPEC *cspec, float cval, STRING clab);

void	init_catspec(CATSPEC *cspec);
void	free_catspec(CATSPEC *cspec);
void	copy_catspec(CATSPEC *cnew, const CATSPEC *cspec);
void	define_catspec(CATSPEC *cspec, STRING class, STRING name, STRING type,
						STRING cat, STRING val, STRING attrib,
						const POINT offset, float angle);

void	init_pltspec(PLTSPEC *cspec);
void	free_pltspec(PLTSPEC *cspec);
void	copy_pltspec(PLTSPEC *cnew, const PLTSPEC *cspec);
void	define_pltspec(PLTSPEC *cspec, STRING type, STRING name,
						const POINT offset, float angle);

#ifdef DEBUG_PSPEC
void	debug_lspec(LSPEC *lspec, STRING msg, int indent);
void	debug_fspec(FSPEC *fspec, STRING msg, int indent);
void	debug_tspec(TSPEC *tspec, STRING msg, int indent);
void	debug_mspec(MSPEC *mspec, STRING msg, int indent);
void	debug_bspec(BSPEC *bspec, STRING msg, int indent);
void	debug_conspec(CONSPEC *cspec, STRING msg, int indent);
void	debug_catspec(CATSPEC *cspec, STRING msg, int indent);
void	debug_pltspec(PLTSPEC *cspec, STRING msg, int indent);
#endif /* DEBUG_PSPEC */

/* Declare functions in pspec_attrib.c */
void	provide_colour_function(COLOUR (*)(STRING));
void	provide_lstyle_function(LSTYLE (*)(STRING));
void	provide_lwidth_function(float (*)(STRING));
void	provide_fstyle_function(FSTYLE (*)(STRING));
void	provide_mtype_function(MTYPE (*)(STRING));
void	provide_font_function(FONT (*)(STRING));
void	provide_size_function(float (*)(STRING));
void	provide_btype_function(BTYPE (*)(STRING));
void	provide_poption_function(LOGICAL (*)(STRING, STRING));

COLOUR	find_direct_colour(STRING name);
COLOUR	find_colour(STRING name, LOGICAL *status);
LSTYLE	find_lstyle(STRING name, LOGICAL *status);
float	find_lwidth(STRING name, LOGICAL *status);
FSTYLE	find_fstyle(STRING name, LOGICAL *status);
MTYPE	find_mtype(STRING name, LOGICAL *status);
FONT	find_font(STRING name, LOGICAL *status);
float	find_size(STRING name, LOGICAL *status);
float	find_offset(STRING name, LOGICAL *status);
BTYPE	find_btype(STRING name, LOGICAL *status);
LOGICAL	find_poption(STRING name, STRING type, LOGICAL *status);

/* Declare functions in pspec_units.c */
void	init_uspec(USPEC *uspec);
void	skip_uspec(USPEC *uspec);
void	free_uspec(USPEC *uspec);
void	copy_uspec(USPEC *unew, const USPEC *uspec);
void	define_uspec(USPEC *uspec, STRING name, double factor, double offset);
void	recall_uspec(USPEC *uspec,
						STRING *name, double *factor, double *offset);
double	convert_by_uspec(const USPEC *uto, const USPEC *ufrom, double value);

#ifdef DEBUG_PSPEC
void	debug_uspec(USPEC *uspec, STRING msg, int indent);
#endif /* DEBUG_PSPEC */

/* Now it has been included */
#endif
