/**********************************************************************/
/** @file cal.c
 *
 * Routines to handle the Controlled Attribute List
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c a l . c                                                          *
*                                                                      *
*   Routines to handle the Controlled Attribute List                   *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2002 Environment Canada                  *
*     Version 8 (c) Copyright 2009 Environment Canada                  *
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

#define CAL_INIT
#include "cal.h"
#include "config_info.h"

#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>


/* Commonly used literal tags */
#define Dsymbol  "\260"	/* ISO fonts (preferred) */

/***********************************************************************
*                                                                      *
*  C A L _ c r e a t e _ b y _ n a m e                                 *
*  C A L _ c r e a t e _ b y _ e d e f                                 *
*  C A L _ c r e a t e _ d e f a u l t                                 *
*  C A L _ c r e a t e _ e m p t y                                     *
*  C A L _ d e s t r o y                                               *
*  C A L _ e m p t y                                                   *
*  C A L _ c l e a n                                                   *
*  C A L _ d u p l i c a t e                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Create Category Attribute List give the names of the element and
 * level.
 *
 *	@param[in]	elem		element name
 *	@param[in]	level		level name
 *  @return A CAL structure for the given field.
 **********************************************************************/
CAL		CAL_create_by_name
	(
	STRING	elem,
	STRING	level
	)

	{
	FpaConfigFieldStruct	*fdef;
	FpaConfigElementStruct	*edef;

	if (blank(elem))  elem  = FpaCanyElement;
	if (blank(level)) level = FpaCanyLevel;

	fdef = get_field_info(elem, level);
	edef = (fdef)? fdef->element: get_element_info(elem);

	return CAL_create_by_edef(edef);
	}

/**********************************************************************/

/**********************************************************************/
/** Create a Category Attribute List structure given an EDEF structure.
 *
 *	@param[in]	*edef	Element structure
 *  @return A CAL structure for the given field.
 **********************************************************************/
CAL		CAL_create_by_edef
	(
	FpaConfigElementStruct	*edef
	)

	{
	int								i;
	STRING							name, dval;
	ATTRIB_LIST						al;
	FpaConfigElementAttribStruct	*attlist;

	if ( IsNull(edef) )              return CAL_create_default();
	if ( IsNull(edef->elem_detail) )
		{
		edef = get_element_info(edef->name);
		if ( IsNull(edef) )          return CAL_create_default();
		}

	attlist = edef->elem_detail->attributes;
	if ( IsNull(attlist) )           return CAL_create_default();

	/* Create an empty attribute list */
	al = create_attrib_list();
	al->defs = (POINTER) attlist;

	/* Add the attributes from the config file */
	for (i=0; i<attlist->nattribs; i++)
		{
		name = attlist->attrib_names[i];
		dval = attlist->attrib_back_defs[i];
		if ( CAL_no_value(dval) ) continue;
		(void) add_attribute(al, name, dval);
		}

	/* Return the result */
	return (CAL) al;
	}

/**********************************************************************/

/**********************************************************************/
/** Create a default Category Attribute List structure.
 *
 * @return A CAL structure setup with default values.
 **********************************************************************/
CAL		CAL_create_default
	(
	)

	{
	return (CAL) create_default_attrib_list(NULL, NULL, NULL);
	}

/**********************************************************************/

/**********************************************************************/
/** Create an empty Category Attribute List structure.
 *
 * @return An empty CAL structure.
 **********************************************************************/
CAL		CAL_create_empty
	(
	)

	{
	return (CAL) create_attrib_list();
	}

/**********************************************************************/

/**********************************************************************/
/** Destroy a Category Attribute List structure.
 *
 *	@param[in]		cal			Category Attribute List
 *  @return A (Null) CAL.
 **********************************************************************/
CAL		CAL_destroy
	(
	CAL		cal
	)

	{
	ATTRIB_LIST	al;

	al = (ATTRIB_LIST) cal;
	return ( (CAL) destroy_attrib_list(al) );
	}

/**********************************************************************/

/**********************************************************************/
/** Empty a Category Attribute List structure
 *
 *	@param[in]		cal			Category Attribute List
 **********************************************************************/
