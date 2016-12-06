/**********************************************************************/
/** @file ingest_info.h
 *
 * Structures for reading and storing ingest configurations.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   i n g e s t _ i n f o . h                                          *
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
#ifndef INGEST_INFO_DEFS
#define INGEST_INFO_DEFS

/* We need FPA definitions */
#include "read_config.h"
#include <tools/tools.h>
#include <fpa_types.h>

#ifdef INGEST_INFO_INIT

/* Identifier tags to access configuration files in setup file */
#define FpaGIngestsFile   "ingest"
#define FpaGdefault       "default"
#define FpaGopenBrace     "{"
#define FpaGcloseBrace    "}"
#define FpaGplaceHolder   "-"
#define FpaGwildCard      "*"
#define FpaGequalSign     "="
#define FpaGnoSection     -1

#define FpaGblockGrib	"Grib"
/* Grib block options */
#define FpaGblockSource "Sources"
enum	{ FpaGblockSourceName, FpaGblockSourceInfo };
#define FpaGblockElem   "Elements"
enum	{ FpaGblockESourceList, FpaGblockElementName, FpaGblockElementInfo };
#define FpaGblockLevel  "Levels"
enum	{ FpaGblockLSourceList, FpaGblockLevelName, FpaGblockLevelInfo };
#define FpaGblockField  "Fields"
#define FpaGblockData   "DataFiles"



typedef struct
	{
	unsigned short	centre;			/**< Identification of originating/generating centre */
	unsigned short	sub_centre;		/**< Identification of originating/generating 
									     sub-centre*/
	unsigned short	template;		/**< Product Definition Template */
	unsigned char	type;			/**< Type of generation process */
	unsigned char	process;		/**< Analysis or forecast 
									     generating process identifier */
	unsigned char	bkgd_process;	/**< Background generating process identifier*/
	STRING			name;			/**< Source name */
	LOGICAL			valid;			/**< Is this source valid? */
	}FpaIngestSourceStruct;

typedef struct
	{
	unsigned char	discipline;		/**< Discipline of processed data in GRIB message */
	unsigned char	category;		/**< category number */
	unsigned char	parameter;		/**< parameter number */
	unsigned short	template;		/**< Product Definition Template */
	STRING			element;		/**< Element name */
	STRING			units;			/**< Unit name */
	LOGICAL			valid;			/**< Is this element valid? */
	}FpaIngestElementStruct;

typedef struct
	{
	unsigned char	type;			/**< type of fixed surface */
	unsigned char	id;				/**< GRIB level id number */
	float			scale;			/**< scale factor */
	float			offset;			/**< offset */
	float			scale2;			/**< scale factor for second level */
	float			offset2;		/**< offset for second level */
	STRING			tag;			/**< Level name or postfix */
	LOGICAL			valid;			/**< Is this level valid? */
	}FpaIngestLevelStruct;

/** List elements by source name */
typedef struct
	{
	STRING					source;		/**< Source associated with this list */
	int						nelements;	/**< number of elements in list */
	FpaIngestElementStruct	**elements;	/**< list of elements for source */
	int						nlevels;	/**< number of levels in list */
	FpaIngestLevelStruct	**levels;	/**< list of levels for source */
	}FpaIngestSourceListStruct;

/** structure of fields to skip or process */
typedef struct
	{
	STRING	source;			/**< Source */
	STRING	element;		/**< Element of field */
	STRING	level;			/**< Level of field */
	}FpaIngestFieldStruct;

typedef struct
	{
	STRING	source_from;	/**< Source to redirect from */
	STRING	source_to;		/**< Source to redirect to   */
	}FpaIngestRedirectStruct;

typedef struct
	{
	STRING	element;		/**< Element of field to rescale */
	STRING	level;			/**< Level of field to rescale   */
	float	scale;			/**< Amount to scale data by     */
	float	offset;			/**< Amount to offset data by    */
	}FpaIngestRescaleStruct;

typedef struct
	{
	FpaIngestFieldStruct	*process;	/**< List of fields to process */
	int						nprocess;
	FpaIngestFieldStruct	*skip;		/**< List of fields to skip */
	int						nskip;
	FpaIngestRedirectStruct *redirect;	/**< List of sources to redirect */
	int						nredirect;
	FpaIngestRescaleStruct  *rescale;	/**< List of fields to rescale */
	int						nrescale;
	}FpaIngestProcessStruct;


#endif

/***********************************************************************
*                                                                      *
*  Declare external functions in ingest_info.c                         *
*                                                                      *
***********************************************************************/

/* Interface functions ... reading configuration file */
LOGICAL	read_complete_ingest_file(void);

/* Replace Grib parameters with configured tags */
LOGICAL	ingest_grib_models( int, int *, STRING);
LOGICAL	ingest_grib_elements( int, STRING, int *, STRING, STRING);
LOGICAL	ingest_grib_levels( int, STRING, int, int *, float *, float *, float *, float *, STRING);
/* Interface functions ... fields to skip */
LOGICAL	skip_grib_field(int, STRING, STRING, STRING);
LOGICAL	skip_grib_datafile(int, STRING, STRING, STRING);
STRING	redirect_field(int, STRING);
STRING	redirect_datafile(int, STRING);
LOGICAL rescale_datafile(int, STRING, STRING, float *, float *);

/* Now it has been included */
#endif
