/*********************************************************************/
/**	@file plot.c
 *
 * Routines to handle the PSET and PLOT objects.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      p l o t . c                                                     *
*                                                                      *
*      Routines to handle the PSET and PLOT objects.                   *
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

#define PLOT_INIT
#include "plot.h"
#include "projection.h"

#include <fpa_getmem.h>
#include <fpa_math.h>
#include <tools/tools.h>
#include <string.h>

int		PlotCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ p l o t                                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new plot with default attributes.
 *
 * @return Pointer to new plot object. You will need to destroy this
 * object when you are finished with it.
 *********************************************************************/

PLOT	create_plot(void)

	{
	PLOT	plot;

	/* Allocate memory for structure */
	plot = INITMEM(struct PLOT_struct,1);
	if (!plot) return NullPlot;

	/* Initialize the structure */
	plot->subs   = NullPsubList;
	plot->nsubs  = 0;
	plot->pts    = NullPointList;
	plot->numpts = 0;
	plot->maxpts = 0;
	plot->cspecs = (PLTSPEC *) 0;
	plot->ncspec = 0;

	/* Return the new plot */
	PlotCount++;
	return plot;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ p l o t                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy a plot.
 *
 *	@param[in] 	plot	plot to be destroyed
 *  @return NullPlot
 *********************************************************************/

PLOT	destroy_plot

	(
	PLOT	plot
	)

	{
	int		isub;
	PSUB	*sub;

	/* Do nothing if plot not there */
	if (!plot) return NullPlot;

	/* Free the space for point buffer */
	empty_plot(plot);
	FREEMEM(plot->pts);
	plot->numpts = 0;
	plot->maxpts = 0;

	/* Free the space for sub-field buffer */
	if (plot->subs)
		{
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			sub->proto = destroy_item(sub->type,sub->proto);
			FREEMEM(sub->name);
			FREEMEM(sub->type);
			FREEMEM(sub->sval1);
			FREEMEM(sub->sval2);
			FREEMEM(sub->ival1);
			FREEMEM(sub->ival2);
			FREEMEM(sub->fval1);
			FREEMEM(sub->fval2);
			}
		FREEMEM(plot->subs);
		}
	plot->nsubs = 0;

	/* Free the space for plot subfield specs */
	define_plot_pltspecs(plot,0,(PLTSPEC *)0);

	/* Free the structure itself */
	FREEMEM(plot);
	PlotCount--;
	return NullPlot;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ p l o t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Zero the list in the given plot without freeing too much
 * allocated space.
 *
 *	@param[in] 	plot	plot to empty
 *********************************************************************/

void	empty_plot

	(
	PLOT	plot
	)

	{
	int		isub, ip;
	PSUB	*sub;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Empty sub-field buffer */
	if (plot->subs)
		{
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			if (sub->sval1)
				{
				for (ip=0; ip<plot->numpts; ip++) FREEMEM(sub->sval1[ip]);
				}
			if (sub->sval2)
				{
				for (ip=0; ip<plot->numpts; ip++) FREEMEM(sub->sval2[ip]);
				}
			}
		}

	/* Empty point buffer */
	plot->numpts = 0;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ p l o t                                               *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Copy one plot to another.
 *
 *	@param[in] 	plot	plot to be copied
 *  @return Pointer to copy of given plot. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/

PLOT	copy_plot

	(
	const PLOT	plot
	)

	{
	PLOT	p;
	int		ipt, isub;
	PSUB	*sub;
	STRING	c1, c2;
	int		i1, i2;
	float	f1, f2;
	ITEM	proto;

	/* Do nothing if plot not there */
	if (!plot) return NullPlot;

	/* Create a new plot */
	p = create_plot();

	/* Duplicate plot subfield specs */
	define_plot_pltspecs(p,(int) plot->ncspec,plot->cspecs);

	/* Duplicate the sub-fields */
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub   = plot->subs + isub;
		proto = copy_item(sub->type,sub->proto);
		add_subfld_to_plot(p,sub->name,sub->type,proto);
		}

	/* Duplicate the point list */
	for (ipt=0; ipt<plot->numpts; ipt++)
		{
		add_point_to_plot(p,plot->pts[ipt]);
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			c1  = (sub->sval1) ? sub->sval1[ipt] : NULL;
			c2  = (sub->sval2) ? sub->sval2[ipt] : NULL;
			i1  = (sub->ival1) ? sub->ival1[ipt] : 0;
			i2  = (sub->ival2) ? sub->ival2[ipt] : 0;
			f1  = (sub->fval1) ? sub->fval1[ipt] : 0.0;
			f2  = (sub->fval2) ? sub->fval2[ipt] : 0.0;
			define_subfld_value(p,sub->name,ipt,c1,c2,i1,i2,f1,f2);
			}
		}

	return p;
	}

