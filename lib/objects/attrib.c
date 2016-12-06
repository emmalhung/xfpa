/***********************************************************************/
/**	@file	attrib.c
 *
 * Routines to handle the ATTRIB and ATTRIB_LIST objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*      a t t r i b . c                                                 *
*                                                                      *
*      Routines to handle the ATTRIB and ATTRIB_LIST objects.          *
*                                                                      *
*     Version 4 (c) Copyright 1997 Environment Canada (AES)            *
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

#define ATTRIB_INIT
#include "attrib.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_macros.h>

int		AttribListCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ a t t r i b _ l i s t                             *
*      d e s t r o y _ a t t r i b _ l i s t                           *
*      e m p t y _ a t t r i b _ l i s t                               *
*      c l e a n _ a t t r i b _ l i s t                               *
*      c o p y _ a t t r i b _ l i s t                                 *
*                                                                      *
*      Create, destroy, empty, copy an attrib_list.                    *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Create an attribute list.
 *
 * @return pointer to new attribute list.
 ***********************************************************************/
ATTRIB_LIST	create_attrib_list

	(void)

	{
	ATTRIB_LIST	al;

	/* Allocate memory for structure */
	al = INITMEM(struct ATTRIB_LIST_struct, 1);
	if (!al) return NullAttribList;

	/* Initialize the structure */
	al->attribs  = NullAttribPtr;
	al->nattribs = 0;
	al->defs     = NullPointer;

	/* Return the new attrib_list */
	AttribListCount++;
	return al;
	}

/**********************************************************************/

/***********************************************************************/
/**	Destroy an attribute list.
 *
 * Free all memory used by list.
 *
 * @param[in]	al	attribute list to destroy.
 * @return NullAttribList pointer.
 ***********************************************************************/
ATTRIB_LIST	destroy_attrib_list

	(
	ATTRIB_LIST	al
	)

	{
	if ( IsNull(al) ) return NullAttribList;

	empty_attrib_list(al);
	FREEMEM(al);
	return NullAttribList;
	}

/**********************************************************************/

/***********************************************************************/
/**	Empty an attribute list.
 *
 * Remove all elements from attribute list.
 *
 * @param[in]	al	attribute list to empty.
 ***********************************************************************/
void	empty_attrib_list

	(
	ATTRIB_LIST	al
	)

	{
	int		ia;

	if ( IsNull(al) ) return;

	for (ia=0; ia<al->nattribs; ia++)
		{
		free_attrib(al->attribs+ia);
		}
	FREEMEM(al->attribs);
	al->nattribs = 0;
	}

/**********************************************************************/

/***********************************************************************/
/**	Clean out attribute list
 *
 * Removes any empty attributes from list.
 *
 * @param[in]	al	attribute list to clean.
 ***********************************************************************/
void	clean_attrib_list

	(
	ATTRIB_LIST	al
	)

	{
	int		ia, na;
	ATTRIB	*att;

	if ( IsNull(al) ) return;

	na = al->nattribs;
	for (ia=na-1; ia>=0; ia--)
		{
		att = al->attribs + ia;
		if ( blank(att->value) ) (void) remove_attribute(al, att->name);
		}

	if (al->nattribs < na)
		al->attribs = GETMEM(al->attribs, ATTRIB, al->nattribs);
	}

/**********************************************************************/

/***********************************************************************/
/**	Copy an attribute list
 *
 * Make a duplicate attribute list.
 *
 * @param	al	attribute list to make a copy of.
 * @return pointer to copy of list.
 ***********************************************************************/
ATTRIB_LIST	copy_attrib_list

	(
	ATTRIB_LIST	al
	)

	{
	ATTRIB_LIST	copy;
	int			na, ia;

	if ( IsNull(al) )   return NullAttribList;

	copy = create_attrib_list();
	if ( IsNull(copy) ) return NullAttribList;
	copy->defs = al->defs;
	na = al->nattribs;
	if ( na <= 0 )      return copy;

	copy->nattribs = na;
	copy->attribs  = INITMEM(ATTRIB, na);
	for (ia=0; ia<na; ia++)
		{
		init_attrib(copy->attribs+ia);
		copy_attrib(copy->attribs+ia, al->attribs+ia);
		}
	return copy;
	}

