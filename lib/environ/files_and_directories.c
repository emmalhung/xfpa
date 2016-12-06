/**********************************************************************/
/** @file files_and_directories.c
 *
 * Routines for constructing file names and directory paths.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   f i l e s _ a n d _ d i r e c t o r i e s . c                      *
*                                                                      *
*   Routines for constructing file names and directory paths           *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
*     Version 8 (c) Copyright 2009 Environment Canada                  *
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

#define FILES_AND_DIRS_INIT		/* To initialize declarations in */
								/*  files_and_directories.h      */

#include "read_setup.h"
#include "config_structs.h"
#include "config_info.h"
#include "target_map.h"
#include "meta.h"
#include "files_and_directories.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

#undef DEBUG_FILE_IDENTS

/* Interface functions */
/*  ... these are defined in fields_and_directories.h */

/* Internal static functions (Files and Filenames) */
static STRING	data_file_path(STRING, STRING);
static STRING	map_file_path(STRING, FpaConfigElementStruct *,
													FpaConfigLevelStruct *);
static STRING	depiction_file(FpaConfigSourceStruct *, STRING);

/* Internal static functions (Directories) */
static void		shuffle_directory(void);
static int		data_directory_run_times(FLD_DESCRIPT *, STRING **);
static int		data_directory_valid_times(FLD_DESCRIPT *, int, STRING **);
static int		data_directory_fields(FLD_DESCRIPT *, int,
													FpaConfigFieldStruct ***);

/* Internal static functions (File Identifier Sorting and Matching) */
static int		strecmp(const void *, const void *);
static int		strlcmp(const void *, const void *);
static int		strvcmp(const void *, const void *);
static STRING	set_field_search(LOGICAL, FLD_DESCRIPT *);

/* Units cross-referenced in configuration file */
static	const	STRING	Hrs = "hr";

/***********************************************************************
*                                                                      *
*   i n i t _ f l d _ d e s c r i p t                                  *
*   s e t _ f l d _ d e s c r i p t                                    *
*   c o p y _ f l d _ d e s c r i p t                                  *
*   s a m e _ f l d _ d e s c r i p t                                  *
*   s a m e _ f l d _ d e s c r i p t _ n o _ m a p                    *
*                                                                      *
*   Routines to handle FLD_DESCRIPT Objects which contain information  *
*   on given fields accessed in the FPA libraries.                     *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Initializes a Field Descriptor Structure
 *
 *	@param[in]	*fdesc		field descriptor
 **********************************************************************/
void				init_fld_descript

	(
	FLD_DESCRIPT	*fdesc
	)

	{

	/* Return now if no Object to initialize */
	if ( IsNull(fdesc) ) return;

	/* Initialize field descriptor with default values */
	(void) copy_fld_descript(fdesc, &FpaNoFDesc);

	/* Set the default map projection from the current target map */
	(void) set_fld_descript(fdesc, FpaF_MAP_PROJECTION, get_target_map(),
								FpaF_END_OF_LIST);
	}

/**********************************************************************/
/** Sets the values of a Field Descriptor Structure.
 * Variable length input consists of type, value pairs concluding with
 * a FpaF_END_OF_LIST.
 * Values that can be set are:
 * - FpaF_MAP_PROJECTION
 * - FpaF_DIRECTORY_PATH
 * - FpaF_SOURCE
 * - FpaF_SOURCE_NAME
 * - FpaF_SUBSOURCE
 * - FpaF_SUBSOURCE_NAME
 * - FpaF_RUN_TIME
 * - FpaF_VALID_TIME
 * - FpaF_ELEMENT
 * - FpaF_ELEMENT_NAME
 * - FpaF_LEVEL
 * - FpaF_LEVEL_NAME
 * - FpaF_FIELD_DATA_TYPE
 * - FpaF_FIELD_MACRO
 * - FpaF_WIND_FUNCTION_NAME
 * - FpaF_VALUE_FUNCTION_NAME
 * - FpaF_WIND_CALCULATION
 * - FpaF_WIND_CALCULATION_NAME
 * - FpaF_VALUE_CALCULATION
 * - FpaF_VALUE_CALCULATION_NAME
 * - FpaF_CALCULATION_TYPE_NAME
 *
 *	@param[in]	*fdesc	field descriptor
 *	@param[in]  ...     List of type, value pairs
 * 	@return True if successful.
 **********************************************************************/
