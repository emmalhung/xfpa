/**********************************************************************/
/** @file lchain.c
 *
 *  Routines to handle the LCHAIN objects.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*      l c h a i n . c                                                 *
*                                                                      *
*      Routines to handle the LCHAIN objects.                          *
*                                                                      *
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

#define LCHAIN_INIT
#include "lchain.h"

#include <tools/tools.h>

#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

#undef DEBUG_MEMBER

/***********************************************************************
*                                                                      *
*      Routines specific to the LCHAIN object                          *
*                                                                      *
***********************************************************************/

int		LchainCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ l c h a i n                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create a new link chain with given reference time
 *
 * @return	new lchain object.
 * 			(You should destroy the link chain when you are finished with it)
 ***********************************************************************/
LCHAIN	create_lchain(void)

	{
	LCHAIN	lnew;

	/* Allocate space for the structure */
	lnew = INITMEM(struct LCHAIN_struct, 1);
	if (!lnew) return NullLchain;

	/* Initialize the structure */
	lnew->xtime    = NullString;
	lnew->attrib   = create_attrib_list();
	lnew->splus    = 0;
	lnew->eplus    = 0;
	lnew->lnum     = 0;
	lnew->nodes    = NullLnodeList;
	lnew->dointerp = FALSE;
	lnew->minterp  = 0;
	lnew->inum     = 0;
	lnew->interps  = NullLinterpList;
	lnew->track    = NullLine;
	init_lspec(&lnew->lspec);

	/* Return the new link chain */
	LchainCount++;
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ l c h a i n                                     *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Destroy space allocated to the given link chain.
 *
 * 	@param[in]	lchain	link chain to be destroyed.
 * 	@return NullLchain pointer.
 ***********************************************************************/
LCHAIN	destroy_lchain

	(
	LCHAIN	lchain	/* link chain to be destroyed */
	)

	{
	int		ii;

	/* Do nothing if link chain not there */
	if (!lchain) return NullLchain;

	/* Free the space used by the T0 reference time stamp */
	FREEMEM(lchain->xtime);

	/* Free the space used by the attribute list */
	lchain->attrib = destroy_attrib_list(lchain->attrib);

	/* Free the space used by link nodes */
	for (ii=0; ii<lchain->lnum; ii++)
		lchain->nodes[ii] = destroy_lnode(lchain->nodes[ii]);
	FREEMEM(lchain->nodes);

	/* Free the space used by interpolated nodes */
	for (ii=0; ii<lchain->inum; ii++)
		lchain->interps[ii] = destroy_linterp(lchain->interps[ii]);
	FREEMEM(lchain->interps);

	/* Free the space used by the link chain track */
	lchain->track = destroy_line(lchain->track);

	/* Free the space used by presentation specs */
	free_lspec(&lchain->lspec);

	/* Free the structure itself */
	FREEMEM(lchain);
	LchainCount--;
	return NullLchain;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ l c h a i n                                         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Empty the data buffers of the given link chain.
 *
 * 	@param[in]	lchain	link chain to empty.
 ***********************************************************************/
void	empty_lchain

	(
	LCHAIN	lchain	/* link chain to be emptied */
	)

	{
	int		ii;

	/* Do nothing if link chain not there */
	if (!lchain) return;

	/* Free the space used by the attribute list */
	empty_attrib_list(lchain->attrib);

	/* Reset the link node structures ... except for the node time */
	for (ii=0; ii<lchain->lnum; ii++)
		(void) empty_lnode(lchain->nodes[ii]);

	/* Reset the interpolated node structures ... except for the node time */
	lchain->dointerp = FALSE;
	for (ii=0; ii<lchain->inum; ii++)
		(void) empty_linterp(lchain->interps[ii]);

	/* Reset the link chain track */
	(void) empty_line(lchain->track);
	}

/***********************************************************************
*                                                                      *
*      c o p y _ l c h a i n                                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create an exact copy of a link chain.
 *
 * @param[in]	lchain	link chain to be copied.
 * @return	Pointer to copy of link chain.
 * 			(You should destroy the copy when you are finished with it)
 ***********************************************************************/
LCHAIN	copy_lchain

	(
	const LCHAIN	lchain	/* link chain to be copied */
	)

	{
	int		ii;
	LCHAIN	lnew;

	/* Do nothing if link chain not there */
	if (!lchain) return NullLchain;

	/* Create an empty copy */
	lnew = create_lchain();

	/* Duplicate the T0 reference time */
	lnew->xtime = safe_strdup(lchain->xtime);

	/* Duplicate the attributes */
	lnew->attrib = copy_attrib_list(lchain->attrib);

	/* Duplicate the start and end times */
	lnew->splus = lchain->splus;
	lnew->eplus = lchain->eplus;

	/* Duplicate the link nodes */
	if (lchain->lnum > 0)
		{
		lnew->lnum  = lchain->lnum;
		lnew->nodes = INITMEM(LNODE, lnew->lnum);
		for (ii=0; ii<lchain->lnum; ii++)
			lnew->nodes[ii] = copy_lnode(lchain->nodes[ii]);
		}

	/* Duplicate the interpolated nodes */
	lnew->dointerp = lchain->dointerp;
	lnew->minterp  = lchain->minterp;
	if (lchain->inum > 0)
		{
		lnew->inum    = lchain->inum;
		lnew->interps = INITMEM(LINTERP, lnew->inum);
		for (ii=0; ii<lchain->inum; ii++)
			lnew->interps[ii] = copy_linterp(lchain->interps[ii]);
		}

	/* Duplicate the link chain track */
	lnew->track = copy_line(lchain->track);

	/* Duplicate presentation specs */
	copy_lspec(&lnew->lspec, &lchain->lspec);

	/* Return the copy */
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      v a l i d _ l c h a i n                                         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Check the link nodes of a link chain.
 *
 * @param[in]	lchain	link chain to be checked.
 * @param[out]	lnum	number of acceptable link nodes.
 * @param[out]	inum	number of acceptable interpolated nodes.
 * @return True if link nodes are acceptable.
 ***********************************************************************/
LOGICAL	valid_lchain

	(
	const LCHAIN	lchain,	/* link chain to be checked */
	int				*lnum,	/* number of acceptable link nodes */
	int				*inum	/* number of acceptable interpolated nodes */
	)

	{
	int		ii, nl, nc, nf, ni;

	/* Initialize return parameters */
	if (lnum) *lnum = 0;
	if (inum) *inum = 0;

	/* Return FALSE no link chain or link nodes */
	if (!lchain)           return FALSE;
	if (lchain->lnum <= 0) return FALSE;

	/* Check for acceptable link nodes */
	for (nl=0, ii=0; ii<lchain->lnum; ii++)
		{
		if (!lchain->nodes[ii]->there)              continue;
		if (lchain->nodes[ii]->ltype != LchainNode) continue;
		nl++;
		}

	/* Check for acceptable control nodes */
	for (nc=0, ii=0; ii<lchain->lnum; ii++)
		{
		if (!lchain->nodes[ii]->there)                 continue;
		if (lchain->nodes[ii]->ltype != LchainControl) continue;
		nc++;
		}

	/* Check for acceptable floating nodes */
	for (nf=0, ii=0; ii<lchain->lnum; ii++)
		{
		if (!lchain->nodes[ii]->there)                  continue;
		if (lchain->nodes[ii]->ltype != LchainFloating) continue;
		nf++;
		}

	/* Return FALSE if no acceptable link or control nodes */
	/* Note that link chain cannot be all floating nodes!  */
	if (nl <= 0 && nc <= 0) return FALSE;

	/* Check for acceptable interpolated nodes */
	for (ni=0, ii=0; ii<lchain->inum; ii++)
		if (lchain->interps[ii]->there) ni++;

	/* Set return parameters */
	if (lnum) *lnum = nl + nc + nf;
	if (inum) *inum = ni;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l c h a i n _ r e f e r e n c e _ t i m e         *
*      d e f i n e _ l c h a i n _ s t a r t _ t i m e                 *
*      d e f i n e _ l c h a i n _ e n d _ t i m e                     *
*      d e f i n e _ l c h a i n _ a t t r i b s                       *
*      d e f i n e _ l c h a i n _ d e f a u l t _ a t t r i b s       *
*      r e c a l l _ l c h a i n _ a t t r i b s                       *
*      d e f i n e _ l c h a i n _ i n t e r p _ d e l t a             *
*                                                                      *
*      Set the parameters for a link chain.                            *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Set the reference time and reset all node times in a link chain.
 *
 * @param[in]	lchain	link chain to be modified.
 * @param[in]	xtime	new reference time stamp for link chain.
 ***********************************************************************/
void	define_lchain_reference_time

	(
	const LCHAIN	lchain,	/* link chain to be modified */
	STRING			xtime	/* new reference time stamp */
	)

	{
	int		ii, idif;
	int		xyear, xjday, xhour, xmin;
	int		lyear, ljday, lhour, lmin;
	LOGICAL xlocal, xmins, llocal, lmins;

	/* Do nothing if link chain not there or no reference time */
	/*  or reference time has not changed                      */
	if (!lchain)                    return;
	if (blank(xtime))               return;
	if (same(xtime, lchain->xtime)) return;

	/* Error message if problem determining reference times */
	if ( !parse_tstamp(xtime, &xyear, &xjday, &xhour, &xmin, &xlocal, &xmins)
			|| xlocal )
		{
		(void) pr_error("Lchains",
			"Error with new link chain reference time: %s\n", xtime);
		return;
		}

	/* Set the reference time if not yet set */
	if (blank(lchain->xtime))
		{
		lchain->xtime = safe_strdup(xtime);
		return;
		}

	/* Determine time difference between reference times */
	if ( !parse_tstamp(lchain->xtime, &lyear, &ljday, &lhour, &lmin,
			&llocal, &lmins) || llocal )
		{
		(void) pr_error("Lchains",
			"Error with current link chain reference time: %s\n",
			lchain->xtime);
		return;
		}
	idif = mdif(lyear, ljday, lhour, lmin, xyear, xjday, xhour, xmin);

	/* Replace the link chain reference time */
	FREEMEM(lchain->xtime);
	lchain->xtime = safe_strdup(xtime);

	/* Return now if no time difference between reference times */
	if (idif == 0) return;

	/* Reset the link chain start and end times */
	lchain->splus -= idif;
	lchain->eplus -= idif;

	/* Reset the link node times */
	if (lchain->lnum > 0)
		{
		for (ii=0; ii<lchain->lnum; ii++) lchain->nodes[ii]->mplus -= idif;
		}

	/* Reset the interpolated node times */
	if (lchain->inum > 0)
		{
		for (ii=0; ii<lchain->inum; ii++) lchain->interps[ii]->mplus -= idif;
		}
	}

/***********************************************************************/
/** Set the start time in a link chain and remove all nodes before start.
 *
 * @param[in]	lchain	link chain to be modified.
 * @param[in]	splus	new start time for link chain.
 ***********************************************************************/
void	define_lchain_start_time

	(
	const LCHAIN	lchain,	/* link chain to be modified */
	int				splus	/* new start time */
	)

	{
	int		ii, mplus;

	/* Do nothing if link chain not there or start time has not changed */
	if (!lchain)                return;
	if (splus == lchain->splus) return;

	/* Replace the link chain start time */
	lchain->splus = splus;

	/* Remove link nodes before new start time */
	for (ii=lchain->lnum-1; ii>=0; ii--)
		{
		mplus = lchain->nodes[ii]->mplus;
		if (mplus < splus) (void) remove_lchain_lnode(lchain, ii);
		}
	
	/* Interpolated nodes and track need to be recalculated */
	lchain->dointerp  = TRUE;
	}

/***********************************************************************/
/** Set the end time in a link chain and remove all nodes after end.
 *
 * @param[in]	lchain	link chain to be modified.
 * @param[in]	eplus	new end time for link chain.
 ***********************************************************************/
void	define_lchain_end_time

	(
	const LCHAIN	lchain,	/* link chain to be modified */
	int				eplus	/* new end time */
	)

	{
	int		ii, mplus;

	/* Do nothing if link chain not there or end time has not changed */
	if (!lchain)                return;
	if (eplus == lchain->eplus) return;

	/* Replace the link chain end time */
	lchain->eplus = eplus;

	/* Remove link nodes after new end time */
	for (ii=lchain->lnum-1; ii>=0; ii--)
		{
		mplus = lchain->nodes[ii]->mplus;
		if (mplus > eplus) (void) remove_lchain_lnode(lchain, ii);
		}

	/* Interpolated nodes and track need to be recalculated */
	lchain->dointerp  = TRUE;
	}

/***********************************************************************/
/** Set the attributes for a link chain.
 *
 * @param[in]	lchain	link chain to be modified.
 * @param[in]	attribs	new list of attributes.
 ***********************************************************************/
void	define_lchain_attribs

	(
	const LCHAIN	lchain,	/* link chain to be modified */
	ATTRIB_LIST		attribs	/* new list of attributes */
	)

	{

	/* Do nothing if link chain not there */
	if (!lchain) return;

	/* Free the attribute list */
	lchain->attrib = destroy_attrib_list(lchain->attrib);

	/* Reset the attribute list (if required) */
	if (NotNull(attribs)) lchain->attrib = copy_attrib_list(attribs);

	/* Define the default attributes */
	define_lchain_default_attribs(lchain);
	return;
	}

/***********************************************************************/
/** Set the default attributes for a link chain.
 *
 * @param[in]	lchain	link chain to be modified.
 ***********************************************************************/
void	define_lchain_default_attribs

	(
	const LCHAIN	lchain	/* link chain to be modified */
	)

	{
	int		ii;
	char	iplus[20];

	/* Do nothing if link chain not there */
	if (!lchain) return;

	/* Reset the default attributes */
	(void) add_attribute(lchain->attrib, AttribLchainReference, lchain->xtime);
	(void) sprintf(iplus, "%d", lchain->splus);
	(void) add_attribute(lchain->attrib, AttribLchainStartTime, iplus);
	(void) sprintf(iplus, "%d", lchain->eplus);
	(void) add_attribute(lchain->attrib, AttribLchainEndTime,   iplus);

	/* Reset the default attributes for all link nodes */
	if (lchain->lnum > 0)
		{
		for (ii=0; ii<lchain->lnum; ii++)
			define_lnode_default_attribs(lchain->nodes[ii]);
		}

	/* Reset the default attributes for all interpolated nodes */
	if (lchain->inum > 0)
		{
		for (ii=0; ii<lchain->inum; ii++)
			define_linterp_default_attribs(lchain->interps[ii]);
		}
	}

/*********************************************************************/
/** Retrieve the attributes of the given link chain.
 *
 *	@param[in] 	lchain		requested link chain.
 *	@param[out] *attribs	list of attributes.
 *********************************************************************/
void	recall_lchain_attribs

	(
	LCHAIN		lchain,		/* Link chain */
	ATTRIB_LIST	*attribs	/* List of attributes for link chain */
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (lchain) ? lchain->attrib : NullAttribList;
	}

/***********************************************************************/
/** Set the interpolation delta for a link track.
 *
 * @param[in]	lchain	link chain to be modified.
 * @param[in]	minterp	new interpolation delta for link track.
 ***********************************************************************/
void	define_lchain_interp_delta

	(
	const LCHAIN	lchain,	/* link chain to be modified */
	int				minterp	/* new interpolation delta */
	)

	{
	int		ii, idif;
	int		xyear, xjday, xhour, xmin;
	int		lyear, ljday, lhour, lmin;
	LOGICAL xlocal, xmins, llocal, lmins;

	/* Do nothing if link chain not there */
	if (!lchain)      return;

	/* Set the interpolation delta if not yet set */
	if (lchain->minterp <= 0)
		{
		lchain->minterp = minterp;
		return;
		}

	/* Replace the link chain interpolation delta (if required) */
	if (lchain->minterp != minterp)
		{
		lchain->dointerp = TRUE;
		lchain->minterp  = minterp;
		}

	return;
	}

/***********************************************************************
*                                                                      *
*      c r e a t e _ l n o d e                                         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create a new link node
 *
 * @param[in] 	mplus	time for link node.
 * @return	new lnode object.
 ***********************************************************************/
LNODE	create_lnode

	(
	int		mplus		/* time for link node (in minutes from T0 reference) */
	)

	{
	LNODE	lnew;

	/* Allocate space for the structure */
	lnew = INITMEM(struct LNODE_struct, 1);
	if (!lnew) return NullLnode;

	/* Initialize the structure */
	lnew->there   = FALSE;
	lnew->guess   = FALSE;
	/* >>>>> this gets changed to input value <<<<< */
	lnew->ltype   = LchainUnknown;
	/* >>>>> this gets changed to input value <<<<< */
	lnew->mplus   = mplus;
	lnew->attrib  = create_attrib_list();
	lnew->attach  = -1;
	lnew->mtype   =  0;
	lnew->imem    = -1;
	lnew->members = NullNodeMemList;
	lnew->nmem    = 0;
	set_point(lnew->node, -1., -1.);

	/* Return the new link node */
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ l n o d e                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Destroy space allocated to the given link node.
 *
 * 	@param[in]	lnode	link node to be destroyed.
 * 	@return NullLnode pointer.
 ***********************************************************************/
LNODE	destroy_lnode

	(
	LNODE	lnode	/* link node to be destroyed */
	)

	{
	int		ii;

	/* Do nothing if link node not there */
	if (!lnode) return NullLnode;

	/* Free the space used by the attribute list */
	lnode->attrib = destroy_attrib_list(lnode->attrib);

	/* Free the node members */
	for (ii=0; ii<lnode->nmem; ii++)
		(void) free_node_mem(lnode->members + ii);
	FREEMEM(lnode->members);

	/* Free the structure itself */
	FREEMEM(lnode);
	return NullLnode;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ l n o d e                                           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Empty the data buffers of the given link node, except for the time.
 *
 * 	@param[in]	lnode	link node to empty.
 ***********************************************************************/
void	empty_lnode

	(
	LNODE	lnode	/* link node to be emptied */
	)

	{
	int		ii;

	/* Do nothing if link node not there */
	if (!lnode) return;

	/* Free the space used by the attribute list */
	empty_attrib_list(lnode->attrib);

	/* Free the node members */
	for (ii=0; ii<lnode->nmem; ii++)
		(void) free_node_mem(lnode->members + ii);
	FREEMEM(lnode->members);
	lnode->nmem = 0;

	/* Reset the data structure ... except for the node type and time */
	lnode->there  = FALSE;
	lnode->guess  = FALSE;
	lnode->attach = -1;
	lnode->mtype  =  0;
	lnode->imem   = -1;
	set_point(lnode->node, -1., -1.);
	}

/***********************************************************************
*                                                                      *
*      c o p y _ l n o d e                                             *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create an exact copy of a link node.
 *
 * @param[in]	lnode	link node to be copied.
 * @return	Pointer to copy of link node.
 * 			(You should destroy the copy when you are finished with it)
 ***********************************************************************/
LNODE	copy_lnode

	(
	const LNODE	lnode	/* link node to be copied */
	)

	{
	int		ii;
	LNODE	lnew;
	NODEMEM	*nodemem;

	/* Do nothing if link node not there */
	if (!lnode) return NullLnode;

	/* Create an empty copy */
	lnew = create_lnode(lnode->mplus);

	/* Duplicate the link node information */
	lnew->there  = lnode->there;
	lnew->guess  = lnode->guess;
	lnew->ltype  = lnode->ltype;
	lnew->attrib = copy_attrib_list(lnode->attrib);
	lnew->attach = lnode->attach;
	lnew->mtype  = lnode->mtype;
	lnew->imem   = lnode->imem;
	copy_point(lnew->node, lnode->node);

	/* Duplicate the node members */
	for (ii=0; ii<lnode->nmem; ii++)
		{
		nodemem = lnode->members + ii;
		(void) add_lnode_member(lnew, nodemem->name, nodemem->type);
		(void) copy_node_mem(lnew->members + ii, nodemem);
		}

	/* Return the copy */
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l n o d e _ t y p e                               *
*      d e f i n e _ l n o d e _ n o d e                               *
*      d e f i n e _ l n o d e _ a t t r i b s                         *
*      d e f i n e _ l n o d e _ d e f a u l t _ a t t r i b s         *
*      r e c a l l _ l n o d e _ a t t r i b s                         *
*      d e f i n e _ l n o d e _ a t t a c h                           *
*                                                                      *
*      Set the parameters for a link node.                             *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Set the node type for a link node.
 *
 * @param[in]	lnode	link node to be modified.
 * @param[in]	there	new check for link node in use.
 * @param[in]	guess	new check for guess link node.
 * @param[in]	ltype	new link node type.
 ***********************************************************************/
void	define_lnode_type

	(
	const LNODE	lnode,	/* link node to be modified */
	LOGICAL		there,	/* is the link node in use? */
	LOGICAL		guess,	/* is the link node a guess? */
	LMEMBER		ltype	/* link node type */
	)

	{

	/* Do nothing if link node not there */
	if (!lnode) return;

	/* Reset the link node type */
	lnode->there = there;
	lnode->guess = guess;
	lnode->ltype = ltype;
	return;
	}

/***********************************************************************/
/** Set the node location for a link node.
 *
 * @param[in]	lnode	link node to be modified.
 * @param[in]	node	new node location.
 ***********************************************************************/
void	define_lnode_node

	(
	const LNODE	lnode,	/* link node to be modified */
	POINT		node	/* link node location */
	)

	{

	/* Do nothing if link node not there */
	if (!lnode) return;

	/* Reset the link node location */
	copy_point(lnode->node, node);
	return;
	}

/***********************************************************************/
/** Set the attributes for a link node.
 *
 * @param[in]	lnode	link node to be modified.
 * @param[in]	attribs	new list of attributes.
 ***********************************************************************/
void	define_lnode_attribs

	(
	const LNODE	lnode,	/* link node to be modified */
	ATTRIB_LIST	attribs	/* new list of attributes */
	)

	{

	/* Do nothing if link node not there */
	if (!lnode) return;

	/* Free the attribute list */
	lnode->attrib = destroy_attrib_list(lnode->attrib);

	/* Reset the attribute list (if required) */
	if (NotNull(attribs)) lnode->attrib = copy_attrib_list(attribs);

	/* Define the default attributes */
	define_lnode_default_attribs(lnode);
	return;
	}

/***********************************************************************/
/** Set the default attributes for a link node.
 *
 * @param[in]	lnode	link node to be modified.
 ***********************************************************************/
void	define_lnode_default_attribs

	(
	const LNODE	lnode	/* link node to be modified */
	)

	{
	char	iplus[20];

	/* Do nothing if link node not there */
	if (!lnode) return;

	/* Reset the default attributes */
	if (lnode->ltype == LchainFloating)
		{
		(void) add_attribute(lnode->attrib, AttribLnodeType,
													FpaNodeClass_Floating);
		(void) add_attribute(lnode->attrib, AttribCategory,
													FpaNodeClass_Floating);
		}
	else if (lnode->ltype == LchainControl && lnode->guess)
		{
		(void) add_attribute(lnode->attrib, AttribLnodeType,
													FpaNodeClass_ControlGuess);
		(void) add_attribute(lnode->attrib, AttribCategory,
													FpaNodeClass_ControlGuess);
		}
	else if (lnode->ltype == LchainControl)
		{
		(void) add_attribute(lnode->attrib, AttribLnodeType,
													FpaNodeClass_Control);
		(void) add_attribute(lnode->attrib, AttribCategory,
													FpaNodeClass_Control);
		}
	else if (lnode->ltype == LchainNode && lnode->guess)
		{
		(void) add_attribute(lnode->attrib, AttribLnodeType,
													FpaNodeClass_NormalGuess);
		(void) add_attribute(lnode->attrib, AttribCategory,
													FpaNodeClass_NormalGuess);
		}
	else
		{
		(void) add_attribute(lnode->attrib, AttribLnodeType,
													FpaNodeClass_Normal);
		(void) add_attribute(lnode->attrib, AttribCategory,
													FpaNodeClass_Normal);
		}
	(void) sprintf(iplus, "%d", lnode->mplus);
	(void) add_attribute(lnode->attrib, AttribLnodeTime, iplus);
	return;
	}

/*********************************************************************/
/** Retrieve the attributes of the given link node.
 *
 *	@param[in] 	lnode		requested link node.
 *	@param[out] *attribs	list of attributes.
 *********************************************************************/
void	recall_lnode_attribs

	(
	LNODE		lnode,		/* Link node */
	ATTRIB_LIST	*attribs	/* List of attributes for link node */
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (lnode) ? lnode->attrib : NullAttribList;
	}

/***********************************************************************/
/** Set the attach parameters for a link node.
 *
 * @param[in]	lnode	link node to be modified.
 * @param[in]	attach	new index to item that node is attached to.
 * @param[in]	mtype	new type of item memeber.
 * @param[in]	imem	new index to type of item member.
 ***********************************************************************/
void	define_lnode_attach

	(
	const LNODE	lnode,	/* link node to be modified */
	int			attach,	/* index to item that link node is attached to */
	int			mtype,	/* type of item member for link node */
	int			imem	/* index to type of item member for link node */
	)

	{

	/* Do nothing if link node not there */
	if (!lnode) return;

	/* Reset the attach parameters */
	lnode->attach = attach;
	lnode->mtype  = mtype;
	lnode->imem   = imem;
	return;
	}

/***********************************************************************
*                                                                      *
*      a d d _ l n o d e _ m e m b e r                                 *
*      r e m o v e _ l n o d e _ m e m b e r                           *
*      w h i c h _ l n o d e _ m e m b e r                             *
*      b u i l d _ l n o d e _ m e m b e r s                           *
*                                                                      *
*      Add/remove members for displaying the given link node.          *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Add a node member for displaying the given link node.
 *
 *	@param[in] 	lnode	given link node
 *	@param[in] 	name	name of node member to add
 *	@param[in] 	type	type of node member
 *********************************************************************/
void	add_lnode_member

	(
	LNODE	lnode,
	STRING	name,
	NDMTYPE	type
	)

	{
	int		imem;
	NODEMEM	*mem;

	if (IsNull(lnode)) return;

	imem = which_lnode_member(lnode, name, &mem);
	if (IsNull(mem))
		{
		imem = lnode->nmem++;
		lnode->members = GETMEM(lnode->members, NODEMEM, lnode->nmem);
		mem = lnode->members + imem;
		(void) init_node_mem(mem);
		}
	define_node_mem_type(mem, name, type);
	}

/**********************************************************************/

/*********************************************************************/
/** Remove a node member from the given link node.
 *
 *	@param[in] 	lnode	given link node
 *	@param[in] 	name	name of node member to remove
 *********************************************************************/
void	remove_lnode_member

	(
	LNODE	lnode,
	STRING	name
	)

	{
	int		imem;
	NODEMEM	*mem;

	if (IsNull(lnode)) return;

	imem = which_lnode_member(lnode, name, &mem);
	if (IsNull(mem)) return;

	/* Compress the list */
	lnode->nmem--;
	for ( ; imem<lnode->nmem; imem++)
		{
		(void) copy_node_mem(mem, mem+1);
		mem++;
		}
	(void) free_node_mem(mem);
	}

/**********************************************************************/

/*********************************************************************/
/** Lookup a particular node member of a given link node.
 *
 *	@param[in] 	lnode	given link node
 *	@param[in] 	name	name of member to lookup
 *	@param[out]	**mem	return a pointer to the node member object
 * 	@return The index of the node member in the member list. <0 if
 * 			failed.
 *********************************************************************/
int		which_lnode_member

	(
	LNODE	lnode,
	STRING	name,
	NODEMEM	**mem
	)

	{
	int		imem;
	NODEMEM	*cmem;

	if (NotNull(mem)) *mem = NullNodeMemPtr;

	if (IsNull(lnode)) return -1;
	if (blank(name))   return -1;

	for (imem=0; imem<lnode->nmem; imem++)
		{
		cmem = lnode->members + imem;
		if (!same(name, cmem->name)) continue;

		if (NotNull(mem)) *mem = cmem;
		return imem;
		}

	return -1;
	}

/**********************************************************************/
/**  Build node members for a given link node.
 * 	@param[in]	lnode	Link node to build node members
 * 	@param[in]	ncspec	Number of category specifications
 * 	@param[in]	*cspecs	List of category specifications
 **********************************************************************/

void	build_lnode_members

	(
	LNODE			lnode,
	int				ncspec,
	const CATSPEC	*cspecs
	)

	{
	int		isp, imem;
	STRING	mclass, pname;
	CATSPEC	*cs;
	NODEMEM	*mem;
	NDMTYPE	mtype;
	LOGICAL	found;

	if (IsNull(lnode)) return;
	if (ncspec <= 0)   return;

#	ifdef DEBUG_MEMBER
	(void) pr_status("Lnodes", "build_lnode_members[]\n");
#	endif /* DEBUG_MEMBER */

	/* Set the node class */
	mclass = lnode->guess? FpaNodeClass_NormalGuess: FpaNodeClass_Normal;

	/* See that link node has members to match all members from catspecs */
	pname = NULL;
	found = FALSE;
	for (isp=0; isp<ncspec; isp++)
		{
		cs = (CATSPEC *) cspecs + isp;

		/* Only apply to the first unique member of the matching class */
		/* Note that this saves a placeholder for presentation */
		/*  ... the actual presentation is defined later!      */
		if (!same(cs->mclass, mclass)) continue;
		found = TRUE;
		if (same(cs->name, pname))     continue;
		pname = cs->name;

		/* See if link node already has this member */
		imem = which_lnode_member(lnode, cs->name, 0);
		if (imem >= 0) continue;

		/* Check member type */
		mtype = node_mem_type(cs->type);
		if (mtype < 0)
			{
			(void) pr_error("Lnodes",
					"Unrecognized link node member type \"%s\"\n", cs->type);
			continue;
			}

#		ifdef DEBUG_MEMBER
		(void) pr_status("Lnodes",
					"  Adding link node member: %s %s %s  for class: %s\n",
					cs->type, cs->name,
					(!blank(cs->attrib)? cs->attrib: "-"), cs->mclass);
#		endif /* DEBUG_MEMBER */

		/* Add the member */
		add_lnode_member(lnode, cs->name, mtype);
		mem = lnode->members + lnode->nmem - 1;
		if (!blank(cs->attrib)) define_node_mem_attrib(mem, cs->attrib);
		}

	/* Complain if no presentation was found */
	if (lnode->nmem <= 0)
		{
		if (!found) (void) pr_error("Lnodes",
					"No presentation for node class \"%s\"\n", mclass);
		else        (void) pr_error("Lnodes",
					"No members for node class \"%s\"\n", mclass);
		}
	}

/***********************************************************************
*                                                                      *
*      a d d _ l c h a i n _ l n o d e                                 *
*      r e m o v e _ l c h a i n _ l n o d e                           *
*                                                                      *
*      Set or reset the link nodes for the given link chain.           *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Add (or replace) a link node for the given link chain.
 *
 * @param[in]	lchain	link chain to be given the node.
 * @param[in]	lnode	link node to add.
 * @return	index of link node.
 ***********************************************************************/
int		add_lchain_lnode

	(
	LCHAIN	lchain,	/* given link chain */
	LNODE	lnode	/* link node to add */
	)

	{
	int		lnum, ii, il;

	/* Do nothing if link chain not there */
	if (!lchain) return -1;
	if (!lnode)  return -1;

	/* Determine location of link node in link chain */
	lnum = lchain->lnum;
	for (ii=0; ii<lnum; ii++)
		{
		if      (lnode->mplus > lchain->nodes[ii]->mplus) continue;
		else if (lnode->mplus < lchain->nodes[ii]->mplus) break;
		else
			{

			/* Replace the link node at matching time */
			lchain->nodes[ii] = destroy_lnode(lchain->nodes[ii]);
			lchain->nodes[ii] = lnode;
			lchain->dointerp  = TRUE;
			return ii;
			}
		}

	/* Make space for the new link node */
	lchain->lnum++;
	lchain->nodes = GETMEM(lchain->nodes, LNODE, lchain->lnum);

	/* Move link nodes to make space for new link node (if required) */
	if (ii < lnum)
		{
		for (il=lnum; il>ii; il--)
			lchain->nodes[il] = lchain->nodes[il-1];
		}

	/* Add the new link node */
	lchain->nodes[ii] = lnode;
	lchain->dointerp  = TRUE;
	return ii;
	}

/***********************************************************************/
/**	Remove a link node from the given link chain.
 *
 * @param[in]	lchain	link chain to remove the node from.
 * @param[in]	inode	link node to remove.
 * @return	True if successful.
 ***********************************************************************/
LOGICAL	remove_lchain_lnode

	(
	LCHAIN	lchain,	/* given link chain */
	int		inode	/* link node to remove */
	)

	{
	int		ii;

	/* Do nothing if link chain not there or no link nodes */
	if (!lchain)               return FALSE;
	if (lchain->lnum <= 0)     return FALSE;
	if (inode < 0)             return FALSE;
	if (inode >= lchain->lnum) return FALSE;

	/* Free space used by this link node */
	lchain->nodes[inode] = destroy_lnode(lchain->nodes[inode]);

	/* Remove the link node from the list */
	lchain->lnum--;
	for (ii=inode; ii<lchain->lnum; ii++)
		{
		lchain->nodes[ii] = lchain->nodes[ii+1];
		}
	lchain->nodes[ii] = NullLnode;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      c r e a t e _ l i n t e r p                                     *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create a new interpolated node
 *
 * @return	new linterp object.
 ***********************************************************************/
LINTERP	create_linterp

	(
	int		mplus		/* time for interpolated node (in minutes from T0 */
	)

	{
	LINTERP	lnew;

	/* Allocate space for the structure */
	lnew = INITMEM(struct LINTERP_struct, 1);
	if (!lnew) return NullLinterp;

	/* Initialize the structure */
	lnew->there   = FALSE;
	lnew->depict  = FALSE;
	lnew->mplus   = mplus;
	lnew->attrib  = create_attrib_list();
	lnew->members = NullNodeMemList;
	lnew->nmem    = 0;
	set_point(lnew->node, -1., -1.);

	/* Return the new interpolated node */
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ l i n t e r p                                   *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Destroy space allocated to the given interpolated node.
 *
 * 	@param[in]	linterp	interpolated node to be destroyed.
 * 	@return NullLinterp pointer.
 ***********************************************************************/
LINTERP	destroy_linterp

	(
	LINTERP	linterp	/* interpolated node to be destroyed */
	)

	{
	int		ii;

	/* Do nothing if interpolated node not there */
	if (!linterp) return NullLinterp;

	/* Free the space used by the attribute list */
	linterp->attrib = destroy_attrib_list(linterp->attrib);

	/* Free the node members */
	for (ii=0; ii<linterp->nmem; ii++)
		(void) free_node_mem(linterp->members + ii);
	FREEMEM(linterp->members);

	/* Free the structure itself */
	FREEMEM(linterp);
	return NullLinterp;
	}

/***********************************************************************
*                                                                      *
*      e m p t y _ l i n t e r p                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Empty the data buffers of the given interpolated node, except for the time.
 *
 * 	@param[in]	linterp	interpolated node to empty.
 ***********************************************************************/
void	empty_linterp

	(
	LINTERP	linterp	/* interpolated node to be emptied */
	)

	{
	int		ii;

	/* Do nothing if interpolated node not there */
	if (!linterp) return;

	/* Free the space used by the attribute list */
	empty_attrib_list(linterp->attrib);

	/* Free the node members */
	for (ii=0; ii<linterp->nmem; ii++)
		(void) free_node_mem(linterp->members + ii);
	FREEMEM(linterp->members);
	linterp->nmem = 0;

	/* Reset the data structure ... except for the node time */
	linterp->there  = FALSE;
	linterp->depict = FALSE;
	set_point(linterp->node, -1., -1.);
	}

/***********************************************************************
*                                                                      *
*      c o p y _ l i n t e r p                                         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Create an exact copy of a interpolated node.
 *
 * @param[in]	linterp	interpolated node to be copied.
 * @return	Pointer to copy of interpolated node.
 * 			(You should destroy the copy when you are finished with it)
 ***********************************************************************/
LINTERP	copy_linterp

	(
	const LINTERP	linterp	/* interpolated node to be copied */
	)

	{
	int		ii;
	LINTERP	lnew;
	NODEMEM	*nodemem;

	/* Do nothing if interpolated node not there */
	if (!linterp) return NullLinterp;

	/* Create an empty copy */
	lnew = create_linterp(linterp->mplus);

	/* Duplicate the interpolated node information */
	lnew->there   = linterp->there;
	lnew->depict  = linterp->depict;
	lnew->attrib  = copy_attrib_list(linterp->attrib);
	copy_point(lnew->node, linterp->node);

	/* Duplicate the node members */
	for (ii=0; ii<linterp->nmem; ii++)
		{
		nodemem = linterp->members + ii;
		(void) add_linterp_member(lnew, nodemem->name, nodemem->type);
		(void) copy_node_mem(lnew->members + ii, nodemem);
		}

	/* Return the copy */
	return lnew;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ l i n t e r p _ n o d e                           *
*      d e f i n e _ l i n t e r p _ a t t r i b s                     *
*      d e f i n e _ l i n t e r p _ d e f a u l t _ a t t r i b s     *
*      r e c a l l _ l i n t e r p _ a t t r i b s                     *
*                                                                      *
*      Set the parameters for an interpolated node.                    *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Set the node parameters for an interpolated node.
 *
 * @param[in]	linterp	interpolated node to be modified.
 * @param[in]	there	new check for interpolated node in use.
 * @param[in]	depict	new check for interpolated node at depiction time.
 * @param[in]	node	new node location.
 ***********************************************************************/
void	define_linterp_node

	(
	const LINTERP	linterp,	/* interpolated node to be modified */
	LOGICAL			there,		/* is the interpolated node in use? */
	LOGICAL			depict,		/* is the interpolated node a depiction? */
	POINT			node		/* interpolated node location */
	)

	{

	/* Do nothing if interpolated node not there */
	if (!linterp) return;

	/* Reset the interpolated node location */
	linterp->there  = there;
	linterp->depict = depict;
	copy_point(linterp->node, node);
	return;
	}

/***********************************************************************/
/** Set the attributes for an interpolated node.
 *
 * @param[in]	linterp	interpolated node to be modified.
 * @param[in]	attribs	new list of attributes.
 ***********************************************************************/
void	define_linterp_attribs

	(
	const LINTERP	linterp,	/* interpolated node to be modified */
	ATTRIB_LIST		attribs		/* new list of attributes */
	)

	{

	/* Do nothing if interpolated node not there */
	if (!linterp) return;

	/* Free the attribute list */
	linterp->attrib = destroy_attrib_list(linterp->attrib);

	/* Reset the attribute list (if required) */
	if (NotNull(attribs)) linterp->attrib = copy_attrib_list(attribs);

	/* Define the default attributes */
	define_linterp_default_attribs(linterp);
	return;
	}

/***********************************************************************/
/** Set the default attributes for an interpolated node.
 *
 * @param[in]	linterp	interpolated node to be modified.
 ***********************************************************************/
void	define_linterp_default_attribs

	(
	const LINTERP	linterp		/* interpolated node to be modified */
	)

	{
	char	iplus[20];

	/* Do nothing if interpolated node not there */
	if (!linterp) return;

	/* Reset the default attributes */
	(void) add_attribute(linterp->attrib, AttribLnodeType, FpaNodeClass_Interp);
	(void) add_attribute(linterp->attrib, AttribCategory,  FpaNodeClass_Interp);
	(void) sprintf(iplus, "%d", linterp->mplus);
	(void) add_attribute(linterp->attrib, AttribLnodeTime, iplus);
	return;
	}

/*********************************************************************/
/** Retrieve the attributes of the given interpolated node.
 *
 *	@param[in] 	linterp		requested interpolated node.
 *	@param[out] *attribs	list of attributes.
 *********************************************************************/
void	recall_linterp_attribs

	(
	LINTERP		linterp,	/* Interpolated node */
	ATTRIB_LIST	*attribs	/* List of attributes for interpolated node */
	)

	{
	/* Retrieve the attributes */
	if (attribs) *attribs = (linterp) ? linterp->attrib : NullAttribList;
	}

/***********************************************************************
*                                                                      *
*      a d d _ l i n t e r p _ m e m b e r                             *
*      r e m o v e _ l i n t e r p _ m e m b e r                       *
*      w h i c h _ l i n t e r p _ m e m b e r                         *
*      b u i l d _ l i n t e r p _ m e m b e r s                       *
*                                                                      *
*      Add/remove members for displaying the given interpolated node.  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Add a node member for displaying the given interpolated node.
 *
 *	@param[in] 	linterp	given interpolated node
 *	@param[in] 	name	name of node member to add
 *	@param[in] 	type	type of node member
 *********************************************************************/
void	add_linterp_member

	(
	LINTERP	linterp,
	STRING	name,
	NDMTYPE	type
	)

	{
	int		imem;
	NODEMEM	*mem;

	if (IsNull(linterp)) return;

	imem = which_linterp_member(linterp, name, &mem);
	if (IsNull(mem))
		{
		imem = linterp->nmem++;
		linterp->members = GETMEM(linterp->members, NODEMEM, linterp->nmem);
		mem = linterp->members + imem;
		(void) init_node_mem(mem);
		}
	define_node_mem_type(mem, name, type);
	}

/**********************************************************************/

/*********************************************************************/
/** Remove a node member from the given interpolated node.
 *
 *	@param[in] 	linterp	given interpolated node
 *	@param[in] 	name	name of node member to remove
 *********************************************************************/
void	remove_linterp_member

	(
	LINTERP	linterp,
	STRING	name
	)

	{
	int		imem;
	NODEMEM	*mem;

	if (IsNull(linterp)) return;

	imem = which_linterp_member(linterp, name, &mem);
	if (IsNull(mem)) return;

	/* Compress the list */
	linterp->nmem--;
	for ( ; imem<linterp->nmem; imem++)
		{
		(void) copy_node_mem(mem, mem+1);
		mem++;
		}
	(void) free_node_mem(mem);
	}

/**********************************************************************/

/*********************************************************************/
/** Lookup a particular node member of a given interpolated node.
 *
 *	@param[in] 	linterp	given interpolated node
 *	@param[in] 	name	name of member to lookup
 *	@param[out]	**mem	return a pointer to the node member object
 * 	@return The index of the node member in the member list. <0 if
 * 			failed.
 *********************************************************************/
int		which_linterp_member

	(
	LINTERP	linterp,
	STRING	name,
	NODEMEM	**mem
	)

	{
	int		imem;
	NODEMEM	*cmem;

	if (NotNull(mem)) *mem = NullNodeMemPtr;

	if (IsNull(linterp)) return -1;
	if (blank(name))     return -1;

	for (imem=0; imem<linterp->nmem; imem++)
		{
		cmem = linterp->members + imem;
		if (!same(name, cmem->name)) continue;

		if (NotNull(mem)) *mem = cmem;
		return imem;
		}

	return -1;
	}

/**********************************************************************/
/**  Build node members for a given interpolated node.
 * 	@param[in]	linterp	Interpolated node to build node members
 * 	@param[in]	ncspec	Number of category specifications
 * 	@param[in]	*cspecs	List of category specifications
 **********************************************************************/

void	build_linterp_members

	(
	LINTERP			linterp,
	int				ncspec,
	const CATSPEC	*cspecs
	)

	{
	int		isp, imem;
	STRING	mclass, pname;
	CATSPEC	*cs;
	NODEMEM	*mem;
	NDMTYPE	mtype;
	LOGICAL	found;

	if (IsNull(linterp)) return;
	if (ncspec <= 0)     return;

#	ifdef DEBUG_MEMBER
	(void) pr_status("Lnodes", "build_linterp_members[]\n");
#	endif /* DEBUG_MEMBER */

	/* Set the node class */
	mclass = FpaNodeClass_Interp;

	/* See that interpolated node has members to match all members */
	/*  from catspecs */
	pname = NULL;
	found = FALSE;
	for (isp=0; isp<ncspec; isp++)
		{
		cs = (CATSPEC *) cspecs + isp;

		/* Only apply to the first unique member of the matching class */
		/* Note that this saves a placeholder for presentation */
		/*  ... the actual presentation is defined later!      */
		if (!same(cs->mclass, mclass)) continue;
		found = TRUE;
		if (same(cs->name, pname))     continue;
		pname = cs->name;

		/* See if interpolated node already has this member */
		imem = which_linterp_member(linterp, cs->name, 0);
		if (imem >= 0) continue;

		/* Check member type */
		mtype = node_mem_type(cs->type);
		if (mtype < 0)
			{
			(void) pr_error("Lnodes",
					"Unrecognized interpolated node member type \"%s\"\n",
					cs->type);
			continue;
			}

#		ifdef DEBUG_MEMBER
		(void) pr_info("Lnodes",
					"  Adding interpolated node member: %s %s %s  for class: %s\n",
					cs->type, cs->name,
					(!blank(cs->attrib)? cs->attrib: "-"), cs->mclass);
#		endif /* DEBUG_MEMBER */

		/* Add the member */
		add_linterp_member(linterp, cs->name, mtype);
		mem = linterp->members + linterp->nmem - 1;
		if (!blank(cs->attrib)) define_node_mem_attrib(mem, cs->attrib);
		}

	/* Complain if no presentation was found */
	if (linterp->nmem <= 0)
		{
		if (!found) (void) pr_error("Lnodes",
					"No presentation for node class \"%s\"\n", mclass);
		else        (void) pr_error("Lnodes",
					"No members for node class \"%s\"\n", mclass);
		}
	}

/***********************************************************************
*                                                                      *
*      w h i c h _ l c h a i n _ n o d e                               *
*                                                                      *
*      Identify link nodes that match a given time.                    *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Identify the node that matches the given time.
 *
 * @param[in]	lchain	link chain containing nodes to match.
 * @param[in] 	ltype	type of node to match.
 * @param[in] 	mplus	time to match.
 * @return	Index of matching node (-1 if no match found).
 ***********************************************************************/

int		which_lchain_node

	(
	LCHAIN	lchain,
	LMEMBER	ltype,
	int		mplus
	)

	{
	int		ii;

	/* Return if no link chain or no nodes of the correct type */
	if (!lchain)                                           return -1;
	if      (ltype == LchainUnknown)                       return -1;
	else if (ltype == LchainNode     && lchain->lnum <= 0) return -1;
	else if (ltype == LchainControl  && lchain->lnum <= 0) return -1;
	else if (ltype == LchainFloating && lchain->lnum <= 0) return -1;
	else if (ltype == LchainInterp   && lchain->inum <= 0) return -1;

	/* Check for matching time for link nodes */
	if (ltype == LchainNode)
		{
		for (ii=0; ii<lchain->lnum; ii++)
			{
			if (lchain->nodes[ii]->mplus == mplus
				&& lchain->nodes[ii]->ltype == LchainNode) return ii;
			else if (lchain->nodes[ii]->mplus == mplus)    break;
			}
		}

	/* Check for matching time for control nodes */
	else if (ltype == LchainControl)
		{
		for (ii=0; ii<lchain->lnum; ii++)
			{
			if (lchain->nodes[ii]->mplus == mplus
				&& lchain->nodes[ii]->ltype == LchainControl) return ii;
			else if (lchain->nodes[ii]->mplus == mplus)       break;
			}
		}

	/* Check for matching time for floating nodes */
	else if (ltype == LchainFloating)
		{
		for (ii=0; ii<lchain->lnum; ii++)
			{
			if (lchain->nodes[ii]->mplus == mplus
				&& lchain->nodes[ii]->ltype == LchainFloating) return ii;
			else if (lchain->nodes[ii]->mplus == mplus)        break;
			}
		}

	/* Check for matching time for interpolated nodes */
	else if (ltype == LchainInterp)
		{
		for (ii=0; ii<lchain->inum; ii++)
			{
			if (lchain->interps[ii]->mplus == mplus) return ii;
			}
		}
	
	/* No matching time found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*      l c h a i n _ s t a r t _ n o d e                               *
*      l c h a i n _ e n d _ n o d e                                   *
*                                                                      *
*      Check if a node matches the link chain start or end time.       *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Check if a link node matches the link chain start time.
 *
 * @param[in]	lchain	link chain to check.
 * @param[in] 	ltype	type of node to check.
 * @param[in] 	inode	node to check.
 * @return	True if node time matches link chain start time.
 ***********************************************************************/

LOGICAL	lchain_start_node

	(
	LCHAIN	lchain,
	LMEMBER	ltype,
	int		inode
	)

	{
	int		ii, mplus;

	/* Return if no link chain or no nodes of the correct type */
	if (!lchain)   return FALSE;
	if (inode < 0) return FALSE;
	if (ltype == LchainNode)
		{
		if (lchain->lnum <= 0 || inode >= lchain->lnum) return FALSE;
		if (lchain->nodes[inode]->ltype != ltype)       return FALSE;
		}
	else if (ltype == LchainControl)
		{
		if (lchain->lnum <= 0 || inode >= lchain->lnum) return FALSE;
		if (lchain->nodes[inode]->ltype != ltype)       return FALSE;
		}
	else if (ltype == LchainFloating)
		{
		if (lchain->lnum <= 0 || inode >= lchain->lnum) return FALSE;
		if (lchain->nodes[inode]->ltype != ltype)       return FALSE;
		}
	else if (ltype == LchainInterp)
		{
		if (lchain->inum <= 0 || inode >= lchain->inum) return FALSE;
		}
	else                                                return FALSE;

	/* Set the node time */
	if      (ltype == LchainNode)     mplus = lchain->nodes[inode]->mplus;
	else if (ltype == LchainControl)  mplus = lchain->nodes[inode]->mplus;
	else if (ltype == LchainFloating) mplus = lchain->nodes[inode]->mplus;
	else if (ltype == LchainInterp)   mplus = lchain->interps[inode]->mplus;

	/* Return whether node time matches link chain start time */
	return (mplus == lchain->splus)? TRUE: FALSE;
	}

/**********************************************************************/

/***********************************************************************/
/**	Check if a link node matches the link chain end time.
 *
 * @param[in]	lchain	link chain to check.
 * @param[in] 	ltype	type of node to check.
 * @param[in] 	inode	node to check.
 * @return	True if node time matches link chain end time.
 ***********************************************************************/

LOGICAL	lchain_end_node

	(
	LCHAIN	lchain,
	LMEMBER	ltype,
	int		inode
	)

	{
	int		ii, mplus;

	/* Return if no link chain or no nodes of the correct type */
	if (!lchain)   return FALSE;
	if (inode < 0) return FALSE;
	if (ltype == LchainNode)
		{
		if (lchain->lnum <= 0 || inode >= lchain->lnum) return FALSE;
		if (lchain->nodes[inode]->ltype != ltype)       return FALSE;
		}
	else if (ltype == LchainControl)
		{
		if (lchain->lnum <= 0 || inode >= lchain->lnum) return FALSE;
		if (lchain->nodes[inode]->ltype != ltype)       return FALSE;
		}
	else if (ltype == LchainFloating)
		{
		if (lchain->lnum <= 0 || inode >= lchain->lnum) return FALSE;
		if (lchain->nodes[inode]->ltype != ltype)       return FALSE;
		}
	else if (ltype == LchainInterp)
		if (lchain->inum <= 0 || inode >= lchain->inum) return FALSE;
	else                                                return FALSE;

	/* Set the node time */
	if      (ltype == LchainNode)     mplus = lchain->nodes[inode]->mplus;
	else if (ltype == LchainControl)  mplus = lchain->nodes[inode]->mplus;
	else if (ltype == LchainFloating) mplus = lchain->nodes[inode]->mplus;
	else if (ltype == LchainInterp)   mplus = lchain->interps[inode]->mplus;

	/* Return whether node time matches link chain end time */
	return (mplus == lchain->eplus)? TRUE: FALSE;
	}

/***********************************************************************
*                                                                      *
*      i n i t _ n o d e _ m e m                                       *
*      c o p y _ n o d e _ m e m                                       *
*      f r e e _ n o d e _ m e m                                       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Initialize a link chain node member object.
 *
 *	@param[in] 	*mem	link chain node member to initialize
 *********************************************************************/
void	init_node_mem

	(
	NODEMEM	*mem
	)

	{
	if (IsNull(mem)) return;

	mem->name   = NULL;
	mem->type   = NodeNone;
	mem->attrib = NULL;

	copy_point(mem->offset, ZeroPoint);
	init_tspec(&mem->tspec);
	init_mspec(&mem->mspec);
	init_bspec(&mem->bspec);
	}

/**********************************************************************/

/*********************************************************************/
/** Copy the given link chain node member object.
 *
 *	@param[out]	*mem	copy
 *	@param[in]  	*lmem	original
 *********************************************************************/
void	copy_node_mem

	(
	NODEMEM			*mem,
	const NODEMEM	*lmem
	)

	{
	if (IsNull(mem)) return;

	mem->name   = STRMEM(mem->name,   lmem->name);
	mem->type   = lmem->type;
	mem->attrib = STRMEM(mem->attrib, lmem->attrib);

	copy_point(mem->offset, lmem->offset);
	copy_tspec(&mem->tspec, &lmem->tspec);
	copy_mspec(&mem->mspec, &lmem->mspec);
	copy_bspec(&mem->bspec, &lmem->bspec);
	}

/**********************************************************************/

/*********************************************************************/
/** Return the memory for the given link chain node member to the system.
 *
 *	@param[in] 	*mem	link chain node member to destroy
 *********************************************************************/
void	free_node_mem

	(
	NODEMEM	*mem
	)

	{
	if (IsNull(mem)) return;

	FREEMEM(mem->name);
	mem->type = NodeNone;
	FREEMEM(mem->attrib);

	free_tspec(&mem->tspec);
	free_mspec(&mem->mspec);
	free_bspec(&mem->bspec);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ n o d e _ m e m _ t y p e                         *
*      d e f i n e _ n o d e _ m e m _ a t t r i b                     *
*      d e f i n e _ n o d e _ m e m _ o f f s e t                     *
*      d e f i n e _ n o d e _ m e m _ o f f s e t _ x y               *
*                                                                      *
*      Define node member type, attributes, or offsets.                *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Define node member type.
 *
 *	@param[in] 	*mem	node member
 *	@param[in] 	name	member name to define
 *	@param[in] 	type	member type to define
 *********************************************************************/
void	define_node_mem_type

	(
	NODEMEM	*mem,
	STRING	name,
	NDMTYPE	type
	)

	{
	if (IsNull(mem)) return;

	mem->name = STRMEM(mem->name,  name);

	/* Make sure type is legal */
	switch (type)
		{
		case NodeText:
		case NodeMark:
		case NodeBarb:
			break;
		default:
			type = NodeNone;
		}
	mem->type = type;
	}

/**********************************************************************/

/*********************************************************************/
/** Define node member attribute
 *
 *	@param[in] 	*mem	node member
 *	@param[in] 	attrib	name of attribute
 *********************************************************************/
void	define_node_mem_attrib

	(
	NODEMEM	*mem,
	STRING	attrib
	)

	{
	if (IsNull(mem)) return;

	mem->attrib = STRMEM(mem->attrib, attrib);
	}

/**********************************************************************/


/*********************************************************************/
/** Define node member offset
 *
 *	@param[in] 	*mem	node member
 *	@param[in] 	offset	x-y offset of spot
 *********************************************************************/
void	define_node_mem_offset

	(
	NODEMEM	*mem,
	POINT	offset
	)

	{
	if (IsNull(mem)) return;

	if (offset) copy_point(mem->offset, offset);
	else        copy_point(mem->offset, ZeroPoint);
	}

/**********************************************************************/

/*********************************************************************/
/** Define node member offset as xy-pair.
 *
 *	@param[in] 	*mem	node member
 *	@param[in] 	xoff	x offset
 *	@param[in] 	yoff	y offset
 *********************************************************************/
void	define_node_mem_offset_xy

	(
	NODEMEM	*mem,
	float	xoff,
	float	yoff
	)

	{
	if (IsNull(mem)) return;

	set_point(mem->offset, xoff, yoff);
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ n o d e _ m e m _ p s p e c s                     *
*      r e c a l l _ n o d e _ m e m _ p s p e c s                     *
*      d e f i n e _ n o d e _ m e m _ p s p e c                       *
*      r e c a l l _ n o d e _ m e m _ p s p e c                       *
*                                                                      *
*      Set/reset or retrieve the presentation specs of the given       *
*      link chain node member.                                         *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set or reset link chain node member presentation specs.
 *
 * The node member makes of copy of the given spec objects for itself.
 *
 *	@param[in] 	*mem	link chain node member
 *	@param[in] 	*tspec	text specs
 *	@param[in] 	*mspec	mark display specs
 *	@param[in] 	*bspec	wind barb display specs
 *********************************************************************/
void	define_node_mem_pspecs

	(
	NODEMEM	*mem,
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
/** Retrieve link chain node member presentation specs.
 *
 *	@param[in] 	*mem		link chain node member
 *	@param[out]	**tspec		text specs
 *	@param[out]	**mspec		mark display specs
 *	@param[out]	**bspec		wind barb display specs
 *********************************************************************/
void	recall_node_mem_pspecs

	(
	NODEMEM	*mem,
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
 *	@param[in] 	*mem	link chain node member
 *	@param[in] 	param	parameter to set
 *	@param[in] 	value	value to set
 *********************************************************************/
void	define_node_mem_pspec

	(
	NODEMEM	*mem,
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
 *	@param[in] 	*mem	link chain node member
 *	@param[in] 	param	parameter to lookup
 *	@param[out]	value	value returned
 *********************************************************************/
void	recall_node_mem_pspec

	(
	NODEMEM	*mem,
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
*      n o d e _ m e m _ t y p e                                       *
*      n o d e _ m e m _ t y p e _ s t r i n g                         *
*                                                                      *
***********************************************************************/

static	const	ITBL	Tlist[] =
			{
				{ NodeNone, "none"   },
				{ NodeText, "text"   },
				{ NodeText, "string" },
				{ NodeText, "label"  },
				{ NodeMark, "mark"   },
				{ NodeMark, "marker" },
				{ NodeBarb, "barb"   },
				{ NodeBarb, "wind"   },
			};
static	const	int	NumTlist = ITBL_SIZE(Tlist);

/**********************************************************************/

/*********************************************************************/
/** Retrieve a link chain node type object by its name.
 *
 *	@param[in] 	tname	type name to lookup
 *  @return Pointer to link chain node type object.
 *********************************************************************/
NDMTYPE	node_mem_type

	(
	STRING	tname
	)

	{
	NDMTYPE	tcode;

	tcode = find_itbl_entry(Tlist, NumTlist, tname);
	if (tcode >= 0) return tcode;
	return NodeNone;
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve a link chain node type's name.
 *
 *	@param[in] 	tcode	type object to lookup
 *	@return link chain node type name
 *********************************************************************/
STRING	node_mem_type_string

	(
	NDMTYPE	tcode
	)

	{
	STRING	tname;

	tname = find_itbl_string(Tlist, NumTlist, tcode);
	if (NotNull(tname)) return tname;
	return find_itbl_string(Tlist, NumTlist, NodeNone);
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ l c h a i n                                 *
*      h i g h l i g h t _ l c h a i n _ n o d e s                     *
*      h i g h l i g h t _ l n o d e                                   *
*      h i g h l i g h t _ l i n t e r p                               *
*      w i d e n _ l c h a i n                                         *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Set the highlight flag for the given link chain.
 *
 * @note a negative value means to erase.
 *
 * @param[in]	lchain	link chain to highlight.
 * @param[in]	lcode	track highlight code.
 ***********************************************************************/
void	highlight_lchain

	(
	LCHAIN	lchain,
	HILITE	lcode
	)

	{

	/* Do nothing if no link chain */
	if (!lchain) return;

	if (lcode != SkipHilite)
		define_lspec_value(&lchain->lspec, LINE_HILITE, (POINTER)&lcode);
	}

/**********************************************************************/

/***********************************************************************/
/**	Set the highlight flag for the nodes of the given link chain.
 *
 * @note a negative value means to erase.
 *
 * @param[in]	lchain	link chain to highlight nodes.
 * @param[in]	tcode	label highlight code.
 * @param[in]	mcode	mark highlight code.
 ***********************************************************************/
void	highlight_lchain_nodes

	(
	LCHAIN	lchain,
	HILITE	tcode,
	HILITE	mcode
	)

	{
	int		ii;
	LNODE	lnode;
	LINTERP	linterp;

	/* Do nothing if no link chain */
	if (!lchain) return;

	/* Apply highlights to all link nodes */
	for (ii=0; ii<lchain->lnum; ii++)
		{
		lnode = lchain->nodes[ii];
		highlight_lnode(lnode, tcode, mcode);
		}

	/* Apply highlights to all interpolated nodes */
	for (ii=0; ii<lchain->inum; ii++)
		{
		linterp = lchain->interps[ii];
		highlight_linterp(linterp, tcode, mcode);
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Set the highlight flag for an individual link node.
 *
 * @note a negative value means to erase.
 *
 * @param[in]	lnode	link node to highlight.
 * @param[in]	tcode	label highlight code.
 * @param[in]	mcode	mark highlight code.
 ***********************************************************************/
void	highlight_lnode

	(
	LNODE	lnode,
	HILITE	tcode,
	HILITE	mcode
	)

	{
	int		imem;
	NODEMEM	*mem;

	/* Do nothing if no link node */
	if (!lnode) return;

	/* Apply highlights to the link node */
	for (imem=0; imem<lnode->nmem; imem++)
		{
		mem = lnode->members + imem;
		if (tcode != SkipHilite)
			define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&tcode);
		if (mcode != SkipHilite)
			define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&mcode);
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Set the highlight flag for an individual interpolated node.
 *
 * @note a negative value means to erase.
 *
 * @param[in]	linterp	interpolated node to highlight.
 * @param[in]	tcode	label highlight code.
 * @param[in]	mcode	mark highlight code.
 ***********************************************************************/
void	highlight_linterp

	(
	LINTERP	linterp,
	HILITE	tcode,
	HILITE	mcode
	)

	{
	int		imem;
	NODEMEM	*mem;

	/* Do nothing if no interpolated node */
	if (!linterp) return;

	/* Apply highlights to the interpolated node */
	for (imem=0; imem<linterp->nmem; imem++)
		{
		mem = linterp->members + imem;
		if (tcode != SkipHilite)
			define_tspec_value(&mem->tspec, TEXT_HILITE, (POINTER)&tcode);
		if (mcode != SkipHilite)
			define_mspec_value(&mem->mspec, MARK_HILITE, (POINTER)&mcode);
		}
	}

/**********************************************************************/

/***********************************************************************/
/**	Set the line width flag for the given link chain.
 *
 * Looks up the current line width, adds delta and resets the width.
 *
 * @param[in]	lchain	link chain to adjust.
 * @param[in]	delta	amount by which to change the width.
 ***********************************************************************/
void	widen_lchain

	(
	LCHAIN	lchain,
	float	delta
	)

	{
	float	width;

	/* Do nothing if no link chain */
	if (!lchain) return;

	/* Widen simple link chains by delta - patterned link chains by 1.25 */
	recall_lspec_value(&lchain->lspec, LINE_WIDTH, (POINTER)&width);
	if (!lchain->lspec.pattern) width += delta;
	else if (delta > 0.0)       width *= 1.25;
	else if (delta < 0.0)       width /= 1.25;
	define_lspec_value(&lchain->lspec, LINE_WIDTH, (POINTER)&width);
	}
