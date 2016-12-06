/*********************************************************************/
/** @file metafile.h
 *
 * METAFILE object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    m e t a f i l e . h                                               *
*                                                                      *
*    METAFILE object definitions (include file)                        *
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
#ifndef METAFILE_DEFS
#define METAFILE_DEFS

/* We need definitions for various objects */
#include "field.h"
#include "raster.h"
#include "projection.h"

/* Define LEGEND object */
typedef STRING	LEGEND;

/** Define METAFILE object */
typedef struct METAFILE_struct
	{
	STRING					istamp;		/**< issue timestamp */
	STRING					vstamp;		/**< valid timestamp */
	STRING					tag;		/**< arbitrary tag */
	MAP_PROJ				mproj;		/**< map projection */
	int						nsrc;		/**< nsrc */
	MAP_PROJ				*sproj;		/**< source projection list */
	COMP_INFO				*scomp;		/**< comp info */
	STRING					bgndname;	/**< background file */
	struct METAFILE_struct	*bgnd;		/**< background structure */
	LEGEND					lgnd1;		/**< legends */
	LEGEND					lgnd2;		/**< legends */
	FIELD					*fields;	/**< field buffer */
	int						numfld;		/**< number of fields */
	int						maxfld;		/**< max field number */
	} *METAFILE;


/* Convenient definitions */
#define NullMeta    NullPtr(METAFILE)
#define NullMetaPtr NullPtr(METAFILE *)

/* Declare all functions in metafile.c */
METAFILE	create_metafile(void);
METAFILE	copy_metafile(const METAFILE mf);
METAFILE	merge_metafiles(int nummeta, const METAFILE *metain);
METAFILE	destroy_metafile(METAFILE mf);
void		empty_metafile(METAFILE mf);
void		define_mf_tstamp(METAFILE mf, STRING issue, STRING valid);
void		define_mf_model(METAFILE mf, STRING model);
void		define_mf_projection(METAFILE mf, const MAP_PROJ *mproj);
void		define_mf_bgnd(METAFILE mf, STRING bgndname, METAFILE bgnd);
void		define_mf_lgnd(METAFILE mf, STRING lgnd1, STRING lgnd2);
void		free_mf_source_proj(METAFILE mf);
int			find_mf_source_proj(METAFILE mf, const MAP_PROJ *mproj);
void		add_mf_source_proj(METAFILE mf, const MAP_PROJ *mproj);
float		coverage_mf_source_proj(METAFILE mf);
void		define_mf_source_comp(METAFILE mf, int isrc,
						const COMP_INFO *cinfo);
LOGICAL		merge_mf_source_comp(METAFILE mf, int isrc,
						const COMP_INFO *cinfo);
LOGICAL		ready_mf_source_comp(METAFILE mf);
FIELD		add_field_to_metafile(METAFILE mf, FIELD fld);
SURFACE		add_sfc_to_metafile(METAFILE mf,
						STRING entity, STRING elem, STRING level, SURFACE sfc);
RASTER		add_raster_to_metafile(METAFILE mf,
						STRING entity, STRING elem, STRING level, RASTER rast);
SET			add_set_to_metafile(METAFILE mf,
						STRING entity, STRING elem, STRING level, SET set);
SET			add_item_to_metafile(METAFILE mf, STRING settype,
						STRING entity, STRING elem, STRING level, ITEM item);
SET			add_item_to_metafile_start(METAFILE mf, STRING settype,
						STRING entity, STRING elem, STRING level, ITEM item);
PLOT		add_plot_to_metafile(METAFILE mf,
						STRING entity, STRING elem, STRING level, PLOT plot);
FIELD		make_mf_field(METAFILE mf, STRING fldtype, STRING settype,
						STRING entity, STRING elem, STRING level);
SURFACE		make_mf_sfc(METAFILE mf, STRING entity, STRING elem, STRING level);
RASTER		make_mf_raster(METAFILE mf, STRING entity, STRING elem, STRING level);
SET			make_mf_set(METAFILE mf, STRING settype,
						STRING entity, STRING elem, STRING level);
PLOT		make_mf_plot(METAFILE mf, STRING entity, STRING elem, STRING level);
FIELD		copy_mf_field(METAFILE mf, STRING fldtype, STRING settype,
						STRING entity, STRING elem, STRING level);
SURFACE		copy_mf_sfc(METAFILE mf, STRING entity, STRING elem, STRING level);
RASTER		copy_mf_raster(METAFILE mf, STRING entity, STRING elem, STRING level);
SET			copy_mf_set(METAFILE mf, STRING settype,
						STRING entity, STRING elem, STRING level);
PLOT		copy_mf_plot(METAFILE mf, STRING entity, STRING elem, STRING level);
FIELD		find_mf_field(METAFILE mf, STRING fldtype, STRING settype,
						STRING entity, STRING elem, STRING level);
SURFACE		find_mf_sfc(METAFILE mf, STRING entity, STRING elem, STRING level);
RASTER		find_mf_raster(METAFILE mf, STRING entity, STRING elem, STRING level);
SET			find_mf_set(METAFILE mf, STRING settype,
						STRING entity, STRING elem, STRING level);
PLOT		find_mf_plot(METAFILE mf, STRING entity, STRING elem, STRING level);
FIELD		take_mf_field(METAFILE mf, STRING fieldtype, STRING settype,
						STRING entity, STRING elem, STRING level);
SURFACE		take_mf_sfc(METAFILE mf, STRING entity, STRING elem, STRING level);
RASTER		take_mf_raster(METAFILE mf, STRING entity, STRING elem, STRING level);
SET			take_mf_set(METAFILE mf, STRING settype,
						STRING entity, STRING elem, STRING level);
PLOT		take_mf_plot(METAFILE mf, STRING entity, STRING elem, STRING level);
LOGICAL		reproject_xy_metafiles(METAFILE umeta, METAFILE vmeta,
						const MAP_PROJ *srcmproj, const MAP_PROJ *targetmproj);

/* Now it has been included */
#endif
