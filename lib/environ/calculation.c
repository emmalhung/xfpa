/**********************************************************************/
/** @file calculation.c
 *
 * Routines to handle various calculations, formmatting and encoding
 * for winds and values.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c a l c u l a t i o n . c                                          *
*                                                                      *
*   Routines to handle various calculations, formatting and encoding   *
*   for winds and values.                                              *
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

#define CALC_INIT
#include "calculation.h"
#include "config_info.h"
#include <fpa_getmem.h>


/* Commonly used literal tags */
#define WindAbs  "abs"
#define WindRel  "model"
#define Degrees  "degrees"
#define DegreesT "degrees_true"
#define Dsymbol  "\260"	/* ISO fonts (preferred) */
#define DsymANSI "\263"	/* ANSI fonts (supported on input) */
#define Percent  "percent"
#define Psymbol  "%"
#define MperS    "m/s"
#define Knots    "knots"
#define Ksymbol  "knots"


static STRING	build_wind_attrib(STRING, LOGICAL, float, STRING);
static LOGICAL	parse_wind_attrib(STRING, STRING, LOGICAL *, float *, STRING *);


/***********************************************************************
*                                                                      *
*   b u i l d _ w i n d _ v a l u e _ s t r i n g                      *
*   p a r s e _ w i n d _ v a l u e _ s t r i n g                      *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Construct a string which represents a wind value (direction, speed
 * and optional gust).  Returns an allocated string.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*wv		wind values
 * 	@return Wind value as a STRING.
 **********************************************************************/
STRING		build_wind_value_string

	(
	WIND_VAL	*wv
	)

	{
	char	value[64];
	float	vdir, vspd, vgust;

	if (IsNull(wv))    return NullString;
	if (wv->dir < 0)   return NullString;
	if (wv->speed < 0) return NullString;

	vdir  = fround(wv->dir, 0);
	vspd  = fround(wv->speed, 0);
	vgust = fround(wv->gust, 0);

	if (vgust > vspd) (void) sprintf(value, "%g%s %g:%g %s",
								vdir, Dsymbol, vspd, vgust, Ksymbol);
	else              (void) sprintf(value, "%g%s %g %s",
								vdir, Dsymbol, vspd, Ksymbol);

	return strdup(value);
	}

/**********************************************************************/

/**********************************************************************/
/** Parse a wind value string.
 *
 * Decipher a wind value, as built by build_wind_value_string.
 *
 * 	@param[in]	value	value to parse
 * 	@param[out]	*wv		wind value struct
 *  @return True if success.
 **********************************************************************/
LOGICAL	parse_wind_value_string

	(
	STRING		value,
	WIND_VAL	*wv
	)

	{
	int		nf;
	float	vdir, vspd, vgust;
	char	dunits[64], sunits[64];

	if (IsNull(wv)) return FALSE;
	wv->dir   = 0;
	wv->dunit = DegreesT;	/* Warning - pointer */
	wv->speed = 0;
	wv->gust  = 0;
	wv->sunit = Knots;		/* Warning - pointer */

	if (blank(value)) return FALSE;
	nf = sscanf(value, "%g%s %g:%g%s", &vdir, dunits, &vspd, &vgust, sunits);
	if (nf <= 2) return FALSE;
	if (nf <= 3)
		{
		nf = sscanf(value, "%g%s %g%s", &vdir, dunits, &vspd, sunits);
		vgust = 0;
		}
	if (!same(dunits, Dsymbol) && !same(dunits, DsymANSI)
			&& !same_ic(dunits, Degrees) && !same_ic(dunits, DegreesT))
		return FALSE;
	if (!same(sunits, Ksymbol)) return FALSE;

	wv->dir   = vdir;
	wv->speed = vspd;
	wv->gust  = vgust;
	return TRUE;
	}


