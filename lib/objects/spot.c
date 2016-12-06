/*********************************************************************/
/** @file spot.c
 *
 * Routines to handle SPOT objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s p o t . c                                                     *
*                                                                      *
*      Routines to handle the SPOT objects.                            *
*                                                                      *
*     Version 4 (c) Copyright 1997 Environment Canada (AES)            *
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

#define SPOT_INIT
#include "spot.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <string.h>

#undef DEBUG_SPOTS

int		SpotCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ s p o t                                           *
*      d e s t r o y _ s p o t                                         *
*      c o p y _ s p o t                                               *
*                                                                      *
*      Create/destroy/copy SPOT object.                                *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Create a new SPOT object.
 *
 *	@param[in] 	anchor		location of anchor point
 *	@param[in] 	class		spot class (label_type)
 *	@param[in] 	feature		spot feature attachment
 *	@param[in] 	attribs		attributes
 *  @return Pointer to new spot object. You will need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/
SPOT	create_spot

	(
	const POINT	anchor,
	STRING		class,
	SPFEAT		feature,
	ATTRIB_LIST	attribs
	)

	{
	SPOT	spot;

	/* Allocate space for the structure */
	spot = INITMEM(struct SPOT_struct, 1);
	if (IsNull(spot)) return NullSpot;

	/* Initialize the structure */
	spot->attrib  = NullAttribList;
	spot->mclass  = NULL;
	spot->feature = AttachNone;
	spot->members = NullSpmemList;
	spot->nmem    = 0;

	/* Set attributes and location */
	define_spot_anchor(spot, anchor);
	define_spot_class(spot, class);
	define_spot_feature(spot, feature);
	define_spot_attribs(spot, attribs);

	/* Return the new spot */
	SpotCount++;
	return spot;
	}

/**********************************************************************/

/*********************************************************************/
/** Destroy a SPOT object and return its memory to the system.
 *
 *	@param[in] 	spot	spot to be destroyed
 * 	@return NullSpot
 *********************************************************************/
SPOT	destroy_spot

	(
	SPOT	spot
	)

	{
	int		imem;

	/* Do nothing if spot not there */
	if (IsNull(spot)) return NullSpot;

	/* Free the space used by its members */
	for (imem=0; imem<spot->nmem; imem++)
		{
		free_spmem(spot->members + imem);
		}
	FREEMEM(spot->members);
	spot->nmem = 0;

	/* Free the space used by attributes */
	FREEMEM(spot->mclass);
	destroy_attrib_list(spot->attrib);

	/* Free the structure itself */
	FREEMEM(spot);
	SpotCount--;
	return NullSpot;
	}

/**********************************************************************/

/*********************************************************************/
/** Copy the given SPOT object.
 *
 *	@param[in] 	spot	spot to be copied
 * 	@return Pointer to a copy of the given object. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/
SPOT	copy_spot

	(
	const SPOT	spot
	)

	{
	SPOT	copy;
	SPMEM	*smem;
	int		imem;

	if (IsNull(spot)) return NullSpot;

	copy = create_spot(spot->anchor, spot->mclass, spot->feature, spot->attrib);

	for (imem=0; imem<spot->nmem; imem++)
		{
		smem = spot->members + imem;
		add_spot_member(copy, smem->name, SpotNone);
		copy_spmem(copy->members + imem, smem);
		}

	return copy;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p o t _ c l a s s                               *
*      r e c a l l _ s p o t _ c l a s s                               *
*                                                                      *
*      Set/reset or retrieve the class of the given spot.              *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the class of the given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	class	spot class
 *********************************************************************/
