/***********************************************************************
*                                                                      *
*   g r i b s . c                                                      *
*                                                                      *
*   Routines to access the Gribs Configuration File, which contains    *
*   default information for GRIB models or elements/units not          *
*   recognized by the GRIB decoder, and fields in the GRIB message     *
*   which the user does not wish to process                            *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
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

#define GRIBS_INIT		/* To initialize declarations in gribs.h */

#include "gribs.h"

#include <string.h>
#include <stdio.h>

/* Structure to hold default model definition */
typedef	struct
	{
	int		centre_id;		/* Id for centre issuing GRIB data */
	int		model_id;		/* Id for model type for GRIB data */
	FpaConfigSourceStruct
				*source;	/* pointer to FPA source info */
	} GRIB_MODEL_DEF;

/* Structure to hold default element/units definition */
typedef	struct
	{
	int		parameter;		/* Element id number for GRIB data */
	FpaConfigElementStruct
				*element;	/* pointer to FPA element info */
	FpaConfigUnitStruct
				*units;		/* pointer to FPA units info */
	} GRIB_ELEM_DEF;

/* Structure to hold fields to skip in processing */
typedef	struct
	{
	FpaConfigElementStruct
				*element;	/* pointer to FPA element info */
	FpaConfigLevelStruct
				*level;		/* pointer to FPA level info */
	} GRIB_SKIP_DEF;


/* Interface functions               */
/*  ... these are defined in gribs.h */

/* Internal static functions to access grib config info */
static	LOGICAL				read_grib_config();
static	GRIB_MODEL_DEF		*find_default_model(STRING);
static	GRIB_ELEM_DEF		*find_default_element(STRING);
static	GRIB_SKIP_DEF		*find_field_skip(STRING, STRING);

/***********************************************************************
*                                                                      *
*   r e a d _ c o m p l e t e _ g r i b s _ f i l e                    *
*                                                                      *
*   This function reads all blocks of the gribs configuration files.   *
*                                                                      *
***********************************************************************/

