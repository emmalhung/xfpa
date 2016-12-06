/**********************************************************************/
/** @file lchain.h
 *
 *  LCHAIN object definitions (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    l c h a i n . h                                                   *
*                                                                      *
*    LCHAIN object definitions (include file)                          *
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

/* See if already included */
#ifndef LCHAIN_DEFS
#define LCHAIN_DEFS

/* We need definitions for other objects */
#include "line.h"
#include "pspec.h"
#include "attrib.h"

/* Define node class MACROS used for display */
#define FpaNodeClass_Unknown			"unknown"
#define FpaNodeClass_Normal				"normal"
#define FpaNodeClass_NormalGuess		"normal-guess"
#define FpaNodeClass_Control			"control"
#define FpaNodeClass_ControlGuess		"control-guess"
#define FpaNodeClass_Floating			"floating"
#define FpaNodeClass_Interp				"interp"
#define FpaNodeClass_NormalLabel		"Normal Node"
#define FpaNodeClass_NormalGuessLabel	"Guess Node"
#define FpaNodeClass_ControlLabel		"Control Node"
#define FpaNodeClass_ControlGuessLabel	"Guess Control Node"
#define FpaNodeClass_FloatingLabel		"Floating Node"
#define FpaNodeClass_InterpLabel		"Interpolated Node"

/* This is an earlier obsolete format! */
#define FpaNodeClass_Guess				"guess"

/** Define link chain node identifiers */
typedef	enum
	{
	LchainUnknown,	/**< ignore */
	LchainNode,		/**< normal link node (track/attributes) */
	LchainControl,	/**< control node for intermediate times (track) */
	LchainFloating,	/**< floating node for intermediate times (attributes) */
	LchainInterp	/**< interpolated node */
	} LMEMBER;

/** Define NDMTYPE - LCHAIN node member type */
typedef	enum
	{
	NodeNone,	/**< ignore */
	NodeText,	/**< string label */
	NodeMark,	/**< marker */
	NodeBarb	/**< wind or velocity barb */
	} NDMTYPE;

/** Define NDMEM object - node member of an LCHAIN object */
typedef	struct NODEMEM_struct
	{
	STRING	name;		/**< member name */
	NDMTYPE	type;		/**< type - text, mark, etc. */
	STRING	attrib;		/**< name of attribute to use */
	POINT	offset;		/**< offset from node */
	TSPEC	tspec;		/**< how to display text */
	MSPEC	mspec;		/**< how to display mark */
	BSPEC	bspec;		/**< how to display barb */
	} NODEMEM;

/* Define LNODE object for LCHAIN */
/** a structure to store information for individual link nodes */
typedef	struct	LNODE_struct
	{
	LOGICAL		there;		/**< is this link node in use? */
	LOGICAL		guess;		/**< is this link node a guess? */
	LMEMBER		ltype;		/**< link node type */
	int			mplus;		/**< link node time (minutes from T0) */
	POINT		node;		/**< link node location */
	ATTRIB_LIST	attrib;		/**< list of attributes */
	NODEMEM		*members;	/**< list of members to display nodes */
	int			nmem;		/**< number of node members */
	int			attach;		/**< index to item that link node is attached to */
	int			mtype;		/**< type of item member for link node       */
							/**<  ... boundary, hole, divide, label, etc */
	int			imem;		/**< index to type of item member for link node */
	} *LNODE;

/* Define LINTERP structure to store interpolated link positions */
typedef	struct	LINTERP_struct
	{
	LOGICAL		there;		/**< is this interpolated node in use? */
	LOGICAL		depict;		/**< is this interpolated node a depiction? */
	int			mplus;		/**< interpolation time (minutes from T0) */
	POINT		node;		/**< interpolated node location */
	ATTRIB_LIST	attrib;		/**< list of attributes */
	NODEMEM		*members;	/**< list of members to display nodes */
	int			nmem;		/**< number of node members */
	} *LINTERP;