void	define_spot_class

	(
	SPOT	spot,
	STRING	class
	)

	{
	/* Do nothing if spot not there */
	if (IsNull(spot)) return;

	/* Set given class */
	spot->mclass = STRMEM(spot->mclass, class);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the class of the given spot.
 *
 *	@param[in] 	spot	requested spot
 *	@param[out]	*class	spot class
 *********************************************************************/
void	recall_spot_class

	(
	SPOT	spot,
	STRING	*class
	)

	{
	/* Retrieve the attributes */
	if (class) *class = (spot) ? spot->mclass : NULL;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p o t _ f e a t u r e                           *
*      r e c a l l _ s p o t _ f e a t u r e                           *
*                                                                      *
*      Set/reset or retrieve the feature attachment of the given spot. *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the feature attachment of the given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	feature	spot feature
 *********************************************************************/
void	define_spot_feature

	(
	SPOT	spot,
	SPFEAT	feature
	)

	{
	/* Do nothing if spot not there */
	if (IsNull(spot)) return;

	/* Set given feature */
	spot->feature = feature;
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the feature attachment of the given spot.
 *
 *	@param[in] 	spot		requested spot
 *	@param[out]	*feature	spot feature
 *********************************************************************/
void	recall_spot_feature

	(
	SPOT	spot,
	SPFEAT	*feature
	)

	{
	/* Retrieve the attributes */
	if (feature) *feature = (spot) ? spot->feature : AttachNone;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p o t _ a t t r i b s                           *
*      r e c a l l _ s p o t _ a t t r i b s                           *
*                                                                      *
*      Set/reset or retrieve the attributes of the given spot.         *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the attributes of the given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	attribs	attributes
 *********************************************************************/
void	define_spot_attribs

	(
	SPOT		spot,
	ATTRIB_LIST	attribs
	)

	{
	/* Do nothing if spot not there */
	if (IsNull(spot)) return;

	/* Set given attributes */
	spot->attrib = destroy_attrib_list(spot->attrib);
	if (NotNull(attribs)) spot->attrib = copy_attrib_list(attribs);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given spot.
 *
 *	@param[in] 	spot		requested spot
 *	@param[in] 	*attribs	attributes
 *********************************************************************/
void	recall_spot_attribs

	(
	SPOT		spot,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (spot) ? spot->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p o t _ a n c h o r                             *
*      r e c a l l _ s p o t _ a n c h o r                             *
*                                                                      *
*      Set/reset or retrieve the location of the given spot.           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the location of the given spot.
 *********************************************************************/
void	define_spot_anchor

	(
	SPOT		spot,		/* given spot */
	const POINT	anchor		/* spot location */
	)

	{
	if (IsNull(spot)) return;

	/* Set all attributes */
	if (anchor) copy_point(spot->anchor, anchor);
	else        copy_point(spot->anchor, ZeroPoint);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the location of the given spot.
 *
 *	@param[in] 	spot		given spot
 *	@param[out]	anchor		spot location
 *********************************************************************/
void	recall_spot_anchor

	(
	SPOT	spot,
	POINT	anchor
	)

	{
	/* Retrieve all the attributes */
	if (anchor)
		{
		if (NotNull(spot)) copy_point(anchor, spot->anchor);
		else               copy_point(anchor, ZeroPoint);
		}
	}

/***********************************************************************
*                                                                      *
*      a d d _ s p o t _ m e m b e r                                   *
*      r e m o v e _ s p o t _ m e m b e r                             *
*      w h i c h _ s p o t _ m e m b e r                               *
*      b u i l d _ s p o t _ m e m b e r s                             *
*                                                                      *
*      Add/remove members in the given spot.                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Add a spot member to the given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	name	name of member to add
 *	@param[in] 	type	type of member
 *********************************************************************/
void	add_spot_member

	(
	SPOT	spot,
	STRING	name,
	SPTYPE	type
	)

	{
	int		imem;
	SPMEM	*mem;

	if (IsNull(spot)) return;

	imem = which_spot_member(spot, name, &mem);
	if (IsNull(mem))
		{
		imem = spot->nmem++;
		spot->members = GETMEM(spot->members, SPMEM, spot->nmem);
		mem = spot->members + imem;
		init_spmem(mem);
		}
	define_spmem_type(mem, name, type);
	}

/**********************************************************************/

/*********************************************************************/
/** Remove  a spot member from the given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	name	name of member to remove
 *********************************************************************/
void	remove_spot_member

	(
	SPOT	spot,
	STRING	name
	)

	{
	int		imem;
	SPMEM	*mem;

	if (IsNull(spot)) return;

	imem = which_spot_member(spot, name, &mem);
	if (IsNull(mem)) return;

	/* Compress the list */
	spot->nmem--;
	for ( ; imem<spot->nmem; imem++)
		{
		copy_spmem(mem, mem+1);
		mem++;
		}
	free_spmem(mem);
	}

/**********************************************************************/

/*********************************************************************/
/** Lookup a particular spot member of a given spot.
 *
 *	@param[in] 	spot	given spot
 *	@param[in] 	name	name of member to lookup
 *	@param[out]	**mem	return a pointer to the spot member object
 * 	@return The index of the spot member in the member list. <0 if
 * 			failed.
 *********************************************************************/
int		which_spot_member

	(
	SPOT	spot,
	STRING	name,
	SPMEM	**mem
	)

	{
	int		imem;
	SPMEM	*cmem;

	if (NotNull(mem)) *mem = NullSpmemPtr;

	if (IsNull(spot)) return -1;
	if (blank(name))  return -1;

	for (imem=0; imem<spot->nmem; imem++)
		{
		cmem = spot->members + imem;
		if (!same(name, cmem->name)) continue;

		if (NotNull(mem)) *mem = cmem;
		return imem;
		}

	return -1;
	}

/**********************************************************************/
/**  Build spot members
 * 	@param[in]	spot	Spot to build
 * 	@param[in]	ncspec	Number of category specifications
 * 	@param[in]	*cspecs	List of category specifications
 **********************************************************************/

void	build_spot_members

	(
	SPOT			spot,
	int				ncspec,
	const CATSPEC	*cspecs
	)

	{
	int		isp, imem;
	STRING	pname;
	CATSPEC	*cs;
	SPMEM	*mem;
	SPTYPE	mtype;
	LOGICAL	found;

	if (IsNull(spot)) return;
	if (ncspec <= 0)  return;

	/* See that spot has members to match all "spot_members" from catspecs */
	pname = NULL;
	found = FALSE;
	for (isp=0; isp<ncspec; isp++)
		{
		cs = (CATSPEC *) cspecs + isp;

		/* Only apply to the first unique member of the matching class */
		/* Note that this saves a placeholder for presentation */
		/*  ... the actual presentation is defined later!      */
		if (!same(cs->mclass, spot->mclass)) continue;
		found = TRUE;
		if (same(cs->name, pname))           continue;
		pname = cs->name;

		/* See if spot already has this member */
		imem = which_spot_member(spot, cs->name, 0);
		if (imem >= 0) continue;

		/* Check member type */
		mtype = spmem_type(cs->type);
		if (mtype < 0)
			{
			(void) pr_error("Spots",
						"Unrecognized spot_member type \"%s\"\n", cs->type);
			continue;
			}

#		ifdef DEBUG_SPOTS
		(void) pr_info("Spots",
					"Adding spot member: %s %s %s  for class: %s\n",
					cs->type, cs->name,
					(!blank(cs->attrib)? cs->attrib: "-"), cs->mclass);
#		endif /* DEBUG_SPOTS */

		/* Add the member */
		add_spot_member(spot, cs->name, mtype);
		mem = spot->members + spot->nmem - 1;
		if (!blank(cs->attrib)) define_spmem_attrib(mem, cs->attrib);
		}

	/* Complain if no presentation was found */
	if (spot->nmem <= 0)
		{
		if (!found) (void) pr_error("Spots",
					"No presentation for spot class \"%s\"\n", spot->mclass);
		else        (void) pr_error("Spots",
					"No spot_members for spot class \"%s\"\n", spot->mclass);
		}
	}

/***********************************************************************
*                                                                      *
*      i n i t _ s p m e m                                             *
*      c o p y _ s p m e m                                             *
*      f r e e _ s p m e m                                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Initialize a spot member object.
 *
 *	@param[in] 	*mem	spot member to initialize
 *********************************************************************/
void	init_spmem

	(
	SPMEM	*mem
	)

	{
	if (IsNull(mem)) return;

	mem->name   = NULL;
	mem->type   = SpotNone;
	mem->attrib = NULL;
	mem->fmt    = NULL;

	copy_point(mem->offset, ZeroPoint);
	init_tspec(&mem->tspec);
	init_mspec(&mem->mspec);
	init_bspec(&mem->bspec);
	}

/**********************************************************************/

/*********************************************************************/
/** Copy the given spot member object.
 *
 *	@param[out]	*mem	copy
 *	@param[in]  	*smem	original
 *********************************************************************/
void	copy_spmem

	(
	SPMEM		*mem,
	const SPMEM	*smem
	)

	{
	if (IsNull(mem)) return;

	mem->name   = STRMEM(mem->name,   smem->name);
	mem->type   = smem->type;
	mem->attrib = STRMEM(mem->attrib, smem->attrib);
	mem->fmt    = STRMEM(mem->fmt,    smem->fmt);

	copy_point(mem->offset, smem->offset);
	copy_tspec(&mem->tspec, &smem->tspec);
	copy_mspec(&mem->mspec, &smem->mspec);
	copy_bspec(&mem->bspec, &smem->bspec);
	}

/**********************************************************************/

/*********************************************************************/
/** Return the memory for the given spot member to the system.
 *
 *	@param[in] 	*mem	spot member to destroy
 *********************************************************************/
void	free_spmem

	(
	SPMEM	*mem
	)

	{
	if (IsNull(mem)) return;

	FREEMEM(mem->name);
	mem->type = SpotNone;
	FREEMEM(mem->attrib);
	FREEMEM(mem->fmt);

	free_tspec(&mem->tspec);
	free_mspec(&mem->mspec);
	free_bspec(&mem->bspec);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p m e m _ t y p e                               *
*      d e f i n e _ s p m e m _ a t t r i b                           *
*      d e f i n e _ s p m e m _ o f f s e t                           *
*      d e f i n e _ s p m e m _ o f f s e t _ x y                     *
*                                                                      *
*      Define member type, attributes, or offsets.                     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Define member type.
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	name	member name to define
 *	@param[in] 	type	member type to define
 *********************************************************************/
void	define_spmem_type

	(
	SPMEM	*mem,
	STRING	name,
	SPTYPE	type
	)

	{
	if (IsNull(mem)) return;

	mem->name = STRMEM(mem->name,  name);

	/* Make sure type is legal */
	switch (type)
		{
		case SpotText:
		case SpotFval:
		case SpotIval:
		case SpotMark:
		case SpotBarb:	break;
		default:		type = SpotNone;
		}
	mem->type = type;
	}

/**********************************************************************/

/*********************************************************************/
/** Define spot member attribute
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	attrib	name of attribute
 *********************************************************************/
void	define_spmem_attrib

	(
	SPMEM	*mem,
	STRING	attrib
	)

	{
	if (IsNull(mem)) return;

	mem->attrib = STRMEM(mem->attrib, attrib);
	}

/**********************************************************************/


/*********************************************************************/
/** Define spot member offset
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	offset	x-y offset of spot
 *********************************************************************/
void	define_spmem_offset

	(
	SPMEM	*mem,
	POINT	offset
	)

	{
	if (IsNull(mem)) return;

	if (offset) copy_point(mem->offset, offset);
	else        copy_point(mem->offset, ZeroPoint);
	}

/**********************************************************************/

/*********************************************************************/
/** Define spot member offset as xy-pair.
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	xoff	x offset
 *	@param[in] 	yoff	y offset
 *********************************************************************/
void	define_spmem_offset_xy

	(
	SPMEM	*mem,
	float	xoff,
	float	yoff
	)

	{
	if (IsNull(mem)) return;

	set_point(mem->offset, xoff, yoff);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s p m e m _ p s p e c s                           *
*      r e c a l l _ s p m e m _ p s p e c s                           *
*      d e f i n e _ s p m e m _ p s p e c                             *
*      r e c a l l _ s p m e m _ p s p e c                             *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given       *
*      spot member.                                                    *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset spot member presentation specs.
 *
 * The spot member makes of copy of the given spec objects for itself.
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	*tspec	Text specs
 *	@param[in] 	*mspec	min/max/saddle point display specs
 *	@param[in] 	*bspec	barb presentation specs
 *********************************************************************/
void	define_spmem_pspecs

	(
	SPMEM	*mem,
	TSPEC	*tspec,
	MSPEC	*mspec,
	BSPEC	*bspec
	)

	{
	if (IsNull(mem)) return;

	if (NotNull(tspec)) copy_tspec(&mem->tspec, tspec);
	if (NotNull(mspec)) copy_mspec(&mem->mspec, mspec);
	if (NotNull(bspec)) copy_bspec(&mem->bspec, bspec);
	}

/**********************************************************************/


/*********************************************************************/
/** Retrieve spot member presentation specs.
 *
 *	@param[in] 	*mem		spot member
 *	@param[out]	**tspec		text specs
 *	@param[out]	**mspec		min/max/saddle point display specs
 *	@param[out]	**bspec		barb presentation specs
 *********************************************************************/
void	recall_spmem_pspecs

	(
	SPMEM	*mem,
	TSPEC	**tspec,
	MSPEC	**mspec,
	BSPEC	**bspec
	)

	{
	if (IsNull(mem)) return;

	if (NotNull(tspec)) *tspec = &mem->tspec;
	if (NotNull(mspec)) *mspec = &mem->mspec;
	if (NotNull(bspec)) *bspec = &mem->bspec;
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset a particular presentation spec.
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	param	parameter to set
 *	@param[in] 	value	value to set
 *********************************************************************/
void	define_spmem_pspec

	(
	SPMEM	*mem,
	PPARAM	param,
	POINTER	value
	)

	{
	if (IsNull(mem)) return;

	/* Set the given parameter */
	define_tspec_value(&mem->tspec, param, value);
	define_mspec_value(&mem->mspec, param, value);
	define_bspec_value(&mem->bspec, param, value);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve a particular presentation spec.
 *
 *	@param[in] 	*mem	spot member
 *	@param[in] 	param	parameter to lookup
 *	@param[out]	value	value returned
 *********************************************************************/
void	recall_spmem_pspec

	(
	SPMEM	*mem,
	PPARAM	param,
	POINTER	value
	)

	{
	if (IsNull(mem)) return;

	/* Return the requested parameter */
	recall_tspec_value(&mem->tspec, param, value);
	recall_mspec_value(&mem->mspec, param, value);
	recall_bspec_value(&mem->bspec, param, value);
	}

/***********************************************************************
*                                                                      *
*      s p o t _ f e a t u r e                                         *
*      s p o t _ f e a t u r e _ s t r i n g                           *
*      s p m e m _ t y p e                                             *
*      s p m e m _ t y p e _ s t r i n g                               *
*                                                                      *
***********************************************************************/

static	const	ITBL	Flist[] =
			{
				{ AttachNone,    "none"    },
				{ AttachAuto,    "auto"    },
				{ AttachContour, "contour" },
				{ AttachMax,     "max"     },
				{ AttachMin,     "min"     },
				{ AttachCol,     "col"     },
				{ AttachBound,   "bound"   },
				{ AttachDiv,     "div"     },
				{ AttachLine,    "line"    },
				{ AttachPoint,   "point"   },
			};
static	const	int	NumFlist = ITBL_SIZE(Flist);

static	const	ITBL	Tlist[] =
			{
				{ SpotNone, "none"   },
				{ SpotText, "text"   },
				{ SpotText, "string" },
				{ SpotText, "label"  },
				{ SpotFval, "float"  },
				{ SpotIval, "int"    },
				{ SpotMark, "mark"   },
				{ SpotMark, "marker" },
				{ SpotBarb, "barb"   },
				{ SpotBarb, "wind"   },
			};
static	const	int	NumTlist = ITBL_SIZE(Tlist);

/**********************************************************************/

/*********************************************************************/
/** Retrieve a spot feature (attach code) by its name.
 *
 *	@param[in] 	fname	name of feature to retrieve
 * 	@return Attach code for given feature. AttachNone if no other code
 * 			can be found.
 *********************************************************************/
SPFEAT	spot_feature

	(
	STRING	fname
	)

	{
	SPFEAT	fcode;

	fcode = find_itbl_entry(Flist, NumFlist, fname);
	if (fcode >= 0) return fcode;
	return AttachNone;
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the string associated with the given attach code.
 *
 *	@param[in] 	fcode	spot feature to lookup
 *  @return Pointer to spot feature name.
 *********************************************************************/
STRING	spot_feature_string

	(
	SPFEAT	fcode
	)

	{
	STRING	fname;

	fname = find_itbl_string(Flist, NumFlist, fcode);
	if (NotNull(fname)) return fname;
	return find_itbl_string(Flist, NumFlist, AttachNone);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve a spot type object by its name.
 *
 *	@param[in] 	tname	type name to lookup
 *  @return Pointer to spot type object.
 *********************************************************************/
SPTYPE	spmem_type

	(
	STRING	tname
	)

	{
	SPTYPE	tcode;

	tcode = find_itbl_entry(Tlist, NumTlist, tname);
	if (tcode >= 0) return tcode;
	return SpotNone;
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve a spot type's name.
 *
 *	@param[in] 	tcode	type object to lookup
 *	@return spot type name
 *********************************************************************/
STRING	spmem_type_string

	(
	SPTYPE	tcode
	)

	{
	STRING	tname;

	tname = find_itbl_string(Tlist, NumTlist, tcode);
	if (NotNull(tname)) return tname;
	return find_itbl_string(Tlist, NumTlist, SpotNone);
	}