LOGICAL			read_complete_gribs_file

	(
	)

	{

	/* Ensure that each block of gribs configuration files has been read */
	if ( !read_grib_config() ) return FALSE;

	/* Return TRUE if all blocks have been read */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   r e p l a c e _ g r i b _ d e f a u l t _ m o d e l                *
*   r e p l a c e _ g r i b _ d e f a u l t _ e l e m e n t            *
*                                                                      *
***********************************************************************/

LOGICAL			replace_grib_default_model

	(
	STRING		*model		/* GRIB model (as name or number) */
	)

	{
	GRIB_MODEL_DEF	*mdef;

	/* Check if model is a default GRIB model */
	mdef = find_default_model(*model);

	/* Return all the required parameters */
	if ( mdef && model ) *model = mdef->source->name;
	return ( mdef ) ? TRUE: FALSE;
	}

LOGICAL			replace_grib_default_element

	(
	STRING		*element,	/* GRIB element (as name or number) */
	STRING		*units		/* GRIB element units */
	)

	{
	GRIB_ELEM_DEF	*edef;

	/* Check if element is a default GRIB element */
	edef = find_default_element(*element);

	/* Return all the required parameters */
	if ( edef && element ) *element = edef->element->name;
	if ( edef && units )   *units   = edef->units->name;
	return ( edef ) ? TRUE: FALSE;
	}

/***********************************************************************
*                                                                      *
*   s k i p _ g r i b _ f i e l d _ o r i g                            *
*                                                                      *
***********************************************************************/

LOGICAL			skip_grib_field_orig

	(
	STRING		element,	/* element name */
	STRING		level		/* level name */
	)

	{
	GRIB_SKIP_DEF	*sdef;

	/* Check if element and level are in list of fields to skip */
	sdef = find_field_skip(element, level);

	/* Set return based on whether field is found */
	return (sdef) ? TRUE: FALSE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES:                                         *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/* Global variables to hold grib config info */
static	LOGICAL			Gready         = FALSE;
static	LOGICAL			Gvalid         = FALSE;
static	int				NGribModelDefs = 0;
static	GRIB_MODEL_DEF	*GribModelDefs = NullPtr(GRIB_MODEL_DEF *);
static	int				NGribElemDefs  = 0;
static	GRIB_ELEM_DEF	*GribElemDefs  = NullPtr(GRIB_ELEM_DEF *);
static	int				NGribSkipDefs  = 0;
static	GRIB_SKIP_DEF	*GribSkipDefs  = NullPtr(GRIB_SKIP_DEF *);

/* Buffer for storing line read from grib config file */
static	char	GribLine[FPAC_MAX_LENGTH];

/***********************************************************************
*                                                                      *
*   r e a d _ g r i b _ c o n f i g                                    *
*                                                                      *
***********************************************************************/

#define	OKARG(arg)	( !blank(arg) && !same(arg, FpaCplaceHolder) )

static	LOGICAL			read_grib_config

	(
	)

	{
	LOGICAL					valid;
	STRING					line, cmd, arg;
	GRIB_MODEL_DEF			*mdef;
	GRIB_ELEM_DEF			*edef;
	GRIB_SKIP_DEF			*sdef;

	/* Do nothing if already input */
	if ( Gready ) return Gvalid;

	/* Set the flag to prevent re-reading the config file */
	Gready = TRUE;
	Gvalid = FALSE;

	/* Open the grib config file */
	if ( !open_config_file("gribs") )
		{
		(void) fprintf(stderr, "[read_grib_config]");
		(void) fprintf(stderr, " Gribs config file unknown or not found\n");
		return Gvalid;
		}

	/* Read the file */
	Gvalid = TRUE;
	while ( line = config_file_line() )
		{

		/* Make a copy of the current line */
		(void) strcpy(GribLine, line);

		/* Extract the first argument from the current line */
		cmd = string_arg(line);

		/* Check for "gribmodel" specifier */
		if ( same(cmd, "gribmodel") )
			{
			/* Enlarge the definition list */
			NGribModelDefs++;
			GribModelDefs = GETMEM(GribModelDefs, GRIB_MODEL_DEF,
															NGribModelDefs);
			mdef          = GribModelDefs + (NGribModelDefs-1);
			mdef->centre_id = 0;
			mdef->model_id  = 0;
			mdef->source    = NullPtr(FpaConfigSourceStruct *);

			/* Fill in the new definition from the arguments */
			mdef->centre_id = int_arg(line, &valid);
			mdef->model_id  = int_arg(line, &valid);
			arg = strdup_arg(line);
			if ( OKARG(arg) )
				mdef->source = identify_source(arg, FpaCblank);
			FREEMEM(arg);
			if ( !mdef->source )
				{
				(void) fprintf(stderr, "[read_grib_config]");
				(void) fprintf(stderr, " Invalid FPA Source for: \"%s\"\n",
						GribLine);
				Gvalid = FALSE;
				}
			}

		/* Check for "gribelement" specifier */
		else if ( same(cmd, "gribelement") )
			{
			/* Enlarge the definition list */
			NGribElemDefs++;
			GribElemDefs = GETMEM(GribElemDefs, GRIB_ELEM_DEF, NGribElemDefs);
			edef         = GribElemDefs + (NGribElemDefs-1);
			edef->parameter = 0;
			edef->element   = NullPtr(FpaConfigElementStruct *);
			edef->units     = NullPtr(FpaConfigUnitStruct *);

			/* Fill in the new definition from the arguments */
			edef->parameter = int_arg(line, &valid);
			arg = strdup_arg(line);
			if ( OKARG(arg) )
				edef->element = identify_element(arg);
			FREEMEM(arg);
			if ( !edef->element )
				{
				(void) fprintf(stderr, "[read_grib_config]");
				(void) fprintf(stderr, " Invalid FPA Element for: \"%s\"\n",
						GribLine);
				Gvalid = FALSE;
				}
			arg = strdup_arg(line);
			if ( OKARG(arg) )
				edef->units = identify_unit(arg);
			FREEMEM(arg);
			if ( !edef->units )
				{
				(void) fprintf(stderr, "[read_grib_config]");
				(void) fprintf(stderr, " Invalid FPA Units for: \"%s\"\n",
						GribLine);
				Gvalid = FALSE;
				}
			}

		/* Check for "gribfieldskip" specifier */
		else if ( same(cmd, "gribfieldskip") )
			{
			/* Enlarge the definition list */
			NGribSkipDefs++;
			GribSkipDefs = GETMEM(GribSkipDefs, GRIB_SKIP_DEF, NGribSkipDefs);
			sdef         = GribSkipDefs + (NGribSkipDefs-1);
			sdef->element = NullPtr(FpaConfigElementStruct *);
			sdef->level   = NullPtr(FpaConfigLevelStruct *);

			/* Fill in the new definition from the arguments */
			arg = strdup_arg(line);
			if ( OKARG(arg) )
				sdef->element = identify_element(arg);
			FREEMEM(arg);
			if ( !sdef->element )
				{
				(void) fprintf(stderr, "[read_grib_config]");
				(void) fprintf(stderr, " Invalid FPA Element for: \"%s\"\n",
						GribLine);
				Gvalid = FALSE;
				}
			arg = strdup_arg(line);
			if ( OKARG(arg) )
				sdef->level = identify_level(arg);
			FREEMEM(arg);
			if ( !sdef->level )
				{
				(void) fprintf(stderr, "[read_grib_config]");
				(void) fprintf(stderr, " Invalid FPA Level for: \"%s\"\n",
						GribLine);
				Gvalid = FALSE;
				}
			if ( !consistent_element_and_level(sdef->element, sdef->level) )
				{
				(void) fprintf(stderr, "[read_grib_config]");
				(void) fprintf(stderr, " Inconsistent FPA Element/Level for: \"%s\"\n",
						GribLine);
				Gvalid = FALSE;
				}
			}
		}

	/* Now return error checking parameter */
	return Gvalid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ d e f a u l t _ m o d e l                                *
*   f i n d _ d e f a u l t _ e l e m e n t                            *
*                                                                      *
***********************************************************************/

static	GRIB_MODEL_DEF	*find_default_model

	(
	STRING		string		/* GRIB model (as name or number) */
	)

	{
	int				nc, centre, model, imod;
	GRIB_MODEL_DEF	*mdef;

	/* Only respond to a valid request */
	if ( blank(string) ) return NullPtr(GRIB_MODEL_DEF *);

	/* Input the config file (if necessary) */
	if ( !read_grib_config() ) return NullPtr(GRIB_MODEL_DEF *);

	/* Return now if string does not have correct format */
	if ( !same_start(string, "gribmodel") ) return NullPtr(GRIB_MODEL_DEF *);

	/* Get the centre id and model id from the string */
	nc = (int) strcspn(string, ":");
	if ( sscanf(string+nc+1, "%d:%d", &centre, &model) != 2 )
			return NullPtr(GRIB_MODEL_DEF *);

	/* Check through the default model list with the centre/model ids */
	for ( imod=0; imod<NGribModelDefs; imod++ )
		{
		mdef = GribModelDefs + imod;
		if ( (centre == mdef->centre_id)
				&& (model == mdef->model_id) ) return mdef;
		}

	/* Return Null if centre/model ids not found */
	return NullPtr(GRIB_MODEL_DEF *);
	}

static	GRIB_ELEM_DEF	*find_default_element

	(
	STRING		string		/* GRIB element (as name or number) */
	)

	{
	int				nc, parameter, ielem;
	GRIB_ELEM_DEF	*edef;

	/* Only respond to a valid request */
	if ( blank(string) ) return NullPtr(GRIB_ELEM_DEF *);

	/* Input the config file (if necessary) */
	if ( !read_grib_config() ) return NullPtr(GRIB_ELEM_DEF *);

	/* Return now if string does not have correct format */
	if ( !same_start(string, "gribelement") ) return NullPtr(GRIB_ELEM_DEF *);

	/* Get the element parameter from the string */
	nc = (int) strcspn(string, ":");
	if ( sscanf(string+nc+1, "%d", &parameter) != 1 )
			return NullPtr(GRIB_ELEM_DEF *);

	/* Check through the default element list with the element parameter */
	for ( ielem=0; ielem<NGribElemDefs; ielem++ )
		{
		edef = GribElemDefs + ielem;
		if ( parameter == edef->parameter ) return edef;
		}

	/* Return Null if element id not found */
	return NullPtr(GRIB_ELEM_DEF *);
	}

static	GRIB_SKIP_DEF	*find_field_skip

	(
	STRING		element,	/* element name */
	STRING		level		/* level name */
	)

	{
	int						iskip;
	GRIB_SKIP_DEF			*sdef;

	/* Only respond to a valid request */
	if ( blank(element) || blank(level) ) return NullPtr(GRIB_SKIP_DEF *);

	/* Input the config file (if necessary) */
	if ( !read_grib_config() ) return NullPtr(GRIB_SKIP_DEF *);

	/* Check through the field skip list for the element and level */
	for ( iskip=0; iskip<NGribSkipDefs; iskip++ )
		{
		sdef = GribSkipDefs + iskip;
		if ( equivalent_element_definitions(sdef->element->name, element)
				&& equivalent_level_definitions(sdef->level->name, level) )
			return sdef;
		}

	/* Return Null if matching element and level not found */
	return NullPtr(GRIB_SKIP_DEF *);
	}
