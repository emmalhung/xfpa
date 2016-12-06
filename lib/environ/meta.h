/**********************************************************************/
/** @file meta.h
 *
 *  Routines to read and write metafiles (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   m e t a . h                                                        *
*                                                                      *
*   Routines to read and write metafiles (include file)                *
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
#ifndef META_DEFS
#define META_DEFS


/* We need definitions for low level types and other Objects */
#include "config_structs.h"
#include <objects/objects.h>
#include <fpa_types.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for metafile read/write routines       *
*                                                                      *
***********************************************************************/

#ifdef META_INIT


#endif

#define	META_LATLON 1
#define META_OLDFMT 2
#define META_DOMAX  4

#define	MetaRetainPlot			"RetainPlot"
#define	MetaNoRetainPlot		"NoRetainPlot"
#define	MetaRasterReadAll		"RasterReadAll"
#define	MetaRasterReadMetadata	"RasterReadMetadata"


/***********************************************************************
*                                                                      *
*  Declare external functions in meta_read.c and meta_write.c          *
*                                                                      *
***********************************************************************/

void		set_metafile_input_mode(STRING mode);
METAFILE	read_metafile(STRING  meta_name, const MAP_PROJ *bproj);
MAP_PROJ	*find_meta_map_projection(STRING meta_name);
STRING		find_meta_revision(STRING meta_name);
LOGICAL		read_meta_info(STRING meta_name, STRING process,
						int *nlist, TABLE **list);
int			search_meta_fields(STRING meta_name, FpaConfigFieldStruct ***fdefs);
int			search_link_fields(STRING link_name, FpaConfigFieldStruct ***fdefs);
LOGICAL		parse_metafile_projection(STRING tbuf, PROJ_DEF *proj);
LOGICAL		parse_metafile_mapdef(STRING tbuf, MAP_DEF *mdef);

void		backup_metafile(STRING meta_name, STRING backup_name,
						METAFILE meta, int maxdig);
void		write_metafile_special(STRING meta_name,
						METAFILE meta, int maxdig, int mode);
void		write_metafile(STRING meta_name, METAFILE meta, int maxdig);
STRING		format_metafile_projection(PROJ_DEF *proj);
STRING		format_metafile_mapdef(MAP_DEF *mdef, int maxdig);


/* Now it has been included */
#endif
