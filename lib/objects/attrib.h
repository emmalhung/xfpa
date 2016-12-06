/**********************************************************************/
/** @file attrib.h
 *
 *  ATTRIB and ATTRIB_LIST object definitions (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    a t t r i b . h                                                   *
*                                                                      *
*    ATTRIB and ATTRIB_LIST object definitions (include file)          *
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
#ifndef ATTRIB_DEFS
#define ATTRIB_DEFS

#include <fpa_types.h>

#define	DELTA_ATTRIB 1

/* Define ATTRIB structure */
/** a single attribute */
typedef	struct
	{
	STRING	name;	/**< attribute identifier */
	STRING	value;	/**< the actual value of this attribute */
	} ATTRIB;

/* Define ATTRIB_LIST object */
/** a set of attributes for one object */
typedef struct ATTRIB_LIST_struct
	{
	ATTRIB	*attribs;	/**< list of attributes */
	int		nattribs;	/**< number of attributes */
	POINTER	defs;		/**< pointer to config defaults (see environ/cal.c) */
	} *ATTRIB_LIST;

/* Define CAL (Controlled Attribute List) object */
#define	CAL	ATTRIB_LIST

/* Define ERULE object - attribute entry rule function pointer */
typedef	void		(ERULE_FUNC)(CAL);
typedef	ERULE_FUNC	(*ERULE);

/* Convenient definitions */
#define NullAttrib        NullPtr(ATTRIB)
#define NullAttribPtr     NullPtr(ATTRIB *)
#define NullAttribList    NullPtr(ATTRIB_LIST)
#define NullAttribListPtr NullPtr(ATTRIB_LIST *)
#define NullCal           NullPtr(CAL)
#define NullCalPtr        NullPtr(CAL *)
#define NullErule         NullPtr(ERULE)
#define NullEruleList     NullPtr(ERULE *)