void	CAL_empty
	(
	CAL		cal
	)

	{
	ATTRIB_LIST	al;

	al = (ATTRIB_LIST) cal;
	empty_attrib_list(al);
	}

/**********************************************************************/

/**********************************************************************/
/** Clean a Category Attribute List structure
 *
 *	@param[in]		cal			Category Attribute List
 **********************************************************************/
void	CAL_clean
	(
	CAL		cal
	)

	{
	ATTRIB_LIST	al;

	al = (ATTRIB_LIST) cal;
	clean_attrib_list(al);
	}

/**********************************************************************/

/**********************************************************************/
/** Duplicate a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 * @return A copy of the given CAL structure.
 **********************************************************************/
CAL		CAL_duplicate
	(
	const CAL	cal
	)

	{
	ATTRIB_LIST	al;

	al = (ATTRIB_LIST) cal;
	return ( (CAL) copy_attrib_list(al) );
	}

/***********************************************************************
*                                                                      *
*  C A L _ a d d _ a t t r i b u t e                                   *
*  C A L _ s e t _ a t t r i b u t e                                   *
*  C A L _ g e t _ a t t r i b u t e                                   *
*  C A L _ h a s _ a t t r i b u t e                                   *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Add an attribute to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	name		Name of attribute to add
 *	@param[in]	value		Value of attribute
 **********************************************************************/
void	CAL_add_attribute
	(
	CAL		cal,
	STRING	name,
	STRING	value
	)

	{
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;

	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, name, value);
	}

/**********************************************************************/

/**********************************************************************/
/** Set an attribute in a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	name		Name of attribute to set
 *	@param[in]	value		Value to set attribute
 **********************************************************************/
void	CAL_set_attribute
	(
	CAL		cal,
	STRING	name,
	STRING	value
	)

	{
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;
	if ( !CAL_has_attribute(cal, name) ) return;

	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, name, value);
	}

/**********************************************************************/

/**********************************************************************/
/** Get an attribute in a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	name		Name of attribute to look for
 *  @return The value of attribute.
 * 			Returned value is stored in a static variable within
 * 			function if you are not going to use it immediately it
 * 			is safest to make a copy with safe_strcpy or safe_strdup.
 **********************************************************************/
STRING	CAL_get_attribute
	(
	CAL		cal,
	STRING	name
	)

	{
	ATTRIB_LIST	al;
	ATTRIB		*at;
	STRING		value;
	int			i;
	FpaConfigElementAttribStruct	*attlist;

	if ( IsNull(cal) ) return CAL_NO_VALUE;

	al = (ATTRIB_LIST) cal;
	at = get_attribute(al, name, &value);
	if ( NotNull(at) )
		{
		return ( CAL_is_value(value) )? value: CAL_NO_VALUE;
		}

	/* Find the attribute in the default list from the config file */
	if (IsNull(al->defs)) return CAL_NO_VALUE;

	attlist = (FpaConfigElementAttribStruct *) al->defs;
	for (i=0; i<attlist->nattribs; i++)
		{
		if ( same_ic(name, attlist->attrib_names[i]) )
			{
			value = attlist->attrib_back_defs[i];
			return ( CAL_is_value(value) )? value: CAL_NO_VALUE;
			}
		}

	return CAL_NO_VALUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Check if a Category Attribute List structure has the requested
 * attribute.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	name		Name of attribute to look for
 * @return True if CAL has the attribute.
 **********************************************************************/
LOGICAL	CAL_has_attribute
	(
	CAL		cal,
	STRING	name
	)

	{
	ATTRIB_LIST	al;
	ATTRIB		*at;
	int			i;
	FpaConfigElementAttribStruct	*attlist;

	if ( IsNull(cal) ) return FALSE;

	al = (ATTRIB_LIST) cal;
	at = find_attribute(al, name);
	if (NotNull(at)) return TRUE;

	/* Find the attribute in the default list from the config file */
	if (IsNull(al->defs)) return FALSE;
	attlist = (FpaConfigElementAttribStruct *) al->defs;
	for (i=0; i<attlist->nattribs; i++)
		{
		if ( same_ic(name, attlist->attrib_names[i]) ) return TRUE;
		}

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*  C A L _ a d d _ l o c a t i o n                                     *
*  C A L _ a d d _ p r o x i m i t y                                   *
*  C A L _ a d d _ n e g a t i v e _ p r o x i m i t y                 *
*  C A L _ a d d _ a r e a _ s i z e                                   *
*  C A L _ a d d _ l i n e _ d i r                                     *
*  C A L _ a d d _ l i n e _ l e n                                     *
*  C A L _ a d d _ l c h a i n _ n o d e _ m o t i o n                 *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Add a location to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	pos			Coordinate to add
 **********************************************************************/
void	CAL_add_location
	(
	CAL			cal,
	MAP_PROJ	*mproj,
	POINT		pos
	)

	{
	float		flat, flon;
	char		slat[20], slon[20];
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;

	if ( !pos_to_ll(mproj, pos, &flat, &flon) ) return;

	(void) sprintf(slat, "%.4f", flat);
	(void) sprintf(slon, "%.4f", flon);

	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribLatitude,  slat);
	(void) add_attribute(al, AttribLongitude, slon);
	}

/**********************************************************************/
/** Add proximity to a feature to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	spos		Sample location
 *	@param[in]	epos		Feature location
 **********************************************************************/
void	CAL_add_proximity
	(
	CAL			cal,
	MAP_PROJ	*mproj,
	POINT		spos,
	POINT		epos
	)

	{
	float		fcircle;
	char		scircle[20];
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;

	/* Determine great circle distance between start and end positions */
	fcircle = great_circle_distance(mproj, spos, epos);

	/* Set value in km */
	(void) sprintf(scircle, "%.2f", fcircle/1000.0);

	/* Add to attributes */
	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribProximity, scircle);
	}

