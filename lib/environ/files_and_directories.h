/**********************************************************************/
/** @file files_and_directories.h
 *
 *  Routines for constructing file names and directory paths
 *  (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   f i l e s _ a n d _ d i r e c t o r i e s . h                      *
*                                                                      *
*   Routines for constructing file names and directory paths           *
*    (include file)                                                    *
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
#ifndef FILES_AND_DIRS_DEFS
#define FILES_AND_DIRS_DEFS


/* We need definitions for low level types and other Objects */
#include <fpa_types.h>
#include <objects/objects.h>


/* We need definitions for configuration file structures */
#include "config_structs.h"


/***********************************************************************
*                                                                      *
*  Initialize default filenames for editor functions                   *
*                                                                      *
***********************************************************************/

/* Define default names for last drawn and last moved edit boundaries */
#define FpaEditorLastDrawnBoundary	"FPA-EditorLastDrawnBoundary"
#define FpaEditorLastMovedBoundary	"FPA-EditorLastMovedBoundary"

/* Define default name for last stomp edit boundary */
#define FpaEditorLastStompBoundary	"FPA-EditorLastStompBoundary"

/* Define default name for last drawn hole */
#define FpaEditorLastDrawnHole		"FPA-EditorLastDrawnHole"

/***********************************************************************
*                                                                      *
*  Initialize defined constants for files_and_directories routines     *
*                                                                      *
***********************************************************************/

/* Define Regular-Expression masks for the dirlist() function  */
/*  to match the usual (new format) metafile names             */
/* Most of these are used as a format string to produce a mask */
#	ifdef FILES_AND_DIRS_INIT
	const	STRING	MetaSearchAll        = "^.*~.*~[0-9]*-[0-9]*-[0-9]*.*$";
	const	STRING	MetaSearchField      = "^%s~[0-9]*-[0-9]*-[0-9]*.*$";
	const	STRING	MetaSearchElem       = "^%s~.*~[0-9]*-[0-9]*-[0-9]*.*$";
	const	STRING	MetaSearchLevel      = "^.*~%s~[0-9]*-[0-9]*-[0-9]*.*$";
	const	STRING	MetaSearchValid      = "^.*~.*~%s.*$";
	const	STRING	MetaSearchFieldValid = "^%s.*$";
	const	STRING	MetaSearchElemValid  = "^%s~.*~%s.*$";
	const	STRING	MetaSearchLevelValid = "^.*~%s~%s.*$";
#	else
	extern	const	STRING	MetaSearchAll;
	extern	const	STRING	MetaSearchField;
	extern	const	STRING	MetaSearchElem;
	extern	const	STRING	MetaSearchLevel;
	extern	const	STRING	MetaSearchValid;
	extern	const	STRING	MetaSearchFieldValid;
	extern	const	STRING	MetaSearchElemValid;
	extern	const	STRING	MetaSearchLevelValid;
#	endif

/* Define Regular-Expression masks for the dirlist() function  */
/*  to match the usual (old format) metafile names             */
/* Most of these are used as a format string to produce a mask */
#	ifdef FILES_AND_DIRS_INIT
	const	STRING	SearchMetaAll        = "^.*_[0-9]*:[0-9]*:[0-9]*.*$";
	const	STRING	SearchMetaField      = "^%s_[0-9]*:[0-9]*:[0-9]*.*$";
	const	STRING	SearchMetaElem       = "^%s.*_[0-9]*:[0-9]*:[0-9]*.*$";
	const	STRING	SearchMetaLevel      = "^.*%s_[0-9]*:[0-9]*:[0-9]*.*$";
	const	STRING	SearchMetaValid      = "^.*_%s.*$";
	const	STRING	SearchMetaFieldValid = "^%s.*$";
	const	STRING	SearchMetaElemValid  = "^%s.*_%s.*$";
	const	STRING	SearchMetaLevelValid = "^.*%s_%s.*$";
#	else
	extern	const	STRING	SearchMetaAll;
	extern	const	STRING	SearchMetaField;
	extern	const	STRING	SearchMetaElem;
	extern	const	STRING	SearchMetaLevel;
	extern	const	STRING	SearchMetaValid;
	extern	const	STRING	SearchMetaFieldValid;
	extern	const	STRING	SearchMetaElemValid;
	extern	const	STRING	SearchMetaLevelValid;
