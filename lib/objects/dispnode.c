/*********************************************************************/
/**	@file dispnode.c
 *
 * Routines to handle the DISPNODE object
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      d i s p n o d e . c                                             *
*                                                                      *
*      Routines to handle the DISPNODE object.                         *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#define DISPNODE_INIT
#include "dispnode.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

int		DispnodeCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ d i s p n o d e                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new dispnode with given parent dispnode.
 *
 * @return Pointer to new dispnode object. You need to destroy this
 * object when you are finished with it.
 *********************************************************************/

DISPNODE	create_dispnode(void)

	{
	DISPNODE	dn;

	/* Allocate memory for structure */
	dn = INITMEM(struct DISPNODE_struct,1);
	if (!dn) return NullDn;

	/* Initialize an empty raster */
	dn->snap = 0;

	/* Initialize subtree */
	dn->parent  = NullDn;
	dn->kids    = NullDnList;
	dn->numkids = 0;
	dn->maxkids = 0;

	/* Set node type and initialize data */
	dn->dntype    = DnNone;
	dn->sptype    = NullString;
	dn->data.meta = NullMeta;
	dn->data.sfc  = NullSfc;
	dn->data.set  = NullSet;
	dn->data.plot = NullPlot;
	define_dn_data(dn,"none",NULL);

	/* Set node's default transformation */
	define_dn_xform(dn,"identity",NullBox,NullBox,NullMapProj,NullXform);

	/* Set node's background attributes */
	define_dn_bgnd(dn,SkipColour,SkipColour,SkipColour,SkipColour);

	/* Set the node's flags */
	define_dn_vis(dn,TRUE);

	/* Return the new dispnode */
	DispnodeCount++;
	return dn;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ d i s p n o d e                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy the given dispnode.  If it has children, destroy them
 * first. 
 *
 *	@param[in] 	dn	the given dispnode
 *  @return NullDn
 *********************************************************************/

DISPNODE	destroy_dispnode

	(
	DISPNODE	dn
	)

	{
	/* Do nothing if dn is NULL */
	if (!dn) return NullDn;

	/* Free the data storage */
	delete_dn_subtree(dn);
	delete_dn_data(dn);

	/* Now free the dispnode storage itself */
	FREEMEM(dn);
	DispnodeCount--;
	return NullDn;
	}

/***********************************************************************
*                                                                      *
*      t o u c h _ d i s p n o d e                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set the flags of the given dispnode to force redrawing.
 *
 *	@param[in]  dn	the given dispnode
 *********************************************************************/

void		touch_dispnode

	(
	DISPNODE    dn
	)

	{
	int	i;

	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Set redraw flag of node and its children */
	dn->shown = TRUE;
	for (i=0; i<dn->numkids; i++)
		touch_dispnode(dn->kids[i]);
	}

/***********************************************************************
*                                                                      *
*      a d d _ d n _ t o _ s u b t r e e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Add the given dispnode to the subtree of the given parent
 * dispnode.
 *
 *	@param[in] 	dn		given dispnode
 *	@param[in] 	parent	desired parent
 *********************************************************************/

void		add_dn_to_subtree

	(
	DISPNODE	dn,
	DISPNODE	parent
	)

	{
	DISPNODE	pn;
	int			i, m, n;

	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Remove from original parent (if different from new one) */
	pn = dn->parent;
	if (pn == parent) return;
	if (pn)	{

		/* Search for given dispnode in subtree */
		for (i=0; i < pn->numkids; i++)
			if (pn->kids[i] == dn) break;

		/* If found remove it and move siblings ahead */
		if (i < pn->numkids)
			{
			pn->numkids--;
			for (; i < pn->numkids; i++)
				pn->kids[i] = pn->kids[i+1];
			pn->kids[pn->numkids] = NullDn;
			}
		}

	/* Attach to new parent */
	dn->parent = parent;
	if (!parent) return;

	/* Add to children of new parent */
	n = parent->numkids;
	m = parent->maxkids;
	if (n >= m)
		{
		m = parent->maxkids += DELTA_DISPNODE;
		parent->kids = GETMEM(parent->kids,DISPNODE,m);
		}
	parent->numkids++;
	parent->kids[n] = dn;

	/* Set up default transform */
	define_dn_xform(dn,"identity",NullBox,NullBox,NullMapProj,NullXform);
	}

/***********************************************************************
*                                                                      *
*      d e l e t e _ d n _ s u b t r e e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Delete all the child nodes of the given dispnode.
 *
 *	@param[in] 	dn	the specified dispnode (parent)
 *********************************************************************/

void		delete_dn_subtree

	(
	DISPNODE	dn
	)

	{
	int	i;

	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Free all the children */
	for (i=0; i < dn->numkids; i++)
		dn->kids[i] = destroy_dispnode(dn->kids[i]);

	/* Free child buffer and reset number of children */
	FREEMEM(dn->kids);
	dn->numkids = 0;
	dn->maxkids = 0;
	}

/***********************************************************************
*                                                                      *
*      r e c a l l _ d n _ s u b t r e e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the subtree information of the given dispnode.
 *
 *	@param[in] 	dn			specified dispnode
 *	@param[out] 	**kids		child buffer
 *	@param[out] 	*numkids	number of children
 *	@param[out] 	*maxkids	allocated children
 *********************************************************************/
void		recall_dn_subtree

	(
	DISPNODE	dn,
	DISPNODE	**kids,
	int			*numkids,
	int			*maxkids
	)

	{
	/* Return the requested parameters */
	*kids    = (dn) ? dn->kids    : NullDnList;
	*numkids = (dn) ? dn->numkids : 0;
	*maxkids = (dn) ? dn->maxkids : 0;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ d n _ d a t a                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Define the given dispnode as the specified type, and provide
 * the corresponding data.
 *
 *	@param[in] 	dn		the given dispnode
 *	@param[in] 	type	specified data type
 *	@param[in] 	data	the data
 *********************************************************************/

void		define_dn_data

	(
	DISPNODE	dn,
	STRING		type,
	POINTER		data
	)

	{
	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Delete original data */
	delete_dn_data(dn);
	if (!data) return;

	/* Set new data type */
	if (same(type,"metafile"))
		{
		dn->dntype     = DnMeta;
		dn->data.meta  = (METAFILE) data;
		}

	else if (same(type,"surface"))
		{
		dn->dntype    = DnSfc;
		dn->data.sfc  = (SURFACE) data;
		}

	else if (same(type,"set"))
		{
		dn->dntype    = DnSet;
		dn->data.set  = (SET) data;
		}

	else if (same(type,"plot"))
		{
		dn->dntype     = DnPlot;
		dn->data.plot  = (PLOT) data;
		}

	else if (same_start(type,"special"))
		{
		dn->dntype     = DnSpecial;
		dn->data.ptr   = data;
		if (type[7] == ':')
			{
			dn->sptype = strdup(type+8);
			}
		}

	else return;	/* Otherwise leave data empty */
	}

/***********************************************************************
*                                                                      *
*      d e l e t e _ d n _ d a t a                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Free the space allocated to the given dispnode's data.
 *
 *	@param[in] 	dn	the given dispnode
 *********************************************************************/

void		delete_dn_data

	(
	DISPNODE	dn
	)

	{
	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Free the data */
	switch (dn->dntype)
		{
		case DnMeta:	dn->data.meta = destroy_metafile(dn->data.meta);
						break;

		case DnSfc:		dn->data.sfc  = destroy_surface(dn->data.sfc);
						break;

		case DnSet:		dn->data.set  = destroy_set(dn->data.set);
						break;

		case DnPlot:	dn->data.plot = destroy_plot(dn->data.plot);
						break;

		case DnSpecial:	dn->data.ptr  = 0;	/* Must be freed elsewhere! */
						FREEMEM(dn->sptype);
						break;
		}

	/* Reset type to no data */
	dn->dntype = DnNone;
	}

/***********************************************************************
*                                                                      *
*      r e c a l l _ d n _ d a t a                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the displayable data of the given dispnode.
 *
 *	@param[in] 	dn		the given dispnode
 *	@param[out] 	*type	specified data type
 *	@param[out] 	*data	the data
 *********************************************************************/

void		recall_dn_data

	(
	DISPNODE	dn,
	STRING		*type,
	POINTER		*data
	)

	{
	static	char	tbuf[30] = "";

	/* Do nothing if dn is NULL */
	if (!dn)
		{
		*type = NULL;
		*data = NULL;
		return;
		}

	switch (dn->dntype)
		{
		case DnMeta:	*type = "metafile";
						*data = (POINTER) dn->data.meta;
						break;

		case DnSfc:		*type = "surface";
						*data = (POINTER) dn->data.sfc;
						break;

		case DnSet:		*type = "set";
						*data = (POINTER) dn->data.set;
						break;

		case DnPlot:	*type = "plot";
						*data = (POINTER) dn->data.plot;
						break;

		case DnSpecial:	(void) strcpy(tbuf, "special:");
						(void) strcat(tbuf, dn->sptype);
						*type = tbuf;
						*data = dn->data.ptr;
						break;

		default:		*type = "none";
						*data = NullPointer;
		}
	}


/***********************************************************************
*                                                                      *
*      d e f i n e _ d n _ x f o r m                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Define the dispnode transform in terms of the given type and
 *      given viewport, window and map projection information. 
 *
 *	@param[in] 	dn			The given dispnode
 *	@param[in] 	type		transform type
 *	@param[in] 	*viewport	Viewport description
 *	@param[in] 	*window		Window description
 *	@param[in] 	*mproj		Map projection ("map" only)
 *	@param[in] 	xform		Actual transform ("general" only)
 *********************************************************************/

void		define_dn_xform

	(
	DISPNODE		dn,
	STRING			type,
	const BOX		*viewport,
	const BOX		*window,
	const MAP_PROJ	*mproj,
	const XFORM		xform
	)

	{
	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Define the transform type */
	if (same(type,"root"))          dn->dxtype = DxRoot;
	else if (same(type,"window"))   dn->dxtype = DxWindow;
	else if (same(type,"map"))      dn->dxtype = DxMap;
	else if (same(type,"mapdef"))   dn->dxtype = DxMap;
	else if (same(type,"general"))  dn->dxtype = DxGen;
	else if (same(type,"identity")) dn->dxtype = DxIdent;
	else                            dn->dxtype = DxNone;

	/* Define the viewport */
	if (viewport)        copy_box(&(dn->viewport),viewport);
	else if (dn->parent) copy_box(&(dn->viewport),&(dn->parent->window));
	else                 copy_box(&(dn->viewport),&UnitBox);

	/* Define the window from the window or map projection description */
	if (mproj && dn->dxtype==DxMap)
		{
		dn->window.left   = 0.0;
		dn->window.right  = mproj->definition.xlen;
		dn->window.bottom = 0.0;
		dn->window.top    = mproj->definition.ylen;
		}
	else if (window) copy_box(&(dn->window),window);
	else             copy_box(&(dn->window),&(dn->viewport));

	/* Define the map projection  */
	if (mproj && dn->dxtype==DxMap)
		 copy_map_projection(&(dn->mproj), mproj);
	else if (dn->parent && dn->dxtype!=DxWindow)
		 copy_map_projection(&(dn->mproj), &(dn->parent->mproj));
	else copy_map_projection(&(dn->mproj), &NoMapProj);

	/* Define the actual transform */
	if (dn->dxtype == DxGen)
		{
		if (!xform) copy_xform(dn->xform,IdentXform);
		else        copy_xform(dn->xform,xform);
		}
	else if ((window && dn->dxtype==DxWindow) || (mproj && dn->dxtype==DxMap))
		{
		block_xform(dn->xform,&(dn->viewport),&(dn->window));
		}
	else if (dn->dxtype != DxRoot)
		{
		dn->dxtype = DxIdent;
		}
	}

/***********************************************************************
*                                                                      *
*      r e c a l l _ d n _ x f o r m                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the transform information of the given dispnode.
 *
 *	@param[in] 	dn			The given dispnode
 *	@param[out] 	*type		transform type
 *	@param[out] 	*viewport	Viewport description
 *	@param[out] 	*window		Window description
 *	@param[out] 	*mproj		Map projection ("map" only)
 *	@param[out] 	xform		Actual transform ("general" only)
 *********************************************************************/

void		recall_dn_xform

	(
	DISPNODE	dn,
	STRING		*type,
	BOX			*viewport,
	BOX			*window,
	MAP_PROJ	*mproj,
	XFORM		xform
	)

	{
	/* Do nothing if dn is NULL */
	*type = NULL;
	if (!dn) return;

	/* Return transform type */
	switch (dn->dxtype)
		{
		case DxRoot:	*type = "root";
						break;

		case DxWindow:	*type = "window";
						break;

		case DxMap:		*type = "map";
						break;

		case DxGen:		*type = "general";
						break;

		case DxIdent:	*type = "identity";
						break;

		default:		*type = "none";
		}

	/* Return xform info */
	copy_box(viewport,&(dn->viewport));
	copy_box(window,&(dn->window));
	copy_map_projection(mproj,&(dn->mproj));
	copy_xform(xform,dn->xform);
	}

/***********************************************************************
*                                                                      *
*      m o v e _ d i s p n o d e                                       *
*      s i z e _ d i s p n o d e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Change the location of the given dispnode.          
 *
 * Move may be absolute or relative, according to justification
 * codes:                                                      
   @verbatim
   hjust =
   	- 'l' ---> absolute move, left justified        
   	- 'r' ---> absolute move, right justified       
   	- 'c' ---> absolute move, centred               
    - otherwise relative move                       
  
   vjust = 
   	- 'b' ---> absolute move, bottom justified      
   	- 't' ---> absolute move, top justified         
  	- 'c' ---> absolute move, centred               
   	- otherwise relative move                       
   	@endverbatim
 *
 *	@param[in] 	dn 		dispnode to modify
 *	@param[in] 	x		new x location
 *	@param[in] 	y		new y location
 *	@param[in] 	hjust	horizontal justification code
 *	@param[in] 	vjust	verticle justification code
 *********************************************************************/

void		move_dispnode

	(
	DISPNODE	dn,
	float		x,
	float		y,
	HJUST		hjust,
	VJUST		vjust
	)

	{
	STRING		type;
	BOX			viewport, window;
	MAP_PROJ	mproj;
	XFORM		xform;

	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Find out what it has currently */
	recall_dn_xform(dn,&type,&viewport,&window,&mproj,xform);

	/* Compute new viewport */
	if (hjust == Hl) x -= viewport.left;
	if (hjust == Hr) x -= viewport.right;
	if (hjust == Hc) x -= (viewport.right + viewport.left) / 2;
	if (vjust == VB) y -= viewport.bottom;
	if (vjust == VT) y -= viewport.top;
	if (vjust == Vb) y -= viewport.bottom;
	if (vjust == Vt) y -= viewport.top;
	if (vjust == Vc) y -= (viewport.top + viewport.bottom) / 2;
	viewport.left   += x;
	viewport.right  += x;
	viewport.bottom += y;
	viewport.top    += y;

	/* Now redefine the transform */
	define_dn_xform(dn,type,&viewport,&window,&mproj,xform);
	}

/*********************************************************************/
/** Change the size of the given dispnode.          
 *
 *	@param[in] 	dn	dispnode to modify
 *	@param[in] 	sx	new x size
 *	@param[in] 	sy	new y size
 *********************************************************************/
void		size_dispnode

	(
	DISPNODE	dn,
	float		sx,
	float		sy
	)

	{
	STRING		type;
	BOX			viewport, window;
	MAP_PROJ	mproj;
	XFORM		xform;

	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Find out what it has currently */
	recall_dn_xform(dn,&type,&viewport,&window,&mproj,xform);

	/* Compute new viewport */
	viewport.right = viewport.left   + sx;
	viewport.top   = viewport.bottom + sy;

	/* Compute new window */
	if (same(type, "window"))
		{
		window.right = window.left   + sx/xform[X][X];
		window.top   = window.bottom + sy/xform[Y][Y];
		}

	/* Now redefine the transform */
	define_dn_xform(dn,type,&viewport,&window,&mproj,xform);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ d n _ b g n d                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set the dispnode background attributes.
 *
 *	@param[in] 	dn		dispnode to set
 *	@param[in] 	vfill	viewport fill colour (<0 = transparent)
 *	@param[in] 	vedge	viewport edge colour (<0 = no edge)
 *	@param[in] 	wfill	window fill colour (<0 = transparent)
 *	@param[in] 	wedge	window edge colour (<0 = no edge)
 *********************************************************************/

void		define_dn_bgnd

	(
	DISPNODE	dn,
	COLOUR		vfill,
	COLOUR		vedge,
	COLOUR		wfill,
	COLOUR		wedge
	)

	{
	/* Do nothing if dn is NULL */
	if (!dn) return;

	/* Set the attributes */
	dn->vfill = vfill;
	dn->vedge = vedge;
	dn->wfill = wfill;
	dn->wedge = wedge;
	}

/***********************************************************************
*                                                                      *
*      r e c a l l _ d n _ b g n d                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the dispnode background attributes.
 *
 *	@param[in] 	dn		dispnode to set
 *	@param[out] 	*vfill	viewport fill colour (<0 = transparent)
 *	@param[out] 	*vedge	viewport edge colour (<0 = no edge)
 *	@param[out] 	*wfill	window fill colour (<0 = transparent)
 *	@param[out] 	*wedge	window edge colour (<0 = no edge)
 *********************************************************************/

void		recall_dn_bgnd

	(
	DISPNODE	dn,
	COLOUR		*vfill,
	COLOUR		*vedge,
	COLOUR		*wfill,
	COLOUR		*wedge
	)

	{
	/* Return desired items */
	*vfill = (dn) ? dn->vfill : SkipColour;
	*vedge = (dn) ? dn->vedge : SkipColour;
	*wfill = (dn) ? dn->wfill : SkipColour;
	*wedge = (dn) ? dn->wedge : SkipColour;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ d n _ v i s                                       *
*      r e c a l l _ d n _ v i s                                       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset dispnode visibility state
 *	@param[in] 	dn		dispnode to set
 *	@param[in] 	shown	desired visibility state
 *********************************************************************/
void		define_dn_vis

	(
	DISPNODE	dn,
	LOGICAL		shown
	)

	{
	if (dn) dn->shown = shown;
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve dispnode visibility state
 *
 *	@param[in] 	dn		dispnode to query 
 *	@param[out] 	*shown	is node to be shown? 
 *********************************************************************/
void		recall_dn_vis

	(
	DISPNODE	dn,
	LOGICAL		*shown
	)

	{
	if (shown) *shown = (dn)? dn->shown: FALSE;
	}

/***********************************************************************
*                                                                      *
*      i n s i d e _ d n _ v i e w p o r t                             *
*      i n s i d e _ d n _ v i e w p o r t _ x y                       *
*      i n s i d e _ d n _ w i n d o w                                 *
*      i n s i d e _ d n _ w i n d o w _ x y                           *
*                                                                      *
*      Indicate whether the given point is inside the specified        *
*      dispnode window.                                                *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Check if point is inside viewport.
 *
 * Point is specified as a POINT object.
 *
 *	@param[in]   dn	dispnode 
 *	@param[in] 	p	point 
 *  @return True if inside viewport.
 *********************************************************************/
LOGICAL		inside_dn_viewport

	(
	DISPNODE    dn,
	POINT		p
	)

	{
	if (!p)  return FALSE;
	if (!dn) return FALSE;
	return inside_box(&dn->viewport,p);
	}

/**********************************************************************/

/*********************************************************************/
/** Check if point is inside viewport.
 *
 * Point is specified as x/y pair
 *
 * @return True if inside viewport.
 *********************************************************************/
LOGICAL		inside_dn_viewport_xy

	(
	DISPNODE	dn,
	float		x,
	float		y
	)

	{
	if (!dn) return FALSE;
	return inside_box_xy(&dn->viewport,x,y);
	}

/**********************************************************************/

/*********************************************************************/
/** Check if point is inside window.
 *
 * Point is specified as a POINT object.
 *
 * @return True if inside window.
 *********************************************************************/
LOGICAL		inside_dn_window

	(
	DISPNODE    dn,
	POINT		p
	)

	{
	if (!p)  return FALSE;
	if (!dn) return FALSE;
	return inside_box(&dn->window,p);
	}

/**********************************************************************/

/*********************************************************************/
/** Check if point is inside window.
 *
 * Point is specified as x/y pair
 *
 * @return True if inside window.
 *********************************************************************/
LOGICAL		inside_dn_window_xy

	(
	DISPNODE	dn,
	float		x,
	float		y
	)

	{
	if (!dn) return FALSE;
	return inside_box_xy(&dn->window,x,y);
	}

/***********************************************************************
*                                                                      *
*      i n i t _ p a n e l                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Construct a "panel" from a dispnode, with optional viewing
 * and background colour control.
 *
 *	@param[in] 	pn			parent dispnode
 *	@param[in] 	ptype		type of panel (root, map, window)
 *	@param[in]  *viewport	viewport definition
 *	@param[in]  *window		window definition
 *	@param[in] 	vfill		viewport fill and edge colour names
 *	@param[in] 	vedge		viewport fill and edge colour names
 *	@param[in] 	wfill		window fill and edge colour names
 *	@param[in] 	wedge		window fill and edge colour names
 *********************************************************************/

DISPNODE	init_panel

	(
	DISPNODE	pn,
	STRING		ptype,
	const BOX	*viewport,
	const BOX	*window,
	STRING		vfill,
	STRING		vedge,
	STRING		wfill,
	STRING		wedge
	)

	{
	COLOUR		vf, ve, wf, we;
	DISPNODE	dn;

	/* Interpret fill and edge colours */
	vf = (blank(vfill))? SkipColour: find_colour(vfill, (LOGICAL *) 0);
	ve = (blank(vedge))? SkipColour: find_colour(vedge, (LOGICAL *) 0);
	wf = (blank(wfill))? SkipColour: find_colour(wfill, (LOGICAL *) 0);
	we = (blank(wedge))? SkipColour: find_colour(wedge, (LOGICAL *) 0);

	/* Now set up the dispnode as configured */
	/* close_config_file(); ??? */
	dn = create_dispnode();
	add_dn_to_subtree(dn, pn);
	define_dn_xform(dn, ptype, viewport, window, NullMapProj, NullXform);
	define_dn_bgnd(dn, vf, ve, wf, we);
	return dn;
	}