/**********************************************************************/
/** Add -proximity to a feature to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	spos		Sample location
 *	@param[in]	epos		Feature location
 **********************************************************************/
void	CAL_add_negative_proximity
	(
	CAL			cal,
	MAP_PROJ	*mproj,
	POINT		spos,
	POINT		epos
	)

	{
	float		fcircle;
	char		scircle[20];
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;

	/* Determine great circle distance between start and end positions */
	fcircle = great_circle_distance(mproj, spos, epos);

	/* Set value in km */
	(void) sprintf(scircle, "%.2f", -fcircle/1000.0);

	/* Add to attributes */
	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribProximity, scircle);
	}

/**********************************************************************/
/** Add size of area (or subarea) to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	spos		Sample location for area (or subarea)
 *	@param[in]	size		Size of area feature
 **********************************************************************/
void	CAL_add_area_size

	(
	CAL			cal,
	MAP_PROJ	*mproj,
	POINT		spos,
	float		size
	)

	{
	float		flat, flon, scalx, scaly, xsize;
	char		sval[20];
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;
	if ( !pos_to_ll(mproj, spos, &flat, &flon) ) return;

	/* Determine map scaling for sample location */
	if ( !ll_distort(mproj, flat, flon, &scalx, &scaly) ) return;

	/* Set value in km2 */
	xsize  = size / scalx / scaly;
	xsize *= mproj->definition.units / 1000.0;
	xsize *= mproj->definition.units / 1000.0;
	(void) sprintf(sval, "%.1f", xsize);

	/* >>>>> testing <<<<< */
	(void) pr_diag("CAL_add_area_size",
			"Size: %.1f  Distort: %.3f/%.3f  Final: %.1f\n",
			size, scalx, scaly, xsize);
	/* >>>>> testing <<<<< */

	/* Add to attributes */
	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribAreaSize, sval);
	}

/**********************************************************************/
/** Add direction of line segment to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	spos		Start position on line segment
 *	@param[in]	epos		End position on line segment
 **********************************************************************/
