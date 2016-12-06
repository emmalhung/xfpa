/*********************************************************************/
/** @file dispnode.h
 *
 * DISPNODE object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    d i s p n o d e . h                                               *
*                                                                      *
*    DISPNODE object definitions (include file)                        *
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
#ifndef DISPNODE_DEFS
#define DISPNODE_DEFS

/* We need definitions for various objects */
#include "metafile.h"
#include "pspec.h"

/* Define DISPNODE */
/** type descriptors */
typedef	enum	{ DnNone, DnMeta, DnSfc, DnSet, DnPlot, DnSpecial }
		DNTYPE;
/** xform descriptors */
typedef	enum	{ DxNone, DxRoot, DxWindow, DxMap, DxGen, DxIdent }
		DXTYPE;

/** Define DISPNODE object */
typedef struct DISPNODE_struct
	{
	/* Subtree */
	struct DISPNODE_struct	*parent;	/**< parent dispnode */
	struct DISPNODE_struct	**kids;		/**< children */
	int				numkids;			/**< number of kids */
	int				maxkids;			/**< allocated kids */

	/** Data */
	DNTYPE	dntype;						/**< what type? */
	STRING	sptype;						/**< what special type? */
	union	{
			METAFILE	meta;			/**< metafile data */
			SURFACE		sfc;			/**< scalar field data */
			SET			set;			/**< set data */
			PLOT		plot;			/**< plot data */
			POINTER		ptr;			/**< special data */
			} data;						/**< actual data */

	/* Transform */
	DXTYPE	dxtype;						/**< what kind of transform? */
	BOX			viewport;				/**< viewport def */
	BOX			window;					/**< window def */
	MAP_PROJ	mproj;					/**< map projection */
	XFORM		xform;					/**< transform matrix */

	/* Background */
	COLOUR	vfill;						/**< viewport fill colour */
	COLOUR	vedge;						/**< viewport edge colour */
	COLOUR	wfill;						/**< window fill colour */
	COLOUR	wedge;						/**< window edge colour */

	/** Visibility flag */
	LOGICAL	shown;

	/** Optional raster snapshot buffer */
	int		snap;
	} *DISPNODE;

/* Convenient definitions */
#define DELTA_DISPNODE 1
#define NullDn        NullPtr(DISPNODE)
#define NullDnPtr     NullPtr(DISPNODE *)
#define NullDnList    NullPtr(DISPNODE *)
#define NullDnListPtr NullPtr(DISPNODE **)

/* Declare all functions in dispnode.c */
DISPNODE	create_dispnode(void);
DISPNODE	destroy_dispnode(DISPNODE dn);
void		touch_dispnode(DISPNODE dn);
void		add_dn_to_subtree(DISPNODE dn, DISPNODE parent);
void		delete_dn_subtree(DISPNODE dn);
void		recall_dn_subtree(DISPNODE dn,
						DISPNODE **kids, int *numkids, int *maxkids);
void		define_dn_data(DISPNODE dn, STRING type, POINTER data);
void		delete_dn_data(DISPNODE dn);
void		recall_dn_data(DISPNODE dn, STRING *type, POINTER *data);
void		define_dn_xform(DISPNODE dn, STRING type,
						const BOX *viewport, const BOX *window,
						const MAP_PROJ *mproj, const XFORM xform);
void		recall_dn_xform(DISPNODE dn, STRING *type,
						BOX *viewport, BOX *window,
						MAP_PROJ *mproj, XFORM xform);
void		move_dispnode(DISPNODE dn,
						float x, float y, HJUST hjust, VJUST vjust);
void		size_dispnode(DISPNODE dn, float sx, float sy);
void		define_dn_bgnd(DISPNODE dn, COLOUR vfill, COLOUR vedge,
						COLOUR wfill, COLOUR wedge);
void		recall_dn_bgnd(DISPNODE dn, COLOUR *vfill, COLOUR *vedge,
						COLOUR *wfill, COLOUR *wedge);
void		define_dn_vis(DISPNODE dn, LOGICAL shown);
void		recall_dn_vis(DISPNODE dn, LOGICAL *shown);
LOGICAL		inside_dn_viewport(DISPNODE dn, POINT p);
LOGICAL		inside_dn_viewport_xy(DISPNODE dn, float x, float y);
LOGICAL		inside_dn_window(DISPNODE dn, POINT p);
LOGICAL		inside_dn_window_xy(DISPNODE dn, float x, float y);
DISPNODE	init_panel(DISPNODE dn, STRING ptype,
						const BOX *viewport, const BOX *window,
						STRING vfill, STRING vedge, STRING wfill, STRING wedge);


/* Now it has been included */
#endif
