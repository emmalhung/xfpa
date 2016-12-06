/**********************************************************************/
/** @file config_structs.h
 *
 * Defines all structures and keywords necessary for new Version 4.0
 * configuration files (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c o n f i g _ s t r u c t s . h                                    *
*                                                                      *
*   Defines all structures and keywords necessary for new Version 4.0  *
*    configuration files (include file)                                *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*	  Version 7 (c) Copyright 2006 Environment Canada				   *
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
************************************************************************/

#ifndef CONFIG_STRUCTS_DEFS
#define CONFIG_STRUCTS_DEFS

/* We need definitions for low level types and other Objects */
#include <fpa_types.h>
#include <objects/objects.h>


/***********************************************************************
*                                                                      *
*  Defining keywords and pre-defined values in FPA directory tree      *
*                                                                      *
************************************************************************/
 /* Define directory MACROS used in FPA setup file */
#define	FpaDir_Depict		"depict"
#define	FpaDir_Interp		"interp"
#define	FpaDir_Backup		"backup"
#define	FpaDir_Maps			"Maps"
#define	FpaDir_CommonMaps	"maps.common"

 /* Define filename MACROS used in FPA directory tree structure */
#define	FpaFile_Dstamp		"Dstamp"
#define	FpaFile_Prev		"Prev"
#define	FpaFile_Links		"Links"
#define	FpaFile_Scratch		"Scratch"
#define	FpaFile_ShuffleLock	".SHUFFLE"
#define	FpaFile_FileLock	".FILES-"

/* Define delimiter in FPA  <source>:<subsource>  strings */
#define	FpaFsourceDelimiter		":"


/* New format FPA metafile names */

/* Define metafile delimiter  <element_ident>~<levelident>~<validtime> */
#define	FpaFidentDelimiter	"~"

/* Fixed (maximum) lengths for file identifiers for new format metafile names */
#define FILE_IDENT_LEN	128
#define ELEM_IDENT_LEN	32
#define LEVEL_IDENT_LEN	32
#define VALID_TIME_LEN	32

/* Old format FPA metafile names */

/* Define metafile delimiter  <element_id><levelid>_<validtime> */
#define	FpaFvalidDelimiter	"_"

/* Fixed (maximum) lengths for file identifiers for old format metafile names */
#define FILE_ID_LEN		64
#define ELEM_ID_LEN		2
#define LEVEL_ID_LEN	32


/* Fixed (maximum) length for source and subsource names */
#define SOURCE_NAME_LEN		32


/* Fixed (maximum) length for names for config_file_message() calls */
#define CONFIG_FILE_MESSAGE_LEN		1024


/***********************************************************************
*                                                                      *
*  Defining keywords and pre-defined values in configuration file      *
*                                                                      *
************************************************************************/

 /* Identifier tags to access configuration files in setup file */
#define	FpaCblocksFile       "config"
#define	FpaCunitsFile        "config"
#define	FpaCconstantsFile    "config"
#define	FpaCsourcesFile      "config"
#define	FpaCgroupsFile       "config"
#define	FpaClevelsFile       "config"
#define	FpaCelementsFile     "config"
#define	FpaCfieldsFile       "config"
#define	FpaCcrossRefsFile    "config"
#define	FpaCsamplesFile      "config"

/* Set maximum allowed length of lines in configuration files */
#define	FPAC_MAX_LENGTH		4096


/* Revision line tag for all configuration files */
#define	FpaCrevisionLine  "revision"


/* Include line tag for additional configuration files */
#define	FpaCincludeFile   "include"

 /* Characters for reading all configuration files */
#define	FpaCopenBrace     "{"
#define	FpaCcloseBrace    "}"
#define	FpaCplaceHolder   "-"
#define	FpaCequalSign     "="

 /* Common to several blocks */
#define	FpaClabel         "label"
#define	FpaCshortLabel    "short_label"
#define	FpaCdescription   "description"
#define	FpaCfileId        "file_id"
#define	FpaCfileIdent     "file_ident"
#define	FpaCalias         "alias"
#define	FpaCnone          "None"
#define	FpaCblank         ""
#define	FpaCany           "Any"
#define	FpaCdefault       "Default"
#define	FpaC_NONE         9999
#define	FpaCnoMacro       -1
#define	FpaCnoSection     -1

 /* Default parameters */
#define	FpaCdefaultScatteredTypesClass  "plot"
#define	FpaCdefaultLabellingTypesClass  "label"


 /* Units block related MACROS */
#define	FpaCblockUnits      "Units"
/* Units block options */
enum	{ FpaCblockUnitsName,  FpaCblockUnitsInfo };

#define	FpaCmksUnits        "MKS"
#define	FpaCmksEquivalent       "MKS_equivalent"
#define	FpaCmksConversion       "MKS_conversion"


 /* Constants block related MACROS */
#define	FpaCblockConstants      "Constants"
/* Constants block options*/
enum	{ FpaCblockConstantsName,  FpaCblockConstantsInfo };

#define	FpaCconstant                "constant"
#define	FpaCconstantVal                 "constant (value)"
#define	FpaCconstantUnit                "constant (units)"

 /* Sources block related MACROS */
#define	FpaCblockSources      "Sources"
/* Sources Metafile info */
enum	{ FpaCblockSourcesName,        FpaCblockSourcesInfo,
			FpaCblockSourcesSubName,   FpaCblockSourcesSubInfo,
			FpaCblockSourcesAllied,
			FpaCblockSourcesProgram,   FpaCblockSourcesProgramInfo,
			FpaCblockSourcesFile,      FpaCblockSourcesFileInfo,
			FpaCblockSourcesField,     FpaCblockSourcesFieldInfo,
			FpaCblockSourcesWind,      FpaCblockSourcesWindInfo,
			FpaCblockSourcesValue,     FpaCblockSourcesValueInfo,
			FpaCblockSourcesMetafile,  FpaCblockSourcesMetafileInfo };

#define	FpaCanySource         "Any_Source"
#define	FpaCminutesRequired       "minutes_required"
#define	FpaCsourceType            "source_type"
#define	FpaCdirectoryTag          "directory_tag"
#define	FpaCdirectoryPath         "directory_path"
#define	FpaCdirectoryLayers       "directory_layers"
#define	FpaCsubSources            "subsources"
#define	FpaCsubDirectoryPath          "sub_directory_path"
/** Source type options */
typedef enum
		{ FpaC_DEPICTION,	/**< Depiction as a source */
		  FpaC_GUIDANCE,	/**< Guidance as a source */
		  FpaC_ALLIED,		/**< Allied Model as a source */
		  FpaC_MAPS,		/**< Maps as a source */
		  FpaC_DIRECT,		/**< Direct as a source */
		  FpaC_SRC_ANY,		/**< Any source */
		  FpaC_SRC_NOTUSED	/**< Not used */
		} FpaCsourceTypeOption;

#define	FpaCalliedModel           "allied_model"
#define	FpaCalliedTimeMatching        "time_matching"
#define	FpaCalliedSourceInfo          "source_info"
#define	FpaCalliedPreProcess          "pre_process"
#define	FpaCalliedProcess             "process"
#define	FpaCalliedPostProcess         "post_process"
#define	FpaCalliedPrograms            "programs"
#define	FpaCalliedFiles               "files"
#define	FpaCalliedRequiredFields      "required_fields"
#define	FpaCalliedRequiredWinds       "required_wind_crossrefs"
#define	FpaCalliedRequiredValues      "required_value_crossrefs"
#define	FpaCalliedMetafiles           "metafiles"
#define	FpaCalliedProgramPath             "program_path"
#define	FpaCalliedFilePath                "file_path"
#define	FpaCalliedFieldInfo               "field_info"
#define	FpaCalliedAttribInfoReset         "attribute_info_reset"
#define	FpaCalliedAttribInfo              "attribute_info"
#define	FpaCalliedAttribInfoTag               "attribute_info (tag)"
#define	FpaCalliedAttribInfoName              "attribute_info (name)"
#define	FpaCalliedAttribInfoUnit              "attribute_info (units)"
#define	FpaCalliedNodeAttribInfoReset     "node_attribute_info_reset"
#define	FpaCalliedNodeAttribInfo          "node_attribute_info"
#define	FpaCalliedNodeAttribInfoTag           "node_attribute_info (tag)"
#define	FpaCalliedNodeAttribInfoName          "node_attribute_info (name)"
#define	FpaCalliedNodeAttribInfoUnit          "node_attribute_info (units)"
#define	FpaCalliedDefAttribInfoReset      "default_attrib_info_reset"
#define	FpaCalliedDefAttribInfo           "default_attrib_info"
#define	FpaCalliedDefAttribInfoName           "default_attrib_info (name)"
#define	FpaCalliedDefAttribInfoValue          "default_attrib_info (value)"
#define	FpaCalliedSubFieldInfo            "sub_field_info"
#define	FpaCalliedSubFieldInfoVal             "sub_field_info (value)"
#define	FpaCalliedSubFieldInfoUnit            "sub_field_info (units)"
#define	FpaCalliedCrossRefInfo            "crossref_info"
#define	FpaCalliedFileAlias               "file_alias"
/** Allied data type options */
typedef enum
		{ FpaC_ALLIED_PROGRAMS,		/**< Allied program data */
		  FpaC_ALLIED_FILES,		/**< Allied file data */
		  FpaC_ALLIED_FIELDS,		/**< Allied field data */
		  FpaC_ALLIED_WINDS,		/**< Allied wind data */
		  FpaC_ALLIED_VALUES, 		/**< Allied value data */
		  FpaC_ALLIED_METAFILES		/**< Allied metafile data */
		} FpaCalliedDataTypeOption;

 /* Groups block related MACROS */
