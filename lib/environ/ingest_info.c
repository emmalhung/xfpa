/**********************************************************************/
/** @file ingest_info.c
 *
 * Functions for reading and storing ingest
 * configurations.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*   i n g e s t _ i n f o . c                                          *
*                                                                      *
*   Functions for reading and storing ingest configurations            *
*                                                                      *
*     Version 6 (c) Copyright 2006 Environment Canada                  *
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
#define INGEST_INFO_INIT	/* to initialize declarations in ingest_info.h */

#include "ingest_info.h"
#include <fpa_macros.h>
#include <string.h>
#include <stdio.h>

/* Interface function                      */
/*  ... these are defined in ingest_info.h */

/* Internal static functions */
void print_test();

/* Internal static functions (All blocks) */
static	LOGICAL	read_blocks_info(void);

/* Internal static functions (Block Reading) */
static	LOGICAL read_grib_info(void);
static	LOGICAL read_field_info(int, FILE *);
static	LOGICAL read_datafile_info(int, FILE *);
static	LOGICAL read_source_info(int, FILE *);
static	LOGICAL	read_element_info(int, FILE *);
static	LOGICAL	read_level_info(int, FILE *);

/* Internal Static functions  Find structures in a list */
static  FpaIngestSourceStruct		*find_source(int, int *);
static  FpaIngestSourceListStruct	*find_source_list(int, STRING);
static  FpaIngestElementStruct		*find_element(int, int *, FpaIngestSourceListStruct *);
static  FpaIngestLevelStruct		*find_level(int, int, FpaIngestSourceListStruct *);

/* Add pointers to source list */
static void add_element_to_list(int, FpaIngestSourceListStruct *, FpaIngestElementStruct *);
static void add_level_to_list(int, FpaIngestSourceListStruct *, FpaIngestLevelStruct *);

/* Internal Static functions Add new structures to list */
static  FpaIngestSourceStruct		*init_source(int, STRING);
static  FpaIngestSourceListStruct	*init_source_list(int, STRING);
static  FpaIngestElementStruct		*init_element(int, STRING, STRING);
static  FpaIngestLevelStruct		*init_level(int, STRING);
static  FpaIngestFieldStruct		*init_process_field(int, LOGICAL);
static  FpaIngestFieldStruct		*init_process_datafile(int, LOGICAL);
static  FpaIngestRedirectStruct		*init_redirect_field(int);
static  FpaIngestRedirectStruct		*init_redirect_datafile(int);
static  FpaIngestRescaleStruct		*init_rescale_datafile(int);

/* Internal Static comparison Functions */
void simple_sort_sources (int);
void simple_sort_elements (FpaIngestSourceListStruct *);
void simple_sort_levels (FpaIngestSourceListStruct *);
void simple_sort_redirect_field (int);
void simple_sort_redirect_data (int);
void simple_sort_rescale_data (int);

/* Internal static function (Section Checking) */
static	int		push_section(int);
static	int		pop_section(void);
/* convertion from string to integer */
static LOGICAL strtoushrt ( STRING, unsigned short	*);
static LOGICAL strtouchar ( STRING, unsigned char	*);

/* Storage Lists */
static	FpaIngestElementStruct		**GRIBElements = NullPtr(FpaIngestElementStruct **);
static	int							NumGRIBElements = 0;
static	FpaIngestLevelStruct		**GRIBLevels = NullPtr(FpaIngestLevelStruct **);
static	int							NumGRIBLevels = 0;

/* GRIB edition 1/0 Lists */
/** List sources for GRIB edition 0/1 */
static	FpaIngestSourceStruct		**GRIB1Sources = NullPtr(FpaIngestSourceStruct **);
static	int							NumGRIB1Sources = 0;
/** List Elements by source for GRIB edition 0/1 */
static	FpaIngestSourceListStruct	**GRIB1SLists = NullPtr(FpaIngestSourceListStruct **);
static	int							NumGRIB1SLists = 0;
/** List of fields to skip or process into metafiles for GRIB edition 0/1 */
static	FpaIngestProcessStruct		GRIB1Metafiles;
/** List of fields to skip or process into data files for GRIB edition 0/1 */
static	FpaIngestProcessStruct		GRIB1Datafiles;

/* GRIB edition 2 Lists */
/** List sources for GRIB edition 2 */
static	FpaIngestSourceStruct		**GRIB2Sources = NullPtr(FpaIngestSourceStruct **);
static	int							NumGRIB2Sources = 0;
/** List Elements by source for GRIB edition 2 */
static	FpaIngestSourceListStruct	**GRIB2SLists = NullPtr(FpaIngestSourceListStruct **);
static	int							NumGRIB2SLists = 0;
/** List of fields to skip or process into metafiles for GRIB edition 2 */
static	FpaIngestProcessStruct		GRIB2Metafiles;
/** List of fields to skip or process into data files for GRIB edition 2 */
static	FpaIngestProcessStruct		GRIB2Datafiles;

static	LOGICAL		Iread	= FALSE;	/* Ingest Config file has been read */
static	LOGICAL		Ivalid	= TRUE;		/* Ingest Config file had no errors */


/************************************************************/
/** Lookup model tag based on parameters.
 *
 * The parameters we use are:
 *  Edition 1: centre, sub_center.
 *  Edition 2: centre, sub_center, product template,
 *   type of forecast, process, and background process id's.
 *
 * @param[in]	edition		the GRIB edition
 * @param[in] 	*params		a list of parameters
 * @param[out]	*tag		the FPA model string.
 * @return True if successful.
 ************************************************************/
LOGICAL ingest_grib_models
	(
	 int edition,
	 int *params,
	 STRING		tag
	 )
	{
	FpaIngestSourceStruct	*sdef = NullPtr(FpaIngestSourceStruct *);

	/* Find the source */
	sdef =	find_source(edition, params);

	/* Check if the source is valid */
	if ( NotNull(sdef) )
		{
		(void) safe_strcpy(tag, sdef->name);
		return TRUE;
		}
	return FALSE;
	}

/************************************************************/
/** Lookup element tag based on parameters.
 *
 * This function returns the name of the element if found.
 *
 *  The parameters are:
 *  Edition 1: parameter table version, parameter
 *  Edition 2: discipline, category, parameter and template
 *
 * @param[in]	edition	GRIB edition
 * @param[in]	source	Source to match
 * @param[in]	params	List of parameters
 * @param[out]	element	Element tag
 * @param[out]	units	Units tag
 *
 * @return True if successful.
 ************************************************************/
LOGICAL ingest_grib_elements
	(
	 int		edition,
	 STRING		source,
	 int 		*params,
	 STRING		element,
	 STRING		units
	 )
	{
	FpaIngestElementStruct		*eptr = NullPtr(FpaIngestElementStruct *);

	/* See if the element is specific to the source */
	eptr = find_element(edition, params, find_source_list(edition, source) );

	/* Otherwise check the default list */
	if ( IsNull(eptr) )
		eptr = find_element(edition, params, find_source_list(edition, "default") );

	if ( NotNull(eptr) )
		{
		if ( NotNull(element) )	(void) safe_strcpy(element, eptr->element);
		if ( NotNull(units) )	(void) safe_strcpy(units, eptr->units);
		return TRUE;
		}
	return FALSE;
	}

/************************************************************/
/** Lookup level tag and scale/offset information
 * based on parameters.
 *
 * This function does not form the level name. It returns the
 * information necessary for the calling function to assemble
 * the level name based on the GRIB edition level specifications.
 *
 *  The parameters are:
 *  Edition 0,1 & 2: Level id.
 *
 * @param[in]	edition		GRIB edition
 * @param[in]	source		Source to match
 * @param[in]	id			Level id
 * @param[out]	*method		Type level: fixed surface, level, layer
 * @param[out] 	*scale		First scale value.
 * @param[out] 	*offset		First offset value.
 * @param[out] 	*scale2		Second scale value.
 * @param[out] 	*offset2	Second offset value.
 * @param[out]	tag			Level name or postfix.
 *
 * @return True if successful.
 ************************************************************/
LOGICAL ingest_grib_levels
	(
	 int		edition,
	 STRING		source,
	 int		id,
	 int 		*method,
	 float 		*scale,
	 float 		*offset,
	 float 		*scale2,
	 float 		*offset2,
	 STRING		tag
	)
	{
	FpaIngestLevelStruct		*lptr = NullPtr(FpaIngestLevelStruct *);

	/* See if the level is specific to the source */
	lptr = find_level(edition, id, find_source_list(edition, source) );

	/* Otherwise check the default list */
	if ( IsNull(lptr) )
		lptr = find_level(edition, id, find_source_list(edition, "default") );

	/* Fill in the requested information */
	if ( NotNull(lptr) )
		{
		(void) safe_strcpy(tag, lptr->tag);
		if ( NotNull(method) )	*method	 = lptr->type;
		if ( NotNull(scale) )	*scale 	 = lptr->scale;
		if ( NotNull(offset) )	*offset	 = lptr->offset;
		if ( NotNull(scale2) )	*scale2	 = lptr->scale2;
		if ( NotNull(offset2) )	*offset2 = lptr->offset2;
		return TRUE;
		}
	return FALSE;
	}

/************************************************************/
/** Look for source, element, level combination in the
 * process/skip lists.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	source	Souce to match
 * @param[in]	element	Element to match
 * @param[in]	level	Level to match
 * @return True field should be skipped.
 ************************************************************/
LOGICAL skip_grib_field
	(
	 int	edition,
	 STRING source,
	 STRING	element,
	 STRING level
	)
	{
	int ii, np, ns;
	FpaIngestFieldStruct	*pptr = NullPtr(FpaIngestFieldStruct *);
	FpaIngestFieldStruct	*sptr = NullPtr(FpaIngestFieldStruct *);

	/* GRIB 1/0 */
	switch(edition)
		{
		case 0:
		case 1:
			np   = GRIB1Metafiles.nprocess;
			ns   = GRIB1Metafiles.nskip;
			pptr = GRIB1Metafiles.process;
			sptr = GRIB1Metafiles.skip;
			break;
		case 2:
			np   = GRIB2Metafiles.nprocess;
			ns   = GRIB2Metafiles.nskip;
			pptr = GRIB2Metafiles.process;
			sptr = GRIB2Metafiles.skip;
			break;
		}

	/* Check process list first */
	if ( np> 0 )
		{
		for (ii = 0; ii < np; ii++, pptr++ )
			{
			/* Match made if same string or '*' */
			if ( (same(source,  pptr->source)  || same(pptr->source,  FpaGwildCard) ) &&
				 (same(element, pptr->element) || same(pptr->element, FpaGwildCard) ) &&
				 (same(level,   pptr->level)   || same(pptr->level,   FpaGwildCard) ) )
				return FALSE; /* Don't skip! */
			}
		/* Can't find field in process list so skip it */
		return TRUE;
		}
	else if ( ns > 0 )/* No process list so check skip list */
		{
		for (ii = 0; ii < ns ; ii++, sptr++ )
			{
			if ( (same(source,  sptr->source)  || same(sptr->source,  FpaGwildCard) ) &&
				 (same(element, sptr->element) || same(sptr->element, FpaGwildCard) ) &&
				 (same(level,   sptr->level)   || same(sptr->level,   FpaGwildCard) ) )
				return TRUE; /* Skip it! */
			}
		/* Can't find field in skip list so process it */
		return FALSE;
		}
	else /* There were no process/skip lists, default to process everything */
		return FALSE;
	}

