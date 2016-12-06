/**********************************************************************/
/** @file cal.h
 *
 *  Routines to the Controlled Attribute List (CAL) (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   c a l . h                                                          *
*                                                                      *
*   Routines to handle the Controlled Attribute List (CAL)             *
*   (include file)                                                     *
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
#ifndef CAL_DEFS
#define CAL_DEFS


/* We need definitions for low level types and other Objects */
#include <tools/tools.h>
#include <objects/objects.h>
#include <fpa_types.h>

#include "config_info.h"


/***********************************************************************
*                                                                      *
*  Initialize defined constants for CAL handling routines              *
*                                                                      *
***********************************************************************/
/** @name Preset Attributes */
/* @{ */
#define	CALlatitude              AttribLatitude				/**< Latitude */
#define	CALlongitude             AttribLongitude			/**< Longitude */
#define	CALproximity             AttribProximity			/**< Proximity */
#define	CALcategory              AttribCategory				/**< Category*/
#define	CALautolabel             AttribAutolabel			/**< Auto Label */
#define	CALuserlabel             AttribUserlabel			/**< User Label */
#define	CALlinetype              AttribLineType				/**< Line Type */
#define	CALlinetypedefault       AttribLineTypeDefault		/**< Line Type Default */
#define	CALscatteredtype         AttribScatteredType		/**< Scattered Type */
#define	CALscatteredtypedefault  AttribScatteredTypeDefault	/**< Scattered Type Default */
#define	CALlabeltype             AttribLabelType			/**< Label Type */
/* @} */

#define	CAL_NO_VALUE ""									/**< Empty value */
#define	CAL_is_value(val) (!blank(val))					/**< Test if value is not blank */
#define	CAL_no_value(val) (blank(val))					/**< Test if value is blank */
#define	CAL_is_preset(val) (same_start(val, "FPA_"))	/**< Test if value is preset */
#ifdef CAL_INIT

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in cal.c                                 *
*                                                                      *
***********************************************************************/
CAL		CAL_create_by_name(STRING elem, STRING level);
CAL		CAL_create_by_edef(FpaConfigElementStruct *edef);
CAL		CAL_create_default(void);
CAL		CAL_create_empty(void);
CAL		CAL_destroy(CAL cal);
void	CAL_empty(CAL cal);
void	CAL_clean(CAL cal);
CAL		CAL_duplicate(const CAL cal);
void	CAL_add_attribute(CAL cal, STRING name, STRING value);
void	CAL_set_attribute(CAL cal, STRING name, STRING value);
STRING	CAL_get_attribute(CAL cal, STRING name);
LOGICAL	CAL_has_attribute(CAL cal, STRING name);
void	CAL_add_location(CAL cal, MAP_PROJ *mproj, POINT pos);
void	CAL_add_proximity(CAL cal, MAP_PROJ *mproj, POINT spos, POINT epos);
void	CAL_add_negative_proximity(CAL cal,
									MAP_PROJ *mproj, POINT spos, POINT epos);
void	CAL_add_area_size(CAL cal, MAP_PROJ *mproj, POINT spos, float size);
void	CAL_add_line_dir(CAL cal, MAP_PROJ *mproj, POINT spos, POINT epos);
void	CAL_add_line_len(CAL cal, MAP_PROJ *mproj, LINE line);
void	CAL_add_lchain_node_motion(CAL cal,
									MAP_PROJ *mproj, LCHAIN lchain, int mplus);
void	CAL_set_defaults(CAL cal, STRING sub, STRING val, STRING lab);
void	CAL_get_defaults(CAL cal, STRING *sub, STRING *val, STRING *lab);
void	CAL_get_attribute_names(CAL cal, STRING **names, int *num);
void	CAL_merge(CAL cal, CAL scal, LOGICAL overwrite);
LOGICAL	CAL_same(CAL cal1, CAL cal2);
LOGICAL	valid_field_attribute(STRING elem, STRING level, STRING name);
LOGICAL	valid_edef_attribute(FpaConfigElementStruct *edef, STRING name);
SPOT	create_spot_by_name(STRING elem, STRING level, POINT pos, CAL cal);


/* Now it has been included */
#endif