/* Define LCHAIN structure to store time link chains */
typedef	struct	LCHAIN_struct
	{
	STRING		xtime;		/**< T0 reference time for link chain */
	ATTRIB_LIST	attrib;		/**< list of attributes */
	int			splus;		/**< start time for link chain (minutes from T0) */
	int			eplus;		/**< end time for link chain (minutes from T0) */
	int			lnum;		/**< link node count */
	LNODE		*nodes;		/**< set of link nodes */
	LOGICAL		dointerp;	/**< does the link chain need to be interpolated? */
	int			minterp;	/**< interpolation delta for link track (minutes) */
	int			inum;		/**< interpolated node count */
	LINTERP		*interps;	/**< interpolated node positions */
	LINE		track;		/**< link chain track (from interpolated nodes) */
	LSPEC		lspec;		/**< how to display track */
	} *LCHAIN;

/* Convenient definitions */
#define NullLchain           NullPtr(LCHAIN)
#define NullLchainPtr        NullPtr(LCHAIN *)
#define NullLchainList       NullPtr(LCHAIN *)
#define NullLchainListPtr    NullPtr(LCHAIN **)

#define NullLnode            NullPtr(LNODE)
#define NullLnodePtr         NullPtr(LNODE *)
#define NullLnodeList        NullPtr(LNODE *)
#define NullLnodeListPtr     NullPtr(LNODE **)

#define NullLinterp          NullPtr(LINTERP)
#define NullLinterpPtr       NullPtr(LINTERP *)
#define NullLinterpList      NullPtr(LINTERP *)
#define NullLinterpListPtr   NullPtr(LINTERP **)

#define NullNodeMem          NullPtr(NODEMEM)
#define NullNodeMemPtr       NullPtr(NODEMEM *)
#define NullNodeMemList      NullPtr(NODEMEM *)
#define NullNodeMemListPtr   NullPtr(NODEMEM **)


/* Declare all functions in lchain.c */
LCHAIN	create_lchain(void);
LCHAIN	destroy_lchain(LCHAIN lchain);
void	empty_lchain(LCHAIN lchain);
LCHAIN	copy_lchain(const LCHAIN lchain);
LOGICAL	valid_lchain(const LCHAIN lchain, int *lnum, int *inum);
void	define_lchain_reference_time(LCHAIN lchain, STRING xtime);
void	define_lchain_start_time(LCHAIN lchain, int splus);
void	define_lchain_end_time(LCHAIN lchain, int eplus);
void	define_lchain_attribs(LCHAIN lchain, ATTRIB_LIST attrib);
void	define_lchain_default_attribs(LCHAIN lchain);
void	recall_lchain_attribs(LCHAIN lchain, ATTRIB_LIST *attrib);
void	define_lchain_interp_delta(LCHAIN lchain, int minterp);

LNODE	create_lnode(int mplus);
LNODE	destroy_lnode(LNODE lnode);
void	empty_lnode(LNODE lnode);
LNODE	copy_lnode(LNODE lnode);
void	define_lnode_type(LNODE lnode, LOGICAL there, LOGICAL guess,
							LMEMBER ltype);
void	define_lnode_node(LNODE lnode, POINT node);
void	define_lnode_attribs(LNODE lnode, ATTRIB_LIST attrib);
void	define_lnode_default_attribs(LNODE lnode);
void	recall_lnode_attribs(LNODE lnode, ATTRIB_LIST *attrib);
void	define_lnode_attach(LNODE lnode, int attach, int mtype, int imem);
void	add_lnode_member(LNODE lnode, STRING name, NDMTYPE type);
void	remove_lnode_member(LNODE lnode, STRING name);
int		which_lnode_member(LNODE lnode, STRING name, NODEMEM **mem);
void	build_lnode_members(LNODE lnode, int ncspec, const CATSPEC *cspecs);
int		add_lchain_lnode(LCHAIN lchain, LNODE lnode);
LOGICAL	remove_lchain_lnode(LCHAIN lchain, int inode);