LOGICAL				set_fld_descript

	(
	FLD_DESCRIPT	*fdesc,
	...
	)

	{
	va_list						args;
	int							key;
	STRING						name;
	int							year, jday, hour, min, macro;
	LOGICAL						local;
	size_t						nlen;
	MAP_PROJ					*mproj;
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceSubStruct	*subdef;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;
	FpaConfigFieldStruct		*fdef;
	LOGICAL						valid;

	/* Return FALSE if no Object to set */
	if ( IsNull(fdesc) ) return FALSE;

	/* Initialize pointer to field structure */
	fdesc->fdef = NullPtr(FpaConfigFieldStruct *);

	/* Set error checking parameter */
	valid = TRUE;

	/* Set args to point to first argument in list */
	va_start(args, fdesc);

	/* Set parameters in field descriptor from input list          */
	/* Note that special FpaF_END_OF_LIST key is will return key=0 */
	/*  and will therefore cause an exit of the while loop!        */
	while ( key = va_arg(args, int) )
		{
		/* Branch to set parameter based on key */
		switch ( key )
			{

			/* Set map projection */
			case FpaF_MAP_PROJECTION:
				mproj = va_arg(args, MAP_PROJ *);
				if ( NotNull(mproj) )
					{
					(void) copy_map_projection(&fdesc->mproj, mproj);
					}
				else
					{
					(void) copy_map_projection(&fdesc->mproj, &NoMapProj);
					}
				break;

			/* Set directory path */
			case FpaF_DIRECTORY_PATH:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					(void) strcpy(fdesc->path, name);
					}
				else
					{
					(void) strcpy(fdesc->path, FpaCblank);
					}
				break;

			/* Set source */
			case FpaF_SOURCE:
				sdef = va_arg(args, FpaConfigSourceStruct *);
				if ( NotNull(sdef) )
					{
					fdesc->sdef   = sdef;
					fdesc->subdef = sdef->src_sub;
					}
				else
					{
					fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
					fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
					}
				break;

			/* Set source by name */
			case FpaF_SOURCE_NAME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					sdef = identify_source(name, FpaCblank);
					if ( NotNull(sdef) )
						{
						fdesc->sdef   = sdef;
						fdesc->subdef = sdef->src_sub;
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Unknown source: \"%s\"!\n",
									SafeStr(name));
						fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
						fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
						valid = FALSE;
						}
					}
				else
					{
					fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
					fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
					}
				break;

			/* Set sub source */
			case FpaF_SUBSOURCE:
				subdef = va_arg(args, FpaConfigSourceSubStruct *);
				if ( NotNull(subdef) )
					{
					if ( NotNull(fdesc->sdef) )
						{
						sdef = identify_source(fdesc->sdef->name, subdef->name);
						if ( NotNull(sdef) )
							{
							fdesc->sdef   = sdef;
							fdesc->subdef = sdef->src_sub;
							}
						else
							{
							(void) pr_error("Environ",
								"[set_fld_descript] Unknown source: \"%s\" \"%s\"!\n",
										SafeStr(fdesc->sdef->name),
										SafeStr(subdef->name));
							fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
							fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
							valid = FALSE;
							}
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] No source for subsource: \"%s\"!\n",
									SafeStr(subdef->name));
						fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
						fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
						valid = FALSE;
						}
					}
				else
					{
					(void) pr_error("Environ",
						"[set_fld_descript] Unrecognizable subsource pointer!\n");
					fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
					fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
					valid = FALSE;
					}
				break;

			/* Set sub source by name */
			case FpaF_SUBSOURCE_NAME:
				name = va_arg(args, STRING);
				if ( NotNull(fdesc->sdef) )
					{
					sdef = identify_source(fdesc->sdef->name, name);
					if ( NotNull(sdef) )
						{
						fdesc->sdef   = sdef;
						fdesc->subdef = sdef->src_sub;
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Unknown source: \"%s\" \"%s\"!\n",
									SafeStr(fdesc->sdef->name),
									SafeStr(name));
						fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
						fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
						valid = FALSE;
						}
					}
				else
					{
					(void) pr_error("Environ",
						"[set_fld_descript] No source for subsource: \"%s\"!\n",
								SafeStr(name));
					fdesc->sdef   = NullPtr(FpaConfigSourceStruct *);
					fdesc->subdef = NullPtr(FpaConfigSourceSubStruct *);
					valid = FALSE;
					}
				break;

			/* Set run timestamp */
			case FpaF_RUN_TIME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					if ( parse_tstamp(name, &year, &jday, &hour, &min, &local,
							NullLogicalPtr) )
						{
						(void) strcpy(fdesc->rtime, name);
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Error in run timestamp: \"%s\"!\n",
									SafeStr(name));
						(void) strcpy(fdesc->rtime, FpaCblank);
						valid = FALSE;
						}
					}
				else
					{
					(void) strcpy(fdesc->rtime, FpaCblank);
					}
				break;

			/* Set valid timestamp */
			case FpaF_VALID_TIME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					if ( parse_tstamp(name, &year, &jday, &hour, &min, &local,
							NullLogicalPtr) )
						{
						(void) strcpy(fdesc->vtime, name);
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Error in valid timestamp: \"%s\"!\n",
									SafeStr(name));
						(void) strcpy(fdesc->vtime, FpaCblank);
						valid = FALSE;
						}
					}
				else
					{
					(void) strcpy(fdesc->vtime, FpaCblank);
					}
				break;

			/* Set element */
			case FpaF_ELEMENT:
				edef = va_arg(args, FpaConfigElementStruct *);
				if ( NotNull(edef) )
					{
					fdesc->edef = edef;
					}
				else
					{
					fdesc->edef = NullPtr(FpaConfigElementStruct *);
					}
				break;

			/* Set element by name */
			case FpaF_ELEMENT_NAME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					edef = identify_element(name);
					if ( NotNull(edef) )
						{
						fdesc->edef = edef;
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Unknown element: \"%s\"!\n",
									SafeStr(name));
						fdesc->edef = NullPtr(FpaConfigElementStruct *);
						valid = FALSE;
						}
					}
				else
					{
					fdesc->edef = NullPtr(FpaConfigElementStruct *);
					}
				break;

			/* Set level */
			case FpaF_LEVEL:
				ldef = va_arg(args, FpaConfigLevelStruct *);
				if ( NotNull(ldef) )
					{
					fdesc->ldef = ldef;
					}
				else
					{
					fdesc->ldef = NullPtr(FpaConfigLevelStruct *);
					}
				break;

			/* Set level by name */
			case FpaF_LEVEL_NAME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					ldef = identify_level(name);
					if ( NotNull(ldef) )
						{
						fdesc->ldef = ldef;
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Unknown level: \"%s\"!\n",
									SafeStr(name));
						fdesc->ldef = NullPtr(FpaConfigLevelStruct *);
						valid = FALSE;
						}
					}
				else
					{
					fdesc->ldef = NullPtr(FpaConfigLevelStruct *);
					}
				break;

			/* Set field type by name */
			case FpaF_FIELD_DATA_TYPE:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					macro = field_data_type(name);
					if ( macro != FpaCnoMacro )
						{
						fdesc->fmacro = macro;
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] Unknown field type: \"%s\"!\n",
									SafeStr(name));
						fdesc->fmacro = FpaCnoMacro;
						valid = FALSE;
						}
					}
				else
					{
					fdesc->fmacro = FpaCnoMacro;
					}
				break;

			/* Set field type by macro */
			case FpaF_FIELD_MACRO:
				macro = va_arg(args, int);
				if ( check_field_macro(macro) )
					{
					fdesc->fmacro = macro;
					}
				else
					{
					(void) pr_error("Environ",
						"[set_fld_descript] Unknown field macro: \"%d\"!\n",
								macro);
					fdesc->fmacro = FpaCnoMacro;
					valid = FALSE;
					}
				break;

			/* Set wind function by name */
			case FpaF_WIND_FUNCTION_NAME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					nlen = strlen(name);
					if ( (int) nlen < MAX_FCHRS )
						{
						(void) strcpy(fdesc->wind_func_name, name);
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] More than %d characters\n",
									MAX_FCHRS);
						(void) pr_error("Environ",
							"[set_fld_descript]  for \"wind_function = %s\"!\n",
									name);
						(void) strcpy(fdesc->wind_func_name, FpaCblank);
						valid = FALSE;
						}
					}
				else
					{
					(void) strcpy(fdesc->wind_func_name, FpaCblank);
					}
				break;

			/* Set value function by name */
			case FpaF_VALUE_FUNCTION_NAME:
				name = va_arg(args, STRING);
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					nlen = strlen(name);
					if ( (int) nlen < MAX_FCHRS )
						{
						(void) strcpy(fdesc->value_func_name, name);
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] More than %d characters\n",
									MAX_FCHRS);
						(void) pr_error("Environ",
							"[set_fld_descript]  for \"value_function = %s\"!\n",
									name);
						(void) strcpy(fdesc->value_func_name, FpaCblank);
						valid = FALSE;
						}
					}
				else
					{
					(void) strcpy(fdesc->value_func_name, FpaCblank);
					}
				break;

		/* >>> the following is obsolete in next version <<< */
			/* Set wind calculation type */
			case FpaF_WIND_CALCULATION:
				macro = va_arg(args, int);
				(void) pr_error("Environ",
					"[set_fld_descript] Obsolete key in routine call!\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Remove: \"FpaF_WIND_CALCULATION, xxx\"\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Contact the FPA Development Group for more information!\n");
				(void) strcpy(fdesc->wind_func_name, FpaCblank);
				break;
		/* >>> the preceding is obsolete in next version <<< */

		/* >>> the following is obsolete in next version <<< */
			/* Set wind function by name */
			case FpaF_WIND_CALCULATION_NAME:
				name = va_arg(args, STRING);
				(void) pr_error("Environ",
					"[set_fld_descript] Obsolete key in routine call!\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Replace: \"FpaF_WIND_CALCULATION_NAME, xxx\"\n");
				(void) pr_error("Environ",
					"[set_fld_descript] With:    \"FpaF_WIND_FUNCTION_NAME, xxx\"\n");
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					nlen = strlen(name);
					if ( (int) nlen < MAX_FCHRS )
						{
						(void) strcpy(fdesc->wind_func_name, name);
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] More than %d characters\n",
									MAX_FCHRS);
						(void) pr_error("Environ",
							"[set_fld_descript]  for \"wind_function = %s\"!\n",
									name);
						(void) strcpy(fdesc->wind_func_name, FpaCblank);
						valid = FALSE;
						}
					}
				else
					{
					(void) strcpy(fdesc->wind_func_name, FpaCblank);
					}
				break;
		/* >>> the preceding is obsolete in next version <<< */

		/* >>> the following is obsolete in next version <<< */
			/* Set value calculation type */
			case FpaF_VALUE_CALCULATION:
				macro = va_arg(args, int);
				(void) pr_error("Environ",
					"[set_fld_descript] Obsolete key in routine call!\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Remove: \"FpaF_VALUE_CALCULATION, xxx\"\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Contact the FPA Development Group for more information!\n");
				(void) strcpy(fdesc->value_func_name, FpaCblank);
				break;
		/* >>> the preceding is obsolete in next version <<< */

		/* >>> the following is obsolete in next version <<< */
			/* Set value function by name */
			case FpaF_VALUE_CALCULATION_NAME:
				name = va_arg(args, STRING);
				(void) pr_error("Environ",
					"[set_fld_descript] Obsolete key in routine call!\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Replace: \"FpaF_VALUE_CALCULATION_NAME, xxx\"\n");
				(void) pr_error("Environ",
					"[set_fld_descript] With:    \"FpaF_VALUE_FUNCTION_NAME, xxx\"\n");
				if ( !blank(name) && !same(name, FpaCblank) )
					{
					nlen = strlen(name);
					if ( (int) nlen < MAX_FCHRS )
						{
						(void) strcpy(fdesc->value_func_name, name);
						}
					else
						{
						(void) pr_error("Environ",
							"[set_fld_descript] More than %d characters\n",
									MAX_FCHRS);
						(void) pr_error("Environ",
							"[set_fld_descript]  for \"value_function = %s\"!\n",
									name);
						(void) strcpy(fdesc->value_func_name, FpaCblank);
						valid = FALSE;
						}
					}
				else
					{
					(void) strcpy(fdesc->value_func_name, FpaCblank);
					}
				break;
		/* >>> the preceding is obsolete in next version <<< */

		/* >>> the following is obsolete in next version <<< */
			/* Set old calculation type by name */
			case FpaF_CALCULATION_TYPE_NAME:
				name = va_arg(args, STRING);
				(void) pr_error("Environ",
					"[set_fld_descript] Obsolete key in routine call!\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Remove: \"FpaF_CALCULATION_TYPE_NAME, xxx\"\n");
				(void) pr_error("Environ",
					"[set_fld_descript] Contact the FPA Development Group for more information!\n");
				(void) strcpy(fdesc->wind_func_name,  FpaCblank);
				(void) strcpy(fdesc->value_func_name, FpaCblank);
				break;
		/* >>> the preceding is obsolete in next version <<< */

			/* Error for unknown key */
			default:
				(void) pr_error("Environ",
					"[set_fld_descript] Unrecognizable key!\n");
				va_end(args);
				return FALSE;
			}
		}

	/* Reset args at end of list */
	va_end(args);

	/* Reset field pointer if element and level are set */
	/*  ... and can create a valid field!               */
	if ( NotNull(fdesc->edef) && NotNull(fdesc->ldef) )
		{

		/* Check for valid field */
		fdef = identify_field(fdesc->edef->name, fdesc->ldef->name);

		/* Reset all pointers for valid fields */
		if ( NotNull(fdef) )
			{
			fdesc->fdef = fdef;
			fdesc->edef = fdef->element;
			fdesc->ldef = fdef->level;
			}
		else
			{
			valid = FALSE;
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/
/** Copies the contents of fdesc2 into fdesc1.
 *	@param[out]	*fdesc1	Copy of field descriptor
 *	@param[in]  *fdesc2	Original field descriptor
 **********************************************************************/
void				copy_fld_descript

	(
	FLD_DESCRIPT		*fdesc1,
	const FLD_DESCRIPT	*fdesc2
	)

	{
	if ( IsNull(fdesc1) ) return;
	if ( IsNull(fdesc2) ) return;

	/* Copy map projection */
	(void) copy_map_projection(&fdesc1->mproj, &fdesc2->mproj);

	/* Copy directory information */
	(void) strcpy(fdesc1->path,  fdesc2->path);
	fdesc1->sdef   = fdesc2->sdef;
	fdesc1->subdef = fdesc2->subdef;
	(void) strcpy(fdesc1->rtime, fdesc2->rtime);

	/* Copy field information */
	fdesc1->edef   = fdesc2->edef;
	fdesc1->ldef   = fdesc2->ldef;
	fdesc1->fdef   = fdesc2->fdef;
	fdesc1->fmacro = fdesc2->fmacro;
	(void) strcpy(fdesc1->vtime,           fdesc2->vtime);
	(void) strcpy(fdesc1->wind_func_name,  fdesc2->wind_func_name);
	(void) strcpy(fdesc1->value_func_name, fdesc2->value_func_name);
	}

/**********************************************************************/
/**	Compare two field descriptors
 *
 *	@param[in]	*fdesc1		first field descriptor
 *	@param[in]	*fdesc2		second field descriptor
 * 	@return True if field descriptors are equivalent.
 **********************************************************************/
LOGICAL				same_fld_descript

	(
	FLD_DESCRIPT	*fdesc1,
	FLD_DESCRIPT	*fdesc2
	)

	{
	if ( IsNull(fdesc1) && IsNull(fdesc2) ) return TRUE;
	if ( IsNull(fdesc1) ) return FALSE;
	if ( IsNull(fdesc2) ) return FALSE;

	/* Compare map projections */
	if ( !same_map_projection(&fdesc1->mproj, &fdesc2->mproj) ) return FALSE;

	/* Compare directory parameters */
	if ( !same(fdesc1->path,  fdesc2->path) )  return FALSE;
	if ( fdesc1->sdef   != fdesc2->sdef )      return FALSE;
	if ( fdesc1->subdef != fdesc2->subdef )    return FALSE;

	/* Compare run timestamps */
	if ( !matching_tstamps(fdesc1->rtime, fdesc2->rtime) ) return FALSE;

	/* Compare field parameters */
	if ( fdesc1->edef   != fdesc2->edef )      return FALSE;
	if ( fdesc1->ldef   != fdesc2->ldef )      return FALSE;
	if ( fdesc1->fdef   != fdesc2->fdef )      return FALSE;
	if ( fdesc1->fmacro != fdesc2->fmacro )    return FALSE;

	/* Compare valid timestamps */
	if ( !matching_tstamps(fdesc1->vtime, fdesc2->vtime) ) return FALSE;

	/* Compare function names */
	if ( !same(fdesc1->wind_func_name,  fdesc2->wind_func_name) )  return FALSE;
	if ( !same(fdesc1->value_func_name, fdesc2->value_func_name) ) return FALSE;

	return TRUE;
	}

/**********************************************************************/
/**	Compare two field descriptors, but do not check the map
 * projections.
 *
 *	@param[in]	*fdesc1		first field descriptor
 *	@param[in]	*fdesc2		second field descriptor
 * 	@return True if field descriptors are equivalent.
 **********************************************************************/
LOGICAL				same_fld_descript_no_map

	(
	FLD_DESCRIPT	*fdesc1,
	FLD_DESCRIPT	*fdesc2
	)

	{
	if ( IsNull(fdesc1) && IsNull(fdesc2) ) return TRUE;
	if ( IsNull(fdesc1) ) return FALSE;
	if ( IsNull(fdesc2) ) return FALSE;

	/* Compare directory parameters */
	if ( !same(fdesc1->path,  fdesc2->path) )  return FALSE;
	if ( fdesc1->sdef   != fdesc2->sdef )      return FALSE;
	if ( fdesc1->subdef != fdesc2->subdef )    return FALSE;

	/* Compare run timestamps */
	if ( !matching_tstamps(fdesc1->rtime, fdesc2->rtime) ) return FALSE;

	/* Compare field parameters */
	if ( fdesc1->edef   != fdesc2->edef )      return FALSE;
	if ( fdesc1->ldef   != fdesc2->ldef )      return FALSE;
	if ( fdesc1->fdef   != fdesc2->fdef )      return FALSE;
	if ( fdesc1->fmacro != fdesc2->fmacro )    return FALSE;

	/* Compare valid timestamps */
	if ( !matching_tstamps(fdesc1->vtime, fdesc2->vtime) ) return FALSE;

	/* Compare function names */
	if ( !same(fdesc1->wind_func_name,  fdesc2->wind_func_name) )  return FALSE;
	if ( !same(fdesc1->value_func_name, fdesc2->value_func_name) ) return FALSE;

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   d a t a _ d i r e c t o r y _ p a t h                              *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Return a full directory path from a directory tag,
 * directory path, and directory subpath.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	dirtag		Directory tag from setup file
 *	@param[in]	dirpath		Directory path name
 *	@param[in]	subpath		Directory subpath name
 * 	@return String containing full directory path.
 **********************************************************************/

STRING				data_directory_path

	(
	STRING			dirtag,
	STRING			dirpath,
	STRING			subpath
	)

	{
	STRING	sdir;

	/* Determine directory path using directory tag from setup file */
	if ( !blank(dirtag) )
		{

		/* Get the directory from the setup file directory tag */
		sdir = get_directory(dirtag);
		if ( blank(sdir) ) return NullString;

		/* Return the full directory name */
		return pathname(pathname(sdir, dirpath), subpath);
		}

	/* Determine directory path if no directory tag from setup file given */
	else
		{

		/* Return the full directory name */
		return pathname(dirpath, subpath);
		}
	}

/***********************************************************************
*                                                                      *
*   f i n d _ d a t a _ d i r e c t o r y                              *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Routine to find a data directory for reading data files.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	dirtag		Directory tag from setup file
 *	@param[in]	dirpath		Directory path name
 *	@param[in]	subpath		Directory subpath name
 *	@param[in]	rtime		Run timestamp for directory
 * 	@return String containing full directory path for the appropriate
 * 			rtime.
 **********************************************************************/

STRING				find_data_directory

	(
	STRING			dirtag,
	STRING			dirpath,
	STRING			subpath,
	STRING			rtime
	)

	{
	int		nread;
	STRING	dir, dstamp;
	FILE	*fp;
	char	rt[MAX_NCHRS];

	/* Static buffer for directory name */
	static	char	presentdir[MAX_BCHRS] = "";

	/* Find the base directory name */
	dir = data_directory_path(dirtag, dirpath, subpath);
	if ( blank(dir) )
		{
		(void) pr_error("Environ",
			"[find_data_directory] Cannot build directory name!\n");
		(void) pr_error("Environ",
			"     Directory tag: \"%s\"  Directory path/subpath: \"%s / %s\"\n",
					SafeStr(dirtag), SafeStr(dirpath), SafeStr(subpath));
		return NullString;
		}

	/* Warning message if base directory cannot be found */
	if ( !find_directory(dir) )
		{
		(void) pr_warning("Environ",
			"[find_data_directory] Cannot find directory: \"%s\"!\n",
			SafeStr(dir));
		return NullString;
		}

	/* Return the base directory if that's all we wanted */
	(void) strcpy(presentdir, dir);
	if ( blank(rtime) ) return presentdir;

	/* Return the base directory if a run timestamp cannot be found */
	dstamp = pathname(presentdir, FpaFile_Dstamp);
	if ( !find_file(dstamp) ) return presentdir;

	/* Search the directory structure for specified run time */
	while (TRUE)
		{

		/* Return Null if directory not found */
		if ( !find_directory(presentdir) ) return NullString;

		/* Return Null if problem finding present run timestamp */
		dstamp = pathname(presentdir, FpaFile_Dstamp);
		if ( IsNull( fp = fopen(dstamp, "r") ) ) return NullString;

		/* Return Null if problem reading present run timestamp */
		nread = fscanf(fp, "%s", rt);
		(void) fclose(fp);
		if ( nread != 1 ) return NullString;

		/* Return the data directory if the run timestamp matches */
		if ( matching_tstamps(rt, rtime) ) return presentdir;

		/* Look in the previous directory in the directory structure */
		dir = pathname(presentdir, FpaFile_Prev);
		(void) strcpy(presentdir, dir);
		}
	}

/***********************************************************************
*                                                                      *
*   s e t _ s h u f f l e _ l o c k                                    *
*   r e l e a s e _ s h u f f l e _ l o c k                            *
*                                                                      *
*   Routines to set or release a shuffle lock on a given directory.    *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Set a shuffle lock on directory
 *
 *	@param[in]	dir			directory name
 **********************************************************************/
LOGICAL				set_shuffle_lock

	(
	STRING			dir
	)

	{
	STRING		path;
	int			status;
	LOGICAL		fstatus;

	/* Get path for shuffle lock for the given directory */
	path = pathname(dir, FpaFile_ShuffleLock);
	if ( blank(path) ) return FALSE;

	/* Set a shuffle lock for the given directory */
	status = fslock(path, 1, 120);
	if ( status < 0 )
		{

		/* Take over the shuffle lock, since no one should have */
		/*  a shuffle lock for more than 120 seconds!           */
		(void) fprintf(stderr,
				"[set_shuffle_lock] Taking over shuffle lock \"%s\"!\n",
				path);
		status = fsunlk(path);
		if ( status < 0 )
			{
			(void) fprintf(stderr,
					"[set_shuffle_lock] Unable to release shuffle lock!\n");
			}
		status = fslock(path, 0, 0);
		if ( status < 0 )
			{
			(void) fprintf(stderr,
					"[set_shuffle_lock] Unable to take over shuffle lock!\n");
			return FALSE;
			}
		}

	/* Now check that all file locks in the directory have been released */
	/*  ... and release them all if required!                            */
	fstatus = file_locks_released(dir, 1, 60);
	if ( !fstatus )
		{
		(void) fprintf(stderr,
				"[set_shuffle_lock] Not all file locks released in \"%s\"!\n",
				dir);
		fstatus = release_all_file_locks(dir);
		}

	/* Return code for success of setting shuffle lock */
	/*  and releasing all file locks                   */
	return fstatus;
	}

/**********************************************************************/

/**********************************************************************/
/** Release a shuffle lock on directory
 *
 *	@param[in]	dir			directory name
 **********************************************************************/
LOGICAL				release_shuffle_lock

	(
	STRING			dir
	)

	{
	STRING		path;
	int			status;

	/* Get path for shuffle lock for the given directory */
	path = pathname(dir, FpaFile_ShuffleLock);
	if ( blank(path) ) return FALSE;

	/* Release shuffle lock for the given directory */
	status = fsunlk(path);
	if ( status < 0 )
		{
		(void) fprintf(stderr,
			"[release_shuffle_lock] Unable to release shuffle lock \"%s\"!\n",
			path);
		}

	/* Return code for success of releasing shuffle lock */
	return ( status == 0 ) ? TRUE: FALSE;
	}

/***********************************************************************
*                                                                      *
*   s e t _ f i l e _ l o c k                                          *
*   r e l e a s e _ f i l e _ l o c k                                  *
*   f i l e _ l o c k s _ r e l e a s e d                              *
*   r e l e a s e _ a l l _ f i l e _ l o c k s                        *
*                                                                      *
*   Routines to set, check or release file locks in a given directory. *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Set a file lock in a directory
 *
 *	@param[in]	dir			directory name
 *	@param[in]	vtime		valid time string
 **********************************************************************/
LOGICAL				set_file_lock

	(
	STRING			dir,
	STRING			vtime
	)

	{
	STRING		path, dirpath;
	char		fpath[MAX_BCHRS];
	int			status;

	/* Wait for shuffle lock on this directory to be released */
	path = pathname(dir, FpaFile_ShuffleLock);
	if ( blank(path) ) return FALSE;
	status = fswait(path, 1, 300);
	if ( status < 0 )
		{
		(void) fprintf(stderr,
				"[set_file_lock] Shuffle lock on \"%s\"!\n", dir);
		return FALSE;
		}

	/* Get path for file lock (without valid time) in the given directory */
	path = pathname(dir, FpaFile_FileLock);
	dirpath = safe_strdup(path);

	/* Add the valid time string to the file lock path */
	(void) safe_strcpy(fpath, dirpath);
	(void) safe_strcat(fpath, vtime);
	FREEMEM(dirpath);
	if ( blank(fpath) ) return FALSE;

	/* Set a file lock in the given directory */
	status = fslock(fpath, 1, 300);
	if ( status < 0 )
		{

		/* Take over the file lock, since no one should have */
		/*  a file lock for more than 300 seconds!           */
		(void) fprintf(stderr,
				"[set_file_lock] Taking over file lock \"%s\"!\n", fpath);
		status = fsunlk(fpath);
		if ( status < 0 )
			{
			(void) fprintf(stderr,
					"[set_file_lock] Unable to release file lock!\n");
			}
		status = fslock(fpath, 0, 0);
		if ( status < 0 )
			{
			(void) fprintf(stderr,
					"[set_file_lock] Unable to take over file lock!\n");
			}
		}

	/* Return code for success of setting file lock */
	return ( status == 0 ) ? TRUE: FALSE;
	}

/**********************************************************************/

/**********************************************************************/
/** Release a file lock in a directory
 *
 *	@param[in]	dir			directory name
 *	@param[in]	vtime		valid time string
 **********************************************************************/
LOGICAL				release_file_lock

	(
	STRING			dir,
	STRING			vtime
	)

	{
	STRING		path, dirpath;
	char		fpath[MAX_BCHRS];
	int			status;

	/* Get path for file lock (without valid time) in the given directory */
	path = pathname(dir, FpaFile_FileLock);
	dirpath = safe_strdup(path);

	/* Add the valid time string to the file lock path */
	(void) safe_strcpy(fpath, dirpath);
	(void) safe_strcat(fpath, vtime);
	FREEMEM(dirpath);
	if ( blank(fpath) ) return FALSE;

	/* Release file lock in the given directory */
	status = fsunlk(fpath);
	if ( status < 0 )
		{
		(void) fprintf(stderr,
			"[release_file_lock] Unable to release file lock \"%s\"!\n",
			fpath);
		}

	/* Return code for success of releasing file lock */
	return ( status == 0 ) ? TRUE: FALSE;
	}

/**********************************************************************/
/** Check that file locks in a directory have been released
 *
 *	@param[in]	dir			directory name
 *	@param[in]	stime		time between attempts
 *	@param[in]	tries		how many attempts should we make?
 **********************************************************************/
LOGICAL				file_locks_released

	(
	STRING			dir,
	int				tdelta,
	int				tries
	)

	{
	int			nfiles, nn;
	STRING		*files;

	/* Keep trying to find existing file locks */
	if (tries  <= 0) tries = 1;
	if (tdelta <= 0) tries = 1;
	while (tries--)
		{

		/* Find all file locks in the given directory */
		nfiles = dirlist(dir, FpaFile_FileLock, &files);

		/* Return if no more file locks found */
		if (nfiles <= 0) return TRUE;

		/* Print out file lock files */
		(void) fprintf(stderr,
			"[file_locks_released] Directory: \"%s\"  with %d file locks!\n",
			dir, nfiles);
		for (nn=0; nn<nfiles; nn++)
			(void) fprintf(stderr,
				"[file_locks_released]   File lock: \"%s\"\n", files[nn]);

		/* Try again */
		(void) sleep((UNSIGN) tdelta);
		}

	/* Return code based on whether file locks still found */
	return ( nfiles <= 0 ) ? TRUE: FALSE;
	}

/**********************************************************************/
/** Release all file locks in a directory
 *
 *	@param[in]	dir			directory name
 **********************************************************************/
LOGICAL				release_all_file_locks

	(
	STRING			dir
	)

	{
	int			nfiles, nn, status;
	STRING		*files;
	LOGICAL		fstatus;

	/* Find all file locks in the given directory */
	nfiles = dirlist(dir, FpaFile_FileLock, &files);
	if (nfiles <= 0) return TRUE;

	/* Release the file locks */
	fstatus = TRUE;
	for (nn=0; nn<nfiles; nn++)
		{

		/* Release file lock in the given directory */
		status = fsunlk(files[nn]);
		if ( status < 0 )
			{
			(void) fprintf(stderr,
					"[release_all_file_locks] Unable to release file lock \"%s\"!\n",
					files[nn]);
			fstatus = FALSE;
			}
		}

	/* Return code for success of releasing all file locks */
	return fstatus;
	}

/***********************************************************************
*                                                                      *
*   s o u r c e _ d i r e c t o r y                                    *
*   s o u r c e _ d i r e c t o r y _ b y _ n a m e                    *
*   p r e p a r e _ s o u r c e _ d i r e c t o r y                    *
*   p r e p a r e _ s o u r c e _ d i r e c t o r y _ b y _ n a m e    *
*                                                                      *
*   Routines to find an FPA source directory for reading data files or *
*   to prepare an FPA source directory for writing data files.         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Find an FPA source directory for reading data files by
 * a field descriptor.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return full path name of source directory.
 **********************************************************************/
STRING				source_directory

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceSubStruct	*subdef;
	FpaConfigSourceIOStruct		*sio;
	STRING						rstamp;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NullString;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Set run time (with minutes correctly encoded) */
	if ( sdef->minutes_rqd )
		rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
	else
		rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);

	/* Return the directory path */
	return find_data_directory(sio->src_tag, sio->src_path,
								subdef->sub_path, rstamp);
	}

/**********************************************************************/

/**********************************************************************/
/** Find an FPA source directory for reading data files by
 * name.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	name		Source name for directory
 *	@param[in]	subname		Subsource name for directory
 *	@param[in]	rtime		Run timestamp for directory
 * 	@return full path name of source directory.
 **********************************************************************/
STRING				source_directory_by_name

	(
	STRING			name,
	STRING			subname,
	STRING			rtime
	)

	{
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceIOStruct		*sio;
	FpaConfigSourceSubStruct	*subdef;
	STRING						rstamp;

	/* Check for the named source */
	sdef = identify_source(name, subname);
	if ( IsNull(sdef) ) return NullString;

	/* Set pointers to SourceIO and SourceSub structures */
	sio    = sdef->src_io;
	subdef = sdef->src_sub;
	if ( IsNull(sio) || IsNull(subdef) ) return NullString;

	/* Set run time (with minutes correctly encoded) */
	if ( sdef->minutes_rqd )
		rstamp = tstamp_to_minutes(rtime, NullInt);
	else
		rstamp = tstamp_to_hours(rtime, TRUE, NullInt);

	/* Return the directory path */
	return find_data_directory(sio->src_tag, sio->src_path, subdef->sub_path,
								rstamp);
	}

/**********************************************************************/
/** Prepare an FPA source directory for writing data files.
 * This function does any file shuffling and directory creation that
 * may be required.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return full path to default source directory.
 **********************************************************************/
STRING				prepare_source_directory

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	int							nn, nread, ncomp;
	LOGICAL						created;
	STRING						dirpath, datefile;
	FILE						*fp;
	char						dstamp[MAX_NCHRS];
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceSubStruct	*subdef;
	FpaConfigSourceIOStruct		*sio;
	STRING						rstamp;

	/* Static buffers for directory names */
	static	char	workingdir[MAX_BCHRS]  = "";
	static	char	datadir[MAX_BCHRS]     = "";
	static	char	basedir[MAX_BCHRS]     = "";

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NullString;

	/* Set run time (with minutes correctly encoded) */
	if ( fdesc->sdef->minutes_rqd )
		rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
	else
		rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Get the working directory to return to when finished! */
	(void) getcwd(workingdir, MAX_BCHRS);

	/* Return the directory path if the directory already exists! */
	dirpath = find_data_directory(sio->src_tag, sio->src_path,
									subdef->sub_path, rstamp);
	if ( !blank(dirpath) )
		{
		(void) strcpy(datadir, dirpath);

		/* Check Guidance and Allied Model type directories */
		/*  for a datetime stamp file!                      */
		switch ( sdef->src_type )
			{

			/* Branch to Guidance or Allied Model type directories */
			case FpaC_GUIDANCE:
			case FpaC_ALLIED:
				datefile = pathname(datadir, FpaFile_Dstamp);
				if ( find_file(datefile) ) return datadir;

				/* Try to create a datetime stamp file! */
				else
					{
					/* Cannot create a datetime stamp file */
					if ( blank(rstamp) ) return NullString;

					/* Try to get to the data directory */
					if ( chdir(datadir) != 0 ) return NullString;

					/* Create the datetime stamp file */
					fp = fopen(FpaFile_Dstamp, "w");
					(void) fprintf(fp, "%s\n", rstamp);
					(void) fclose(fp);

					/* Return to the working directory      */
					/*  before returning the directory path */
					(void) chdir(workingdir);
					return datadir;
					}

			/* Branch to directories without datetime stamp files */
			default:
				return datadir;
			}
		}

	/* Directory does not exist ... so we will try to create it */

	/* First set the base directory */
	dirpath = data_directory_path(sio->src_tag, sio->src_path,
									subdef->sub_path);
	(void) strcpy(basedir, dirpath);

	/* Then create the base directory and all sub directories (if necessary) */
	(void) strcpy(datadir, basedir);
	for ( nn=2; nn<=sio->src_layers; nn++ )
		{
		dirpath = pathname(datadir, FpaFile_Prev);
		(void) strcpy(datadir, dirpath);
		}
	if ( !create_directory(datadir, S_IRWXU|S_IRWXG|S_IRWXO, &created) )
		{

		/* Another process may be creating this directory at the same time */
		/*  ... so wait a moment and then check again */
		(void) sleep(1);
		dirpath = find_data_directory(sio->src_tag, sio->src_path,
										subdef->sub_path, rstamp);
		if ( !blank(dirpath) )
			{
			(void) pr_error("Environ",
				"[prepare_source_directory] Directory: \"%s\" just created!\n",
						SafeStr(dirpath));
			return dirpath;
			}

		else
			{
			(void) pr_error("Environ",
				"[prepare_source_directory] Cannot create directory: \"%s\" in \"%s\"\n",
						SafeStr(datadir), SafeStr(workingdir));
			return NullString;
			}
		}
	else if ( created )
		{
		(void) pr_warning("Environ",
			"[prepare_source_directory] Creating directory: \"%s\" in \"%s\"\n",
					SafeStr(datadir), SafeStr(workingdir));
		}
	else
		{

		/* Another process may be creating this directory at the same time */
		/*  ... so wait a moment and then check again */
		(void) sleep(1);
		dirpath = find_data_directory(sio->src_tag, sio->src_path,
										subdef->sub_path, rstamp);
		if ( !blank(dirpath) )
			{
			(void) pr_error("Environ",
				"[prepare_source_directory] Directory: \"%s\" just created!\n",
						SafeStr(dirpath));
			return dirpath;
			}
		}

	/* Place a SHUFFLE lock on the base directory while we prepare */
	/*  the requested directory in the data directory tree         */
	if ( !set_shuffle_lock(basedir) )
		{
		(void) pr_error("Environ",
			"[prepare_source_directory] Cannot set shuffle lock!\n");
		(void) pr_error("Environ",
			"     Base directory: \"%s\" in \"%s\"\n",
					SafeStr(basedir), SafeStr(workingdir));
		return NullString;
		}

	/* Try to fit the run time into the data directory tree */
	(void) strcpy(datadir, basedir);
	while ( TRUE )
		{

		/* Error message if data directory cannot be reached           */
		/*  ... data directory cannot be fit into data directory tree! */
		if ( chdir(datadir) != 0 )
			{
			(void) pr_error("Environ",
				"[prepare_source_directory] Cannot access data directory: \"%s\"\n",
							SafeStr(datadir));
			(void) pr_error("Environ",
				"     Base directory: \"%s\" in \"%s\"   Run time (too old?): \"%s\"\n",
						SafeStr(basedir), SafeStr(workingdir),
						SafeStr(rstamp));
			(void) chdir(workingdir);
			(void) release_shuffle_lock(basedir);
			return NullString;
			}

		/* Check for timestamp in present data directory */
		if ( NotNull( fp = fopen(FpaFile_Dstamp, "r") ) )
			{

			/* Check the timestamp */
			nread = fscanf(fp, "%s", dstamp);
			(void) fclose(fp);

			/* Invalid timestamp ... so use this data directory! */
			if ( nread != 1 || !valid_tstamp(dstamp) )
				break;

			/* If timestamps agree ... this is the data directory we want! */
			/* Note that run timestamps should never be local times, so we */
			/*  do not worry about the center longitude!                   */
			ncomp = compare_tstamps(dstamp, rstamp, 0.0);
			if ( ncomp == 0 )
				break;

			/* If present timestamp is older ... this is the data directory */
			/*  we want, but will have to shuffle present data files first! */
			else if ( ncomp < 0 )
				{
				(void) fprintf(stdout, "    Shuffling data files for: %s %s\n",
						SafeStr(sdef->name), SafeStr(subdef->name));
				(void) shuffle_directory();
				break;
				}

			/* If present timestamp is newer ... look in the next directory */
			else
				{
				dirpath = pathname(datadir, FpaFile_Prev);
				(void) strcpy(datadir, dirpath);
				continue;
				}
			}

		/* No timestamp found ... so this is the data directory we want! */
		else
			break;
		}

	/* Now write a datetime stamp in the present directory */
	/*  for Guidance or Allied Model type directories      */
	switch ( sdef->src_type )
		{

		/* Branch to Guidance or Allied Model type directories */
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			fp = fopen(FpaFile_Dstamp, "w");
			(void) fprintf(fp, "%s\n", SafeStr(rstamp));
			(void) fclose(fp);
			break;

		/* Branch to directories without datetime stamp files */
		default:
			break;
		}

	/* Return to the working directory and release the   */
	/*  SHUFFLE lock before returning the directory path */
	(void) chdir(workingdir);
	(void) release_shuffle_lock(basedir);
	return datadir;
	}