#define	FpaCblockGroups      "Groups"
#define	FpaCblockGFields         "Groups/Fields"
#define	FpaCblockGElements       "Groups/Elements"
/* Groups block options */
enum	{ FpaCblockGroupsSection,
			FpaCblockGroupsFields,      FpaCblockGroupsElements,
			FpaCblockGroupsFieldsInfo,  FpaCblockGroupsElementsInfo };

#define	FpaCanyGroup         "Any_Group"
#define	FpaCnotDisplayed     "Not_Displayed"
#define	FpaCmiscellaneous    "Miscellaneous"
#define	FpaCgenericEqtn      "Generic_Equation"

 /* Levels block related MACROS */
#define	FpaCblockLevels      "Levels"
/* Level block options */
enum	{ FpaCblockLevelsName,  FpaCblockLevelsInfo };

#define	FpaCanyLevel         "Any_Level"
#define	FpaClevelType            "level_type"
#define	FpaClevelLevels          "level_levels"
/** Level type options */
typedef enum
		{ FpaC_MSL,				/**< MSL level */
		  FpaC_SURFACE,			/**< Surface level */
		  FpaC_LEVEL,			/**< Level level */
		  FpaC_LAYER,			/**< Layer level */
		  FpaC_GEOGRAPHY,		/**< Geography level */
		  FpaC_ANNOTATION,		/**< Annotation level */
		  FpaC_LVL_ANY,			/**< Any Level level */
		  FpaC_LVL_NOTUSED		/**< Not Used */
		} FpaClevelTypeOption;
/** Levels category options */
typedef enum
		{ FpaC_LEVELS_MSL,			/**< MSL */
		  FpaC_LEVELS_SURFACE,		/**< Surface */
		  FpaC_LEVELS_PRESSURE,		/**< Pressure */
		  FpaC_LEVELS_HEIGHT,		/**< Height */
		  FpaC_LEVELS_SIGMA,		/**< Sigma */
		  FpaC_LEVELS_THETA,		/**< Theta */
		  FpaC_LEVELS_GEOGRAPHY,	/**< Geography */
		  FpaC_LEVELS_ANNOTATION	/**< Annotation */
		} FpaClevelsCategoryOption;

/* Elements block related MACROS */

#define	FpaCblockElements      "Elements"
/* Element block options */
enum	{ FpaCblockElementsName,             FpaCblockElementsInfo,
			FpaCblockElementsTimeDep,
			FpaCblockElementsLineTypes,      FpaCblockElementsScatteredTypes,
			FpaCblockElementsAttributes,     FpaCblockElementsEditor,
			FpaCblockElementsLabelling,      FpaCblockElementsSampling,
			FpaCblockElementsLinking,        FpaCblockElementsEquation,
			FpaCblockElementsValCalc,        FpaCblockElementsComponents,
			FpaCblockElementsLineTypesSet,   FpaCblockElementsScatteredTypesSet,
			FpaCblockElementsAttribsSet,     FpaCblockElementsLabelTypes,
			FpaCblockElementsScatteredAttribsSet,
			FpaCblockElementsLabelTypesSet,
			FpaCblockElementsLabelAttribsSet };

/** Attach options */
typedef enum
		{ FpaC_NO_ATTACH,		/**< Don't attach */
		  FpaC_ATTACH_AUTO,		/**< Closest point */
		  FpaC_ATTACH_MIN,		/**< Closest min point */
		  FpaC_ATTACH_MAX,		/**< Closest max point */
		  FpaC_ATTACH_COL,		/**< Closest saddle point */
		  FpaC_ATTACH_CONTOUR,	/**< Closest contour */
		  FpaC_ATTACH_BOUND,	/**< Closest boundary */
		  FpaC_ATTACH_DIV,		/**< Closest dividing line */
		  FpaC_ATTACH_LINE,		/**< Closest line */
		  FpaC_ATTACH_POINT		/**< Closest point */
		} FpaCattachOption;

#define	FpaCanyElement         "Any_Element"
#define	FpaCelementGroup           "element_group"
#define	FpaCfieldType              "field_type"
/** Field type options */
typedef enum
		{ FpaC_CONTINUOUS,		/**< Continous field */
		  FpaC_VECTOR,			/**< Vector field */
		  FpaC_DISCRETE,		/**< Discrete field */
		  FpaC_WIND,			/**< Wind field */
		  FpaC_LINE,			/**< Line field */
		  FpaC_SCATTERED,		/**< Scattered field */
		  FpaC_LCHAIN,			/**< Link chain field */
		  FpaC_SPECIAL, 		/**< Special field */
		  FpaC_ELEM_NOTUSED		/**< Not Used */
		} FpaCfieldTypeOption;
#define	FpaCdisplayFormat          "display_format"

#define	FpaCprecision              "precision"
#define	FpaCprecisionVal               "precision (value)"
#define	FpaCprecisionUnit              "precision (units)"

#define	FpaCtimeDependence         "time_dependence"
#define	FpaCtimeDepType                "time_type"
#define	FpaCtimeDepDailyRange          "daily_range"
#define	FpaCtimeDepDailyRangeVal           "daily_range (values)"
#define	FpaCtimeDepDailyRangeUnit          "daily_range (units)"

/* Note that FpaC_TIMEDEP_ANY is used to check against */
/*  FpaC_DAILY or FpaC_STATIC or FpaC_NORMAL           */
/** Time dependancy options */
typedef enum
		{ FpaC_DAILY = 01,			/**< Daily field */
		  FpaC_STATIC = 02,			/**< Static field */
		  FpaC_NORMAL = 04,			/**< Normal field */
		  FpaC_TIMEDEP_ANY = 07		/**< Any */
		} FpaCtimeDepTypeOption;

#define	FpaCwindClass              "wind_class"
/** Wind class options */
typedef enum
		{ FpaC_PRESSURE = 01,		/**< Pressure class */
		  FpaC_HEIGHT = 02, 		/**< Height class */
		  FpaC_THICKNESS = 03,		/**< Thickness class */
		  FpaC_ADJUSTMENT = 04,		/**< Adjustment class */
		  FpaC_NOWIND = 99			/**< No Wind class */
		} FpaCwindClassOption;

#define	FpaClineTypesReset         "line_types_reset"
#define	FpaClineTypes              "line_types"
#define	FpaClineTypesLabel             "type_label"
#define	FpaClineTypesShortLabel        "type_short_label"
#define	FpaClineTypesPattern           "pattern"

/* >>> the following are obsolete in next version <<< */
#define	FpaCsubelementsReset       "subelements_reset"	/**< Obsolete */
#define	FpaCsubelements            "subelements"	/**< Obsolete */
#define	FpaCsubelementsLabel           "sub_label"	/**< Obsolete */
/* >>> the preceding are obsolete in next version <<< */

#define	FpaCscatteredTypesReset    "scattered_types_reset"
#define	FpaCscatteredTypes         "scattered_types"
#define	FpaCscatteredTypesLabel        "type_label"
#define	FpaCscatteredTypesShortLabel   "type_short_label"
#define	FpaCscatteredTypesClass        "type_class"
#define	FpaCscatteredTypesEntryFile    "type_entry_file"
#define	FpaCscatteredTypesModifyFile   "type_modify_file"
#define	FpaCscatteredTypesAttach       "type_attach"
#define	FpaCscatteredTypesAttribDefaultsReset  "attribute_defaults_reset"
#define	FpaCscatteredTypesAttribDefaults       "attribute_defaults"
#define	FpaCscatteredTypesEntryRulesReset      "type_rules_reset"
#define	FpaCscatteredTypesPyEntryRulesReset    "python_type_rules_reset"
#define	FpaCscatteredTypesEntryRules           "type_rules"
#define	FpaCscatteredTypesPyEntryRules         "python_type_rules"

