/**********************************************************************/
/**	@file config_info.c
 *
 * Routines for reading and accessing new Version 4.0 configuration
 * file information
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c o n f i g _ i n f o . c                                          *
*                                                                      *
*   Routines for reading and accessing new Version 4.0 configuration   *
*   file information                                                   *
*                                                                      *
*   NOTE: The internal function OKARG() checks for blank strings or    *
*         obsolete placeholders ("-") in the configuration files.      *
*         The internal function ISARG() checks for missing strings or  *
*         obsolete placeholders ("-") in the configuration files.      *
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

#define CONFIG_INFO_INIT	/* To initialize declarations in config_info.h */
#define CONFIG_STRUCTS_INIT	/* To initialize declarations in config_structs.h */

#include "read_setup.h"
#include "config_structs.h"
#include "read_config.h"
#include "config_info.h"
#include "rules.h"
#include "calculation.h"

#include <objects/objects.h>
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>
#include <fpa_math.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Interface functions                     */
/*  ... these are defined in config_info.h */

/* Internal static functions (All blocks) */
static LOGICAL	read_blocks_info(void);

/* Internal static functions (Units block) */
static LOGICAL	read_units_info(void);
static FpaConfigUnitStruct
					*find_unit(STRING);
static FpaConfigUnitStruct
					*init_unit(STRING);
static void		add_unit_identifier(STRING, FpaConfigUnitStruct *);

/* Internal static functions (Constants block) */
static LOGICAL	read_constants_info(void);
static FpaConfigConstantStruct
					*find_constant(STRING);
static FpaConfigConstantStruct
					*init_constant(STRING);
static void		add_constant_identifier(STRING, FpaConfigConstantStruct *);

/* Internal static functions (Sources block) */
static LOGICAL	read_sources_info(void);
static LOGICAL	read_source_subsource_info(FILE **, FpaConfigSourceStruct *);
static FpaConfigSourceStruct
					*read_source_allied_info(STRING, STRING);
static LOGICAL	read_allied_programs_info(FILE **, FpaConfigSourceStruct *);
static LOGICAL	read_allied_files_info(FILE **, FpaConfigSourceStruct *);
static LOGICAL	read_allied_fields_info(FILE **, FpaConfigSourceStruct *);
static LOGICAL	read_allied_winds_info(FILE **, FpaConfigSourceStruct *);
static LOGICAL	read_allied_values_info(FILE **, FpaConfigSourceStruct *);
static LOGICAL	read_allied_metafiles_info(FILE **, FpaConfigSourceStruct *);
static FpaConfigSourceStruct
					*find_source(STRING);
static FpaConfigSourceStruct
					*init_source(STRING);
static FpaConfigSourceSubStruct
					*init_source_subsource(STRING);
static FpaConfigSourceAlliedStruct
					*init_source_allied(void);
static FpaConfigAlliedProgramsStruct
					*init_allied_programs(void);
static FpaConfigAlliedFilesStruct
					*init_allied_files(void);
static FpaConfigAlliedFieldsStruct
					*init_allied_fields(void);
static FpaConfigAlliedWindsStruct
					*init_allied_winds(void);
static FpaConfigAlliedValuesStruct
					*init_allied_values(void);
static FpaConfigAlliedMetafilesStruct
					*init_allied_metafiles(void);
static LOGICAL	set_source_location(FILE *, FpaConfigSourceStruct *);
static void		add_source_aliases(STRING, FpaConfigSourceStruct *);
static void		add_source_identifier(STRING, FpaConfigSourceStruct *);

/* Internal static functions (Groups block) */
static LOGICAL	read_groups_info(void);
static FpaConfigGroupStruct
					*find_field_group(STRING);
static FpaConfigGroupStruct
					*init_field_group(STRING);
static void		add_field_group_identifier(STRING, FpaConfigGroupStruct *);
static FpaConfigGroupStruct
					*find_element_group(STRING);
static FpaConfigGroupStruct
					*init_element_group(STRING);
static void		add_element_group_identifier(STRING, FpaConfigGroupStruct *);

/* Internal static functions (Levels block) */
static LOGICAL	read_levels_info(void);
static FpaConfigLevelStruct
					*find_level(STRING);
static FpaConfigLevelStruct
					*init_level(STRING);
static void		add_level_aliases(STRING, FpaConfigLevelStruct *);
static void		add_level_file_ident(STRING, FpaConfigLevelStruct *);
static void		add_level_fileid(STRING, FpaConfigLevelStruct *);
static void		add_level_identifier(STRING, FpaConfigLevelStruct *);
static LOGICAL	set_level_levels(STRING, FpaConfigLevelStruct *);

/* Internal static functions (Elements block) */
static LOGICAL	read_elements_info(void);
static FpaConfigElementStruct
					*read_element_detailed_info(STRING);
static FpaConfigElementStruct
					*find_element(STRING);
static FpaConfigElementStruct
					*init_element(STRING);
static LOGICAL	set_element_location(FILE *, FpaConfigElementStruct *);
static FpaConfigElementDetailStruct
					*init_element_detail(void);
static FpaConfigElementLineTypeStruct
					*init_element_line_types(void);
static FpaConfigElementScatteredTypeStruct
					*init_element_scattered_types(void);
static FpaConfigElementAttribStruct
					*init_element_attributes(int);
static FpaConfigElementEditorStruct
					*init_element_editor(int);
static FpaConfigElementLabellingStruct
					*init_element_labelling(int);
static void		init_element_labelling_types(int,
											FpaConfigElementLabellingStruct *);
static FpaConfigElementSamplingStruct
					*init_element_sampling(int);
static FpaConfigElementLinkingStruct
					*init_element_linking(int);
static FpaConfigElementEquationStruct
					*init_element_equation(void);
static FpaConfigElementValCalcStruct
					*init_element_valcalc(void);
static FpaConfigElementComponentStruct
					*init_element_components(void);

static void		add_element_aliases(STRING, FpaConfigElementStruct *);
static void		add_element_file_ident(STRING, FpaConfigElementStruct *);
static void		add_element_fileid(STRING, FpaConfigElementStruct *);
static void		add_element_identifier(STRING, FpaConfigElementStruct *);
static int		add_element_line_type(STRING, FpaConfigElementLineTypeStruct *);
static int		add_element_scattered_type(STRING,
											FpaConfigElementScatteredTypeStruct *);
static LOGICAL	add_element_type_attrib(STRING, STRING,
											FpaConfigDefaultAttribStruct *);
static LOGICAL	add_element_type_rules(STRING, FpaConfigEntryRuleStruct *);
static LOGICAL	add_element_type_py_rules(STRING, FpaConfigEntryRuleStruct *);
static int		add_element_attribute(STRING, STRING,
											FpaConfigElementAttribStruct *);
static LOGICAL	set_element_attribute_backdef(STRING, STRING, STRING *);
static LOGICAL	set_element_editor_entry_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	set_element_editor_node_entry_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	set_element_editor_modify_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	set_element_editor_node_modify_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	set_element_editor_memory_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	set_element_editor_back_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	set_element_editor_back_mem_file(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	add_element_editor_rules(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	add_element_editor_py_rules(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	add_element_editor_node_rules(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	add_element_editor_py_node_rules(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	add_element_editor_merge_fields(STRING, int,
											FpaConfigElementEditorStruct *);
static LOGICAL	add_element_editor_link_fields(STRING, int,
											FpaConfigElementEditorStruct *);
static int		add_element_labelling_type(STRING,
											FpaConfigElementLabellingStruct *);
static LOGICAL	add_continuous_sample_values(STRING,
											FpaConfigContinuousSamplingStruct *);
static LOGICAL	add_continuous_sample_winds(STRING,
											FpaConfigContinuousSamplingStruct *);
static LOGICAL	add_vector_sample_values(STRING,
											FpaConfigVectorSamplingStruct *);
static LOGICAL	add_vector_sample_winds(STRING,
											FpaConfigVectorSamplingStruct *);
static LOGICAL	add_discrete_sample_attribs(STRING,
											FpaConfigDiscreteSamplingStruct *);
static LOGICAL	add_wind_sample_values(STRING, FpaConfigWindSamplingStruct *);
static LOGICAL	add_wind_sample_crossrefs(STRING, FpaConfigWindSamplingStruct *);
static LOGICAL	add_line_sample_attribs(STRING, FpaConfigLineSamplingStruct *);
static LOGICAL	add_scattered_sample_attribs(STRING,
											FpaConfigScatteredSamplingStruct *);
static LOGICAL	add_lchain_sample_attribs(STRING,
											FpaConfigLchainSamplingStruct *);
static LOGICAL	add_element_link_fields(STRING, int,
											FpaConfigElementLinkingStruct *);

static LOGICAL	set_vcalc_src_types(STRING, FpaConfigElementValCalcStruct *);
static LOGICAL	set_components_component(STRING, COMPONENT,
											FpaConfigElementComponentStruct *);
static FpaConfigElementStruct
					*copy_element(FpaConfigElementStruct *);
static FpaConfigElementDetailStruct
					*copy_element_detail(int, FpaConfigElementDetailStruct *);
static FpaConfigElementLineTypeStruct
					*copy_element_line_types(FpaConfigElementLineTypeStruct *);
static FpaConfigElementScatteredTypeStruct
					*copy_element_scattered_types(FpaConfigElementScatteredTypeStruct *);
static FpaConfigElementAttribStruct
					*copy_element_attributes(FpaConfigElementAttribStruct *);
static FpaConfigElementEditorStruct
					*copy_element_editor(int, FpaConfigElementEditorStruct *);
static FpaConfigElementLabellingStruct
					*copy_element_labelling(FpaConfigElementLabellingStruct *);
static FpaConfigElementSamplingStruct
					*copy_element_sampling(int,
											FpaConfigElementSamplingStruct *);
static FpaConfigElementLinkingStruct
					*copy_element_linking(int, FpaConfigElementLinkingStruct *);
static FpaConfigElementEquationStruct
					*copy_element_equation(FpaConfigElementEquationStruct *);
static FpaConfigElementValCalcStruct
					*copy_element_valcalc(FpaConfigElementValCalcStruct *);
static FpaConfigElementComponentStruct
					*copy_element_components(FpaConfigElementComponentStruct *);

static FpaConfigElementLineTypeStruct
					*free_element_line_types(FpaConfigElementLineTypeStruct *);
static FpaConfigElementScatteredTypeStruct
					*free_element_scattered_types(FpaConfigElementScatteredTypeStruct *);
static void		free_element_type_attribs(FpaConfigDefaultAttribStruct *);
static void		free_element_type_rules(FpaConfigEntryRuleStruct *);
static void		free_element_type_py_rules(FpaConfigEntryRuleStruct *);
static FpaConfigElementAttribStruct
					*free_element_attributes(FpaConfigElementAttribStruct *);
static FpaConfigElementEditorStruct
					*free_element_editor(int, FpaConfigElementEditorStruct *);
static void		free_element_editor_rules(int, FpaConfigElementEditorStruct *);
static void		free_element_editor_py_rules(int, FpaConfigElementEditorStruct *);
static void		free_element_editor_node_rules(int,
											FpaConfigElementEditorStruct *);
static void		free_element_editor_py_node_rules(int,
											FpaConfigElementEditorStruct *);
static void		free_element_editor_merge_fields(int,
											FpaConfigElementEditorStruct *);
static void		free_element_editor_link_fields(int,
											FpaConfigElementEditorStruct *);
static FpaConfigElementLabellingStruct
					*free_element_labelling(FpaConfigElementLabellingStruct *);
static void		free_element_labelling_types(FpaConfigElementLabellingStruct *);
static FpaConfigElementSamplingStruct
					*free_element_sampling(int,
											FpaConfigElementSamplingStruct *);
static FpaConfigElementLinkingStruct
					*free_element_linking(int, FpaConfigElementLinkingStruct *);
static void		free_element_link_fields(int, FpaConfigElementLinkingStruct *);
static FpaConfigElementEquationStruct
					*free_element_equation(FpaConfigElementEquationStruct *);
static FpaConfigElementValCalcStruct
					*free_element_valcalc(FpaConfigElementValCalcStruct *);

/* Internal static functions (Fields block) */
static LOGICAL	read_fields_info(void);
static FpaConfigFieldStruct
					*read_field_detailed_info(STRING, STRING);
static FpaConfigFieldStruct
					*find_field(FpaConfigElementStruct *,
													FpaConfigLevelStruct *);
static FpaConfigFieldStruct
					*init_field(FpaConfigElementStruct *,
													FpaConfigLevelStruct *);
static LOGICAL	set_field_location(FILE *, FpaConfigFieldStruct *);
static void		set_field_group_and_labels(FpaConfigFieldStruct *);

/* Internal static functions (CrossRefs block) */
static LOGICAL	read_crossrefs_info(void);
static LOGICAL	read_crossref_field_info(FILE **, STRING,
													FpaConfigCrossRefStruct *);
static FpaConfigCrossRefStruct
					*find_wind_crossref(STRING);
static FpaConfigCrossRefStruct
					*init_wind_crossref(STRING);
static void		add_wind_crossref_identifier(STRING, FpaConfigCrossRefStruct *);
static FpaConfigCrossRefStruct
					*find_value_crossref(STRING);
static FpaConfigCrossRefStruct
					*init_value_crossref(STRING);
static void		add_value_crossref_identifier(STRING, FpaConfigCrossRefStruct *);

/* Internal static functions (Samples block) */
static LOGICAL	read_samples_info(void);
static FpaConfigSampleStruct
					*find_valuetype_sample(STRING);
static FpaConfigSampleStruct
					*init_valuetype_sample(STRING);
static void		add_valuetype_sample_identifier(STRING, FpaConfigSampleStruct *);
static FpaConfigSampleStruct
					*find_windtype_sample(STRING);
static FpaConfigSampleStruct
					*init_windtype_sample(STRING);
static void		add_windtype_sample_identifier(STRING, FpaConfigSampleStruct *);

/* Internal static functions (Section Checking) */
static int		push_section(int);
static int		pop_section(void);

/** Error messages for reading configuration files */
typedef enum
		{ FpaCmsgName,        FpaCmsgAlias,        FpaCmsgField,
			FpaCmsgCRef,      FpaCmsgLinkFld,      FpaCmsgSection,
			FpaCmsgKeyword,   FpaCmsgNoEqual,      FpaCmsgParameter,
			FpaCmsgDirTag,    FpaCmsgReset,        FpaCmsgResetNone,
			FpaCmsgResetDefault,
			FpaCmsgMissLine,  FpaCmsgMissSection,  FpaCmsgMissName,
			FpaCmsgSupport,   FpaCmsgMSupport,     FpaCmsgMember,
			FpaCmsgMatch,     FpaCmsgMergeType,    FpaCmsgMergeUnits,
			FpaCmsgInvalid,   FpaCmsgObsolete,     FpaCmsgReplace,
			FpaCmsgBlock
		} FpaCmessageOption;

/* Internal static functions (Miscellaneous Functions) */
static void		config_file_message(STRING, STRING, STRING, STRING,
															FpaCmessageOption);
static int		config_file_macro(STRING, int, const FPA_MACRO_LIST *);
static int		compare_identifiers(const void *, const void *);
static int		compare_identifiers_ic(const void *, const void *);
static int		compare_field_identifiers(const void *, const void *);
static LOGICAL	file_ident_format(STRING, int);

/* Units cross-referenced in configuration file */
static	const	STRING	Hrs = "hr";

/* Functions for reading configuration file arguments */
#define	OKARG(arg)	( !blank(arg) && !same(arg, FpaCplaceHolder) )
#define	ISARG(arg)	( arg && !same(arg, FpaCplaceHolder) )

/***********************************************************************
*                                                                      *
*   r e a d _ c o m p l e t e _ c o n f i g _ f i l e                  *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** This function reads all blocks of the configuration files.
 *
 * @return True if successful.
 **********************************************************************/

LOGICAL					read_complete_config_file

	(
	)

	{

	/* Ensure that all blocks of configuration files are recognized */
	if ( !read_blocks_info() )    return FALSE;

	/* Ensure that each block of configuration files has been read */
	if ( !read_units_info() )     return FALSE;
	if ( !read_constants_info() ) return FALSE;
	if ( !read_sources_info() )   return FALSE;
	if ( !read_groups_info() )    return FALSE;
	if ( !read_levels_info() )    return FALSE;
	if ( !read_elements_info() )  return FALSE;
	if ( !read_fields_info() )    return FALSE;
	if ( !read_crossrefs_info() ) return FALSE;
	if ( !read_samples_info() )   return FALSE;

	/* Return TRUE if all blocks have been read */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   c o n s i s t e n t _ e l e m e n t _ a n d _ l e v e l            *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** Check if an element and level structure are consistent.
 *
 * This function checks whether an element and level structure have
 * consistent level types.  (Structures without consistent level
 * types cannot be combined as a field!)
 *
 * Note that an element level type of FpaC_LEVEL is consistent with
 * a level level type of FpaC_SURFACE or FpaC_LEVEL!
 *
 *	@param[in]	*edef		Element structure
 *	@param[in]	*ldef		Level structure
 * @return True if consistent
 **********************************************************************/

LOGICAL					consistent_element_and_level

	(
	FpaConfigElementStruct	*edef,
	FpaConfigLevelStruct	*ldef
	)

	{

	/* Return FALSE if element or level structure is missing */
	if ( IsNull(edef) || IsNull(ldef) ) return FALSE;

	/* Compare level types for element and level structures */
	switch ( edef->lvl_type )
		{

		/* Level type FpaC_MSL */
		case FpaC_MSL:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_MSL:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Level type FpaC_SURFACE */
		case FpaC_SURFACE:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_SURFACE:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Level type FpaC_LEVEL */
		case FpaC_LEVEL:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_SURFACE:
				case FpaC_LEVEL:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Level type FpaC_LAYER */
		case FpaC_LAYER:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_LAYER:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Level type FpaC_GEOGRAPHY */
		case FpaC_GEOGRAPHY:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_GEOGRAPHY:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Level type FpaC_ANNOTATION */
		case FpaC_ANNOTATION:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_ANNOTATION:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Level type FpaC_LVL_ANY (Special case!) */
		case FpaC_LVL_ANY:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_MSL:
				case FpaC_SURFACE:
				case FpaC_LEVEL:
				case FpaC_LAYER:
				case FpaC_GEOGRAPHY:
				case FpaC_ANNOTATION:
				case FpaC_LVL_ANY:
					return TRUE;

				/* Return FALSE for level type FpaC_LVL_NOTUSED */
				/*  or for unknown level types                  */
				default:
					return FALSE;
				}

		/* Level type FpaC_LVL_NOTUSED (Special case!) */
		case FpaC_LVL_NOTUSED:
			switch ( ldef->lvl_type )
				{

				/* Return TRUE for all acceptable level types */
				case FpaC_LVL_NOTUSED:
					return TRUE;

				/* Return FALSE for all other level types */
				default:
					return FALSE;
				}

		/* Return FALSE for unknown level types */
		default:
			return FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*   m i n u t e s _ i n _ d e p i c t i o n s                          *
*   c h e c k _ d e p i c t i o n _ m i n u t e s                      *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** This function returns the use of minutes in depiction files.
 *
 * @return True if depiction files use minutes.
 **********************************************************************/

LOGICAL					minutes_in_depictions

	(
	)

	{
	FpaConfigSourceStruct	*sdef;

	static	LOGICAL	FirstCall       = TRUE;
	static	LOGICAL	MinutesRequired = FALSE;

	/* Check for the "depict" directory minutes flag              */
	/* This is called once since Config files are read only once! */
	if ( FirstCall )
		{
		sdef = identify_source(FpaDir_Depict, FpaCblank);
		if ( NotNull(sdef) ) MinutesRequired = sdef->minutes_rqd;
		FirstCall = FALSE;
		}

	/* Return the flag for minutes required */
	return MinutesRequired;
	}

/************************************************************************/
/** This function checks if the "depict", "interp" and "backup"
 * directories are consistent in using minutes in filenames.
 *
 * @return True if consistent
 **********************************************************************/
LOGICAL					check_depiction_minutes

	(
	)

	{
	LOGICAL					consistent, mins_depict, mins_interp, mins_backup;
	FpaConfigSourceStruct	*sdef;

	/* Initialize return parameter */
	consistent = TRUE;

	/* Set minutes flag in "depict" directory */
	sdef = identify_source(FpaDir_Depict, FpaCblank);
	mins_depict = minutes_in_depictions();

	/* Check for consistent flags only if the "depict" directory exists */
	if ( NotNull(sdef) )
		{

		/* Check for minutes flag in "interp" directory */
		sdef = identify_source(FpaDir_Interp, FpaCblank);
		if ( NotNull(sdef) )
			{
			mins_interp = sdef->minutes_rqd;
			if ( ( mins_interp && !mins_depict )
					|| ( !mins_interp && mins_depict ) )
				{
				(void) fprintf(stderr, "[check_depiction_minutes]");
				(void) fprintf(stderr, " Inconsistent settings for");
				(void) fprintf(stderr, " \"minutes_required\" keyword");
				(void) fprintf(stderr, " in Config files\n");
				(void) fprintf(stderr, "[check_depiction_minutes]");
				(void) fprintf(stderr, "  Source \"depict\":");
				if ( mins_depict ) (void) fprintf(stderr, " TRUE");
				else               (void) fprintf(stderr, " FALSE");
				(void) fprintf(stderr, "  Source \"interp\":");
				if ( mins_interp ) (void) fprintf(stderr, " TRUE\n");
				else               (void) fprintf(stderr, " FALSE\n");
				consistent = FALSE;
				}
			}

		/* Check for minutes flag in "backup" directory */
		sdef = identify_source(FpaDir_Backup, FpaCblank);
		if ( NotNull(sdef) )
			{
			mins_backup = sdef->minutes_rqd;
			if ( ( mins_backup && !mins_depict )
					|| ( !mins_backup && mins_depict ) )
				{
				(void) fprintf(stderr, "[check_depiction_minutes]");
				(void) fprintf(stderr, " Inconsistent settings for");
				(void) fprintf(stderr, " \"minutes_required\" keyword");
				(void) fprintf(stderr, " in Config files\n");
				(void) fprintf(stderr, "[check_depiction_minutes]");
				(void) fprintf(stderr, "  Source \"depict\":");
				if ( mins_depict ) (void) fprintf(stderr, " TRUE");
				else               (void) fprintf(stderr, " FALSE");
				(void) fprintf(stderr, "  Source \"backup\":");
				if ( mins_backup ) (void) fprintf(stderr, " TRUE\n");
				else               (void) fprintf(stderr, " FALSE\n");
				consistent = FALSE;
				}
			}
		}

	/* Return results of checking */
	return consistent;
	}

/***********************************************************************
*                                                                      *
*   e n t i t y _ f r o m _ f i e l d _ t y p e                        *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** This function returns an entity string based on an enumerated
 * field type.
 *
 *	@param[in]	type		enumerated field type
 * 	@return String based on an enumerated field type.
 **********************************************************************/

STRING					entity_from_field_type

	(
	int			type
	)

	{

	/* Return entity based on enumerated field type */
	switch (type)
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:   return "a";

		/* Vector field type */
		case FpaC_VECTOR:       return "v";

		/* Discrete field type */
		case FpaC_DISCRETE:     return "b";

		/* Wind field type */
		case FpaC_WIND:         return "b";

		/* Line field type */
		case FpaC_LINE:         return "c";

		/* Scattered field type */
		case FpaC_SCATTERED:    return "d";

		/* Link chain field type */
		case FpaC_LCHAIN:       return "l";

		/* Default for all other field types */
		default:            return NullString;
		}
	}

/***********************************************************************
*                                                                      *
*   c h e c k _ f i e l d _ m a c r o                                  *
*   f i e l d _ d a t a _ t y p e                                      *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** This function checks if an enumerated field type macro is in the
 * FpaCfieldTypes list in config_structs.h.
 *
 *	@param[in]	macro		field type macro
 * 	@return True if macro is in the FpaCfieldTypes list.
 **********************************************************************/

LOGICAL					check_field_macro

	(
	int			macro
	)

	{

	/* Check for acceptable field type macros */
	if ( macro == FpaCnoMacro )       return FALSE;
	if ( macro  < 0 )                 return FALSE;
	if ( macro >= NumFpaCfieldTypes ) return FALSE;

	/* Return TRUE if field type macro is acceptable */
	return TRUE;
	}

/************************************************************************/
/** This function returns an enumerated field type macro from the
 * FpaCfieldTypes list in config_structs.h.
 *
 *	@param[in]	name		field type name
 * 	@return enumerated field type macro.
 **********************************************************************/
int						field_data_type

	(
	STRING		name
	)

	{
	int		nn;

	/* Check name against each label in the FpaCfieldTypes list */
	for ( nn=0; nn<NumFpaCfieldTypes; nn++ )
		{
		if ( same_ic(name, FpaCfieldTypes[nn].label) )
				return FpaCfieldTypes[nn].macro;
		}

	/* Default return if name not found */
	return FpaCnoMacro;
	}

/************************************************************************/
/** This function returns an enumerated display format macro from the
 * FpaCdisplayFormats list in config_structs.h based on the element and
 * level names for the field.
 *
 *	@param[in]	element		field element name
 *	@param[in]	level		field level name
 * 	@return enumerated field display format.
 **********************************************************************/
int						field_display_format

	(
	STRING		element,
	STRING		level
	)

	{
	FpaConfigFieldStruct	*fdef;

	/* Get the detailed element information */
	fdef = get_field_info(element, level);

	/* Return the field display format */
	if (NotNull(fdef)) return fdef->element->display_format;

	/* Default return if name not found */
	else               return FpaCnoMacro;
	}

/***********************************************************************
*                                                                      *
*   c h e c k _ a t t a c h _ o p t i o n                              *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** This function checks if a field has an acceptable attach option.
 *
 *  @param[in]	ftype		field type macro
 *  @param[in]	attach		field attach option
 *  @param[in]	*feature	feature attachment
 * 	@return True if field has an acceptable attach option.
 **********************************************************************/
LOGICAL					check_attach_option

	(
	FpaCfieldTypeOption		ftype,
	FpaCattachOption		attach,
	SPFEAT					*feature
	)

	{
	SPFEAT	xfeature;

	/* Initialize return parameter */
	if (NotNull(feature)) *feature = AttachNone;

	/* Check for acceptable attach options for each type of field */
	switch (ftype)
		{
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
			switch (attach)
				{
				case FpaC_NO_ATTACH:		xfeature = AttachNone;		break;
				case FpaC_ATTACH_AUTO:		xfeature = AttachAuto;		break;
				case FpaC_ATTACH_CONTOUR:	xfeature = AttachContour;	break;
				case FpaC_ATTACH_MIN:		xfeature = AttachMin;		break;
				case FpaC_ATTACH_MAX:		xfeature = AttachMax;		break;
				case FpaC_ATTACH_COL:		xfeature = AttachCol;		break;
				default:
					return FALSE;
				}
			break;

		case FpaC_DISCRETE:
			switch (attach)
				{
				case FpaC_NO_ATTACH:		xfeature = AttachNone;		break;
				case FpaC_ATTACH_AUTO:		xfeature = AttachAuto;		break;
				case FpaC_ATTACH_BOUND:		xfeature = AttachBound;		break;
				case FpaC_ATTACH_DIV:		xfeature = AttachDiv;		break;
				default:
					return FALSE;
				}
			break;

		case FpaC_WIND:
			switch (attach)
				{
				case FpaC_NO_ATTACH:		xfeature = AttachNone;		break;
				case FpaC_ATTACH_AUTO:		xfeature = AttachAuto;		break;
				case FpaC_ATTACH_BOUND:		xfeature = AttachBound;		break;
				case FpaC_ATTACH_DIV:		xfeature = AttachDiv;		break;
				default:
					return FALSE;
				}
			break;

		case FpaC_LINE:
			switch (attach)
				{
				case FpaC_NO_ATTACH:		xfeature = AttachNone;		break;
				case FpaC_ATTACH_AUTO:		xfeature = AttachAuto;		break;
				case FpaC_ATTACH_LINE:		xfeature = AttachLine;		break;
				default:
					return FALSE;
				}
			break;

		case FpaC_SCATTERED:
			switch (attach)
				{
				case FpaC_NO_ATTACH:		xfeature = AttachNone;		break;
				case FpaC_ATTACH_AUTO:		xfeature = AttachAuto;		break;
				case FpaC_ATTACH_POINT:		xfeature = AttachPoint;		break;
				default:
					return FALSE;
				}
			break;

		case FpaC_LCHAIN:
		default:
			return FALSE;
		}

	/* Return TRUE if field type macro is acceptable */
	if (NotNull(feature)) *feature = xfeature;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   p a r s e _ s o u r c e _ n a m e                                  *
*                                                                      *
***********************************************************************/
/************************************************************************/
/** This function returns pointers to source and subsource names
 *   determined from parsing a string of format \<source\>:\<subsource\>
 *
 *	@param[in]		ident		source/subsource identifier
 *	@param[out]		*name		source name
 *	@param[out]		*subname	subsource name
 *  @return True if successful in setting name and subname.
 **********************************************************************/
LOGICAL						parse_source_name

	(
	STRING		ident,
	STRING		*name,
	STRING		*subname
	)

	{
	size_t		nlen, ndelim, nsrc, nsubsrc;

	/* Static buffers for source and subsource names */
	static	char	src[SOURCE_NAME_LEN + 1];
	static	char	subsrc[SOURCE_NAME_LEN + 1];

	/* Initialize return parameters */
	if ( NotNull(name) )    *name    = NullString;
	if ( NotNull(subname) ) *subname = NullString;

	/* Return FALSE if no source/subsource identifier passed */
	if ( blank(ident) ) return FALSE;

	/* Identify length of source name and location of subsource */
	/*  name (if one is found!)                                 */
	nlen   = strlen(ident);
	ndelim = strcspn(ident, FpaFsourceDelimiter);
	nsrc   = ndelim;
	if ( nlen > ndelim )
		nsubsrc = nlen - ndelim - 1;
	else
		nsubsrc = 0;

	/* Error message if source identifier is wrong length! */
	if ( (int) nsrc > SOURCE_NAME_LEN || (int) nsubsrc > SOURCE_NAME_LEN )
		{
		(void) pr_error("Config",
				"[parse_source_name] Error in length of source name \"%s\"!\n",
				ident);
		return FALSE;
		}

	/* Return ident as the source name if no delimiter found */
	if ( ndelim >= nlen )
		{
		(void) strcpy(src, ident);
		if ( NotNull(name) ) *name = src;
		return TRUE;
		}

	/* Split the source/subsource identifier into source and subsource names */
	(void) strncpy(src, ident, nsrc);
	src[nsrc] = '\0';
	(void) strcpy(subsrc, ident + ndelim + 1);

	/* Set source and subsource names and return TRUE if all went OK */
	if ( NotNull(name) )    *name    = src;
	if ( NotNull(subname) ) *subname = subsrc;
	return TRUE;
	}


/***********************************************************************
*                                                                      *
*   x y _ c o m p o n e n t _ f i e l d                                *
*   d m _ c o m p o n e n t _ f i e l d                                *
*   w h i c h _ c o m p o n e n t s                                    *
*   c h e c k _ r e p r o j e c t i o n _ f o r _ c o m p o n e n t s  *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** This function checks whether an element has x and y components.
 *
 * 	@param[in]	name		element name
 * 	@return True if element has x and y components.
 **********************************************************************/
LOGICAL					xy_component_field

	(
	STRING		name
	)

	{
	FpaConfigElementStruct	*edef;

	/* Get detailed information for element */
	edef = get_element_info(name);
	if ( IsNull(edef) ) return FALSE;
	if ( IsNull(edef->elem_detail) ) return FALSE;

	/* Return FALSE if field does not contain components */
	if ( IsNull(edef->elem_detail->components) ) return FALSE;

	/* Check for x/y component information */
	switch ( edef->elem_detail->components->cinfo->need )
		{

		/* Return TRUE for fields with x/y component information */
		case XY_Comp:
			return TRUE;

		/* Return FALSE for all other fields */
		default:
			return FALSE;
		}
	}

/**********************************************************************/
/** This function checks whether an element has direction and
 * magnitude components.
 *
 *	@param[in]	name		element name
 * 	@return True if element has direction and magnitude components.
 **********************************************************************/
LOGICAL					dm_component_field

	(
	STRING		name
	)

	{
	FpaConfigElementStruct	*edef;

	/* Get detailed information for element */
	edef = get_element_info(name);
	if ( IsNull(edef) ) return FALSE;
	if ( IsNull(edef->elem_detail) ) return FALSE;

	/* Return FALSE if field does not contain components */
	if ( IsNull(edef->elem_detail->components) ) return FALSE;

	/* Check for d/m component information */
	switch ( edef->elem_detail->components->cinfo->need )
		{

		/* Return TRUE for fields with d/m component information */
		case DM_Comp:
			return TRUE;

		/* Return FALSE for all other fields */
		default:
			return FALSE;
		}
	}

/**********************************************************************/
/** This function returns the type of component for a given element,
 * as well as the name and type of the second component.
 *
 *	@param[in]	name		element name
 *	@param[out]	*comp_name	element name of second component
 *	@param[out]	*comp_type	type of second component
 * @return The type of component for a given element.
 **********************************************************************/
COMPONENT				which_components

	(
	STRING		name,
	STRING		*comp_name,
	COMPONENT	*comp_type
	)

	{
	int									nc;
	COMPONENT							tcomp, scomp;
	FpaConfigElementStruct				*edef;
	FpaConfigElementComponentStruct		*cmpnt;

	/* Initialize return parameters */
	if ( NotNull(comp_name) ) *comp_name = NullString;
	if ( NotNull(comp_type) ) *comp_type = No_Comp;

	/* Get detailed information for element */
	edef = get_element_info(name);
	if ( IsNull(edef) ) return No_Comp;
	if ( IsNull(edef->elem_detail) ) return No_Comp;

	/* Return if element does not contain components */
	cmpnt = edef->elem_detail->components;
	if ( IsNull(cmpnt) ) return No_Comp;

	/* Identify component for this element name */
	for ( nc=0; nc<cmpnt->ncomp; nc++ )
		if ( edef == cmpnt->comp_edefs[nc] ) break;
	if ( nc >= cmpnt->ncomp )
		{
		(void) fprintf(stderr, "[which_components] Cannot identify");
		(void) fprintf(stderr, " component for element: \"%s\"\n",
				SafeStr(edef->name));
		return No_Comp;
		}
	tcomp = cmpnt->comp_types[nc];

	/* Identify the second component type */
	switch ( tcomp )
		{

		/* X component found ... so identify Y component */
		case X_Comp:
			scomp = Y_Comp;
			break;

		/* Y component found ... so identify X component */
		case Y_Comp:
			scomp = X_Comp;
			break;

		/* D component found ... so identify M component */
		case D_Comp:
			scomp = M_Comp;
			break;

		/* M component found ... so identify D component */
		case M_Comp:
			scomp = D_Comp;
			break;

		/* Return for all other component types */
		default:
			return tcomp;
		}

	/* Identify the other component */
	for ( nc=0; nc<cmpnt->ncomp; nc++ )
		if ( cmpnt->comp_types[nc] == scomp ) break;
	if ( nc >= cmpnt->ncomp )
		{
		(void) fprintf(stderr, "[which_components] Cannot identify");
		(void) fprintf(stderr, " second component for element: \"%s\"\n",
				SafeStr(edef->name));
		return tcomp;
		}

	/* Return the component information */
	if ( NotNull(comp_name) ) *comp_name = cmpnt->comp_edefs[nc]->name;
	if ( NotNull(comp_type) ) *comp_type = scomp;
	return tcomp;
	}

/**********************************************************************/
/** This function checks whether an element with x and y components
 * will require reprojection due to a change of map projections
 *
 *	@param[in]	name		element name
 *	@param[in]	*mprojin	input map projection
 *	@param[out]	*mprojout	output map projection
 * 	@return True if element will require reprojection.
 **********************************************************************/
LOGICAL					check_reprojection_for_components

	(
	STRING		name,
	MAP_PROJ	*mprojin,
	MAP_PROJ	*mprojout
	)

	{

	/* Return FALSE if field does not contain x/y components */
	if ( !xy_component_field(name) ) return FALSE;

	/* Return FALSE if map projections are missing */
	if ( IsNull(mprojin) || IsNull(mprojout) ) return FALSE;

	/* Return TRUE if map projection type has changed    */
	/*  ... x/y components will need to be redetermined! */
	if ( !same_projection(&(mprojin->projection), &(mprojout->projection)) )
		return TRUE;

	/* Check for each type of map projection */
	switch ( mprojin->projection.type )
		{

		/* Return FALSE for map projection types which cannot be rotated */
		case ProjectNone:
		case ProjectLatLon:
		case ProjectPlateCaree:
		case ProjectMercatorEq:
			return FALSE;

		/* Check map projection types that can be rotated */
		case ProjectLatLonAng:
		case ProjectPolarSt:
		case ProjectObliqueSt:
		case ProjectLambertConf:
		default:

			/* Return FALSE if map projection has same reference longitude */
			if ( mprojin->definition.lref == mprojout->definition.lref )
				return FALSE;

			/* Return TRUE if map projection has been rotated    */
			/*  ... x/y components will need to be redetermined! */
			else
				return TRUE;
		}
	}

/***********************************************************************
*                                                                      *
*   i d e n t i f y _ u n i t                                          *
*   i d e n t i f y _ m k s _ u n i t s                                *
*   i d e n t i f y _ m k s _ u n i t s _ f r e e                      *
*   i d e n t i f y _ u n i t s _ b y _ m k s                          *
*   i d e n t i f y _ u n i t s _ b y _ m k s _ f r e e                *
*   c o n v e r t _ v a l u e                                          *
*                                                                      *
***********************************************************************/

/* Storage for Units information */
static	LOGICAL					UnitsRead  = FALSE;
static	LOGICAL					UnitsValid = FALSE;
static	int						NumUnitDef = 0;
static	FpaConfigUnitStruct		**UnitDefs = NullPtr(FpaConfigUnitStruct **);

/* Storage for Units identifier information */
static	int						NumUnitIdent = 0;
static	FPAC_IDENTS				*UnitIdents  = NullPtr(FPAC_IDENTS *);


/************************************************************************/
/** This function returns a pointer to a structure for a named unit
 * in the Units blocks of the configuration files.
 *
 *	@param[in]	name		unit name
 * 	@return Pointer to a named unit structure.
 **********************************************************************/
FpaConfigUnitStruct			*identify_unit

	(
	STRING		name
	)

	{
	FpaConfigUnitStruct		*udef;

	/* Ensure that configuration file has been read */
	if ( !read_units_info() ) return NullPtr(FpaConfigUnitStruct *);

	/* Return Null pointer if unit name not found in list */
	udef = find_unit(name);
	if ( IsNull(udef) ) return NullPtr(FpaConfigUnitStruct *);

	/* Error message if unit data is not OK */
	if ( !udef->valid )
		{
		(void) config_file_message(FpaCblockUnits,
				udef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigUnitStruct *);
		}

	/* Return pointer to Unit structure */
	return udef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for MKS
 * standard units in the Units blocks of the configuration files.
 *
 *	@param[out]	***list	list of MKS units
 *  @return The size of the list.
 **********************************************************************/
int						identify_mks_units

	(
	FpaConfigUnitStruct		***list
	)

	{
	int						nudefs, nn;
	FpaConfigUnitStruct		*udef;
	FpaConfigUnitStruct		**udeflist = NullPtr(FpaConfigUnitStruct **);

	/* Initialize list of pointers to MKS units */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigUnitStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_units_info() ) return 0;

	/* Check for MKS units */
	for ( nudefs=0, nn=0; nn<NumUnitDef; nn++ )
		{
		udef = UnitDefs[nn];
		if ( same(udef->name, udef->MKS) )
			{

			/* Add to list of pointers to MKS units */
			/*  ... but only if list is returned!   */
			nudefs++;
			if ( NotNull(list) )
				{
				udeflist = GETMEM(udeflist, FpaConfigUnitStruct *, nudefs);
				udeflist[nudefs-1] = udef;
				}
			}
		}

	/* Return the list of pointers to MKS units */
	if ( NotNull(list) ) *list = udeflist;
	return nudefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures for MKS
 *   standard units in the Units blocks of the configuration files.
 *
 *	@param[in]	***list		list of Unit Structures
 *	@param[in]	num			size of list
 * 	@return The size of the list (0)
 **********************************************************************/
int						identify_mks_units_free

	(
	FpaConfigUnitStruct		***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for units
 * equivalent to an MKS standard unit in the Units blocks of the
 * configuration files.
 *
 *	@param[in]	name	MKS unit name
 *	@param[out]	***list	list of units with same MKS units
 *  @return Size of the list.
 **********************************************************************/
int						identify_units_by_mks

	(
	STRING					name,
	FpaConfigUnitStruct		***list
	)

	{
	int						nn, nudefs;
	FpaConfigUnitStruct		*udef;
	FpaConfigUnitStruct		**udeflist = NullPtr(FpaConfigUnitStruct **);

	/* Initialize list of pointers to units with same MKS units */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigUnitStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_units_info() ) return 0;

	/* Check for units with same MKS units */
	for ( nudefs=0, nn=0; nn<NumUnitDef; nn++ )
		{
		udef = UnitDefs[nn];
		if ( same(name, udef->MKS) )
			{

			/* Add to list of pointers to units with same MKS units */
			/*  ... but only if list is returned!                   */
			nudefs++;
			if ( NotNull(list) )
				{
				udeflist = GETMEM(udeflist, FpaConfigUnitStruct *, nudefs);
				udeflist[nudefs-1] = udef;
				}
			}
		}

	/* Return the list of pointers to units with same MKS units */
	if ( NotNull(list) ) *list = udeflist;
	return nudefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures for units
 * equivalent to an MKS standard unit in the Units blocks of the
 * configuration files.
 *
 *	@param[in]	***list 	list of Unit Structures
 *	@param[in]	num			size of list
 * 	@return The size of the list (0);
 **********************************************************************/
int						identify_units_by_mks_free

	(
	FpaConfigUnitStruct		***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/**
 * @brief This function converts a value from one named unit to another, if
 *   the conversion can be done.
 *	@param[in]	from		unit name to convert from
 *	@param[in]	value		value in "from" units
 *	@param[in]	to			unit name to convert to
 *	@param[out]	*newvalue	value converted to "to" units
 *  @return True if convertion can be done.
 **********************************************************************/
LOGICAL					convert_value

	(
	STRING		from,
	double		value,
	STRING		to,
	double		*newvalue
	)

	{
	FpaConfigUnitStruct		*ufrom, *uto;

	/* Static buffers to speed up multiple calls */
	static	LOGICAL					MksSet = FALSE;
	static	FpaConfigUnitStruct		*Umks, *UfromSaved, *UtoSaved;

	/* Initialize return parameter */
	if ( NotNull(newvalue) ) *newvalue = 0.0;

	/* Ensure that configuration file has been read */
	if ( !read_units_info() ) return FALSE;

	/* Get pointer for FpaCmksUnits units only once! */
	if ( !MksSet )
		{
		Umks = find_unit(FpaCmksUnits);
		MksSet = TRUE;
		}

	/* Get pointers for unit names for conversion */
	if ( NotNull(UfromSaved) && same(from, UfromSaved->name) )
		{
		ufrom = UfromSaved;
		}
	else
		{
		ufrom      = find_unit(from);
		UfromSaved = ufrom;
		}
	if ( NotNull(UtoSaved) && same(to, UtoSaved->name) )
		{
		uto = UtoSaved;
		}
	else
		{
		uto      = find_unit(to);
		UtoSaved = uto;
		}

	/* Return FALSE if problems with either unit name */
	if ( IsNull(ufrom) || !ufrom->valid || IsNull(uto) || !uto->valid )
		{
		if ( IsNull(ufrom) )
			(void) config_file_message(FpaCblockUnits,
					from, FpaCblank, FpaCblank, FpaCmsgMissName);
		else if ( !ufrom->valid )
			(void) config_file_message(FpaCblockUnits,
					ufrom->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		if ( IsNull(uto) )
			(void) config_file_message(FpaCblockUnits,
					to, FpaCblank, FpaCblank, FpaCmsgMissName);
		else if ( !uto->valid )
			(void) config_file_message(FpaCblockUnits,
					uto->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return FALSE;
		}

	/* Return value if unit names match */
	if ( ufrom == uto )
		{
		if ( NotNull(newvalue) ) *newvalue = value;
		return TRUE;
		}

	/* Check for valid MKS units */
	if ( IsNull(Umks) || !Umks->valid )
		{
		if ( IsNull(Umks) )
			(void) config_file_message(FpaCblockUnits,
					FpaCmksUnits, FpaCblank, FpaCblank, FpaCmsgMissName);
		else if ( !Umks->valid )
			(void) config_file_message(FpaCblockUnits,
					Umks->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return FALSE;
		}

	/* Check if conversion can be done                        */
	/*  ... though any units can be converted to or from MKS! */
	if ( (!same(ufrom->MKS, uto->MKS)) && (ufrom != Umks) && (uto != Umks) )
		{
		(void) pr_error("Config",
				"[convert_value] Cannot convert from \"%s\" to \"%s\"!\n",
				ufrom->name, uto->name);
		return FALSE;
		}

	/* Convert the value and return TRUE */
	if ( NotNull(newvalue) )
		{
		value    -= ufrom->offset;
		value    /= ufrom->factor;
		value    *= uto->factor;
		value    += uto->offset;
		*newvalue = value;
		}
	return TRUE;
	}


/***********************************************************************
*                                                                      *
*   i d e n t i f y _ c o n s t a n t                                  *
*   i d e n t i f y _ c o n s t a n t s _ b y _ g r o u p              *
*   i d e n t i f y _ c o n s t a n t s _ b y _ g r o u p _ f r e e    *
*                                                                      *
***********************************************************************/

/* Storage for Constants information */
static	LOGICAL					ConstantsRead  = FALSE;
static	LOGICAL					ConstantsValid = FALSE;
static	int						NumConstDef    = 0;
static	FpaConfigConstantStruct	**ConstDefs    = NullPtr(FpaConfigConstantStruct **);

/* Storage for Constants identifier information */
static	int						NumConstIdent  = 0;
static	FPAC_IDENTS				*ConstIdents   = NullPtr(FPAC_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named
 * constant in the Constants blocks of the configuration files.
 *
 *	@param[in]	name		constant name
 * @return Pointer to named constant structure.
 **********************************************************************/
FpaConfigConstantStruct		*identify_constant

	(
	STRING		name
	)

	{
	FpaConfigConstantStruct		*cdef;

	/* Ensure that configuration file has been read */
	if ( !read_constants_info() ) return NullPtr(FpaConfigConstantStruct *);

	/* Return Null pointer if constant name not found in list */
	cdef = find_constant(name);
	if ( IsNull(cdef) ) return NullPtr(FpaConfigConstantStruct *);

	/* Error message if constant data is not OK */
	if ( !cdef->valid )
		{
		(void) config_file_message(FpaCblockConstants,
				cdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigConstantStruct *);
		}

	/* Return pointer to Constant structure */
	return cdef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for a given
 * group of constants in the Constants blocks of the configuration
 * files.
 *
 *	@param[in]	cgroup	constant group name (not presently used!)
 *	@param[out]	***list	list of constants in the same constant group
 *  @return The size of the list
 **********************************************************************/
int						identify_constants_by_group

	(
	STRING					cgroup,
	FpaConfigConstantStruct	***list
	)

	{
	int						ncdefs, nn;
	FpaConfigConstantStruct	*cdef;
	FpaConfigConstantStruct	**cdeflist = NullPtr(FpaConfigConstantStruct **);

	/* Initialize list of pointers to constants in the same constant group */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigConstantStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_constants_info() ) return 0;

	/* Add to list of pointers to constants with same constant group */
	/*  ... but only if list is returned!                            */
	/* Note that ALL constants are presently added to the list!      */
	for ( ncdefs=0, nn=0; nn<NumConstDef; nn++ )
		{
		cdef = ConstDefs[nn];
		if ( !blank(cgroup) )
			{

			/* Add to list of pointers to constants in the same constant group */
			ncdefs++;
			if ( NotNull(list) )
				{
				cdeflist = GETMEM(cdeflist, FpaConfigConstantStruct *, ncdefs);
				cdeflist[ncdefs-1] = cdef;
				}
			}
		}

	/* Return the list of pointers to constants in the same constant group */
	if ( NotNull(list) ) *list = cdeflist;
	return ncdefs;
	}

/**********************************************************************/

/************************************************************************/
/**  This function frees a list of pointers to structures for a given
 * group of constants in the Constants blocks of the configuration files.
 *
 *	@param[in]	***list		list of Constant Structures
 *	@param[in]	num			size of list
 * 	@return The size of the list (0)
 **********************************************************************/
int						identify_constants_by_group_free

	(
	FpaConfigConstantStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/***********************************************************************
*                                                                      *
*   i d e n t i f y _ s o u r c e                                      *
*   g e t _ s o u r c e _ i n f o                                      *
*   i d e n t i f y _ s o u r c e s _ b y _ t y p e                    *
*   i d e n t i f y _ s o u r c e s _ b y _ t y p e _ f r e e          *
*   i d e n t i f y _ s o u r c e _ a l i a s e s                      *
*   i d e n t i f y _ s o u r c e _ a l i a s e s _ f r e e            *
*   e q u i v a l e n t _ s o u r c e _ d e f i n i t i o n s          *
*   s o u r c e _ a l l i e d _ d a t a _ l o c a t i o n              *
*                                                                      *
***********************************************************************/
/* Storage for Sources information */
static	LOGICAL					SourcesRead  = FALSE;
static	LOGICAL					SourcesValid = FALSE;
static	int						NumSourceDef = 0;
static	FpaConfigSourceStruct	**SourceDefs = NullPtr(FpaConfigSourceStruct **);

/* Storage for Sources identifier information */
static	int						NumSourceIdent = 0;
static	FPAC_IDENTS				*SourceIdents  = NullPtr(FPAC_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named source
 * in the Sources blocks of the configuration files.
 *
 * Note that source name comparisons are case insensitive!
 *
 *	@param[in]	name		source name
 *	@param[in]	subname		subsource name
 * 	@return Named source information.
 **********************************************************************/
FpaConfigSourceStruct		*identify_source

	(
	STRING		name,
	STRING		subname
	)

	{
	int						nn;
	STRING					src, subsrc;
	char					srcbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigSourceStruct	*sdef;

	/* Ensure that configuration file has been read */
	if ( !read_sources_info() ) return NullPtr(FpaConfigSourceStruct *);

	/* Parse the source name (if required) */
	if ( !parse_source_name(name, &src, &subsrc) )
			return NullPtr(FpaConfigSourceStruct *);

	/* Return Null pointer if source name not found in list */
	sdef = find_source(src);
	if ( IsNull(sdef) ) return NullPtr(FpaConfigSourceStruct *);

	/* Error message if source data is not OK */
	if ( !sdef->valid )
		{
		(void) config_file_message(FpaCblockSources,
				sdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigSourceStruct *);
		}

	/* Return pointer to Source structure if no subsource name given */
	/*  ... after setting subsource pointer to the default!          */
	if ( blank(subname) && blank(subsrc) )
		{
		sdef->src_sub = sdef->subsrcs[0];
		return sdef;
		}

	/* Warning if subsource passed in both source and subsource names! */
	/* Note that the subsource name will be used if the two disagree!  */
	if ( !blank(subname) && !blank(subsrc) )
		{
		(void) pr_warning("Config",
				"[identify_source] Subsource duplication in names!\n");
		(void) pr_warning("Config",
				"     Source name: \"%s\"   Subsource name: \"%s\"\n",
						name, subname);
		if ( !same(subname, subsrc) ) subsrc = subname;
		}

	/* Set subsource name for checking */
	else if ( blank(subsrc) ) subsrc = subname;

	/* Check for subsource name in list                */
	/* Note that the first declaration is the default! */
	for ( nn=1; nn<sdef->nsubsrc; nn++ )
		{
		if ( same_ic(subsrc, sdef->subsrcs[nn]->name) )
			{

			/* Set pointer to matching subsource and */
			/*  return pointer to Source structure   */
			sdef->src_sub = sdef->subsrcs[nn];
			return sdef;
			}
		}

	/* Error message if subsource name not found in list */
	(void) strcpy(srcbuf, sdef->name);
	(void) strcat(srcbuf, " ");
	(void) strcat(srcbuf, subsrc);
	(void) config_file_message(FpaCblockSources,
			srcbuf, FpaCblank, FpaCblank, FpaCmsgMissName);
	return NullPtr(FpaConfigSourceStruct *);
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a pointer to a structure for a named source
 * in the Sources blocks of the configuration files.
 *
 * Note that source name comparisons are case insensitive!
 *
 *	@param[in]	name		source name
 *	@param[in]	subname		subsource name
 * 	@return Allied Model information.
 **********************************************************************/
FpaConfigSourceStruct		*get_source_info

	(
	STRING		name,
	STRING		subname
	)

	{
	int								srctype, mm, nn;
	FpaConfigSourceStruct			*sdef, *sdefinfo;
	FpaConfigSourceAlliedStruct		*allied;

	/* Get pointer to structure with Allied Model information added */
	sdef = read_source_allied_info(name, subname);
	if ( IsNull(sdef) || !sdef->valid_allied )
			return NullPtr(FpaConfigSourceStruct *);

	/* Set pointers to Allied Model information */
	srctype = sdef->src_type;
	allied  = sdef->allied;

	/* Special one time check for Allied Model required info and metafiles */
	if ( srctype == FpaC_ALLIED && NotNull(allied) && !allied->check_allied )
		{
		(void) pr_diag("Config", "One time Allied Model check for: \"%s\"\n",
				sdef->name);
		allied->check_allied = TRUE;

		/* Set default source for Allied Model input (if required) */
		if ( IsNull(allied->src_def) )
			{
			sdefinfo = identify_source(FpaDir_Depict, FpaCblank);
			if ( NotNull(sdefinfo) )
				{
				allied->src_def = sdefinfo;
				allied->sub_def = sdefinfo->src_sub;
				}
			else
				{
				(void) pr_error("Config",
						"[get_source_info] No default directory \"%s\" for Allied Model input files!\n",
						FpaDir_Depict);
				sdef->valid_allied = FALSE;
				}
			}

		/* Check all Allied Model required fields for field and source info */
		if ( NotNull(allied->fields) && allied->fields->nfields > 0 )
			{

			/* Check each required field for field information */
			for ( nn=0; nn<allied->fields->nfields; nn++ )
				{
				if ( IsNull(allied->fields->flds[nn]) )
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedRequiredFields,
							FpaCalliedFieldInfo, FpaCmsgMissLine);
					sdef->valid_allied = FALSE;
					}
				}

			/* Set source info to default values (if required) */
			for ( nn=0; nn<allied->fields->nfields; nn++ )
				{
				if ( IsNull(allied->fields->src_defs[nn]) )
					{
					allied->fields->src_defs[nn] = allied->src_def;
					allied->fields->sub_defs[nn] = allied->sub_def;
					}
				}
			}

		/* Check all Allied Model required wind crossreferences */
		/*  for source info                                     */
		if ( NotNull(allied->winds) && allied->winds->nwinds > 0 )
			{

			/* Set source info to default values (if required) */
			for ( nn=0; nn<allied->winds->nwinds; nn++ )
				{
				if ( IsNull(allied->winds->src_defs[nn]) )
					{
					allied->winds->src_defs[nn] = allied->src_def;
					allied->winds->sub_defs[nn] = allied->sub_def;
					}
				}
			}

		/* Check all Allied Model required value crossreferences */
		/*  for source info                                      */
		if ( NotNull(allied->values) && allied->values->nvalues > 0 )
			{

			/* Set source info to default values (if required) */
			for ( nn=0; nn<allied->values->nvalues; nn++ )
				{
				if ( IsNull(allied->values->src_defs[nn]) )
					{
					allied->values->src_defs[nn] = allied->src_def;
					allied->values->sub_defs[nn] = allied->sub_def;
					}
				}
			}

		/* Check all Allied Model metafiles for field and matching file info */
		if ( NotNull(allied->metafiles) && allied->metafiles->nfiles > 0 )
			{

			/* Check each metafile for field information */
			for ( nn=0; nn<allied->metafiles->nfiles; nn++ )
				{
				if ( IsNull(allied->metafiles->flds[nn]) )
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedMetafiles,
							FpaCalliedFieldInfo, FpaCmsgMissLine);
					sdef->valid_allied = FALSE;
					}
				}

			/* Check that all Allied Model metafile input file aliases are */
			/*  from files in Allied Model file list                       */
			for ( nn=0; nn<allied->metafiles->nfiles; nn++ )
				{

				/* Skip metafiles that are not from Allied Model files */
				if ( blank(allied->metafiles->file_aliases[nn]) ) continue;

				/* Error message if there are no Allied Model files */
				if ( IsNull(allied->files) || allied->files->nfiles < 1 )
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCblank,
							FpaCalliedFiles, FpaCmsgMissLine);
					sdef->valid_allied = FALSE;
					break;
					}

				/* Checking against Allied Model file list */
				for ( mm=0; mm<allied->files->nfiles; mm++ )
					if ( same_ic(allied->metafiles->file_aliases[nn],
									allied->files->aliases[mm]) ) break;
				if ( mm >= allied->files->nfiles )
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCblank,
							FpaCalliedMetafiles, FpaCmsgMatch);
					sdef->valid_allied = FALSE;
					}
				}
			}
		}

	/* Return pointer based on Allied Model information OK */
	return ( sdef->valid_allied ) ? sdef: NullPtr(FpaConfigSourceStruct *);
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for a given
 * type of source in the Sources blocks of the configuration files.
 *
 *	@param[in]	stype	enumerated source type
 *	@param[out]	***list	list of sources with same source type
 * @return The size of the list.
 **********************************************************************/
int						identify_sources_by_type

	(
	int						stype,
	FpaConfigSourceStruct	***list
	)

	{
	int						nsdefs, nn;
	FpaConfigSourceStruct	*sdef;
	FpaConfigSourceStruct	**sdeflist = NullPtr(FpaConfigSourceStruct **);

	/* Initialize list of pointers to sources with same source type */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigSourceStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_sources_info() ) return 0;

	/* Check for sources with same source type */
	for ( nsdefs=0, nn=0; nn<NumSourceDef; nn++ )
		{
		sdef = SourceDefs[nn];
		if ( stype == FpaC_SRC_ANY || stype == sdef->src_type )
			{

			/* Add to list of pointers to sources with same source type */
			/*  ... but only if list is returned!                       */
			nsdefs++;
			if ( NotNull(list) )
				{
				sdeflist = GETMEM(sdeflist, FpaConfigSourceStruct *, nsdefs);
				sdeflist[nsdefs-1] = sdef;
				}
			}
		}

	/* Return the list of pointers to sources with same source type */
	if ( NotNull(list) ) *list = sdeflist;
	return nsdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures for a given
 * type of source in the Sources blocks of the configuration files.
 *
 *	@param[in]	***list		list of Source Structures
 *	@param[in]	num			size of list
 * 	@return The size of list (0)
 **********************************************************************/
int						identify_sources_by_type_free

	(
	FpaConfigSourceStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to aliases for a given
 * source in the Sources blocks of the configuration files.
 *
 *	@param[in]	*sdef	Source structure
 *	@param[out]	**list	list of source aliases
 * @return The size of the list.
 **********************************************************************/
int						identify_source_aliases

	(
	FpaConfigSourceStruct	*sdef,
	STRING					**list
	)

	{
	int			naliases, nn;
	STRING		*aliaslist = NullStringList;

	/* Initialize list of pointers to source aliases */
	if ( NotNull(list) ) *list = NullStringList;

	/* Ensure that configuration file has been read */
	if ( !read_sources_info() ) return 0;

	/* Check for sources that are aliases */
	for ( naliases=0, nn=0; nn<NumSourceIdent; nn++ )
		{
		if ( sdef == (FpaConfigSourceStruct *) SourceIdents[nn].pdef )
			{

			/* Add to list of aliases             */
			/*  ... but only if list is returned! */
			naliases++;
			if ( NotNull(list) )
				{
				aliaslist = GETMEM(aliaslist, STRING, naliases);
				aliaslist[naliases-1] = SourceIdents[nn].ident;
				}
			}
		}

	/* Return the list of aliases of the source name */
	if ( NotNull(list) ) *list = aliaslist;
	return naliases;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to aliases for a given
 * source in the Sources blocks of the configuration files.
 *
 *	@param[in]	**list		list of Source aliases
 *	@param[in]	num			number of pointers in list
 * 	@return The size of the list (0).
 **********************************************************************/
int						identify_source_aliases_free

	(
	STRING		**list,
	int			num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function checks if two source names are equivalent.
 *
 *	@param[in]	name1		first source name to compare
 *	@param[in]	subname1	first subsource name to compare
 *	@param[in]	name2		second source name to compare
 *	@param[in]	subname2	second subsource name to compare
 * 	@return True if source names are equivalent.
 **********************************************************************/
LOGICAL					equivalent_source_definitions

	(
	STRING		name1,
	STRING		subname1,
	STRING		name2,
	STRING		subname2
	)

	{
	FpaConfigSourceStruct	*sdef1, *sdef2;

	/* Static buffer for FpaCanySource source */
	static	FpaConfigSourceStruct	*AnySource = NullPtr(FpaConfigSourceStruct *);

	/* Get pointer for special source FpaCanySource */
	if ( IsNull(AnySource) )
		{
		AnySource = identify_source(FpaCanySource, FpaCblank);
		}

	/* Get pointers for source names to compare */
	sdef1 = identify_source(name1, subname1);
	sdef2 = identify_source(name2, subname2);

	/* Return FALSE if problems with either source name */
	if ( IsNull(sdef1) || IsNull(sdef2) ) return FALSE;

	/* Return TRUE if source and subsource names match */
	if ( (sdef1 == sdef2) && (sdef1->src_sub == sdef2->src_sub) ) return TRUE;

	/* Return FALSE if special source FpaCanySource not found */
	if ( IsNull(AnySource) || !AnySource->valid )
		{
		if ( IsNull(AnySource) )
			(void) config_file_message(FpaCblockSources,
					FpaCanySource, FpaCblank, FpaCblank, FpaCmsgMissName);
		else if ( !AnySource->valid )
			(void) config_file_message(FpaCblockSources,
					AnySource->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return FALSE;
		}

	/* Return TRUE if either source name is special source FpaCanySource */
	if ( (sdef1 == AnySource) || (sdef2 == AnySource) ) return TRUE;

	/* Return FALSE if source names do not match */
	return FALSE;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns the location in the data lists for Allied
 * Model information of a given type and alias (or -1 if not found)
 *
 *	@param[in]	*sdef	Source structure
 *	@param[in]	type	enumerated Allied Model data type
 *	@param[in]	alias	Allied Model alias for data
 * 	@return The array index for the Allied Model
 **********************************************************************/
int							source_allied_data_location

	(
	FpaConfigSourceStruct	*sdef,
	int						type,
	STRING					alias
	)

	{
	int								nn;
	FpaConfigAlliedProgramsStruct	*programs;
	FpaConfigAlliedFilesStruct		*files;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigAlliedWindsStruct		*winds;
	FpaConfigAlliedValuesStruct		*values;
	FpaConfigAlliedMetafilesStruct	*metafiles;

	/* Return -1 if no source information */
	if ( IsNull(sdef) ) return -1;

	/* Search for alias based on type of Allied Model data          */
	/* Note that passing a blank alias will return the first member */
	/*  of the given type of Allied Model data!                     */
	switch (type)
		{

		/* Branch to Allied Model programs */
		case FpaC_ALLIED_PROGRAMS:
			programs = sdef->allied->programs;
			if ( IsNull(programs) || programs->nprogs < 1 ) break;
			if ( blank(alias) ) return 0;
			for ( nn=0; nn<programs->nprogs; nn++ )
				if ( same_ic(alias, programs->aliases[nn]) ) return nn;
			break;

		/* Branch to Allied Model files */
		case FpaC_ALLIED_FILES:
			files = sdef->allied->files;
			if ( IsNull(files) || files->nfiles < 1 ) break;
			if ( blank(alias) ) return 0;
			for ( nn=0; nn<files->nfiles; nn++ )
				if ( same_ic(alias, files->aliases[nn]) ) return nn;
			break;

		/* Branch to Allied Model required fields */
		case FpaC_ALLIED_FIELDS:
			fields = sdef->allied->fields;
			if ( IsNull(fields) || fields->nfields < 1 ) break;
			if ( blank(alias) ) return 0;
			for ( nn=0; nn<fields->nfields; nn++ )
				if ( same_ic(alias, fields->aliases[nn]) ) return nn;
			break;

		/* Branch to Allied Model required wind crossreferences          */
		/* Note that both the alias and crossreference name are checked! */
		case FpaC_ALLIED_WINDS:
			winds = sdef->allied->winds;
			if ( IsNull(winds) || winds->nwinds < 1 ) break;
			if ( blank(alias) ) return 0;
			for ( nn=0; nn<winds->nwinds; nn++ )
				if ( same_ic(alias, winds->aliases[nn])
						|| same_ic(alias, winds->wcrefs[nn]->name) ) return nn;
			break;

		/* Branch to Allied Model required value crossreferences         */
		/* Note that both the alias and crossreference name are checked! */
		case FpaC_ALLIED_VALUES:
			values = sdef->allied->values;
			if ( IsNull(values) || values->nvalues < 1 ) break;
			if ( blank(alias) ) return 0;
			for ( nn=0; nn<values->nvalues; nn++ )
				if ( same_ic(alias, values->aliases[nn])
						|| same_ic(alias, values->vcrefs[nn]->name) ) return nn;
			break;

		/* Branch to Allied Model metafiles */
		case FpaC_ALLIED_METAFILES:
			metafiles = sdef->allied->metafiles;
			if ( IsNull(metafiles) || metafiles->nfiles < 1 ) break;
			if ( blank(alias) ) return 0;
			for ( nn=0; nn<metafiles->nfiles; nn++ )
				if ( same_ic(alias, metafiles->aliases[nn]) ) return nn;
			break;

		/* Default for all other data types */
		default:
			break;
		}

	/* Return -1 if alias not found */
	return -1;
	}

/***********************************************************************
*                                                                      *
*   i d e n t i f y _ g r o u p                                        *
*   i d e n t i f y _ g r o u p s _ f o r _ f i e l d s                *
*   i d e n t i f y _ g r o u p s _ f o r _ f i e l d s _ f r e e      *
*   i d e n t i f y _ g r o u p s _ f o r _ e l e m e n t s            *
*   i d e n t i f y _ g r o u p s _ f o r _ e l e m e n t s _ f r e e  *
*                                                                      *
***********************************************************************/
/* Storage for Groups information */
static	LOGICAL					GroupsRead    = FALSE;
static	LOGICAL					GroupsValid   = FALSE;
static	int						NumGrpFldDef  = 0;
static	FpaConfigGroupStruct	**GrpFldDefs  = NullPtr(FpaConfigGroupStruct **);
static	int						NumGrpElemDef = 0;
static	FpaConfigGroupStruct	**GrpElemDefs = NullPtr(FpaConfigGroupStruct **);

/* Storage for Groups identifier information */
static	int						NumGrpFldIdent  = 0;
static	FPAC_IDENTS				*GrpFldIdents   = NullPtr(FPAC_IDENTS *);
static	int						NumGrpElemIdent = 0;
static	FPAC_IDENTS				*GrpElemIdents  = NullPtr(FPAC_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named field
 * or element group in the Groups blocks of the configuration files.
 *
 * Note that group name comparisons are case insensitive!
 *
 *	@param[in]	type		fields or elements type name
 *	@param[in]	name		group name for fields or elements
 *  @return Pointer to a named field structure or element group.
 **********************************************************************/
FpaConfigGroupStruct		*identify_group

	(
	STRING		type,
	STRING		name
	)

	{
	FpaConfigGroupStruct	*gdef;

	/* Ensure that configuration file has been read */
	if ( !read_groups_info() ) return NullPtr(FpaConfigGroupStruct *);

	/* Find group name for fields or elements                       */
	/*  ... and return Null pointer if group name not found in list */
	if      ( same(type, FpaCblockFields) )   gdef = find_field_group(name);
	else if ( same(type, FpaCblockElements) ) gdef = find_element_group(name);
	else
		{
		(void) pr_error("Config",
				"[identify_group] Unknown group type \"%s\"!\n", type);
		return NullPtr(FpaConfigGroupStruct *);
		}
	if ( IsNull(gdef) ) return NullPtr(FpaConfigGroupStruct *);

	/* Error message if group data for fields or elements is not OK */
	if ( !gdef->valid )
		{
		if ( same(type, FpaCblockFields) )
			(void) config_file_message(FpaCblockGFields,
					gdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		else if ( same(type, FpaCblockElements) )
			(void) config_file_message(FpaCblockGElements,
					gdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigGroupStruct *);
		}

	/* Return pointer to Group structure for fields or elements */
	return gdef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures of groups
 * for Fields in the Groups blocks of the configuration files.
 *
 *	@param[out]	***list	list of to groups for fields
 *	@return The size of the list.
 **********************************************************************/
int						identify_groups_for_fields

	(
	FpaConfigGroupStruct	***list
	)

	{
	int						ngdefs, nn;
	FpaConfigGroupStruct	*gdef;
	FpaConfigGroupStruct	**gdeflist = NullPtr(FpaConfigGroupStruct **);

	/* Initialize list of pointers to groups for fields */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigGroupStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_groups_info() ) return 0;

	/* Add to list of pointers to groups for fields  */
	/*  (except for special case of "Not_Displayed") */
	/*  ... but only if list is returned!            */
	for ( ngdefs=0, nn=0; nn<NumGrpFldDef; nn++ )
		{
		gdef = GrpFldDefs[nn];
		if ( same(gdef->name, FpaCnotDisplayed) ) continue;
		ngdefs++;
		if ( NotNull(list) )
			{
			gdeflist = GETMEM(gdeflist, FpaConfigGroupStruct *, ngdefs);
			gdeflist[ngdefs-1] = gdef;
			}
		}

	/* Return the list of pointers to groups for fields */
	if ( NotNull(list) ) *list = gdeflist;
	return ngdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures of groups
 * for Fields in the Groups blocks of the configuration files.
 *
 *	@param[in]	***list		list of Group Structures
 *	@param[in]	num			number of pointers in list
 * 	@return The size of the list (0).
 **********************************************************************/
int						identify_groups_for_fields_free

	(
	FpaConfigGroupStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures of groups
 * for Elements in the Groups blocks of the configuration files.
 *
 *	@param[out]	***list	list of groups for elements
 *  @return The size of the list.
 **********************************************************************/
int						identify_groups_for_elements

	(
	FpaConfigGroupStruct	***list
	)

	{
	int						ngdefs, nn;
	FpaConfigGroupStruct	*gdef;
	FpaConfigGroupStruct	**gdeflist = NullPtr(FpaConfigGroupStruct **);

	/* Initialize list of pointers to groups for elements */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigGroupStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_groups_info() ) return 0;

	/* Add to list of pointers to groups for elements */
	/*  (except for special cases of "Not_Displayed"  */
	/*  and "Generic_Equation")                       */
	/*  ... but only if list is returned!             */
	for ( ngdefs=0, nn=0; nn<NumGrpElemDef; nn++ )
		{
		gdef = GrpElemDefs[nn];
		if ( same(gdef->name, FpaCnotDisplayed) ) continue;
		if ( same(gdef->name, FpaCgenericEqtn) )  continue;
		ngdefs++;
		if ( NotNull(list) )
			{
			gdeflist = GETMEM(gdeflist, FpaConfigGroupStruct *, ngdefs);
			gdeflist[ngdefs-1] = gdef;
			}
		}

	/* Return the list of pointers to groups for elements */
	if ( NotNull(list) ) *list = gdeflist;
	return ngdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures of groups
 * for Elements in the Groups blocks of the configuration files.
 *
 *	@param[in]	***list 	list of Group Structures
 *	@param[in]	num			number of pointers in list
 *  @return The size of the list (0).
 **********************************************************************/
int						identify_groups_for_elements_free

	(
	FpaConfigGroupStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/***********************************************************************
*                                                                      *
*   i d e n t i f y _ l e v e l                                        *
*   i d e n t i f y _ l e v e l _ f r o m _ l e v e l s                *
*   i d e n t i f y _ l e v e l s _ b y _ t y p e                      *
*   i d e n t i f y _ l e v e l s _ b y _ t y p e _ f r e e            *
*   i d e n t i f y _ l e v e l _ a l i a s e s                        *
*   i d e n t i f y _ l e v e l _ a l i a s e s _ f r e e              *
*   e q u i v a l e n t _ l e v e l _ d e f i n i t i o n s            *
*                                                                      *
***********************************************************************/

/* Storage for Levels information */
static	LOGICAL					LevelsRead  = FALSE;
static	LOGICAL					LevelsValid = FALSE;
static	int						NumLevelDef = 0;
static	FpaConfigLevelStruct	**LevelDefs = NullPtr(FpaConfigLevelStruct **);

/* Storage for Levels identifier information */
static	int						NumLevelIdent = 0;
static	FPAC_IDENTS				*LevelIdents  = NullPtr(FPAC_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named level
 * in the Levels blocks of the configuration files.
 *
 * Note that level name comparisons are case insensitive!
 *
 *	@param[in]	name		level name
 *  @return Named level pointer.
 **********************************************************************/
FpaConfigLevelStruct		*identify_level

	(
	STRING		name
	)

	{
	FpaConfigLevelStruct	*ldef;

	/* Ensure that configuration file has been read */
	if ( !read_levels_info() ) return NullPtr(FpaConfigLevelStruct *);

	/* Return Null pointer if level name not found in list */
	ldef = find_level(name);
	if ( IsNull(ldef) ) return NullPtr(FpaConfigLevelStruct *);

	/* Error message if level data is not OK */
	if ( !ldef->valid )
		{
		(void) config_file_message(FpaCblockLevels,
				ldef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigLevelStruct *);
		}

	/* Special one time file identifier check for levels */
	if ( !ldef->lev_io->check_fident )
		{
		(void) pr_diag("Config", "One time file identifier check for: \"%s\"\n",
				ldef->name);
		ldef->lev_io->check_fident = TRUE;

		/* Warning if only old format file identifier is present */
		if ( blank(ldef->lev_io->fident) && !blank(ldef->lev_io->fid) )
			{
			(void) pr_warning("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockLevels, ldef->name);
			(void) pr_warning("Config", "     Have fid \"%s\" but missing fident!\n",
					ldef->lev_io->fid);
			}
		}

	/* Return pointer to Level structure */
	return ldef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a pointer to a structure for a level in the
 * Levels blocks of the configuration files based on a single level
 * or upper and lower levels.
 *
 * Note that level comparisons are case insensitive!
 *
 *	@param[in]	type		enumerated level type
 *	@param[in]	single		single level
 *	@param[in]	upper		upper level
 *	@param[in]	lower		lower level
 * 	@return Level structure for given level.
 **********************************************************************/
FpaConfigLevelStruct		*identify_level_from_levels

	(
	int			type,
	STRING		single,
	STRING		upper,
	STRING		lower
	)

	{
	int						num, nn;
	FpaConfigLevelStruct	**ldefs;
	FpaConfigLevelStruct	*ldef, *ldefr = NullPtr(FpaConfigLevelStruct *);

	/* Ensure that configuration file has been read */
	if ( !read_levels_info() ) return NullPtr(FpaConfigLevelStruct *);

	/* Determine number of levels based on enumerated level type */
	num = identify_levels_by_type(type, &ldefs);

	/* Check for level name based on level type          */
	/* Note that level comparisons are case insensitive! */
	switch ( type )
		{

		/* Level type FpaC_MSL */
		case FpaC_MSL:
			/* Set pointer if matching level found */
			for ( nn=0; nn<num; nn++ )
				{
				ldef = identify_level(ldefs[nn]->name);
				if ( same_ic(single, ldef->lev_lvls->lvl) )
					{
					ldefr = ldef;
					break;
					}
				}
			break;

		/* Level type FpaC_SURFACE or FpaC_LEVEL */
		case FpaC_SURFACE:
		case FpaC_LEVEL:
			/* Set pointer if matching level found */
			for ( nn=0; nn<num; nn++ )
				{
				ldef = identify_level(ldefs[nn]->name);
				if ( same_ic(single, ldef->lev_lvls->lvl) )
					{
					ldefr = ldef;
					break;
					}
				}
			break;

		/* Level type FpaC_LAYER */
		case FpaC_LAYER:
			/* Set pointer if matching level found */
			for ( nn=0; nn<num; nn++ )
				{
				ldef = identify_level(ldefs[nn]->name);
				if ( same_ic(upper, ldef->lev_lvls->uprlvl)
						&& same_ic(lower, ldef->lev_lvls->lwrlvl) )
					{
					ldefr = ldef;
					break;
					}
				}
			break;

		/* Level type FpaC_GEOGRAPHY */
		case FpaC_GEOGRAPHY:
			/* Set pointer if matching level found */
			for ( nn=0; nn<num; nn++ )
				{
				ldef = identify_level(ldefs[nn]->name);
				if ( same_ic(single, ldef->lev_lvls->lvl) )
					{
					ldefr = ldef;
					break;
					}
				}
			break;

		/* Level type FpaC_ANNOTATION */
		case FpaC_ANNOTATION:
			/* Set pointer if matching level found */
			for ( nn=0; nn<num; nn++ )
				{
				ldef = identify_level(ldefs[nn]->name);
				if ( same_ic(single, ldef->lev_lvls->lvl) )
					{
					ldefr = ldef;
					break;
					}
				}
			break;

		/* Level type FpaC_LVL_ANY (Special case!) */
		case FpaC_LVL_ANY:
			ldefr = identify_level(FpaCanyLevel);
			break;

		/* Do not reset pointer for level type FpaC_LVL_NOTUSED */
		/*  or for unknown level types                          */
		default:
			break;
		}

	/* Return pointer for matching level */
	num = identify_levels_by_type_free(&ldefs, num);
	return ldefr;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for a given
 * level in the Levels blocks of the configuration files.
 *
 *	@param[in]	ltype	enumerated level type
 *	@param[out]	***list	list of levels with same level type
 * @return The size of the list.
 **********************************************************************/
int						identify_levels_by_type

	(
	int						ltype,
	FpaConfigLevelStruct	***list
	)

	{
	int						nldefs, nn;
	FpaConfigLevelStruct	*ldef;
	FpaConfigLevelStruct	**ldeflist = NullPtr(FpaConfigLevelStruct **);

	/* Initialize list of pointers to levels with same level type */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigLevelStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_levels_info() ) return 0;

	/* Add to list of pointers to levels with same level type            */
	/*  ... but only if list is returned!                                */
	/* Note that level type FpaC_LVL_ANY returns all levels!             */
	/* Note that level type FpaC_SURFACE is also a level for FpaC_LEVEL! */
	for ( nldefs=0, nn=0; nn<NumLevelDef; nn++ )
		{
		ldef = LevelDefs[nn];
		if ( ltype == FpaC_LVL_ANY || ltype == ldef->lvl_type
				|| (ltype == FpaC_LEVEL && ldef->lvl_type == FpaC_SURFACE) )
			{

			/* Add to list of pointers to levels with same level type */
			nldefs++;
			if ( NotNull(list) )
				{
				ldeflist = GETMEM(ldeflist, FpaConfigLevelStruct *, nldefs);
				ldeflist[nldefs-1] = ldef;
				}
			}
		}

	/* Return the list of pointers to levels with same level type */
	if ( NotNull(list) ) *list = ldeflist;
	return nldefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures for a given
 * type of level in the Levels blocks of the configuration files.
 *
 *	@param[in]	***list		list of Level Structures
 *	@param[in]	num			size of list
 *  @return The size of the list (0).
 **********************************************************************/
int						identify_levels_by_type_free

	(
	FpaConfigLevelStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to aliases for a given
 * level in the Levels blocks of the configuration files.
 *
 *	@param[in]	*ldef	Level structure
 *	@param[in]	**list	list of level aliases
 *  @return The size of the list.
 **********************************************************************/
int						identify_level_aliases

	(
	FpaConfigLevelStruct	*ldef,
	STRING					**list
	)

	{
	int			naliases, nn;
	STRING		*aliaslist = NullStringList;

	/* Initialize list of pointers to level aliases */
	if ( NotNull(list) ) *list = NullStringList;

	/* Ensure that configuration file has been read */
	if ( !read_levels_info() ) return 0;

	/* Check for levels that are aliases */
	for ( naliases=0, nn=0; nn<NumLevelIdent; nn++ )
		{
		if ( ldef == (FpaConfigLevelStruct *) LevelIdents[nn].pdef )
			{

			/* Add to list of aliases             */
			/*  ... but only if list is returned! */
			naliases++;
			if ( NotNull(list) )
				{
				aliaslist = GETMEM(aliaslist, STRING, naliases);
				aliaslist[naliases-1] = LevelIdents[nn].ident;
				}
			}
		}

	/* Return the list of aliases of the level name */
	if ( NotNull(list) ) *list = aliaslist;
	return naliases;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to aliases for a given
 * level in the Levels blocks of the configuration files.
 *
 *	@param[in]	**list		list of Level aliases
 *	@param[in]	num			size of list
 *  @return The size of the list.
 **********************************************************************/
int						identify_level_aliases_free

	(
	STRING		**list,
	int			num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function checks if two level names are equivalent.
 *
 *	@param[in]	name1		first level name to compare
 *	@param[in]	name2		second level name to compare
 * 	@return True if level names are equivalent.
 **********************************************************************/
LOGICAL					equivalent_level_definitions

	(
	STRING		name1,
	STRING		name2
	)

	{
	FpaConfigLevelStruct	*ldef1, *ldef2;

	/* Static buffer for FpaCanyLevel level */
	static	FpaConfigLevelStruct	*AnyLevel = NullPtr(FpaConfigLevelStruct *);

	/* Get pointer for special level FpaCanyLevel */
	if ( IsNull(AnyLevel) )
		{
		AnyLevel = identify_level(FpaCanyLevel);
		}

	/* Get pointers for level names to compare */
	ldef1 = identify_level(name1);
	ldef2 = identify_level(name2);

	/* Return FALSE if problems with either level name */
	if ( IsNull(ldef1) || IsNull(ldef2) ) return FALSE;

	/* Return TRUE if level names match */
	if ( ldef1 == ldef2 ) return TRUE;

	/* Return FALSE if special level FpaCanyLevel not found */
	if ( IsNull(AnyLevel) || !AnyLevel->valid )
		{
		if ( IsNull(AnyLevel) )
			(void) config_file_message(FpaCblockLevels,
					FpaCanyLevel, FpaCblank, FpaCblank, FpaCmsgMissName);
		else if ( !AnyLevel->valid )
			(void) config_file_message(FpaCblockLevels,
					AnyLevel->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return FALSE;
		}

	/* Return TRUE if either level name is special level FpaCanyLevel */
	if ( (ldef1 == AnyLevel) || (ldef2 == AnyLevel) ) return TRUE;

	/* Return FALSE if level names do not match */
	return FALSE;
	}


/***********************************************************************
*                                                                      *
*   i d e n t i f y _ e l e m e n t                                    *
*   g e t _ e l e m e n t _ i n f o                                    *
*   i d e n t i f y _ e l e m e n t s _ b y _ g r o u p                *
*   i d e n t i f y _ e l e m e n t s _ b y _ g r o u p _ f r e e      *
*   i d e n t i f y _ e l e m e n t _ a l i a s e s                    *
*   i d e n t i f y _ e l e m e n t _ a l i a s e s _ f r e e          *
*   i d e n t i f y _ l i n e _ t y p e _ b y _ n a m e                *
*   i d e n t i f y _ s c a t t e r e d _ t y p e _ b y _ n a m e      *
*   i d e n t i f y _ l a b e l l i n g _ t y p e _ b y _ n a m e      *
*   e q u i v a l e n t _ e l e m e n t _ d e f i n i t i o n s        *
*                                                                      *
***********************************************************************/

/* Storage for Elements information */
static	LOGICAL					ElementsRead  = FALSE;
static	LOGICAL					ElementsValid = FALSE;
static	int						NumElementDef = 0;
static	FpaConfigElementStruct	**ElementDefs = NullPtr(FpaConfigElementStruct **);

/* Storage for Elements identifier information */
static	int						NumElementIdent = 0;
static	FPAC_IDENTS				*ElementIdents  = NullPtr(FPAC_IDENTS *);

/**********************************************************************/

/************************************************************************/
/** This function returns a pointer to a structure for a named
 * element in the Elements blocks of the configuration files.
 *
 * Note that  get_element_info()  returns detailed element
 * information.
 *
 * Note that element name comparisons are case insensitive!
 *
 *	@param[in]	name		element name
 * 	@return Named element structure.
 **********************************************************************/
FpaConfigElementStruct		*identify_element

	(
	STRING		name
	)

	{
	FpaConfigElementStruct	*edef;

	/* Ensure that configuration file has been read */
	if ( !read_elements_info() ) return NullPtr(FpaConfigElementStruct *);

	/* Return Null pointer if element name not found in list */
	edef = find_element(name);
	if ( IsNull(edef) ) return NullPtr(FpaConfigElementStruct *);

	/* Error message if element data is not OK */
	if ( !edef->valid )
		{
		(void) config_file_message(FpaCblockElements,
				edef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigElementStruct *);
		}

	/* Special one time file identifier check for elements */
	if ( !edef->elem_io->check_fident )
		{
		(void) pr_diag("Config", "One time file identifier check for: \"%s\"\n",
				edef->name);
		edef->elem_io->check_fident = TRUE;

		/* Warning if only old format file identifier is present */
		if ( blank(edef->elem_io->fident) && !blank(edef->elem_io->fid) )
			{
			(void) pr_warning("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockElements, edef->name);
			(void) pr_warning("Config", "     Have fid \"%s\" but missing fident!\n",
					edef->elem_io->fid);
			}
		}

	/* Return pointer to Element structure */
	return edef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a pointer to a structure for a named
 * in the Elements blocks of the configuration files.
 * that element name comparisons are case insensitive!
 *
 *	@param[in]	name		element name
 *  @return detailed element information in a named element structure.
 **********************************************************************/
FpaConfigElementStruct		*get_element_info

	(
	STRING		name
	)

	{
	int									fldtype, nn, natt, nsub, nns, nmerge;
	STRING								wmod, wdir, wspd, wgst;
	STRING								xmod, xdir, xspd, xgst;
	STRING								xcat, xval, xlab;
	char								linkbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigElementStruct				*edef;
	FpaConfigElementIOStruct			*eio, *meio;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementAttribStruct		*attrib;
	FpaConfigElementEditorStruct		*editor;
	FpaConfigElementLabellingStruct		*labelling;
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigElementLinkingStruct		*linking;
	FpaConfigElementStruct				**melems;
	FpaConfigElementStruct				*linkedef;
	FpaConfigLevelStruct				*linkldef;

	/* Get pointer to structure with detailed information added */
	edef = read_element_detailed_info(name);
	if ( IsNull(edef) || !edef->valid_detail )
			return NullPtr(FpaConfigElementStruct *);

	/* Set field type */
	fldtype  = edef->fld_type;

	/* Set pointers to detailed element information */
	attrib    = edef->elem_detail->attributes;
	editor    = edef->elem_detail->editor;
	stypes    = edef->elem_detail->scattered_types;
	labelling = edef->elem_detail->labelling;
	sampling  = edef->elem_detail->sampling;
	linking   = edef->elem_detail->linking;

	/* >>> Add new special checks for appropriate elements <<< */

	/* Special one time sampling check for elements */
	if ( NotNull(sampling) && !sampling->check_sampling )
		{
		(void) pr_diag("Config", "One time sampling check for: \"%s\"\n",
				edef->name);
		sampling->check_sampling = TRUE;

		/* Check for sampling attributes for discrete fields */
		if ( fldtype == FpaC_DISCRETE )
			{
			for ( nn=0; nn<sampling->type.discrete->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.discrete->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, FpaCsampling,
							sampling->type.discrete->sattrib_names[nn],
							FpaCmsgMember);
					edef->valid_detail = FALSE;
					}
				}
			}

		/* Check for sampling attributes for line fields */
		else if ( fldtype == FpaC_LINE )
			{
			for ( nn=0; nn<sampling->type.line->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.line->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, FpaCsampling,
							sampling->type.line->sattrib_names[nn],
							FpaCmsgMember);
					edef->valid_detail = FALSE;
					}
				}
			}

		/* Check for sampling attributes for scattered fields */
		else if ( fldtype == FpaC_SCATTERED )
			{
			for ( nn=0; nn<sampling->type.scattered->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.scattered->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, FpaCsampling,
							sampling->type.scattered->sattrib_names[nn],
							FpaCmsgMember);
					edef->valid_detail = FALSE;
					}
				}
			}

		/* Check for sampling attributes for link chain fields */
		if ( fldtype == FpaC_LCHAIN )
			{
			for ( nn=0; nn<sampling->type.lchain->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.lchain->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, FpaCsampling,
							sampling->type.lchain->sattrib_names[nn],
							FpaCmsgMember);
					edef->valid_detail = FALSE;
					}
				}
			}
		}

	/* Special one time editor check for elements */
	if ( NotNull(editor) && !editor->check_editor )
		{
		(void) pr_diag("Config", "One time editor check for: \"%s\"\n",
				edef->name);
		editor->check_editor = TRUE;

		/* Check for editor parameters for continuous fields */
		if ( fldtype == FpaC_CONTINUOUS )
			{
			if ( editor->type.continuous->poke == 0.0
					|| IsNull(editor->type.continuous->units) )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCeditor, FpaCmsgInvalid);
				edef->valid_detail = FALSE;
				}
			}

		/* Check for editor parameters for vector fields */
		else if ( fldtype == FpaC_VECTOR )
			{
			if ( editor->type.vector->mag_poke == 0.0
					|| IsNull(editor->type.vector->mag_units)
					|| editor->type.vector->dir_poke == 0.0
					|| IsNull(editor->type.vector->dir_units) )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCeditor, FpaCmsgInvalid);
				edef->valid_detail = FALSE;
				}
			}

		/* Check for attributes for discrete fields */
		else if ( fldtype == FpaC_DISCRETE )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				edef->valid_detail = FALSE;
				}
			}

		/* Check for attributes and wind cross references for wind fields */
		else if ( fldtype == FpaC_WIND )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				edef->valid_detail = FALSE;
				}

			/* Check consistency of values for default wind attributes */
			else
				{

				/* Extract values for default wind attributes */
				wmod = wdir = wspd = wgst = NullString;
				for ( nn=0; nn<attrib->nattribs; nn++ )
					{
					if ( same_ic(attrib->attrib_names[nn],
														AttribWindModel) )
						{
						wmod = attrib->attrib_back_defs[nn];
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindDirection) )
						{
						wdir = attrib->attrib_back_defs[nn];
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindSpeed) )
						{
						wspd = attrib->attrib_back_defs[nn];
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindGust) )
						{
						wgst = attrib->attrib_back_defs[nn];
						}
					}

				/* Check consistency of default wind attribute values */
				if ( !consistent_wind_attrib_strings(wmod, wdir, wspd, wgst,
						&xmod, &xdir, &xspd, &xgst, &xcat, &xval, &xlab) )
					{
					(void) pr_error("Config",
							"Block: \"%s\"    Name: \"%s\"\n",
							FpaCblockElements, edef->name);
					(void) pr_error("Config",
							"     Inconsistent wind attributes in config files!\n");
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindModel, wmod);
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindDirection, wdir);
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindSpeed, wspd);
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindGust, wgst);
					edef->valid_detail = FALSE;
					}

				/* Reset values for default wind attributes */
				for ( nn=0; nn<attrib->nattribs; nn++ )
					{
					if ( same_ic(attrib->attrib_names[nn],
														AttribWindModel) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xmod);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindDirection) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xdir);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindSpeed) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xspd);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindGust) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xgst);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribCategory) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xcat);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribAutolabel) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xval);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribUserlabel) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xlab);
						}
					}
				}
			}

		/* Check for attributes for line fields */
		else if ( fldtype == FpaC_LINE )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				edef->valid_detail = FALSE;
				}
			}

		/* Check for attributes for scattered fields */
		else if ( fldtype == FpaC_SCATTERED )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				edef->valid_detail = FALSE;
				}
			}

		/* Check for attributes for link chain fields */
		else if ( fldtype == FpaC_LCHAIN )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				edef->valid_detail = FALSE;
				}
			}

		/* Set default modify file for continuous fields */
		if ( fldtype == FpaC_CONTINUOUS )
			{
			if ( IsNull(editor->type.continuous->modify_file)
					&& NotNull(editor->type.continuous->entry_file) )
				{
				editor->type.continuous->modify_file =
					strdup(editor->type.continuous->entry_file);
				}
			}

		/* Set default modify file for vector fields */
		else if ( fldtype == FpaC_VECTOR )
			{
			if ( IsNull(editor->type.vector->modify_file)
					&& NotNull(editor->type.vector->entry_file) )
				{
				editor->type.vector->modify_file =
					strdup(editor->type.vector->entry_file);
				}
			}

		/* Set default modify file for discrete fields */
		else if ( fldtype == FpaC_DISCRETE )
			{
			if ( IsNull(editor->type.discrete->modify_file)
					&& NotNull(editor->type.discrete->entry_file) )
				{
				editor->type.discrete->modify_file =
					strdup(editor->type.discrete->entry_file);
				}
			}

		/* Set default modify file for wind fields */
		else if ( fldtype == FpaC_WIND )
			{
			if ( IsNull(editor->type.wind->modify_file)
					&& NotNull(editor->type.wind->entry_file) )
				{
				editor->type.wind->modify_file =
					strdup(editor->type.wind->entry_file);
				}
			}

		/* Set default modify file for line fields */
		else if ( fldtype == FpaC_LINE )
			{
			if ( IsNull(editor->type.line->modify_file)
					&& NotNull(editor->type.line->entry_file) )
				{
				editor->type.line->modify_file =
					strdup(editor->type.line->entry_file);
				}
			}

		/* Set default modify file for link chain fields */
		else if ( fldtype == FpaC_LCHAIN )
			{
			if ( IsNull(editor->type.lchain->modify_file)
					&& NotNull(editor->type.lchain->entry_file) )
				{
				editor->type.lchain->modify_file =
					strdup(editor->type.lchain->entry_file);
				}
			} 

		/* Set default node modify file for link chain fields */
		if ( fldtype == FpaC_LCHAIN )
			{
			if ( IsNull(editor->type.lchain->node_modify_file)
					&& NotNull(editor->type.lchain->node_entry_file) )
				{
				editor->type.lchain->node_modify_file =
					strdup(editor->type.lchain->node_entry_file);
				}
			} 

		/* Set merge fields based on field type */
		nmerge = 0;
		if ( fldtype == FpaC_CONTINUOUS )
			{
			nmerge  = editor->type.continuous->nmerge;
			melems  = editor->type.continuous->merge_elems;
			}
		else if ( fldtype == FpaC_VECTOR )
			{
			nmerge  = editor->type.vector->nmerge;
			melems  = editor->type.vector->merge_elems;
			}
		else if ( fldtype == FpaC_DISCRETE )
			{
			nmerge  = editor->type.discrete->nmerge;
			melems  = editor->type.discrete->merge_elems;
			}
		else if ( fldtype == FpaC_WIND )
			{
			nmerge  = editor->type.wind->nmerge;
			melems  = editor->type.wind->merge_elems;
			}
		else if ( fldtype == FpaC_LINE )
			{
			nmerge  = editor->type.line->nmerge;
			melems  = editor->type.line->merge_elems;
			}
		else if ( fldtype == FpaC_SCATTERED )
			{
			nmerge  = editor->type.scattered->nmerge;
			melems  = editor->type.scattered->merge_elems;
			}
		else if ( fldtype == FpaC_LCHAIN )
			{
			nmerge  = editor->type.lchain->nmerge;
			melems  = editor->type.lchain->merge_elems;
			}

		/* Check merge fields for matching field type and correct units */
		if ( nmerge > 0 )
			{
			eio = edef->elem_io;
			for ( nn=0; nn<nmerge; nn++ )
				{

				/* Check for matching field type */
				if ( fldtype != melems[nn]->fld_type )
					{
					(void) config_file_message(FpaCblockElements, edef->name,
							melems[nn]->name, FpaCeditor, FpaCmsgMergeType);
					edef->valid_detail = FALSE;
					}

				/* Check for matching units */
				meio = melems[nn]->elem_io;
				if ( IsNull(eio) )         continue;
				if ( IsNull(eio->units) )  continue;
				if ( IsNull(meio) )        continue;
				if ( IsNull(meio->units) ) continue;
				if ( !convert_value(meio->units->name, 0.0,
						eio->units->name, NullDouble) )
					{
					(void) config_file_message(FpaCblockElements, edef->name,
							melems[nn]->name, FpaCeditor, FpaCmsgMergeUnits);
					edef->valid_detail = FALSE;
					}
				}
			}

		/* Note that default modify file for scattered fields is  */
		/*  accessed through ElementScatteredType structure below */
		}

	/* Add default scattered types section (if required) */
	if ( fldtype == FpaC_SCATTERED )
		{

		/* Initialize the ElementScatteredType structure (if required) */
		if ( IsNull(stypes) )
			{
			edef->elem_detail->scattered_types = init_element_scattered_types();
			stypes = edef->elem_detail->scattered_types;
			}

		/* Add a default scattered type to the list (if required) */
		if ( stypes->ntypes < 1 )
			{

			/* Add a default scattered type */
			nsub = add_element_scattered_type(AttribScatteredTypeDefault,
					stypes);
			if ( nsub < 0 )
				{
				(void) config_file_message(FpaCblockElements, edef->name,
						FpaCscatteredTypes, FpaCdefault, FpaCmsgMember);
				edef->valid_detail = FALSE;
				}
			}
		}

	/* Special one time check for scattered types */
	if ( fldtype == FpaC_SCATTERED && !stypes->check_scattered )
		{
		(void) pr_diag("Config", "One time scattered types check for: \"%s\"\n",
				edef->name);
		stypes->check_scattered = TRUE;

		/* Set default entry file for scattered fields */
		for ( nn=0; nn<stypes->ntypes; nn++ )
			{
			if ( IsNull(stypes->type_entry_files[nn]) )
				{
				if ( NotNull(editor)
						&& NotNull(editor->type.scattered->entry_file) )
					{
					stypes->type_entry_files[nn] =
						strdup(editor->type.scattered->entry_file);
					}
				}
			}

		/* Set default modify file for scattered fields */
		for ( nn=0; nn<stypes->ntypes; nn++ )
			{
			if ( IsNull(stypes->type_modify_files[nn]) )
				{
				if ( NotNull(editor)
						&& NotNull(editor->type.scattered->modify_file) )
					{
					stypes->type_modify_files[nn] =
						strdup(editor->type.scattered->modify_file);
					}
				else if ( NotNull(stypes->type_entry_files[nn]) )
					{
					stypes->type_modify_files[nn] =
						strdup(stypes->type_entry_files[nn]);
					}
				}
			}

		/* Set default entry rules for scattered fields */
		for ( nn=0; nn<stypes->ntypes; nn++ )
			{
			if ( stypes->type_rules[nn].nrules <= 0 )
				{
				if ( NotNull(editor)
						&& editor->type.scattered->nrules > 0 )
					{
					nsub = editor->type.scattered->nrules;
					stypes->type_rules[nn].nrules      = nsub;
					stypes->type_rules[nn].entry_rules = INITMEM(STRING, nsub);
					stypes->type_rules[nn].entry_funcs = INITMEM(ERULE,  nsub);
					for ( nns=0; nns<nsub; nns++ )
						{
						stypes->type_rules[nn].entry_rules[nns] =
							strdup(SafeStr(editor->type.scattered->entry_rules[nns]));
						stypes->type_rules[nn].entry_funcs[nns] =
							editor->type.scattered->entry_funcs[nns];
						}
					}
				}
			}
		}

	/* Special one time labelling check for elements */
	if ( NotNull(labelling) && !labelling->check_labelling )
		{
		(void) pr_diag("Config", "One time labelling check for: \"%s\"\n",
				edef->name);
		labelling->check_labelling = TRUE;

		/* Set default modify file for labelling types */
		for ( nn=0; nn<labelling->ntypes; nn++ )
			{
			if ( IsNull(labelling->type_modify_files[nn])
					&& NotNull(labelling->type_entry_files[nn]) )
				{
				labelling->type_modify_files[nn] =
					strdup(labelling->type_entry_files[nn]);
				}
			}
		}

	/* Special one time linking check for elements */
	if ( NotNull(linking) && !linking->check_linking )
		{
		(void) pr_diag("Config", "One time linking check for: \"%s\"\n",
				edef->name);
		linking->check_linking = TRUE;

		/* Check for consistent element and level for linking fields */
		for ( nn=0; nn<linking->nlink; nn++ )
			{
			linkedef = linking->link_elems[nn];
			linkldef = linking->link_levels[nn];
			if ( NotNull(linkedef) && NotNull(linkldef)
					&& !consistent_element_and_level(linkedef, linkldef) )
				{
				(void) strcpy(linkbuf, linkedef->name);
				(void) strcat(linkbuf, " ");
				(void) strcat(linkbuf, linkldef->name);
				(void) config_file_message(FpaCblockElements, edef->name,
						linkbuf, FpaCdefault, FpaCmsgLinkFld);
				edef->valid_detail = FALSE;
				}
			}
		}

	/* Return pointer based on detailed information OK */
	return ( edef->valid_detail ) ? edef: NullPtr(FpaConfigElementStruct *);
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for a given
 * group of elements in the Elements blocks of the configuration files.
 *
 *	@param[in]	egroup	element group name
 *	@param[out]	***list	list of elements in the same element group
 *  @return The size of the list.
 **********************************************************************/
int						identify_elements_by_group

	(
	STRING					egroup,
	FpaConfigElementStruct	***list
	)

	{
	int						nedefs, nn;
	FpaConfigElementStruct	*edef;
	FpaConfigGroupStruct	*gdef;
	FpaConfigElementStruct	**edeflist = NullPtr(FpaConfigElementStruct **);

	/* Initialize list of pointers to elements in the same element group */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigElementStruct **);

	/* Checking for all elements */
	if ( same(egroup, FpaCanyGroup) )
		{
		gdef = NullPtr(FpaConfigGroupStruct *);
		}

	/* Get pointer to element group */
	else
		{
		gdef = identify_group(FpaCblockElements, egroup);
		if ( IsNull(gdef) ) return 0;
		}

	/* Ensure that configuration file has been read */
	if ( !read_elements_info() ) return 0;

	/* Add to list of pointers to elements with same element group */
	/*  ... but only if list is returned!                          */
	for ( nedefs=0, nn=0; nn<NumElementDef; nn++ )
		{
		edef = ElementDefs[nn];
		if ( same(egroup, FpaCanyGroup) || gdef == edef->group )
			{

			/* Add to list of pointers to elements in the same element group */
			nedefs++;
			if ( NotNull(list) )
				{
				edeflist = GETMEM(edeflist, FpaConfigElementStruct *, nedefs);
				edeflist[nedefs-1] = edef;
				}
			}
		}

	/* Return the list of pointers to elements in the same element group */
	if ( NotNull(list) ) *list = edeflist;
	return nedefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures for a given
 * group of elements in the Elements blocks of the configuration files.
 *
 *  @param[in]	***list		list of Element Structures
 *  @param[in]	num			size of list
 *  @return The size of the list.
 **********************************************************************/
int						identify_elements_by_group_free

	(
	FpaConfigElementStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to aliases for a given
 * element in the Elements blocks of the configuration files.
 *
 *	@param[in]	*edef	Element structure
 *	@param[out]	**list	List of element aliases
 *  @return The size of the list.
 **********************************************************************/
int						identify_element_aliases

	(
	FpaConfigElementStruct	*edef,
	STRING					**list
	)

	{
	int			naliases, nn;
	STRING		*aliaslist = NullStringList;

	/* Initialize list of pointers to element aliases */
	if ( NotNull(list) ) *list = NullStringList;

	/* Ensure that configuration file has been read */
	if ( !read_elements_info() ) return 0;

	/* Check for elements that are aliases */
	for ( naliases=0, nn=0; nn<NumElementIdent; nn++ )
		{
		if ( edef == (FpaConfigElementStruct *) ElementIdents[nn].pdef )
			{

			/* Add to list of aliases             */
			/*  ... but only if list is returned! */
			naliases++;
			if ( NotNull(list) )
				{
				aliaslist = GETMEM(aliaslist, STRING, naliases);
				aliaslist[naliases-1] = ElementIdents[nn].ident;
				}
			}
		}

	/* Return the list of aliases of the element name */
	if ( NotNull(list) ) *list = aliaslist;
	return naliases;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to aliases for a given
 * element in the Elements blocks of the configuration files.
 *
 *  @param[in]	**list		List of Element aliases
 *  @param[in]	num			size of list
 *  @return The size of the list (0).
 **********************************************************************/
int						identify_element_aliases_free

	(
	STRING		**list,
	int			num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns information for a given element from the
 * FpaConfigElementLineTypeStruct structure.
 *
 *	@param[in]	element	element name
 *	@param[in]	level	level name
 *	@param[in]	type	line type name
 *	@param[out]	**ltype ElementLineType structure
 *  @return The index of the information in the type_names list of the
 *  		structure.
 **********************************************************************/
int						identify_line_type_by_name

	(
	STRING		element,
	STRING		level,
	STRING		type,
	FpaConfigElementLineTypeStruct	**ltype
	)

	{
	int								ii;
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementStruct			*edef;
	FpaConfigElementLineTypeStruct	*ldef;

	static STRING	LastElement = NullString;
	static STRING	LastLevel   = NullString;
	static STRING	LastType    = NullString;
	static int		LastIdent   = -1;
	static FpaConfigElementLineTypeStruct
					*LastLType  = NullPtr(FpaConfigElementLineTypeStruct *);

	if ( NotNull(ltype) ) *ltype = NullPtr(FpaConfigElementLineTypeStruct *);

	/* Return the saved parameters if nothing has changed */
	if ( same(element, LastElement) && same(level, LastLevel)
			&& same(type, LastType) )
		{
		if ( NotNull(ltype) ) *ltype = LastLType;
		return LastIdent;
		}

	/* Initialize saved parameters */
	FREEMEM(LastElement);
	FREEMEM(LastLevel);
	FREEMEM(LastType);
	LastIdent = -1;
	LastLType = NullPtr(FpaConfigElementLineTypeStruct *);

	/* Save the values for the next call */
	LastElement = strdup(SafeStr(element));
	LastLevel   = strdup(SafeStr(level));
	LastType    = strdup(SafeStr(type));

	/* Get the detailed element information */
	fdef = get_field_info(element, level);
	edef = (NotNull(fdef))? fdef->element: get_element_info(element);
	if ( IsNull(edef) )
		{
		if ( NotNull(ltype) ) *ltype = LastLType;
		return LastIdent;
		}

	/* Get the ElementLineType structure */
	ldef = edef->elem_detail->line_types;
	if ( IsNull(ldef) )
		{
		if ( NotNull(ltype) ) *ltype = LastLType;
		return LastIdent;
		}
	LastLType = ldef;

	/* Match the line type name */
	for ( ii=0; ii<ldef->ntypes; ii++ )
		{
		if ( same(type, ldef->type_names[ii]) )
			{
			LastIdent = ii;
			break;
			}
		}

	if ( NotNull(ltype) ) *ltype = LastLType;
	return LastIdent;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns information for a given element from the
 * FpaConfigElementScatteredTypeStruct structure.
 *
 *	@param[in]	element	element name
 *	@param[in]	level	level name
 *	@param[in]	type	scattered type name
 *	@param[out]	**stype ElementScatteredType structure
 *  @return The index of the information in the type_names list of
 * 			the structure.
 **********************************************************************/
int						identify_scattered_type_by_name

	(
	STRING		element,
	STRING		level,
	STRING		type,
	FpaConfigElementScatteredTypeStruct	**stype
	)

	{
	int									ii;
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementStruct				*edef;
	FpaConfigElementScatteredTypeStruct	*sdef;

	static STRING	LastElement = NullString;
	static STRING	LastLevel   = NullString;
	static STRING	LastType    = NullString;
	static int		LastIdent   = -1;
	static FpaConfigElementScatteredTypeStruct
					*LastSType  = NullPtr(FpaConfigElementScatteredTypeStruct *);

	if ( NotNull(stype) ) *stype = NullPtr(FpaConfigElementScatteredTypeStruct *);

	/* Return the saved parameters if nothing has changed */
	if ( same(element, LastElement) && same(level, LastLevel)
			&& same(type, LastType) )
		{
		if ( NotNull(stype) ) *stype = LastSType;
		return LastIdent;
		}

	/* Initialize saved parameters */
	FREEMEM(LastElement);
	FREEMEM(LastLevel);
	FREEMEM(LastType);
	LastIdent = -1;
	LastSType = NullPtr(FpaConfigElementScatteredTypeStruct *);

	/* Save the values for the next call */
	LastElement = strdup(SafeStr(element));
	LastLevel   = strdup(SafeStr(level));
	LastType    = strdup(SafeStr(type));

	/* Get the detailed element information */
	fdef = get_field_info(element, level);
	edef = (NotNull(fdef))? fdef->element: get_element_info(element);
	if ( IsNull(edef) )
		{
		if ( NotNull(stype) ) *stype = LastSType;
		return LastIdent;
		}

	/* Get the ElementScatteredType structure */
	sdef = edef->elem_detail->scattered_types;
	if ( IsNull(sdef) )
		{
		if ( NotNull(stype) ) *stype = LastSType;
		return LastIdent;
		}
	LastSType = sdef;

	/* Match the scattered type name */
	for ( ii=0; ii<sdef->ntypes; ii++ )
		{
		if ( same(type, sdef->type_names[ii]) )
			{
			LastIdent = ii;
			break;
			}
		}

	/* No matching type found ... is this a default type? */
	if ( LastIdent < 0 )
		{
		if ( sdef->ntypes == 1
				&& same_ic(sdef->type_names[0], AttribScatteredTypeDefault) )
			{
			(void) pr_diag("Config",
					"Matching scattered type \"%s\" to \"%s\" for field \"%s %s\"\n",
						type, AttribScatteredTypeDefault, element, level);
			LastIdent = 0;
			}
		}

	if ( NotNull(stype) ) *stype = LastSType;
	return LastIdent;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns information for a given element from the
 * FpaConfigElementLabellingStruct structure.
 *
 *	@param[in]	element	element name
 *	@param[in]	level	level name
 *	@param[in]	type	labelling type name
 *	@param[out]	**ltype ElementLabelling structure
 * @return The index of the information in the type_names list of the structure.
 **********************************************************************/
int						identify_labelling_type_by_name

	(
	STRING		element,
	STRING		level,
	STRING		type,
	FpaConfigElementLabellingStruct	**ltype
	)

	{
	int								ii;
	STRING							dtype;
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementStruct			*edef;
	FpaConfigElementLabellingStruct	*ldef;

	static STRING	LastElement = NullString;
	static STRING	LastLevel   = NullString;
	static STRING	LastType    = NullString;
	static int		LastIdent   = -1;
	static FpaConfigElementLabellingStruct
					*LastLType  = NullPtr(FpaConfigElementLabellingStruct *);

	if ( NotNull(ltype) ) *ltype = NullPtr(FpaConfigElementLabellingStruct *);

	/* Return the saved parameters if nothing has changed */
	if ( same(element, LastElement) && same(level, LastLevel)
			&& same(type, LastType) )
		{
		if ( NotNull(ltype) ) *ltype = LastLType;
		return LastIdent;
		}

	/* Initialize saved parameters */
	FREEMEM(LastElement);
	FREEMEM(LastLevel);
	FREEMEM(LastType);
	LastIdent = -1;
	LastLType = NullPtr(FpaConfigElementLabellingStruct *);

	/* Save the values for the next call */
	LastElement = strdup(SafeStr(element));
	LastLevel   = strdup(SafeStr(level));
	LastType    = strdup(SafeStr(type));

	/* Get the detailed element information */
	fdef = get_field_info(element, level);
	edef = (NotNull(fdef))? fdef->element: get_element_info(element);
	if ( IsNull(edef) )
		{
		if ( NotNull(ltype) ) *ltype = LastLType;
		return LastIdent;
		}

	/* Get the ElementLabelling structure */
	ldef = edef->elem_detail->labelling;
	if ( IsNull(ldef) )
		{
		if ( NotNull(ltype) ) *ltype = LastLType;
		return LastIdent;
		}
	LastLType = ldef;

	/* Match the labelling type name */
	for ( ii=0; ii<ldef->ntypes; ii++ )
		{
		if ( same(type, ldef->type_names[ii]) )
			{
			LastIdent = ii;
			break;
			}
		}

	/* No matching type found ... is this an automatic labelling type? */
	if ( LastIdent < 0 )
		{

		/* Check for automatic labelling types based on enumerated field type */
		switch ( edef->fld_type )
			{

			/* Continuous field type */
			case FpaC_CONTINUOUS:
				if ( same(type, FpaDefLabContour) )
					dtype = FpaLabellingContinuous;
				else if ( same(type, FpaDefLabLowAtMin) )
					dtype = FpaLabellingContinuous;
				else if ( same(type, FpaDefLabHighAtMax) )
					dtype = FpaLabellingContinuous;
				else
					dtype = FpaCblank;
				break;

			/* Vector field type */
			case FpaC_VECTOR:
				if ( same(type, FpaDefLabContour) )
					dtype = FpaLabellingVector;
				else if ( same(type, FpaDefLabLowAtMin) )
					dtype = FpaLabellingVector;
				else if ( same(type, FpaDefLabHighAtMax) )
					dtype = FpaLabellingVector;
				else
					dtype = FpaCblank;
				break;

			/* Discrete field type */
			case FpaC_DISCRETE:
				if ( same(type, FpaDefLabArea) )
					dtype = FpaLabellingDiscrete;
				else
					dtype = FpaCblank;
				break;

			/* Wind field type */
			case FpaC_WIND:
				if ( same(type, FpaDefLabWind) )
					dtype = FpaLabellingWindBarb;
				else if ( same(type, FpaDefLabAdjustment) )
					dtype = FpaLabellingWindArea;
				else
					dtype = FpaCblank;
				break;

			/* Line field type */
			case FpaC_LINE:
				if ( same(type, FpaDefLabLine) )
					dtype = FpaLabellingLine;
				else
					dtype = FpaCblank;
				break;

			/* Field types that cannot be labelled! */
			case FpaC_SCATTERED:
			case FpaC_LCHAIN:
			default:
				dtype = FpaCblank;
				break;
			}

		/* Match the automatic labelling type name (if found) */
		if ( !blank(dtype) )
			{
			for ( ii=0; ii<ldef->ntypes; ii++ )
				{
				if ( same(dtype, ldef->type_names[ii]) )
					{
					LastIdent = ii;
					break;
					}
				}
			}
		}

	if ( NotNull(ltype) ) *ltype = LastLType;
	return LastIdent;
	}

/**********************************************************************/

/************************************************************************/
/** This function checks if two element names are equivalent.
 *
 *  @param[in]	name1		first element name to compare
 *  @param[in]	name2		second element name to compare
 * @return True if elements are equivalent.
 **********************************************************************/
LOGICAL					equivalent_element_definitions

	(
	STRING		name1,
	STRING		name2
	)

	{
	FpaConfigElementStruct	*edef1, *edef2;

	/* Static buffer for FpaCanyElement element */
	static	FpaConfigElementStruct	*AnyElement = NullPtr(FpaConfigElementStruct *);

	/* Get pointer for special element FpaCanyElement */
	if ( IsNull(AnyElement) )
		{
		AnyElement = identify_element(FpaCanyElement);
		}

	/* Get pointers for element names to compare */
	edef1 = identify_element(name1);
	edef2 = identify_element(name2);

	/* Return FALSE if problems with either element name */
	if ( IsNull(edef1) || IsNull(edef2) ) return FALSE;

	/* Return TRUE if element names match */
	if ( edef1 == edef2 ) return TRUE;

	/* Return FALSE if special element FpaCanyElement not found */
	if ( IsNull(AnyElement) || !AnyElement->valid )
		{
		if ( IsNull(AnyElement) )
			(void) config_file_message(FpaCblockElements,
					FpaCanyElement, FpaCblank, FpaCblank, FpaCmsgMissName);
		else if ( !AnyElement->valid )
			(void) config_file_message(FpaCblockElements,
					AnyElement->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return FALSE;
		}

	/* Return TRUE if either element name is special element FpaCanyElement */
	if ( (edef1 == AnyElement) || (edef2 == AnyElement) ) return TRUE;

	/* Return FALSE if element names do not match */
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*   i d e n t i f y _ f i e l d                                        *
*   g e t _ f i e l d _ i n f o                                        *
*   i d e n t i f y _ f i e l d s _ b y _ g r o u p                    *
*   i d e n t i f y _ f i e l d s _ b y _ g r o u p _ f r e e          *
*                                                                      *
***********************************************************************/

/* Storage for Fields information */
static	LOGICAL					FieldsRead  = FALSE;
static	LOGICAL					FieldsValid = FALSE;
static	int						NumFieldDef = 0;
static	FpaConfigFieldStruct	**FieldDefs = NullPtr(FpaConfigFieldStruct **);

/* Storage for Fields identifier information */
static	int						NumFieldIdent = 0;
static	FPAC_FIELD_IDENTS		*FieldIdents  = NullPtr(FPAC_FIELD_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named
 * field in the Fields blocks of the configuration files.
 * Note that field name comparisons are case insensitive!
 *
 *  @param[in]	element		field element name
 *  @param[in]	level		field level name
 * @return Named field structure.
 **********************************************************************/
FpaConfigFieldStruct		*identify_field

	(
	STRING		element,
	STRING		level
	)

	{
	char					fldbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigElementStruct	*edef;
	FpaConfigLevelStruct	*ldef;
	FpaConfigFieldStruct	*fdef;

	/* Ensure that configuration file has been read */
	if ( !read_fields_info() ) return NullPtr(FpaConfigFieldStruct *);

	/* Identify Element and Level structures for field */
	edef = identify_element(element);
	ldef = identify_level(level);

	/* Return Null if field cannot be created */
	if ( IsNull(edef) || IsNull(ldef) ) return NullPtr(FpaConfigFieldStruct *);

	/* Check for field in list */
	fdef = find_field(edef, ldef);

	/* Error message if field data is not OK */
	if ( NotNull(fdef) && !fdef->valid )
		{
		(void) strcpy(fldbuf, fdef->element->name);
		(void) strcat(fldbuf, " ");
		(void) strcat(fldbuf, fdef->level->name);
		(void) config_file_message(FpaCblockFields,
				fldbuf, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigFieldStruct *);
		}

	/* Return pointer to Field structure in list               */
	/*  ... with default group and labels added (if necessary) */
	if ( NotNull(fdef) )
		{
		(void) set_field_group_and_labels(fdef);
		return fdef;
		}

	/* Return Null if element and level are not consistent */
	if ( !consistent_element_and_level(edef, ldef) )
			return NullPtr(FpaConfigFieldStruct *);

	/* Create field (if not already in list) */
	/*  ... and add default group and labels */
	fdef = init_field(edef, ldef);
	fdef->created_field = TRUE;
	(void) set_field_group_and_labels(fdef);

	/* Return pointer to created Field structure */
	return fdef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a pointer to a structure for a named
 * field in the Fields blocks of the configuration files.
 *
 * Note that field name comparisons are case insensitive!
 *
 *  @param[in]	element		field element name
 *  @param[in]	level		field level name
 * @return detailed field information in the named field structure.
 **********************************************************************/
FpaConfigFieldStruct		*get_field_info

	(
	STRING		element,
	STRING		level
	)

	{
	int									fldtype, nn, natt, nsub, nns;
	char								fldbuf[CONFIG_FILE_MESSAGE_LEN];
	char								linkbuf[CONFIG_FILE_MESSAGE_LEN];
	STRING								wmod, wdir, wspd, wgst;
	STRING								xmod, xdir, xspd, xgst;
	STRING								xcat, xval, xlab;
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementAttribStruct		*attrib;
	FpaConfigElementEditorStruct		*editor;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementLabellingStruct		*labelling;
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigElementLinkingStruct		*linking;
	FpaConfigElementStruct				*linkedef;
	FpaConfigLevelStruct				*linkldef;

	/* Get pointer to structure with detailed information added */
	fdef     = read_field_detailed_info(element, level);
	if ( IsNull(fdef) || !fdef->valid_detail )
			return NullPtr(FpaConfigFieldStruct *);

	fldtype   = fdef->element->fld_type;
	attrib    = fdef->element->elem_detail->attributes;
	editor    = fdef->element->elem_detail->editor;
	stypes    = fdef->element->elem_detail->scattered_types;
	labelling = fdef->element->elem_detail->labelling;
	sampling  = fdef->element->elem_detail->sampling;
	linking   = fdef->element->elem_detail->linking;

	/* Set field name for error messages */
	(void) strcpy(fldbuf, fdef->element->name);
	(void) strcat(fldbuf, " ");
	(void) strcat(fldbuf, fdef->level->name);

	/* Ensure that at least the default attributes have been set */
	if ( IsNull(attrib) )
		{

		/* Set default attributes based on type of field */
		fdef->element->elem_detail->attributes =
				init_element_attributes(fldtype);
		/* Reset the attributes pointer */
		attrib = fdef->element->elem_detail->attributes;
		}

	/* >>> Add new special checks for appropriate fields <<< */

	/* Special one time sampling check for fields */
	if ( NotNull(sampling) && !sampling->check_sampling )
		{
		(void) pr_diag("Config", "One time sampling check for: \"%s %s\"\n",
				fdef->element->name, fdef->level->name);
		sampling->check_sampling = TRUE;

		/* Check for sampling attributes for discrete fields */
		if ( fldtype == FpaC_DISCRETE )
			{
			for ( nn=0; nn<sampling->type.discrete->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.discrete->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCsampling,
							sampling->type.discrete->sattrib_names[nn],
							FpaCmsgMember);
					fdef->valid_detail = FALSE;
					}
				}
			}

		/* Check for sampling attributes for line fields */
		else if ( fldtype == FpaC_LINE )
			{
			for ( nn=0; nn<sampling->type.line->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.line->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCsampling,
							sampling->type.line->sattrib_names[nn],
							FpaCmsgMember);
					fdef->valid_detail = FALSE;
					}
				}
			}

		/* Check for sampling attributes for scattered fields */
		else if ( fldtype == FpaC_SCATTERED )
			{
			for ( nn=0; nn<sampling->type.scattered->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.scattered->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCsampling,
							sampling->type.scattered->sattrib_names[nn],
							FpaCmsgMember);
					fdef->valid_detail = FALSE;
					}
				}
			}

		/* Check for sampling attributes for link chain fields */
		if ( fldtype == FpaC_LCHAIN )
			{
			for ( nn=0; nn<sampling->type.lchain->nsattribs; nn++ )
				{
				natt = add_element_attribute(sampling->type.lchain->sattrib_names[nn],
						FpaCblank, attrib);
				if ( natt < 0 )
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCsampling,
							sampling->type.lchain->sattrib_names[nn],
							FpaCmsgMember);
					fdef->valid_detail = FALSE;
					}
				}
			}
		}

	/* Special one time editor check for fields */
	if ( NotNull(editor) && !editor->check_editor )
		{
		(void) pr_diag("Config", "One time editor check for: \"%s %s\"\n",
				fdef->element->name, fdef->level->name);
		editor->check_editor = TRUE;

		/* Check for editor parameters for continuous fields */
		if ( fldtype == FpaC_CONTINUOUS )
			{
			if ( editor->type.continuous->poke == 0.0
					|| IsNull(editor->type.continuous->units) )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCeditor, FpaCmsgInvalid);
				fdef->valid_detail = FALSE;
				}
			}

		/* Check for editor parameters for continuous fields */
		else if ( fldtype == FpaC_VECTOR )
			{
			if ( editor->type.vector->mag_poke == 0.0
					|| IsNull(editor->type.vector->mag_units)
					|| editor->type.vector->dir_poke == 0.0
					|| IsNull(editor->type.vector->dir_units) )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCeditor, FpaCmsgInvalid);
				fdef->valid_detail = FALSE;
				}
			}

		/* Check for attributes for discrete fields */
		else if ( fldtype == FpaC_DISCRETE )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				fdef->valid_detail = FALSE;
				}
			}

		/* Check for attributes and wind cross references for wind fields */
		else if ( fldtype == FpaC_WIND )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				fdef->valid_detail = FALSE;
				}

			/* Check consistency of values for default wind attributes */
			else
				{

				/* Extract values for default wind attributes */
				wmod = wdir = wspd = wgst = NullString;
				for ( nn=0; nn<attrib->nattribs; nn++ )
					{
					if ( same_ic(attrib->attrib_names[nn],
														AttribWindModel) )
						{
						wmod = attrib->attrib_back_defs[nn];
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindDirection) )
						{
						wdir = attrib->attrib_back_defs[nn];
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindSpeed) )
						{
						wspd = attrib->attrib_back_defs[nn];
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindGust) )
						{
						wgst = attrib->attrib_back_defs[nn];
						}
					}

				/* Check consistency of default wind attribute values */
				if ( !consistent_wind_attrib_strings(wmod, wdir, wspd, wgst,
						&xmod, &xdir, &xspd, &xgst, &xcat, &xval, &xlab) )
					{
					(void) pr_error("Config",
							"Block: \"%s\"    Name: \"%s\"\n",
							FpaCblockFields, fldbuf);
					(void) pr_error("Config",
							"     Inconsistent wind attributes in config files!\n");
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindModel, wmod);
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindDirection, wdir);
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindSpeed, wspd);
					(void) pr_error("Config",
							"       %s = \"%s\"\n", AttribWindGust, wgst);
					fdef->valid_detail = FALSE;
					}

				/* Reset values for default wind attributes */
				for ( nn=0; nn<attrib->nattribs; nn++ )
					{
					if ( same_ic(attrib->attrib_names[nn],
														AttribWindModel) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xmod);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindDirection) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xdir);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindSpeed) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xspd);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribWindGust) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xgst);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribCategory) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xcat);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribAutolabel) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xval);
						}
					else if ( same_ic(attrib->attrib_names[nn],
														AttribUserlabel) )
						{
						attrib->attrib_back_defs[nn] =
							STRMEM(attrib->attrib_back_defs[nn], xlab);
						}
					}
				}
			}

		/* Check for attributes for line fields */
		else if ( fldtype == FpaC_LINE )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				fdef->valid_detail = FALSE;
				}
			}

		/* Check for attributes for scattered fields */
		else if ( fldtype == FpaC_SCATTERED )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				fdef->valid_detail = FALSE;
				}
			}

		/* Check for attributes for link chain fields */
		else if ( fldtype == FpaC_LCHAIN )
			{

			/* Error message if there are no attributes */
			if ( IsNull(attrib) || attrib->nattribs < 1 )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCblank, FpaCattributes, FpaCmsgMissSection);
				fdef->valid_detail = FALSE;
				}
			}

		/* Set default modify file for continuous fields */
		if ( fldtype == FpaC_CONTINUOUS )
			{
			if ( IsNull(editor->type.continuous->modify_file)
					&& NotNull(editor->type.continuous->entry_file) )
				{
				editor->type.continuous->modify_file =
					strdup(editor->type.continuous->entry_file);
				}
			}

		/* Set default modify file for vector fields */
		else if ( fldtype == FpaC_VECTOR )
			{
			if ( IsNull(editor->type.vector->modify_file)
					&& NotNull(editor->type.vector->entry_file) )
				{
				editor->type.vector->modify_file =
					strdup(editor->type.vector->entry_file);
				}
			}

		/* Set default modify file for discrete fields */
		else if ( fldtype == FpaC_DISCRETE )
			{
			if ( IsNull(editor->type.discrete->modify_file)
					&& NotNull(editor->type.discrete->entry_file) )
				{
				editor->type.discrete->modify_file =
					strdup(editor->type.discrete->entry_file);
				}
			}

		/* Set default modify file for wind fields */
		else if ( fldtype == FpaC_WIND )
			{
			if ( IsNull(editor->type.wind->modify_file)
					&& NotNull(editor->type.wind->entry_file) )
				{
				editor->type.wind->modify_file =
					strdup(editor->type.wind->entry_file);
				}
			}

		/* Set default modify file for line fields */
		else if ( fldtype == FpaC_LINE )
			{
			if ( IsNull(editor->type.line->modify_file)
					&& NotNull(editor->type.line->entry_file) )
				{
				editor->type.line->modify_file =
					strdup(editor->type.line->entry_file);
				}
			}

		/* Set default modify file for link chain fields */
		else if ( fldtype == FpaC_LCHAIN )
			{
			if ( IsNull(editor->type.lchain->modify_file)
					&& NotNull(editor->type.lchain->entry_file) )
				{
				editor->type.lchain->modify_file =
					strdup(editor->type.lchain->entry_file);
				}
			}

		/* Set default node modify file for link chain fields */
		if ( fldtype == FpaC_LCHAIN )
			{
			if ( IsNull(editor->type.lchain->node_modify_file)
					&& NotNull(editor->type.lchain->node_entry_file) )
				{
				editor->type.lchain->node_modify_file =
					strdup(editor->type.lchain->node_entry_file);
				}
			}
		}

	/* Add default scattered types section (if required) */
	if ( fldtype == FpaC_SCATTERED )
		{

		/* Initialize the ElementScatteredType structure (if required) */
		if ( IsNull(stypes) )
			{
			fdef->element->elem_detail->scattered_types =
					init_element_scattered_types();
			stypes = fdef->element->elem_detail->scattered_types;
			}

		/* Add a default scattered type to the list (if required) */
		if ( stypes->ntypes < 1 )
			{

			/* Add a default scattered type */
			nsub = add_element_scattered_type(AttribScatteredTypeDefault,
					stypes);
			if ( nsub < 0 )
				{
				(void) config_file_message(FpaCblockFields, fldbuf,
						FpaCscatteredTypes, FpaCdefault, FpaCmsgMember);
				fdef->valid_detail = FALSE;
				}
			}
		}

	/* Special one time check for scattered types */
	if ( fldtype == FpaC_SCATTERED && !stypes->check_scattered )
		{
		(void) pr_diag("Config", "One time scattered types check for: \"%s %s\"\n",
				fdef->element->name, fdef->level->name);
		stypes->check_scattered = TRUE;

		/* Set default entry file for scattered fields */
		for ( nn=0; nn<stypes->ntypes; nn++ )
			{
			if ( IsNull(stypes->type_entry_files[nn]) )
				{
				if ( NotNull(editor)
						&& NotNull(editor->type.scattered->entry_file) )
					{
					stypes->type_entry_files[nn] =
						strdup(editor->type.scattered->entry_file);
					}
				}
			}

		/* Set default modify file for scattered fields */
		for ( nn=0; nn<stypes->ntypes; nn++ )
			{
			if ( IsNull(stypes->type_modify_files[nn]) )
				{
				if ( NotNull(editor)
						&& NotNull(editor->type.scattered->modify_file) )
					{
					stypes->type_modify_files[nn] =
						strdup(editor->type.scattered->modify_file);
					}
				else if ( NotNull(stypes->type_entry_files[nn]) )
					{
					stypes->type_modify_files[nn] =
						strdup(stypes->type_entry_files[nn]);
					}
				}
			}

		/* Set default entry rules for scattered fields */
		for ( nn=0; nn<stypes->ntypes; nn++ )
			{
			if ( stypes->type_rules[nn].nrules <= 0 )
				{
				if ( NotNull(editor)
						&& editor->type.scattered->nrules > 0 )
					{
					nsub = editor->type.scattered->nrules;
					stypes->type_rules[nn].nrules      = nsub;
					stypes->type_rules[nn].entry_rules = INITMEM(STRING, nsub);
					stypes->type_rules[nn].entry_funcs = INITMEM(ERULE,  nsub);
					for ( nns=0; nns<nsub; nns++ )
						{
						stypes->type_rules[nn].entry_rules[nns] =
							strdup(SafeStr(editor->type.scattered->entry_rules[nns]));
						stypes->type_rules[nn].entry_funcs[nns] =
							editor->type.scattered->entry_funcs[nns];
						}
					}
				}
			}
		}

	/* Special one time labelling check for elements */
	if ( NotNull(labelling) && !labelling->check_labelling )
		{
		(void) pr_diag("Config", "One time labelling check for: \"%s %s\"\n",
				fdef->element->name, fdef->level->name);
		labelling->check_labelling = TRUE;

		/* Set default modify file for labelling types */
		for ( nn=0; nn<labelling->ntypes; nn++ )
			{
			if ( IsNull(labelling->type_modify_files[nn])
					&& NotNull(labelling->type_entry_files[nn]) )
				{
				labelling->type_modify_files[nn] =
					strdup(labelling->type_entry_files[nn]);
				}
			}
		}

	/* Special one time linking check for elements */
	if ( NotNull(linking) && !linking->check_linking )
		{
		(void) pr_diag("Config", "One time linking check for: \"%s %s\"\n",
				fdef->element->name, fdef->level->name);
		linking->check_linking = TRUE;

		/* Check for consistent element and level for linking fields */
		for ( nn=0; nn<linking->nlink; nn++ )
			{
			linkedef = linking->link_elems[nn];
			linkldef = linking->link_levels[nn];
			if ( NotNull(linkedef) && NotNull(linkldef)
					&& !consistent_element_and_level(linkedef, linkldef) )
				{
				(void) strcpy(linkbuf, linkedef->name);
				(void) strcat(linkbuf, " ");
				(void) strcat(linkbuf, linkldef->name);
				(void) config_file_message(FpaCblockFields, fldbuf,
						linkbuf, FpaCdefault, FpaCmsgLinkFld);
				fdef->valid_detail = FALSE;
				}
			}
		}

	/* Return pointer based on detailed information OK */
	return ( fdef->valid_detail ) ? fdef: NullPtr(FpaConfigFieldStruct *);
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures for a given
 * group of fields in the Fields blocks of the configuration files.
 *
 *	@param[in]	fgroup	field group name
 *	@param[out]	***list	list of fields in the same field group
 *  @return the size of the list.
 **********************************************************************/
int						identify_fields_by_group

	(
	STRING					fgroup,
	FpaConfigFieldStruct	***list
	)

	{
	int						nfdefs, nn;
	FpaConfigFieldStruct	*fdef;
	FpaConfigGroupStruct	*gdef;
	FpaConfigFieldStruct	**fdeflist = NullPtr(FpaConfigFieldStruct **);

	/* Initialize list of pointers to fields in the same field group */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigFieldStruct **);

	/* Checking for all fields */
	if ( same(fgroup, FpaCanyGroup) )
		{
		gdef = NullPtr(FpaConfigGroupStruct *);
		}

	/* Get pointer to field group */
	else
		{
		gdef = identify_group(FpaCblockFields, fgroup);
		if ( IsNull(gdef) ) return 0;
		}

	/* Ensure that configuration file has been read */
	if ( !read_fields_info() ) return 0;

	/* Add to list of pointers to fields with same field group */
	/*  ... but only if list is returned!                      */
	for ( nfdefs=0, nn=0; nn<NumFieldDef; nn++ )
		{
		fdef = FieldDefs[nn];
		if ( same(fgroup, FpaCanyGroup) || gdef == fdef->group )
			{

			/* Add to list of pointers to fields in the same field group */
			nfdefs++;
			if ( NotNull(list) )
				{
				fdeflist = GETMEM(fdeflist, FpaConfigFieldStruct *, nfdefs);
				fdeflist[nfdefs-1] = fdef;
				}
			}
		}

	/* Return the list of pointers to fields in the same field group */
	if ( NotNull(list) ) *list = fdeflist;
	return nfdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures for a given
 * group of fields in the Fields blocks of the configuration files.
 *
 * @param[in]	***list		list of Field Structures
 * @param[in]	num			size of list
 * @return the size of the list (0).
 **********************************************************************/
int						identify_fields_by_group_free

	(
	FpaConfigFieldStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}


/***********************************************************************
*                                                                      *
*   i d e n t i f y _ c r o s s r e f                                  *
*   i d e n t i f y _ c r o s s r e f s _ f o r _ w i n d s            *
*   i d e n t i f y _ c r o s s r e f s _ f o r _ w i n d s _ f r e e  *
*   i d e n t i f y _ c r o s s r e f s _ f o r _ v a l u e s          *
*   i d e n t i f y _ c r o s s r e f s _ f o r _ v a l u e s _ f r e e*
*                                                                      *
***********************************************************************/

/* Storage for CrossRefs information */
static	LOGICAL					CrossRefsRead  = FALSE;
static	LOGICAL					CrossRefsValid = FALSE;
static	int						NumCRefWindDef = 0;
static	FpaConfigCrossRefStruct	**CRefWindDefs = NullPtr(FpaConfigCrossRefStruct **);
static	int						NumCRefValDef  = 0;
static	FpaConfigCrossRefStruct	**CRefValDefs  = NullPtr(FpaConfigCrossRefStruct **);

/* Storage for CrossRefs identifier information */
static	int						NumCRefWindIdent = 0;
static	FPAC_IDENTS				*CRefWindIdents  = NullPtr(FPAC_IDENTS *);
static	int						NumCRefValIdent  = 0;
static	FPAC_IDENTS				*CRefValIdents   = NullPtr(FPAC_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named wind or
 * value cross-reference in the CrossRefs blocks of the configuration files.
 *
 * Note that cross-reference name comparisons are case insensitive!
 *
 *  @param[in]	type		winds or values type
 *  @param[in]	name		cross-reference name for winds or values
 * 	@return Named cross-reference structure for the
 * 			requested name and type.
 **********************************************************************/
FpaConfigCrossRefStruct		*identify_crossref

	(
	STRING		type,
	STRING		name
	)

	{
	FpaConfigCrossRefStruct	*crdef;

	/* Ensure that configuration file has been read */
	if ( !read_crossrefs_info() ) return NullPtr(FpaConfigCrossRefStruct *);

	/* Find cross-reference name for winds or values                          */
	/*  ... and return Null pointer if cross-reference name not found in list */
	if      ( same(type, FpaCcRefsWinds) )  crdef = find_wind_crossref(name);
	else if ( same(type, FpaCcRefsValues) ) crdef = find_value_crossref(name);
	else
		{
		(void) pr_error("Config",
				"[identify_crossref] Unknown cross-reference type \"%s\"!\n",
						type);
		return NullPtr(FpaConfigCrossRefStruct *);
		}
	if ( IsNull(crdef) ) return NullPtr(FpaConfigCrossRefStruct *);

	/* Error message if cross-reference data for winds or values is not OK */
	if ( !crdef->valid )
		{
		if      ( same(type, FpaCcRefsWinds) )
			(void) config_file_message(FpaCblockCRWinds,
					crdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		else if ( same(type, FpaCcRefsValues) )
			(void) config_file_message(FpaCblockCRValues,
					crdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigCrossRefStruct *);
		}

	/* Return pointer to cross-reference structure for winds or values */
	return crdef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures of
 * cross-references for Winds in the CrossRefs blocks of
 * the configuration files.
 *
 *	@param[in]	***list	list of crossrefs for winds
 * 	@return The size of the list.
 **********************************************************************/
int						identify_crossrefs_for_winds

	(
	FpaConfigCrossRefStruct	***list
	)

	{
	int						ncrdefs, nn;
	FpaConfigCrossRefStruct	*crdef;
	FpaConfigCrossRefStruct	**crdeflist = NullPtr(FpaConfigCrossRefStruct **);

	/* Initialize list of pointers to crossrefs for winds */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigCrossRefStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_crossrefs_info() ) return 0;

	/* Add to list of pointers to crossrefs for winds  */
	/*  ... but only if list is returned!              */
	for ( ncrdefs=0, nn=0; nn<NumCRefWindDef; nn++ )
		{
		crdef = CRefWindDefs[nn];
		ncrdefs++;
		if ( NotNull(list) )
			{
			crdeflist = GETMEM(crdeflist, FpaConfigCrossRefStruct *, ncrdefs);
			crdeflist[ncrdefs-1] = crdef;
			}
		}

	/* Return the list of pointers to crossrefs for winds */
	if ( NotNull(list) ) *list = crdeflist;
	return ncrdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures of
 * cross-references for Winds in the CrossRefs blocks of
 * the configuration files.
 *
 * @param[in]	***list		list of CrossRef Structures
 * @param[in]	num			size of list
 * @return The size of the list (0).
 **********************************************************************/
int						identify_crossrefs_for_winds_free

	(
	FpaConfigCrossRefStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures of
 * cross-references for Values in the CrossRefs blocks of  the
 * configuration files.
 *
 *	@param[out]	***list	list of crossrefs for values
 *  @return The size of the list.
 **********************************************************************/
int						identify_crossrefs_for_values

	(
	FpaConfigCrossRefStruct	***list
	)

	{
	int						ncrdefs, nn;
	FpaConfigCrossRefStruct	*crdef;
	FpaConfigCrossRefStruct	**crdeflist = NullPtr(FpaConfigCrossRefStruct **);

	/* Initialize list of pointers to crossrefs for values */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigCrossRefStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_crossrefs_info() ) return 0;

	/* Add to list of pointers to crossrefs for values */
	/*  ... but only if list is returned!              */
	for ( ncrdefs=0, nn=0; nn<NumCRefValDef; nn++ )
		{
		crdef = CRefValDefs[nn];
		ncrdefs++;
		if ( NotNull(list) )
			{
			crdeflist = GETMEM(crdeflist, FpaConfigCrossRefStruct *, ncrdefs);
			crdeflist[ncrdefs-1] = crdef;
			}
		}

	/* Return the list of pointers to crossrefs for values */
	if ( NotNull(list) ) *list = crdeflist;
	return ncrdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures of
 * cross-references for Values in the CrossRefs blocks of
 * the configuration files.
 *
 *	@param[in]	***list		list of CrossRef Structure
 *	@param[in]	num			size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int						identify_crossrefs_for_values_free

	(
	FpaConfigCrossRefStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}


/***********************************************************************
*                                                                      *
*   i d e n t i f y _ s a m p l e                                      *
*   i d e n t i f y _ s a m p l e s _ f o r _ v a l u e s              *
*   i d e n t i f y _ s a m p l e s _ f o r _ v a l u e s _ f r e e    *
*   i d e n t i f y _ s a m p l e s _ f o r _ w i n d s                *
*   i d e n t i f y _ s a m p l e s _ f o r _ w i n d s _ f r e e      *
*                                                                      *
***********************************************************************/

/* Storage for Samples information */
static	LOGICAL					SamplesRead      = FALSE;
static	LOGICAL					SamplesValid     = FALSE;
static	int						NumSampleValDef  = 0;
static	FpaConfigSampleStruct	**SampleValDefs  = NullPtr(FpaConfigSampleStruct **);
static	int						NumSampleWindDef = 0;
static	FpaConfigSampleStruct	**SampleWindDefs = NullPtr(FpaConfigSampleStruct **);

/* Storage for Samples identifier information */
static	int						NumSampleValIdent  = 0;
static	FPAC_IDENTS				*SampleValIdents   = NullPtr(FPAC_IDENTS *);
static	int						NumSampleWindIdent = 0;
static	FPAC_IDENTS				*SampleWindIdents  = NullPtr(FPAC_IDENTS *);

/************************************************************************/
/** This function returns a pointer to a structure for a named value
 * or wind sample type in the Samples blocks of the configuration
 * files.
 *
 * Note that sample name comparisons are case insensitive!
 *
 *	@param[in]	type		values or winds type
 *	@param[in]	name		sample type name for values or winds
 * 	@return Named sample type structure.
 **********************************************************************/
FpaConfigSampleStruct		*identify_sample

	(
	STRING		type,
	STRING		name
	)

	{
	FpaConfigSampleStruct	*xdef;

	/* Ensure that configuration file has been read */
	if ( !read_samples_info() ) return NullPtr(FpaConfigSampleStruct *);

	/* Find sample type name for values or winds                          */
	/*  ... and return Null pointer if sample type name not found in list */
	if      ( same(type, FpaCsamplesValues) ) xdef = find_valuetype_sample(name);
	else if ( same(type, FpaCsamplesWinds) )  xdef = find_windtype_sample(name);
	else
		{
		(void) pr_error("Config",
				"[identify_sample] Unknown sample type \"%s\"!\n", type);
		return NullPtr(FpaConfigSampleStruct *);
		}
	if ( IsNull(xdef) ) return NullPtr(FpaConfigSampleStruct *);

	/* Error message if sample type data for values or winds is not OK */
	if ( !xdef->valid )
		{
		if      ( same(type, FpaCsamplesValues) )
			(void) config_file_message(FpaCblockSValues,
					xdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		else if ( same(type, FpaCsamplesWinds) )
			(void) config_file_message(FpaCblockSWinds,
					xdef->name, FpaCblank, FpaCblank, FpaCmsgInvalid);
		return NullPtr(FpaConfigSampleStruct *);
		}

	/* Return pointer to Sample structure for values or winds */
	return xdef;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures of samples
 * for Values in the Samples blocks of the configuration files.
 *
 *	@param[out]	***list	list of samples for values
 *  @return The size of the list.
 **********************************************************************/
int						identify_samples_for_values

	(
	FpaConfigSampleStruct	***list
	)

	{
	int						nxdefs, nn;
	FpaConfigSampleStruct	*xdef;
	FpaConfigSampleStruct	**xdeflist = NullPtr(FpaConfigSampleStruct **);

	/* Initialize list of pointers to samples for values */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigSampleStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_samples_info() ) return 0;

	/* Add to list of pointers to samples for values */
	/*  ... but only if list is returned!            */
	for ( nxdefs=0, nn=0; nn<NumSampleValDef; nn++ )
		{
		xdef = SampleValDefs[nn];
		nxdefs++;
		if ( NotNull(list) )
			{
			xdeflist = GETMEM(xdeflist, FpaConfigSampleStruct *, nxdefs);
			xdeflist[nxdefs-1] = xdef;
			}
		}

	/* Return the list of pointers to samples for values */
	if ( NotNull(list) ) *list = xdeflist;
	return nxdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures of samples
 * for Values in the Samples blocks of the configuration files.
 *
 *	@param[in]	***list		list of Sample Structures
 *	@param[in]	num			size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int						identify_samples_for_values_free

	(
	FpaConfigSampleStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/**********************************************************************/

/************************************************************************/
/** This function returns a list of pointers to structures of samples
 * for Winds in the Samples blocks of the configuration files.
 *
 *	@param[out]	***list	list of samples for winds
 * @return The size of the list.
 **********************************************************************/
int						identify_samples_for_winds

	(
	FpaConfigSampleStruct	***list
	)

	{
	int						nxdefs, nn;
	FpaConfigSampleStruct	*xdef;
	FpaConfigSampleStruct	**xdeflist = NullPtr(FpaConfigSampleStruct **);

	/* Initialize list of pointers to samples for winds */
	if ( NotNull(list) ) *list = NullPtr(FpaConfigSampleStruct **);

	/* Ensure that configuration file has been read */
	if ( !read_samples_info() ) return 0;

	/* Add to list of pointers to samples for winds  */
	/*  ... but only if list is returned!              */
	for ( nxdefs=0, nn=0; nn<NumSampleWindDef; nn++ )
		{
		xdef = SampleWindDefs[nn];
		nxdefs++;
		if ( NotNull(list) )
			{
			xdeflist = GETMEM(xdeflist, FpaConfigSampleStruct *, nxdefs);
			xdeflist[nxdefs-1] = xdef;
			}
		}

	/* Return the list of pointers to samples for winds */
	if ( NotNull(list) ) *list = xdeflist;
	return nxdefs;
	}

/**********************************************************************/

/************************************************************************/
/** This function frees a list of pointers to structures of samples
 * for Winds in the Samples blocks of the configuration files.
 *
 *	@param[in]	***list		list of Sample Structures
 *	@param[in]	num			size of list
 * 	@return The size of the list (0).
 **********************************************************************/
int						identify_samples_for_winds_free

	(
	FpaConfigSampleStruct	***list,
	int						num
	)

	{

	/* Free the list of pointers */
	FREEMEM(*list);

	/* Reset the number of pointers and return it */
	num = 0;
	return num;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Check all blocks of configuration file) *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ b l o c k s _ i n f o                                    *
*                                                                      *
*   Check for unrecognized blocks in configuration files.              *
*                                                                      *
***********************************************************************/

/* Storage for Blocks information */
static	LOGICAL		BlocksRead  = FALSE;
static	LOGICAL		BlocksValid = FALSE;

static	LOGICAL					read_blocks_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd;

	static STRING	LastBlock = NullString;

	/* Read the configuration file(s) only once */
	if ( BlocksRead ) return BlocksValid;

	/* Find and open the configuration file */
	if ( !first_config_file_open(FpaCblocksFile, &fpcfg) )
		{
		BlocksRead = TRUE;
		return BlocksValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Checking all blocks!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Skip all recognized blocks of configuration file */
		if ( same(cmd, FpaCblockUnits)
				|| same(cmd, FpaCblockConstants)
				|| same(cmd, FpaCblockSources)
				|| same(cmd, FpaCblockGroups)
				|| same(cmd, FpaCblockLevels)
				|| same(cmd, FpaCblockElements)
				|| same(cmd, FpaCblockFields)
				|| same(cmd, FpaCblockCrossRefs)
				|| same(cmd, FpaCblockSamples) )
			{
			FREEMEM(LastBlock);
			LastBlock = strdup(SafeStr(cmd));
			(void) skip_config_file_block(&fpcfg);
			}

		/* Error message for all unrecognized blocks of configuration file */
		else
			{
			(void) config_file_message(cmd, LastBlock,
					FpaCblank, cline, FpaCmsgBlock);
			(void) skip_config_file_block(&fpcfg);
			}
		}
	FREEMEM(LastBlock);

	/* Set flags for completion of checking */
	BlocksRead  = TRUE;
	BlocksValid = TRUE;
	return BlocksValid;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Units block of configuration file)      *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ u n i t s _ i n f o                                      *
*                                                                      *
*   Read information from Units block of configuration files.          *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_units_info

	(
	)

	{
	int						nn;
	FILE					*fpcfg;
	STRING					cline, cmd, arg;
	int						numbrace, section_id, section;
	LOGICAL					firstline, validfactor, validoffset;
	FpaConfigUnitStruct		*udef;

	/* Read the configuration file(s) only once */
	if ( UnitsRead ) return UnitsValid;

	/* Find and open the configuration file for the Units block */
	if ( !first_config_file_open(FpaCunitsFile, &fpcfg) )
		{
		UnitsRead = TRUE;
		return UnitsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Units block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Units block of configuration file */
		if ( same(cmd, FpaCblockUnits) )
			{

			/* Set counter and identifier for Units block */
			numbrace   = 0;
			section    = FpaCblockUnitsName;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Units block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Units block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Units declaration */
				else if ( numbrace == 1 )
					{

					/* Adding another name in FpaCblockUnitsName section */
					if ( section_id == FpaCblockUnitsName )
						{

						/* Check for unit name already in the list */
						udef = find_unit(cmd);

						/* Add another unit name to the lists */
						if ( IsNull(udef) )
							{
							udef = init_unit(cmd);
							}

						/* Check that unit name is not an alias */
						/*  of another unit!                    */
						else if ( !same(cmd, udef->name) )
							{
							(void) config_file_message(FpaCblockUnits,
									cmd, udef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Units block */
						section = FpaCblockUnitsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockUnits,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Units declarations */
				/*  ... with format of "cmd = value(s)" */
				else
					{

					/* Adding parameters in FpaCblockUnitsInfo section */
					if ( section_id == FpaCblockUnitsInfo )
						{

						/* Unit label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									udef->label = STRMEM(udef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockUnits,
										udef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								udef->valid = FALSE;
								}
							}

						/* Unit short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									udef->sh_label =
											STRMEM(udef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockUnits,
										udef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								udef->valid = FALSE;
								}
							}

						/* Unit MKS equivalent */
						else if ( same(cmd, FpaCmksEquivalent) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( OKARG(arg) )
									{
									udef->MKS = STRMEM(udef->MKS, arg);
									}
								else
									{
									(void) config_file_message(FpaCblockUnits,
											udef->name, FpaCblank,
											FpaCmksEquivalent,
											FpaCmsgParameter);
									udef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockUnits,
										udef->name, FpaCblank,
										FpaCmksEquivalent, FpaCmsgNoEqual);
								udef->valid = FALSE;
								}
							}

						/* Unit conversion */
						else if ( same(cmd, FpaCmksConversion) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								udef->factor = double_arg(cline, &validfactor);
								udef->offset = double_arg(cline, &validoffset);
								if ( !validfactor || !validoffset )
									{
									(void) config_file_message(FpaCblockUnits,
											udef->name, FpaCblank,
											FpaCmksConversion,
											FpaCmsgParameter);
									udef->valid  = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockUnits,
										udef->name, FpaCblank,
										FpaCmksConversion, FpaCmsgNoEqual);
								udef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Units keyword */
						else
							{
							(void) config_file_message(FpaCblockUnits,
									udef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							udef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockUnits,
								udef->name, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Error check for each member of Units block */
	for ( nn=0; nn<NumUnitDef; nn++ )
		{
		udef = UnitDefs[nn];

		/* Ensure that "MKS_equivalent" has been set */
		if ( blank(udef->MKS) )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockUnits, udef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCmksEquivalent, udef->name);
			udef->valid = FALSE;
			}
		}

	/* Set flags for completion of reading */
	UnitsRead  = TRUE;
	UnitsValid = TRUE;
	return UnitsValid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ u n i t                                                  *
*   i n i t _ u n i t                                                  *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Units block, or pointer to initialized structure to contain        *
*   information read from Units block of configuration files.          *
*                                                                      *
***********************************************************************/

static	FpaConfigUnitStruct		*find_unit

	(
	STRING		name		/* unit name */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumUnitIdent < 1 ) return NullPtr(FpaConfigUnitStruct *);

	/* Copy the unit name into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for unit name */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) UnitIdents,
			(size_t) NumUnitIdent, sizeof(FPAC_IDENTS), compare_identifiers);

	/* Return pointer if unit name found in list */
	return ( pident ) ? (FpaConfigUnitStruct *) pident->pdef:
							NullPtr(FpaConfigUnitStruct *);
	}

/**********************************************************************/

static	FpaConfigUnitStruct		*init_unit

	(
	STRING		name		/* unit name */
	)

	{
	FpaConfigUnitStruct		*udef;

	/* Add unit at end of current UnitDefs list */
	NumUnitDef++;
	UnitDefs = GETMEM(UnitDefs, FpaConfigUnitStruct *, NumUnitDef);
	UnitDefs[NumUnitDef-1] = INITMEM(FpaConfigUnitStruct, 1);

	/* Initialize UnitDefs structure */
	udef           = UnitDefs[NumUnitDef - 1];
	udef->name     = strdup(name);
	udef->valid    = TRUE;
	udef->label    = strdup(name);
	udef->sh_label = strdup(name);
	udef->MKS      = NullString;
	udef->factor   = 0.0;
	udef->offset   = 0.0;

	/* Add the name as another identifier */
	(void) add_unit_identifier(name, udef);

	/* Return pointer to UnitDefs structure */
	return udef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ u n i t _ i d e n t i f i e r                              *
*                                                                      *
*   Add another identifier to unit identifier list.                    *
*                                                                      *
***********************************************************************/

static	void					add_unit_identifier

	(
	STRING					ident,		/* unit identifier name */
	FpaConfigUnitStruct		*udef		/* pointer to Unit structure */
	)

	{

	/* Add identifier to list */
	NumUnitIdent++;
	UnitIdents = GETMEM(UnitIdents, FPAC_IDENTS, NumUnitIdent);
	UnitIdents[NumUnitIdent-1].ident = strdup(ident);
	UnitIdents[NumUnitIdent-1].pdef  = (POINTER) udef;

	/* Sort the list */
	(void) qsort((POINTER) UnitIdents, (size_t) NumUnitIdent,
			sizeof(FPAC_IDENTS), compare_identifiers);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Constants block of configuration file)  *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ c o n s t a n t s _ i n f o                              *
*                                                                      *
*   Read information from Constants block of configuration files.      *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_constants_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, arg;
	int						numbrace, section_id, section;
	LOGICAL					firstline, valid;
	FpaConfigConstantStruct	*cdef;
	int						nn;

	/* Read the configuration file(s) only once */
	if ( ConstantsRead ) return ConstantsValid;

	/* Force the Units block of the configuration file to be read first */
	if ( !read_units_info() ) return ConstantsValid;

	/* Find and open the configuration file for the Constants block */
	if ( !first_config_file_open(FpaCconstantsFile, &fpcfg) )
		{
		ConstantsRead = TRUE;
		return ConstantsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Constants block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Constants block of configuration file */
		if ( same(cmd, FpaCblockConstants) )
			{

			/* Set counter and identifier for Constants block */
			numbrace   = 0;
			section    = FpaCblockConstantsName;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Constants block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Constants block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Constants declaration */
				else if ( numbrace == 1 )
					{

					/* Adding another name in FpaCblockConstantsName section */
					if ( section_id == FpaCblockConstantsName )
						{

						/* Check for declaration already in the list */
						cdef = find_constant(cmd);

						/* Add another constant name to the lists */
						if ( IsNull(cdef) )
							{
							cdef = init_constant(cmd);
							}

						/* Check that constant name is not an alias */
						/*  of another constant!                    */
						else if ( !same(cmd, cdef->name) )
							{
							(void) config_file_message(FpaCblockConstants,
									cmd, cdef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Constants block */
						section = FpaCblockConstantsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockConstants,
								cmd, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Constants declarations */
				/*  ... with format of "cmd = value(s)"     */
				else
					{

					/* Adding parameters in FpaCblockConstantsInfo section */
					if ( section_id == FpaCblockConstantsInfo )
						{

						/* Constant label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									cdef->label = STRMEM(cdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockConstants,
										cdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								cdef->valid = FALSE;
								}
							}

						/* Constant short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									cdef->sh_label =
											STRMEM(cdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockConstants,
										cdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								cdef->valid = FALSE;
								}
							}

						/* Constant description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									cdef->description =
											STRMEM(cdef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockConstants,
										cdef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								cdef->valid = FALSE;
								}
							}

						/* Constant value and units */
						else if ( same(cmd, FpaCconstant) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								cdef->value = double_arg(cline, &valid);
								arg         = strdup_arg(cline);
								cdef->units = identify_unit(arg);
								FREEMEM(arg);
								if ( !valid )
									{
									(void) config_file_message(FpaCblockConstants,
											cdef->name, FpaCblank,
											FpaCconstantVal, FpaCmsgParameter);
									cdef->valid = FALSE;
									}
								if ( IsNull(cdef->units) )
									{
									(void) config_file_message(FpaCblockConstants,
											cdef->name, FpaCblank,
											FpaCconstantUnit, FpaCmsgParameter);
									cdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockConstants,
										cdef->name, FpaCblank,
										FpaCconstant, FpaCmsgNoEqual);
								cdef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Constants keyword */
						else
							{
							(void) config_file_message(FpaCblockConstants,
									cdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							cdef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockConstants,
								cdef->name, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Error check for each member of Constants block */
	for ( nn=0; nn<NumConstDef; nn++ )
		{
		cdef = ConstDefs[nn];

		/* Ensure that "constant" has been set */
		if ( IsNull(cdef->units) )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockConstants, cdef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCconstant, cdef->name);
			cdef->valid = FALSE;
			}
		}

	/* Set flags for completion of reading */
	ConstantsRead  = TRUE;
	ConstantsValid = TRUE;
	return ConstantsValid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ c o n s t a n t                                          *
*   i n i t _ c o n s t a n t                                          *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Constants block, or pointer to initialized structure to contain    *
*   information read from Constants block of configuration files.      *
*                                                                      *
***********************************************************************/

static	FpaConfigConstantStruct	*find_constant

	(
	STRING		name		/* constant name */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumConstIdent < 1 ) return NullPtr(FpaConfigConstantStruct *);

	/* Copy the constant name into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for constant name */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) ConstIdents,
			(size_t) NumConstIdent, sizeof(FPAC_IDENTS), compare_identifiers);

	/* Return pointer if constant name found in list */
	return ( pident ) ? (FpaConfigConstantStruct *) pident->pdef:
							NullPtr(FpaConfigConstantStruct *);
	}

/**********************************************************************/

static	FpaConfigConstantStruct	*init_constant

	(
	STRING		name		/* constant name */
	)

	{
	FpaConfigConstantStruct	*cdef;

	/* Add constant at end of current ConstDefs list */
	NumConstDef++;
	ConstDefs = GETMEM(ConstDefs, FpaConfigConstantStruct *, NumConstDef);
	ConstDefs[NumConstDef-1] = INITMEM(FpaConfigConstantStruct, 1);

	/* Initialize ConstDefs structure */
	cdef              = ConstDefs[NumConstDef - 1];
	cdef->name        = strdup(name);
	cdef->valid       = TRUE;
	cdef->label       = strdup(name);
	cdef->sh_label    = strdup(name);
	cdef->description = NullString;
	cdef->value       = 0.0;
	cdef->units       = NullPtr(FpaConfigUnitStruct *);

	/* Add the name as another identifier */
	(void) add_constant_identifier(name, cdef);

	/* Return pointer to ConstDefs structure */
	return cdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ c o n s t a n t _ i d e n t i f i e r                      *
*                                                                      *
*   Add another identifier to constant identifier list.                *
*                                                                      *
***********************************************************************/

static	void					add_constant_identifier

	(
	STRING					ident,		/* constant identifier name */
	FpaConfigConstantStruct	*cdef		/* pointer to Constant structure */
	)

	{

	/* Add identifier to list */
	NumConstIdent++;
	ConstIdents = GETMEM(ConstIdents, FPAC_IDENTS, NumConstIdent);
	ConstIdents[NumConstIdent-1].ident = strdup(ident);
	ConstIdents[NumConstIdent-1].pdef  = (POINTER) cdef;

	/* Sort the list */
	(void) qsort((POINTER) ConstIdents, (size_t) NumConstIdent,
			sizeof(FPAC_IDENTS), compare_identifiers);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Sources block of configuration file)    *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ s o u r c e s _ i n f o                                  *
*   r e a d _ s o u r c e _ s u b s o u r c e _ i n f o                *
*   r e a d _ s o u r c e _ a l l i e d _ i n f o                      *
*   r e a d _ a l l i e d _ p r o g r a m s _ i n f o                  *
*   r e a d _ a l l i e d _ f i l e s _ i n f o                        *
*   r e a d _ a l l i e d _ f i e l d s _ i n f o                      *
*   r e a d _ a l l i e d _ w i n d s _ i n f o                        *
*   r e a d _ a l l i e d _ v a l u e s _ i n f o                      *
*   r e a d _ a l l i e d _ m e t a f i l e s _ i n f o                *
*                                                                      *
*   Read information from Sources block of configuration files.        *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_sources_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, arg, sdir, expath;
	int						numbrace, section_id, section, macro;
	LOGICAL					firstline, valid;
	FpaConfigSourceStruct	*sdef;
	int						nn;

	/* Read the configuration file(s) only once */
	if ( SourcesRead ) return SourcesValid;

	/* Find and open the configuration file for the Sources block */
	if ( !first_config_file_open(FpaCsourcesFile, &fpcfg) )
		{
		SourcesRead = TRUE;
		return SourcesValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Sources block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Sources block of configuration file */
		if ( same(cmd, FpaCblockSources) )
			{

			/* Set counter and identifier for Sources block */
			numbrace   = 0;
			section    = FpaCblockSourcesName;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Sources block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Sources block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Sources declaration */
				else if ( numbrace == 1 )
					{

					/* Adding another name in FpaCblockSourcesName section */
					if ( section_id == FpaCblockSourcesName )
						{

						/* Check for declaration already in the list */
						sdef = find_source(cmd);

						/* Add another source name to the lists */
						if ( IsNull(sdef) )
							{
							sdef = init_source(cmd);
							}

						/* Check that source name is not an alias */
						/*  of another source!                    */
						else if ( !same(cmd, sdef->name) )
							{
							(void) config_file_message(FpaCblockSources,
									cmd, sdef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set location of this block */
						(void) set_source_location(fpcfg, sdef);

						/* Set identifier for next section of Sources block */
						section = FpaCblockSourcesInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockSources,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Sources declarations */
				/*  ... with format of "cmd = value(s)"   */
				else
					{

					/* Adding parameters in FpaCblockSourcesInfo section */
					if ( section_id == FpaCblockSourcesInfo )
						{

						/* Source label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									sdef->label = STRMEM(sdef->label, arg);

									/* This label is also used as the label */
									/*  for the first subsource!            */
									sdef->subsrcs[0]->label = sdef->label;
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									sdef->sh_label =
											STRMEM(sdef->sh_label, arg);

									/* This label is also used as the label */
									/*  for the first subsource!            */
									sdef->subsrcs[0]->sh_label = sdef->sh_label;
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									sdef->description =
											STRMEM(sdef->description, arg);

									/* This description is also used as the  */
									/*  description for the first subsource! */
									sdef->subsrcs[0]->description =
											sdef->description;
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source minutes flag */
						else if ( same(cmd, FpaCminutesRequired) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								sdef->minutes_rqd = logical_arg(cline, &valid);
								if ( !valid )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCminutesRequired,
											FpaCmsgParameter);
									sdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCminutesRequired, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source type */
						else if ( same(cmd, FpaCsourceType) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
										NumFpaCsourceTypes, FpaCsourceTypes);
								if ( macro != FpaCnoMacro )
									{
									sdef->src_type = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCsourceType, FpaCmsgParameter);
									sdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCsourceType, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source name aliases */
						else if ( same(cmd, FpaCalias) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Add all aliases to ident list */
								(void) add_source_aliases(cline, sdef);
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCalias, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Cannot reset source directory tag! */
						else if ( same(cmd, FpaCdirectoryTag)
								&& !blank(sdef->src_io->src_tag) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCdirectoryTag, FpaCmsgReset);
							}

						/* Source directory tag */
						else if ( same(cmd, FpaCdirectoryTag) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( OKARG(arg) )
									{

									/* Ignore special directory tag "None" */
									if ( !same_ic(arg, FpaCnone) )
										{
										sdef->src_io->src_tag = strdup(arg);
										sdir = get_directory(sdef->src_io->src_tag);
										if ( blank(sdir) )
											{
											(void) config_file_message(FpaCblockSources,
													sdef->name,
													sdef->src_io->src_tag,
													FpaCdirectoryTag,
													FpaCmsgDirTag);
											sdef->valid = FALSE;
											}
										}
									}
								else
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCdirectoryTag, FpaCmsgParameter);
									sdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCdirectoryTag, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Cannot reset source directory path! */
						else if ( same(cmd, FpaCdirectoryPath)
								&& !blank(sdef->src_io->src_path) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCdirectoryPath, FpaCmsgReset);
							}

						/* Source directory path */
						else if ( same(cmd, FpaCdirectoryPath) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{

									/* Ignore blank strings or special */
									/*  directory path "None"          */
									if ( !blank(arg) && !same_ic(arg, FpaCnone) )
										{
										expath = env_sub(arg);
										sdef->src_io->src_path = strdup(expath);
										}
									}
								else
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCdirectoryPath,
											FpaCmsgParameter);
									sdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCdirectoryPath, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source directory layers */
						else if ( same(cmd, FpaCdirectoryLayers) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								sdef->src_io->src_layers =
										int_arg(cline, &valid);
								if ( !valid )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCdirectoryLayers,
											FpaCmsgParameter);
									sdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCdirectoryLayers, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Source subsource information */
						else if ( same(cmd, FpaCsubSources) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);

								/* Read subsources block of Sources block */
								if ( blank(arg) )
									{
									if ( !read_source_subsource_info(&fpcfg,
																		sdef) )
										{
										(void) config_file_message(FpaCblockSources,
												sdef->name, FpaCblank,
												FpaCsubSources, FpaCmsgInvalid);
										sdef->valid = FALSE;
										}
									}

								/* Otherwise only FpaCnone is acceptable */
								else
									{
									if ( !same_ic(arg, FpaCnone) )
										{
										(void) config_file_message(FpaCblockSources,
												sdef->name, FpaCblank,
												FpaCsubSources,
												FpaCmsgParameter);
										sdef->valid = FALSE;
										}

									/* But cannot reset subsources to None! */
									/* Note that the first declaration is   */
									/*  the default!                        */
									else if ( sdef->nsubsrc > 1 )
										{
										(void) config_file_message(FpaCblockSources,
												sdef->name, FpaCblank,
												FpaCsubSources,
												FpaCmsgResetNone);
										}
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCsubSources, FpaCmsgNoEqual);
								sdef->valid = FALSE;
								}
							}

						/* Skip keyword for Allied Models (for now) */
						else if ( same(cmd, FpaCalliedModel) )
							{
							(void) skip_config_file_block(&fpcfg);
							}

						/* Set error flag for unrecognized Sources keyword */
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							sdef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Error check for each member of Sources block */
	for ( nn=0; nn<NumSourceDef; nn++ )
		{
		sdef = SourceDefs[nn];

		/* Ensure that "source_type" has been set */
		if ( sdef->src_type == FpaCnoMacro )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockSources, sdef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCsourceType, sdef->name);
			sdef->valid = FALSE;
			}
		}

	/* Set flags for completion of reading */
	SourcesRead  = TRUE;
	SourcesValid = TRUE;
	return SourcesValid;
	}

/**********************************************************************/

static	LOGICAL					read_source_subsource_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING						cline, cmd, arg, expath;
	int							numbrace, section_id, section, nn, nsub;
	LOGICAL						firstline, valid;
	char						srcbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigSourceSubStruct	*subdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for SubSource section of Sources block */
	numbrace   = 0;
	section    = FpaCblockSourcesSubName;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read SubSource block of Sources block of configuration file */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of SubSource block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new SubSource declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesSubName section */
			if ( section_id == FpaCblockSourcesSubName )
				{

				/* Set declarations that are already in the list   */
				/* Note that the first declaration is the default! */
				for ( nn=1; nn<sdef->nsubsrc; nn++ )
					if ( same_ic(cmd, sdef->subsrcs[nn]->name) ) break;
				if ( nn < sdef->nsubsrc )
					{
					subdef = sdef->subsrcs[nn];
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nsub = sdef->nsubsrc++;
					sdef->subsrcs = GETMEM(sdef->subsrcs,
									FpaConfigSourceSubStruct *, sdef->nsubsrc);

					/* Initialize the declaration and set default labels */
					sdef->subsrcs[nsub] = init_source_subsource(cmd);
					subdef              = sdef->subsrcs[nsub];
					subdef->label       = strdup(cmd);
					subdef->sh_label    = strdup(cmd);
					subdef->description = NullString;
					}

				/* Set identifier for next section of SubSource declaration */
				section = FpaCblockSourcesSubInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) strcpy(srcbuf, FpaCsubSources);
				(void) strcat(srcbuf, " ");
				(void) strcat(srcbuf, cmd);
				(void) config_file_message(FpaCblockSources,
						sdef->name, srcbuf, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in SubSource declaration */
		/*  ... with format of "cmd = value(s)"    */
		else
			{

			/* Set source name for error messages */
			(void) strcpy(srcbuf, sdef->name);
			(void) strcat(srcbuf, " ");
			(void) strcat(srcbuf, subdef->name);

			/* Adding parameters in FpaCblockSourcesSubInfo section */
			if ( section_id == FpaCblockSourcesSubInfo )
				{

				/* Sub source label */
				if ( same(cmd, FpaClabel) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( ISARG(arg) )
							{
							subdef->label = STRMEM(subdef->label, arg);
							}
						else
							{
							/* Ignore missing labels, since they  */
							/*  may be from another language!     */
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								srcbuf, FpaCblank,
								FpaClabel, FpaCmsgNoEqual);
						sdef->valid = FALSE;
						valid       = FALSE;
						}
					}

				/* Sub source short label */
				else if ( same(cmd, FpaCshortLabel) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( ISARG(arg) )
							{
							subdef->sh_label = STRMEM(subdef->sh_label, arg);
							}
						else
							{
							/* Ignore missing labels, since they  */
							/*  may be from another language!     */
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								srcbuf, FpaCblank,
								FpaCshortLabel, FpaCmsgNoEqual);
						sdef->valid = FALSE;
						valid       = FALSE;
						}
					}

				/* Sub source description */
				else if ( same(cmd, FpaCdescription) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( ISARG(arg) )
							{
							subdef->description =
									STRMEM(subdef->description, arg);
							}
						else
							{
							/* Ignore missing descriptions, since  */
							/*  they may be from another language! */
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								srcbuf, FpaCblank,
								FpaCdescription, FpaCmsgNoEqual);
						sdef->valid = FALSE;
						valid       = FALSE;
						}
					}

				/* Cannot reset sub source directory path! */
				else if ( same(cmd, FpaCsubDirectoryPath)
						&& !blank(subdef->sub_path) )
					{
					(void) config_file_message(FpaCblockSources,
							srcbuf, FpaCblank,
							FpaCsubDirectoryPath, FpaCmsgReset);
					valid = FALSE;
					}

				/* Sub source directory path */
				else if ( same(cmd, FpaCsubDirectoryPath) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Ignore special sub source directory path "None" */
						arg = string_arg(cline);
						if ( same_ic(arg, FpaCnone) ) continue;

						/* Set sub source directory path */
						if ( OKARG(arg) )
							{
							expath = env_sub(arg);
							subdef->sub_path = strdup(expath);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									srcbuf, FpaCblank,
									FpaCsubDirectoryPath, FpaCmsgParameter);
							sdef->valid = FALSE;
							valid       = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								srcbuf, FpaCblank,
								FpaCsubDirectoryPath, FpaCmsgNoEqual);
						sdef->valid = FALSE;
						valid       = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							srcbuf, FpaCsubSources,
							cmd, FpaCmsgKeyword);
					sdef->valid = FALSE;
					valid       = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						srcbuf, FpaCblank, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/

static	FpaConfigSourceStruct	*read_source_allied_info

	(
	STRING		name,		/* source name */
	STRING		subname		/* subsource name */
	)

	{
	int								nblk;
	FILE							*fpcfg;
	STRING							cline, cmd, arg;
	int								numbrace, section_id, section;
	LOGICAL							firstline, valid;
	FpaConfigSourceStruct			*sdef, *sdefinfo;
	FpaConfigSourceAlliedStruct		*allied;

	/* Find the pointer to the source name */
	sdef = identify_source(name, subname);

	/* Return Null if source name not found */
	if ( IsNull(sdef) ) return NullPtr(FpaConfigSourceStruct *);

	/* Return pointer to structure if no Allied Model information to be read */
	if ( sdef->src_type != FpaC_ALLIED ) return sdef;

	/* Return pointer to structure if Allied Model information has been read */
	if ( NotNull(sdef->allied) ) return sdef;

	/* Force the Levels block of the configuration file to be read first */
	if ( !read_levels_info() ) return sdef;

	/* Force the Elements block of the configuration file to be read next */
	if ( !read_elements_info() ) return sdef;

	/* Force the Fields block of the configuration file to be read next */
	if ( !read_fields_info() ) return sdef;

	/* Force the CrossRefs block of the configuration file to be read next */
	if ( !read_crossrefs_info() ) return sdef;

	/* Add space for Allied Model information */
	sdef->valid_allied = TRUE;
	sdef->allied       = init_source_allied();
	allied             = sdef->allied;

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Sources block for \"%s\"!\n", name);

	/* Read all blocks from configuration files for this Sources declaration */
	for ( nblk=0; nblk<sdef->nblocks; nblk++ )
		{

		/* Re-open and position configuration file for this Sources block */
		fpcfg = NullPtr(FILE *);
		if ( !config_file_open(sdef->filenames[nblk], &fpcfg)
				|| fseek(fpcfg, sdef->locations[nblk], SEEK_SET) != 0 )
			{
			return NullPtr(FpaConfigSourceStruct *);
			}

		/* Diagnostic message */
		(void) pr_diag("Config", "  ... beginning at \"%d\" in file \"%s\"\n",
				sdef->locations[nblk], sdef->filenames[nblk]);

		/* Set counter and identifier for Sources declaration in Sources block */
		numbrace   = 0;
		section    = FpaCblockSourcesInfo;
		section_id = FpaCnoSection;
		firstline  = TRUE;

		/* Read block containing this Sources declaration line by line */
		while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
			{

			/* Extract the first argument from the current line */
			cmd = string_arg(cline);

			/* The first line should be an open bracket */
			if ( firstline )
				{
				firstline = FALSE;
				if ( !same(cmd, FpaCopenBrace) ) break;
				}

			/* Increment counter for open brackets */
			/*  and save the section identifier    */
			if ( same(cmd, FpaCopenBrace) )
				{
				numbrace++;
				section_id = push_section(section);
				}

			/* Decrement counter for close brackets */
			/*  and reset the section identifier    */
			else if ( same(cmd, FpaCcloseBrace) )
				{
				numbrace--;
				section_id = pop_section();

				/* Check for end of Sources block */
				if ( numbrace == 0 ) break;
				}

			/* Set parameters in Sources declaration */
			/*  ... with format of "cmd = value(s)"  */
			else if ( numbrace == 1 )
				{

				/* Adding parameters in FpaCblockSourcesInfo section */
				if ( section_id == FpaCblockSourcesInfo )
					{

					/* Source Allied Model block */
					if ( same(cmd, FpaCalliedModel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set identifier for Allied Model block of */
							/*  Sources block                           */
							if ( blank(arg) )
								{
								section = FpaCblockSourcesAllied;
								}

							/* Error if no Allied Model block! */
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCalliedModel, FpaCmsgParameter);
								sdef->valid_allied = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedModel, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Skip all other keywords */
					else
						{
						(void) skip_config_file_block(&fpcfg);
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCblank, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Sources declaration */
			/*  ... with format of "cmd = value(s)"            */
			else
				{

				/* Adding parameters in FpaCblockSourcesAllied section */
				if ( section_id == FpaCblockSourcesAllied )
					{

					/* Allied Model time matching */
					if ( same(cmd, FpaCalliedTimeMatching) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							allied->time_match = logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCalliedTimeMatching,
										FpaCmsgParameter);
								sdef->valid_allied = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedTimeMatching, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model default source information */
					else if ( same(cmd, FpaCalliedSourceInfo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Note that identify_source() is called twice */
							/*  to handle both source and subsource        */
							/*  ... calls to strdup_arg() are destructive! */
							arg      = strdup_arg(cline);
							sdefinfo = identify_source(arg, FpaCblank);
							FREEMEM(arg);
							arg      = strdup_arg(cline);
							if ( NotNull(sdefinfo) && OKARG(arg) )
								{
								sdefinfo = identify_source(sdefinfo->name, arg);
								}
							FREEMEM(arg);
							if ( NotNull(sdefinfo) )
								{
								allied->src_def = sdefinfo;
								allied->sub_def = sdefinfo->src_sub;
								}
							else
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCblank,
										FpaCalliedSourceInfo, FpaCmsgParameter);
								sdef->valid_allied = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedSourceInfo, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model preprocessing run string */
					else if ( same(cmd, FpaCalliedPreProcess) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							allied->pre_process =
									STRMEM(allied->pre_process, cline);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedPreProcess, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model processing run string */
					else if ( same(cmd, FpaCalliedProcess) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							allied->process =
									STRMEM(allied->process, cline);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedProcess, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model postprocessing run string */
					else if ( same(cmd, FpaCalliedPostProcess) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							allied->post_process =
									STRMEM(allied->post_process, cline);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedPostProcess, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model program information */
					else if ( same(cmd, FpaCalliedPrograms) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Read Allied Model Programs block */
							/*  of Sources block                */
							if ( blank(arg) )
								{
								if ( !read_allied_programs_info(&fpcfg, sdef) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedPrograms, FpaCmsgInvalid);
									sdef->valid_allied = FALSE;
									}
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedPrograms,
											FpaCmsgParameter);
									sdef->valid_allied = FALSE;
									}

								/* But cannot reset programs to None! */
								else if ( NotNull(sdef->allied->programs)
										&& sdef->allied->programs->nprogs > 0 )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedPrograms,
											FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedPrograms, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model file information */
					else if ( same(cmd, FpaCalliedFiles) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Read Allied Model Files block */
							/*  of Sources block             */
							if ( blank(arg) )
								{
								if ( !read_allied_files_info(&fpcfg, sdef) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedFiles, FpaCmsgInvalid);
									sdef->valid_allied = FALSE;
									}
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedFiles, FpaCmsgParameter);
									sdef->valid_allied = FALSE;
									}

								/* But cannot reset files to None! */
								else if ( NotNull(sdef->allied->files)
										&& sdef->allied->files->nfiles > 0 )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedFiles, FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedFiles, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model required fields information */
					else if ( same(cmd, FpaCalliedRequiredFields) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Read Allied Model Required Fields block */
							/*  of Sources block                       */
							if ( blank(arg) )
								{
								if ( !read_allied_fields_info(&fpcfg, sdef) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredFields,
											FpaCmsgInvalid);
									sdef->valid_allied = FALSE;
									}
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredFields,
											FpaCmsgParameter);
									sdef->valid_allied = FALSE;
									}

								/* But cannot reset required fields to None! */
								else if ( NotNull(sdef->allied->fields)
										&& sdef->allied->fields->nfields > 0 )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredFields,
											FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedRequiredFields, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model required wind crossref information */
					else if ( same(cmd, FpaCalliedRequiredWinds) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Read Allied Model Required Winds block */
							/*  of Sources block                      */
							if ( blank(arg) )
								{
								if ( !read_allied_winds_info(&fpcfg, sdef) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredWinds,
											FpaCmsgInvalid);
									sdef->valid_allied = FALSE;
									}
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredWinds,
											FpaCmsgParameter);
									sdef->valid_allied = FALSE;
									}

								/* But cannot reset required wind crossrefs */
								/*  to None!                                */
								else if ( NotNull(sdef->allied->winds)
										&& sdef->allied->winds->nwinds > 0 )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredWinds,
											FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedRequiredWinds, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model required value crossref information */
					else if ( same(cmd, FpaCalliedRequiredValues) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Read Allied Model Required Values block */
							/*  of Sources block                       */
							if ( blank(arg) )
								{
								if ( !read_allied_values_info(&fpcfg, sdef) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredValues,
											FpaCmsgInvalid);
									sdef->valid_allied = FALSE;
									}
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredValues,
											FpaCmsgParameter);
									sdef->valid_allied = FALSE;
									}

								/* But cannot reset required value crossrefs */
								/*  to None!                                 */
								else if ( NotNull(sdef->allied->values)
										&& sdef->allied->values->nvalues > 0 )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedRequiredValues,
											FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedRequiredValues, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Allied Model metafile information */
					else if ( same(cmd, FpaCalliedMetafiles) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Read Allied Model Metafiles block */
							/*  of Sources block                 */
							if ( blank(arg) )
								{
								if ( !read_allied_metafiles_info(&fpcfg, sdef) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedMetafiles,
											FpaCmsgInvalid);
									sdef->valid_allied = FALSE;
									}
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedMetafiles,
											FpaCmsgParameter);
									sdef->valid_allied = FALSE;
									}

								/* But cannot reset metafiles to None! */
								else if ( NotNull(sdef->allied->metafiles)
										&& sdef->allied->metafiles->nfiles > 0 )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name, FpaCblank,
											FpaCalliedMetafiles,
											FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCblank,
									FpaCalliedMetafiles, FpaCmsgNoEqual);
							sdef->valid_allied = FALSE;
							}
						}

					/* Set error flag for unrecognized Sources keyword */
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedModel,
								cmd, FpaCmsgKeyword);
						sdef->valid_allied = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCblank, FpaCblank, FpaCmsgSection);
					}
				}
			}

		/* Diagnostic message */
		(void) pr_diag("Config", "  ... ending at \"%d\" in file \"%s\"\n",
				ftell(fpcfg), sdef->filenames[nblk]);

		/* Close the configuration file for this Sources declaration */
		(void) config_file_close(&fpcfg);
		}

	/* Return pointer when all configuration files have been read */
	return sdef;
	}

/**********************************************************************/

static	LOGICAL					read_allied_programs_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING							cline, cmd, arg, sdir, expath;
	int								numbrace, section_id, section, nn, nprog;
	LOGICAL							firstline, valid;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedProgramsStruct	*programs;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Return FALSE if no structure for Allied Models */
	allied = sdef->allied;
	if ( IsNull(allied) ) return FALSE;

	/* Initialize AlliedPrograms structure (if required) */
	if ( IsNull(allied->programs) ) allied->programs = init_allied_programs();
	programs = allied->programs;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for Allied Model Program section */
	/*  of Sources block                                           */
	numbrace   = 0;
	section    = FpaCblockSourcesProgram;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read Allied Model Program block of Sources block of configuration file */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Allied Model Program block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new Allied Model Program declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesProgram section */
			if ( section_id == FpaCblockSourcesProgram )
				{

				/* Set declarations that are already in the list */
				for ( nn=0; nn<programs->nprogs; nn++ )
					if ( same_ic(cmd, programs->aliases[nn]) ) break;
				if ( nn < programs->nprogs )
					{
					nprog = nn;
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nprog = programs->nprogs++;
					programs->aliases    = GETMEM(programs->aliases,    STRING,
															programs->nprogs);
					programs->src_tags   = GETMEM(programs->src_tags,   STRING,
															programs->nprogs);
					programs->prog_paths = GETMEM(programs->prog_paths, STRING,
															programs->nprogs);

					/* Initialize the declaration                          */
					/* Note that source directory tag for the program is   */
					/*  initialized with the Source default value, and     */
					/*  that the Source default value cannot be redefined! */
					programs->aliases[nprog]    = strdup(cmd);
					programs->src_tags[nprog]   = strdup(sdef->src_io->src_tag);
					programs->prog_paths[nprog] = NullString;
					}

				/* Set identifier for next section of Allied Model Program */
				/*  declaration                                            */
				section = FpaCblockSourcesProgramInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in Allied Model Program declaration */
		/*  ... with format of "cmd = value(s)"               */
		else

			{
			/* Adding parameters in FpaCblockSourcesProgramInfo section */
			if ( section_id == FpaCblockSourcesProgramInfo )
				{

				/* Program directory tag */
				if ( same(cmd, FpaCdirectoryTag) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( OKARG(arg) )
							{
							/* Ignore special directory tag "None" */
							if ( same_ic(arg, FpaCnone) )
								{
								programs->src_tags[nprog] =
										SETSTR(programs->src_tags[nprog],
												FpaCblank);
								}
							else
								{
								programs->src_tags[nprog] =
										STRMEM(programs->src_tags[nprog], arg);
								sdir = get_directory(programs->src_tags[nprog]);
								if ( blank(sdir) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name,
											programs->src_tags[nprog],
											FpaCdirectoryTag, FpaCmsgDirTag);
									sdef->valid_allied = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedPrograms,
									FpaCdirectoryTag, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedPrograms,
								FpaCdirectoryTag, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Program file path */
				else if ( same(cmd, FpaCalliedProgramPath) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( OKARG(arg) )
							{
							expath = env_sub(arg);
							programs->prog_paths[nprog] =
									STRMEM(programs->prog_paths[nprog], expath);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedPrograms,
									FpaCalliedProgramPath, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedPrograms,
								FpaCalliedProgramPath, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedPrograms,
							cmd, FpaCmsgKeyword);
					sdef->valid_allied = FALSE;
					valid              = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/

static	LOGICAL					read_allied_files_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING							cline, cmd, arg, sdir, expath;
	int								numbrace, section_id, section, nn, nfile;
	LOGICAL							firstline, valid;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedFilesStruct		*files;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Return FALSE if no structure for Allied Models */
	allied = sdef->allied;
	if ( IsNull(allied) ) return FALSE;

	/* Initialize AlliedFiles structure (if required) */
	if ( IsNull(allied->files) ) allied->files = init_allied_files();
	files = allied->files;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for Allied Model Files section */
	/*  of Sources block                                         */
	numbrace   = 0;
	section    = FpaCblockSourcesFile;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read Allied Model File block of Sources block of configuration file */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Allied Model File block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new Allied Model File declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesFile section */
			if ( section_id == FpaCblockSourcesFile )
				{

				/* Set declarations that are already in the list */
				for ( nn=0; nn<files->nfiles; nn++ )
					if ( same_ic(cmd, files->aliases[nn]) ) break;
				if ( nn < files->nfiles )
					{
					nfile = nn;
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nfile = files->nfiles++;
					files->aliases    = GETMEM(files->aliases,    STRING,
															files->nfiles);
					files->src_tags   = GETMEM(files->src_tags,   STRING,
															files->nfiles);
					files->file_paths = GETMEM(files->file_paths, STRING,
															files->nfiles);

					/* Initialize the declaration                          */
					/* Note that source directory tag for the file is      */
					/*  initialized with the Source default value, and     */
					/*  that the Source default value cannot be redefined! */
					files->aliases[nfile]    = strdup(cmd);
					files->src_tags[nfile]   = strdup(sdef->src_io->src_tag);
					files->file_paths[nfile] = NullString;
					}

				/* Set identifier for next section of Allied Model File */
				/*  declaration                                         */
				section = FpaCblockSourcesFileInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in Allied Model File declaration */
		/*  ... with format of "cmd = value(s)"            */
		else

			{
			/* Adding parameters in FpaCblockSourcesFileInfo section */
			if ( section_id == FpaCblockSourcesFileInfo )
				{

				/* File directory tag */
				if ( same(cmd, FpaCdirectoryTag) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( OKARG(arg) )
							{
							/* Ignore special directory tag "None" */
							if ( same_ic(arg, FpaCnone) )
								{
								files->src_tags[nfile] =
										SETSTR(files->src_tags[nfile],
												FpaCblank);
								}
							else
								{
								files->src_tags[nfile] =
										STRMEM(files->src_tags[nfile], arg);
								sdir = get_directory(files->src_tags[nfile]);
								if ( blank(sdir) )
									{
									(void) config_file_message(FpaCblockSources,
											sdef->name,
											files->src_tags[nfile],
											FpaCdirectoryTag, FpaCmsgDirTag);
									sdef->valid_allied = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedFiles,
									FpaCdirectoryTag, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedFiles,
								FpaCdirectoryTag, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* File path */
				else if ( same(cmd, FpaCalliedFilePath) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( OKARG(arg) )
							{
							expath = env_sub(arg);
							files->file_paths[nfile] =
									STRMEM(files->file_paths[nfile], expath);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedFiles,
									FpaCalliedFilePath, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedFiles,
								FpaCalliedFilePath, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedFiles,
							cmd, FpaCmsgKeyword);
					sdef->valid_allied = FALSE;
					valid              = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/

static	LOGICAL					read_allied_fields_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING							cline, cmd, arg, argt, arga;
	int								numbrace, section_id, section, nn;
	int								nfield, natt, fkind;
	LOGICAL							firstline, valid;
	LOGICAL							argok, validarg, vargt, varga;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigElementStruct			*edef;
	FpaConfigLevelStruct			*ldef;
	FpaConfigFieldStruct			*fdef;
	FpaConfigSourceStruct			*sdefinfo;
	FpaConfigUnitStruct				*udef;
	FpaConfigAlliedAttribStruct		*attinfo;
	FpaConfigAlliedAttribStruct		*nodeinfo;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Return FALSE if no structure for Allied Models */
	allied = sdef->allied;
	if ( IsNull(allied) ) return FALSE;

	/* Initialize AlliedFields structure (if required) */
	if ( IsNull(allied->fields) ) allied->fields = init_allied_fields();
	fields = allied->fields;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for Allied Model Required Fields section */
	/*  of Sources block                                                   */
	numbrace   = 0;
	section    = FpaCblockSourcesField;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read Allied Model Required Fields block of Sources block */
	/*  of configuration file                                   */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Allied Model Required Fields block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new Allied Model Required Fields declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesField section */
			if ( section_id == FpaCblockSourcesField )
				{

				/* Set declarations that are already in the list */
				for ( nn=0; nn<fields->nfields; nn++ )
					if ( same_ic(cmd, fields->aliases[nn]) ) break;
				if ( nn < fields->nfields )
					{
					nfield = nn;
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nfield = fields->nfields++;
					fields->aliases    = GETMEM(fields->aliases,
								STRING,                      fields->nfields);
					fields->flds       = GETMEM(fields->flds,
								FpaConfigFieldStruct *,      fields->nfields);
					fields->ftypes     = GETMEM(fields->ftypes,
								int,                         fields->nfields);
					fields->attinfo    = GETMEM(fields->attinfo,
								FpaConfigAlliedAttribStruct *, fields->nfields);
					fields->nodeinfo   = GETMEM(fields->nodeinfo,
								FpaConfigAlliedAttribStruct *, fields->nfields);
					fields->sub_fields = GETMEM(fields->sub_fields,
								STRING,                      fields->nfields);
					fields->sub_units  = GETMEM(fields->sub_units,
								FpaConfigUnitStruct *,       fields->nfields);
					fields->src_defs   = GETMEM(fields->src_defs,
								FpaConfigSourceStruct *,     fields->nfields);
					fields->sub_defs   = GETMEM(fields->sub_defs,
								FpaConfigSourceSubStruct *,  fields->nfields);

					/* Initialize the declaration */
					fields->aliases[nfield]    = strdup(cmd);
					fields->flds[nfield]       = NullPtr(FpaConfigFieldStruct *);
					fields->ftypes[nfield]     = FpaCnoMacro;
					fields->attinfo[nfield]    = INITMEM(FpaConfigAlliedAttribStruct, 1);;
					fields->nodeinfo[nfield]   = INITMEM(FpaConfigAlliedAttribStruct, 1);;
					fields->sub_fields[nfield] = NullString;
					fields->sub_units[nfield]  = NullPtr(FpaConfigUnitStruct *);
					fields->src_defs[nfield]   = NullPtr(FpaConfigSourceStruct *);
					fields->sub_defs[nfield]   = NullPtr(FpaConfigSourceSubStruct *);

					/* Initialize the attribute structure */
					fields->attinfo[nfield]->nattribs  = 0;
					fields->attinfo[nfield]->tag       = NullStringList;
					fields->attinfo[nfield]->attname   = NullStringList;
					fields->attinfo[nfield]->attunit   = NullPtr(FpaConfigUnitStruct**);

					/* Initialize the node attribute structure */
					fields->nodeinfo[nfield]->nattribs = 0;
					fields->nodeinfo[nfield]->tag      = NullStringList;
					fields->nodeinfo[nfield]->attname  = NullStringList;
					fields->nodeinfo[nfield]->attunit  = NullPtr(FpaConfigUnitStruct**);
					}

				/* Set identifier for next section of Allied Model */
				/*  Required Fields declaration                    */
				section = FpaCblockSourcesFieldInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in Allied Model Required Fields declaration */
		/*  ... with format of "cmd = value(s)"                       */
		else

			{
			/* Adding parameters in FpaCblockSourcesFieldInfo section */
			if ( section_id == FpaCblockSourcesFieldInfo )
				{

				/* Required field identifier */
				if ( same(cmd, FpaCalliedFieldInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Check for the named element */
						arg  = strdup_arg(cline);
						edef = identify_element(arg);
						FREEMEM(arg);

						/* Check for the named level */
						arg  = strdup_arg(cline);
						ldef = identify_level(arg);
						FREEMEM(arg);

						/* Error message if unrecognizable element or level */
						/*  or error identifying field                      */
						if ( IsNull(edef) || IsNull(ldef)
								|| !consistent_element_and_level(edef, ldef)
								|| IsNull(fdef = identify_field(edef->name,
																ldef->name)) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedFieldInfo, FpaCmsgField);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Identify field from element and level */
						else
							{
							fields->flds[nfield] = fdef;
							}

						/* Check for optional field type */
						if ( !blank(cline) )
							{
							arg   = strdup_arg(cline);
							fkind = field_data_type(arg);
							FREEMEM(arg);

							/* Error message for unrecognizable field type */
							if ( fkind == FpaCnoMacro )
								{
								(void) config_file_message(FpaCblockSources,
										sdef->name, FpaCalliedRequiredFields,
										FpaCalliedFieldInfo, FpaCmsgField);
								sdef->valid_allied = FALSE;
								valid              = FALSE;
								}

							/* Identify field type */
							else
								{
								fields->ftypes[nfield] = fkind;
								}
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedFieldInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Reset required field attribute identifiers */
				else if ( same(cmd, FpaCalliedAttribInfoReset) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Reset required field attribute identifiers */
						validarg = logical_arg(cline, &argok);
						if ( validarg )
							{
							attinfo = fields->attinfo[nfield];
							for ( natt=0; natt<attinfo->nattribs; natt++ )
								{
								FREEMEM(attinfo->tag[natt]);
								FREEMEM(attinfo->attname[natt]);
								attinfo->attunit[natt] = NullPtr(FpaConfigUnitStruct *);
								}
							FREEMEM(attinfo->tag);
							FREEMEM(attinfo->attname);
							FREEMEM(attinfo->attunit);
							attinfo->nattribs = 0;
							}
						if ( !argok )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedAttribInfoReset,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedAttribInfoReset, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Add another required field attribute identifier */
				else if ( same(cmd, FpaCalliedAttribInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Set the attribute tag */
						argt  = strdup_arg(cline);
						vargt = OKARG(argt);
						if ( !vargt )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedAttribInfoTag,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the attribute name */
						arga  = strdup_arg(cline);
						varga = OKARG(arga);
						if ( !varga )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedAttribInfoName,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the attribute units */
						arg   = strdup_arg(cline);
						udef  = identify_unit(arg);
						if ( IsNull(udef) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedAttribInfoUnit,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						FREEMEM(arg);

						/* Add information for another attribute */
						if ( vargt && varga && NotNull(udef) )
							{
							attinfo = fields->attinfo[nfield];
							natt    = attinfo->nattribs++;
							attinfo->tag     = GETMEM(attinfo->tag,
									STRING,                natt+1);
							attinfo->attname = GETMEM(attinfo->attname,
									STRING,                natt+1);
							attinfo->attunit = GETMEM(attinfo->attunit,
									FpaConfigUnitStruct *, natt+1);
							attinfo->tag[natt]     = argt;
							attinfo->attname[natt] = arga;
							attinfo->attunit[natt] = udef;
							}

						/* Otherwise, free space used by attribute information */
						else
							{
							FREEMEM(argt);
							FREEMEM(arga);
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedAttribInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Reset required field node attribute identifiers */
				else if ( same(cmd, FpaCalliedNodeAttribInfoReset) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Reset required field node attribute identifiers */
						validarg = logical_arg(cline, &argok);
						if ( validarg )
							{
							nodeinfo = fields->nodeinfo[nfield];
							for ( natt=0; natt<nodeinfo->nattribs; natt++ )
								{
								FREEMEM(nodeinfo->tag[natt]);
								FREEMEM(nodeinfo->attname[natt]);
								nodeinfo->attunit[natt] = NullPtr(FpaConfigUnitStruct *);
								}
							FREEMEM(nodeinfo->tag);
							FREEMEM(nodeinfo->attname);
							FREEMEM(nodeinfo->attunit);
							nodeinfo->nattribs = 0;
							}
						if ( !argok )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedNodeAttribInfoReset,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedNodeAttribInfoReset, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Add another required field node attribute identifier */
				else if ( same(cmd, FpaCalliedNodeAttribInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Set the node attribute tag */
						argt  = strdup_arg(cline);
						vargt = OKARG(argt);
						if ( !vargt )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedNodeAttribInfoTag,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the node attribute name */
						arga  = strdup_arg(cline);
						varga = OKARG(arga);
						if ( !varga )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedNodeAttribInfoName,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the node attribute units */
						arg   = strdup_arg(cline);
						udef  = identify_unit(arg);
						if ( IsNull(udef) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedNodeAttribInfoUnit,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						FREEMEM(arg);

						/* Add information for another node attribute */
						if ( vargt && varga && NotNull(udef) )
							{
							nodeinfo = fields->nodeinfo[nfield];
							natt     = nodeinfo->nattribs++;
							nodeinfo->tag     = GETMEM(nodeinfo->tag,
									STRING,                natt+1);
							nodeinfo->attname = GETMEM(nodeinfo->attname,
									STRING,                natt+1);
							nodeinfo->attunit = GETMEM(nodeinfo->attunit,
									FpaConfigUnitStruct *, natt+1);
							nodeinfo->tag[natt]     = argt;
							nodeinfo->attname[natt] = arga;
							nodeinfo->attunit[natt] = udef;
							}

						/* Otherwise, free space used by attribute information */
						else
							{
							FREEMEM(argt);
							FREEMEM(arga);
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedNodeAttribInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Required field sub-field identifier */
				else if ( same(cmd, FpaCalliedSubFieldInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Set the sub-field name */
						arg      = string_arg(cline);
						validarg = OKARG(arg);
						if ( validarg )
							{
							fields->sub_fields[nfield] =
									STRMEM(fields->sub_fields[nfield], arg);
							}

						/* Set the sub-field units */
						arg                       = strdup_arg(cline);
						fields->sub_units[nfield] = identify_unit(arg);
						FREEMEM(arg);

						if ( !validarg )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedSubFieldInfoVal,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						if ( IsNull(fields->sub_units[nfield]) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedSubFieldInfoUnit,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedSubFieldInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Required field source information */
				else if ( same(cmd, FpaCalliedSourceInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Note that identify_source() is called twice */
						/*  to handle both source and subsource        */
						/*  ... calls to strdup_arg() are destructive! */
						arg      = strdup_arg(cline);
						sdefinfo = identify_source(arg, FpaCblank);
						FREEMEM(arg);
						arg      = strdup_arg(cline);
						if ( NotNull(sdefinfo) && OKARG(arg) )
							{
							sdefinfo = identify_source(sdefinfo->name, arg);
							}
						FREEMEM(arg);
						if ( NotNull(sdefinfo) )
							{
							fields->src_defs[nfield] = sdefinfo;
							fields->sub_defs[nfield] = sdefinfo->src_sub;
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredFields,
									FpaCalliedSourceInfo, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredFields,
								FpaCalliedSourceInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedRequiredFields,
							cmd, FpaCmsgKeyword);
					sdef->valid_allied = FALSE;
					valid              = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/

static	LOGICAL					read_allied_winds_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING							cline, cmd, arg;
	int								numbrace, section_id, section, nn, nwind;
	LOGICAL							firstline, valid;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedWindsStruct		*winds;
	FpaConfigCrossRefStruct			*crdef;
	FpaConfigSourceStruct			*sdefinfo;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Return FALSE if no structure for Allied Models */
	allied = sdef->allied;
	if ( IsNull(allied) ) return FALSE;

	/* Initialize AlliedWinds structure (if required) */
	if ( IsNull(allied->winds) ) allied->winds = init_allied_winds();
	winds = allied->winds;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for Allied Model Required Winds section */
	/*  of Sources block                                                  */
	numbrace   = 0;
	section    = FpaCblockSourcesWind;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read Allied Model Required Winds block of Sources block */
	/*  of configuration file                                  */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Allied Model Required Winds block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new Allied Model Required Winds declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesWind section */
			if ( section_id == FpaCblockSourcesWind )
				{

				/* Set declarations that are already in the list */
				for ( nn=0; nn<winds->nwinds; nn++ )
					if ( same_ic(cmd, winds->aliases[nn]) ) break;
				if ( nn < winds->nwinds )
					{
					nwind = nn;
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nwind = winds->nwinds++;
					winds->aliases  = GETMEM(winds->aliases,
								STRING,                     winds->nwinds);
					winds->wcrefs   = GETMEM(winds->wcrefs,
								FpaConfigCrossRefStruct *,  winds->nwinds);
					winds->src_defs = GETMEM(winds->src_defs,
								FpaConfigSourceStruct *,    winds->nwinds);
					winds->sub_defs = GETMEM(winds->sub_defs,
								FpaConfigSourceSubStruct *, winds->nwinds);

					/* Initialize the declaration */
					winds->aliases[nwind]  = strdup(cmd);
					winds->wcrefs[nwind]   = NullPtr(FpaConfigCrossRefStruct *);
					winds->src_defs[nwind] = NullPtr(FpaConfigSourceStruct *);
					winds->sub_defs[nwind] = NullPtr(FpaConfigSourceSubStruct *);
					}

				/* Set identifier for next section of Allied Model */
				/*  Required Winds declaration                     */
				section = FpaCblockSourcesWindInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in Allied Model Required Winds declaration */
		/*  ... with format of "cmd = value(s)"                      */
		else

			{
			/* Adding parameters in FpaCblockSourcesWindInfo section */
			if ( section_id == FpaCblockSourcesWindInfo )
				{

				/* Required wind crossreference identifier */
				if ( same(cmd, FpaCalliedCrossRefInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg   = strdup_arg(cline);
						crdef = identify_crossref(FpaCcRefsWinds, arg);
						FREEMEM(arg);
						if ( NotNull(crdef) )
							{
							winds->wcrefs[nwind] = crdef;
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredWinds,
									FpaCalliedCrossRefInfo, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredWinds,
								FpaCalliedCrossRefInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Required wind crossreference source information */
				else if ( same(cmd, FpaCalliedSourceInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Note that identify_source() is called twice */
						/*  to handle both source and subsource        */
						/*  ... calls to strdup_arg() are destructive! */
						arg      = strdup_arg(cline);
						sdefinfo = identify_source(arg, FpaCblank);
						FREEMEM(arg);
						arg      = strdup_arg(cline);
						if ( NotNull(sdefinfo) && OKARG(arg) )
							{
							sdefinfo = identify_source(sdefinfo->name, arg);
							}
						FREEMEM(arg);
						if ( NotNull(sdefinfo) )
							{
							winds->src_defs[nwind] = sdefinfo;
							winds->sub_defs[nwind] = sdefinfo->src_sub;
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredWinds,
									FpaCalliedSourceInfo, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredWinds,
								FpaCalliedSourceInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedRequiredWinds,
							cmd, FpaCmsgKeyword);
					sdef->valid_allied = FALSE;
					valid              = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/

static	LOGICAL					read_allied_values_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING							cline, cmd, arg;
	int								numbrace, section_id, section, nn, nvalue;
	LOGICAL							firstline, valid;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedValuesStruct		*values;
	FpaConfigCrossRefStruct			*crdef;
	FpaConfigSourceStruct			*sdefinfo;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Return FALSE if no structure for Allied Models */
	allied = sdef->allied;
	if ( IsNull(allied) ) return FALSE;

	/* Initialize AlliedValues structure (if required) */
	if ( IsNull(allied->values) ) allied->values = init_allied_values();
	values = allied->values;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for Allied Model Required Values section */
	/*  of Sources block                                                   */
	numbrace   = 0;
	section    = FpaCblockSourcesValue;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read Allied Model Required Values block of Sources block */
	/*  of configuration file                                  */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Allied Model Required Values block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new Allied Model Required Values declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesValue section */
			if ( section_id == FpaCblockSourcesValue )
				{

				/* Set declarations that are already in the list */
				for ( nn=0; nn<values->nvalues; nn++ )
					if ( same_ic(cmd, values->aliases[nn]) ) break;
				if ( nn < values->nvalues )
					{
					nvalue = nn;
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nvalue = values->nvalues++;
					values->aliases  = GETMEM(values->aliases,
								STRING,                     values->nvalues);
					values->vcrefs   = GETMEM(values->vcrefs,
								FpaConfigCrossRefStruct *,  values->nvalues);
					values->src_defs = GETMEM(values->src_defs,
								FpaConfigSourceStruct *,    values->nvalues);
					values->sub_defs = GETMEM(values->sub_defs,
								FpaConfigSourceSubStruct *, values->nvalues);

					/* Initialize the declaration */
					values->aliases[nvalue]  = strdup(cmd);
					values->vcrefs[nvalue]   = NullPtr(FpaConfigCrossRefStruct *);
					values->src_defs[nvalue] = NullPtr(FpaConfigSourceStruct *);
					values->sub_defs[nvalue] = NullPtr(FpaConfigSourceSubStruct *);
					}

				/* Set identifier for next section of Allied Model  */
				/*  Required Values declaration                     */
				section = FpaCblockSourcesValueInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in Allied Model Required Values declaration */
		/*  ... with format of "cmd = value(s)"                       */
		else

			{
			/* Adding parameters in FpaCblockSourcesValueInfo section */
			if ( section_id == FpaCblockSourcesValueInfo )
				{

				/* Required value crossreference identifier */
				if ( same(cmd, FpaCalliedCrossRefInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg   = strdup_arg(cline);
						crdef = identify_crossref(FpaCcRefsValues, arg);
						FREEMEM(arg);
						if ( NotNull(crdef) )
							{
							values->vcrefs[nvalue] = crdef;
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredValues,
									FpaCalliedCrossRefInfo, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredValues,
								FpaCalliedCrossRefInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Required value crossreference source information */
				else if ( same(cmd, FpaCalliedSourceInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Note that identify_source() is called twice */
						/*  to handle both source and subsource        */
						/*  ... calls to strdup_arg() are destructive! */
						arg      = strdup_arg(cline);
						sdefinfo = identify_source(arg, FpaCblank);
						FREEMEM(arg);
						arg      = strdup_arg(cline);
						if ( NotNull(sdefinfo) && OKARG(arg) )
							{
							sdefinfo = identify_source(sdefinfo->name, arg);
							}
						FREEMEM(arg);
						if ( NotNull(sdefinfo) )
							{
							values->src_defs[nvalue] = sdefinfo;
							values->sub_defs[nvalue] = sdefinfo->src_sub;
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedRequiredValues,
									FpaCalliedSourceInfo, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedRequiredValues,
								FpaCalliedSourceInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedRequiredValues,
							cmd, FpaCmsgKeyword);
					sdef->valid_allied = FALSE;
					valid              = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/**********************************************************************/

static	LOGICAL					read_allied_metafiles_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING							cline, cmd, arg, argt, arga, argx;
	int								numbrace, section_id, section, nn;
	int								nfile, natt;
	LOGICAL							firstline, valid;
	LOGICAL							argok, validarg, vargt, varga, vargx;
	FpaConfigSourceAlliedStruct		*allied;
	FpaConfigAlliedMetafilesStruct	*metafiles;
	FpaConfigElementStruct			*edef;
	FpaConfigLevelStruct			*ldef;
	FpaConfigFieldStruct			*fdef;
	FpaConfigUnitStruct				*udef;
	FpaConfigAlliedAttribStruct		*attinfo;
	FpaConfigAlliedDefAttribStruct	*definfo;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Return FALSE if no structure for Allied Models */
	allied = sdef->allied;
	if ( IsNull(allied) ) return FALSE;

	/* Initialize AlliedMetafiles structure (if required) */
	if ( IsNull(allied->metafiles) ) allied->metafiles = init_allied_metafiles();
	metafiles = allied->metafiles;

	/* Set error checking parameter */
	valid = TRUE;

	/* Set counter and identifier for Allied Model Metafiles section */
	/*  of Sources block                                             */
	numbrace   = 0;
	section    = FpaCblockSourcesMetafile;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read Allied Model Metafile block of Sources block of configuration file */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of Allied Model Metafile block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new Allied Model Metafile declaration */
		else if ( numbrace == 1 )
			{

			/* Adding another name in FpaCblockSourcesMetafile section */
			if ( section_id == FpaCblockSourcesMetafile )
				{

				/* Set declarations that are already in the list */
				for ( nn=0; nn<metafiles->nfiles; nn++ )
					if ( same_ic(cmd, metafiles->aliases[nn]) ) break;
				if ( nn < metafiles->nfiles )
					{
					nfile = nn;
					}

				/* Add declarations that are not already in the list */
				else
					{

					/* Add space for another declaration */
					nfile = metafiles->nfiles++;
					metafiles->aliases      = GETMEM(metafiles->aliases,
								STRING,                        metafiles->nfiles);
					metafiles->file_aliases = GETMEM(metafiles->file_aliases,
								STRING,                        metafiles->nfiles);
					metafiles->flds         = GETMEM(metafiles->flds,
								FpaConfigFieldStruct *,        metafiles->nfiles);
					metafiles->attinfo      = GETMEM(metafiles->attinfo,
								FpaConfigAlliedAttribStruct *, metafiles->nfiles);
					metafiles->definfo      = GETMEM(metafiles->definfo,
								FpaConfigAlliedDefAttribStruct *, metafiles->nfiles);

					/* Initialize the declaration */
					metafiles->aliases[nfile]      = strdup(cmd);
					metafiles->file_aliases[nfile] = NullString;
					metafiles->flds[nfile]         = NullPtr(FpaConfigFieldStruct *);
					metafiles->attinfo[nfile]      = INITMEM(FpaConfigAlliedAttribStruct, 1);;
					metafiles->definfo[nfile]      = INITMEM(FpaConfigAlliedDefAttribStruct, 1);;

					/* Initialize the attribute structure */
					metafiles->attinfo[nfile]->nattribs  = 0;
					metafiles->attinfo[nfile]->tag       = NullStringList;
					metafiles->attinfo[nfile]->attname   = NullStringList;
					metafiles->attinfo[nfile]->attunit   = NullPtr(FpaConfigUnitStruct**);

					/* Initialize the default attribute structure */
					metafiles->definfo[nfile]->natt_defs    = 0;
					metafiles->definfo[nfile]->attname_defs = NullStringList;
					metafiles->definfo[nfile]->attval_defs  = NullStringList;
					}

				/* Set identifier for next section of Allied Model Metafile */
				/*  declaration                                             */
				section = FpaCblockSourcesMetafileInfo;
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}

		/* Set parameters in Allied Model Metafile declaration */
		/*  ... with format of "cmd = value(s)"                */
		else

			{
			/* Adding parameters in FpaCblockSourcesMetafileInfo section */
			if ( section_id == FpaCblockSourcesMetafileInfo )
				{

				/* Metafile input file alias */
				if ( same(cmd, FpaCalliedFileAlias) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{
						arg = string_arg(cline);
						if ( OKARG(arg) )
							{
							metafiles->file_aliases[nfile] =
									STRMEM(metafiles->file_aliases[nfile], arg);
							}
						else
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedFileAlias, FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedMetafiles,
								FpaCalliedFileAlias, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Metafile field identifier */
				else if ( same(cmd, FpaCalliedFieldInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Check for the named element */
						arg  = strdup_arg(cline);
						edef = identify_element(arg);
						FREEMEM(arg);

						/* Check for the named level */
						arg  = strdup_arg(cline);
						ldef = identify_level(arg);
						FREEMEM(arg);

						/* Error message if unrecognizable element or level */
						/*  or error identifying field                      */
						if ( IsNull(edef) || IsNull(ldef)
								|| !consistent_element_and_level(edef, ldef)
								|| IsNull(fdef = identify_field(edef->name,
																ldef->name)) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedFieldInfo, FpaCmsgField);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Identify field from element and level */
						else
							{
							metafiles->flds[nfile] = fdef;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedMetafiles,
								FpaCalliedFieldInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Reset metafile attribute identifiers */
				else if ( same(cmd, FpaCalliedAttribInfoReset) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Reset metafile attribute identifiers */
						validarg = logical_arg(cline, &argok);
						if ( validarg )
							{
							attinfo = metafiles->attinfo[nfile];
							for ( natt=0; natt<attinfo->nattribs; natt++ )
								{
								FREEMEM(attinfo->tag[natt]);
								FREEMEM(attinfo->attname[natt]);
								attinfo->attunit[natt] = NullPtr(FpaConfigUnitStruct *);
								}
							FREEMEM(attinfo->tag);
							FREEMEM(attinfo->attname);
							FREEMEM(attinfo->attunit);
							attinfo->nattribs = 0;
							}
						if ( !argok )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedAttribInfoReset,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedMetafiles,
								FpaCalliedAttribInfoReset, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Add another metafile attribute identifier */
				else if ( same(cmd, FpaCalliedAttribInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Set the attribute tag */
						argt  = strdup_arg(cline);
						vargt = OKARG(argt);
						if ( !vargt )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedAttribInfoTag,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the attribute name */
						arga  = strdup_arg(cline);
						varga = OKARG(arga);
						if ( !varga )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedAttribInfoName,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the attribute units */
						arg   = strdup_arg(cline);
						udef  = identify_unit(arg);
						if ( IsNull(udef) )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedAttribInfoUnit,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						FREEMEM(arg);

						/* Add information for another attribute */
						if ( vargt && varga && NotNull(udef) )
							{
							attinfo = metafiles->attinfo[nfile];
							natt    = attinfo->nattribs++;
							attinfo->tag     = GETMEM(attinfo->tag,
									STRING,                natt+1);
							attinfo->attname = GETMEM(attinfo->attname,
									STRING,                natt+1);
							attinfo->attunit = GETMEM(attinfo->attunit,
									FpaConfigUnitStruct *, natt+1);
							attinfo->tag[natt]     = argt;
							attinfo->attname[natt] = arga;
							attinfo->attunit[natt] = udef;
							}

						/* Otherwise, free space used by attribute information */
						else
							{
							FREEMEM(argt);
							FREEMEM(arga);
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedMetafiles,
								FpaCalliedAttribInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Reset metafile default attribute identifier */
				else if ( same(cmd, FpaCalliedDefAttribInfoReset) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Reset metafile default attribute identifier */
						validarg = logical_arg(cline, &argok);
						if ( validarg )
							{
							definfo = metafiles->definfo[nfile];
							for ( natt=0; natt<definfo->natt_defs; natt++ )
								{
								FREEMEM(definfo->attname_defs[natt]);
								FREEMEM(definfo->attval_defs[natt]);
								}
							FREEMEM(definfo->attname_defs);
							FREEMEM(definfo->attval_defs);
							definfo->natt_defs = 0;
							}
						if ( !argok )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedDefAttribInfoReset,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedMetafiles,
								FpaCalliedDefAttribInfoReset, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Add another metafile default attribute identifier */
				else if ( same(cmd, FpaCalliedDefAttribInfo) )
					{
					if ( same(string_arg(cline), FpaCequalSign) )
						{

						/* Set the attribute name */
						arga  = strdup_arg(cline);
						varga = OKARG(arga);
						if ( !varga )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedDefAttribInfoName,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Set the attribute value */
						argx  = strdup_arg(cline);
						vargx = OKARG(argx);
						if ( !vargx )
							{
							(void) config_file_message(FpaCblockSources,
									sdef->name, FpaCalliedMetafiles,
									FpaCalliedDefAttribInfoValue,
									FpaCmsgParameter);
							sdef->valid_allied = FALSE;
							valid              = FALSE;
							}

						/* Add information for another default attribute */
						if ( varga && vargx)
							{
							definfo = metafiles->definfo[nfile];
							natt    = definfo->natt_defs++;
							definfo->attname_defs = GETMEM(definfo->attname_defs,
									STRING,                    natt+1);
							definfo->attval_defs  = GETMEM(definfo->attval_defs,
									STRING,                    natt+1);
							definfo->attname_defs[natt] = arga;
							definfo->attval_defs[natt]  = argx;
							}

						/* Otherwise, free space used by default information */
						else
							{
							FREEMEM(arga);
							FREEMEM(argx);
							}
						}
					else
						{
						(void) config_file_message(FpaCblockSources,
								sdef->name, FpaCalliedMetafiles,
								FpaCalliedDefAttribInfo, FpaCmsgNoEqual);
						sdef->valid_allied = FALSE;
						valid              = FALSE;
						}
					}

				/* Set error flag for unrecognized Sources keyword */
				else
					{
					(void) config_file_message(FpaCblockSources,
							sdef->name, FpaCalliedMetafiles,
							cmd, FpaCmsgKeyword);
					sdef->valid_allied = FALSE;
					valid              = FALSE;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, cmd, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ s o u r c e                                              *
*   i n i t _ s o u r c e                                              *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Sources block, or pointer to initialized structures containing     *
*   information read from Sources block of configuration files.        *
*   Note that source name comparisons are case insensitive!            *
*                                                                      *
***********************************************************************/

static	FpaConfigSourceStruct	*find_source

	(
	STRING		name		/* source name */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumSourceIdent < 1 ) return NullPtr(FpaConfigSourceStruct *);

	/* Copy the source name into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for source name */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) SourceIdents,
			(size_t) NumSourceIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if source name found in list */
	return ( pident ) ? (FpaConfigSourceStruct *) pident->pdef:
							NullPtr(FpaConfigSourceStruct *);
	}

/**********************************************************************/

static	FpaConfigSourceStruct	*init_source

	(
	STRING		name		/* source name */
	)

	{
	FpaConfigSourceStruct	*sdef;

	/* Add source at end of current SourceDefs list */
	NumSourceDef++;
	SourceDefs = GETMEM(SourceDefs, FpaConfigSourceStruct *, NumSourceDef);
	SourceDefs[NumSourceDef-1]    = INITMEM(FpaConfigSourceStruct, 1);

	/* Initialize SourceDefs structure */
	sdef                          = SourceDefs[NumSourceDef - 1];
	sdef->name                    = strdup(name);
	sdef->valid                   = TRUE;
	sdef->nblocks                 = 0;
	sdef->filenames               = NullStringList;
	sdef->locations               = NullLong;
	sdef->label                   = strdup(name);
	sdef->sh_label                = strdup(name);
	sdef->description             = NullString;
	sdef->minutes_rqd             = FALSE;
	sdef->src_type                = FpaCnoMacro;

	/* Initialize SourceIO structure in SourceDefs structure */
	sdef->src_io                  = INITMEM(FpaConfigSourceIOStruct, 1);
	sdef->src_io->src_tag         = NullString;
	sdef->src_io->src_path        = NullString;
	sdef->src_io->src_layers      = 1;

	/* Initialize SourceSub structure in SourceDefs structure */
	/* Note that first SourceSub structure has its labels set */
	/*  to the default labels sdef->label and sdef->sh_label! */
	sdef->src_sub                 = NullPtr(FpaConfigSourceSubStruct *);
	sdef->nsubsrc                 = 1;
	sdef->subsrcs                 = INITMEM(FpaConfigSourceSubStruct *, 1);
	sdef->subsrcs[0]              = init_source_subsource(FpaCblank);
	sdef->subsrcs[0]->label       = sdef->label;
	sdef->subsrcs[0]->sh_label    = sdef->sh_label;
	sdef->subsrcs[0]->description = sdef->description;

	/* Set pointer to SourceAllied structure in SourceDefs structure */
	sdef->valid_allied            = TRUE;
	sdef->allied                  = NullPtr(FpaConfigSourceAlliedStruct *);

	/* Add the name as another identifier */
	(void) add_source_identifier(name, sdef);

	/* Return pointer to SourceDefs structure */
	return sdef;
	}

/***********************************************************************
*                                                                      *
*   i n i t _ s o u r c e _ s u b s o u r c e                          *
*                                                                      *
*   Return pointer to initialized structure for subsource information  *
*   read from Sources block of configuration files.                    *
*                                                                      *
***********************************************************************/

static	FpaConfigSourceSubStruct		*init_source_subsource

	(
	STRING		name		/* sub source name */
	)

	{
	FpaConfigSourceSubStruct	*subdef;

	/* Initialize SourceSub structure */
	subdef = INITMEM(FpaConfigSourceSubStruct, 1);
	subdef->name        = strdup(name);
	subdef->label       = NullString;
	subdef->sh_label    = NullString;
	subdef->description = NullString;
	subdef->sub_path    = NullString;

	/* Return pointer to SourceSub structure */
	return subdef;
	}

/***********************************************************************
*                                                                      *
*   i n i t _ s o u r c e _ a l l i e d                                *
*                                                                      *
*   Return pointer to initialized structure for Allied Model           *
*   information read from Sources block of configuration files.        *
*                                                                      *
*   i n i t _ a l l i e d _ p r o g r a m s                            *
*   i n i t _ a l l i e d _ f i l e s                                  *
*   i n i t _ a l l i e d _ f i e l d s                                *
*   i n i t _ a l l i e d _ w i n d s                                  *
*   i n i t _ a l l i e d _ v a l u e s                                *
*   i n i t _ a l l i e d _ m e t a f i l e s                          *
*                                                                      *
*   Return pointer to initialized structure for Allied Model           *
*   information read from subsections of Sources block of              *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	FpaConfigSourceAlliedStruct		*init_source_allied

	(
	)

	{
	FpaConfigSourceAlliedStruct			*allied;

	/* Initialize SourceAllied structure */
	allied               = INITMEM(FpaConfigSourceAlliedStruct, 1);
	allied->check_allied = FALSE;
	allied->time_match   = FALSE;
	allied->src_def      = NullPtr(FpaConfigSourceStruct *);
	allied->sub_def      = NullPtr(FpaConfigSourceSubStruct *);
	allied->pre_process  = NullString;
	allied->process      = NullString;
	allied->post_process = NullString;
	allied->programs     = NullPtr(FpaConfigAlliedProgramsStruct *);
	allied->files        = NullPtr(FpaConfigAlliedFilesStruct *);
	allied->fields       = NullPtr(FpaConfigAlliedFieldsStruct *);
	allied->winds        = NullPtr(FpaConfigAlliedWindsStruct *);
	allied->values       = NullPtr(FpaConfigAlliedValuesStruct *);
	allied->metafiles    = NullPtr(FpaConfigAlliedMetafilesStruct *);

	/* Return pointer to SourceAllied structure */
	return allied;
	}

/**********************************************************************/

static	FpaConfigAlliedProgramsStruct	*init_allied_programs

	(
	)

	{
	FpaConfigAlliedProgramsStruct		*programs;

	/* Initialize AlliedPrograms structure */
	programs             = INITMEM(FpaConfigAlliedProgramsStruct, 1);
	programs->nprogs     = 0;
	programs->aliases    = NullStringList;
	programs->src_tags   = NullStringList;
	programs->prog_paths = NullStringList;

	/* Return pointer to AlliedPrograms structure */
	return programs;
	}

/**********************************************************************/

static	FpaConfigAlliedFilesStruct		*init_allied_files

	(
	)

	{
	FpaConfigAlliedFilesStruct			*files;

	/* Initialize AlliedFiles structure */
	files             = INITMEM(FpaConfigAlliedFilesStruct, 1);
	files->nfiles     = 0;
	files->aliases    = NullStringList;
	files->src_tags   = NullStringList;
	files->file_paths = NullStringList;

	/* Return pointer to AlliedFiles structure */
	return files;
	}

/**********************************************************************/

static	FpaConfigAlliedFieldsStruct		*init_allied_fields

	(
	)

	{
	FpaConfigAlliedFieldsStruct			*fields;

	/* Initialize AlliedFields structure */
	fields             = INITMEM(FpaConfigAlliedFieldsStruct, 1);
	fields->nfields    = 0;
	fields->aliases    = NullStringList;
	fields->flds       = NullPtr(FpaConfigFieldStruct **);
	fields->ftypes     = NullInt;
	fields->attinfo    = NullPtr(FpaConfigAlliedAttribStruct **);
	fields->nodeinfo   = NullPtr(FpaConfigAlliedAttribStruct **);
	fields->sub_fields = NullStringList;
	fields->sub_units  = NullPtr(FpaConfigUnitStruct **);
	fields->src_defs   = NullPtr(FpaConfigSourceStruct **);
	fields->sub_defs   = NullPtr(FpaConfigSourceSubStruct **);

	/* Return pointer to AlliedFields structure */
	return fields;
	}

/**********************************************************************/

static	FpaConfigAlliedWindsStruct		*init_allied_winds

	(
	)

	{
	FpaConfigAlliedWindsStruct			*winds;

	/* Initialize AlliedWinds structure */
	winds           = INITMEM(FpaConfigAlliedWindsStruct, 1);
	winds->nwinds   = 0;
	winds->aliases  = NullStringList;
	winds->wcrefs   = NullPtr(FpaConfigCrossRefStruct **);
	winds->src_defs = NullPtr(FpaConfigSourceStruct **);
	winds->sub_defs = NullPtr(FpaConfigSourceSubStruct **);

	/* Return pointer to AlliedWinds structure */
	return winds;
	}

/**********************************************************************/

static	FpaConfigAlliedValuesStruct		*init_allied_values

	(
	)

	{
	FpaConfigAlliedValuesStruct			*values;

	/* Initialize AlliedValues structure */
	values           = INITMEM(FpaConfigAlliedValuesStruct, 1);
	values->nvalues   = 0;
	values->aliases  = NullStringList;
	values->vcrefs   = NullPtr(FpaConfigCrossRefStruct **);
	values->src_defs = NullPtr(FpaConfigSourceStruct **);
	values->sub_defs = NullPtr(FpaConfigSourceSubStruct **);

	/* Return pointer to AlliedValues structure */
	return values;
	}

/**********************************************************************/

static	FpaConfigAlliedMetafilesStruct	*init_allied_metafiles

	(
	)

	{
	FpaConfigAlliedMetafilesStruct		*metafiles;

	/* Initialize AlliedMetafiles structure */
	metafiles               = INITMEM(FpaConfigAlliedMetafilesStruct, 1);
	metafiles->nfiles       = 0;
	metafiles->aliases      = NullStringList;
	metafiles->file_aliases = NullStringList;
	metafiles->flds         = NullPtr(FpaConfigFieldStruct **);
	metafiles->attinfo      = NullPtr(FpaConfigAlliedAttribStruct **);
	metafiles->definfo      = NullPtr(FpaConfigAlliedDefAttribStruct **);

	/* Return pointer to AlliedMetafiles structure */
	return metafiles;
	}

/***********************************************************************
*                                                                      *
*   s e t _ s o u r c e _ l o c a t i o n                              *
*                                                                      *
*   Save configuration file name and location for reading Allied Model *
*   information from Sources block of configuration files.             *
*                                                                      *
***********************************************************************/

static	LOGICAL					set_source_location

	(
	FILE					*fpcfg,		/* pointer to configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING		cfgname;
	long int	position;
	int			nblk;

	/* Return FALSE if no structure passed */
	if ( IsNull(sdef) ) return FALSE;

	/* Get file name and location from configuration file */
	if ( !config_file_location(fpcfg, &cfgname, &position) ) return FALSE;

	/* Add configuration file name and location to list */
	nblk = sdef->nblocks++;
	sdef->filenames = GETMEM(sdef->filenames, STRING,   sdef->nblocks);
	sdef->locations = GETMEM(sdef->locations, long int, sdef->nblocks);
	sdef->filenames[nblk] = cfgname;
	sdef->locations[nblk] = position;

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ s o u r c e _ a l i a s e s                                *
*   a d d _ s o u r c e _ i d e n t i f i e r                          *
*                                                                      *
*   Add aliases or identifiers to source identifier list.              *
*   Note that source name comparisons are case insensitive!            *
*                                                                      *
***********************************************************************/

static	void					add_source_aliases

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{
	STRING					arg;
	FpaConfigSourceStruct	*sdefx;

	/* Add all acceptable aliases to ident list ... if not already there! */
	while ( NotNull( arg = string_arg(cline) ) )
		{
		if ( OKARG(arg) )
			{

			/* Add to ident list if alias not found */
			sdefx = find_source(arg);
			if ( IsNull(sdefx) )
				{
				(void) add_source_identifier(arg, sdef);
				}

			/* Error message if alias belongs to another source! */
			else if ( sdefx != sdef )
				{
				(void) config_file_message(FpaCblockSources,
						sdef->name, sdefx->name, arg, FpaCmsgAlias);
				}
			}
		}
	}

/**********************************************************************/

static	void					add_source_identifier

	(
	STRING					ident,		/* source identifier name */
	FpaConfigSourceStruct	*sdef		/* pointer to Source structure */
	)

	{

	/* Add identifier to list */
	NumSourceIdent++;
	SourceIdents = GETMEM(SourceIdents, FPAC_IDENTS, NumSourceIdent);
	SourceIdents[NumSourceIdent-1].ident = strdup(ident);
	SourceIdents[NumSourceIdent-1].pdef  = (POINTER) sdef;

	/* Sort the list */
	(void) qsort((POINTER) SourceIdents, (size_t) NumSourceIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Groups block of configuration file)     *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ g r o u p s _ i n f o                                    *
*                                                                      *
*   Read information from Groups block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_groups_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, arg;
	int						numbrace, section_id, section;
	LOGICAL					firstline;
	FpaConfigGroupStruct	*gdef;

	/* Read the configuration file(s) only once */
	if ( GroupsRead ) return GroupsValid;

	/* Find and open the configuration file for the Groups block */
	if ( !first_config_file_open(FpaCgroupsFile, &fpcfg) )
		{
		GroupsRead = TRUE;
		return GroupsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Groups block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Groups block of configuration file */
		if ( same(cmd, FpaCblockGroups) )
			{

			/* Set counter and identifier for Groups block */
			numbrace   = 0;
			section    = FpaCblockGroupsSection;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Groups block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Groups block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Groups section */
				else if ( numbrace == 1 )
					{

					/* Check for Fields or Elements sections */
					if ( section_id == FpaCblockGroupsSection )
						{

						/* Set identifier for Fields section of Groups block */
						if ( same(cmd, FpaCblockFields) )
							section = FpaCblockGroupsFields;

						/* Set identifier for Elements section of Groups block */
						else if ( same(cmd, FpaCblockElements) )
							section = FpaCblockGroupsElements;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockGroups,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Start of new Fields or Elements declaration */
				else if ( numbrace == 2 )
					{

					/* Adding another name in FpaCblockGroupsFields section */
					if ( section_id == FpaCblockGroupsFields )
						{

						/* Check for declaration already in the list */
						gdef = find_field_group(cmd);

						/* Add another group name to the lists for fields */
						if ( IsNull(gdef) )
							{
							gdef = init_field_group(cmd);
							}

						/* Check that group name for fields is not an alias */
						/*  of another group name for fields!               */
						else if ( !same(cmd, gdef->name) )
							{
							(void) config_file_message(FpaCblockGFields,
									cmd, gdef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Groups Fields block */
						section = FpaCblockGroupsFieldsInfo;
						}

					/* Adding another name in FpaCblockGroupsElements section */
					else if ( section_id == FpaCblockGroupsElements )
						{

						/* Check for declaration already in the list */
						gdef = find_element_group(cmd);

						/* Add another group name to the lists for elements */
						if ( IsNull(gdef) )
							{
							gdef = init_element_group(cmd);
							}

						/* Check that group name for elements is not an alias */
						/*  of another group name for elements!               */
						else if ( !same(cmd, gdef->name) )
							{
							(void) config_file_message(FpaCblockGElements,
									cmd, gdef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Groups Elements block */
						section = FpaCblockGroupsElementsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockGroups,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Groups declarations */
				/*  ... with format of "cmd = value(s)"  */
				else
					{

					/* Adding parameters in FpaCblockGroupsFieldsInfo section */
					if ( section_id == FpaCblockGroupsFieldsInfo )
						{

						/* Groups Fields label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									gdef->label = STRMEM(gdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockGFields,
										gdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								gdef->valid = FALSE;
								}
							}

						/* Groups Fields short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									gdef->sh_label =
											STRMEM(gdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockGFields,
										gdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								gdef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Groups keyword */
						else
							{
							(void) config_file_message(FpaCblockGFields,
									gdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							gdef->valid = FALSE;
							}
						}

					/* Adding parameters in FpaCblockGroupsElementsInfo section */
					else if ( section_id == FpaCblockGroupsElementsInfo )
						{

						/* Groups Elements label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									gdef->label = STRMEM(gdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockGElements,
										gdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								gdef->valid = FALSE;
								}
							}

						/* Groups Elements short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									gdef->sh_label =
											STRMEM(gdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockGElements,
										gdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								gdef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Groups keyword */
						else
							{
							(void) config_file_message(FpaCblockGElements,
									gdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							gdef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockGroups,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Set flags for completion of reading */
	GroupsRead  = TRUE;
	GroupsValid = TRUE;
	return GroupsValid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ f i e l d _ g r o u p                                    *
*   i n i t _ f i e l d _ g r o u p                                    *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Fields section of Groups block, or pointer to initialized          *
*   structure to contain information read from Fields section of       *
*   Groups block of configuration files.                               *
*   Note that group name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	FpaConfigGroupStruct	*find_field_group

	(
	STRING		name		/* group name for fields */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumGrpFldIdent < 1 ) return NullPtr(FpaConfigGroupStruct *);

	/* Copy the group name for fields into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for group name for fields */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) GrpFldIdents,
			(size_t) NumGrpFldIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if group name for field found in list */
	return ( pident ) ? (FpaConfigGroupStruct *) pident->pdef:
							NullPtr(FpaConfigGroupStruct *);
	}

/**********************************************************************/

static	FpaConfigGroupStruct	*init_field_group

	(
	STRING		name		/* group name for fields */
	)

	{
	FpaConfigGroupStruct	*gdef;

	/* Add group name for fields at end of current GrpFldDefs list */
	NumGrpFldDef++;
	GrpFldDefs = GETMEM(GrpFldDefs, FpaConfigGroupStruct *, NumGrpFldDef);
	GrpFldDefs[NumGrpFldDef-1] = INITMEM(FpaConfigGroupStruct, 1);

	/* Initialize GrpFldDefs structure */
	gdef           = GrpFldDefs[NumGrpFldDef - 1];
	gdef->name     = strdup(name);
	gdef->valid    = TRUE;
	gdef->label    = strdup(name);
	gdef->sh_label = strdup(name);

	/* Add the name as another identifier */
	(void) add_field_group_identifier(name, gdef);

	/* Return pointer to GrpFldDefs structure */
	return gdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ f i e l d _ g r o u p _ i d e n t i f i e r                *
*                                                                      *
*   Add another identifier to group identifier list for fields.        *
*   Note that group name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	void					add_field_group_identifier

	(
	STRING					ident,		/* group identifier for fields */
	FpaConfigGroupStruct	*gdef		/* pointer to Group structure
											for fields */
	)

	{

	/* Add identifier to list */
	NumGrpFldIdent++;
	GrpFldIdents = GETMEM(GrpFldIdents, FPAC_IDENTS, NumGrpFldIdent);
	GrpFldIdents[NumGrpFldIdent-1].ident = strdup(ident);
	GrpFldIdents[NumGrpFldIdent-1].pdef  = (POINTER) gdef;

	/* Sort the list */
	(void) qsort((POINTER) GrpFldIdents, (size_t) NumGrpFldIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*   f i n d _ e l e m e n t _ g r o u p                                *
*   i n i t _ e l e m e n t _ g r o u p                                *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Elements section of Groups block, or pointer to initialized        *
*   structure to contain information read from Elements section of     *
*   Groups block of configuration files.                               *
*   Note that group name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	FpaConfigGroupStruct	*find_element_group

	(
	STRING		name		/* group name for elements */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumGrpElemIdent < 1 ) return NullPtr(FpaConfigGroupStruct *);

	/* Copy the group name for elements into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for group name for elements */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) GrpElemIdents,
			(size_t) NumGrpElemIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if group name for elements found in list */
	return ( pident ) ? (FpaConfigGroupStruct *) pident->pdef:
							NullPtr(FpaConfigGroupStruct *);
	}

/**********************************************************************/

static	FpaConfigGroupStruct	*init_element_group

	(
	STRING		name		/* group name for elements */
	)

	{
	FpaConfigGroupStruct	*gdef;

	/* Add group name for elements at end of current GrpElemDefs list */
	NumGrpElemDef++;
	GrpElemDefs = GETMEM(GrpElemDefs, FpaConfigGroupStruct *, NumGrpElemDef);
	GrpElemDefs[NumGrpElemDef-1] = INITMEM(FpaConfigGroupStruct, 1);

	/* Initialize GrpElemDefs structure */
	gdef           = GrpElemDefs[NumGrpElemDef - 1];
	gdef->name     = strdup(name);
	gdef->valid    = TRUE;
	gdef->label    = strdup(name);
	gdef->sh_label = strdup(name);

	/* Add the name as another identifier */
	(void) add_element_group_identifier(name, gdef);

	/* Return pointer to GrpElemDefs structure */
	return gdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ g r o u p _ i d e n t i f i e r            *
*                                                                      *
*   Add another identifier to group identifier list for elements.      *
*   Note that group name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	void					add_element_group_identifier

	(
	STRING					ident,		/* group identifier for elements */
	FpaConfigGroupStruct	*gdef		/* pointer to Group structure
											for elements */
	)

	{

	/* Add identifier to list */
	NumGrpElemIdent++;
	GrpElemIdents = GETMEM(GrpElemIdents, FPAC_IDENTS, NumGrpElemIdent);
	GrpElemIdents[NumGrpElemIdent-1].ident = strdup(ident);
	GrpElemIdents[NumGrpElemIdent-1].pdef  = (POINTER) gdef;

	/* Sort the list */
	(void) qsort((POINTER) GrpElemIdents, (size_t) NumGrpElemIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Levels block of configuration file)     *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ l e v e l s _ i n f o                                    *
*                                                                      *
*   Read information from Levels block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_levels_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, arg;
	int						numbrace, section_id, section, macro;
	LOGICAL					firstline;
	FpaConfigLevelStruct	*ldef;
	int						nn;

	/* Read the configuration file(s) only once */
	if ( LevelsRead ) return LevelsValid;

	/* Force the Groups block of the configuration file to be read first */
	if ( !read_groups_info() ) return LevelsValid;

	/* Find and open the configuration file for the Levels block */
	if ( !first_config_file_open(FpaClevelsFile, &fpcfg) )
		{
		LevelsRead = TRUE;
		return LevelsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Levels block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Levels block of configuration file */
		if ( same(cmd, FpaCblockLevels) )
			{

			/* Set counter and identifier for Levels block */
			numbrace   = 0;
			section    = FpaCblockLevelsName;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Levels block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Levels block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Levels declaration */
				else if ( numbrace == 1 )
					{

					/* Adding another name in FpaCblockLevelsName section */
					if ( section_id == FpaCblockLevelsName )
						{

						/* Check for declaration already in the list */
						ldef = find_level(cmd);

						/* Add another level name to the lists */
						if ( IsNull(ldef) )
							{
							ldef = init_level(cmd);
							}

						/* Check that level name is not an alias */
						/*  of another level!                    */
						else if ( !same(cmd, ldef->name) )
							{
							(void) config_file_message(FpaCblockLevels,
									cmd, ldef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Levels block */
						section = FpaCblockLevelsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockLevels,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Levels declarations */
				/*  ... with format of "cmd = value(s)"  */
				else
					{

					/* Adding parameters in FpaCblockLevelsInfo section */
					if ( section_id == FpaCblockLevelsInfo )
						{

						/* Level label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									ldef->label = STRMEM(ldef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Level short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									ldef->sh_label =
											STRMEM(ldef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Level description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									ldef->description =
											STRMEM(ldef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Cannot reset level type! */
						else if ( same(cmd, FpaClevelType)
								&& ldef->lvl_type != FpaCnoMacro )
							{
							(void) config_file_message(FpaCblockLevels,
									ldef->name, FpaCblank,
									FpaClevelType, FpaCmsgReset);
							}

						/* Level type */
						else if ( same(cmd, FpaClevelType) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
											NumFpaClevelTypes, FpaClevelTypes);
								if ( macro != FpaCnoMacro )
									{
									ldef->lvl_type = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockLevels,
											ldef->name, FpaCblank,
											FpaClevelType, FpaCmsgParameter);
									ldef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaClevelType, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Default field group */
						else if ( same(cmd, FpaCfieldGroup) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg             = strdup_arg(cline);
								ldef->fld_group =
										identify_group(FpaCblockFields, arg);
								FREEMEM(arg);
								if ( IsNull(ldef->fld_group) )
									{
									(void) config_file_message(FpaCblockLevels,
											ldef->name, FpaCblank,
											FpaCfieldGroup, FpaCmsgParameter);
									ldef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaCfieldGroup, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Level name aliases */
						else if ( same(cmd, FpaCalias) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Add all aliases to ident list */
								(void) add_level_aliases(cline, ldef);
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaCalias, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Cannot reset level file ident! */
						else if ( same(cmd, FpaCfileIdent)
								&& !blank(ldef->lev_io->fident) )
							{
							(void) config_file_message(FpaCblockLevels,
									ldef->name, FpaCblank,
									FpaCfileIdent, FpaCmsgReset);
							}

						/* Level file ident */
						else if ( same(cmd, FpaCfileIdent) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Set file ident and add it to ident list */
								(void) add_level_file_ident(cline, ldef);
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaCfileIdent, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Cannot reset level file id! */
						else if ( same(cmd, FpaCfileId)
								&& !blank(ldef->lev_io->fid) )
							{
							(void) config_file_message(FpaCblockLevels,
									ldef->name, FpaCblank,
									FpaCfileId, FpaCmsgReset);
							}

						/* Level file id */
						else if ( same(cmd, FpaCfileId) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Set file id and add it to ident list */
								(void) add_level_fileid(cline, ldef);
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaCfileId, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Cannot reset level levels information! */
						else if ( same(cmd, FpaClevelLevels)
								&& (ldef->lev_lvls->lvl_category != FpaCnoMacro
									|| !blank(ldef->lev_lvls->lvl)
									|| !blank(ldef->lev_lvls->uprlvl)
									|| !blank(ldef->lev_lvls->lwrlvl) ) )
							{
							(void) config_file_message(FpaCblockLevels,
									ldef->name, FpaCblank,
									FpaClevelLevels, FpaCmsgReset);
							}

						/* Level levels information */
						else if ( same(cmd, FpaClevelLevels) )
							{

							/* Set level levels information */
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								if ( !set_level_levels(cline, ldef) )
									{
									(void) config_file_message(FpaCblockLevels,
											ldef->name, FpaCblank,
											FpaClevelLevels, FpaCmsgParameter);
									ldef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockLevels,
										ldef->name, FpaCblank,
										FpaClevelLevels, FpaCmsgNoEqual);
								ldef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Levels keyword */
						else
							{
							(void) config_file_message(FpaCblockLevels,
									ldef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							ldef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockLevels,
								ldef->name, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Error check for each member of Levels block */
	for ( nn=0; nn<NumLevelDef; nn++ )
		{
		ldef = LevelDefs[nn];

		/* Ensure that "level_type" has been set */
		if ( ldef->lvl_type == FpaCnoMacro )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockLevels, ldef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaClevelType, ldef->name);
			ldef->valid = FALSE;
			}
		}

	/* Set flags for completion of reading */
	LevelsRead  = TRUE;
	LevelsValid = TRUE;
	return LevelsValid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ l e v e l                                                *
*   i n i t _ l e v e l                                                *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Levels block, or pointer to initialized structures containing      *
*   information read from Levels block of configuration files.         *
*   Note that level name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	FpaConfigLevelStruct	*find_level

	(
	STRING		name		/* level name */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumLevelIdent < 1 ) return NullPtr(FpaConfigLevelStruct *);

	/* Copy the level name into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for level name */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) LevelIdents,
			(size_t) NumLevelIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if level name found in list */
	return ( pident ) ? (FpaConfigLevelStruct *) pident->pdef:
							NullPtr(FpaConfigLevelStruct *);
	}

/**********************************************************************/

static	FpaConfigLevelStruct	*init_level

	(
	STRING		name		/* level name */
	)

	{
	FpaConfigLevelStruct	*ldef;

	/* Add level at end of current LevelDefs list */
	NumLevelDef++;
	LevelDefs = GETMEM(LevelDefs, FpaConfigLevelStruct *, NumLevelDef);
	LevelDefs[NumLevelDef-1] = INITMEM(FpaConfigLevelStruct, 1);

	/* Initialize LevelDefs structure */
	ldef                         = LevelDefs[NumLevelDef - 1];
	ldef->name                   = strdup(name);
	ldef->valid                  = TRUE;
	ldef->label                  = strdup(name);
	ldef->sh_label               = strdup(name);
	ldef->description            = NullString;
	ldef->lvl_type               = FpaCnoMacro;
	ldef->fld_group              = NullPtr(FpaConfigGroupStruct *);

	/* Initialize LevelIO structure in LevelDefs structure */
	ldef->lev_io                 = INITMEM(FpaConfigLevelIOStruct, 1);
	ldef->lev_io->check_fident   = FALSE;
	ldef->lev_io->fident         = NullString;
	ldef->lev_io->fid            = NullString;

	/* Initialize LevelLevels structure in LevelDefs structure */
	ldef->lev_lvls               = INITMEM(FpaConfigLevelLevelsStruct, 1);
	ldef->lev_lvls->lvl_category = FpaCnoMacro;
	ldef->lev_lvls->lvl          = NullString;
	ldef->lev_lvls->uprlvl       = NullString;
	ldef->lev_lvls->lwrlvl       = NullString;

	/* Add the name as another identifier */
	(void) add_level_identifier(name, ldef);

	/* Return pointer to LevelDefs structure */
	return ldef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ l e v e l _ a l i a s e s                                  *
*   a d d _ l e v e l _ f i l e _ i d e n t                            *
*   a d d _ l e v e l _ f i l e i d                                    *
*   a d d _ l e v e l _ i d e n t i f i e r                            *
*                                                                      *
*   Add aliases, file ids, or identifiers to level identifier list.    *
*   Note that level name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	void					add_level_aliases

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{
	STRING					arg;
	FpaConfigLevelStruct	*ldefx;

	/* Add all acceptable aliases to ident list ... if not already there! */
	while ( NotNull( arg = string_arg(cline) ) )
		{
		if ( OKARG(arg) )
			{

			/* Add to ident list if alias not found */
			ldefx = find_level(arg);
			if ( IsNull(ldefx) )
				{
				(void) add_level_identifier(arg, ldef);
				}

			/* Error message if alias belongs to another level! */
			else if ( ldefx != ldef )
				{
				(void) config_file_message(FpaCblockLevels,
						ldef->name, ldefx->name, arg, FpaCmsgAlias);
				}
			}
		}
	}

/**********************************************************************/

static	void					add_level_file_ident

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{
	STRING					arg;
	FpaConfigLevelStruct	*ldefx;

	/* Ignore special file ident "None" */
	arg = string_arg(cline);
	if ( same_ic(arg, FpaCnone) ) return;

	/* Check for acceptable file idents                      */
	/*  and add them to ident list ... if not already there! */
	if ( OKARG(arg) && file_ident_format(arg, LEVEL_IDENT_LEN) )
		{

		/* Add to ident list if file ident not found */
		ldefx = find_level(arg);
		if ( IsNull(ldefx) )
			{
			(void) add_level_identifier(arg, ldef);
			}

		/* Error message if file ident is an alias of another level! */
		else if ( ldefx != ldef )
			{
			(void) config_file_message(FpaCblockLevels,
					ldef->name, ldefx->name, arg, FpaCmsgAlias);
			}

		/* Set file ident */
		ldef->lev_io->fident = strdup(arg);
		}

	/* Error message for unacceptable file idents */
	else
		{
		(void) config_file_message(FpaCblockLevels,
				ldef->name, FpaCblank, FpaCfileIdent, FpaCmsgParameter);
		ldef->valid = FALSE;
		}
	}

/**********************************************************************/

static	void					add_level_fileid

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{
	STRING					arg;
	FpaConfigLevelStruct	*ldefx;

	/* Ignore special file id "None" */
	arg = string_arg(cline);
	if ( same_ic(arg, FpaCnone) ) return;

	/* Check for acceptable file ids                         */
	/*  and add them to ident list ... if not already there! */
	if ( OKARG(arg) )
		{

		/* Add to ident list if file id not found */
		ldefx = find_level(arg);
		if ( IsNull(ldefx) )
			{
			(void) add_level_identifier(arg, ldef);
			}

		/* Error message if file id is an alias of another level! */
		else if ( ldefx != ldef )
			{
			(void) config_file_message(FpaCblockLevels,
					ldef->name, ldefx->name, arg, FpaCmsgAlias);
			}

		/* Set file id */
		ldef->lev_io->fid = strdup(arg);
		}

	/* Error message for unacceptable file ids */
	else
		{
		(void) config_file_message(FpaCblockLevels,
				ldef->name, FpaCblank, FpaCfileId, FpaCmsgParameter);
		ldef->valid = FALSE;
		}
	}

/**********************************************************************/

static	void					add_level_identifier

	(
	STRING					ident,		/* level identifier name */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{

	/* Add identifier to list */
	NumLevelIdent++;
	LevelIdents = GETMEM(LevelIdents, FPAC_IDENTS, NumLevelIdent);
	LevelIdents[NumLevelIdent-1].ident = strdup(ident);
	LevelIdents[NumLevelIdent-1].pdef  = (POINTER) ldef;

	/* Sort the list */
	(void) qsort((POINTER) LevelIdents, (size_t) NumLevelIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*   s e t _ l e v e l _ l e v e l s                                    *
*                                                                      *
*   Set levels information from "level_levels = ..." line in Levels    *
*   block of configuration files.                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL					set_level_levels

	(
	STRING					cline,		/* level levels information */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{
	int			macro;
	STRING		arg;

	/* Return FALSE for bad levels category string */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Get macro for levels category */
	macro = config_file_macro(arg, NumFpaClevelsCategories,
			FpaClevelsCategories);
	if ( macro == FpaCnoMacro ) return FALSE;

	/* Set levels category */
	ldef->lev_lvls->lvl_category = macro;

	/* Now set single or upper/lower levels  based on level type */
	/*  and levels category and return TRUE if all OK            */
	switch ( ldef->lvl_type )
		{

		/* Level type FpaC_MSL */
		case FpaC_MSL:

			/* Check for allowed categories */
			switch ( macro )
				{

				/* Category FpaC_LEVELS_MSL */
				case FpaC_LEVELS_MSL:
					if ( !blank(ldef->lev_io->fident) )
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fident));
					else
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fid));
					return TRUE;

				/* Return FALSE for other categories */
				default:
					return FALSE;
				}

		/* Level type FpaC_SURFACE */
		case FpaC_SURFACE:

			/* Check for allowed categories */
			switch ( macro )
				{

				/* Category FpaC_LEVELS_SURFACE */
				case FpaC_LEVELS_SURFACE:
					if ( !blank(ldef->lev_io->fident) )
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fident));
					else
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fid));
					return TRUE;

				/* Return FALSE for other categories */
				default:
					return FALSE;
				}

		/* Level type FpaC_LEVEL */
		case FpaC_LEVEL:

			/* Check for allowed categories */
			switch ( macro )
				{

				/* Categories FpaC_LEVELS_PRESSURE/HEIGHT/SIGMA/THETA */
				case FpaC_LEVELS_PRESSURE:
				case FpaC_LEVELS_HEIGHT:
				case FpaC_LEVELS_SIGMA:
				case FpaC_LEVELS_THETA:
					arg = string_arg(cline);
					if ( !OKARG(arg) ) return FALSE;

					ldef->lev_lvls->lvl = strdup(arg);
					return TRUE;

				/* Return FALSE for other categories */
				default:
					return FALSE;
				}

		/* Level type FpaC_LAYER */
		case FpaC_LAYER:

			/* Check for allowed categories */
			switch ( macro )
				{

				/* Categories FpaC_LEVELS_PRESSURE/HEIGHT/SIGMA/THETA */
				case FpaC_LEVELS_PRESSURE:
				case FpaC_LEVELS_HEIGHT:
				case FpaC_LEVELS_SIGMA:
				case FpaC_LEVELS_THETA:
					arg = string_arg(cline);
					if ( !OKARG(arg) ) return FALSE;

					ldef->lev_lvls->uprlvl = strdup(arg);

					arg = string_arg(cline);
					if ( !OKARG(arg) ) return FALSE;

					ldef->lev_lvls->lwrlvl = strdup(arg);
					return TRUE;

				/* Return FALSE for other categories */
				default:
					return FALSE;
				}

		/* Level type FpaC_GEOGRAPHY */
		case FpaC_GEOGRAPHY:

			/* Check for allowed categories */
			switch ( macro )
				{

				/* Category FpaC_LEVELS_GEOGRAPHY */
				case FpaC_LEVELS_GEOGRAPHY:
					if ( !blank(ldef->lev_io->fident) )
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fident));
					else
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fid));
					return TRUE;

				/* Return FALSE for other categories */
				default:
					return FALSE;
				}

		/* Level type FpaC_ANNOTATION */
		case FpaC_ANNOTATION:

			/* Check for allowed categories */
			switch ( macro )
				{

				/* Category FpaC_LEVELS_ANNOTATION */
				case FpaC_LEVELS_ANNOTATION:
					if ( !blank(ldef->lev_io->fident) )
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fident));
					else
						ldef->lev_lvls->lvl = strdup(SafeStr(ldef->lev_io->fid));
					return TRUE;

				/* Return FALSE for other categories */
				default:
					return FALSE;
				}

		/* Return FALSE for level type FpaC_LVL_NOTUSED */
		/*  or for unknown level types                  */
		default:
			return FALSE;
		}
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Elements block of configuration file)   *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ e l e m e n t s _ i n f o                                *
*   r e a d _ e l e m e n t _ d e t a i l e d _ i n f o                *
*                                                                      *
*   Read information from Elements block of configuration files.       *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_elements_info

	(
	)

	{
	FILE							*fpcfg;
	STRING							cline, cmd, arg;
	int								numbrace, section_id, section, macro;
	LOGICAL							firstline, valid, validbegin, validend;
	FpaConfigElementStruct			*edef;
	FpaConfigElementIOStruct		*eio;
	FpaConfigElementTimeDepStruct	*etdep;
	int								nn;

	/* Read the configuration file(s) only once */
	if ( ElementsRead ) return ElementsValid;

	/* Force the Units block of the configuration file to be read first */
	if ( !read_units_info() ) return ElementsValid;

	/* Force the Groups block of the configuration file to be read next */
	if ( !read_groups_info() ) return ElementsValid;

	/* Find and open the configuration file for the Elements block */
	if ( !first_config_file_open(FpaCelementsFile, &fpcfg) )
		{
		ElementsRead = TRUE;
		return ElementsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Elements block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Elements block of configuration file */
		if ( same(cmd, FpaCblockElements) )
			{

			/* Set counter and identifier for Elements block */
			numbrace   = 0;
			section    = FpaCblockElementsName;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Elements block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Elements block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Elements declaration */
				else if ( numbrace == 1 )
					{

					/* Adding another name in FpaCblockElementsName section */
					if ( section_id == FpaCblockElementsName )
						{

						/* Check for declaration already in the list */
						edef = find_element(cmd);

						/* Add another element name to the lists */
						if ( IsNull(edef) )
							{
							edef = init_element(cmd);
							}

						/* Check that element name is not an alias */
						/*  of another element!                    */
						else if ( !same(cmd, edef->name) )
							{
							(void) config_file_message(FpaCblockElements,
									cmd, edef->name,
									FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set location of this block */
						(void) set_element_location(fpcfg, edef);

						/* Set identifier for next section of Elements block */
						section = FpaCblockElementsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockElements,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Elements declarations */
				/*  ... with format of "cmd = value(s)"    */
				else if ( numbrace == 2 )
					{

					/* Adding parameters in FpaCblockElementsInfo section */
					if ( section_id == FpaCblockElementsInfo )
						{

						/* Element label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									edef->label = STRMEM(edef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									edef->sh_label =
											STRMEM(edef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									edef->description =
											STRMEM(edef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element group */
						else if ( same(cmd, FpaCelementGroup) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg         = strdup_arg(cline);
								edef->group =
										identify_group(FpaCblockElements, arg);
								FREEMEM(arg);
								if ( IsNull(edef->group) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCelementGroup, FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCelementGroup, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Cannot reset level type for element! */
						else if ( same(cmd, FpaClevelType)
								&& edef->lvl_type != FpaCnoMacro )
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClevelType, FpaCmsgReset);
							}

						/* Level type for element */
						else if ( same(cmd, FpaClevelType) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
										NumFpaClevelTypes, FpaClevelTypes);
								if ( macro != FpaCnoMacro )
									{
									edef->lvl_type = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaClevelType, FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClevelType, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Default field group */
						else if ( same(cmd, FpaCfieldGroup) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg             = strdup_arg(cline);
								edef->fld_group =
										identify_group(FpaCblockFields, arg);
								FREEMEM(arg);
								if ( IsNull(edef->fld_group) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCfieldGroup, FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCfieldGroup, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Cannot reset element field type! */
						else if ( same(cmd, FpaCfieldType)
								&& edef->fld_type != FpaCnoMacro )
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCfieldType, FpaCmsgReset);
							}

						/* Element field type */
						else if ( same(cmd, FpaCfieldType) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
										NumFpaCfieldTypes, FpaCfieldTypes);
								if ( macro != FpaCnoMacro )
									{
									edef->fld_type = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCfieldType, FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCfieldType, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element display format */
						else if ( same(cmd, FpaCdisplayFormat) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
										NumFpaCdisplayFormats,
										FpaCdisplayFormats);
								if ( macro != FpaCnoMacro )
									{
									edef->display_format = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCdisplayFormat,
											FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCdisplayFormat, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element name aliases */
						else if ( same(cmd, FpaCalias) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Add all aliases to ident list */
								(void) add_element_aliases(cline, edef);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCalias, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Cannot reset element file ident! */
						else if ( same(cmd, FpaCfileIdent)
								&& !blank(edef->elem_io->fident) )
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCfileIdent, FpaCmsgReset);
							}

						/* Element file ident */
						else if ( same(cmd, FpaCfileIdent) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Set file ident and add it to ident list */
								(void) add_element_file_ident(cline, edef);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCfileIdent, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Cannot reset element file id! */
						else if ( same(cmd, FpaCfileId)
								&& !blank(edef->elem_io->fid) )
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCfileId, FpaCmsgReset);
							}

						/* Element file id */
						else if ( same(cmd, FpaCfileId) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{

								/* Set file id and add it to ident list */
								(void) add_element_fileid(cline, edef);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCfileId, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element file precision */
						else if ( same(cmd, FpaCprecision) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								eio = edef->elem_io;
								eio->precision = double_arg(cline, &valid);
								arg            = strdup_arg(cline);
								eio->units     = identify_unit(arg);
								FREEMEM(arg);
								if ( !valid )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCprecisionVal,
											FpaCmsgParameter);
									edef->valid = FALSE;
									}
								if ( IsNull(eio->units) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCprecisionUnit,
											FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCprecision, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element time dependence */
						else if ( same(cmd, FpaCtimeDependence) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);

								/* Set identifier for time dependence */
								/*  section of Elements block         */
								if ( blank(arg) )
									{
									etdep   = edef->elem_tdep;
									section = FpaCblockElementsTimeDep;
									}

								/* Otherwise only FpaC_NORMAL is acceptable */
								else
									{
									macro = config_file_macro(arg,
											NumFpaCtimeDepTypes,
											FpaCtimeDepTypes);
									if ( macro != FpaC_NORMAL )
										{
										(void) config_file_message(FpaCblockElements,
												edef->name,
												FpaCblank,
												FpaCtimeDependence,
												FpaCmsgParameter);
										edef->valid = FALSE;
										}
									else
										{
										edef->elem_tdep->time_dep    = macro;
										edef->elem_tdep->normal_time = 0.0;
										edef->elem_tdep->begin_time  = 0.0;
										edef->elem_tdep->end_time    = 0.0;
										edef->elem_tdep->units       =
												NullPtr(FpaConfigUnitStruct *);
										}
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCtimeDependence, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Skip all keywords for detailed data (for now) */
						else if ( same(cmd, FpaCwindClass)
								|| same(cmd, FpaClineTypesReset)
								|| same(cmd, FpaClineTypes)
								|| same(cmd, FpaCscatteredTypesReset)
								|| same(cmd, FpaCscatteredTypes)
								|| same(cmd, FpaCsubelementsReset)
								|| same(cmd, FpaCsubelements)
								|| same(cmd, FpaCattributesReset)
								|| same(cmd, FpaCattributes)
								|| same(cmd, FpaCeditor)
								|| same(cmd, FpaClabellingReset)
								|| same(cmd, FpaClabelling)
								|| same(cmd, FpaCsamplingReset)
								|| same(cmd, FpaCsampling)
								|| same(cmd, FpaCsample)
								|| same(cmd, FpaClinking)
								|| same(cmd, FpaCequation)
								|| same(cmd, FpaCvalueCalculation)
								|| same(cmd, FpaCcomponents) )
							{
							(void) skip_config_file_block(&fpcfg);
							}

						/* Set error flag for unrecognized Elements keyword */
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							edef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in low level Elements declarations */
				/*  ... with format of "cmd = value(s)"              */
				else
					{

					/* Adding parameters in FpaCblockElementsTimeDep section */
					if ( section_id == FpaCblockElementsTimeDep )
						{

						/* Element time dependence type */
						if ( same(cmd, FpaCtimeDepType) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
										NumFpaCtimeDepTypes, FpaCtimeDepTypes);
								if ( macro != FpaCnoMacro )
									{
									etdep->time_dep = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCtimeDepType, FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCtimeDepType, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Element time dependence range */
						else if ( same(cmd, FpaCtimeDepDailyRange) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								etdep->normal_time =
										double_arg(cline, &valid);
								etdep->begin_time  =
										double_arg(cline, &validbegin);
								etdep->end_time    =
										double_arg(cline, &validend);
								arg                = strdup_arg(cline);
								etdep->units       = identify_unit(arg);
								FREEMEM(arg);
								if ( !valid || !validbegin || !validend )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCtimeDepDailyRangeVal,
											FpaCmsgParameter);
									edef->valid = FALSE;
									}
								if ( IsNull(etdep->units)
										|| !convert_value(etdep->units->name, 1,
															Hrs, NullDouble) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCtimeDepDailyRangeUnit,
											FpaCmsgParameter);
									edef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCtimeDepDailyRange, FpaCmsgNoEqual);
								edef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Elements keyword */
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCtimeDependence,
									cmd, FpaCmsgKeyword);
							edef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Error check for each member of Elements block */
	for ( nn=0; nn<NumElementDef; nn++ )
		{
		edef = ElementDefs[nn];

		/* Ensure that "level_type" has been set */
		if ( edef->lvl_type == FpaCnoMacro )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockElements, edef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaClevelType, edef->name);
			edef->valid = FALSE;
			}

		/* Ensure that "field_type" has been set         */
		/* Note that the default field type is "Special" */
		if ( edef->fld_type == FpaCnoMacro )
			{
			(void) pr_warning("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockElements, edef->name);
			(void) pr_warning("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCfieldType, edef->name);
			edef->fld_type = FpaC_SPECIAL;
			}
		}

	/* Set flags for completion of reading */
	ElementsRead  = TRUE;
	ElementsValid = TRUE;
	return ElementsValid;
	}

/**********************************************************************/

static	FpaConfigElementStruct	*read_element_detailed_info

	(
	STRING		name		/* element name */
	)

	{
	int								nblk, nsub, natt, nlab;
	FILE							*fpcfg;
	STRING							cline, cmd, arg;
	int								fldtype, minterp;
	int								numbrace, section_id, section, macro;
	LOGICAL							firstline, valid, argok;
	FpaConfigElementStruct				*edef;
	FpaConfigElementDetailStruct		*edetail;
	FpaConfigElementLineTypeStruct		*ltypes;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigDefaultAttribStruct		*sattrib;
	FpaConfigEntryRuleStruct			*srule;
	FpaConfigElementAttribStruct		*attrib;
	FpaConfigElementEditorStruct		*editor;
	FpaConfigContinuousEditorStruct		*editContinuous;
	FpaConfigVectorEditorStruct			*editVector;
	FpaConfigDiscreteEditorStruct		*editDiscrete;
	FpaConfigWindEditorStruct			*editWind;
	FpaConfigLineEditorStruct			*editLine;
	FpaConfigScatteredEditorStruct		*editScattered;
	FpaConfigLchainEditorStruct			*editLchain;
	FpaConfigElementLabellingStruct		*labelling;
	FpaConfigDefaultAttribStruct		*lattrib;
	FpaConfigEntryRuleStruct			*lrule;
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigContinuousSamplingStruct	*sampContinuous;
	FpaConfigVectorSamplingStruct		*sampVector;
	FpaConfigDiscreteSamplingStruct		*sampDiscrete;
	FpaConfigWindSamplingStruct			*sampWind;
	FpaConfigLineSamplingStruct			*sampLine;
	FpaConfigScatteredSamplingStruct	*sampScattered;
	FpaConfigLchainSamplingStruct		*sampLchain;
	FpaConfigElementLinkingStruct		*linking;
	FpaConfigElementEquationStruct		*equation;
	FpaConfigElementValCalcStruct		*valcalc;
	FpaConfigElementComponentStruct		*components;
	FpaConfigCrossRefStruct				*crdef;
	FpaConfigSampleStruct				*xdef;

	/* Find the pointer to the element name */
	edef = identify_element(name);

	/* Return Null if element name not found */
	if ( IsNull(edef) ) return NullPtr(FpaConfigElementStruct *);

	/* Return pointer to structure if detailed information has been read */
	if ( NotNull(edef->elem_detail) ) return edef;

	/* Force the CrossRefs block of the configuration file to be read first */
	if ( !read_crossrefs_info() ) return edef;

	/* Force the Samples block of the configuration file to be read next */
	if ( !read_samples_info() ) return edef;

	/* Add space for detailed information */
	edef->valid_detail = TRUE;
	edef->elem_detail  = init_element_detail();
	edetail            = edef->elem_detail;
	fldtype            = edef->fld_type;

	/* Initialize blocks for attributes, labelling, sampling, and linking */
	edetail->attributes = init_element_attributes(fldtype);
	edetail->labelling  = init_element_labelling(fldtype);
	edetail->sampling   = init_element_sampling(fldtype);
	edetail->linking    = init_element_linking(fldtype);

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Elements block for \"%s\"!\n", name);

	/* Read all blocks from configuration files for this Elements declaration */
	for ( nblk=0; nblk<edef->nblocks; nblk++ )
		{

		/* Re-open and position configuration file for this Elements block */
		fpcfg = NullPtr(FILE *);
		if ( !config_file_open(edef->filenames[nblk], &fpcfg)
				|| fseek(fpcfg, edef->locations[nblk], SEEK_SET) != 0 )
			{
			return NullPtr(FpaConfigElementStruct *);
			}

		/* Diagnostic message */
		(void) pr_diag("Config", "  ... beginning at \"%d\" in file \"%s\"\n",
				edef->locations[nblk], edef->filenames[nblk]);

		/* Set counter and identifier for Elements declaration in Elements block */
		numbrace   = 0;
		section    = FpaCblockElementsInfo;
		section_id = FpaCnoSection;
		firstline  = TRUE;

		/* Read block containing this Elements declaration line by line */
		while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
			{

			/* Extract the first argument from the current line */
			cmd = string_arg(cline);

			/* The first line should be an open bracket */
			if ( firstline )
				{
				firstline = FALSE;
				if ( !same(cmd, FpaCopenBrace) ) break;
				}

			/* Increment counter for open brackets */
			/*  and save the section identifier    */
			if ( same(cmd, FpaCopenBrace) )
				{
				numbrace++;
				section_id = push_section(section);
				}

			/* Decrement counter for close brackets */
			/*  and reset the section identifier    */
			else if ( same(cmd, FpaCcloseBrace) )
				{
				numbrace--;
				section_id = pop_section();

				/* Check for end of Elements block */
				if ( numbrace == 0 ) break;
				}

			/* Set parameters in Elements declaration */
			/*  ... with format of "cmd = value(s)"   */
			else if ( numbrace == 1 )
				{

				/* Adding parameters in FpaCblockElementsInfo section */
				if ( section_id == FpaCblockElementsInfo )
					{

					/* Element wind class */
					if ( same(cmd, FpaCwindClass) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							macro = config_file_macro(arg,
									NumFpaCwindClasses, FpaCwindClasses);
							if ( macro != FpaCnoMacro )
								{
								edetail->wd_class = macro;
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCwindClass, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCwindClass, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element line types block reset */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaClineTypesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements line types block */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->line_types =
									free_element_line_types(edetail->line_types);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClineTypesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClineTypesReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Element subelement block reset */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCsubelementsReset) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClineTypesReset,
								FpaCsubelementsReset, FpaCmsgReplace);
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements line types block */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->line_types =
									free_element_line_types(edetail->line_types);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsubelementsReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsubelementsReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Element line types block */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaClineTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize line types block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->line_types) )
									{
									edetail->line_types =
											init_element_line_types();
									}
								ltypes  = edetail->line_types;
								section = FpaCblockElementsLineTypes;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty line types block */
									edetail->line_types =
										free_element_line_types(edetail->line_types);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaClineTypes, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClineTypes, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Element subelements block */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCsubelements) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClineTypes,
								FpaCsubelements, FpaCmsgReplace);
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize line types block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->line_types) )
									{
									edetail->line_types =
											init_element_line_types();
									}
								ltypes  = edetail->line_types;
								section = FpaCblockElementsLineTypes;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty line types block */
									edetail->line_types =
										free_element_line_types(edetail->line_types);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCsubelements, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsubelements, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Element scattered types block reset */
					else if ( fldtype == FpaC_SCATTERED
							&& same(cmd, FpaCscatteredTypesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements scattered types block */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->scattered_types =
									free_element_scattered_types(edetail->scattered_types);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element scattered types block */
					else if ( fldtype == FpaC_SCATTERED
							&& same(cmd, FpaCscatteredTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize scattered types block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->scattered_types) )
									{
									edetail->scattered_types =
											init_element_scattered_types();
									}
								stypes  = edetail->scattered_types;
								section = FpaCblockElementsScatteredTypes;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty scattered types block */
									edetail->scattered_types =
										free_element_scattered_types(edetail->scattered_types);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCscatteredTypes, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypes, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element attributes block reset */
					else if ( same(cmd, FpaCattributesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements attributes block           */
							/*  ... and reinitialize default attributes! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->attributes =
									free_element_attributes(edetail->attributes);
								edetail->attributes =
									init_element_attributes(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCattributesReset, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCattributesReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element attributes block */
					else if ( same(cmd, FpaCattributes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for attributes */
							if ( blank(arg) )
								{
								attrib  = edetail->attributes;
								section = FpaCblockElementsAttributes;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set attributes block with default values */
									edetail->attributes =
										free_element_attributes(edetail->attributes);
									edetail->attributes =
										init_element_attributes(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCattributes, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCattributes, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element editor block */
					else if ( same(cmd, FpaCeditor) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize editor block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->editor) )
									{
									edetail->editor =
											init_element_editor(fldtype);
									}
								if ( IsNull(edetail->editor) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCeditor, FpaCmsgSupport);
									(void) skip_config_file_block(&fpcfg);
									}
								editor  = edetail->editor;
								section = FpaCblockElementsEditor;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty editor block */
									edetail->editor =
											free_element_editor(fldtype,
													edetail->editor);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCeditor, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditor, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element labelling block reset */
					else if ( same(cmd, FpaClabellingReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling block                    */
							/*  ... and reinitialize default labelling! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->labelling =
										free_element_labelling(edetail->labelling);
								edetail->labelling =
										init_element_labelling(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClabellingReset, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element labelling block */
					else if ( same(cmd, FpaClabelling) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize labelling block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->labelling) )
									{
									edetail->labelling =
											init_element_labelling(fldtype);
									}
								labelling = edetail->labelling;
								section   = FpaCblockElementsLabelling;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set labelling block with default values */
									edetail->labelling =
											free_element_labelling(edetail->labelling);
									edetail->labelling =
											init_element_labelling(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaClabelling, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabelling, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element sampling block reset */
					else if ( same(cmd, FpaCsamplingReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset sampling block                    */
							/*  ... and reinitialize default sampling! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->sampling =
										free_element_sampling(fldtype,
												edetail->sampling);
								edetail->sampling =
										init_element_sampling(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingReset, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is not quite ready yet! <<< */
					/* Element sampling block */
					else if ( same(cmd, FpaCsampling) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCsampling, FpaCmsgSupport);
						(void) pr_warning("Config",
								"     Use keyword \"%s\" for now!\n",
								FpaCsample);
						(void) skip_config_file_block(&fpcfg);
						}
				/* >>> the preceding is not quite ready yet! <<< */

					/* Element sample block */
					else if ( same(cmd, FpaCsample) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for sampling */
							if ( blank(arg) )
								{
								sampling = edetail->sampling;
								section  = FpaCblockElementsSampling;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set sampling block with default values */
									edetail->sampling =
											free_element_sampling(fldtype,
													edetail->sampling);
									edetail->sampling =
											init_element_sampling(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCsample, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsample, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element linking block reset */
					else if ( same(cmd, FpaClinkingReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset linking block                    */
							/*  ... and reinitialize default linking! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->linking =
										free_element_linking(fldtype,
												edetail->linking);
								edetail->linking =
										init_element_linking(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClinkingReset, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClinkingReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element linking block */
					else if ( same(cmd, FpaClinking) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for linking */
							if ( blank(arg) )
								{
								linking = edetail->linking;
								section = FpaCblockElementsLinking;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set linking block with default values */
									edetail->linking =
											free_element_linking(fldtype,
													edetail->linking);
									edetail->linking =
											init_element_linking(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaClinking, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClinking, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element equation block */
					else if ( same(cmd, FpaCequation) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize equation block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->equation) )
									{
									edetail->equation =
											init_element_equation();
									}
								equation = edetail->equation;
								section  = FpaCblockElementsEquation;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCequation, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}

								/* Remove an existing equation if requested! */
								else if ( NotNull(edetail->equation) )
									{
									edetail->equation =
										free_element_equation(edetail->equation);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCequation, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element value calculation block */
					else if ( same(cmd, FpaCvalueCalculation) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize value calculation block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->valcalc) )
									{
									edetail->valcalc =
											init_element_valcalc();
									}
								valcalc = edetail->valcalc;
								section = FpaCblockElementsValCalc;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCvalueCalculation,
											FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}

								/* Remove an existing value calculation */
								/*  if requested!                       */
								else if ( NotNull(edetail->valcalc) )
									{
									edetail->valcalc =
										free_element_valcalc(edetail->valcalc);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCvalueCalculation, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element components block */
					else if ( same(cmd, FpaCcomponents) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize components block */
							/*  of Elements block          */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->components) )
									{
									edetail->components =
											init_element_components();
									}
								components = edetail->components;
								section = FpaCblockElementsComponents;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCcomponents, FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}

								/* But cannot reset components to None! */
								else if ( NotNull(edetail->components) )
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCcomponents, FpaCmsgResetNone);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCcomponents, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Skip all other keywords */
					else
						{
						(void) skip_config_file_block(&fpcfg);
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, cmd, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Elements declaration */
			/*  ... with format of "cmd = value(s)"             */
			else if ( numbrace == 2 )
				{

				/* Adding parameters in FpaCblockElementsLineTypes section */
				if ( section_id == FpaCblockElementsLineTypes )
					{

					/* Add another line type name (if not already there!) */
					nsub = add_element_line_type(cmd, ltypes);
					if ( nsub < 0 )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClineTypes,
								cmd, FpaCmsgMember);
						edef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockElementsLineTypesSet;
					}

				/* Adding parameters in FpaCblockElementsScatteredTypes section */
				else if ( section_id == FpaCblockElementsScatteredTypes )
					{

					/* Add another scattered type name (if not already there!) */
					nsub = add_element_scattered_type(cmd, stypes);
					if ( nsub < 0 )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCscatteredTypes,
								cmd, FpaCmsgMember);
						edef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockElementsScatteredTypesSet;
					}

				/* Adding parameters in FpaCblockElementsAttributes section */
				else if ( section_id == FpaCblockElementsAttributes )
					{

					/* Add another attribute name (if not already there!) */
					natt = add_element_attribute(cmd, cline, attrib);
					if ( natt < 0 )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCattributes,
								cmd, FpaCmsgMember);
						edef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockElementsAttribsSet;
					}

				/* Adding parameters in FpaCblockElementsEditor section */
				else if ( section_id == FpaCblockElementsEditor )
					{

					/* First add the parameters common to all fields */

					/* Editor entry file */
					if ( same(cmd, FpaCeditorEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_entry_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorEntryFile, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorEntryFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor modify file */
					else if ( same(cmd, FpaCeditorModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_modify_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorModifyFile, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorModifyFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor memory file */
					else if ( same(cmd, FpaCeditorMemoryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_memory_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorMemoryFile, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorMemoryFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor background entry file */
					else if ( same(cmd, FpaCeditorBackEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_back_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorBackEntryFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorBackEntryFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor background memory file */
					else if ( same(cmd, FpaCeditorBackMemoryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_back_mem_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorBackMemoryFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorBackMemoryFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor entry rules reset */
					else if ( same(cmd, FpaCeditorEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor python entry rules reset */
					else if ( same(cmd, FpaCeditorPyEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_py_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPyEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorPyEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor entry rules */
					else if ( same(cmd, FpaCeditorEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add editor entry rules to list */
							if ( !add_element_editor_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorEntryRules, FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorEntryRules, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor python entry rules */
					else if ( same(cmd, FpaCeditorPyEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add editor entry rules to list */
							if ( !add_element_editor_py_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPyEntryRules, FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorPyEntryRules, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor merge fields reset */
					else if ( same(cmd, FpaCeditorMergeFieldsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor merge fields list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_merge_fields(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorMergeFieldsReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorMergeFieldsReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Editor merge fields */
					else if ( same(cmd, FpaCeditorMergeFields) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add editor merge fields to list */
							if ( !add_element_editor_merge_fields(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorMergeFields,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorMergeFields, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Then add the parameters for particular fields */

					/* Continuous field type ... hi/lo */
					else if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCeditorHilo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editContinuous = editor->type.continuous;
							editContinuous->hilo =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorHilo, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorHilo, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Continuous field type ... poke */
					else if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCeditorPoke) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editContinuous = editor->type.continuous;
							editContinuous->poke  =
									double_arg(cline, &valid);
							arg                   = strdup_arg(cline);
							editContinuous->units = identify_unit(arg);
							FREEMEM(arg);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPokeVal, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							if ( IsNull(editContinuous->units) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPokeUnit, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorPoke, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... hi/lo */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCeditorHilo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editVector = editor->type.vector;
							editVector->hilo =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorHilo, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorHilo, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... poke magnitude */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCeditorMagnitudePoke) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editVector = editor->type.vector;
							editVector->mag_poke  =
									double_arg(cline, &valid);
							arg                  = strdup_arg(cline);
							editVector->mag_units = identify_unit(arg);
							FREEMEM(arg);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPokeVal, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							if ( IsNull(editVector->mag_units) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPokeUnit, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorMagnitudePoke, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... poke direction */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCeditorDirectionPoke) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editVector = editor->type.vector;
							editVector->dir_poke  =
									double_arg(cline, &valid);
							arg                   = strdup_arg(cline);
							editVector->dir_units = identify_unit(arg);
							FREEMEM(arg);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPokeVal, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							if ( IsNull(editVector->dir_units) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPokeUnit, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorDirectionPoke, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... subelement list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorSubelementList) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorSubelementList, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... background list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorBackgroundList) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorBackgroundList, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... background */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorBackground) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorBackground, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Discrete field type ... overlaying */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorOverlaying) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editDiscrete = editor->type.discrete;
							editDiscrete->overlaying =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorOverlaying, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorOverlaying, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Discrete or Wind field types ... display ordering */
					else if ( ( fldtype == FpaC_DISCRETE
								|| fldtype == FpaC_WIND )
							&& same(cmd, FpaCeditorDisplayOrder) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editDiscrete = editor->type.discrete;
							editDiscrete->display_order =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorDisplayOrder,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorDisplayOrder, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node entry file */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_node_entry_file(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorNodeEntryFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorNodeEntryFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node modify file */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_node_modify_file(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorNodeModifyFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorNodeModifyFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node entry rules reset */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset node entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_node_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorNodeEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorNodeEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node entry rules */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add node entry rules to list */
							if ( !add_element_editor_node_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorNodeEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorNodeEntryRules, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... python node entry rules reset */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorPyNodeEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset node entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_py_node_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPyNodeEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorPyNodeEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... python node entry rules */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorPyNodeEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add node entry rules to list */
							if ( !add_element_editor_py_node_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorPyNodeEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorPyNodeEntryRules, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... editor interpolation delta */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorLchainInterpDelta) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							minterp = interpret_hour_minute_string(cline);
							if ( minterp > 0 )
								{
								editLchain = editor->type.lchain;
								editLchain->minterp = minterp;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										edef->name, FpaCblank,
										FpaCeditorLchainInterpDelta,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									edef->name, FpaCblank,
									FpaCeditorLchainInterpDelta, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... editor merge link fields reset */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorLinkFieldsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor merge link fields list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_link_fields(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorLinkFieldsReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorLinkFieldsReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... editor merge link fields */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorLinkFields) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add editor merge link fields to list */
							if ( !add_element_editor_link_fields(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCeditorLinkFields,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCeditorLinkFields, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... menu */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorMenu) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorMenu, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... menu file */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorMenuFile) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorMenuFile, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... menu memory */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorMenuMemory) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorMenuMemory, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Wind field type ... wind cross reference list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCeditorWindList) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorWindList, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Wind field type ... background list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCeditorWindBackgroundList) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorWindBackgroundList, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Wind field type ... background */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCeditorWindBackground) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorWindBackground, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Line field type ... subelement_reset section */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCeditorSubelementReset) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorSubelementReset, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Line field type ... subelement section */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCeditorSubelement) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCeditorSubelement, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCeditor,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsLabelling section */
				else if ( section_id == FpaCblockElementsLabelling )
					{

					/* Labelling types reset */
					if ( same(cmd, FpaClabellingTypesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types list                     */
							/*  ... and reinitialize default labelling types! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_labelling_types(labelling);
								(void) init_element_labelling_types(fldtype,
																	labelling);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClabellingTypesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingTypesReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling types */
					else if ( same(cmd, FpaClabellingTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for labelling types */
							if ( blank(arg) )
								{
								section = FpaCblockElementsLabelTypes;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set labelling types with default values */
									(void) free_element_labelling_types(labelling);
									(void) init_element_labelling_types(fldtype,
																	labelling);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaClabellingTypes,
											FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingTypes, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClabelling,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsSampling section */
				else if ( section_id == FpaCblockElementsSampling )
					{

					/* Continuous field type ... value type list */
					if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						/* Set continuous sampling value type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampContinuous = sampling->type.continuous;
							if ( !add_continuous_sample_values(cline,
									sampContinuous) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingValueSampleTypes,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingValueSampleTypes,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Continuous field type ... wind type list */
					else if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCsamplingWindSampleTypes) )
						{
						/* Set continuous sampling wind type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampContinuous = sampling->type.continuous;
							if ( !add_continuous_sample_winds(cline,
									sampContinuous) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingWindSampleTypes,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingWindSampleTypes,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... value type list */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						/* Set vector sampling value type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampVector = sampling->type.vector;
							if ( !add_vector_sample_values(cline,
									sampVector) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingValueSampleTypes,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingValueSampleTypes,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... wind type list */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCsamplingWindSampleTypes) )
						{
						/* Set vector sampling wind type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampVector = sampling->type.vector;
							if ( !add_vector_sample_winds(cline,
									sampVector) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingWindSampleTypes,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingWindSampleTypes,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Discrete field type ... attribute list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set discrete sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampDiscrete = sampling->type.discrete;
							if ( !add_discrete_sample_attribs(cline,
									sampDiscrete) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Wind field type ... value type list */
					/* >>>>> has this become obsolete??? <<<<< */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						/* Set wind sampling value type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampWind = sampling->type.wind;
							if ( !add_wind_sample_values(cline, sampWind) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingValueSampleTypes,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingValueSampleTypes,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Wind field type ... wind cross reference list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCsamplingWindCrossRefs) )
						{
						/* Set wind sampling wind cross reference list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampWind = sampling->type.wind;
							if ( !add_wind_sample_crossrefs(cline, sampWind) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingWindCrossRefs,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingWindCrossRefs, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Wind field type ... wind type */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCsamplingWindSampleType) )
						{
						/* Set wind sampling wind cross reference list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampWind = sampling->type.wind;
							arg      = strdup_arg(cline);
							xdef     = identify_sample(FpaCsamplesWinds, arg);
							FREEMEM(arg);
							if ( NotNull(xdef)
									&& same(xdef->samp_func,
												FpaDefaultWindFunc) )
								{
								sampWind->windsample = xdef;
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingWindSampleType,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingWindSampleType, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Line field type ... attribute list */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set line sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampLine = sampling->type.line;
							if ( !add_line_sample_attribs(cline, sampLine) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered field type ... attribute list */
					else if ( fldtype == FpaC_SCATTERED
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set scattered sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampScattered = sampling->type.scattered;
							if ( !add_scattered_sample_attribs(cline,
									sampScattered) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... attribute list */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set link chain sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampLchain = sampling->type.lchain;
							if ( !add_lchain_sample_attribs(cline, sampLchain) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... value type list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCsamplingValueSampleTypes, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Line field type ... value type list */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCsamplingValueSampleTypes, FpaCmsgObsolete);
						edef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCsample,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsLinking section */
				else if ( section_id == FpaCblockElementsLinking )
					{

					/* Linking interpolation delta */
					if ( same(cmd, FpaClinkingInterpDelta) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							minterp = interpret_hour_minute_string(cline);
							if ( minterp > 0 )
								{
								linking->minterp = minterp;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										edef->name, FpaCblank,
										FpaClinkingInterpDelta,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									edef->name, FpaCblank,
									FpaClinkingInterpDelta, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Linking fields reset */
					else if ( same(cmd, FpaClinkingFieldsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset linking fields list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_link_fields(fldtype,
																	linking);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClinkingFieldsReset, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClinkingFieldsReset, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Linking fields */
					else if ( same(cmd, FpaClinkingFields) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add link fields to list */
							if ( !add_element_link_fields(cline,
															fldtype, linking) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClinkingFields, FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClinkingFields, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClinking, cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsEquation section */
				else if ( section_id == FpaCblockElementsEquation )
					{

					/* Element equation force calculation */
					if ( same(cmd, FpaCequationForceCalc) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							equation->force = logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCequationForceCalc,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCequationForceCalc, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element equation string */
					else if ( same(cmd, FpaCequationString) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							equation->eqtn = STRMEM(equation->eqtn, cline);
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCequationString, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element equation units */
					else if ( same(cmd, FpaCequationUnits) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg             = strdup_arg(cline);
							equation->units = identify_unit(arg);
							FREEMEM(arg);
							if ( IsNull(equation->units) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCequationUnits, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCequationUnits, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCequation,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsValCalc section */
				else if ( section_id == FpaCblockElementsValCalc )
					{

					/* Element value calculation force calculation */
					if ( same(cmd, FpaCvalCalcForceCalc) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							valcalc->force = logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCvalCalcForceCalc,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCvalCalcForceCalc, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element value calculation crossreference */
					else if ( same(cmd, FpaCvalCalcCrossRef) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg   = strdup_arg(cline);
							crdef = identify_crossref(FpaCcRefsValues, arg);
							FREEMEM(arg);

							if ( NotNull(crdef) )
								{
								valcalc->vcalc = crdef;
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCvalCalcCrossRef, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCvalCalcCrossRef, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element value calculation source type(s) */
					else if ( same(cmd, FpaCvalCalcSrcTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_vcalc_src_types(cline, valcalc) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCvalCalcSrcTypes, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCvalCalcSrcTypes, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCvalueCalculation,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsComponents section */
				else if ( section_id == FpaCblockElementsComponents )
					{

					/* Element components x component  */
					/* Note that type is reset to x/y! */
					if ( same(cmd, FpaCcomponentsXcomp) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg               = string_arg(cline);
							components->cinfo = &(XYCompInfo);

							if ( !set_components_component(arg, X_Comp,
									components) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCcomponentsXcomp, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCcomponentsXcomp, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element components y component  */
					/* Note that type is reset to x/y! */
					else if ( same(cmd, FpaCcomponentsYcomp) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg               = string_arg(cline);
							components->cinfo = &(XYCompInfo);

							if ( !set_components_component(arg, Y_Comp,
									components) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCcomponentsYcomp, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCcomponentsYcomp, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element components direction component          */
					/* Note that type is reset to direction/magnitude! */
					else if ( same(cmd, FpaCcomponentsDcomp) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg               = string_arg(cline);
							components->cinfo = &(DMCompInfo);

							if ( !set_components_component(arg, D_Comp,
									components) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCcomponentsDcomp, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCcomponentsDcomp, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Element components magnitude component          */
					/* Note that type is reset to direction/magnitude! */
					else if ( same(cmd, FpaCcomponentsMcomp) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg               = string_arg(cline);
							components->cinfo = &(DMCompInfo);

							if ( !set_components_component(arg, M_Comp,
									components) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCcomponentsMcomp, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCcomponentsMcomp, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCcomponents,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, cmd, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Elements declaration */
			/*  ... with format of "cmd = value(s)"             */
			else if ( numbrace == 3 )
				{

				/* Adding parameters in FpaCblockElementsLineTypesSet section */
				if ( section_id == FpaCblockElementsLineTypesSet )
					{

					/* Line type label */
					if ( same(cmd, FpaClineTypesLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								ltypes->type_labels[nsub] =
										STRMEM(ltypes->type_labels[nsub], arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClineTypesLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Subelement label */
					else if ( same(cmd, FpaCsubelementsLabel) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClineTypesLabel,
								FpaCsubelementsLabel, FpaCmsgReplace);
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								ltypes->type_labels[nsub] =
										STRMEM(ltypes->type_labels[nsub], arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCsubelementsLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Line type short label */
					else if ( same(cmd, FpaClineTypesShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								ltypes->type_sh_labels[nsub] =
										STRMEM(ltypes->type_sh_labels[nsub],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClineTypesShortLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Line types pattern file */
					else if ( same(cmd, FpaClineTypesPattern) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( OKARG(arg) )
								{
								ltypes->patterns[nsub] =
										STRMEM(ltypes->patterns[nsub], arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaClineTypesPattern,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClineTypesPattern, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClineTypes,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsScatteredTypesSet section */
				else if ( section_id == FpaCblockElementsScatteredTypesSet )
					{

					/* Scattered type label */
					if ( same(cmd, FpaCscatteredTypesLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_labels[nsub] =
										STRMEM(stypes->type_labels[nsub], arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type short label */
					else if ( same(cmd, FpaCscatteredTypesShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_sh_labels[nsub] =
										STRMEM(stypes->type_sh_labels[nsub],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesShortLabel,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type style class */
					else if ( same(cmd, FpaCscatteredTypesClass) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_classes[nsub] =
										STRMEM(stypes->type_classes[nsub], arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesClass,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesClass, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered types entry file */
					else if ( same(cmd, FpaCscatteredTypesEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_entry_files[nsub] =
										STRMEM(stypes->type_entry_files[nsub],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesEntryFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesEntryFile,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered types modify file */
					else if ( same(cmd, FpaCscatteredTypesModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_modify_files[nsub] =
										STRMEM(stypes->type_modify_files[nsub],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesModifyFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesModifyFile,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered types attach option */
					else if ( same(cmd, FpaCscatteredTypesAttach) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg   = string_arg(cline);
							macro = config_file_macro(arg,
											NumFpaCattachOpts, FpaCattachOpts);
							if ( macro != FpaCnoMacro
									&& check_attach_option(fldtype,
											macro, (SPFEAT *) 0))
								{
								stypes->type_attach_opts[nsub] = macro;
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesAttach,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesAttach, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type default attributes reset */
					else if ( same(cmd, FpaCscatteredTypesAttribDefaultsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset scattered types default attributes list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								sattrib = &(stypes->type_attribs[nsub]);
								(void) free_element_type_attribs(sattrib);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesAttribDefaultsReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesAttribDefaultsReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type default attributes */
					else if ( same(cmd, FpaCscatteredTypesAttribDefaults) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for scattered types */
							/*  default attributes                 */
							if ( blank(arg) )
								{
								sattrib = &(stypes->type_attribs[nsub]);
								section = FpaCblockElementsScatteredAttribsSet;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Remove scattered types default attributes */
									sattrib = &(stypes->type_attribs[nsub]);
									(void) free_element_type_attribs(sattrib);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name, FpaCblank,
											FpaCscatteredTypesAttribDefaults,
											FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesAttribDefaults,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type entry rules reset */
					else if ( same(cmd, FpaCscatteredTypesEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset scattered types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								srule = &(stypes->type_rules[nsub]);
								(void) free_element_type_rules(srule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type entry rules */
					else if ( same(cmd, FpaCscatteredTypesEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add scattered type entry rules to list */
							srule = &(stypes->type_rules[nsub]);
							if ( !add_element_type_rules(cline, srule) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesEntryRules,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type python entry rules reset */
					else if ( same(cmd, FpaCscatteredTypesPyEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset scattered types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								srule = &(stypes->type_rules[nsub]);
								(void) free_element_type_py_rules(srule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesPyEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesPyEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Scattered type python entry rules */
					else if ( same(cmd, FpaCscatteredTypesPyEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add scattered type entry rules to list */
							srule = &(stypes->type_rules[nsub]);
							if ( !add_element_type_py_rules(cline, srule) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCscatteredTypesPyEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCscatteredTypesPyEntryRules,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCscatteredTypes,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsAttribsSet section */
				else if ( section_id == FpaCblockElementsAttribsSet )
					{

					/* Attribute label */
					if ( same(cmd, FpaCattributesLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								attrib->attrib_labels[natt] =
										STRMEM(attrib->attrib_labels[natt],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCattributesLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Attribute short label */
					else if ( same(cmd, FpaCattributesShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								attrib->attrib_sh_labels[natt] =
										STRMEM(attrib->attrib_sh_labels[natt],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCattributesShortLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Attribute default value */
					else if ( same(cmd, FpaCattributesBackDefault) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_attribute_backdef(cline,
									attrib->attrib_names[natt],
									&(attrib->attrib_back_defs[natt])) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, FpaCblank,
										FpaCattributesBackDefault,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaCattributesBackDefault, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCattributes,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsLabelTypes section */
				else if ( section_id == FpaCblockElementsLabelTypes )
					{

					/* Add another labelling type (if not already there!) */
					nlab = add_element_labelling_type(cmd, labelling);
					if ( nlab < 0 )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClabellingTypes,
								cmd, FpaCmsgMember);
						edef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockElementsLabelTypesSet;
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, cmd, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Elements declaration */
			/*  ... with format of "cmd = value(s)"             */
			else if ( numbrace == 4 )
				{

				/* Adding parameters in FpaCblockElementsScatteredAttribsSet section */
				if ( section_id == FpaCblockElementsScatteredAttribsSet )
					{
					if ( !add_element_type_attrib(cmd, cline, sattrib) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaCblank,
								FpaCscatteredTypesAttribDefaults,
								FpaCmsgParameter);
						edef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockElementsLabelTypesSet section */
				else if ( section_id == FpaCblockElementsLabelTypesSet )
					{

					/* Labelling type label */
					if ( same(cmd, FpaClabellingLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_labels[nlab] =
										STRMEM(labelling->type_labels[nlab],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type short label */
					else if ( same(cmd, FpaClabellingShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_sh_labels[nlab] =
										STRMEM(labelling->type_sh_labels[nlab],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingShortLabel, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling style class */
					else if ( same(cmd, FpaClabellingClass) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_classes[nlab] =
										STRMEM(labelling->type_classes[nlab],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingClass, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingClass, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling entry file */
					else if ( same(cmd, FpaClabellingEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_entry_files[nlab] =
										STRMEM(labelling->type_entry_files[nlab],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingEntryFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingEntryFile, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling modify file */
					else if ( same(cmd, FpaClabellingModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_modify_files[nlab] =
										STRMEM(labelling->type_modify_files[nlab],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingModifyFile,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingModifyFile,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type attach option */
					else if ( same(cmd, FpaClabellingAttach) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg   = string_arg(cline);
							macro = config_file_macro(arg,
											NumFpaCattachOpts, FpaCattachOpts);
							if ( macro != FpaCnoMacro
									&& check_attach_option(fldtype,
											macro, (SPFEAT *) 0))
								{
								labelling->type_attach_opts[nlab] = macro;
								}
							else
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingAttach, FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingAttach, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type default attributes reset */
					else if ( same(cmd, FpaClabellingAttribDefaultsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types default attributes list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								lattrib = &(labelling->type_attribs[nlab]);
								(void) free_element_type_attribs(lattrib);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingAttribDefaultsReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingAttribDefaultsReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type default attributes */
					else if ( same(cmd, FpaClabellingAttribDefaults) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for label default attributes */
							if ( blank(arg) )
								{
								lattrib = &(labelling->type_attribs[nlab]);
								section = FpaCblockElementsLabelAttribsSet;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Remove label default attributes */
									lattrib = &(labelling->type_attribs[nlab]);
									(void) free_element_type_attribs(lattrib);
									}
								else
									{
									(void) config_file_message(FpaCblockElements,
											edef->name,
											labelling->type_names[nlab],
											FpaClabellingAttribDefaults,
											FpaCmsgParameter);
									edef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingAttribDefaults,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type entry rules reset */
					else if ( same(cmd, FpaClabellingEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								lrule = &(labelling->type_rules[nlab]);
								(void) free_element_type_rules(lrule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type entry rules */
					else if ( same(cmd, FpaClabellingEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add labelling entry rules to list */
							lrule = &(labelling->type_rules[nlab]);
							if ( !add_element_type_rules(cline, lrule) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingEntryRules, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type python entry rules reset */
					else if ( same(cmd, FpaClabellingPyEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								lrule = &(labelling->type_rules[nlab]);
								(void) free_element_type_py_rules(lrule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingPyEntryRulesReset,
										FpaCmsgParameter);
								edef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingPyEntryRulesReset,
									FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Labelling type python entry rules */
					else if ( same(cmd, FpaClabellingPyEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add labelling entry rules to list */
							lrule = &(labelling->type_rules[nlab]);
							if ( !add_element_type_py_rules(cline, lrule) )
								{
								(void) config_file_message(FpaCblockElements,
										edef->name, labelling->type_names[nlab],
										FpaClabellingPyEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockElements,
									edef->name, FpaCblank,
									FpaClabellingPyEntryRules, FpaCmsgNoEqual);
							edef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Elements keyword */
					else
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, FpaClabellingTypes,
								cmd, FpaCmsgKeyword);
						edef->valid_detail = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, cmd, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Elements declaration */
			/*  ... with format of "cmd = value(s)"             */
			else
				{

				/* Adding parameters in FpaCblockElementsLabelAttribsSet section */
				if ( section_id == FpaCblockElementsLabelAttribsSet )
					{
					if ( !add_element_type_attrib(cmd, cline, lattrib) )
						{
						(void) config_file_message(FpaCblockElements,
								edef->name, labelling->type_names[nlab],
								FpaClabellingAttribDefaults, FpaCmsgParameter);
						edef->valid_detail = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockElements,
							edef->name, cmd, FpaCblank, FpaCmsgSection);
					}
				}
			}

		/* Diagnostic message */
		(void) pr_diag("Config", "  ... ending at \"%d\" in file \"%s\"\n",
				ftell(fpcfg), edef->filenames[nblk]);

		/* Close the configuration file for this Elements declaration */
		(void) config_file_close(&fpcfg);
		}

	/* Return pointer when all configuration files have been read */
	return edef;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ e l e m e n t                                            *
*   i n i t _ e l e m e n t                                            *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Elements block, or pointer to initialized structure containing     *
*   information read from Elements block of configuration files.       *
*   Note that element name comparisons are case insensitive!           *
*                                                                      *
***********************************************************************/

static	FpaConfigElementStruct	*find_element

	(
	STRING		name		/* element name */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumElementIdent < 1 ) return NullPtr(FpaConfigElementStruct *);

	/* Copy the element name into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for element name */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) ElementIdents,
			(size_t) NumElementIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if element name found in list */
	return ( pident ) ? (FpaConfigElementStruct *) pident->pdef:
							NullPtr(FpaConfigElementStruct *);
	}

/**********************************************************************/

static	FpaConfigElementStruct	*init_element

	(
	STRING		name		/* element name */
	)

	{
	FpaConfigElementStruct	*edef;

	/* Add element at end of current ElementDefs list */
	NumElementDef++;
	ElementDefs = GETMEM(ElementDefs, FpaConfigElementStruct *, NumElementDef);
	ElementDefs[NumElementDef-1] = INITMEM(FpaConfigElementStruct, 1);

	/* Initialize ElementDefs structure */
	edef                         = ElementDefs[NumElementDef - 1];
	edef->name                   = strdup(name);
	edef->valid                  = TRUE;
	edef->nblocks                = 0;
	edef->filenames              = NullStringList;
	edef->locations              = NullLong;
	edef->label                  = strdup(name);
	edef->sh_label               = strdup(name);
	edef->description            = NullString;
	edef->group                  = identify_group(FpaCblockElements,
														FpaCmiscellaneous);
	edef->lvl_type               = FpaCnoMacro;
	edef->fld_group              = NullPtr(FpaConfigGroupStruct *);
	edef->fld_type               = FpaCnoMacro;
	edef->display_format         = DisplayFormatSimple;

	/* Initialize ElementIO structure in ElementDefs structure */
	edef->elem_io                = INITMEM(FpaConfigElementIOStruct, 1);
	edef->elem_io->check_fident  = FALSE;
	edef->elem_io->fident        = NullString;
	edef->elem_io->fid           = NullString;
	edef->elem_io->precision     = 0.01;
	edef->elem_io->units         = NullPtr(FpaConfigUnitStruct *);

	/* Initialize ElementTimeDep structure in ElementDefs structure */
	edef->elem_tdep              = INITMEM(FpaConfigElementTimeDepStruct, 1);
	edef->elem_tdep->time_dep    = FpaC_NORMAL;
	edef->elem_tdep->normal_time = 0.0;
	edef->elem_tdep->begin_time  = 0.0;
	edef->elem_tdep->end_time    = 0.0;
	edef->elem_tdep->units       = NullPtr(FpaConfigUnitStruct *);

	/* Set pointer to ElementDetail structure in ElementDefs structure */
	edef->valid_detail           = FALSE;
	edef->elem_detail            = NullPtr(FpaConfigElementDetailStruct *);

	/* Add the name as another identifier */
	(void) add_element_identifier(name, edef);

	/* Return pointer to ElementDefs structure */
	return edef;
	}

/***********************************************************************
*                                                                      *
*   s e t _ e l e m e n t _ l o c a t i o n                            *
*                                                                      *
*   Save configuration file name and location for reading detailed     *
*   information from Elements block of configuration files.            *
*                                                                      *
***********************************************************************/

static	LOGICAL					set_element_location

	(
	FILE					*fpcfg,		/* pointer to configuration file */
	FpaConfigElementStruct	*edef		/* pointer to Element structure */
	)

	{
	STRING		cfgname;
	long int	position;
	int			nblk;

	/* Return FALSE if no structure passed */
	if ( IsNull(edef) ) return FALSE;

	/* Get file name and location from configuration file */
	if ( !config_file_location(fpcfg, &cfgname, &position) ) return FALSE;

	/* Add configuration file name and location to list */
	nblk = edef->nblocks++;
	edef->filenames = GETMEM(edef->filenames, STRING,   edef->nblocks);
	edef->locations = GETMEM(edef->locations, long int, edef->nblocks);
	edef->filenames[nblk] = cfgname;
	edef->locations[nblk] = position;

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   i n i t _ e l e m e n t _ d e t a i l                              *
*                                                                      *
*   Return pointer to initialized structure for detailed information   *
*   read from Elements block of configuration files.                   *
*                                                                      *
*   i n i t _ e l e m e n t _ l i n e _ t y p e s                      *
*   i n i t _ e l e m e n t _ s c a t t e r e d _ t y p e s            *
*   i n i t _ e l e m e n t _ a t t r i b u t e s                      *
*   i n i t _ e l e m e n t _ e d i t o r                              *
*   i n i t _ e l e m e n t _ l a b e l l i n g                        *
*   i n i t _ e l e m e n t _ l a b e l l i n g _ t y p e s            *
*   i n i t _ e l e m e n t _ s a m p l i n g                          *
*   i n i t _ e l e m e n t _ l i n k i n g                            *
*   i n i t _ e l e m e n t _ e q u a t i o n                          *
*   i n i t _ e l e m e n t _ v a l c a l c                            *
*   i n i t _ e l e m e n t _ c o m p o n e n t s                      *
*                                                                      *
*   Return pointer to initialized structure for detailed information   *
*   read from subsections of Elements block of configuration files.    *
*                                                                      *
***********************************************************************/

static	FpaConfigElementDetailStruct	*init_element_detail

	(
	)

	{
	FpaConfigElementDetailStruct		*edetail;

	/* Initialize ElementDetail structure */
	edetail                  = INITMEM(FpaConfigElementDetailStruct, 1);
	edetail->wd_class        = FpaC_NOWIND;
	edetail->line_types      = NullPtr(FpaConfigElementLineTypeStruct *);
	edetail->scattered_types = NullPtr(FpaConfigElementScatteredTypeStruct *);
	edetail->attributes      = NullPtr(FpaConfigElementAttribStruct *);
	edetail->editor          = NullPtr(FpaConfigElementEditorStruct *);
	edetail->labelling       = NullPtr(FpaConfigElementLabellingStruct *);
	edetail->sampling        = NullPtr(FpaConfigElementSamplingStruct *);
	edetail->linking         = NullPtr(FpaConfigElementLinkingStruct *);
	edetail->equation        = NullPtr(FpaConfigElementEquationStruct *);
	edetail->valcalc         = NullPtr(FpaConfigElementValCalcStruct *);
	edetail->components      = NullPtr(FpaConfigElementComponentStruct *);

	/* Return pointer to ElementDetail structure */
	return edetail;
	}

/**********************************************************************/

static	FpaConfigElementLineTypeStruct	*init_element_line_types

	(
	)

	{
	FpaConfigElementLineTypeStruct		*ltypes;

	/* Initialize ElementLineType structure */
	ltypes = INITMEM(FpaConfigElementLineTypeStruct, 1);
	ltypes->ntypes         = 0;
	ltypes->type_names     = NullStringPtr;
	ltypes->type_labels    = NullStringPtr;
	ltypes->type_sh_labels = NullStringPtr;
	ltypes->patterns       = NullStringPtr;

	/* Return pointer to ElementLineType structure */
	return ltypes;
	}

/**********************************************************************/

static	FpaConfigElementScatteredTypeStruct	*init_element_scattered_types

	(
	)

	{
	FpaConfigElementScatteredTypeStruct	*stypes;

	/* Initialize ElementScatteredType structure */
	stypes = INITMEM(FpaConfigElementScatteredTypeStruct, 1);
	stypes->check_scattered   = FALSE;
	stypes->ntypes            = 0;
	stypes->type_names        = NullStringList;
	stypes->type_labels       = NullStringList;
	stypes->type_sh_labels    = NullStringList;
	stypes->type_classes      = NullStringList;
	stypes->type_entry_files  = NullStringList;
	stypes->type_modify_files = NullStringList;
	stypes->type_attach_opts  = NullPtr(FpaCattachOption *);
	stypes->type_attribs      = NullPtr(FpaConfigDefaultAttribStruct *);
	stypes->type_rules        = NullPtr(FpaConfigEntryRuleStruct *);

	/* Return pointer to ElementScatteredType structure */
	return stypes;
	}

/**********************************************************************/

static	FpaConfigElementAttribStruct	*init_element_attributes

	(
	int			type		/* enumerated field type */
	)


	{
	FpaConfigElementAttribStruct		*attribs;

	/* Initialize ElementAttrib structure */
	attribs = INITMEM(FpaConfigElementAttribStruct, 1);

	/* Set default attributes ... common to all enumerated field types */
	attribs->nattribs         = 6;
	attribs->attrib_names     = INITMEM(STRING, attribs->nattribs);
	attribs->attrib_labels    = INITMEM(STRING, attribs->nattribs);
	attribs->attrib_sh_labels = INITMEM(STRING, attribs->nattribs);
	attribs->attrib_back_defs = INITMEM(STRING, attribs->nattribs);
	attribs->attrib_names[0]     = strdup(AttribUserlabel);
	attribs->attrib_labels[0]    = strdup(AttribUserlabelLabel);
	attribs->attrib_sh_labels[0] = strdup(AttribUserlabelLabel);
	attribs->attrib_back_defs[0] = strdup(AttribUserlabelDefault);
	attribs->attrib_names[1]     = strdup(AttribAutolabel);
	attribs->attrib_labels[1]    = strdup(AttribAutolabelLabel);
	attribs->attrib_sh_labels[1] = strdup(AttribAutolabelLabel);
	attribs->attrib_back_defs[1] = strdup(AttribAutolabelDefault);
	attribs->attrib_names[2]     = strdup(AttribCategory);
	attribs->attrib_labels[2]    = strdup(AttribCategoryLabel);
	attribs->attrib_sh_labels[2] = strdup(AttribCategoryLabel);
	attribs->attrib_back_defs[2] = strdup(AttribCategoryDefault);
	attribs->attrib_names[3]     = strdup(AttribLabelType);
	attribs->attrib_labels[3]    = strdup(AttribLabelTypeLabel);
	attribs->attrib_sh_labels[3] = strdup(AttribLabelTypeLabel);
	attribs->attrib_back_defs[3] = strdup(FpaCblank);
	attribs->attrib_names[4]     = strdup(AttribLatitude);
	attribs->attrib_labels[4]    = strdup(AttribLatitudeLabel);
	attribs->attrib_sh_labels[4] = strdup(AttribLatitudeLabel);
	attribs->attrib_back_defs[4] = strdup(FpaCblank);
	attribs->attrib_names[5]     = strdup(AttribLongitude);
	attribs->attrib_labels[5]    = strdup(AttribLongitudeLabel);
	attribs->attrib_sh_labels[5] = strdup(AttribLongitudeLabel);
	attribs->attrib_back_defs[5] = strdup(FpaCblank);

	/* Add default attributes based on enumerated field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			attribs->nattribs         = 9;
			attribs->attrib_names     = GETMEM(attribs->attrib_names,
												STRING, attribs->nattribs);
			attribs->attrib_labels    = GETMEM(attribs->attrib_labels,
												STRING, attribs->nattribs);
			attribs->attrib_sh_labels = GETMEM(attribs->attrib_sh_labels,
												STRING, attribs->nattribs);
			attribs->attrib_back_defs = GETMEM(attribs->attrib_back_defs,
												STRING, attribs->nattribs);
			attribs->attrib_names[6]     = strdup(AttribEvalContour);
			attribs->attrib_labels[6]    = strdup(AttribEvalContourLabel);
			attribs->attrib_sh_labels[6] = strdup(AttribEvalContourLabel);
			attribs->attrib_back_defs[6] = strdup(FpaCblank);
			attribs->attrib_names[7]     = strdup(AttribEvalSpval);
			attribs->attrib_labels[7]    = strdup(AttribEvalSpvalLabel);
			attribs->attrib_sh_labels[7] = strdup(AttribEvalSpvalLabel);
			attribs->attrib_back_defs[7] = strdup(FpaCblank);
			attribs->attrib_names[8]     = strdup(AttribEvalWind);
			attribs->attrib_labels[8]    = strdup(AttribEvalWindLabel);
			attribs->attrib_sh_labels[8] = strdup(AttribEvalWindLabel);
			attribs->attrib_back_defs[8] = strdup(FpaCblank);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			attribs->nattribs         = 9;
			attribs->attrib_names     = GETMEM(attribs->attrib_names,
												STRING, attribs->nattribs);
			attribs->attrib_labels    = GETMEM(attribs->attrib_labels,
												STRING, attribs->nattribs);
			attribs->attrib_sh_labels = GETMEM(attribs->attrib_sh_labels,
												STRING, attribs->nattribs);
			attribs->attrib_back_defs = GETMEM(attribs->attrib_back_defs,
												STRING, attribs->nattribs);
			attribs->attrib_names[6]     = strdup(AttribEvalContour);
			attribs->attrib_labels[6]    = strdup(AttribEvalContourLabel);
			attribs->attrib_sh_labels[6] = strdup(AttribEvalContourLabel);
			attribs->attrib_back_defs[6] = strdup(FpaCblank);
			attribs->attrib_names[7]     = strdup(AttribEvalSpval);
			attribs->attrib_labels[7]    = strdup(AttribEvalSpvalLabel);
			attribs->attrib_sh_labels[7] = strdup(AttribEvalSpvalLabel);
			attribs->attrib_back_defs[7] = strdup(FpaCblank);
			attribs->attrib_names[8]     = strdup(AttribEvalWind);
			attribs->attrib_labels[8]    = strdup(AttribEvalWindLabel);
			attribs->attrib_sh_labels[8] = strdup(AttribEvalWindLabel);
			attribs->attrib_back_defs[8] = strdup(FpaCblank);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			break;

		/* Wind field type */
		case FpaC_WIND:
			attribs->nattribs         = 11;
			attribs->attrib_names     = GETMEM(attribs->attrib_names,
												STRING, attribs->nattribs);
			attribs->attrib_labels    = GETMEM(attribs->attrib_labels,
												STRING, attribs->nattribs);
			attribs->attrib_sh_labels = GETMEM(attribs->attrib_sh_labels,
												STRING, attribs->nattribs);
			attribs->attrib_back_defs = GETMEM(attribs->attrib_back_defs,
												STRING, attribs->nattribs);
			attribs->attrib_names[6]     = strdup(AttribWindModel);
			attribs->attrib_labels[6]    = strdup(AttribWindModelLabel);
			attribs->attrib_sh_labels[6] = strdup(AttribWindModelLabel);
			attribs->attrib_back_defs[6] = strdup(FpaCblank);
			attribs->attrib_names[7]     = strdup(AttribWindDirection);
			attribs->attrib_labels[7]    = strdup(AttribWindDirectionLabel);
			attribs->attrib_sh_labels[7] = strdup(AttribWindDirectionLabel);
			attribs->attrib_back_defs[7] = strdup(FpaCblank);
			attribs->attrib_names[8]     = strdup(AttribWindSpeed);
			attribs->attrib_labels[8]    = strdup(AttribWindSpeedLabel);
			attribs->attrib_sh_labels[8] = strdup(AttribWindSpeedLabel);
			attribs->attrib_back_defs[8] = strdup(FpaCblank);
			attribs->attrib_names[9]     = strdup(AttribWindGust);
			attribs->attrib_labels[9]    = strdup(AttribWindGustLabel);
			attribs->attrib_sh_labels[9] = strdup(AttribWindGustLabel);
			attribs->attrib_back_defs[9] = strdup(FpaCblank);
			attribs->attrib_names[10]     = strdup(AttribEvalWind);
			attribs->attrib_labels[10]    = strdup(AttribEvalWindLabel);
			attribs->attrib_sh_labels[10] = strdup(AttribEvalWindLabel);
			attribs->attrib_back_defs[10] = strdup(FpaCblank);
			break;

		/* Line field type */
		case FpaC_LINE:
			attribs->nattribs         = 10;
			attribs->attrib_names     = GETMEM(attribs->attrib_names,
												STRING, attribs->nattribs);
			attribs->attrib_labels    = GETMEM(attribs->attrib_labels,
												STRING, attribs->nattribs);
			attribs->attrib_sh_labels = GETMEM(attribs->attrib_sh_labels,
												STRING, attribs->nattribs);
			attribs->attrib_back_defs = GETMEM(attribs->attrib_back_defs,
												STRING, attribs->nattribs);
			attribs->attrib_names[6]     = strdup(AttribLineType);
			attribs->attrib_labels[6]    = strdup(AttribLineTypeLabel);
			attribs->attrib_sh_labels[6] = strdup(AttribLineTypeLabel);
			attribs->attrib_back_defs[6] = strdup(FpaCblank);
			/* >>>
			attribs->attrib_back_defs[6] = strdup(AttribLineTypeDefault);
			<<< */
			attribs->attrib_names[7]     = strdup(AttribProximity);
			attribs->attrib_labels[7]    = strdup(AttribProximityLabel);
			attribs->attrib_sh_labels[7] = strdup(AttribProximityLabel);
			attribs->attrib_back_defs[7] = strdup(FpaCblank);
			attribs->attrib_names[8]     = strdup(AttribLineDirection);
			attribs->attrib_labels[8]    = strdup(AttribLineDirectionLabel);
			attribs->attrib_sh_labels[8] = strdup(AttribLineDirectionLabel);
			attribs->attrib_back_defs[8] = strdup(FpaCblank);
			attribs->attrib_names[9]     = strdup(AttribLineLength);
			attribs->attrib_labels[9]    = strdup(AttribLineLengthLabel);
			attribs->attrib_sh_labels[9] = strdup(AttribLineLengthLabel);
			attribs->attrib_back_defs[9] = strdup(FpaCblank);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			attribs->nattribs         = 8;
			attribs->attrib_names     = GETMEM(attribs->attrib_names,
												STRING, attribs->nattribs);
			attribs->attrib_labels    = GETMEM(attribs->attrib_labels,
												STRING, attribs->nattribs);
			attribs->attrib_sh_labels = GETMEM(attribs->attrib_sh_labels,
												STRING, attribs->nattribs);
			attribs->attrib_back_defs = GETMEM(attribs->attrib_back_defs,
												STRING, attribs->nattribs);
			attribs->attrib_names[6]     = strdup(AttribScatteredType);
			attribs->attrib_labels[6]    = strdup(AttribScatteredTypeLabel);
			attribs->attrib_sh_labels[6] = strdup(AttribScatteredTypeLabel);
			attribs->attrib_back_defs[6] = strdup(AttribScatteredTypeDefault);
			attribs->attrib_names[7]     = strdup(AttribProximity);
			attribs->attrib_labels[7]    = strdup(AttribProximityLabel);
			attribs->attrib_sh_labels[7] = strdup(AttribProximityLabel);
			attribs->attrib_back_defs[7] = strdup(FpaCblank);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			attribs->nattribs         = 18;
			attribs->attrib_names     = GETMEM(attribs->attrib_names,
												STRING, attribs->nattribs);
			attribs->attrib_labels    = GETMEM(attribs->attrib_labels,
												STRING, attribs->nattribs);
			attribs->attrib_sh_labels = GETMEM(attribs->attrib_sh_labels,
												STRING, attribs->nattribs);
			attribs->attrib_back_defs = GETMEM(attribs->attrib_back_defs,
												STRING, attribs->nattribs);
			attribs->attrib_names[6]     = strdup(AttribLchainReference);
			attribs->attrib_labels[6]    = strdup(AttribLchainReferenceLabel);
			attribs->attrib_sh_labels[6] = strdup(AttribLchainReferenceLabel);
			attribs->attrib_back_defs[6] = strdup(FpaCblank);
			attribs->attrib_names[7]     = strdup(AttribLchainStartTime);
			attribs->attrib_labels[7]    = strdup(AttribLchainStartTimeLabel);
			attribs->attrib_sh_labels[7] = strdup(AttribLchainStartTimeLabel);
			attribs->attrib_back_defs[7] = strdup(FpaCblank);
			attribs->attrib_names[8]     = strdup(AttribLchainEndTime);
			attribs->attrib_labels[8]    = strdup(AttribLchainEndTimeLabel);
			attribs->attrib_sh_labels[8] = strdup(AttribLchainEndTimeLabel);
			attribs->attrib_back_defs[8] = strdup(FpaCblank);
			attribs->attrib_names[9]     = strdup(AttribLchainStartTstamp);
			attribs->attrib_labels[9]    = strdup(AttribLchainStartTstampLabel);
			attribs->attrib_sh_labels[9] = strdup(AttribLchainStartTstampLabel);
			attribs->attrib_back_defs[9] = strdup(FpaCblank);
			attribs->attrib_names[10]     = strdup(AttribLchainEndTstamp);
			attribs->attrib_labels[10]    = strdup(AttribLchainEndTstampLabel);
			attribs->attrib_sh_labels[10] = strdup(AttribLchainEndTstampLabel);
			attribs->attrib_back_defs[10] = strdup(FpaCblank);
			attribs->attrib_names[11]     = strdup(AttribLnodeType);
			attribs->attrib_labels[11]    = strdup(AttribLnodeTypeLabel);
			attribs->attrib_sh_labels[11] = strdup(AttribLnodeTypeLabel);
			attribs->attrib_back_defs[11] = strdup(FpaCblank);
			attribs->attrib_names[12]     = strdup(AttribLnodeTime);
			attribs->attrib_labels[12]    = strdup(AttribLnodeTimeLabel);
			attribs->attrib_sh_labels[12] = strdup(AttribLnodeTimeLabel);
			attribs->attrib_back_defs[12] = strdup(FpaCblank);
			attribs->attrib_names[13]     = strdup(AttribLnodeTstamp);
			attribs->attrib_labels[13]    = strdup(AttribLnodeTstampLabel);
			attribs->attrib_sh_labels[13] = strdup(AttribLnodeTstampLabel);
			attribs->attrib_back_defs[13] = strdup(FpaCblank);
			attribs->attrib_names[14]     = strdup(AttribLnodeDirection);
			attribs->attrib_labels[14]    = strdup(AttribLnodeDirectionLabel);
			attribs->attrib_sh_labels[14] = strdup(AttribLnodeDirectionLabel);
			attribs->attrib_back_defs[14] = strdup(FpaCblank);
			attribs->attrib_names[15]     = strdup(AttribLnodeSpeed);
			attribs->attrib_labels[15]    = strdup(AttribLnodeSpeedLabel);
			attribs->attrib_sh_labels[15] = strdup(AttribLnodeSpeedLabel);
			attribs->attrib_back_defs[15] = strdup(FpaCblank);
			attribs->attrib_names[16]     = strdup(AttribLnodeVector);
			attribs->attrib_labels[16]    = strdup(AttribLnodeVectorLabel);
			attribs->attrib_sh_labels[16] = strdup(AttribLnodeVectorLabel);
			attribs->attrib_back_defs[16] = strdup(FpaCblank);
			attribs->attrib_names[17]     = strdup(AttribProximity);
			attribs->attrib_labels[17]    = strdup(AttribProximityLabel);
			attribs->attrib_sh_labels[17] = strdup(AttribProximityLabel);
			attribs->attrib_back_defs[17] = strdup(FpaCblank);
			break;

		/* Special field type */
		case FpaC_SPECIAL:
			break;

		/* Unknown field type */
		default:
			break;
		}

	/* Return pointer to ElementAttrib structure */
	return attribs;
	}

/**********************************************************************/

static	FpaConfigElementEditorStruct	*init_element_editor

	(
	int			type		/* enumerated field type */
	)


	{
	FpaConfigElementEditorStruct		*editor;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Initialize ElementEditor structure */
	editor               = INITMEM(FpaConfigElementEditorStruct, 1);
	editor->check_editor = FALSE;

	/* Initialize editor structure based on enumerated field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:

			continuous = INITMEM(FpaConfigContinuousEditorStruct, 1);
			continuous->hilo             = FALSE;
			continuous->poke             = 0.0;
			continuous->units            = NullPtr(FpaConfigUnitStruct *);
			continuous->entry_file       = NullString;
			continuous->modify_file      = NullString;
			continuous->memory_file      = NullString;
			continuous->back_entry_file  = NullString;
			continuous->back_memory_file = NullString;
			continuous->nrules           = 0;
			continuous->entry_rules      = NullStringList;
			continuous->entry_funcs      = NullEruleList;
			continuous->py_nrules        = 0;
			continuous->py_entry_rules   = NullStringList;
			continuous->nmerge           = 0;
			continuous->merge_elems      = NullPtr(FpaConfigElementStruct **);
			continuous->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			editor->type.continuous = continuous;
			break;

		/* Vector field type */
		case FpaC_VECTOR:

			vector = INITMEM(FpaConfigVectorEditorStruct, 1);
			vector->hilo             = FALSE;
			vector->mag_poke         = 0.0;
			vector->mag_units        = NullPtr(FpaConfigUnitStruct *);
			vector->dir_poke         = 0.0;
			vector->dir_units        = NullPtr(FpaConfigUnitStruct *);
			vector->entry_file       = NullString;
			vector->modify_file      = NullString;
			vector->memory_file      = NullString;
			vector->back_entry_file  = NullString;
			vector->back_memory_file = NullString;
			vector->nrules           = 0;
			vector->entry_rules      = NullStringList;
			vector->entry_funcs      = NullEruleList;
			vector->py_nrules        = 0;
			vector->py_entry_rules   = NullStringList;
			vector->nmerge           = 0;
			vector->merge_elems      = NullPtr(FpaConfigElementStruct **);
			vector->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			editor->type.vector = vector;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:

			/* Initialize DiscreteEditor structure */
			discrete = INITMEM(FpaConfigDiscreteEditorStruct, 1);
			discrete->overlaying       = FALSE;
			discrete->display_order    = TRUE;
			discrete->entry_file       = NullString;
			discrete->modify_file      = NullString;
			discrete->memory_file      = NullString;
			discrete->back_entry_file  = NullString;
			discrete->back_memory_file = NullString;
			discrete->nrules           = 0;
			discrete->entry_rules      = NullStringList;
			discrete->entry_funcs      = NullEruleList;
			discrete->py_nrules        = 0;
			discrete->py_entry_rules   = NullStringList;
			discrete->nmerge           = 0;
			discrete->merge_elems      = NullPtr(FpaConfigElementStruct **);
			discrete->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			editor->type.discrete = discrete;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = INITMEM(FpaConfigWindEditorStruct, 1);
			wind->display_order    = TRUE;
			wind->entry_file       = NullString;
			wind->modify_file      = NullString;
			wind->memory_file      = NullString;
			wind->back_entry_file  = NullString;
			wind->back_memory_file = NullString;
			wind->nrules           = 0;
			wind->entry_rules      = NullStringList;
			wind->entry_funcs      = NullEruleList;
			wind->py_nrules        = 0;
			wind->py_entry_rules   = NullStringList;
			wind->nmerge           = 0;
			wind->merge_elems      = NullPtr(FpaConfigElementStruct **);
			wind->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			editor->type.wind = wind;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = INITMEM(FpaConfigLineEditorStruct, 1);
			line->entry_file       = NullString;
			line->modify_file      = NullString;
			line->memory_file      = NullString;
			line->back_entry_file  = NullString;
			line->back_memory_file = NullString;
			line->nrules           = 0;
			line->entry_rules      = NullStringList;
			line->entry_funcs      = NullEruleList;
			line->py_nrules        = 0;
			line->py_entry_rules   = NullStringList;
			line->nmerge           = 0;
			line->merge_elems      = NullPtr(FpaConfigElementStruct **);
			line->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			editor->type.line = line;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = INITMEM(FpaConfigScatteredEditorStruct, 1);
			scattered->entry_file       = NullString;
			scattered->modify_file      = NullString;
			scattered->memory_file      = NullString;
			scattered->back_entry_file  = NullString;
			scattered->back_memory_file = NullString;
			scattered->nrules           = 0;
			scattered->entry_rules      = NullStringList;
			scattered->entry_funcs      = NullEruleList;
			scattered->py_nrules        = 0;
			scattered->py_entry_rules   = NullStringList;
			scattered->nmerge           = 0;
			scattered->merge_elems      = NullPtr(FpaConfigElementStruct **);
			scattered->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			editor->type.scattered = scattered;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:

			/* Initialize LchainEditor structure */
			lchain = INITMEM(FpaConfigLchainEditorStruct, 1);
			lchain->entry_file       = NullString;
			lchain->modify_file      = NullString;
			lchain->memory_file      = NullString;
			lchain->back_entry_file  = NullString;
			lchain->back_memory_file = NullString;
			lchain->nrules           = 0;
			lchain->entry_rules      = NullStringList;
			lchain->entry_funcs      = NullEruleList;
			lchain->py_nrules        = 0;
			lchain->py_entry_rules   = NullStringList;
			lchain->node_entry_file  = NullString;
			lchain->node_modify_file = NullString;
			lchain->nnode_rules      = 0;
			lchain->node_entry_rules = NullStringList;
			lchain->node_entry_funcs = NullEruleList;
			lchain->py_nnode_rules   = 0;
			lchain->py_node_entry_rules = NullStringList;
			lchain->nmerge           = 0;
			lchain->merge_elems      = NullPtr(FpaConfigElementStruct **);
			lchain->merge_levels     = NullPtr(FpaConfigLevelStruct **);
			lchain->nlink            = 0;
			lchain->link_elems       = NullPtr(FpaConfigElementStruct **);
			lchain->link_levels      = NullPtr(FpaConfigLevelStruct **);
			lchain->minterp          = -1;
			editor->type.lchain = lchain;
			break;

		/* Field types that cannot be edited! */
		default:
			editor->type.continuous =
					NullPtr(FpaConfigContinuousEditorStruct *);
			break;
		}

	/* Return pointer to ElementEditor structure */
	return editor;
	}

/**********************************************************************/

static	FpaConfigElementLabellingStruct	*init_element_labelling

	(
	int			type		/* enumerated field type */
	)

	{
	FpaConfigElementLabellingStruct		*labelling;

	/* Initialize ElementLabelling structure */
	labelling = INITMEM(FpaConfigElementLabellingStruct, 1);

	/* Initialize labelling types */
	(void) init_element_labelling_types(type, labelling);

	/* Return pointer to ElementLabelling structure */
	return labelling;
	}

/**********************************************************************/

static	void					init_element_labelling_types

	(
	int									type,		/* enumerated field type */
	FpaConfigElementLabellingStruct		*labelling	/* pointer to
														ElementLabelling structure */
	)

	{

	/* Return if no structure passed */
	if ( IsNull(labelling) ) return;

	/* Initialize labelling types based on enumerated field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			labelling->check_labelling   = FALSE;
			labelling->ntypes            = 1;
			labelling->type_names        = INITMEM(STRING, 1);
			labelling->type_labels       = INITMEM(STRING, 1);
			labelling->type_sh_labels    = INITMEM(STRING, 1);
			labelling->type_classes      = INITMEM(STRING, 1);
			labelling->type_entry_files  = INITMEM(STRING, 1);
			labelling->type_modify_files = INITMEM(STRING, 1);
			labelling->type_attach_opts  = INITMEM(FpaCattachOption, 1);
			labelling->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct, 1);
			labelling->type_rules        = INITMEM(FpaConfigEntryRuleStruct, 1);
			labelling->type_names[0]        = strdup(FpaLabellingContinuous);
			labelling->type_labels[0]       = strdup(FpaLabellingContinuousLabel);
			labelling->type_sh_labels[0]    = strdup(FpaLabellingContinuousShort);
			labelling->type_classes[0]      = NullString;
			labelling->type_entry_files[0]  = NullString;
			labelling->type_modify_files[0] = NullString;
			labelling->type_attach_opts[0]  = FpaC_ATTACH_AUTO;
			labelling->type_attribs[0].nattrib_defs      = 0;
			labelling->type_attribs[0].attrib_def_names  = NullStringList;
			labelling->type_attribs[0].attrib_def_values = NullStringList;
			labelling->type_rules[0].nrules              = 0;
			labelling->type_rules[0].entry_rules         = NullStringList;
			labelling->type_rules[0].entry_funcs         = NullEruleList;
			labelling->type_rules[0].py_nrules           = 0;
			labelling->type_rules[0].py_entry_rules      = NullStringList;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			labelling->check_labelling   = FALSE;
			labelling->ntypes            = 1;
			labelling->type_names        = INITMEM(STRING, 1);
			labelling->type_labels       = INITMEM(STRING, 1);
			labelling->type_sh_labels    = INITMEM(STRING, 1);
			labelling->type_classes      = INITMEM(STRING, 1);
			labelling->type_entry_files  = INITMEM(STRING, 1);
			labelling->type_modify_files = INITMEM(STRING, 1);
			labelling->type_attach_opts  = INITMEM(FpaCattachOption, 1);
			labelling->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct, 1);
			labelling->type_rules        = INITMEM(FpaConfigEntryRuleStruct, 1);
			labelling->type_names[0]        = strdup(FpaLabellingVector);
			labelling->type_labels[0]       = strdup(FpaLabellingVectorLabel);
			labelling->type_sh_labels[0]    = strdup(FpaLabellingVectorShort);
			labelling->type_classes[0]      = NullString;
			labelling->type_entry_files[0]  = NullString;
			labelling->type_modify_files[0] = NullString;
			labelling->type_attach_opts[0]  = FpaC_ATTACH_AUTO;
			labelling->type_attribs[0].nattrib_defs      = 0;
			labelling->type_attribs[0].attrib_def_names  = NullStringList;
			labelling->type_attribs[0].attrib_def_values = NullStringList;
			labelling->type_rules[0].nrules              = 0;
			labelling->type_rules[0].entry_rules         = NullStringList;
			labelling->type_rules[0].entry_funcs         = NullEruleList;
			labelling->type_rules[0].py_nrules           = 0;
			labelling->type_rules[0].py_entry_rules      = NullStringList;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			labelling->check_labelling   = FALSE;
			labelling->ntypes            = 1;
			labelling->type_names        = INITMEM(STRING, 1);
			labelling->type_labels       = INITMEM(STRING, 1);
			labelling->type_sh_labels    = INITMEM(STRING, 1);
			labelling->type_classes      = INITMEM(STRING, 1);
			labelling->type_entry_files  = INITMEM(STRING, 1);
			labelling->type_modify_files = INITMEM(STRING, 1);
			labelling->type_attach_opts  = INITMEM(FpaCattachOption, 1);
			labelling->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct, 1);
			labelling->type_rules        = INITMEM(FpaConfigEntryRuleStruct, 1);
			labelling->type_names[0]        = strdup(FpaLabellingDiscrete);
			labelling->type_labels[0]       = strdup(FpaLabellingDiscreteLabel);
			labelling->type_sh_labels[0]    = strdup(FpaLabellingDiscreteShort);
			labelling->type_classes[0]      = NullString;
			labelling->type_entry_files[0]  = NullString;
			labelling->type_modify_files[0] = NullString;
			labelling->type_attach_opts[0]  = FpaC_ATTACH_AUTO;
			labelling->type_attribs[0].nattrib_defs      = 0;
			labelling->type_attribs[0].attrib_def_names  = NullStringList;
			labelling->type_attribs[0].attrib_def_values = NullStringList;
			labelling->type_rules[0].nrules              = 0;
			labelling->type_rules[0].entry_rules         = NullStringList;
			labelling->type_rules[0].entry_funcs         = NullEruleList;
			labelling->type_rules[0].py_nrules           = 0;
			labelling->type_rules[0].py_entry_rules      = NullStringList;
			break;

		/* Wind field type */
		case FpaC_WIND:
			labelling->check_labelling   = FALSE;
			labelling->ntypes            = 2;
			labelling->type_names        = INITMEM(STRING, 2);
			labelling->type_labels       = INITMEM(STRING, 2);
			labelling->type_sh_labels    = INITMEM(STRING, 2);
			labelling->type_classes      = INITMEM(STRING, 2);
			labelling->type_entry_files  = INITMEM(STRING, 2);
			labelling->type_modify_files = INITMEM(STRING, 2);
			labelling->type_attach_opts  = INITMEM(FpaCattachOption, 2);
			labelling->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct, 2);
			labelling->type_rules        = INITMEM(FpaConfigEntryRuleStruct, 2);
			labelling->type_names[0]        = strdup(FpaLabellingWindBarb);
			labelling->type_labels[0]       = strdup(FpaLabellingWindBarbLabel);
			labelling->type_sh_labels[0]    = strdup(FpaLabellingWindBarbShort);
			labelling->type_classes[0]      = NullString;
			labelling->type_entry_files[0]  = NullString;
			labelling->type_modify_files[0] = NullString;
			labelling->type_attach_opts[0]  = FpaC_ATTACH_AUTO;
			labelling->type_attribs[0].nattrib_defs      = 0;
			labelling->type_attribs[0].attrib_def_names  = NullStringList;
			labelling->type_attribs[0].attrib_def_values = NullStringList;
			labelling->type_rules[0].nrules              = 0;
			labelling->type_rules[0].entry_rules         = NullStringList;
			labelling->type_rules[0].entry_funcs         = NullEruleList;
			labelling->type_rules[0].py_nrules           = 0;
			labelling->type_rules[0].py_entry_rules      = NullStringList;
			labelling->type_names[1]        = strdup(FpaLabellingWindArea);
			labelling->type_labels[1]       = strdup(FpaLabellingWindAreaLabel);
			labelling->type_sh_labels[1]    = strdup(FpaLabellingWindAreaShort);
			labelling->type_classes[1]      = NullString;
			labelling->type_entry_files[1]  = NullString;
			labelling->type_modify_files[1] = NullString;
			labelling->type_attach_opts[1]  = FpaC_ATTACH_AUTO;
			labelling->type_attribs[1].nattrib_defs      = 0;
			labelling->type_attribs[1].attrib_def_names  = NullStringList;
			labelling->type_attribs[1].attrib_def_values = NullStringList;
			labelling->type_rules[1].nrules              = 0;
			labelling->type_rules[1].entry_rules         = NullStringList;
			labelling->type_rules[1].entry_funcs         = NullEruleList;
			labelling->type_rules[1].py_nrules           = 0;
			labelling->type_rules[1].py_entry_rules      = NullStringList;
			break;

		/* Line field type */
		case FpaC_LINE:
			labelling->check_labelling   = FALSE;
			labelling->ntypes            = 1;
			labelling->type_names        = INITMEM(STRING, 1);
			labelling->type_labels       = INITMEM(STRING, 1);
			labelling->type_sh_labels    = INITMEM(STRING, 1);
			labelling->type_classes      = INITMEM(STRING, 1);
			labelling->type_entry_files  = INITMEM(STRING, 1);
			labelling->type_modify_files = INITMEM(STRING, 1);
			labelling->type_attach_opts  = INITMEM(FpaCattachOption, 1);
			labelling->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct, 1);
			labelling->type_rules        = INITMEM(FpaConfigEntryRuleStruct, 1);
			labelling->type_names[0]        = strdup(FpaLabellingLine);
			labelling->type_labels[0]       = strdup(FpaLabellingLineLabel);
			labelling->type_sh_labels[0]    = strdup(FpaLabellingLineShort);
			labelling->type_classes[0]      = NullString;
			labelling->type_entry_files[0]  = NullString;
			labelling->type_modify_files[0] = NullString;
			labelling->type_attach_opts[0]  = FpaC_ATTACH_AUTO;
			labelling->type_attribs[0].nattrib_defs      = 0;
			labelling->type_attribs[0].attrib_def_names  = NullStringList;
			labelling->type_attribs[0].attrib_def_values = NullStringList;
			labelling->type_rules[0].nrules              = 0;
			labelling->type_rules[0].entry_rules         = NullStringList;
			labelling->type_rules[0].entry_funcs         = NullEruleList;
			labelling->type_rules[0].py_nrules           = 0;
			labelling->type_rules[0].py_entry_rules      = NullStringList;
			break;

		/* Field types that cannot be labelled! */
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
		default:
			labelling->check_labelling   = FALSE;
			labelling->ntypes            = 0;
			labelling->type_names        = NullStringList;
			labelling->type_labels       = NullStringList;
			labelling->type_sh_labels    = NullStringList;
			labelling->type_classes      = INITMEM(STRING, 1);
			labelling->type_entry_files  = NullStringList;
			labelling->type_modify_files = NullStringList;
			labelling->type_attach_opts  = NullPtr(FpaCattachOption *);
			labelling->type_attribs      = NullPtr(FpaConfigDefaultAttribStruct *);
			labelling->type_rules        = NullPtr(FpaConfigEntryRuleStruct *);
			break;
		}
	}

/**********************************************************************/

static	FpaConfigElementSamplingStruct	*init_element_sampling

	(
	int			type		/* enumerated field type */
	)

	{
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigContinuousSamplingStruct	*continuous;
	FpaConfigVectorSamplingStruct		*vector;
	FpaConfigDiscreteSamplingStruct		*discrete;
	FpaConfigWindSamplingStruct			*wind;
	FpaConfigLineSamplingStruct			*line;
	FpaConfigScatteredSamplingStruct	*scattered;
	FpaConfigLchainSamplingStruct		*lchain;
	FpaConfigSampleStruct				*xdef;
	int									nsamp;

	/* Initialize ElementSampling structure */
	sampling                 = INITMEM(FpaConfigElementSamplingStruct, 1);
	sampling->check_sampling = FALSE;

	/* Initialize Sampling structure based on enumerated field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = INITMEM(FpaConfigContinuousSamplingStruct, 1);
			continuous->nsample    = 0;
			continuous->samples    = NullPtr(FpaConfigSampleStruct **);
			continuous->nwindsamp  = 0;
			continuous->windsamps  = NullPtr(FpaConfigSampleStruct **);

			/* Add default sampling types */
			xdef = identify_sample(FpaCsamplesValues, FpaCsampleValue);
			if ( NotNull(xdef) )
				{
				nsamp = continuous->nsample++;
				continuous->samples = GETMEM(continuous->samples,
										FpaConfigSampleStruct *,
										continuous->nsample);
				continuous->samples[nsamp] = xdef;
				}

			sampling->type.continuous = continuous;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = INITMEM(FpaConfigVectorSamplingStruct, 1);
			vector->nsample    = 0;
			vector->samples    = NullPtr(FpaConfigSampleStruct **);
			vector->nwindsamp  = 0;
			vector->windsamps  = NullPtr(FpaConfigSampleStruct **);

			/* Add default sampling types */
			xdef = identify_sample(FpaCsamplesValues, FpaCsampleMagnitude);
			if ( NotNull(xdef) )
				{
				nsamp = vector->nsample++;
				vector->samples = GETMEM(vector->samples,
									FpaConfigSampleStruct *, vector->nsample);
				vector->samples[nsamp] = xdef;
				}
			xdef = identify_sample(FpaCsamplesValues, FpaCsampleDirection);
			if ( NotNull(xdef) )
				{
				nsamp = vector->nsample++;
				vector->samples = GETMEM(vector->samples,
									FpaConfigSampleStruct *, vector->nsample);
				vector->samples[nsamp] = xdef;
				}
			xdef = identify_sample(FpaCsamplesValues, FpaCsampleValue);
			if ( NotNull(xdef) )
				{
				nsamp = vector->nsample++;
				vector->samples = GETMEM(vector->samples,
									FpaConfigSampleStruct *, vector->nsample);
				vector->samples[nsamp] = xdef;
				}

			sampling->type.vector = vector;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = INITMEM(FpaConfigDiscreteSamplingStruct, 1);
			discrete->nsattribs     = 0;
			discrete->sattrib_names = NullStringPtr;
			sampling->type.discrete = discrete;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = INITMEM(FpaConfigWindSamplingStruct, 1);
			wind->nsample    = 0;
			wind->samples    = NullPtr(FpaConfigSampleStruct **);
			wind->nwcref     = 0;
			wind->wcrefs     = NullPtr(FpaConfigCrossRefStruct **);
			wind->windsample = NullPtr(FpaConfigSampleStruct *);

			/* Add default sampling types */
			xdef = identify_sample(FpaCsamplesValues, FpaCsampleValue);
			if ( NotNull(xdef) )
				{
				nsamp = wind->nsample++;
				wind->samples = GETMEM(wind->samples,
									FpaConfigSampleStruct *, wind->nsample);
				wind->samples[nsamp] = xdef;
				}
			xdef = identify_sample(FpaCsamplesValues, FpaCsampleLabel);
			if ( NotNull(xdef) )
				{
				nsamp = wind->nsample++;
				wind->samples = GETMEM(wind->samples,
									FpaConfigSampleStruct *, wind->nsample);
				wind->samples[nsamp] = xdef;
				}
			xdef = identify_sample(FpaCsamplesWinds, FpaCsampleAdjusted);
			if ( NotNull(xdef) ) wind->windsample = xdef;

			sampling->type.wind = wind;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = INITMEM(FpaConfigLineSamplingStruct, 1);
			line->nsattribs     = 0;
			line->sattrib_names = NullStringPtr;
			sampling->type.line = line;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = INITMEM(FpaConfigScatteredSamplingStruct, 1);
			scattered->nsattribs     = 0;
			scattered->sattrib_names = NullStringPtr;
			sampling->type.scattered = scattered;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = INITMEM(FpaConfigLchainSamplingStruct, 1);
			lchain->nsattribs     = 0;
			lchain->sattrib_names = NullStringPtr;
			sampling->type.lchain = lchain;
			break;

		/* Unknown field types */
		default:
			sampling->type.continuous = NullPtr(FpaConfigContinuousSamplingStruct *);
			break;
		}

	/* Return pointer to ElementSampling structure */
	return sampling;
	}

/**********************************************************************/

static	FpaConfigElementLinkingStruct	*init_element_linking

	(
	int			type		/* enumerated field type */
	)

	{
	FpaConfigElementLinkingStruct		*linking;

	/* Initialize ElementLinking structure */
	linking                = INITMEM(FpaConfigElementLinkingStruct, 1);
	linking->check_linking = FALSE;

	/* Initialize Linking structure based on enumerated field type */
	switch ( type )
		{

		/* For the moment ... all field types have same structure */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
			linking->minterp       = -1;
			linking->nlink	       =  0;
			linking->link_elems    = NullPtr(FpaConfigElementStruct **);
			linking->link_levels   = NullPtr(FpaConfigLevelStruct **);
			break;

		/* Unknown field types */
		default:
			FREEMEM(linking);
			break;
		}

	/* Return pointer to ElementLinking structure */
	return linking;
	}

/**********************************************************************/

static	FpaConfigElementEquationStruct	*init_element_equation

	(
	)

	{
	FpaConfigElementEquationStruct		*equation;

	/* Initialize ElementEquation structure */
	equation = INITMEM(FpaConfigElementEquationStruct, 1);
	equation->force   = FALSE;
	equation->eqtn    = NullString;
	equation->units   = NullPtr(FpaConfigUnitStruct *);

	/* Return pointer to ElementEquation structure */
	return equation;
	}

/**********************************************************************/

static	FpaConfigElementValCalcStruct	*init_element_valcalc

	(
	)

	{
	FpaConfigElementValCalcStruct		*valcalc;

	/* Initialize ElementValCalc structure */
	valcalc            = INITMEM(FpaConfigElementValCalcStruct, 1);
	valcalc->force     = FALSE;
	valcalc->vcalc     = NullPtr(FpaConfigCrossRefStruct *);
	valcalc->nsrc_type = 0;
	valcalc->src_types = NullEnum;

	/* Return pointer to ElementValCalc structure */
	return valcalc;
	}

/**********************************************************************/

static	FpaConfigElementComponentStruct	*init_element_components

	(
	)

	{
	FpaConfigElementComponentStruct		*components;

	/* Initialize ElementComponent structure                       */
	/* Note that components are initialized as x and y components! */
	components             = INITMEM(FpaConfigElementComponentStruct, 1);
	components->cinfo      = &(NoCompInfo);
	components->ncomp      = 0;
	components->comp_edefs = NullPtr(FpaConfigElementStruct **);
	components->comp_types = NullPtr(COMPONENT *);

	/* Return pointer to ElementComponent structure */
	return components;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ a l i a s e s                              *
*   a d d _ e l e m e n t _ f i l e _ i d e n t                        *
*   a d d _ e l e m e n t _ f i l e i d                                *
*   a d d _ e l e m e n t _ i d e n t i f i e r                        *
*                                                                      *
*   Add aliases, file ids, or identifiers to element identifier list.  *
*   Note that element name comparisons are case insensitive!           *
*                                                                      *
***********************************************************************/

static	void					add_element_aliases

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigElementStruct	*edef		/* pointer to Element structure */
	)

	{
	STRING					arg;
	FpaConfigElementStruct	*edefx;

	/* Add all acceptable aliases to ident list ... if not already there! */
	while ( NotNull( arg = string_arg(cline) ) )
		{
		if ( OKARG(arg) )
			{

			/* Add to ident list if alias not found */
			edefx = find_element(arg);
			if ( IsNull(edefx) )
				{
				(void) add_element_identifier(arg, edef);
				}

			/* Error message if alias belongs to another element! */
			else if ( edefx != edef )
				{
				(void) config_file_message(FpaCblockElements,
						edef->name, edefx->name, arg, FpaCmsgAlias);
				}
			}
		}
	}

/**********************************************************************/

static	void					add_element_file_ident

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigElementStruct	*edef		/* pointer to Element structure */
	)

	{
	STRING						arg;
	FpaConfigElementStruct		*edefx;

	/* Ignore special file ident "None" */
	arg = string_arg(cline);
	if ( same_ic(arg, FpaCnone) ) return;

	/* Check for acceptable file idents                        */
	/*  ... and add them to ident list (if not already there!) */
	if ( OKARG(arg) && file_ident_format(arg, ELEM_IDENT_LEN) )
		{

		/* Add to ident list if file ident not found */
		edefx = find_element(arg);
		if ( IsNull(edefx) )
			{
			(void) add_element_identifier(arg, edef);
			}

		/* Error message if file ident is an alias of another element! */
		else if ( edefx != edef )
			{
			(void) config_file_message(FpaCblockElements,
					edef->name, edefx->name, arg, FpaCmsgAlias);
			}

		/* Set file ident */
		edef->elem_io->fident = strdup(arg);
		}

	/* Error message for unacceptable file idents */
	else
		{
		(void) config_file_message(FpaCblockElements,
				edef->name, FpaCblank, FpaCfileIdent, FpaCmsgParameter);
		edef->valid = FALSE;
		}
	}

/**********************************************************************/

static	void					add_element_fileid

	(
	STRING					cline,		/* line in configuration file */
	FpaConfigElementStruct	*edef		/* pointer to Element structure */
	)

	{
	STRING						arg;
	FpaConfigElementStruct		*edefx;

	/* Ignore special file id "None" */
	arg = string_arg(cline);
	if ( same_ic(arg, FpaCnone) ) return;

	/* Check for acceptable file ids                           */
	/*  ... which must be exactly ELEM_ID_LEN characters long! */
	/*  ... and add them to ident list (if not already there!) */
	if ( OKARG(arg) && (int) strlen(arg) == ELEM_ID_LEN )
		{

		/* Add to ident list if file id not found */
		edefx = find_element(arg);
		if ( IsNull(edefx) )
			{
			(void) add_element_identifier(arg, edef);
			}

		/* Error message if file id is an alias of another element! */
		else if ( edefx != edef )
			{
			(void) config_file_message(FpaCblockElements,
					edef->name, edefx->name, arg, FpaCmsgAlias);
			}

		/* Set file id */
		edef->elem_io->fid = strdup(arg);
		}

	/* Error message for unacceptable file ids */
	else
		{
		(void) config_file_message(FpaCblockElements,
				edef->name, FpaCblank, FpaCfileId, FpaCmsgParameter);
		edef->valid = FALSE;
		}
	}

/**********************************************************************/

static	void					add_element_identifier

	(
	STRING					ident,		/* element identifier name */
	FpaConfigElementStruct	*edef		/* pointer to Element structure */
	)

	{

	/* Add identifier to list */
	NumElementIdent++;
	ElementIdents = GETMEM(ElementIdents, FPAC_IDENTS, NumElementIdent);
	ElementIdents[NumElementIdent-1].ident = strdup(ident);
	ElementIdents[NumElementIdent-1].pdef  = (POINTER) edef;

	/* Sort the list */
	(void) qsort((POINTER) ElementIdents, (size_t) NumElementIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ l i n e _ t y p e s                        *
*                                                                      *
*   Initialize line type information for Elements block of             *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	int								add_element_line_type

	(
	STRING							name,	/* line type name */
	FpaConfigElementLineTypeStruct	*ltypes	/* pointer to  LineType structure */
	)

	{
	int		ntyp;

	/* Error return if no structure passed */
	if ( IsNull(ltypes) ) return -1;

	/* Return location if name is already in the list */
	for ( ntyp=0; ntyp<ltypes->ntypes; ntyp++ )
		if ( same_ic(name, ltypes->type_names[ntyp]) ) return ntyp;

	/* Add another line type name */
	ntyp = ltypes->ntypes++;
	ltypes->type_names     = GETMEM(ltypes->type_names,     STRING,
																ltypes->ntypes);
	ltypes->type_labels    = GETMEM(ltypes->type_labels,    STRING,
																ltypes->ntypes);
	ltypes->type_sh_labels = GETMEM(ltypes->type_sh_labels, STRING,
																ltypes->ntypes);
	ltypes->patterns       = GETMEM(ltypes->patterns,       STRING,
																ltypes->ntypes);

	/* Initialize line type name, labels and pattern file */
	ltypes->type_names[ntyp]     = strdup(name);
	ltypes->type_labels[ntyp]    = strdup(name);
	ltypes->type_sh_labels[ntyp] = strdup(name);
	ltypes->patterns[ntyp]       = NullString;

	/* Return location if all went OK */
	return ntyp;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ s c a t t e r e d _ t y p e s              *
*                                                                      *
*   Initialize scattered type information for Elements block of        *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	int								add_element_scattered_type

	(
	STRING								name,	/* scattered type name */
	FpaConfigElementScatteredTypeStruct	*stypes	/* pointer to
													ScatteredType structure */
	)

	{
	int		ntyp;

	/* Error return if no structure passed */
	if ( IsNull(stypes) ) return -1;

	/* Return location if name is already in the list */
	for ( ntyp=0; ntyp<stypes->ntypes; ntyp++ )
		if ( same_ic(name, stypes->type_names[ntyp]) ) return ntyp;

	/* Add another scattered type name */
	ntyp = stypes->ntypes++;
	stypes->type_names        = GETMEM(stypes->type_names,       STRING,
																stypes->ntypes);
	stypes->type_labels       = GETMEM(stypes->type_labels,      STRING,
																stypes->ntypes);
	stypes->type_sh_labels    = GETMEM(stypes->type_sh_labels,   STRING,
																stypes->ntypes);
	stypes->type_classes      = GETMEM(stypes->type_classes,     STRING,
																stypes->ntypes);
	stypes->type_entry_files  = GETMEM(stypes->type_entry_files, STRING,
																stypes->ntypes);
	stypes->type_modify_files = GETMEM(stypes->type_modify_files, STRING,
																stypes->ntypes);
	stypes->type_attach_opts  = GETMEM(stypes->type_attach_opts,
											FpaCattachOption,   stypes->ntypes);
	stypes->type_attribs      = GETMEM(stypes->type_attribs,
											FpaConfigDefaultAttribStruct,
																stypes->ntypes);
	stypes->type_rules        = GETMEM(stypes->type_rules,
											FpaConfigEntryRuleStruct,
																stypes->ntypes);

	/* Initialize scattered type name, labels, attach options, */
	/*  attributes, and entry rules                            */
	stypes->type_names[ntyp]        = strdup(name);
	stypes->type_labels[ntyp]       = strdup(name);
	stypes->type_sh_labels[ntyp]    = strdup(name);
	stypes->type_classes[ntyp]      = strdup(FpaCdefaultScatteredTypesClass);
	stypes->type_entry_files[ntyp]  = NullString;
	stypes->type_modify_files[ntyp] = NullString;
	stypes->type_attach_opts[ntyp]  = FpaC_NO_ATTACH;
	stypes->type_attribs[ntyp].nattrib_defs      = 0;
	stypes->type_attribs[ntyp].attrib_def_names  = NullStringList;
	stypes->type_attribs[ntyp].attrib_def_values = NullStringList;
	stypes->type_rules[ntyp].nrules              = 0;
	stypes->type_rules[ntyp].entry_rules         = NullStringList;
	stypes->type_rules[ntyp].entry_funcs         = NullEruleList;
	stypes->type_rules[ntyp].py_nrules           = 0;
	stypes->type_rules[ntyp].py_entry_rules      = NullStringList;

	/* Return location if all went OK */
	return ntyp;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ t y p e _ a t t r i b                      *
*                                                                      *
*   Set default value for attribute in section of Elements block of    *
*   configuration files.                                               *
*                                                                      *
*   a d d _ e l e m e n t _ t y p e _ r u l e s                        *
*                                                                      *
*   Set list of entry rules in section of Elements block of            *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	LOGICAL							add_element_type_attrib

	(
	STRING							name,		/* attribute name */
	STRING							cline,		/* remainder of line in
													configuration file  */
	FpaConfigDefaultAttribStruct	*attrib		/* pointer to
													DefaultAttrib structure */
	)

	{
	int		natt;

	/* Return FALSE if no structure passed */
	if ( IsNull(attrib) ) return FALSE;

	/* Check if the attribute name is already in the list */
	for ( natt=0; natt<attrib->nattrib_defs; natt++ )
		if ( same_ic(name, attrib->attrib_def_names[natt]) ) break;

	/* Add another default attribute */
	if ( natt >= attrib->nattrib_defs )
		{
		natt = attrib->nattrib_defs++;
		attrib->attrib_def_names  = GETMEM(attrib->attrib_def_names,  STRING,
														attrib->nattrib_defs);
		attrib->attrib_def_values = GETMEM(attrib->attrib_def_values, STRING,
														attrib->nattrib_defs);
		attrib->attrib_def_names[natt]  = strdup(name);
		attrib->attrib_def_values[natt] = strdup(FpaCblank);
		}

	/* Now set the default attribute value */
	if ( !set_element_attribute_backdef(cline, attrib->attrib_def_names[natt],
			&(attrib->attrib_def_values[natt])) ) return FALSE;

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							add_element_type_rules

	(
	STRING						cline,	/* line in configuration file */
	FpaConfigEntryRuleStruct	*rule	/* pointer to EntryRule structure */
	)

	{
	LOGICAL		valid;
	int			nr;
	STRING		arg;
	ERULE		func;

	/* Return FALSE if no structure passed */
	if ( IsNull(rule) ) return FALSE;

	/* Add all acceptable entry rules to the list */
	valid = TRUE;
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Return FALSE for bad strings */
		if ( !OKARG(arg) ) return FALSE;

		/* Find function corresponding to entry rule name */
		func = identify_rule_function(arg);
		if ( !func )
			{
			(void) pr_error("Config",
					"[identify_rule_function] No entry rule: \"%s\"!\n", arg);
			valid = FALSE;
			continue;
			}

		/* Check that entry rule is not already in the list */
		for ( nr=0; nr<rule->nrules; nr++ )
			if ( same_ic(arg, rule->entry_rules[nr]) ) break;
		if ( nr < rule->nrules ) continue;

		/* Add to entry rules list */
		rule->nrules++;
		rule->entry_rules = GETMEM(rule->entry_rules, STRING, rule->nrules);
		rule->entry_funcs = GETMEM(rule->entry_funcs, ERULE,  rule->nrules);
		rule->entry_rules[rule->nrules - 1] = strdup(arg);
		rule->entry_funcs[rule->nrules - 1] = func;
		}

	/* Return error code */
	return valid;
	}

/**********************************************************************/

static	LOGICAL							add_element_type_py_rules

	(
	STRING						cline,	/* line in configuration file */
	FpaConfigEntryRuleStruct	*rule	/* pointer to EntryRule structure */
	)

	{
	LOGICAL		valid;
	int			nr;
	STRING		arg;

	/* Return FALSE if no structure passed */
	if ( IsNull(rule) ) return FALSE;

	/* Add all acceptable entry rules to the list */
	valid = TRUE;
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Return FALSE for bad strings */
		if ( !OKARG(arg) ) return FALSE;

		/* TODO: Check that file exits */

		/* Check that entry rule is not already in the list */
		for ( nr=0; nr<rule->py_nrules; nr++ )
			if ( same_ic(arg, rule->py_entry_rules[nr]) ) break;
		if ( nr < rule->py_nrules ) continue;

		/* Add to entry rules list */
		rule->py_nrules++;
		rule->py_entry_rules = GETMEM(rule->py_entry_rules, STRING, rule->py_nrules);
		rule->py_entry_rules[rule->py_nrules - 1] = strdup(arg);
		}

	/* Return error code */
	return valid;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ a t t r i b u t e                          *
*   s e t _ e l e m e n t _ a t t r i b u t e _ b a c k d e f          *
*                                                                      *
*   Initialize attribute information for Elements block of             *
*   configuration files.                                               *
*                                                                      *
*   s e t _ e l e m e n t _ a t t r i b u t e _ b a c k d e f          *
*                                                                      *
*   Set background values for attributes in Elements block of          *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	int								add_element_attribute

	(
	STRING							name,		/* attribute name */
	STRING							cline,		/* remainder of line in
													configuration file  */
	FpaConfigElementAttribStruct	*attrib		/* pointer to
													Attrib structure */
	)

	{
	int			natt;

	/* Error return if no structure passed */
	if ( IsNull(attrib) ) return -1;

	/* Check if name is already in the list */
	/* Set location if name is already in the list */
	for ( natt=0; natt<attrib->nattribs; natt++ )
		if ( same_ic(name, attrib->attrib_names[natt]) ) break;

	/* Add another attribute name if not found */
	if ( natt >= attrib->nattribs )
		{
		natt = attrib->nattribs++;
		attrib->attrib_names     = GETMEM(attrib->attrib_names,     STRING,
															attrib->nattribs);
		attrib->attrib_labels    = GETMEM(attrib->attrib_labels,    STRING,
															attrib->nattribs);
		attrib->attrib_sh_labels = GETMEM(attrib->attrib_sh_labels, STRING,
															attrib->nattribs);
		attrib->attrib_back_defs = GETMEM(attrib->attrib_back_defs,  STRING,
															attrib->nattribs);

		/* Initialize attribute name, labels and default setting */
		attrib->attrib_names[natt]     = strdup(name);
		attrib->attrib_labels[natt]    = strdup(name);
		attrib->attrib_sh_labels[natt] = strdup(name);
		attrib->attrib_back_defs[natt] = strdup(FpaCblank);
		}

	/* Return location if remainder of line is blank */
	if ( blank(cline) ) return natt;

	/* Set attribute background default value from remainder of line */
	if ( !set_element_attribute_backdef(cline, attrib->attrib_names[natt],
			&(attrib->attrib_back_defs[natt])) ) return -1;

	/* Return location of attribute if all went OK */
	return natt;
	}

/**********************************************************************/

static	LOGICAL							set_element_attribute_backdef

	(
	STRING							cline,		/* remainder of line in
													configuration file  */
	STRING							attname,	/* attribute name */
	STRING							*attval		/* attribute default value */
	)

	{
	STRING		arg;

	/* Return FALSE for bad strings */
	if ( blank(cline) )   return FALSE;
	if ( IsNull(attval) ) return FALSE;

	/* Free space used by attribute default value (if required) */
	if ( NotNull(*attval) ) FREEMEM(*attval);

	/* Check for special wind attributes */
	if ( same_ic(attname, AttribWindModel)
			|| same_ic(attname, AttribWindDirection)
			|| same_ic(attname, AttribWindSpeed)
			|| same_ic(attname, AttribWindGust) )
		{
		arg = build_wind_attrib_string(attname, cline);
		if ( !OKARG(arg) ) return FALSE;
		*attval = strdup(arg);
		}

	/* Set all other attributes */
	else
		{
		arg = string_arg(cline);
		if ( !ISARG(arg) ) return FALSE;
		*attval = strdup(arg);
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   s e t _ e l e m e n t _ e d i t o r _ e n t r y _ f i l e          *
*   s e t _ e l e m e n t _ e d i t o r _ n o d e _ e n t r y          *
*                                                            _ f i l e *
*   s e t _ e l e m e n t _ e d i t o r _ m o d i f y _ f i l e        *
*   s e t _ e l e m e n t _ e d i t o r _ n o d e _ m o d i f y        *
*                                                            _ f i l e *
*   s e t _ e l e m e n t _ e d i t o r _ m e m o r y _ f i l e        *
*   s e t _ e l e m e n t _ e d i t o r _ b a c k _ f i l e            *
*   s e t _ e l e m e n t _ e d i t o r _ b a c k _ m e m _ f i l e    *
*   a d d _ e l e m e n t _ e d i t o r _ r u l e s                    *
*   a d d _ e l e m e n t _ e d i t o r _ n o d e _ r u l e s          *
*   a d d _ e l e m e n t _ e d i t o r _ m e r g e _ f i e l d s      *
*   a d d _ e l e m e n t _ e d i t o r _ l i n k _ f i e l d s        *
*                                                                      *
*   Initialize editor information for Elements block of                *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	LOGICAL							set_element_editor_entry_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set entry file name based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			continuous->entry_file = STRMEM(continuous->entry_file, arg);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			vector->entry_file = STRMEM(vector->entry_file, arg);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			discrete->entry_file = STRMEM(discrete->entry_file, arg);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			wind->entry_file = STRMEM(wind->entry_file, arg);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			line->entry_file = STRMEM(line->entry_file, arg);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			scattered->entry_file = STRMEM(scattered->entry_file, arg);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->entry_file = STRMEM(lchain->entry_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							set_element_editor_node_entry_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set link chain node entry file name based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->node_entry_file = STRMEM(lchain->node_entry_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							set_element_editor_modify_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set modify file name based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			continuous->modify_file = STRMEM(continuous->modify_file, arg);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			vector->modify_file = STRMEM(vector->modify_file, arg);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			discrete->modify_file = STRMEM(discrete->modify_file, arg);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			wind->modify_file = STRMEM(wind->modify_file, arg);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			line->modify_file = STRMEM(line->modify_file, arg);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			scattered->modify_file = STRMEM(scattered->modify_file, arg);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->modify_file = STRMEM(lchain->modify_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							set_element_editor_node_modify_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set link chain node modify file name based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->node_modify_file = STRMEM(lchain->node_modify_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							set_element_editor_memory_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set memory file name based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			continuous->memory_file = STRMEM(continuous->memory_file, arg);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			vector->memory_file = STRMEM(vector->memory_file, arg);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			discrete->memory_file = STRMEM(discrete->memory_file, arg);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			wind->memory_file = STRMEM(wind->memory_file, arg);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			line->memory_file = STRMEM(line->memory_file, arg);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			scattered->memory_file = STRMEM(scattered->memory_file, arg);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->memory_file = STRMEM(lchain->memory_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							set_element_editor_back_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set background entry file name based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			continuous->back_entry_file =
					STRMEM(continuous->back_entry_file, arg);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			vector->back_entry_file = STRMEM(vector->back_entry_file, arg);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			discrete->back_entry_file = STRMEM(discrete->back_entry_file, arg);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			wind->back_entry_file = STRMEM(wind->back_entry_file, arg);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			line->back_entry_file = STRMEM(line->back_entry_file, arg);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			scattered->back_entry_file =
					STRMEM(scattered->back_entry_file, arg);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->back_entry_file = STRMEM(lchain->back_entry_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							set_element_editor_back_mem_file

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	STRING								arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Return FALSE for bad strings */
	arg = string_arg(cline);
	if ( !OKARG(arg) ) return FALSE;

	/* Set background memory file name based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			continuous->back_memory_file =
					STRMEM(continuous->back_memory_file, arg);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			vector->back_memory_file = STRMEM(vector->back_memory_file, arg);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			discrete->back_memory_file =
					STRMEM(discrete->back_memory_file, arg);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			wind->back_memory_file = STRMEM(wind->back_memory_file, arg);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			line->back_memory_file = STRMEM(line->back_memory_file, arg);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			scattered->back_memory_file =
					STRMEM(scattered->back_memory_file, arg);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			lchain->back_memory_file = STRMEM(lchain->back_memory_file, arg);
			break;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL							add_element_editor_rules

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	LOGICAL								valid;
	int									nrules, nr;
	STRING								*entry_rules, arg;
	ERULE								*entry_funcs, func;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Set entry rules parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			nrules      = continuous->nrules;
			entry_rules = continuous->entry_rules;
			entry_funcs = continuous->entry_funcs;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			nrules      = vector->nrules;
			entry_rules = vector->entry_rules;
			entry_funcs = vector->entry_funcs;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			nrules      = discrete->nrules;
			entry_rules = discrete->entry_rules;
			entry_funcs = discrete->entry_funcs;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			nrules      = wind->nrules;
			entry_rules = wind->entry_rules;
			entry_funcs = wind->entry_funcs;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			nrules      = line->nrules;
			entry_rules = line->entry_rules;
			entry_funcs = line->entry_funcs;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			nrules      = scattered->nrules;
			entry_rules = scattered->entry_rules;
			entry_funcs = scattered->entry_funcs;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			nrules      = lchain->nrules;
			entry_rules = lchain->entry_rules;
			entry_funcs = lchain->entry_funcs;
			break;
		}

	/* Add all acceptable entry rules to the list */
	valid = TRUE;
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Return FALSE for bad strings */
		if ( !OKARG(arg) ) return FALSE;

		/* Find function corresponding to entry rule name */
		func = identify_rule_function(arg);
		if ( !func )
			{
			(void) pr_error("Config",
					"[identify_rule_function] No entry rule: \"%s\"!\n", arg);
			valid = FALSE;
			continue;
			}

		/* Check that entry rule is not already in the list */
		for ( nr=0; nr<nrules; nr++ )
			if ( same_ic(arg, entry_rules[nr]) ) break;
		if ( nr < nrules ) continue;

		/* Add to entry rules list */
		nrules++;
		entry_rules = GETMEM(entry_rules, STRING, nrules);
		entry_funcs = GETMEM(entry_funcs, ERULE,  nrules);
		entry_rules[nrules - 1] = strdup(arg);
		entry_funcs[nrules - 1] = func;
		}

	/* Reset entry rules parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous->nrules      = nrules;
			continuous->entry_rules = entry_rules;
			continuous->entry_funcs = entry_funcs;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector->nrules      = nrules;
			vector->entry_rules = entry_rules;
			vector->entry_funcs = entry_funcs;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete->nrules      = nrules;
			discrete->entry_rules = entry_rules;
			discrete->entry_funcs = entry_funcs;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind->nrules      = nrules;
			wind->entry_rules = entry_rules;
			wind->entry_funcs = entry_funcs;
			break;

		/* Line field type */
		case FpaC_LINE:
			line->nrules      = nrules;
			line->entry_rules = entry_rules;
			line->entry_funcs = entry_funcs;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered->nrules      = nrules;
			scattered->entry_rules = entry_rules;
			scattered->entry_funcs = entry_funcs;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain->nrules      = nrules;
			lchain->entry_rules = entry_rules;
			lchain->entry_funcs = entry_funcs;
			break;
		}

	/* Return error code */
	return valid;
	}

/**********************************************************************/

static	LOGICAL							add_element_editor_py_rules

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	LOGICAL								valid;
	int									nrules, nr;
	STRING								*entry_rules, arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Set entry rules parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			nrules      = continuous->py_nrules;
			entry_rules = continuous->py_entry_rules;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			nrules      = vector->py_nrules;
			entry_rules = vector->py_entry_rules;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			nrules      = discrete->py_nrules;
			entry_rules = discrete->py_entry_rules;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			nrules      = wind->py_nrules;
			entry_rules = wind->py_entry_rules;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			nrules      = line->py_nrules;
			entry_rules = line->py_entry_rules;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			nrules      = scattered->py_nrules;
			entry_rules = scattered->py_entry_rules;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			nrules      = lchain->py_nrules;
			entry_rules = lchain->py_entry_rules;
			break;
		}

	/* Add all acceptable entry rules to the list */
	valid = TRUE;
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Return FALSE for bad strings */
		if ( !OKARG(arg) ) return FALSE;

		/*>>> TODO: Confirm script file exists here?  <<<*/

		/* Check that entry rule is not already in the list */
		for ( nr=0; nr<nrules; nr++ )
			if ( same_ic(arg, entry_rules[nr]) ) break;
		if ( nr < nrules ) continue;

		/* Add to entry rules list */
		nrules++;
		entry_rules = GETMEM(entry_rules, STRING, nrules);
		entry_rules[nrules - 1] = strdup(arg);
		}

	/* Reset entry rules parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous->py_nrules      = nrules;
			continuous->py_entry_rules = entry_rules;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector->py_nrules      = nrules;
			vector->py_entry_rules = entry_rules;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete->py_nrules      = nrules;
			discrete->py_entry_rules = entry_rules;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind->py_nrules      = nrules;
			wind->py_entry_rules = entry_rules;
			break;

		/* Line field type */
		case FpaC_LINE:
			line->py_nrules      = nrules;
			line->py_entry_rules = entry_rules;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered->py_nrules      = nrules;
			scattered->py_entry_rules = entry_rules;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain->py_nrules      = nrules;
			lchain->py_entry_rules = entry_rules;
			break;
		}

	/* Return error code */
	return valid;
	}

/**********************************************************************/

static	LOGICAL							add_element_editor_node_rules

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	LOGICAL								valid;
	int									nrules, nr;
	STRING								*entry_rules, arg;
	ERULE								*entry_funcs, func;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Set entry rules parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			nrules      = lchain->nnode_rules;
			entry_rules = lchain->node_entry_rules;
			entry_funcs = lchain->node_entry_funcs;
			break;
		}

	/* Add all acceptable entry rules to the list */
	valid = TRUE;
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Return FALSE for bad strings */
		if ( !OKARG(arg) ) return FALSE;

		/* Find function corresponding to entry rule name */
		func = identify_rule_function(arg);
		if ( !func )
			{
			(void) pr_error("Config",
					"[identify_rule_function] No entry rule: \"%s\"!\n", arg);
			valid = FALSE;
			continue;
			}

		/* Check that entry rule is not already in the list */
		for ( nr=0; nr<nrules; nr++ )
			if ( same_ic(arg, entry_rules[nr]) ) break;
		if ( nr < nrules ) continue;

		/* Add to entry rules list */
		nrules++;
		entry_rules = GETMEM(entry_rules, STRING, nrules);
		entry_funcs = GETMEM(entry_funcs, ERULE,  nrules);
		entry_rules[nrules - 1] = strdup(arg);
		entry_funcs[nrules - 1] = func;
		}

	/* Reset entry rules parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain->nnode_rules      = nrules;
			lchain->node_entry_rules = entry_rules;
			lchain->node_entry_funcs = entry_funcs;
			break;
		}

	/* Return error code */
	return valid;
	}

/**********************************************************************/

static	LOGICAL							add_element_editor_py_node_rules

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	LOGICAL								valid;
	int									nrules, nr;
	STRING								*entry_rules, arg;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Set entry rules parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			nrules      = lchain->py_nnode_rules;
			entry_rules = lchain->py_node_entry_rules;
			break;
		}

	/* Add all acceptable entry rules to the list */
	valid = TRUE;
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Return FALSE for bad strings */
		if ( !OKARG(arg) ) return FALSE;

		
		/*>>> TODO: Confirm script file exists here?  <<<*/

		/* Check that entry rule is not already in the list */
		for ( nr=0; nr<nrules; nr++ )
			if ( same_ic(arg, entry_rules[nr]) ) break;
		if ( nr < nrules ) continue;

		/* Add to entry rules list */
		nrules++;
		entry_rules = GETMEM(entry_rules, STRING, nrules);
		entry_rules[nrules - 1] = strdup(arg);
		}

	/* Reset entry rules parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain->py_nnode_rules      = nrules;
			lchain->py_node_entry_rules = entry_rules;
			break;
		}

	/* Return error code */
	return valid;
	}

/**********************************************************************/

static	LOGICAL							add_element_editor_merge_fields

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	LOGICAL								valid;
	int									nmerge, nm;
	STRING								arg;
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;
	FpaConfigElementStruct				*edef, **merge_elems;
	FpaConfigLevelStruct				*ldef, **merge_levels;

	/* Static buffers for element and level names for merge fields */
	static	char	element[CONFIG_LABEL_LEN];
	static	char	level[CONFIG_LABEL_LEN];

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Set merge field parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			nmerge       = continuous->nmerge;
			merge_elems  = continuous->merge_elems;
			merge_levels = continuous->merge_levels;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			nmerge       = vector->nmerge;
			merge_elems  = vector->merge_elems;
			merge_levels = vector->merge_levels;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			nmerge       = discrete->nmerge;
			merge_elems  = discrete->merge_elems;
			merge_levels = discrete->merge_levels;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			nmerge       = wind->nmerge;
			merge_elems  = wind->merge_elems;
			merge_levels = wind->merge_levels;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			nmerge       = line->nmerge;
			merge_elems  = line->merge_elems;
			merge_levels = line->merge_levels;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			nmerge       = scattered->nmerge;
			merge_elems  = scattered->merge_elems;
			merge_levels = scattered->merge_levels;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			nmerge       = lchain->nmerge;
			merge_elems  = lchain->merge_elems;
			merge_levels = lchain->merge_levels;
			break;
		}

	/* Add all acceptable merge fields to the list */
	valid = TRUE;
	while ( !blank(cline) )
		{

		/* Check for the named element */
		arg = string_arg(cline);
		if ( !OKARG(arg) ) return FALSE;
		(void) strcpy(element, arg);
		edef = identify_element(element);
		if ( IsNull(edef) ) return FALSE;

		/* Check for the named level (if present) */
		arg = string_arg(cline);
		if ( !OKARG(arg) ) return FALSE;
		(void) strcpy(level, arg);
		if ( same_ic(level, FpaCdefault) )
			{
			ldef = NullPtr(FpaConfigLevelStruct *);
			}
		else
			{
			ldef = identify_level(arg);
			if ( IsNull(ldef) ) return FALSE;
			}

		/* Check that merge field is not already in the list */
		for ( nm=0; nm<nmerge; nm++ )
			{
			if ( edef == merge_elems[nm] && ldef == merge_levels[nm] ) break;
			}
		if ( nm < nmerge ) continue;

		/* Add to merge field to list */
		nmerge++;
		merge_elems  = GETMEM(merge_elems,  FpaConfigElementStruct *, nmerge);
		merge_levels = GETMEM(merge_levels, FpaConfigLevelStruct *,   nmerge);
		merge_elems[nmerge - 1]  = edef;
		merge_levels[nmerge - 1] = ldef;
		}

	/* Reset merge field parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous->nmerge       = nmerge;
			continuous->merge_elems  = merge_elems;
			continuous->merge_levels = merge_levels;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector->nmerge       = nmerge;
			vector->merge_elems  = merge_elems;
			vector->merge_levels = merge_levels;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete->nmerge       = nmerge;
			discrete->merge_elems  = merge_elems;
			discrete->merge_levels = merge_levels;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind->nmerge       = nmerge;
			wind->merge_elems  = merge_elems;
			wind->merge_levels = merge_levels;
			break;

		/* Line field type */
		case FpaC_LINE:
			line->nmerge       = nmerge;
			line->merge_elems  = merge_elems;
			line->merge_levels = merge_levels;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered->nmerge       = nmerge;
			scattered->merge_elems  = merge_elems;
			scattered->merge_levels = merge_levels;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain->nmerge       = nmerge;
			lchain->merge_elems  = merge_elems;
			lchain->merge_levels = merge_levels;
			break;
		}

	/* Return error code */
	return valid;
	}

/**********************************************************************/

static	LOGICAL							add_element_editor_link_fields

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	LOGICAL								valid;
	int									nlink, nl;
	STRING								arg;
	FpaConfigLchainEditorStruct			*lchain;
	FpaConfigElementStruct				*edef, **link_elems;
	FpaConfigLevelStruct				*ldef, **link_levels;

	/* Static buffers for element and level names for merge link fields */
	static	char	element[CONFIG_LABEL_LEN];
	static	char	level[CONFIG_LABEL_LEN];

	/* Return FALSE if no structure passed */
	if ( IsNull(editor) ) return FALSE;

	/* Set merge link field parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			nlink       = lchain->nlink;
			link_elems  = lchain->link_elems;
			link_levels = lchain->link_levels;
			break;

		/* Cannot merge link fields for other field types */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
			/* >>>>> error message??? <<<<< */
			return FALSE;
		}

	/* Add all acceptable merge link fields to the list */
	valid = TRUE;
	while ( !blank(cline) )
		{

		/* Check for the named element */
		arg = string_arg(cline);
		if ( !OKARG(arg) ) return FALSE;
		(void) strcpy(element, arg);
		edef = identify_element(element);
		if ( IsNull(edef) ) return FALSE;

		/* Check for the named level (if present) */
		arg = string_arg(cline);
		if ( !OKARG(arg) ) return FALSE;
		(void) strcpy(level, arg);
		if ( same_ic(level, FpaCdefault) )
			{
			ldef = NullPtr(FpaConfigLevelStruct *);
			}
		else
			{
			ldef = identify_level(arg);
			if ( IsNull(ldef) ) return FALSE;
			}

		/* Check that merge link field is not already in the list */
		for ( nl=0; nl<nlink; nl++ )
			{
			if ( edef == link_elems[nl] && ldef == link_levels[nl] ) break;
			}
		if ( nl < nlink ) continue;

		/* Add to merge link field to list */
		nlink++;
		link_elems  = GETMEM(link_elems,  FpaConfigElementStruct *, nlink);
		link_levels = GETMEM(link_levels, FpaConfigLevelStruct *,   nlink);
		link_elems[nlink - 1]  = edef;
		link_levels[nlink - 1] = ldef;
		}

	/* Reset merge link field parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain->nlink       = nlink;
			lchain->link_elems  = link_elems;
			lchain->link_levels = link_levels;
			break;
		}

	/* Return error code */
	return valid;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ l a b e l l i n g _ t y p e                *
*                                                                      *
*   Initialize labelling information for Elements block of             *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	int								add_element_labelling_type

	(
	STRING							name,		/* labelling type name */
	FpaConfigElementLabellingStruct	*labelling	/* pointer to
													ElementLabelling structure */
	)

	{
	int		nlab;

	/* Error return if no structure passed */
	if ( IsNull(labelling) ) return -1;

	/* Return location if name is already in the list */
	for ( nlab=0; nlab<labelling->ntypes; nlab++ )
		if ( same_ic(name, labelling->type_names[nlab]) ) return nlab;

	/* Add another labelling type name */
	nlab = labelling->ntypes++;
	labelling->type_names        = GETMEM(labelling->type_names,       STRING,
														labelling->ntypes);
	labelling->type_labels       = GETMEM(labelling->type_labels,      STRING,
														labelling->ntypes);
	labelling->type_sh_labels    = GETMEM(labelling->type_sh_labels,   STRING,
														labelling->ntypes);
	labelling->type_classes      = GETMEM(labelling->type_classes,     STRING,
														labelling->ntypes);
	labelling->type_entry_files  = GETMEM(labelling->type_entry_files, STRING,
														labelling->ntypes);
	labelling->type_modify_files = GETMEM(labelling->type_modify_files, STRING,
														labelling->ntypes);
	labelling->type_attach_opts  = GETMEM(labelling->type_attach_opts,
											FpaCattachOption,
														labelling->ntypes);
	labelling->type_attribs      = GETMEM(labelling->type_attribs,
											FpaConfigDefaultAttribStruct,
														labelling->ntypes);
	labelling->type_rules        = GETMEM(labelling->type_rules,
											FpaConfigEntryRuleStruct,
														labelling->ntypes);

	/* Initialize labelling type name, labels, attach options, */
	/*  attributes, and entry rules                            */
	labelling->type_names[nlab]        = strdup(name);
	labelling->type_labels[nlab]       = strdup(name);
	labelling->type_sh_labels[nlab]    = strdup(name);
	labelling->type_classes[nlab]      = NullString;
	labelling->type_entry_files[nlab]  = NullString;
	labelling->type_modify_files[nlab] = NullString;
	labelling->type_attach_opts[nlab]  = FpaCnoMacro;
	labelling->type_attribs[nlab].nattrib_defs      = 0;
	labelling->type_attribs[nlab].attrib_def_names  = NullStringList;
	labelling->type_attribs[nlab].attrib_def_values = NullStringList;
	labelling->type_rules[nlab].nrules              = 0;
	labelling->type_rules[nlab].entry_rules         = NullStringList;
	labelling->type_rules[nlab].entry_funcs         = NullEruleList;
	labelling->type_rules[nlab].py_nrules           = 0;
	labelling->type_rules[nlab].py_entry_rules      = NullStringList;

	/* Return location if all went OK */
	return nlab;
	}

/***********************************************************************
*                                                                      *
*   a d d _ c o n t i n u o u s _ s a m p l e _ v a l u e s            *
*   a d d _ c o n t i n u o u s _ s a m p l e _ w i n d s              *
*                                                                      *
*   Set detailed information from "value_sample_types =" or from       *
*   "wind_sample_types =" line of "sample =" block in Elements         *
*   block of configuration files.                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_continuous_sample_values

	(
	STRING								cline,	/* line in configuration file */
	FpaConfigContinuousSamplingStruct	*continuous	/* pointer to Continuous
														Sampling structure   */
	)

	{
	int							nn;
	STRING						arg;
	FpaConfigSampleStruct		*xdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(continuous) ) return FALSE;

	/* Add all acceptable continuous value samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Get continuous sample value */
		xdef = identify_sample(FpaCsamplesValues, arg);
		FREEMEM(arg);

		/* Return FALSE for unrecognized continuous value samples */
		if ( IsNull(xdef) ) return FALSE;

		/* Check that continuous value sample is not already in the list */
		for ( nn=0; nn<continuous->nsample; nn++ )
			if ( xdef == continuous->samples[nn] ) break;
		if ( nn < continuous->nsample ) continue;

		/* Add to continuous value sample list */
		continuous->nsample++;
		continuous->samples = GETMEM(continuous->samples,
								FpaConfigSampleStruct *, continuous->nsample);
		continuous->samples[continuous->nsample - 1] = xdef;
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL					add_continuous_sample_winds

	(
	STRING								cline,	/* line in configuration file */
	FpaConfigContinuousSamplingStruct	*continuous	/* pointer to Continuous
														Sampling structure   */
	)

	{
	int							nn;
	STRING						arg;
	FpaConfigSampleStruct		*xdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(continuous) ) return FALSE;

	/* Add all acceptable continuous wind samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Get continuous sample wind */
		xdef = identify_sample(FpaCsamplesWinds, arg);
		FREEMEM(arg);

		/* Return FALSE for unrecognized continuous wind samples */
		if ( IsNull(xdef) ) return FALSE;

		/* Check that continuous wind sample is not already in the list */
		for ( nn=0; nn<continuous->nwindsamp; nn++ )
			if ( xdef == continuous->windsamps[nn] ) break;
		if ( nn < continuous->nwindsamp ) continue;

		/* Add to continuous wind sample list */
		continuous->nwindsamp++;
		continuous->windsamps = GETMEM(continuous->windsamps,
								FpaConfigSampleStruct *, continuous->nwindsamp);
		continuous->windsamps[continuous->nwindsamp - 1] = xdef;
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ v e c t o r _ s a m p l e _ v a l u e s                    *
*   a d d _ v e c t o r _ s a m p l e _ w i n d s                      *
*                                                                      *
*   Set detailed information from "value_sample_types =" or from       *
*   "wind_sample_types =" line of "sample =" block in Elements         *
*   block of configuration files.                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_vector_sample_values

	(
	STRING							cline,	/* line in configuration file */
	FpaConfigVectorSamplingStruct	*vector /* pointer to
												VectorSampling structure */
	)

	{
	int							nn;
	STRING						arg;
	FpaConfigSampleStruct		*xdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(vector) ) return FALSE;

	/* Add all acceptable vector value samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Get vector sample value */
		xdef = identify_sample(FpaCsamplesValues, arg);
		FREEMEM(arg);

		/* Return FALSE for unrecognized vector value samples */
		if ( IsNull(xdef) ) return FALSE;

		/* Check that vector value sample is not already in the list */
		for ( nn=0; nn<vector->nsample; nn++ )
			if ( xdef == vector->samples[nn] ) break;
		if ( nn < vector->nsample ) continue;

		/* Add to vector value sample list */
		vector->nsample++;
		vector->samples = GETMEM(vector->samples,
								FpaConfigSampleStruct *, vector->nsample);
		vector->samples[vector->nsample - 1] = xdef;
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL					add_vector_sample_winds

	(
	STRING							cline,	/* line in configuration file */
	FpaConfigVectorSamplingStruct	*vector /* pointer to
												VectorSampling structure */
	)

	{
	int							nn;
	STRING						arg;
	FpaConfigSampleStruct		*xdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(vector) ) return FALSE;

	/* Add all acceptable vector wind samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Get vector sample wind */
		xdef = identify_sample(FpaCsamplesWinds, arg);
		FREEMEM(arg);

		/* Return FALSE for unrecognized vector wind samples */
		if ( IsNull(xdef) ) return FALSE;

		/* Check that vector wind sample is not already in the list */
		for ( nn=0; nn<vector->nwindsamp; nn++ )
			if ( xdef == vector->windsamps[nn] ) break;
		if ( nn < vector->nwindsamp ) continue;

		/* Add to vector wind sample list */
		vector->nwindsamp++;
		vector->windsamps = GETMEM(vector->windsamps,
								FpaConfigSampleStruct *, vector->nwindsamp);
		vector->windsamps[vector->nwindsamp - 1] = xdef;
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ d i s c r e t e _ s a m p l e _ a t t r i b s              *
*                                                                      *
*   Set detailed information from "attrib_sample_names =" line of      *
*   "sample =" block in Elements block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_discrete_sample_attribs

	(
	STRING							cline,		/* line in configuration file */
	FpaConfigDiscreteSamplingStruct	*discrete	/* pointer to Discrete
													Sampling structure */
	)

	{
	int			nn;
	STRING		arg;

	/* Return FALSE if no structure passed */
	if ( IsNull(discrete) ) return FALSE;

	/* Add all acceptable discrete attribute samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Check that discrete attribute sample is not already in the list */
		for ( nn=0; nn<discrete->nsattribs; nn++ )
			if ( same_ic(arg, discrete->sattrib_names[nn]) ) break;
		if ( nn < discrete->nsattribs )
			{
			FREEMEM(arg);
			continue;
			}

		/* Add to discrete attribute sample list */
		discrete->nsattribs++;
		discrete->sattrib_names =
				GETMEM(discrete->sattrib_names, STRING, discrete->nsattribs);
		discrete->sattrib_names[discrete->nsattribs - 1] = strdup(arg);
		FREEMEM(arg);
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ w i n d _ s a m p l e _ v a l u e s                        *
*   a d d _ w i n d _ s a m p l e _ c r o s s r e f s                  *
*                                                                      *
*   Set detailed information from "value_sample_types =" or from       *
*   "wind_crossrefs =" line of "sample =" block in Elements            *
*   block of configuration files.                                      *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_wind_sample_values

	(
	STRING							cline,		/* line in configuration file */
	FpaConfigWindSamplingStruct		*wind		/* pointer to
													WindSampling structure */
	)

	{
	int							nn;
	STRING						arg;
	FpaConfigSampleStruct		*xdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(wind) ) return FALSE;

	/* Add all acceptable wind value samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Get wind sample value */
		xdef = identify_sample(FpaCsamplesValues, arg);
		FREEMEM(arg);

		/* Return FALSE for unrecognized wind value samples */
		if ( IsNull(xdef) ) return FALSE;

		/* Check that wind value sample is not already in the list */
		for ( nn=0; nn<wind->nsample; nn++ )
			if ( xdef == wind->samples[nn] ) break;
		if ( nn < wind->nsample ) continue;

		/* Add to wind value sample list */
		wind->nsample++;
		wind->samples = GETMEM(wind->samples,
								FpaConfigSampleStruct *, wind->nsample);
		wind->samples[wind->nsample - 1] = xdef;
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	LOGICAL					add_wind_sample_crossrefs

	(
	STRING							cline,		/* line in configuration file */
	FpaConfigWindSamplingStruct		*wind		/* pointer to
													WindSampling structure */
	)

	{
	int							nn;
	STRING						arg;
	FpaConfigCrossRefStruct		*crdef;

	/* Return FALSE if no structure passed */
	if ( IsNull(wind) ) return FALSE;

	/* Add all acceptable wind cross references to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Get wind cross reference */
		crdef = identify_crossref(FpaCcRefsWinds, arg);
		FREEMEM(arg);

		/* Return FALSE for unrecognized wind cross references */
		if ( IsNull(crdef) ) return FALSE;

		/* Check that wind cross reference is not already in the list */
		for ( nn=0; nn<wind->nwcref; nn++ )
			if ( crdef == wind->wcrefs[nn] ) break;
		if ( nn < wind->nwcref ) continue;

		/* Add to wind cross reference list */
		wind->nwcref++;
		wind->wcrefs = GETMEM(wind->wcrefs,
								FpaConfigCrossRefStruct *, wind->nwcref);
		wind->wcrefs[wind->nwcref - 1] = crdef;
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ l i n e _ s a m p l e _ a t t r i b s                      *
*                                                                      *
*   Set detailed information from "attrib_sample_names =" line of      *
*   "sample =" block in Elements block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_line_sample_attribs

	(
	STRING							cline,		/* line in configuration file */
	FpaConfigLineSamplingStruct		*line		/* pointer to
													LineSampling structure */
	)

	{
	int			nn;
	STRING		arg;

	/* Return FALSE if no structure passed */
	if ( IsNull(line) ) return FALSE;

	/* Add all acceptable line attribute samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Check that line attribute sample is not already in the list */
		for ( nn=0; nn<line->nsattribs; nn++ )
			if ( same_ic(arg, line->sattrib_names[nn]) ) break;
		if ( nn < line->nsattribs )
			{
			FREEMEM(arg);
			continue;
			}

		/* Add to line attribute sample list */
		line->nsattribs++;
		line->sattrib_names =
				GETMEM(line->sattrib_names, STRING, line->nsattribs);
		line->sattrib_names[line->nsattribs - 1] = strdup(arg);
		FREEMEM(arg);
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ s c a t t e r e d _ s a m p l e _ a t t r i b s            *
*                                                                      *
*   Set detailed information from "attrib_sample_names =" line of      *
*   "sample =" block in Elements block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_scattered_sample_attribs

	(
	STRING								cline,		/* line in configuration
														file */
	FpaConfigScatteredSamplingStruct	*scattered	/* pointer to Scattered
														Sampling structure  */
	)

	{
	int			nn;
	STRING		arg;

	/* Return FALSE if no structure passed */
	if ( IsNull(scattered) ) return FALSE;

	/* Add all acceptable scattered attribute samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Check that scattered attribute sample is not already in the list */
		for ( nn=0; nn<scattered->nsattribs; nn++ )
			if ( same_ic(arg, scattered->sattrib_names[nn]) ) break;
		if ( nn < scattered->nsattribs )
			{
			FREEMEM(arg);
			continue;
			}

		/* Add to scattered attribute sample list */
		scattered->nsattribs++;
		scattered->sattrib_names =
				GETMEM(scattered->sattrib_names, STRING, scattered->nsattribs);
		scattered->sattrib_names[scattered->nsattribs - 1] = strdup(arg);
		FREEMEM(arg);
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ l c h a i n _ s a m p l e _ a t t r i b s                  *
*                                                                      *
*   Set detailed information from "attrib_sample_names =" line of      *
*   "sample =" block in Elements block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					add_lchain_sample_attribs

	(
	STRING							cline,		/* line in configuration file */
	FpaConfigLchainSamplingStruct	*lchain		/* pointer to Lchain
													Sampling structure */
	)

	{
	int			nn;
	STRING		arg;

	/* Return FALSE if no structure passed */
	if ( IsNull(lchain) ) return FALSE;

	/* Add all acceptable link chain attribute samples to new list */
	while ( NotNull( arg = strdup_arg(cline) ) )
		{

		/* Check that link chain attribute sample is not already in the list */
		for ( nn=0; nn<lchain->nsattribs; nn++ )
			if ( same_ic(arg, lchain->sattrib_names[nn]) ) break;
		if ( nn < lchain->nsattribs )
			{
			FREEMEM(arg);
			continue;
			}

		/* Add to link chain attribute sample list */
		lchain->nsattribs++;
		lchain->sattrib_names =
				GETMEM(lchain->sattrib_names, STRING, lchain->nsattribs);
		lchain->sattrib_names[lchain->nsattribs - 1] = strdup(arg);
		FREEMEM(arg);
		}
	FREEMEM(arg);

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   a d d _ e l e m e n t _ l i n k _ f i e l d s                      *
*                                                                      *
*   Set detailed information from "link_fields =" line of              *
*   "linking =" block in Elements block of configuration files.        *
*                                                                      *
***********************************************************************/

static	LOGICAL							add_element_link_fields

	(
	STRING							cline,		/* line in configuration file */
	int								type,		/* enumerated field type */
	FpaConfigElementLinkingStruct	*linking	/* pointer to
													ElementLinking structure */
	)

	{
	LOGICAL								valid;
	int									nlink, nl;
	STRING								arg;
	FpaConfigElementStruct				*edef, **link_elems;
	FpaConfigLevelStruct				*ldef, **link_levels;

	/* Static buffers for element and level names for link fields */
	static	char	element[CONFIG_LABEL_LEN];
	static	char	level[CONFIG_LABEL_LEN];

	/* Return FALSE if no structure passed */
	if ( IsNull(linking) ) return FALSE;

	/* Set link field parameters based on field type */
	switch ( type )
		{

		/* For the moment ... all field types have same structure */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
			nlink       = linking->nlink;
			link_elems  = linking->link_elems;
			link_levels = linking->link_levels;
			break;

		/* Unknown field types */
		default:
			return FALSE;
		}

	/* Add all acceptable link fields to the list */
	valid = TRUE;
	while ( !blank(cline) )
		{

		/* Check for the named element */
		arg = string_arg(cline);
		if ( !OKARG(arg) ) return FALSE;
		(void) strcpy(element, arg);
		edef = identify_element(element);
		if ( IsNull(edef) ) return FALSE;

		/* Check for the named level (if present) */
		arg = string_arg(cline);
		if ( !OKARG(arg) ) return FALSE;
		(void) strcpy(level, arg);
		if ( same_ic(level, FpaCdefault) )
			{
			ldef = NullPtr(FpaConfigLevelStruct *);
			}
		else
			{
			ldef = identify_level(arg);
			if ( IsNull(ldef) ) return FALSE;
			}

		/* Check that link field is not already in the list */
		for ( nl=0; nl<nlink; nl++ )
			{
			if ( edef == link_elems[nl] && ldef == link_levels[nl] ) break;
			}
		if ( nl < nlink ) continue;

		/* Add to link field to list */
		nlink++;
		link_elems  = GETMEM(link_elems,  FpaConfigElementStruct *, nlink);
		link_levels = GETMEM(link_levels, FpaConfigLevelStruct *,   nlink);
		link_elems[nlink - 1]  = edef;
		link_levels[nlink - 1] = ldef;
		}

	/* Reset link field parameters based on field type */
	switch ( type )
		{

		/* For the moment ... all field types have same structure */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
			linking->nlink       = nlink;
			linking->link_elems  = link_elems;
			linking->link_levels = link_levels;
			break;
		}

	/* Return error code */
	return valid;
	}

/***********************************************************************
*                                                                      *
*   s e t _ v c a l c _ s r c _ t y p e s                              *
*                                                                      *
*   Set detailed information from "source_types =" line of             *
*   "value_calculation =" block in Elements block of configuration     *
*   files.                                                             *
*                                                                      *
***********************************************************************/

static	LOGICAL					set_vcalc_src_types

	(
	STRING							cline,		/* line in configuration file */
	FpaConfigElementValCalcStruct	*valcalc	/* pointer to
													ValCalc structure */
	)

	{
	int			nn, macro;
	STRING		arg;

	/* Return FALSE if no structure passed */
	if ( IsNull(valcalc) ) return FALSE;

	/* Empty the current source type list */
	if ( valcalc->nsrc_type > 0 )
		{
		FREEMEM(valcalc->src_types);
		valcalc->nsrc_type = 0;
		}

	/* Add all acceptable source types to new list */
	while ( NotNull( arg = string_arg(cline) ) )
		{

		/* Identify source type */
		macro = config_file_macro(arg, NumFpaCsourceTypes, FpaCsourceTypes);

		/* Return FALSE for unrecognized source types */
		if ( macro == FpaCnoMacro ) return FALSE;

		/* Check that source type is not already in the list */
		for ( nn=0; nn<valcalc->nsrc_type; nn++ )
			if ( macro == valcalc->src_types[nn] ) break;
		if ( nn < valcalc->nsrc_type ) continue;

		/* Add to source type list */
		valcalc->nsrc_type++;
		valcalc->src_types = GETMEM(valcalc->src_types, FpaCsourceTypeOption,
										valcalc->nsrc_type);
		valcalc->src_types[valcalc->nsrc_type - 1] = macro;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   s e t _ c o m p o n e n t s _ c o m p o n e n t                    *
*                                                                      *
*   Set detailed information from "x_component =" or "y_component ="   *
*   line of "components =" block in Elements block of configuration    *
*   files.                                                             *
*                                                                      *
***********************************************************************/

static	LOGICAL					set_components_component

	(
	STRING							element,	/* component element */
	COMPONENT						comptype,	/* component type */
	FpaConfigElementComponentStruct	*components	/* pointer to
													ElementComponent structure */
	)

	{
	int						nn;
	FpaConfigElementStruct	*edef;

	/* Return FALSE if no structure passed */
	if ( IsNull(components) ) return FALSE;

	/* Identify component element */
	edef = identify_element(element);

	/* Return FALSE for unrecognized element */
	if ( IsNull(edef) ) return FALSE;

	/* Check that component type is not already in the list */
	for ( nn=0; nn<components->ncomp; nn++ )
		if ( comptype == components->comp_types[nn] ) break;

	/* Replace element info for component if already in the list */
	if ( nn < components->ncomp )
		{
		components->comp_edefs[nn] = edef;
		}

	/* Otherwise add to components list */
	else
		{
		components->ncomp++;
		components->comp_edefs = GETMEM(components->comp_edefs,
									FpaConfigElementStruct *, components->ncomp);
		components->comp_types = GETMEM(components->comp_types,
									COMPONENT,                components->ncomp);
		components->comp_edefs[components->ncomp - 1] = edef;
		components->comp_types[components->ncomp - 1] = comptype;
		}

	/* Return TRUE if all went OK */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   c o p y _ e l e m e n t                                            *
*   c o p y _ e l e m e n t _ d e t a i l                              *
*   c o p y _ e l e m e n t _ l i n e _ t y p e s                      *
*   c o p y _ e l e m e n t _ s c a t t e r e d _ t y p e s            *
*   c o p y _ e l e m e n t _ a t t r i b u t e s                      *
*   c o p y _ e l e m e n t _ e d i t o r                              *
*   c o p y _ e l e m e n t _ l a b e l l i n g                        *
*   c o p y _ e l e m e n t _ s a m p l i n g                          *
*   c o p y _ e l e m e n t _ l i n k i n g                            *
*   c o p y _ e l e m e n t _ e q u a t i o n                          *
*   c o p y _ e l e m e n t _ v a l c a l c                            *
*   c o p y _ e l e m e n t _ c o m p o n e n t s                      *
*                                                                      *
*   Return pointer to structure containing an exact copy of            *
*   information (or detailed information) read from Elements block of  *
*   configuration files.                                               *
*                                                                      *
***********************************************************************/

static	FpaConfigElementStruct	*copy_element

	(
	FpaConfigElementStruct	*edef		/* pointer to Element structure */
	)

	{
	FpaConfigElementStruct		*xdef;
	int							nn;

	/* Return Null if no structure passed */
	if ( IsNull(edef) ) return NullPtr(FpaConfigElementStruct *);

	/* Copy the ElementDefs structure */
	xdef                         = INITMEM(FpaConfigElementStruct, 1);
	xdef->name                   = strdup(SafeStr(edef->name));
	xdef->valid                  = edef->valid;
	xdef->nblocks                = edef->nblocks;
	if ( xdef->nblocks > 0 )
		{
		xdef->filenames = INITMEM(STRING,   xdef->nblocks);
		xdef->locations = INITMEM(long int, xdef->nblocks);
		for ( nn=0; nn<xdef->nblocks; nn++ )
			{
			xdef->filenames[nn]  = edef->filenames[nn];
			xdef->locations[nn]  = edef->locations[nn];
			}
		}
	else
		{
		xdef->filenames = NullStringList;
		xdef->locations = NullLong;
		}
	xdef->label                  = strdup(SafeStr(edef->label));
	xdef->sh_label               = strdup(SafeStr(edef->sh_label));
	xdef->description            = strdup(SafeStr(edef->description));
	xdef->group                  = edef->group;
	xdef->lvl_type               = edef->lvl_type;
	xdef->fld_group              = edef->fld_group;
	xdef->fld_type               = edef->fld_type;
	xdef->display_format         = edef->display_format;
	xdef->valid_detail           = edef->valid_detail;

	/* Copy the ElementIO structure in ElementDefs structure */
	xdef->elem_io                = INITMEM(FpaConfigElementIOStruct, 1);
	xdef->elem_io->fident        = strdup(SafeStr(edef->elem_io->fident));
	xdef->elem_io->fid           = strdup(SafeStr(edef->elem_io->fid));
	xdef->elem_io->precision     = edef->elem_io->precision;
	xdef->elem_io->units         = edef->elem_io->units;

	/* Copy the ElementTimeDep structure in ElementDefs structure */
	xdef->elem_tdep              = INITMEM(FpaConfigElementTimeDepStruct, 1);
	xdef->elem_tdep->time_dep    = edef->elem_tdep->time_dep;
	xdef->elem_tdep->normal_time = edef->elem_tdep->normal_time;
	xdef->elem_tdep->begin_time  = edef->elem_tdep->begin_time;
	xdef->elem_tdep->end_time    = edef->elem_tdep->end_time;
	xdef->elem_tdep->units       = edef->elem_tdep->units;

	/* Copy the ElementDetail structure in ElementDefs structure */
	xdef->elem_detail            = copy_element_detail(edef->fld_type,
														edef->elem_detail);

	/* Return pointer to copy of Element structure */
	return xdef;
	}

/**********************************************************************/

static	FpaConfigElementDetailStruct	*copy_element_detail

	(
	int								type,		/* enumerated field type */
	FpaConfigElementDetailStruct	*edetail	/* pointer to
													ElementDetail structure */
	)

	{
	FpaConfigElementDetailStruct	*xdetail;

	/* Return Null if no structure passed */
	if ( IsNull(edetail) ) return NullPtr(FpaConfigElementDetailStruct *);

	/* Copy the ElementDetail structure */
	xdetail                  = INITMEM(FpaConfigElementDetailStruct, 1);
	xdetail->wd_class        = edetail->wd_class;

	/* Copy the LineType structure in the ElementDetail structure */
	xdetail->line_types      = copy_element_line_types(edetail->line_types);

	/* Copy the ScatteredType structure in the ElementDetail structure */
	xdetail->scattered_types =
						copy_element_scattered_types(edetail->scattered_types);

	/* Copy the Attrib structure in the ElementDetail structure */
	xdetail->attributes      = copy_element_attributes(edetail->attributes);

	/* Copy the Editor structure in the ElementDetail structure */
	xdetail->editor          = copy_element_editor(type, edetail->editor);

	/* Copy the Labelling structure in the ElementDetail structure */
	xdetail->labelling       = copy_element_labelling(edetail->labelling);

	/* Copy the Sampling structure in the ElementDetail structure */
	xdetail->sampling        = copy_element_sampling(type, edetail->sampling);

	/* Copy the Linking structure in the ElementDetail structure */
	xdetail->linking         = copy_element_linking(type, edetail->linking);

	/* Copy the Equation structure in the ElementDetail structure */
	xdetail->equation        = copy_element_equation(edetail->equation);

	/* Copy the ValCalc structure in the ElementDetail structure */
	xdetail->valcalc         = copy_element_valcalc(edetail->valcalc);

	/* Copy the Component structure in the ElementDetail structure */
	xdetail->components      = copy_element_components(edetail->components);

	/* Return pointer to copy of ElementDetail structure */
	return xdetail;
	}

/**********************************************************************/

static	FpaConfigElementLineTypeStruct	*copy_element_line_types

	(
	FpaConfigElementLineTypeStruct	*ltypes	/* pointer to
												ElementLineType structure */
	)

	{
	int								nn, nx;
	FpaConfigElementLineTypeStruct	*xltypes;

	/* Return Null if no structure passed */
	if ( IsNull(ltypes) ) return NullPtr(FpaConfigElementLineTypeStruct *);

	/* Copy the ElementLineType structure */
	xltypes         = INITMEM(FpaConfigElementLineTypeStruct, 1);
	xltypes->ntypes = ltypes->ntypes;

	/* Copy the parameter lists in the ElementLineType structure */
	if ( xltypes->ntypes > 0 )
		{
		nx = xltypes->ntypes;
		xltypes->type_names     = INITMEM(STRING, nx);
		xltypes->type_labels    = INITMEM(STRING, nx);
		xltypes->type_sh_labels = INITMEM(STRING, nx);
		xltypes->patterns       = INITMEM(STRING, nx);

		for ( nn=0; nn<nx; nn++ )
			{
			xltypes->type_names[nn]     =
									strdup(SafeStr(ltypes->type_names[nn]));
			xltypes->type_labels[nn]    =
									strdup(SafeStr(ltypes->type_labels[nn]));
			xltypes->type_sh_labels[nn] =
									strdup(SafeStr(ltypes->type_sh_labels[nn]));
			xltypes->patterns[nn]       =
									strdup(SafeStr(ltypes->patterns[nn]));
			}
		}
	else
		{
		xltypes->type_names     = NullStringList;
		xltypes->type_labels    = NullStringList;
		xltypes->type_sh_labels = NullStringList;
		xltypes->patterns       = NullStringList;
		}

	/* Return pointer to copy of ElementLineType structure */
	return xltypes;
	}

/**********************************************************************/

static	FpaConfigElementScatteredTypeStruct	*copy_element_scattered_types

	(
	FpaConfigElementScatteredTypeStruct	*stypes	/* pointer to
													ElementScatteredType structure */
	)

	{
	int									nn, nx, mm, mx;
	FpaConfigElementScatteredTypeStruct	*xstypes;
	FpaConfigDefaultAttribStruct		sattrib;
	FpaConfigEntryRuleStruct			srule;

	/* Return Null if no structure passed */
	if ( IsNull(stypes) ) return NullPtr(FpaConfigElementScatteredTypeStruct *);

	/* Copy the ElementScatteredType structure */
	xstypes         = INITMEM(FpaConfigElementScatteredTypeStruct, 1);
	xstypes->check_scattered = stypes->check_scattered;
	xstypes->ntypes          = stypes->ntypes;

	/* Copy the parameter lists in the ElementScatteredType structure */
	if ( xstypes->ntypes > 0 )
		{
		nx = xstypes->ntypes;
		xstypes->type_names        = INITMEM(STRING,                       nx);
		xstypes->type_labels       = INITMEM(STRING,                       nx);
		xstypes->type_sh_labels    = INITMEM(STRING,                       nx);
		xstypes->type_classes      = INITMEM(STRING,                       nx);
		xstypes->type_entry_files  = INITMEM(STRING,                       nx);
		xstypes->type_modify_files = INITMEM(STRING,                       nx);
		xstypes->type_attach_opts  = INITMEM(FpaCattachOption,             nx);
		xstypes->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct, nx);
		xstypes->type_rules        = INITMEM(FpaConfigEntryRuleStruct,     nx);

		for ( nn=0; nn<nx; nn++ )
			{
			xstypes->type_names[nn]        =
								strdup(SafeStr(stypes->type_names[nn]));
			xstypes->type_labels[nn]       =
								strdup(SafeStr(stypes->type_labels[nn]));
			xstypes->type_sh_labels[nn]    =
								strdup(SafeStr(stypes->type_sh_labels[nn]));
			xstypes->type_classes[nn]      =
								strdup(SafeStr(stypes->type_classes[nn]));
			xstypes->type_entry_files[nn]  =
								strdup(SafeStr(stypes->type_entry_files[nn]));
			xstypes->type_modify_files[nn] =
								strdup(SafeStr(stypes->type_modify_files[nn]));
			xstypes->type_attach_opts[nn]  = stypes->type_attach_opts[nn];

			sattrib = stypes->type_attribs[nn];
			xstypes->type_attribs[nn].nattrib_defs = sattrib.nattrib_defs;
			if ( xstypes->type_attribs[nn].nattrib_defs > 0 )
				{
				mx = xstypes->type_attribs[nn].nattrib_defs;
				xstypes->type_attribs[nn].attrib_def_names  =
															INITMEM(STRING, mx);
				xstypes->type_attribs[nn].attrib_def_values =
															INITMEM(STRING, mx);
				for ( mm=0; mm<mx; mm++ )
					{
					xstypes->type_attribs[nn].attrib_def_names[mm]  =
								strdup(SafeStr(sattrib.attrib_def_names[mm]));
					xstypes->type_attribs[nn].attrib_def_values[mm] =
								strdup(SafeStr(sattrib.attrib_def_values[mm]));
					}
				}
			else
				{
				xstypes->type_attribs[nn].attrib_def_names  = NullStringList;
				xstypes->type_attribs[nn].attrib_def_values = NullStringList;
				}

			srule = stypes->type_rules[nn];
			xstypes->type_rules[nn].nrules = srule.nrules;
			if ( xstypes->type_rules[nn].nrules > 0 )
				{
				mx = xstypes->type_rules[nn].nrules;
				xstypes->type_rules[nn].entry_rules = INITMEM(STRING, mx);
				xstypes->type_rules[nn].entry_funcs = INITMEM(ERULE,  mx);
				for ( mm=0; mm<mx; mm++ )
					{
					xstypes->type_rules[nn].entry_rules[mm] =
										strdup(SafeStr(srule.entry_rules[mm]));
					xstypes->type_rules[nn].entry_funcs[mm] =
										srule.entry_funcs[mm];
					}
				}
			else
				{
				xstypes->type_rules[nn].entry_rules = NullStringList;
				xstypes->type_rules[nn].entry_funcs = NullEruleList;
				}
			}
		}
	else
		{
		xstypes->type_names        = NullStringList;
		xstypes->type_labels       = NullStringList;
		xstypes->type_sh_labels    = NullStringList;
		xstypes->type_classes      = NullStringList;
		xstypes->type_entry_files  = NullStringList;
		xstypes->type_modify_files = NullStringList;
		xstypes->type_attach_opts  = NullPtr(FpaCattachOption *);
		xstypes->type_attribs      = NullPtr(FpaConfigDefaultAttribStruct *);
		xstypes->type_rules        = NullPtr(FpaConfigEntryRuleStruct *);
		}

	/* Return pointer to copy of ElementScatteredType structure */
	return xstypes;
	}

/**********************************************************************/

static	FpaConfigElementAttribStruct	*copy_element_attributes

	(
	FpaConfigElementAttribStruct	*attrib		/* pointer to
													ElementAttrib structure */
	)

	{
	int								nn, nx;
	FpaConfigElementAttribStruct	*xattrib;

	/* Return Null if no structure passed */
	if ( IsNull(attrib) ) return NullPtr(FpaConfigElementAttribStruct *);

	/* Copy the ElementAttrib structure */
	xattrib           = INITMEM(FpaConfigElementAttribStruct, 1);
	xattrib->nattribs = attrib->nattribs;

	/* Copy the parameter lists in the ElementAttrib structure */
	if ( xattrib->nattribs > 0 )
		{
		nx = xattrib->nattribs;
		xattrib->attrib_names     = INITMEM(STRING, nx);
		xattrib->attrib_labels    = INITMEM(STRING, nx);
		xattrib->attrib_sh_labels = INITMEM(STRING, nx);
		xattrib->attrib_back_defs = INITMEM(STRING, nx);

		for ( nn=0; nn<nx; nn++ )
			{
			xattrib->attrib_names[nn]     =
				strdup(SafeStr(attrib->attrib_names[nn]));
			xattrib->attrib_labels[nn]    =
				strdup(SafeStr(attrib->attrib_labels[nn]));
			xattrib->attrib_sh_labels[nn] =
				strdup(SafeStr(attrib->attrib_sh_labels[nn]));
			xattrib->attrib_back_defs[nn] =
				strdup(SafeStr(attrib->attrib_back_defs[nn]));
			}
		}
	else
		{
		xattrib->attrib_names     = NullStringList;
		xattrib->attrib_labels    = NullStringList;
		xattrib->attrib_sh_labels = NullStringList;
		xattrib->attrib_back_defs = NullStringList;
		}

	/* Return pointer to copy of ElementAttrib structure */
	return xattrib;
	}

/**********************************************************************/

static	FpaConfigElementEditorStruct	*copy_element_editor

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	int								nn;
	FpaConfigElementEditorStruct	*xeditor;
	FpaConfigContinuousEditorStruct	*xcontinuous;
	FpaConfigVectorEditorStruct		*xvector;
	FpaConfigDiscreteEditorStruct	*xdiscrete;
	FpaConfigWindEditorStruct		*xwind;
	FpaConfigLineEditorStruct		*xline;
	FpaConfigScatteredEditorStruct	*xscattered;
	FpaConfigLchainEditorStruct		*xlchain;

	/* Return Null if no structure passed */
	if ( IsNull(editor) ) return NullPtr(FpaConfigElementEditorStruct *);

	/* Copy the ElementEditor structure */
	xeditor               = INITMEM(FpaConfigElementEditorStruct, 1);

	/* Reset the one time ElementEditor checking flag */
	xeditor->check_editor = FALSE;

	/* Copy editor information based on type of field */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			xeditor->type.continuous =
					INITMEM(FpaConfigContinuousEditorStruct, 1);
			xcontinuous = xeditor->type.continuous;

			xcontinuous->hilo             = editor->type.continuous->hilo;
			xcontinuous->poke             = editor->type.continuous->poke;
			xcontinuous->units            = editor->type.continuous->units;

			xcontinuous->entry_file       =
				strdup(SafeStr(editor->type.continuous->entry_file));
			xcontinuous->modify_file   =
				strdup(SafeStr(editor->type.continuous->modify_file));
			xcontinuous->memory_file      =
				strdup(SafeStr(editor->type.continuous->memory_file));
			xcontinuous->back_entry_file  =
				strdup(SafeStr(editor->type.continuous->back_entry_file));
			xcontinuous->back_memory_file =
				strdup(SafeStr(editor->type.continuous->back_memory_file));

			xcontinuous->nrules           = editor->type.continuous->nrules;
			if ( xcontinuous->nrules > 0 )
				{
				xcontinuous->entry_rules = INITMEM(STRING, xcontinuous->nrules);
				xcontinuous->entry_funcs = INITMEM(ERULE,  xcontinuous->nrules);
				for ( nn=0; nn<xcontinuous->nrules; nn++ )
					{
					xcontinuous->entry_rules[nn] =
						strdup(SafeStr(editor->type.continuous->entry_rules[nn]));
					xcontinuous->entry_funcs[nn] =
						editor->type.continuous->entry_funcs[nn];
					}
				}
			else
				{
				xcontinuous->entry_rules  = NullStringList;
				xcontinuous->entry_funcs  = NullEruleList;
				}

			xcontinuous->nmerge           = editor->type.continuous->nmerge;
			if ( xcontinuous->nmerge > 0 )
				{
				xcontinuous->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xcontinuous->nmerge);
				xcontinuous->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xcontinuous->nmerge);
				for ( nn=0; nn<xcontinuous->nmerge; nn++ )
					{
					xcontinuous->merge_elems[nn] =
						editor->type.continuous->merge_elems[nn];
					xcontinuous->merge_levels[nn] =
						editor->type.continuous->merge_levels[nn];
					}
				}
			else
				{
				xcontinuous->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xcontinuous->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			xeditor->type.vector =
					INITMEM(FpaConfigVectorEditorStruct, 1);
			xvector = xeditor->type.vector;

			xvector->hilo             = editor->type.vector->hilo;
			xvector->mag_poke         = editor->type.vector->mag_poke;
			xvector->mag_units        = editor->type.vector->mag_units;
			xvector->dir_poke         = editor->type.vector->dir_poke;
			xvector->dir_units        = editor->type.vector->dir_units;

			xvector->entry_file       =
				strdup(SafeStr(editor->type.vector->entry_file));
			xvector->modify_file   =
				strdup(SafeStr(editor->type.vector->modify_file));
			xvector->memory_file      =
				strdup(SafeStr(editor->type.vector->memory_file));
			xvector->back_entry_file  =
				strdup(SafeStr(editor->type.vector->back_entry_file));
			xvector->back_memory_file =
				strdup(SafeStr(editor->type.vector->back_memory_file));

			xvector->nrules           = editor->type.vector->nrules;
			if ( xvector->nrules > 0 )
				{
				xvector->entry_rules  = INITMEM(STRING, xvector->nrules);
				xvector->entry_funcs  = INITMEM(ERULE,  xvector->nrules);
				for ( nn=0; nn<xvector->nrules; nn++ )
					{
					xvector->entry_rules[nn] =
						strdup(SafeStr(editor->type.vector->entry_rules[nn]));
					xvector->entry_funcs[nn] =
						editor->type.vector->entry_funcs[nn];
					}
				}
			else
				{
				xvector->entry_rules  = NullStringList;
				xvector->entry_funcs  = NullEruleList;
				}

			xvector->nmerge           = editor->type.vector->nmerge;
			if ( xvector->nmerge > 0 )
				{
				xvector->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xvector->nmerge);
				xvector->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xvector->nmerge);
				for ( nn=0; nn<xvector->nmerge; nn++ )
					{
					xvector->merge_elems[nn] =
						editor->type.vector->merge_elems[nn];
					xvector->merge_levels[nn] =
						editor->type.vector->merge_levels[nn];
					}
				}
			else
				{
				xvector->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xvector->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			xeditor->type.discrete =
					INITMEM(FpaConfigDiscreteEditorStruct, 1);
			xdiscrete = xeditor->type.discrete;

			xdiscrete->overlaying       = editor->type.discrete->overlaying;
			xdiscrete->display_order    = editor->type.discrete->display_order;

			xdiscrete->entry_file       =
				strdup(SafeStr(editor->type.discrete->entry_file));
			xdiscrete->modify_file   =
				strdup(SafeStr(editor->type.discrete->modify_file));
			xdiscrete->memory_file      =
				strdup(SafeStr(editor->type.discrete->memory_file));
			xdiscrete->back_entry_file  =
				strdup(SafeStr(editor->type.discrete->back_entry_file));
			xdiscrete->back_memory_file =
				strdup(SafeStr(editor->type.discrete->back_memory_file));

			xdiscrete->nrules           = editor->type.discrete->nrules;
			if ( xdiscrete->nrules > 0 )
				{
				xdiscrete->entry_rules  = INITMEM(STRING, xdiscrete->nrules);
				xdiscrete->entry_funcs  = INITMEM(ERULE,  xdiscrete->nrules);
				for ( nn=0; nn<xdiscrete->nrules; nn++ )
					{
					xdiscrete->entry_rules[nn] =
						strdup(SafeStr(editor->type.discrete->entry_rules[nn]));
					xdiscrete->entry_funcs[nn] =
						editor->type.discrete->entry_funcs[nn];
					}
				}
			else
				{
				xdiscrete->entry_rules  = NullStringList;
				xdiscrete->entry_funcs  = NullEruleList;
				}

			xdiscrete->nmerge           = editor->type.discrete->nmerge;
			if ( xdiscrete->nmerge > 0 )
				{
				xdiscrete->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xdiscrete->nmerge);
				xdiscrete->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xdiscrete->nmerge);
				for ( nn=0; nn<xdiscrete->nmerge; nn++ )
					{
					xdiscrete->merge_elems[nn] =
						editor->type.discrete->merge_elems[nn];
					xdiscrete->merge_levels[nn] =
						editor->type.discrete->merge_levels[nn];
					}
				}
			else
				{
				xdiscrete->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xdiscrete->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Wind field type */
		case FpaC_WIND:
			xeditor->type.wind =
					INITMEM(FpaConfigWindEditorStruct, 1);
			xwind = xeditor->type.wind;

			xwind->display_order    = editor->type.wind->display_order;

			xwind->entry_file       =
				strdup(SafeStr(editor->type.wind->entry_file));
			xwind->modify_file   =
				strdup(SafeStr(editor->type.wind->modify_file));
			xwind->memory_file      =
				strdup(SafeStr(editor->type.wind->memory_file));
			xwind->back_entry_file  =
				strdup(SafeStr(editor->type.wind->back_entry_file));
			xwind->back_memory_file =
				strdup(SafeStr(editor->type.wind->back_memory_file));

			xwind->nrules           = editor->type.wind->nrules;
			if ( xwind->nrules > 0 )
				{
				xwind->entry_rules  = INITMEM(STRING, xwind->nrules);
				xwind->entry_funcs  = INITMEM(ERULE,  xwind->nrules);
				for ( nn=0; nn<xwind->nrules; nn++ )
					{
					xwind->entry_rules[nn] =
						strdup(SafeStr(editor->type.wind->entry_rules[nn]));
					xwind->entry_funcs[nn] =
						editor->type.wind->entry_funcs[nn];
					}
				}
			else
				{
				xwind->entry_rules  = NullStringList;
				xwind->entry_funcs  = NullEruleList;
				}

			xwind->nmerge           = editor->type.wind->nmerge;
			if ( xwind->nmerge > 0 )
				{
				xwind->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xwind->nmerge);
				xwind->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xwind->nmerge);
				for ( nn=0; nn<xwind->nmerge; nn++ )
					{
					xwind->merge_elems[nn] =
						editor->type.wind->merge_elems[nn];
					xwind->merge_levels[nn] =
						editor->type.wind->merge_levels[nn];
					}
				}
			else
				{
				xwind->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xwind->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Line field type */
		case FpaC_LINE:
			xeditor->type.line =
					INITMEM(FpaConfigLineEditorStruct, 1);
			xline = xeditor->type.line;

			xline->entry_file       =
				strdup(SafeStr(editor->type.line->entry_file));
			xline->modify_file   =
				strdup(SafeStr(editor->type.line->modify_file));
			xline->memory_file      =
				strdup(SafeStr(editor->type.line->memory_file));
			xline->back_entry_file  =
				strdup(SafeStr(editor->type.line->back_entry_file));
			xline->back_memory_file =
				strdup(SafeStr(editor->type.line->back_memory_file));

			xline->nrules           = editor->type.line->nrules;
			if ( xline->nrules > 0 )
				{
				xline->entry_rules  = INITMEM(STRING, xline->nrules);
				xline->entry_funcs  = INITMEM(ERULE,  xline->nrules);
				for ( nn=0; nn<xline->nrules; nn++ )
					{
					xline->entry_rules[nn] =
						strdup(SafeStr(editor->type.line->entry_rules[nn]));
					xline->entry_funcs[nn] =
						editor->type.line->entry_funcs[nn];
					}
				}
			else
				{
				xline->entry_rules  = NullStringList;
				xline->entry_funcs  = NullEruleList;
				}

			xline->nmerge           = editor->type.line->nmerge;
			if ( xline->nmerge > 0 )
				{
				xline->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xline->nmerge);
				xline->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xline->nmerge);
				for ( nn=0; nn<xline->nmerge; nn++ )
					{
					xline->merge_elems[nn] =
						editor->type.line->merge_elems[nn];
					xline->merge_levels[nn] =
						editor->type.line->merge_levels[nn];
					}
				}
			else
				{
				xline->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xline->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			xeditor->type.scattered =
					INITMEM(FpaConfigScatteredEditorStruct, 1);
			xscattered = xeditor->type.scattered;

			xscattered->entry_file       =
				strdup(SafeStr(editor->type.scattered->entry_file));
			xscattered->modify_file   =
				strdup(SafeStr(editor->type.scattered->modify_file));
			xscattered->memory_file      =
				strdup(SafeStr(editor->type.scattered->memory_file));
			xscattered->back_entry_file  =
				strdup(SafeStr(editor->type.scattered->back_entry_file));
			xscattered->back_memory_file =
				strdup(SafeStr(editor->type.scattered->back_memory_file));

			xscattered->nrules           = editor->type.scattered->nrules;
			if ( xscattered->nrules > 0 )
				{
				xscattered->entry_rules  = INITMEM(STRING, xscattered->nrules);
				xscattered->entry_funcs  = INITMEM(ERULE,  xscattered->nrules);
				for ( nn=0; nn<xscattered->nrules; nn++ )
					{
					xscattered->entry_rules[nn] =
						strdup(SafeStr(editor->type.scattered->entry_rules[nn]));
					xscattered->entry_funcs[nn] =
						editor->type.scattered->entry_funcs[nn];
					}
				}
			else
				{
				xscattered->entry_rules  = NullStringList;
				xscattered->entry_funcs  = NullEruleList;
				}

			xscattered->nmerge           = editor->type.scattered->nmerge;
			if ( xscattered->nmerge > 0 )
				{
				xscattered->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xscattered->nmerge);
				xscattered->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xscattered->nmerge);
				for ( nn=0; nn<xscattered->nmerge; nn++ )
					{
					xscattered->merge_elems[nn] =
						editor->type.scattered->merge_elems[nn];
					xscattered->merge_levels[nn] =
						editor->type.scattered->merge_levels[nn];
					}
				}
			else
				{
				xscattered->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xscattered->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			xeditor->type.lchain = INITMEM(FpaConfigLchainEditorStruct, 1);
			xlchain = xeditor->type.lchain;

			xlchain->entry_file       =
				strdup(SafeStr(editor->type.lchain->entry_file));
			xlchain->modify_file   =
				strdup(SafeStr(editor->type.lchain->modify_file));
			xlchain->memory_file      =
				strdup(SafeStr(editor->type.lchain->memory_file));
			xlchain->back_entry_file  =
				strdup(SafeStr(editor->type.lchain->back_entry_file));
			xlchain->back_memory_file =
				strdup(SafeStr(editor->type.lchain->back_memory_file));

			xlchain->nrules           = editor->type.lchain->nrules;
			if ( xlchain->nrules > 0 )
				{
				xlchain->entry_rules  = INITMEM(STRING, xlchain->nrules);
				xlchain->entry_funcs  = INITMEM(ERULE,  xlchain->nrules);
				for ( nn=0; nn<xlchain->nrules; nn++ )
					{
					xlchain->entry_rules[nn] =
						strdup(SafeStr(editor->type.lchain->entry_rules[nn]));
					xlchain->entry_funcs[nn] =
						editor->type.lchain->entry_funcs[nn];
					}
				}
			else
				{
				xlchain->entry_rules  = NullStringList;
				xlchain->entry_funcs  = NullEruleList;
				}

			xlchain->node_entry_file  =
				strdup(SafeStr(editor->type.lchain->node_entry_file));
			xlchain->node_modify_file =
				strdup(SafeStr(editor->type.lchain->node_modify_file));

			xlchain->nnode_rules      = editor->type.lchain->nnode_rules;
			if ( xlchain->nnode_rules > 0 )
				{
				xlchain->node_entry_rules  = INITMEM(STRING, xlchain->nnode_rules);
				xlchain->node_entry_funcs  = INITMEM(ERULE,  xlchain->nnode_rules);
				for ( nn=0; nn<xlchain->nnode_rules; nn++ )
					{
					xlchain->node_entry_rules[nn] =
						strdup(SafeStr(editor->type.lchain->node_entry_rules[nn]));
					xlchain->node_entry_funcs[nn] =
						editor->type.lchain->node_entry_funcs[nn];
					}
				}
			else
				{
				xlchain->node_entry_rules  = NullStringList;
				xlchain->node_entry_funcs  = NullEruleList;
				}

			xlchain->nmerge           = editor->type.lchain->nmerge;
			if ( xlchain->nmerge > 0 )
				{
				xlchain->merge_elems  = INITMEM(FpaConfigElementStruct *,
														xlchain->nmerge);
				xlchain->merge_levels = INITMEM(FpaConfigLevelStruct *,
														xlchain->nmerge);
				for ( nn=0; nn<xlchain->nmerge; nn++ )
					{
					xlchain->merge_elems[nn] =
						editor->type.lchain->merge_elems[nn];
					xlchain->merge_levels[nn] =
						editor->type.lchain->merge_levels[nn];
					}
				}
			else
				{
				xlchain->merge_elems  = NullPtr(FpaConfigElementStruct **);
				xlchain->merge_levels = NullPtr(FpaConfigLevelStruct **);
				}

			xlchain->nlink            = editor->type.lchain->nlink;
			if ( xlchain->nlink > 0 )
				{
				xlchain->link_elems  = INITMEM(FpaConfigElementStruct *,
														xlchain->nlink);
				xlchain->link_levels = INITMEM(FpaConfigLevelStruct *,
														xlchain->nlink);
				for ( nn=0; nn<xlchain->nlink; nn++ )
					{
					xlchain->link_elems[nn] =
						editor->type.lchain->link_elems[nn];
					xlchain->link_levels[nn] =
						editor->type.lchain->link_levels[nn];
					}
				}
			else
				{
				xlchain->link_elems  = NullPtr(FpaConfigElementStruct **);
				xlchain->link_levels = NullPtr(FpaConfigLevelStruct **);
				}
			xlchain->minterp          = editor->type.lchain->minterp;
			break;

		/* Unknown field types */
		default:
			xeditor->type.continuous = NullPtr(FpaConfigContinuousEditorStruct *);
			break;
		}

	/* Return pointer to copy of ElementEditor structure */
	return xeditor;
	}

/**********************************************************************/

static	FpaConfigElementLabellingStruct	*copy_element_labelling

	(
	FpaConfigElementLabellingStruct		*labelling	/* pointer to
														ElementLabelling structure */
	)

	{
	int									nn, nx, mm, mx;
	FpaConfigElementLabellingStruct		*xlabelling;
	FpaConfigDefaultAttribStruct		lattrib;
	FpaConfigEntryRuleStruct			lrule;

	/* Return Null if no structure passed */
	if ( IsNull(labelling) ) return NullPtr(FpaConfigElementLabellingStruct *);

	/* Copy the ElementLabelling structure */
	xlabelling                  = INITMEM(FpaConfigElementLabellingStruct, 1);
	xlabelling->check_labelling = FALSE;
	xlabelling->ntypes          = labelling->ntypes;

	/* Copy the labelling types in the ElementLabelling structure */
	if ( xlabelling->ntypes > 0 )
		{
		nx = xlabelling->ntypes;
		xlabelling->type_names        = INITMEM(STRING,                     nx);
		xlabelling->type_labels       = INITMEM(STRING,                     nx);
		xlabelling->type_sh_labels    = INITMEM(STRING,                     nx);
		xlabelling->type_classes      = INITMEM(STRING,                     nx);
		xlabelling->type_entry_files  = INITMEM(STRING,                     nx);
		xlabelling->type_modify_files = INITMEM(STRING,                     nx);
		xlabelling->type_attach_opts  = INITMEM(FpaCattachOption,           nx);
		xlabelling->type_attribs      = INITMEM(FpaConfigDefaultAttribStruct,
																			nx);
		xlabelling->type_rules        = INITMEM(FpaConfigEntryRuleStruct,   nx);

		for ( nn=0; nn<nx; nn++ )
			{
			xlabelling->type_names[nn]        =
				strdup(SafeStr(labelling->type_names[nn]));
			xlabelling->type_labels[nn]       =
				strdup(SafeStr(labelling->type_labels[nn]));
			xlabelling->type_sh_labels[nn]    =
				strdup(SafeStr(labelling->type_sh_labels[nn]));
			xlabelling->type_classes[nn]      =
				strdup(SafeStr(labelling->type_classes[nn]));
			xlabelling->type_entry_files[nn]  =
				strdup(SafeStr(labelling->type_entry_files[nn]));
			xlabelling->type_modify_files[nn] =
				strdup(SafeStr(labelling->type_modify_files[nn]));
			xlabelling->type_attach_opts[nn]  = labelling->type_attach_opts[nn];

			lattrib = labelling->type_attribs[nn];
			xlabelling->type_attribs[nn].nattrib_defs = lattrib.nattrib_defs;
			if ( xlabelling->type_attribs[nn].nattrib_defs > 0 )
				{
				mx = xlabelling->type_attribs[nn].nattrib_defs;
				xlabelling->type_attribs[nn].attrib_def_names  =
															INITMEM(STRING, mx);
				xlabelling->type_attribs[nn].attrib_def_values =
															INITMEM(STRING, mx);
				for ( mm=0; mm<mx; mm++ )
					{
					xlabelling->type_attribs[nn].attrib_def_names[mm]  =
							strdup(SafeStr(lattrib.attrib_def_names[mm]));
					xlabelling->type_attribs[nn].attrib_def_values[mm] =
							strdup(SafeStr(lattrib.attrib_def_values[mm]));
					}
				}
			else
				{
				xlabelling->type_attribs[nn].attrib_def_names  = NullStringList;
				xlabelling->type_attribs[nn].attrib_def_values = NullStringList;
				}
			lrule = labelling->type_rules[nn];
			xlabelling->type_rules[nn].nrules = lrule.nrules;
			if ( xlabelling->type_rules[nn].nrules > 0 )
				{
				mx = xlabelling->type_rules[nn].nrules;
				xlabelling->type_rules[nn].entry_rules = INITMEM(STRING, mx);
				xlabelling->type_rules[nn].entry_funcs = INITMEM(ERULE,  mx);
				for ( mm=0; mm<mx; mm++ )
					{
					xlabelling->type_rules[nn].entry_rules[mm] =
							strdup(SafeStr(lrule.entry_rules[mm]));
					xlabelling->type_rules[nn].entry_funcs[mm] =
							lrule.entry_funcs[mm];
					}
				}
			else
				{
				xlabelling->type_rules[nn].entry_rules = NullStringList;
				xlabelling->type_rules[nn].entry_funcs = NullEruleList;
				}
			}
		}
	else
		{
		xlabelling->type_names        = NullStringList;
		xlabelling->type_labels       = NullStringList;
		xlabelling->type_sh_labels    = NullStringList;
		xlabelling->type_classes      = NullStringList;
		xlabelling->type_entry_files  = NullStringList;
		xlabelling->type_modify_files = NullStringList;
		xlabelling->type_attach_opts  = NullPtr(FpaCattachOption *);
		xlabelling->type_attribs      = NullPtr(FpaConfigDefaultAttribStruct *);
		xlabelling->type_rules        = NullPtr(FpaConfigEntryRuleStruct *);
		}

	/* Return pointer to copy of ElementLabelling structure */
	return xlabelling;
	}

/**********************************************************************/

static	FpaConfigElementSamplingStruct	*copy_element_sampling

	(
	int								type,		/* enumerated field type */
	FpaConfigElementSamplingStruct	*sampling	/* pointer to
													ElementSampling structure */
	)

	{
	int									nn;
	FpaConfigElementSamplingStruct		*xsampling;
	FpaConfigContinuousSamplingStruct	*xcontinuous;
	FpaConfigVectorSamplingStruct		*xvector;
	FpaConfigDiscreteSamplingStruct		*xdiscrete;
	FpaConfigWindSamplingStruct			*xwind;
	FpaConfigLineSamplingStruct			*xline;
	FpaConfigScatteredSamplingStruct	*xscattered;
	FpaConfigLchainSamplingStruct		*xlchain;

	/* Return Null if no structure passed */
	if ( IsNull(sampling) ) return NullPtr(FpaConfigElementSamplingStruct *);

	/* Copy the ElementSampling structure */
	xsampling                 = INITMEM(FpaConfigElementSamplingStruct, 1);

	/* Reset the one time ElementSampling checking flag */
	xsampling->check_sampling = FALSE;

	/* Copy sampling information based on type of field */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			xsampling->type.continuous =
					INITMEM(FpaConfigContinuousSamplingStruct, 1);
			xcontinuous = xsampling->type.continuous;

			xcontinuous->nsample = sampling->type.continuous->nsample;
			if ( xcontinuous->nsample > 0 )
				{
				xcontinuous->samples = INITMEM(FpaConfigSampleStruct *,
														xcontinuous->nsample);
				for ( nn=0; nn<xcontinuous->nsample; nn++ )
					{
					xcontinuous->samples[nn] =
						sampling->type.continuous->samples[nn];
					}
				}
			else
				{
				xcontinuous->samples = NullPtr(FpaConfigSampleStruct **);
				}

			xcontinuous->nwindsamp = sampling->type.continuous->nwindsamp;
			if ( xcontinuous->nwindsamp > 0 )
				{
				xcontinuous->windsamps = INITMEM(FpaConfigSampleStruct *,
														xcontinuous->nwindsamp);
				for ( nn=0; nn<xcontinuous->nwindsamp; nn++ )
					{
					xcontinuous->windsamps[nn] =
						sampling->type.continuous->windsamps[nn];
					}
				}
			else
				{
				xcontinuous->windsamps = NullPtr(FpaConfigSampleStruct **);
				}

			break;

		/* Vector field type */
		case FpaC_VECTOR:
			xsampling->type.vector =
					INITMEM(FpaConfigVectorSamplingStruct, 1);
			xvector = xsampling->type.vector;

			xvector->nsample = sampling->type.vector->nsample;
			if ( xvector->nsample > 0 )
				{
				xvector->samples = INITMEM(FpaConfigSampleStruct *,
														xvector->nsample);
				for ( nn=0; nn<xvector->nsample; nn++ )
					{
					xvector->samples[nn] =
						sampling->type.vector->samples[nn];
					}
				}
			else
				{
				xvector->samples = NullPtr(FpaConfigSampleStruct **);
				}

			xvector->nwindsamp = sampling->type.vector->nwindsamp;
			if ( xvector->nwindsamp > 0 )
				{
				xvector->windsamps = INITMEM(FpaConfigSampleStruct *,
														xvector->nwindsamp);
				for ( nn=0; nn<xvector->nwindsamp; nn++ )
					{
					xvector->windsamps[nn] =
						sampling->type.vector->windsamps[nn];
					}
				}
			else
				{
				xvector->windsamps = NullPtr(FpaConfigSampleStruct **);
				}

			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			xsampling->type.discrete =
					INITMEM(FpaConfigDiscreteSamplingStruct, 1);
			xdiscrete = xsampling->type.discrete;

			xdiscrete->nsattribs = sampling->type.discrete->nsattribs;
			if ( xdiscrete->nsattribs > 0 )
				{
				xdiscrete->sattrib_names =
						INITMEM(STRING, xdiscrete->nsattribs);
				for ( nn=0; nn<xdiscrete->nsattribs; nn++ )
					{
					xdiscrete->sattrib_names[nn] =
							strdup(sampling->type.discrete->sattrib_names[nn]);
					}
				}
			else
				{
				xdiscrete->sattrib_names = NullStringPtr;
				}

			break;

		/* Wind field type */
		case FpaC_WIND:
			xsampling->type.wind = INITMEM(FpaConfigWindSamplingStruct, 1);
			xwind = xsampling->type.wind;

			xwind->nsample = sampling->type.wind->nsample;
			if ( xwind->nsample > 0 )
				{
				xwind->samples = INITMEM(FpaConfigSampleStruct *,
														xwind->nsample);
				for ( nn=0; nn<xwind->nsample; nn++ )
					{
					xwind->samples[nn] = sampling->type.wind->samples[nn];
					}
				}
			else
				{
				xwind->samples = NullPtr(FpaConfigSampleStruct **);
				}

			xwind->windsample = sampling->type.wind->windsample;

			xwind->nwcref = sampling->type.wind->nwcref;
			if ( xwind->nwcref > 0 )
				{
				xwind->wcrefs = INITMEM(FpaConfigCrossRefStruct *,
														xwind->nwcref);
				for ( nn=0; nn<xwind->nwcref; nn++ )
					{
					xwind->wcrefs[nn] = sampling->type.wind->wcrefs[nn];
					}
				}
			else
				{
				xwind->wcrefs = NullPtr(FpaConfigCrossRefStruct **);
				}

			break;

		/* Line field type */
		case FpaC_LINE:
			xsampling->type.line = INITMEM(FpaConfigLineSamplingStruct, 1);
			xline = xsampling->type.line;

			xline->nsattribs = sampling->type.line->nsattribs;
			if ( xline->nsattribs > 0 )
				{
				xline->sattrib_names = INITMEM(STRING, xline->nsattribs);
				for ( nn=0; nn<xline->nsattribs; nn++ )
					{
					xline->sattrib_names[nn] =
							strdup(sampling->type.line->sattrib_names[nn]);
					}
				}
			else
				{
				xline->sattrib_names = NullStringPtr;
				}

			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			xsampling->type.scattered = INITMEM(FpaConfigScatteredSamplingStruct, 1);
			xscattered = xsampling->type.scattered;

			xscattered->nsattribs = sampling->type.scattered->nsattribs;
			if ( xscattered->nsattribs > 0 )
				{
				xscattered->sattrib_names =
						INITMEM(STRING, xscattered->nsattribs);
				for ( nn=0; nn<xscattered->nsattribs; nn++ )
					{
					xscattered->sattrib_names[nn] =
							strdup(sampling->type.scattered->sattrib_names[nn]);
					}
				}
			else
				{
				xscattered->sattrib_names = NullStringPtr;
				}

			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			xsampling->type.lchain =
					INITMEM(FpaConfigLchainSamplingStruct, 1);
			xlchain = xsampling->type.lchain;

			xlchain->nsattribs = sampling->type.lchain->nsattribs;
			if ( xlchain->nsattribs > 0 )
				{
				xlchain->sattrib_names =
						INITMEM(STRING, xlchain->nsattribs);
				for ( nn=0; nn<xlchain->nsattribs; nn++ )
					{
					xlchain->sattrib_names[nn] =
							strdup(sampling->type.lchain->sattrib_names[nn]);
					}
				}
			else
				{
				xlchain->sattrib_names = NullStringPtr;
				}

			break;

		/* Unknown field types */
		default:
			xsampling->type.continuous = NullPtr(FpaConfigContinuousSamplingStruct *);
			break;
		}

	/* Return pointer to copy of ElementSampling structure */
	return xsampling;
	}

/**********************************************************************/

static	FpaConfigElementLinkingStruct	*copy_element_linking

	(
	int								type,		/* enumerated field type */
	FpaConfigElementLinkingStruct	*linking	/* pointer to
													ElementLinking structure */
	)

	{
	int									nn;
	FpaConfigElementLinkingStruct		*xlinking;

	/* Return Null if no structure passed */
	if ( IsNull(linking) ) return NullPtr(FpaConfigElementLinkingStruct *);

	/* Copy the ElementLinking structure */
	xlinking                = INITMEM(FpaConfigElementLinkingStruct, 1);

	/* Reset the one time ElementLinking checking flag */
	xlinking->check_linking = FALSE;

	/* Copy linking information based on type of field */
	switch ( type )
		{

		/* For the moment ... all field types have same structure */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:

			/* Copy the interpolation delta */
			xlinking->minterp       = linking->minterp;

			/* Copy the fields for linking */
			xlinking->nlink         = linking->nlink;
			if ( xlinking->nlink > 0 )
				{
				xlinking->link_elems  = INITMEM(FpaConfigElementStruct *,
														xlinking->nlink);
				xlinking->link_levels = INITMEM(FpaConfigLevelStruct *,
														xlinking->nlink);
				for ( nn=0; nn<xlinking->nlink; nn++ )
					{
					xlinking->link_elems[nn]  = linking->link_elems[nn];
					xlinking->link_levels[nn] = linking->link_levels[nn];
					}
				}
			else
				{
				xlinking->link_elems  = NullPtr(FpaConfigElementStruct **);
				xlinking->link_levels = NullPtr(FpaConfigLevelStruct **);
				}
			break;

		/* Unknown field types */
		default:
			break;
		}

	/* Return pointer to copy of ElementLinking structure */
	return xlinking;
	}

/**********************************************************************/

static	FpaConfigElementEquationStruct	*copy_element_equation

	(
	FpaConfigElementEquationStruct	*equation	/* pointer to
													ElementEquation structure */
	)

	{
	FpaConfigElementEquationStruct	*xequation;

	/* Return Null if no structure passed */
	if ( IsNull(equation) ) return NullPtr(FpaConfigElementEquationStruct *);

	/* Copy the ElementEquation structure */
	xequation        = INITMEM(FpaConfigElementEquationStruct, 1);
	xequation->force = equation->force;
	xequation->eqtn  = strdup(SafeStr(equation->eqtn));
	xequation->units = equation->units;

	/* Return pointer to copy of ElementEquation structure */
	return xequation;
	}

/**********************************************************************/

static	FpaConfigElementValCalcStruct	*copy_element_valcalc

	(
	FpaConfigElementValCalcStruct	*valcalc	/* pointer to
													ElementValCalc structure */
	)

	{
	int								nn;
	FpaConfigElementValCalcStruct	*xvalcalc;

	/* Return Null if no structure passed */
	if ( IsNull(valcalc) ) return NullPtr(FpaConfigElementValCalcStruct *);

	/* Copy the ElementValCalc structure */
	xvalcalc            = INITMEM(FpaConfigElementValCalcStruct, 1);
	xvalcalc->force     = valcalc->force;
	xvalcalc->vcalc     = valcalc->vcalc;
	xvalcalc->nsrc_type = valcalc->nsrc_type;
	if ( xvalcalc->nsrc_type > 0 )
		{
		xvalcalc->src_types = NullEnum;
		xvalcalc->src_types = GETMEM(xvalcalc->src_types, FpaCsourceTypeOption,
											xvalcalc->nsrc_type);
		/*
		xvalcalc->src_types = INITMEM(FpaCsourceTypeOption,
											xvalcalc->nsrc_type);
		*/
		for ( nn=0; nn<xvalcalc->nsrc_type; nn++ )
			{
			xvalcalc->src_types[nn] = valcalc->src_types[nn];
			}
		}
	else
		{
		xvalcalc->src_types = NullEnum;
		}

	/* Return pointer to copy of ElementValCalc structure */
	return xvalcalc;
	}

/**********************************************************************/

static	FpaConfigElementComponentStruct	*copy_element_components

	(
	FpaConfigElementComponentStruct	*components	/* pointer to
													ElementComponent structure */
	)

	{
	int								nn;
	FpaConfigElementComponentStruct	*xcomponents;

	/* Return Null if no structure passed */
	if ( IsNull(components) ) return NullPtr(FpaConfigElementComponentStruct *);

	/* Copy the ElementComponent structure */
	xcomponents            = INITMEM(FpaConfigElementComponentStruct, 1);
	xcomponents->ncomp = components->ncomp;
	if ( xcomponents->ncomp > 0 )
		{
		xcomponents->comp_edefs = INITMEM(FpaConfigElementStruct *,
												xcomponents->ncomp);
		xcomponents->comp_types = INITMEM(COMPONENT, xcomponents->ncomp);
		for ( nn=0; nn<xcomponents->ncomp; nn++ )
			{
			xcomponents->comp_edefs[nn] = components->comp_edefs[nn];
			xcomponents->comp_types[nn] = components->comp_types[nn];
			}
		}
	else
		{
		xcomponents->comp_edefs = NullPtr(FpaConfigElementStruct **);
		xcomponents->comp_types = NullPtr(COMPONENT *);
		}

	/* Return pointer to copy of ElementComponent structure */
	return xcomponents;
	}

/***********************************************************************
*                                                                      *
*   f r e e _ e l e m e n t _ l i n e _ t y p e s                      *
*   f r e e _ e l e m e n t _ s c a t t e r e d _ t y p e s            *
*   f r e e _ e l e m e n t _ t y p e _ r u l e s                      *
*   f r e e _ e l e m e n t _ t y p e _ a t t r i b s                  *
*   f r e e _ e l e m e n t _ a t t r i b u t e s                      *
*   f r e e _ e l e m e n t _ e d i t o r                              *
*   f r e e _ e l e m e n t _ e d i t o r _ r u l e s                  *
*   f r e e _ e l e m e n t _ e d i t o r _ n o d e _ r u l e s        *
*   f r e e _ e l e m e n t _ e d i t o r _ m e r g e _ f i e l d s    *
*   f r e e _ e l e m e n t _ e d i t o r _ l i n k _ f i e l d s      *
*   f r e e _ e l e m e n t _ l a b e l l i n g                        *
*   f r e e _ e l e m e n t _ l a b e l l i n g _ t y p e s            *
*   f r e e _ e l e m e n t _ s a m p l i n g                          *
*   f r e e _ e l e m e n t _ l i n k i n g                            *
*   f r e e _ e l e m e n t _ l i n k _ f i e l d s                    *
*   f r e e _ e l e m e n t _ e q u a t i o n                          *
*   f r e e _ e l e m e n t _ v a l c a l c                            *
*                                                                      *
*   Free memory of structures for detailed information read from       *
*   Elements block of configuration files.                             *
*                                                                      *
***********************************************************************/

static	FpaConfigElementLineTypeStruct	*free_element_line_types

	(
	FpaConfigElementLineTypeStruct	*ltypes	/* pointer to
												ElementLineType structure */
	)

	{

	/* Return Null if no structure passed */
	if ( IsNull(ltypes) ) return NullPtr(FpaConfigElementLineTypeStruct *);

	/* Free the parameter lists in the ElementLineType structure */
	if ( ltypes->ntypes > 0 )
		{
		FREELIST(ltypes->type_names,     ltypes->ntypes);
		FREELIST(ltypes->type_labels,    ltypes->ntypes);
		FREELIST(ltypes->type_sh_labels, ltypes->ntypes);
		FREELIST(ltypes->patterns,       ltypes->ntypes);
		ltypes->ntypes = 0;
		}

	/* Free the ElementLineType structure */
	FREEMEM(ltypes);
	return ltypes;
	}

/**********************************************************************/

static	FpaConfigElementScatteredTypeStruct	*free_element_scattered_types

	(
	FpaConfigElementScatteredTypeStruct	*stypes	/* pointer to
													ElementScatteredType structure */
	)

	{
	int								nn;

	/* Return Null if no structure passed */
	if ( IsNull(stypes) ) return NullPtr(FpaConfigElementScatteredTypeStruct *);

	/* Free the parameter lists in the ElementScatteredType structure */
	if ( stypes->ntypes > 0 )
		{
		FREELIST(stypes->type_names,        stypes->ntypes);
		FREELIST(stypes->type_labels,       stypes->ntypes);
		FREELIST(stypes->type_sh_labels,    stypes->ntypes);
		FREELIST(stypes->type_classes,      stypes->ntypes);
		FREELIST(stypes->type_entry_files,  stypes->ntypes);
		FREELIST(stypes->type_modify_files, stypes->ntypes);
		FREEMEM(stypes->type_attach_opts);
		for ( nn=0; nn<stypes->ntypes; nn++ )
			(void) free_element_type_attribs(&stypes->type_attribs[nn]);
		FREEMEM(stypes->type_attribs);
		for ( nn=0; nn<stypes->ntypes; nn++ )
			(void) free_element_type_rules(&stypes->type_rules[nn]);
		FREEMEM(stypes->type_rules);
		stypes->ntypes = 0;
		}

	/* Free the ElementScatteredType structure */
	FREEMEM(stypes);
	return stypes;
	}

/**********************************************************************/

static	void					free_element_type_attribs

	(
	FpaConfigDefaultAttribStruct	*lattrib	/* pointer to
													DefaultAttrib structure */
	)

	{

	/* Return if no structure passed */
	if ( IsNull(lattrib) ) return;

	/* Free the attribute default values in the DefaultAttrib structure */
	if ( lattrib->nattrib_defs > 0 )
		{
		FREELIST(lattrib->attrib_def_names,  lattrib->nattrib_defs);
		FREELIST(lattrib->attrib_def_values, lattrib->nattrib_defs);
		}

	/* Reset the number of attribute defaults */
	lattrib->nattrib_defs = 0;
	}

/**********************************************************************/

static	void					free_element_type_rules

	(
	FpaConfigEntryRuleStruct	*lrule	/* pointer to EntryRule structure */
	)

	{

	/* Return if no structure passed */
	if ( IsNull(lrule) ) return;

	/* Free the entry rules in the EntryRule structure */
	if ( lrule->nrules > 0 )
		{
		FREELIST(lrule->entry_rules,  lrule->nrules);
		}

	/* Free the entry rule function pointers in the structure */
	FREEMEM(lrule->entry_funcs);

	/* Reset the number of attribute defaults */
	lrule->nrules = 0;
	}

/**********************************************************************/

static	void					free_element_type_py_rules

	(
	FpaConfigEntryRuleStruct	*lrule	/* pointer to EntryRule structure */
	)

	{

	/* Return if no structure passed */
	if ( IsNull(lrule) ) return;

	/* Free the entry rules in the EntryRule structure */
	if ( lrule->py_nrules > 0 )
		{
		FREELIST(lrule->py_entry_rules,  lrule->py_nrules);
		}

	/* Reset the number of attribute defaults */
	lrule->py_nrules = 0;
	}
/**********************************************************************/

static	FpaConfigElementAttribStruct	*free_element_attributes

	(
	FpaConfigElementAttribStruct	*attrib		/* pointer to
													ElementAttrib structure */
	)

	{

	/* Return Null if no structure passed */
	if ( IsNull(attrib) ) return NullPtr(FpaConfigElementAttribStruct *);

	/* Free the parameter lists in the ElementAttrib structure */
	if ( attrib->nattribs > 0 )
		{
		FREELIST(attrib->attrib_names,     attrib->nattribs);
		FREELIST(attrib->attrib_labels,    attrib->nattribs);
		FREELIST(attrib->attrib_sh_labels, attrib->nattribs);
		FREELIST(attrib->attrib_back_defs, attrib->nattribs);
		attrib->nattribs = 0;
		}

	/* Free the ElementAttrib structure */
	FREEMEM(attrib);
	return attrib;
	}

/**********************************************************************/

static	FpaConfigElementEditorStruct	*free_element_editor

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return Null if no structure passed */
	if ( IsNull(editor) ) return NullPtr(FpaConfigElementEditorStruct *);

	/* Free the ElementEditor structure based on enumerated field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			FREEMEM(continuous->entry_file);
			FREEMEM(continuous->modify_file);
			FREEMEM(continuous->memory_file);
			FREEMEM(continuous->back_entry_file);
			FREEMEM(continuous->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_merge_fields(type, editor);
			FREEMEM(editor->type.continuous);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			FREEMEM(vector->entry_file);
			FREEMEM(vector->modify_file);
			FREEMEM(vector->memory_file);
			FREEMEM(vector->back_entry_file);
			FREEMEM(vector->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_merge_fields(type, editor);
			FREEMEM(editor->type.vector);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			FREEMEM(discrete->entry_file);
			FREEMEM(discrete->modify_file);
			FREEMEM(discrete->memory_file);
			FREEMEM(discrete->back_entry_file);
			FREEMEM(discrete->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_merge_fields(type, editor);
			FREEMEM(editor->type.discrete);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			FREEMEM(wind->entry_file);
			FREEMEM(wind->modify_file);
			FREEMEM(wind->memory_file);
			FREEMEM(wind->back_entry_file);
			FREEMEM(wind->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_merge_fields(type, editor);
			FREEMEM(editor->type.wind);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			FREEMEM(line->entry_file);
			FREEMEM(line->modify_file);
			FREEMEM(line->memory_file);
			FREEMEM(line->back_entry_file);
			FREEMEM(line->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_merge_fields(type, editor);
			FREEMEM(editor->type.line);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			FREEMEM(scattered->entry_file);
			FREEMEM(scattered->modify_file);
			FREEMEM(scattered->memory_file);
			FREEMEM(scattered->back_entry_file);
			FREEMEM(scattered->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_merge_fields(type, editor);
			FREEMEM(editor->type.scattered);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREEMEM(lchain->entry_file);
			FREEMEM(lchain->modify_file);
			FREEMEM(lchain->memory_file);
			FREEMEM(lchain->back_entry_file);
			FREEMEM(lchain->back_memory_file);
			(void) free_element_editor_rules(type, editor);
			(void) free_element_editor_py_rules(type, editor);
			(void) free_element_editor_node_rules(type, editor);
			FREEMEM(lchain->node_entry_file);
			FREEMEM(lchain->node_modify_file);
			(void) free_element_editor_merge_fields(type, editor);
			(void) free_element_editor_link_fields(type, editor);
			FREEMEM(editor->type.lchain);
			break;

		/* Unknown field types */
		default:
			break;
		}

	/* Free the ElementEditor structure */
	FREEMEM(editor);
	return editor;
	}

/**********************************************************************/

static	void							free_element_editor_rules

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return if no structure passed */
	if ( IsNull(editor) ) return;

	/* Free entry rules parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			FREELIST(continuous->entry_rules, continuous->nrules);
			FREEMEM(continuous->entry_funcs);
			continuous->nrules = 0;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			FREELIST(vector->entry_rules, vector->nrules);
			FREEMEM(vector->entry_funcs);
			vector->nrules = 0;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			FREELIST(discrete->entry_rules, discrete->nrules);
			FREEMEM(discrete->entry_funcs);
			discrete->nrules = 0;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			FREELIST(wind->entry_rules, wind->nrules);
			FREEMEM(wind->entry_funcs);
			wind->nrules = 0;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			FREELIST(line->entry_rules, line->nrules);
			FREEMEM(line->entry_funcs);
			line->nrules = 0;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			FREELIST(scattered->entry_rules, scattered->nrules);
			FREEMEM(scattered->entry_funcs);
			scattered->nrules = 0;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREELIST(lchain->entry_rules, lchain->nrules);
			FREEMEM(lchain->entry_funcs);
			lchain->nrules = 0;
			break;
		}
	}

/**********************************************************************/

static	void							free_element_editor_py_rules

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return if no structure passed */
	if ( IsNull(editor) ) return;

	/* Free entry rules parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			FREELIST(continuous->py_entry_rules, continuous->py_nrules);
			continuous->py_nrules = 0;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			FREELIST(vector->py_entry_rules, vector->py_nrules);
			vector->py_nrules = 0;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			FREELIST(discrete->py_entry_rules, discrete->py_nrules);
			discrete->py_nrules = 0;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			FREELIST(wind->py_entry_rules, wind->py_nrules);
			wind->py_nrules = 0;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			FREELIST(line->py_entry_rules, line->py_nrules);
			line->py_nrules = 0;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			FREELIST(scattered->py_entry_rules, scattered->py_nrules);
			scattered->py_nrules = 0;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREELIST(lchain->py_entry_rules, lchain->py_nrules);
			lchain->py_nrules = 0;
			break;
		}
	}

/**********************************************************************/

static	void							free_element_editor_node_rules

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigLchainEditorStruct			*lchain;

	/* Return if no structure passed */
	if ( IsNull(editor) ) return;

	/* Free link chain node rules parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREELIST(lchain->node_entry_rules, lchain->nnode_rules);
			FREEMEM(lchain->node_entry_funcs);
			lchain->nnode_rules = 0;
			break;
		}
	}

/**********************************************************************/

static	void							free_element_editor_py_node_rules

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigLchainEditorStruct			*lchain;

	/* Return if no structure passed */
	if ( IsNull(editor) ) return;

	/* Free link chain node rules parameters based on field type */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREELIST(lchain->py_node_entry_rules, lchain->py_nnode_rules);
			lchain->py_nnode_rules = 0;
			break;
		}
	}

/**********************************************************************/

static	void							free_element_editor_merge_fields

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigContinuousEditorStruct		*continuous;
	FpaConfigVectorEditorStruct			*vector;
	FpaConfigDiscreteEditorStruct		*discrete;
	FpaConfigWindEditorStruct			*wind;
	FpaConfigLineEditorStruct			*line;
	FpaConfigScatteredEditorStruct		*scattered;
	FpaConfigLchainEditorStruct			*lchain;

	/* Return if no structure passed */
	if ( IsNull(editor) ) return;

	/* Free merge field parameters based on field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = editor->type.continuous;
			FREEMEM(continuous->merge_elems);
			FREEMEM(continuous->merge_levels);
			continuous->nmerge = 0;
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = editor->type.vector;
			FREEMEM(vector->merge_elems);
			FREEMEM(vector->merge_levels);
			vector->nmerge = 0;
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = editor->type.discrete;
			FREEMEM(discrete->merge_elems);
			FREEMEM(discrete->merge_levels);
			discrete->nmerge = 0;
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = editor->type.wind;
			FREEMEM(wind->merge_elems);
			FREEMEM(wind->merge_levels);
			wind->nmerge = 0;
			break;

		/* Line field type */
		case FpaC_LINE:
			line = editor->type.line;
			FREEMEM(line->merge_elems);
			FREEMEM(line->merge_levels);
			line->nmerge = 0;
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = editor->type.scattered;
			FREEMEM(scattered->merge_elems);
			FREEMEM(scattered->merge_levels);
			scattered->nmerge = 0;
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREEMEM(lchain->merge_elems);
			FREEMEM(lchain->merge_levels);
			lchain->nmerge = 0;
			break;
		}
	}

/**********************************************************************/

static	void							free_element_editor_link_fields

	(
	int								type,		/* enumerated field type */
	FpaConfigElementEditorStruct	*editor		/* pointer to
													ElementEditor structure */
	)

	{
	FpaConfigLchainEditorStruct			*lchain;

	/* Return if no structure passed */
	if ( IsNull(editor) ) return;

	/* Free merge link field parameters based on field type */
	/* Note only link chain fields have merge link fields   */
	switch ( type )
		{

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = editor->type.lchain;
			FREEMEM(lchain->link_elems);
			FREEMEM(lchain->link_levels);
			lchain->nlink = 0;
			break;
		}
	}

/**********************************************************************/

static	FpaConfigElementLabellingStruct	*free_element_labelling

	(
	FpaConfigElementLabellingStruct		*labelling	/* pointer to
														ElementLabelling structure */
	)

	{

	/* Return Null if no structure passed */
	if ( IsNull(labelling) ) return NullPtr(FpaConfigElementLabellingStruct *);

	/* Free the labelling types in the ElementLabelling structure */
	(void) free_element_labelling_types(labelling);

	/* Free the ElementLabelling structure */
	FREEMEM(labelling);
	return labelling;
	}

/**********************************************************************/

static	void					free_element_labelling_types

	(
	FpaConfigElementLabellingStruct		*labelling	/* pointer to
														ElementLabelling structure */
	)

	{
	int								nn;

	/* Return if no structure passed */
	if ( IsNull(labelling) ) return;

	/* Free the labelling types in the ElementLabelling structure */
	if ( labelling->ntypes > 0 )
		{
		FREELIST(labelling->type_names,        labelling->ntypes);
		FREELIST(labelling->type_labels,       labelling->ntypes);
		FREELIST(labelling->type_sh_labels,    labelling->ntypes);
		FREELIST(labelling->type_classes,      labelling->ntypes);
		FREELIST(labelling->type_entry_files,  labelling->ntypes);
		FREELIST(labelling->type_modify_files, labelling->ntypes);
		FREEMEM(labelling->type_attach_opts);
		for ( nn=0; nn<labelling->ntypes; nn++ )
			(void) free_element_type_attribs(&labelling->type_attribs[nn]);
		FREEMEM(labelling->type_attribs);
		for ( nn=0; nn<labelling->ntypes; nn++ )
			(void) free_element_type_rules(&labelling->type_rules[nn]);
		FREEMEM(labelling->type_rules);
		labelling->ntypes = 0;
		}
	}

/**********************************************************************/

static	FpaConfigElementSamplingStruct	*free_element_sampling

	(
	int								type,		/* enumerated field type */
	FpaConfigElementSamplingStruct	*sampling	/* pointer to
													ElementSampling structure */
	)

	{
	FpaConfigContinuousSamplingStruct	*continuous;
	FpaConfigVectorSamplingStruct		*vector;
	FpaConfigDiscreteSamplingStruct		*discrete;
	FpaConfigWindSamplingStruct			*wind;
	FpaConfigLineSamplingStruct			*line;
	FpaConfigScatteredSamplingStruct	*scattered;
	FpaConfigLchainSamplingStruct		*lchain;

	/* Return Null if no structure passed */
	if ( IsNull(sampling) ) return NullPtr(FpaConfigElementSamplingStruct *);

	/* Free the ElementSampling structure based on enumerated field type */
	switch ( type )
		{

		/* Continuous field type */
		case FpaC_CONTINUOUS:
			continuous = sampling->type.continuous;
			FREEMEM(continuous->samples);
			FREEMEM(continuous->windsamps);
			FREEMEM(sampling->type.continuous);
			break;

		/* Vector field type */
		case FpaC_VECTOR:
			vector = sampling->type.vector;
			FREEMEM(vector->samples);
			FREEMEM(vector->windsamps);
			FREEMEM(sampling->type.vector);
			break;

		/* Discrete field type */
		case FpaC_DISCRETE:
			discrete = sampling->type.discrete;
			FREEMEM(discrete->sattrib_names);
			FREEMEM(sampling->type.discrete);
			break;

		/* Wind field type */
		case FpaC_WIND:
			wind = sampling->type.wind;
			FREEMEM(wind->samples);
			FREEMEM(wind->wcrefs);
			FREEMEM(sampling->type.wind);
			break;

		/* Line field type */
		case FpaC_LINE:
			line = sampling->type.line;
			FREEMEM(line->sattrib_names);
			FREEMEM(sampling->type.line);
			break;

		/* Scattered field type */
		case FpaC_SCATTERED:
			scattered = sampling->type.scattered;
			FREEMEM(scattered->sattrib_names);
			FREEMEM(sampling->type.scattered);
			break;

		/* Link chain field type */
		case FpaC_LCHAIN:
			lchain = sampling->type.lchain;
			FREEMEM(lchain->sattrib_names);
			FREEMEM(sampling->type.lchain);
			break;

		/* Unknown field types */
		default:
			break;
		}

	/* Free the ElementSampling structure */
	FREEMEM(sampling);
	return sampling;
	}

/**********************************************************************/

static	FpaConfigElementLinkingStruct	*free_element_linking

	(
	int								type,		/* enumerated field type */
	FpaConfigElementLinkingStruct	*linking	/* pointer to
													ElementLinking structure */
	)

	{

	/* Return Null if no structure passed */
	if ( IsNull(linking) ) return NullPtr(FpaConfigElementLinkingStruct *);

	/* Free the ElementLinking structure based on enumerated field type */
	switch ( type )
		{

		/* For the moment ... all field types have same structure */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
			FREEMEM(linking->link_elems);
			FREEMEM(linking->link_levels);
			linking->nlink = 0;
			break;

		/* Unknown field types */
		default:
			break;
		}

	/* Free the ElementLinking structure */
	FREEMEM(linking);
	return linking;
	}

/**********************************************************************/

static	void							free_element_link_fields

	(
	int								type,		/* enumerated field type */
	FpaConfigElementLinkingStruct	*linking	/* pointer to
													ElementLinking structure */
	)

	{

	/* Return if no structure passed */
	if ( IsNull(linking) ) return;

	/* Free link field parameters based on field type */
	switch ( type )
		{

		/* For the moment ... all field types have same structure */
		case FpaC_CONTINUOUS:
		case FpaC_VECTOR:
		case FpaC_DISCRETE:
		case FpaC_WIND:
		case FpaC_LINE:
		case FpaC_SCATTERED:
		case FpaC_LCHAIN:
			FREEMEM(linking->link_elems);
			FREEMEM(linking->link_levels);
			linking->nlink = 0;
			break;
		}
	}

/**********************************************************************/

static	FpaConfigElementEquationStruct	*free_element_equation

	(
	FpaConfigElementEquationStruct	*equation	/* pointer to
													ElementEquation structure */
	)

	{

	/* Return Null if no structure passed */
	if ( IsNull(equation) ) return NullPtr(FpaConfigElementEquationStruct *);

	/* Free the equation string in the ElementEquation structure */
	FREEMEM(equation->eqtn);

	/* Reset the units pointer in the ElementEquation structure */
	equation->units = NullPtr(FpaConfigUnitStruct *);

	/* Free the ElementEquation structure */
	FREEMEM(equation);
	return equation;
	}

/**********************************************************************/

static	FpaConfigElementValCalcStruct	*free_element_valcalc

	(
	FpaConfigElementValCalcStruct	*valcalc	/* pointer to
													ElementValCalc structure */
	)

	{

	/* Return Null if no structure passed */
	if ( IsNull(valcalc) ) return NullPtr(FpaConfigElementValCalcStruct *);

	/* Reset the crossref pointer in the ElementValCalc structure */
	valcalc->vcalc = NullPtr(FpaConfigCrossRefStruct *);

	/* Free the source types in the ElementValCalc structure */
	FREEMEM(valcalc->src_types);

	/* Free the ElementValCalc structure */
	FREEMEM(valcalc);
	return valcalc;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Fields block of configuration file)     *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ f i e l d s _ i n f o                                    *
*   r e a d _ f i e l d _ d e t a i l e d _ i n f o                    *
*                                                                      *
*   Read information from Fields block of configuration files.         *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_fields_info

	(
	)

	{
	FILE							*fpcfg;
	STRING							cline, cmd, arg;
	int								numbrace, section_id, section;
	LOGICAL							firstline;
	char							fldbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigFieldStruct			*fdef;
	FpaConfigElementStruct			*edef;
	FpaConfigLevelStruct			*ldef;

	/* Static buffers for element and level names for field */
	static	char	element[CONFIG_LABEL_LEN];
	static	char	level[CONFIG_LABEL_LEN];

	/* Read the configuration file(s) only once */
	if ( FieldsRead ) return FieldsValid;

	/* Force the Groups block of the configuration file to be read first */
	if ( !read_groups_info() ) return FieldsValid;

	/* Force the Levels block of the configuration file to be read next */
	if ( !read_levels_info() ) return FieldsValid;

	/* Force the Elements block of the configuration file to be read next */
	if ( !read_elements_info() ) return FieldsValid;

	/* Find and open the configuration file for the Fields block */
	if ( !first_config_file_open(FpaCfieldsFile, &fpcfg) )
		{
		FieldsRead = TRUE;
		return FieldsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Fields block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Fields block of configuration file */
		if ( same(cmd, FpaCblockFields) )
			{

			/* Set counter and identifier for Fields block */
			numbrace   = 0;
			section    = FpaCblockFieldsName;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Fields block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Fields block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Fields declaration */
				else if ( numbrace == 1 )
					{

					/* Adding another field in FpaCblockFieldsName section */
					if ( section_id == FpaCblockFieldsName )
						{

						/* Check for the named element */
						arg  = strdup(cmd);
						edef = identify_element(arg);
						(void) safe_strcpy(element, arg);
						FREEMEM(arg);

						/* Check for the named level */
						arg  = strdup_arg(cline);
						ldef = identify_level(arg);
						(void) safe_strcpy(level, arg);
						FREEMEM(arg);

						/* Error message if unrecognizable element or level */
						if ( IsNull(edef) || IsNull(ldef)
								|| !consistent_element_and_level(edef, ldef) )
							{
							(void) strcpy(fldbuf, element);
							(void) strcat(fldbuf, " ");
							(void) strcat(fldbuf, level);
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCblank, FpaCmsgField);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Check for field already in the list */
						fdef = find_field(edef, ldef);

						/* Add another field name to the lists   */
						/*  ... and add default group and labels */
						if ( IsNull(fdef) )
							{
							fdef = init_field(edef, ldef);
							(void) set_field_group_and_labels(fdef);
							}

						/* Set location of this block */
						(void) set_field_location(fpcfg, fdef);

						/* Set identifier for next section of Fields block */
						section = FpaCblockFieldsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockFields,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Fields declarations */
				/*  ... with format of "cmd = value(s)"  */
				else
					{

					/* Set field name for error messages */
					(void) strcpy(fldbuf, fdef->element->name);
					(void) strcat(fldbuf, " ");
					(void) strcat(fldbuf, fdef->level->name);

					/* Adding parameters in FpaCblockFieldsInfo section */
					if ( section_id == FpaCblockFieldsInfo )
						{

						/* Field label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									fdef->label = STRMEM(fdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								fdef->valid = FALSE;
								}
							}

						/* Field short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									fdef->sh_label =
											STRMEM(fdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								fdef->valid = FALSE;
								}
							}

						/* Field group */
						else if ( same(cmd, FpaCfieldGroup) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg         = strdup_arg(cline);
								fdef->group =
										identify_group(FpaCblockFields, arg);
								FREEMEM(arg);
								if ( IsNull(fdef->group) )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCfieldGroup, FpaCmsgParameter);
									fdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCfieldGroup, FpaCmsgNoEqual);
								fdef->valid = FALSE;
								}
							}

						/* Skip all keywords for detailed data (for now) */
						else if ( same(cmd, FpaCelementInfo)
								|| same(cmd, FpaClevelInfo) )
							{
							(void) skip_config_file_block(&fpcfg);
							}

						/* Set error flag for unrecognized Fields keyword */
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank, cmd, FpaCmsgKeyword);
							fdef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Set flags for completion of reading */
	FieldsRead  = TRUE;
	FieldsValid = TRUE;
	return FieldsValid;
	}

/**********************************************************************/

static	FpaConfigFieldStruct	*read_field_detailed_info

	(
	STRING		element,	/* field element name */
	STRING		level		/* field level name */
	)

	{
	int									nblk, nsub, natt, nlab;
	FILE								*fpcfg;
	STRING								cline, cmd, arg;
	int									fldtype, minterp;
	int									numbrace, section_id, section, macro;
	LOGICAL								firstline, valid, argok;
	char								fldbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementStruct				*edef;
	FpaConfigLevelStruct				*ldef;
	FpaConfigElementDetailStruct		*edetail;
	FpaConfigElementLineTypeStruct		*ltypes;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigDefaultAttribStruct		*sattrib;
	FpaConfigEntryRuleStruct			*srule;
	FpaConfigElementAttribStruct		*attrib;
	FpaConfigElementEditorStruct		*editor;
	FpaConfigContinuousEditorStruct		*editContinuous;
	FpaConfigVectorEditorStruct			*editVector;
	FpaConfigDiscreteEditorStruct		*editDiscrete;
	FpaConfigWindEditorStruct			*editWind;
	FpaConfigLineEditorStruct			*editLine;
	FpaConfigScatteredEditorStruct		*editScattered;
	FpaConfigLchainEditorStruct			*editLchain;
	FpaConfigElementLabellingStruct		*labelling;
	FpaConfigDefaultAttribStruct		*lattrib;
	FpaConfigEntryRuleStruct			*lrule;
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigContinuousSamplingStruct	*sampContinuous;
	FpaConfigVectorSamplingStruct		*sampVector;
	FpaConfigDiscreteSamplingStruct		*sampDiscrete;
	FpaConfigWindSamplingStruct			*sampWind;
	FpaConfigLineSamplingStruct			*sampLine;
	FpaConfigScatteredSamplingStruct	*sampScattered;
	FpaConfigLchainSamplingStruct		*sampLchain;
	FpaConfigElementLinkingStruct		*linking;
	FpaConfigElementEquationStruct		*equation;
	FpaConfigElementValCalcStruct		*valcalc;
	FpaConfigCrossRefStruct				*crdef;
	FpaConfigSampleStruct				*xdef;

	/* Find the pointer to the named field */
	fdef = identify_field(element, level);

	/* Return Null if field not found */
	if ( IsNull(fdef) ) return NullPtr(FpaConfigFieldStruct *);

	/* Return pointer to structure if detailed information has been read */
	if ( fdef->field_detail ) return fdef;

	/* Add space for detailed information */
	fdef->field_detail = TRUE;
	fdef->valid_detail = TRUE;

	/* Get detailed information for element */
	edef = get_element_info(element);
	if ( NotNull(edef) ) fdef->element = edef;

	/* Get information for level */
	ldef = identify_level(level);
	if ( NotNull(ldef) ) fdef->level   = ldef;

	/* Return now if problem with element or level information */
	if ( IsNull(edef) || IsNull(ldef) )
		{
		fdef->valid_detail = FALSE;
		return fdef;
		}

	/* Return now if this is a created field                           */
	/*  ... since there will be no information in configuration files! */
	if ( fdef->created_field ) return fdef;

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Fields block for \"%s %s\"!\n",
			element, level);

	/* Set field name for error messages */
	(void) strcpy(fldbuf, fdef->element->name);
	(void) strcat(fldbuf, " ");
	(void) strcat(fldbuf, fdef->level->name);

	/* Set field type */
	fldtype = fdef->element->fld_type;

	/* Read all blocks from configuration files for this Fields declaration */
	for ( nblk=0; nblk<fdef->nblocks; nblk++ )
		{

		/* Re-open and position configuration file for this Fields block */
		fpcfg = NullPtr(FILE *);
		if ( !config_file_open(fdef->filenames[nblk], &fpcfg)
				|| fseek(fpcfg, fdef->locations[nblk], SEEK_SET) != 0 )
			{
			return NullPtr(FpaConfigFieldStruct *);
			}

		/* Diagnostic message */
		(void) pr_diag("Config", "  ... beginning at \"%d\" in file \"%s\"\n",
				fdef->locations[nblk], fdef->filenames[nblk]);

		/* Set counter and identifier for Fields declaration in Fields block */
		numbrace   = 0;
		section    = FpaCblockFieldsInfo;
		section_id = FpaCnoSection;
		firstline  = TRUE;

		/* Read block containing this Fields declaration line by line */
		while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
			{

			/* Extract the first argument from the current line */
			cmd = string_arg(cline);

			/* The first line should be an open bracket */
			if ( firstline )
				{
				firstline = FALSE;
				if ( !same(cmd, FpaCopenBrace) ) break;
				}

			/* Increment counter for open brackets */
			/*  and save the section identifier    */
			if ( same(cmd, FpaCopenBrace) )
				{
				numbrace++;
				section_id = push_section(section);
				}

			/* Decrement counter for close brackets */
			/*  and reset the section identifier    */
			else if ( same(cmd, FpaCcloseBrace) )
				{
				numbrace--;
				section_id = pop_section();

				/* Check for end of Fields block */
				if ( numbrace == 0 ) break;
				}

			/* Set parameters in Fields declaration */
			/*  ... with format of "cmd = value(s)" */
			else if ( numbrace == 1 )
				{

				/* Adding parameters in FpaCblockFieldsInfo section */
				if ( section_id == FpaCblockFieldsInfo )
					{

					/* Element override */
					if ( same(cmd, FpaCelementInfo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set element override */
							if ( blank(arg) )
								{
								/* Copy the complete Element structure */
								/*  ... if not already copied!         */
								if ( !fdef->override_element )
									{
									fdef->element = copy_element(edef);
									fdef->override_element = TRUE;
									}
								edetail = fdef->element->elem_detail;
								section = FpaCblockFieldsElement;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCdefault) )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCelementInfo, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}

								/* But cannot reset override to Default! */
								else if ( fdef->override_element )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCelementInfo,
											FpaCmsgResetDefault);
									}

								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCelementInfo, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Level override */
					else if ( same(cmd, FpaClevelInfo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set level override */
							if ( blank(arg) )
								{
								/* >>> Level override is not <<< */
								/* >>> presently supported!  <<< */
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClevelInfo, FpaCmsgSupport);
								(void) skip_config_file_block(&fpcfg);
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCdefault) )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaClevelInfo, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}

								/* But cannot reset override to Default! */
								else if ( fdef->override_level )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaClevelInfo, FpaCmsgResetDefault);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClevelInfo, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Skip all other keywords */
					else
						{
						(void) skip_config_file_block(&fpcfg);
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCblank, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Fields declaration */
			/*  ... with format of "cmd = value(s)"           */
			else if ( numbrace == 2 )
				{

				/* Adding parameters in FpaCblockFieldsElement section */
				if ( section_id == FpaCblockFieldsElement )
					{

					/* Element line types block reset */
					if ( same(cmd, FpaClineTypesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements line types block */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->line_types =
									free_element_line_types(edetail->line_types);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClineTypesReset, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClineTypesReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Element subelement block reset */
					else if ( same(cmd, FpaCsubelementsReset) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClineTypesReset,
								FpaCsubelementsReset, FpaCmsgReplace);
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements line types block */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->line_types =
									free_element_line_types(edetail->line_types);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsubelementsReset, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsubelementsReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}
				/* >>> the preceding is obsolete in next version <<< */


					/* Element line types block */
					else if ( same(cmd, FpaClineTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize line types block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->line_types) )
									{
									edetail->line_types =
											init_element_line_types();
									}
								ltypes  = edetail->line_types;
								section = FpaCblockFieldsLineTypes;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty line types block */
									edetail->line_types =
										free_element_line_types(edetail->line_types);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaClineTypes, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClineTypes, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Element subelements block */
					else if ( same(cmd, FpaCsubelements) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClineTypes,
								FpaCsubelements, FpaCmsgReplace);
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize line types block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->line_types) )
									{
									edetail->line_types =
											init_element_line_types();
									}
								ltypes  = edetail->line_types;
								section = FpaCblockFieldsLineTypes;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty line types block */
									edetail->line_types =
										free_element_line_types(edetail->line_types);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCsubelements, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsubelements, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Element scattered types block reset */
					else if ( same(cmd, FpaCscatteredTypesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements scattered types block */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->scattered_types =
									free_element_scattered_types(edetail->scattered_types);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Element scattered types block */
					else if ( same(cmd, FpaCscatteredTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize scattered types block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->scattered_types) )
									{
									edetail->scattered_types =
											init_element_scattered_types();
									}
								stypes  = edetail->scattered_types;
								section = FpaCblockFieldsScatteredTypes;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty scattered types block */
									edetail->scattered_types =
										free_element_scattered_types(edetail->scattered_types);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCscatteredTypes,
											FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypes, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field element attributes block reset */
					else if ( same(cmd, FpaCattributesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset Elements attributes block           */
							/*  ... and reinitialize default attributes! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->attributes =
									free_element_attributes(edetail->attributes);
								edetail->attributes =
									init_element_attributes(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCattributesReset, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCattributesReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field element attributes block */
					else if ( same(cmd, FpaCattributes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for attributes */
							if ( blank(arg) )
								{
								attrib  = edetail->attributes;
								section = FpaCblockFieldsAttributes;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set attributes block with default values */
									edetail->attributes =
										free_element_attributes(edetail->attributes);
									edetail->attributes =
										init_element_attributes(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCattributes, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCattributes, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field element editor block */
					else if ( same(cmd, FpaCeditor) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize editor block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->editor) )
									{
									edetail->editor =
											init_element_editor(fldtype);
									}
								if ( IsNull(edetail->editor) )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCeditor, FpaCmsgSupport);
									(void) skip_config_file_block(&fpcfg);
									}
								editor  = edetail->editor;
								section = FpaCblockFieldsEditor;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Empty editor block */
									edetail->editor =
											free_element_editor(fldtype,
													edetail->editor);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCeditor, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditor, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field labelling block reset */
					else if ( same(cmd, FpaClabellingReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling block                    */
							/*  ... and reinitialize default labelling! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->labelling =
										free_element_labelling(edetail->labelling);
								edetail->labelling =
										init_element_labelling(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClabellingReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field labelling block */
					else if ( same(cmd, FpaClabelling) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize labelling block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->labelling) )
									{
									edetail->labelling =
											init_element_labelling(fldtype);
									}
								labelling = edetail->labelling;
								section   = FpaCblockFieldsLabelling;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set labelling block with default values */
									edetail->labelling =
											free_element_labelling(edetail->labelling);
									edetail->labelling =
											init_element_labelling(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaClabelling, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabelling, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field sampling block reset */
					else if ( same(cmd, FpaCsamplingReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset sampling block                    */
							/*  ... and reinitialize default sampling! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->sampling =
										free_element_sampling(fldtype,
												edetail->sampling);
								edetail->sampling =
										init_element_sampling(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingReset, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is not quite ready yet! <<< */
					/* Field sampling block */
					else if ( same(cmd, FpaCsampling) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCsampling, FpaCmsgSupport);
						(void) pr_warning("Config",
								"     Use keyword \"%s\" for now!\n",
								FpaCsample);
						(void) skip_config_file_block(&fpcfg);
						}
				/* >>> the preceding is not quite ready yet! <<< */

					/* Field sample block */
					else if ( same(cmd, FpaCsample) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for sampling */
							if ( blank(arg) )
								{
								sampling = edetail->sampling;
								section  = FpaCblockFieldsSampling;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set sampling block with default values */
									edetail->sampling =
											free_element_sampling(fldtype,
													edetail->sampling);
									edetail->sampling =
											init_element_sampling(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCsample, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsample, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field linking block reset */
					else if ( same(cmd, FpaClinkingReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset linking block                    */
							/*  ... and reinitialize default linking! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								edetail->linking =
										free_element_linking(fldtype,
												edetail->linking);
								edetail->linking =
										init_element_linking(fldtype);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClinkingReset, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClinkingReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field linking block */
					else if ( same(cmd, FpaClinking) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for linking */
							if ( blank(arg) )
								{
								linking = edetail->linking;
								section = FpaCblockFieldsLinking;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set linking block with default values */
									edetail->linking =
											free_element_linking(fldtype,
													edetail->linking);
									edetail->linking =
											init_element_linking(fldtype);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaClinking, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClinking, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field equation block */
					else if ( same(cmd, FpaCequation) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize equation block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->equation) )
									{
									edetail->equation =
											init_element_equation();
									}
								equation = edetail->equation;
								section  = FpaCblockFieldsEquation;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCequation, FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}

								/* Remove an existing equation if requested! */
								else if ( NotNull(edetail->equation) )
									{
									edetail->equation =
										free_element_equation(edetail->equation);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCequation, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field value calculation block */
					else if ( same(cmd, FpaCvalueCalculation) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Initialize value calculation block */
							if ( blank(arg) )
								{
								if ( IsNull(edetail->valcalc) )
									{
									edetail->valcalc =
											init_element_valcalc();
									}
								valcalc = edetail->valcalc;
								section = FpaCblockFieldsValCalc;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( !same_ic(arg, FpaCnone) )
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCvalueCalculation,
											FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}

								/* Remove an existing value calculation */
								/*  if requested!                       */
								else if ( NotNull(edetail->valcalc) )
									{
									edetail->valcalc =
										free_element_valcalc(edetail->valcalc);
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCvalueCalculation, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Skip all other keywords */
					else
						{
						(void) skip_config_file_block(&fpcfg);
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCblank, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Fields declaration */
			/*  ... with format of "cmd = value(s)"           */
			else if ( numbrace == 3 )
				{

				/* Adding parameters in FpaCblockFieldsLineTypes section */
				if ( section_id == FpaCblockFieldsLineTypes )
					{

					/* Add another line type name (if not already there!) */
					nsub = add_element_line_type(cmd, ltypes);
					if ( nsub < 0 )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClineTypes,
								cmd, FpaCmsgMember);
						fdef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockFieldsLineTypesSet;
					}

				/* Adding parameters in FpaCblockFieldsScatteredTypes section */
				else if ( section_id == FpaCblockFieldsScatteredTypes )
					{

					/* Add another scattered type name (if not already there!) */
					nsub = add_element_scattered_type(cmd, stypes);
					if ( nsub < 0 )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCscatteredTypes,
								cmd, FpaCmsgMember);
						fdef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockFieldsScatteredTypesSet;
					}

				/* Adding parameters in FpaCblockFieldsAttributes section */
				else if ( section_id == FpaCblockFieldsAttributes )
					{

					/* Add another attribute name (if not already there!) */
					natt = add_element_attribute(cmd, cline, attrib);
					if ( natt < 0 )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCattributes,
								cmd, FpaCmsgMember);
						fdef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockFieldsAttribsSet;
					}

				/* Adding parameters in FpaCblockFieldsEditor section */
				else if ( section_id == FpaCblockFieldsEditor )
					{

					/* First add the parameters common to all fields */

					/* Editor entry file */
					if ( same(cmd, FpaCeditorEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_entry_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorEntryFile, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorEntryFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor modify file */
					else if ( same(cmd, FpaCeditorModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_modify_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorModifyFile, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorModifyFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor memory file */
					else if ( same(cmd, FpaCeditorMemoryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_memory_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorMemoryFile, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorMemoryFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor background entry file */
					else if ( same(cmd, FpaCeditorBackEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_back_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorBackEntryFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorBackEntryFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor background memory file */
					else if ( same(cmd, FpaCeditorBackMemoryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_back_mem_file(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorBackMemoryFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorBackMemoryFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor entry rules reset */
					else if ( same(cmd, FpaCeditorEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorEntryRulesReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor entry rules */
					else if ( same(cmd, FpaCeditorEntryRules) )
						{
						/* Add editor entry rules to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_editor_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorEntryRules, FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorEntryRules, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor python entry rules reset */
					else if ( same(cmd, FpaCeditorPyEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_py_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPyEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorPyEntryRulesReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor python entry rules */
					else if ( same(cmd, FpaCeditorPyEntryRules) )
						{
						/* Add editor entry rules to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_editor_py_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPyEntryRules, FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorPyEntryRules, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor merge fields reset */
					else if ( same(cmd, FpaCeditorMergeFieldsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor merge fields list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_merge_fields(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorMergeFieldsReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorMergeFieldsReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Editor merge fields */
					else if ( same(cmd, FpaCeditorMergeFields) )
						{
						/* Add editor merge fields to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_editor_merge_fields(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorMergeFields,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorMergeFields, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Then add the parameters for particular fields */

					/* Continuous field type ... hi/lo */
					else if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCeditorHilo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editContinuous = editor->type.continuous;
							editContinuous->hilo =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorHilo, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorHilo, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Continuous field type ... poke */
					else if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCeditorPoke) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editContinuous = editor->type.continuous;
							editContinuous->poke  =
									double_arg(cline, &valid);
							arg                   = strdup_arg(cline);
							editContinuous->units = identify_unit(arg);
							FREEMEM(arg);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPokeVal, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							if ( IsNull(editContinuous->units) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPokeUnit, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorPoke, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... hi/lo */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCeditorHilo) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editVector = editor->type.vector;
							editVector->hilo =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorHilo, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorHilo, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... poke magnitude */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCeditorMagnitudePoke) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editVector = editor->type.vector;
							editVector->mag_poke  =
									double_arg(cline, &valid);
							arg                   = strdup_arg(cline);
							editVector->mag_units = identify_unit(arg);
							FREEMEM(arg);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPokeVal, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							if ( IsNull(editVector->mag_units) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPokeUnit, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorMagnitudePoke, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... poke direction */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCeditorDirectionPoke) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editVector = editor->type.vector;
							editVector->dir_poke  =
									double_arg(cline, &valid);
							arg                   = strdup_arg(cline);
							editVector->dir_units = identify_unit(arg);
							FREEMEM(arg);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPokeVal, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							if ( IsNull(editVector->dir_units) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPokeUnit, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorDirectionPoke, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... subelement list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorSubelementList) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorSubelementList, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... background list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorBackgroundList) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorBackgroundList, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... background */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorBackground) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorBackground, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Discrete field type ... overlaying */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorOverlaying) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editDiscrete = editor->type.discrete;
							editDiscrete->overlaying =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorOverlaying, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorOverlaying, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Discrete or Wind field types ... display ordering */
					else if ( ( fldtype == FpaC_DISCRETE
								|| fldtype == FpaC_WIND )
							&& same(cmd, FpaCeditorDisplayOrder) )

						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							editDiscrete = editor->type.discrete;
							editDiscrete->display_order =
									logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorDisplayOrder,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorDisplayOrder, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node entry file */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_node_entry_file(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorNodeEntryFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorNodeEntryFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node modify file */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_editor_node_modify_file(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorNodeModifyFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorNodeModifyFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node entry rules reset */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset node entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_node_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorNodeEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorNodeEntryRulesReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... python node entry rules reset */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorPyNodeEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset node entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_py_node_rules(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPyNodeEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorPyNodeEntryRulesReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... node entry rules */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorNodeEntryRules) )
						{

						/* Add node entry rules to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_editor_node_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorNodeEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorNodeEntryRules, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... python node entry rules */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorPyNodeEntryRules) )
						{

						/* Add node entry rules to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_editor_py_node_rules(cline, fldtype,
																	editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorPyNodeEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorPyNodeEntryRules, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... editor interpolation delta */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorLchainInterpDelta) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							minterp = interpret_hour_minute_string(cline);
							if ( minterp > 0 )
								{
								editLchain = editor->type.lchain;
								editLchain->minterp = minterp;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorLchainInterpDelta,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorLchainInterpDelta, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... editor merge link fields reset */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorLinkFieldsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset editor merge link fields list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_editor_link_fields(fldtype,
																	editor);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCeditorLinkFieldsReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCeditorLinkFieldsReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... editor merge link fields */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCeditorLinkFields) )
						{
						/* Add editor merge link fields to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_editor_link_fields(cline,
															fldtype, editor) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank, FpaCeditorLinkFields,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank, FpaCeditorLinkFields,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... menu */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorMenu) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorMenu, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... menu file */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorMenuFile) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorMenuFile, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... menu memory */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCeditorMenuMemory) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorMenuMemory, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Wind field type ... wind cross reference list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCeditorWindList) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorWindList, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Wind field type ... background list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCeditorWindBackgroundList) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorWindBackgroundList, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Wind field type ... background */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCeditorWindBackground) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorWindBackground, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Line field type ... subelement_reset section */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCeditorSubelementReset) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorSubelementReset, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Line field type ... subelement section */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCeditorSubelement) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCeditorSubelement, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCeditor, cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsLabelling section */
				else if ( section_id == FpaCblockFieldsLabelling )
					{

					/* Labelling types reset */
					if ( same(cmd, FpaClabellingTypesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types list                     */
							/*  ... and reinitialize default labelling types! */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_labelling_types(labelling);
								(void) init_element_labelling_types(fldtype,
																	labelling);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClabellingTypesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingTypesReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling types */
					else if ( same(cmd, FpaClabellingTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for labelling types */
							if ( blank(arg) )
								{
								section = FpaCblockFieldsLabelTypes;
								}

							/* Otherwise only FpaCdefault is acceptable */
							else
								{
								if ( same_ic(arg, FpaCdefault) )
									{
									/* Set labelling types with default values */
									(void) free_element_labelling_types(labelling);
									(void) init_element_labelling_types(fldtype,
																	labelling);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaClabellingTypes,
											FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingTypes, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClabelling,
								cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsSampling section */
				else if ( section_id == FpaCblockFieldsSampling )
					{

					/* Continuous field type ... value type list */
					if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						/* Set continuous sampling value type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampContinuous = sampling->type.continuous;
							if ( !add_continuous_sample_values(cline,
									sampContinuous) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingValueSampleTypes,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingValueSampleTypes,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Continuous field type ... wind type list */
					else if ( fldtype == FpaC_CONTINUOUS
							&& same(cmd, FpaCsamplingWindSampleTypes) )
						{
						/* Set continuous sampling wind type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampContinuous = sampling->type.continuous;
							if ( !add_continuous_sample_winds(cline,
									sampContinuous) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingWindSampleTypes,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingWindSampleTypes,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... value type list */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						/* Set vector sampling value type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampVector = sampling->type.vector;
							if ( !add_vector_sample_values(cline,
									sampVector) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingValueSampleTypes,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingValueSampleTypes,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Vector field type ... wind type list */
					else if ( fldtype == FpaC_VECTOR
							&& same(cmd, FpaCsamplingWindSampleTypes) )
						{
						/* Set vector sampling wind type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampVector = sampling->type.vector;
							if ( !add_vector_sample_winds(cline,
									sampVector) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingWindSampleTypes,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingWindSampleTypes,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Discrete field type ... attribute list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set discrete sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampDiscrete = sampling->type.discrete;
							if ( !add_discrete_sample_attribs(cline,
									sampDiscrete) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Wind field type ... value type list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						/* Set wind sampling value type list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampWind = sampling->type.wind;
							if ( !add_wind_sample_values(cline, sampWind) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingValueSampleTypes,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingValueSampleTypes,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Wind field type ... wind cross reference list */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCsamplingWindCrossRefs) )
						{
						/* Set wind sampling wind cross reference list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampWind = sampling->type.wind;
							if ( !add_wind_sample_crossrefs(cline, sampWind) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingWindCrossRefs,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingWindCrossRefs, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Wind field type ... wind type */
					else if ( fldtype == FpaC_WIND
							&& same(cmd, FpaCsamplingWindSampleType) )
						{
						/* Set wind sampling wind cross reference list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampWind = sampling->type.wind;
							arg      = strdup_arg(cline);
							xdef     = identify_sample(FpaCsamplesWinds, arg);
							FREEMEM(arg);
							if ( NotNull(xdef)
									&& same(xdef->samp_func,
												FpaDefaultWindFunc) )
								{
								sampWind->windsample = xdef;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingWindSampleType,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingWindSampleType, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Line field type ... attribute list */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set line sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampLine = sampling->type.line;
							if ( !add_line_sample_attribs(cline, sampLine) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered field type ... attribute list */
					else if ( fldtype == FpaC_SCATTERED
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set scattered sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampScattered = sampling->type.scattered;
							if ( !add_scattered_sample_attribs(cline,
									sampScattered) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Link chain field type ... attribute list */
					else if ( fldtype == FpaC_LCHAIN
							&& same(cmd, FpaCsamplingAttribSampleNames) )
						{
						/* Set link chain sampling attribute list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							sampLchain = sampling->type.lchain;
							if ( !add_lchain_sample_attribs(cline, sampLchain) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCsamplingAttribSampleNames,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsamplingAttribSampleNames,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Discrete field type ... value type list */
					else if ( fldtype == FpaC_DISCRETE
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCsamplingValueSampleTypes, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

				/* >>> the following is obsolete in next version <<< */
					/* Line field type ... value type list */
					else if ( fldtype == FpaC_LINE
							&& same(cmd, FpaCsamplingValueSampleTypes) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCsamplingValueSampleTypes, FpaCmsgObsolete);
						fdef->valid_detail = FALSE;
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCsample, cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsLinking section */
				else if ( section_id == FpaCblockFieldsLinking )
					{

					/* Linking interpolation delta */
					if ( same(cmd, FpaClinkingInterpDelta) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							minterp = interpret_hour_minute_string(cline);
							if ( minterp > 0 )
								{
								linking->minterp = minterp;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClinkingInterpDelta,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClinkingInterpDelta, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Linking fields reset */
					else if ( same(cmd, FpaClinkingFieldsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset link fields list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								(void) free_element_link_fields(fldtype,
																	linking);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClinkingFieldsReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClinkingFieldsReset, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Linking fields */
					else if ( same(cmd, FpaClinkingFields) )
						{
						/* Add link fields to list */
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !add_element_link_fields(cline,
															fldtype, linking) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClinkingFields, FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClinkingFields, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClinking, cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsEquation section */
				else if ( section_id == FpaCblockFieldsEquation )
					{

					/* Field equation force calculation */
					if ( same(cmd, FpaCequationForceCalc) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							equation->force = logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCequationForceCalc,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCequationForceCalc, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field equation string */
					else if ( same(cmd, FpaCequationString) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							equation->eqtn = STRMEM(equation->eqtn, cline);
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCequationString, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field equation units */
					else if ( same(cmd, FpaCequationUnits) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg             = strdup_arg(cline);
							equation->units = identify_unit(arg);
							FREEMEM(arg);
							if ( IsNull(equation->units) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCequationUnits, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCequationUnits, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCequation, cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsValCalc section */
				else if ( section_id == FpaCblockFieldsValCalc )
					{

					/* Field value calculation force calculation */
					if ( same(cmd, FpaCvalCalcForceCalc) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							valcalc->force = logical_arg(cline, &valid);
							if ( !valid )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCvalCalcForceCalc,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCvalCalcForceCalc, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field value calculation crossreference */
					else if ( same(cmd, FpaCvalCalcCrossRef) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg   = strdup_arg(cline);
							crdef = identify_crossref(FpaCcRefsValues, arg);
							FREEMEM(arg);

							if ( NotNull(crdef) )
								{
								valcalc->vcalc = crdef;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCvalCalcCrossRef, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCvalCalcCrossRef, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Field value calculation source type(s) */
					else if ( same(cmd, FpaCvalCalcSrcTypes) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_vcalc_src_types(cline, valcalc) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCvalCalcSrcTypes, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCvalCalcSrcTypes, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCvalueCalculation,
								cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCblank, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Fields declaration */
			/*  ... with format of "cmd = value(s)"           */
			else if ( numbrace == 4 )
				{

				/* Adding parameters in FpaCblockFieldsLineTypesSet section */
				if ( section_id == FpaCblockFieldsLineTypesSet )
					{

					/* Line type label */
					if ( same(cmd, FpaClineTypesLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								ltypes->type_labels[nsub] =
										STRMEM(ltypes->type_labels[nsub], arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClineTypesLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

				/* >>> the following is obsolete in next version <<< */
					/* Subelements label */
					else if ( same(cmd, FpaCsubelementsLabel) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClineTypesLabel,
								FpaCsubelementsLabel, FpaCmsgReplace);
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								ltypes->type_labels[nsub] =
										STRMEM(ltypes->type_labels[nsub], arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCsubelementsLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}
				/* >>> the preceding is obsolete in next version <<< */

					/* Line type short label */
					else if ( same(cmd, FpaClineTypesShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								ltypes->type_sh_labels[nsub] =
										STRMEM(ltypes->type_sh_labels[nsub],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClineTypesShortLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Line type pattern file */
					else if ( same(cmd, FpaClineTypesPattern) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( OKARG(arg) )
								{
								ltypes->patterns[nsub] =
										STRMEM(ltypes->patterns[nsub], arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaClineTypesPattern, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClineTypesPattern, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClineTypes,
								cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsScatteredTypesSet section */
				else if ( section_id == FpaCblockFieldsScatteredTypesSet )
					{

					/* Scattered type label */
					if ( same(cmd, FpaCscatteredTypesLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_labels[nsub] =
										STRMEM(stypes->type_labels[nsub], arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type short label */
					else if ( same(cmd, FpaCscatteredTypesShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg  = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_sh_labels[nsub] =
										STRMEM(stypes->type_sh_labels[nsub],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesShortLabel,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered types style class */
					else if ( same(cmd, FpaCscatteredTypesClass) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_classes[nsub] =
										STRMEM(stypes->type_classes[nsub], arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesClass,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesClass, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered types entry file */
					else if ( same(cmd, FpaCscatteredTypesEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_entry_files[nsub] =
										STRMEM(stypes->type_entry_files[nsub],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesEntryFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesEntryFile,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered types modify file */
					else if ( same(cmd, FpaCscatteredTypesModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								stypes->type_modify_files[nsub] =
										STRMEM(stypes->type_modify_files[nsub],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesModifyFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesModifyFile,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type attach option */
					else if ( same(cmd, FpaCscatteredTypesAttach) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg   = string_arg(cline);
							macro = config_file_macro(arg,
											NumFpaCattachOpts, FpaCattachOpts);
							if ( macro != FpaCnoMacro
									&& check_attach_option(fldtype,
											macro, (SPFEAT *) 0))
								{
								stypes->type_attach_opts[nsub] = macro;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesAttach,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesAttach, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type default attributes reset */
					else if ( same(cmd, FpaCscatteredTypesAttribDefaultsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset scattered types default attributes list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								sattrib = &(stypes->type_attribs[nsub]);
								(void) free_element_type_attribs(sattrib);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesAttribDefaultsReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesAttribDefaultsReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type default attributes */
					else if ( same(cmd, FpaCscatteredTypesAttribDefaults) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for scattered types */
							/*  default attributes                 */
							if ( blank(arg) )
								{
								sattrib = &(stypes->type_attribs[nsub]);
								section = FpaCblockFieldsScatteredAttribsSet;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Remove scattered types  default attributes */
									sattrib = &(stypes->type_attribs[nsub]);
									(void) free_element_type_attribs(sattrib);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, FpaCblank,
											FpaCscatteredTypesAttribDefaults,
											FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesAttribDefaults,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type entry rules reset */
					else if ( same(cmd, FpaCscatteredTypesEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset scattered types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								srule = &(stypes->type_rules[nsub]);
								(void) free_element_type_rules(srule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesEntryRulesReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type entry rules */
					else if ( same(cmd, FpaCscatteredTypesEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add scattered type entry rules to list */
							srule = &(stypes->type_rules[nsub]);
							if ( !add_element_type_rules(cline, srule) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesEntryRules,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type python entry rules reset */
					else if ( same(cmd, FpaCscatteredTypesPyEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset scattered types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								srule = &(stypes->type_rules[nsub]);
								(void) free_element_type_py_rules(srule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesPyEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesPyEntryRulesReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Scattered type python entry rules */
					else if ( same(cmd, FpaCscatteredTypesPyEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add scattered type entry rules to list */
							srule = &(stypes->type_rules[nsub]);
							if ( !add_element_type_py_rules(cline, srule) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCscatteredTypesPyEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCscatteredTypesPyEntryRules,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCscatteredTypes,
								cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsAttribsSet section */
				else if ( section_id == FpaCblockFieldsAttribsSet )
					{

					/* Attribute label */
					if ( same(cmd, FpaCattributesLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								attrib->attrib_labels[natt] =
										STRMEM(attrib->attrib_labels[natt],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCattributesLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Attribute short label */
					else if ( same(cmd, FpaCattributesShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								attrib->attrib_sh_labels[natt] =
										STRMEM(attrib->attrib_sh_labels[natt],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCattributesShortLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Attribute default value */
					else if ( same(cmd, FpaCattributesBackDefault) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							if ( !set_element_attribute_backdef(cline,
									attrib->attrib_names[natt],
									&(attrib->attrib_back_defs[natt])) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, FpaCblank,
										FpaCattributesBackDefault,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaCattributesBackDefault, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCattributes, cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsLabelTypes section */
				else if ( section_id == FpaCblockFieldsLabelTypes )
					{

					/* Add another labelling type (if not already there!) */
					nlab = add_element_labelling_type(cmd, labelling);
					if ( nlab < 0 )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClabellingTypes,
								cmd, FpaCmsgMember);
						fdef->valid_detail = FALSE;
						(void) skip_config_file_block(&fpcfg);
						continue;
						}
					section = FpaCblockFieldsLabelTypesSet;
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, FpaCblank, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Fields declaration */
			/*  ... with format of "cmd = value(s)"             */
			else if ( numbrace == 5 )
				{

				/* Adding parameters in FpaCblockFieldsScatteredAttribsSet section */
				if ( section_id == FpaCblockFieldsScatteredAttribsSet )
					{
					if ( !add_element_type_attrib(cmd, cline, sattrib) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaCblank,
								FpaCscatteredTypesAttribDefaults,
								FpaCmsgParameter);
						fdef->valid_detail = FALSE;
						}
					}

				/* Adding parameters in FpaCblockFieldsLabelTypesSet section */
				else if ( section_id == FpaCblockFieldsLabelTypesSet )
					{

					/* Labelling type label */
					if ( same(cmd, FpaClabellingLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_labels[nlab] =
										STRMEM(labelling->type_labels[nlab],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type short label */
					else if ( same(cmd, FpaClabellingShortLabel) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_sh_labels[nlab] =
										STRMEM(labelling->type_sh_labels[nlab],
												arg);
								}
							else
								{
								/* Ignore missing labels, since they  */
								/*  may be from another language!     */
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingShortLabel, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling style class */
					else if ( same(cmd, FpaClabellingClass) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_classes[nlab] =
										STRMEM(labelling->type_classes[nlab],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingClass, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingClass, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling entry file */
					else if ( same(cmd, FpaClabellingEntryFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_entry_files[nlab] =
										STRMEM(labelling->type_entry_files[nlab],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingEntryFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingEntryFile, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling modify file */
					else if ( same(cmd, FpaClabellingModifyFile) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);
							if ( ISARG(arg) )
								{
								labelling->type_modify_files[nlab] =
										STRMEM(labelling->type_modify_files[nlab],
												arg);
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingModifyFile,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingModifyFile,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type attach option */
					else if ( same(cmd, FpaClabellingAttach) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg   = string_arg(cline);
							macro = config_file_macro(arg,
											NumFpaCattachOpts, FpaCattachOpts);
							if ( macro != FpaCnoMacro
									&& check_attach_option(fldtype,
											macro, (SPFEAT *) 0))
								{
								labelling->type_attach_opts[nlab] = macro;
								}
							else
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingAttach, FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingAttach, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type default attributes reset */
					else if ( same(cmd, FpaClabellingAttribDefaultsReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types default attributes list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								lattrib = &(labelling->type_attribs[nlab]);
								(void) free_element_type_attribs(lattrib);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingAttribDefaultsReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingAttribDefaultsReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type default attributes */
					else if ( same(cmd, FpaClabellingAttribDefaults) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{
							arg = string_arg(cline);

							/* Set the section for label default attributes */
							if ( blank(arg) )
								{
								lattrib = &(labelling->type_attribs[nlab]);
								section = FpaCblockFieldsLabelAttribsSet;
								}

							/* Otherwise only FpaCnone is acceptable */
							else
								{
								if ( same_ic(arg, FpaCnone) )
									{

									/* Remove label default attributes */
									lattrib = &(labelling->type_attribs[nlab]);
									(void) free_element_type_attribs(lattrib);
									}
								else
									{
									(void) config_file_message(FpaCblockFields,
											fldbuf, labelling->type_names[nlab],
											FpaClabellingAttribDefaults,
											FpaCmsgParameter);
									fdef->valid_detail = FALSE;
									}
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingAttribDefaults,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type entry rules reset */
					else if ( same(cmd, FpaClabellingEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								lrule = &(labelling->type_rules[nlab]);
								(void) free_element_type_rules(lrule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingEntryRulesReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type entry rules */
					else if ( same(cmd, FpaClabellingEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add labelling entry rules to list */
							lrule = &(labelling->type_rules[nlab]);
							if ( !add_element_type_rules(cline, lrule) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingEntryRules, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type python entry rules reset */
					else if ( same(cmd, FpaClabellingPyEntryRulesReset) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Reset labelling types entry rules list */
							valid = logical_arg(cline, &argok);
							if ( valid )
								{
								lrule = &(labelling->type_rules[nlab]);
								(void) free_element_type_py_rules(lrule);
								}
							if ( !argok )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingPyEntryRulesReset,
										FpaCmsgParameter);
								fdef->valid_detail = FALSE;
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingPyEntryRulesReset,
									FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Labelling type python entry rules */
					else if ( same(cmd, FpaClabellingPyEntryRules) )
						{
						if ( same(string_arg(cline), FpaCequalSign) )
							{

							/* Add labelling entry rules to list */
							lrule = &(labelling->type_rules[nlab]);
							if ( !add_element_type_py_rules(cline, lrule) )
								{
								(void) config_file_message(FpaCblockFields,
										fldbuf, labelling->type_names[nlab],
										FpaClabellingPyEntryRules,
										FpaCmsgParameter);
								}
							}
						else
							{
							(void) config_file_message(FpaCblockFields,
									fldbuf, FpaCblank,
									FpaClabellingPyEntryRules, FpaCmsgNoEqual);
							fdef->valid_detail = FALSE;
							}
						}

					/* Set error flag for unrecognized Fields keyword */
					else
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, FpaClabellingTypes,
								cmd, FpaCmsgKeyword);
						fdef->valid_detail = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, cmd, FpaCblank, FpaCmsgSection);
					}
				}

			/* Set parameters in low level Fields declaration */
			/*  ... with format of "cmd = value(s)"             */
			else
				{

				/* Adding parameters in FpaCblockFieldsLabelAttribsSet section */
				if ( section_id == FpaCblockFieldsLabelAttribsSet )
					{
					if ( !add_element_type_attrib(cmd, cline, lattrib) )
						{
						(void) config_file_message(FpaCblockFields,
								fldbuf, labelling->type_names[nlab],
								FpaClabellingAttribDefaults, FpaCmsgParameter);
						fdef->valid_detail = FALSE;
						}
					}

				/*  Error in section identification */
				else
					{
					(void) config_file_message(FpaCblockFields,
							fldbuf, cmd, FpaCblank, FpaCmsgSection);
					}
				}
			}

		/* Diagnostic message */
		(void) pr_diag("Config", "  ... ending at \"%d\" in file \"%s\"\n",
				ftell(fpcfg), fdef->filenames[nblk]);

		/* Close the configuration file for this Fields declaration */
		(void) config_file_close(&fpcfg);
		}

	/* Return pointer when all configuration files have been read */
	return fdef;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ f i e l d                                                *
*   i n i t _ f i e l d                                                *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Fields block, or pointer to initialized structures containing      *
*   information read from Fields block of configuration files.         *
*   Note that field name comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	FpaConfigFieldStruct	*find_field

	(
	FpaConfigElementStruct	*edef,		/* pointer to Element structure */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{
	FPAC_FIELD_IDENTS			*pident;

	/* Static buffer for searching */
	static	FPAC_FIELD_IDENTS	*sident = NullPtr(FPAC_FIELD_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_FIELD_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumFieldIdent < 1 ) return NullPtr(FpaConfigFieldStruct *);

	/* Copy the element and level names into static structure for searching */
	sident->element = edef->name;
	sident->level   = ldef->name;

	/* Search the identifier list for element and level names */
	pident = (FPAC_FIELD_IDENTS *) bsearch((POINTER) sident,
			(POINTER) FieldIdents, (size_t) NumFieldIdent,
			sizeof(FPAC_FIELD_IDENTS), compare_field_identifiers);

	/* Return pointer if element and level names found in list */
	return ( pident ) ? (FpaConfigFieldStruct *) pident->pdef:
							NullPtr(FpaConfigFieldStruct *);
	}

/**********************************************************************/

static	FpaConfigFieldStruct	*init_field

	(
	FpaConfigElementStruct	*edef,		/* pointer to Element structure */
	FpaConfigLevelStruct	*ldef		/* pointer to Level structure */
	)

	{
	FpaConfigFieldStruct	*fdef;

	/* Add field at end of current FieldDefs list */
	NumFieldDef++;
	FieldDefs = GETMEM(FieldDefs, FpaConfigFieldStruct *, NumFieldDef);
	FieldDefs[NumFieldDef-1] = INITMEM(FpaConfigFieldStruct, 1);

	/* Initialize FieldDefs structure */
	fdef                   = FieldDefs[NumFieldDef - 1];
	fdef->element          = edef;
	fdef->level            = ldef;
	fdef->valid            = TRUE;
	fdef->nblocks          = 0;
	fdef->filenames        = NullStringList;
	fdef->locations        = NullLong;
	fdef->label            = NullString;
	fdef->sh_label         = NullString;
	fdef->group            = NullPtr(FpaConfigGroupStruct *);
	fdef->created_field    = FALSE;
	fdef->field_detail     = FALSE;
	fdef->valid_detail     = FALSE;
	fdef->override_element = FALSE;
	fdef->override_level   = FALSE;

	/* Add pointers to field identifiers list */
	NumFieldIdent++;
	FieldIdents = GETMEM(FieldIdents, FPAC_FIELD_IDENTS, NumFieldIdent);
	FieldIdents[NumFieldIdent-1].element = edef->name;
	FieldIdents[NumFieldIdent-1].level   = ldef->name;
	FieldIdents[NumFieldIdent-1].pdef    = (POINTER) fdef;

	/* Sort the list */
	(void) qsort((POINTER) FieldIdents, (size_t) NumFieldIdent,
			sizeof(FPAC_FIELD_IDENTS), compare_field_identifiers);

	/* Return pointer to FieldDefs structure */
	return fdef;
	}

/***********************************************************************
*                                                                      *
*   s e t _ f i e l d _ l o c a t i o n                                *
*                                                                      *
*   Save configuration file name and location for reading detailed     *
*   information from Fields block of configuration files.              *
*                                                                      *
*   s e t _ f i e l d _ g r o u p _ a n d _ l a b e l s                *
*                                                                      *
*   Set default group and labels for created fields.                   *
*                                                                      *
***********************************************************************/

static	LOGICAL					set_field_location

	(
	FILE					*fpcfg,		/* pointer to configuration file */
	FpaConfigFieldStruct	*fdef		/* pointer to Field structure */
	)

	{
	STRING		cfgname;
	long int	position;
	int			nblk;

	/* Return FALSE if no structure passed */
	if ( IsNull(fdef) ) return FALSE;

	/* Get file name and location from configuration file */
	if ( !config_file_location(fpcfg, &cfgname, &position) ) return FALSE;

	/* Add configuration file name and location to list */
	nblk = fdef->nblocks++;
	fdef->filenames = GETMEM(fdef->filenames, STRING,   fdef->nblocks);
	fdef->locations = GETMEM(fdef->locations, long int, fdef->nblocks);
	fdef->filenames[nblk] = cfgname;
	fdef->locations[nblk] = position;

	/* Return TRUE if all went OK */
	return TRUE;
	}

/**********************************************************************/

static	void					set_field_group_and_labels

	(
	FpaConfigFieldStruct	*fdef		/* pointer to Field structure */
	)

	{

	/* Static buffer for field labels */
	static	char	fldlabel[CONFIG_LABEL_LEN];

	/* Set group, if not already set */
	if ( IsNull(fdef->group) )
		{

		/* Set group from element, if it is there */
		if ( NotNull(fdef->element->fld_group) )
			fdef->group = fdef->element->fld_group;

		/* Set group from level, if it is there */
		else if ( NotNull(fdef->level->fld_group) )
			fdef->group = fdef->level->fld_group;

		/* Set group to default group FpaCmiscellaneous */
		else
			fdef->group = identify_group(FpaCblockFields, FpaCmiscellaneous);
		}

	/* Create field label, if not already there */
	if ( IsNull(fdef->label) )
		{
		(void) strcpy(fldlabel, fdef->level->label);
		(void) strcat(fldlabel, " ");
		(void) strcat(fldlabel, fdef->element->label);
		fdef->label = strdup(fldlabel);
		}

	/* Create field short label, if not already there */
	if ( IsNull(fdef->sh_label) )
		{
		(void) strcpy(fldlabel, fdef->level->sh_label);
		(void) strcat(fldlabel, " ");
		(void) strcat(fldlabel, fdef->element->sh_label);
		fdef->sh_label = strdup(fldlabel);
		}
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (CrossRefs block of configuration file)  *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ c r o s s r e f s _ i n f o                              *
*   r e a d _ c r o s s r e f _ f i e l d _ i n f o                    *
*                                                                      *
*   Read information from CrossRefs block of configuration files.      *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_crossrefs_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, arg;
	int						numbrace, section_id, section;
	LOGICAL					firstline, valid;
	FpaConfigCrossRefStruct	*crdef;
	int						nn;

	/* Read the configuration file(s) only once */
	if ( CrossRefsRead ) return CrossRefsValid;

	/* Force the Levels block of the configuration file to be read first */
	if ( !read_levels_info() ) return CrossRefsValid;

	/* Force the Elements block of the configuration file to be read next */
	if ( !read_elements_info() ) return CrossRefsValid;

	/* Force the Fields block of the configuration file to be read next */
	if ( !read_fields_info() ) return CrossRefsValid;

	/* Find and open the configuration file for the CrossRefs block */
	if ( !first_config_file_open(FpaCcrossRefsFile, &fpcfg) )
		{
		CrossRefsRead = TRUE;
		return CrossRefsValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading CrossRefs block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read CrossRefs block of configuration file */
		if ( same(cmd, FpaCblockCrossRefs) )
			{

			/* Set counter and identifier for CrossRefs block */
			numbrace   = 0;
			section    = FpaCblockCrossRefsSection;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read CrossRefs block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of CrossRefs block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new CrossRefs section */
				else if ( numbrace == 1 )
					{

					/* Check for Winds or Values sections */
					if ( section_id == FpaCblockCrossRefsSection )
						{

						/* Set identifier for Winds section of CrossRefs block */
						if ( same(cmd, FpaCcRefsWinds) )
							section = FpaCblockCrossRefsWinds;

						/* Set identifier for Values section of CrossRefs block */
						else if ( same(cmd, FpaCcRefsValues) )
							section = FpaCblockCrossRefsValues;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockCrossRefs,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Start of new Winds or Values declaration */
				else if ( numbrace == 2 )
					{

					/* Adding another name in FpaCblockCrossRefsWinds section */
					if ( section_id == FpaCblockCrossRefsWinds )
						{

						/* Check for declaration already in the list */
						crdef = find_wind_crossref(cmd);

						/* Add another crossref name to the lists for winds */
						if ( IsNull(crdef) )
							{
							crdef = init_wind_crossref(cmd);
							}

						/* Check that crossref name for winds is not an alias */
						/*  of another crossref name for winds!               */
						else if ( !same(cmd, crdef->name) )
							{
							(void) config_file_message(FpaCblockCRWinds,
									cmd, crdef->name, FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of CrossRefs Winds */
						section = FpaCblockCrossRefsWindsInfo;
						}

					/* Adding another name in FpaCblockCrossRefsValues section */
					else if ( section_id == FpaCblockCrossRefsValues )
						{

						/* Check for declaration already in the list */
						crdef = find_value_crossref(cmd);

						/* Add another crossref name to the lists for values */
						if ( IsNull(crdef) )
							{
							crdef = init_value_crossref(cmd);
							}

						/* Check that crossref name for values is not an */
						/*  alias of another crossref name for values!   */
						else if ( !same(cmd, crdef->name) )
							{
							(void) config_file_message(FpaCblockCRValues,
									cmd, crdef->name, FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of CrossRefs Values */
						section = FpaCblockCrossRefsValuesInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockCrossRefs,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in CrossRefs declarations */
				/*  ... with format of "cmd = value(s)"     */
				else
					{

					/* Adding parameters in FpaCblockCrossRefsWindsInfo section */
					if ( section_id == FpaCblockCrossRefsWindsInfo )
						{

						/* CrossRefs Winds label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->label = STRMEM(crdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRWinds,
										crdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Winds short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->sh_label =
											STRMEM(crdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRWinds,
										crdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Winds description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->description =
											STRMEM(crdef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRWinds,
										crdef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Winds function name */
						else if ( same(cmd, FpaCwindFunction) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->func_name =
											STRMEM(crdef->func_name, arg);
									}
								else
									{
									(void) config_file_message(FpaCblockCRWinds,
											crdef->name, FpaCblank,
											FpaCwindFunction, FpaCmsgParameter);
									crdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRWinds,
										crdef->name, FpaCblank,
										FpaCwindFunction, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

					/* >>> the following is obsolete in next version <<< */
						/* CrossRefs Winds calculation type */
						else if ( same(cmd, FpaCwindCalcType) )
							{
							(void) config_file_message(FpaCblockCRWinds,
									crdef->name, FpaCblank,
									FpaCwindCalcType, FpaCmsgObsolete);
							crdef->valid = FALSE;
							}
					/* >>> the preceding is obsolete in next version <<< */

						/* CrossRefs Winds fields */
						else if ( same(cmd, FpaCcrossRefFields) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);

								/* Read Winds fields block of CrossRefs block */
								if ( blank(arg) )
									{
									if ( !read_crossref_field_info(&fpcfg,
													FpaCblockCRWinds, crdef) )
										{
										(void) config_file_message(FpaCblockCRWinds,
												crdef->name, FpaCblank,
												FpaCcrossRefFields,
												FpaCmsgInvalid);
										crdef->valid = FALSE;
										}
									}

								/* Otherwise only FpaCnone is acceptable */
								else
									{
									if ( !same_ic(arg, FpaCnone) )
										{
										(void) config_file_message(FpaCblockCRWinds,
												crdef->name, FpaCblank,
												FpaCcrossRefFields,
												FpaCmsgParameter);
										crdef->valid = FALSE;
										}

									/* But cannot reset Winds fields to None! */
									else if ( crdef->nfld >= 1 )
										{
										(void) config_file_message(FpaCblockCRWinds,
												crdef->name, FpaCblank,
												FpaCcrossRefFields,
												FpaCmsgResetNone);
										}
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRWinds,
										crdef->name, FpaCblank,
										FpaCcrossRefFields, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized CrossRefs keyword */
						else
							{
							(void) config_file_message(FpaCblockCRWinds,
									crdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							crdef->valid = FALSE;
							}
						}

					/* Adding parameters in FpaCblockCrossRefsValuesInfo section */
					else if ( section_id == FpaCblockCrossRefsValuesInfo )
						{

						/* CrossRefs Values label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->label = STRMEM(crdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Values short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->sh_label =
											STRMEM(crdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Values description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->description =
											STRMEM(crdef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Values function name */
						else if ( same(cmd, FpaCvalueFunction) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									crdef->func_name =
											STRMEM(crdef->func_name, arg);
									}
								else
									{
									(void) config_file_message(FpaCblockCRValues,
											crdef->name, FpaCblank,
											FpaCvalueFunction,
											FpaCmsgParameter);
									crdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaCvalueFunction, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

					/* >>> the following is obsolete in next version <<< */
						/* CrossRefs Values calculation type */
						else if ( same(cmd, FpaCvalueCalcType) )
							{
							(void) config_file_message(FpaCblockCRValues,
									crdef->name, FpaCblank,
									FpaCvalueCalcType, FpaCmsgObsolete);
							crdef->valid = FALSE;
							}
					/* >>> the preceding is obsolete in next version <<< */

						/* CrossRefs Values time weight for calculations */
						else if ( same(cmd, FpaCtimeWeight) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								crdef->wgtt   = double_arg(cline, &valid);
								arg           = strdup_arg(cline);
								crdef->unit_t = identify_unit(arg);
								FREEMEM(arg);
								if ( !valid )
									{
									(void) config_file_message(FpaCblockCRValues,
											crdef->name, FpaCblank,
											FpaCtimeWeightVal,
											FpaCmsgParameter);
									crdef->valid = FALSE;
									}
								if ( IsNull(crdef->unit_t) )
									{
									(void) config_file_message(FpaCblockCRValues,
											crdef->name, FpaCblank,
											FpaCtimeWeightUnit,
											FpaCmsgParameter);
									crdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaCtimeWeight, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Values value weight for calculations */
						else if ( same(cmd, FpaCvalueWeight) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								crdef->wgtv   = double_arg(cline, &valid);
								arg           = strdup_arg(cline);
								crdef->unit_v = identify_unit(arg);
								FREEMEM(arg);
								if ( !valid )
									{
									(void) config_file_message(FpaCblockCRValues,
											crdef->name, FpaCblank,
											FpaCvalueWeightVal,
											FpaCmsgParameter);
									crdef->valid = FALSE;
									}
								if ( IsNull(crdef->unit_v) )
									{
									(void) config_file_message(FpaCblockCRValues,
											crdef->name, FpaCblank,
											FpaCvalueWeightUnit,
											FpaCmsgParameter);
									crdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaCvalueWeight, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* CrossRefs Values fields */
						else if ( same(cmd, FpaCcrossRefFields) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);

								/* Read Values fields block of CrossRefs block */
								if ( blank(arg) )
									{
									if ( !read_crossref_field_info(&fpcfg,
													FpaCblockCRValues, crdef) )
										{
										(void) config_file_message(FpaCblockCRValues,
												crdef->name, FpaCblank,
												FpaCcrossRefFields,
												FpaCmsgInvalid);
										crdef->valid = FALSE;
										}
									}

								/* Otherwise only FpaCnone is acceptable */
								else
									{
									if ( !same_ic(arg, FpaCnone) )
										{
										(void) config_file_message(FpaCblockCRValues,
												crdef->name, FpaCblank,
												FpaCcrossRefFields,
												FpaCmsgParameter);
										crdef->valid = FALSE;
										}

									/* But cannot reset Values fields to None! */
									else if ( crdef->nfld >= 1 )
										{
										(void) config_file_message(FpaCblockCRValues,
												crdef->name, FpaCblank,
												FpaCcrossRefFields,
												FpaCmsgResetNone);
										}
									}
								}
							else
								{
								(void) config_file_message(FpaCblockCRValues,
										crdef->name, FpaCblank,
										FpaCcrossRefFields, FpaCmsgNoEqual);
								crdef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized CrossRefs keyword */
						else
							{
							(void) config_file_message(FpaCblockCRValues,
									crdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							crdef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockCrossRefs,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Add default "FPA_Absolute_Wind_Model" to CrossRefs Winds */
	crdef = find_wind_crossref(FpaAbsWindModel);
	if ( IsNull(crdef) )
		{
		crdef = init_wind_crossref(FpaAbsWindModel);
		crdef->func_name = strdup(FpaAbsWindFunc);
		crdef->nfld      = 1;
		crdef->flds      = INITMEM(FpaConfigFieldStruct *, 1);
		crdef->flds[0]   = identify_field(FpaCanyElement, FpaCanyLevel);
		}

	/* Error check for each member of CrossRefs Winds block */
	for ( nn=0; nn<NumCRefWindDef; nn++ )
		{
		crdef = CRefWindDefs[nn];

		/* Ensure that "wind_function" has been set */
		if ( blank(crdef->func_name) )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockCRWinds, crdef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCwindFunction, crdef->name);
			crdef->valid = FALSE;
			}
		}

	/* Error check for each member of CrossRefs Values block */
	for ( nn=0; nn<NumCRefValDef; nn++ )
		{
		crdef = CRefValDefs[nn];

		/* Ensure that "value_function" has been set */
		if ( blank(crdef->func_name) )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockCRValues, crdef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCvalueFunction, crdef->name);
			crdef->valid = FALSE;
			}
		}

	/* Set flags for completion of reading */
	CrossRefsRead  = TRUE;
	CrossRefsValid = TRUE;
	return CrossRefsValid;
	}

/**********************************************************************/

static	LOGICAL					read_crossref_field_info

	(
	FILE					**fpcfg,	/* pointer to configuration file */
	STRING					cblock,		/* cross reference block */
	FpaConfigCrossRefStruct	*crdef		/* pointer to CrossRef structure */
	)

	{
	STRING						cline, cmd, arg;
	int							numbrace, section_id, section, nn;
	LOGICAL						firstline, valid;
	char						crefbuf[CONFIG_FILE_MESSAGE_LEN];
	FpaConfigFieldStruct		*fdef;
	FpaConfigElementStruct		*edef;
	FpaConfigLevelStruct		*ldef;

	/* Static buffers for element and level names for field */
	static	char	element[CONFIG_LABEL_LEN];
	static	char	level[CONFIG_LABEL_LEN];

	/* Return FALSE if no structure passed */
	if ( IsNull(crdef) ) return FALSE;

	/* Set error checking parameter */
	valid = TRUE;

	/* Empty the current field list */
	if ( crdef->nfld > 0 )
		{
		FREEMEM(crdef->flds);
		crdef->nfld = 0;
		}

	/* Set counter and identifier for CrossRefsFields section */
	/*  of CrossRefs block                                    */
	numbrace   = 0;
	section    = FpaCblockCrossRefsFields;
	section_id = FpaCnoSection;
	firstline  = TRUE;

	/* Read CrossRefsFields block of CrossRefs block of configuration file */
	while ( NotNull( cline = read_config_file_line(fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* The first line should be an open bracket */
		if ( firstline )
			{
			firstline = FALSE;
			if ( !same(cmd, FpaCopenBrace) ) break;
			}

		/* Increment counter for open brackets */
		/*  and save the section identifier    */
		if ( same(cmd, FpaCopenBrace) )
			{
			numbrace++;
			section_id = push_section(section);
			}

		/* Decrement counter for close brackets */
		/*  and reset the section identifier    */
		else if ( same(cmd, FpaCcloseBrace) )
			{
			numbrace--;
			section_id = pop_section();

			/* Check for end of CrossRefsFields block */
			if ( numbrace == 0 ) break;
			}

		/* Start of new CrossRefsFields declaration */
		else
			{

			/* Adding another name in FpaCblockCrossRefsFields section */
			if ( section_id == FpaCblockCrossRefsFields )
				{

				/* Check for the named element */
				arg  = strdup(cmd);
				edef = identify_element(arg);
				(void) safe_strcpy(element, arg);
				FREEMEM(arg);

				/* Check for the named level */
				arg  = strdup_arg(cline);
				ldef = identify_level(arg);
				(void) safe_strcpy(level, arg);
				FREEMEM(arg);

				/* Error message if unrecognizable element or level */
				if ( IsNull(edef) || IsNull(ldef)
						|| !consistent_element_and_level(edef, ldef) )
					{
					(void) strcpy(crefbuf, element);
					(void) strcat(crefbuf, " ");
					(void) strcat(crefbuf, level);
					(void) config_file_message(cblock,
							crdef->name, crefbuf, FpaCblank, FpaCmsgCRef);
					crdef->valid = FALSE;
					valid        = FALSE;
					continue;
					}

				/* Identify field from element and level */
				fdef = identify_field(edef->name, ldef->name);

				/* Error message if unrecognizable field */
				if ( IsNull(fdef) )
					{
					(void) strcpy(crefbuf, element);
					(void) strcat(crefbuf, " ");
					(void) strcat(crefbuf, level);
					(void) config_file_message(cblock,
							crdef->name, crefbuf, FpaCblank, FpaCmsgCRef);
					crdef->valid = FALSE;
					valid        = FALSE;
					continue;
					}

				/* Add fields that are not already in the list */
				for ( nn=0; nn<crdef->nfld; nn++ )
					if ( fdef == crdef->flds[nn] ) break;
				if ( nn >= crdef->nfld )
					{
					crdef->nfld++;
					crdef->flds = GETMEM(crdef->flds, FpaConfigFieldStruct *,
																crdef->nfld);
					crdef->flds[crdef->nfld - 1] = fdef;
					}
				}

			/*  Error in section identification */
			else
				{
				(void) config_file_message(cblock,
						cmd, FpaCblank, FpaCblank, FpaCmsgSection);
				valid = FALSE;
				}
			}
		}

	/* Return error checking parameter */
	return valid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ w i n d _ c r o s s r e f                                *
*   i n i t _ w i n d _ c r o s s r e f                                *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Winds section of CrossRefs block, or pointer to initialized        *
*   structure to contain information read from Winds section of        *
*   CrossRefs block of configuration files.                            *
*   Note that crossref name comparisons are case insensitive!          *
*                                                                      *
***********************************************************************/

static	FpaConfigCrossRefStruct	*find_wind_crossref

	(
	STRING		name		/* crossref name for winds */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumCRefWindIdent < 1 ) return NullPtr(FpaConfigCrossRefStruct *);

	/* Copy the crossref name for winds into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for crossref name for winds */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) CRefWindIdents,
			(size_t) NumCRefWindIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if crossref name for wind found in list */
	return ( pident ) ? (FpaConfigCrossRefStruct *) pident->pdef:
							NullPtr(FpaConfigCrossRefStruct *);
	}

/**********************************************************************/

static	FpaConfigCrossRefStruct	*init_wind_crossref

	(
	STRING		name		/* crossref name for winds */
	)

	{
	FpaConfigCrossRefStruct	*crdef;

	/* Add crossref name for winds at end of current CRefWindDefs list */
	NumCRefWindDef++;
	CRefWindDefs = GETMEM(CRefWindDefs, FpaConfigCrossRefStruct *, NumCRefWindDef);
	CRefWindDefs[NumCRefWindDef-1] = INITMEM(FpaConfigCrossRefStruct, 1);

	/* Initialize CRefWindDefs structure */
	crdef              = CRefWindDefs[NumCRefWindDef - 1];
	crdef->name        = strdup(name);
	crdef->valid       = TRUE;
	crdef->label       = strdup(name);
	crdef->sh_label    = strdup(name);
	crdef->description = NullString;
	crdef->func_name   = NullString;
	crdef->wgtt        = 0.0;
	crdef->unit_t      = NullPtr(FpaConfigUnitStruct *);
	crdef->wgtv        = 0.0;
	crdef->unit_v      = NullPtr(FpaConfigUnitStruct *);
	crdef->nfld        = 0;
	crdef->flds        = NullPtr(FpaConfigFieldStruct **);

	/* Add the name as another identifier */
	(void) add_wind_crossref_identifier(name, crdef);

	/* Return pointer to CRefWindDefs structure */
	return crdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ w i n d _ c r o s s r e f _ i d e n t i f i e r            *
*                                                                      *
*   Add another identifier to crossref identifier list for winds.      *
*   Note that crossref name comparisons are case insensitive!          *
*                                                                      *
***********************************************************************/

static	void					add_wind_crossref_identifier

	(
	STRING					ident,		/* crossref identifier for winds */
	FpaConfigCrossRefStruct	*crdef		/* pointer to CrossRef structure
											for winds */
	)

	{

	/* Add identifier to list */
	NumCRefWindIdent++;
	CRefWindIdents = GETMEM(CRefWindIdents, FPAC_IDENTS, NumCRefWindIdent);
	CRefWindIdents[NumCRefWindIdent-1].ident = strdup(ident);
	CRefWindIdents[NumCRefWindIdent-1].pdef  = (POINTER) crdef;

	/* Sort the list */
	(void) qsort((POINTER) CRefWindIdents, (size_t) NumCRefWindIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*   f i n d _ v a l u e _ c r o s s r e f                              *
*   i n i t _ v a l u e _ c r o s s r e f                              *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Values section of CrossRefs block, or pointer to initialized       *
*   structure to contain information read from Values section of       *
*   CrossRefs block of configuration files.                            *
*   Note that crossref name comparisons are case insensitive!          *
*                                                                      *
***********************************************************************/

static	FpaConfigCrossRefStruct	*find_value_crossref

	(
	STRING		name		/* crossref name for values */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumCRefValIdent < 1 ) return NullPtr(FpaConfigCrossRefStruct *);

	/* Copy the crossref name for values into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for crossref name for values */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) CRefValIdents,
			(size_t) NumCRefValIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if crossref name for values found in list */
	return ( pident ) ? (FpaConfigCrossRefStruct *) pident->pdef:
							NullPtr(FpaConfigCrossRefStruct *);
	}

/**********************************************************************/

static	FpaConfigCrossRefStruct	*init_value_crossref

	(
	STRING		name		/* crossref name for values */
	)

	{
	FpaConfigCrossRefStruct	*crdef;

	/* Add crossref name for values at end of current CRefValDefs list */
	NumCRefValDef++;
	CRefValDefs = GETMEM(CRefValDefs, FpaConfigCrossRefStruct *, NumCRefValDef);
	CRefValDefs[NumCRefValDef-1] = INITMEM(FpaConfigCrossRefStruct, 1);

	/* Initialize CRefValDefs structure */
	crdef              = CRefValDefs[NumCRefValDef - 1];
	crdef->name        = strdup(name);
	crdef->valid       = TRUE;
	crdef->label       = strdup(name);
	crdef->sh_label    = strdup(name);
	crdef->description = NullString;
	crdef->func_name   = NullString;
	crdef->wgtt        = 0.0;
	crdef->unit_t      = NullPtr(FpaConfigUnitStruct *);
	crdef->wgtv        = 0.0;
	crdef->unit_v      = NullPtr(FpaConfigUnitStruct *);
	crdef->nfld        = 0;
	crdef->flds        = NullPtr(FpaConfigFieldStruct **);

	/* Add the name as another identifier */
	(void) add_value_crossref_identifier(name, crdef);

	/* Return pointer to CRefValDefs structure */
	return crdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ v a l u e _ c r o s s r e f _ i d e n t i f i e r          *
*                                                                      *
*   Add another identifier to crossref identifier list for values.     *
*   Note that crossref name comparisons are case insensitive!          *
*                                                                      *
***********************************************************************/

static	void					add_value_crossref_identifier

	(
	STRING					ident,		/* crossref identifier for values */
	FpaConfigCrossRefStruct	*crdef		/* pointer to CrossRef structure
											for values */
	)

	{

	/* Add identifier to list */
	NumCRefValIdent++;
	CRefValIdents = GETMEM(CRefValIdents, FPAC_IDENTS, NumCRefValIdent);
	CRefValIdents[NumCRefValIdent-1].ident = strdup(ident);
	CRefValIdents[NumCRefValIdent-1].pdef  = (POINTER) crdef;

	/* Sort the list */
	(void) qsort((POINTER) CRefValIdents, (size_t) NumCRefValIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Samples block of configuration file)    *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   r e a d _ s a m p l e s _ i n f o                                  *
*                                                                      *
*   Read information from Samples block of configuration files.        *
*                                                                      *
***********************************************************************/

static	LOGICAL					read_samples_info

	(
	)

	{
	FILE					*fpcfg;
	STRING					cline, cmd, arg;
	int						numbrace, section_id, section, macro;
	LOGICAL					firstline;
	FpaConfigSampleStruct	*xdef;
	int						nn;

	/* Read the configuration file(s) only once */
	if ( SamplesRead ) return SamplesValid;

	/* Find and open the configuration file for the Samples block */
	if ( !first_config_file_open(FpaCsamplesFile, &fpcfg) )
		{
		SamplesRead = TRUE;
		return SamplesValid;
		}

	/* Diagnostic message */
	(void) pr_diag("Config", "Reading Samples block!\n");

	/* Read the configuration file block by block                   */
	/* Note that read_config_file_line() handles "include" lines    */
	/*  and closes each configuration file as the last line is read */
	while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
		{

		/* Extract the first argument from the current line */
		cmd = string_arg(cline);

		/* Read Samples block of configuration file */
		if ( same(cmd, FpaCblockSamples) )
			{

			/* Set counter and identifier for Samples block */
			numbrace   = 0;
			section    = FpaCblockSamplesSection;
			section_id = FpaCnoSection;
			firstline  = TRUE;

			/* Read Samples block line by line */
			while ( NotNull( cline = read_config_file_line(&fpcfg) ) )
				{

				/* Extract the first argument from the current line */
				cmd = string_arg(cline);

				/* The first line should be an open bracket */
				if ( firstline )
					{
					firstline = FALSE;
					if ( !same(cmd, FpaCopenBrace) ) break;
					}

				/* Increment counter for open brackets */
				/*  and save the section identifier    */
				if ( same(cmd, FpaCopenBrace) )
					{
					numbrace++;
					section_id = push_section(section);
					}

				/* Decrement counter for close brackets */
				/*  and reset the section identifier    */
				else if ( same(cmd, FpaCcloseBrace) )
					{
					numbrace--;
					section_id = pop_section();

					/* Check for end of Samples block */
					if ( numbrace == 0 ) break;
					}

				/* Start of new Samples section */
				else if ( numbrace == 1 )
					{

					/* Check for Values or Winds sections */
					if ( section_id == FpaCblockSamplesSection )
						{

						/* Set identifier for Values section of Samples block */
						if ( same(cmd, FpaCsamplesValues) )
							section = FpaCblockSamplesValues;

						/* Set identifier for Winds section of Samples block */
						else if ( same(cmd, FpaCsamplesWinds) )
							section = FpaCblockSamplesWinds;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockSamples,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Start of new Values or Winds declaration */
				else if ( numbrace == 2 )
					{

					/* Adding another name in FpaCblockSamplesValues section */
					if ( section_id == FpaCblockSamplesValues )
						{

						/* Check for declaration already in the list */
						xdef = find_valuetype_sample(cmd);

						/* Add another sample name to the lists for values */
						if ( IsNull(xdef) )
							{
							xdef = init_valuetype_sample(cmd);
							}

						/* Check that sample name for values is not an */
						/*  alias of another sample name for values!   */
						else if ( !same(cmd, xdef->name) )
							{
							(void) config_file_message(FpaCblockSValues,
									cmd, xdef->name, FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Samples Values */
						section = FpaCblockSamplesValuesInfo;
						}

					/* Adding another name in FpaCblockSamplesWinds section */
					else if ( section_id == FpaCblockSamplesWinds )
						{

						/* Check for declaration already in the list */
						xdef = find_windtype_sample(cmd);

						/* Add another sample name to the lists for winds */
						if ( IsNull(xdef) )
							{
							xdef = init_windtype_sample(cmd);
							}

						/* Check that sample name for winds is not an alias */
						/*  of another sample name for winds!               */
						else if ( !same(cmd, xdef->name) )
							{
							(void) config_file_message(FpaCblockSWinds,
									cmd, xdef->name, FpaCblank, FpaCmsgName);
							(void) skip_config_file_block(&fpcfg);
							continue;
							}

						/* Set identifier for next section of Samples Winds */
						section = FpaCblockSamplesWindsInfo;
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockSamples,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}

				/* Set parameters in Samples declarations */
				/*  ... with format of "cmd = value(s)"   */
				else
					{

					/* Adding parameters in FpaCblockSamplesValuesInfo section */
					if ( section_id == FpaCblockSamplesValuesInfo )
						{

						/* Samples Values label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->label = STRMEM(xdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSValues,
										xdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Samples Values short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->sh_label =
											STRMEM(xdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSValues,
										xdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Samples Values description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->description =
											STRMEM(xdef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSValues,
										xdef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Samples Values calculation type */
						else if ( same(cmd, FpaCvalueSampType) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								macro = config_file_macro(arg,
										NumFpaCsampleTypes, FpaCsampleTypes);
								if ( macro != FpaCnoMacro )
									{
									xdef->samp_name =
											STRMEM(xdef->samp_name, arg);
									xdef->samp_type = macro;
									}
								else
									{
									(void) config_file_message(FpaCblockSValues,
											xdef->name, FpaCblank,
											FpaCvalueSampType,
											FpaCmsgParameter);
									xdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSValues,
										xdef->name, FpaCblank,
										FpaCvalueSampType, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Set error flag for unrecognized Samples keyword */
						else
							{
							(void) config_file_message(FpaCblockSValues,
									xdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							xdef->valid = FALSE;
							}
						}

					/* Adding parameters in FpaCblockSamplesWindsInfo section */
					else if ( section_id == FpaCblockSamplesWindsInfo )
						{

						/* Samples Winds label */
						if ( same(cmd, FpaClabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->label =
											STRMEM(xdef->label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSWinds,
										xdef->name, FpaCblank,
										FpaClabel, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Samples Winds short label */
						else if ( same(cmd, FpaCshortLabel) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->sh_label =
											STRMEM(xdef->sh_label, arg);
									}
								else
									{
									/* Ignore missing labels, since they  */
									/*  may be from another language!     */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSWinds,
										xdef->name, FpaCblank,
										FpaCshortLabel, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Samples Winds description */
						else if ( same(cmd, FpaCdescription) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->description =
											STRMEM(xdef->description, arg);
									}
								else
									{
									/* Ignore missing descriptions, since  */
									/*  they may be from another language! */
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSWinds,
										xdef->name, FpaCblank,
										FpaCdescription, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

						/* Samples Winds function name */
						else if ( same(cmd, FpaCwindFunction) )
							{
							if ( same(string_arg(cline), FpaCequalSign) )
								{
								arg = string_arg(cline);
								if ( ISARG(arg) )
									{
									xdef->samp_func =
											STRMEM(xdef->samp_func, arg);
									xdef->samp_type = FpaCnoMacro;
									}
								else
									{
									(void) config_file_message(FpaCblockSWinds,
											xdef->name, FpaCblank,
											FpaCwindFunction, FpaCmsgParameter);
									xdef->valid = FALSE;
									}
								}
							else
								{
								(void) config_file_message(FpaCblockSWinds,
										xdef->name, FpaCblank,
										FpaCwindFunction, FpaCmsgNoEqual);
								xdef->valid = FALSE;
								}
							}

					/* >>> the following is obsolete in next version <<< */
						/* Samples Winds calculation type */
						else if ( same(cmd, FpaCwindCalcType) )
							{
							(void) config_file_message(FpaCblockSWinds,
									xdef->name, FpaCblank,
									FpaCwindCalcType, FpaCmsgObsolete);
							xdef->valid = FALSE;
							}
					/* >>> the preceding is obsolete in next version <<< */

						/* Set error flag for unrecognized Samples keyword */
						else
							{
							(void) config_file_message(FpaCblockSWinds,
									xdef->name, FpaCblank,
									cmd, FpaCmsgKeyword);
							xdef->valid = FALSE;
							}
						}

					/*  Error in section identification */
					else
						{
						(void) config_file_message(FpaCblockSamples,
								cmd, FpaCblank, FpaCblank, FpaCmsgSection);
						}
					}
				}
			}

		/* Skip all other blocks in configuration file */
		else
			{
			(void) skip_config_file_block(&fpcfg);
			}
		}

	/* Error check for each member of Samples Values block */
	for ( nn=0; nn<NumSampleValDef; nn++ )
		{
		xdef = SampleValDefs[nn];

		/* Ensure that "value_samptype" has been set */
		if ( xdef->samp_type == FpaCnoMacro )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockSValues, xdef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCvalueSampType, xdef->name);
			xdef->valid = FALSE;
			}
		}

	/* Error check for each member of Samples Winds block */
	for ( nn=0; nn<NumSampleWindDef; nn++ )
		{
		xdef = SampleWindDefs[nn];

		/* Ensure that "wind_function" has been set */
		if ( blank(xdef->samp_func) )
			{
			(void) pr_error("Config", "Block: \"%s\"    Name: \"%s\"\n",
					FpaCblockSWinds, xdef->name);
			(void) pr_error("Config", "     No valid \"%s\" line for \"%s\"!\n",
					FpaCwindFunction, xdef->name);
			xdef->valid = FALSE;
			}
		}

	/* Set flags for completion of reading */
	SamplesRead  = TRUE;
	SamplesValid = TRUE;
	return SamplesValid;
	}

/***********************************************************************
*                                                                      *
*   f i n d _ v a l u e t y p e _ s a m p l e                          *
*   i n i t _ v a l u e t y p e _ s a m p l e                          *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Values section of Samples block, or pointer to initialized         *
*   structure to contain information read from Values section of       *
*   Samples block of configuration files.                              *
*   Note that sample name comparisons are case insensitive!            *
*                                                                      *
***********************************************************************/

static	FpaConfigSampleStruct	*find_valuetype_sample

	(
	STRING		name		/* sample name for values */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumSampleValIdent < 1 ) return NullPtr(FpaConfigSampleStruct *);

	/* Copy the sample name for values into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for sample name for values */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) SampleValIdents,
			(size_t) NumSampleValIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if sample name for values found in list */
	return ( pident ) ? (FpaConfigSampleStruct *) pident->pdef:
							NullPtr(FpaConfigSampleStruct *);
	}

/**********************************************************************/

static	FpaConfigSampleStruct	*init_valuetype_sample

	(
	STRING		name		/* sample name for values */
	)

	{
	FpaConfigSampleStruct	*xdef;

	/* Add sample name for values at end of current SampleValDefs list */
	NumSampleValDef++;
	SampleValDefs = GETMEM(SampleValDefs, FpaConfigSampleStruct *,
													NumSampleValDef);
	SampleValDefs[NumSampleValDef-1] = INITMEM(FpaConfigSampleStruct, 1);

	/* Initialize SampleValDefs structure */
	xdef              = SampleValDefs[NumSampleValDef - 1];
	xdef->name        = strdup(name);
	xdef->valid       = TRUE;
	xdef->label       = strdup(name);
	xdef->sh_label    = strdup(name);
	xdef->description = NullString;
	xdef->samp_name   = NullString;
	xdef->samp_type   = FpaCnoMacro;
	xdef->samp_func   = NullString;

	/* Add the name as another identifier */
	(void) add_valuetype_sample_identifier(name, xdef);

	/* Return pointer to SampleValDefs structure */
	return xdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ v a l u e t y p e _ s a m p l e _ i d e n t i f i e r      *
*                                                                      *
*   Add another identifier to sample identifier list for values.       *
*   Note that sample name comparisons are case insensitive!            *
*                                                                      *
***********************************************************************/

static	void					add_valuetype_sample_identifier

	(
	STRING					ident,		/* sample identifier for values */
	FpaConfigSampleStruct	*xdef		/* pointer to Sample structure
											for values */
	)

	{

	/* Add identifier to list */
	NumSampleValIdent++;
	SampleValIdents = GETMEM(SampleValIdents, FPAC_IDENTS, NumSampleValIdent);
	SampleValIdents[NumSampleValIdent-1].ident = strdup(ident);
	SampleValIdents[NumSampleValIdent-1].pdef  = (POINTER) xdef;

	/* Sort the list */
	(void) qsort((POINTER) SampleValIdents, (size_t) NumSampleValIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*   f i n d _ w i n d t y p e _ s a m p l e                            *
*   i n i t _ w i n d t y p e _ s a m p l e                            *
*                                                                      *
*   Return pointer to structure containing information read from       *
*   Winds section of Samples block, or pointer to initialized          *
*   structure to contain information read from Winds section of        *
*   Samples block of configuration files.                              *
*   Note that sample name comparisons are case insensitive!            *
*                                                                      *
***********************************************************************/

static	FpaConfigSampleStruct	*find_windtype_sample

	(
	STRING		name		/* sample name for winds */
	)

	{
	FPAC_IDENTS				*pident;

	/* Static buffer for searching */
	static	FPAC_IDENTS		*sident = NullPtr(FPAC_IDENTS *);

	/* Allocate space for static structure for searching */
	if ( IsNull(sident) )
		{
		sident       = INITMEM(FPAC_IDENTS, 1);
		sident->pdef = NullPointer;
		}

	/* Return Null if no identifiers in the list */
	if ( NumSampleWindIdent < 1 ) return NullPtr(FpaConfigSampleStruct *);

	/* Copy the sample name for winds into static structure for searching */
	sident->ident = name;

	/* Search the identifier list for sample name for winds */
	pident = (FPAC_IDENTS *) bsearch((POINTER) sident, (POINTER) SampleWindIdents,
			(size_t) NumSampleWindIdent, sizeof(FPAC_IDENTS),
			compare_identifiers_ic);

	/* Return pointer if sample name for wind found in list */
	return ( pident ) ? (FpaConfigSampleStruct *) pident->pdef:
							NullPtr(FpaConfigSampleStruct *);
	}

/**********************************************************************/

static	FpaConfigSampleStruct	*init_windtype_sample

	(
	STRING		name		/* sample name for winds */
	)

	{
	FpaConfigSampleStruct	*xdef;

	/* Add sample name for winds at end of current SampleWindDefs list */
	NumSampleWindDef++;
	SampleWindDefs = GETMEM(SampleWindDefs, FpaConfigSampleStruct *,
													NumSampleWindDef);
	SampleWindDefs[NumSampleWindDef-1] = INITMEM(FpaConfigSampleStruct, 1);

	/* Initialize SampleWindDefs structure */
	xdef              = SampleWindDefs[NumSampleWindDef - 1];
	xdef->name        = strdup(name);
	xdef->valid       = TRUE;
	xdef->label       = strdup(name);
	xdef->sh_label    = strdup(name);
	xdef->description = NullString;
	xdef->samp_name   = NullString;
	xdef->samp_type   = FpaCnoMacro;
	xdef->samp_func   = NullString;

	/* Add the name as another identifier */
	(void) add_windtype_sample_identifier(name, xdef);

	/* Return pointer to SampleWindDefs structure */
	return xdef;
	}

/***********************************************************************
*                                                                      *
*   a d d _ w i n d t y p e _ s a m p l e _ i d e n t i f i e r        *
*                                                                      *
*   Add another identifier to sample identifier list for winds.        *
*   Note that sample name comparisons are case insensitive!            *
*                                                                      *
***********************************************************************/

static	void					add_windtype_sample_identifier

	(
	STRING					ident,		/* sample identifier for winds */
	FpaConfigSampleStruct	*xdef		/* pointer to Sample structure
											for winds */
	)

	{

	/* Add identifier to list */
	NumSampleWindIdent++;
	SampleWindIdents = GETMEM(SampleWindIdents, FPAC_IDENTS, NumSampleWindIdent);
	SampleWindIdents[NumSampleWindIdent-1].ident = strdup(ident);
	SampleWindIdents[NumSampleWindIdent-1].pdef  = (POINTER) xdef;

	/* Sort the list */
	(void) qsort((POINTER) SampleWindIdents, (size_t) NumSampleWindIdent,
			sizeof(FPAC_IDENTS), compare_identifiers_ic);
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Section Checking)                       *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

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

static	int			push_section

	(
	int			section		/* section identifier */
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

static	int			pop_section

	(
	)

	{

	/* Return immediately if no section identifiers in list */
	if ( NumSectionIds < 1 )  return FpaCnoSection;

	/* Return last section identifier from list */
	NumSectionIds--;
	if ( NumSectionIds >= 1 ) return SectionIds[NumSectionIds-1];
	else                      return FpaCnoSection;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Miscellaneous Functions)                *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*   c o n f i g _ f i l e _ m e s s a g e                              *
*                                                                      *
*   This function prints error messages for configuration files.       *
*                                                                      *
***********************************************************************/

static	void		config_file_message

	(
	STRING				block,		/* block name */
	STRING				name,		/* name of block member */
	STRING				xname,		/* name of another block member
										or sub section of block */
	STRING				keyword,	/* keyword or keyword value in block */
	FpaCmessageOption	message		/* message indicator */
	)

	{
	STRING		cfgname;

	/* Print error message based on message indicator */
	switch ( message )
		{

		/* Error in name of block member ... already an alias in lists! */
		case FpaCmsgName:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"\n", block, name);
			(void) pr_error("Config",
					"     \"%s\" is an alias of \"%s\"!\n", name, xname);
			return;

		/* Warning ... alias is already in lists! */
		case FpaCmsgAlias:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_warning("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_warning("Config",
					"Block: \"%s\"    Name: \"%s\"    Alias: \"%s\"\n",
					block, name, keyword);
			(void) pr_warning("Config",
					"     \"%s\" is an alias of \"%s\"!\n", keyword, xname);
			return;

		/* Error ... unrecognizable field! */
		case FpaCmsgField:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) && blank(keyword) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"\n", block, name);
			else if ( blank(keyword) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
						block, name, xname);
			else if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Unrecognizable field!\n");
			return;

		/* Error ... unrecognizable crossreference field! */
		case FpaCmsgCRef:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
					block, name, FpaCcrossRefFields);
			(void) pr_error("Config",
					"     Unrecognizable crossreference field: \"%s\"\n",
					xname);
			return;

		/* Error ... unrecognizable linking field! */
		case FpaCmsgLinkFld:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
					block, name, FpaClinking);
			(void) pr_error("Config",
					"     Unrecognizable link field: \"%s\"\n",
					xname);
			return;

		/* Error in section identifier ... layout is wrong! */
		case FpaCmsgSection:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"\n",
						block, name);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
						block, name, xname);
			(void) pr_error("Config",
					"     Layout of this section may be wrong!\n");
			return;

		/* Error ... unrecognized keyword! */
		case FpaCmsgKeyword:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Unknown (or misplaced) keyword \"%s\"!\n",
					keyword);
			return;

		/* Error ... missing equal sign! */
		case FpaCmsgNoEqual:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Missing \"=\" in \"%s\" line!\n",
					keyword);
			return;

		/* Error ... unacceptable parameter! */
		case FpaCmsgParameter:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Unacceptable parameter(s) for keyword \"%s\"!\n",
					keyword);
			return;

		/* Error in directory source tag ... not found in setup file! */
		case FpaCmsgDirTag:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
					block, name, keyword);
			(void) pr_error("Config",
					"     Directory tag \"%s\" not found in setup file!\n",
					xname);
			return;

		/* Error ... trying to reset a fixed parameter! */
		case FpaCmsgReset:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Cannot reset keyword \"%s\"!\n", keyword);
			return;

		/* Error ... trying to reset an allocated block! */
		case FpaCmsgResetNone:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
					block, name, keyword);
			(void) pr_error("Config",
					"     Cannot reset keyword \"%s\" to None!\n", keyword);
			return;

		/* Error ... trying to reset a field override! */
		case FpaCmsgResetDefault:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
					block, name, keyword);
			(void) pr_error("Config",
					"     Cannot reset keyword \"%s\" to Default!\n",
					keyword);
			return;

		/* Error ... missing line in config files! */
		case FpaCmsgMissLine:
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Missing \"%s\" line in config files!\n",
					keyword);
			return;

		/* Error ... missing section in config files! */
		case FpaCmsgMissSection:
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"\n",
						block, name);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
						block, name, xname);
			(void) pr_error("Config",
					"     Missing \"%s\" section in config files!\n",
					keyword);
			return;

		/* Error ... missing name! */
		case FpaCmsgMissName:
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"\n",
						block, name);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
						block, name, xname);
			(void) pr_error("Config",
					"     Name is missing from config files!\n");
			return;

		/* Warning ... choice is not presently supported! */
		case FpaCmsgSupport:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_warning("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_warning("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_warning("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_warning("Config",
					"     Choice is not presently supported!\n");
			return;

		/* Warning ... making choices from list is not presently supported! */
		case FpaCmsgMSupport:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_warning("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_warning("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_warning("Config",
					"     Making choices from list is not presently supported!\n");
			return;

		/* Error ... problem adding member to list! */
		case FpaCmsgMember:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Member: \"%s\"\n",
					block, name, xname, keyword);
			(void) pr_error("Config",
					"     Problem adding member \"%s\" to \"%s\" list!\n",
					keyword, xname);
			return;

		/* Error ... parameter does not match list! */
		case FpaCmsgMatch:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Parameter in \"%s\" line is not in allowed list!\n",
					keyword);
			return;

		/* Error ... Merge field type does not match! */
		case FpaCmsgMergeType:
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
					block, name, keyword);
			(void) pr_error("Config",
					"     Merge element \"%s\" has incorrect field type!\n",
					xname);
			return;

		/* Error ... Merge field units do not match! */
		case FpaCmsgMergeUnits:
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
					block, name, keyword);
			(void) pr_error("Config",
					"     Merge element \"%s\" has incompatible units!\n",
					xname);
			return;

		/* Error ... invalid data! */
		case FpaCmsgInvalid:
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"\n",
						block, name);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"\n",
						block, name, xname);
			(void) pr_error("Config",
					"     Invalid data in config files!\n");
			return;

		/* Error ... Obsolete keyword! */
		case FpaCmsgObsolete:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Obsolete keyword in config files!\n");
			(void) pr_error("Config",
					"     Please contact the FPA Development Group!\n");
			return;

		/* Error ... Obsolete keyword! (Replaced by ...) */
		case FpaCmsgReplace:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			(void) pr_error("Config",
					"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
					block, name, keyword);
			(void) pr_error("Config",
					"     Obsolete keyword in config files!\n");
			(void) pr_error("Config",
					"   Replace keyword: \"%s\"  with: \"%s\"\n",
					keyword, xname);
			return;

		/* Error in block name ... unrecognized block name! */
		case FpaCmsgBlock:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(keyword) )
				(void) pr_error("Config",
						"Block: \"%s\"\n", block);
			else
				(void) pr_error("Config",
						"Block: \"%s %s\"\n", block, keyword);
			if ( blank(name) )
				(void) pr_error("Config",
						"     Unrecognized block name!\n");
			else
				(void) pr_error("Config",
						"     Unrecognized block name! (After block: \"%s\")\n",
						name);
			return;

		/* Unknown error message */
		default:
			(void) config_file_location(NullPtr(FILE *), &cfgname, NullLong);
			(void) pr_error("Config", "Config file: \"%s\"\n", cfgname);
			if ( blank(xname) )
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Keyword: \"%s\"\n",
						block, name, keyword);
			else
				(void) pr_error("Config",
						"Block: \"%s\"    Name: \"%s\"    Section: \"%s\"    Keyword: \"%s\"\n",
						block, name, xname, keyword);
			(void) pr_error("Config",
					"     Error in \"%s\" line!\n", keyword);
			return;
		}
	}

/***********************************************************************
*                                                                      *
*   c o n f i g _ f i l e _ m a c r o                                  *
*                                                                      *
*   This function returns an enumerated type macro for a given name    *
*   which should be a member of the FPA macro structure passed.        *
*   Note that name comparisons are case insensitive!                   *
*                                                                      *
***********************************************************************/

static	int			config_file_macro

	(
	STRING					name,		/* name to match */
	int						nmacro,		/* number of enumerated type macros */
	const FPA_MACRO_LIST	*macros		/* structure containing enumerated
											type macros and their labels   */
	)

	{
	int		nn;

	/* Check name against each label in the structure */
	for ( nn=0; nn<nmacro; nn++ )
		{
		if ( same_ic(name, macros[nn].label) ) return macros[nn].macro;
		}

	/* Return FpaCnoMacro if name not found in structure */
	return FpaCnoMacro;
	}

/***********************************************************************
*                                                                      *
*   c o m p a r e _ i d e n t i f i e r s                              *
*   c o m p a r e _ i d e n t i f i e r s _ i c                        *
*                                                                      *
*   These functions compare identifiers in a list of FPAC_IDENTS       *
*   for searching with bsearch() or sorting with qsort().              *
*   Note that identifier comparisons in the  ..._ic  function are      *
*   case insensitive!                                                  *
*                                                                      *
*   c o m p a r e _ f i e l d _ i d e n t i f i e r s                  *
*                                                                      *
*   This function compares identifiers in a list of FPAC_FIELD_IDENTS  *
*   for searching with bsearch() or sorting with qsort().              *
*   Note that identifier comparisons are case insensitive!             *
*                                                                      *
***********************************************************************/

static	int			compare_identifiers

	(
	const void	*idlist1,	/* pointer to structure containing
								first identifier */
	const void	*idlist2	/* pointer to structure containing
								second identifier */
	)

	{

	/* Error returns for missing identifiers */
	if ( IsNull(idlist1) )                          return  1;
	if ( IsNull(((FPAC_IDENTS *) idlist1)->ident) ) return  1;
	if ( IsNull(idlist2) )                          return -1;
	if ( IsNull(((FPAC_IDENTS *) idlist2)->ident) ) return -1;

	/* Compare structures based on identifiers */
	return strcmp(((FPAC_IDENTS *) idlist1)->ident,
						((FPAC_IDENTS*) idlist2)->ident);
	}

/**********************************************************************/

static	int			compare_identifiers_ic

	(
	const void	*idlist1,	/* pointer to structure containing
							first identifier */
	const void	*idlist2	/* pointer to structure containing
							second identifier */
	)

	{

	/* Error returns for missing identifiers */
	if ( IsNull(idlist1) )                          return  1;
	if ( IsNull(((FPAC_IDENTS *) idlist1)->ident) ) return  1;
	if ( IsNull(idlist2) )                          return -1;
	if ( IsNull(((FPAC_IDENTS *) idlist2)->ident) ) return -1;

	/* Compare structures based on identifiers ... ignoring case! */
	return strcasecmp(((FPAC_IDENTS *) idlist1)->ident,
						((FPAC_IDENTS*) idlist2)->ident);
	}

/**********************************************************************/

static	int			compare_field_identifiers

	(
	const void	*idlist1,	/* pointer to structure containing
								first field identifier */
	const void	*idlist2	/* pointer to structure containing
								second field identifier */
	)

	{
	int		cmp;

	/* Error returns for missing identifiers */
	if ( IsNull(idlist1) )                                  return  1;
	if ( IsNull(((FPAC_FIELD_IDENTS *) idlist1)->element) ) return  1;
	if ( IsNull(((FPAC_FIELD_IDENTS *) idlist1)->level) )   return  1;
	if ( IsNull(idlist2) )                                  return -1;
	if ( IsNull(((FPAC_FIELD_IDENTS *) idlist2)->element) ) return -1;
	if ( IsNull(((FPAC_FIELD_IDENTS *) idlist2)->level) )   return -1;

	/* Compare structures based on element names ... ignoring case! */
	cmp = strcasecmp(((FPAC_FIELD_IDENTS *) idlist1)->element,
						((FPAC_FIELD_IDENTS*) idlist2)->element);

	/* Compare structures based on level names ... ignoring case! */
	if ( cmp == 0 ) cmp = strcasecmp(((FPAC_FIELD_IDENTS *) idlist1)->level,
										((FPAC_FIELD_IDENTS*) idlist2)->level);

	/* Return comparison */
	return cmp;
	}

/***********************************************************************
*                                                                      *
*   f i l e _ i d e n t _ f o r m a t                                  *
*                                                                      *
*   This function checks the format for file identifiers in the        *
*   Elements or Levels block for length and appropriate characters.    *
*                                                                      *
***********************************************************************/

static	LOGICAL		file_ident_format

	(
	STRING	ident,	/* file identifier to check */
	int		maxlen	/* maximum length of file identifier */
	)

	{
	char	*cp, c;

	/* Check for length of file identifier */
	if ( (int) strlen(ident) > maxlen )
		{
		(void) pr_error("Config",
				"[file_ident_format] File identifier \"%s\" longer than %d characters!\n",
				ident, maxlen);
		return FALSE;
		}

	/* Check for format of characters in file identifier                 */
	/*  ... which can only be alphanumeric character, dash or underscore */
	cp = ident;
	while ( (c = *(cp++)) != '\0' )
		{
		if ( isalnum(c) )    continue;
		else if ( c == '-' ) continue;
		else if ( c == '_' ) continue;
		else
			{
			(void) pr_error("Config",
					"  File identifier \"%s\" can only contain alphanumeric characters, dashes or underscores!\n",
					ident);
			return FALSE;
			}
		}

	/* Return TRUE if file identifier is acceptable */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     STATIC (LOCAL) ROUTINES (Testing routines)                       *
*                                                                      *
*     All the routines after this point are available only within      *
*     this file.                                                       *
*                                                                      *
***********************************************************************/

#ifdef CONFIG_STANDALONE

/**********************************************************************
 *** routine to test identify_field                                 ***
 **********************************************************************/

static	void	test_identify_field

	(
	STRING		element,	/* field element */
	STRING		level		/* field level */
	)

	{
	FpaConfigFieldStruct		*fdef;

	(void) fprintf(stdout, "  Field: %s %s\n", element, level);
	if ( NotNull( fdef = identify_field(element, level) ) )
		{
		(void) fprintf(stdout, "    Name: %s %s",
				fdef->element->name, fdef->level->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				fdef->label, fdef->sh_label);
		(void) fprintf(stdout, "      Pointers: <%d> <%d> <%d>\n",
				fdef, fdef->element, fdef->level);
		if ( NotNull(fdef->group) )
			{
			(void) fprintf(stdout, "      Field Group: %s\n",
					fdef->group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Field Group\n");
			}
		(void) fprintf(stdout, "      Field type: %d",
				fdef->element->fld_type);
		(void) fprintf(stdout, "    Display format: %d\n",
				fdef->element->display_format);
		if ( NotNull(fdef->element->group) )
			{
			(void) fprintf(stdout, "      Element Group: %s\n",
					fdef->element->group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Element Group\n");
			}
		(void) fprintf(stdout, "      Level Types: %d %d\n",
				fdef->element->lvl_type, fdef->level->lvl_type);
		(void) fprintf(stdout, "      File Ids: \"%s\" \"%s\"",
				fdef->element->elem_io->fident, fdef->level->lev_io->fident);
		(void) fprintf(stdout, " (Old: \"%s\" \"%s\")\n",
				fdef->element->elem_io->fid, fdef->level->lev_io->fid);
		(void) fprintf(stdout, "      Precision: %g",
				fdef->element->elem_io->precision);
		if ( NotNull(fdef->element->elem_io->units) )
			{
			(void) fprintf(stdout, "    Units: %s\n",
					fdef->element->elem_io->units->name);
			}
		else
			{
			(void) fprintf(stdout, "    No Units\n");
			}
		if ( fdef->element->elem_tdep->time_dep == FpaC_DAILY )
			{
			(void) fprintf(stdout, "      Time Dependence: Daily");
			(void) fprintf(stdout, "   Normal Time: %g  Begin Time: %g",
					fdef->element->elem_tdep->normal_time,
					fdef->element->elem_tdep->begin_time);
			(void) fprintf(stdout, "  End Time: %g  Units: %s\n",
					fdef->element->elem_tdep->end_time,
					fdef->element->elem_tdep->units->name);
			}
		else if ( fdef->element->elem_tdep->time_dep == FpaC_STATIC )
			{
			(void) fprintf(stdout, "      Time Dependence: Static\n");
			}
		else
			{
			(void) fprintf(stdout, "      Time Dependence: Normal\n");
			}
		if ( NotNull(fdef->level->lev_lvls) )
			{
			(void) fprintf(stdout, "      Level category: %d    Level: %s",
					fdef->level->lev_lvls->lvl_category,
					fdef->level->lev_lvls->lvl);
			(void) fprintf(stdout, "    Upper Level: %s    Lower Level: %s\n",
					fdef->level->lev_lvls->uprlvl,
					fdef->level->lev_lvls->lwrlvl);
			}
		else
			{
			(void) fprintf(stdout, "      No Levels information\n");
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for field\n");
		}
	}

/**********************************************************************
 *** routine to test get_field_info                                 ***
 **********************************************************************/

static	void	test_get_field_info

	(
	STRING		element,	/* field element */
	STRING		level		/* field level */
	)

	{
	int									nn, macro, nt;
	FpaConfigFieldStruct				*fdef;
	FpaConfigElementLineTypeStruct		*ltypes;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementAttribStruct		*attrib;
	FpaConfigElementEditorStruct		*editor;
	FpaConfigElementLabellingStruct		*labelling;
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigElementLinkingStruct		*linking;

	(void) fprintf(stdout, "  Field name: %s %s\n", element, level);
	if ( NotNull( fdef = get_field_info(element, level) ) )
		{
		(void) fprintf(stdout, "    Name: %s %s",
				fdef->element->name, fdef->level->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				fdef->label, fdef->sh_label);
		(void) fprintf(stdout, "      Pointers: <%d> <%d> <%d>\n",
				fdef, fdef->element, fdef->level);
		if ( NotNull(fdef->group) )
			{
			(void) fprintf(stdout, "      Field Group: %s\n",
					fdef->group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Field Group\n");
			}
		(void) fprintf(stdout, "      Field type: %d",
				fdef->element->fld_type);
		(void) fprintf(stdout, "    Display format: %d\n",
				fdef->element->display_format);
		if ( NotNull(fdef->element->group) )
			{
			(void) fprintf(stdout, "      Element Group: %s\n",
					fdef->element->group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Element Group\n");
			}
		(void) fprintf(stdout, "      Level Types: %d %d\n",
				fdef->element->lvl_type, fdef->level->lvl_type);
		(void) fprintf(stdout, "      File Ids: \"%s\" \"%s\"",
				fdef->element->elem_io->fident, fdef->level->lev_io->fident);
		(void) fprintf(stdout, " (Old: \"%s\" \"%s\")\n",
				fdef->element->elem_io->fid, fdef->level->lev_io->fid);
		(void) fprintf(stdout, "      Precision: %g",
				fdef->element->elem_io->precision);
		if ( NotNull(fdef->element->elem_io->units) )
			{
			(void) fprintf(stdout, "    Units: %s\n",
					fdef->element->elem_io->units->name);
			}
		else
			{
			(void) fprintf(stdout, "    No Units\n");
			}
		if ( fdef->element->elem_tdep->time_dep == FpaC_DAILY )
			{
			(void) fprintf(stdout, "      Time Dependence: Daily");
			(void) fprintf(stdout, "   Normal Time: %g  Begin Time: %g",
					fdef->element->elem_tdep->normal_time,
					fdef->element->elem_tdep->begin_time);
			(void) fprintf(stdout, "  End Time: %g  Units: %s\n",
					fdef->element->elem_tdep->end_time,
					fdef->element->elem_tdep->units->name);
			}
		else if ( fdef->element->elem_tdep->time_dep == FpaC_STATIC )
			{
			(void) fprintf(stdout, "      Time Dependence: Static\n");
			}
		else
			{
			(void) fprintf(stdout, "      Time Dependence: Normal\n");
			}
		if ( NotNull(fdef->element->elem_detail) )
			{
			(void) fprintf(stdout, "      Wind class: %d\n",
					fdef->element->elem_detail->wd_class);

			/* Line type information */
			if ( NotNull(fdef->element->elem_detail->line_types) )
				{
				ltypes = fdef->element->elem_detail->line_types;

				(void) fprintf(stdout, "      Number of Line Types: %d\n",
						ltypes->ntypes);
				for (nn=0; nn<ltypes->ntypes; nn++)
					{
					(void) fprintf(stdout, "        Line Type: %s",
							ltypes->type_names[nn]);
					(void) fprintf(stdout, "    Type label: %s",
							ltypes->type_labels[nn]);
					(void) fprintf(stdout, "    Short label: %s\n",
							ltypes->type_sh_labels[nn]);
					(void) fprintf(stdout, "          Pattern File: %s\n",
							ltypes->patterns[nn]);
					}
				}
			else if ( fdef->element->fld_type == FpaC_LINE )
				{
				(void) fprintf(stdout, "      No Line Type information\n");
				}

			/* Scattered type information */
			if ( NotNull(fdef->element->elem_detail->scattered_types) )
				{
				stypes = fdef->element->elem_detail->scattered_types;

				(void) fprintf(stdout, "      Number of Scattered Types: %d\n",
						stypes->ntypes);
				for (nn=0; nn<stypes->ntypes; nn++)
					{
					(void) fprintf(stdout, "        Scattered Type: %s",
							stypes->type_names[nn]);
					(void) fprintf(stdout, "    Type label: %s",
							stypes->type_labels[nn]);
					(void) fprintf(stdout, "    Short label: %s\n",
							stypes->type_sh_labels[nn]);
					(void) fprintf(stdout, "          Entry File: %s",
							stypes->type_entry_files[nn]);
					(void) fprintf(stdout, "    Modify File: %s\n",
							stypes->type_modify_files[nn]);
					(void) fprintf(stdout, "          Style Class: %s",
							stypes->type_classes[nn]);
					(void) fprintf(stdout, "    Attach: %d\n",
							stypes->type_attach_opts[nn]);
					(void) fprintf(stdout, "          Number of Default Attributes: %d\n",
							stypes->type_attribs[nn].nattrib_defs);
					for (nt=0; nt<stypes->type_attribs[nn].nattrib_defs; nt++)
						{
						(void) fprintf(stdout, "            Default attribute: %s",
								stypes->type_attribs[nn].attrib_def_names[nt]);
						(void) fprintf(stdout, "    Value: %s\n",
								stypes->type_attribs[nn].attrib_def_values[nt]);
						}
					if ( stypes->type_rules[nn].nrules > 0 )
						{
						(void) fprintf(stdout, "          Number of Entry Rules: %d\n",
								stypes->type_rules[nn].nrules);
						(void) fprintf(stdout, "          Entry Rules:");
						for (nt=0; nt<stypes->type_rules[nn].nrules; nt++)
							(void) fprintf(stdout, "  %s",
									stypes->type_rules[nn].entry_rules[nt]);
						(void) fprintf(stdout, "\n");
						}
					}
				}
			else if ( fdef->element->fld_type == FpaC_SCATTERED )
				{
				(void) fprintf(stdout, "      No Scattered Type information\n");
				}

			/* Attribute information */
			if ( NotNull(fdef->element->elem_detail->attributes) )
				{
				attrib = fdef->element->elem_detail->attributes;

				(void) fprintf(stdout, "      Number of Attributes: %d\n",
						attrib->nattribs);
				for (nn=0; nn<attrib->nattribs; nn++)
					{
					(void) fprintf(stdout, "        Attribute name: %s",
							attrib->attrib_names[nn]);
					(void) fprintf(stdout, "    Label: %s    Short Label: %s",
							attrib->attrib_labels[nn],
							attrib->attrib_sh_labels[nn]);
					(void) fprintf(stdout, "    Default: %s\n",
							attrib->attrib_back_defs[nn]);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Attribute information\n");
				}

			/* Editor information */
			if ( NotNull(fdef->element->elem_detail->editor) )
				{
				(void) fprintf(stdout, "      Editor information\n");
				editor = fdef->element->elem_detail->editor;
				switch ( fdef->element->fld_type )
					{

					case FpaC_CONTINUOUS:
						if ( editor->type.continuous->hilo )
							(void) fprintf(stdout, "        HiLo: T");
						else
							(void) fprintf(stdout, "        HiLo: F");
						(void) fprintf(stdout, "    Poke: %g",
								editor->type.continuous->poke);
						if ( NotNull(editor->type.continuous->units) )
							{
							(void) fprintf(stdout, "    Units: %s\n",
									editor->type.continuous->units->name);
							}
						else
							{
							(void) fprintf(stdout, "    No Units\n");
							}
						if ( !blank(editor->type.continuous->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.continuous->entry_file);
						if ( !blank(editor->type.continuous->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.continuous->modify_file);
						if ( !blank(editor->type.continuous->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.continuous->memory_file);
						if ( !blank(editor->type.continuous->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.continuous->back_entry_file);
						if ( !blank(editor->type.continuous->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.continuous->back_memory_file);
						if ( editor->type.continuous->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.continuous->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.continuous->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.continuous->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.continuous->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.continuous->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.continuous->nmerge; nn++)
								{
								if ( NotNull(editor->type.continuous->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.continuous->merge_elems[nn]->name,
											editor->type.continuous->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.continuous->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_VECTOR:
						if ( editor->type.vector->hilo )
							(void) fprintf(stdout, "        HiLo: T");
						else
							(void) fprintf(stdout, "        HiLo: F");
						(void) fprintf(stdout, "    Magnitude Poke: %g",
								editor->type.vector->mag_poke);
						if ( NotNull(editor->type.vector->mag_units) )
							{
							(void) fprintf(stdout, "    Units: %s\n",
									editor->type.vector->mag_units->name);
							}
						else
							{
							(void) fprintf(stdout, "    No Units\n");
							}
						(void) fprintf(stdout, "    Direction Poke: %g",
								editor->type.vector->dir_poke);
						if ( NotNull(editor->type.vector->dir_units) )
							{
							(void) fprintf(stdout, "    Units: %s\n",
									editor->type.vector->dir_units->name);
							}
						else
							{
							(void) fprintf(stdout, "    No Units\n");
							}
						if ( !blank(editor->type.vector->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.vector->entry_file);
						if ( !blank(editor->type.vector->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.vector->modify_file);
						if ( !blank(editor->type.vector->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.vector->memory_file);
						if ( !blank(editor->type.vector->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.vector->back_entry_file);
						if ( !blank(editor->type.vector->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.vector->back_memory_file);
						if ( editor->type.vector->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.vector->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.vector->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.vector->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.vector->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.vector->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.vector->nmerge; nn++)
								{
								if ( NotNull(editor->type.vector->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.vector->merge_elems[nn]->name,
											editor->type.vector->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.vector->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_DISCRETE:
						if ( editor->type.discrete->overlaying )
							(void) fprintf(stdout, "        Overlaying: T");
						else
							(void) fprintf(stdout, "        Overlaying: F");
						if ( editor->type.discrete->display_order )
							(void) fprintf(stdout, "     Display Order: T\n");
						else
							(void) fprintf(stdout, "     Display Order: F\n");
						if ( !blank(editor->type.discrete->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.discrete->entry_file);
						if ( !blank(editor->type.discrete->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.discrete->modify_file);
						if ( !blank(editor->type.discrete->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.discrete->memory_file);
						if ( !blank(editor->type.discrete->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.discrete->back_entry_file);
						if ( !blank(editor->type.discrete->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.discrete->back_memory_file);
						if ( editor->type.discrete->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.discrete->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.discrete->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.discrete->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.discrete->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.discrete->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.discrete->nmerge; nn++)
								{
								if ( NotNull(editor->type.discrete->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.discrete->merge_elems[nn]->name,
											editor->type.discrete->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.discrete->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_WIND:
						if ( editor->type.wind->display_order )
							(void) fprintf(stdout, "        Display Order: T\n");
						else
							(void) fprintf(stdout, "        Display Order: F\n");
						if ( !blank(editor->type.wind->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.wind->entry_file);
						if ( !blank(editor->type.wind->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.wind->modify_file);
						if ( !blank(editor->type.wind->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.wind->memory_file);
						if ( !blank(editor->type.wind->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.wind->back_entry_file);
						if ( !blank(editor->type.wind->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.wind->back_memory_file);
						if ( editor->type.wind->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.wind->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.wind->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.wind->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.wind->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.wind->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.wind->nmerge; nn++)
								{
								if ( NotNull(editor->type.wind->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.wind->merge_elems[nn]->name,
											editor->type.wind->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.wind->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_LINE:
						if ( !blank(editor->type.line->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.line->entry_file);
						if ( !blank(editor->type.line->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.line->modify_file);
						if ( !blank(editor->type.line->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.line->memory_file);
						if ( !blank(editor->type.line->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.line->back_entry_file);
						if ( !blank(editor->type.line->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.line->back_memory_file);
						if ( editor->type.line->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.line->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.line->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.line->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.line->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.line->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.line->nmerge; nn++)
								{
								if ( NotNull(editor->type.line->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.line->merge_elems[nn]->name,
											editor->type.line->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.line->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_SCATTERED:
						if ( !blank(editor->type.scattered->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.scattered->entry_file);
						if ( !blank(editor->type.scattered->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.scattered->modify_file);
						if ( !blank(editor->type.scattered->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.scattered->memory_file);
						if ( !blank(editor->type.scattered->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.scattered->back_entry_file);
						if ( !blank(editor->type.scattered->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.scattered->back_memory_file);
						if ( editor->type.scattered->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.scattered->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.scattered->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.scattered->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.scattered->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.scattered->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.scattered->nmerge; nn++)
								{
								if ( NotNull(editor->type.scattered->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.scattered->merge_elems[nn]->name,
											editor->type.scattered->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.scattered->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_LCHAIN:
						if ( !blank(editor->type.lchain->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.lchain->entry_file);
						if ( !blank(editor->type.lchain->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.lchain->modify_file);
						if ( !blank(editor->type.lchain->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.lchain->memory_file);
						if ( !blank(editor->type.lchain->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.lchain->back_entry_file);
						if ( !blank(editor->type.lchain->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.lchain->back_memory_file);
						if ( editor->type.lchain->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.lchain->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.lchain->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.lchain->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( !blank(editor->type.lchain->node_entry_file) )
							(void) fprintf(stdout, "        Node Entry File: %s\n",
									editor->type.lchain->node_entry_file);
						if ( !blank(editor->type.lchain->node_modify_file) )
							(void) fprintf(stdout, "        Node Modify File: %s\n",
									editor->type.lchain->node_modify_file);
						if ( editor->type.lchain->nnode_rules > 0 )
							{
							(void) fprintf(stdout, "        Number of Node Entry Rules: %d",
									editor->type.lchain->nnode_rules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Node Entry Rules:");
							for (nn=0; nn<editor->type.lchain->nnode_rules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.lchain->node_entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.lchain->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.lchain->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.lchain->nmerge; nn++)
								{
								if ( NotNull(editor->type.lchain->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.lchain->merge_elems[nn]->name,
											editor->type.lchain->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.lchain->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.lchain->nlink > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Link Fields: %d",
									editor->type.lchain->nlink);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Link Fields:");
							for (nn=0; nn<editor->type.lchain->nlink; nn++)
								{
								if ( NotNull(editor->type.lchain->link_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.lchain->link_elems[nn]->name,
											editor->type.lchain->link_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.lchain->link_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.lchain->minterp > 0 )
							{
							(void) fprintf(stdout, "        Interpolation delta: %d minute\n",
									editor->type.lchain->minterp);
							}
						break;
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Editor information\n");
				}

			/* Labelling information */
			if ( NotNull(fdef->element->elem_detail->labelling) )
				{
				labelling = fdef->element->elem_detail->labelling;
				(void) fprintf(stdout, "      Number of Labelling Types: %d\n",
						labelling->ntypes);
				for (nn=0; nn<labelling->ntypes; nn++)
					{
					(void) fprintf(stdout, "        Labelling type: %s",
							labelling->type_names[nn]);
					(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
							labelling->type_labels[nn],
							labelling->type_sh_labels[nn]);
					(void) fprintf(stdout, "          Entry File: %s",
							labelling->type_entry_files[nn]);
					(void) fprintf(stdout, "    Modify File: %s\n",
							labelling->type_modify_files[nn]);
					(void) fprintf(stdout, "          Style Class: %s",
							labelling->type_classes[nn]);
					(void) fprintf(stdout, "    Attach: %d\n",
							labelling->type_attach_opts[nn]);
					(void) fprintf(stdout, "          Number of Default Attributes: %d\n",
							labelling->type_attribs[nn].nattrib_defs);
					for (nt=0; nt<labelling->type_attribs[nn].nattrib_defs; nt++)
						{
						(void) fprintf(stdout, "            Default attribute: %s",
								labelling->type_attribs[nn].attrib_def_names[nt]);
						(void) fprintf(stdout, "    Value: %s\n",
								labelling->type_attribs[nn].attrib_def_values[nt]);
						}
					if ( labelling->type_rules[nn].nrules > 0 )
						{
						(void) fprintf(stdout, "          Number of Entry Rules: %d\n",
								labelling->type_rules[nn].nrules);
						(void) fprintf(stdout, "          Entry Rules:");
						for (nt=0; nt<labelling->type_rules[nn].nrules; nt++)
							(void) fprintf(stdout, "  %s",
									labelling->type_rules[nn].entry_rules[nt]);
						(void) fprintf(stdout, "\n");
						}
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Labelling Types\n");
				}

			/* Sampling information */
			if ( NotNull(fdef->element->elem_detail->sampling) )
				{
				sampling = fdef->element->elem_detail->sampling;
				switch ( fdef->element->fld_type )
					{

					case FpaC_CONTINUOUS:
						(void) fprintf(stdout, "      Number of Value samples: %d",
								sampling->type.continuous->nsample);
						(void) fprintf(stdout, "    Value Samples:");
						for (nn=0; nn<sampling->type.continuous->nsample; nn++)
							(void) fprintf(stdout, " %s",
									sampling->type.continuous->samples[nn]->name);
						(void) fprintf(stdout, "\n");
						if ( sampling->type.continuous->nwindsamp > 0 )
							{
							(void) fprintf(stdout, "      Number of Wind samples: %d",
									sampling->type.continuous->nwindsamp);
							(void) fprintf(stdout, "    Wind Samples:");
							for (nn=0; nn<sampling->type.continuous->nwindsamp; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.continuous->windsamps[nn]->name);
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_VECTOR:
						(void) fprintf(stdout, "      Number of Value samples: %d",
								sampling->type.vector->nsample);
						(void) fprintf(stdout, "    Value Samples:");
						for (nn=0; nn<sampling->type.vector->nsample; nn++)
							(void) fprintf(stdout, " %s",
									sampling->type.vector->samples[nn]->name);
						(void) fprintf(stdout, "\n");
						if ( sampling->type.vector->nwindsamp > 0 )
							{
							(void) fprintf(stdout, "      Number of Wind samples: %d",
									sampling->type.vector->nwindsamp);
							(void) fprintf(stdout, "    Wind Samples:");
							for (nn=0; nn<sampling->type.vector->nwindsamp; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.vector->windsamps[nn]->name);
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_DISCRETE:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.discrete->nsattribs);
						if ( sampling->type.discrete->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.discrete->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.discrete->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;

					case FpaC_WIND:
						(void) fprintf(stdout, "      Number of Value samples: %d",
								sampling->type.wind->nsample);
						(void) fprintf(stdout, "    Value Samples:");
						for (nn=0; nn<sampling->type.wind->nsample; nn++)
							(void) fprintf(stdout, " %s",
									sampling->type.wind->samples[nn]->name);
						(void) fprintf(stdout, "\n");
						if ( sampling->type.wind->nwcref > 0 )
							{
							(void) fprintf(stdout, "      Number of Wind crossrefs: %d",
									sampling->type.wind->nwcref);
							(void) fprintf(stdout, "    Wind crossrefs:");
							for (nn=0; nn<sampling->type.wind->nwcref; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.wind->wcrefs[nn]->name);
							(void) fprintf(stdout, "\n");
							}
						(void) fprintf(stdout, "      Wind Sample: %s\n",
									sampling->type.wind->windsample->name);
						break;

					case FpaC_LINE:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.line->nsattribs);
						if ( sampling->type.line->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.line->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.line->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;

					case FpaC_SCATTERED:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.scattered->nsattribs);
						if ( sampling->type.scattered->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.scattered->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.scattered->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;

					case FpaC_LCHAIN:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.lchain->nsattribs);
						if ( sampling->type.lchain->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.lchain->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.lchain->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Samples\n");
				}

			/* Linking information */
			if ( NotNull(fdef->element->elem_detail->linking) )
				{
				linking = fdef->element->elem_detail->linking;
				switch ( fdef->element->fld_type )
					{

					case FpaC_CONTINUOUS:
					case FpaC_VECTOR:
					case FpaC_DISCRETE:
					case FpaC_WIND:
					case FpaC_LINE:
					case FpaC_SCATTERED:
					case FpaC_LCHAIN:
						if ( linking->minterp > 0 || linking->nlink > 0 )
							{
							if ( linking->minterp > 0 )
								(void) fprintf(stdout, "      Interpolation delta: %d\n",
										linking->minterp);
							if ( linking->nlink > 0 )
								{
								(void) fprintf(stdout, "      Number of Link Fields: %d\n",
										linking->nlink);
								(void) fprintf(stdout, "      Link Fields:");
								for (nn=0; nn<linking->nlink; nn++)
									{
									if ( NotNull(linking->link_levels[nn]) )
										(void) fprintf(stdout, "  \"%s %s\"",
												linking->link_elems[nn]->name,
												linking->link_levels[nn]->name);
									else
										(void) fprintf(stdout, "  \"%s -\"",
												linking->link_elems[nn]->name);
									}
								(void) fprintf(stdout, "\n");
								}
							}
						break;
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Linking information\n");
				}

			/* Equation information */
			if ( NotNull(fdef->element->elem_detail->equation) )
				{
				(void) fprintf(stdout, "      Equation string: %s",
						fdef->element->elem_detail->equation->eqtn);
				if ( NotNull(fdef->element->elem_detail->equation->units) )
					{
					(void) fprintf(stdout, "    Units: %s\n",
							fdef->element->elem_detail->equation->units->name);
					}
				else
					{
					(void) fprintf(stdout, "    No Units\n");
					}
				if ( fdef->element->elem_detail->equation->force )
					(void) fprintf(stdout, "        Force equation calculation: T\n");
				else
					(void) fprintf(stdout, "        Force equation calculation: F\n");
				}
			else
				{
				(void) fprintf(stdout, "      No Equation information\n");
				}

			/* Value calculation information */
			if ( NotNull(fdef->element->elem_detail->valcalc) )
				{
				(void) fprintf(stdout, "      Value calculation information ...\n");
				if ( fdef->element->elem_detail->valcalc->force )
					(void) fprintf(stdout, "        Force calculation: T\n");
				else
					(void) fprintf(stdout, "        Force calculation: F\n");
				if ( NotNull(fdef->element->elem_detail->valcalc->vcalc) )
					{
					(void) fprintf(stdout, "        Cross reference: %s\n",
							fdef->element->elem_detail->valcalc->vcalc->name);
					}
				if ( fdef->element->elem_detail->valcalc->nsrc_type > 0 )
					{
					(void) fprintf(stdout, "        Number of crossref source types: %d",
							fdef->element->elem_detail->valcalc->nsrc_type);
					(void) fprintf(stdout, "    Source types:");
					for (nn=0; nn<fdef->element->elem_detail->valcalc->nsrc_type; nn++)
						(void) fprintf(stdout, "  %d",
								fdef->element->elem_detail->valcalc->src_types[nn]);
					(void) fprintf(stdout, "\n");
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Value calculation information\n");
				}

			/* Component information */
			if ( NotNull(fdef->element->elem_detail->components) )
				{
				(void) fprintf(stdout, "      Required component type (combined): %d\n",
							fdef->element->elem_detail->components->cinfo->need);
				(void) fprintf(stdout, "      Number of components: %d",
							fdef->element->elem_detail->components->ncomp);
				for (nn=0; nn<fdef->element->elem_detail->components->ncomp; nn++)
					{
					(void) fprintf(stdout, "    Name/Type: %s %d",
							fdef->element->elem_detail->components->comp_edefs[nn]->name,
							fdef->element->elem_detail->components->comp_types[nn]);
					}
				(void) fprintf(stdout, "\n");
				}
			else
				{
				(void) fprintf(stdout, "      No Components\n");
				}
			}
		else
			{
			(void) fprintf(stdout, "      No Detailed Element information\n");
			}
		if ( NotNull(fdef->level->lev_lvls) )
			{
			(void) fprintf(stdout, "      Level category: %d    Level: %s",
					fdef->level->lev_lvls->lvl_category,
					fdef->level->lev_lvls->lvl);
			(void) fprintf(stdout, "    Upper Level: %s    Lower Level: %s\n",
					fdef->level->lev_lvls->uprlvl,
					fdef->level->lev_lvls->lwrlvl);
			}
		else
			{
			(void) fprintf(stdout, "      No Levels information\n");
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for field\n");
		}
	}

/**********************************************************************
 *** routine to test identify_fields_by_group                       ***
 **********************************************************************/

static	void	test_identify_fields_by_group

	(
	STRING		group		/* field group */
	)

	{
	int							num, nn;
	FpaConfigFieldStruct		**fdefs;

	(void) fprintf(stdout, "  Field group: %s\n", group);
	num = identify_fields_by_group(group, &fdefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of Fields: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s %s",
					fdefs[nn]->element->name, fdefs[nn]->level->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					fdefs[nn]->label, fdefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointers: <%d> <%d> <%d>\n",
					fdefs[nn], fdefs[nn]->element, fdefs[nn]->level);
			if ( NotNull(fdefs[nn]->group) )
				{
				(void) fprintf(stdout, "        Group: %s",
						fdefs[nn]->group->name);
				}
			else
				{
				(void) fprintf(stdout, "        No Group");
				}
			(void) fprintf(stdout, "    File Ids: \"%s\" \"%s\"",
				fdefs[nn]->element->elem_io->fident,
				fdefs[nn]->level->lev_io->fident);
			(void) fprintf(stdout, " (Old: \"%s\" \"%s\")\n",
				fdefs[nn]->element->elem_io->fid,
				fdefs[nn]->level->lev_io->fid);
			(void) fprintf(stdout, "        Precision: %g",
				fdefs[nn]->element->elem_io->precision);
			if ( NotNull(fdefs[nn]->element->elem_io->units) )
				{
				(void) fprintf(stdout, "    Units: %s\n",
					fdefs[nn]->element->elem_io->units->name);
				}
			else
				{
				(void) fprintf(stdout, "    No Units\n");
				}
			if ( fdefs[nn]->element->elem_tdep->time_dep == FpaC_DAILY )
				{
				(void) fprintf(stdout, "        Time Dependence: Daily");
				(void) fprintf(stdout, "   Normal Time: %g  Begin Time: %g",
						fdefs[nn]->element->elem_tdep->normal_time,
						fdefs[nn]->element->elem_tdep->begin_time);
				(void) fprintf(stdout, "  End Time: %g  Units: %s\n",
						fdefs[nn]->element->elem_tdep->end_time,
						fdefs[nn]->element->elem_tdep->units->name);
				}
			else if ( fdefs[nn]->element->elem_tdep->time_dep == FpaC_STATIC )
				{
				(void) fprintf(stdout, "        Time Dependence: Static\n");
				}
			else
				{
				(void) fprintf(stdout, "        Time Dependence: Normal\n");
				}
			}
		num = identify_fields_by_group_free(&fdefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No Fields for this group\n");
		}
	}

/**********************************************************************
 *** routine to test identify_element                               ***
 **********************************************************************/

static	void	test_identify_element

	(
	STRING		name		/* element name */
	)

	{
	int							nn, nalias;
	STRING						*elist;
	FpaConfigElementStruct		*edef;

	(void) fprintf(stdout, "  Element name: %s\n", name);
	if ( NotNull( edef = identify_element(name) ) )
		{
		(void) fprintf(stdout, "    Name: %s",
				edef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				edef->label, edef->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", edef);
		if ( NotNull(edef->group) )
			{
			(void) fprintf(stdout, "      Group: %s",
					edef->group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Group");
			}
		(void) fprintf(stdout, "    Level Type: %d",
				edef->lvl_type);
		(void) fprintf(stdout, "    Field type: %d",
				edef->fld_type);
		(void) fprintf(stdout, "    Display format: %d\n",
				edef->display_format);
		(void) fprintf(stdout, "      Alias Names:");
		nalias = identify_element_aliases(edef, &elist);
		for ( nn=0; nn<nalias; nn++)
			{
			(void) fprintf(stdout, " %s", elist[nn]);
			}
		(void) fprintf(stdout, "\n");
		nalias = identify_element_aliases_free(&elist, nalias);
		if ( NotNull(edef->elem_io) )
			{
			(void) fprintf(stdout, "      File Id: %s (Old: %s)\n",
					edef->elem_io->fident, edef->elem_io->fid);
			(void) fprintf(stdout, "      Precision: %g",
					edef->elem_io->precision);
			if ( NotNull(edef->elem_io->units) )
				{
				(void) fprintf(stdout, "    Units: %s\n",
						edef->elem_io->units->name);
				}
			else
				{
				(void) fprintf(stdout, "    No Units\n");
				}
			}
		if ( edef->elem_tdep->time_dep == FpaC_DAILY )
			{
			(void) fprintf(stdout, "      Time Dependence: Daily");
			(void) fprintf(stdout, "   Normal Time: %g  Begin Time: %g",
					edef->elem_tdep->normal_time,
					edef->elem_tdep->begin_time);
			(void) fprintf(stdout, "  End Time: %g  Units: %s\n",
					edef->elem_tdep->end_time,
					edef->elem_tdep->units->name);
			}
		else if ( edef->elem_tdep->time_dep == FpaC_STATIC )
			{
			(void) fprintf(stdout, "      Time Dependence: Static\n");
			}
		else
			{
			(void) fprintf(stdout, "      Time Dependence: Normal\n");
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for element\n");
		}
	}

/**********************************************************************
 *** routine to test get_element_info                               ***
 **********************************************************************/

static	void	test_get_element_info

	(
	STRING		name		/* element name */
	)

	{
	int									nn, macro, nt;
	FpaConfigElementStruct				*edef;
	FpaConfigElementLineTypeStruct		*ltypes;
	FpaConfigElementScatteredTypeStruct	*stypes;
	FpaConfigElementAttribStruct		*attrib;
	FpaConfigElementEditorStruct		*editor;
	FpaConfigElementLabellingStruct		*labelling;
	FpaConfigElementSamplingStruct		*sampling;
	FpaConfigElementLinkingStruct		*linking;

	(void) fprintf(stdout, "  Element name: %s\n", name);
	if ( NotNull( edef = get_element_info(name) ) )
		{
		(void) fprintf(stdout, "    Name: %s",
				edef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				edef->label, edef->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", edef);
		if ( NotNull(edef->fld_group) )
			{
			(void) fprintf(stdout, "      Field Group: %s\n",
					edef->fld_group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Field Group\n");
			}
		if ( NotNull(edef->group) )
			{
			(void) fprintf(stdout, "      Element Group: %s\n",
					edef->group->name);
			}
		else
			{
			(void) fprintf(stdout, "      No Element Group\n");
			}
		(void) fprintf(stdout, "      Level Type: %d",
				edef->lvl_type);
		(void) fprintf(stdout, "    Field type: %d",
				edef->fld_type);
		(void) fprintf(stdout, "    Display format: %d\n",
				edef->display_format);
		if ( NotNull(edef->elem_io) )
			{
			(void) fprintf(stdout, "      File Id: %s (Old: %s)\n",
					edef->elem_io->fident, edef->elem_io->fid);
			(void) fprintf(stdout, "      Precision: %g",
					edef->elem_io->precision);
			if ( NotNull(edef->elem_io->units) )
				{
				(void) fprintf(stdout, "    Units: %s\n",
						edef->elem_io->units->name);
				}
			else
				{
				(void) fprintf(stdout, "    No Units\n");
				}
			}
		if ( edef->elem_tdep->time_dep == FpaC_DAILY )
			{
			(void) fprintf(stdout, "      Time Dependence: Daily");
			(void) fprintf(stdout, "   Normal Time: %g  Begin Time: %g",
					edef->elem_tdep->normal_time,
					edef->elem_tdep->begin_time);
			(void) fprintf(stdout, "  End Time: %g  Units: %s\n",
					edef->elem_tdep->end_time,
					edef->elem_tdep->units->name);
			}
		else if ( edef->elem_tdep->time_dep == FpaC_STATIC )
			{
			(void) fprintf(stdout, "      Time Dependence: Static\n");
			}
		else
			{
			(void) fprintf(stdout, "      Time Dependence: Normal\n");
			}
		if ( NotNull(edef->elem_detail) )
			{
			(void) fprintf(stdout, "      Wind class: %d\n",
					edef->elem_detail->wd_class);

			/* Line Type information */
			if ( NotNull(edef->elem_detail->line_types) )
				{
				ltypes = edef->elem_detail->line_types;

				(void) fprintf(stdout, "      Number of Line Types: %d\n",
						ltypes->ntypes);
				for (nn=0; nn<ltypes->ntypes; nn++)
					{
					(void) fprintf(stdout, "        Line Type: %s",
							ltypes->type_names[nn]);
					(void) fprintf(stdout, "    Type label: %s",
							ltypes->type_labels[nn]);
					(void) fprintf(stdout, "    Short label: %s\n",
							ltypes->type_sh_labels[nn]);
					(void) fprintf(stdout, "          Pattern File: %s\n",
							ltypes->patterns[nn]);
					}
				}
			else if ( edef->fld_type == FpaC_LINE )
				{
				(void) fprintf(stdout, "      No Line Type information\n");
				}

			/* Scattered Type information */
			if ( NotNull(edef->elem_detail->scattered_types) )
				{
				stypes = edef->elem_detail->scattered_types;

				(void) fprintf(stdout, "      Number of Scattered Types: %d\n",
						stypes->ntypes);
				for (nn=0; nn<stypes->ntypes; nn++)
					{
					(void) fprintf(stdout, "        Scattered Type: %s",
							stypes->type_names[nn]);
					(void) fprintf(stdout, "    Type label: %s",
							stypes->type_labels[nn]);
					(void) fprintf(stdout, "    Short label: %s\n",
							stypes->type_sh_labels[nn]);
					(void) fprintf(stdout, "          Entry File: %s",
							stypes->type_entry_files[nn]);
					(void) fprintf(stdout, "    Modify File: %s\n",
							stypes->type_modify_files[nn]);
					(void) fprintf(stdout, "          Style Class: %s",
							stypes->type_classes[nn]);
					(void) fprintf(stdout, "    Attach: %d\n",
							stypes->type_attach_opts[nn]);
					(void) fprintf(stdout, "          Number of Default Attributes: %d\n",
							stypes->type_attribs[nn].nattrib_defs);
					for (nt=0; nt<stypes->type_attribs[nn].nattrib_defs; nt++)
						{
						(void) fprintf(stdout, "            Default attribute: %s",
								stypes->type_attribs[nn].attrib_def_names[nt]);
						(void) fprintf(stdout, "    Value: %s\n",
								stypes->type_attribs[nn].attrib_def_values[nt]);
						}
					if ( stypes->type_rules[nn].nrules > 0 )
						{
						(void) fprintf(stdout, "          Number of Entry Rules: %d\n",
								stypes->type_rules[nn].nrules);
						(void) fprintf(stdout, "          Entry Rules:");
						for (nt=0; nt<stypes->type_rules[nn].nrules; nt++)
							(void) fprintf(stdout, "  %s",
									stypes->type_rules[nn].entry_rules[nt]);
						(void) fprintf(stdout, "\n");
						}
					}
				}
			else if ( edef->fld_type == FpaC_SCATTERED )
				{
				(void) fprintf(stdout, "      No Scattered Type information\n");
				}

			/* Attribute information */
			if ( NotNull(edef->elem_detail->attributes) )
				{
				attrib = edef->elem_detail->attributes;

				(void) fprintf(stdout, "      Number of Attributes: %d\n",
						attrib->nattribs);
				for (nn=0; nn<attrib->nattribs; nn++)
					{
					(void) fprintf(stdout, "        Attribute name: %s",
							attrib->attrib_names[nn]);
					(void) fprintf(stdout, "    Label: %s    Short Label: %s",
							attrib->attrib_labels[nn],
							attrib->attrib_sh_labels[nn]);
					(void) fprintf(stdout, "    Default: %s\n",
							attrib->attrib_back_defs[nn]);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Attribute information\n");
				}

			/* Editor information */
			if ( NotNull(edef->elem_detail->editor) )
				{
				(void) fprintf(stdout, "      Editor information\n");
				editor = edef->elem_detail->editor;
				switch ( edef->fld_type )
					{

					case FpaC_CONTINUOUS:
						if ( editor->type.continuous->hilo )
							(void) fprintf(stdout, "        HiLo: T");
						else
							(void) fprintf(stdout, "        HiLo: F");
						(void) fprintf(stdout, "    Poke: %g",
								editor->type.continuous->poke);
						if ( NotNull(editor->type.continuous->units) )
							{
							(void) fprintf(stdout, "    Units: %s\n",
									editor->type.continuous->units->name);
							}
						else
							{
							(void) fprintf(stdout, "    No Units\n");
							}
						if ( !blank(editor->type.continuous->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.continuous->entry_file);
						if ( !blank(editor->type.continuous->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.continuous->modify_file);
						if ( !blank(editor->type.continuous->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.continuous->memory_file);
						if ( !blank(editor->type.continuous->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.continuous->back_entry_file);
						if ( !blank(editor->type.continuous->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.continuous->back_memory_file);
						if ( editor->type.continuous->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.continuous->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.continuous->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.continuous->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.continuous->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.continuous->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.continuous->nmerge; nn++)
								{
								if ( NotNull(editor->type.continuous->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.continuous->merge_elems[nn]->name,
											editor->type.continuous->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.continuous->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_VECTOR:
						if ( editor->type.vector->hilo )
							(void) fprintf(stdout, "        HiLo: T");
						else
							(void) fprintf(stdout, "        HiLo: F");
						(void) fprintf(stdout, "    Magnitude Poke: %g",
								editor->type.vector->mag_poke);
						if ( NotNull(editor->type.vector->mag_units) )
							{
							(void) fprintf(stdout, "    Units: %s\n",
									editor->type.vector->mag_units->name);
							}
						else
							{
							(void) fprintf(stdout, "    No Units\n");
							}
						(void) fprintf(stdout, "    Direction Poke: %g",
								editor->type.vector->dir_poke);
						if ( NotNull(editor->type.vector->dir_units) )
							{
							(void) fprintf(stdout, "    Units: %s\n",
									editor->type.vector->dir_units->name);
							}
						else
							{
							(void) fprintf(stdout, "    No Units\n");
							}
						if ( !blank(editor->type.vector->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.vector->entry_file);
						if ( !blank(editor->type.vector->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.vector->modify_file);
						if ( !blank(editor->type.vector->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.vector->memory_file);
						if ( !blank(editor->type.vector->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.vector->back_entry_file);
						if ( !blank(editor->type.vector->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.vector->back_memory_file);
						if ( editor->type.vector->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.vector->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.vector->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.vector->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.vector->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.vector->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.vector->nmerge; nn++)
								{
								if ( NotNull(editor->type.vector->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.vector->merge_elems[nn]->name,
											editor->type.vector->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.vector->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_DISCRETE:
						if ( editor->type.discrete->overlaying )
							(void) fprintf(stdout, "        Overlaying: T");
						else
							(void) fprintf(stdout, "        Overlaying: F");
						if ( editor->type.discrete->display_order )
							(void) fprintf(stdout, "     Display Order: T\n");
						else
							(void) fprintf(stdout, "     Display Order: F\n");
						if ( !blank(editor->type.discrete->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.discrete->entry_file);
						if ( !blank(editor->type.discrete->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.discrete->modify_file);
						if ( !blank(editor->type.discrete->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.discrete->memory_file);
						if ( !blank(editor->type.discrete->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.discrete->back_entry_file);
						if ( !blank(editor->type.discrete->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.discrete->back_memory_file);
						if ( editor->type.discrete->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.discrete->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.discrete->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.discrete->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.discrete->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.discrete->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.discrete->nmerge; nn++)
								{
								if ( NotNull(editor->type.discrete->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.discrete->merge_elems[nn]->name,
											editor->type.discrete->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.discrete->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_WIND:
						if ( editor->type.wind->display_order )
							(void) fprintf(stdout, "        Display Order: T\n");
						else
							(void) fprintf(stdout, "        Display Order: F\n");
						if ( !blank(editor->type.wind->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.wind->entry_file);
						if ( !blank(editor->type.wind->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.wind->modify_file);
						if ( !blank(editor->type.wind->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.wind->memory_file);
						if ( !blank(editor->type.wind->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.wind->back_entry_file);
						if ( !blank(editor->type.wind->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.wind->back_memory_file);
						if ( editor->type.wind->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.wind->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.wind->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.wind->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.wind->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.wind->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.wind->nmerge; nn++)
								{
								if ( NotNull(editor->type.wind->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.wind->merge_elems[nn]->name,
											editor->type.wind->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.wind->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_LINE:
						if ( !blank(editor->type.line->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.line->entry_file);
						if ( !blank(editor->type.line->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.line->modify_file);
						if ( !blank(editor->type.line->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.line->memory_file);
						if ( !blank(editor->type.line->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.line->back_entry_file);
						if ( !blank(editor->type.line->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.line->back_memory_file);
						if ( editor->type.line->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.line->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.line->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.line->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.line->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.line->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.line->nmerge; nn++)
								{
								if ( NotNull(editor->type.line->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.line->merge_elems[nn]->name,
											editor->type.line->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.line->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_SCATTERED:
						if ( !blank(editor->type.scattered->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.scattered->entry_file);
						if ( !blank(editor->type.scattered->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.scattered->modify_file);
						if ( !blank(editor->type.scattered->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.scattered->memory_file);
						if ( !blank(editor->type.scattered->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.scattered->back_entry_file);
						if ( !blank(editor->type.scattered->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.scattered->back_memory_file);
						if ( editor->type.scattered->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.scattered->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.scattered->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.scattered->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.scattered->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.scattered->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.scattered->nmerge; nn++)
								{
								if ( NotNull(editor->type.scattered->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.scattered->merge_elems[nn]->name,
											editor->type.scattered->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.scattered->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_LCHAIN:
						if ( !blank(editor->type.lchain->entry_file) )
							(void) fprintf(stdout, "        Entry File: %s\n",
									editor->type.lchain->entry_file);
						if ( !blank(editor->type.lchain->modify_file) )
							(void) fprintf(stdout, "        Modify File: %s\n",
									editor->type.lchain->modify_file);
						if ( !blank(editor->type.lchain->memory_file) )
							(void) fprintf(stdout, "        Memory File: %s\n",
									editor->type.lchain->memory_file);
						if ( !blank(editor->type.lchain->back_entry_file) )
							(void) fprintf(stdout, "        Background Entry File: %s\n",
									editor->type.lchain->back_entry_file);
						if ( !blank(editor->type.lchain->back_memory_file) )
							(void) fprintf(stdout, "        Background Memory File: %s\n",
									editor->type.lchain->back_memory_file);
						if ( editor->type.lchain->nrules > 0 )
							{
							(void) fprintf(stdout, "        Number of Entry Rules: %d",
									editor->type.lchain->nrules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Entry Rules:");
							for (nn=0; nn<editor->type.lchain->nrules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.lchain->entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( !blank(editor->type.lchain->node_entry_file) )
							(void) fprintf(stdout, "        Node Entry File: %s\n",
									editor->type.lchain->node_entry_file);
						if ( !blank(editor->type.lchain->node_modify_file) )
							(void) fprintf(stdout, "        Node Modify File: %s\n",
									editor->type.lchain->node_modify_file);
						if ( editor->type.lchain->nnode_rules > 0 )
							{
							(void) fprintf(stdout, "        Number of Node Entry Rules: %d",
									editor->type.lchain->nnode_rules);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Node Entry Rules:");
							for (nn=0; nn<editor->type.lchain->nnode_rules; nn++)
								(void) fprintf(stdout, "  %s",
										editor->type.lchain->node_entry_rules[nn]);
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.lchain->nmerge > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Fields: %d",
									editor->type.lchain->nmerge);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Fields:");
							for (nn=0; nn<editor->type.lchain->nmerge; nn++)
								{
								if ( NotNull(editor->type.lchain->merge_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.lchain->merge_elems[nn]->name,
											editor->type.lchain->merge_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.lchain->merge_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.lchain->nlink > 0 )
							{
							(void) fprintf(stdout, "        Number of Merge Link Fields: %d",
									editor->type.lchain->nlink);
							(void) fprintf(stdout, "\n");
							(void) fprintf(stdout, "        Merge Link Fields:");
							for (nn=0; nn<editor->type.lchain->nlink; nn++)
								{
								if ( NotNull(editor->type.lchain->link_levels[nn]) )
									(void) fprintf(stdout, "  \"%s %s\"",
											editor->type.lchain->link_elems[nn]->name,
											editor->type.lchain->link_levels[nn]->name);
								else
									(void) fprintf(stdout, "  \"%s -\"",
											editor->type.lchain->link_elems[nn]->name);
								}
							(void) fprintf(stdout, "\n");
							}
						if ( editor->type.lchain->minterp > 0 )
							{
							(void) fprintf(stdout, "        Interpolation delta: %d minute\n",
									editor->type.lchain->minterp);
							}
						break;
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Editor information\n");
				}

			/* Labelling information */
			if ( NotNull(edef->elem_detail->labelling) )
				{
				labelling = edef->elem_detail->labelling;
				(void) fprintf(stdout, "      Number of Labelling Types: %d\n",
						labelling->ntypes);
				for (nn=0; nn<labelling->ntypes; nn++)
					{
					(void) fprintf(stdout, "        Labelling type: %s",
							labelling->type_names[nn]);
					(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
							labelling->type_labels[nn],
							labelling->type_sh_labels[nn]);
					(void) fprintf(stdout, "          Entry File: %s",
							labelling->type_entry_files[nn]);
					(void) fprintf(stdout, "    Modify File: %s\n",
							labelling->type_modify_files[nn]);
					(void) fprintf(stdout, "          Style Class: %s",
							labelling->type_classes[nn]);
					(void) fprintf(stdout, "    Attach: %d\n",
							labelling->type_attach_opts[nn]);
					(void) fprintf(stdout, "          Number of Default Attributes: %d\n",
							labelling->type_attribs[nn].nattrib_defs);
					for (nt=0; nt<labelling->type_attribs[nn].nattrib_defs; nt++)
						{
						(void) fprintf(stdout, "            Default attribute: %s",
								labelling->type_attribs[nn].attrib_def_names[nt]);
						(void) fprintf(stdout, "    Value: %s\n",
								labelling->type_attribs[nn].attrib_def_values[nt]);
						}
					if ( labelling->type_rules[nn].nrules > 0 )
						{
						(void) fprintf(stdout, "          Number of Entry Rules: %d\n",
								labelling->type_rules[nn].nrules);
						(void) fprintf(stdout, "          Entry Rules:");
						for (nt=0; nt<labelling->type_rules[nn].nrules; nt++)
							(void) fprintf(stdout, "  %s",
									labelling->type_rules[nn].entry_rules[nt]);
						(void) fprintf(stdout, "\n");
						}
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Labelling Types\n");
				}

			/* Sampling information */
			if ( NotNull(edef->elem_detail->sampling) )
				{
				sampling = edef->elem_detail->sampling;
				switch ( edef->fld_type )
					{

					case FpaC_CONTINUOUS:
						(void) fprintf(stdout, "      Number of Value samples: %d",
								sampling->type.continuous->nsample);
						(void) fprintf(stdout, "    Value Samples:");
						for (nn=0; nn<sampling->type.continuous->nsample; nn++)
							(void) fprintf(stdout, " %s",
									sampling->type.continuous->samples[nn]->name);
						(void) fprintf(stdout, "\n");
						if ( sampling->type.continuous->nwindsamp > 0 )
							{
							(void) fprintf(stdout, "      Number of Wind samples: %d",
									sampling->type.continuous->nwindsamp);
							(void) fprintf(stdout, "    Wind Samples:");
							for (nn=0; nn<sampling->type.continuous->nwindsamp; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.continuous->windsamps[nn]->name);
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_VECTOR:
						(void) fprintf(stdout, "      Number of Value samples: %d",
								sampling->type.vector->nsample);
						(void) fprintf(stdout, "    Value Samples:");
						for (nn=0; nn<sampling->type.vector->nsample; nn++)
							(void) fprintf(stdout, " %s",
									sampling->type.vector->samples[nn]->name);
						(void) fprintf(stdout, "\n");
						if ( sampling->type.vector->nwindsamp > 0 )
							{
							(void) fprintf(stdout, "      Number of Wind samples: %d",
									sampling->type.vector->nwindsamp);
							(void) fprintf(stdout, "    Wind Samples:");
							for (nn=0; nn<sampling->type.vector->nwindsamp; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.vector->windsamps[nn]->name);
							(void) fprintf(stdout, "\n");
							}
						break;

					case FpaC_DISCRETE:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.discrete->nsattribs);
						if ( sampling->type.discrete->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.discrete->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.discrete->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;

					case FpaC_WIND:
						(void) fprintf(stdout, "      Number of Value samples: %d",
								sampling->type.wind->nsample);
						(void) fprintf(stdout, "    Value Samples:");
						for (nn=0; nn<sampling->type.wind->nsample; nn++)
							(void) fprintf(stdout, " %s",
									sampling->type.wind->samples[nn]->name);
						(void) fprintf(stdout, "\n");
						if ( sampling->type.wind->nwcref > 0 )
							{
							(void) fprintf(stdout, "      Number of Wind crossrefs: %d",
									sampling->type.wind->nwcref);
							(void) fprintf(stdout, "    Wind crossrefs:");
							for (nn=0; nn<sampling->type.wind->nwcref; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.wind->wcrefs[nn]->name);
							(void) fprintf(stdout, "\n");
							}
						(void) fprintf(stdout, "      Wind Sample: %s\n",
									sampling->type.wind->windsample->name);
						break;

					case FpaC_LINE:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.line->nsattribs);
						if ( sampling->type.line->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.line->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.line->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;

					case FpaC_SCATTERED:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.scattered->nsattribs);
						if ( sampling->type.scattered->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.scattered->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.scattered->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;

					case FpaC_LCHAIN:
						(void) fprintf(stdout, "      Number of Attribute samples: %d",
								sampling->type.lchain->nsattribs);
						if ( sampling->type.lchain->nsattribs > 0 )
							{
							(void) fprintf(stdout, "    Attribute Samples:");
							for (nn=0; nn<sampling->type.lchain->nsattribs; nn++)
								(void) fprintf(stdout, " %s",
										sampling->type.lchain->sattrib_names[nn]);
							}
						(void) fprintf(stdout, "\n");
						break;
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Samples\n");
				}

			/* Linking information */
			if ( NotNull(edef->elem_detail->linking) )
				{
				linking = edef->elem_detail->linking;
				switch ( edef->fld_type )
					{

					case FpaC_CONTINUOUS:
					case FpaC_VECTOR:
					case FpaC_DISCRETE:
					case FpaC_WIND:
					case FpaC_LINE:
					case FpaC_SCATTERED:
					case FpaC_LCHAIN:
						if ( linking->minterp > 0 || linking->nlink > 0 )
							{
							if ( linking->minterp > 0 )
								(void) fprintf(stdout, "      Interpolation delta: %d\n",
										linking->minterp);
							if ( linking->nlink > 0 )
								{
								(void) fprintf(stdout, "      Number of Link Fields: %d\n",
										linking->nlink);
								(void) fprintf(stdout, "      Link Fields:");
								for (nn=0; nn<linking->nlink; nn++)
									{
									if ( NotNull(linking->link_levels[nn]) )
										(void) fprintf(stdout, "  \"%s %s\"",
												linking->link_elems[nn]->name,
												linking->link_levels[nn]->name);
									else
										(void) fprintf(stdout, "  \"%s -\"",
												linking->link_elems[nn]->name);
									}
								(void) fprintf(stdout, "\n");
								}
							}
						break;
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Linking\n");
				}

			/* Equation information */
			if ( NotNull(edef->elem_detail->equation) )
				{
				(void) fprintf(stdout, "      Equation string: %s",
						edef->elem_detail->equation->eqtn);
				if ( NotNull(edef->elem_detail->equation->units) )
					{
					(void) fprintf(stdout, "    Units: %s\n",
							edef->elem_detail->equation->units->name);
					}
				else
					{
					(void) fprintf(stdout, "    No Units\n");
					}
				if ( edef->elem_detail->equation->force )
					(void) fprintf(stdout, "        Force equation calculation: T\n");
				else
					(void) fprintf(stdout, "        Force equation calculation: F\n");
				}
			else
				{
				(void) fprintf(stdout, "      No Equation information\n");
				}

			/* Value calculation information */
			if ( NotNull(edef->elem_detail->valcalc) )
				{
				(void) fprintf(stdout, "      Value calculation information ...\n");
				if ( edef->elem_detail->valcalc->force )
					(void) fprintf(stdout, "        Force calculation: T\n");
				else
					(void) fprintf(stdout, "        Force calculation: F\n");
				if ( NotNull(edef->elem_detail->valcalc->vcalc) )
					(void) fprintf(stdout, "        Cross reference: %s\n",
						edef->elem_detail->valcalc->vcalc->name);
				if ( edef->elem_detail->valcalc->nsrc_type > 0 )
					{
					(void) fprintf(stdout, "        Number of crossref source types: %d",
							edef->elem_detail->valcalc->nsrc_type);
					(void) fprintf(stdout, "    Source types:");
					for (nn=0; nn<edef->elem_detail->valcalc->nsrc_type; nn++)
						(void) fprintf(stdout, "  %d",
								edef->elem_detail->valcalc->src_types[nn]);
					(void) fprintf(stdout, "\n");
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Value calculation information\n");
				}

			/* Component information */
			if ( NotNull(edef->elem_detail->components) )
				{
				(void) fprintf(stdout, "      Required component type (combined): %d\n",
							edef->elem_detail->components->cinfo->need);
				(void) fprintf(stdout, "      Number of components: %d",
							edef->elem_detail->components->ncomp);
				for (nn=0; nn<edef->elem_detail->components->ncomp; nn++)
					{
					(void) fprintf(stdout, "    Name/Type: %s %d",
							edef->elem_detail->components->comp_edefs[nn]->name,
							edef->elem_detail->components->comp_types[nn]);
					}
				(void) fprintf(stdout, "\n");
				}
			else
				{
				(void) fprintf(stdout, "      No Components\n");
				}
			}
		else
			{
			(void) fprintf(stdout, "      No Detailed information\n");
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for element\n");
		}
	}

/**********************************************************************
 *** routine to test identify_elements_by_group                     ***
 **********************************************************************/

static	void	test_identify_elements_by_group

	(
	STRING		group		/* element group */
	)

	{
	int							num, nn;
	FpaConfigElementStruct		**edefs;

	(void) fprintf(stdout, "  Element group: %s\n", group);
	num = identify_elements_by_group(group, &edefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of Elements: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					edefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					edefs[nn]->label, edefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", edefs[nn]);
			if ( NotNull(edefs[nn]->fld_group) )
				{
				(void) fprintf(stdout, "        Field Group: %s\n",
						edefs[nn]->fld_group->name);
				}
			else
				{
				(void) fprintf(stdout, "        No Field Group\n");
				}
			(void) fprintf(stdout, "        Field type: %d",
					edefs[nn]->fld_type);
			(void) fprintf(stdout, "    Display format: %d\n",
					edefs[nn]->display_format);
			if ( NotNull(edefs[nn]->group) )
				{
				(void) fprintf(stdout, "        Element Group: %s\n",
						edefs[nn]->group->name);
				}
			else
				{
				(void) fprintf(stdout, "        No Element Group\n");
				}
			(void) fprintf(stdout, "        Level Type: %d\n",
					edefs[nn]->lvl_type);
			if ( NotNull(edefs[nn]->elem_io) )
				{
				(void) fprintf(stdout, "        File Id: %s (Old: %s)\n",
						edefs[nn]->elem_io->fident, edefs[nn]->elem_io->fid);
				(void) fprintf(stdout, "        Precision: %g",
						edefs[nn]->elem_io->precision);
				if ( NotNull(edefs[nn]->elem_io->units) )
					{
					(void) fprintf(stdout, "    Units: %s\n",
							edefs[nn]->elem_io->units->name);
					}
				else
					{
					(void) fprintf(stdout, "    No Units\n");
					}
				}
			if ( edefs[nn]->elem_tdep->time_dep == FpaC_DAILY )
				{
				(void) fprintf(stdout, "        Time Dependence: Daily");
				(void) fprintf(stdout, "   Normal Time: %g  Begin Time: %g",
						edefs[nn]->elem_tdep->normal_time,
						edefs[nn]->elem_tdep->begin_time);
				(void) fprintf(stdout, "  End Time: %g  Units: %s\n",
						edefs[nn]->elem_tdep->end_time,
						edefs[nn]->elem_tdep->units->name);
				}
			else if ( edefs[nn]->elem_tdep->time_dep == FpaC_STATIC )
				{
				(void) fprintf(stdout, "        Time Dependence: Static\n");
				}
			else
				{
				(void) fprintf(stdout, "        Time Dependence: Normal\n");
				}
			}
		num = identify_elements_by_group_free(&edefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No Elements for this group\n");
		}
	}

/**********************************************************************
 *** routine to test identify_level                                 ***
 **********************************************************************/

static	void	test_identify_level

	(
	STRING		name		/* level name */
	)

	{
	int							nn, nalias;
	STRING						*llist;
	FpaConfigLevelStruct		*ldef;

	(void) fprintf(stdout, "  Level name: %s\n", name);
	if ( NotNull( ldef = identify_level(name) ) )
		{
		(void) fprintf(stdout, "    Name: %s",
				ldef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				ldef->label, ldef->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", ldef);
		(void) fprintf(stdout, "      Level Type: %d\n",
				ldef->lvl_type);
		(void) fprintf(stdout, "      Alias Names:");
		nalias = identify_level_aliases(ldef, &llist);
		for ( nn=0; nn<nalias; nn++)
			{
			(void) fprintf(stdout, " %s", llist[nn]);
			}
		(void) fprintf(stdout, "\n");
		nalias = identify_level_aliases_free(&llist, nalias);
		if ( NotNull(ldef->lev_io) )
			{
			(void) fprintf(stdout, "      File Id: %s (Old: %s)",
					ldef->lev_io->fident, ldef->lev_io->fid);
			}
		if ( NotNull(ldef->lev_lvls) )
			{
			(void) fprintf(stdout, "      Category: %d    Level: %s",
					ldef->lev_lvls->lvl_category, ldef->lev_lvls->lvl);
			(void) fprintf(stdout, "    Upper Level: %s    Lower Level: %s\n",
					ldef->lev_lvls->uprlvl, ldef->lev_lvls->lwrlvl);
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for level\n");
		}
	}

/**********************************************************************
 *** routine to test identify_levels_by_type                        ***
 **********************************************************************/

static	void	test_identify_levels_by_type

	(
	int			type		/* enumerated level type */
	)

	{
	int							num, nn;
	FpaConfigLevelStruct		**ldefs;

	(void) fprintf(stdout, "  Enumerated level type: %d\n", type);
	num = identify_levels_by_type(type, &ldefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of Levels: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					ldefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					ldefs[nn]->label, ldefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", ldefs[nn]);
			}
		num = identify_levels_by_type_free(&ldefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No Levels for this type\n");
		}
	}

/**********************************************************************
 *** routine to test identify_level_from_levels                     ***
 **********************************************************************/

static	void	test_identify_level_from_levels

	(
	int			type,		/* enumerated level type */
	STRING		single,		/* single level (msl, sfc, geog, or level) */
	STRING		upper,		/* upper level (of layer) */
	STRING		lower		/* lower level (of layer) */
	)

	{
	FpaConfigLevelStruct		*ldef;

	(void) fprintf(stdout, "  Level type: %d    Single level: %s", type, single);
	(void) fprintf(stdout, "    Upper level: %s    Lower level: %s\n", upper, lower);
	ldef = identify_level_from_levels(type, single, upper, lower);
	if ( NotNull(ldef) )
		{
		(void) fprintf(stdout, "    Level name: %s\n", ldef->name);
		(void) fprintf(stdout, "      Pointer: <%d>\n", ldef);
		}
	else
		{
		(void) fprintf(stdout, "    No Level name for this type and levels \n");
		}
	}

/**********************************************************************
 *** routine to test identify_groups_for_fields/elements            ***
 **********************************************************************/

static	void	test_identify_groups

	(
	)

	{
	int							num, nn;
	FpaConfigGroupStruct		**gdefs;

	(void) fprintf(stdout, "  Field Groups\n");
	num = identify_groups_for_fields(&gdefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of field groups: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					gdefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					gdefs[nn]->label, gdefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", gdefs[nn]);
			}
		num = identify_groups_for_fields_free(&gdefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No field groups\n");
		}

	(void) fprintf(stdout, "  Element Groups\n");
	num = identify_groups_for_elements(&gdefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of element groups: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					gdefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					gdefs[nn]->label, gdefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", gdefs[nn]);
			}
		num = identify_groups_for_elements_free(&gdefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No element groups\n");
		}
	}

/**********************************************************************
 *** routine to test identify_crossref                              ***
 **********************************************************************/

static	void	test_identify_crossref

	(
	STRING		ctype,		/* cross reference type */
	STRING		name		/* cross reference name */
	)

	{
	int							nn;
	FpaConfigCrossRefStruct		*crdef;

	(void) fprintf(stdout, "  Cross reference type: %s    Name: %s\n", ctype, name);
	if ( NotNull(crdef = identify_crossref(ctype, name)) )
		{
		(void) fprintf(stdout, "    Name: %s",
				crdef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				crdef->label, crdef->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", crdef);
		(void) fprintf(stdout, "      Function name: %s\n",
				crdef->func_name);
		if ( NotNull(crdef->unit_t) )
			(void) fprintf(stdout, "      Time weight: %g  %s\n",
					crdef->wgtt, crdef->unit_t->name);
		if ( NotNull(crdef->unit_v) )
			(void) fprintf(stdout, "      Value weight: %g  %s\n",
					crdef->wgtv, crdef->unit_v->name);
		(void) fprintf(stdout, "      Number of cross reference fields: %d\n",
				crdef->nfld);
		for ( nn=0; nn<crdef->nfld; nn++ )
			{
			(void) fprintf(stdout, "        Field: %s  %s\n",
					crdef->flds[nn]->element->name,
					crdef->flds[nn]->level->name);
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for cross reference\n");
		}
	}

/**********************************************************************
 *** routine to test identify_crossrefs_for_winds                   ***
 **********************************************************************/

static	void	test_identify_crossrefs_for_winds

	(
	)

	{
	int							num, nn, nnx;
	FpaConfigCrossRefStruct		**crdefs;

	num = identify_crossrefs_for_winds(&crdefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of Wind CrossRefs: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					crdefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					crdefs[nn]->label, crdefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", crdefs[nn]);
			(void) fprintf(stdout, "        Function name: %s\n",
					crdefs[nn]->func_name);
			(void) fprintf(stdout, "        Number of cross reference fields: %d\n",
					crdefs[nn]->nfld);
			for ( nnx=0; nnx<crdefs[nn]->nfld; nnx++ )
				{
				(void) fprintf(stdout, "          Field: %s  %s\n",
						crdefs[nn]->flds[nnx]->element->name,
						crdefs[nn]->flds[nnx]->level->name);
				}
			}
		num = identify_crossrefs_for_winds_free(&crdefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No Wind CrossRefs\n");
		}
	}

/**********************************************************************
 *** routine to test identify_crossrefs_for_values                  ***
 **********************************************************************/

static	void	test_identify_crossrefs_for_values

	(
	)

	{
	int							num, nn, nnx;
	FpaConfigCrossRefStruct		**crdefs;

	num = identify_crossrefs_for_values(&crdefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of Value CrossRefs: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					crdefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					crdefs[nn]->label, crdefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", crdefs[nn]);
			(void) fprintf(stdout, "        Function name: %s\n",
					crdefs[nn]->func_name);
			if ( NotNull(crdefs[nn]->unit_t) )
				(void) fprintf(stdout, "        Time weight: %g  %s\n",
						crdefs[nn]->wgtt, crdefs[nn]->unit_t->name);
			if ( NotNull(crdefs[nn]->unit_v) )
				(void) fprintf(stdout, "        Value weight: %g  %s\n",
						crdefs[nn]->wgtv, crdefs[nn]->unit_v->name);
			(void) fprintf(stdout, "        Number of cross reference fields: %d\n",
					crdefs[nn]->nfld);
			for ( nnx=0; nnx<crdefs[nn]->nfld; nnx++ )
				{
				(void) fprintf(stdout, "          Field: %s  %s\n",
						crdefs[nn]->flds[nnx]->element->name,
						crdefs[nn]->flds[nnx]->level->name);
				}
			}
		num = identify_crossrefs_for_values_free(&crdefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No Value CrossRefs\n");
		}
	}

/**********************************************************************
 *** routine to test identify_sample                                ***
 **********************************************************************/

static	void	test_identify_sample

	(
	STRING		stype,		/* sample type */
	STRING		name		/* sample name */
	)

	{
	FpaConfigSampleStruct		*xdef;

	(void) fprintf(stdout, "  Sample type: %s    Name: %s\n", stype, name);
	if ( NotNull( xdef = identify_sample(stype, name) ) )
		{
		(void) fprintf(stdout, "    Name: %s",
				xdef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				xdef->label, xdef->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", xdef);
		if ( same(stype, FpaCsamplesValues) )
			{
			(void) fprintf(stdout, "      Sample value calculation Type: %s",
					xdef->samp_name);
			(void) fprintf(stdout, "    Macro: %d\n", xdef->samp_type);
			}
		else if ( same(stype, FpaCsamplesWinds) )
			{
			(void) fprintf(stdout, "      Sample function name: %s\n",
					xdef->samp_func);
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for sample\n");
		}
	}

/**********************************************************************
 *** routine to test identify_source                                ***
 **********************************************************************/

static	void	test_identify_source

	(
	STRING		name,		/* source name */
	STRING		subname		/* subsource name */
	)

	{
	int							nn, nalias;
	STRING						*slist;
	FpaConfigSourceStruct		*sdef;

	if ( blank(subname) )
			(void) fprintf(stdout, "  Source name: %s\n", name);
	else
			(void) fprintf(stdout, "  Source name: %s %s\n", name, subname);
	if ( NotNull( sdef = identify_source(name, subname) ) )
		{
		if ( blank(subname) )
				(void) fprintf(stdout, "    Name: %s", sdef->name);
		else
				(void) fprintf(stdout, "    Name: %s %s",
						sdef->name, sdef->src_sub->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				sdef->src_sub->label, sdef->src_sub->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", sdef);
		(void) fprintf(stdout, "      Source Type: %d", sdef->src_type);
		if ( sdef->minutes_rqd )
			(void) fprintf(stdout, "    Timestamps Use Minutes!\n");
		else
			(void) fprintf(stdout, "\n");
		(void) fprintf(stdout, "      Alias Names:");
		nalias = identify_source_aliases(sdef, &slist);
		for ( nn=0; nn<nalias; nn++)
			{
			(void) fprintf(stdout, " %s", slist[nn]);
			}
		(void) fprintf(stdout, "\n");
		nalias = identify_source_aliases_free(&slist, nalias);
		(void) fprintf(stdout, "      Subsource Names:");
		for ( nn=1; nn<sdef->nsubsrc; nn++)
			{
			(void) fprintf(stdout, " %s", sdef->subsrcs[nn]->name);
			}
		(void) fprintf(stdout, "\n");
		if ( NotNull(sdef->src_io) )
			{
			(void) fprintf(stdout, "      Directory Tag: %s    Path: %s",
					sdef->src_io->src_tag, sdef->src_io->src_path);
			if ( !blank(sdef->src_sub->sub_path) )
					(void) fprintf(stdout, "    Sub Directory Path: %s",
							sdef->src_sub->sub_path);
			(void) fprintf(stdout, "    Layers: %d\n", sdef->src_io->src_layers);
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for source\n");
		}
	}

/**********************************************************************
 *** routine to test get_source_info                                ***
 **********************************************************************/

static	void	test_get_source_info

	(
	STRING		name,		/* source name */
	STRING		subname		/* subsource name */
	)

	{
	int								nn, nna;
	FpaConfigSourceStruct			*sdef;
	FpaConfigAlliedProgramsStruct	*programs;
	FpaConfigAlliedFilesStruct		*files;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigAlliedWindsStruct		*winds;
	FpaConfigAlliedValuesStruct		*values;
	FpaConfigAlliedMetafilesStruct	*metafiles;

	if ( blank(subname) )
			(void) fprintf(stdout, "  Source name: %s\n", name);
	else
			(void) fprintf(stdout, "  Source name: %s %s\n", name, subname);
	if ( NotNull( sdef = get_source_info(name, subname) ) )
		{
		if ( blank(subname) )
				(void) fprintf(stdout, "    Name: %s", sdef->name);
		else
				(void) fprintf(stdout, "    Name: %s %s",
						sdef->name, sdef->src_sub->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				sdef->src_sub->label, sdef->src_sub->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", sdef);
		(void) fprintf(stdout, "      Source Type: %d", sdef->src_type);
		if ( sdef->minutes_rqd )
			(void) fprintf(stdout, "    Timestamps Use Minutes!\n");
		else
			(void) fprintf(stdout, "\n");
		(void) fprintf(stdout, "      Subsource Names:");
		for ( nn=1; nn<sdef->nsubsrc; nn++)
			{
			(void) fprintf(stdout, " %s", sdef->subsrcs[nn]->name);
			}
		(void) fprintf(stdout, "\n");
		if ( NotNull(sdef->src_io) )
			{
			(void) fprintf(stdout, "      Directory Tag: %s    Path: %s",
					sdef->src_io->src_tag, sdef->src_io->src_path);
			if ( !blank(sdef->src_sub->sub_path) )
					(void) fprintf(stdout, "    Sub Directory Path: %s",
							sdef->src_sub->sub_path);
			(void) fprintf(stdout, "    Layers: %d\n", sdef->src_io->src_layers);
			}
		if ( NotNull(sdef->allied) )
			{
			if ( sdef->allied->time_match )
				(void) fprintf(stdout, "      Allied Model time matching: T\n");
			else
				(void) fprintf(stdout, "      Allied Model time matching: F\n");
			/* Process run strings */
			if ( NotNull(sdef->allied->pre_process) )
				(void) fprintf(stdout, "      Allied Model preprocess: %s\n",
						sdef->allied->pre_process);
			if ( NotNull(sdef->allied->process) )
				(void) fprintf(stdout, "      Allied Model process: %s\n",
						sdef->allied->process);
			if ( NotNull(sdef->allied->post_process) )
				(void) fprintf(stdout, "      Allied Model postprocess: %s\n",
						sdef->allied->post_process);

			/* Program information */
			if ( NotNull(sdef->allied->programs) )
				{
				programs = sdef->allied->programs;
				(void) fprintf(stdout, "      Number of Allied Model Programs: %d\n",
						programs->nprogs);
				for ( nn=0; nn<programs->nprogs; nn++)
					{
					(void) fprintf(stdout, "        Alias: %s",
							programs->aliases[nn]);
					(void) fprintf(stdout, "    Directory tag: %s",
							programs->src_tags[nn]);
					(void) fprintf(stdout, "    Path: %s\n",
							programs->prog_paths[nn]);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Allied Model Program information\n");
				}

			/* File information */
			if ( NotNull(sdef->allied->files) )
				{
				files = sdef->allied->files;
				(void) fprintf(stdout, "      Number of Allied Model Files: %d\n",
						files->nfiles);
				for ( nn=0; nn<files->nfiles; nn++)
					{
					(void) fprintf(stdout, "        Alias: %s",
							files->aliases[nn]);
					(void) fprintf(stdout, "    Directory tag: %s",
							files->src_tags[nn]);
					(void) fprintf(stdout, "    Path: %s\n",
							files->file_paths[nn]);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Allied Model File information\n");
				}

			/* Required Fields information */
			if ( NotNull(sdef->allied->fields) )
				{
				fields = sdef->allied->fields;
				(void) fprintf(stdout, "      Number of Required Fields: %d\n",
						fields->nfields);
				for ( nn=0; nn<fields->nfields; nn++)
					{
					(void) fprintf(stdout, "        Alias: %s",
							fields->aliases[nn]);
					(void) fprintf(stdout, "    Field name: %s %s\n",
							fields->flds[nn]->element->name,
							fields->flds[nn]->level->name);
					if ( fields->attinfo[nn]->nattribs > 0 )
						{
						(void) fprintf(stdout,
								"          Information for %d attributes\n",
								fields->attinfo[nn]->nattribs);
						for ( nna=0; nna<fields->attinfo[nn]->nattribs; nna++)
							{
							(void) fprintf(stdout,
									"            Attribute: %d   Tag: %s  Name: %s  Units: %s\n",
									nna, fields->attinfo[nn]->tag[nna],
									fields->attinfo[nn]->attname[nna],
									fields->attinfo[nn]->attunit[nna]->name);
							}
						}
					if ( fields->nodeinfo[nn]->nattribs > 0 )
						{
						(void) fprintf(stdout,
								"          Information for %d node attributes\n",
								fields->nodeinfo[nn]->nattribs);
						for ( nna=0; nna<fields->nodeinfo[nn]->nattribs; nna++)
							{
							(void) fprintf(stdout,
									"            Node attribute: %d   Tag: %s  Name: %s  Units: %s\n",
									nna, fields->nodeinfo[nn]->tag[nna],
									fields->nodeinfo[nn]->attname[nna],
									fields->nodeinfo[nn]->attunit[nna]->name);
							}
						}
					if ( !blank(fields->sub_fields[nn]) )
						{
						(void) fprintf(stdout,
								"          Sub-field name/units: %s %s\n",
								fields->sub_fields[nn],
								fields->sub_units[nn]->name);
						}
					(void) fprintf(stdout,
							"          Source name: %s %s\n",
							fields->src_defs[nn]->name,
							fields->sub_defs[nn]->name);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Required Field information\n");
				}

			/* Required Wind Crossreferences information */
			if ( NotNull(sdef->allied->winds) )
				{
				winds = sdef->allied->winds;
				(void) fprintf(stdout, "      Number of Required Wind Crossrefs: %d\n",
						winds->nwinds);
				for ( nn=0; nn<winds->nwinds; nn++)
					{
					(void) fprintf(stdout, "        Alias: %s",
							winds->aliases[nn]);
					(void) fprintf(stdout, "    Crossref name: %s",
							winds->wcrefs[nn]->name);
					(void) fprintf(stdout, "    Source name: %s %s\n",
							winds->src_defs[nn]->name,
							winds->sub_defs[nn]->name);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Required Wind Crossref information\n");
				}

			/* Required Value Crossreferences information */
			if ( NotNull(sdef->allied->values) )
				{
				values = sdef->allied->values;
				(void) fprintf(stdout, "      Number of Required Value Crossrefs: %d\n",
						values->nvalues);
				for ( nn=0; nn<values->nvalues; nn++)
					{
					(void) fprintf(stdout, "        Alias: %s",
							values->aliases[nn]);
					(void) fprintf(stdout, "    Crossref name: %s",
							values->vcrefs[nn]->name);
					(void) fprintf(stdout, "    Source name: %s %s\n",
							values->src_defs[nn]->name,
							values->sub_defs[nn]->name);
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Required Value Crossref information\n");
				}

			/* Metafile information */
			if ( NotNull(sdef->allied->metafiles) )
				{
				metafiles = sdef->allied->metafiles;
				(void) fprintf(stdout, "      Number of Allied Model Metafiles: %d\n",
						metafiles->nfiles);
				for ( nn=0; nn<metafiles->nfiles; nn++)
					{
					(void) fprintf(stdout, "        Alias: %s",
							metafiles->aliases[nn]);
					(void) fprintf(stdout, "        File alias: %s",
							metafiles->file_aliases[nn]);
					(void) fprintf(stdout, "    Field name: %s %s\n",
							metafiles->flds[nn]->element->name,
							metafiles->flds[nn]->level->name);
					if ( metafiles->attinfo[nn]->nattribs > 0 )
						{
						(void) fprintf(stdout,
								"          Information for %d attributes\n",
								metafiles->attinfo[nn]->nattribs);
						for ( nna=0; nna<metafiles->attinfo[nn]->nattribs; nna++)
							{
							(void) fprintf(stdout,
									"            Attribute: %d   Tag: %s  Name: %s  Units: %s\n",
									nna, metafiles->attinfo[nn]->tag[nna],
									metafiles->attinfo[nn]->attname[nna],
									metafiles->attinfo[nn]->attunit[nna]->name);
							}
						}
					if ( metafiles->definfo[nn]->natt_defs > 0 )
						{
						(void) fprintf(stdout,
								"          Information for %d default attributes\n",
								metafiles->definfo[nn]->natt_defs);
						for ( nna=0; nna<metafiles->definfo[nn]->natt_defs; nna++)
							{
							(void) fprintf(stdout,
									"            Default Attribute: %d   Name: %s  Value: %s\n",
									nna,
									metafiles->definfo[nn]->attname_defs[nna],
									metafiles->definfo[nn]->attval_defs[nna]);
							}
						}
					}
				}
			else
				{
				(void) fprintf(stdout, "      No Allied Model Metafile information\n");
				}
			}
		else
			{
			(void) fprintf(stdout, "      No Allied Model information\n");
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for source\n");
		}
	}

/**********************************************************************
 *** routine to test identify_sources_by_type                       ***
 **********************************************************************/

static	void	test_identify_sources_by_type

	(
	int			type		/* enumerated source type */
	)

	{
	int							num, nn;
	FpaConfigSourceStruct		**sdefs;

	(void) fprintf(stdout, "  Enumerated source type: %d\n", type);
	num = identify_sources_by_type(type, &sdefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "    Number of Sources: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "      Name: %s",
					sdefs[nn]->name);
			(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
					sdefs[nn]->label, sdefs[nn]->sh_label);
			(void) fprintf(stdout, "        Pointer: <%d>\n", sdefs[nn]);
			}
		num = identify_sources_by_type_free(&sdefs, num);
		}
	else
		{
		(void) fprintf(stdout, "    No Sources for this type\n");
		}
	}

/**********************************************************************
 *** routine to test identify_constant                              ***
 **********************************************************************/

static	void	test_identify_constant

	(
	STRING		name		/* constant name */
	)

	{
	FpaConfigConstantStruct		*cdef;

	(void) fprintf(stdout, "  Constant name: %s\n", name);
	if ( NotNull( cdef = identify_constant(name) ) )
		{
		(void) fprintf(stdout, "    Name: %s",
				cdef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				cdef->label, cdef->sh_label);
		(void) fprintf(stdout, "      Pointer: <%d>\n", cdef);
		(void) fprintf(stdout, "      Value: %g\n",
				cdef->value);
		if ( NotNull(cdef->units) )
			{
			(void) fprintf(stdout, "      Units ... Name: %s    MKS: %s",
					cdef->units->name, cdef->units->MKS);
			(void) fprintf(stdout, "    Factor: %g    Offset: %g\n",
					cdef->units->factor, cdef->units->offset);
			}
		else
			{
			(void) fprintf(stdout, "      No Units\n");
			}
		}
	else
		{
		(void) fprintf(stdout, "    No information for constant\n");
		}
	}

/**********************************************************************
 *** routine to test identify_unit                                  ***
 **********************************************************************/

static	void	test_identify_unit

	(
	STRING		name		/* unit name */
	)

	{
	FpaConfigUnitStruct		*udef;

	(void) fprintf(stdout, "  Unit name: %s\n", name);
	if ( NotNull( udef = identify_unit(name) ) )
		{
		(void) fprintf(stdout, "    Name: %s",
				udef->name);
		(void) fprintf(stdout, "    Label: %s    Short Label: %s\n",
				udef->label, udef->sh_label);
		(void) fprintf(stdout, "      MKS: %s    Factor: %g    Offset: %g\n",
				udef->MKS, udef->factor, udef->offset);
		(void) fprintf(stdout, "      Pointer: <%d>\n", udef);
		}
	else
		{
		(void) fprintf(stdout, "    No information for unit\n");
		}
	}

/**********************************************************************
 *** routine to test identify_mks_units and identify_units_by_mks   ***
 **********************************************************************/

static	void	test_identify_mks

	(
	)

	{
	int						num, nn, numu, nnu;
	FpaConfigUnitStruct		**udefs, **udefsu;

	num = identify_mks_units(&udefs);
	if ( num > 0 )
		{
		(void) fprintf(stdout, "  Number of MKS units: %d\n", num);
		for ( nn=0; nn<num; nn++ )
			{
			(void) fprintf(stdout, "    Name: %s",
					udefs[nn]->name);
			(void) fprintf(stdout, "    MKS: %s    Factor: %g    Offset: %g\n",
					udefs[nn]->MKS, udefs[nn]->factor, udefs[nn]->offset);
			(void) fprintf(stdout, "      Pointer: <%d>\n", udefs[nn]);

			numu = identify_units_by_mks(udefs[nn]->MKS, &udefsu);
			(void) fprintf(stdout, "        Units with same MKS: %d\n", numu);
			for ( nnu=0; nnu<numu; nnu++ )
				{
				(void) fprintf(stdout, "          Name: %s",
						udefsu[nnu]->name);
				(void) fprintf(stdout, "    MKS: %s    Factor: %g    Offset: %g\n",
						udefsu[nnu]->MKS, udefsu[nnu]->factor,
						udefsu[nnu]->offset);
				(void) fprintf(stdout, "            Pointer: <%d>\n", udefsu[nnu]);
				}
			numu = identify_units_by_mks_free(&udefsu, numu);
			}
		num = identify_mks_units_free(&udefs, num);
		}
	else
		{
		(void) fprintf(stdout, "  No MKS units\n");
		}
	}

/**********************************************************************
 *** routine to test convert_value                                  ***
 **********************************************************************/

static	void	test_convert_value

	(
	STRING		from,		/* unit name to convert from */
	double		value,		/* value to convert */
	STRING		to			/* unit name to convert to */
	)

	{
	double		newvalue;

	(void) fprintf(stdout, "  Convert: %g   From: %s   To: %s\n", value, from, to);
	if ( convert_value(from, value, to, &newvalue) )
		{
		(void) fprintf(stdout, "    Value: %g\n", newvalue);
		}
	else
		{
		(void) fprintf(stdout, "    Value cannot be converted\n");
		}
	}

/**********************************************************************
 *** main routine to test all routines                              ***
 **********************************************************************/

int		main

(
int			argc,
STRING		*argv
)

{
int			nsetup;
STRING		setupfile, *setuplist, path;
STRING		element, level, crossref, sample, source, subsource, constant, unit;
STRING		single, upper, lower, from, to;

/* Set defaults for CONFIG_STANDALONE */

/* ... First set the default output units */
(void) setvbuf(stdout, NullString, _IOLBF, 0);
(void) setvbuf(stderr, NullString, _IOLBF, 0);

/* ... Next set diagnostic print mode */
(void) pr_control(NULL, 5, 2);

/* ... Next get setup file for testing */
(void) fpalib_license(FpaAccessLib);
setupfile = strdup(argv[1]);
(void) fprintf(stdout, "Setup File: %s\n", setupfile);
nsetup = setup_files(setupfile, &setuplist);
if ( !define_setup(nsetup, setuplist) )
	{
	(void) fprintf(stderr, "Fatal problem with Setup File: %s\n", setupfile);
	return -1;
	}

/* Test for reading complete configuration file */
/* >>>
(void) read_complete_config_file();
*/

/* Testing for identify_field */
(void) fprintf(stdout, "\n\n*** Testing for  identify_field() ***\n");
element = "temperature";		level = "sfc";
(void) test_identify_field(element, level);
element = "sfc";				level = "msl";
(void) test_identify_field(element, level);
element = "Pres";				level = "msl";
(void) test_identify_field(element, level);
element = "model_diff";			level = "msl";
(void) test_identify_field(element, level);
element = "fronts";				level = "surface";
(void) test_identify_field(element, level);
element = "wind";				level = "surface";
(void) test_identify_field(element, level);
element = "freezing_spray";		level = "surface";
(void) test_identify_field(element, level);
element = "pop";				level = "surface";
(void) test_identify_field(element, level);

/* Testing for get_field_info */
(void) fprintf(stdout, "\n\n*** Testing for  get_field_info() ***\n");
element = "temperature";		level = "msl";
(void) test_get_field_info(element, level);
element = "model_diff";			level = "msl";
(void) test_get_field_info(element, level);
element = "fronts";				level = "surface";
(void) test_get_field_info(element, level);
element = "temperature";		level = "Sfc";
(void) test_get_field_info(element, level);
element = "wind";				level = "surface";
(void) test_get_field_info(element, level);
element = "freezing_spray";		level = "surface";
(void) test_get_field_info(element, level);
element = "pop";				level = "surface";
(void) test_get_field_info(element, level);
element = "maxtmp";				level = "surface";
(void) test_get_field_info(element, level);
element = "temp";				level = "500";
(void) test_get_field_info(element, level);
element = "system_wx";			level = "sfc";
(void) test_get_field_info(element, level);
element = "u_thermal";			level = "501k";
(void) test_get_field_info(element, level);
element = "v_wind";				level = "sfc";
(void) test_get_field_info(element, level);
element = "scribe_dailymax";	level = "sfc";
(void) test_get_field_info(element, level);
element = "storm_tracks";		level = "sfc";
(void) test_get_field_info(element, level);

/* Testing for identify_fields_by_group */
(void) fprintf(stdout, "\n\n*** Testing for  identify_fields_by_group() ***\n");
(void) test_identify_fields_by_group("Miscellaneous");
(void) test_identify_fields_by_group("Surface");
(void) test_identify_fields_by_group("Sea");

/* Testing for identify_element */
(void) fprintf(stdout, "\n\n*** Testing for  identify_element() ***\n");
element = "temperature";
(void) test_identify_element(element);
element = "sfc";
(void) test_identify_element(element);
element = "Pres";
(void) test_identify_element(element);
element = "model_diff";
(void) test_identify_element(element);
element = "xt";
(void) test_identify_element(element);

/* Testing for get_element_info */
(void) fprintf(stdout, "\n\n*** Testing for  get_element_info() ***\n");
element = "temperature";
(void) test_get_element_info(element);
element = "model_diff";
(void) test_get_element_info(element);
element = "sfc";
(void) test_get_element_info(element);
element = "pop";
(void) test_get_element_info(element);
element = "fronts";
(void) test_get_element_info(element);
element = "u_wind";
(void) test_get_element_info(element);
element = "pressure";
(void) test_get_element_info(element);
element = "scribe_dailymax";
(void) test_get_element_info(element);

/* Testing for identify_elements_by_group */
(void) fprintf(stdout, "\n\n*** Testing for  identify_elements_by_group() ***\n");
(void) test_identify_elements_by_group("Miscellaneous");
(void) test_identify_elements_by_group(FpaCgenericEqtn);
(void) test_identify_elements_by_group("Pressure");

/* Testing for identify_level */
(void) fprintf(stdout, "\n\n*** Testing for  identify_level() ***\n");
level = "sfc";
(void) test_identify_level(level);
level = "Surface";
(void) test_identify_level(level);
level = "temperature";
(void) test_identify_level(level);
level = "SURFACE";
(void) test_identify_level(level);
level = "500-1000mb";
(void) test_identify_level(level);
level = "MSL";
(void) test_identify_level(level);
level = "100";
(void) test_identify_level(level);
level = "550";
(void) test_identify_level(level);
level = "msL";
(void) test_identify_level(level);

/* Testing for identify_levels_by_type */
(void) fprintf(stdout, "\n\n*** Testing for  identify_levels_by_type() ***\n");
(void) test_identify_levels_by_type(FpaC_MSL);
(void) test_identify_levels_by_type(FpaC_LAYER);
(void) test_identify_levels_by_type(FpaC_LEVEL);
(void) test_identify_levels_by_type(FpaC_SURFACE);
(void) test_identify_levels_by_type(FpaC_GEOGRAPHY);

/* Testing for identify_level_from_levels */
(void) fprintf(stdout, "\n\n*** Testing for  identify_level_from_levels() ***\n");
single = "msL";		upper = "500";		lower = "1000";
(void) test_identify_level_from_levels(FpaC_MSL, single, upper, lower);
(void) test_identify_level_from_levels(FpaC_LAYER, single, upper, lower);
(void) test_identify_level_from_levels(FpaC_LVL_ANY, single, upper, lower);
(void) test_identify_level_from_levels(FpaC_LEVEL, single, upper, lower);
single = "700";
(void) test_identify_level_from_levels(FpaC_LEVEL, single, upper, lower);
single = "sfc";
(void) test_identify_level_from_levels(FpaC_LEVEL, single, upper, lower);
single = "SURFACE";
(void) test_identify_level_from_levels(FpaC_LEVEL, single, upper, lower);

/* Testing for identify_groups_for_fields/elements */
(void) fprintf(stdout, "\n\n*** Testing for  identify_groups_for_fields()");
(void) fprintf(stdout, "  and  identify_groups_for_elements() ***\n");
(void) test_identify_groups();

/* Testing for identify_crossref */
(void) fprintf(stdout, "\n\n*** Testing for  identify_crossref() ***\n");
crossref = "Fpa_Wind";
(void) test_identify_crossref(FpaCcRefsWinds, crossref);
crossref = "vg_msl";
(void) test_identify_crossref(FpaCcRefsWinds, crossref);
crossref = "cardone_sfc";
(void) test_identify_crossref(FpaCcRefsWinds, crossref);
crossref = "pg_msl";
(void) test_identify_crossref(FpaCcRefsWinds, crossref);
crossref = "max_temp_time";
(void) test_identify_crossref(FpaCcRefsValues, crossref);
crossref = "maxtemp";
(void) test_identify_crossref(FpaCcRefsValues, crossref);

/* Testing for identify_crossrefs_for_winds */
(void) fprintf(stdout, "\n\n*** Testing for  identify_crossrefs_for_winds() ***\n");
(void) test_identify_crossrefs_for_winds();

/* Testing for identify_crossrefs_for_values */
(void) fprintf(stdout, "\n\n*** Testing for  identify_crossrefs_for_values() ***\n");
(void) test_identify_crossrefs_for_values();

/* Testing for identify_sample */
(void) fprintf(stdout, "\n\n*** Testing for  identify_sample() ***\n");
sample = "Value_Sample";
(void) test_identify_sample(FpaCsamplesValues, sample);
sample = "Curvature_Sample";
(void) test_identify_sample(FpaCsamplesValues, sample);
sample = "Geostrophic_Wind";
(void) test_identify_sample(FpaCsamplesWinds, sample);
sample = "adjusted_wind";
(void) test_identify_sample(FpaCsamplesWinds, sample);

/* Testing for identify_source */
(void) fprintf(stdout, "\n\n*** Testing for  identify_source() ***\n");
source = "FEM";				subsource = FpaCblank;
(void) test_identify_source(source, subsource);
source = "fem";				subsource = FpaCblank;
(void) test_identify_source(source, subsource);
source = "fem_anal";		subsource = FpaCblank;
(void) test_identify_source(source, subsource);
source = "spectral";		subsource = FpaCblank;
(void) test_identify_source(source, subsource);
source = "donelan";			subsource = FpaCblank;
(void) test_identify_source(source, subsource);
source = "donelan";			subsource = "Lake_Michigan";
(void) test_identify_source(source, subsource);
source = "donelan";			subsource = "Lake_Athabaska";
(void) test_identify_source(source, subsource);
source = "donelan:lake_michigan";		subsource = "";
(void) test_identify_source(source, subsource);
source = "donelan:lake_michigan";		subsource = "lake_mich";
(void) test_identify_source(source, subsource);
source = "donelan:lake_michigan";		subsource = "lake_ontario";
(void) test_identify_source(source, subsource);
source = "donelan:lake_ontario";		subsource = "lake_michigan";
(void) test_identify_source(source, subsource);

/* Testing for get_source_info */
(void) fprintf(stdout, "\n\n*** Testing for  get_source_info() ***\n");
source = "fem";				subsource = FpaCblank;
(void) test_get_source_info(source, subsource);
source = "MaxDailyTemp";	subsource = FpaCblank;
(void) test_get_source_info(source, subsource);
source = "TempCorrected";	subsource = FpaCblank;
(void) test_get_source_info(source, subsource);
source = "donelan";			subsource = FpaCblank;
(void) test_get_source_info(source, subsource);
source = "donelan";			subsource = "Lake_Michigan";
(void) test_get_source_info(source, subsource);
source = "donelan";			subsource = "Lake_Athabaska";
(void) test_get_source_info(source, subsource);
source = "donelan:lake_michigan";		subsource = "";
(void) test_get_source_info(source, subsource);
source = "TestCreateArea";	subsource = FpaCblank;
(void) test_get_source_info(source, subsource);

/* Testing for identify_sources_by_type */
(void) fprintf(stdout, "\n\n*** Testing for  identify_sources_by_type() ***\n");
(void) test_identify_sources_by_type(FpaC_GUIDANCE);

/* Testing for identify_constant */
(void) fprintf(stdout, "\n\n*** Testing for  identify_constant() ***\n");
constant = "RAD";
(void) test_identify_constant(constant);
constant = "ZEROC";
(void) test_identify_constant(constant);
constant = "SOLAR";
(void) test_identify_constant(constant);

/* Testing for identify_mks_units and identify_units_by_mks */
(void) fprintf(stdout, "\n\n*** Testing for  identify_mks_units()");
(void) fprintf(stdout, "  and  identify_units_by_mks() ***\n");
(void) test_identify_mks();

/* Testing for identify_unit */
(void) fprintf(stdout, "\n\n*** Testing for  identify_unit() ***\n");
unit = "m/s";
(void) test_identify_unit(unit);
unit = "Kdegrees";
(void) test_identify_unit(unit);
unit = "kdegrees";
(void) test_identify_unit(unit);
unit = "Dm/3hr";
(void) test_identify_unit(unit);

/* Testing for convert_value */
(void) fprintf(stdout, "\n\n*** Testing for  convert_value() ***\n");
from = "mb";			to = "Pa";
(void) test_convert_value(from, 1000.0, to);
from = "degreesC";		to = "degreesK";
(void) test_convert_value(from, 0.0, to);
from = "degreesC";		to = "MKS";
(void) test_convert_value(from, 0.0, to);
from = "MKS";			to = "degreesC";
(void) test_convert_value(from, 273.0, to);
from = "m/s";			to = "degreesC";
(void) test_convert_value(from, 273.0, to);
from = "MKs";			to = "degreesC";
(void) test_convert_value(from, 273.0, to);

return 0;
}

#endif /* CONFIG_STANDALONE */
