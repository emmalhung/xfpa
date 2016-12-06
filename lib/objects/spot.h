/*********************************************************************/
/** @file spot.h
 *
 * SPOT object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    s p o t . h                                                       *
*                                                                      *
*    SPOT object definitions (include file)                            *
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
#ifndef SPOT_DEFS
#define SPOT_DEFS

/* We need definitions for other objects */
#include "pspec.h"
#include "attrib.h"

/** Define SPTYPE - SPOT member type */
typedef	enum
	{
	SpotNone,	/**< ignore */
	SpotText,	/**< string label */
	SpotFval,	/**< float number as a string label */
	SpotIval,	/**< integer number as a string label */
	SpotMark,	/**< marker */
	SpotBarb	/**< wind or velocity barb */
	} SPTYPE;

/** Define SPMEM object - subfield/member of a SPOT object */
typedef	struct SPMEM_struct
	{
	STRING	name;		/**< member name */
	SPTYPE	type;		/**< type - text, mark, barb, etc. */
	STRING	attrib;		/**< name of attribute to use */
	STRING	fmt;		/**< format instructions */
	POINT	offset;		/**< offset from spot anchor */
	TSPEC	tspec;		/**< how to display text */
	MSPEC	mspec;		/**< how to display mark */
	BSPEC	bspec;		/**< how to display barb */
	} SPMEM;

/** Define SPFEAT - feature that a SPOT label attaches to */
typedef	enum
	{
	AttachNone,		/**< anchor goes at pointer and stays where it was put */
	AttachAuto,		/**< software decides which feature to attach to */
	AttachContour,	/**< anchor goes at nearest contour */
	AttachMax,		/**< anchor goes at nearest maximum */
	AttachMin,		/**< anchor goes at nearest minimum */
	AttachCol,		/**< anchor goes at nearest col (saddle) */
	AttachBound,	/**< anchor goes at nearest area boundary */
	AttachDiv,		/**< anchor goes at nearest area dividing line */
	AttachLine,		/**< anchor goes at nearest line */
	AttachPoint		/**< anchor goes at nearest point */
	} SPFEAT;

/** Define SPOT object - a generic label/annotation object with given location */
typedef struct SPOT_struct
	{
	POINT		anchor;		/**< location of anchor point */
	STRING		mclass;		/**< spot member class */
	SPFEAT		feature;	/**< feature attachment */
	ATTRIB_LIST	attrib;		/**< attribute list */
	SPMEM		*members;	/**< list of spot members */
	int			nmem;		/**< number of members */
	} *SPOT;

/* Convenient definitions */
#define NullSpmem        NullPtr(SPMEM)
#define NullSpmemPtr     NullPtr(SPMEM *)
#define NullSpmemList    NullPtr(SPMEM *)
#define NullSpmemListPtr NullPtr(SPMEM **)
#define NullSpot         NullPtr(SPOT)
#define NullSpotPtr      NullPtr(SPOT *)
#define NullSpotList     NullPtr(SPOT *)
#define NullSpotListPtr  NullPtr(SPOT **)

/* Declare all functions in spot.c */
SPOT	create_spot(const POINT anchor, STRING class, SPFEAT feature,
						ATTRIB_LIST attribs);
SPOT	destroy_spot(SPOT spot);
SPOT	copy_spot(const SPOT spot);
void	define_spot_class(SPOT spot, STRING class);
void	recall_spot_class(SPOT spot, STRING *class);
void	define_spot_feature(SPOT spot, SPFEAT feature);
void	recall_spot_feature(SPOT spot, SPFEAT *feature);
void	define_spot_attribs(SPOT spot, ATTRIB_LIST attribs);
void	recall_spot_attribs(SPOT spot, ATTRIB_LIST *attribs);
void	define_spot_anchor(SPOT spot, const POINT anchor);
void	recall_spot_anchor(SPOT spot, POINT anchor);
void	add_spot_member(SPOT spot, STRING name, SPTYPE type);
void	remove_spot_member(SPOT spot, STRING name);
int		which_spot_member(SPOT spot, STRING name, SPMEM **mem);
void	build_spot_members(SPOT spot, int ncspec, const CATSPEC *cspecs);
void	init_spmem(SPMEM *mem);
void	copy_spmem(SPMEM *mcopy, const SPMEM *mem);
void	free_spmem(SPMEM *mem);
void	define_spmem_type(SPMEM *mem, STRING name, SPTYPE type);
void	define_spmem_attrib(SPMEM *mem, STRING attrib);
void	define_spmem_offset(SPMEM *mem, POINT offset);
void	define_spmem_offset_xy(SPMEM *mem, float xoff, float yoff);
void	define_spmem_pspecs(SPMEM *mem,
						TSPEC *tspec, MSPEC *mspec, BSPEC *bspec);
void	recall_spmem_pspecs(SPMEM *mem,
						TSPEC **tspec, MSPEC **mspec, BSPEC **bspec);
void	define_spmem_pspec(SPMEM *mem, PPARAM param, POINTER value);
void	recall_spmem_pspec(SPMEM *mem, PPARAM param, POINTER value);
SPFEAT	spot_feature(STRING fname);
STRING	spot_feature_string(SPFEAT feature);
SPTYPE	spmem_type(STRING tname);
STRING	spmem_type_string(SPTYPE type);

/* Functions in spot_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
