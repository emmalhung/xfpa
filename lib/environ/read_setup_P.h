/**********************************************************************/
/** @file read_setup_P.h
 *
 *  Private data for reading setup and configuration files (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r e a d _ s e t u p _ P . h                                        *
*                                                                      *
*   Private data for reading setup and configuration files (include    *
*   file)                                                              *
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

#include <fpa_types.h>

/***********************************************************************
*                                                                      *
*  Initialize defined constants and static lists for read_setup        *
*  routines                                                            *
*                                                                      *
***********************************************************************/

#ifdef READ_SETUP_INIT

/* These are the standard set of directory key:path pairs with fallback */
/* directory */

static TABLE2 default_dirs[] = {
    { "AModels.Data"       , "AModels.DATA"                      ,
									NULL                               },
    { "AModels.Exec"       , "$FPA/AModels.EXEC"                 ,
									NULL                               },
    { "config"             , "$HOME/config"                      ,
									NULL                               },
    { "cormet"             , "$HOME/setup/pdf/cormet"            ,
									NULL                               },
    { "corout"             , "CorOut"                            ,
									NULL                               },
    { "cormet_symbols"     , "$HOME/setup/pdf/cormet/common/cmf" ,
									"$FPA/setup/pdf/cormet/common/cmf" },
    { "ctables"            , "$HOME/config/Ctables"              ,
									NULL                               },
    { "Data"               , ""                                  ,
									NULL                               },
    { "dcw"                , "$FPA/data/dcw"                     ,
									NULL                               },
    { "ExternalDepictions" , "EXTERNAL"                          ,
									NULL                               },
    { "graphics"           , "$HOME/setup/pdf/graphics"          ,
									NULL                               },
    { "Guidance"           , "Guidance"                          ,
									NULL                               },
    { "fcst.work"          , "Forecast/Working"                  ,
									NULL                               },
    { "fcst.release"       , "Forecast/Released"                 ,
									NULL                               },
    { "fcst.concept"       , "Forecast/Concepts"                 ,
									NULL                               },
    { "help.source"        , "$FPA/doc/online"                   ,
									NULL                               },
    { "ingest.src"         , "$FPA_LOCAL_GRIB"                   ,
									NULL                               },
    { "ingest.stat"        , "Guidance"                          ,
									NULL                               },
    { "ingest.log"         , "Guidance"                          ,
									NULL                               },
    { "lookups"            , "$HOME/config/lookups"              ,
									"$FPA/config/lookups"              },
    { "map.cfg"            , "$FPA/config/App_Map"               ,
									NULL                               },
    { "Maps"               , "Maps"                              ,
									"$FPA/data/common/CommonMaps"      },
    { "maps.common"        , "$FPA/data/common/CommonMaps"       ,
									NULL                               },
    { "memory.cfg"         , "$HOME/config/Memory"               ,
									"$FPA/config/Memory"               },
    { "menus.cfg"          , "$HOME/config/Menus"                ,
									"$FPA/config/Menus"                },
    { "metafiles"          , "$HOME/setup/pdf/metafiles"         ,
									NULL                               },
    { "patterns"           , "$HOME/config/patterns"             ,
									"$FPA/config/patterns"             },
    { "point_fcst"         , "$HOME/setup/pdf/point_fcst"        ,
									NULL                               },
    { "preset_lists"       , "$HOME/setup/preset_lists"          ,
									"$FPA/setup/preset_lists"          },
    { "psmet"              , "$HOME/setup/pdf/psmet"             ,
									NULL                               },
    { "psout"              , "PSOut"                             ,
									NULL                               },
    { "psmet_symbols"      , "$HOME/setup/pdf/psmet/common/ps"   ,
									"$FPA/setup/pdf/psmet/common/ps"   },
    { "scratch_files"      , "/tmp"                              ,
									NULL                               },
    { "setup"              , "$HOME/setup"                       ,
									NULL                               },
    { "svgmet"              , "$HOME/setup/pdf/svgmet"           ,
									NULL                               },
    { "svgout"              , "SVGOut"                           ,
									NULL                               },
    { "svgmet_symbols"      , "$HOME/setup/pdf/svgmet/common/svg" ,
									"$FPA/setup/pdf/svgmet/common/svg" },
    { "symbols"            , "$HOME/config/symbols"              ,
									"$FPA/config/symbols"              },
    { "texmet"             , "$HOME/setup/pdf/texmet"            ,
									NULL                               },
    { "texout"             , "TexOut"                            ,
									NULL                               },
};

static TABLE default_config_dirs[] = {
	{ "config",       "Config"                },
	{ "presentation", "Presentation"          },
	{ "forecasts",    "$FPA/config/Forecasts" },
	{ "gribs",        "$FPA/config/Gribs"     },
	{ "image",        "$FPA/config/Image"     }
};


#endif