/**********************************************************************/
/** Prepare an FPA source directory for writing data files.
 * This function creates a field descriptor from name, subname and rtime
 * and then calls prepare_source_directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	name		Source name for directory
 *	@param[in]	subname 	Subsource name for directory
 *	@param[in]	rtime		Run timestamp for directory
 * 	@return full path to default source directory.
 **********************************************************************/
STRING				prepare_source_directory_by_name

	(
	STRING			name,
	STRING			subname,
	STRING			rtime
	)

	{
	FLD_DESCRIPT	descript;

	/* Initialize a field descriptor to hold the source information */
	(void) init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_SOURCE_NAME,    name,
							FpaF_SUBSOURCE_NAME, subname,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) ) return NullString;

	/* Prepare the source directory */
	return prepare_source_directory(&descript);
	}

/***********************************************************************
*                                                                      *
*   s o u r c e _ r u n _ t i m e _ l i s t                            *
*   s o u r c e _ r u n _ t i m e _ l i s t _ f r e e                  *
*                                                                      *
*   Return run timestamps for base directory and all sub directories   *
*   for a given source.                                                *
*   (The _free() function frees the list of run timestamps.)           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Return run timestamps for base directory and all sub
 * directories for a given source as a list.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[out]	**list		list of run times
 *  @return The size of the list.
 **********************************************************************/
int					source_run_time_list

	(
	FLD_DESCRIPT	*fdesc,
	STRING			**list
	)

	{
	int						ntimes;
	STRING					*times;
	FpaConfigSourceStruct	*sdef;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullStringList;

	/* Return 0 if no structure passed */
	if ( IsNull(fdesc) ) return 0;

	/* Set pointer to Source structure */
	sdef   = fdesc->sdef;
	if ( IsNull(sdef) ) return 0;

	/* Return list of run times based on type of source */
	switch ( sdef->src_type )
		{

		/* Branch to Guidance or Allied Model type files */
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			ntimes = data_directory_run_times(fdesc, &times);
			if ( NotNull(list) ) *list = times;
			return ntimes;

		/* Return 0 for source types which do not use run times */
		default:
			return 0;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Free runtime timestamps list.
 *
 *	@param[in]	**list		list of run times
 *	@param[in]	num			size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int					source_run_time_list_free

	(
	STRING			**list,
	int				num
	)

	{

	/* Free the list of run times */
	FREELIST(*list, num);

	/* Reset the number of run times and return it */
	num = 0;
	return num;
	}

/***********************************************************************
*                                                                      *
*   s o u r c e _ v a l i d _ t i m e _ l i s t                        *
*   s o u r c e _ v a l i d _ t i m e _ l i s t _ f r e e              *
*   s o u r c e _ v a l i d _ t i m e _ s u b l i s t                  *
*   s o u r c e _ v a l i d _ t i m e _ s u b l i s t _ f r e e        *
*                                                                      *
*   Return valid timestamps for a given source and run time            *
*   ... or a subset of the list based on given list members.           *
*                                                                      *
*   NOTE that the valid times are matched with element, level, or      *
*   valid time if these are set in the field descriptor!               *
*   (The _free() functions free the list of valid timestamps.)         *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return valid timestamps for a given source and run time.
 *
 *	@param[in]	*fdesc	field descriptor
 *	@param[in]	macro	enumerated time dependence to match
 *	@param[in]	**list	list of valid times for given source and run time
 *  @return The size of the list.
 **********************************************************************/
int					source_valid_time_list

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	STRING			**list
	)

	{
	int						ntimes;
	STRING					*times;
	FpaConfigSourceStruct	*sdef;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullStringList;

	/* Return 0 if no structure passed */
	if ( IsNull(fdesc) ) return 0;

	/* Set pointer to Source structure */
	sdef = fdesc->sdef;
	if ( IsNull(sdef) ) return 0;

	/* Return list of valid times based on type of source */
	switch ( sdef->src_type )
		{

		/* Branch to Depictions, Guidance or Allied Model type files */
		case FpaC_DEPICTION:
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			ntimes = data_directory_valid_times(fdesc, macro, &times);
			if ( NotNull(list) ) *list = times;
			return ntimes;

		/* Return 0 for source types which do not use valid times */
		default:
			return 0;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Free source valid timestamps.
 *
 *	@param[in]	**list		list of valid times
 *	@param[in]	num			size of list
 *  @return The size of the list.
 **********************************************************************/
int					source_valid_time_list_free

	(
	STRING			**list,
	int				num
	)

	{

	/* Free the list of valid times */
	FREELIST(*list, num);

	/* Reset the number of valid times and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/**********************************************************************/
/** Return a subset of valid timestamps for a given source and
 * run time based on given list members.
 *
 *	@param[in]	*fdesc	field descriptor
 *	@param[in]	macro	enumerated time dependence to match
 *	@param[in]	nsub	number of valid times requested
 *	@param[in]	vbgn	valid time at start of series
 *	@param[in]	vcen	valid time at center of series
 *	@param[in]	vend	valid time at end of series
 *	@param[out]	**list	list of valid times for given source and run time
 *  @return The size of the list.
 **********************************************************************/
int					source_valid_time_sublist

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	int				nsub,
	STRING			vbgn,
	STRING			vcen,
	STRING			vend,
	STRING			**list
	)

	{
	FLD_DESCRIPT	descript;
	int				nfpa;
	STRING			*vtfpa;
	int				nn, nbgn, nend, ncen, nvt;
	STRING			*vtlist;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullStringList;

	/* Return 0 if no structure passed */
	if ( IsNull(fdesc) ) return 0;

	/* Determine list of all possible timestamps */
	(void) copy_fld_descript(&descript, fdesc);
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, FpaCblank,
							FpaF_END_OF_LIST) ) return 0;
	nfpa = source_valid_time_list(&descript, macro, &vtfpa);
	if ( nfpa < 1 ) return 0;

	/* If both vbgn and vend are specified, extract valid */
	/*  times between vbgn and vend                       */
	if ( !blank(vbgn) && !same(vbgn, FpaCblank)
			&& !blank(vend) && !same(vend, FpaCblank) )
		{
		nbgn = closest_source_valid_time(&descript, macro, vbgn, NullStringPtr);
		nend = closest_source_valid_time(&descript, macro, vend, NullStringPtr);
		if ( nbgn < 0 || nend < 0 )
			return source_valid_time_list_free(&vtfpa, nfpa);
		}

	/* If vbgn is specified, extract nsub valid times after vbgn */
	else if ( !blank(vbgn) && !same(vbgn, FpaCblank) && nsub > 0 )
		{
		nbgn = closest_source_valid_time(&descript, macro, vbgn, NullStringPtr);
		if ( nbgn < 0 ) return source_valid_time_list_free(&vtfpa, nfpa);

		nend = nbgn + nsub - 1;
		if ( nend > nfpa - 1 ) nend = nfpa - 1;
		}

	/* If vend is specified, extract nsub valid times up to vend */
	else if ( !blank(vend) && !same(vend, FpaCblank) && nsub > 0 )
		{
		nend = closest_source_valid_time(&descript, macro, vend, NullStringPtr);
		if ( nend < 0 ) return source_valid_time_list_free(&vtfpa, nfpa);

		nbgn = nend - nsub + 1;
		if ( nbgn < 0 ) nbgn = 0;
		}

	/* If vcen is specified, extract nsub valid times about vcen */
	else if ( !blank(vcen) && !same(vcen, FpaCblank) && nsub > 0 )
		{
		ncen = closest_source_valid_time(&descript, macro, vcen, NullStringPtr);
		if ( ncen < 0 ) return source_valid_time_list_free(&vtfpa, nfpa);

		if ( nsub >= nfpa )
			{
			nbgn = 0;
			nend = nfpa - 1;
			}
		else
			{
			nbgn = ncen - nsub/2;
			nend = nbgn + nsub - 1;
			if ( nbgn < 0 )
				{
				nbgn = 0;
				nend = nsub - 1;
				}
			else if ( nend > nfpa - 1 )
				{
				nbgn = nfpa - nsub;
				nend = nfpa - 1;
				}
			}
		}

	/* Error return for all other cases */
	else
		{
		return source_valid_time_list_free(&vtfpa, nfpa);
		}

	/* Make copies of valid times to return */
	nvt    = nend - nbgn + 1;
	vtlist = INITMEM(STRING, nvt);
	for ( nn=nbgn; nn<=nend; nn++ )
		{
		vtlist[nn-nbgn] = strdup(vtfpa[nn]);
		}

	/* Set return variables and return number of times in series */
	nfpa = source_valid_time_list_free(&vtfpa, nfpa);
	if ( NotNull(list) ) *list = vtlist;
	return nvt;
	}

/**********************************************************************/

/**********************************************************************/
/** Free source valid timestamps.
 *
 *	@param[in]	**list		list of valid times
 *	@param[in]	num			size of list
 *  @return The size of the list (0).
 **********************************************************************/
int					source_valid_time_sublist_free

	(
	STRING			**list,
	int				num
	)

	{

	/* Free the list of valid times */
	return source_valid_time_list_free(list, num);
	}

/***********************************************************************
*                                                                      *
*   m a t c h e d _ s o u r c e _ v a l i d _ t i m e                  *
*   c l o s e s t _ s o u r c e _ v a l i d _ t i m e                  *
*                                                                      *
*   Return location of requested valid timestamp from a list of valid  *
*   timestamps for a given source and run time.                        *
*                                                                      *
*   NOTE that the valid times are matched with element, level, or      *
*   valid time if these are set in the field descriptor!               *
*                                                                      *
*   matched_... returns matched time in list or previous.              *
*   closest_... returns closest time in list.                          *
*                                                                      *
*   m a t c h e d _ s o u r c e _ v a l i d _ t i m e _ r e s e t      *
*   c l o s e s t _ s o u r c e _ v a l i d _ t i m e _ r e s e t      *
*                                                                      *
*   Reset the valid timestamp in a field descriptor based on the       *
*   matched or closest valid timestamp for a given source and run time.*
*                                                                      *
*   NOTE that the valid times are matched with element, level, or      *
*   valid time if these are set in the field descriptor!               *
*                                                                      *
*   matched_... resets to matched time in list or previous time.       *
*   closest_... resets to closest time in list.                        *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return location of requested valid timestamp
 * (or the previous) from a list of valid timestamps for a
 * given source and run time.
 *
 * @note that the valid times are matched with element, level or
 * valid time if these are set in the field descriptor!
 *
 *	@param[in]	*fdesc	 	field descriptor
 *	@param[in]	macro		enumerated time dependence to match
 *	@param[in]	mtchtime	valid time to match
 *	@param[out]	*vtime		matched valid time in list
 *  @return The index of the valid timestamp in the list.
 **********************************************************************/
int					matched_source_valid_time

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	STRING			mtchtime,
	STRING			*vtime
	)

	{
	int				myear, mjday, mhour, mmin;
	LOGICAL			mlocal;
	FLD_DESCRIPT	descript;
	int				nfpa;
	STRING			vtr, *vtfpa;
	int				imtch, vyear, vjday, vhour, vmin;
	LOGICAL			flocal, lmatch;

	static	STRING	ts_match = NullString;

	/* Initialize return parameter */
	if ( NotNull(vtime) ) *vtime = NullString;

	/* Return if no structure passed */
	if ( IsNull(fdesc) ) return (-1);

	/* Convert valid time to match based on type of field */
	if ( NotNull(fdesc->edef) )
		{
		if ( fdesc->edef->elem_tdep->time_dep == FpaC_DAILY )
			vtr = gmt_to_local(mtchtime, fdesc->mproj.clon);
		else
			vtr = local_to_gmt(mtchtime, fdesc->mproj.clon);
		if ( !parse_tstamp(vtr, &myear, &mjday, &mhour, &mmin,
				&mlocal, NullLogicalPtr) ) return (-1);
		}

	/* Interpret date and time from valid time to match */
	else
		{
		if ( !parse_tstamp(mtchtime, &myear, &mjday, &mhour, &mmin,
				&mlocal, NullLogicalPtr) ) return (-1);
		}

	/* Set valid timestamps from directory and element/level */
	(void) copy_fld_descript(&descript, fdesc);
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, FpaCblank,
							FpaF_END_OF_LIST) ) return (-1);
	nfpa = source_valid_time_list(&descript, macro, &vtfpa);

	/* Set valid timestamps in directory without using element/level */
	/*  ... to allow for calculated fields!                          */
	if ( nfpa < 1 )
		{
		if ( !set_fld_descript(&descript, FpaF_ELEMENT_NAME, FpaCblank,
				FpaF_LEVEL_NAME, FpaCblank, FpaF_END_OF_LIST) ) return (-1);
		nfpa = source_valid_time_list(&descript, macro, &vtfpa);
		if ( nfpa < 1) return (-1);
		}

	/* Find valid timestamp at or just before valid time to match */
	lmatch = FALSE;
	for ( imtch=nfpa-1; imtch>=0; imtch-- )
		{

		/* Extract the time parameters */
		if ( !parse_tstamp(vtfpa[imtch], &vyear, &vjday, &vhour, &vmin,
				&flocal, NullLogicalPtr) )
			{
			nfpa = source_valid_time_list_free(&vtfpa, nfpa);
			return (-1);
			}

		/* Go on if the local time flags do not match */
		if ( (mlocal && !flocal) || (!mlocal && flocal) ) continue;

		/* Compare the times */
		lmatch = TRUE;
		if ( mdif(vyear, vjday, vhour, vmin, myear, mjday, mhour, mmin) >= 0 )
			break;
		}

	/* Check if any times were successfully compared */
	if ( !lmatch ) return (-1);

	/* Default is first timestamp in list */
	if ( imtch < 0 ) imtch = 0;

	/* Make copy of matched timestamp */
	if ( NotNull(ts_match) ) FREEMEM(ts_match);
	ts_match = strdup(vtfpa[imtch]);
	nfpa     = source_valid_time_list_free(&vtfpa, nfpa);

	/* Set return variable and return location of matched valid time */
	if ( NotNull(vtime) ) *vtime = ts_match;
	return imtch;
	}

/**********************************************************************/

/**********************************************************************/
/** Return location of requested valid timestamp
 * (or the closest) from a list of valid timestamps for a
 * given source and run time.
 *
 * @note that the valid times are matched with element, level or
 * valid time if these are set in the field descriptor!
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	macro		enumerated time dependence to match
 *	@param[in]	mtchtime	valid time to match
 *	@param[out]	*vtime		closest valid time in list
 *  @return The index of the valid timestamp in the list.
 **********************************************************************/
int					closest_source_valid_time

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	STRING			mtchtime,
	STRING			*vtime
	)

	{
	int				myear, mjday, mhour, mmin;
	LOGICAL			mlocal;
	FLD_DESCRIPT	descript;
	int				nfpa;
	STRING			vtr, *vtfpa;
	int				imtch, itdf, imaxtdf, vyear, vjday, vhour, vmin;
	LOGICAL			flocal, lmatch;

	static	STRING	ts_match = NullString;

	/* Initialize return parameter */
	if ( NotNull(vtime) ) *vtime = NullString;

	/* Return if no structure passed */
	if ( IsNull(fdesc) ) return (-1);

	/* Convert valid time to match based on type of field */
	if ( NotNull(fdesc->edef) )
		{
		if ( fdesc->edef->elem_tdep->time_dep == FpaC_DAILY )
			vtr = gmt_to_local(mtchtime, fdesc->mproj.clon);
		else
			vtr = local_to_gmt(mtchtime, fdesc->mproj.clon);
		if ( !parse_tstamp(vtr, &myear, &mjday, &mhour, &mmin,
				&mlocal, NullLogicalPtr) ) return (-1);
		}

	/* Interpret date and time from valid time to match */
	else
		{
		if ( !parse_tstamp(mtchtime, &myear, &mjday, &mhour, &mmin,
				&mlocal, NullLogicalPtr) ) return (-1);
		}

	/* Set valid timestamps from directory and element/level */
	(void) copy_fld_descript(&descript, fdesc);
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, FpaCblank,
							FpaF_END_OF_LIST) ) return (-1);
	nfpa = source_valid_time_list(&descript, macro, &vtfpa);

	/* Set valid timestamps in directory without using element/level */
	/*  ... to allow for calculated fields!                          */
	if ( nfpa < 1 )
		{
		if ( !set_fld_descript(&descript, FpaF_ELEMENT_NAME, FpaCblank,
				FpaF_LEVEL_NAME, FpaCblank, FpaF_END_OF_LIST) ) return (-1);
		nfpa = source_valid_time_list(&descript, macro, &vtfpa);
		if ( nfpa < 1) return (-1);
		}

	/* Find closest valid timestamp to valid time to match */
	lmatch = FALSE;
	for ( imtch=0; imtch<nfpa; imtch++ )
		{

		/* Extract the time parameters */
		if ( !parse_tstamp(vtfpa[imtch], &vyear, &vjday, &vhour, &vmin,
				&flocal, NullLogicalPtr) )
			{
			nfpa = source_valid_time_list_free(&vtfpa, nfpa);
			return (-1);
			}

		/* Go on if the local time flags do not match */
		if ( (mlocal && !flocal) || (!mlocal && flocal) ) continue;

		/* Compare the times */
		lmatch = TRUE;
		itdf = abs(mdif(vyear, vjday, vhour, vmin, myear, mjday, mhour, mmin));
		if ( imtch == 0 )
			imaxtdf = itdf;
		else
			{
			if ( itdf >= imaxtdf ) break;
			imaxtdf = itdf;
			}
		}

	/* Check if any times were successfully compared */
	if ( !lmatch ) return (-1);

	/* Closest timestamp is the previous one in list */
	if ( imtch > 0 ) imtch--;

	/* Make copy of closest timestamp */
	if ( NotNull(ts_match) ) FREEMEM(ts_match);
	ts_match = strdup(vtfpa[imtch]);
	nfpa     = source_valid_time_list_free(&vtfpa, nfpa);

	/* Set return variable and return location of closest valid time */
	if ( NotNull(vtime) ) *vtime = ts_match;
	return imtch;
	}

/**********************************************************************/

/**********************************************************************/
/** Reset the valid timestamp in a field descriptor based on
 * the matched (or previous) valid timestamp for a given source and
 * run time.
 *
 * @note that the valid times are matched with element, level or
 * valid time if these are set in the field descriptor!
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	macro		enumerated time dependence to match
 *	@param[in]	mtchtime	valid time to match
 * 	@return True if successful.
 **********************************************************************/
LOGICAL				matched_source_valid_time_reset

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	STRING			mtchtime
	)

	{
	FLD_DESCRIPT	descript;
	STRING			vtime;

	/* Return if no structure passed */
	if ( IsNull(fdesc) ) return FALSE;

	/* Return now if the file exists for this valid time */
	if ( !blank(find_meta_filename(fdesc)) ) return TRUE;

	/* Make copy of field descriptor for finding matched valid time */
	(void) copy_fld_descript(&descript, fdesc);
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, NullString, FpaF_END_OF_LIST) )
			return FALSE;

	/* Determine matched valid time (if possible) */
	if ( matched_source_valid_time(&descript, macro, mtchtime, &vtime) < 0 )
			return FALSE;

	/* Reset matched valid time in field descriptor */
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST) )
			return FALSE;
	(void) copy_fld_descript(fdesc, &descript);
	return TRUE;
	}

/**********************************************************************/

/**********************************************************************/
/** Reset the valid timestamp in a field descriptor based on
 * the closest valid timestamp for a given source and run time.
 *
 * @note that the valid times are matched with element, level or
 * valid time if these are set in the field descriptor!
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	macro		enumerated time dependence to match
 *	@param[in]	mtchtime	valid time to match
 * 	@return True if successful.
 **********************************************************************/
LOGICAL				closest_source_valid_time_reset

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	STRING			mtchtime
	)

	{
	FLD_DESCRIPT	descript;
	STRING			vtime;

	/* Return if no structure passed */
	if ( IsNull(fdesc) ) return FALSE;

	/* Return now if the file exists for this valid time */
	if ( !blank(find_meta_filename(fdesc)) ) return TRUE;

	/* Make copy of field descriptor for finding closest valid time */
	(void) copy_fld_descript(&descript, fdesc);
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, NullString, FpaF_END_OF_LIST) )
			return FALSE;

	/* Determine closest valid time (if possible) */
	if ( closest_source_valid_time(&descript, macro, mtchtime, &vtime) < 0 )
			return FALSE;

	/* Reset closest valid time in field descriptor */
	if ( !set_fld_descript(&descript, FpaF_VALID_TIME, vtime, FpaF_END_OF_LIST) )
			return FALSE;
	(void) copy_fld_descript(fdesc, &descript);
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   d a i l y _ f i e l d _ l o c a l _ t i m e s                      *
*   d a i l y _ f i e l d _ l o c a l _ t i m e s _ f r e e            *
*                                                                      *
*   Return list of valid timestamps at which to calculate daily fields *
*   based on a given input field centered at a given longitude.        *
*   (The _free() function frees the list of valid timestamps.)         *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return list of valid timestamps at which to calculate daily
 * fields based on a given input field centred at a given logitude.
 *
 *	@param[in]	*fdesc	 field descriptor for daily field
 *	@param[in]	*fdescin field descriptor for input field
 *	@param[in]	macro	 enumerated time dependence to match
 *	@param[in]	clon	 center longitude for field
 *	@param[out]	**list	 list of valid times at which to calculate daily field
 *  @return The size of the list.
 **********************************************************************/