/************************************************************/
/** Look for source, element, level combination in the
 * process/skip lists.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	source	Souce to match
 * @param[in]	element	Element to match
 * @param[in]	level	Level to match
 * @return True field should be skipped.
 ************************************************************/
LOGICAL skip_grib_datafile
	(
	 int	edition,
	 STRING source,
	 STRING	element,
	 STRING level
	)
	{
	int ii, np, ns;
	FpaIngestFieldStruct	*pptr = NullPtr(FpaIngestFieldStruct *);
	FpaIngestFieldStruct	*sptr = NullPtr(FpaIngestFieldStruct *);

	switch(edition)
		{
		case 0:
		case 1:
			np   = GRIB1Datafiles.nprocess;
			ns   = GRIB1Datafiles.nskip;
			pptr = GRIB1Datafiles.process;
			sptr = GRIB1Datafiles.skip;
			break;
		case 2:
			np   = GRIB2Datafiles.nprocess;
			ns   = GRIB2Datafiles.nskip;
			pptr = GRIB2Datafiles.process;
			sptr = GRIB2Datafiles.skip;
			break;
		}

	/* Check process list first */
	if ( np> 0 )
		{
		for (ii = 0; ii < np; ii++, pptr++ )
			{
			/* Match if same string or '*' */
			if ( (same(source,  pptr->source)  || same(pptr->source,  FpaGwildCard) ) &&
				 (same(element, pptr->element) || same(pptr->element, FpaGwildCard) ) &&
				 (same(level,   pptr->level)   || same(pptr->level,   FpaGwildCard) ) )
				return FALSE; /* Don't skip! */
			}
		/* Can't find field in process list so skip it */
		return TRUE;
		}
	else if ( ns > 0 )/* No process list so check skip list */
		{
		for (ii = 0; ii < ns ; ii++, sptr++ )
			{
			if ( (same(source,  sptr->source)  || same(sptr->source,  FpaGwildCard) ) &&
				 (same(element, sptr->element) || same(sptr->element, FpaGwildCard) ) &&
				 (same(level,   sptr->level)   || same(sptr->level,   FpaGwildCard) ) )
				return TRUE; /* Skip it! */
			}
		/* Can't find field in skip list so process it */
		return FALSE;
		}
	else /* There were no process/skip lists, default to skip everything */
		return TRUE;
	}

/************************************************************/
/** Replace source name with another if requested.
 *
 * @param[in]	edition		GRIB edition \#
 * @param[in]	source		Old source
 * @return TRUE redirect is required
 ************************************************************/
STRING redirect_field
	(
	 int	edition,
	 STRING	source
	)
	{
	int ii, nr=0;
	FpaIngestRedirectStruct	*rptr = NullPtr(FpaIngestRedirectStruct *);

	switch(edition)
		{
		case 0:
		case 1:
			nr   = GRIB1Metafiles.nredirect;
			rptr = GRIB1Metafiles.redirect;
			break;
		case 2:
			nr   = GRIB2Metafiles.nredirect;
			rptr = GRIB2Metafiles.redirect;
			break;
		}

	if ( nr < 1 )	return source;

	/* If a redirect request is found return redirected source. */
	for ( ii = 0; ii < nr; ii++, rptr++)
		{
		if ( same(rptr->source_from, source) ) return rptr->source_to;
		}
	/* If no redirect request found then return original source */
	return source;
	}
/************************************************************/
/** Replace source name with another if requested.
 *
 * @param[in]	edition		GRIB edition \#
 * @param[in]	source		Old source
 * @return Pointer to the source requested
 ************************************************************/
STRING redirect_datafile
	(
	 int	edition,
	 STRING	source
	)
	{
	int ii, nr=0;
	FpaIngestRedirectStruct	*rptr = NullPtr(FpaIngestRedirectStruct *);

	switch(edition)
		{
		case 0:
		case 1:
			nr   = GRIB1Datafiles.nredirect;
			rptr = GRIB1Datafiles.redirect;
			break;
		case 2:
			nr   = GRIB2Datafiles.nredirect;
			rptr = GRIB2Datafiles.redirect;
			break;
		}

	if ( nr < 1 )
		{
		(void) pr_warning("Ingest", "Datafiles have not been redirected.\n");
		(void) pr_warning("Ingest",
				"  FPA may crash if metafiles and datafiles are saved to the same directory.\n");
		return source;
		}

	/* If a redirect request is found return redirected source. */
	for ( ii = 0; ii < nr; ii++, rptr++)
		{
		if ( same(rptr->source_from, source) ) return rptr->source_to;
		}
	/* If no redirect request found then return original source */
	(void) pr_warning("Ingest", "Datafiles have not been redirected.\n");
	(void) pr_warning("Ingest",
			"  FPA may crash if metafiles and datafiles are saved to the same directory.\n");
	return source;
	}

/************************************************************/
/** Lookup scale & offset values for specified field.
 *
 * @param[in]		edition	GRIB edition \#
 * @param[in]		element	element of field.
 * @param[in]		level	level of field.
 * @param[out]		scale	amount to scale values by.
 * @param[out]		offset	amount to offset values by.
 * @return TRUE rescale is required.
 ************************************************************/
LOGICAL rescale_datafile
	(
	 int		edition,
	 STRING		element,
	 STRING		level,
	 float 		*scale,
	 float 		*offset
	)
	{
	int ii, ns=0;
	FpaIngestRescaleStruct	*sptr = NullPtr(FpaIngestRescaleStruct *);

	/* Default return values */
	*scale = 1;
	*offset = 0;

	switch(edition)
		{
		case 0:
		case 1:
			ns   = GRIB1Datafiles.nrescale;
			sptr = GRIB1Datafiles.rescale;
			break;
		case 2:
			ns   = GRIB2Datafiles.nrescale;
			sptr = GRIB2Datafiles.rescale;
			break;
		}

	if ( ns < 1 )	return FALSE;

	/* If a redirect request is found return redirected source. */
	for ( ii = 0; ii < ns; ii++, sptr++)
		{
		if ( ( same(sptr->element, element) ) &&
			 ( same(sptr->level, level) || same(sptr->level, FpaGwildCard) ) )
			{
			*scale 	= sptr->scale;
			*offset = sptr->offset;
			return TRUE;
			}
		}
	/* No Match */
	return FALSE;
	}

/************************************************************/
/** Read the ingest config file
 *
 * @return True if successful.
 ************************************************************/
LOGICAL	read_complete_ingest_file
	(
	)
	{
	int ii;

	/* Ensure that all blocks of the ingest configuration file are recognized */
	if ( !read_blocks_info() )	return FALSE;

	/* Ensure that each block of the ingest configuration file has been read */
	if ( !read_grib_info() )	return FALSE;

	/*
	(void)print_test();
	*/
	/* Return TRUE if all blocks have been read */
	return TRUE;
	}

/************************************************************/
/************************************************************
 * Internal Static Functions
 ************************************************************/
/************************************************************/

/************************************************************
 * READ ALL BLOCK INFORMATION
 *
 * r e a d _ b l o c k s _ i n f o
 ************************************************************/

/* Storage for Blocks information */
static	LOGICAL		BlocksRead  = FALSE;
static	LOGICAL		BlocksValid = FALSE;

static	LOGICAL					read_blocks_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, cfgname;

	/* Read the ingest configuration file(s) only once */
	if ( BlocksRead ) return BlocksValid;

	/* Find and open the configuration file */
	if ( !first_config_file_open(FpaGIngestsFile, &fpcfg) )
		{
		BlocksRead = TRUE;
		return BlocksValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Checking all blocks!\n");

	/* Read the Ingest configuration file block by block
	 * Note that read_config_file_line() handles "include" lines
	 *  and closes each configuration file as the last line is read
	 */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Skip all recognized blocks of configuration file */
		if ( same(cmd, FpaGblockGrib) )
			{
			(void) skip_config_file_block(&fpcfg);
			}

		/* Error message for all unrecognized blocks of ingest config file */
		else
			{
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Ingest", "Ingest config file: \"%s\"\n", cfgname);
			if ( blank(cline) )
				(void) pr_error("Ingest", "Block: \"%s\"\n", cmd);
			else
				(void) pr_error("Ingest", "Block: \"%s %s\"\n", cmd, cline);
			(void) pr_error("Ingest",
					"     Unrecognized block name!\n");
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Set flags for completion of checking */
	BlocksRead  = TRUE;
	BlocksValid = TRUE;
	return BlocksValid;
	}

/************************************************************
 * READ GRIBS BLOCK INFORMATION
 *
 * r e a d _ g r i b _ i n f o
 * r e a d _ s o u r c e _ i n f o
 * r e a d _ e l e m e n t _ i n f o
 * r e a d _ l e v e l _ i n f o
 * r e a d _ f i e l d _ i n f o
 * r e a d _ d a t a f i l e _ i n f o
 ************************************************************/
/************************************************************/
/** Read the Gribs block of the ingest config file
 *
 * @return TRUE if there were no errors
 ************************************************************/
static LOGICAL	read_grib_info
	(
	)
	{
	int			grib_edition=0;
	FILE		*fpcfg;
	STRING		cline, cmd;
	int			numbrace, arg;
	LOGICAL		firstline, valid;

	/* Do nothing if already input */
	if (Iread)	return Ivalid;

	if (!first_config_file_open(FpaGIngestsFile, &fpcfg) )
		{
		Iread = TRUE;
		return Ivalid = FALSE;
		}
	(void) pr_diag("Ingest", "Reading Ingest File!\n");

	/* Read the Ingest configuration file block by block
	 * Note that read_config_file_line() handles "include" lines
	 *  and closes each configuration file as the last line is read
	 */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{
		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Grib Block */
		if ( same(cmd, FpaGblockGrib) )
			{
			(void) pr_diag("Ingest", "Reading Grib Block!\n");
			/* Set counter for Grib Block */
			numbrace 	= 0;
			firstline	= TRUE;

			/* The next argument from the current line should
			 * be the GRIB edition */
			grib_edition = int_arg(cline,&valid);
			if ( !valid )
				{
				(void) pr_error("Ingest", "Grib block must have edition");
				Iread = TRUE;
				return Ivalid = FALSE;
				}
			/* Check grib edition is supported */
			else if ( grib_edition < 0 || grib_edition > 2)
				{
				(void) pr_error("Ingest", "Invalid GRIB edition [%d] in Grib block\n",
								grib_edition);
				Iread = TRUE;
				return Ivalid = FALSE;
				}

			/* Read Grib block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaGopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				if ( same(cmd,FpaGopenBrace) )
					{
					numbrace++;
					}
				/* Decrement counter for close brackets */
				else if ( same (cmd, FpaGcloseBrace) )
					{
					numbrace--;

					/* Check for end of Grib block */
					if ( numbrace == 0 ) break;
					}

				else if ( numbrace == 1 )
					{
					/* Not a brace thus must be one of: */
					if ( same(cmd, FpaGblockSource) )
						{ /* Sources Block */
						if (!read_source_info(grib_edition, fpcfg)) Ivalid = FALSE;
						}
					else if ( same(cmd, FpaGblockElem) )
						{ /* Elements Block */
						if (!read_element_info(grib_edition, fpcfg)) Ivalid = FALSE;
						}
					else if ( same(cmd, FpaGblockLevel) )
						{ /* Levels Block */
						if (!read_level_info(grib_edition, fpcfg)) Ivalid = FALSE;
						}
					else if ( same(cmd, FpaGblockField) )
						{ /* Fields Block */
						if (!read_field_info(grib_edition, fpcfg)) Ivalid = FALSE;
						}
					else if ( same(cmd, FpaGblockData) )
						{ /* DataFiles Block */
						if (!read_datafile_info(grib_edition, fpcfg)) Ivalid = FALSE;
						}
					else
						{	/* Unrecognized block */
						(void) pr_error("Ingest", "Unrecognizable block name:\"%s\"\n",
										cmd);
						(void) skip_to_end_of_block(&fpcfg);
						Ivalid = FALSE;
						}
					}
				}
			}
		}
	Iread = TRUE;
	return Ivalid;
	}