/***********************************************************************
*                                                                      *
*      a d d _ s u b f l d _ t o _ p l o t                             *
*      r e m o v e _ s u b f l d _ f r o m _ p l o t  (gone?)          *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Add the given sub-field to the given plot.
 *
 *	@param[in] 	plot		plot to add sub-field to
 *	@param[in] 	subname		sub-field name
 *	@param[in] 	type		sub-field type
 *	@param[in] 	proto		prototype sub-field member (actual object)
 *********************************************************************/
void	add_subfld_to_plot

	(
	PLOT	plot,
	STRING	subname,
	STRING	type,
	ITEM	proto
	)

	{
	int		isub, ipt;
	PSUB	*sub;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Allocate room for a new sub-field */
	isub = plot->nsubs++;
	plot->subs = GETMEM(plot->subs,PSUB,plot->nsubs);
	sub = plot->subs + isub;

	/* Define main attributes */
	sub->name  = INITSTR(subname);
	sub->type  = INITSTR(type);
	sub->proto = proto;

	/* Set up value arrays for appropriate item type */
	sub->sval1 = (STRING *) 0;
	sub->sval2 = (STRING *) 0;
	sub->ival1 = (int *) 0;
	sub->ival2 = (int *) 0;
	sub->fval1 = (float *) 0;
	sub->fval2 = (float *) 0;
	if (same(type,"area"))
		{
		if (plot->maxpts > 0) sub->sval1 = INITMEM(STRING,plot->maxpts);
		}
	else if (same(type,"curve"))
		{
		if (plot->maxpts > 0) sub->sval1 = INITMEM(STRING,plot->maxpts);
		}
	else if (same(type,"label"))
		{
		if (plot->maxpts > 0) sub->sval1 = INITMEM(STRING,plot->maxpts);
		if (plot->maxpts > 0) sub->sval2 = INITMEM(STRING,plot->maxpts);
		}
	else if (same(type,"mark"))
		{
		if (plot->maxpts > 0) sub->sval1 = INITMEM(STRING,plot->maxpts);
		}
	else if (same(type,"barb"))
		{
		if (plot->maxpts > 0) sub->fval1 = INITMEM(float,plot->maxpts); /*dir*/
		if (plot->maxpts > 0) sub->fval2 = INITMEM(float,plot->maxpts); /*spd*/
		}
	else if (same(type,"button"))
		{
		if (plot->maxpts > 0) sub->sval1 = INITMEM(STRING,plot->maxpts);
		if (plot->maxpts > 0) sub->sval2 = INITMEM(STRING,plot->maxpts);
		}
	else if (same(type,"int"))
		{
		if (plot->maxpts > 0) sub->ival1 = INITMEM(int,plot->maxpts);
		}
	else if (same(type,"float"))
		{
		if (plot->maxpts > 0) sub->fval1 = INITMEM(float,plot->maxpts);
		}

	/* Check against plot subfield specs */
	if (IsNull(proto))
		{
		sub->proto = create_bgnd_item(type,subname,"","");
		invoke_item_pltspec(type,sub->proto,plot->ncspec,plot->cspecs);
		}

	/* Initialize value buffers */
	for (ipt=0; ipt<plot->numpts; ipt++)
		{
		if (sub->sval1) sub->sval1[ipt] = NULL;
		if (sub->sval2) sub->sval2[ipt] = NULL;
		take_subfld_value(plot,sub->name,ipt,sub->proto);
		}
	}

/***********************************************************************
*                                                                      *
*      a d d _ p o i n t _ t o _ p l o t                               *
*      r e m o v e _ p o i n t _ f r o m _ p l o t                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Add the given point to the given plot.
 *
 *	@param[in] 	plot	plot to add point to
 *	@param[in] 	point	point to add to plot
 *********************************************************************/