LINTERP	create_linterp(int mplus);
LINTERP	destroy_linterp(LINTERP linterp);
void	empty_linterp(LINTERP linterp);
LINTERP	copy_linterp(LINTERP linterp);
void	define_linterp_node(LINTERP linterp, LOGICAL there, LOGICAL depict,
							POINT node);
void	define_linterp_attribs(LINTERP linterp, ATTRIB_LIST attrib);
void	define_linterp_default_attribs(LINTERP linterp);
void	recall_linterp_attribs(LINTERP linterp, ATTRIB_LIST *attrib);
void	add_linterp_member(LINTERP linterp, STRING name, NDMTYPE type);
void	remove_linterp_member(LINTERP linterp, STRING name);
int		which_linterp_member(LINTERP linterp, STRING name, NODEMEM **mem);
void	build_linterp_members(LINTERP linterp, int ncspec,
							const CATSPEC *cspecs);

int		which_lchain_node(LCHAIN lchain, LMEMBER ltype, int mplus);
LOGICAL	lchain_start_node(LCHAIN lchain, LMEMBER ltype, int inode);
LOGICAL	lchain_end_node(LCHAIN lchain, LMEMBER ltype, int inode);

void	init_node_mem(NODEMEM *mem);
void	copy_node_mem(NODEMEM *mcopy, const NODEMEM *mem);
void	free_node_mem(NODEMEM *mem);
void	define_node_mem_type(NODEMEM *mem, STRING name, NDMTYPE type);
void	define_node_mem_attrib(NODEMEM *mem, STRING attrib);
void	define_node_mem_offset(NODEMEM *mem, POINT offset);
void	define_node_mem_offset_xy(NODEMEM *mem, float xoff, float yoff);
void	define_node_mem_pspecs(NODEMEM *mem,
						TSPEC *tspec, MSPEC *mspec, BSPEC *bspec);
void	recall_node_mem_pspecs(NODEMEM *mem,
						TSPEC **tspec, MSPEC **mspec, BSPEC **bspec);
void	define_node_mem_pspec(NODEMEM *mem, PPARAM param, POINTER value);
void	recall_node_mem_pspec(NODEMEM *mem, PPARAM param, POINTER value);
NDMTYPE	node_mem_type(STRING tname);
STRING	node_mem_type_string(NDMTYPE type);

void	highlight_lchain(LCHAIN lchain, HILITE lcode);
void	highlight_lchain_nodes(LCHAIN lchain, HILITE tcode, HILITE mcode);
void	highlight_lnode(LNODE lnode, HILITE tcode, HILITE mcode);
void	highlight_linterp(LINTERP linterp, HILITE tcode, HILITE mcode);
void	widen_lchain(LCHAIN lchain, float delta);

/* Declare all functions in lchain_oper.c */
void	lchain_properties(LCHAIN lchain, float *length);
void	lchain_test_point(LCHAIN lchain, POINT ptest, float *pdist,
							POINT ppoint, int *pseg, LOGICAL *right);
LOGICAL	inbox_lchain(LCHAIN lchain, const BOX *box);
LOGICAL	translate_lchain(LCHAIN lchain, float dx, float dy);
LOGICAL	rotate_lchain(LCHAIN lchain, POINT ref, float angle);
LOGICAL	interpolate_lchain(LCHAIN lchain);
int		lchain_closest_node(LCHAIN lchain, float dtol, float atol,
							POINT ptest, float *pdist, POINT ppos,
							LMEMBER *ltype, int *inum, int **inodes);
void	promote_lchain_node(LCHAIN lchain, LMEMBER ltype, int mplus);

int		nearest_lchain_lnode(LCHAIN lchain, float dtol, POINT ptest,
							float *pdist, POINT ppos, int *lnum, int **lnodes);
int		nearest_lchain_linterp(LCHAIN lchain, float dtol, POINT ptest,
							float *pdist, POINT ppos, int *inum, int **inodes);

/* Declare all functions in lchain_prep.c */

/* Functions in lchain_set.c are declared in set_oper.h */

/* Now it has been included */
#endif
