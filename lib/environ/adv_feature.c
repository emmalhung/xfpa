/**********************************************************************/
/** @file adv_feature.c
 *
 * Routines to interpret the "advanced_features" setup block
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   a d v _ f e a t u r e . c                                          *
*                                                                      *
*   Routines to interpret the "advanced_features" setup block          *
*                                                                      *
*     Version 5 (c) Copyright 1999 Environment Canada (AES)            *
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

#define ADV_INIT		/* To initialize declarations in adv_feature.h */

#include "adv_feature.h"
#include "read_setup.h"

#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

/* Internal static function to read "advanced_features" setup block */
static	void	read_adv_setup(void);

/* Global variables to hold feature information */
static	LOGICAL	AdvBlockReady = FALSE;		/* Has it been read yet */

/***********************************************************************
*                                                                      *
*   a d v _ f e a t u r e                                              *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Search for feature in the advanced feature block of the setup file.
 *
 *  @param[in]	feature	the feature to look up.
 *  @return a string containing the feature mode.
 ***********************************************************************/
STRING	adv_feature

	(
	STRING	feature
	)

	{
	if (!AdvBlockReady) read_adv_setup();

	/* Now search for the requested feature */
	return get_feature_mode(feature);
	}

/***********************************************************************
*                                                                      *
*   r e a d _ a d v _ s e t u p                                        *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/** Read the advanced setup block and store it for future use.
 ***********************************************************************/
static	void	read_adv_setup(void)

	{
	STRING	key, line;
	char	fname[50], fmode[50];
	LOGICAL	OK;

	/* Ready even if not found */
	AdvBlockReady = TRUE;

	if ( !find_setup_block("advanced_features", FALSE) ) return;

	/* Interpret the block */
	while ( line = setup_block_line() )
		{
		/* Read the keyword */
		key = string_arg(line);

		/* Only the "feature" keyword is recognized */
		if (same(key, "feature"))
			{
			/* Read the feature name */
			(void) strcpy_arg(fname, line, &OK);
			if (!OK) continue;

			/* Read the feature mode */
			(void) strcpy_arg(fmode, line, &OK);
			if (!OK) (void) strcpy(fmode, "");

			pr_diag("Advanced.Features", "Feature: %s %s\n", fname, fmode);
			set_feature_mode(fname, fmode);
			}
		}
	}