/***********************************************************************
*                                                                      *
*      a d d _ a t t r i b u t e                                       *
*      s e t _ a t t r i b u t e                                       *
*      g e t _ a t t r i b u t e                                       *
*      r e m o v e _ a t t r i b u t e                                 *
*      f i n d _ a t t r i b u t e                                     *
*                                                                      *
*      Add, set, get an attribute value.  Find an attribute.           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Add an attribute to an attribute list.
 *
 * @param[in]	al		list to add attribute to.
 * @param[in]	name	name of attribute to add.
 * @param[in]	val		value of attribute to add.
 * @return	pointer to new attribute in list.
 ***********************************************************************/
ATTRIB	*add_attribute

	(
	ATTRIB_LIST	al,
	STRING		name,
	STRING		val
	)

	{
	ATTRIB	*att;
	int		na;

	if ( IsNull(al) ) return NullAttribPtr;

	att = find_attribute(al, name);
	if ( IsNull(att) )
		{
		na = ++(al->nattribs);
		al->attribs = GETMEM(al->attribs, ATTRIB, na);
		att = al->attribs + na-1;
		init_attrib(att);
		}

	define_attrib(att, name, val);
	return att;
	}

/**********************************************************************/

/***********************************************************************/
/**	Reset the value of an attribute in an attribute list.
 *
 * Set the value of a named attribute.
 *
 * @param[in]	al		list edit attribute in.
 * @param[in]	name	name of attribute to edit.
 * @param[in]	val		new value of attribute.
 * @return	pointer to changed attribute in list. NullAttributePrt if edit failed.
 ***********************************************************************/
ATTRIB	*set_attribute

	(
	ATTRIB_LIST	al,
	STRING		name,
	STRING		val
	)

	{
	ATTRIB	*att;

	if ( IsNull(al) )  return NullAttribPtr;

	att = find_attribute(al, name);
	if ( IsNull(att) ) return NullAttribPtr;

	define_attrib(att, name, val);
	return att;
	}

/**********************************************************************/

/***********************************************************************/
/**	Lookup an attribute value from an attribute list.
 *
 * Lookup the value of an attribute given the attribute's name.
 *
 * @param[in]	al		list to be examined.
 * @param[in]	name	name of attribute lookup
 * @param[out]	val		value of attribute.
 * @return	pointer to attribute in list. NullAttributePrt if edit failed.
 ***********************************************************************/
ATTRIB	*get_attribute

	(
	ATTRIB_LIST	al,
	STRING		name,
	STRING		*val
	)

	{
	ATTRIB	*att;

	if ( NotNull(val) ) *val = NullString;
	if ( IsNull(al) )   return NullAttribPtr;

	att = find_attribute(al, name);
	if ( IsNull(att) ) return NullAttribPtr;

	recall_attrib(att, NullStringPtr, val);
	return att;
	}

/**********************************************************************/

/***********************************************************************/
/**	Remove an attribute value from an attribute list.
 *
 * @param[in]	al		list to be examined.
 * @param[in]	name	name of attribute to remove.
 * @return	NullAttributePrt
 ***********************************************************************/
ATTRIB	*remove_attribute

	(
	ATTRIB_LIST	al,
	STRING		name
	)

	{
	ATTRIB	*att;
	int		ia, ja;

	if ( IsNull(al) )  return NullAttribPtr;

	for (ia=0; ia<al->nattribs; ia++)
		{
		att = al->attribs + ia;
		if ( same_ic(name, att->name) ) break;
		}
	if ( ia >= al->nattribs ) return NullAttribPtr;

	for (ja=ia; ja<al->nattribs; ja++)
		{
		att = al->attribs + ja;
		if ( ja < al->nattribs-1 ) copy_attrib(att, att+1);
		else                       free_attrib(att);
		}
	al->nattribs--;

	return NullAttribPtr;
	}

/**********************************************************************/

/***********************************************************************/
/**	Find an attribute item in an attribute list.
 *
 * @param[in]	al		list to be examined.
 * @param[in]	name	name of attribute to find.
 * @return	Pointer to attribute item for named attribute or
 * NullAttributePrt if it could not be found.
 ***********************************************************************/