void	CAL_add_line_dir
	(
	CAL			cal,
	MAP_PROJ	*mproj,
	POINT		spos,
	POINT		epos
	)

	{
	float		flat, flon;
	double		dang, dir;
	char		sdir[20];
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;
	if ( !pos_to_ll(mproj, spos, &flat, &flon) ) return;

	/* Determine direction (towards) in degrees true */
	dir = 0.0;
	if (point_dist(spos, epos) > 0.0)
		{
		dang = atan2deg((double) (epos[Y]-spos[Y]), (double) (epos[X]-spos[X]));
		dir  = wind_dir_true(mproj, flat, flon, (float) dang);
		}

	/* Set value in degrees */
	(void) sprintf(sdir, "%.2f", dir);

	/* Add to attributes */
	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribLineDirection, sdir);
	}

/**********************************************************************/
/** Add line length to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	line		Line feature
 **********************************************************************/
void	CAL_add_line_len
	(
	CAL			cal,
	MAP_PROJ	*mproj,
	LINE		line
	)

	{
	float		flen;
	char		slen[20];
	ATTRIB_LIST	al;

	if ( IsNull(cal) )  return;
	if ( IsNull(line) ) return;

	/* Determine great circle length of line */
	flen = great_circle_line_length(mproj, line);

	/* Set value in km */
	(void) sprintf(slen, "%.2f", flen/1000.0);

	/* Add to attributes */
	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribLineLength, slen);
	}

/**********************************************************************/
/** Add motion at link chain node to a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	*mproj		Applicable map projection
 *	@param[in]	lchain		Link chain
 *	@param[in]	mplus		Time of link chain node
 **********************************************************************/
void	CAL_add_lchain_node_motion
	(
	CAL			cal,
	MAP_PROJ	*mproj,
	LCHAIN		lchain,
	int			mplus
	)

	{
	int			nnum, next, mdiff;
	float		xdist, xspd, dir;
	POINT		spos, npos;
	char		sdir[20], sspd[20], svect[40];
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;
	if ( IsNull(lchain) ) return;

	/* Interpolate the link chain (if required) */
	if ( lchain->dointerp ) (void) interpolate_lchain(lchain);

	/* Set default attributes if no link chain track for motion */
	if ( lchain->inum < 2 )
		{
		al = (ATTRIB_LIST) cal;
		(void) add_attribute(al, AttribLnodeDirection, "0");
		(void) add_attribute(al, AttribLnodeSpeed,     "0");
		(void) add_attribute(al, AttribLnodeVector,    "0");
		return;
		}

	/* Set default attributes if node time is outside start/end time for chain */
	if ( mplus < lchain->splus || mplus > lchain->eplus )
		{
		al = (ATTRIB_LIST) cal;
		(void) add_attribute(al, AttribLnodeDirection, "0");
		(void) add_attribute(al, AttribLnodeSpeed,     "0");
		(void) add_attribute(al, AttribLnodeVector,    "0");
		return;
		}

	/* Determine direction of link chain track at this link node */
	/*  ... using next interpolated node after node time         */
	/*  ... or previous interpolated node for last node on chain */
	nnum = which_lchain_node(lchain, LchainInterp, mplus);
	if ( nnum < lchain->inum - 1 )
		{
		next = nnum + 1;
		}
	else
		{
		next = nnum;
		nnum = (next > 0)? next - 1: next;
		}
	(void) copy_point(spos, lchain->interps[nnum]->node);
	(void) copy_point(npos, lchain->interps[next]->node);

	/* Determine direction (towards) in degrees true */
	dir = great_circle_bearing(mproj, spos, npos);

	/* Set direction in degrees */
	(void) sprintf(sdir, "%d", NINT(dir));

	/* Determine link chain speed at this link node location    */
	xdist = 0.0;
	if ( next > nnum )
		xdist += great_circle_distance(mproj, spos, npos);
	mdiff = lchain->interps[next]->mplus - lchain->interps[nnum]->mplus;
	xspd  = (mdiff > 0) ? (xdist / (60.0 * mdiff)): 0.0;

	/* Set speed in m/s */
	(void) sprintf(sspd, "%d", NINT(xspd));

	/* Set vector in m/s @ degrees */
	(void) sprintf(svect, "%d/%d%s", NINT(xspd), NINT(dir), Dsymbol);

	/* Add to attributes */
	al = (ATTRIB_LIST) cal;
	(void) add_attribute(al, AttribLnodeDirection, sdir);
	(void) add_attribute(al, AttribLnodeSpeed,     sspd);
	(void) add_attribute(al, AttribLnodeVector,    svect);
	}