void	add_point_to_plot

	(
	PLOT	plot,
	POINT	point
	)

	{
	int		isub, ipt;
	PSUB	*sub;

	/* Do nothing if plot or point not there */
	if (!plot) return;
	if (!point) return;

	/* See if we need more space */
	ipt = plot->numpts++;
	if (plot->numpts > plot->maxpts)
		{
		plot->maxpts += DELTA_PLOT;
		plot->pts     = GETMEM(plot->pts,POINT,plot->maxpts);
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			if (same(sub->type,"area"))
				{
				sub->sval1 = GETMEM(sub->sval1,STRING,plot->maxpts);
				}
			else if (same(sub->type,"curve"))
				{
				sub->sval1 = GETMEM(sub->sval1,STRING,plot->maxpts);
				}
			else if (same(sub->type,"label"))
				{
				sub->sval1 = GETMEM(sub->sval1,STRING,plot->maxpts);
				sub->sval2 = GETMEM(sub->sval2,STRING,plot->maxpts);
				}
			else if (same(sub->type,"mark"))
				{
				sub->sval1 = GETMEM(sub->sval1,STRING,plot->maxpts);
				}
			else if (same(sub->type,"barb"))
				{
				sub->fval1 = GETMEM(sub->fval1,float,plot->maxpts); /*dir*/
				sub->fval2 = GETMEM(sub->fval2,float,plot->maxpts); /*spd*/
				}
			else if (same(sub->type,"button"))
				{
				sub->sval1 = GETMEM(sub->sval1,STRING,plot->maxpts);
				sub->sval2 = GETMEM(sub->sval2,STRING,plot->maxpts);
				}
			else if (same(sub->type,"int"))
				{
				sub->ival1 = GETMEM(sub->ival1,int,plot->maxpts);
				}
			else if (same(sub->type,"float"))
				{
				sub->fval1 = GETMEM(sub->fval1,float,plot->maxpts);
				}
			}
		}

	/* Copy the given point to the point list */
	copy_point(plot->pts[ipt],point);

	/* Now copy the attributes of the prototype */
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub = plot->subs + isub;
		if (sub->sval1) sub->sval1[ipt] = NULL;
		if (sub->sval2) sub->sval2[ipt] = NULL;
		take_subfld_value(plot,sub->name,ipt,sub->proto);
		}
	}

/*********************************************************************/
/** Remove the given point from the given plot.
 *
 *	@param[in] 	plot	plot to remove point from
 *	@param[in] 	point	point to remove from plot
 *********************************************************************/
void	remove_point_from_plot

	(
	PLOT	plot,
	POINT	point
	)

	{
	int		isub, ipt, irem;
	PSUB	*sub;

	/* Do nothing if plot or point not there */
	if (!plot) return;
	if (!point) return;

	/* Determine whether the given point is present */
	irem = which_plot_point(plot,point);
	if (irem < 0) return;

	/* Compress the point list */
	plot->numpts--;
	for (ipt=irem; ipt<plot->numpts; ipt++)
		{
		copy_point(plot->pts[ipt],plot->pts[ipt+1]);
		}

	/* Now compress (and free) the attributes in each subfield */
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub = plot->subs + isub;
		if (sub->sval1) FREEMEM(sub->sval1[irem]);
		if (sub->sval2) FREEMEM(sub->sval2[irem]);
		for (ipt=irem; ipt<plot->numpts; ipt++)
			{
			if (sub->sval1) sub->sval1[ipt] = sub->sval1[ipt+1];
			if (sub->sval2) sub->sval2[ipt] = sub->sval2[ipt+1];
			if (sub->ival1) sub->ival1[ipt] = sub->ival1[ipt+1];
			if (sub->ival2) sub->ival2[ipt] = sub->ival2[ipt+1];
			if (sub->fval1) sub->fval1[ipt] = sub->fval1[ipt+1];
			if (sub->fval2) sub->fval2[ipt] = sub->fval2[ipt+1];
			}
		if (sub->sval1) sub->sval1[ipt] = NULL;
		if (sub->sval2) sub->sval2[ipt] = NULL;
		if (sub->ival1) sub->ival1[ipt] = 0;
		if (sub->ival2) sub->ival2[ipt] = 0;
		if (sub->fval1) sub->fval1[ipt] = 0.0;
		if (sub->fval2) sub->fval2[ipt] = 0.0;
		}
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ s u b f l d _ v a l u e                           *
*      t a k e _ s u b f l d _ v a l u e                               *
*                                                                      *
***********************************************************************/