int					daily_field_local_times

	(
	FLD_DESCRIPT	*fdesc,
	FLD_DESCRIPT	*fdescin,
	int				macro,
	float			clon,
	STRING			**list
	)

	{
	double							dhours;
	FLD_DESCRIPT					descript;
	int								nlocal, nfpa, nn, ll, macroin;
	STRING							*vtlocal, *vtfpa, vtime;
	int								byr, bjd, bhr, bmin, eyr, ejd, ehr, emin;
	int								lyr, ljd, lhr, lmin, lsec;
	LOGICAL							bloc, eloc, lloc;
	FpaConfigElementTimeDepStruct	*tdep;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullStringList;

	/* Initialize internal list of daily timestamps */
	nlocal  = 0;
	vtlocal = NullStringPtr;

	/* Return 0 if no structures passed */
	if ( IsNull(fdesc) || IsNull(fdescin) ) return 0;
	if ( IsNull(fdesc->edef) ) return 0;

	/* Get daily time information */
	tdep = fdesc->edef->elem_tdep;
	if ( tdep->time_dep != FpaC_DAILY )
		{
		(void) pr_error("Environ",
				"[daily_field_local_times] \"%s\" is not a Daily field!\n",
						SafeStr(fdesc->edef->name));
		return 0;
		}

	/* Set daily normal time for field */
	if ( !convert_value(tdep->units->name, tdep->normal_time, Hrs, &dhours) )
		{
		(void) pr_error("Environ",
				"[daily_field_local_times] Error in normal time for: \"%s\"\n",
						SafeStr(fdesc->edef->name));
		return 0;
		}

	/* Set field descriptor for determining daily timestamps */
	(void) copy_fld_descript(&descript, fdescin);
	(void) set_fld_descript(&descript, FpaF_VALID_TIME, FpaCblank,
										FpaF_END_OF_LIST);

	/* Get daily timestamps (if requested) */
	if ( !(macro ^ FpaC_TIMEDEP_ANY) || (macro & FpaC_DAILY) )
		{

		/* Get list of valid timestamps from input directory */
		nfpa = source_valid_time_list(&descript, FpaC_DAILY, &vtfpa);
		if ( nfpa >= 1 )
			{

			/* Add copy of unique valid timestamps to internal list */
			for ( nn=0; nn<nfpa; nn++ )
				{

				/* Check that timestamp is not in internal list */
				for ( ll=0; ll<nlocal; ll++ )
					{
					if ( same(vtlocal[ll], vtfpa[nn]) ) break;
					}
				if ( ll < nlocal) continue;

				/* Copy timestamp to the internal list */
				nlocal++;
				vtlocal = GETMEM(vtlocal, STRING, nlocal);
				vtlocal[nlocal-1] = strdup(vtfpa[nn]);
				}

			/* Free list of valid timestamps from input directory */
			nfpa = source_valid_time_list_free(&vtfpa, nfpa);
			}

		/* Diagnostic messages if no daily fields found */
		else
			{
			(void) pr_diag("Environ",
					"[daily_field_local_times] No daily fields for: \"%s %s\"\n",
							((fdescin->edef)? fdescin->edef->name: FpaCanyElement),
							((fdescin->ldef)? fdescin->ldef->name: FpaCanyLevel));
			(void) pr_diag("Environ",
					"                            from: \"%s %s\" at: \"%s\"\n",
							((fdescin->sdef)? fdescin->sdef->name: ""),
							((fdescin->subdef)? fdescin->subdef->name: ""),
							SafeStr(fdescin->rtime));
			}
		}

	/* Build list of daily timestamps (if requested) */
	if ( !(macro ^ FpaC_TIMEDEP_ANY)
			|| ( macro & FpaC_STATIC ) || ( macro & FpaC_NORMAL ) )
		{

		/* Set macro for getting list of valid timestamps */
		if ( !(macro ^ FpaC_TIMEDEP_ANY) )
			macroin = ( FpaC_STATIC | FpaC_NORMAL );
		else if ( ( macro & FpaC_STATIC ) && ( macro & FpaC_NORMAL ) )
			macroin = ( FpaC_STATIC | FpaC_NORMAL );
		else if ( ( macro & FpaC_STATIC ) )
			macroin = ( FpaC_STATIC );
		else
			macroin = ( FpaC_NORMAL );

		/* Get list of valid timestamps from input directory */
		nfpa = source_valid_time_list(&descript, macroin, &vtfpa);
		if ( nfpa >= 1 )
			{

			/* Determine local begin and end times from input valid times */
			if ( parse_tstamp(gmt_to_local(vtfpa[0], clon),
							&byr, &bjd, &bhr, &bmin, &bloc, NullLogicalPtr)
				&& parse_tstamp(gmt_to_local(vtfpa[nfpa-1], clon),
							&eyr, &ejd, &ehr, &emin, &eloc, NullLogicalPtr) )

				{

				/* Initialize local daily normal time */
				lyr  = byr;
				ljd  = bjd;
				lhr  = NINT(dhours);
				lmin = 0;
				lsec = 0;
				lloc = TRUE;

				/* Build local timestamps for daily fields within */
				/*  range of begin and end input valid times      */
				for ( ; ; ljd++ )
					{

					/* Jump out of loop when beyond end valid time */
					(void) tnorm(&lyr, &ljd, &lhr, &lmin, &lsec);
					if ( mdif(eyr, ejd, ehr, emin, lyr, ljd, lhr, lmin) > 0 )
						break;

					/* Add copy of unique valid timestamps to internal list */
					/*  when between begin and end valid time               */
					if ( mdif(byr, bjd, bhr, bmin, lyr, ljd, lhr, lmin) >= 0 )
						{

						/* Build local timestamp */
						vtime = build_tstamp(lyr, ljd, lhr, lmin, lloc, FALSE);

						/* Check that timestamp is not in internal list */
						for ( ll=0; ll<nlocal; ll++ )
							{
							if ( same(vtlocal[ll], vtime) ) break;
							}
						if ( ll < nlocal) continue;

						/* Copy timestamp to internal list */
						nlocal++;
						vtlocal = GETMEM(vtlocal, STRING, nlocal);
						vtlocal[nlocal-1] = strdup(vtime);
						}
					}
				}

			/* Error if problem parsing timestamps */
			else
				{
				(void) pr_error("Environ",
						"[daily_field_local_times] Error in valid times: \"%s\" or \"%s\"\n",
								SafeStr(vtfpa[0]), SafeStr(vtfpa[nfpa-1]));
				(void) pr_error("Environ",
						"                            from: \"%s %s\" at: \"%s\"\n",
								((fdescin->sdef)? fdescin->sdef->name: ""),
								((fdescin->subdef)? fdescin->subdef->name: ""),
								SafeStr(fdescin->rtime));
				}

			/* Free list of valid timestamps from input directory */
			nfpa = source_valid_time_list_free(&vtfpa, nfpa);
			}

		/* Diagnostic messages if no daily fields found */
		else
			{
			(void) pr_diag("Environ",
					"[daily_field_local_times] No normal fields for: \"%s %s\"\n",
							((fdescin->edef)? fdescin->edef->name: FpaCanyElement),
							((fdescin->ldef)? fdescin->ldef->name: FpaCanyLevel));
			(void) pr_diag("Environ",
					"                            from: \"%s %s\" at: \"%s\"\n",
							((fdescin->sdef)? fdescin->sdef->name: ""),
							((fdescin->subdef)? fdescin->subdef->name: ""),
							SafeStr(fdescin->rtime));
			}
		}

	/* Set pointer to list of local timestamps and */
	/*  return number of local timestamps          */
	if ( NotNull(list) ) *list = vtlocal;
	vtlocal = NullStringPtr;
	return nlocal;
	}

/**********************************************************************/

/**********************************************************************/
/** Free list of valid timestamps at which to calculate daily
 * fields.
 *
 *	@param[in]	**list		list of valid times
 *	@param[in]	num			size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int					daily_field_local_times_free

	(
	STRING			**list,
	int				num
	)

	{

	/* Free the list of valid times */
	return source_valid_time_list_free(list, num);
	}

/***********************************************************************
*                                                                      *
*   s o u r c e _ v a l i d _ r a n g e _ f o r _ d a i l y            *
*   s o u r c e _ v a l i d _ r a n g e _ f o r _ d a i l y _ f r e e  *
*                                                                      *
*   Return list of valid timestamps for calculating daily fields       *
*   based on a given input field centered at a given longitude.        *
*   (The _free() function frees the list of valid timestamps.)         *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return list of valid timestamps for calculating daily fields
 * based on a given input field centered at a given longitude.
 *
 *	@param[in]	*fdesc	 field descriptor for daily field
 *	@param[in]	*fdescin field descriptor for input field
 *	@param[in]	macro	 enumerated time dependence to match
 *	@param[in]	clon	 center longitude for field
 *	@param[in]	**list	 list of valid times for calculating daily field
 *  @return The size of the list.
 **********************************************************************/
int					source_valid_range_for_daily

	(
	FLD_DESCRIPT	*fdesc,
	FLD_DESCRIPT	*fdescin,
	int				macro,
	float			clon,
	STRING			**list
	)

	{
	int								lyr, ljd, lhr, lmin;
	int								byr, bjd, bhr, bmin, bsec;
	int								eyr, ejd, ehr, emin, esec;
	int								nvt;
	LOGICAL							lloc, status;
	double							dbgn, dend;
	FpaConfigElementTimeDepStruct	*tdep;

	/* Static buffers for begin and end times */
	static	STRING					btime = NullString;
	static	STRING					etime = NullString;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullStringList;

	/* Return 0 if no structures passed */
	if ( IsNull(fdesc) || IsNull(fdescin) ) return 0;

	/* Convert valid time to local date and time */
	if ( !parse_tstamp(fdesc->vtime, &lyr, &ljd, &lhr, &lmin, &lloc,
			NullLogicalPtr)
			|| !lloc )
		{
		(void) pr_error("Environ",
				"[source_valid_range_for_daily] Error in daily field valid time: \"%s\"\n",
						SafeStr(fdesc->vtime));
		return 0;
		}

	/* Set default begin and end hour if no field information passed */
	if ( IsNull(fdesc->edef) )
		{
		bhr =  0;
		ehr = 24;
		}

	/* Determine begin and end hour from field information */
	else
		{

		/* Get daily time information for field */
		tdep = fdesc->edef->elem_tdep;
		if ( tdep->time_dep != FpaC_DAILY )
			{
			(void) pr_error("Environ",
					"[source_valid_range_for_daily] \"%s\" is not a Daily field!\n",
							SafeStr(fdesc->edef->name));
			return 0;
			}

		/* Set begin and end times for daily field */
		if ( !convert_value(tdep->units->name, tdep->begin_time, Hrs, &dbgn)
				|| !convert_value(tdep->units->name, tdep->end_time, Hrs, &dend) )
			{
			(void) pr_error("Environ",
					"[source_valid_range_for_daily] Error in begin/end times for: \"%s\"\n",
							SafeStr(fdesc->edef->name));
			return 0;
			}
		bhr = NINT(dbgn);
		ehr = NINT(dend);
		}

	/* Check for local timestamps within begin and end time range */
	if ( lhr < bhr || lhr > ehr )
		{

		/* Adjust begin and end time for local time at start of day */
		if ( lhr >= (bhr-24) && lhr <= (ehr-24) )
			{
			bhr -=24;
			ehr -=24;
			}

		/* Adjust begin and end time for local time at end of day */
		else if ( lhr >= (bhr+24) && lhr <= (ehr+24) )
			{
			bhr +=24;
			ehr +=24;
			}

		/* Return if local time is outside begin and end time range */
		else
			{
			return 0;
			}
		}

	/* Convert begin time to GMT using year and day from local time */
	/*  and begin time for daily field                              */
	byr  = lyr;
	bjd  = ljd;
	bmin = 0;
	bsec = 0;
	(void) tnorm(&byr, &bjd, &bhr, &bmin, &bsec);
	if ( NotNull(btime) ) FREEMEM(btime);
	btime = strdup(local_to_gmt(build_tstamp(byr, bjd, bhr, bmin, TRUE, FALSE), clon));

	/* Convert end time to GMT using year and day from local time */
	/*  and end time for daily field                              */
	eyr  = lyr;
	ejd  = ljd;
	emin = 0;
	esec = 0;
	(void) tnorm(&eyr, &ejd, &ehr, &emin, &esec);
	if ( NotNull(etime) ) FREEMEM(etime);
	etime = strdup(local_to_gmt(build_tstamp(eyr, ejd, ehr, emin, TRUE, FALSE), clon));

	/* Determine valid timelist based on begin and end timestamps */
	nvt = source_valid_time_sublist(fdescin, macro, 0,
			btime, NullString, etime, list);

	/* Check valid times for all times outside range */
	if ( nvt == 1 )
		{
		bmin = calc_prog_time_minutes(btime, *(list[0]), &status);
		emin = calc_prog_time_minutes(etime, *(list[0]), &status);
		if ( (bmin < 0 && emin < 0) || (bmin > 0 && emin > 0) )
			return source_valid_time_sublist_free(list, nvt);
		}

	/* Return the valid timelist */
	return nvt;
	}

/**********************************************************************/

/**********************************************************************/
/** Free list of valid timestamps for calculating daily fields.
 *
 *	@param[in]	**list		list of valid times
 *	@param[in]	num			size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int					source_valid_range_for_daily_free

	(
	STRING			**list,
	int				num
	)

	{

	/* Free the list of valid times */
	return source_valid_time_list_free(list, num);
	}

/***********************************************************************
*                                                                      *
*   s o u r c e _ f i e l d _ l i s t                                  *
*   s o u r c e _ f i e l d _ l i s t _ f r e e                        *
*                                                                      *
*   Return pointers to Field structures for a given source and run     *
*   time.  Note that the fields are matched with element, level, or    *
*   valid time if these are set in the field descriptor!               *
*   (The _free() function frees the list of Field structure pointers.) *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return pointers to Field structures for a given source and
 * run time. Note that the fields are matched with element, level, or
 * valid time if these are set in the field descriptor!
 *
 *	@param[in]	*fdesc	field descriptor
 *	@param[in]	macro	enumerated time dependence to match
 *	@param[out] ***list	list of Field structures for given source/run time
 *  @return The size of the list.
 **********************************************************************/