/************************************************************/
/** Read the Sources sub-block of the ingest config file
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*fpcfg	Pointer to current config file
 * @return	TRUE if no errors were found.
 ************************************************************/
static LOGICAL read_source_info
	(
	 int	edition,
	 FILE	*fpcfg
	)
	{
	STRING	cline, cmd, arg, source=NullString;
	int		numbrace, section_id, section;
	LOGICAL	firstline;
	FpaIngestSourceStruct *sdef = NullPtr(FpaIngestSourceStruct *);

	/* Diagnostic message */
	(void)	pr_diag("Ingest", "Reading Sources block!\n");

	numbrace 	= 0;
	section  	= FpaGblockSourceName;
	section_id	= FpaGnoSection;
	firstline	= TRUE;

	/* Read Sources block line by line */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if (firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaGopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaGopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}
		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaGcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Sources block */
			if (numbrace == 0 ) break;
			}
		else if (numbrace == 1 )
			{

			/* Adding another name in the FpaGblockSourceName section */
			if ( section_id == FpaGblockSourceName )
				{
				if ( NotNull(source) ) FREEMEM(source);
				source = strdup(cmd);
				/* Set identifier for next section of Sources block */
				section = FpaGblockSourceInfo;
				}
			else
				{
				(void) pr_error("Ingest", "Error reading Sources block!\n");
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		/* Set parameters in Sources declarations */
		/*  ... with format of "cmd = value(s) "  */
		else
			{
			/* Adding parameters in FpaGblockSourceInfo section */
			if ( section_id == FpaGblockSourceInfo )
				{
				/* Grib parameters */
				if ( same(cmd, "parameters") )
					{
					if ( same(string_arg(cline), FpaGequalSign) )
						{
						sdef 	= init_source(edition, source);
						switch (edition)
							{
							case 0:
							case 1:
								/* Read the centre id */
								if ( !strtoushrt(string_arg(cline), &sdef->centre) )
									sdef->valid = FALSE;
								/* Process id */
								if ( !strtouchar(string_arg(cline), &sdef->process) )
									sdef->valid = FALSE;
								/* If centre is a wild card then so must process */
								else if (sdef->centre == USHRT_MAX
										&& sdef->process != UCHAR_MAX)
									{
									sdef->valid = FALSE;
									(void) pr_warning("Ingest",
													  "Problem reading source \"%s\"\n",
													  sdef->name);
									(void) pr_warning("Ingest",
													  "\tprocess requires centre id.\n");
									}
								break;
							case 2:
								/* Read the centre id */
								if ( !strtoushrt(string_arg(cline), &sdef->centre) )
									sdef->valid = FALSE;
								/* sub_centre id */
								if ( !strtoushrt(string_arg(cline), &sdef->sub_centre) )
									sdef->valid = FALSE;
								/* If centre is a wild card then so must sub_centre */
								else if (sdef->centre == USHRT_MAX
										&& sdef->sub_centre != USHRT_MAX)
									{
									sdef->valid = FALSE;
									(void) pr_warning("Ingest",
													  "Problem reading source \"%s\"\n",
													  sdef->name);
									(void) pr_warning("Ingest",
													  "\tsub_centre requires centre id.\n");
									}
								/* Template */
								if ( !strtoushrt(string_arg(cline), &sdef->template) )
									sdef->valid = FALSE;
								/* Forecast/Analysis type */
								if ( !strtouchar(string_arg(cline), &sdef->type) )
									sdef->valid = FALSE;
								/* Forecast requires template to be set */
								else if (sdef->template == USHRT_MAX &&
										sdef->type != UCHAR_MAX)
									{
									sdef->valid = FALSE;
									(void) pr_warning("Ingest",
											"Problem reading source \"%s\"\n",
											sdef->name);
									(void) pr_warning("Ingest",
													  "\ttype requires template.\n");
									}
								/* Process */
								if ( !strtouchar(string_arg(cline), &sdef->process) )
									sdef->valid = FALSE;
								else if (sdef->type == UCHAR_MAX &&
										sdef->process != UCHAR_MAX)
									{
									sdef->valid = FALSE;
									(void) pr_warning("Ingest",
													  "Problem reading source \"%s\"\n",
													  sdef->name);
									(void) pr_warning("Ingest",
													  "\tprocess requires forecat type.\n");
									}
								/* Background Process */
								if ( !strtouchar(string_arg(cline), &sdef->bkgd_process))
										sdef->valid = FALSE;
								else if (sdef->process == UCHAR_MAX &&
										sdef->bkgd_process != UCHAR_MAX)
									{
									sdef->valid = FALSE;
									(void) pr_warning("Ingest",
													  "Problem reading source \"%s\"\n",
													  sdef->name);
									(void) pr_warning("Ingest",
													  "\tbkgd_process requires process.\n");
									}
								break;
							}

						if ( !sdef->valid)
						(void) pr_warning("Ingest",
										"Unable to read parameters for source \"%s\"\n",
										sdef->name);
						simple_sort_sources(edition);
						}
					else
						{
						(void) pr_error("Ingest", "Missing \"=\" in Sources block!\n");
						(void) skip_to_end_of_block(&fpcfg);
						Ivalid = FALSE;
						continue;
						}
					}
				}
			else
				{
				(void) pr_error("Ingest",
								"Unexpected keyword in Sources block: \"%s\"\n", cmd);
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		}/* End While */
	if ( NotNull(source) ) FREEMEM(source);
	return Ivalid;
	}

/************************************************************/
/** Read the Elements sub-block of the ingest config file
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*fpcfg	Pointer to current config file
 * @return	TRUE if no errors were found.
 ************************************************************/
static LOGICAL read_element_info
	(
	 int	edition,
	 FILE	*fpcfg
	)
	{
	STRING	cline, cmd, arg, element = NullString, units = NullString;
	int		numbrace, section_id, section, nsources=0, ii;
	LOGICAL	firstline;
	FpaIngestSourceListStruct 	**sources = NullPtr(FpaIngestSourceListStruct **);
	FpaIngestElementStruct 		*edef  = NullPtr(FpaIngestElementStruct *);

	/* Diagnostic message */
	(void)	pr_diag("Ingest", "Reading Elements block!\n");

	numbrace 	= 0;
	section  	= FpaGblockESourceList;
	section_id	= FpaGnoSection;
	firstline	= TRUE;

	/* Read Elements block line by line */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if (firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaGopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaGopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}
		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaGcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Elements block */
			if (numbrace == 0 ) break;
			}
		else if (numbrace == 1 )
			{

			if ( section_id == FpaGblockESourceList )
				{
				nsources = 0;
				while ( !blank(cmd) )
					{
					/* Add to the list of sources to work on */
					nsources++;
					sources = GETMEM(sources, FpaIngestSourceListStruct *, nsources);
					sources[nsources-1] = find_source_list(edition, cmd);
					if ( IsNull(sources[nsources-1]) )
						sources[nsources-1] = init_source_list(edition, cmd);

					/* Extract the next argument from the current line */
					cmd = string_arg(cline);
					}
				/* Set identifier for next section of Elements block */
				section = FpaGblockElementName;
				}
			else
				{
				(void) pr_error("Ingest", "Error reading Elements block!\n");
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		/* Element name subblock */
		else if (numbrace == 2)
			{
			/* New Element name */
			if ( section_id == FpaGblockElementName )
				{
				if ( NotNull(element) ) FREEMEM(element);
				if ( NotNull(units) )   FREEMEM(units);
				element    = strdup(cmd);
				units      = strdup_arg(cline);
				/* Make sure that element and unit tags are valid */
				if ( blank(element) )
					{
					(void) pr_error("Ingest", "Missing element tag!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				if ( blank(units) )
					{
					(void) pr_error("Ingest", "Missing units tag!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				section    = FpaGblockElementInfo;
				}
			else
				{
				(void) pr_error("Ingest", "Error reading Elements block!\n");
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		else
			{
			if ( section_id == FpaGblockElementInfo )
				{
				/* Grib parameters */
				if ( same(cmd, "parameters") )
					{
					if ( same(string_arg(cline), FpaGequalSign) )
						{
						edef 	= init_element(edition, element, units);

						switch (edition)
							{
							case 0:
							case 1:
								/* category == Parameter Table version number in GRIB1 */
								if ( !strtouchar(string_arg(cline), &edef->category ) )
									edef->valid = FALSE;
								if ( !strtouchar(string_arg(cline), &edef->parameter ) )
									edef->valid = FALSE;
								/* Parameter cannot be a wildcard */
								else if (edef->parameter == UCHAR_MAX )
									{
									edef->valid = FALSE;
									(void) pr_warning("Ingest",
											"Element \"%s\" parameter not specified\n",
											edef->element);
									}
								break;
							case 2:
								if ( !strtouchar(string_arg(cline), &edef->discipline) )
									edef->valid = FALSE;
								/* Discipline cannot be a wildcard */
								else if (edef->discipline == UCHAR_MAX )
									{
									edef->valid = FALSE;
									(void) pr_warning("Ingest",
											"Element \"%s\" discipline not specified\n",
											edef->element);
									}
								if ( !strtoushrt(string_arg(cline), &edef->template) )
									edef->valid = FALSE;
								if ( !strtouchar(string_arg(cline), &edef->category) )
									edef->valid = FALSE;
								/* Category cannot be a wildcard */
								else if (edef->category == UCHAR_MAX )
									{
									edef->valid = FALSE;
									(void) pr_warning("Ingest",
											"Element \"%s\" category not specified\n",
											edef->element);
									}
								if ( !strtouchar(string_arg(cline), &edef->parameter) )
									edef->valid = FALSE;
								/* Parameter cannot be a wildcard */
								else if (edef->parameter == UCHAR_MAX )
									{
									edef->valid = FALSE;
									(void) pr_warning("Ingest",
											"Element \"%s\" paramater not specified\n",
											edef->element);
									}
								break;
							}
						/* Each source should add new element if it's valid */
						if (edef->valid)
							for (ii = 0; ii < nsources; ii++)
								(void) add_element_to_list(edition,sources[ii],edef);
						else
							(void) pr_warning("Ingest",
										"Unable to read parameters for element \"%s\"\n",
										edef->element);
						}
					else
						{
						(void) pr_error("Ingest", "Missing \"=\" in Elements block!\n");
						(void) skip_to_end_of_block(&fpcfg);
						Ivalid = FALSE;
						continue;
						}
					}
				}
			else
				{
				(void) pr_error("Ingest",
								"Unexpected keyword in Elements block: \"%s\"\n", cmd);
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		}/* End While */
	FREEMEM(sources);	/* Release memory for temporary sources list. */
	if (NotNull(element))	FREEMEM(element);
	if (NotNull(units))		FREEMEM(units);
	return Ivalid;
	}

/************************************************************/
/** Read the Levels sub-block of the ingest config file
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*fpcfg	Pointer to current config file
 * @return	TRUE if no errors were found.
 ************************************************************/
static LOGICAL read_level_info
	(
	 int	edition,
	 FILE	*fpcfg
	)
	{
	STRING	cline, cmd, arg, tag = NullString;
	int		numbrace, section_id, section, nsources=0, ii;
	LOGICAL	firstline, valid;

	FpaIngestSourceListStruct 	**sources 	= NullPtr(FpaIngestSourceListStruct **);
	FpaIngestLevelStruct 		*ldef  		= NullPtr(FpaIngestLevelStruct *);

	/* Diagnostic message */
	(void)	pr_diag("Ingest", "Reading Levels block!\n");

	numbrace 	= 0;
	section  	= FpaGblockLSourceList;
	section_id	= FpaGnoSection;
	firstline	= TRUE;

	/* Read Levels block line by line */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if (firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaGopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaGopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}
		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaGcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Levels block */
			if (numbrace == 0 ) break;
			}
		else if (numbrace == 1 )
			{

			if ( section_id == FpaGblockLSourceList )
				{
				nsources = 0;	/* Reset temporary list */
				while ( !blank(cmd) )
					{
					/* Add to the list of sources to work on */
					nsources++;
					sources = GETMEM(sources, FpaIngestSourceListStruct *, nsources);
					sources[nsources-1] = find_source_list(edition, cmd);
					if ( IsNull(sources[nsources-1]) )
						sources[nsources-1] = init_source_list(edition, cmd);

					/* Extract the next argument from the current line */
					cmd = string_arg(cline);
					}
				/* Set identifier for next section of Levels block */
				section = FpaGblockLevelName;
				}
			else
				{
				(void) pr_error("Ingest", "Error reading Levels block!\n");
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		/* Level name subblock */
		else if (numbrace == 2)
			{
			/* New Level name */
			if ( section_id == FpaGblockLevelName )
				{
				if ( NotNull(tag) ) FREEMEM(tag);
				tag     = strdup(cmd);
				/* Make sure the tag is a valid string */
				if ( blank(tag) )
					{
					(void) pr_error("Ingest", "Error reading Levels block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				section = FpaGblockLevelInfo;
				}
			else
				{
				(void) pr_error("Ingest", "Error reading Levels block!\n");
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		else
			{
			if ( section_id == FpaGblockLevelInfo )
				{
				/* Grib parameters */
				if ( same(cmd, "parameters") )
					{
					if ( same(string_arg(cline), FpaGequalSign) )
						{
						ldef 	= init_level(edition, tag);
						if ( !strtouchar(string_arg(cline), &ldef->type ) ||
								(ldef->type == UCHAR_MAX) )
							{
							ldef->valid = FALSE;
							(void) pr_warning("Ingest", "Invalid level type for \"%s\"!\n",
											ldef->tag);
							}
						if ( !strtouchar(string_arg(cline), &ldef->id ) ||
								(ldef->id == UCHAR_MAX) )
							{
							ldef->valid = FALSE;
							(void) pr_warning("Ingest", "Invalid level id for \"%s\"!\n",
											ldef->tag);
							}
						ldef->scale = float_arg(cline, &valid);
						if ( !valid )
							{
							ldef->valid = FALSE;
							(void) pr_warning("Ingest", "Invalid level scale for \"%s\"!\n",
											ldef->tag);
							}
						ldef->offset = float_arg(cline, &valid);
						if ( !valid )
							{
							ldef->valid = FALSE;
							(void) pr_warning("Ingest", "Invalid level offset for \"%s\"!\n",
											ldef->tag);
							}
						if ( edition == 1 || edition == 0)
							{
							ldef->scale2 = float_arg(cline, &valid);
							if ( !valid )
								{
								ldef->valid = FALSE;
								(void) pr_warning("Ingest",
												"Invalid level scale2 for \"%s\"!\n",
												ldef->tag);
								}
							ldef->offset2 = float_arg(cline, &valid);
							if ( !valid )
								{
								ldef->valid = FALSE;
								(void) pr_warning("Ingest",
												"Invalid level offset2 for \"%s\"!\n",
												ldef->tag);
								}
							}

						/* Each source should add new level if it's valid */
						if (ldef->valid)
							for (ii = 0; ii < nsources; ii++)
								(void) add_level_to_list(edition,sources[ii],ldef);
						else
							(void) pr_warning("Ingest",
										"Unable to read parameters for level \"%s\"\n",
										ldef->tag);

						}
					else
						{
						(void) pr_error("Ingest", "Missing \"=\" in Levels block!\n");
						(void) skip_to_end_of_block(&fpcfg);
						Ivalid = FALSE;
						continue;
						}
					}
				}
			else
				{
				(void) pr_error("Ingest",
								"Unexpected keyword in Levels block: \"%s\"\n", cmd);
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		}/* End While */
	FREEMEM(sources);	/* Release memory for temporary sources list. */
	if ( NotNull(tag) ) FREEMEM(tag);
	return Ivalid;
	}

/************************************************************/
/** Read the Fields sub-block of the ingest config file
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*fpcfg	Pointer to current config file
 * @return	TRUE if no errors were found.
 ************************************************************/
static LOGICAL read_field_info
	(
	 int	edition,
	 FILE	*fpcfg
	)
	{
	STRING	cline, cmd, arg;
	int		numbrace;
	LOGICAL	firstline, valid;

	FpaIngestFieldStruct	*fptr	= NullPtr(FpaIngestFieldStruct *);
	FpaIngestRedirectStruct	*rptr	= NullPtr(FpaIngestRedirectStruct *);

	/* Diagnostic message */
	(void)	pr_diag("Ingest", "Reading Fields block!\n");

	numbrace 	= 0;
	firstline	= TRUE;

	/* Read Fields block line by line */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if (firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaGopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		if ( same(cmd, FpaGopenBrace) )
			{
			numbrace++;
			}
		/* Decrement counter for close brackets */
		else if ( same(cmd, FpaGcloseBrace) )
			{
			numbrace--;

			/* Check for end of Fields block */
			if (numbrace == 0 ) break;
			}
		else if (numbrace == 1 )
			{
			if ( same(cmd, "process") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					fptr = init_process_field(edition, TRUE);
					fptr->source  = strdup_arg(cline);
					fptr->element = strdup_arg(cline);
					fptr->level   = strdup_arg(cline);
					/* Check the parameters */
					if ( blank(fptr->source) || blank(fptr->element)
							|| blank(fptr->level))
						{
						(void) pr_warning("Ingest","Field process list requires\n");
						(void) pr_warning("Ingest","	source, element and level\n");
						continue;
						}
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in Fields block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else if ( same(cmd, "skip") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					fptr = init_process_field(edition, FALSE);
					fptr->source  = strdup_arg(cline);
					fptr->element = strdup_arg(cline);
					fptr->level   = strdup_arg(cline);
					/* Check the parameters */
					if ( blank(fptr->source) || blank(fptr->element)
							|| blank(fptr->level))
						{
						(void) pr_warning("Ingest","Field skip list requires\n");
						(void) pr_warning("Ingest","	source, element and level\n");
						continue;
						}
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in Fields block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else if ( same(cmd, "redirect") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					rptr = init_redirect_field(edition);
					rptr->source_from  = strdup_arg(cline);
					rptr->source_to    = strdup_arg(cline);
					/* Check the parameters */
					if ( blank(rptr->source_from) || blank(rptr->source_to) )
						{
						(void) pr_warning("Ingest","Field redirect list requires\n");
						(void) pr_warning("Ingest","	source_from and source_to\n");
						continue;
						}
					(void) simple_sort_redirect_field(edition);
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in Fields block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else
				{
				(void) pr_error("Ingest",
								"Unexpected keyword in Fields block: \"%s\"\n", cmd);
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		}/* End While */
	return Ivalid;
	}

/************************************************************/
/** Read the DataFiles sub-block of the ingest config file
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*fpcfg	Pointer to current config file
 * @return	TRUE if no errors were found.
 ************************************************************/
static LOGICAL read_datafile_info
	(
	 int	edition,
	 FILE	*fpcfg
	)
	{
	STRING	cline, cmd, arg;
	int		numbrace;
	LOGICAL	firstline, valid;
	FpaIngestFieldStruct	*fptr	= NullPtr(FpaIngestFieldStruct *);
	FpaIngestRedirectStruct	*rptr	= NullPtr(FpaIngestRedirectStruct *);
	FpaIngestRescaleStruct	*sptr	= NullPtr(FpaIngestRescaleStruct *);

	/* Diagnostic message */
	(void)	pr_diag("Ingest", "Reading DataFiles block!\n");

	numbrace 	= 0;
	firstline	= TRUE;

	/* Read DataFiles block line by line */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if (firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaGopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		if ( same(cmd, FpaGopenBrace) )
			{
			numbrace++;
			}
		/* Decrement counter for close brackets */
		else if ( same(cmd, FpaGcloseBrace) )
			{
			numbrace--;

			/* Check for end of DataFiles block */
			if (numbrace == 0 ) break;
			}
		else if (numbrace == 1 )
			{
			if ( same(cmd, "process") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					fptr = init_process_datafile(edition, TRUE);
					fptr->source  = strdup_arg(cline);
					fptr->element = strdup_arg(cline);
					fptr->level   = strdup_arg(cline);
					/* Check the parameters */
					if ( blank(fptr->source) || blank(fptr->element)
							|| blank(fptr->level))
						{
						(void) pr_warning("Ingest","DataFiles process list requires\n");
						(void) pr_warning("Ingest","	source, element and level\n");
						continue;
						}
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in DataFiles block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else if ( same(cmd, "skip") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					fptr = init_process_datafile(edition, FALSE);
					fptr->source  = strdup_arg(cline);
					fptr->element = strdup_arg(cline);
					fptr->level   = strdup_arg(cline);
					/* Check the parameters */
					if ( blank(fptr->source) || blank(fptr->element)
							|| blank(fptr->level))
						{
						(void) pr_warning("Ingest","DataFiles skip list requires\n");
						(void) pr_warning("Ingest","	source, element and level\n");
						continue;
						}
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in DataFiles block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else if ( same(cmd, "redirect") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					rptr = init_redirect_datafile(edition);
					rptr->source_from  = strdup_arg(cline);
					rptr->source_to    = strdup_arg(cline);
					/* Check the parameters */
					if ( blank(rptr->source_from) || blank(rptr->source_to) )
						{
						(void) pr_warning("Ingest","DataFiles redirect list requires\n");
						(void) pr_warning("Ingest","	source_from and source_to\n");
						continue;
						}
					(void) simple_sort_redirect_data(edition);
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in DataFiles block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else if ( same(cmd, "rescale") )
				{
				if ( same(string_arg(cline), FpaGequalSign) )
					{
					sptr = init_rescale_datafile(edition);
					sptr->element  = strdup_arg(cline);
					sptr->level    = strdup_arg(cline);
					sptr->scale	   = float_arg(cline, &valid);
					if (valid)
						sptr->offset   = float_arg(cline, &valid);

					/* Check the parameters */
					if ( blank(sptr->element) || blank(sptr->level) || !valid )
						{
						(void) pr_warning("Ingest","DataFiles rescale list requires\n");
						(void) pr_warning("Ingest",
										  "	element, level, scale and offset.\n");
						continue;
						}
					(void) simple_sort_rescale_data(edition);
					}
				else
					{
					(void) pr_error("Ingest", "Missing \"=\" in DataFiles block!\n");
					(void) skip_to_end_of_block(&fpcfg);
					Ivalid = FALSE;
					continue;
					}
				}
			else
				{
				(void) pr_error("Ingest",
								"Unexpected keyword in DataFiles block: \"%s\"\n", cmd);
				(void) skip_to_end_of_block(&fpcfg);
				Ivalid = FALSE;
				continue;
				}
			}
		}/* End While */
	return Ivalid;
	}

/************************************************************/
/* FIND LIST ITEMS
 *
 * f i n d _ s o u r c e
 * f i n d _ s o u r c e _ l i s t
 * f i n d _ e l e m e n t
 * f i n d _ l e v e l
 ************************************************************/
/************************************************************/
/** Find a source in the list of sources.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*params	List of parameters.
 * 							Size is determined by edition
 * @return	Pointer to a source if found, Null otherwise.
 ************************************************************/
static  FpaIngestSourceStruct	*find_source
	(
	 int	edition,
	 int	*params
	)
	{
	/* Search by parameters */
	FpaIngestSourceStruct	**sources 	= NullPtr(FpaIngestSourceStruct **);
	FpaIngestSourceStruct	*sdef		= NullPtr(FpaIngestSourceStruct *);
	int						nsources, ii;

	/* Choose the appropriate list */
	switch (edition)
		{
		case 0:
		case 1:
			sources = GRIB1Sources;
			nsources = NumGRIB1Sources;
			break;
		case 2:
			sources = GRIB2Sources;
			nsources = NumGRIB2Sources;
			break;
		}

	/* Return if there are no sources to search */
	if (nsources < 1) return NullPtr(FpaIngestSourceStruct *);

	switch (edition)	/* Search according to edition # */
		{
		case 0:
		case 1:
			for (ii=0; ii<nsources; ii++)
				{
				sdef = sources[ii];
				if ( (sdef->valid ) &&
					 (sdef->centre  == params[0] || sdef->centre  == USHRT_MAX ) &&
					 (sdef->process == params[1] || sdef->process == UCHAR_MAX ) )
						{
						return sdef;
						}
				}
			break;
		case 2:
			for (ii=0; ii<nsources; ii++)
				{
				sdef = sources[ii];
				if ((sdef->valid ) &&
					(sdef->centre       == params[0] || sdef->centre     == USHRT_MAX ) &&
					(sdef->sub_centre   == params[1] || sdef->sub_centre == USHRT_MAX ) &&
					(sdef->template     == params[2] || sdef->template   == USHRT_MAX ) &&
					(sdef->type         == params[3] || sdef->type       == UCHAR_MAX ) &&
					(sdef->process      == params[4] || sdef->process    == UCHAR_MAX ) &&
					(sdef->bkgd_process == params[5] || sdef->bkgd_process == UCHAR_MAX ))
						{
						return sdef;
						}
				}
		}
	/* Return null if nothing was found */
	return NullPtr(FpaIngestSourceStruct *);
	}

/************************************************************/
/** Find an element/level list for a given source name.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	source	Name of the source list to find
 * @return pointer to a souce list matching the given source.
 ************************************************************/
static  FpaIngestSourceListStruct	*find_source_list
	(
	 int	edition,
	 STRING	source
	)
	{
	int ii, num;
	FpaIngestSourceListStruct **slist = NullPtr(FpaIngestSourceListStruct **);
	FpaIngestSourceListStruct *found  = NullPtr(FpaIngestSourceListStruct *);

	/* Pick the appropriate list by edition */
	switch (edition)
		{
		case 0:
		case 1:
			slist = GRIB1SLists;
			num   = NumGRIB1SLists;
			break;
		case 2:
			slist = GRIB2SLists;
			num   = NumGRIB2SLists;
			break;
		}

	/* Search by Source name */
	for (ii=0; ii< num; ii++)
		{
		found = slist[ii];
		if ( same(found->source, source) ) return found;
		}

	return NullPtr(FpaIngestSourceListStruct *);
	}

/************************************************************/
/** Find an element in a given element list
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*params	list of parameters.
 * 					Size of list determined by edition.
 * @param[in]	*slist	List of elements to search.
 * @return Pointer to element if found, Null otherwise.
 ************************************************************/
static  FpaIngestElementStruct	*find_element
	(
	 int						edition,
	 int						*params,
	 FpaIngestSourceListStruct 	*slist

	)
	{
	int ii;
	FpaIngestElementStruct *elem	= NullPtr(FpaIngestElementStruct *);

	/* Return if list is empty */
	if ( IsNull(slist) || slist->nelements < 1 ) return NullPtr(FpaIngestElementStruct *);

	/* Search by parameters */
	switch (edition)
		{
		case 0:
		case 1:
			/* For edition 1, category is being used for
			 * the Parameter table version number */
			for (ii = 0; ii< slist->nelements; ii++)
				{
				elem = slist->elements[ii];
				if ( (elem->valid) &&
				 	(elem->parameter == params[1] ) &&
				 	(elem->category == params[0] || elem->category == UCHAR_MAX) )
					return elem;
				}
			break;
		case 2:
			for (ii = 0; ii< slist->nelements; ii++)
				{
				elem = slist->elements[ii];
				if ( (elem->valid) &&
				 	(elem->discipline == params[0] ) &&
				 	(elem->category   == params[1] ) &&
				 	(elem->parameter  == params[2] ) &&
				 	(elem->template   == params[3] || elem->template   == USHRT_MAX) )
					return elem;
				}
			break;
		}

	/* No matching element was found */
	return NullPtr(FpaIngestElementStruct *);
	}

/************************************************************/
/** Find a level in a given level list
 *
 * @param[in]	edition	GRIB edition
 * @param[in]	id		Level id
 * @param[in]	slist	List of levels to search.
 * @return pointer to level if found, null otherwise.
 ************************************************************/
static  FpaIngestLevelStruct	*find_level
	(
	 int						edition,
	 int						id,
	 FpaIngestSourceListStruct 	*slist
	)
	{
	/* Search by parameters */
	int ii;
	FpaIngestLevelStruct *lev	= NullPtr(FpaIngestLevelStruct *);

	/* Return if list is empty */
	if ( IsNull(slist) || slist->nlevels < 1 ) return NullPtr(FpaIngestLevelStruct *);

	for (ii = 0; ii< slist->nlevels; ii++)
		{
		lev = slist->levels[ii];
		if ( (lev->valid) &&
		 	(lev->id == id || lev->id == UCHAR_MAX) )
			return lev;
		}

	/* No matching level was found */
	return NullPtr(FpaIngestLevelStruct *);
	}

/************************************************************/
/* INITIALIZE AND ADD NEW STRUCTURES TO LIST
 *
 * i n i t _ s o u r c e
 * i n i t _ s o u r c e _ l i s t
 * i n i t _ e l e m e n t
 * i n i t _ l e v e l
 ************************************************************/
/************************************************************/
/** Add a new source to the source list
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	source	new source name.
 * @return		pointer to new souce struct.
 ************************************************************/
static  FpaIngestSourceStruct	*init_source
	(
	 int	edition,
	 STRING source
	)
	{
	FpaIngestSourceStruct	*sdef	= NullPtr(FpaIngestSourceStruct *);

	/* return nothing if source is empty */
	if ( blank(source) )  return NullPtr(FpaIngestSourceStruct *);

	/*Add source at end of current Source list */
	switch (edition)
		{
		case 0:
		case 1:
			NumGRIB1Sources++;
			GRIB1Sources = GETMEM(GRIB1Sources, FpaIngestSourceStruct *, NumGRIB1Sources);
			GRIB1Sources[NumGRIB1Sources-1] = INITMEM(FpaIngestSourceStruct,1);
			sdef = GRIB1Sources[NumGRIB1Sources-1];
			break;
		case 2:
			NumGRIB2Sources++;
			GRIB2Sources = GETMEM(GRIB2Sources, FpaIngestSourceStruct *, NumGRIB2Sources);
			GRIB2Sources[NumGRIB2Sources-1] = INITMEM(FpaIngestSourceStruct,1);
			sdef = GRIB2Sources[NumGRIB2Sources-1];
			break;
		}
	sdef->name   		= strdup(source);
	sdef->centre 		= USHRT_MAX;	/* Initialize as missing */
	sdef->sub_centre 	= USHRT_MAX;	/* Initialize as missing */
	sdef->template	 	= USHRT_MAX;	/* Initialize as missing */
	sdef->type		 	= UCHAR_MAX;	/* Initialize as missing */
	sdef->process	 	= UCHAR_MAX;	/* Initialize as missing */
	sdef->bkgd_process	= UCHAR_MAX;	/* Initialize as missing */
	sdef->valid 		= TRUE;
	return sdef;
	}

/************************************************************/
/** Initialize an element list for a given source.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	source	Name of source list.
 * @return		pointer to a source list structure.
 ************************************************************/
static	FpaIngestSourceListStruct *init_source_list
	(
	 int	edition,
	 STRING source
	)
	{
	FpaIngestSourceListStruct		*slist = NullPtr(FpaIngestSourceListStruct *);

	if ( blank(source) )	return NullPtr(FpaIngestSourceListStruct *);

	/*Add source a end of current Source list */
	switch (edition)
		{
		case 0:
		case 1:
			NumGRIB1SLists++;
			GRIB1SLists = GETMEM(GRIB1SLists,
					FpaIngestSourceListStruct *, NumGRIB1SLists);
			GRIB1SLists[NumGRIB1SLists-1] = INITMEM(FpaIngestSourceListStruct,1);
			slist = GRIB1SLists[NumGRIB1SLists-1];
			break;
		case 2:
			NumGRIB2SLists++;
			GRIB2SLists = GETMEM(GRIB2SLists,
					FpaIngestSourceListStruct *, NumGRIB2SLists);
			GRIB2SLists[NumGRIB2SLists-1] = INITMEM(FpaIngestSourceListStruct,1);
			slist = GRIB2SLists[NumGRIB2SLists-1];
			break;
		}

	slist->source = strdup(source);
	slist->nelements = 0;
	slist->nlevels	 = 0;
	slist->elements	 = NullPtr(FpaIngestElementStruct **);
	slist->levels	 = NullPtr(FpaIngestLevelStruct **);

	return slist;
	}

/************************************************************/
/** Insert pointer to element struct in source list
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*slist	Source list to add the element to.
 * @param[in]	*edef	Element to add.
 ************************************************************/
static void add_element_to_list
	(
	 int						edition,
	 FpaIngestSourceListStruct 	*slist,
	 FpaIngestElementStruct 	*edef
	 )
	{
	slist->nelements++;
	slist->elements = GETMEM(slist->elements, FpaIngestElementStruct *, slist->nelements);
	slist->elements[slist->nelements-1] = edef;
	simple_sort_elements(slist);
	}

/************************************************************/
/** Insert pointer to level struct in source list
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	*slist	Source list to add the level to.
 * @param[in]	*edef	Level to add.
 ************************************************************/
static void add_level_to_list
	(
	 int						edition,
	 FpaIngestSourceListStruct 	*slist,
	 FpaIngestLevelStruct 		*ldef
	 )
	{
	slist->nlevels++;
	slist->levels = GETMEM(slist->levels, FpaIngestLevelStruct *, slist->nlevels);
	slist->levels[slist->nlevels-1] = ldef;
	simple_sort_levels(slist);
	}

/************************************************************/
/** Add a new element to the master list of elements
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	element	Element tag
 * @param[in]	units	Unit tag
 * @return pointer to the new element.
 ************************************************************/
static  FpaIngestElementStruct	*init_element
	(
	 int	edition,
	 STRING element,
	 STRING units
	)
	{
	FpaIngestElementStruct		*edef = NullPtr(FpaIngestElementStruct *);

	/* return nothing if element or units are blank */
	if ( blank(element) )	return NullPtr(FpaIngestElementStruct *);
	if ( blank(units) )		return NullPtr(FpaIngestElementStruct *);

	/* Add element a end of current element list */
	NumGRIBElements++;
	GRIBElements = GETMEM(GRIBElements, FpaIngestElementStruct *, NumGRIBElements);
	GRIBElements[NumGRIBElements-1] = INITMEM(FpaIngestElementStruct,1);
	edef = GRIBElements[NumGRIBElements-1];

	/* Initialize values */
	edef->element       = strdup(element);
	edef->units         = strdup(units);
	edef->discipline	= UCHAR_MAX;	/* Initialize as missing */
	edef->category		= UCHAR_MAX;	/* Initialize as missing */
	edef->parameter		= UCHAR_MAX;	/* Initialize as missing */
	edef->template		= USHRT_MAX;	/* Initialize as missing */
	edef->valid			= TRUE;
	return edef;
	}

/************************************************************/
/** Add a new level to the master list of levels.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	level	Level tag
 * @return pointer to the new level
 ************************************************************/
static  FpaIngestLevelStruct	*init_level
	(
	 int	edition,
	 STRING level
	)
	{
	FpaIngestLevelStruct		*ldef = NullPtr(FpaIngestLevelStruct *);

	/* Return nothing if level is empty */
	if ( blank(level) )	return NullPtr(FpaIngestLevelStruct *);

	/*Add Level a end of current Level list */
	NumGRIBLevels++;
	GRIBLevels = GETMEM(GRIBLevels, FpaIngestLevelStruct *, NumGRIBLevels);
	GRIBLevels[NumGRIBLevels-1] = INITMEM(FpaIngestLevelStruct,1);
	ldef = GRIBLevels[NumGRIBLevels-1];

	/* Initialize values */
	ldef->tag		= strdup(level);
	ldef->type		= UCHAR_MAX;	/* Initialize to missing */
	ldef->id		= UCHAR_MAX;	/* Initialize to missing */
	ldef->scale		= 0.0;			/* Initialize to 0.0 */
	ldef->offset	= 0.0;			/* Initialize to 0.0 */
	ldef->scale2	= 0.0;			/* Initialize to 0.0 */
	ldef->offset2	= 0.0;			/* Initialize to 0.0 */
	ldef->valid		= TRUE;
	return ldef;
	}

/************************************************************/
/** Add a field to the process or skip lists.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	process	Process or skip specified fields?
 * @return	pointer to new Field structure.
 ************************************************************/
static  FpaIngestFieldStruct		*init_process_field
	(
	 int edition,
	 LOGICAL process
	 )
	{
	switch(edition)
		{
		case 0:
		case 1:
			/* return pointer to process list */
			if ( process )
				{
				GRIB1Metafiles.nprocess++;
				GRIB1Metafiles.process = GETMEM(GRIB1Metafiles.process,
						FpaIngestFieldStruct , GRIB1Metafiles.nprocess);
				return &(GRIB1Metafiles.process[GRIB1Metafiles.nprocess-1]);
				}
			/* return pointer to skip list */
			else
				{
				GRIB1Metafiles.nskip++;
				GRIB1Metafiles.skip = GETMEM(GRIB1Metafiles.skip,
						FpaIngestFieldStruct , GRIB1Metafiles.nskip);
				return &(GRIB1Metafiles.skip[GRIB1Metafiles.nskip-1]);
				}
			break;
		case 2:
			/* return pointer to process list */
			if ( process )
				{
				GRIB2Metafiles.nprocess++;
				GRIB2Metafiles.process = GETMEM(GRIB2Metafiles.process,
						FpaIngestFieldStruct , GRIB2Metafiles.nprocess);
				return &(GRIB2Metafiles.process[GRIB2Metafiles.nprocess-1]);
				}
			/* return pointer to skip list */
			else
				{
				GRIB2Metafiles.nskip++;
				GRIB2Metafiles.skip = GETMEM(GRIB2Metafiles.skip,
						FpaIngestFieldStruct , GRIB2Metafiles.nskip);
				return &(GRIB2Metafiles.skip[GRIB2Metafiles.nskip-1]);
				}
			break;
		}
		/* Unknown Grib Edition */
		return NullPtr(FpaIngestFieldStruct *);
	}

/************************************************************/
/** Add a datafile to the process or skip lists.
 *
 * @param[in]	edition	GRIB edition \#
 * @param[in]	process	Process or skip?
 * @return	Pointer to new Field structure
 ************************************************************/
static  FpaIngestFieldStruct		*init_process_datafile
	(
	 int edition,
	 LOGICAL process
	 )
	{
	/* Chose actions base on edition # */
	switch(edition)
		{
		case 0:
		case 1:
			/* return pointer to process list */
			if ( process )
				{
				GRIB1Datafiles.nprocess++;
				GRIB1Datafiles.process = GETMEM(GRIB1Datafiles.process,
						FpaIngestFieldStruct , GRIB1Datafiles.nprocess);
				return &(GRIB1Datafiles.process[GRIB1Datafiles.nprocess-1]);
				}
			/* return pointer to skip list */
			else
				{
				GRIB1Datafiles.nskip++;
				GRIB1Datafiles.skip = GETMEM(GRIB1Datafiles.skip,
						FpaIngestFieldStruct , GRIB1Datafiles.nskip);
				return &(GRIB1Datafiles.skip[GRIB1Datafiles.nskip-1]);
				}
			break;
		case 2:
			/* return pointer to process list */
			if ( process )
				{
				GRIB2Datafiles.nprocess++;
				GRIB2Datafiles.process = GETMEM(GRIB2Datafiles.process,
						FpaIngestFieldStruct , GRIB2Datafiles.nprocess);
				return &(GRIB2Datafiles.process[GRIB2Datafiles.nprocess-1]);
				}
			/* return pointer to skip list */
			else
				{
				GRIB2Datafiles.nskip++;
				GRIB2Datafiles.skip = GETMEM(GRIB2Datafiles.skip,
						FpaIngestFieldStruct , GRIB2Datafiles.nskip);
				return &(GRIB2Datafiles.skip[GRIB2Datafiles.nskip-1]);
				}
			break;
		}
	/* Unknown grib edition */
	return NullPtr(FpaIngestFieldStruct *);
	}

/************************************************************/
/** Add a source to the redirect list
 *
 * @param[in]	edition	GRIB edition \#
 * @return	Pointer to new Field structure
 ************************************************************/
static  FpaIngestRedirectStruct		*init_redirect_field
	(
	 int edition
	)
	{
	switch(edition)
		{
		case 0:
		case 1:
			GRIB1Metafiles.nredirect++;
			GRIB1Metafiles.redirect = GETMEM(GRIB1Metafiles.redirect,
					FpaIngestRedirectStruct , GRIB1Metafiles.nredirect);
			return &(GRIB1Metafiles.redirect[GRIB1Metafiles.nredirect-1]);
		case 2:
			GRIB2Metafiles.nredirect++;
			GRIB2Metafiles.redirect = GETMEM(GRIB2Metafiles.redirect,
					FpaIngestRedirectStruct , GRIB2Metafiles.nredirect);
			return &(GRIB2Metafiles.redirect[GRIB2Metafiles.nredirect-1]);
		}
	}

/************************************************************/
/** Add a source to the redirect list
 *
 * @param[in]	edition	GRIB edition \#
 * @return	Pointer to new Field structure
 ************************************************************/
static  FpaIngestRedirectStruct		*init_redirect_datafile
	(
	 int edition
	)
	{
	switch(edition)
		{
		case 0:
		case 1:
			GRIB1Datafiles.nredirect++;
			GRIB1Datafiles.redirect = GETMEM(GRIB1Datafiles.redirect,
					FpaIngestRedirectStruct , GRIB1Datafiles.nredirect);
			return &(GRIB1Datafiles.redirect[GRIB1Datafiles.nredirect-1]);
		case 2:
			GRIB2Datafiles.nredirect++;
			GRIB2Datafiles.redirect = GETMEM(GRIB2Datafiles.redirect,
					FpaIngestRedirectStruct , GRIB2Datafiles.nredirect);
			return &(GRIB2Datafiles.redirect[GRIB2Datafiles.nredirect-1]);
		}
	}

/************************************************************/
/** Add a source to the rescale list
 *
 * @param[in]	edition	GRIB edition \#
 * @return	Pointer to new Field structure
 ************************************************************/
static  FpaIngestRescaleStruct		*init_rescale_datafile
	(
	 int edition
	)
	{
	switch(edition)
		{
		case 0:
		case 1:
			GRIB1Datafiles.nrescale++;
			GRIB1Datafiles.rescale = GETMEM(GRIB1Datafiles.rescale,
					FpaIngestRescaleStruct , GRIB1Datafiles.nrescale);
			return &(GRIB1Datafiles.rescale[GRIB1Datafiles.nrescale-1]);
		case 2:
			GRIB2Datafiles.nrescale++;
			GRIB2Datafiles.rescale = GETMEM(GRIB2Datafiles.rescale,
					FpaIngestRescaleStruct , GRIB2Datafiles.nrescale);
			return &(GRIB2Datafiles.rescale[GRIB2Datafiles.nrescale-1]);
		}
	}

/************************************************************/
/* COMPARISON FUNCTIONS
 * c o m p a r e _ s o u r c e
 * c o m p a r e _ e l e m e n t
 * c o m p a r e _ l e v e l
 ************************************************************/
/************************************************************/
/** Comparison function for use with qsort.
 *
 * @param[in]	idlist1	pointer to structure of first source
 * @param[in]	idlist2	pointer to structure of second source
 * @return
 * 	- < 0 if idlist1 < idlist2
 * 	- > 0 if idlist1 > idlist2
 * 	- > 0 in case of a tie newer entries should be prefered.
 ************************************************************/
static int	compare_sources
	(
	 const FpaIngestSourceStruct	*s1,
	 const FpaIngestSourceStruct	*s2
	)
	{
	int cmp;
	/* Error returns for missing identifiers */
	if ( IsNull(s1) )			return  1;
	if ( !s1->valid )			return  1;
	if ( IsNull(s2) )			return -1;
	if ( !s2->valid )			return -1;

	cmp = s1->centre 		- s2->centre;
	if (cmp == 0) cmp = s1->sub_centre	- s2->sub_centre;
	if (cmp == 0) cmp = s1->template	- s2->template;
	if (cmp == 0) cmp = s1->type 		- s2->type;
	if (cmp == 0) cmp = s1->process 	- s2->process;
	if (cmp == 0) cmp = s1->bkgd_process- s2->bkgd_process;
	if (cmp == 0) return 1;	/* In case of a tie, new entries should be prefered */
	return cmp;
	}

void simple_sort_sources
	(
	 int edition
	)

	{
	int ii, size;
	FpaIngestSourceStruct *tmp;
	FpaIngestSourceStruct **list;

	switch (edition)
		{
		case 0:
		case 1:
			list = GRIB1Sources;
			size = NumGRIB1Sources;
			break;
		case 2:
			list = GRIB2Sources;
			size = NumGRIB2Sources;
			break;
		}

	for (ii = size-1; ii > 0; ii--)
		{
		if ( compare_sources(list[ii-1], list[ii]) > 0 ) /* Swap */
			{
			tmp        = list[ii-1];
			list[ii-1] = list[ii];
			list[ii]   = tmp;
			}
		else return; /* Done */
		}
	return;
	}
/************************************************************/
/** Comparison function for use with qsort on element lists.
 *
 * @param[in]	idlist1	pointer to structure of first element
 * @param[in]	idlist2	pointer to structure of second element
 * @return
 * 	- < 0 if idlist1 < idlist2
 * 	- > 0 if idlist1 > idlist2
 * 	- > 0 in case of a tie newer entries should be prefered.
 ************************************************************/
static int	compare_elements
	(
	 const FpaIngestElementStruct	*e1,
	 const FpaIngestElementStruct	*e2
	)
	{
	int cmp;

	/* Error returns for missing identifiers */
	if ( IsNull(e1) )			return  1;
	if ( !e1->valid )			return  1;
	if ( IsNull(e2) )			return -1;
	if ( !e2->valid )			return -1;

	cmp = e1->discipline	- e2->discipline;
	if (cmp == 0) cmp = e1->category 	- e2->category;
	if (cmp == 0) cmp = e1->parameter 	- e2->parameter;
	if (cmp == 0) cmp = e1->template	- e2->template;
	if (cmp == 0) return 1;	/* In case of a tie, new entries should be prefered */
	return cmp;
	}

void simple_sort_elements
	(
	 FpaIngestSourceListStruct *sources
	)

	{
	int ii, size;
	FpaIngestElementStruct    *tmp;
	FpaIngestElementStruct    **list;

	list = sources->elements;
	size = sources->nelements;

	for (ii = size-1; ii > 0; ii--)
		{
		if ( compare_elements(list[ii-1], list[ii]) > 0 ) /* Swap */
			{
			tmp        = list[ii-1];
			list[ii-1] = list[ii];
			list[ii]   = tmp;
			}
		else return; /* Done */
		}
	return;
	}
/************************************************************/
/** Comparison function for use with qsort on level lists.
 *
 * @param[in]	idlist1	pointer to structure of first level
 * @param[in]	idlist2	pointer to structure of second level
 * @return
 * 	- < 0 if idlist1 < idlist2
 * 	- > 0 if idlist1 > idlist2
 * 	- > 0 in case of a tie newer entries should be prefered.
 ************************************************************/
static int	compare_levels
	(
	 FpaIngestLevelStruct *l1,
	 FpaIngestLevelStruct *l2
	)
	{
	int cmp;

	/* Error returns for missing identifiers */
	if ( IsNull(l1) )			return  1;
	if ( !l1->valid )			return  1;
	if ( IsNull(l2) )			return -1;
	if ( !l2->valid )			return -1;

	cmp = l1->id	- l2->id;
	if (cmp == 0) return 1;	/* In case of a tie, new entries should be prefered */
	return cmp;
	}

void simple_sort_levels
	(
	FpaIngestSourceListStruct  *sources
	)

	{
	int ii, size;
	FpaIngestLevelStruct      *tmp;
	FpaIngestLevelStruct      **list;

	list = sources->levels;
	size = sources->nlevels;

	for (ii = size-1; ii > 0; ii--)
		{
		if ( compare_levels(list[ii-1], list[ii]) > 0 ) /* Swap */
			{
			tmp        = list[ii-1];
			list[ii-1] = list[ii];
			list[ii]   = tmp;
			}
		else return; /* Done */
		}
	return;
	}

/************************************************************/
/** Compare the Redirect structs
 *
 * @param[in]	idlist1	pointer to structure of first source
 * @param[in]	idlist2	pointer to structure of second source
 * @return
 * 	- < 0 if idlist1 < idlist2
 * 	- > 0 if idlist1 > idlist2
 * 	- > 0 in case of a tie newer entries should be prefered.
 ************************************************************/
static int	compare_redirect
	(
	 FpaIngestRedirectStruct *e1,
	 FpaIngestRedirectStruct *e2
	)
	{
	int cmp;

	if ( IsNull(e1) )				return  1;
	if ( IsNull(e1->source_from) )	return  1;
	if ( IsNull(e2) )				return -1;
	if ( IsNull(e2->source_from) )	return -1;

	/* Otherwise do a string compare */
	cmp =  strcmp( e1->source_from, e2->source_from);
	if (cmp == 0 ) return 1; /* In case of a tie, new entries should be prefered */
	return cmp;
	}

void simple_sort_redirect_field
	(
	 int edition
	)

	{
	int ii, size;
	FpaIngestRedirectStruct tmp;
	FpaIngestRedirectStruct *list;

	switch (edition)
		{
		case 0:
		case 1:
			list = GRIB1Metafiles.redirect;
			size = GRIB1Metafiles.nredirect;
			break;
		case 2:
			list = GRIB2Metafiles.redirect;
			size = GRIB2Metafiles.nredirect;
			break;
		}

	for (ii = size-1; ii > 0; ii--)
		{
		if ( compare_redirect(&list[ii-1], &list[ii]) > 0 ) /* Swap */
			{
			tmp        = list[ii-1];
			list[ii-1] = list[ii];
			list[ii]   = tmp;
			}
		else return; /* Done */
		}
	return;
	}

void simple_sort_redirect_data
	(
	 int edition
	)

	{
	int ii, size;
	FpaIngestRedirectStruct tmp;
	FpaIngestRedirectStruct *list;

	switch (edition)
		{
		case 0:
		case 1:
			list = GRIB1Datafiles.redirect;
			size = GRIB1Datafiles.nredirect;
			break;
		case 2:
			list = GRIB2Datafiles.redirect;
			size = GRIB2Datafiles.nredirect;
			break;
		}

	for (ii = size-1; ii > 0; ii--)
		{
		if ( compare_redirect(&list[ii-1], &list[ii]) > 0 ) /* Swap */
			{
			tmp        = list[ii-1];
			list[ii-1] = list[ii];
			list[ii]   = tmp;
			}
		else return; /* Done */
		}
	return;
	}
/************************************************************/
/** Compare the Rescale structs
 *
 * @param[in]	idlist1	pointer to structure of first field
 * @param[in]	idlist2	pointer to structure of second field
 * @return
 * 	- < 0 if idlist1 < idlist2
 * 	- > 0 if idlist1 > idlist2
 * 	- > 0 in case of a tie newer entries should be prefered.
 ************************************************************/
static int	compare_rescale
	(
	 FpaIngestRescaleStruct	*e1,
	 FpaIngestRescaleStruct	*e2
	)
	{
	int cmp;

	if ( IsNull(e1) )			return  1;
	if ( IsNull(e1->element) )	return  1;
	if ( IsNull(e1->level) )	return  1;
	if ( IsNull(e2) )			return -1;
	if ( IsNull(e2->element) )	return -1;
	if ( IsNull(e2->level) )	return -1;

	/* Otherwise do a string compare */
	if ( !same(e1->element, e2->element) )
		{
		if ( same(e1->element, FpaGwildCard) ) return 1; /* Wildcards are always bigger */
		if ( same(e2->element, FpaGwildCard) ) return -1;
		cmp =  strcmp( e1->element, e2->element);	/* must be nonzero */
		}
	else
		{
		if ( same(e1->level, FpaGwildCard) ) return 1;	/* Wildcards are always bigger */
		if ( same(e2->level, FpaGwildCard) ) return -1;
		cmp =  strcmp( e1->level, e2->level);
		}
	if (cmp == 0 ) return 1; /* In case of a tie, new entries should be prefered */
	return cmp;
	}

void simple_sort_rescale_data
	(
	 int edition
	)

	{
	int ii, size;
	FpaIngestRescaleStruct tmp;
	FpaIngestRescaleStruct *list;

	switch (edition)
		{
		case 0:
		case 1:
			list = GRIB1Datafiles.rescale;
			size = GRIB1Datafiles.nrescale;
			break;
		case 2:
			list = GRIB2Datafiles.rescale;
			size = GRIB2Datafiles.nrescale;
			break;
		}

	for (ii = size-1; ii > 0; ii--)
		{
		if ( compare_rescale(&list[ii-1], &list[ii]) > 0 ) /* Swap */
			{
			tmp        = list[ii-1];
			list[ii-1] = list[ii];
			list[ii]   = tmp;
			}
		else return; /* Done */
		}
	return;
	}
/***********************************************************************
*                                                                      *
*   p u s h _ s e c t i o n                                            *
*   p o p _ s e c t i o n                                              *
*                                                                      *
*   Remember which section of configuration file should be read next.  *
*                                                                      *
***********************************************************************/

/* Storage for section identifiers */
static	int		NumSectionIds = 0;
static	int		MaxSectionIds = 0;
static	int		*SectionIds   = NullInt;

/**********************************************************************/

/************************************************************/
/** Push section id onto queue
 *
 * @param[in]	section	section identifier
 * @return		the section identifier
 ************************************************************/
static	int			push_section

	(
	int			section
	)

	{

	/* Add section identifier to list */
	NumSectionIds++;
	if ( NumSectionIds > MaxSectionIds )
		{
		MaxSectionIds = NumSectionIds;
		SectionIds = GETMEM(SectionIds, int, NumSectionIds);
		}
	SectionIds[NumSectionIds-1] = section;

	/* Return the section identifier */
	return SectionIds[NumSectionIds-1];
	}

/**********************************************************************/

/************************************************************/
/** Pop section ID from queue
 *
 * @return	the last section identifier from the list
 ************************************************************/
static	int			pop_section

	(
	)

	{

	/* Return immediately if no section identifiers in list */
	if ( NumSectionIds < 1 )  return FpaGnoSection;

	/* Return last section identifier from list */
	NumSectionIds--;
	if ( NumSectionIds >= 1 ) return SectionIds[NumSectionIds-1];
	else                      return FpaGnoSection;
	}

/************************************************************/
/** Convert string into unsignded short int.
 *
 * If the short is * then substitute it with the max value
 * for an unsigned short.
 *
 * @param[in]	in	input string to be converted
 * @param[out]	out	output short integer
 * @return TRUE if successful
 ************************************************************/
static LOGICAL strtoushrt
	(
	 STRING			in,
	 unsigned short	*out
	)
	{

	/* If string is empty we have an error */
	if ( IsNull(in) ) return FALSE;

	if ( same(in, "0") )				*out = 0;
	else if ( same(in, FpaGwildCard) ) 	*out = USHRT_MAX;
	else
		{
		*out = (unsigned short)atoi(in);
		if ( *out == 0 )	return FALSE;
		}
	return TRUE;
	}

/************************************************************/
/** Convert string into unsignded char.
 *
 * If the char is * then substitute it with the max value
 * for an unsigned char.
 *
 * @param[in]	in	input string to be converted
 * @param[out]	out	output short integer
 * @return TRUE if successful
 ************************************************************/
static LOGICAL strtouchar
	(
	 STRING			in,
	 unsigned char	*out
	)
	{

	/* If string is empty we have an error */
	if ( IsNull(in) ) return FALSE;

	if ( same(in, "0") )				*out = 0;
	else if ( same(in, FpaGwildCard) ) 	*out = UCHAR_MAX;
	/* If atoi fails it returns 0. But 0 at this point is not
	 * a valid answer so I know if failed */
	else
		{
		*out = (unsigned char)atoi(in);
		if ( *out == 0 )	return FALSE;
		}
	return TRUE;
	}

/************************************************************/
/** Print out lists of sources, elements and levels for
 * debugging.
 ************************************************************/
void print_test()
	{
	int ii, jj;

	/* Print Sources */
	if ( NumGRIB1Sources > 0 )
		{
		fprintf(stdout, "GRIB Edition 1 Sources:\n");

		fprintf(stdout, "valid:\n");
		for (ii=0; ii< NumGRIB1Sources; ii++)
			{
			if (GRIB1Sources[ii]->valid)
				{
				fprintf(stdout, "Source %d: %s %d %d\n", ii,
						GRIB1Sources[ii]->name,
						GRIB1Sources[ii]->centre,
						GRIB1Sources[ii]->process);
				}
			}
		fprintf(stdout, "invalid:\n");
		for (ii=0; ii< NumGRIB1Sources; ii++)
			{
			if (!GRIB1Sources[ii]->valid)
				{
				fprintf(stdout, "Source %d: %s %d %d\n", ii,
						GRIB1Sources[ii]->name,
						GRIB1Sources[ii]->centre,
						GRIB1Sources[ii]->process);
				}
			}
		}

	if ( NumGRIB2Sources > 0 )
		{
		fprintf(stdout, "GRIB Edition 2 Sources:\n");
		fprintf(stdout, "valid:\n");
		for (ii=0; ii< NumGRIB2Sources; ii++)
			{
			if (GRIB2Sources[ii]->valid)
				{
				fprintf(stdout, "Source %d: %s %d %d %d %d %d %d\n", ii,
						GRIB2Sources[ii]->name,
						GRIB2Sources[ii]->centre,
						GRIB2Sources[ii]->sub_centre,
						GRIB2Sources[ii]->template,
						GRIB2Sources[ii]->type,
						GRIB2Sources[ii]->process,
						GRIB2Sources[ii]->bkgd_process);
				}
			}
		fprintf(stdout, "invalid:\n");
		for (ii=0; ii< NumGRIB2Sources; ii++)
			{
			if (!GRIB2Sources[ii]->valid)
				{
				fprintf(stdout, "Source %d: %s %d %d %d %d %d %d %d\n", ii,
						GRIB2Sources[ii]->name,
						GRIB2Sources[ii]->valid,
						GRIB2Sources[ii]->centre,
						GRIB2Sources[ii]->sub_centre,
						GRIB2Sources[ii]->template,
						GRIB2Sources[ii]->type,
						GRIB2Sources[ii]->process,
						GRIB2Sources[ii]->bkgd_process);
				}
			}
		}

	/* Print Elements */
	fprintf(stdout, "GRIB Edition 1 Elements:\n");
	for (ii = 0; ii< NumGRIB1SLists; ii++)
		{
		fprintf(stdout, "valid:\n");
		/* Sort the list of elements in priority order */
		for (jj = 0; jj< GRIB1SLists[ii]->nelements; jj++)
			{
			if ( GRIB1SLists[ii]->elements[jj]->valid)
				fprintf(stdout, "Elements %d: %s %s %s %d %d\n", jj,
						GRIB1SLists[ii]->source,
						GRIB1SLists[ii]->elements[jj]->element,
						GRIB1SLists[ii]->elements[jj]->units,
						GRIB1SLists[ii]->elements[jj]->category,
						GRIB1SLists[ii]->elements[jj]->parameter);
			}
		}
	fprintf(stdout, "GRIB Edition 2 Elements:\n");
	for (ii = 0; ii< NumGRIB2SLists; ii++)
		{
		/* Sort the list of elements in priority order */

		for (jj = 0; jj< GRIB2SLists[ii]->nelements; jj++)
			{
			if ( GRIB2SLists[ii]->elements[jj]->valid)
				fprintf(stdout, "Elements %d: %s %s %s %d %d %d %d\n", jj,
						GRIB2SLists[ii]->source,
						GRIB2SLists[ii]->elements[jj]->element,
						GRIB2SLists[ii]->elements[jj]->units,
						GRIB2SLists[ii]->elements[jj]->discipline,
						GRIB2SLists[ii]->elements[jj]->template,
						GRIB2SLists[ii]->elements[jj]->category,
						GRIB2SLists[ii]->elements[jj]->parameter);

			}
		}

	/* Print Levels */
	fprintf(stdout, "GRIB Edition 1 Levels:\n");
	for (ii = 0; ii< NumGRIB1SLists; ii++)
		{
		fprintf(stdout, "valid:\n");

		for (jj = 0; jj< GRIB1SLists[ii]->nlevels; jj++)
			{
			if ( GRIB1SLists[ii]->levels[jj]->valid)
				fprintf(stdout, "Levels %d: %s %s %d %d %.3f %.3f %.3f %.3f\n", jj,
						GRIB1SLists[ii]->source,
						GRIB1SLists[ii]->levels[jj]->tag,
						GRIB1SLists[ii]->levels[jj]->type,
						GRIB1SLists[ii]->levels[jj]->id,
						GRIB1SLists[ii]->levels[jj]->scale,
						GRIB1SLists[ii]->levels[jj]->offset,
						GRIB1SLists[ii]->levels[jj]->scale2,
						GRIB1SLists[ii]->levels[jj]->offset2);
			}
		}
	fprintf(stdout, "GRIB Edition 2 Levels:\n");
	for (ii = 0; ii< NumGRIB2SLists; ii++)
		{
		for (jj = 0; jj< GRIB2SLists[ii]->nlevels; jj++)
			{
			if ( GRIB2SLists[ii]->levels[jj]->valid)
				fprintf(stdout, "Levels %d: %s %s %d %d %.3f %.3f\n", jj,
						GRIB2SLists[ii]->source,
						GRIB2SLists[ii]->levels[jj]->tag,
						GRIB2SLists[ii]->levels[jj]->type,
						GRIB2SLists[ii]->levels[jj]->id,
						GRIB2SLists[ii]->levels[jj]->scale,
						GRIB2SLists[ii]->levels[jj]->offset);

			}
		}
	fprintf(stdout, "GRIB Edition 1 Metafiles:\n");
	for (ii = 0; ii < GRIB1Metafiles.nprocess; ii ++)
		fprintf(stdout, "Process %d: %s %s %s\n", ii, 
				GRIB1Metafiles.process[ii].source,
				GRIB1Metafiles.process[ii].element,
				GRIB1Metafiles.process[ii].level);
	for (ii = 0; ii < GRIB1Metafiles.nskip; ii ++)
		fprintf(stdout, "Skip %d: %s %s %s\n", ii, 
				GRIB1Metafiles.skip[ii].source,
				GRIB1Metafiles.skip[ii].element,
				GRIB1Metafiles.skip[ii].level);
	for (ii = 0; ii < GRIB1Metafiles.nredirect; ii ++)
		fprintf(stdout, "Redirect %d: %s %s \n", ii, 
				GRIB1Metafiles.redirect[ii].source_from,
				GRIB1Metafiles.redirect[ii].source_to);
	fprintf(stdout, "GRIB Edition 1 Datafiles:\n");
	for (ii = 0; ii < GRIB1Datafiles.nprocess; ii ++)
		fprintf(stdout, "Process %d: %s %s %s\n", ii, 
				GRIB1Datafiles.process[ii].source,
				GRIB1Datafiles.process[ii].element,
				GRIB1Datafiles.process[ii].level);
	for (ii = 0; ii < GRIB1Datafiles.nskip; ii ++)
		fprintf(stdout, "Skip %d: %s %s %s\n", ii, 
				GRIB1Datafiles.skip[ii].source,
				GRIB1Datafiles.skip[ii].element,
				GRIB1Datafiles.skip[ii].level);
	for (ii = 0; ii < GRIB1Datafiles.nredirect; ii ++)
		fprintf(stdout, "Redirect %d: %s %s \n", ii, 
				GRIB1Datafiles.redirect[ii].source_from,
				GRIB1Datafiles.redirect[ii].source_to);
	for (ii = 0; ii < GRIB1Datafiles.nrescale; ii ++)
		fprintf(stdout, "Rescale %d: %s %s %f %f\n", ii, 
				GRIB1Datafiles.rescale[ii].element,
				GRIB1Datafiles.rescale[ii].level,
				GRIB1Datafiles.rescale[ii].scale,
				GRIB1Datafiles.rescale[ii].offset);
	fprintf(stdout, "GRIB Edition 2 Metafiles:\n");
	for (ii = 0; ii < GRIB2Metafiles.nprocess; ii ++)
		fprintf(stdout, "Process %d: %s %s %s\n", ii, 
				GRIB2Metafiles.process[ii].source,
				GRIB2Metafiles.process[ii].element,
				GRIB2Metafiles.process[ii].level);
	for (ii = 0; ii < GRIB2Metafiles.nskip; ii ++)
		fprintf(stdout, "Skip %d: %s %s %s\n", ii, 
				GRIB2Metafiles.skip[ii].source,
				GRIB2Metafiles.skip[ii].element,
				GRIB2Metafiles.skip[ii].level);
	for (ii = 0; ii < GRIB2Metafiles.nredirect; ii ++)
		fprintf(stdout, "Redirect %d: %s %s \n", ii, 
				GRIB2Metafiles.redirect[ii].source_from,
				GRIB2Metafiles.redirect[ii].source_to);
	fprintf(stdout, "GRIB Edition 2 Datafiles:\n");
	for (ii = 0; ii < GRIB2Datafiles.nprocess; ii ++)
		fprintf(stdout, "Process %d: %s %s %s\n", ii, 
				GRIB2Datafiles.process[ii].source,
				GRIB2Datafiles.process[ii].element,
				GRIB2Datafiles.process[ii].level);
	for (ii = 0; ii < GRIB2Datafiles.nskip; ii ++)
		fprintf(stdout, "Skip %d: %s %s %s\n", ii, 
				GRIB2Datafiles.skip[ii].source,
				GRIB2Datafiles.skip[ii].element,
				GRIB2Datafiles.skip[ii].level);
	for (ii = 0; ii < GRIB2Datafiles.nredirect; ii ++)
		fprintf(stdout, "Redirect %d: %s %s \n", ii, 
				GRIB2Datafiles.redirect[ii].source_from,
				GRIB2Datafiles.redirect[ii].source_to);
	for (ii = 0; ii < GRIB2Datafiles.nrescale; ii ++)
		fprintf(stdout, "Rescale %d: %s %s %f %f\n", ii, 
				GRIB2Datafiles.rescale[ii].element,
				GRIB2Datafiles.rescale[ii].level,
				GRIB2Datafiles.rescale[ii].scale,
				GRIB2Datafiles.rescale[ii].offset);
	}
