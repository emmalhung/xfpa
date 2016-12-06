/*********************************************************************/
/** @file set.c
 *
 * Routines to handle the ITEM and SET objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      s e t . c                                                       *
*                                                                      *
*      Routines to handle the ITEM and SET objects.                    *
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

#define SET_INIT
#include "set.h"
#include "set_oper.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

int		SetCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ s e t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new set with given attributes.
 *
 *	@param[in] 	type	type of item - all items must be this type
 * 	@return Pointer to new set object. You will have to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

SET		create_set

	(
	STRING	type
	)

	{
	SET	set;

	/* Allocate memory for structure */
	set = INITMEM(struct SET_struct, 1);
	if (!set) return NullSet;

	/* Initialize the structure */
	set->type   = NULL;
	set->bgnd   = NullItem;
	set->list   = NullItemList;
	set->num    = 0;
	set->max    = 0;
	set->cspecs = (CATSPEC *) 0;
	set->ncspec = 0;
	set->xspecs = (CATSPEC *) 0;
	set->nxspec = 0;

	define_set_type(set, type);

	/* Return the new set */
	SetCount++;
	return set;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ s e t                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy a set.
 *
 *	@param[in] 	set		set to be destroyed
 * 	@return NullSet
 *********************************************************************/

SET		destroy_set

	(
	SET		set
	)

	{
	int		i;
	ITEM	*cp;

	/* Do nothing if not there */
	if (!set) return NullSet;

	/* Free space for list */
	if (cp = set->list)
		{
		for (i=0; i<set->num; i++)
			{
			cp[i] = destroy_item(set->type, cp[i]);
			}
		FREEMEM(set->list);
		}

	/* Free space for background */
	define_set_bgnd(set, NullItem);

	/* Free space for type */
	FREEMEM(set->type);

	/* Free space used for category specs */
	define_set_catspecs(set, 0, (CATSPEC *) 0);

	/* Free structure itself */
	FREEMEM(set);
	SetCount--;
	return NullSet;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ s e t                                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Copy one set to another.
 *
 *	@param[in] 	set		set to be copied
 * 	@return Pointer to copy of given set. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/

SET		copy_set

	(
	const SET	set
	)

	{
	return append_set(NullSet, set);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s e t _ b g n d                                   *
*      d e f i n e _ s e t _ b g v a l                                 *
*      d e f i n e _ s e t _ b g _ a t t r i b s                       *
*      r e c a l l _ s e t _ b g _ a t t r i b s                       *
*                                                                      *
*      Set the background attributes of a given set.                   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset the background value of a SET object.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	bgnd	item exhibiting background attributes
 *********************************************************************/
void	define_set_bgnd

	(
	SET		set,
	ITEM	bgnd
	)

	{
	/* Do nothing if no set given */
	if (!set) return;

	/* Get rid of old background */
	set->bgnd = destroy_item(set->type, set->bgnd);

	/* Copy the new background */
	if (!bgnd) return;
	set->bgnd = copy_item(set->type, bgnd);
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset the background value of a SET object.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	subelem		subelement name
 *	@param[in] 	value		back ground value of element
 *	@param[in] 	label		background label
 *********************************************************************/
void	define_set_bgval

	(
	SET		set,
	STRING	subelem,
	STRING	value,
	STRING	label
	)

	{
	/* Do nothing if no set given */
	if (!set) return;

	/* Set the background value */
	/* Make the background if necessary - create an item of the set type */
	set->bgnd = destroy_item(set->type, set->bgnd);
	set->bgnd = create_bgnd_item(set->type, subelem, value, label);
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset the back ground attributes of a set.
 *
 *	@param[in] 	set		given area
 *	@param[in] 	attribs	attributes
 *********************************************************************/
void	define_set_bg_attribs

	(
	SET			set,
	ATTRIB_LIST	attribs
	)

	{
	/* Do nothing if set not there */
	if (!set) return;

	/* Set the background value */
	/* Make the background if necessary - create an item of the set type */
	set->bgnd = destroy_item(set->type, set->bgnd);
	set->bgnd = create_bgnd_item(set->type, NULL, NULL, NULL);
	define_item_attribs(set->type, set->bgnd, attribs);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the background attributes of a set.
 *
 *	@param[in] 	set			requested area
 *	@param[in] 	*attribs	attributes
 *********************************************************************/
void	recall_set_bg_attribs

	(
	SET			set,
	ATTRIB_LIST	*attribs
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = NullAttribList;
	if (!set || !set->bgnd) return;

	recall_item_attribs(set->type, set->bgnd, attribs);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s e t _ t y p e                                   *
*      r e c a l l _ s e t _ t y p e                                   *
*                                                                      *
*      Set attributes of a given set.                                  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set attributes of a given set.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	type	set type
 *********************************************************************/
void	define_set_type

	(
	SET		set,
	STRING	type
	)

	{
	/* Do nothing if no set given */
	if (!set) return;

	/* If set type has already been defined empty the list */
	if (!blank(set->type))
		{
		define_set_bgnd(set, NullItem);
		empty_set(set);
		}

	/* Allocate memory for character attributes */
	set->type = STRMEM(set->type, type);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the set type of a given set.
 *
 *	@param[in] 	set		requested set pointer
 *	@param[in] 	*type	set type
 *********************************************************************/
void	recall_set_type

	(
	SET		set,
	STRING	*type
	)

	{
	/* Retrieve all the attributes */
	if (type) *type = (set) ? set->type : NULL;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ s e t                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Zero the list in the given set without freeing any allocated space.
 *
 *	@param[in] 	set		set to initialize
 *********************************************************************/

void	empty_set

	(
	SET		set
	)

	{
	int		i;
	ITEM	*cp;

	/* Do nothing if not there */
	if (!set) return;

	/* Zero each item */
	cp = set->list;
	for (i=0; i<set->num; i++)
		{
		cp[i] = empty_item(set->type, cp[i]);
		}

	/* Zero the set itself */
	set->num = 0;
	return;
	}

/***********************************************************************
*                                                                      *
*      a p p e n d _ s e t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Append one set to another.
 *
 *	@param[in] 	set1	set to be appended to
 *	@param[in] 	set2	set to be appended
 * 	@return Pointer to extended set object. Since set1 is the set that
 * 			gets appened to you the return pointer points to it.
 *********************************************************************/

SET		append_set

	(
	SET			set1,
	const SET	set2
	)

	{
	int		i, nnew, nn;
	ITEM	*cp1, *cp2;

	/* Do nothing if set2 not there */
	if (!set2) return set1;

	/* If set1 not there, create an empty copy of set2 first */
	if (!set1)
		{
		set1 = create_set(set2->type);
		define_set_bgnd(set1, set2->bgnd);
		define_set_catspecs(set1, (int) set2->ncspec, set2->cspecs);
		define_set_secondary_catspecs(set1, (int) set2->nxspec, set2->xspecs);
		}

	/* If types are different do nothing */
	if (!same(set1->type, set2->type)) return set1;

	/* Allocate more room in set1 if required */
	if (set2->num <= 0) return set1;
	nnew = set1->num + set2->num;
	if (nnew > set1->max)
		{
		nn = (nnew-1)%DELTA_SET + 1;
		set1->max = nnew + DELTA_SET - nn;
		set1->list = GETMEM(set1->list, ITEM, set1->max);
		}

	/* Copy the list to the new set */
	cp1 = set1->list;
	cp2 = set2->list;
	for (i=0; i<set2->num; i++)
		{
		cp1[set1->num++] = copy_item(set1->type, cp2[i]);
		}

	/* Return the new set */
	return set1;
	}

/***********************************************************************
*                                                                      *
*      w h i c h _ s e t _ i t e m                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find a given item in a set and return its position in the list.
 *
 *	@param[in] 	set		set to search
 *	@param[in] 	item	item to look for
 * 	@return The index of the item in the set.
 *********************************************************************/

int	which_set_item

	(
	SET		set,
	ITEM	item
	)

	{
	int	i;

	/* Do nothing if set not there */
	if (!set)  return -1;
	if (!item) return -1;

	/* Search the list */
	for (i=0; i<set->num; i++)
		if (set->list[i] == item) return i;

	/* Not found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*      m o v e _ s e t _ i t e m                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Move the given item to the given location in the set list.
 *
 *	@param[in] 	set		set to rearrange
 *	@param[in] 	ifrom	initial index
 *	@param[in] 	ito		index to move item to
 *********************************************************************/

void	move_set_item

	(
	SET		set,
	int		ifrom,
	int		ito
	)

	{
	int		imem, idir;
	ITEM	move;

	/* Do nothing if set not there or pointers out of bounds */
	if (!set)              return;
	if (ifrom < 0)         return;
	if (ifrom >= set->num) return;
	if (ito < 0)           return;
	if (ito >= set->num)   return;
	if (ito == ifrom)      return;

	/* Take out the item in question */
	move = set->list[ifrom];

	/* Move all the items between the original and target positions */
	idir = SIGN(ito-ifrom);
	for (imem=ifrom; imem!=ito; imem+=idir)
		{
		set->list[imem] = set->list[imem+idir];
		}

	/* Put the item in its new position */
	set->list[ito] = move;
	}

/***********************************************************************
*                                                                      *
*      a d d _ i t e m _ t o _ s e t                                   *
*      a d d _ i t e m _ t o _ s e t _ s t a r t                       *
*      r e m o v e _ i t e m _ f r o m _ s e t                         *
*                                                                      *
*      Add or remove the given item to/from the given set.             *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/**	Add the given item to the given set.
 *
 *	@param[in] 	set		set to add item to
 *	@param[in] 	item	item to add to set
 * 	@return Pointer to item added. NullItem if failed to add item.
 *********************************************************************/
ITEM	add_item_to_set

	(
	SET		set,
	ITEM	item
	)

	{
	/* Do nothing if set not there */
	if (!set)  return NullItem;
	if (!item) return NullItem;

	/* See if we need more space */
	if (set->num >= set->max)
		{
		set->max  += DELTA_SET;
		set->list  = GETMEM(set->list, ITEM, set->max);
		}

	/* Add the given item to the item list */
	set->list[set->num] = item;
	set->num++;

	/* Check against category specs */
	invoke_item_catspec(set->type, item, set->ncspec, set->cspecs);

	/* Check against secondary category specs for link chains */
	if (same(set->type, "lchain"))
		invoke_item_catspec("nodes", item, set->nxspec, set->xspecs);

	/* Return the item */
	return item;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given item to the start of the given set.
 *
 *	SET		set		set to add item to
 *	ITEM	item	item to add to set
 *  @return Pointer to item added. NullItem if failed to add item.
 *********************************************************************/
ITEM	add_item_to_set_start

	(
	SET		set,
	ITEM	item
	)

	{
	int		i;
	ITEM	*cp;

	/* Do nothing if set not there */
	if (!set)  return NullItem;
	if (!item) return NullItem;

	/* See if we need more space */
	if (set->num >= set->max)
		{
		set->max  += DELTA_SET;
		set->list  = GETMEM(set->list, ITEM, set->max);
		}

	/* Move other members up */
	set->num++;
	cp = set->list;
	for (i=set->num-1; i>0; i--)
		cp[i] = cp[i-1];

	/* Add the given item to the front of the list */
	set->list[0] = item;

	/* Check against category specs */
	invoke_item_catspec(set->type, item, set->ncspec, set->cspecs);

	/* Check against secondary category specs for link chains */
	if (same(set->type, "lchain"))
		invoke_item_catspec("nodes", item, set->nxspec, set->xspecs);

	/* Return the item */
	return item;
	}

/**********************************************************************/

/*********************************************************************/
/** Remove the given item from the given set.
 *
 *	@param[in] 	set		set to remove item from
 *	@param[in] 	item	item to remove from set
 * 	@return NullItem if successful. The given item if failed.
 *********************************************************************/
ITEM	remove_item_from_set

	(
	SET		set,
	ITEM	item
	)

	{
	int		i;
	ITEM	*cp;

	/* Do nothing if set or item not there */
	if (!set)  return item;
	if (!item) return NullItem;

	/* See if item is in the given set */
	/* If not there return unscathed */
	i = which_set_item(set, item);
	if (i < 0) return item;

	/* Remove item from active list */
	item = destroy_item(set->type, item);
	cp   = set->list;
	set->num--;
	for (; i < set->num; i++)
		cp[i] = cp[i+1];
	return item;
	}

/***********************************************************************
*                                                                      *
*      r e c a l l _ s e t _ l i s t                                   *
*      r e c a l l _ s e t _ i t e m                                   *
*                                                                      *
*      Retrieve the item buffer information of a given set.            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Retrieve the item buffer information of a given set.
 *
 *	@param[in] 	set		requested set pointer
 *	@param[out]	**list	start of item buffer
 *	@param[out]	*num	current number of list
 *	@param[out]	*max	current reserve of list
 *********************************************************************/
void	recall_set_list

	(
	SET		set,
	ITEM	**list,
	int		*num,
	int		*max
	)

	{
	/* Retrieve all the desired info */
	if (list) *list = (set) ? set->list : NullItemList;
	if (num)  *num  = (set) ? set->num  : 0;
	if (max)  *max  = (set) ? set->max  : 0;
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve an item by item number.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	itemno	requested item number
 * 	@return Pointer to the requested item object.
 *********************************************************************/
ITEM	recall_set_item

	(
	SET		set,
	int		itemno
	)

	{
	if (!set)               return NullItem;
	if (itemno < 0)         return NullItem;
	if (itemno >= set->num) return NullItem;

	return set->list[itemno];
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s e t _ c a t s p e c s                           *
*      a d d _ c a t s p e c _ t o _ s e t                             *
*      r e c a l l _ s e t _ c a t s p e c s                           *
*      f i n d _ s e t _ c a t s p e c                                 *
*      d e f i n e _ s e t _ s e c o n d a r y _ c a t s p e c s       *
*      a d d _ s e c o n d a r y _ c a t s p e c _ t o _ s e t         *
*      i n v o k e _ s e t _ c a t s p e c s                           *
*                                                                      *
*      Define the category specs for the given set.                    *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Define the category specs for the given set.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	ncspec		number of category specs in cspecs
 *	@param[in] 	*cspecs		list of category specifications
 *********************************************************************/
void	define_set_catspecs

	(
	SET		set,
	int		ncspec,
	CATSPEC	*cspecs
	)

	{
	int		ic;

	/* Do nothing if no set */
	if (!set) return;

	/* Get rid of existing cspec list */
	if (set->cspecs)
		{
		for (ic=0; ic<set->ncspec; ic++)
			{
			free_catspec(set->cspecs + ic);
			}
		FREEMEM(set->cspecs);
		}
	set->ncspec = 0;

	/* Add the new cspec list one at a time */
	if (!cspecs) return;
	for (ic=0; ic<ncspec; ic++)
		{
		add_catspec_to_set(set, cspecs+ic);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given category specification to the given set.
 *
 *	@param[in] 	set		Set to append to
 *	@param[in] 	*cspec	specification to add
 *********************************************************************/
void	add_catspec_to_set

	(
	SET		set,
	CATSPEC	*cspec
	)

	{
	CATSPEC	*csnew;

	/* Do nothing if nothing to work on */
	if (!set)   return;
	if (!cspec) return;

	/* Expand the cspec list */
	set->ncspec++;
	set->cspecs = GETMEM(set->cspecs, CATSPEC, set->ncspec);
	csnew = set->cspecs + (set->ncspec-1);
	init_catspec(csnew);

	/* Copy the given cspec into the new one */
	copy_catspec(csnew, cspec);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the category specifications for a given set.
 *
 *	@param[in] 	set			given set
 *	@param[out]	*ncspec		size of list returned
 *	@param[out]	**cspecs	list of category specifications
 *********************************************************************/
void	recall_set_catspecs

	(
	SET		set,
	int		*ncspec,
	CATSPEC	**cspecs
	)

	{
	/* Retrieve all the desired info */
	if (cspecs) *cspecs = (set) ? set->cspecs : (CATSPEC *) 0;
	if (ncspec) *ncspec = (set) ? set->ncspec : 0;
	}

/**********************************************************************/

/*********************************************************************/
/** Find a the category specifications for a given subelement.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	subelem		given sub element name
 * 	@return Pointer to list of category specifications. Null if failed.
 *********************************************************************/
CATSPEC	*find_set_catspec

	(
	SET		set,
	STRING	subelem
	)

	{
	CATSPEC	*cspec;
	int		isp;

	if (!set)           return (CATSPEC *) 0;
	if (blank(subelem)) return (CATSPEC *) 0;

	/* Find which category spec corresponds to the subelement */
	for (isp=0; isp<set->ncspec; isp++)
		{
		cspec = set->cspecs + isp;
		if (same(subelem, cspec->name)) return cspec;
		}

	/* Not found */
	return (CATSPEC *) 0;
	}

/**********************************************************************/

/*********************************************************************/
/** Define the secondary category specs for the given set.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	nxspec		number of secondary category specs in xspecs
 *	@param[in] 	*xspecs		list of secondary category specifications
 *********************************************************************/
void	define_set_secondary_catspecs

	(
	SET		set,
	int		nxspec,
	CATSPEC	*xspecs
	)

	{
	int		ic;

	/* Do nothing if no set */
	if (!set) return;

	/* Get rid of existing xspec list */
	if (set->xspecs)
		{
		for (ic=0; ic<set->nxspec; ic++)
			{
			free_catspec(set->xspecs + ic);
			}
		FREEMEM(set->xspecs);
		}
	set->nxspec = 0;

	/* Add the new xspec list one at a time */
	if (!xspecs) return;
	for (ic=0; ic<nxspec; ic++)
		{
		add_secondary_catspec_to_set(set, xspecs+ic);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given secondary category specification to the given set.
 *
 *	@param[in] 	set		Set to append to
 *	@param[in] 	*xspec	secondary specification to add
 *********************************************************************/
void	add_secondary_catspec_to_set

	(
	SET		set,
	CATSPEC	*xspec
	)

	{
	CATSPEC	*csnew;

	/* Do nothing if nothing to work on */
	if (!set)   return;
	if (!xspec) return;

	/* Expand the xspec list */
	set->nxspec++;
	set->xspecs = GETMEM(set->xspecs, CATSPEC, set->nxspec);
	csnew = set->xspecs + (set->nxspec-1);
	init_catspec(csnew);

	/* Copy the given xspec into the new one */
	copy_catspec(csnew, xspec);
	}

/**********************************************************************/

/*********************************************************************/
/** Invoke a given set's category specifications.
 *
 *	@param[in] 	set		given set
 *********************************************************************/
void	invoke_set_catspecs

	(
	SET		set
	)

	{
	int		i;
	ITEM	item;

	/* Return if nothing to change */
	if (!set) return;

	item = set->bgnd;
	if (item)
		{
		invoke_item_catspec(set->type, item, set->ncspec, set->cspecs);
		}
	for (i=0; i<set->num; i++)
		{
		item = set->list[i];
		if (item)
			{
			invoke_item_catspec(set->type, item, set->ncspec, set->cspecs);
			if (same(set->type, "lchain"))
				invoke_item_catspec("nodes", item, set->nxspec, set->xspecs);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      c h a n g e _ s e t _ p s p e c                                 *
*      r e c a l l _ s e t _ p s p e c                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Override the presentation specs of all items in the given set.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	param	name of parameter to override
 *	@param[in] 	value	value to override with
 *********************************************************************/

void	change_set_pspec

	(
	SET		set,
	PPARAM	param,
	POINTER	value
	)

	{
	int		i;

	/* Do nothing if set does not exist */
	if (!set) return;

	change_item_pspec(set->type, set->bgnd, param, value);
	for (i=0; i<set->num; i++)
		{
		change_item_pspec(set->type, set->list[i], param, value);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the presentation specs of all items in the given set.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	param		name of parameter to retrieve
 *	@param[in] 	value		value retrieved
 *********************************************************************/
void	recall_set_pspec

	(
	SET		set,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if set does not exist */
	if (!set) return;

	recall_item_pspec(set->type, set->bgnd, param, value);
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ s e t                                       *
*      h i g h l i g h t _ s e t _ c a t e g o r y                     *
*      h i g h l i g h t _ s e t _ s e c o n d a r y                   *
*      h i g h l i g h t _ s e t _ s e c o n d a r y _ c a t e g o r y *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set or reset the highlight code for all items in the given set.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	code	hilite code to set
 *********************************************************************/

void	highlight_set

	(
	SET		set,
	HILITE	code
	)

	{
	int		i, fcode;
	CATSPEC	*cspec;

	/* Return if nothing to change */
	if (!set) return;

	/* Set category spec highlight code */
	fcode = MIN(code, 0);
	for (i=0; i<set->ncspec; i++)
		{
		cspec = set->cspecs + i;
		define_lspec_value(&cspec->lspec, LINE_HILITE, (POINTER) &code);
		define_fspec_value(&cspec->fspec, FILL_HILITE, (POINTER) &fcode);
		define_tspec_value(&cspec->tspec, TEXT_HILITE, (POINTER) &code);
		define_mspec_value(&cspec->mspec, MARK_HILITE, (POINTER) &code);
		define_bspec_value(&cspec->bspec, BARB_HILITE, (POINTER) &code);
		}

	highlight_item(set->type, set->bgnd, code);
	for (i=0; i<set->num; i++)
		{
		highlight_item(set->type, set->list[i], code);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset the category spec highlight code for all items
 * in the given set.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	category	given category
 *	@param[in] 	code		hilite code
 *********************************************************************/
void	highlight_set_category

	(
	SET		set,
	STRING	category,
	HILITE	code
	)

	{
	int		i, fcode;
	CATSPEC	*cspec;

	/* Return if nothing to change */
	if (!set) return;

	/* Set category spec highlight code */
	fcode = MIN(code, 0);
	for (i=0; i<set->ncspec; i++)
		{
		cspec = set->cspecs + i;
		if (!same(cspec->name, category)) continue;
		define_lspec_value(&cspec->lspec, LINE_HILITE, (POINTER) &code);
		define_fspec_value(&cspec->fspec, FILL_HILITE, (POINTER) &fcode);
		define_tspec_value(&cspec->tspec, TEXT_HILITE, (POINTER) &code);
		define_mspec_value(&cspec->mspec, MARK_HILITE, (POINTER) &code);
		define_bspec_value(&cspec->bspec, BARB_HILITE, (POINTER) &code);
		}

	highlight_item_category(set->type, set->bgnd, category, code);
	for (i=0; i<set->num; i++)
		{
		highlight_item_category(set->type, set->list[i], category, code);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset the highlight code for all secondary items in the
 * given set.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	code	hilite code to set
 *********************************************************************/

void	highlight_set_secondary

	(
	SET		set,
	HILITE	code
	)

	{
	int		i, fcode;
	CATSPEC	*xspec;

	/* Return if nothing to change */
	if (!set) return;

	/* Set category spec highlight code */
	fcode = MIN(code, 0);
	for (i=0; i<set->nxspec; i++)
		{
		xspec = set->xspecs + i;
		define_lspec_value(&xspec->lspec, LINE_HILITE, (POINTER) &code);
		define_fspec_value(&xspec->fspec, FILL_HILITE, (POINTER) &fcode);
		define_tspec_value(&xspec->tspec, TEXT_HILITE, (POINTER) &code);
		define_mspec_value(&xspec->mspec, MARK_HILITE, (POINTER) &code);
		define_bspec_value(&xspec->bspec, BARB_HILITE, (POINTER) &code);
		}

	/* Note that only link chains have secondary presentation */
	for (i=0; i<set->num; i++)
		{
		if (same(set->type, "lchain"))
			highlight_item("nodes", set->list[i], code);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Set or reset the category spec highlight code for all secondary
 * items in the given set.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	category	given category
 *	@param[in] 	code		hilite code
 *********************************************************************/
void	highlight_set_secondary_category

	(
	SET		set,
	STRING	category,
	HILITE	code
	)

	{
	int		i, fcode;
	CATSPEC	*xspec;

	/* Return if nothing to change */
	if (!set) return;

	/* Set category spec highlight code */
	fcode = MIN(code, 0);
	for (i=0; i<set->nxspec; i++)
		{
		xspec = set->xspecs + i;
		if (!same(xspec->name, category)) continue;
		define_lspec_value(&xspec->lspec, LINE_HILITE, (POINTER) &code);
		define_fspec_value(&xspec->fspec, FILL_HILITE, (POINTER) &fcode);
		define_tspec_value(&xspec->tspec, TEXT_HILITE, (POINTER) &code);
		define_mspec_value(&xspec->mspec, MARK_HILITE, (POINTER) &code);
		define_bspec_value(&xspec->bspec, BARB_HILITE, (POINTER) &code);
		}

	/* Note that only link chains have secondary presentation */
	for (i=0; i<set->num; i++)
		{
		if (same(set->type, "lchain"))
			highlight_item_category("nodes", set->list[i], category, code);
		}
	}

/***********************************************************************
*                                                                      *
*      s t r i p _ s e t                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Remove items from the given set that are outside the given box
 * definition.
 *
 *	@param[in] 	set		given set
 *	@param[in] 	*box	given box
 *********************************************************************/

void	strip_set

	(
	SET			set,
	const BOX	*box
	)

	{
	int		i, j;
	ITEM	item;

	/* Do nothing if set or box not there */
	if (!set) return;
	if (!box) return;

	for (i=0, j=0; i<set->num; i++)
		{
		item = set->list[i];

		/* Remove item from set if outside box */
		if (! inbox_item(set->type, item, box))
			{
			set->list[i] = destroy_item(set->type, item);
			}

		/* Otherwise move it up if necessary */
		else
			{
			if (j < i)
				{
				set->list[i] = NullItem;
				set->list[j] = item;
				}
			j++;
			}
		}

	/* Reset size of set */
	set->num = j;
	}

/***********************************************************************
*                                                                      *
*      r e p r o j e c t _ s e t                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reproject the given set from the first map projection to the second.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	*smproj		initial projection
 *	@param[in] 	*tmproj		destination projection
 * 	@return Ture if successful.
 *********************************************************************/

LOGICAL		reproject_set

	(
	SET				set,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	LOGICAL	reproj, remap, rescale;
	int		i;
	float	sunits, tunits, sfact;
	ITEM	item;

	/* Do nothing if set is null */
	if (!set) return FALSE;

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

	/* Remap the background */
	if (rescale) (void) scale_item(set->type, set->bgnd, sfact, sfact);
	else         (void) reproject_item(set->type, set->bgnd, smproj, tmproj);

	/* Remap all items in the set */
	for (i=0; i<set->num; i++)
		{
		/* Reproject all points in each item */
		item = set->list[i];
		if (rescale) (void) scale_item(set->type, item, sfact, sfact);
		else         (void) reproject_item(set->type, item, smproj, tmproj);
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      s e t _ c o u n t                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Count the number of items in the given set, with the given category.
 *
 *	@param[in] 	set			given set
 *	@param[in] 	category	category to match
 * 	@return The number items in the set that match the category.
 *********************************************************************/

int		set_count

	(
	SET		set,
	STRING	category
	)

	{
	int		i, count=0;
	STRING	cat;
	LOGICAL	rev=FALSE;

	if (!set)            return 0;
	if (set->num <= 0)   return 0;
	if (blank(category)) return set->num;

	if (category[0] == '!')
		{
		category++;
		rev = TRUE;
		}

	for (i=0; i<set->num; i++)
		{
		cat = item_attribute(set->type, set->list[i], AttribCategory);
		if (!rev && !same(category, cat)) continue;
		if ( rev &&  same(category, cat)) continue;
		count ++;
		}

	return count;
	}

/***********************************************************************
*                                                                      *
*      p r e p a r e _ s e t                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Build set members according to presentation specs.
 *
 *	@param[in] 	set		given set
 *********************************************************************/

void	prepare_set

	(
	SET		set
	)

	{
	if (IsNull(set))   return;
	if (set->num <= 0) return;

	if (same(set->type, "spot"))
		{
		prepare_spot_set(set);
		invoke_set_catspecs(set);
		}
	}
