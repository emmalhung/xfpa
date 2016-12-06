/************************************************************************/
/** @file depict_fields.c
 *
 * Routines to access the depictions setup block and provide
 * information about what fields make up the depiction.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   d e p i c t _ f i e l d s . c                                      *
*                                                                      *
*   Routines to access the "depictions" setup block and provide        *
*   information about what fields make up the depiction.               *
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

#define DEPICT_FIELDS_INIT

#include "read_setup.h"
#include "config_structs.h"
#include "config_info.h"
#include "depict_fields.h"

#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>

#include <stdio.h>

/* Internal static function to read "depiction" setup block */
static	LOGICAL		read_depiction_setup(void);

/* Global variables to hold depiction field information */
static	LOGICAL					Dready   = FALSE;
static	int						Nfield   = 0;
static	FpaConfigFieldStruct	**Fields = NullPtr(FpaConfigFieldStruct **);

/***********************************************************************
*                                                                      *
*   d e p i c t _ f i e l d _ l i s t                                  *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return a list of field identifiers
 *
 *	@param[out]	***fids	list of field identifiers
 *  @return The size of the list.
 **********************************************************************/
int					depict_field_list

	(
	FpaConfigFieldStruct	***fids
	)

	{
	/* Read the setup if necessary */
	if (fids) *fids = NullPtr(FpaConfigFieldStruct **);
	if ( !read_depiction_setup() ) return 0;

	/* Return information about the depiction */
	if (fids) *fids = Fields;
	return Nfield;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES:                                         *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ d e p i c t i o n _ s e t u p                            *
*                                                                      *
***********************************************************************/

static	LOGICAL		read_depiction_setup

	(
	)

	{
	STRING					line, cmd;
	char					elem[50], level[50];
	FpaConfigFieldStruct	*fid;
	int						ifield;
	LOGICAL					status;

	/* Have we already read the setup? */
	if (Dready) return TRUE;

	/* Fool the software into reading the fields config first */
	(void) identify_element("unknown");

	/* Read the "depictions" setup block */
	if ( !find_setup_block("depiction", TRUE) ) return FALSE;
	while( line = setup_block_line() )
		{
		cmd = string_arg(line);

		if (same(cmd, "field"))
			{
			/* Read element and level */
			strcpy_arg(elem, line, &status);	if (!status) continue;
			strcpy_arg(level, line, &status);	if (!status) continue;

			/* See if the field is recognized */
			fid = get_field_info(elem, level);
			if (!fid)
				{
				/* Unrecognized field */
				pr_error("Fields",
					"Unrecognized field: \"%s\" \"%s\" in depiction block.\n",
					elem, level);
				continue;
				}

			/* Ignore unrecognized fields */
			switch (fid->element->fld_type)
				{
				case FpaC_VECTOR:
				case FpaC_CONTINUOUS:
				case FpaC_DISCRETE:
				case FpaC_WIND:
				case FpaC_LINE:
				case FpaC_SCATTERED:
				case FpaC_LCHAIN:
					break;

				default:
					pr_error("Fields",
						"Cannot add field: \"%s\" \"%s\" in depiction block.\n",
						elem, level);
					pr_error("Fields", " Unrecognized field type!\n");
					continue;
				}
			if (blank(fid->element->elem_io->fident)
					|| blank(fid->level->lev_io->fident))
				{
				pr_error("Fields",
					"Cannot add field: \"%s\" \"%s\" in depiction block.\n",
					elem, level);
				pr_error("Fields", " Missing element or level \"file_ident\"!\n");
				continue;
				}

			/* See if we already have this field */
			for (ifield=0; ifield<Nfield; ifield++)
				{
				if (fid == Fields[ifield]) break;
				}
			if (ifield < Nfield)
				{
				/* Already have this field */
				continue;
				}

			/* Add the field */
			(Nfield)++;
			Fields = GETMEM(Fields, FpaConfigFieldStruct *, Nfield);
			Fields[Nfield-1] = fid;
			}

		else
			{
			/* Unrecognized line */
			}
		}

	Dready = TRUE;
	return TRUE;
	}