int					source_field_list

	(
	FLD_DESCRIPT			*fdesc,
	int						macro,
	FpaConfigFieldStruct	***list
	)

	{
	int						nflds;
	FpaConfigFieldStruct	**fdefs;
	FpaConfigSourceStruct	*sdef;

	/* Initialize return parameter */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigFieldStruct **);

	/* Return 0 if no structure passed */
	if ( IsNull(fdesc) ) return 0;

	/* Set pointer to Source structure */
	sdef = fdesc->sdef;
	if ( IsNull(sdef) ) return 0;

	/* Return list of pointers to Field structures based on type of source */
	switch ( sdef->src_type )
		{

		/* Branch to Depictions, Guidance or Allied Model type files */
		case FpaC_DEPICTION:
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			nflds = data_directory_fields(fdesc, macro, &fdefs);
			if ( NotNull(list) ) *list = fdefs;
			return nflds;

		/* Return 0 for source types which do contain data files */
		default:
			return 0;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Free pointers to Field structures for a given source and
 * run time.
 *
 *	@param[in] ***list	list of Field structures
 *	@param[in]	num		size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int					source_field_list_free

	(
	FpaConfigFieldStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers to Field Structures */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/***********************************************************************
*                                                                      *
*   c h e c k _ s o u r c e _ m i n u t e s _ i n _ f i l e n a m e s  *
*                                                                      *
*   This function checks that filename timestamps in a given directory *
*   are consistent with the directory minutes required flag.           *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** This function checks that filename timestamps in a given
 * directory are consistent with the directory minutes required flag.
 *
 *	@param[in]	name		Source name for directory
 * 	@return True if filename timestamps are consistent.
 **********************************************************************/
LOGICAL				check_source_minutes_in_filenames

	(
	STRING			name
	)

	{
	LOGICAL			consistent, mins;
	int				nfpa, nn;
	STRING			*vtfpa, dirpath;
	FLD_DESCRIPT	descript;

	/* Initialize a field descriptor to hold the source information */
	(void) init_fld_descript(&descript);
	if ( !set_fld_descript(&descript,
							FpaF_SOURCE_NAME,    name,
							FpaF_END_OF_LIST) ) return TRUE;

	/* Get list of valid timestamps from input directory */
	nfpa = source_valid_time_list(&descript, FpaC_TIMEDEP_ANY, &vtfpa);

	/* Check each valid timestamp for consistency with minutes flag */
	consistent = TRUE;
	for ( nn=0; nn<nfpa; nn++ )
		{

		/* Parse the timestamp */
		(void) parse_tstamp(vtfpa[nn], NullInt, NullInt, NullInt, NullInt,
				NullLogical, &mins);

		/* Compare to the directory minutes flag */
		if ( descript.sdef->minutes_rqd && !mins )
			{
			dirpath = source_directory(&descript);
			(void) fprintf(stderr, "[check_source_minutes_in_filenames]");
			(void) fprintf(stderr, " Timestamps missing minutes in");
			(void) fprintf(stderr, " directory: \"%s\"\n", dirpath);
			(void) fprintf(stderr, "[check_source_minutes_in_filenames]");
			(void) fprintf(stderr, "  Run script \"fpadb_minutes %s\"\n",
					dirpath);
			consistent = FALSE;
			break;
			}
		else if ( !descript.sdef->minutes_rqd && mins )
			{
			dirpath = source_directory(&descript);
			(void) fprintf(stderr, "[check_source_minutes_in_filenames]");
			(void) fprintf(stderr, " Timestamps have minutes in");
			(void) fprintf(stderr, " directory: \"%s\"\n", dirpath);
			(void) fprintf(stderr, "[check_source_minutes_in_filenames]");
			(void) fprintf(stderr, "  May be problem with directory!\n");
			consistent = FALSE;
			break;
			}
		}

	/* Free list of valid timestamps from input directory */
	nfpa = source_valid_time_list_free(&vtfpa, nfpa);

	/* Return the results of consistency checking */
	return consistent;
	}

/***********************************************************************
*                                                                      *
*   s o u r c e _ p a t h _ b y _ n a m e                              *
*                                                                      *
*   This function creates a full path name from a named source         *
*   (and subsource), run time string, and file name.                   *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** This function creates a full path name from a named source
 * (and subsource), run time string, and file name.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	name		Source name for directory
 *	@param[in]	subname 	Subsource name for directory
 *	@param[in]	rtime		Run timestamp for directory
 *	@param[in]	ident		file identifier name
 * 	@return The full path name for a source file.
 **********************************************************************/
STRING				source_path_by_name

	(
	STRING			name,
	STRING			subname,
	STRING			rtime,
	STRING			ident
	)

	{
	STRING		dir;

	/* Get source directory */
	dir = source_directory_by_name(name, subname, rtime);

	/* Return full pathname for file */
	return pathname(dir, ident);
	}

/***********************************************************************
*                                                                      *
*   c o n s t r u c t _ f i l e _ i d e n t i f i e r                  *
*                                                                      *
*   This function creates a file identifier (new format) from a named  *
*   element, level, and (optionally) valid time.                       *
*                                                                      *
*   b u i l d _ f i l e _ i d e n t i f i e r                          *
*                                                                      *
*   This function creates a file identifier (old format) from a named  *
*   element, level, and (optionally) valid time.                       *
*                                                                      *
*   p a r s e _ f i l e _ i d e n t i f i e r                          *
*                                                                      *
*   This function returns pointers to Element and Level structures     *
*   (and possibly a valid timestamp) determined from parsing a file    *
*   identfier (new or old format).                                     *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** This function creates a file identifier (new format) from a named
 * element, level and (optionally) valid time.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	element 		element name
 *	@param[in]	level			level name
 *	@param[in]	vtime			vtime name
 * @return File identifier.
 **********************************************************************/
STRING				construct_file_identifier

	(
	STRING			element,
	STRING			level,
	STRING			vtime
	)

	{
	size_t						nelem, nlev, nvt;
	int							year, jday, hour, min, sec, nids;
	LOGICAL						local, mins;
	STRING						vstring;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;

	/* Static buffer for file identifiers */
	static	char	fileident[FILE_IDENT_LEN];

	/* Static buffers for diagnostic messages */
	static	int		NumElementFident = 0;
	static	STRING	*ElementFidents  = NullStringList;
	static	int		NumLevelFident   = 0;
	static	STRING	*LevelFidents    = NullStringList;

	/* Check for file identifier for the named element */
	edef = identify_element(element);
	if ( IsNull(edef) )
		{
		(void) pr_error("Environ",
			"[construct_file_identifier] Unknown element \"%s\"\n",
				SafeStr(element));
		return NullString;
		}
	if ( blank(edef->elem_io->fident) )
		{
		for (nids=0; nids<NumElementFident; nids++)
			{
			if (same(element, ElementFidents[nids])) break;
			}
		if (nids >= NumElementFident)
			{
			(void) pr_diag("Environ",
				"[construct_file_identifier] No \"file_ident\" for element \"%s\"\n",
					SafeStr(element));
			NumElementFident++;
			ElementFidents = GETMEM(ElementFidents, STRING, NumElementFident);
			ElementFidents[NumElementFident-1] = safe_strdup(element);
			}
		return NullString;
		}

	/* Check for file identifier for the named level */
	ldef = identify_level(level);
	if ( IsNull(ldef) )
		{
		(void) pr_error("Environ",
			"[construct_file_identifier] Unknown level \"%s\"\n",
				SafeStr(level));
		return NullString;
		}
	if ( blank(ldef->lev_io->fident) )
		{
		for (nids=0; nids<NumLevelFident; nids++)
			{
			if (same(level, LevelFidents[nids])) break;
			}
		if (nids >= NumLevelFident)
			{
			(void) pr_diag("Environ",
				"[construct_file_identifier] No \"file_ident\" for level \"%s\"\n",
					SafeStr(level));
			NumLevelFident++;
			LevelFidents = GETMEM(LevelFidents, STRING, NumLevelFident);
			LevelFidents[NumLevelFident-1] = safe_strdup(level);
			}
		return NullString;
		}

	/* Check that element and level are consistent */
	if ( !consistent_element_and_level(edef, ldef) )
		{
		(void) pr_error("Environ",
			"[construct_file_identifier] Inconsistent element \"%s\" and level \"%s\"\n",
				SafeStr(element), SafeStr(level));
		return NullString;
		}

	/* Check that the valid time is acceptable */
	if ( !blank(vtime) &&
			!parse_tstamp(vtime, &year, &jday, &hour, &min, &local, &mins) )
		{
		(void) pr_error("Environ",
			"[construct_file_identifier] Error in valid timestamp \"%s\"\n",
				SafeStr(vtime));
		return NullString;
		}

	/* Construct new format metafile timestring */
	if ( !blank(vtime) )
		{
		sec = 0;
		vstring = metafile_timestring(year, jday, hour, min, sec,
										local, mins, FALSE);
		nvt = strlen(vstring) + 1;
		}
	else
		{
		vstring = NullString;
		nvt = 0;
		}

	/* Check length of file identifier */
	nelem = strlen(edef->elem_io->fident);
	nlev  = strlen(ldef->lev_io->fident);
	if ( (int) (nelem + 1 + nlev + nvt) > FILE_IDENT_LEN )
		{
		(void) pr_error("Environ",
				"[construct_file_identifier] File identifier too long!\n");
		(void) pr_error("Environ",
				"     Element ident: \"%s\"  Level ident: \"%s\"  Valid time: \"%s\"\n",
						SafeStr(edef->elem_io->fident),
						SafeStr(ldef->lev_io->fident),
						SafeStr(vtime));
		return NullString;
		}

	/* Build the file identifier prefix */
	(void) strcpy(fileident, edef->elem_io->fident);
	(void) strcat(fileident, FpaFidentDelimiter);
	(void) strcat(fileident, ldef->lev_io->fident);

	/* Add the metafile timestring (if required) */
	if ( !blank(vstring) )
		{
		(void) strcat(fileident, FpaFidentDelimiter);
		(void) strcat(fileident, vstring);
		}

	/* Return the file identifier */
	return fileident;
	}

/**********************************************************************/

/**********************************************************************/
/** This function creates a file identifier (old format) from a named
 * element, level and (optionally) valid time.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	element 		element name
 *	@param[in]	level			level name
 *	@param[in]	vtime			vtime name
 * @return File identifier.
 **********************************************************************/
STRING				build_file_identifier

	(
	STRING			element,
	STRING			level,
	STRING			vtime
	)

	{
	size_t						nelem, nlev, nvt;
	int							year, jday, hour, min, nids;
	LOGICAL						local;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;

	/* Static buffer for file identifiers */
	static	char	fileid[FILE_ID_LEN + 1];

	/* Static buffers for diagnostic messages */
	static	int		NumElementFid = 0;
	static	STRING	*ElementFids  = NullStringList;
	static	int		NumLevelFid   = 0;
	static	STRING	*LevelFids    = NullStringList;

	/* Check for file identifier for the named element */
	edef = identify_element(element);
	if ( IsNull(edef) )
		{
		(void) pr_error("Environ",
			"[build_file_identifier] Unknown element \"%s\"\n",
				SafeStr(element));
		return NullString;
		}
	if ( blank(edef->elem_io->fid) )
		{
		for (nids=0; nids<NumElementFid; nids++)
			{
			if (same(element, ElementFids[nids])) break;
			}
		if (nids >= NumElementFid)
			{
			(void) pr_diag("Environ",
				"[build_file_identifier] No \"file_id\" for element \"%s\"\n",
					SafeStr(element));
			NumElementFid++;
			ElementFids = GETMEM(ElementFids, STRING, NumElementFid);
			ElementFids[NumElementFid-1] = safe_strdup(element);
			}
		return NullString;
		}

	/* Check for file identifier for the named level */
	ldef = identify_level(level);
	if ( IsNull(ldef) )
		{
		(void) pr_error("Environ",
			"[build_file_identifier] Unknown level \"%s\"\n",
				SafeStr(level));
		return NullString;
		}
	if ( blank(ldef->lev_io->fid) )
		{
		for (nids=0; nids<NumLevelFid; nids++)
			{
			if (same(level, LevelFids[nids])) break;
			}
		if (nids >= NumLevelFid)
			{
			(void) pr_diag("Environ",
				"[build_file_identifier] No \"file_id\" for level \"%s\"\n",
					SafeStr(level));
			NumLevelFid++;
			LevelFids = GETMEM(LevelFids, STRING, NumLevelFid);
			LevelFids[NumLevelFid-1] = safe_strdup(level);
			}
		return NullString;
		}

	/* Check that element and level are consistent */
	if ( !consistent_element_and_level(edef, ldef) )
		{
		(void) pr_error("Environ",
			"[build_file_identifier] Inconsistent element \"%s\" and level \"%s\"\n",
				SafeStr(element), SafeStr(level));
		return NullString;
		}

	/* Check that the valid time is acceptable */
	if ( !blank(vtime) &&
		 !parse_tstamp(vtime, &year, &jday, &hour, &min, &local,
				NullLogicalPtr) )
		{
		(void) pr_error("Environ",
			"[build_file_identifier] Error in valid timestamp \"%s\"\n",
				SafeStr(vtime));
		return NullString;
		}

	/* Check length of file identifier */
	nelem = strlen(edef->elem_io->fid);
	nlev  = strlen(ldef->lev_io->fid);
	if ( !blank(vtime) ) nvt = strlen(vtime) + 1;
	else                 nvt = 0;
	if ( (int) (nelem + nlev + nvt) > FILE_ID_LEN )
		{
		(void) pr_error("Environ",
				"[build_file_identifier] File identifier too long!\n");
		(void) pr_error("Environ",
				"     Element id: \"%s\"  Level id: \"%s\"  Valid time: \"%s\"\n",
						SafeStr(edef->elem_io->fid), SafeStr(ldef->lev_io->fid),
						SafeStr(vtime));
		return NullString;
		}

	/* Build the file identifier prefix */
	(void) strcpy(fileid, edef->elem_io->fid);
	(void) strcat(fileid, ldef->lev_io->fid);

	/* Add the valid timestamp (if passed) */
	if ( !blank(vtime) )
		{
		(void) strcat(fileid, FpaFvalidDelimiter);
		(void) strcat(fileid, vtime);
		}

	/* Return the file identifier */
	return fileid;
	}

/**********************************************************************/

/**********************************************************************/
/** This function returns pointers to Element and Level
 * structures (and possibly a valid timestamp) determined from parsing
 * a file identfier.
 *
 *	@param[in]	ident	file identifier
 *	@param[out] **edef	Element structure
 *	@param[out] **ldef	Level structure
 *	@param[out]	*vtime	valid timestamp
 *  @return True if successful.
 **********************************************************************/
LOGICAL				parse_file_identifier

	(
	STRING					ident,
	FpaConfigElementStruct	**edef,
	FpaConfigLevelStruct	**ldef,
	STRING					*vtime
	)

	{
	size_t						nlen, ndelim, ndelimx, nelm, nlev, nvt;
	int							year, jday, hour, min;
	LOGICAL						local;
	STRING						tident, vident, vstring;
	FpaConfigElementStruct		*edefx;
	FpaConfigLevelStruct		*ldefx;

	/* Static buffers for element, level, and valid time parts of identifier */
	static	char	elemfid[ELEM_IDENT_LEN + 1];
	static	char	levelfid[LEVEL_IDENT_LEN + 1];
	static	char	validtime[VALID_TIME_LEN + 1];

	/* Initialize return parameters */
	if ( NotNull(edef) )  *edef  = NullPtr(FpaConfigElementStruct *);
	if ( NotNull(ldef) )  *ldef  = NullPtr(FpaConfigLevelStruct *);
	if ( NotNull(vtime) ) *vtime = NullString;

	/* Return immediately if no file identifier */
	if ( blank(ident) ) return FALSE;

	/* Identify length of file identifier */
	nlen = safe_strlen(ident);

	/* Parse new format file identifiers */
	ndelim = strcspn(ident, FpaFidentDelimiter);
	if (ndelim < nlen)
		{

		/* Identify location of valid time (if another delimiter is found!) */
		tident  = ident + (int) ndelim + 1;
		ndelimx = strcspn(tident, FpaFidentDelimiter);

		/* Set length of element name, level name and valid time */
		nelm = ndelim;
		nlev = nlen - ndelim - 1;
		if (ndelimx < nlev)
			{
			nvt  = nlev - ndelimx - 1;
			nlev = nlev - nvt - 1;
			}
		else
			{
			nvt = 0;
			}

#		ifdef DEBUG_FILE_IDENTS
		(void) pr_diag("Environ",
				"[parse_file_identifier] New format file identifier \"%s\"!\n",
						SafeStr(ident));
		(void) pr_diag("Environ",
				"[parse_file_identifier]   Element/Level/ValidTime: %d %d %d\n",
						nelm, nlev, nvt);
#		endif /* DEBUG_FILE_IDENTS */

		/* Error message if file identifier is wrong length! */
		if ( (int) nelm > ELEM_IDENT_LEN || (int) nlev > LEVEL_IDENT_LEN
				|| (int) nvt > VALID_TIME_LEN )
			{
			(void) pr_error("Environ",
					"[parse_file_identifier] Error in length of file identifier \"%s\"!\n",
							SafeStr(ident));
			return FALSE;
			}

		/* Extract the element name from the file identifier */
		(void) strncpy(elemfid, ident, nelm);
		elemfid[nelm] = '\0';

		/* Extract the level name from the file identifier */
		(void) strncpy(levelfid, tident, nlev);
		levelfid[nlev] = '\0';

		/* Extract the valid timestamp (if it exists!) */
		if ( (int) nvt > 0 )
			{
			vident  = tident + (int) ndelimx + 1;
			vstring = interpret_timestring(vident, NullString, 0.0);
			if ( blank(vstring) )
				{
				(void) pr_error("Environ",
						"[parse_file_identifier] Error in valid time format: \"%s\"!\n",
								SafeStr(vident));
				return FALSE;
				}
			(void) strcpy(validtime, vstring);
			}
		else
			{
			validtime[0] = '\0';
			}

#		ifdef DEBUG_FILE_IDENTS
		(void) pr_diag("Environ",
				"[parse_file_identifier]   Element/Level/ValidTime: %s %s %s\n",
						SafeStr(elemfid), SafeStr(levelfid), SafeStr(validtime));
#		endif /* DEBUG_FILE_IDENTS */
		}

	/* Parse old format file identifiers */
	else
		{

		/* Identify location of valid time delimiter (if one is found!) */
		ndelim = strcspn(ident, FpaFvalidDelimiter);
		if ( (int) ndelim > ELEM_ID_LEN )
			nlev = ndelim - ELEM_ID_LEN;
		else
			nlev = 0;
		if ( nlen > ndelim )
			nvt = nlen - ndelim - 1;
		else
			nvt = 0;

		/* Error message if file identifier is wrong length! */
		if ( (int) nlen < ELEM_ID_LEN || (int) nlev > LEVEL_ID_LEN
				|| (int) nvt > VALID_TIME_LEN )
			{
			(void) pr_error("Environ",
					"[parse_file_identifier] Error in length of file identifier \"%s\"!\n",
							SafeStr(ident));
			return FALSE;
			}

		/* Extract the element name from the file identifier */
		/* Assume the element part is always ELEM_ID_LEN long! */
		(void) strncpy(elemfid, ident, ELEM_ID_LEN);
		elemfid[ELEM_ID_LEN] = '\0';

		/* Extract the level name from the file identifier */
		tident = ident + ELEM_ID_LEN;
		(void) strncpy(levelfid, tident, nlev);
		levelfid[nlev] = '\0';

		/* Extract the valid timestamp (if it exists!) */
		if ( (int) nvt > 0 )
			{
			vident = ident + (int) ndelim + 1;
			(void) strcpy(validtime, vident);
			}
		else
			{
			validtime[0] = '\0';
			}
		}

	/* Check for the element file identifier */
	edefx = identify_element(elemfid);
	if ( NotNull(edef) ) *edef = edefx;

	/* Check for the level file identifier */
	ldefx = identify_level(levelfid);
	if ( NotNull(ldef) ) *ldef = ldefx;

	/* Set the valid timestamp */
	if ( NotNull(vtime) ) *vtime = validtime;

	/* Return FALSE if element or level file identifier not found */
	if ( IsNull(edefx) )
		{
		(void) pr_error("Environ",
			"[parse_file_identifier] Unknown element \"file_id\" - \"%s\"\n",
				SafeStr(elemfid));
		}
	if ( IsNull(ldefx) )
		{
		(void) pr_error("Environ",
			"[parse_file_identifier] Unknown level \"file_id\" - \"%s\"\n",
				SafeStr(levelfid));
		}
	if ( IsNull(edefx) || IsNull(ldefx) ) return FALSE;

	/* Return FALSE if element and level are not consistent */
	if ( !consistent_element_and_level(edefx, ldefx) )
		{
		(void) pr_error("Environ",
			"[parse_file_identifier] Inconsistent element \"%s\" and level \"%s\"\n",
				SafeStr(edefx->name), SafeStr(ldefx->name));
		return FALSE;
		}

	/* Return FALSE if problem with valid timestamp */
	if ( !blank(validtime) &&
		 !parse_tstamp(validtime, &year, &jday, &hour, &min, &local,
				NullLogicalPtr) )
		{
		(void) pr_error("Environ",
			"[parse_file_identifier] Error in valid timestamp \"%s\"\n",
				validtime);
		return FALSE;
		}

	/* Return TRUE if all is OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   c o n s t r u c t _ l i n k _ i d e n t i f i e r                  *
*                                                                      *
*   This function creates a file identifier (new format) for a link    *
*   file from a named element and level.                               *
*                                                                      *
*   p a r s e _ l i n k _ i d e n t i f i e r                          *
*                                                                      *
*   This function returns pointers to Element and Level structures     *
*   determined from parsing a link file identfier (new format).        *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** This function creates a file identifier (new format) for a link
 * file from a named element and level.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	element 		element name
 *	@param[in]	level			level name
 * @return Link file identifier.
 **********************************************************************/
STRING				construct_link_identifier

	(
	STRING			element,
	STRING			level
	)

	{
	size_t						nelem, nlev, nlink;
	int							nids;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;

	/* Static buffer for link file identifiers */
	static	char	linkident[FILE_IDENT_LEN];

	/* Static buffers for diagnostic messages */
	static	int		NumElementFident = 0;
	static	STRING	*ElementFidents  = NullStringList;
	static	int		NumLevelFident   = 0;
	static	STRING	*LevelFidents    = NullStringList;

	/* Check for file identifier for the named element */
	edef = identify_element(element);
	if ( IsNull(edef) )
		{
		(void) pr_error("Environ",
			"[construct_link_identifier] Unknown element \"%s\"\n",
				SafeStr(element));
		return NullString;
		}
	if ( blank(edef->elem_io->fident) )
		{
		for (nids=0; nids<NumElementFident; nids++)
			{
			if (same(element, ElementFidents[nids])) break;
			}
		if (nids >= NumElementFident)
			{
			(void) pr_diag("Environ",
				"[construct_link_identifier] No \"file_ident\" for element \"%s\"\n",
					SafeStr(element));
			NumElementFident++;
			ElementFidents = GETMEM(ElementFidents, STRING, NumElementFident);
			ElementFidents[NumElementFident-1] = safe_strdup(element);
			}
		return NullString;
		}

	/* Check for file identifier for the named level */
	ldef = identify_level(level);
	if ( IsNull(ldef) )
		{
		(void) pr_error("Environ",
			"[construct_link_identifier] Unknown level \"%s\"\n",
				SafeStr(level));
		return NullString;
		}
	if ( blank(ldef->lev_io->fident) )
		{
		for (nids=0; nids<NumLevelFident; nids++)
			{
			if (same(level, LevelFidents[nids])) break;
			}
		if (nids >= NumLevelFident)
			{
			(void) pr_diag("Environ",
				"[construct_link_identifier] No \"file_ident\" for level \"%s\"\n",
					SafeStr(level));
			NumLevelFident++;
			LevelFidents = GETMEM(LevelFidents, STRING, NumLevelFident);
			LevelFidents[NumLevelFident-1] = safe_strdup(level);
			}
		return NullString;
		}

	/* Check that element and level are consistent */
	if ( !consistent_element_and_level(edef, ldef) )
		{
		(void) pr_error("Environ",
			"[construct_link_identifier] Inconsistent element \"%s\" and level \"%s\"\n",
				SafeStr(element), SafeStr(level));
		return NullString;
		}

	/* Check length of link file identifier */
	nelem = strlen(edef->elem_io->fident);
	nlev  = strlen(ldef->lev_io->fident);
	nlink = strlen(FpaFile_Links);
	if ( (int) (nelem + 1 + nlev + 1 + nlink) > FILE_IDENT_LEN )
		{
		(void) pr_error("Environ",
				"[construct_link_identifier] File identifier too long!\n");
		(void) pr_error("Environ",
				"     Element ident: \"%s\"  Level ident: \"%s\"  Link suffix: \"%s\"\n",
						SafeStr(edef->elem_io->fident),
						SafeStr(ldef->lev_io->fident), FpaFile_Links);
		return NullString;
		}

	/* Build the link file identifier prefix */
	(void) strcpy(linkident, edef->elem_io->fident);
	(void) strcat(linkident, FpaFidentDelimiter);
	(void) strcat(linkident, ldef->lev_io->fident);

	/* Add the link file suffix */
	(void) strcat(linkident, FpaFidentDelimiter);
	(void) strcat(linkident, FpaFile_Links);

	/* Return the link file identifier */
	return linkident;
	}

/**********************************************************************/

/**********************************************************************/
/** This function returns pointers to Element and Level structures
 * determined from parsing a link file identfier.
 *
 *	@param[in]	ident	link file identifier
 *	@param[out] **edef	Element structure
 *	@param[out] **ldef	Level structure
 *  @return True if successful.
 **********************************************************************/
LOGICAL				parse_link_identifier

	(
	STRING					ident,
	FpaConfigElementStruct	**edef,
	FpaConfigLevelStruct	**ldef
	)

	{
	size_t						nlen, ndelim, ndelimx, nelm, nlev, nlink;
	int							year, jday, hour, min;
	LOGICAL						local;
	STRING						tident, lident;
	FpaConfigElementStruct		*edefx;
	FpaConfigLevelStruct		*ldefx;

	/* Static buffers for element and level parts of identifier */
	static	char	elemfid[ELEM_IDENT_LEN + 1];
	static	char	levelfid[LEVEL_IDENT_LEN + 1];
	static	char	lsuffix[VALID_TIME_LEN + 1];

	/* Initialize return parameters */
	if ( NotNull(edef) )  *edef  = NullPtr(FpaConfigElementStruct *);
	if ( NotNull(ldef) )  *ldef  = NullPtr(FpaConfigLevelStruct *);

	/* Return immediately if no link file identifier */
	if ( blank(ident) ) return FALSE;

	/* Identify length of link file identifier */
	nlen = safe_strlen(ident);

	/* Parse new format link file identifiers */
	ndelim = strcspn(ident, FpaFidentDelimiter);
	if (ndelim < nlen)
		{

		/* Identify location of link suffix (if another delimiter is found!) */
		tident  = ident + (int) ndelim + 1;
		ndelimx = strcspn(tident, FpaFidentDelimiter);

		/* Set length of element name, level name and link suffix */
		nelm = ndelim;
		nlev = nlen - ndelim - 1;
		if (ndelimx < nlev)
			{
			nlink = nlev - ndelimx - 1;
			nlev  = nlev - nlink - 1;
			}
		else
			{
			nlink = 0;
			}

#		ifdef DEBUG_FILE_IDENTS
		(void) pr_diag("Environ",
				"[parse_link_identifier] New format file identifier \"%s\"!\n",
						SafeStr(ident));
		(void) pr_diag("Environ",
				"[parse_link_identifier]   Element/Level/LinkSuffix: %d %d %d\n",
						nelm, nlev, nlink);
#		endif /* DEBUG_FILE_IDENTS */

		/* Error message if file identifier is wrong length! */
		if ( (int) nelm > ELEM_IDENT_LEN || (int) nlev > LEVEL_IDENT_LEN
				|| (int) nlink > VALID_TIME_LEN )
			{
			(void) pr_error("Environ",
					"[parse_link_identifier] Error in length of file identifier \"%s\"!\n",
							SafeStr(ident));
			return FALSE;
			}

		/* Extract the element name from the file identifier */
		(void) strncpy(elemfid, ident, nelm);
		elemfid[nelm] = '\0';

		/* Extract the level name from the file identifier */
		(void) strncpy(levelfid, tident, nlev);
		levelfid[nlev] = '\0';

		/* Extract the link file suffix */
		if ( (int) nlink > 0 )
			{
			lident  = tident + (int) ndelimx + 1;
			(void) strncpy(lsuffix, ident, nelm);
			if ( !same_ic(lsuffix, FpaFile_Links) )
				{
				(void) pr_error("Environ",
						"[parse_link_identifier] Error in link file suffix: \"%s\"!\n",
								SafeStr(lsuffix));
				return FALSE;
				}
			}
		else
			{
			(void) pr_error("Environ",
					"[parse_link_identifier] Missing link file suffix!\n");
			return FALSE;
			}

#		ifdef DEBUG_FILE_IDENTS
		(void) pr_diag("Environ",
				"[parse_link_identifier]   Element/Level/LinkSuffix: %s %s %s\n",
						SafeStr(elemfid), SafeStr(levelfid), SafeStr(lsuffix));
#		endif /* DEBUG_FILE_IDENTS */
		}

	/* Check for the element file identifier */
	edefx = identify_element(elemfid);
	if ( NotNull(edef) ) *edef = edefx;

	/* Check for the level file identifier */
	ldefx = identify_level(levelfid);
	if ( NotNull(ldef) ) *ldef = ldefx;

	/* Return FALSE if element or level file identifier not found */
	if ( IsNull(edefx) )
		{
		(void) pr_error("Environ",
			"[parse_link_identifier] Unknown element \"file_id\" - \"%s\"\n",
				SafeStr(elemfid));
		}
	if ( IsNull(ldefx) )
		{
		(void) pr_error("Environ",
			"[parse_link_identifier] Unknown level \"file_id\" - \"%s\"\n",
				SafeStr(levelfid));
		}
	if ( IsNull(edefx) || IsNull(ldefx) ) return FALSE;

	/* Return FALSE if element and level are not consistent */
	if ( !consistent_element_and_level(edefx, ldefx) )
		{
		(void) pr_error("Environ",
			"[parse_link_identifier] Inconsistent element \"%s\" and level \"%s\"\n",
				SafeStr(edefx->name), SafeStr(ldefx->name));
		return FALSE;
		}

	/* Return TRUE if all is OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ m e t a _ f i l e n a m e                    *
*                                                                      *
*     Build the name of the metafile (new format) that should contain  *
*     the given field, whether or not it exists or contains the field. *
*                                                                      *
*     b u i l d _ m e t a _ f i l e n a m e                            *
*                                                                      *
*     Build the name of the metafile (old format) that should contain  *
*     the given field, whether or not it exists or contains the field. *
*                                                                      *
*     c h e c k _ m e t a _ f i l e n a m e                            *
*                                                                      *
*     Build the name of the metafile that should contain the given     *
*     field, then check to see if it exists and contains the field.    *
*     (This routine looks for an exact match in valid time!)           *
*                                                                      *
*     f i n d _ m e t a _ f i l e n a m e                              *
*                                                                      *
*     Build the name of the metafile that should contain the field     *
*     from which the given data can be extracted, then check to see    *
*     if it exists and contains the field.                             *
*     (This routine also checks for "matching" metafiles for daily     *
*      or static fields!)                                              *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Build the name of the metafile that should contain the
 * given field, whether or not it exists or contains the field.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 *  @return The metafile name.
 **********************************************************************/
STRING				construct_meta_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	LOGICAL							useminutes;
	STRING							dirpath, rstamp, vstamp, ident;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceSubStruct		*subdef;
	FpaConfigSourceIOStruct			*sio;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Return Null if no element or level passed */
	if ( IsNull(fdesc->edef) ) return NullString;
	if ( IsNull(fdesc->ldef) ) return NullString;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NullString;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Set the minutes flag from the source directory */
	useminutes = sdef->minutes_rqd;

	/* Build file name based on type of source */
	switch ( sdef->src_type )
		{

		/* Branch to Depiction type files */
		case FpaC_DEPICTION:
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, FpaCblank);
			if ( useminutes )
				vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
			else
				vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);
			ident   = construct_file_identifier(fdesc->edef->name,
												fdesc->ldef->name, vstamp);
			return data_file_path(dirpath, ident);

		/* Branch to Guidance or Allied Model type files */
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			if ( useminutes )
				rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
			else
				rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, rstamp);
			if ( useminutes )
				vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
			else
				vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);
			ident   = construct_file_identifier(fdesc->edef->name,
												fdesc->ldef->name, vstamp);
			return data_file_path(dirpath, ident);

		/* Branch to Map type files */
		case FpaC_MAPS:
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, FpaCblank);
			return map_file_path(dirpath, fdesc->edef, fdesc->ldef);

		/* Unsupported source type */
		default:
			return NullString;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Build the name of the metafile that should contain the
 * given field, whether or not it exists or contains the field.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 *  @return The metafile name.
 **********************************************************************/