/*ARGSUSED*/
/*********************************************************************/
/** Define the associated values of the given sub-field member.
 *
 *	@param[in] 	plot		given plot
 *	@param[in] 	subname		subfield name to define value of
 *	@param[in] 	member		index of the memeber to define
 *	@param[in] 	c1			String var 1
 *	@param[in] 	c2			String var 2
 *	@param[in] 	i1			Int var 1
 *	@param[in] 	i2			Int var 2
 *	@param[in] 	f1			Float var 1
 *	@param[in] 	f2			Float var 2
 *********************************************************************/
void	define_subfld_value

	(
	PLOT	plot,
	STRING	subname,
	int		member,
	STRING	c1,
	STRING	c2,
	int		i1,
	int		i2,
	float	f1,
	float	f2
	)

	{
	int		isub;
	PSUB	*sub;

	/* Return if nothing to change */
	if (!plot)                  return;
	if (member < 0)             return;
	if (member >= plot->numpts) return;

	/* Find the given sub-field */
	isub = which_plot_subfld(plot,subname);
	if (isub < 0) return;
	sub = plot->subs + isub;

	/* Set up values for appropriate item type */
	if (same(sub->type,"area"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		}
	else if (same(sub->type,"curve"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		}
	else if (same(sub->type,"label"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		if (sub->sval2) sub->sval2[member] = STRMEM(sub->sval2[member],c2);
		}
	else if (same(sub->type,"mark"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		}
	else if (same(sub->type,"barb"))
		{
		if (sub->fval1) sub->fval1[member] = f1; /*dir*/
		if (sub->fval2) sub->fval2[member] = f2; /*spd*/
		}
	else if (same(sub->type,"button"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		if (sub->sval2) sub->sval2[member] = STRMEM(sub->sval2[member],c2);
		}
	else if (same(sub->type,"int"))
		{
		if (sub->ival1) sub->ival1[member] = i1;
		}
	else if (same(sub->type,"float"))
		{
		if (sub->fval1) sub->fval1[member] = f1;
		}
	}

/*********************************************************************/
/** Take the associated values of the given sub-field member
 * from the given prototype item.
 *
 *	@param[in] 	plot		plot to search
 *	@param[in] 	subname		subfield to look at
 *	@param[in] 	member		index to member to query
 *	@param[in] 	proto		prototype sub-field member (actual object)
 *********************************************************************/
void	take_subfld_value

	(
	PLOT	plot,
	STRING	subname,
	int		member,
	ITEM	proto
	)

	{
	int		isub;
	PSUB	*sub;
	STRING	c1, c2;

	/* Return if nothing to change */
	if (!plot)                  return;
	if (member < 0)             return;
	if (member >= plot->numpts) return;
	if (!proto)                 return;

	/* Find the given sub-field */
	isub = which_plot_subfld(plot,subname);
	if (isub < 0) return;
	sub = plot->subs + isub;

	/* Set up values for appropriate item type */
	c1 = item_attribute(sub->type, proto, AttribAutolabel);
	c2 = item_attribute(sub->type, proto, AttribUserlabel);
	if (same(sub->type,"area"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		}
	else if (same(sub->type,"curve"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		}
	else if (same(sub->type,"label"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		if (sub->sval2) sub->sval2[member] = STRMEM(sub->sval2[member],c2);
		}
	else if (same(sub->type,"mark"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		}
	else if (same(sub->type,"barb"))
		{
		if (sub->fval1) sub->fval1[member] = ((BARB) proto)->dir;   /*dir*/
		if (sub->fval2) sub->fval2[member] = ((BARB) proto)->speed; /*spd*/
		}
	else if (same(sub->type,"button"))
		{
		if (sub->sval1) sub->sval1[member] = STRMEM(sub->sval1[member],c1);
		if (sub->sval2) sub->sval2[member] = STRMEM(sub->sval2[member],c2);
		}
	/* Note: int and float members do not have prototypes */
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ p l o t _ p l t s p e c s                         *
*      a d d _ p l t s p e c _ t o _ p l o t                           *
*      r e c a l l _ p l o t _ p l t s p e c s                         *
*      f i n d _ p l o t _ p l t s p e c                               *
*      i n v o k e _ p l o t _ p l t s p e c s                         *
*                                                                      *
*      Define the plot subfield specs for the given plot.              *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/**	Defined the plot specifications for a specific member of the plot.
 *
 * Replaces old plot spec list with new one.
 *
 *	@param[in] 	plot		given Plot
 *	@param[in] 	ncspec		size of cspecs
 *	@param[in] 	*cspecs		list of new specifications
 *********************************************************************/
void	define_plot_pltspecs

	(
	PLOT	plot,
	int		ncspec,
	PLTSPEC	*cspecs
	)

	{
	int		ic;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Get rid of existing cspec list */
	if (plot->cspecs)
		{
		for (ic=0; ic<plot->ncspec; ic++)
			{
			free_pltspec(plot->cspecs + ic);
			}
		FREEMEM(plot->cspecs);
		}
	plot->ncspec = 0;

	/* Add the new cspec list one at a time */
	if (!cspecs) return;
	for (ic=0; ic<ncspec; ic++)
		{
		add_pltspec_to_plot(plot,cspecs+ic);
		}
	}

/*********************************************************************/
/** Add a plot specification to the plot.
 *
 *	@param[in] 	plot		plot to add to
 *	@param[in] 	*cspec		new spec to add
 *********************************************************************/
void	add_pltspec_to_plot

	(
	PLOT	plot,
	PLTSPEC	*cspec
	)

	{
	PLTSPEC	*csnew;

	/* Return if nothing to change */
	if (!plot)   return;
	if (!cspec) return;

	/* Expand the cspec list */
	plot->ncspec++;
	plot->cspecs = GETMEM(plot->cspecs,PLTSPEC,plot->ncspec);
	csnew = plot->cspecs + (plot->ncspec-1);
	init_pltspec(csnew);

	/* Copy the given cspec into the new one */
	copy_pltspec(csnew,cspec);
	}

/*********************************************************************/
/** Retrieve plot specifications.
 *
 *	@param[in] 	plot		plot to query
 *	@param[out]	*ncspec		return size of cspecs
 *	@param[out]	**cspecs	list of plot specifications
 *********************************************************************/
void	recall_plot_pltspecs

	(
	PLOT	plot,
	int		*ncspec,
	PLTSPEC	**cspecs
	)

	{
	/* Retrieve all the desired info */
	if (cspecs) *cspecs = (plot) ? plot->cspecs : (PLTSPEC *) 0;
	if (ncspec) *ncspec = (plot) ? plot->ncspec : 0;
	}

/*********************************************************************/
/** Retrieve a specific plot specification.
 *
 *	@param[in] 	plot	Plot to search
 *	@param[in] 	subname	subfield to retrieve
 *  @return Point to pltspec object.
 *********************************************************************/
PLTSPEC	*find_plot_pltspec

	(
	PLOT	plot,
	STRING	subname
	)

	{
	PLTSPEC	*cspec;
	int		isp;

	/* Do nothing if plot not there */
	if (!plot)          return (PLTSPEC *) 0;
	if (blank(subname)) return (PLTSPEC *) 0;

	/* Find which plot spec corresponds to the subfield name */
	for (isp=0; isp<plot->ncspec; isp++)
		{
		cspec = plot->cspecs + isp;
		if (same(subname,cspec->name)) return cspec;
		}

	/* Not found */
	return (PLTSPEC *) 0;
	}

/*********************************************************************/
/** Invoke plot specifications for a given plot.
 *
 *	@param[in] 	plot	given plot
 *********************************************************************/
void	invoke_plot_pltspecs

	(
	PLOT	plot
	)

	{
	int		isub;
	PSUB	*sub;

	/* Do nothing if plot not there */
	if (!plot) return;

	if (plot->subs)
		{
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			invoke_item_pltspec(sub->type,sub->proto,plot->ncspec,plot->cspecs);
			}
		}
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ p l o t                                     *
*      h i g h l i g h t _ s u b f l d                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reset the highlight code of a given plot.
 *
 *	@param[in] 	plot	given plot
 *	@param[in] 	code	hilite code
 *********************************************************************/

void	highlight_plot

	(
	PLOT	plot,
	HILITE	code
	)

	{
	int		i, isub, fcode;
	PSUB	*sub;
	PLTSPEC	*cspec;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Set plot subfield spec highlight code */
	fcode = MIN(code,0);
	for (i=0; i<plot->ncspec; i++)
		{
		cspec = plot->cspecs + i;
		define_lspec_value(&cspec->lspec,LINE_HILITE,(POINTER) &code);
		define_fspec_value(&cspec->fspec,FILL_HILITE,(POINTER) &fcode);
		define_tspec_value(&cspec->tspec,TEXT_HILITE,(POINTER) &code);
		define_mspec_value(&cspec->mspec,MARK_HILITE,(POINTER) &code);
		define_bspec_value(&cspec->bspec,BARB_HILITE,(POINTER) &code);
		}

	if (plot->subs)
		{
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			highlight_item(sub->type,sub->proto,code);
			}
		}
	}

/*********************************************************************/
/** Reset the highlight code of a given sub-field.
 *
 *	@param[in] 	plot		given plot
 *	@param[in] 	subname		given subfield
 *	@param[in]  	code		hilite code
 *********************************************************************/
void	highlight_subfld

	(
	PLOT	plot,
	STRING	subname,
	HILITE	code
	)

	{
	int		isub;
	PSUB	*sub;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Find the given sub-field */
	isub = which_plot_subfld(plot,subname);
	if (isub < 0) return;
	sub = plot->subs + isub;

	/* Change highlight code of the prototype item */
	highlight_item(sub->type,sub->proto,code);
	}

/***********************************************************************
*                                                                      *
*      c h a n g e _ s u b f l d _ p s p e c                           *
*      r e c a l l _ s u b f l d _ p s p e c                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Override the presentation specs of all items in the given plot
 * subfield.
 *
 *	@param[in] 	plot		given plot
 *	@param[in] 	subname		subfield name
 *	@param[in] 	param		parameter to override
 *	@param[in] 	value		value to override with
 *********************************************************************/

void	change_subfld_pspec

	(
	PLOT	plot,
	STRING	subname,
	PPARAM	param,
	POINTER	value
	)

	{
	int		isub;
	PSUB	*sub;
	ITEM	item;
	STRING	type;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Find the given sub-field */
	isub = which_plot_subfld(plot,subname);
	if (isub < 0) return;
	sub  = plot->subs + isub;
	item = sub->proto;
	type = sub->type;

	change_item_pspec(type,item,param,value);
	}

/*********************************************************************/
/** Retrieve the presentation specs of all items in the given plot
 * subfield.
 *
 *	@param[in] 	plot		given plot
 *	@param[in] 	subname 	subfield name
 *	@param[in] 	param 		parameter to retrieve
 *	@param[out]	value		value retrieved
 *********************************************************************/
void	recall_subfld_pspec

	(
	PLOT	plot,
	STRING	subname,
	PPARAM	param,
	POINTER	value
	)

	{
	int		isub;
	PSUB	*sub;
	ITEM	item;
	STRING	type;

	/* Do nothing if plot not there */
	if (!plot) return;

	/* Find the given sub-field */
	isub = which_plot_subfld(plot,subname);
	if (isub < 0) return;
	sub  = plot->subs + isub;
	item = sub->proto;
	type = sub->type;

	recall_item_pspec(type,item,param,value);
	}

/***********************************************************************
*                                                                      *
*      w h i c h _ p l o t _ s u b f l d                               *
*      w h i c h _ p l o t _ p o i n t                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the named sub-field of the given plot.
 *
	@param[in] 	plot	given plot
	@param[in] 	subname subfield name to find
 *  @return The index of the subfield in the plot object. <0 if not
 * 			found.
 *********************************************************************/

int		which_plot_subfld

	(
	PLOT	plot,
	STRING	subname
	)

	{
	int		isub;
	PSUB	*sub;

	/* Do nothing if plot not there */
	if (!plot) return -1;

	/* Find the given sub-field */
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub = plot->subs + isub;
		if (same(subname,sub->name)) return isub;
		}

	return -1;
	}

/*********************************************************************/
/** Find the given point in the given plot if present.
 *
 *	@param[in] 	plot	given plot
 *	@param[in] 	point	point to look for
 *  @return The index of the point in the plot object. <0 if not
 * 			found.
 *********************************************************************/
int		which_plot_point

	(
	PLOT	plot,
	POINT	point
	)

	{
	int		ipt;

	/* Do nothing if plot or point not there */
	if (!plot) return -1;
	if (!point) return -1;

	/* Search through list of points */
	for (ipt=0; ipt<plot->numpts; ipt++)
		{
		if (plot->pts[ipt][X] != point[X]) continue;
		if (plot->pts[ipt][Y] != point[Y]) continue;
		return ipt;
		}

	/* Not found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*      c l o s e s t _ p l o t _ p o i n t                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find closest member in the given plot to the given point.
 *
 *	@param[in] 	plot	plot to be examined
 *	@param[in] 	point	reference point
 *	@param[out]	*pdist	distance to closest member
 *	@param[out]	ppoint	actual member location
 * 	@return The index of the closest point in the plot to the
 * 			given point. <0 if not found.
 *********************************************************************/

int		closest_plot_point

	(
	PLOT	plot,
	POINT	point,
	float	*pdist,
	POINT	ppoint
	)

	{
	int		i;
	float	mdist, dist;
	int		member;
	POINT	mpoint;
	int		found = FALSE;

	copy_point(mpoint, ZeroPoint);
	mdist  = -1;
	member = -1;

	/* Set to reasonable default values */
	if (pdist) *pdist = mdist;
	if (ppoint) copy_point(ppoint, mpoint);

	/* Do nothing if plot or point not there */
	if (!plot)  return member;
	if (!point) return member;

	/* Examine all points in the plot */
	for (i=0; i<plot->numpts; i++)
		{
		dist = hypot(point[X]-plot->pts[i][X],point[Y]-plot->pts[i][Y]);
		if ((!found) || ((dist >= 0) && (dist < mdist)))
			{
			found  = TRUE;
			member = i;
			mdist  = dist;
			copy_point(mpoint, plot->pts[i]);
			}
		}

	/* Return the closest member */
	if (pdist) *pdist = mdist;
	copy_point(ppoint, mpoint);
	return member;
	}

/***********************************************************************
*                                                                      *
*      s t r i p _ p l o t                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Remove the points from the given plot that are outside the given box.
 *
 *	@param[in] 	plot	given plot
 *	@param[in] 	*box	bounding box
 *********************************************************************/

void	strip_plot

	(
	PLOT		plot,
	const BOX	*box
	)

	{
	int		i, j, isub;
	PSUB	*sub;

	/* Return if nothing to change */
	if (!plot) return;
	if (!box)  return;

	for (i=0, j=0; i<plot->numpts; i++)
		{
		/* Remove point if outside */
		if (! inside_box(box, plot->pts[i]) )
			{
			for (isub=0; isub<plot->nsubs; isub++)
				{
				sub = plot->subs + isub;
				if (sub->sval1) FREEMEM(sub->sval1[i]);
				if (sub->sval2) FREEMEM(sub->sval2[i]);
				if (sub->ival1) sub->ival1[i] = 0;
				if (sub->ival2) sub->ival2[i] = 0;
				if (sub->fval1) sub->fval1[i] = 0.0;
				if (sub->fval2) sub->fval2[i] = 0.0;
				}
			copy_point(plot->pts[i], ZeroPoint);
			}

		/* Otherwise move it up if necessary */
		else
			{
			if (j < i)
				{
				for (isub=0; isub<plot->nsubs; isub++)
					{
					sub = plot->subs + isub;
					if (sub->sval1) sub->sval1[j] = sub->sval1[i];
					if (sub->sval2) sub->sval2[j] = sub->sval2[i];
					if (sub->ival1) sub->ival1[j] = sub->ival1[i];
					if (sub->ival2) sub->ival2[j] = sub->ival2[i];
					if (sub->fval1) sub->fval1[j] = sub->fval1[i];
					if (sub->fval2) sub->fval2[j] = sub->fval2[i];
					if (sub->sval1) sub->sval1[i] = NULL;
					if (sub->sval2) sub->sval2[i] = NULL;
					if (sub->ival1) sub->ival1[i] = 0;
					if (sub->ival2) sub->ival2[i] = 0;
					if (sub->fval1) sub->fval1[i] = 0.0;
					if (sub->fval2) sub->fval2[i] = 0.0;
					}
				copy_point(plot->pts[j], plot->pts[i]);
				copy_point(plot->pts[i], ZeroPoint);
				}
			j++;
			}
		}

	/* Reset number of points */
	plot->numpts = j;
	}

/***********************************************************************
*                                                                      *
*      r e p r o j e c t _ p l o t                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reproject the given plot from the first map projection to the second.
 *
 *	@param[in] 	plot	give plot
 *	@param[in]  *smproj	original projection
 *	@param[in]  *tmproj	new projection
 *  @return True if successful.
 *********************************************************************/

LOGICAL		reproject_plot

	(
	PLOT			plot,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	LOGICAL	reproj, remap, rescale;
	int		isub, ipt;
	float	sunits, tunits, sfact;
	POINT	tpos;
	PSUB	*sub;

	/* Do nothing if plot and starting map projection not there */
	if (!plot) return FALSE;
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

	/* Rescale all subfield prototypes */
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub = plot->subs + isub;
		(void) scale_item(sub->type, sub->proto, sfact, sfact);
		}

	/* Remap all points in the plot */
	for (ipt=0; ipt<plot->numpts; ipt++)
		{
		if (rescale)
			{
			plot->pts[ipt][X] *= sfact;
			plot->pts[ipt][Y] *= sfact;
			}
		else
			{
			(void) pos_to_pos(smproj, plot->pts[ipt], tmproj, tpos);
			(void) copy_point(plot->pts[ipt], tpos);
			}
		}

	return TRUE;
	}

#ifdef DEBUG_PSPEC
/***********************************************************************
*                                                                      *
*      d e b u g _ p l o t                                             *
*                                                                      *
***********************************************************************/

LOGICAL	debug_plot

	(
	PLOT	plot,
	STRING	msg
	)

	{
	int		isub, ip;
	PSUB	*sub;

	(void) printf("%s:\n",msg);
	if (!plot) return FALSE;

	(void) printf("   subfields:\n");
	for (isub=0; isub<plot->nsubs; isub++)
		{
		sub = plot->subs + isub;
		(void) printf("      %s %s\n",sub->name,sub->type);
		if (same(sub->type,"area"))
			{
			debug_lspec(&((AREA) sub->proto)->lspec,"lspec",9);
			debug_fspec(&((AREA) sub->proto)->fspec,"fspec",9);
			}
		else if (same(sub->type,"curve"))
			{
			debug_lspec(&((CURVE) sub->proto)->lspec,"lspec",9);
			}
		else if (same(sub->type,"label"))
			{
			debug_tspec(&((LABEL) sub->proto)->tspec,"tspec",9);
			}
		else if (same(sub->type,"mark"))
			{
			debug_mspec(&((MARK) sub->proto)->mspec,"mspec",9);
			}
		else if (same(sub->type,"barb"))
			{
			debug_bspec(&((BARB) sub->proto)->bspec,"bspec",9);
			debug_tspec(&((BARB) sub->proto)->tspec,"tspec",9);
			}
		else if (same(sub->type,"button"))
			{
			debug_lspec(&((BUTTON) sub->proto)->lspec,"lspec",9);
			debug_fspec(&((BUTTON) sub->proto)->fspec,"fspec",9);
			debug_tspec(&((BUTTON) sub->proto)->tspec,"tspec",9);
			}
		/* Note: int and float members do not have prototypes */
		}

	(void) printf("   points:\n");
	for (ip=0; ip<plot->numpts; ip++)
		{
		(void) printf("      (%g,%g)",plot->pts[ip][X],plot->pts[ip][Y]);
		for (isub=0; isub<plot->nsubs; isub++)
			{
			sub = plot->subs + isub;
			if (same(sub->type,"area"))
				{
				(void) printf(" \"%s\"",sub->sval1[ip]);
				}
			else if (same(sub->type,"curve"))
				{
				(void) printf(" \"%s\"",sub->sval1[ip]);
				}
			else if (same(sub->type,"label"))
				{
				(void) printf(" \"%s\"",sub->sval2[ip]);
				}
			else if (same(sub->type,"mark"))
				{
				(void) printf(" \"%s\"",sub->sval1[ip]);
				}
			else if (same(sub->type,"barb"))
				{
				(void) printf(" %d",NINT(sub->fval1[ip])); /*dir*/
				(void) printf(" %d",NINT(sub->fval2[ip])); /*spd*/
				}
			else if (same(sub->type,"button"))
				{
				(void) printf(" \"%s\"",sub->sval1[ip]);
				}
			else if (same(sub->type,"label"))
				{
				(void) printf(" \"%s\"",sub->sval1[ip]);
				}
			else if (same(sub->type,"int"))
				{
				(void) printf(" %d",sub->ival1[ip]);
				}
			else if (same(sub->type,"float"))
				{
				(void) printf(" %f",sub->fval1[ip]);
				}
			}
		(void) printf("\n");
		}
	return TRUE;
	}
#endif /* DEBUG_PSPEC */