/***********************************************************************
*                                                                      *
*  C A L _ s e t _ d e f a u l t s                                     *
*  C A L _ g e t _ d e f a u l t s                                     *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Set the defaults for a Category Attribute List structure.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	sub			Default background value ?not sure?
 *	@param[in]	val			Default value
 *	@param[in]	lab			Default label
 **********************************************************************/
void	CAL_set_defaults

	(
	CAL		cal,
	STRING	sub,
	STRING	val,
	STRING	lab
	)

	{
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;

	al = (ATTRIB_LIST) cal;
	add_default_attributes(al, sub, val, lab);
	}

/******************************************************************************/

/**********************************************************************/
/** Get the defaults for a Category Attribute List structure.
 *	@param[in]	cal		Category Attribute List
 *	@param[out]	*sub	Default background value ?not sure?
 *	@param[out]	*val	Default value
 *	@param[out]	*lab	Default label
 **********************************************************************/
void	CAL_get_defaults

	(
	CAL		cal,
	STRING	*sub,
	STRING	*val,
	STRING	*lab
	)

	{
	ATTRIB_LIST	al;

	if ( IsNull(cal) ) return;

	al = (ATTRIB_LIST) cal;
	get_default_attributes(al, sub, val, lab);
	}

/***********************************************************************
*                                                                      *
*  C A L _ g e t _ a t t r i b u t e _ n a m e s                       *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Get the available attribute names from a Category Attribute List
 * structure.
 *	@param[in]	cal			Category Attribute List
 *	@param[out]	**names		List of attribute names
 *	@param[out]	*num		Size of names list
 **********************************************************************/
void	CAL_get_attribute_names

	(
	CAL		cal,
	STRING	**names,
	int		*num
	)

	{
	STRING	*list;
	int		na, ia;

	if ( NotNull(names) ) *names = NullStringPtr;
	if ( NotNull(num) )   *num   = 0;
	if ( IsNull(cal) )    return;
	na = cal->nattribs;
	if ( na <= 0 )        return;

	if ( NotNull(names) )
		{
		list = INITMEM(STRING, na);
		for (ia=0; ia<na; ia++)
			{
			list[ia] = cal->attribs[ia].name;
			}
		*names = list;
		}
	if ( NotNull(num) ) *num = na;
	}

/***********************************************************************
*                                                                      *
*  C A L _ m e r g e                                                   *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Merge two Category Attribute List structures together.
 *
 *	@param[in]	cal			Category Attribute List
 *	@param[in]	scal		These attributes will be added to cal
 *	@param[in]	over		Should we overwrite values in cal?
 **********************************************************************/
void	CAL_merge

	(
	CAL		cal,
	CAL		scal,
	LOGICAL	over
	)

	{
	int		i;
	STRING	val, sval, name;
	ATTRIB	*at;

	if (IsNull(scal)) return;
	if (IsNull(cal))  return;

	for (i=0; i<scal->nattribs; i++)
		{
		name = scal->attribs[i].name;
		sval = scal->attribs[i].value;
		if (CAL_no_value(sval)) continue;
		if (!over)
			{
			at = get_attribute(cal, name, &val);
			if ( NotNull(at) && CAL_is_value(val) ) continue;
			}

		(void) add_attribute(cal, name, sval);
		}
	}

/***********************************************************************
*                                                                      *
*  C A L _ s a m e                                                     *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Compare two Category Attribute List structures.
 *
 *	@param[in]	cal1			1st Category Attribute List
 *	@param[in]	cal2			2nd Category Attribute List
 * @return True if they are the same.
 **********************************************************************/
LOGICAL	CAL_same

	(
	CAL		cal1,
	CAL		cal2
	)

	{
	return same_attrib_list(cal1, cal2);
	}