#define	FpaCattributesReset        "attributes_reset"
#define	FpaCattributes             "attributes"
#define	FpaCattributesLabel            "attribute_label"
#define	FpaCattributesShortLabel       "attribute_short_label"
#define	FpaCattributesBackDefault      "attribute_background_default"

#define	FpaCeditor                 "editor"
#define	FpaCeditorHilo                 "hilo"
#define	FpaCeditorPoke                 "poke"
#define	FpaCeditorPokeVal                  "poke (value)"
#define	FpaCeditorPokeUnit                 "poke (units)"
#define	FpaCeditorMagnitudePoke        "magnitude_poke"
#define	FpaCeditorDirectionPoke        "direction_poke"

/* >>> the following are obsolete in next version <<< */
#define	FpaCeditorSubelementList       "subelement_list"	/**< Obsolete */
#define	FpaCeditorBackgroundList       "background_list"	/**< Obsolete */
#define	FpaCeditorBackground           "background"	/**< Obsolete */
/* >>> the preceding are obsolete in next version <<< */

#define	FpaCeditorOverlaying             "overlaying"
#define	FpaCeditorDisplayOrder           "display_order"
#define	FpaCeditorEntryFile              "entry_file"
#define	FpaCeditorModifyFile             "modify_file"
#define	FpaCeditorMemoryFile             "memory_file"
#define	FpaCeditorBackEntryFile          "background_entry_file"
#define	FpaCeditorBackMemoryFile         "background_memory_file"
#define	FpaCeditorEntryRulesReset        "entry_rules_reset"
#define	FpaCeditorPyEntryRulesReset      "python_entry_rules_reset"
#define	FpaCeditorEntryRules             "entry_rules"
#define	FpaCeditorPyEntryRules           "python_entry_rules"
#define	FpaCeditorMergeFieldsReset       "merge_fields_reset"
#define	FpaCeditorMergeFields            "merge_fields"
#define	FpaCeditorLchainInterpDelta      "interpolation_delta"
#define	FpaCeditorLinkFieldsReset        "link_fields_reset"
#define	FpaCeditorLinkFields             "link_fields"
#define	FpaCeditorNodeEntryFile          "node_entry_file"
#define	FpaCeditorNodeModifyFile         "node_modify_file"
#define	FpaCeditorNodeEntryRulesReset    "node_entry_rules_reset"
#define	FpaCeditorPyNodeEntryRulesReset  "python_node_entry_rules_reset"
#define	FpaCeditorNodeEntryRules         "node_entry_rules"
#define	FpaCeditorPyNodeEntryRules       "python_node_entry_rules"

/* >>> the following are obsolete in next version <<< */
#define	FpaCeditorWindList             "wind_list"	/**< Obsolete */
#define	FpaCeditorWindBackgroundList   "wind_background_list"	/**< Obsolete */
#define	FpaCeditorWindBackground       "wind_background"	/**< Obsolete */
/* >>> the preceding are obsolete in next version <<< */

/* >>> the following are obsolete in next version <<< */
#define	FpaCeditorMenu                 "menu"	/**< Obsolete */
#define	FpaCeditorMenuFile             "menu_file"	/**< Obsolete */
#define	FpaCeditorMenuMemory           "menu_memory"	/**< Obsolete */
/* >>> the preceding are obsolete in next version <<< */

/* >>> the following are obsolete in next version <<< */
#define	FpaCeditorSubelementReset      "subelement_reset"	/**< Obsolete */
#define	FpaCeditorSubelement           "subelement"	/**< Obsolete */
/*     >>> the following are never reached! <<< */
#define	FpaCeditorSubLabel                 "sub_label"	/**< Obsolete */
#define	FpaCeditorBitmap                   "bitmap"	/**< Obsolete */
#define	FpaCeditorAssignment               "assignment"	/**< Obsolete */
/*     >>> the preceding are never reached! <<< */
/* >>> the preceding are obsolete in next version <<< */

#define	FpaClabellingReset         "labelling_reset"
#define	FpaClabelling              "labelling"
#define	FpaClabellingTypesReset        "label_types_reset"
#define	FpaClabellingTypes             "label_types"
#define	FpaClabellingLabel                 "type_label"
#define	FpaClabellingShortLabel            "type_short_label"
#define	FpaClabellingClass                 "type_class"
#define	FpaClabellingEntryFile             "type_entry_file"
#define	FpaClabellingModifyFile            "type_modify_file"
#define	FpaClabellingAttach                "type_attach"
#define	FpaClabellingAttribDefaultsReset   "attribute_defaults_reset"
#define	FpaClabellingAttribDefaults        "attribute_defaults"
#define	FpaClabellingEntryRulesReset       "type_rules_reset"
#define	FpaClabellingPyEntryRulesReset     "python_type_rules_reset"
#define	FpaClabellingEntryRules            "type_rules"
#define	FpaClabellingPyEntryRules          "python_type_rules"

#define	FpaCsamplingReset          "sampling_reset"
#define	FpaCsampling               "sampling"
#define	FpaCsample                 "sample"
#define	FpaCsamplingValueSampleTypes   "value_sample_types"
#define	FpaCsamplingAttribSampleNames  "attribute_sample_names"
#define	FpaCsamplingWindSampleType     "wind_sample_type"
#define	FpaCsamplingWindSampleTypes    "wind_sample_types"
#define	FpaCsamplingWindCrossRefs      "wind_crossrefs"

#define	FpaClinkingReset           "linking_reset"
#define	FpaClinking                "linking"
#define	FpaClinkingInterpDelta         "interpolation_delta"
#define	FpaClinkingFieldsReset         "link_fields_reset"
#define	FpaClinkingFields              "link_fields"

#define	FpaCequation               "equation"
#define	FpaCequationForceCalc          "force_calculation"
#define	FpaCequationString             "equation_string"
#define	FpaCequationUnits              "equation_units"

#define	FpaCvalueCalculation       "value_calculation"
#define	FpaCvalCalcForceCalc           "force_calculation"
#define	FpaCvalCalcCrossRef            "value_crossref"
#define	FpaCvalCalcSrcTypes            "source_types"

#define	FpaCcomponents             "components"
#define	FpaCcomponentsXcomp            "x_component"
#define	FpaCcomponentsYcomp            "y_component"
#define	FpaCcomponentsDcomp            "direction_component"
#define	FpaCcomponentsMcomp            "magnitude_component"

/* Fields block related MACROS */
#define	FpaCblockFields      "Fields"
/* Field block options */
enum	{ FpaCblockFieldsName,             FpaCblockFieldsInfo,
			FpaCblockFieldsElement,        FpaCblockFieldsLevel,
			FpaCblockFieldsLineTypes,      FpaCblockFieldsScatteredTypes,
			FpaCblockFieldsAttributes,     FpaCblockFieldsEditor,
			FpaCblockFieldsLabelling,      FpaCblockFieldsSampling,
			FpaCblockFieldsLinking,        FpaCblockFieldsEquation,
			FpaCblockFieldsValCalc,
			FpaCblockFieldsLineTypesSet,   FpaCblockFieldsScatteredTypesSet,
			FpaCblockFieldsAttribsSet,     FpaCblockFieldsLabelTypes,
			FpaCblockFieldsScatteredAttribsSet,
			FpaCblockFieldsLabelTypesSet,
			FpaCblockFieldsLabelAttribsSet };

#define	FpaCfieldGroup           "field_group"
#define	FpaCelementInfo          "element_info"
#define	FpaClevelInfo            "level_info"

/* CrossRefs block related MACROS */
#define	FpaCblockCrossRefs      "CrossRefs"
#define	FpaCblockCRWinds            "CrossRefs/Winds"
#define	FpaCblockCRValues           "CrossRefs/Values"
/* CrossRef block options */
enum	{ FpaCblockCrossRefsSection,
			FpaCblockCrossRefsWinds,      FpaCblockCrossRefsValues,
			FpaCblockCrossRefsWindsInfo,  FpaCblockCrossRefsValuesInfo,
			FpaCblockCrossRefsFields };