#	endif


/* Set Default string sizes for long buffers or short names */
/** Often used to define a buffer size for large buffers */
#define MAX_BCHRS	1024
#define MAX_FCHRS	128
/** Often used to define a buffer size for short names */
#define MAX_NCHRS	32


/** Define "FLD_DESCRIPT" structure:  Descriptor for specific fields */
typedef struct FLD_DESCRIPT_struct
{
	MAP_PROJ	mproj;						/**< Map projection for field */
	char		path[MAX_BCHRS];			/**< Path name for field */
	FpaConfigSourceStruct
					*sdef;					/**< Source info for field */
	FpaConfigSourceSubStruct
					*subdef;				/**< Subsource info for field */
	char		rtime[MAX_NCHRS];			/**< Run time stamp for field */
	char		vtime[MAX_NCHRS];			/**< Valid time stamp for field */
	FpaConfigElementStruct
					*edef;					/**< Element info for field */
	FpaConfigLevelStruct
					*ldef;					/**< Level info for field */
	FpaConfigFieldStruct
					*fdef;					/**< Field info for field */
	int			fmacro;						/**< Enumerated field type from 
											  FpaCfieldTypeOption list  */
	char		wind_func_name[MAX_FCHRS];	/**< Wind function name for field */
	char		value_func_name[MAX_FCHRS];	/**< Value function name for field */
} FLD_DESCRIPT;


/** Define Default FLD_DESCRIPT values */
#define	FpaNO_FDESC		{ NO_MAPPROJ, FpaCblank, \
							NullPtr(FpaConfigSourceStruct *), \
							NullPtr(FpaConfigSourceSubStruct *), \
							FpaCblank, FpaCblank, \
							NullPtr(FpaConfigElementStruct *), \
							NullPtr(FpaConfigLevelStruct *), \
							NullPtr(FpaConfigFieldStruct *), \
							FpaCnoMacro, FpaCblank, FpaCblank }

#ifdef FILES_AND_DIRS_INIT
	const	FLD_DESCRIPT	*FpaNullFDesc    = (FLD_DESCRIPT *)(0);
	const	FLD_DESCRIPT	FpaNoFDesc       = FpaNO_FDESC;
#else
	extern	const	FLD_DESCRIPT	*FpaNullFDesc;
	extern	const	FLD_DESCRIPT	FpaNoFDesc;
#endif


/** Set recognized types for set_fld_descript() routine */
typedef enum
		{ FpaF_END_OF_LIST,
			FpaF_MAP_PROJECTION,         FpaF_DIRECTORY_PATH,
			FpaF_SOURCE,                 FpaF_SOURCE_NAME,
			FpaF_SUBSOURCE,              FpaF_SUBSOURCE_NAME,
			FpaF_RUN_TIME,               FpaF_VALID_TIME,
			FpaF_ELEMENT,                FpaF_ELEMENT_NAME,
			FpaF_LEVEL,                  FpaF_LEVEL_NAME,
			FpaF_FIELD_DATA_TYPE,        FpaF_FIELD_MACRO,
			FpaF_WIND_FUNCTION_NAME,     FpaF_VALUE_FUNCTION_NAME,
			/* >>> the following are obsolete in next version <<< */
			FpaF_WIND_CALCULATION,			/**< Obsolete */
			FpaF_WIND_CALCULATION_NAME,		/**< Obsolete */
			FpaF_VALUE_CALCULATION,			/**< Obsolete */
			FpaF_VALUE_CALCULATION_NAME,	/**< Obsolete */
			FpaF_CALCULATION_TYPE_NAME		/**< Obsolete */
			/* >>> the preceding are obsolete in next version <<< */
		} FpaFfldDescriptOption;


/***********************************************************************
*                                                                      *
*  Declare external functions in files_and_directories.c               *
*                                                                      *
***********************************************************************/