/***********************************************************************
*                                                                      *
*  v a l i d _ f i e l d _ a t t r i b u t e                           *
*  v a l i d _ e d e f _ a t t r i b u t e                             *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Check if an attribute is valid for a field.
 *
 *	@param[in]	elem		Element name
 *	@param[in]	level		Level name
 *	@param[in]	name		Attribute name
 *  @return True if attribute is valid.
 **********************************************************************/
LOGICAL	valid_field_attribute
	(
	STRING	elem,
	STRING	level,
	STRING	name
	)

	{
	FpaConfigFieldStruct	*fdef;
	FpaConfigElementStruct	*edef;

	if ( blank(name) )  return FALSE;
	if ( blank(elem) )  elem  = FpaCanyElement;
	if ( blank(level) ) level = FpaCanyLevel;

	fdef = get_field_info(elem, level);
	edef = (fdef)? fdef->element: get_element_info(elem);

	return valid_edef_attribute(edef, name);
	}

/**********************************************************************/

/**********************************************************************/
/** Check if an attribute is valid for an element.
 *
 *	@param[in]	*edef	Element Structure to check
 *	@param[in]	name	Attribute name to check
 * 	@return True if attribute is valid.
 **********************************************************************/
LOGICAL	valid_edef_attribute
	(
	FpaConfigElementStruct	*edef,
	STRING					name
	)

	{
	int								i;
	FpaConfigElementAttribStruct	*attlist;

	if ( blank(name) )               return FALSE;
	if ( IsNull(edef) )              return FALSE;
	if ( IsNull(edef->elem_detail) )
		{
		edef = get_element_info(edef->name);
		if ( IsNull(edef) )          return FALSE;
		}

	attlist = edef->elem_detail->attributes;
	if ( IsNull(attlist) )           return FALSE;

	/* Find the attribute in the default list from the config file */
	for (i=0; i<attlist->nattribs; i++)
		{
		if ( same_ic(name, attlist->attrib_names[i]) ) return TRUE;
		}

	/* Not found */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*  c r e a t e _ s p o t _ b y _ n a m e                               *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Create a spot give an element name, level name, CAL,
 * and spot location.
 *
 *	@param[in]	elem		element name
 *	@param[in]	level		level name
 *	@param[in]	pos			spot location
 *	@param[in]	cal			attribute list structure
 * 	@return SPOT point created.
 **********************************************************************/
SPOT	create_spot_by_name
	(
	STRING	elem,
	STRING	level,
	POINT	pos,
	CAL		cal
	)

	{
	int					ii;
	STRING				value, mclass;
	FpaCattachOption	attach;
	SPFEAT				feature;
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementLabellingStruct		*labelling;

	/* Get the detailed field information */
	fdef = get_field_info(elem, level);
	if (IsNull(fdef)) return NullSpot;

	/* Identify "class" and "attach" based on the type of data */
	switch (fdef->element->fld_type)
		{

		/* Scattered fields use the scattered types block */
		case FpaC_SCATTERED:

			value = CAL_get_attribute(cal, CALscatteredtype);
			ii = identify_scattered_type_by_name(elem, level,
					value, &stypes);
			if (NotNull(stypes) && ii >= 0)
				{
				mclass = stypes->type_classes[ii];
				attach = stypes->type_attach_opts[ii];
				}
			else
				{
				mclass = FpaCdefaultScatteredTypesClass;
				attach = FpaC_NO_ATTACH;
				}
			break;

		/* All other fields use the labelling block */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_LCHAIN:
		default:

			value = CAL_get_attribute(cal, CALlabeltype);
			ii = identify_labelling_type_by_name(elem, level,
					value, &labelling);
			if (NotNull(stypes) && ii >= 0)
				{
				mclass = labelling->type_classes[ii];
				attach = labelling->type_attach_opts[ii];
				}
			else
				{
				mclass = FpaCdefaultLabellingTypesClass;
				attach = FpaC_NO_ATTACH;
				}
			break;
		}

	/* Interpret the attach option */
	if (!check_attach_option(fdef->element->fld_type, attach, &feature))
		return NullSpot;

	/* Create a SPOT object using the parameters for the spot */
	return create_spot(pos, mclass, feature, cal);
	}