/* Special attributes that relate to special processing */
/* Category, AutoLabel and UserLabel will replace subelement, value and */
/* label respectively */
#define	AttribAll                  "FPA_all_attrib"
#define	AttribAllLabel             "All Attributes"
#define	AttribFieldLabels          "FPA_field_labels"
#define	AttribFieldLabelsLabel     "Field Labels"
#define	AttribLinkNodes            "FPA_link_nodes"
#define	AttribLinkNodesLabel       "Link nodes"
#define	AttribLatitude             "FPA_latitude"
#define	AttribLatitudeLabel        "Latitude"
#define	AttribLongitude            "FPA_longitude"
#define	AttribLongitudeLabel       "Longitude"
#define	AttribProximity            "FPA_proximity"
#define	AttribProximityLabel       "Proximity (km)"
#define	AttribAreaSize             "FPA_area_size"
#define	AttribAreaSizeLabel        "Area Size (km2)"
#define	AttribLineDirection        "FPA_line_direction"
#define	AttribLineDirectionLabel   "Line Direction"
#define	AttribLineLength           "FPA_line_length"
#define	AttribLineLengthLabel      "Line Length (km)"
#define	AttribCategory             "FPA_category"
#define	AttribCategoryLabel        "Category"
#define	AttribCategoryDefault      "default"
#define	AttribCategoryAll          "All"
#define	AttribAutolabel            "FPA_auto_label"
#define	AttribAutolabelLabel       "Value"
#define	AttribAutolabelDefault     "No Label"
#define	AttribUserlabel            "FPA_user_label"
#define	AttribUserlabelLabel       "Label"
#define	AttribUserlabelDefault     "No Label"
#define	AttribWindModel            "FPA_wind_model"
#define	AttribWindModelLabel       "Wind Model"
#define	AttribWindDirection        "FPA_wind_direction"
#define	AttribWindDirectionLabel   "Wind Direction"
#define	AttribWindSpeed            "FPA_wind_speed"
#define	AttribWindSpeedLabel       "Wind Speed"
#define	AttribWindGust             "FPA_wind_gust"
#define	AttribWindGustLabel        "Wind Gust"
#define	AttribLineType             "FPA_line_type"
#define	AttribLineTypeLabel        "Line Type"
#define	AttribLineTypeDefault      "default"
#define	AttribScatteredType        "FPA_scattered_type"
#define	AttribScatteredTypeLabel   "Scattered Type"
#define	AttribScatteredTypeDefault "default"
#define	AttribLchainReference      "FPA_lchain_reference"
#define	AttribLchainReferenceLabel "Reference Time"
#define	AttribLchainStartTime      "FPA_lchain_start_time"
#define	AttribLchainStartTimeLabel "Start Time"
#define	AttribLchainEndTime        "FPA_lchain_end_time"
#define	AttribLchainEndTimeLabel   "End Time"
#define	AttribLchainStartTstamp      "FPA_lchain_start_tstamp"
#define	AttribLchainStartTstampLabel "Start Timestamp"
#define	AttribLchainEndTstamp        "FPA_lchain_end_tstamp"
#define	AttribLchainEndTstampLabel   "End Timestamp"
#define	AttribLnodeType            "FPA_lnode_type"
#define	AttribLnodeTypeLabel       "Node Type"
#define	AttribLnodeTime            "FPA_lnode_time"
#define	AttribLnodeTimeLabel       "Node Time"
#define	AttribLnodeTstamp            "FPA_lnode_tstamp"
#define	AttribLnodeTstampLabel       "Node Timestamp"
#define	AttribLnodeDirection       "FPA_lnode_direction"
#define	AttribLnodeDirectionLabel  "Node Direction"
#define	AttribLnodeSpeed           "FPA_lnode_speed"
#define	AttribLnodeSpeedLabel      "Node Speed (m/s)"
#define	AttribLnodeVector          "FPA_lnode_vector"
#define	AttribLnodeVectorLabel     "Node Speed@Direction"
#define	AttribLabelType            "FPA_label_type"
#define	AttribLabelTypeLabel       "Label Type"
#define	AttribLabelFeature         "FPA_label_feature"
#define	AttribLabelFeatureLabel    "Label Feature"
#define	AttribEvalContour          "EVAL_contour"
#define	AttribEvalContourLabel     "Contour Value"
#define	AttribEvalSpval            "EVAL_spval"
#define	AttribEvalSpvalLabel       "Field Value"
#define	AttribEvalWind             "EVAL_wind"
#define	AttribEvalWindLabel        "Wind Value"
#define	AttribEvalVector           "EVAL_vector"
#define	AttribEvalVectorLabel      "Vector Value"
#define	AttribReferenceLatitude    "FPA_reference_latitude"
#define	AttribReferenceLatLabel    "Reference Latitude"
#define	AttribReferenceLongitude   "FPA_reference_longitude"
#define	AttribReferenceLonLabel    "Reference Longitude"
#define	AttribReferenceTime        "FPA_reference_time"
#define	AttribReferenceTimeLabel   "Reference Time"

/* Declare all functions in attrib.c */
ATTRIB_LIST	create_attrib_list(void);
ATTRIB_LIST	destroy_attrib_list(ATTRIB_LIST al);
void		empty_attrib_list(ATTRIB_LIST al);
void		clean_attrib_list(ATTRIB_LIST al);
ATTRIB_LIST	copy_attrib_list(const ATTRIB_LIST al);
ATTRIB		*add_attribute(ATTRIB_LIST al, STRING name, STRING val);
ATTRIB		*set_attribute(ATTRIB_LIST al, STRING name, STRING val);
ATTRIB		*get_attribute(ATTRIB_LIST al, STRING name, STRING *val);
ATTRIB		*remove_attribute(ATTRIB_LIST al, STRING name);
ATTRIB		*find_attribute(ATTRIB_LIST al, STRING name);
void		init_attrib(ATTRIB *att);
void		free_attrib(ATTRIB *att);
void		copy_attrib(ATTRIB *to, const ATTRIB *from);
void		define_attrib(ATTRIB *att, STRING name, STRING value);
void		recall_attrib(ATTRIB *att, STRING *name, STRING *value);
ATTRIB_LIST	create_default_attrib_list(STRING cat, STRING alab, STRING ulab);
void		add_default_attributes(ATTRIB_LIST al,
									STRING cat, STRING alab, STRING ulab);
void		set_default_attributes(ATTRIB_LIST al,
									STRING cat, STRING alab, STRING ulab);
void		get_default_attributes(ATTRIB_LIST al,
									STRING *cat, STRING *alab, STRING *ulab);
LOGICAL		same_attrib_list(const ATTRIB_LIST al1, const ATTRIB_LIST al2);
void		debug_attrib_list(STRING msg, ATTRIB_LIST al);

/* Now it has been included */
#endif