void	init_fld_descript(FLD_DESCRIPT *fdesc);
LOGICAL	set_fld_descript(FLD_DESCRIPT *fdesc, ...);
void	copy_fld_descript(FLD_DESCRIPT *fdescnew, const FLD_DESCRIPT *fdesc);
LOGICAL	same_fld_descript(FLD_DESCRIPT *fdesc1, FLD_DESCRIPT *fdesc2);
LOGICAL	same_fld_descript_no_map(FLD_DESCRIPT *fdesc1, FLD_DESCRIPT *fdesc2);
LOGICAL	set_shuffle_lock(STRING dir);
LOGICAL	release_shuffle_lock(STRING dir);
LOGICAL	set_file_lock(STRING dir, STRING vtime);
LOGICAL	release_file_lock(STRING dir, STRING vtime);
LOGICAL	file_locks_released(STRING dir, int tdelta, int tries);
LOGICAL	release_all_file_locks(STRING dir);
STRING	data_directory_path(STRING dirtag, STRING dirpath, STRING subpath);
STRING	find_data_directory(STRING dirtag, STRING dirpath, STRING subpath,
						STRING rtime);
STRING	source_directory(FLD_DESCRIPT *fdesc);
STRING	source_directory_by_name(STRING source, STRING subsrc, STRING rtime);
STRING	prepare_source_directory(FLD_DESCRIPT *fdesc);
STRING	prepare_source_directory_by_name(STRING source, STRING subsrc,
						STRING rtime);
int		source_run_time_list(FLD_DESCRIPT *fdesc, STRING **rlist);
int		source_run_time_list_free(STRING **rlist, int num);
int		source_valid_time_list(FLD_DESCRIPT *fdesc, int macro, STRING **vlist);
int		source_valid_time_list_free(STRING **vlist, int num);
int		source_valid_time_sublist(FLD_DESCRIPT *fdesc, int macro, int nsub,
						STRING vbgn, STRING vcen, STRING vend, STRING **vlist);
int		source_valid_time_sublist_free(STRING **vlist, int num);
int		matched_source_valid_time(FLD_DESCRIPT *fdesc, int macro,
						STRING mtchtime, STRING *vtime);
int		closest_source_valid_time(FLD_DESCRIPT *fdesc, int macro,
						STRING mtchtime, STRING *vtime);
LOGICAL	matched_source_valid_time_reset(FLD_DESCRIPT *fdesc, int macro,
						STRING mtchtime);
LOGICAL	closest_source_valid_time_reset(FLD_DESCRIPT *fdesc, int macro,
						STRING mtchtime);
int		daily_field_local_times(FLD_DESCRIPT *fdesc, FLD_DESCRIPT *fdescin,
						int macro, float clon, STRING **vlist);
int		daily_field_local_times_free(STRING **vlist, int num);
int		source_valid_range_for_daily(FLD_DESCRIPT *fdesc, FLD_DESCRIPT *fdescin,
						int macro, float clon, STRING **vlist);
int		source_valid_range_for_daily_free(STRING **vlist, int num);
int		source_field_list(FLD_DESCRIPT *fdesc, int macro,
						FpaConfigFieldStruct ***fdefs);
int		source_field_list_free(FpaConfigFieldStruct ***fdefs, int num);
LOGICAL	check_source_minutes_in_filenames(STRING source);
STRING	source_path_by_name(STRING source, STRING subsrc, STRING rtime,
						STRING ident);
STRING	construct_file_identifier(STRING elem, STRING level, STRING vtime);
STRING	build_file_identifier(STRING elem, STRING level, STRING vtime);
LOGICAL	parse_file_identifier(STRING ident, FpaConfigElementStruct **edef,
						FpaConfigLevelStruct **ldef, STRING *vtime);
STRING	construct_link_identifier(STRING elem, STRING level);
LOGICAL	parse_link_identifier(STRING ident, FpaConfigElementStruct **edef,
						FpaConfigLevelStruct **ldef);
STRING	construct_meta_filename(FLD_DESCRIPT *fdesc);
STRING	build_meta_filename(FLD_DESCRIPT *fdesc);
STRING	check_meta_filename(FLD_DESCRIPT *fdesc);
STRING	find_meta_filename(FLD_DESCRIPT *fdesc);
STRING	build_allied_filename(FLD_DESCRIPT *fdesc, int atype, STRING alias);
STRING	check_allied_filename(FLD_DESCRIPT *fdesc, int atype, STRING alias);
STRING	construct_link_filename(FLD_DESCRIPT *fdesc);
STRING	check_link_filename(FLD_DESCRIPT *fdesc);
STRING	background_file(STRING name);
STRING	depiction_scratch_file(void);
STRING	depiction_link_file(void);
STRING	named_depiction_file(STRING fname);


/* Now it has been included */
#endif