ATTRIB	*find_attribute

	(
	ATTRIB_LIST	al,
	STRING		name
	)

	{
	ATTRIB	*att;
	int		ia;

	if ( IsNull(al) )   return NullAttribPtr;

	for (ia=0; ia<al->nattribs; ia++)
		{
		att = al->attribs + ia;
		if ( same_ic(name, att->name) ) return att;
		}
	return NullAttribPtr;
	}

/***********************************************************************
*                                                                      *
*      i n i t _ a t t r i b                                           *
*      f r e e _ a t t r i b                                           *
*      c o p y _ a t t r i b                                           *
*                                                                      *
*      Initialize, free and copy an attrib.                            *
*                                                                      *
***********************************************************************/
/***********************************************************************/
/**	 Initialize an attribute item.
 *
 * @param[in]	*att	item to initialize.
 ***********************************************************************/
void	init_attrib

	(
	ATTRIB	*att
	)

	{
	if ( IsNull(att) ) return;

	att->name  = NullString;
	att->value = NullString;
	}

/**********************************************************************/

/***********************************************************************/
/**	Free memory for an attribute item.
 *
 * @param[in]	*att	attribute item to free.
 ***********************************************************************/
void	free_attrib

	(
	ATTRIB	*att
	)

	{
	if ( IsNull(att) ) return;

	FREEMEM(att->name);
	FREEMEM(att->value);
	}

/**********************************************************************/

/***********************************************************************/
/**	Make an exact copy of an attribute item.
 *
 * @param[in]	*to		attribute item to copy.
 * @param[out]	*from	copy of given attribute item.
 ***********************************************************************/