#define	FpaCcRefsWinds              "Winds"
#define	FpaCcRefsValues             "Values"
#define	FpaCwindFunction                "wind_function"
#define	FpaCvalueFunction               "value_function"
#define	FpaCtimeWeight                  "time_weight"
#define	FpaCtimeWeightVal                   "time_weight (value)"
#define	FpaCtimeWeightUnit                  "time_weight (units)"
#define	FpaCvalueWeight                 "value_weight"
#define	FpaCvalueWeightVal                  "value_weight (value)"
#define	FpaCvalueWeightUnit                 "value_weight (units)"
#define	FpaCcrossRefFields              "crossref_fields"

/* >>> the following are obsolete in next version <<< */
#define	FpaCwindCalcType                "wind_calctype"	/**< Obsolete */
#define	FpaCwindCalculationField        "FPA_Adjusted_Wind_Func"	/**< Obsolete */
#define	FpaCvalueCalcType               "value_calctype"	/**< Obsolete */
#define	FpaCvalueCalculationValue       "FPA_Value_Func"	/**< Obsolete */
/* >>> the preceding are obsolete in next version <<< */

 /* Samples block related MACROS */
#define	FpaCblockSamples      "Samples"
#define	FpaCblockSValues          "Samples/Values"
#define	FpaCblockSWinds           "Samples/Winds"
/* Samples block options */
enum	{ FpaCblockSamplesSection,
			FpaCblockSamplesValues,      FpaCblockSamplesWinds,
			FpaCblockSamplesValuesInfo,  FpaCblockSamplesWindsInfo };

#define	FpaCsamplesValues         "Values"
#define	FpaCsamplesWinds          "Winds"
#define	FpaCvalueSampType             "value_samptype"
/** Sample options */
typedef enum
		{ FpaC_SAMPLE_VALUE = 300,	/**< Sample the value */
		  FpaC_SAMPLE_GRADIENT,		/**< Sample the gradient */
		  FpaC_SAMPLE_CURVATURE,	/**< Sample the curvature */
		  FpaC_SAMPLE_MAGNITUDE,	/**< Sample the magnitude */
		  FpaC_SAMPLE_DIRECTION,	/**< Sample the direction */
		  FpaC_SAMPLE_LABEL,		/**< Sample the label */
		  FpaC_SAMPLE_CATEGORY,		/**< Sample the category */
		  FpaC_SAMPLE_ATTRIBUTE		/**< Sample the attribute */
		} FpaCsampleOption;


/* These "Default" value and wind type names must be in the Samples block! */
#define	FpaCsampleValue       "Value_Sample"
#define	FpaCsampleMagnitude   "Magnitude_Sample"
#define	FpaCsampleDirection   "Direction_Sample"
#define	FpaCsampleLabel       "Label_Sample"
#define	FpaCsampleAdjusted    "FPA_Adjusted_Wind_Func"


/* These strings are used in interface-ingred sample control */
#define	FpaCsampleControlAttribType     "AttribType"
#define	FpaCsampleControlValueType      "ValueType"
#define	FpaCsampleControlWindType       "WindType"
#define	FpaCsampleControlWindCrossRef   "WindCrossRef"
#define	FpaCsampleControlFieldLabels    "FieldLabels"
#define	FpaCsampleControlLinkNodes      "LinkNodes"


/* These "Default" Labelling types are used for automatic labelling */
#define	FpaDefLabContour                "contour"
#define	FpaDefLabLowAtMin               "low_at_min"
#define	FpaDefLabHighAtMax              "high_at_max"
#define	FpaDefLabArea                   "area"
#define	FpaDefLabWind                   "wind"
#define	FpaDefLabAdjustment             "adjustment"
#define	FpaDefLabLine                   "line"
#define	FpaDefLabLegend                 "legend"


/* These "Default" Labelling types are always included in the labelling lists */
#define	FpaLabellingContinuous          "FPA_continuous_labelling"
#define	FpaLabellingContinuousLabel     "Automatic"
#define	FpaLabellingContinuousShort     "Auto"
#define	FpaLabellingVector              "FPA_vector_labelling"
#define	FpaLabellingVectorLabel         "Automatic"
#define	FpaLabellingVectorShort         "Auto"
#define	FpaLabellingDiscrete            "FPA_discrete_labelling"
#define	FpaLabellingDiscreteLabel       "Label"
#define	FpaLabellingDiscreteShort       "Label"
#define	FpaLabellingWindBarb            "FPA_wind_barb_labelling"
#define	FpaLabellingWindBarbLabel       "Adjusted Wind"
#define	FpaLabellingWindBarbShort       "Adjusted"
#define	FpaLabellingWindArea            "FPA_wind_area_labelling"
#define	FpaLabellingWindAreaLabel       "Label"
#define	FpaLabellingWindAreaShort       "Label"
#define	FpaLabellingLine                "FPA_line_labelling"
#define	FpaLabellingLineLabel           "Label"
#define	FpaLabellingLineShort           "Label"
#define	FpaLabellingScattered           "FPA_scattered_labelling"
#define	FpaLabellingScatteredLabel      "Label"
#define	FpaLabellingScatteredShort      "Label"

/***********************************************************************
*                                                                      *
*  Stuctures for holding information in configuration file             *
*                                                                      *
************************************************************************/


/***** Units block of configuration file *****/

/** Structure for holding frequently accessed information of Units block */
typedef struct ConfigUnitStruct_struct
	{
	STRING		name;				/**< name for unit */
	LOGICAL		valid;				/**< error flag for unit */
	STRING		label;				/**< label for unit */
	STRING		sh_label;			/**< short label for unit */
	STRING		MKS;				/**< equivalent MKS unit name */
	double		factor;				/**< factor value */
	double		offset;				/**< offset value */
									/**< Note that to convert from "name"
									  to "MKS" divide by "factor"
									  and subtract "offset"           */
	} FpaConfigUnitStruct;


/***** Constants block of configuration file *****/

/** Structure for holding frequently accessed information of Constants block */
typedef struct ConfigConstantStruct_struct
	{
	STRING		name;				/**< name for constant */
	LOGICAL		valid;				/**< error flag for constant */
	STRING		label;				/**< label for constant */
	STRING		sh_label;			/**< short label for constant */
	STRING		description;		/**< description for constant */
	double		value;				/**< value for constant */
	FpaConfigUnitStruct
					*units;			/**< pointer to units for constant */
	} FpaConfigConstantStruct;


/***** Sources block of configuration file *****/

/** Structure for holding input/output information of Sources block */
typedef struct ConfigSourceIOStruct_struct
	{
	STRING		src_tag;			/**< source directory tag from setup file */
	STRING		src_path;			/**< source directory path
									  (relative to src_tag) */
	int			src_layers;			/**< number of layers in source directory
									  tree (including base directory)     */
	} FpaConfigSourceIOStruct;

/** Structure for holding subsource information of Sources block */
typedef struct ConfigSourceSubStruct_struct
	{
	STRING		name;				/**< name for subsource */
	STRING		label;				/**< label for subsource */
	STRING		sh_label;			/**< short label for subsource */
	STRING		description;		/**< description for subsource */
	STRING		sub_path;			/**< sub-directory path
									  (relative to src_tag/src_path) */
	} FpaConfigSourceSubStruct;


/* Structures for holding Allied Model information of Sources block */

/** Structure for holding attribute information for Fields or Metafiles */
typedef struct ConfigAlliedAttribStruct_struct
	{
	int			nattribs;			/**< number of attributes */
	STRING		*tag;				/**< program tag for attributes */
	STRING		*attname;			/**< attribute names */
	struct	ConfigUnitStruct_struct
					**attunit;		/**< attribute units */
	} FpaConfigAlliedAttribStruct;

/** Structure for holding default attribute information for Metafiles */
typedef struct ConfigAlliedDefAttribStruct_struct
	{
	int			natt_defs;			/**< number of default attributes */
	STRING		*attname_defs;		/**< default attribute names */
	STRING		*attval_defs;		/**< default attributes values */
	} FpaConfigAlliedDefAttribStruct;

/**  Structure for holding programs information */
typedef struct ConfigAlliedProgramsStruct_struct
	{
	int			nprogs;				/**< number of programs */
	STRING		*aliases;			/**< program aliases */
	STRING		*src_tags;			/**< program directory tags from setup file
									  (for location of input files)         */
	STRING		*prog_paths;		/**< program file paths
									  (relative to src_tags/src_path) */
	} FpaConfigAlliedProgramsStruct;

/**  Structure for holding files information */
typedef struct ConfigAlliedFilesStruct_struct
	{
	int			nfiles;				/**< number of files */
	STRING		*aliases;			/**< file aliases */
	STRING		*src_tags;			/**< file directory tags from setup file
									  (for location of input files)      */
	STRING		*file_paths;		/**< file paths (relative to
									  src_tags/src_path/sub_path) */
	} FpaConfigAlliedFilesStruct;