STRING				build_meta_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	LOGICAL							useminutes;
	STRING							dirpath, rstamp, vstamp, ident;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceSubStruct		*subdef;
	FpaConfigSourceIOStruct			*sio;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Return Null if no element or level passed */
	if ( IsNull(fdesc->edef) ) return NullString;
	if ( IsNull(fdesc->ldef) ) return NullString;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NullString;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Set the minutes flag from the source directory */
	useminutes = sdef->minutes_rqd;

	/* Build file name based on type of source */
	switch ( sdef->src_type )
		{

		/* Branch to Depiction type files */
		case FpaC_DEPICTION:
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, FpaCblank);
			if ( useminutes )
				vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
			else
				vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);
			ident   = build_file_identifier(fdesc->edef->name,
											fdesc->ldef->name, vstamp);
			return data_file_path(dirpath, ident);

		/* Branch to Guidance or Allied Model type files */
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			if ( useminutes )
				rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
			else
				rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, rstamp);
			if ( useminutes )
				vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
			else
				vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);
			ident   = build_file_identifier(fdesc->edef->name,
											fdesc->ldef->name, vstamp);
			return data_file_path(dirpath, ident);

		/* Branch to Map type files */
		case FpaC_MAPS:
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, FpaCblank);
			return map_file_path(dirpath, fdesc->edef, fdesc->ldef);

		/* Unsupported source type */
		default:
			return NullString;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Build the name of the metafile that should contain the
 * given field, then check to see if it exists and contains the field.
 * This field looks for an exact match in valid time!
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return The metafile name. NullString if not valid.
 **********************************************************************/
STRING				check_meta_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING					path;
	int						nn, flist;
	FpaConfigFieldStruct	**fdefs;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Compose the new format filename for the given field */
	path = construct_meta_filename(fdesc);

#	ifdef DEBUG_FILE_IDENTS
	if ( !blank(path) )
		(void) pr_diag("Environ", "New format metaname: \"%s\"\n", path);
#	endif /* DEBUG_FILE_IDENTS */

	/* Check if the file cannot be found or does not exist */
	if ( blank(path) || !find_file(path) )
		{

		/* Try the old format filename for the given field */
		path = build_meta_filename(fdesc);

#		ifdef DEBUG_FILE_IDENTS
		if ( !blank(path) )
			(void) pr_diag("Environ", "Old format metaname: \"%s\"\n", path);
#		endif /* DEBUG_FILE_IDENTS */

		/* Return Null if the file cannot be found or does not exist */
		if ( blank(path) || !find_file(path) ) return NullString;
		}

	/* Now check whether file contains the requested field */
	flist = search_meta_fields(path, &fdefs);
	for ( nn=0; nn<flist; nn++ )
		{
		if ( fdesc->fdef == fdefs[nn] ) return path;
		}

	/* Return Null if field cannot be found in file */
	if ( IsNull(fdesc->fdef) ) return NullString;
	(void) pr_warning("Environ",
		"[check_meta_filename] Cannot find field: \"%s %s\"\n",
			((fdesc->fdef->element)? fdesc->fdef->element->name: ""),
			((fdesc->fdef->level)?   fdesc->fdef->level->name:   ""));
	(void) pr_warning("Environ",
		"[check_meta_filename]   in file: %s\n", path);
	return NullString;
	}

/**********************************************************************/

/**********************************************************************/
/** Build the name of the metafile that should contain the
 * field from which the given data can be extracted, then check to see
 * if it exists and contains the field. This routine also checks for
 * "matching" metafiles for daily or static fields!
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return The metafile name. NullString if not valid.
 **********************************************************************/
STRING				find_meta_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING							path, vtlcl, vstamp;
	FLD_DESCRIPT					descript;
	int								year, jday, hour, min;
	double							vhour, bhour, ehour;
	int								tyear, tjday, thour, tmin;
	LOGICAL							local, tloc;
	int								istime, nstime, ndiff;
	STRING							*stimes;
	FpaConfigSourceStruct			*sdef;
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementStruct			*edef;
	FpaConfigElementTimeDepStruct	*tdep;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Try to find the file as described by the field descriptor */
	path = check_meta_filename(fdesc);
	if ( !blank(path) ) return path;

	/* Set pointer to Source structure */
	sdef = fdesc->sdef;
	if ( IsNull(sdef) ) return NullString;

	/* Set pointer to Field structure */
	fdef = fdesc->fdef;
	if ( IsNull(fdef) ) return NullString;

	/* Set pointer to ElementTimeDep structure */
	edef = fdef->element;
	if ( IsNull(edef) ) return NullString;
	tdep = edef->elem_tdep;
	if ( IsNull(tdep) ) return NullString;

	/* Check for file based on time dependence */
	switch ( tdep->time_dep )
		{

		/* Check for FpaC_DAILY fields */
		case FpaC_DAILY:

			/* Convert valid time to LMT (if required) */
			vtlcl = gmt_to_local(fdesc->vtime, fdesc->mproj.clon);
			if ( !parse_tstamp(vtlcl, &year, &jday, &hour, &min,
						&local, NullLogicalPtr) ) return NullString;
			(void) tnorm(&year, &jday, &hour, &min, NullInt);
			vhour = (double) hour + ((double) min / 60.0);

			/* Get the acceptable time range for this field (in hours) */
			/* The time range is adjusted so that the begin time will  */
			/*  "match" with the valid time given above!               */
			if ( !convert_value(tdep->units->name, tdep->begin_time,
									Hrs, &bhour) ) return NullString;
			if ( !convert_value(tdep->units->name, tdep->end_time,
									Hrs, &ehour) ) return NullString;
			while ( ehour < bhour )      ehour += 24.0;
			while ( ehour-bhour > 24.0 ) ehour -= 24.0;
			while ( vhour > ehour )
				{
				bhour += 24.0;
				ehour += 24.0;
				}
			while ( vhour < bhour )
				{
				bhour -= 24.0;
				ehour -= 24.0;
				}

			/* Make sure the requested valid time is within the acceptable */
			/*  time range before we worry about whether the days match    */
			if ( vhour < bhour || vhour > ehour) return NullString;

			/* Get a list of valid times for this field */
			(void) copy_fld_descript(&descript, fdesc);
			if ( !set_fld_descript(&descript, FpaF_VALID_TIME, FpaCblank,
									FpaF_END_OF_LIST) ) return NullString;
			nstime = source_valid_time_list(&descript, FpaC_DAILY, &stimes);
			if ( nstime <= 0 ) return NullString;

			/* Search for a field from the same day as the requested field */
			for ( istime=0; istime<nstime; istime++ )
				{
				if ( !parse_tstamp(stimes[istime], &tyear, &tjday, &thour,
							&tmin, &tloc, NullLogicalPtr) )
					{
					nstime = source_valid_time_list_free(&stimes, nstime);
					return NullString;
					}
				ndiff = jdif(tyear, tjday, year, jday);

				/* Matching day found ... so find path */
				if ( ndiff == 0 )
					{
					if ( !set_fld_descript(&descript,
											FpaF_VALID_TIME, stimes[istime],
											FpaF_END_OF_LIST) )
						{
						nstime = source_valid_time_list_free(&stimes, nstime);
						return NullString;
						}
					path = check_meta_filename(&descript);
					break;
					}
				}

			/* Return path (or Null if no matching day found) */
			(void) source_valid_time_list_free(&stimes, nstime);
			return (istime < nstime) ? path: NullString;

		/* Check for FpaC_STATIC fields */
		case FpaC_STATIC:

			/* Get a list of valid times for this field */
			(void) copy_fld_descript(&descript, fdesc);
			if ( !set_fld_descript(&descript, FpaF_VALID_TIME, FpaCblank,
									FpaF_END_OF_LIST) ) return NullString;
			nstime = source_valid_time_list(&descript, FpaC_STATIC, &stimes);
			if ( nstime <= 0 ) return NullString;

			/* Find preceding valid time from list of valid times */
			for ( istime=nstime-1; istime>=0; istime-- )
				{

				/* Compare valid time (with minutes correctly encoded) */
				if ( sdef->minutes_rqd )
					vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
				else
					vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);
				if ( strcmp(vstamp, stimes[istime]) < 0) continue;

				/* Preceding valid time found ... so find path */
				if ( !set_fld_descript(&descript,
										FpaF_VALID_TIME, stimes[istime],
										FpaF_END_OF_LIST) )
					{
					nstime = source_valid_time_list_free(&stimes, nstime);
					return NullString;
					}
				path = check_meta_filename(&descript);
				break;
				}

			/* Return path or Null (if no preceding valid time found) */
			nstime = source_valid_time_list_free(&stimes, nstime);
			return (istime >= 0) ? path: NullString;

		/* Return Null for all other types of fields */
		default:
			return NullString;
		}
	}

/***********************************************************************
*                                                                      *
*   b u i l d _ a l l i e d _ f i l e n a m e                          *
*                                                                      *
*   Build the name of the file that should contain the given Allied    *
*   Model data, whether or not it exists.                              *
*                                                                      *
*   c h e c k _ a l l i e d _ f i l e n a m e                          *
*                                                                      *
*   Build the name of the file that should contain the given Allied    *
*   Model data, then check to see if it exists.                        *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Build the name of the file that should contain the given
 * Allied Model data, whether or not it exists.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	type		type of Allied Model data
 *	@param[in]	alias		alias for Allied Model data
 * 	@return The name of the allied model data file.
 **********************************************************************/
STRING				build_allied_filename

	(
	FLD_DESCRIPT	*fdesc,
	int				type,
	STRING			alias
	)

	{
	int								nloc;
	STRING							dirpath, rstamp;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceIOStruct			*sio;
	FpaConfigSourceSubStruct		*subdef;
	FpaConfigAlliedProgramsStruct	*programs;
	FpaConfigAlliedFilesStruct		*files;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Set pointers to Source, SourceIO and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) || IsNull(sdef->allied) )
			return NullString;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Get location of information based on type of Allied Model data */
	nloc = source_allied_data_location(sdef, type, alias);
	if ( nloc < 0 ) return NullString;

	/* Build path name based on type of Allied Model data */
	switch ( type )
		{

		/* Branch to Allied Model programs                            */
		/* Note that programs have no subsource or runtime dependence */
		case FpaC_ALLIED_PROGRAMS:
			programs = sdef->allied->programs;
			dirpath  = find_data_directory(programs->src_tags[nloc],
						sio->src_path, FpaCblank, FpaCblank);
			return data_file_path(dirpath, programs->prog_paths[nloc]);

		/* Branch to Allied Model files */
		case FpaC_ALLIED_FILES:
			files   = sdef->allied->files;
			if ( sdef->minutes_rqd )
				rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
			else
				rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);
			dirpath = find_data_directory(files->src_tags[nloc],
						sio->src_path, subdef->sub_path, rstamp);
			return data_file_path(dirpath, files->file_paths[nloc]);

		/* Return Null for all other data types */
		default:
			return NullString;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Build the name of the file that should contain the given
 * Allied Model data, then check to see if it exists.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 *	@param[in]	type		type of Allied Model data
 *	@param[in]	alias		alias for Allied Model data
 * 	@return The name of the Allied Model data file. NullString if it's
 * 			not valid.
 **********************************************************************/
STRING				check_allied_filename

	(
	FLD_DESCRIPT	*fdesc,
	int				type,
	STRING			alias
	)

	{
	STRING			path;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Determine the normal filename for the Allied Model data */
	path = build_allied_filename(fdesc, type, alias);

	/* Return Null if filename cannot be found */
	if ( blank(path) ) return NullString;

	/* Return Null if file cannot be found */
	if ( !find_file(path) ) return NullString;

	/* Return filename */
	return path;
	}

/***********************************************************************
*                                                                      *
*     c o n s t r u c t _ l i n k _ f i l e n a m e                    *
*                                                                      *
*     Build the name of a link file whether or not it exists.          *
*     Note that all link files are new format files.                   *
*                                                                      *
*     c h e c k _ l i n k _ f i l e n a m e                            *
*                                                                      *
*     Build the name of a link file, then check to see if it exists    *
*     and contains a set of link chains.                               *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Build the name of a new format link file, whether or not it exists.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 *  @return The link file name.
 **********************************************************************/
STRING				construct_link_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING							dirpath, ident, rstamp;
	FpaConfigSourceStruct			*sdef;
	FpaConfigSourceSubStruct		*subdef;
	FpaConfigSourceIOStruct			*sio;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Return Null if no element or level passed */
	if ( IsNull(fdesc->edef) ) return NullString;
	if ( IsNull(fdesc->ldef) ) return NullString;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NullString;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Build file name based on type of source */
	switch ( sdef->src_type )
		{

		/* Branch to Depiction type files */
		case FpaC_DEPICTION:
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, FpaCblank);
			ident   = construct_link_identifier(fdesc->edef->name,
													fdesc->ldef->name);
			return data_file_path(dirpath, ident);

		/* Branch to Guidance or Allied Model type files */
		case FpaC_GUIDANCE:
		case FpaC_ALLIED:
			if ( sdef->minutes_rqd )
				rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
			else
				rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);
			dirpath = find_data_directory(sio->src_tag, sio->src_path,
											subdef->sub_path, rstamp);
			ident   = construct_link_identifier(fdesc->edef->name,
													fdesc->ldef->name);
			return data_file_path(dirpath, ident);

		/* Unsupported source type */
		default:
			return NullString;
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Build the name of a new format link file, then check to see if it
 * exists and contains a set of link chains.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	*fdesc		field descriptor
 * 	@return The link file name. NullString if not valid.
 **********************************************************************/
STRING				check_link_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	STRING					path;
	int						nn, flist;
	FpaConfigFieldStruct	**fdefs;

	/* Return Null if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Compose the new format link name for the given field */
	path = construct_link_filename(fdesc);

#	ifdef DEBUG_FILE_IDENTS
	if ( !blank(path) )
		(void) pr_diag("Environ", "New format link name: \"%s\"\n", path);
#	endif /* DEBUG_FILE_IDENTS */

	/* Return Null if the link file cannot be found or does not exist */
	if ( blank(path) || !find_file(path) ) return NullString;

	/* Now check whether link file contains a set of link chains */
	flist = search_link_fields(path, &fdefs);
	for ( nn=0; nn<flist; nn++ )
		{
		if ( fdesc->fdef == fdefs[nn] ) return path;
		}

	/* Return Null if set of link chains cannot be found in link file */
	if ( IsNull(fdesc->fdef) ) return NullString;
	(void) pr_warning("Environ",
		"[check_link_filename] Cannot find link chains for field: \"%s %s\"\n",
			((fdesc->fdef->element)? fdesc->fdef->element->name: ""),
			((fdesc->fdef->level)?   fdesc->fdef->level->name:   ""));
	(void) pr_warning("Environ",
		"[check_link_filename]   in file: %s\n", path);
	return NullString;
	}

/***********************************************************************
*                                                                      *
*   b a c k g r o u n d _ f i l e                                      *
*                                                                      *
*   Return the full filename of a Maps type file.                      *
*                                                                      *
*   d e p i c t i o n _ s c r a t c h _ f i l e                        *
*   d e p i c t i o n _ l i n k _ f i l e                              *
*   n a m e d _ d e p i c t i o n _ f i l e                            *
*                                                                      *
*   Return the full filename of a Depiction type file.                 *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Return the full filename of a Maps type file.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	name		map name
 * 	@return Full pathname to Map type file requested.
 **********************************************************************/
STRING				background_file

	(
	STRING			name
	)

	{
	STRING	dir;

	/* Search the common map directory for the map file */
	if ( name[0] == '+' )
		{
		dir = FpaDir_CommonMaps;
		return get_file(dir, name+1);
		}

	/* Search the default map directory for the map file    */
	/* Note that absolute pathnames are returned unchanged! */
	else
		{
		dir = FpaDir_Maps;
		return get_file(dir, env_sub(name));
		}
	}

/**********************************************************************/

/**********************************************************************/
/** Return full filename of the Scratchpad file in the standard
 * depiction directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return Full pathname to the Scratchpad file.
 **********************************************************************/
STRING				depiction_scratch_file

	(
	)

	{
	FpaConfigSourceStruct		*sdef;

	/* Check for the standard depiction directory */
	sdef = identify_source(FpaDir_Depict, FpaCblank);
	if ( IsNull(sdef) ) return NullString;

	/* Return the Scratchpad file in the standard depiction directory */
	return depiction_file(sdef, FpaFile_Scratch);
	}

/**********************************************************************/

/**********************************************************************/
/** Return full filename of the Links file in the standard
 * depiction directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 * @return Full pathname to the Links file.
 **********************************************************************/
STRING				depiction_link_file

	(
	)

	{
	FpaConfigSourceStruct		*sdef;

	/* Check for the standard depiction directory */
	sdef = identify_source(FpaDir_Depict, FpaCblank);
	if ( IsNull(sdef) ) return NullString;

	/* Return the Links file in the standard depiction directory */
	return depiction_file(sdef, FpaFile_Links);
	}

/**********************************************************************/

