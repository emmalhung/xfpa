/**********************************************************************/
/** @file diag.c
 *
 * Routines to interpret the diag_control setup block.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   d i a g . c                                                        *
*                                                                      *
*   Routines to interpret the "diag_control" setup block               *
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

#define DIAG_INIT		/* To initialize declarations in diag.h */

#include "diag.h"
#include "read_setup.h"

#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <string.h>

/* Internal static function to read "diag" setup block */
static	void	read_diag_setup(int, int);

/* Global variables to hold diag information */
static	LOGICAL	DiagBlockReady = FALSE;		/* Has it been read yet */
static	int		DefaultLevel   = -1;
static	int		ModuleStyle    = -1;

/***********************************************************************
*                                                                      *
*   d i a g _ c o n t r o l                                            *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Setup specific module control from the setup file.
 *
 *	@param[in]	enable		Diag On or Off
 *	@param[in]	dlevel		Diag Level
 *	@param[in]	dstyle		Diag print Style
 **********************************************************************/
void	diag_control

	(
	LOGICAL	enable,
	int		dlevel,
	int		dstyle
	)

	{
	/* Set the default modes first */
	if (dlevel >= 0) DefaultLevel = dlevel;
	if (dstyle >= 0) ModuleStyle  = dstyle;
	pr_control(NullString, DefaultLevel, ModuleStyle);
	pr_info("Diag.Control", "Output: %d Style: %d\n",
			DefaultLevel, ModuleStyle);

	/* Now get specific module control from the setup file */
	if (enable) read_diag_setup(dlevel, dstyle);
	}

/***********************************************************************
*                                                                      *
*   r e a d _ d i a g _ s e t u p                                      *
*                                                                      *
***********************************************************************/

static	void	read_diag_setup

	(
	int		dlevel,
	int		dstyle
	)

	{
	int			level, mstyle;
	STRING		key, line, arg;
	char		module[50];

	/* Ready even if not found */
	DiagBlockReady = TRUE;

	if ( !find_setup_block("diag_control", FALSE) ) return;

	/* Interpret the diag block */
	while ( line = setup_block_line() )
		{
		/* Read the keyword */
		key = string_arg(line);

		/* If there is a setup default, only use it if it hasn't yet */
		/* been set */
		if (same(key, "default"))
			{
			/* Read the default diagnostic level */
			arg  = string_arg(line);		if (blank(arg)) continue;
			if (same(arg, "*"))
				level = DefaultLevel;
			else
				{
				if (sscanf(arg, "%d", &level) < 1)          continue;
				if (DefaultLevel < 0 || dlevel < 0) DefaultLevel = level;
				}

			/* Read the module name style */
			arg  = string_arg(line);		if (blank(arg)) continue;
			if (same(arg, "*"))
				mstyle = ModuleStyle;
			else
				{
				if (sscanf(arg, "%d", &mstyle) < 1)         continue;
				if (ModuleStyle < 0 || dstyle < 0) ModuleStyle = mstyle;
				}

			pr_control(NullString, DefaultLevel, ModuleStyle);
			pr_info("Diag.Control", "Output: %d Style: %d\n",
					DefaultLevel, ModuleStyle);
			}

		/* Set individual module modes */
		else if (same(key, "module"))
			{
			/* Read the module name */
			arg  = string_arg(line);		if (blank(arg)) continue;
			(void) strcpy(module, arg);

			/* Read the diag level */
			arg  = string_arg(line);		if (blank(arg)) continue;
			if (same(arg, "*"))
				level = DefaultLevel;
			else
				{
				if (sscanf(arg, "%d", &level) < 1)          continue;
				}

			pr_control(module, level, mstyle);
			pr_diag("Diag.Control", "Module: %s Output: %d Style: %d\n",
					module, level, mstyle);
			}
		}
	}
