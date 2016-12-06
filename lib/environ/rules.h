/**********************************************************************/
/** @file rules.h
 *
 *  Routines to handle entry rules for field attributes (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r u l e s . h                                                      *
*                                                                      *
*   Routines to handle entry rules for field attributes (include file) *
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
#ifndef RULE_DEFS
#define RULE_DEFS


/* We need definitions for low level types and other Objects */
#include <tools/tools.h>
#include <objects/objects.h>
#include <fpa_types.h>

#include "cal.h"


/** Define entry rule function search list structure */
typedef struct
	{
	ERULE	function;	/**< Rule function */
	STRING  label;		/**< Rule label */
	} ERULE_TABLE;

/***********************************************************************
*                                                                      *
*  Initialize defined constants for ERULE handling routines            *
*                                                                      *
***********************************************************************/

#ifdef RULE_INIT

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in rules.c and user_rules.c              *
*                                                                      *
***********************************************************************/

ERULE	identify_rule_function(STRING rname);
ERULE	identify_user_rule_function(STRING rname);
void	display_rule_functions(void);
void	display_user_rule_functions(void);
void	CAL_invoke_rules(CAL cal, int nrules, ERULE *rules);
void	CAL_invoke_python_rules(CAL cal, int nrules, STRING *rules);
void	CAL_invoke_entry_rules_by_name(CAL cal, STRING elem, STRING level);
void	CAL_invoke_label_rules_by_name(CAL cal, STRING elem, STRING level);
void	CAL_invoke_lnode_rules_by_name(CAL cal, STRING elem, STRING level);
void	CAL_invoke_all_lchain_lnode_rules(LCHAIN chain,
												STRING elem, STRING level);

/* Now it has been included */
#endif