void	copy_attrib

	(
	ATTRIB			*to,
	const ATTRIB	*from
	)

	{
	if ( IsNull(to) )   return;
	if ( IsNull(from) ) return;

	define_attrib(to, from->name, from->value);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ a t t r i b                                       *
*      r e c a l l _ a t t r i b                                       *
*                                                                      *
*      Define, recall attribute values.                                *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Define an attribute value.
 *
 * Set the attribute name and value.
 *
 * @param[in]	*att	attribute item to set.
 * @param[in]	name	name of attribute.
 * @param[in]	value	value of attribute.
 ***********************************************************************/
void	define_attrib

	(
	ATTRIB	*att,
	STRING	name,
	STRING	value
	)

	{
	if ( IsNull(att) ) return;

	if ( NotNull(name) && !same_ic(name, att->name) )
		{
		att->name = STRMEM(att->name,  name);
		}
	if ( NotNull(value) )
		{
		att->value = STRMEM(att->value, value);
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Recall the name and value of a given attribute item.
 *
 * @param[in]	*att	attribute item to examine.
 * @param[out]	*name	name of attribute.
 * @param[out]	*value	value of attribute.
 ***********************************************************************/
void	recall_attrib

	(
	ATTRIB	*att,
	STRING	*name,
	STRING	*value
	)

	{
	if ( NotNull(name) )  *name  = NullString;
	if ( NotNull(value) ) *value = NullString;
	if ( IsNull(att) )    return;

	if ( NotNull(name) )  *name  = att->name;
	if ( NotNull(value) ) *value = att->value;
	}

/***********************************************************************
*                                                                      *
*      c r e a t e _ d e f a u l t _ a t t r i b _ l i s t             *
*      a d d _ d e f a u l t _ a t t r i b u t e s                     *
*      s e t _ d e f a u l t _ a t t r i b u t e s                     *
*      g e t _ d e f a u l t _ a t t r i b u t e s                     *
*                                                                      *
*      Manage an attribute list with the minimum (default) ones.       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Create the default attribute list.
 *
 * Creates and initializes an attribute list with the default FPA attributes.
 *
 * @param[in]	cat		FPA category
 * @param[in]	alab	FPA auto label
 * @param[in]	ulab	FPA user label
 * @return pointer to attribute list containing default FPA attributes.
 ***********************************************************************/
ATTRIB_LIST	create_default_attrib_list

	(
	STRING	cat,
	STRING	alab,
	STRING	ulab
	)

	{
	ATTRIB_LIST	al;

	/* Create an empty attribute list */
	al = create_attrib_list();

	/* Insert the default attributes only */
	add_default_attributes(al, cat, alab, ulab);
	return al;
	}

/******************************************************************************/

/***********************************************************************/
/**	Add the default FPA attributes to an attribute list.
 *
 * @param[in]	al		attribute list to add to.
 * @param[in]	cat		FPA category
 * @param[in]	alab	FPA auto label
 * @param[in]	ulab	FPA user label
 ***********************************************************************/
void    add_default_attributes

	(
	ATTRIB_LIST	al,
	STRING		cat,
	STRING		alab,
	STRING		ulab
	)

	{
	if ( IsNull(al) ) return;
	(void) add_attribute(al, AttribCategory,  cat);
	(void) add_attribute(al, AttribAutolabel, alab);
	(void) add_attribute(al, AttribUserlabel, ulab);
	}

/******************************************************************************/

/***********************************************************************/
/**	Set the default FPA attributes in an attribute list.
 *
 * Reset default attributes already contained in the list.
 *
 * @param[in]	al		list to set defaults on.
 * @param[in]	cat		FPA category.
 * @param[in]	alab	FPA auto label.
 * @param[in]	ulab	FPA user label.
 ***********************************************************************/
void    set_default_attributes

	(
	ATTRIB_LIST	al,
	STRING		cat,
	STRING		alab,
	STRING		ulab
	)

	{
	if ( IsNull(al) ) return;
	(void) set_attribute(al, AttribCategory,  cat);
	(void) set_attribute(al, AttribAutolabel, alab);
	(void) set_attribute(al, AttribUserlabel, ulab);
	}

/******************************************************************************/

/***********************************************************************/
/**	Lookup values for Default FPA attributes in a given attribute list.
 *
 * @param[in]	al	attribute list to examine.
 * @param[out]	*cat	FPA category.
 * @param[out]	*alab	FPA auto label.
 * @param[out]	*ulab	FPA user label.
 ***********************************************************************/
void    get_default_attributes

	(
	ATTRIB_LIST	al,
	STRING		*cat,
	STRING		*alab,
	STRING		*ulab
	)

	{
	if (cat)  *cat  = NULL;
	if (alab) *alab = NULL;
	if (ulab) *ulab = NULL;
	if ( IsNull(al) ) return;
	if (cat)  (void) get_attribute(al, AttribCategory,  cat);
	if (alab) (void) get_attribute(al, AttribAutolabel, alab);
	if (ulab) (void) get_attribute(al, AttribUserlabel, ulab);
	}

/***********************************************************************
*                                                                      *
*      s a m e _ a t t r i b _ l i s t                                 *
*                                                                      *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	 Comare two attribute lists.
 *
 * Do the 2 given attribute lists match?  (Order doesn't matter)
 *
 * @param[in]	al1	first attribute list.
 * @param[in]	al2	second attribute list.
 * @return True if attribute lists match.
 ***********************************************************************/
LOGICAL	same_attrib_list

	(
	const ATTRIB_LIST	al1,
	const ATTRIB_LIST	al2
	)

	{
	int		ia;
	ATTRIB	*att1, *att2;
	STRING	name, val1, val2;

	if (IsNull(al1) && IsNull(al2))     return TRUE;
	if (IsNull(al1))                    return FALSE;
	if (IsNull(al2))                    return FALSE;
	if (al1->nattribs != al2->nattribs) return FALSE;

	for (ia=0; ia<al1->nattribs; ia++)
		{
		att1 = al1->attribs + ia;
		recall_attrib(att1, &name, &val1);

		att2 = get_attribute(al2, name, &val2);
		if (IsNull(att2))      return FALSE;
		if (!same(val1, val2)) return FALSE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      d e b u g _ a t t r i b _ l i s t                               *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Print the contents of the given ATTRIB_LIST.
 *
 * @param[in]	msg	diagnostic message.
 * @param[in]	al	attribute list to print.
 ***********************************************************************/
void	debug_attrib_list

	(
	STRING		msg,
	ATTRIB_LIST	al
	)

	{
	int		ia;
	ATTRIB	*att;
	STRING	name, val;

	if (IsNull(al))        return;
	if (al->nattribs <= 0) return;

	if (!blank(msg)) (void) printf("%s:\n", msg);

	for (ia=0; ia<al->nattribs; ia++)
		{
		att = al->attribs + ia;
		recall_attrib(att, &name, &val);
		(void) printf("\t%s:\t\'%s\'\n", name, val);
		}
	}