/**  Structure for holding required fields information */
typedef struct ConfigAlliedFieldsStruct_struct
	{
	int			nfields;			/**< number of required fields */
	STRING		*aliases;			/**< aliases for required fields */
	struct	ConfigFieldStruct_struct
					**flds;			/**< identifiers for required fields */
	int			*ftypes;			/**< field types for required fields */
	struct	ConfigAlliedAttribStruct_struct
					**attinfo;		/**< attribute info for each field */
	struct	ConfigAlliedAttribStruct_struct
					**nodeinfo;		/**< node attribute info for each field */
	STRING		*sub_fields;		/**< sub-field names for required fields
									  (if the field is plot type!)       */
	struct	ConfigUnitStruct_struct
					**sub_units;	/**< sub-field units for required fields
									  (if the field is plot type!)       */
	struct	ConfigSourceStruct_struct
					**src_defs;		/**< sources for required fields */
	struct	ConfigSourceSubStruct_struct
					**sub_defs;		/**< sub sources for required fields */
	} FpaConfigAlliedFieldsStruct;

/**  Structure for holding required wind crossreferences information */
typedef struct ConfigAlliedWindsStruct_struct
	{
	int			nwinds;				/**< number of required wind crossrefs */
	STRING		*aliases;			/**< aliases for required wind crossrefs */
	struct	ConfigCrossRefStruct_struct
					**wcrefs;		/**< identifiers for required wind crossrefs */
	struct	ConfigSourceStruct_struct
					**src_defs;		/**< sources for required wind crossrefs */
	struct	ConfigSourceSubStruct_struct
					**sub_defs;		/**< sub sources for required wind crossrefs */
	} FpaConfigAlliedWindsStruct;

/**  Structure for holding required value crossreferences information */
typedef struct ConfigAlliedValuesStruct_struct
	{
	int			nvalues;			/**< number of required value crossrefs */
	STRING		*aliases;			/**< aliases for required value crossrefs */
	struct	ConfigCrossRefStruct_struct
					**vcrefs;		/**< identifiers for required value crossrefs */
	struct	ConfigSourceStruct_struct
					**src_defs;		/**< sources for required value crossrefs */
	struct	ConfigSourceSubStruct_struct
					**sub_defs;		/**< sub sources for required value crossrefs */
	} FpaConfigAlliedValuesStruct;

/**  Structure for holding metafiles information */
typedef struct ConfigAlliedMetafilesStruct_struct
	{
	int			nfiles;				/**< number of metafiles */
	STRING		*aliases;			/**< metafile aliases */
	STRING		*file_aliases;		/**< input file aliases
									(where the metafile comes from) */
	struct	ConfigFieldStruct_struct
					**flds;			/**< metafile field identifiers */
	struct	ConfigAlliedAttribStruct_struct
					**attinfo;		/**< attribute info for each metafile */
	struct	ConfigAlliedDefAttribStruct_struct
					**definfo;		/**< default attrib info for each metafile */
	} FpaConfigAlliedMetafilesStruct;

/** Structure for holding Allied Model information of Sources block */
typedef struct ConfigSourceAlliedStruct_struct
	{
	LOGICAL		check_allied;		/**< one time Allied Model checking flag */
	LOGICAL		time_match;			/**< time matching flag for input fields */
	struct	ConfigSourceStruct_struct
					*src_def;		/**< default source for input fields */
	struct	ConfigSourceSubStruct_struct
					*sub_def;		/**< default sub source for input fields */
	STRING		pre_process;		/**< Allied Model preprocessing run string */
	STRING		process;			/**< Allied Model processing run string */
	STRING		post_process;		/**< Allied Model postprocessing run string */
	FpaConfigAlliedProgramsStruct
					*programs;		/**< Allied Model programs information */
	FpaConfigAlliedFilesStruct
					*files;			/**< Allied Model files information */
	FpaConfigAlliedFieldsStruct
					*fields;		/**< Allied Model fields information */
	FpaConfigAlliedWindsStruct
					*winds;			/**< Allied Model wind crossref information */
	FpaConfigAlliedValuesStruct
					*values;		/**< Allied Model value crossref information */
	FpaConfigAlliedMetafilesStruct
					*metafiles;		/**< Allied Model metafiles information */
	} FpaConfigSourceAlliedStruct;

/** Structure for holding frequently accessed information of Sources block */
typedef struct ConfigSourceStruct_struct
	{
	STRING		name;				/**< name for source */
	LOGICAL		valid;				/**< error flag for source */
	int			nblocks;			/**< number of config file blocks */
	STRING		*filenames;			/**< name of file containing each block */
	long int	*locations;			/**< location in file of each block */
	STRING		label;				/**< label for source */
	STRING		sh_label;			/**< short label for source */
	STRING		description;		/**< description for source */
	LOGICAL		minutes_rqd;		/**< flag for minutes in source timestamps */
	FpaCsourceTypeOption
					src_type;		/**< enumerated source type */
	FpaConfigSourceIOStruct
					*src_io;		/**< source input/output information */
	FpaConfigSourceSubStruct
					*src_sub;		/**< subsource information */
	int			nsubsrc;			/**< number of subsources */
	FpaConfigSourceSubStruct
					**subsrcs;		/**< list of subsource information */
	LOGICAL		valid_allied;		/**< error flag for Allied Model information */
	FpaConfigSourceAlliedStruct
					*allied;		/**< Allied Model information */
	} FpaConfigSourceStruct;


/***** Groups block of configuration file *****/

/** Structure for holding information of Groups block for Fields or Elements */
typedef struct ConfigGroupStruct_struct
	{
	STRING		name;				/**< name of field or element group */
	LOGICAL		valid;				/**< error flag for group */
	STRING		label;				/**< label of group */
	STRING		sh_label;			/**< short label of group */
	} FpaConfigGroupStruct;


/***** Levels block of configuration file *****/

/** Structure for holding input/output information of Levels block */
typedef struct ConfigLevelIOStruct_struct
	{
	LOGICAL		check_fident;		/**< one time file identifier checking flag */
	STRING		fident;				/**< file identifier for level */
	STRING		fid;				/**< old file identifier for level */
	} FpaConfigLevelIOStruct;

/** Structure for holding levels information of Levels block */
typedef struct ConfigLevelLevelsStruct_struct
	{
	FpaClevelsCategoryOption
					lvl_category;	/**< enumerated level category */
	STRING		lvl;				/**< single level */
	STRING		uprlvl;				/**< upper level of layer */
	STRING		lwrlvl;				/**< lower level of layer */
	} FpaConfigLevelLevelsStruct;

/** Structure for holding frequently accessed information of Levels block */
typedef struct ConfigLevelStruct_struct
	{
	STRING		name;				/**< normal name for level */
	LOGICAL		valid;				/**< error flag for level */
	STRING		label;				/**< label for level */
	STRING		sh_label;			/**< short label for level */
	STRING		description;		/**< description for level */
	FpaClevelTypeOption
					lvl_type;		/**< enumerated level type */
	FpaConfigGroupStruct
					*fld_group;		/**< pointer to default field group */
	FpaConfigLevelIOStruct
					*lev_io;		/**< level input/output information */
	FpaConfigLevelLevelsStruct
					*lev_lvls;		/**< level levels information */
	} FpaConfigLevelStruct;


/***** Elements block of configuration file *****/

/** Structure for holding input/output information of Elements block */
typedef struct ConfigElementIOStruct_struct
	{
	LOGICAL		check_fident;		/**< one time file identifier checking flag */
	STRING		fident;				/**< file identifier for element */
	STRING		fid;				/**< old file identifier for element */
	double		precision;			/**< metafile precision for element */
	FpaConfigUnitStruct
					*units;			/**< pointer to natural units for element */
	} FpaConfigElementIOStruct;

/** Structure for holding time dependence information of Elements block */
typedef struct ConfigElementTimeDepStruct_struct
	{
	FpaCtimeDepTypeOption
					time_dep;		/**< enumerated time dependence */
	double		normal_time;		/**< normal valid time for element */
	double		begin_time;			/**< begin time for display */
	double		end_time;			/**< end time for display */
	FpaConfigUnitStruct
					*units;			/**< pointer to units for times */
	} FpaConfigElementTimeDepStruct;

/** Structure for holding default attribute information */
typedef struct ConfigDefaultAttribStruct_struct
	{
	int			nattrib_defs;		/**< number of default attributes */
	STRING		*attrib_def_names;	/**< names of default attributes */
	STRING		*attrib_def_values;	/**< default values of attributes */
	} FpaConfigDefaultAttribStruct;