/**********************************************************************/
/** Return full filename of a metafile in the standard
 * depiction directory.
 *
 * @note returned value is stored in a static variable within function
 * if you are not going to use it immediately it is safest to make a
 * copy with safe_strcpy or safe_strdup.
 *
 *	@param[in]	fname		depiction file name
 * 	@return Full pathname to the metafile.
 **********************************************************************/
STRING				named_depiction_file

	(
	STRING	fname
	)

	{
	FpaConfigSourceStruct		*sdef;

	/* Return if no depiction filename passed */
	if (blank(fname)) return NullString;

	/* Check for the standard depiction directory */
	sdef = identify_source(FpaDir_Depict, FpaCblank);
	if ( IsNull(sdef) ) return NullString;

	/* Return the filename in the standard depiction directory */
	return depiction_file(sdef, fname);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Files and Filenames)                    *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   d a t a _ f i l e _ p a t h                                        *
*                                                                      *
*   Return a full file path from a directory and file identifier.      *
*                                                                      *
*   m a p _ f i l e _ p a t h                                          *
*                                                                      *
*   Return a full map path from a directory and element and level      *
*   structures to match.                                               *
*   Note that the map directory is read only once!                     *
*                                                                      *
***********************************************************************/

static	STRING		data_file_path

	(
	STRING			dir,		/* Directory path name */
	STRING			ident		/* File identifier name */
	)

	{

	/* Return Null if no directory or file identifier */
	if ( blank(dir) || blank(ident) ) return NullString;

	/* Return full pathname */
	return pathname(dir, ident);
	}

/**********************************************************************/

/* Structures for Map field info */
typedef struct MAPFLDst
	{
	char					path[MAX_BCHRS];
	FpaConfigFieldStruct	*fdef;
	} MAPFLD;
typedef struct MAPDIRst
	{
	char					dir[MAX_BCHRS];
	int						nmapfld;
	MAPFLD					*mapflds;
	} MAPDIR;

static	STRING		map_file_path

	(
	STRING			dir,		/* Directory path name */
	FpaConfigElementStruct
					*edef,		/* Pointer to Element Structure */
	FpaConfigLevelStruct
					*ldef		/* Pointer to Level Structure */
	)

	{
	char					path[MAX_BCHRS];
	int						idir, ifile, ilist, ifld, nfiles, flist;
	STRING					*files;
	FpaConfigFieldStruct	*fdef, **fdefs;
	MAPDIR					*mdir;
	MAPFLD					*mfld;

	/* Static buffers for Map field info */
	static	MAPDIR	*mapdir = NullPtr(MAPDIR *);
	static	int		nmapdir = 0;

	/* Return Null if no directory, element, or level  passed */
	if ( blank(dir) ) return NullString;
	if ( IsNull(edef) || IsNull(ldef) ) return NullString;

	/* Find Map information for given directory */
	mdir = NullPtr(MAPDIR *);
	for ( idir=0; idir<nmapdir; idir++ )
		{
		mdir = mapdir + idir;
		if ( same(mdir->dir, dir) ) break;
		}
	if ( idir >= nmapdir )
		{

		/* Build an entry for this directory (if not already read) */
		idir   = nmapdir++;
		mapdir = GETMEM(mapdir, MAPDIR, nmapdir);
		mdir   = mapdir + idir;
		(void) strcpy(mdir->dir, dir);
		mdir->nmapfld = 0;
		mdir->mapflds = NullPtr(MAPFLD *);

		/* Find out what Map files are in this directory */
		nfiles = dirlist(dir, "^.*$", &files);

		/* Find out what fields are in each Map file */
		for ( ifile=0; ifile<nfiles; ifile++ )
			{
			(void) strcpy(path, pathname(dir, files[ifile]));
			flist = search_meta_fields(path, &fdefs);
			if ( flist <= 0 ) continue;

			ifld = mdir->nmapfld;
			mdir->nmapfld += flist;
			mdir->mapflds = GETMEM(mdir->mapflds, MAPFLD, mdir->nmapfld);
			for ( ilist=0; ilist<flist; ilist++, ifld++ )
				{
				mfld = mdir->mapflds + ifld;
				(void) strcpy(mfld->path,  path);
				mfld->fdef = fdefs[ilist];
				}
			}
		}

	/* Now find the field if it is contained in Map files in this directory */
	for ( ifld=0; ifld<mdir->nmapfld; ifld++ )
		{
		mfld = mdir->mapflds + ifld;
		fdef = identify_field(edef->name, ldef->name);
		if ( mfld->fdef == fdef ) return mfld->path;
		}

	/* Return Null if field not found in any Map file in this directory */
	return NullString;
	}

/***********************************************************************
*                                                                      *
*   d e p i c t i o n _ f i l e                                        *
*                                                                      *
*   Return a full file path for a Depiction type file.                 *
*                                                                      *
***********************************************************************/

static	STRING		depiction_file

	(
	FpaConfigSourceStruct
					*sdef,		/* Source structure for directory */
	STRING			name		/* file name identifier */
	)

	{
	STRING						dir;
	FpaConfigSourceIOStruct		*sio;

	/* Check for Source structure */
	if ( IsNull(sdef) ) return NullString;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NullString;

	/* Set directory */
	dir = find_data_directory(sio->src_tag, sio->src_path,
			sdef->src_sub->sub_path, FpaCblank);
	if ( blank(dir) ) return NullString;

	/* Return path based on directory and filename */
	return pathname(dir, name);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Directories)                            *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   s h u f f l e _ d i r e c t o r y                                  *
*                                                                      *
*   Shuffle files of the present directory to the next level of the    *
*   directory tree.  If there is no next level, the files are deleted. *
*                                                                      *
***********************************************************************/

static	void		shuffle_directory

	(
	)

	{
	int			nfiles, ifile;
	STRING		*files, file;
	LOGICAL		move = FALSE;

	/* Shuffle data files only if timestamp is found in present directory */
	if ( find_file(FpaFile_Dstamp) )
		{

		/* See if previous directory exists */
		if ( chdir(FpaFile_Prev) == 0 )
			{

			/* Shuffle down files in the previous directory */
			(void) shuffle_directory();
			(void) chdir("..");
			move = TRUE;
			}
		}

	/* Get list of data files in the current directory   */
	/* Move them to the previous directory (if required) */
	/*  and then delete them from the current directory  */
	nfiles = dirlist(".", NullString, &files);
	for ( ifile=0; ifile<nfiles; ifile++ )
		{
		file = files[ifile];

		/* Do not move the SHUFFLE lock! */
		if ( same(file, FpaFile_ShuffleLock) ) continue;

		/* Only move regular files */
		if ( find_file(file) )
			{
			if ( move ) (void) link(file, pathname(FpaFile_Prev, file));
			(void) unlink(file);
			}
		}
	}

/***********************************************************************
*                                                                      *
*   d a t a _ d i r e c t o r y _ r u n _ t i m e s                    *
*                                                                      *
*   Return run timestamps for each directory in data directory tree    *
*   which contains data files.                                         *
*                                                                      *
***********************************************************************/

static	int			data_directory_run_times

	(
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	STRING			**times		/* list of run timestamps */
								/*  for data directory    */
	)

	{
	int							nread;
	STRING						dpath;
	FILE						*fp;
	char						dstamp[MAX_NCHRS];
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceSubStruct	*subdef;
	FpaConfigSourceIOStruct		*sio;

	/* Static buffer for directory name */
	static	char	presentdir[MAX_BCHRS];

	/* Variables to store run time list */
	int		NumTimes  = 0;
	STRING	*TimeList = NullStringList;

	/* Initialize return parameter */
	if ( NotNull(times) ) *times = NullStringList;

	/* Return immediately if no structure passed */
	if ( IsNull(fdesc) ) return NumTimes;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NumTimes;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NumTimes;

	/* Set the base directory path */
	dpath =  find_data_directory(sio->src_tag, sio->src_path,
									subdef->sub_path, FpaCblank);
	if ( blank(dpath) ) return NumTimes;

	/* Build run time list from run timestamp files in base directory */
	/*  and in each previous subdirectory                             */
	(void) strcpy(presentdir, dpath);
	while ( TRUE )
		{

		/* Return number of run timestamps if present directory not found */
		if ( !find_directory(presentdir) )
			{
			if ( NotNull(times) ) *times = TimeList;
			return NumTimes;
			}

		/* Return number of run timestamps if problem finding present one */
		dpath = pathname(presentdir, FpaFile_Dstamp);
		if ( IsNull( fp = fopen(dpath, "r") ) )
			{
			if ( NotNull(times) ) *times = TimeList;
			return NumTimes;
			}

		/* Return number of run timestamps if problem reading present one */
		nread = fscanf(fp, "%s", dstamp);
		(void) fclose(fp);
		if ( nread != 1 )
			{
			if ( NotNull(times) ) *times = TimeList;
			return NumTimes;
			}

		/* Add a copy of the run timestamp to the list */
		/*  ... but only if the list is returned!      */
		NumTimes++;
		if ( NotNull(times) )
			{
			TimeList = GETMEM(TimeList, STRING, NumTimes);
			TimeList[NumTimes-1] = strdup(dstamp);
			}

		/* Look in previous directory */
		dpath = pathname(presentdir, FpaFile_Prev);
		(void) strcpy(presentdir, dpath);
		}
	}

/***********************************************************************
*                                                                      *
*   d a t a _ d i r e c t o r y _ v a l i d _ t i m e s                *
*                                                                      *
*   Return valid timestamps for data files in a given data directory.  *
*                                                                      *
***********************************************************************/

static	int			data_directory_valid_times

	(
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	int				macro,		/* enumerated time dependence to match */
	STRING			**times		/* list of valid timestamps */
								/*  for data directory      */
	)

	{
	int							nfiles, ifl, ii;
	LOGICAL						newformat;
	STRING						rstamp, dpath, search, *files, vtime;
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceSubStruct	*subdef;
	FpaConfigSourceIOStruct		*sio;
	FpaConfigElementStruct		*edef;

	/* Variables to store valid time list */
	int		NumTimes  = 0;
	STRING	*TimeList = NullStringList;

	/* Initialize return parameter */
	if ( NotNull(times) ) *times = NullStringList;

	/* Return immediately if no structure passed */
	if ( IsNull(fdesc) ) return NumTimes;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NumTimes;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NumTimes;

	/* Set run time (with minutes correctly encoded) */
	if ( sdef->minutes_rqd )
		rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
	else
		rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);

	/* Set directory path */
	dpath = find_data_directory(sio->src_tag, sio->src_path,
									subdef->sub_path, rstamp);
	if ( blank(dpath) ) return NumTimes;

	/* Check for new and old format FPA metafile names */
	newformat = TRUE;
	while (TRUE)
		{

		/* Set search parameter (new or old format) from field descriptor */
		search = set_field_search(newformat, fdesc);

		/* Get the list of files from the requested directory */
		if ( blank(search) ) nfiles = 0;
		else                 nfiles = dirlist(dpath, search, &files);

		/* Build list of valid timestamps */
		for ( ifl=0; ifl<nfiles; ifl++ )
			{

			/* Parse the filename to return element and valid timestamp */
			if ( !parse_file_identifier(files[ifl],
					&edef, NullPtr(FpaConfigLevelStruct **), &vtime) ) continue;

			/* Check for special case for matching all time dependence types */
			/*  or try to match the time dependence macro                    */
			if ( !(macro ^ FpaC_TIMEDEP_ANY)
					|| (macro & edef->elem_tdep->time_dep) )
				{

				/* Only save unique valid timestamps */
				for ( ii=0; ii<NumTimes; ii++ )
					{
					if ( same(vtime, TimeList[ii]) ) break;;
					}
				if ( ii < NumTimes ) continue;

				/* Only save recognized timestrings */
				if ( blank(interpret_timestring(vtime, NullString, 0.0)) )
					{
					(void) pr_error("Environ",
						"Unrecognized timestring: \"%s\"\n", SafeStr(vtime));
					(void) pr_error("Environ", "  in directory: %s\n", dpath);
					continue;
					}

				/* Add valid timestamp to list            */
				/*  ... but only if the list is returned! */
				NumTimes++;
				if ( NotNull(times) )
					{
					TimeList = GETMEM(TimeList, STRING, NumTimes);
					TimeList[NumTimes-1] = strdup(vtime);
					}
				}
			}

		/* Reset format type */
		if ( newformat ) newformat = FALSE;
		else             break;
		}

	/* Return if no valid timestamps saved */
	if ( NumTimes <= 0 ) return NumTimes;

	/* Sort valid timestamps */
	qsort((POINTER) TimeList, (size_t) NumTimes, sizeof(STRING), strvcmp);

	/* Return the list of valid timestamps */
	if ( NotNull(times) ) *times = TimeList;
	return NumTimes;
	}

/***********************************************************************
*                                                                      *
*   d a t a _ d i r e c t o r y _ f i e l d s                          *
*                                                                      *
*   Return pointers to Field structures for data files in a given data *
*   directory.                                                         *
*                                                                      *
***********************************************************************/

static	int			data_directory_fields

	(
	FLD_DESCRIPT	*fdesc,		/* pointer to field descriptor */
	int				macro,		/* enumerated time dependence to match */
	FpaConfigFieldStruct
					***fields	/* list of pointers to Field structures */
								/*  for data directory                  */
	)

	{
	int							nfiles, ifl, ii;
	LOGICAL						newformat;
	STRING						rstamp, dpath, search, *files;
	FpaConfigSourceStruct		*sdef;
	FpaConfigSourceSubStruct	*subdef;
	FpaConfigSourceIOStruct		*sio;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;
	FpaConfigFieldStruct		*fdef;

	/* Variables to store Field structure list */
	int						NumFields   = 0;
	FpaConfigFieldStruct	**FieldList = NullPtr(FpaConfigFieldStruct **);

	/* Initialize return parameter */
	if ( NotNull(fields) ) *fields = NullPtr(FpaConfigFieldStruct **);

	/* Return immediately if no structure passed */
	if ( IsNull(fdesc) ) return NumFields;

	/* Set pointers to Source and SourceSub structures */
	sdef   = fdesc->sdef;
	subdef = fdesc->subdef;
	if ( IsNull(sdef) || IsNull(subdef) ) return NumFields;

	/* Set pointer to SourceIO structure */
	sio = sdef->src_io;
	if ( IsNull(sio) ) return NumFields;

	/* Set run time (with minutes correctly encoded) */
	if ( sdef->minutes_rqd )
		rstamp = tstamp_to_minutes(fdesc->rtime, NullInt);
	else
		rstamp = tstamp_to_hours(fdesc->rtime, TRUE, NullInt);

	/* Set directory path */
	dpath = find_data_directory(sio->src_tag, sio->src_path,
									subdef->sub_path, rstamp);
	if ( blank(dpath) ) return NumFields;

	/* Check for new and old format FPA metafile names */
	newformat = TRUE;
	while (TRUE)
		{

		/* Set search parameter (new or old format) from field descriptor */
		search = set_field_search(newformat, fdesc);

		/* Get the list of files from the requested directory */
		if ( blank(search) ) nfiles = 0;
		else                 nfiles = dirlist(dpath, search, &files);

		/* Build list of pointers to Field structures */
		for ( ifl=0; ifl<nfiles; ifl++ )
			{

			/* Parse the filename to return element and level */
			if ( !parse_file_identifier(files[ifl], &edef, &ldef,
					NullStringPtr) ) continue;

			/* Check for special case for matching all time dependence types */
			/*  or try to match the time dependence macro                    */
			if ( !(macro ^ FpaC_TIMEDEP_ANY)
					|| (macro & edef->elem_tdep->time_dep) )
				{

				/* Set pointer to Field structure from element and level */
				fdef = identify_field(edef->name, ldef->name);

				/* Only save recognized fields */
				if ( IsNull(fdef) )
					{
					(void) pr_error("Environ",
						"Unrecognized field: \"%s\" \"%s\"\n",
						SafeStr(edef->name), SafeStr(ldef->name));
					(void) pr_error("Environ", "  in directory: %s\n", dpath);
					continue;
					}

				/* Only save unique fields */
				for ( ii=0; ii<NumFields; ii++ )
					{
					if ( fdef == FieldList[ii] ) break;
					}
				if ( ii < NumFields ) continue;

				/* Add pointer to Field structure to list */
				/*  ... but only if the list is returned! */
				NumFields++;
				if ( NotNull(fields) )
					{
					FieldList = GETMEM(FieldList, FpaConfigFieldStruct *,
										NumFields);
					FieldList[NumFields-1] = fdef;
					}
				}
			}

		/* Reset format type */
		if ( newformat ) newformat = FALSE;
		else             break;
		}

	/* Sort files according to field element identifiers */
	qsort((POINTER) FieldList, (size_t) NumFields,
						sizeof(FpaConfigFieldStruct *), strecmp);

	/* Return the list of pointers to Field structures */
	if ( NotNull(fields) ) *fields = FieldList;
	return NumFields;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (File Sorting and Matching)              *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   s t r e c m p                                                      *
*   s t r l c m p                                                      *
*   s t r v c m p                                                      *
*                                                                      *
*   Internal functions to compare metafile name information using      *
*   element names, level names or valid timestamp                      *
*                                                                      *
***********************************************************************/

static	int			strecmp

	(
	const void		*a,			/* pointer to first Field Structure */
	const void		*b			/* pointer to second Field Structure */
	)

	{

	/* Compare element names in field structure */
	return strcmp((*(FpaConfigFieldStruct **) a)->element->name,
				  (*(FpaConfigFieldStruct **) b)->element->name);
	}

static	int			strlcmp

	(
	const void		*a,			/* pointer to first Field Structure */
	const void		*b			/* pointer to second Field Structure */
	)

	{

	/* Compare level names in field structure */
	return strcmp((*(FpaConfigFieldStruct **) a)->level->name,
				  (*(FpaConfigFieldStruct **) b)->level->name);
	}

static	int			strvcmp

	(
	const void		*a,			/* pointer to first valid timestamp */
	const void		*b			/* pointer to second valid timestamp */
	)

	{

	/* Compare valid timestamps according to year, julian day, hour, etc */
	return strcmp(*(STRING *)a, *(STRING *)b);
	}

/***********************************************************************
*                                                                      *
*     s e t _ f i e l d _ s e a r c h                                  *
*                                                                      *
*     Internal function to set metafile name search string             *
*                                                                      *
***********************************************************************/

static	STRING		set_field_search

	(
	LOGICAL			newformat,	/* new format meta filenames? */
	FLD_DESCRIPT	*fdesc		/* pointer to field descriptor */
	)

	{
	STRING						vstamp, efid, lfid, ident;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;

	/* Static buffer for search string */
	static	char	search[FILE_ID_LEN];

	/* Return if no structure passed */
	if ( IsNull(fdesc) ) return NullString;

	/* Return if no source structure passed */
	if ( IsNull(fdesc->sdef) ) return NullString;

	/* Initialize search string */
	search[0] = '\0';

	/* Set search parameter for new format FPA metafiles */
	if ( newformat )
		{

		/* Set valid time (with minutes correctly encoded) */
		if ( fdesc->sdef->minutes_rqd )
			vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
		else
			vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);

		/* Set identifiers for element, level, and filename */
		edef  = fdesc->edef;
		efid  = (NotNull(edef))? edef->elem_io->fident: FpaCblank;
		ldef  = fdesc->ldef;
		lfid  = (NotNull(ldef))? ldef->lev_io->fident:  FpaCblank;
		ident = (NotNull(edef) && NotNull(ldef))?
					construct_file_identifier(edef->name, ldef->name, vstamp):
						FpaCblank;

		/* Set search string for matching all fields */
		if ( IsNull(edef) && IsNull(ldef) && blank(fdesc->vtime) )
			{
			(void) sprintf(search, MetaSearchAll);
			}

		/* Set search string for matching element only */
		else if ( IsNull(ldef) && blank(fdesc->vtime) )
			{
			(void) sprintf(search, MetaSearchElem, efid);
			}

		/* Set search string for matching level only */
		else if ( IsNull(edef) && blank(fdesc->vtime) )
			{
			(void) sprintf(search, MetaSearchLevel, lfid);
			}

		/* Set search string for matching valid time only */
		else if ( IsNull(edef) && IsNull(ldef) )
			{
			(void) sprintf(search, MetaSearchValid, fdesc->vtime);
			}

		/* Set search string for matching element and level       */
		/* Note that only valid element/level pairs will be used! */
		else if ( blank(fdesc->vtime) )
			{
			if ( !blank(ident) ) (void) sprintf(search, MetaSearchField, ident);
			}

		/* Set search string for matching element and valid time */
		else if ( IsNull(ldef) )
			{
			(void) sprintf(search, MetaSearchElemValid, efid, fdesc->vtime);
			}

		/* Set search string for matching level and valid time */
		else if ( IsNull(edef) )
			{
			(void) sprintf(search, MetaSearchLevelValid, lfid, fdesc->vtime);
			}

		/* Set search string for matching element, level, and valid time      */
		/* Note that only valid element/level/time combinations will be used! */
		else
			{
			if ( !blank(ident) )
				(void) sprintf(search, MetaSearchFieldValid, ident);
			}
		}

	/* Set search parameter for new format FPA metafiles */
	else
		{

		/* Set valid time (with minutes correctly encoded) */
		if ( fdesc->sdef->minutes_rqd )
			vstamp = tstamp_to_minutes(fdesc->vtime, NullInt);
		else
			vstamp = tstamp_to_hours(fdesc->vtime, TRUE, NullInt);

		/* Set identifiers for element, level, and filename */
		edef  = fdesc->edef;
		efid  = (NotNull(edef))? edef->elem_io->fid: FpaCblank;
		ldef  = fdesc->ldef;
		lfid  = (NotNull(ldef))? ldef->lev_io->fid:  FpaCblank;
		ident = (NotNull(edef) && NotNull(ldef))?
					build_file_identifier(edef->name, ldef->name, vstamp):
						FpaCblank;

		/* Set search string for matching all fields */
		if ( IsNull(edef) && IsNull(ldef) && blank(fdesc->vtime) )
			{
			(void) sprintf(search, SearchMetaAll);
			}

		/* Set search string for matching element only */
		else if ( IsNull(ldef) && blank(fdesc->vtime) )
			{
			(void) sprintf(search, SearchMetaElem, efid);
			}

		/* Set search string for matching level only */
		else if ( IsNull(edef) && blank(fdesc->vtime) )
			{
			(void) sprintf(search, SearchMetaLevel, lfid);
			}

		/* Set search string for matching valid time only */
		else if ( IsNull(edef) && IsNull(ldef) )
			{
			(void) sprintf(search, SearchMetaValid, fdesc->vtime);
			}

		/* Set search string for matching element and level       */
		/* Note that only valid element/level pairs will be used! */
		else if ( blank(fdesc->vtime) )
			{
			if ( !blank(ident) ) (void) sprintf(search, SearchMetaField, ident);
			}

		/* Set search string for matching element and valid time */
		else if ( IsNull(ldef) )
			{
			(void) sprintf(search, SearchMetaElemValid, efid, fdesc->vtime);
			}

		/* Set search string for matching level and valid time */
		else if ( IsNull(edef) )
			{
			(void) sprintf(search, SearchMetaLevelValid, lfid, fdesc->vtime);
			}

		/* Set search string for matching element, level, and valid time      */
		/* Note that only valid element/level/time combinations will be used! */
		else
			{
			if ( !blank(ident) )
				(void) sprintf(search, SearchMetaFieldValid, ident);
			}
		}

	/* Return search string */
	return search;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing static routines)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#ifdef FILES_AND_DIRS_STANDALONE

