/**********************************************************************/
/** @file area.c
 *
 *  Routines to handle the AREA and SUBAREA objects.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*      a r e a . c                                                     *
*                                                                      *
*      Routines to handle the AREA and SUBAREA objects.                *
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

#define AREA_INIT
#include "area.h"

#include <tools/tools.h>

#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

/***********************************************************************
*                                                                      *
*      Routines specific to the AREA object                            *
*                                                                      *
***********************************************************************/

int		AreaCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ a r e a                                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create a new area with given attributes
 *
 * @param[in] 	subelem	subelement.
 * @param[in]	value	value assigned to the enclosed area.
 * @param[in]	label	label assigned to the enclosed area.
 * @return	new area object.
 * 			(You should destroy the area when you are finished with it)
 ***********************************************************************/
AREA	create_area

	(
	STRING	subelem,	/* subelement */
	STRING	value,		/* value assigned to the enclosed area */
	STRING	label		/* label assigned to the enclosed area */
	)

	{
	AREA	anew;

	/* Allocate space for the structure */
	anew = INITMEM(struct AREA_struct, 1);
	if (!anew) return NullArea;

	/* Initialize the structure */
	anew->bound    = NullBound;
	anew->attrib   = NullAttribList;
	anew->subelem  = NULL;
	anew->value    = NULL;
	anew->label    = NULL;
	anew->numdiv   = 0;
	anew->divlines = NullLineList;
	anew->subids   = NullInt;
	anew->subareas = NullSubAreaList;
	anew->visready = FALSE;
	init_lspec(&anew->lspec);
	init_fspec(&anew->fspec);

	/* Set area value */
	define_area_value(anew, subelem, value, label);

	/* Return the new area */
	AreaCount++;
	return anew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ a r e a                                         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Destroy space allocated to the given area.
 *
 * 	@param[in]	area	area to be destroyed.
 * 	@return NullArea pointer.
 ***********************************************************************/
AREA	destroy_area

	(
	AREA	area	/* area to be destroyed */
	)

	{
	/* Do nothing if area not there */
	if (!area) return NullArea;

	/* Free the space used by attributes */
	FREEMEM(area->subelem);
	FREEMEM(area->value);
	FREEMEM(area->label);
	destroy_attrib_list(area->attrib);

	/* Free the space used by boundary and subarea buffer */
	empty_area(area);
	destroy_bound(area->bound);

	/* Free the space used by presentation specs */
	free_lspec(&area->lspec);
	free_fspec(&area->fspec);

	/* Free the structure itself */
	FREEMEM(area);
	AreaCount--;
	return NullArea;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ a r e a                                             *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Empty the data buffers of the given area.
 *
 * 	@param[in]	area	area to empty.
 ***********************************************************************/
void	empty_area

	(
	AREA	area	/* area to be emptied */
	)

	{
	int		i, ndiv, nsub;

	/* Do nothing if area not there */
	if (!area) return;

	/* Free the space used by subarea and division buffers */
	if ((ndiv = area->numdiv) > 0 || area->subareas)
		{
		nsub = ndiv + 1;
		nsub = MAX(nsub, 0);

		/* Free the subarea buffer */
		for (i=0; i<nsub; i++)
			(void) destroy_subarea(area->subareas[i]);
		FREEMEM(area->subareas);

		/* Free the subarea id buffer */
		FREEMEM(area->subids);

		/* Free the division buffer */
		for (i=0; i<ndiv; i++)
			(void) destroy_line(area->divlines[i]);
		FREEMEM(area->divlines);

		/* Zero the division counter */
		area->numdiv = 0;
		}

	/* Free the space used by whole boundary and holes */
	empty_bound(area->bound);
	}

/***********************************************************************
*                                                                      *
*      c o p y _ a r e a                                               *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create an exact copy of an area.
 *
 * @param[in]	area	area to be copied.
 * @param[in]	all		do we want to copy the whole structure?
 * @return	Pointer to copy of area.
 * 			(You should destroy the copy when you are finished with it)
 ***********************************************************************/
AREA	copy_area

	(
	const AREA	area,	/* area to be copied */
	LOGICAL		all		/* do we want to copy the whole structure */
	)

	{
	int		ndiv, nsub, idiv, isub, isv, iseg, ihole, j;
	AREA	anew;
	SUBAREA	snew;
	SUBVIS	svis;
	SEGMENT	seg, xseg;
	LINE	hole, xline;

	/* Do nothing if area not there */
	if (!area) return NullArea;

	/* Create an empty copy */
	anew = create_area(NULL, NULL, NULL);
	define_area_attribs(anew, area->attrib);

	/* Duplicate the whole boundary with holes */
	anew->bound = copy_bound(area->bound);

	/* Duplicate presentation specs */
	copy_lspec(&anew->lspec, &area->lspec);
	copy_fspec(&anew->fspec, &area->fspec);

	/* Return now if no area boundary */
	if (!anew->bound) return anew;

	/* Duplicate the divisions and subareas */
	if ((ndiv = area->numdiv) > 0 || area->subareas)
		{
		nsub = ndiv + 1;
		anew->numdiv   = ndiv;
		anew->divlines = GETMEM(anew->divlines, LINE, ndiv);
		anew->subids   = GETMEM(anew->subids,   int,  ndiv);
		anew->subareas = GETMEM(anew->subareas, SUBAREA, nsub);

		for (idiv=0; idiv<ndiv; idiv++)
			{
			anew->divlines[idiv] = copy_line(area->divlines[idiv]);
			anew->subids[idiv]   = area->subids[idiv];
			}

		for (isub=0; isub<nsub; isub++)
			{
			snew = copy_subarea(area->subareas[isub], all);
			anew->subareas[isub] = snew;
			if (!snew) continue;

			/* Force the segments of the new subarea to point to the */
			/* corresponding lines in anew */
			for (iseg=0; iseg<snew->numseg; iseg++)
				{
				seg = snew->segments[iseg];

				/* Is it a dividing line? */
				j = which_area_divide(area, seg->line);
				if (j >= 0)
					{
					seg->line = anew->divlines[j];
					continue;
					}

				/* If not, is it a hole in the main area? */
				j = which_bound_hole(area->bound, seg->line);
				if (j >= 0)
					{
					seg->line = anew->bound->holes[j];
					continue;
					}

				/* If not, is it the main boundary? */
				if (seg->line == area->bound->boundary)
					{
					seg->line = anew->bound->boundary;
					continue;
					}

				/* If not, we're in trouble! */
				seg->line = NullLine;
				}

			if (!all) continue;

			/* Force the visible segments of the new subarea to point to the */
			/* corresponding lines in anew */
			for (isv=0; isv<snew->nsubvis; isv++)
				{
				svis = snew->subvis[isv];

				for (iseg=0; iseg<svis->numvis; iseg++)
					{
					seg = svis->segvis[iseg];

					/* Is it a dividing line? */
					j = which_area_divide(area, seg->line);
					if (j >= 0)
						{
						seg->line = anew->divlines[j];
						continue;
						}

					/* If not, is it a hole in the main area? */
					j = which_bound_hole(area->bound, seg->line);
					if (j >= 0)
						{
						seg->line = anew->bound->holes[j];
						continue;
						}

					/* If not, is it the main boundary? */
					if (seg->line == area->bound->boundary)
						{
						seg->line = anew->bound->boundary;
						continue;
						}

					/* If not, this may be an exclusive connection between */
					/*  a dividing line and a hole!                        */
					/* If so, duplicate line and make segment exclusive    */
					xseg = area->subareas[isub]->subvis[isv]->segvis[iseg];
					if (seg_exclusive(xseg))
						{
						xline     = copy_line(xseg->line);
						seg->line = xline;
						trim_segment_buffer(seg);
						continue;
						}

					/* If not, we're in trouble! */
					seg->line = NullLine;
					}
				}

			/* Force the holes of the new subarea to point to the */
			/* corresponding holes in anew */
			for (ihole=0; ihole<snew->numhole; ihole++)
				{
				hole = snew->holes[ihole];

				/* Is it a hole in the main area? */
				j = which_bound_hole(area->bound, hole);
				if (j >= 0)
					{
					snew->holes[ihole] = anew->bound->holes[j];
					continue;
					}

				/* If not, we're in trouble! */
				snew->holes[ihole] = NullLine;
				}
			}
		}

	/* Set flag for visible subareas */
	if (all) anew->visready = area->visready;
	else     anew->visready = FALSE;
	return anew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ a r e a                                           *
*      d e f i n e _ a r e a _ b o u n d a r y                         *
*      d e f i n e _ a r e a _ h o l e s                               *
*      a d d _ a r e a _ h o l e                                       *
*      r e m o v e _ a r e a _ h o l e                                 *
*      r e m o v e _ a l l _ a r e a _ h o l e s                       *
*                                                                      *
*      Set or reset the boundary or holes of the given area.           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set or reset the boundary of the given area.
 *
 * @param[in]	area		area to set.
 * @param[in]	boundary	line to define boundary.
 * @param[in]	numhole		number of holes.
 * @param[in]	*holes		list of holes.
 ***********************************************************************/
void	define_area

	(
	AREA		area,		/* given area */
	LINE		boundary,	/* line to define boundary */
	int			numhole,	/* number of holes */
	const LINE	*holes		/* list of holes */
	)

	{
	/* Do nothing if area not there */
	if (!area) return;

	/* Clear subarea and division line buffers and */
	/* create an empty bound if necessary */
	empty_area(area);
	if (!area->bound) area->bound = create_bound();

	/* Define the boundary and holes */
	define_bound(area->bound, boundary, numhole, holes);
	}

/**********************************************************************/

/***********************************************************************/
/**	Set or reset the boundary of the given area.
 *
 * @param[in]	area		area to set.
 * @param[in]	boundary	line to define boundary.
 ***********************************************************************/
void	define_area_boundary

	(
	AREA	area,		/* given area */
	LINE	boundary	/* line to define boundary */
	)

	{
	/* Do nothing if area not there */
	if (!area) return;

	/* Clear subarea and division line buffers and */
	/* create an empty bound if necessary */
	empty_area(area);
	if (!area->bound) area->bound = create_bound();

	/* Define the boundary */
	define_bound_boundary(area->bound, boundary);
	}

/**********************************************************************/

/***********************************************************************/
/**	Set or reset the given area with holes.
 *
 * @param[in]	area	area to be given holes.
 * @param[in]	numhole	number of holes to add.
 * @param[in]	*holes	list of holes to add.
 ***********************************************************************/
void	define_area_holes

	(
	AREA		area,		/* given area */
	int			numhole,	/* number of holes */
	const LINE	*holes		/* list of holes */
	)

	{
	/* Do nothing if area not there */
	if (!area) return;

	/* Clear subarea and division line buffers and */
	/* create an empty bound if necessary */
	empty_area(area);
	if (!area->bound) area->bound = create_bound();

	/* Define the new hole list */
	define_bound_holes(area->bound, numhole, holes);
	}

/**********************************************************************/

/***********************************************************************/
/**	Add a hole to the given area.
 *
 * @param[in]	area	area to be given the hole.
 * @param[in]	hole	hole to add.
 ***********************************************************************/
void	add_area_hole

	(
	AREA	area,	/* given area */
	LINE	hole	/* hole to add */
	)

	{
	/* Do nothing if area not there */
	if (!area)        return;
	if (!hole)        return;
	if (!area->bound) return;

	/* Add the new hole to the list */
	add_bound_hole(area->bound, hole);

	/* Redefine visible portions of area */
	area->visready = FALSE;
	}

/**********************************************************************/

/***********************************************************************/
/** Remove a hole from an area.
 *
 * @param[in]	area	area to have a hole removed.
 * @param[in]	hole	the hole to remove.
 ***********************************************************************/
void	remove_area_hole

	(
	AREA	area,	/* given area */
	LINE	hole	/* hole to remove */
	)

	{
	int		nsub, isub, isv;
	SUBAREA	sub;

	/* Do nothing if area not there */
	if (!area)        return;
	if (!hole)        return;
	if (!area->bound) return;

	/* Remove the hole */
	remove_bound_hole(area->bound, hole);

	/* Hole will be removed                                           */
	/*  ... so remove visible portions of area that might refer to it */
	if (NotNull(area->subareas))
		{
		nsub = area->numdiv + 1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			if (IsNull(sub)) continue;
			if (sub->nsubvis > 0)
				{
				for (isv=0; isv<sub->nsubvis; isv++)
					(void) destroy_subvis(sub->subvis[isv]);
				FREEMEM(sub->subvis);
				sub->nsubvis = 0;
				}
			if (sub->numhole > 0)
				{
				FREEMEM(sub->holes);
				sub->numhole = 0;
				}
			sub->visready = FALSE;
			}
		}

	/* Redefine visible portions of area */
	area->visready = FALSE;
	}

/**********************************************************************/

/***********************************************************************/
/** Remove all holes from an area.
 *
 * @param[in]	area	area to have all holes removed.
 ***********************************************************************/
void	remove_all_area_holes

	(
	AREA	area	/* given area */
	)

	{
	int		ihole, nsub, isub, isv;
	BOUND	bound;
	SUBAREA	sub;

	/* Do nothing if area not there */
	if (!area)        return;
	if (!area->bound) return;

	/* Do nothing if no holes */
	bound = area->bound;
	if (bound->numhole < 1) return;

	/* Holes will be removed                                            */
	/*  ... so remove visible portions of area that might refer to them */
	if (NotNull(area->subareas))
		{
		nsub = area->numdiv + 1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			if (IsNull(sub)) continue;
			if (sub->nsubvis > 0)
				{
				for (isv=0; isv<sub->nsubvis; isv++)
					(void) destroy_subvis(sub->subvis[isv]);
				FREEMEM(sub->subvis);
				sub->nsubvis = 0;
				}
			if (sub->numhole > 0)
				{
				FREEMEM(sub->holes);
				sub->numhole = 0;
				}
			sub->visready = FALSE;
			}
		}

	/* Remove all holes from the list */
	for (ihole=0; ihole<bound->numhole; ihole++)
		(void) destroy_line(bound->holes[ihole]);
	bound->numhole = 0;

	/* Redefine visible portions of area */
	area->visready = FALSE;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ a r e a _ v a l u e                               *
*      r e c a l l _ a r e a _ v a l u e                               *
*                                                                      *
*      Set/reset or retrieve the value of the given area.              *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set or reset the value of the given area.
 *
 * @param[in]	area	area to set.
 * @param[in]	subelem	subelement
 * @param[in]	value	value assigned to the enclosed area.
 * @param[in]	label	label assigned to the enclosed area.
 ***********************************************************************/
void	define_area_value

	(
	AREA	area,		/* given area */
	STRING	subelem,	/* subelement */
	STRING	value,		/* value assigned to the enclosed area */
	STRING	label		/* label assigned to the enclosed area */
	)

	{
	int		nsub;
	SUBAREA	sub;

	/* Do nothing if area not there */
	if (!area) return;

	/* Set given attributes */
	area->subelem = STRMEM(area->subelem,subelem);
	area->value   = STRMEM(area->value  ,value);
	area->label   = STRMEM(area->label  ,label);

	/* Replicate in attributes */
	if (IsNull(area->attrib)) area->attrib = create_attrib_list();
	add_default_attributes(area->attrib, subelem, value, label);

	if (area->subareas)
		{
		nsub = area->numdiv + 1;
		if (nsub == 1)
			{
			sub = area->subareas[0];
			define_subarea_attribs(sub, area->attrib);
			}
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Recall the value of an area.
 *
 * @param[in]	area	area to examine.
 * @param[out]	subelem	subelement.
 * @param[out]	value	value assigned to the enclosed area
 * @param[out]	label	label assigned to the enclosed area
 ***********************************************************************/
void	recall_area_value

	(
	AREA	area,		/* requested area */
	STRING	*subelem,	/* subelement */
	STRING	*value,		/* value assigned to the enclosed area */
	STRING	*label		/* label assigned to the enclosed area */
	)

	{
	/* Initialize return parameters */
	if (subelem) *subelem = NULL;
	if (value)   *value   = NULL;
	if (label)   *label   = NULL;
	if (!area) return;

	/* Retrieve the attributes */
	get_default_attributes(area->attrib, subelem, value, label);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ a r e a _ a t t r i b s                           *
*      r e c a l l _ a r e a _ a t t r i b s                           *
*                                                                      *
*      Set/reset or retrieve the attributes of the given area.         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set or reset the attributes of the given area.
 *
 * @param[in]	area	area to set.
 * @param[in]	attribs	attributes to assign.
 ***********************************************************************/
void	define_area_attribs

	(
	AREA		area,	/* given area */
	ATTRIB_LIST	attribs	/* attributes */
	)

	{
	int		nsub;
	SUBAREA	sub;
	STRING	subelem, value, label;

	/* Do nothing if area not there */
	if (!area) return;

	/* Set given attributes */
	area->attrib = destroy_attrib_list(area->attrib);
	if (NotNull(attribs)) area->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(area->attrib, &subelem, &value, &label);
	area->subelem = STRMEM(area->subelem,subelem);
	area->value   = STRMEM(area->value  ,value);
	area->label   = STRMEM(area->label  ,label);

	if (area->subareas)
		{
		nsub = area->numdiv + 1;
		if (nsub == 1)
			{
			sub = area->subareas[0];
			define_subarea_attribs(sub, area->attrib);
			}
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Recall the attributes of the given area.
 *
 * @param[in]	area	area to lookup.
 * @param[out]	attribs	attributes of the given area.
 ***********************************************************************/
void	recall_area_attribs

	(
	AREA		area,		/* requested area */
	ATTRIB_LIST	*attribs	/* attributes */
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (area) ? area->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ a r e a _ p s p e c                               *
*      r e c a l l _ a r e a _ p s p e c                               *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given       *
*      area.                                                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Set or reset the presentation specs of the given area.
 *
 * @param[in]	area	area to set.
 * @param[in]	param	name of parameter to set.
 * @param[in]	value	value to give parameter.
 ***********************************************************************/
void	define_area_pspec

	(
	AREA	area,
	PPARAM	param,
	POINTER	value
	)

	{
	int		isub, nsub;
	SUBAREA	sub;

	/* Do nothing if area does not exist */
	if (!area) return;

	/* Set the given parameter */
	define_lspec_value(&area->lspec,param,value);
	define_fspec_value(&area->fspec,param,value);

	if (area->subareas)
		{
		nsub = area->numdiv + 1;
		for (isub=0; isub<nsub; isub++)
			{
			sub = area->subareas[isub];
			define_lspec_value(&sub->lspec,param,value);
			define_fspec_value(&sub->fspec,param,value);
			}
		}
	}

/**********************************************************************/

/***********************************************************************/
/** Recall the presentation specs of the given area.
 *
 * @param[in]	area	area to lookup.
 * @param[in]	param	parameter to lookup in area.
 * @param[out]	value	value of the requested parameter.
 ***********************************************************************/
void	recall_area_pspec

	(
	AREA	area,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if area does not exist */
	if (!area) return;

	/* Return the requested parameter */
	recall_lspec_value(&area->lspec,param,value);
	recall_fspec_value(&area->fspec,param,value);
	}

/***********************************************************************
*                                                                      *
*      w h i c h _ a r e a _ h o l e                                   *
*      w h i c h _ a r e a _ d i v i d e                               *
*      w h i c h _ a r e a _ s u b a r e a                             *
*                                                                      *
*      Find the position of the given hole, dividing line or subarea.  *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Find the position of the given hole.
 *
 * @param[in]	area	area to search.
 * @param[in]	line	hole to look for.
 * @return location of the hole in the list of holes. -1 if hole could
 * not be found.
 ***********************************************************************/
int		which_area_hole

	(
	AREA	area,	/* area to search */
	LINE	line	/* hole to look for */
	)

	{
	/* Do nothing if area not there */
	if (!area) return -1;
	if (!line) return -1;

	/* Search the list */
	return (which_bound_hole(area->bound, line));
	}

/**********************************************************************/

/***********************************************************************/
/**	Find the position of the given dividing line.
 *
 * @param[in]	area	area to search.
 * @param[in]	line	dividing line to look for.
 * @return location of the dividing line in the list of dividing lines.
 * -1 if dividing line could not be found.
 ***********************************************************************/
int		which_area_divide

	(
	AREA	area,	/* area to search */
	LINE	line	/* dividing line to look for */
	)

	{
	int		i;

	/* Do nothing if area not there */
	if (!area) return -1;
	if (!line) return -1;

	/* Search the list */
	for (i=0; i<area->numdiv; i++)
	    if (area->divlines[i] == line) return i;

	/* Not found */
	return -1;
	}

/**********************************************************************/

/***********************************************************************/
/**	Find the position of the given subarea.
 *
 * @param[in]	area	area to search.
 * @param[in]	sub		subarea to look for.
 * @return location of the subarea in the list of subareas.
 * -1 if the subarea could not be found.
 ***********************************************************************/
int		which_area_subarea

	(
	AREA	area,	/* area to search */
	SUBAREA	sub		/* subarea to look for */
	)

	{
	int		i;

	/* Do nothing if area not there */
	if (!area) return -1;
	if (!sub)  return -1;

	/* Search the list */
	if (area->subareas)
		{
		for (i=0; i<area->numdiv+1; i++)
			if (area->subareas[i] == sub) return i;
		}

	/* Not found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ a r e a                                     *
*      w i d e n _ a r e a                                             *
*                                                                      *
*      Set the highlight flag for the given area.                      *
*      Note: a negative value means to erase.                          *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set the highlight flag for the given area.
 *
 * @note a negative value means to erase.
 *
 * @param[in]	area	area to highlight.
 * @param[in]	pcode	perimeter highlight code.
 * @param[in]	fcode	fill highlight code.
 ***********************************************************************/
void	highlight_area

	(
	AREA	area,
	HILITE	pcode,
	HILITE	fcode
	)

	{
	int		isub;

	/* Do nothing if no area */
	if (!area) return;

	if (pcode != SkipHilite)
		define_lspec_value(&area->lspec, LINE_HILITE, (POINTER)&pcode);
	if (fcode != SkipHilite)
		define_fspec_value(&area->fspec, FILL_HILITE, (POINTER)&fcode);

	/* Carry invocation to subareas of an area */
	if ( area->subareas )
		{
		for (isub=0; isub<(area->numdiv+1); isub++)
			{
			highlight_subarea(area->subareas[isub], pcode, fcode);
			}
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Set the line width flag for the given area.
 *
 * Looks up the current line width, adds delta and resets the width.
 *
 * @param[in]	area	area to adjust.
 * @param[in]	delta	amount by which to change the width.
 ***********************************************************************/
void	widen_area

	(
	AREA	area,
	float	delta
	)

	{
	int		isub;
	float	width;

	/* Do nothing if no area */
	if (!area) return;

	/* Widen simple outlines by delta - patterned outlines by 1.25 */
	recall_lspec_value(&area->lspec, LINE_WIDTH, (POINTER)&width);
	if (!area->lspec.pattern) width += delta;
	else if (delta > 0.0)     width *= 1.25;
	else if (delta < 0.0)     width /= 1.25;
	define_lspec_value(&area->lspec, LINE_WIDTH, (POINTER)&width);

	/* Carry invocation to subareas of an area */
	if ( area->subareas )
		{
		for (isub=0; isub<(area->numdiv+1); isub++)
			{
			widen_subarea(area->subareas[isub], delta);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      Routines specific to the SUBAREA object                         *
*                                                                      *
***********************************************************************/

int		SubAreaCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ s u b a r e a                                     *
*                                                                      *
*      Create a new subarea with given attributes.                     *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Create a new subarea with given attributes.
 *
 * @param[in]	subelem	subelement.
 * @param[in]	value	value assigned to the enclosed subarea.
 * @param[in]	label	label assigned to the enclosed subarea.
 * @return	pointer to a new subarea. (You should destroy the subarea
 * when you are finished with it).
 ***********************************************************************/
SUBAREA	create_subarea

	(
	STRING	subelem,	/* subelement */
	STRING	value,		/* value assigned to the enclosed subarea */
	STRING	label		/* label assigned to the enclosed subarea */
	)

	{
	SUBAREA	snew;

	/* Allocate space for the structure */
	snew = INITMEM(struct SUBAREA_struct, 1);
	if (!snew) return NullSubArea;

	/* Initialize the structure */
	snew->numseg   = 0;
	snew->segments = NullSegmentList;
	snew->visready = FALSE;
	snew->nsubvis  = 0;
	snew->subvis   = NullSubVisList;
	snew->numhole  = 0;
	snew->holes    = NullLineList;
	snew->attrib   = NullAttribList;
	snew->subelem  = NULL;
	snew->value    = NULL;
	snew->label    = NULL;
	init_lspec(&snew->lspec);
	init_fspec(&snew->fspec);

	/* Set subarea value */
	define_subarea_value(snew, subelem, value, label);

	/* Return the new subarea */
	SubAreaCount++;
	return snew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ s u b a r e a                                   *
*                                                                      *
*      Destroy space allocated to the given subarea.                   *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Return memory resources for given subarea.
 *
 * @param[in]	subarea	subarea to be destroyed.
 * @return	NullSubArea pointer.
 ***********************************************************************/
SUBAREA	destroy_subarea

	(
	SUBAREA	subarea	/* subarea to be destroyed */
	)

	{
	/* Do nothing if subarea not there */
	if (!subarea) return NullSubArea;

	/* Free the space used by attributes */
	FREEMEM(subarea->subelem);
	FREEMEM(subarea->value);
	FREEMEM(subarea->label);
	destroy_attrib_list(subarea->attrib);

	/* Free the space used by segment and visible area buffers */
	empty_subarea(subarea);

	/* Free the space used by presentation specs */
	free_lspec(&subarea->lspec);
	free_fspec(&subarea->fspec);

	/* Free the structure itself */
	FREEMEM(subarea);
	SubAreaCount--;
	return NullSubArea;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ s u b a r e a                                       *
*                                                                      *
*      Empty the data buffers of the given subarea.                    *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Empty the data buffers of the given subarea.
 *
 * @param[in]	subarea	subarea to be emptied.
 ***********************************************************************/
void	empty_subarea

	(
	SUBAREA	subarea	/* subarea to be emptied */
	)

	{
	int		i;

	/* Do nothing if subarea not there */
	if (!subarea) return;

	/* Free the space used by the visible area buffer */
	if (subarea->nsubvis > 0)
		{
		for (i=0; i<subarea->nsubvis; i++)
			(void) destroy_subvis(subarea->subvis[i]);
		FREEMEM(subarea->subvis);
		subarea->nsubvis = 0;
		}
	subarea->visready = FALSE;

	/* Free the space used by segment buffer */
	if (subarea->numseg > 0)
		{
		for (i=0; i<subarea->numseg; i++)
			(void) destroy_segment(subarea->segments[i]);
		FREEMEM(subarea->segments);
		subarea->numseg = 0;
		}

	/* Free space used by hole list */
	if (subarea->numhole > 0)
		{
		FREEMEM(subarea->holes);
		subarea->numhole = 0;
		}
	}

/***********************************************************************
*                                                                      *
*      c o p y _ s u b a r e a                                         *
*                                                                      *
*      Create an exact copy of a subarea.                              *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Create an exact copy of the subarea.
 *
 * @param[in]	subarea		subarea to be copied.
 * @param[in]	all			do we want to copy the whole structure?
 ***********************************************************************/
SUBAREA	copy_subarea

	(
	const SUBAREA	subarea,	/* subarea to be copied */
	LOGICAL			all			/* do we want to copy the whole structure */
	)

	{
	int		n, i, nsv, isv;
	SUBAREA	snew;
	SUBVIS	svis, ovis;

	/* Do nothing if subarea not there */
	if (!subarea) return NullSubArea;

	/* Create an empty copy */
	snew = create_subarea(NULL, NULL, NULL);
	define_subarea_attribs(snew, subarea->attrib);

	/* Duplicate presentation specs */
	copy_lspec(&snew->lspec, &subarea->lspec);
	copy_fspec(&snew->fspec, &subarea->fspec);

	/* Duplicate the segment buffer (do not clone lines) */
	if ((n = subarea->numseg) > 0)
		{
		snew->numseg   = n;
		snew->segments = GETMEM(snew->segments, SEGMENT, n);

		for (i=0; i<n; i++)
			{
			snew->segments[i] = copy_segment(subarea->segments[i], FALSE);
			}
		}

	/* Duplicate the visible areas if requested */
	if (all && subarea->visready)
		{
		if ((nsv = subarea->nsubvis) > 0)
			{
			snew->nsubvis = nsv;
			snew->subvis  = GETMEM(snew->subvis, SUBVIS, nsv);

			for (isv=0; isv<nsv; isv++)
				{
				ovis = subarea->subvis[isv];
				svis = snew->subvis[isv] = create_subvis();

				if ((n = ovis->numvis) > 0)
					{
					svis->numvis = n;
					svis->segvis = INITMEM(SEGMENT, n);

					for (i=0; i<n; i++)
						{
						svis->segvis[i] = copy_segment(ovis->segvis[i], FALSE);
						}
					}
				}
			}
		if ((n = subarea->numhole) > 0)
			{
			snew->numhole = n;
			snew->holes   = GETMEM(snew->holes, LINE, n);

			for (i=0; i<n; i++)
				{
				snew->holes[i] = subarea->holes[i];
				}
			}
		snew->visready = TRUE;
		}

	return snew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u b a r e a _ v a l u e                         *
*      r e c a l l _ s u b a r e a _ v a l u e                         *
*                                                                      *
*      Set/reset or retrieve the value of the given subarea.           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set or reset the value of the given subarea.
 *
 * @param[in]	subarea	subarea to set.
 * @param[in]	subelem	subelement
 * @param[in]	value	value assigned to the enclosed subarea.
 * @param[in]	label	label assigned to the enclosed subarea.
 ***********************************************************************/
void	define_subarea_value

	(
	SUBAREA	subarea,	/* given subarea */
	STRING	subelem,	/* subelement */
	STRING	value,		/* value assigned to the enclosed subarea */
	STRING	label		/* label assigned to the enclosed subarea */
	)

	{
	/* Do nothing if subarea not there */
	if (!subarea) return;

	/* Set given attributes */
	subarea->subelem = STRMEM(subarea->subelem,subelem);
	subarea->value   = STRMEM(subarea->value  ,value);
	subarea->label   = STRMEM(subarea->label  ,label);

	/* Replicate in attributes */
	if (IsNull(subarea->attrib)) subarea->attrib = create_attrib_list();
	add_default_attributes(subarea->attrib, subelem, value, label);
	}

/**********************************************************************/

/***********************************************************************/
/**	Lookup the value of the given subarea.
 *
 * @param[in]	subarea	subarea to search.
 * @param[out]	subelem	subelement.
 * @param[out]	value	value assigned to the enclosed subarea.
 * @param[out]	label	label assigned to the enclosed subarea.
 ***********************************************************************/
void	recall_subarea_value

	(
	SUBAREA	subarea,	/* requested subarea */
	STRING	*subelem,	/* subelement */
	STRING	*value,		/* value assigned to the enclosed subarea */
	STRING	*label		/* label assigned to the enclosed subarea */
	)

	{
	/* Initialize return parameters */
	if (subelem) *subelem = NULL;
	if (value)   *value   = NULL;
	if (label)   *label   = NULL;
	if (!subarea) return;

	/* Retrieve the attributes */
	get_default_attributes(subarea->attrib, subelem, value, label);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u b a r e a _ a t t r i b s                     *
*      r e c a l l _ s u b a r e a _ a t t r i b s                     *
*                                                                      *
*      Set/reset or retrieve the attributes of the given subarea.      *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set or reset the attributes of the given subarea.
 *
 * @param[in]	subarea	subarea to set.
 * @param[in]	attribs	list of attributes to assign to subarea.
 ***********************************************************************/
void	define_subarea_attribs

	(
	SUBAREA		subarea,	/* given subarea */
	ATTRIB_LIST	attribs		/* attributes */
	)

	{
	STRING	subelem, value, label;

	/* Do nothing if subarea not there */
	if (!subarea) return;

	/* Set given attributes */
	subarea->attrib = destroy_attrib_list(subarea->attrib);
	if (NotNull(attribs)) subarea->attrib = copy_attrib_list(attribs);

	/* >>> define sub, val, lab <<< */
	get_default_attributes(subarea->attrib, &subelem, &value, &label);
	subarea->subelem = STRMEM(subarea->subelem,subelem);
	subarea->value   = STRMEM(subarea->value  ,value);
	subarea->label   = STRMEM(subarea->label  ,label);
	}

/**********************************************************************/

/***********************************************************************/
/**	Lookup the attributes of the given subarea.
 *
 * @param[in]	subarea		subarea to lookup.
 * @param[out]	*attribs	attributes of subarea.
 ***********************************************************************/
void	recall_subarea_attribs

	(
	SUBAREA		subarea,	/* requested subarea */
	ATTRIB_LIST	*attribs	/* attributes */
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (subarea) ? subarea->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u b a r e a _ p s p e c                         *
*      r e c a l l _ s u b a r e a _ p s p e c                         *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given       *
*      subarea.                                                        *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set or reset the presentation specs of the given subarea.
 *
 * @param[in]	subarea		subarea to set presentation for.
 * @param[in]	param		parameter to set.
 * @param[in]	value		value to give parameter.
 ***********************************************************************/
void	define_subarea_pspec

	(
	SUBAREA	subarea,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if subarea does not exist */
	if (!subarea) return;

	/* Set the given parameter */
	define_lspec_value(&subarea->lspec,param,value);
	define_fspec_value(&subarea->fspec,param,value);
	}

/**********************************************************************/

/***********************************************************************/
/** Lookup the presentation specs of the given subarea.
 *
 * @param[in]	subarea	subarea to search.
 * @param[in]	param	parameter to lookup.
 * @param[in]	value	value of parameter.
 ***********************************************************************/
void	recall_subarea_pspec

	(
	SUBAREA	subarea,
	PPARAM	param,
	POINTER	value
	)

	{
	/* Do nothing if subarea does not exist */
	if (!subarea) return;

	/* Return the requested parameter */
	recall_lspec_value(&subarea->lspec,param,value);
	recall_fspec_value(&subarea->fspec,param,value);
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ s u b a r e a                               *
*      w i d e n _ s u b a r e a                                       *
*                                                                      *
*      Set the highlight flag for the given subarea.                   *
*      Note: a negative value means to erase.                          *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set the highlight flag for the given subarea.
 *
 * @note a negative value means to erase.
 *
 * @param[in]	sub		subarea to set.
 * @param[in]	pcode	perimeter highlight code.
 * @param[in]	fcode	fill highlight code.
 ***********************************************************************/
void	highlight_subarea

	(
	SUBAREA	sub,
	HILITE	pcode,
	HILITE	fcode
	)

	{
	/* Do nothing if no subarea */
	if (!sub) return;

	if (pcode != SkipHilite)
		define_lspec_value(&sub->lspec, LINE_HILITE, (POINTER)&pcode);
	if (fcode != SkipHilite)
		define_fspec_value(&sub->fspec, FILL_HILITE, (POINTER)&fcode);
	}

/**********************************************************************/

/***********************************************************************/
/**	Winden boundary line around a subarea.
 *
 * Boundary line is increased by the value of delta.
 *
 * @param[in]	sub		subarea to set.
 * @param[in]	delta	change is line width.
 ***********************************************************************/
void	widen_subarea

	(
	SUBAREA	sub,
	float	delta
	)

	{
	float	width;

	/* Do nothing if no subarea */
	if (!sub) return;

	/* Widen simple outlines by delta - patterned outlines by 1.25 */
	recall_lspec_value(&sub->lspec, LINE_WIDTH, (POINTER)&width);
	if (!sub->lspec.pattern) width += delta;
	else if (delta > 0.0)    width *= 1.25;
	else if (delta < 0.0)    width /= 1.25;
	define_lspec_value(&sub->lspec, LINE_WIDTH, (POINTER)&width);
	}

/***********************************************************************
*                                                                      *
*      Routines specific to the SUBVIS object                          *
*                                                                      *
***********************************************************************/

int		SubVisCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ s u b v i s                                       *
*                                                                      *
*      Create a new visible segment list.                              *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Create a new visible segment list.
 *
 * @return	pointer to a new visible segment list. (You should destroy it
 * when you are finished with it).
 ***********************************************************************/
SUBVIS	create_subvis

	(
	)

	{
	SUBVIS	vnew;

	/* Allocate space for the structure */
	vnew = INITMEM(struct SUBVIS_struct, 1);
	if (!vnew) return NullSubVis;

	/* Initialize the structure */
	vnew->numvis = 0;
	vnew->segvis = NullSegmentList;

	/* Return the new visible segment list */
	SubVisCount++;
	return vnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ s u b v i s                                     *
*                                                                      *
*      Destroy space allocated to the given visible segment list.      *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Return memory resources for given visible segment list.
 *
 * @param[in]	subvis	visible segment list to be destroyed.
 * @return	NullSubVis pointer.
 ***********************************************************************/
SUBVIS	destroy_subvis

	(
	SUBVIS	subvis	/* visible segment list to be destroyed */
	)

	{
	int		i;

	/* Do nothing if visible segment list not there */
	if (!subvis) return NullSubVis;

	/* Free the space used by visible segments */
	if (subvis->numvis > 0)
		{
		for (i=0; i<subvis->numvis; i++)
			(void) destroy_segment(subvis->segvis[i]);
		FREEMEM(subvis->segvis);
		subvis->numvis = 0;
		}

	/* Free the structure itself */
	FREEMEM(subvis);
	SubVisCount--;
	return NullSubVis;
	}