/** Structure for holding entry rules information */
typedef struct ConfigEntryRuleStruct_struct
	{
	int			nrules;				/**< number of rules */
	STRING		*entry_rules;		/**< list of rule names */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;				/**< number of rules */
	STRING		*py_entry_rules;		/**< list of rule names */
	} FpaConfigEntryRuleStruct;

/** Structure for holding line types information of Elements block */
typedef struct ConfigElementLineTypeStruct_struct
	{
	int			ntypes;				/**< number of line types */
	STRING		*type_names;		/**< names of line types */
	STRING		*type_labels;		/**< labels of line types */
	STRING		*type_sh_labels;	/**< short labels of line types */
	STRING		*patterns;			/**< pattern filenames of line types */
	} FpaConfigElementLineTypeStruct;

/* Structures for holding scattered information of Elements block */

/** Structure for holding scattered types information of Elements block */
typedef struct ConfigElementScatteredTypeStruct_struct
	{
	LOGICAL		check_scattered;	/**< one time scattered types checking flag */
	int			ntypes;				/**< number of scattered types */
	STRING		*type_names;		/**< names of scattered types */
	STRING		*type_labels;		/**< labels of scattered types */
	STRING		*type_sh_labels;	/**< short labels of scattered types */
	STRING		*type_classes;		/**< style classes of scattered types */
	STRING		*type_entry_files;	/**< entry files of scattered types */
	STRING		*type_modify_files;	/**< modify entry files of scattered types */

	FpaCattachOption
				*type_attach_opts;	/**< features to attach scattered types to */
	FpaConfigDefaultAttribStruct
					*type_attribs;	/**< default attributes for scattered types */
	FpaConfigEntryRuleStruct
					*type_rules;	/**< entry rules for scattered types */
	} FpaConfigElementScatteredTypeStruct;

/** Structure for holding attributes information of Elements block */
typedef struct ConfigElementAttribStruct_struct
	{
	int			nattribs;			/**< number of element attributes */
	STRING		*attrib_names;		/**< names of element attributes */
	STRING		*attrib_labels;		/**< labels of element attributes */
	STRING		*attrib_sh_labels;	/**< short labels of element attributes */
	STRING		*attrib_back_defs;	/**< background default values of
									  element attributes          */
	} FpaConfigElementAttribStruct;

/* Structures for holding editor information of Elements block */

/**  Structure for holding continuous field information */
typedef struct ConfigContinuousEditorStruct_struct
	{
	LOGICAL		hilo;				/**< show high or low marker? */
	double		poke;				/**< range of poke */
	FpaConfigUnitStruct
					*units;			/**< pointer to units for poke */
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	} FpaConfigContinuousEditorStruct;

/**  Structure for holding vector field information */
typedef struct ConfigVectorEditorStruct_struct
	{
	LOGICAL		hilo;				/**< show high or low marker? */
	double		mag_poke;			/**< range of poke for vector magnitude */
	FpaConfigUnitStruct
					*mag_units;		/**< pointer to units for poke (magnitude) */
	double		dir_poke;			/**< range of poke for field direction */
	FpaConfigUnitStruct
					*dir_units;		/**< pointer to units for poke (direction) */
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	} FpaConfigVectorEditorStruct;

/**  Structure for holding discrete field information */
typedef struct ConfigDiscreteEditorStruct_struct
	{
	LOGICAL		overlaying;			/**< allow overlaying of areas? */
	LOGICAL		display_order;		/**< display order of areas? */
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	} FpaConfigDiscreteEditorStruct;

/**  Structure for holding wind field information */
typedef struct ConfigWindEditorStruct_struct
	{
	LOGICAL		display_order;		/**< display order of wind areas? */
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	} FpaConfigWindEditorStruct;

/**  Structure for holding line field information */
typedef struct ConfigLineEditorStruct_struct
	{
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	} FpaConfigLineEditorStruct;

/**  Structure for holding scattered field information */
typedef struct ConfigScatteredEditorStruct_struct
	{
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	} FpaConfigScatteredEditorStruct;

/**  Structure for holding link chain field information */
typedef struct ConfigLchainEditorStruct_struct
	{
	STRING		entry_file;			/**< entry file for attributes */
	STRING		modify_file;		/**< modify entry file for attributes */
	STRING		memory_file;		/**< memory file for attributes */
	STRING		back_entry_file;	/**< entry file for background attributes */
	STRING		back_memory_file;	/**< memory file for background attributes */
	int			nrules;				/**< number of rules for attributes */
	STRING		*entry_rules;		/**< list of rule names for attributes */
	ERULE		*entry_funcs;		/**< list of function pointers for rules */
	int			py_nrules;			/**< number of python rules for attributes */
	STRING		*py_entry_rules;	/**< list of python rule names for attributes */
	STRING		node_entry_file;	/**< entry file for node attributes */
	STRING		node_modify_file;	/**< modify entry file for node attributes */
	int			nnode_rules;		/**< number of rules for node attributes */
	STRING		*node_entry_rules;	/**< list of rule names for node attributes */
	ERULE		*node_entry_funcs;	/**< list of function pointers for rules */
	int			py_nnode_rules;		/**< number of python rules for attributes */
	STRING		*py_node_entry_rules;/**< list of python rule names for attributes */
	int			nmerge;				/**< number of fields for merging */
	struct	ConfigElementStruct_struct
					**merge_elems;	/**< pointers to elements for merging */
	struct	ConfigLevelStruct_struct
					**merge_levels;	/**< pointers to levels for merging */
	int			nlink;				/**< number of link fields for merging */
	struct	ConfigElementStruct_struct
					**link_elems;	/**< pointers to link elements for merging */
	struct	ConfigLevelStruct_struct
					**link_levels;	/**< pointers to link levels for merging */
	int			minterp;			/**< link chain interpolation delta (in min) */
	} FpaConfigLchainEditorStruct;

/** Structure for holding editor information of Elements block */
typedef struct ConfigElementEditorStruct_struct
	{
	LOGICAL		check_editor;		/**< one time editor checking flag */
	union FpaConfigElementEditorUnion
		{
		FpaConfigContinuousEditorStruct
					*continuous;	/**< editor information for
									  continuous fields     */
		FpaConfigVectorEditorStruct
					*vector;		/**< editor information for
									  vector fields     */
		FpaConfigDiscreteEditorStruct
					*discrete;		/**< editor information for
									  discrete fields       */
		FpaConfigWindEditorStruct
					*wind;			/**< editor information for
									  wind fields           */
		FpaConfigLineEditorStruct
					*line;			/**< editor information for
									  line fields           */
		FpaConfigScatteredEditorStruct
					*scattered;		/**< editor information for
									  scattered fields      */
		FpaConfigLchainEditorStruct
					*lchain;		/**< editor information for
									  link chain fields     */
		} type;						/**< type struct */
	} FpaConfigElementEditorStruct;

/** Structure for holding labelling information of Elements block */
typedef struct ConfigElementLabellingStruct_struct
	{
	LOGICAL		check_labelling;	/**< one time labelling checking flag */
	int			ntypes;				/**< number of labelling types */
	STRING		*type_names;		/**< names of labelling types */
	STRING		*type_labels;		/**< labels of labelling types */
	STRING		*type_sh_labels;	/**< short labels of labelling types */
	STRING		*type_classes;		/**< style classes of labelling types */
	STRING		*type_entry_files;	/**< entry files of labelling types */
	STRING		*type_modify_files;	/**< modify entry files of labelling types */
	FpaCattachOption
				*type_attach_opts;	/**< features to attach label types to */
	FpaConfigDefaultAttribStruct
					*type_attribs;	/**< default attributes for labelling types */
	FpaConfigEntryRuleStruct
					*type_rules;	/**< entry rules for labelling types */
	} FpaConfigElementLabellingStruct;

/* Structures for holding sampling information of Elements block */

/**  Structure for holding continuous field information */
typedef struct ConfigContinuousSamplingStruct_struct
	{
	int			nsample;			/**< number of value sample types */
	struct	ConfigSampleStruct_struct
					**samples;		/**< pointers to value sample types */
	int			nwindsamp;			/**< number of wind sample types */
	struct	ConfigSampleStruct_struct
					**windsamps;	/**< pointers to wind sample types */
	} FpaConfigContinuousSamplingStruct;