/***********************************************************************
*                                                                      *
*   b u i l d _ w i n d _ a t t r i b s                                *
*   p a r s e _ w i n d _ a t t r i b s                                *
*   c o n s i s t e n t _ w i n d _ a t t r i b s                      *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Build a wind attributes structure.
 *
 * Construct the set of wind attributes that correspond to the
 * given WIND_CALC struct.
 *
 *	@param[in]	cal	Category attribute list
 *	@param[out]	*wc	Wind calculation structure
 *  @return True if success.
 **********************************************************************/
LOGICAL	build_wind_attribs

	(
	CAL			cal,
	WIND_CALC	*wc
	)

	{
	float	vdir, vspd, vgust;
	STRING	val, wdir, wspd, wgust, dunits, sunits;
	LOGICAL	drel, srel;

	if (IsNull(cal)) return FALSE;
	if (IsNull(wc))  return FALSE;

	vdir  = fround(wc->dir, 0);
	vspd  = fround(wc->speed, 0);
	vgust = fround(wc->gust, 0);

	/* If model not given - must be absolute */
	val = wc->model;
	if (blank(val))
		{
		if (wc->rel_dir)   return FALSE;
		if (wc->rel_speed) return FALSE;
		val = FpaAbsWindModel;
		}
	CAL_add_attribute(cal, AttribWindModel, val);
	CAL_add_attribute(cal, AttribCategory,  val);

	/* Handle direction */
	drel   = wc->rel_dir;
	dunits = Dsymbol;
	if (!drel && vdir<0) return FALSE;
	wdir   = build_wind_attrib(AttribWindDirection, drel, vdir, dunits);
	CAL_add_attribute(cal, AttribWindDirection, wdir);

	/* Handle speed */
	srel   = wc->rel_speed;
	if (vspd < 0) return FALSE;
	sunits = (srel)? Psymbol: Ksymbol;
	wspd   = build_wind_attrib(AttribWindSpeed, srel, vspd, sunits);
	CAL_add_attribute(cal, AttribWindSpeed, wspd);

	/* Handle optional gust */
	if (vgust > vspd)
		 wgust = build_wind_attrib(AttribWindGust, srel, vgust, sunits);
	else wgust = NullString;
	CAL_add_attribute(cal, AttribWindGust, wgust);

	/* Handle auto-label and user-label */
	val = build_wind_label_attrib(AttribAutolabel, wc);
	CAL_add_attribute(cal, AttribAutolabel, val);
	val = build_wind_label_attrib(AttribUserlabel, wc);
	CAL_add_attribute(cal, AttribUserlabel, val);

	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Decipher the wind attributes in the given CAL struct into a
 * WIND_CALC struct.
 *
 *	@param[in]	cal	Category attribute list to search
 *	@param[out]	*wc	WIND_CALC struct
 *  @return True if success.
 **********************************************************************/
LOGICAL	parse_wind_attribs

	(
	CAL			cal,
	WIND_CALC	*wc
	)

	{
	STRING	wmodel, wdir, wspeed, wgust;
	float	vdir, vspeed, vgust;
	LOGICAL	drel, srel, grel;

	static	STRING	AbsWind = FpaAbsWindModel;

	if (IsNull(cal)) return FALSE;
	if (IsNull(wc))  return FALSE;

	/* Obtain wind attributes */
	wmodel = CAL_get_attribute(cal, AttribWindModel);
	wdir   = CAL_get_attribute(cal, AttribWindDirection);
	wspeed = CAL_get_attribute(cal, AttribWindSpeed);
	wgust  = CAL_get_attribute(cal, AttribWindGust);

	/* Set model to absolute (if required) */
	if (blank(wmodel)) wmodel = AbsWind;

	/* Parse direction */
	drel = TRUE;
	vdir = 0;
	if (!blank(wdir)
		&& !parse_wind_attrib(AttribWindDirection, wdir, &drel, &vdir,
				NullStringPtr)) return FALSE;

	/* Parse speed */
	srel   = TRUE;
	vspeed = 100;
	if (!blank(wspeed)
		&& !parse_wind_attrib(AttribWindSpeed, wspeed, &srel, &vspeed,
				NullStringPtr)) return FALSE;

	/* Parse gust (if required) */
	grel  = srel;
	vgust = 0;
	if (!blank(wgust)
		&& !parse_wind_attrib(AttribWindGust, wgust, &grel, &vgust,
				NullStringPtr)) return FALSE;
	if (srel && !grel) vgust = vspeed;
	if (grel && !srel) vgust = vspeed;

	/* Check model */
	if (same_ic(wmodel, FpaAbsWindModel))
		{
		if (drel || srel) return FALSE;
		}

	/* Now build the structure */
	wc->model     = wmodel;		/* Note just copying pointer */
	wc->dir       = vdir;
	wc->speed     = vspeed;
	wc->gust      = vgust;
	wc->rel_dir   = drel;
	wc->rel_speed = srel;
	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Make sure the wind attributes in the given CAL struct are consistent.
 *
 *	@param[in]	cal		Category Attribute List to check
 * 	@return True if consistent.
 **********************************************************************/
LOGICAL		consistent_wind_attribs

	(
	CAL		cal
	)

	{
	STRING	wmodel, wdir, wspeed, wgust;

	if (IsNull(cal)) return FALSE;

	wmodel = CAL_get_attribute(cal, AttribWindModel);
	wdir   = CAL_get_attribute(cal, AttribWindDirection);
	wspeed = CAL_get_attribute(cal, AttribWindSpeed);
	wgust  = CAL_get_attribute(cal, AttribWindGust);

	return consistent_wind_attrib_strings(wmodel, wdir, wspeed, wgust,
				NullStringPtr, NullStringPtr, NullStringPtr, NullStringPtr,
				NullStringPtr, NullStringPtr, NullStringPtr);
	}


/***********************************************************************
*                                                                      *
*   b u i l d _ w i n d _ a t t r i b _ s t r i n g                    *
*   c o n s i s t e n t _ w i n d _ a t t r i b _ s t r i n g s        *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Build a wind attributes STRING.
 *
 * Construct the requested wind attribute string from the given
 * simplified config-file string.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	att			Name of attribute to label
 *	@param[in]	cfgval		Value from config file.
 *  @return Wind attributes as a STRING.
 **********************************************************************/
STRING		build_wind_attrib_string

	(
	STRING	att,
	STRING	cfgval
	)

	{
	int		nc;
	float	fval;
	char	sval[64];
	char	junk[64];

	static	STRING	Satt = NullString;

	FREEMEM(Satt);

	fval = 0;
	(void) strcpy(sval, "");
	(void) strcpy(junk, "");

	if (same_ic(att, AttribWindModel))
		{
		nc = sscanf(cfgval, "%s%s", sval, junk);
		if (nc < 1) return NullString;
		if (nc > 1) return NullString;
		att = build_wind_attrib(att, TRUE, 0, sval);
		}
	else if (same_ic(att, AttribWindDirection))
		{
		nc = sscanf(cfgval, "%g%s%s", &fval, sval, junk);
		if (nc < 1) return NullString;
		if (nc > 2) return NullString;
		att = build_wind_attrib(att, TRUE, fval, sval);
		}
	else if (same_ic(att, AttribWindSpeed))
		{
		nc = sscanf(cfgval, "%g%s%s", &fval, sval, junk);
		if (nc < 1) return NullString;
		if (nc > 2) return NullString;
		att = build_wind_attrib(att, TRUE, fval, sval);
		}
	else if (same_ic(att, AttribWindGust))
		{
		nc = sscanf(cfgval, "%g%s%s", &fval, sval, junk);
		if (nc < 1) return NullString;
		if (nc > 2) return NullString;
		att = build_wind_attrib(att, TRUE, fval, sval);
		}
	else
		{
		return NullString;
		}

	Satt = INITSTR(att);
	return Satt;
	}

/**********************************************************************/

/**********************************************************************/
/** Make sure the given wind attributes are consistent.
 *
 *	@param[in]	wmodel		Wind model
 *	@param[in]	wdir		Wind direction
 *	@param[in]	wspeed		Wind speed
 *	@param[in]	wgust		Wind gust
 *	@param[out]	*cmodel		new model
 *	@param[out]	*cdir		new dir
 *	@param[out]	*cspeed		new speed
 *	@param[out]	*cgust		new gust
 *	@param[out]	*ccat		new category
 *	@param[out]	*cval		new value
 *	@param[out]	*clab		new label
 * @return True if consistent.
 **********************************************************************/
LOGICAL		consistent_wind_attrib_strings

	(
	STRING	wmodel,
	STRING	wdir,
	STRING	wspeed,
	STRING	wgust,
	STRING	*cmodel,
	STRING	*cdir,
	STRING	*cspeed,
	STRING	*cgust,
	STRING	*ccat,
	STRING	*cval,
	STRING	*clab
	)

	{
	LOGICAL	consist;
	LOGICAL	drel, srel, grel;
	float	vdir, vspeed, vgust;
	STRING	dunits, sunits, gunits;
	char	dubuf[64], subuf[64];
	STRING	wval, wlab;
	FpaConfigCrossRefStruct		*crdef;

	static	STRING	Smodel = NullString;
	static	STRING	Sdir   = NullString;
	static	STRING	Sspeed = NullString;
	static	STRING	Sgust  = NullString;
	static	STRING	Scat   = NullString;
	static	STRING	Sval   = NullString;
	static	STRING	Slab   = NullString;

	FREEMEM(Smodel);
	FREEMEM(Sdir);
	FREEMEM(Sspeed);
	FREEMEM(Sgust);
	FREEMEM(Scat);
	FREEMEM(Sval);
	FREEMEM(Slab);

	consist = TRUE;

	/* Can we read the direction info */
	if (!blank(wdir)
		&& !parse_wind_attrib(AttribWindDirection, wdir, &drel, &vdir, &dunits))
		{
		consist = FALSE;
		wdir    = NullString;
		}
	if (blank(wdir))
		{
		if (same_ic(wmodel, FpaAbsWindModel))
			{
			drel    = FALSE;
			vdir    = 0;
			dunits  = Dsymbol;
			}
		else
			{
			drel    = TRUE;
			vdir    = 0;
			dunits  = Dsymbol;
			}
		}
	else dunits = strcpy(dubuf, dunits);

	/* Can we read the speed info */
	if (!blank(wspeed)
		&& !parse_wind_attrib(AttribWindSpeed, wspeed, &srel, &vspeed, &sunits))
		{
		consist = FALSE;
		wspeed  = NullString;
		}
	if (blank(wspeed))
		{
		if (same_ic(wmodel, FpaAbsWindModel))
			{
			srel    = FALSE;
			vspeed  = 0;
			sunits  = Ksymbol;
			}
		else
			{
			srel    = TRUE;
			vspeed  = 100;
			sunits  = Psymbol;
			}
		}
	else sunits = strcpy(subuf, sunits);

	/* Can we read the gust info and is it consistent with the speed info */
	if (blank(wgust))
		{
		vgust = 0;
		}
	else if (!parse_wind_attrib(AttribWindGust, wgust, &grel, &vgust, &gunits)
			|| (srel && !grel)
			|| (grel && !srel)
			|| !same(sunits, gunits)
			)
		{
		consist = FALSE;
		vgust   = 0;
		}

	/* Make sure the model is either "FPA_Absolute_Wind_Model" */
	/*  or a recognized wind crossreference                    */
	if (blank(wmodel))
		{
		consist = FALSE;
		wmodel  = FpaAbsWindModel;
		}
	else if (!same_ic(wmodel, FpaAbsWindModel))
		{
		/* If direction and speed are both absolute then the wind model */
		/* might as well be Absolute */
		if (!drel && !srel)
			{
			wmodel  = FpaAbsWindModel;
			}
		else
			{
			/* Make sure we can find the model */
			crdef = identify_crossref(FpaCcRefsWinds, wmodel);
			if (IsNull(crdef))
				{
				consist = FALSE;
				wmodel  = FpaAbsWindModel;
				}
			}
		}

	/* Make sure direction and speed info are consistent with an           */
	/* Absolute wind model (model may have been changed to Absolute above) */
	if (same_ic(wmodel, FpaAbsWindModel))
		{
		/* Cannot have a relative direction with Absolute model */
		if (drel)
			{
			consist = FALSE;
			drel    = FALSE;
			vdir    = 0;
			dunits  = Dsymbol;
			}

		/* Cannot have a relative speed or gust with Absolute model */
		if (srel)
			{
			consist = FALSE;
			srel    = FALSE;
			vspeed  = 0;
			vgust   = 0;
			sunits  = Ksymbol;
			}
		}

	Smodel = INITSTR(wmodel);
	wdir   = build_wind_attrib(AttribWindDirection, drel, vdir, dunits);
	Sdir   = INITSTR(wdir);
	wspeed = build_wind_attrib(AttribWindSpeed, srel, vspeed, sunits);
	Sspeed = INITSTR(wspeed);
	wgust  = build_wind_attrib(AttribWindGust, srel, vgust, sunits);
	Sgust  = INITSTR(wgust);
	if (consist && (ccat || cval || clab))
		{
		WIND_CALC	wc;

		wc.model     = wmodel;		/* Note just copying pointer */
		wc.dir       = vdir;
		wc.speed     = vspeed;
		wc.gust      = vgust;
		wc.rel_dir   = drel;
		wc.rel_speed = srel;

		Scat   = INITSTR(wmodel);
		wval   = build_wind_label_attrib(AttribAutolabel, &wc);
		Sval   = INITSTR(wval);
		wlab   = build_wind_label_attrib(AttribUserlabel, &wc);
		Slab   = INITSTR(wlab);
		}

	/* Copy back new values */
	if (cmodel) *cmodel = Smodel;
	if (cdir)   *cdir   = Sdir;
	if (cspeed) *cspeed = Sspeed;
	if (cgust)  *cgust  = Sgust;
	if (ccat)   *ccat   = Scat;
	if (cval)   *cval   = Sval;
	if (clab)   *clab   = Slab;
	return consist;
	}

/***********************************************************************
*                                                                      *
*   b u i l d _ w i n d _ l a b e l _ a t t r i b                      *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Build a wind label STRING.
 *
 * Construct the wind label and value attributes from the given
 * WIND_CALC struct.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	att		Name of attribute to fetch
 *	@param[out]	*wc		Wind model
 * @return Wind label as a STRING.
 **********************************************************************/
STRING	build_wind_label_attrib

	(
	STRING		att,
	WIND_CALC	*wc
	)

	{
	FpaConfigCrossRefStruct		*crdef;
	STRING						wmodel;
	char						*bp;

	static char					buf[256];

	if (IsNull(wc)) return NullString;

	/* Determine how to indicate the model */
	if (same_ic(att, AttribUserlabel))
		{
		if (wc->rel_dir || wc->rel_speed)
			{
			if (blank(wc->model)) return NullString;
			crdef = identify_crossref(FpaCcRefsWinds, wc->model);
			if (IsNull(crdef)) return NullString;
			wmodel = crdef->sh_label;
			}
		else wmodel = NullString;
		}
	else if (same_ic(att, AttribAutolabel))
		{
		wmodel = NullString;
		}
	else return NullString;

	/* Add direction */
	(void) strcpy(buf, "");
	bp = buf;
	if (wc->rel_dir)
		{
		(void) sprintf(bp, "%+g%s", wc->dir, Dsymbol);
		bp = buf + strlen(buf);

		/* Add wind model here if speed is absolute */
		if (!wc->rel_speed)
			{
			if (!blank(wmodel))
				{
				(void) sprintf(bp, " %s", wmodel);
				bp = buf + strlen(buf);
				}
			}
		}
	else
		{
		if (wc->dir < 0) return NullString;
		(void) sprintf(bp, "%g%s", wc->dir, Dsymbol);
		bp = buf + strlen(buf);
		}

	/* Add speed and gust */
	if (wc->rel_speed)
		{
		(void) sprintf(bp, " %g%s", wc->speed, Psymbol);
		bp = buf + strlen(buf);

		if (wc->gust > wc->speed)
			{
			(void) sprintf(bp, " G%g%s", wc->gust, Psymbol);
			bp = buf + strlen(buf);
			}

		/* Add wind model here (if not already done) */
		if (!blank(wmodel))
			{
			(void) sprintf(bp, " %s", wmodel);
			bp = buf + strlen(buf);
			}
		}
	else
		{
		if (wc->speed < 0) return NullString;
		(void) sprintf(bp, " %g%s", wc->speed, Ksymbol);
		bp = buf + strlen(buf);

		if (wc->gust > wc->speed)
			{
			(void) sprintf(bp, " G%g%s", wc->gust, Ksymbol);
			bp = buf + strlen(buf);
			}
		}

	return buf;
	}

/***********************************************************************
*                                                                      *
*   s t a t i c   f u n c t i o n s                                    *
*                                                                      *
***********************************************************************/
/***********************************************************************
*                                                                      *
*   b u i l d _ w i n d _ a t t r i b                                  *
*                                                                      *
*   Construct the requested wind attribute from the given values.      *
*                                                                      *
*   p a r s e _ w i n d _ a t t r i b                                  *
*                                                                      *
*   Parse the requested wind attribute into the given values.          *
*                                                                      *
***********************************************************************/

static STRING	build_wind_attrib

	(
	STRING	att,
	LOGICAL	rel,
	float	fval,
	STRING	sval
	)

	{
	double				vval;
	FpaConfigUnitStruct	*udef;

	static	char	buf[256];

	/* Build model */
	if (same_ic(att, AttribWindModel))
		{
		if (blank(sval)) return FpaAbsWindModel;
		else             return sval;
		}

	/* Build direction */
	else if (same_ic(att, AttribWindDirection))
		{
		vval = fval;
		if (!rel && fval < 0) return NullString;
		if (!same(sval, Dsymbol) && !same(sval, DsymANSI)
				&& !same(sval, Degrees) && !same(sval, DegreesT))
			{
			udef = identify_unit(sval);
			if (IsNull(udef))                                return NullString;
			if (!same(udef->MKS, DegreesT))                  return NullString;
			if (!convert_value(sval, fval, DegreesT, &vval)) return NullString;
			}
		if (rel) (void) sprintf(buf, "%s %+g%s", WindRel, vval, Dsymbol);
		else     (void) sprintf(buf, "%s %g%s",  WindAbs, vval, Dsymbol);
		return buf;
		}

	/* Build speed */
	else if (same_ic(att, AttribWindSpeed))
		{
		vval = fval;
		if (fval < 0) return NullString;
		if (rel)
			{
			if (!same(sval, Psymbol) && !same(sval, Percent))
				{
				udef = identify_unit(sval);
				if (IsNull(udef))                               return NullString;
				if (!same(udef->MKS, Percent))                  return NullString;
				if (!convert_value(sval, fval, Percent, &vval)) return NullString;
				}
			(void) sprintf(buf, "%s %g%s",  WindRel, vval, Psymbol);
			}
		else
			{
			if (!same(sval, Ksymbol) && !same(sval, Knots))
				{
				udef = identify_unit(sval);
				if (IsNull(udef))                               return NullString;
				if (!same(udef->MKS, MperS))                    return NullString;
				if (!convert_value(sval, fval, Ksymbol, &vval)) return NullString;
				}
			(void) sprintf(buf, "%s %g %s", WindAbs, vval, Ksymbol);
			}
		return buf;
		}

	/* Build gust */
	else if (same_ic(att, AttribWindGust))
		{
		vval = fval;
		if (fval < 0) return NullString;
		if (rel)
			{
			if (!same(sval, Psymbol) && !same(sval, Percent))
				{
				udef = identify_unit(sval);
				if (IsNull(udef))                               return NullString;
				if (!same(udef->MKS, Percent))                  return NullString;
				if (!convert_value(sval, fval, Psymbol, &vval)) return NullString;
				}
			(void) sprintf(buf, "%s %g%s",  WindRel, vval, Psymbol);
			}
		else
			{
			if (!same(sval, Ksymbol) && !same(sval, Knots))
				{
				udef = identify_unit(sval);
				if (IsNull(udef))                               return NullString;
				if (!same(udef->MKS, MperS))                    return NullString;
				if (!convert_value(sval, fval, Ksymbol, &vval)) return NullString;
				}
			(void) sprintf(buf, "%s %g %s", WindAbs, vval, Ksymbol);
			}
		return buf;
		}

	return NullString;
	}

/**********************************************************************/

static LOGICAL	parse_wind_attrib

	(
	STRING	att,
	STRING	str,
	LOGICAL	*rel,
	float	*fval,
	STRING	*sval
	)

	{
	float	vval;
	LOGICAL	vrel;
	char	mode[64];
	int		nr;

	static	char	sbuf[64];

	if (rel)  *rel  = FALSE;
	if (fval) *fval = 0;
	if (sval) *sval = NullString;

	if (same_ic(att, AttribWindModel))
		{
		if (sval) *sval = str;
		return TRUE;
		}

	/* Parse direction */
	else if (same_ic(att, AttribWindDirection))
		{
		vrel = TRUE;
		vval = 0;
		if (!blank(str))
			{
			nr = sscanf(str, "%s %g%s", mode, &vval, sbuf);
			if (nr < 2)                   return FALSE;
			if (same(mode, WindRel))      vrel = TRUE;
			else if (same(mode, WindAbs)) vrel = FALSE;
			else                          return FALSE;
			if (!same(sbuf, Dsymbol)
				&& !same(sbuf, DsymANSI)) return FALSE;
			}

		if (rel)  *rel  = vrel;
		if (fval) *fval = vval;
		if (sval) *sval = sbuf;
		return TRUE;
		}

	/* Parse speed */
	else if (same_ic(att, AttribWindSpeed))
		{
		vrel = TRUE;
		vval = 100;
		if (!blank(str))
			{
			nr = sscanf(str, "%s %g%s", mode, &vval, sbuf);
			if (nr < 2)                        return FALSE;
			if (same(mode, WindRel))           vrel = TRUE;
			else if (same(mode, WindAbs))      vrel = FALSE;
			else                               return FALSE;
			if (vrel  && !same(sbuf, Psymbol)) return FALSE;
			if (!vrel && !same(sbuf, Ksymbol)) return FALSE;
			}

		if (rel)  *rel  = vrel;
		if (fval) *fval = vval;
		if (sval) *sval = sbuf;
		return TRUE;
		}

	/* Parse gust */
	else if (same_ic(att, AttribWindGust))
		{
		vrel = TRUE;
		vval = 0;
		if (!blank(str))
			{
			nr = sscanf(str, "%s %g%s", mode, &vval, sbuf);
			if (nr < 2)                        return FALSE;
			if (same(mode, WindRel))           vrel = TRUE;
			else if (same(mode, WindAbs))      vrel = FALSE;
			else                               return FALSE;
			if (vrel  && !same(sbuf, Psymbol)) return FALSE;
			if (!vrel && !same(sbuf, Ksymbol)) return FALSE;
			}

		if (rel)  *rel  = vrel;
		if (fval) *fval = vval;
		if (sval) *sval = sbuf;
		return TRUE;
		}

	return FALSE;
	}