/**********************************************************************
 *** routine to test source_run_time_list source_valid_time_list    ***
 **********************************************************************/

static void test_run_valid_time_list

	(
	FLD_DESCRIPT	*fdesc
	)

	{
	int		nr, nrun, nv, num;
	STRING	*rlist, *vlist;

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) )
		{
		(void) fprintf(stderr, "\nRun and Valid times for: %s\n",
				SafeStr(fdesc->sdef->src_sub->label));
		}
	else
		{
		(void) fprintf(stderr, "\nRun and Valid times for: \n");
		}

	nrun = source_run_time_list(fdesc, &rlist);
	if ( nrun < 1 )
		{
		(void) fprintf(stderr, "  No run times\n");
		num = source_valid_time_list(fdesc, FpaC_TIMEDEP_ANY, &vlist);
		if ( num < 1 )
			(void) fprintf(stderr, "    No valid times\n");
		else
			for (nv=0; nv<num; nv++)
				{
				(void) fprintf(stderr, "    Valid time: %s\n", vlist[nv]);
				}
		}
	else
		{
		for (nr=0; nr<nrun; nr++)
			{
			(void) fprintf(stderr, "  Run time: %s\n", rlist[nr]);
			(void) set_fld_descript(fdesc, FpaF_RUN_TIME, rlist[nr],
										FpaF_END_OF_LIST);
			num = source_valid_time_list(fdesc, FpaC_TIMEDEP_ANY, &vlist);
			if ( num < 1 )
				(void) fprintf(stderr, "    No valid times\n");
			else
				for (nv=0; nv<num; nv++)
					{
					(void) fprintf(stderr, "    Valid time: %s\n", vlist[nv]);
					}
			}
		}
	}

/**********************************************************************
 *** routine to test matched/closest_source_valid_time              ***
 **********************************************************************/

static void test_matched_closest_valid_time

	(
	FLD_DESCRIPT	*fdesc,
	STRING			mtchtime
	)

	{
	int		num;
	STRING	vlist;

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) && NotNull(fdesc->subdef) )
		{
		(void) fprintf(stderr, "\nValid time test for: %s %s  run: %s  time: %s\n",
				fdesc->sdef->name, fdesc->subdef->name, fdesc->rtime, mtchtime);
		}
	else
		{
		(void) fprintf(stderr, "\nValid time test for:    run: %s  time: %s\n",
				fdesc->rtime, mtchtime);
		}

	num = matched_source_valid_time(fdesc, FpaC_TIMEDEP_ANY, mtchtime, &vlist);
	if ( num < 0 )
		(void) fprintf(stderr, "  No matching valid time\n");
	else
		(void) fprintf(stderr, "  Matching valid time: %d   %s\n", num, vlist);

	num = closest_source_valid_time(fdesc, FpaC_TIMEDEP_ANY, mtchtime, &vlist);
	if ( num < 0 )
		(void) fprintf(stderr, "  No closest valid time\n");
	else
		(void) fprintf(stderr, "  Closest valid time:  %d   %s\n", num, vlist);
	}

/**********************************************************************
 *** routine to test source_valid_time_sublist                      ***
 **********************************************************************/

static void test_valid_time_sublist

	(
	FLD_DESCRIPT	*fdesc,
	int				macro,
	int				nin,
	STRING			vbgn,
	STRING			vcen,
	STRING			vend
	)

	{
	int		num, nv;
	STRING	*vlist;

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) && NotNull(fdesc->subdef) )
		{
		(void) fprintf(stderr, "\nValid time series test for: %s %s  run: %s\n",
				fdesc->sdef->name, fdesc->subdef->name, fdesc->rtime);
		}
	else
		{
		(void) fprintf(stderr, "\nValid time series test for:    run: %s\n",
				fdesc->rtime);
		}

	(void) fprintf(stderr, "  nin: %d   vbgn: %s   vcen: %s   vend: %s",
			nin, vbgn, vcen, vend);
	(void) fprintf(stderr, "   macro: %d\n", macro);

	num = source_valid_time_sublist(fdesc, macro,
			nin, vbgn, vcen, vend, &vlist);
	if ( num < 1 )
		(void) fprintf(stderr, "    No valid times in series\n");
	else
		for (nv=0; nv<num; nv++)
			{
			(void) fprintf(stderr, "    Valid time: %s\n", vlist[nv]);
			}
	}

/**********************************************************************
 *** routine to test source_valid_time_list                         ***
 **********************************************************************/

static void test_valid_time_list

	(
	FLD_DESCRIPT	*fdesc,
	int				macro
	)

	{
	int		num, nv;
	STRING	*vlist;

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) && NotNull(fdesc->subdef) )
		{
		(void) fprintf(stderr, "\nValid times test for: %s %s  run: %s\n",
				fdesc->sdef->name, fdesc->subdef->name, fdesc->rtime);
		}
	else
		{
		(void) fprintf(stderr, "\nValid times test for:    run: %s\n",
				fdesc->rtime);
		}

	if ( NotNull(fdesc->edef) )
		{
		(void) fprintf(stderr, "  element: \"%s\"", fdesc->edef->name);
		}
	else
		{
		(void) fprintf(stderr, "  element: \"\"");
		}

	if ( NotNull(fdesc->ldef) )
		{
		(void) fprintf(stderr, "   level: \"%s\"", fdesc->ldef->name);
		}
	else
		{
		(void) fprintf(stderr, "   level: \"\"");
		}

	(void) fprintf(stderr, "   vtime: \"%s\"   macro: %d\n",
			fdesc->vtime, macro);

	num = source_valid_time_list(fdesc, macro, &vlist);
	if ( num < 1 )
		(void) fprintf(stderr, "    No valid times\n");
	else
		for (nv=0; nv<num; nv++)
			{
			(void) fprintf(stderr, "    Valid time: %s\n", vlist[nv]);
			}
	}

/**********************************************************************
 *** routine to test source_field_list                              ***
 **********************************************************************/

static void test_field_list

	(
	FLD_DESCRIPT	*fdesc,
	int				macro
	)

	{
	int						num, nn;
	FpaConfigFieldStruct	**flist;

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) && NotNull(fdesc->subdef) )
		{
		(void) fprintf(stderr, "\nField list test for: %s %s  run: %s\n",
				fdesc->sdef->name, fdesc->subdef->name, fdesc->rtime);
		}
	else
		{
		(void) fprintf(stderr, "\nField list test for:    run: %s\n",
				fdesc->rtime);
		}

	if ( NotNull(fdesc->edef) )
		{
		(void) fprintf(stderr, "  element: \"%s\"", fdesc->edef->name);
		}
	else
		{
		(void) fprintf(stderr, "  element: \"\"");
		}

	if ( NotNull(fdesc->ldef) )
		{
		(void) fprintf(stderr, "   level: \"%s\"", fdesc->ldef->name);
		}
	else
		{
		(void) fprintf(stderr, "   level: \"\"");
		}

	(void) fprintf(stderr, "   vtime: \"%s\"   macro: %d\n",
			fdesc->vtime, macro);

	num = source_field_list(fdesc, macro, &flist);
	if ( num < 1 )
		(void) fprintf(stderr, "    No fields\n");
	else
		for (nn=0; nn<num; nn++)
			{
			(void) fprintf(stderr, "    Element: %s   Level: %s\n",
					flist[nn]->element->name, flist[nn]->level->name);
			}
	}

/**********************************************************************
 *** routine to test build/check/find_meta_filename                 ***
 **********************************************************************/

static void test_check_find_meta_filename

	(
	FLD_DESCRIPT	*fdesc
	)

	{

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) && NotNull(fdesc->subdef) )
		{
		(void) fprintf(stderr, "\nFilename test for: %s %s  run: %s\n",
				fdesc->sdef->name, fdesc->subdef->name, fdesc->rtime);
		}
	else
		{
		(void) fprintf(stderr, "\nFilename test for:    run: %s\n",
				fdesc->rtime);
		}

	if ( NotNull(fdesc->edef) )
		{
		(void) fprintf(stderr, "  element: \"%s\"", fdesc->edef->name);
		}
	else
		{
		(void) fprintf(stderr, "  element: \"\"");
		}

	if ( NotNull(fdesc->ldef) )
		{
		(void) fprintf(stderr, "   level: \"%s\"", fdesc->ldef->name);
		}
	else
		{
		(void) fprintf(stderr, "   level: \"\"");
		}

	(void) fprintf(stderr, "   vtime: \"%s\"\n", fdesc->vtime);

	(void) fprintf(stderr, "    Build filename: \"%s\"\n",
			build_meta_filename(fdesc));
	(void) fprintf(stderr, "    Check filename: \"%s\"\n",
			check_meta_filename(fdesc));
	(void) fprintf(stderr, "    Find filename: \"%s\"\n",
			find_meta_filename(fdesc));
	}

/**********************************************************************
 *** routine to test prepare_source_directory                       ***
 **********************************************************************/

static void test_prepare_source_directory

	(
	FLD_DESCRIPT	*fdesc
	)

	{

	if ( IsNull(fdesc) ) return;

	if ( NotNull(fdesc->sdef) && NotNull(fdesc->subdef) )
		{
		(void) fprintf(stderr, "\nCreate directory test for: %s %s  run: %s\n",
				fdesc->sdef->name, fdesc->subdef->name, fdesc->rtime);
		}
	else
		{
		(void) fprintf(stderr, "\nCreate directory test for:    run: %s\n",
				fdesc->rtime);
		}

	(void) fprintf(stderr, "    Existing directory path: \"%s\"\n",
			source_directory(fdesc));
	(void) fprintf(stderr, "    Final directory path: \"%s\"\n",
			prepare_source_directory(fdesc));
	}

/***********************************************************************
*                                                                      *
*   OPTIONAL STAND-ALONE TEST PROGRAM:                                 *
*                                                                      *
***********************************************************************/

int main

	(
	int		argc,
	STRING	argv[]
	)

	{
	int				nsetup;
	STRING			setupfile, *setuplist;
	MAP_PROJ		*mproj;
	STRING			types, element, level, source;
	STRING			mtchtime, vbgn, vcen, vend;
	FLD_DESCRIPT	fdesc;

	/* Set Defaults for FILES_AND_DIRS_STANDALONE */

	/* ... First set the default output units */
	(void) setvbuf(stdout, NullString, _IOLBF, 0);
	(void) setvbuf(stderr, NullString, _IOLBF, 0);

	/* ... Next get setup file for FPA */
	(void) fpalib_license(FpaAccessLib);
	setupfile = strdup(argv[1]);
	(void) fprintf(stderr, "Setup File: %s\n", setupfile);
	nsetup = setup_files(setupfile, &setuplist);
	if ( !define_setup(nsetup, setuplist) )
		{
		(void) fprintf(stderr, "Fatal problem with Setup File: %s\n",
				setupfile);
		return -1;
		}

	/* ... Next set Default Map Projection */
	mproj = get_target_map();
	if ( IsNull(mproj) )
		{
		(void) fprintf(stderr, "Fatal problem with Default Map Projection\n");
		return -1;
		}

	(void) fprintf(stderr, "\n\nBasemap  olat: %f", mproj->definition.olat);
	(void) fprintf(stderr, "  olon: %f", mproj->definition.olon);
	(void) fprintf(stderr, "  lref: %f\n", mproj->definition.lref);
	(void) fprintf(stderr, "         xorg: %f", mproj->definition.xorg);
	(void) fprintf(stderr, "  yorg: %f\n", mproj->definition.yorg);
	(void) fprintf(stderr, "         xlen: %f", mproj->definition.xlen);
	(void) fprintf(stderr, "  ylen: %f", mproj->definition.ylen);
	(void) fprintf(stderr, "  units: %f\n", mproj->definition.units);

	(void) fprintf(stderr, "\nGrid definition  nx: %d", mproj->grid.nx);
	(void) fprintf(stderr, "  ny: %d", mproj->grid.ny);
	(void) fprintf(stderr, "  gridlen: %f", mproj->grid.gridlen);
	(void) fprintf(stderr, "  units: %f\n", mproj->grid.units);

	/* Initialize field descriptor for testing */
	(void) init_fld_descript(&fdesc);

	/* Testing for source_run_time_list source_valid_time_list routines */
	(void) fprintf(stderr, "\n\n***Testing for  source_run_time_list");
	(void) fprintf(stderr, "  source_valid_time_list  routines***\n");
	(void) set_fld_descript(&fdesc, FpaF_MAP_PROJECTION, mproj,
								FpaF_END_OF_LIST);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM",
								FpaF_END_OF_LIST);
	test_run_valid_time_list(&fdesc);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "spectral",
								FpaF_END_OF_LIST);
	test_run_valid_time_list(&fdesc);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "depict",
								FpaF_END_OF_LIST);
	test_run_valid_time_list(&fdesc);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "donelan",
								FpaF_SUBSOURCE_NAME, "lake_ontario",
								FpaF_END_OF_LIST);
	test_run_valid_time_list(&fdesc);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "odgp_natwave",
								FpaF_END_OF_LIST);
	test_run_valid_time_list(&fdesc);

	/* Testing for matched/closest_source_valid_time routines */
	(void) fprintf(stderr, "\n\n***Testing for  matched/closest_source_valid_time");
	(void) fprintf(stderr, "  routines***\n");
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM",
								FpaF_RUN_TIME, "1991:238:12",
								FpaF_END_OF_LIST);
	mtchtime = "1991:229:00";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	mtchtime = "1991:239:05";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	mtchtime = "1991:239:06";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	mtchtime = "1991:239:07";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	mtchtime = "1991:249:00";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "spectral",
								FpaF_RUN_TIME, "1991:222:00",
								FpaF_END_OF_LIST);
	mtchtime = "1991:223:11";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	(void) set_fld_descript(&fdesc, FpaF_RUN_TIME, FpaCblank,
								FpaF_END_OF_LIST);
	mtchtime = "1991:223:11";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "Depict",
								FpaF_END_OF_LIST);
	mtchtime = "1991:239:07";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "donelan",
								FpaF_SUBSOURCE_NAME, "lake_ontario",
								FpaF_RUN_TIME, "1991:238:12",
								FpaF_END_OF_LIST);
	mtchtime = "1991:239:07";
	test_matched_closest_valid_time(&fdesc, mtchtime);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "Twist",
								FpaF_RUN_TIME, FpaCblank,
								FpaF_END_OF_LIST);
	mtchtime = "1991:239:07";
	test_matched_closest_valid_time(&fdesc, mtchtime);

	/* Testing for source_valid_time_sublist routine */
	(void) fprintf(stderr, "\n\n***Testing for  source_valid_time_sublist  ***\n");
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM",
								FpaF_RUN_TIME, "1991:238:12",
								FpaF_END_OF_LIST);
	vbgn = "1991:229:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											vbgn, NullString, NullString);
	vbgn = "1991:249:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											vbgn, NullString, NullString);
	vend = "1991:229:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, NullString, vend);
	vbgn = "1991:229:00";		vend = "1991:229:12";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											vbgn, NullString, vend);
	vbgn = "1991:239:00";		vend = "1991:239:12";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											vbgn, NullString, vend);
	vcen = "1991:230:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	vcen = "1991:238:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	vcen = "1991:239:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	vcen = "1991:240:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	vcen = "1991:250:00";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "donelan",
								FpaF_SUBSOURCE_NAME, "lake_ontario",
								FpaF_RUN_TIME, "1991:238:12",
								FpaF_END_OF_LIST);
	vbgn = "1991:239:00";		vend = "1991:239:10";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											vbgn, NullString, vend);
	vcen = "1991:239:12";
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM_ANAL",
								FpaF_RUN_TIME, FpaCblank,
								FpaF_ELEMENT_NAME, "pn",
								FpaF_LEVEL_NAME, "msl",
								FpaF_END_OF_LIST);
	test_valid_time_sublist(&fdesc, FpaC_TIMEDEP_ANY, 5,
											NullString, vcen, NullString);
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM_ANAL",
								FpaF_RUN_TIME, FpaCblank,
								FpaF_ELEMENT_NAME, FpaCblank,
								FpaF_LEVEL_NAME, FpaCblank,
								FpaF_END_OF_LIST);
	test_valid_time_sublist(&fdesc, FpaC_DAILY,  5,
											NullString, vcen, NullString);
	test_valid_time_sublist(&fdesc, FpaC_STATIC, 5,
											NullString, vcen, NullString);
	test_valid_time_sublist(&fdesc, FpaC_NORMAL, 5,
											NullString, vcen, NullString);
	test_valid_time_sublist(&fdesc, (FpaC_DAILY | FpaC_STATIC), 5,
											NullString, vcen, NullString);

	/* Testing for source_valid_time_list routine */
	(void) fprintf(stderr, "\n\n***Testing for  source_valid_time_list  routine***\n");
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM_ANAL",
								FpaF_RUN_TIME, FpaCblank,
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tt",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, FpaCblank,
								FpaF_LEVEL_NAME, "sfc",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_LEVEL_NAME, FpaCblank,
								FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "pn",
								FpaF_VALID_TIME, "1991:223:12",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, FpaCblank,
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, "1991:224:00",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_VALID_TIME, "1991:223:12",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tt",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, FpaCblank,
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "pn",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, FpaCblank,
								FpaF_LEVEL_NAME, FpaCblank,
								FpaF_VALID_TIME, FpaCblank,
								FpaF_END_OF_LIST);
	test_valid_time_list(&fdesc, FpaC_NORMAL);
	test_valid_time_list(&fdesc, FpaC_STATIC);
	test_valid_time_list(&fdesc, (FpaC_DAILY | FpaC_STATIC));

	/* Testing for source_field_list routine */
	(void) fprintf(stderr, "\n\n***Testing for  source_field_list  ***\n");
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM_ANAL",
								FpaF_RUN_TIME, FpaCblank,
								FpaF_ELEMENT, NullPointer,
								FpaF_LEVEL, NullPointer,
								FpaF_VALID_TIME, NullString,
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tt",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_LEVEL_NAME, NullPointer,
								FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "pn",
								FpaF_VALID_TIME, "1991:223:12",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, NullPointer,
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, "1991:224:00",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_VALID_TIME, "1991:223:12",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tt",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, NullString,
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tt",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "pn",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, "1991:223:00",
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_TIMEDEP_ANY);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT, NullPointer,
								FpaF_LEVEL, NullPointer,
								FpaF_VALID_TIME, NullPointer,
								FpaF_END_OF_LIST);
	test_field_list(&fdesc, FpaC_NORMAL);
	test_field_list(&fdesc, FpaC_STATIC);
	test_field_list(&fdesc, (FpaC_DAILY | FpaC_STATIC));

	/* Testing for build/check/find_meta_filename routines */
	(void) fprintf(stderr, "\n\n***Testing for  build/check/find_meta_filename");
	(void) fprintf(stderr, "  routines***\n");
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "FEM_ANAL",
								FpaF_RUN_TIME, FpaCblank,
								FpaF_ELEMENT_NAME, "pn",
								FpaF_LEVEL_NAME, "sfc",
								FpaF_VALID_TIME, "1991:240:00",
								FpaF_END_OF_LIST);
	test_check_find_meta_filename(&fdesc);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tx",
								FpaF_END_OF_LIST);
	test_check_find_meta_filename(&fdesc);
	(void) set_fld_descript(&fdesc, FpaF_ELEMENT_NAME, "tm",
								FpaF_VALID_TIME, "1991:238:12",
								FpaF_END_OF_LIST);
	test_check_find_meta_filename(&fdesc);

	/* Testing for prepare_source_directory routine */
	(void) fprintf(stderr, "\n\n***Testing for  prepare_source_directory  ***\n");
	(void) set_fld_descript(&fdesc, FpaF_SOURCE_NAME, "ECMWF",
								FpaF_RUN_TIME, "1991:238:12",
								FpaF_ELEMENT, NullPointer,
								FpaF_LEVEL, NullPointer,
								FpaF_VALID_TIME, NullPointer,
								FpaF_END_OF_LIST);
	test_prepare_source_directory(&fdesc);

	/* Testing for ... */

	return 0;
	}

#endif /* FILES_AND_DIRS_STANDALONE */
