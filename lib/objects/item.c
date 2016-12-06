/*********************************************************************/
/**	@file item.c
 *
 * Routines to handle the ITEM object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      i t e m . c                                                     *
*                                                                      *
*      Routines to handle the ITEM object.                             *
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

#define ITEM_INIT
#include "item.h"

#include <tools/tools.h>
#include <fpa_math.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      c r e a t e _ b g n d _ i t e m                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create an item suitable for the background member of a set.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	subelem subelement name
 *	@param[in] 	value	value
 *	@param[in] 	label	label
 *  @return Pointer to an item object. You will need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

ITEM	create_bgnd_item

	(
	STRING	type,
	STRING	subelem,
	STRING	value,
	STRING	label
	)

	{
	SPOT		spot;
	ATTRIB_LIST	att;

	static	POINT	pos = ZERO_POINT;
	static	BOX		box = UNIT_BOX;

	/* Do nothing if type or item are null */
	if (!type)                    return NullItem;

	/* Create the item */
	else if (same(type,"area"))   return (ITEM) create_area(subelem,value,
															   label);
	else if (same(type,"barb"))   return (ITEM) create_barb(subelem,pos,
																0.,0.,0.);
	else if (same(type,"button")) return (ITEM) create_button(subelem,value,
															  label,&box);
	else if (same(type,"curve"))  return (ITEM) create_curve(subelem,value,
															 label);
	else if (same(type,"label"))  return (ITEM) create_label(subelem,value,
															 label,pos,0.);
	else if (same(type,"mark"))   return (ITEM) create_mark(subelem,value,
															label,pos,0.);
	else if (same(type,"spot"))
				{
				att  = create_attrib_list();
				add_default_attributes(att, subelem, value, label);
				spot = create_spot(pos, subelem, AttachNone, att);
				return (ITEM) spot;
				}

	else if (same(type,"lchain")) return (ITEM) create_lchain();

	else                          return NullItem;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ i t e m                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy the given item of the specified type.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be destroyed
 *  @return Null if successful. The given item otherwise.
 *********************************************************************/

