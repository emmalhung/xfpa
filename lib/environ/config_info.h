/**********************************************************************/
/** @file config_info.h
 *
 *  Routines for accessing new Version 4.0 configuration file
 *   information (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c o n f i g _ i n f o . h                                          *
*                                                                      *
*   Routines for accessing new Version 4.0 configuration file          *
*    information (include file)                                        *
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
***********************************************************************/

/* See if already included */
#ifndef CONFIG_INFO_DEFS
#define CONFIG_INFO_DEFS


/* We need definitions for low level types */
#include <fpa_types.h>

/* We need definitions for configuration file structures */
#include "config_structs.h"


/***********************************************************************
*                                                                      *
*  Initialize structures and defined constants for config_info         *
*   routines                                                           *
*                                                                      *
************************************************************************/

#ifdef CONFIG_INFO_INIT

/** Fixed lengths for internal labels */
#define CONFIG_LABEL_LEN		64

/** Structures for holding block identifiers */
typedef struct
	{
	STRING		ident;
	POINTER		pdef;
	} FPAC_IDENTS;
/** Structures for holding block identifiers */
typedef struct
	{
	STRING		element;
	STRING		level;
	POINTER		pdef;
	} FPAC_FIELD_IDENTS;

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in config_info.c                         *
*                                                                      *
***********************************************************************/

/* General configuration file routines */
LOGICAL	read_complete_config_file(void);
LOGICAL	consistent_element_and_level(FpaConfigElementStruct *edef,
							FpaConfigLevelStruct *ldef);
LOGICAL	minutes_in_depictions(void);
LOGICAL	check_depiction_minutes(void);
STRING	entity_from_field_type(int ftype);
LOGICAL	check_field_macro(int macro);
int		field_data_type(STRING fname);
int		field_display_format(STRING element, STRING level);
LOGICAL	check_attach_option(FpaCfieldTypeOption ftype,
							FpaCattachOption attach, SPFEAT *feature);
LOGICAL	parse_source_name(STRING ident, STRING *source, STRING *subsrc);

LOGICAL		xy_component_field(STRING elem);
LOGICAL		dm_component_field(STRING elem);
COMPONENT	which_components(STRING elem,
							STRING *comp_name, COMPONENT *comp_type);
LOGICAL		check_reprojection_for_components(STRING elem,
							MAP_PROJ *mprojin, MAP_PROJ *mprojout);

/* Routines for Units block */
FpaConfigUnitStruct		*identify_unit(STRING uname);
int		identify_mks_units(FpaConfigUnitStruct ***udefs);
int		identify_mks_units_free(FpaConfigUnitStruct ***udefs, int num);
int		identify_units_by_mks(STRING uname, FpaConfigUnitStruct ***udefs);
int		identify_units_by_mks_free(FpaConfigUnitStruct ***udefs, int num);
LOGICAL	convert_value(STRING ufrom, double value, STRING uto, double *newvalue);

/* Routines for Constants block */
FpaConfigConstantStruct	*identify_constant(STRING cname);
int		identify_constants_by_group(STRING cgroup,
							FpaConfigConstantStruct ***cdefs);
int		identify_constants_by_group_free(FpaConfigConstantStruct ***cdefs,
							int num);

/* Routines for Sources block */
FpaConfigSourceStruct	*identify_source(STRING source, STRING subsrc);
FpaConfigSourceStruct	*get_source_info(STRING source, STRING subsrc);
int		identify_sources_by_type(int stype, FpaConfigSourceStruct ***sdefs);
int		identify_sources_by_type_free(FpaConfigSourceStruct ***sdefs, int num);
int		identify_source_aliases(FpaConfigSourceStruct *sdef, STRING **slist);
int		identify_source_aliases_free(STRING **slist, int num);
LOGICAL	equivalent_source_definitions(STRING source1, STRING subsrc1,
							STRING source2, STRING subsrc2);
int		source_allied_data_location(FpaConfigSourceStruct *sdef,
							int atype, STRING alias);

/* Routines for Groups block */
FpaConfigGroupStruct	*identify_group(STRING gtype, STRING gname);
int		identify_groups_for_fields(FpaConfigGroupStruct ***gdefs);
int		identify_groups_for_fields_free(FpaConfigGroupStruct ***gdefs, int num);
int		identify_groups_for_elements(FpaConfigGroupStruct ***gdefs);
int		identify_groups_for_elements_free(FpaConfigGroupStruct ***gdefs,
							int num);

/* Routines for Levels block */
FpaConfigLevelStruct	*identify_level(STRING level);
FpaConfigLevelStruct	*identify_level_from_levels(int ltype,
							STRING single, STRING upper, STRING lower);
int		identify_levels_by_type(int ltype, FpaConfigLevelStruct ***ldefs);
int		identify_levels_by_type_free(FpaConfigLevelStruct ***ldefs, int num);
int		identify_level_aliases(FpaConfigLevelStruct *ldef, STRING **llist);
int		identify_level_aliases_free(STRING **llist, int num);
LOGICAL	equivalent_level_definitions(STRING level1, STRING level2);

/* Routines for Elements block */
FpaConfigElementStruct	*identify_element(STRING elem);
FpaConfigElementStruct	*get_element_info(STRING elem);
int		identify_elements_by_group(STRING gname,
							FpaConfigElementStruct ***edefs);
int		identify_elements_by_group_free(FpaConfigElementStruct ***edefs,
							int num);
int		identify_element_aliases(FpaConfigElementStruct *edef, STRING **elist);
int		identify_element_aliases_free(STRING **elist, int num);
int		identify_line_type_by_name(STRING elem, STRING level, STRING type,
							FpaConfigElementLineTypeStruct **eldefs);
int		identify_scattered_type_by_name(STRING elem, STRING level, STRING type,
							FpaConfigElementScatteredTypeStruct **esdefs);
int		identify_labelling_type_by_name(STRING elem, STRING level, STRING type,
							FpaConfigElementLabellingStruct **exdefs);
LOGICAL	equivalent_element_definitions(STRING elem1, STRING elem2);

/* Routines for Fields block */
FpaConfigFieldStruct	*identify_field(STRING elem, STRING level);
FpaConfigFieldStruct	*get_field_info(STRING elem, STRING level);
int		identify_fields_by_group(STRING gname, FpaConfigFieldStruct ***fdefs);
int		identify_fields_by_group_free(FpaConfigFieldStruct ***fdefs, int num);

/* Routines for CrossRefs block */
FpaConfigCrossRefStruct	*identify_crossref(STRING crtype, STRING crname);
int		identify_crossrefs_for_winds(FpaConfigCrossRefStruct ***crdefs);
int		identify_crossrefs_for_winds_free(FpaConfigCrossRefStruct ***crdefs,
							int num);
int		identify_crossrefs_for_values(FpaConfigCrossRefStruct ***crdefs);
int		identify_crossrefs_for_values_free(FpaConfigCrossRefStruct ***crdefs,
							int num);

/* Routines for Samples block */
FpaConfigSampleStruct	*identify_sample(STRING ptype, STRING pname);
int		identify_samples_for_values(FpaConfigSampleStruct ***pdefs);
int		identify_samples_for_values_free(FpaConfigSampleStruct ***pdefs,
							int num);
int		identify_samples_for_winds(FpaConfigSampleStruct ***pdefs);
int		identify_samples_for_winds_free(FpaConfigSampleStruct ***pdefs,
							int num);


/* Now it has been included */
#endif