/**  Structure for holding vector field information */
typedef struct ConfigVectorSamplingStruct_struct
	{
	int			nsample;			/**< number of value sample types */
	struct	ConfigSampleStruct_struct
					**samples;		/**< pointers to value sample types */
	int			nwindsamp;			/**< number of wind sample types */
	struct	ConfigSampleStruct_struct
					**windsamps;	/**< pointers to wind sample types */
	} FpaConfigVectorSamplingStruct;

/**  Structure for holding discrete field information */
typedef struct ConfigDiscreteSamplingStruct_struct
	{
	int			nsattribs;			/**< number of sampling attributes */
	STRING		*sattrib_names;		/**< names of sampling attributes */
	} FpaConfigDiscreteSamplingStruct;

/**  Structure for holding wind field information */
typedef struct ConfigWindSamplingStruct_struct
	{
	int			nsample;			/**< number of value sample types */
	struct	ConfigSampleStruct_struct
					**samples;		/**< pointers to value sample types */
	int			nwcref;				/**< number of allowed
									  wind cross references */
	struct	ConfigCrossRefStruct_struct
					**wcrefs;		/**< pointers to allowed
									  wind cross references */
	struct	ConfigSampleStruct_struct
					*windsample;	/**< pointer to wind sample type
									for "model" wind           */
	} FpaConfigWindSamplingStruct;

/**  Structure for holding line field information */
typedef struct ConfigLineSamplingStruct_struct
	{
	int			nsattribs;			/**< number of sampling attributes */
	STRING		*sattrib_names;		/**< names of sampling attributes */
	} FpaConfigLineSamplingStruct;

/**  Structure for holding scattered field information */
typedef struct ConfigScatteredSamplingStruct_struct
	{
	int			nsattribs;			/**< number of sampling attributes */
	STRING		*sattrib_names;		/**< names of sampling attributes */
	} FpaConfigScatteredSamplingStruct;

/**  Structure for holding link chain field information */
typedef struct ConfigLchainSamplingStruct_struct
	{
	int			nsattribs;			/**< number of sampling attributes */
	STRING		*sattrib_names;		/**< names of sampling attributes */
	} FpaConfigLchainSamplingStruct;

/** Structure for holding sampling information of Elements block */
typedef struct ConfigElementSamplingStruct_struct
	{
	LOGICAL		check_sampling;		/**< one time sampling checking flag */
	union FpaConfigElementSamplingUnion
		{
		FpaConfigContinuousSamplingStruct
					*continuous;	/**< sampling information for
									  continuous fields       */
		FpaConfigVectorSamplingStruct
					*vector;		/**< sampling information for
									  vector fields       */
		FpaConfigDiscreteSamplingStruct
					*discrete;		/**< sampling information for
									  discrete fields         */
		FpaConfigWindSamplingStruct
					*wind;			/**< sampling information for
									  wind fields             */
		FpaConfigLineSamplingStruct
					*line;			/**< sampling information for
									  line fields             */
		FpaConfigScatteredSamplingStruct
					*scattered;		/**< sampling information for
									  scattered fields        */
		FpaConfigLchainSamplingStruct
					*lchain;		/**< sampling information for
									  link chain fields       */
		} type;						/**< type struct */
	} FpaConfigElementSamplingStruct;

/** Structure for holding linking information of Elements block */
typedef struct ConfigElementLinkingStruct_struct
	{
	LOGICAL		check_linking;		/**< one time linking checking flag */
	int			minterp;			/**< interpolation delta for link chains (in min) */
	int			nlink;				/**< number of link fields for merging */
	struct	ConfigElementStruct_struct
					**link_elems;	/**< pointers to link elements for merging */
	struct	ConfigLevelStruct_struct
					**link_levels;	/**< pointers to link levels for merging */
	} FpaConfigElementLinkingStruct;

/** Structure for holding equation information of Elements block */
typedef struct ConfigElementEquationStruct_struct
	{
	LOGICAL		force;				/**< force calculation? */
	STRING		eqtn;				/**< equation string */
	FpaConfigUnitStruct
					*units;			/**< pointer to units for equation */
	} FpaConfigElementEquationStruct;

/** Structure for holding value calculation information of Elements block */
typedef struct ConfigElementValCalcStruct_struct
	{
	LOGICAL		force;				/**< force calculation? */
	struct	ConfigCrossRefStruct_struct
						*vcalc;		/**< value calculation cross reference */
	int			nsrc_type;			/**< number of source types */
	FpaCsourceTypeOption
					*src_types;		/**< list of enumerated source types */
	} FpaConfigElementValCalcStruct;

/** Structure for holding components information of Elements block */
typedef struct ConfigElementComponentStruct_struct
	{
	const COMP_INFO
					*cinfo;			/**< required components info */
	int			ncomp;				/**< number of components */
	struct	ConfigElementStruct_struct
					**comp_edefs;	/**< list of pointers to element info */
	COMPONENT	*comp_types;		/**< list of component info */
	} FpaConfigElementComponentStruct;

/** Structure for holding detailed information of Elements block */
typedef struct ConfigElementDetailStruct_struct
	{
	FpaCwindClassOption
					wd_class;			/**< enumerated wind class for element */
	FpaConfigElementLineTypeStruct
					*line_types;		/**< line types info */
	FpaConfigElementScatteredTypeStruct
					*scattered_types;	/**< scattered types info */
	FpaConfigElementAttribStruct
					*attributes;		/**< attributes info */
	FpaConfigElementEditorStruct
					*editor;			/**< editor info */
	FpaConfigElementLabellingStruct
					*labelling;			/**< labelling info */
	FpaConfigElementSamplingStruct
					*sampling;			/**< sampling info */
	FpaConfigElementLinkingStruct
					*linking;			/**< linking info */
	FpaConfigElementEquationStruct
					*equation;			/**< equation info */
	FpaConfigElementValCalcStruct
					*valcalc;			/**< value calculation info */
	FpaConfigElementComponentStruct
					*components;		/**< components info */
	} FpaConfigElementDetailStruct;

/** Structure for holding frequently accessed information of Elements block */
typedef struct ConfigElementStruct_struct
	{
	STRING		name;				/**< normal name for element */
	LOGICAL		valid;				/**< error flag for element */
	int			nblocks;			/**< number of config file blocks */
	STRING		*filenames;			/**< name of file containing each block */
	long int	*locations;			/**< location in file of each block */
	STRING		label;				/**< label for element */
	STRING		sh_label;			/**< short label for element */
	STRING		description;		/**< description for element */
	FpaConfigGroupStruct
					*group;			/**< pointer to element group info */
	FpaClevelTypeOption
					lvl_type;		/**< enumerated level type */
	FpaConfigGroupStruct
					*fld_group;		/**< pointer to default field group */
	FpaCfieldTypeOption
					fld_type;		/**< enumerated field type for element */
	int				display_format;	/**< enumerated display format */
	FpaConfigElementIOStruct
					*elem_io;		/**< element input/output information */
	FpaConfigElementTimeDepStruct
					*elem_tdep;		/**< element time dependence information */
	LOGICAL		valid_detail;		/**< error flag for detailed element info */
	FpaConfigElementDetailStruct
					*elem_detail;	/**< detailed element information */
	} FpaConfigElementStruct;


/***** Fields block of configuration file *****/

/** Structure for holding frequently accessed information of Fields block */
typedef struct ConfigFieldStruct_struct
	{
	FpaConfigElementStruct
					*element;		/**< pointer to element info */
	FpaConfigLevelStruct
					*level;			/**< pointer to level info */
	LOGICAL		valid;				/**< error flag for field */
	int			nblocks;			/**< number of config file blocks */
	STRING		*filenames;			/**< name of file containing each block */
	long int	*locations;			/**< location in file of each block */
	STRING		label;				/**< label for field */
	STRING		sh_label;			/**< short label for field */
	FpaConfigGroupStruct
					*group;			/**< pointer to field group info */
	LOGICAL		created_field;		/**< flag for created fields
									   not from config file! */
	LOGICAL		field_detail;		/**< flag for detailed field info */
	LOGICAL		valid_detail;		/**< error flag for detailed field info */
	LOGICAL		override_element;	/**< override flag for element info */
	LOGICAL		override_level;		/**< override flag for level info */
	} FpaConfigFieldStruct;


/***** CrossRefs block of configuration file *****/