ITEM	destroy_item

	(
	STRING	type,
	ITEM	item
	)

	{
	/* Do nothing if type or item are null */
	if (!type || !item)           return item;

	/* Destroy the item */
	else if (same(type,"area"))   return (ITEM) destroy_area((AREA) item);
	else if (same(type,"barb"))   return (ITEM) destroy_barb((BARB) item);
	else if (same(type,"button")) return (ITEM) destroy_button((BUTTON) item);
	else if (same(type,"curve"))  return (ITEM) destroy_curve((CURVE) item);
	else if (same(type,"label"))  return (ITEM) destroy_label((LABEL) item);
	else if (same(type,"mark"))   return (ITEM) destroy_mark((MARK) item);
	else if (same(type,"spot"))   return (ITEM) destroy_spot((SPOT) item);
	else if (same(type,"lchain")) return (ITEM) destroy_lchain((LCHAIN) item);
	else                          return item;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ i t e m                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Empty the given item of the specified type.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be emptied
 *  @return Null if successful. The given item otherwise.
 *********************************************************************/

ITEM	empty_item

	(
	STRING	type,
	ITEM	item
	)

	{
	/* Do nothing if type or item are null */
	if (!type || !item)           return item;

	/* Same as destroy for now */
	else if (same(type,"area"))   return (ITEM) destroy_area((AREA) item);
	else if (same(type,"barb"))   return (ITEM) destroy_barb((BARB) item);
	else if (same(type,"button")) return (ITEM) destroy_button((BUTTON) item);
	else if (same(type,"curve"))  return (ITEM) destroy_curve((CURVE) item);
	else if (same(type,"label"))  return (ITEM) destroy_label((LABEL) item);
	else if (same(type,"mark"))   return (ITEM) destroy_mark((MARK) item);
	else if (same(type,"spot"))   return (ITEM) destroy_spot((SPOT) item);
	else if (same(type,"lchain")) return (ITEM) destroy_lchain((LCHAIN) item);
	else                          return item;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ i t e m                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Copy the given item of the specified type.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *  @return Pointer to a copy of the item. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/
ITEM	copy_item

	(
	STRING		type,
	const ITEM	item
	)

	{
	/* Do nothing if type or item are null */
	if (!type || !item)           return NullItem;

	/* Copy the item if possible */
	else if (same(type,"area"))   return (ITEM) copy_area((AREA) item, TRUE);
	else if (same(type,"barb"))   return (ITEM) copy_barb((BARB) item);
	else if (same(type,"button")) return (ITEM) copy_button((BUTTON) item);
	else if (same(type,"curve"))  return (ITEM) copy_curve((CURVE) item);
	else if (same(type,"label"))  return (ITEM) copy_label((LABEL) item);
	else if (same(type,"mark"))   return (ITEM) copy_mark((MARK) item);
	else if (same(type,"spot"))   return (ITEM) copy_spot((SPOT) item);
	else if (same(type,"lchain")) return (ITEM) copy_lchain((LCHAIN) item);
	else                          return NullItem;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ i t e m _ v a l u e                               *
*      r e c a l l _ i t e m _ v a l u e                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the values of the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	given item
 *	@param[in] 	subelem subelement name
 *	@param[in] 	value	value to set
 *	@param[in] 	label	label to set
 *********************************************************************/
void	define_item_value

	(
	STRING	type,
	ITEM	item,
	STRING	subelem,
	STRING	value,
	STRING	label
	)

	{
	SPOT		spot;

	static	POINT	pos = ZERO_POINT;

	/* Do nothing if type or item are null */
	if (!type || !item)           return;

	/* Define the item values if possible */
	if (same(type,"subarea"))     define_subarea_value((SUBAREA)item,
											subelem,value,label);
	else if (same(type,"area"))   define_area_value((AREA)item,
											subelem,value,label);
	else if (same(type,"barb"))   define_barb_value((BARB)item,
											subelem,pos,0.,0.,0.);
	else if (same(type,"button")) define_button_value((BUTTON)item,
											subelem,value);
	else if (same(type,"curve"))  define_curve_value((CURVE)item,
											subelem,value,label);
	else if (same(type,"label"))  define_label_value((LABEL)item,
											subelem,value,label);
	else if (same(type,"mark"))   define_mark_value((MARK)item,
											subelem,value,label);
	else if (same(type,"spot"))
				{
				spot = (SPOT) item;
				if (IsNull(spot->attrib)) spot->attrib = create_attrib_list();
				add_default_attributes(spot->attrib, subelem, value, label);
				}
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ i t e m _ a t t r i b s                           *
*      r e c a l l _ i t e m _ a t t r i b s                           *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the attributes of the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	given item
 *	@param[in] 	attribs	attributes
 *********************************************************************/
void	define_item_attribs

	(
	STRING		type,
	ITEM		item,
	ATTRIB_LIST	attribs
	)

	{
	/* Do nothing if type or item are null */
	if (!type || !item)           return;

	/* Define the item attributes if possible */
	if (same(type,"subarea"))      define_subarea_attribs((SUBAREA) item, attribs);
	else if (same(type,"area"))    define_area_attribs((AREA) item, attribs);
	else if (same(type,"barb"))    define_barb_attribs((BARB) item, attribs);
	else if (same(type,"button"))  define_button_attribs((BUTTON) item, attribs);
	else if (same(type,"curve"))   define_curve_attribs((CURVE) item, attribs);
	else if (same(type,"label"))   define_label_attribs((LABEL) item, attribs);
	else if (same(type,"mark"))    define_mark_attribs((MARK) item, attribs);
	else if (same(type,"spot"))    define_spot_attribs((SPOT) item, attribs);
	else if (same(type,"lchain"))  define_lchain_attribs((LCHAIN) item, attribs);
	else if (same(type,"lnode"))   define_lnode_attribs((LNODE) item, attribs);
	else if (same(type,"linterp")) define_linterp_attribs((LINTERP) item, attribs);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the attributes of the given item.
 *
 *	@param[in] 	type		specified item type
 *	@param[in] 	item		requested item
 *	@param[out]	*attribs	attributes
 *********************************************************************/
void	recall_item_attribs

	(
	STRING		type,
	ITEM		item,
	ATTRIB_LIST	*attribs
	)

	{
	/* Do nothing if type or item are null */
	if (attribs) *attribs = NullAttribList;
	if (!type || !item) return;

	/* Define the item attributes if possible */
	if (same(type,"subarea"))      recall_subarea_attribs((SUBAREA) item, attribs);
	else if (same(type,"area"))    recall_area_attribs((AREA) item, attribs);
	else if (same(type,"barb"))    recall_barb_attribs((BARB) item, attribs);
	else if (same(type,"button"))  recall_button_attribs((BUTTON) item, attribs);
	else if (same(type,"curve"))   recall_curve_attribs((CURVE) item, attribs);
	else if (same(type,"label"))   recall_label_attribs((LABEL) item, attribs);
	else if (same(type,"mark"))    recall_mark_attribs((MARK) item, attribs);
	else if (same(type,"spot"))    recall_spot_attribs((SPOT) item, attribs);
	else if (same(type,"lchain"))  recall_lchain_attribs((LCHAIN) item, attribs);
	else if (same(type,"lnode"))   recall_lnode_attribs((LNODE) item, attribs);
	else if (same(type,"linterp")) recall_linterp_attribs((LINTERP) item, attribs);
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ i t e m                                     *
*      h i g h l i g h t _ i t e m _ c a t e g o r y                   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set the highlight flag for the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *	@param[in] 	code	hilite code
 *********************************************************************/
void	highlight_item

	(
	STRING	type,
	ITEM	item,
	HILITE	code
	)

	{
	int	fcode;

	/* Do nothing if type or item are null */
	if (!type || !item) return;
	fcode = MIN(code,0);

	/* AREA item */
	if (same(type,"area"))
		{
		AREA	harea;
		int		isub;

		harea = (AREA) item;
		define_lspec_value(&harea->lspec, LINE_HILITE, (POINTER)&code);
		define_fspec_value(&harea->fspec, FILL_HILITE, (POINTER)&fcode);

		/* Carry invocation to subareas of an area */
		if ( harea->subareas )
			{
			for (isub=0; isub<(harea->numdiv+1); isub++)
				{
				highlight_item("subarea", (ITEM) harea->subareas[isub], code);
				}
			}
		}

	/* SUBAREA item */
	else if (same(type,"subarea"))
		{
		/* Trick to carry invocation to subareas of an area */
		/* Subareas are not members of sets */
		define_lspec_value(&((SUBAREA) item)->lspec, LINE_HILITE,
						   									(POINTER)&code);
		define_fspec_value(&((SUBAREA) item)->fspec, FILL_HILITE,
						   									(POINTER)&fcode);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		define_bspec_value(&((BARB) item)->bspec, BARB_HILITE,
															(POINTER)&code);
		define_tspec_value(&((BARB) item)->tspec, TEXT_HILITE,
															(POINTER)&code);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		define_lspec_value(&((BUTTON) item)->lspec, LINE_HILITE,
															(POINTER)&code);
		define_fspec_value(&((BUTTON) item)->fspec, FILL_HILITE,
															(POINTER)&fcode);
		define_tspec_value(&((BUTTON) item)->tspec, TEXT_HILITE,
															(POINTER)&code);
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		define_lspec_value(&((CURVE) item)->lspec, LINE_HILITE,
															(POINTER)&code);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		define_tspec_value(&((LABEL) item)->tspec, TEXT_HILITE,
															(POINTER)&code);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		define_mspec_value(&((MARK) item)->mspec, MARK_HILITE,
															(POINTER)&code);
		}

	/* SPOT item */
	else if (same(type,"spot"))
		{
		SPOT	spot;
		int		imem;
		SPMEM	*mem;

		spot = (SPOT) item;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&code);
			define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&code);
			define_bspec_value(&mem->bspec, BARB_HILITE, (POINTER)&code);
			}
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{
		define_lspec_value(&((LCHAIN) item)->lspec, LINE_HILITE,
															(POINTER)&code);
		}

	/* Link nodes of LCHAIN item */
	else if (same(type,"nodes"))
		{
		LCHAIN	lchain;
		int		inode;
		LNODE	lnode;
		LINTERP	linterp;
		int		imem;
		NODEMEM	*mem;

		lchain = (LCHAIN) item;

		/* Link nodes */
		for (inode=0; inode<lchain->lnum; inode++)
			{
			lnode = lchain->nodes[inode];
			for (imem=0; imem<lnode->nmem; imem++)
				{
				mem = lnode->members + imem;
				define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&code);
				define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&code);
				define_bspec_value(&mem->bspec, BARB_HILITE, (POINTER)&code);
				}
			}

		/* Interpolated nodes */
		for (inode=0; inode<lchain->inum; inode++)
			{
			linterp = lchain->interps[inode];
			for (imem=0; imem<linterp->nmem; imem++)
				{
				mem = linterp->members + imem;
				define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&code);
				define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&code);
				define_bspec_value(&mem->bspec, BARB_HILITE, (POINTER)&code);
				}
			}
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Set the highlight flag for the given item and category.
 *
 *	@param[in] 	type		specified item type
 *	@param[in] 	item		item to be hilited
 *	@param[in] 	category	specified category
 *	@param[in] 	code		hilite code
 *********************************************************************/
void	highlight_item_category

	(
	STRING	type,
	ITEM	item,
	STRING	category,
	HILITE	code
	)

	{
	int		fcode;
	STRING	cat;
	AREA	harea;
	int		isub;

	/* Do nothing if type or item are null */
	if (!type || !item) return;
	fcode = MIN(code,0);

	/* AREA item */
	if (same(type,"area"))
		{
		harea = (AREA) item;
		(void) get_attribute(harea->attrib, AttribCategory, &cat);
		if (same(cat, category))
			{
			define_lspec_value(&harea->lspec, LINE_HILITE, (POINTER)&code);
			define_fspec_value(&harea->fspec, FILL_HILITE, (POINTER)&fcode);
			}

		/* Carry invocation to subareas of an area */
		if ( harea->subareas )
			{
			for (isub=0; isub<(harea->numdiv+1); isub++)
				{
				highlight_item_category("subarea",
							(ITEM) harea->subareas[isub], category, code);
				}
			}
		}

	/* SUBAREA item */
	else if (same(type,"subarea"))
		{
		/* Trick to carry invocation to subareas of an area */
		/* Subareas are not members of sets */
		(void) get_attribute(((SUBAREA)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_lspec_value(&((SUBAREA) item)->lspec, LINE_HILITE,
														   (POINTER)&code);
		define_fspec_value(&((SUBAREA) item)->fspec, FILL_HILITE,
						   								(POINTER)&fcode);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		(void) get_attribute(((BARB)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_bspec_value(&((BARB) item)->bspec, BARB_HILITE,
															(POINTER)&code);
		define_tspec_value(&((BARB) item)->tspec, TEXT_HILITE,
															(POINTER)&code);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		(void) get_attribute(((BUTTON)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_lspec_value(&((BUTTON) item)->lspec, LINE_HILITE,
															(POINTER)&code);
		define_fspec_value(&((BUTTON) item)->fspec, FILL_HILITE,
															(POINTER)&fcode);
		define_tspec_value(&((BUTTON) item)->tspec, TEXT_HILITE,
															(POINTER)&code);
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		(void) get_attribute(((CURVE)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_lspec_value(&((CURVE) item)->lspec, LINE_HILITE,
															(POINTER)&code);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		(void) get_attribute(((LABEL)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_tspec_value(&((LABEL) item)->tspec, TEXT_HILITE,
															(POINTER)&code);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		(void) get_attribute(((MARK)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_mspec_value(&((MARK) item)->mspec, MARK_HILITE,
															(POINTER)&code);
		}

	/* SPOT item */
	else if (same(type,"spot"))
		{
		SPOT	spot;
		int		imem;
		SPMEM	*mem;

		spot = (SPOT) item;
		(void) get_attribute(spot->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&code);
			define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&code);
			define_bspec_value(&mem->bspec, BARB_HILITE, (POINTER)&code);
			}
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{
		(void) get_attribute(((LCHAIN)item)->attrib, AttribCategory, &cat);
		if (!same(cat, category)) return;
		define_lspec_value(&((LCHAIN) item)->lspec, LINE_HILITE,
															(POINTER)&code);
		}

	/* Link nodes of LCHAIN item */
	else if (same(type,"nodes"))
		{
		LCHAIN	lchain;
		int		inode;
		LNODE	lnode;
		LINTERP	linterp;
		int		imem;
		NODEMEM	*mem;

		lchain = (LCHAIN) item;

		/* Link nodes */
		for (inode=0; inode<lchain->lnum; inode++)
			{
			lnode = lchain->nodes[inode];
			(void) get_attribute(lnode->attrib, AttribCategory, &cat);
			if (!same(cat, category)) continue;
			for (imem=0; imem<lnode->nmem; imem++)
				{
				mem = lnode->members + imem;
				define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&code);
				define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&code);
				define_bspec_value(&mem->bspec, BARB_HILITE, (POINTER)&code);
				}
			}

		/* Interpolated nodes */
		for (inode=0; inode<lchain->inum; inode++)
			{
			linterp = lchain->interps[inode];
			(void) get_attribute(linterp->attrib, AttribCategory, &cat);
			if (!same(cat, category)) continue;
			for (imem=0; imem<linterp->nmem; imem++)
				{
				mem = linterp->members + imem;
				define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&code);
				define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&code);
				define_bspec_value(&mem->bspec, BARB_HILITE, (POINTER)&code);
				}
			}
		}
	}

/***********************************************************************
*                                                                      *
*      i n v o k e _ i t e m _ c a t s p e c                           *
*      i n v o k e _ i t e m _ p l t s p e c                           *
*                                                                      *
***********************************************************************/

/* Special pseudo-items: */
typedef struct
	{
	SPMEM	*spmem;
	STRING	mclass;
	CAL		att;
	} SPAM;
typedef struct
	{
	NODEMEM	*ndmem;
	STRING	mclass;
	CAL		att;
	} NDAM;

/*********************************************************************/
/**	Invoke an item's category specification.
 *
 *	@param[in] 	type	type of item
 *	@param[in] 	item	item to invoke
 *	@param[in] 	ncspec	number of specifications
 *	@param[in]  *cspecs	list of specifications
 *********************************************************************/
void	invoke_item_catspec

	(
	STRING			type,
	ITEM			item,
	int				ncspec,
	const CATSPEC	*cspecs
	)

	{
	STRING	ccat, cat, val, class, mname;
	int		isp;
	CATSPEC	*cs, *ds;
	CATSPEC	*cspec = (CATSPEC *) 0;
	LSPEC	*lspec = (LSPEC *) 0;
	FSPEC	*fspec = (FSPEC *) 0;
	TSPEC	*tspec = (TSPEC *) 0;
	MSPEC	*mspec = (MSPEC *) 0;
	BSPEC	*bspec = (BSPEC *) 0;
	SPMEM	*spmem;
	NODEMEM	*ndmem;

	static	LSPEC	lspint;
	static	FSPEC	fspint;
	static	TSPEC	tspint;
	static	MSPEC	mspint;
	static	BSPEC	bspint;
	static	POINT	offset;
	static	STRING	attrib;

	/* Do nothing if type or item are null */
	if (!type || !item) return;

	/* Get item category (formerly subelement) and value */
	class = NULL;
	mname = NULL;
	cat   = item_attribute(type, item, AttribCategory);
	val   = item_attribute(type, item, AttribAutolabel);

	/* Set class etc for spot members */
	if (same(type,"spmem"))
		{
		class = ((SPAM *) item)->mclass;
		spmem = ((SPAM *) item)->spmem;
		mname = spmem->name;
		val   = NULL;
		}

	/* Set class etc for node members */
	else if (same(type,"ndmem"))
		{
		class = ((NDAM *) item)->mclass;
		ndmem = ((NDAM *) item)->ndmem;
		mname = ndmem->name;
		val   = NULL;
		}

	/* Carry invocation to subareas of an area */
	else if (same(type,"area"))
		{
		AREA	harea;
		int		isub;

		harea = (AREA) item;
		if ( harea->subareas )
			{
			for (isub=0; isub<(harea->numdiv+1); isub++)
				{
				invoke_item_catspec("subarea",((ITEM) harea->subareas[isub]),
									ncspec,cspecs);
				}
			}

		/* Carry on to set specs for the area object itself */
		}

	/* Carry invocation to members of a spot */
	else if (same(type,"spot"))
		{
		SPOT	spot;
		int		imem;
		SPAM	sp;

		spot = (SPOT) item;

		/* Members should match the spec list */
		if (spot->nmem <= 0) build_spot_members(spot, ncspec, cspecs);

		/* Invoke specs for each member */
		for (imem=0; imem<spot->nmem; imem++)
			{
			sp.spmem  = spot->members + imem;
			sp.mclass = spot->mclass;
			sp.att    = spot->attrib;
			invoke_item_catspec("spmem",((ITEM) &sp), ncspec, cspecs);
			}

		/* No specs for the spot object itself */
		return;
		}

	/* Carry invocation to node members of a link chain */
	else if (same(type,"nodes"))
		{
		LCHAIN	lchain;
		int		inode;
		LNODE	lnode;
		LINTERP	linterp;
		int		imem;
		NDAM	nd;

		/* Interpolate the link chain (if necessary) */
		lchain = (LCHAIN) item;
		if (lchain->dointerp) interpolate_lchain(lchain);

		/* Link nodes */
		for (inode=0; inode<lchain->lnum; inode++)
			{

			/* Members should match the spec list */
			lnode = lchain->nodes[inode];
			if (lnode->nmem <= 0) build_lnode_members(lnode, ncspec, cspecs);

			/* Invoke specs for each member */
			for (imem=0; imem<lnode->nmem; imem++)
				{
				nd.ndmem  = lnode->members + imem;
				nd.mclass = lnode->guess? FpaNodeClass_NormalGuess:
											FpaNodeClass_Normal;
				nd.att    = lnode->attrib;
				invoke_item_catspec("ndmem", ((ITEM) &nd), ncspec, cspecs);
				}
			}

		/* Interpolated nodes */
		for (inode=0; inode<lchain->inum; inode++)
			{

			/* Members should match the spec list */
			linterp = lchain->interps[inode];
			if (linterp->nmem <= 0)
				build_linterp_members(linterp, ncspec, cspecs);

			/* Invoke specs for each member */
			for (imem=0; imem<linterp->nmem; imem++)
				{
				nd.ndmem  = linterp->members + imem;
				nd.mclass = FpaNodeClass_Interp;
				nd.att    = linterp->attrib;
				invoke_item_catspec("ndmem", ((ITEM) &nd), ncspec, cspecs);
				}
			}

		/* No further specs for the link nodes */
		return;
		}

	/* Use the category to define the presentation specs */
	/* Interpret internally coded presentation specs first */
	copy_point(offset, ZeroPoint);
	attrib = NULL;
	if (same(cat,"LSPEC"))
		{ lspec = &lspint;
		string_lspec(lspec,val);
		}
	else if (same(cat,"FSPEC"))
		{
		fspec = &fspint;
		string_fspec(fspec,val);
		}
	else if (same(cat,"TSPEC"))
		{
		tspec = &tspint;
		string_tspec(tspec,val);
		}
	else if (same(cat,"MSPEC"))
		{
		mspec = &mspint;
		string_mspec(mspec,val);
		}
	else if (same(cat,"BSPEC"))
		{
		bspec = &bspint;
		string_bspec(bspec,val);
		}
	else
		{
		/* Leave alone if no category specs */
		if (ncspec <= 0) return;

		/* Find which category spec corresponds to the item category */
		ds = (CATSPEC *)NULL;
		for (isp=0; isp<ncspec; isp++)
			{
			cs = (CATSPEC *) cspecs + isp;
			if ( NotNull(class) && !same(class, cs->mclass)) continue;
			if ( NotNull(mname) && !same(mname, cs->name) )  continue;

			if (same(cs->cat, "default"))
				{
				ds = cs;
				continue;
				}
			else if (same(cs->cat, "category"))
				{
				ccat = cat;
				}
			else
				{
				if ( blank(cs->cat) ) ccat = cat;
				else ccat = item_attribute(type, item, cs->cat);
				}

			if ( NotNull(ccat)  && !same(ccat,  cs->val)  ) continue;
			cspec = cs;
			break;
			}

		/* Use default ("ALL") category spec for unknown categories */
		if (!cspec) cspec = ds;
		if (!cspec) return;

		/* Copy the presentation spec from the category spec */
		if (cspec)
			{
			lspec  = &cspec->lspec;
			fspec  = &cspec->fspec;
			tspec  = &cspec->tspec;
			mspec  = &cspec->mspec;
			bspec  = &cspec->bspec;
			attrib = cspec->attrib;
			copy_point(offset, cspec->offset);
			}
		}

	/* AREA item */
	if (same(type,"area"))
		{
		copy_lspec(&((AREA) item)->lspec,lspec);
		copy_fspec(&((AREA) item)->fspec,fspec);
		}

	/* SUBAREA item */
	else if (same(type,"subarea"))
		{
		/* Trick to carry invocation to subareas of an area */
		/* Subareas are not members of sets */
		copy_lspec(&((SUBAREA) item)->lspec,lspec);
		copy_fspec(&((SUBAREA) item)->fspec,fspec);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		copy_bspec(&((BARB) item)->bspec,bspec);
		copy_tspec(&((BARB) item)->tspec,tspec);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		copy_lspec(&((BUTTON) item)->lspec,lspec);
		copy_fspec(&((BUTTON) item)->fspec,fspec);
		copy_tspec(&((BUTTON) item)->tspec,tspec);
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		copy_lspec(&((CURVE) item)->lspec,lspec);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		copy_tspec(&((LABEL) item)->tspec,tspec);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		copy_mspec(&((MARK) item)->mspec,mspec);
		}

	/* SPOT member (SPMEM *) item */
	else if (same(type,"spmem"))
		{
		spmem = ((SPAM *) item)->spmem;
		define_spmem_attrib(spmem, attrib);
		copy_point(spmem->offset,offset);
		copy_tspec(&spmem->tspec,tspec);
		copy_mspec(&spmem->mspec,mspec);
		copy_bspec(&spmem->bspec,bspec);
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{
		copy_lspec(&((LCHAIN) item)->lspec,lspec);
		}

	/* Link node member (NDMEM *) item */
	else if (same(type,"ndmem"))
		{
		ndmem = ((NDAM *) item)->ndmem;
		define_node_mem_attrib(ndmem, attrib);
		copy_point(ndmem->offset,offset);
		copy_tspec(&ndmem->tspec,tspec);
		copy_mspec(&ndmem->mspec,mspec);
		copy_bspec(&ndmem->bspec,bspec);
		}
	}

/**********************************************************************/

/*********************************************************************/
/**	Invoke an item's category specification.
 *
 *	@param[in] 	type	type of object
 *	@param[in] 	item	item to invoke
 *	@param[in] 	ncspec	number of specifications
 *	@param[in] 	*cspecs	list of specifications
 *********************************************************************/
void	invoke_item_pltspec

	(
	STRING			type,
	ITEM			item,
	int				ncspec,
	const PLTSPEC	*cspecs
	)

	{
	STRING	cat, val;
	int		isp;
	PLTSPEC	*cspec = (PLTSPEC *) 0;
	LSPEC	*lspec = (LSPEC *) 0;
	FSPEC	*fspec = (FSPEC *) 0;
	TSPEC	*tspec = (TSPEC *) 0;
	MSPEC	*mspec = (MSPEC *) 0;
	BSPEC	*bspec = (BSPEC *) 0;

	static	LSPEC	lspint;
	static	FSPEC	fspint;
	static	TSPEC	tspint;
	static	MSPEC	mspint;
	static	BSPEC	bspint;

	/* Do nothing if type or item are null */
	if (!type || !item) return;

	/* Get item category (formerly subelement) and value */
	cat = item_attribute(type, item, AttribCategory);
	val = item_attribute(type, item, AttribAutolabel);

	/* Carry invocation to subareas of an area */
	if (same(type,"area"))
		{
		AREA	harea;
		int		isub;

		harea = (AREA) item;
		if ( harea->subareas )
			{
			for (isub=0; isub<(harea->numdiv+1); isub++)
				{
				invoke_item_pltspec("subarea",((ITEM) harea->subareas[isub]),
									ncspec,cspecs);
				}
			}

		/* Carry on to set specs for the area object itself */
		}

	/* Carry invocation to members of a spot */
	else if (same(type,"spot"))
		{
		SPOT	spot;
		int		imem;
		SPAM	sp;

		spot = (SPOT) item;
		for (imem=0; imem<spot->nmem; imem++)
			{
			sp.spmem = spot->members + imem;
			sp.att   = spot->attrib;
			invoke_item_pltspec("spmem",((ITEM) &sp),ncspec,cspecs);
			}

		/* No specs for the spot object itself */
		return;
		}

	/* Use the category to define the presentation specs */
	/* Interpret internally coded presentation specs first */
	if (blank(cat)) return;
	else if (same(cat,"LSPEC"))
		{
		lspec = &lspint;
		string_lspec(lspec,val);
		}
	else if (same(cat,"FSPEC"))
		{
		fspec = &fspint;
		string_fspec(fspec,val);
		}
	else if (same(cat,"TSPEC"))
		{
		tspec = &tspint;
		string_tspec(tspec,val);
		}
	else if (same(cat,"MSPEC"))
		{
		mspec = &mspint;
		string_mspec(mspec,val);
		}
	else if (same(cat,"BSPEC"))
		{
		bspec = &bspint;
		string_bspec(bspec,val);
		}
	else
		{
		/* Leave alone if no plot specs */
		if (ncspec <= 0) return;

		/* Find which subfield spec corresponds to the category */
		for (isp=0; isp<ncspec; isp++)
			{
			cspec = (PLTSPEC *) cspecs + isp;
			if (same(cat,cspec->name)) break;
			cspec = (PLTSPEC *) 0;
			}

		/* Use NO default for unknown categories */
		/* if (!cspec) cspec = (PLTSPEC *) cspecs; */
		if (!cspec) return;

		/* Copy the presentation spec from the plot spec */
		if (cspec)
			{
			lspec = &cspec->lspec;
			fspec = &cspec->fspec;
			tspec = &cspec->tspec;
			mspec = &cspec->mspec;
			bspec = &cspec->bspec;
			}
		}

	/* AREA item */
	if (same(type,"area"))
		{
		copy_lspec(&((AREA) item)->lspec,lspec);
		copy_fspec(&((AREA) item)->fspec,fspec);
		}

	/* SUBAREA item */
	else if (same(type,"subarea"))
		{
		/* Trick to carry invocation to subareas of an area */
		/* Subareas are not members of sets */
		copy_lspec(&((SUBAREA) item)->lspec,lspec);
		copy_fspec(&((SUBAREA) item)->fspec,fspec);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		copy_bspec(&((BARB) item)->bspec,bspec);
		copy_tspec(&((BARB) item)->tspec,tspec);
		if (cspec) copy_point(((BARB) item)->anchor,cspec->offset);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		copy_lspec(&((BUTTON) item)->lspec,lspec);
		copy_fspec(&((BUTTON) item)->fspec,fspec);
		copy_tspec(&((BUTTON) item)->tspec,tspec);
		if (cspec) copy_point(((BUTTON) item)->lpos,cspec->offset);
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		copy_lspec(&((CURVE) item)->lspec,lspec);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		copy_tspec(&((LABEL) item)->tspec,tspec);
		if (cspec) copy_point(((LABEL) item)->anchor,cspec->offset);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		copy_mspec(&((MARK) item)->mspec,mspec);
		if (cspec) copy_point(((MARK) item)->anchor,cspec->offset);
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{
		copy_lspec(&((LCHAIN) item)->lspec,lspec);
		}
	}

/***********************************************************************
*                                                                      *
*      c h a n g e _ i t e m _ p s p e c                               *
*      r e c a l l _ i t e m _ p s p e c                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Override the presentation specs of the given item.
 *
 *	@param[in] 	type	type of object
 *	@param[in] 	item	given item
 *	@param[in] 	param	paramter to change
 *	@param[in] 	value	new value
 *********************************************************************/

void	change_item_pspec

	(
	STRING	type,
	ITEM	item,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if type or item are null */
	if (blank(type) || !item) return;

	/* Change the appropriate pspec */

	/* AREA item */
	else if (same(type,"area"))
		{
		AREA	harea;
		int		isub;

		harea = (AREA) item;
		define_lspec_value(&harea->lspec,param,value);
		define_fspec_value(&harea->fspec,param,value);

		/* Carry change to subareas of an area */
		if ( harea->subareas )
			{
			for (isub=0; isub<(harea->numdiv+1); isub++)
				{
				change_item_pspec("subarea",((ITEM) harea->subareas[isub]),
								  param, value);
				}
			}
		}

	/* SUBAREA item */
	else if (same(type,"subarea"))
		{
		/* Trick to carry change to subareas of an area */
		/* Subareas are not members of sets */
		define_lspec_value(&((SUBAREA) item)->lspec,param,value);
		define_fspec_value(&((SUBAREA) item)->fspec,param,value);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		define_bspec_value(&((BARB) item)->bspec,param,value);
		define_tspec_value(&((BARB) item)->tspec,param,value);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		define_lspec_value(&((BUTTON) item)->lspec,param,value);
		define_fspec_value(&((BUTTON) item)->fspec,param,value);
		define_tspec_value(&((BUTTON) item)->tspec,param,value);
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		define_lspec_value(&((CURVE) item)->lspec,param,value);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		define_tspec_value(&((LABEL) item)->tspec,param,value);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		define_mspec_value(&((MARK) item)->mspec,param,value);
		}

	/* SPOT item */
	else if (same(type,"spot"))
		{

		/* Carry invocation to members of a spot */
		SPOT	spot;
		int		imem;
		SPMEM	*mem;

		spot = (SPOT) item;
		for (imem=0; imem<spot->nmem; imem++)
			{
			mem = spot->members + imem;
			define_tspec_value(&mem->tspec,param,value);
			define_mspec_value(&mem->mspec,param,value);
			define_bspec_value(&mem->bspec,param,value);
			}
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{

		LCHAIN	lchain;
		int		inode;
		LNODE	lnode;
		LINTERP	linterp;
		int		imem;
		NODEMEM	*mem;

		lchain = (LCHAIN) item;
		define_lspec_value(&lchain->lspec,param,value);

		/* Carry changes to normal nodes of LCHAIN item */
		for (inode=0; inode<lchain->lnum; inode++)
			{
			lnode = lchain->nodes[inode];
			for (imem=0; imem<lnode->nmem; imem++)
				{
				mem = lnode->members + imem;
				define_tspec_value(&mem->tspec,param,value);
				define_mspec_value(&mem->mspec,param,value);
				define_bspec_value(&mem->bspec,param,value);
				}
			}

		/* Carry changes to interpolated nodes of LCHAIN item */
		for (inode=0; inode<lchain->inum; inode++)
			{
			linterp = lchain->interps[inode];
			for (imem=0; imem<linterp->nmem; imem++)
				{
				mem = linterp->members + imem;
				define_tspec_value(&mem->tspec,param,value);
				define_mspec_value(&mem->mspec,param,value);
				define_bspec_value(&mem->bspec,param,value);
				}
			}
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the presentation specs of the given item.
 *
 *	@param[in] 	type	object type
 *	@param[in] 	item	item object
 *	@param[in] 	param	parameter to change
 *	@param[out]	value	returned value
 *********************************************************************/
void	recall_item_pspec

	(
	STRING	type,
	ITEM	item,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if type or item are null */
	if (blank(type) || !item) return;

	/* Find the appropriate pspec */

	/* AREA item */
	else if (same(type,"area"))
		{
		recall_lspec_value(&((AREA) item)->lspec,param,value);
		recall_fspec_value(&((AREA) item)->fspec,param,value);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		recall_bspec_value(&((BARB) item)->bspec,param,value);
		recall_tspec_value(&((BARB) item)->tspec,param,value);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		recall_lspec_value(&((BUTTON) item)->lspec,param,value);
		recall_fspec_value(&((BUTTON) item)->fspec,param,value);
		recall_tspec_value(&((BUTTON) item)->tspec,param,value);
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		recall_lspec_value(&((CURVE) item)->lspec,param,value);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		recall_tspec_value(&((LABEL) item)->tspec,param,value);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		recall_mspec_value(&((MARK) item)->mspec,param,value);
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{
		recall_lspec_value(&((LCHAIN) item)->lspec,param,value);
		}
	}

/***********************************************************************
*                                                                      *
*      i n b o x _ i t e m                                             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Determine if the given item passes through the given box.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be tested
 *	@param[in] 	*box	box to test against
 *  @return True if item passes through the box.
 *********************************************************************/
LOGICAL	inbox_item

	(
	STRING		type,
	ITEM		item,
	const BOX	*box
	)

	{
	/* Do nothing if type or item are null */
	if (!type || !item) return FALSE;

	/* AREA item */
	else if (same(type,"area"))
		{
		return inbox_bound(((AREA) item)->bound, box);
		}

	/* BARB item */
	else if (same(type,"barb"))
		{
		return inside_box(box, ((BARB) item)->anchor);
		}

	/* BUTTON item */
	else if (same(type,"button"))
		{
		BOX		bbox;

		bbox = ((BUTTON) item)->box;
		if (inside_box_xy(box, bbox.left,  bbox.bottom)) return TRUE;
		if (inside_box_xy(box, bbox.left,  bbox.top))    return TRUE;
		if (inside_box_xy(box, bbox.right, bbox.bottom)) return TRUE;
		if (inside_box_xy(box, bbox.right, bbox.top))    return TRUE;
		return FALSE;
		}

	/* CURVE item */
	else if (same(type,"curve"))
		{
		return inbox_curve(((CURVE) item), box);
		}

	/* LABEL item */
	else if (same(type,"label"))
		{
		return inside_box(box, ((LABEL) item)->anchor);
		}

	/* MARK item */
	else if (same(type,"mark"))
		{
		return inside_box(box, ((MARK) item)->anchor);
		}

	/* SPOT item */
	else if (same(type,"spot"))
		{
		return inside_box(box, ((SPOT) item)->anchor);
		}

	/* LCHAIN item */
	else if (same(type,"lchain"))
		{
		return inbox_lchain(((LCHAIN) item), box);
		}

	else return FALSE;
	}

/***********************************************************************
*                                                                      *
*      r e p r o j e c t _ i t e m                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reproject the given item from the first map projection to
 * the second.
 *
 *	@param[in] 	type		specified item type
 *	@param[in] 	item		item to be reprojected
 *	@param[in] 	*smproj		starting map projection
 *	@param[in] 	*tmproj		target map projection
 * @return True if successful.
 *********************************************************************/

LOGICAL		reproject_item

	(
	STRING			type,
	ITEM			item,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	LOGICAL	reproj, remap, rescale;
	float	sunits, tunits, sfact;
	AREA	area;
	LINE	line;
	float	*pos;
	POINT	tpos;
	BOX		*box;
	LCHAIN	lchain;
	int		iseg, ip, ii;


	/* Do nothing if type or item are null */
	if (blank(type) || !item) return FALSE;

	/* Do nothing if no starting map projection */
	if (!smproj) return FALSE;

	/* Do we need to reproject? */
	reproj = (LOGICAL)
				(
				tmproj!=NullMapProj &&
				!same_projection(&smproj->projection, &tmproj->projection)
				);

	/* Do we need to remap? */
	remap = (LOGICAL)
				(
				reproj ||
					(
					tmproj!=NullMapProj &&
					!equivalent_map_def(&smproj->definition,
										&tmproj->definition)
					)
				);

	/* Do we just need to rescale? */
	sunits  = smproj->definition.units;
	tunits  = (tmproj!=NullMapProj)? tmproj->definition.units: sunits;
	sfact   = sunits / tunits;
	rescale = (LOGICAL) (!remap && sunits!=tunits);

	if (!reproj && !remap && !rescale) return TRUE;

	if (rescale) return scale_item(type, item, sfact, sfact);

	/* Transform the appropriate item type */
	if (same(type,"area"))
		{
		area = (AREA) item;
		if ( NotNull(area->bound) )
			{
			line = area->bound->boundary;
			for (ip=0; ip<line->numpts; ip++)
				{
				(void) pos_to_pos(smproj, line->points[ip], tmproj, tpos);
				(void) copy_point(line->points[ip], tpos);
				}
			for (iseg=0; iseg<area->bound->numhole; iseg++)
				{
				line = area->bound->holes[iseg];
				for (ip=0; ip<line->numpts; ip++)
					{
					(void) pos_to_pos(smproj, line->points[ip], tmproj, tpos);
					(void) copy_point(line->points[ip], tpos);
					}
				}
			for (iseg=0; iseg<area->numdiv; iseg++)
				{
				line = area->divlines[iseg];
				for (ip=0; ip<line->numpts; ip++)
					{
					(void) pos_to_pos(smproj, line->points[ip], tmproj, tpos);
					(void) copy_point(line->points[ip], tpos);
					}
				}
			/* >>> Don't worry about visible areas yet */
			}
		}
	else if (same(type,"barb"))
		{
		pos = ((BARB) item)->anchor;
		(void) pos_to_pos(smproj, pos, tmproj, tpos);
		(void) copy_point(pos, tpos);
		}
	else if (same(type,"button"))
		{
		pos = ((BUTTON) item)->lpos;
		box = &(((BUTTON) item)->box);
		(void) pos_to_pos(smproj, pos, tmproj, tpos);
		(void) copy_point(pos, tpos);
		/* Just scale the box since it cannot be rotated */
		box->left   *= sfact;
		box->right  *= sfact;
		box->bottom *= sfact;
		box->top    *= sfact;
		}
	else if (same(type,"curve"))
		{
		line = ((CURVE) item)->line;
		for (ip=0; ip<line->numpts; ip++)
			{
			(void) pos_to_pos(smproj, line->points[ip], tmproj, tpos);
			(void) copy_point(line->points[ip], tpos);
			}
		}
	else if (same(type,"label"))
		{
		pos = ((LABEL) item)->anchor;
		(void) pos_to_pos(smproj, pos, tmproj, tpos);
		(void) copy_point(pos, tpos);
		}
	else if (same(type,"mark"))
		{
		pos = ((MARK) item)->anchor;
		(void) pos_to_pos(smproj, pos, tmproj, tpos);
		(void) copy_point(pos, tpos);
		}
	else if (same(type,"spot"))
		{
		pos = ((SPOT) item)->anchor;
		(void) pos_to_pos(smproj, pos, tmproj, tpos);
		(void) copy_point(pos, tpos);
		}
	else if (same(type,"lchain"))
		{
		lchain = (LCHAIN) item;
		for (ii=0; ii<lchain->lnum; ii++)
			{
			if (!lchain->nodes[ii]->there) continue;
			(void) pos_to_pos(smproj, lchain->nodes[ii]->node, tmproj, tpos);
			(void) copy_point(lchain->nodes[ii]->node, tpos);
			}
		for (ii=0; ii<lchain->inum; ii++)
			{
			if (!lchain->interps[ii]->there) continue;
			(void) pos_to_pos(smproj, lchain->interps[ii]->node, tmproj, tpos);
			(void) copy_point(lchain->interps[ii]->node, tpos);
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      o f f s e t _ i t e m                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Offset the given item of the specified type.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *	@param[in] 	xoff	offset in x-direction
 *	@param[in] 	yoff	offset in y-direction
 *  @return Ture if successful.
 *********************************************************************/

LOGICAL	offset_item

	(
	STRING	type,
	ITEM	item,
	float	xoff,
	float	yoff
	)

	{
	AREA	area;
	BARB	barb;
	BUTTON	button;
	CURVE	curve;
	LABEL	label;
	MARK	mark;
	SPOT	spot;
	LINE	line;
	LCHAIN	lchain;
	int		iseg, ip, ii;

	/* Do nothing if type or item are null */
	if (!type || !item) return FALSE;

	/* Offset the item if possible */
	else if (same(type,"area"))
		{
		area = (AREA) item;
		if (area->bound && area->bound->boundary)
			{
			line = area->bound->boundary;
			for (ip=0; ip<line->numpts; ip++)
				{
				line->points[ip][X] += xoff;
				line->points[ip][Y] += yoff;
				}
			for (iseg=0; iseg<area->bound->numhole; iseg++)
				{
				line = area->bound->holes[iseg];
				for (ip=0; ip<line->numpts; ip++)
					{
					line->points[ip][X] += xoff;
					line->points[ip][Y] += yoff;
					}
				}
			}
		for (iseg=0; iseg<area->numdiv; iseg++)
			{
			line = area->divlines[iseg];
			for (ip=0; ip<line->numpts; ip++)
				{
				line->points[ip][X] += xoff;
				line->points[ip][Y] += yoff;
				}
			}
		/* >>> Don't worry about visible areas yet */
		}
	else if (same(type,"barb"))
		{
		barb = (BARB) item;
		barb->anchor[X] += xoff;
		barb->anchor[Y] += yoff;
		}
	else if (same(type,"button"))
		{
		button = (BUTTON) item;
		button->box.left   += xoff;
		button->box.right  += xoff;
		button->box.bottom += yoff;
		button->box.top    += yoff;
		button->lpos[X]    += xoff;
		button->lpos[Y]    += yoff;
		}
	else if (same(type,"curve"))
		{
		curve = (CURVE) item;
		if (curve->line)
			{
			line = curve->line;
			for (ip=0; ip<line->numpts; ip++)
				{
				line->points[ip][X] += xoff;
				line->points[ip][Y] += yoff;
				}
			}
		}
	else if (same(type,"label"))
		{
		label = (LABEL) item;
		label->anchor[X] += xoff;
		label->anchor[Y] += yoff;
		}
	else if (same(type,"mark"))
		{
		mark = (MARK) item;
		mark->anchor[X] += xoff;
		mark->anchor[Y] += yoff;
		}
	else if (same(type,"spot"))
		{
		spot = (SPOT) item;
		spot->anchor[X] += xoff;
		spot->anchor[Y] += yoff;
		}
	else if (same(type,"lchain"))
		{
		lchain = (LCHAIN) item;
		for (ii=0; ii<lchain->lnum; ii++)
			{
			if (!lchain->nodes[ii]->there) continue;
			lchain->nodes[ii]->node[X] += xoff;
			lchain->nodes[ii]->node[Y] += yoff;
			}
		for (ii=0; ii<lchain->inum; ii++)
			{
			if (!lchain->interps[ii]->there) continue;
			lchain->interps[ii]->node[X] += xoff;
			lchain->interps[ii]->node[Y] += yoff;
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      s c a l e _ i t e m                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Scale the given item of the specified type.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *	@param[in] 	xscale	scale in x-direction
 *	@param[in] 	yscale	scale in y-direction
 *  @return True if successful.
 *********************************************************************/

LOGICAL	scale_item

	(
	STRING	type,
	ITEM	item,
	float	xscale,
	float	yscale
	)

	{
	AREA	area;
	BARB	barb;
	BUTTON	button;
	CURVE	curve;
	LABEL	label;
	MARK	mark;
	SPOT	spot;
	LINE	line;
	LCHAIN	lchain;
	int		iseg, ip, ii;

	/* Do nothing if type or item are null */
	if (!type || !item) return FALSE;

	/* Scale the item if possible */
	else if (same(type,"area"))
		{
		area = (AREA) item;
		if (area->bound && area->bound->boundary)
			{
			line = area->bound->boundary;
			for (ip=0; ip<line->numpts; ip++)
				{
				line->points[ip][X] *= xscale;
				line->points[ip][Y] *= yscale;
				}
			for (iseg=0; iseg<area->bound->numhole; iseg++)
				{
				line = area->bound->holes[iseg];
				for (ip=0; ip<line->numpts; ip++)
					{
					line->points[ip][X] *= xscale;
					line->points[ip][Y] *= yscale;
					}
				}
			}
		for (iseg=0; iseg<area->numdiv; iseg++)
			{
			line = area->divlines[iseg];
			for (ip=0; ip<line->numpts; ip++)
				{
				line->points[ip][X] *= xscale;
				line->points[ip][Y] *= yscale;
				}
			}
		/* >>> Don't worry about visible areas yet */
		}
	else if (same(type,"barb"))
		{
		barb = (BARB) item;
		barb->anchor[X] *= xscale;
		barb->anchor[Y] *= yscale;
		}
	else if (same(type,"button"))
		{
		button = (BUTTON) item;
		button->box.left   *= xscale;
		button->box.right  *= xscale;
		button->box.bottom *= yscale;
		button->box.top    *= yscale;
		button->lpos[X]    *= xscale;
		button->lpos[Y]    *= yscale;
		}
	else if (same(type,"curve"))
		{
		curve = (CURVE) item;
		if (curve->line)
			{
			line = curve->line;
			for (ip=0; ip<line->numpts; ip++)
				{
				line->points[ip][X] *= xscale;
				line->points[ip][Y] *= yscale;
				}
			}
		}
	else if (same(type,"label"))
		{
		label = (LABEL) item;
		label->anchor[X] *= xscale;
		label->anchor[Y] *= yscale;
		}
	else if (same(type,"mark"))
		{
		mark = (MARK) item;
		mark->anchor[X] *= xscale;
		mark->anchor[Y] *= yscale;
		}
	else if (same(type,"spot"))
		{
		spot = (SPOT) item;
		spot->anchor[X] *= xscale;
		spot->anchor[Y] *= yscale;
		}
	else if (same(type,"lchain"))
		{
		lchain = (LCHAIN) item;
		for (ii=0; ii<lchain->lnum; ii++)
			{
			if (!lchain->nodes[ii]->there) continue;
			lchain->nodes[ii]->node[X] *= xscale;
			lchain->nodes[ii]->node[Y] *= yscale;
			}
		for (ii=0; ii<lchain->inum; ii++)
			{
			if (!lchain->interps[ii]->there) continue;
			lchain->interps[ii]->node[X] *= xscale;
			lchain->interps[ii]->node[Y] *= yscale;
			}
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      i t e m _ a t t r i b u t e                                     *
*      i t e m _ c a t e g o r y                                       *
*      i t e m _ a u t o l a b e l                                     *
*      i t e m _ u s e r l a b e l                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Return the requested attribute from the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item
 *	@param[in] 	name	attribute name
 *  @return Pointer to a STRING containing the value of the given
 * 			attribute. Pointer returned points to static memory within
 * 			the function. If you are not going to use the result
 * 			immediately it is safest to make a copy using safe_strcpy
 * 			or safe_strdup. You DO NOT destroy this object!
 *********************************************************************/

STRING	item_attribute

	(
	STRING	type,
	ITEM	item,
	STRING	name
	)

	{
	ATTRIB_LIST	al;
	STRING		val;

	/* Do nothing if type or item are null */
	if (!type || !item) return NULL;
	if (blank(name))    return NULL;

	/* Obtain the attribute list if possible */
		/* Trick to carry invocation to subareas of an area */
		/* Subareas are not members of sets */
		/* Trick to carry invocation to members of a spot */
		/* Spot members are not members of sets */
		/* Trick to carry invocation to nodes of a link chain */
		/* Link chain nodes are not members of sets */
	else if (same(type,"area"))    al = ((AREA) item)->attrib;
	else if (same(type,"subarea")) al = ((SUBAREA) item)->attrib;
	else if (same(type,"barb"))    al = ((BARB) item)->attrib;
	else if (same(type,"button"))  al = ((BUTTON) item)->attrib;
	else if (same(type,"curve"))   al = ((CURVE) item)->attrib;
	else if (same(type,"label"))   al = ((LABEL) item)->attrib;
	else if (same(type,"mark"))    al = ((MARK) item)->attrib;
	else if (same(type,"spot"))    al = ((SPOT) item)->attrib;
	else if (same(type,"spmem"))   al = ((SPAM *) item)->att;
	else if (same(type,"lchain"))  al = ((LCHAIN) item)->attrib;
	else if (same(type,"ndmem"))   al = ((NDAM *) item)->att;
	else                           return NULL;

	if (IsNull(al)) return NULL;

	/* Return the requested attribute if possible */
	(void) get_attribute(al, name, &val);
	return val;
	}

/**********************************************************************/

/*********************************************************************/
/** Return the category attribute from the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *  @return Pointer to a STRING containing the value of the given
 * 			attribute. Pointer returned points to static memory
 * 			within the function. If you are not going to use the
 * 			result immediately it is safest to make a copy using
 * 			safe_strcpy or safe_strdup. You DO NOT destroy this object!
 *********************************************************************/
STRING	item_category

	(
	STRING	type,
	ITEM	item
	)

	{
	return item_attribute(type, item, AttribCategory);
	}

/**********************************************************************/

/*********************************************************************/
/** Return the auto label  attribute from the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *  @return Pointer to a STRING containing the value of the given
 *  		attribute. Pointer returned points to static memory within
 *  		the function. If you are not going to use the result
 *  		immediately it is safest to make a copy using safe_strcpy
 *  		or safe_strdup. You DO NOT destroy this object!
 *********************************************************************/
STRING	item_autolabel

	(
	STRING	type,
	ITEM	item
	)

	{
	return item_attribute(type, item, AttribAutolabel);
	}

/**********************************************************************/

/*********************************************************************/
/** Return the user label attribute from the given item.
 *
 *	@param[in] 	type	specified item type
 *	@param[in] 	item	item to be copied
 *  @return Pointer to a STRING containing the value of the given
 * 			attribute. Pointer returned points to static memory within
 * 			the function. If you are not going to use the result
 * 			immediately it is safest to make a copy using safe_strcpy
 * 			or safe_strdup. You DO NOT destroy this object!
 *********************************************************************/
STRING	item_userlabel

	(
	STRING	type,
	ITEM	item
	)

	{
	return item_attribute(type, item, AttribUserlabel);
	}

/***********************************************************************
*                                                                      *
*      i t e m c m p                                                   *
*      i t e m r e v                                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Compare two items by some numerical value - use with qsort(3)
 * items arranged in ascending order
 *
 * @return
 * -  1 if s1 > s2,
 * - -1 if s1 < s2,
 * -  0 if s1=s2.
 *********************************************************************/

int	itemcmp

	(
	const void *s1,
	const void *s2
	)

	{
	if (!s1) return 1;
	if (!s2) return -1;
	if (((SITEM *) s1)->value > ((SITEM *) s2)->value) return 1;
	if (((SITEM *) s2)->value > ((SITEM *) s1)->value) return -1;
	return 0;
	}

/*********************************************************************/
/** Compare two items by some numerical value - use with qsort(3)
 * items arranged in decending order
 *
 * @return
 * -  1 if s1 < s2,
 * - -1 if s1 > s2,
 * -  0 if s1=s2.
 *********************************************************************/
int	itemrev

	(
	const void *s1,
	const void *s2
	)

	{
	if (!s1) return -1;
	if (!s2) return 1;
	if (((SITEM *) s1)->value > ((SITEM *) s2)->value) return -1;
	if (((SITEM *) s2)->value > ((SITEM *) s1)->value) return 1;
	return 0;
	}