/** Structure for holding information of CrossRefs block for Winds or Values */
typedef struct ConfigCrossRefStruct_struct
	{
	STRING		name;				/**< name of wind or value cross-reference */
	LOGICAL		valid;				/**< error flag for cross-reference */
	STRING		label;				/**< label for cross-reference */
	STRING		sh_label;			/**< short label for cross-reference */
	STRING		description;		/**< description for cross-reference */
	STRING		func_name;			/**< name of wind or value function */
	double		wgtt;				/**< time weight for value calculations */
	FpaConfigUnitStruct
					*unit_t;		/**< pointer to units for time weight */
	double		wgtv;				/**< value weight for value calculations */
	FpaConfigUnitStruct
					*unit_v;		/**< pointer to units for value weight */
	int			nfld;				/**< number of cross-referenced fields */
	FpaConfigFieldStruct
					**flds;			/**< pointers to cross-referenced fields */
	} FpaConfigCrossRefStruct;


/***** Samples block of configuration file *****/

/** Structure for holding information of Samples block for Values or Winds */
typedef struct ConfigSampleStruct_struct
	{
	STRING		name;				/**< name of value or wind sample */
	LOGICAL		valid;				/**< error flag for sample */
	STRING		label;				/**< label for sample */
	STRING		sh_label;			/**< short label for sample */
	STRING		description;		/**< description for sample */
	STRING		samp_name;			/**< name of type of value to sample */
	int			samp_type;			/**< enumerated type of value to sample */
	STRING		samp_func;			/**< name of wind function for sampling */
	} FpaConfigSampleStruct;


/***********************************************************************
*                                                                      *
*  Initialize enumerated types for all structures                      *
*                                                                      *
************************************************************************/

#ifdef CONFIG_STRUCTS_INIT


/***** Enumerated type macros for Sources block *****/


/** Define source types of Sources block in configuration file */
static const FPA_MACRO_LIST FpaCsourceTypes[] =
	{
		{ FpaC_DEPICTION,       "Depiction" },
		{ FpaC_GUIDANCE,        "Guidance"  },
		{ FpaC_ALLIED,          "Allied"    },
		{ FpaC_MAPS,            "Maps"      },
		{ FpaC_DIRECT,          "Direct"    },
		{ FpaC_SRC_ANY,         "Any"       },
		{ FpaC_SRC_NOTUSED,     "NotUsed"   },
	};

/** Set number of predefined source types */
static const int NumFpaCsourceTypes =
	(int) (sizeof(FpaCsourceTypes) / sizeof(FPA_MACRO_LIST));


/***** Enumerated type macros for Levels block *****/


/** Define level types of Levels/Elements blocks in configuration file */
static const FPA_MACRO_LIST FpaClevelTypes[] =
	{
		{ FpaC_MSL,             "Msl"        },
		{ FpaC_SURFACE,         "Surface"    },
		{ FpaC_LEVEL,           "Level"      },
		{ FpaC_LAYER,           "Layer"      },
		{ FpaC_GEOGRAPHY,       "Geography"  },
		{ FpaC_ANNOTATION,      "Annotation" },
		{ FpaC_LVL_ANY,         "Any"        },
		{ FpaC_LVL_NOTUSED,     "NotUsed"    },
	};

/** Set number of predefined level types */
static const int NumFpaClevelTypes =
	(int) (sizeof(FpaClevelTypes) / sizeof(FPA_MACRO_LIST));


/** Define level categories of Levels block in configuration file */
static const FPA_MACRO_LIST FpaClevelsCategories[] =
	{
		{ FpaC_LEVELS_MSL,            "Msl"        },
		{ FpaC_LEVELS_SURFACE,        "Surface"    },
		{ FpaC_LEVELS_PRESSURE,       "Pressure"   },
		{ FpaC_LEVELS_HEIGHT,         "Height"     },
		{ FpaC_LEVELS_SIGMA,          "Sigma"      },
		{ FpaC_LEVELS_THETA,          "Theta"      },
		{ FpaC_LEVELS_GEOGRAPHY,      "Geography"  },
		{ FpaC_LEVELS_ANNOTATION,     "Annotation" },
	};

/** Set number of predefined level categories */
static const int NumFpaClevelsCategories =
	(int) (sizeof(FpaClevelsCategories) / sizeof(FPA_MACRO_LIST));


/***** Enumerated type macros for Elements block *****/


/** Define field types of Elements block in configuration file */
static const FPA_MACRO_LIST FpaCfieldTypes[] =
	{
		{ FpaC_CONTINUOUS,       "Continuous" },
		{ FpaC_VECTOR,           "Vector"     },
		{ FpaC_DISCRETE,         "Discrete"   },
		{ FpaC_WIND,             "Wind"       },
		{ FpaC_LINE,             "Line"       },
		{ FpaC_SCATTERED,        "Scattered"  },
		{ FpaC_LCHAIN,           "LChain"     },
		{ FpaC_SPECIAL,          "Special"    },
		{ FpaC_ELEM_NOTUSED,     "NotUsed"    },
	};

/** Set number of predefined field types */
static const int NumFpaCfieldTypes =
	(int) (sizeof(FpaCfieldTypes) / sizeof(FPA_MACRO_LIST));


/** Define display formats of Elements block in configuration file */
static const FPA_MACRO_LIST FpaCdisplayFormats[] =
	{
		{ DisplayFormatSimple,     "Simple"  },
		{ DisplayFormatComplex,    "Complex" },
	};

/** Set number of predefined display formats */
static const int NumFpaCdisplayFormats =
	(int) (sizeof(FpaCdisplayFormats) / sizeof(FPA_MACRO_LIST));


/** Define time dependence types of Elements block in configuration file */
static const FPA_MACRO_LIST FpaCtimeDepTypes[] =
	{
		{ FpaC_DAILY,      "Daily"   },
		{ FpaC_STATIC,     "Static"  },
		{ FpaC_NORMAL,     "Normal"  },
	};

/** Set number of predefined time dependence types */
static const int NumFpaCtimeDepTypes =
	(int) (sizeof(FpaCtimeDepTypes) / sizeof(FPA_MACRO_LIST));


/** Define wind classes of Elements block in configuration file */
static const FPA_MACRO_LIST FpaCwindClasses[] =
	{
		{ FpaC_PRESSURE,       "Pressure"   },
		{ FpaC_HEIGHT,         "Height"     },
		{ FpaC_THICKNESS,      "Thickness"  },
		{ FpaC_ADJUSTMENT,     "Adjustment" },
		{ FpaC_NOWIND,         "None"       },
	};

/** Set number of predefined wind classes */
static const int NumFpaCwindClasses =
	(int) (sizeof(FpaCwindClasses) / sizeof(FPA_MACRO_LIST));


/** Define attachment options for Elements block in configuration file */
static const FPA_MACRO_LIST FpaCattachOpts[] =
	{
		{ FpaC_NO_ATTACH,           "no_attach"       },
		{ FpaC_ATTACH_AUTO,         "attach_auto"     },
		{ FpaC_ATTACH_MIN,          "attach_min"      },
		{ FpaC_ATTACH_MAX,          "attach_max"      },
		{ FpaC_ATTACH_COL,          "attach_col"      },
		{ FpaC_ATTACH_CONTOUR,      "attach_contour"  },
		{ FpaC_ATTACH_BOUND,        "attach_boundary" },
		{ FpaC_ATTACH_DIV,          "attach_divide"   },
		{ FpaC_ATTACH_LINE,         "attach_line"     },
		{ FpaC_ATTACH_POINT,        "attach_point"    },
	};

/** Set number of predefined attachment options */
static const int NumFpaCattachOpts =
	(int) (sizeof(FpaCattachOpts) / sizeof(FPA_MACRO_LIST));


/***** Enumerated type macros for Samples block *****/


/** Define value sample types of Samples block in configuration file */
static const FPA_MACRO_LIST FpaCsampleTypes[] =
	{
		{ FpaC_SAMPLE_VALUE,         "FPA_Sample_Value"         },
		{ FpaC_SAMPLE_GRADIENT,      "FPA_Sample_Gradient"      },
		{ FpaC_SAMPLE_CURVATURE,     "FPA_Sample_Curvature"     },
		{ FpaC_SAMPLE_MAGNITUDE,     "FPA_Sample_Magnitude"     },
		{ FpaC_SAMPLE_DIRECTION,     "FPA_Sample_Direction"     },
		{ FpaC_SAMPLE_LABEL,         "FPA_Sample_Label"         },
		{ FpaC_SAMPLE_CATEGORY,      "FPA_Sample_Category"      },
		{ FpaC_SAMPLE_ATTRIBUTE,     "FPA_Sample_Attribute"     },
	};

/** Set number of predefined sample types */
static const int NumFpaCsampleTypes =
	(int) (sizeof(FpaCsampleTypes) / sizeof(FPA_MACRO_LIST));


#endif


/* Now it has been included */
#endif
