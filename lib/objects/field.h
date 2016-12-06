/*********************************************************************/
/** @file field.h
 *
 * FIELD object definitions (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    f i e l d . h                                                     *
*                                                                      *
*    FIELD object definitions (include file)                           *
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
#ifndef FIELD_DEFS
#define FIELD_DEFS

/* We need definitions for various objects */
#include "surface.h"
#include "plot.h"
#include "set.h"
#include "raster.h"

/** FIELD type descriptor */
typedef	enum	{ FtypeNone, FtypeSfc, FtypeSet, FtypePlot, FtypeRaster } FTYPE;

/** FIELD object */
typedef struct FIELD_struct
	{
	STRING	entity;		/**< entity */
	STRING	element;	/**< element */
	STRING	level;		/**< level (sfc, pressure, sigma, theta...) */
	FTYPE	ftype;		/**< which type? */
	/** actual data */
	union	{
			SURFACE	sfc;	/**< scalar field data */
			SET		set;	/**< set data */
			PLOT	plot;	/**< plot data */
			RASTER	raster;	/**< raster object */
			} data;
	} *FIELD;


/* Convenient definitions */
#define DELTA_FIELD 1
#define NullFld        NullPtr(FIELD)
#define NullFldPtr     NullPtr(FIELD *)
#define NullFldList    NullPtr(FIELD *)
#define NullFldListPtr NullPtr(FIELD **)

/* Declare all functions in field.c */
FIELD		create_field(STRING entity, STRING element, STRING level);
FIELD		copy_field(const FIELD fld);
FIELD		destroy_field(FIELD fld);
void		define_fld_info(FIELD fld,
						STRING entity, STRING element, STRING level);
void		recall_fld_info(FIELD fld,
						STRING *entity, STRING *element, STRING *level);
void		define_fld_data(FIELD fld, STRING type, POINTER data);
void		recall_fld_data(FIELD fld, STRING *type, POINTER *data);
void		delete_fld_data(FIELD fld);
void		change_fld_pspec(FIELD fld, PPARAM param, POINTER value);
void		recall_fld_pspec(FIELD fld, PPARAM param, POINTER value);
void		highlight_field(FIELD fld, HILITE code);
LOGICAL		reproject_xy_fields(FIELD ufld, FIELD vfld,
						const MAP_PROJ *smproj, const MAP_PROJ *tmproj);
FIELD		build_field_2D(STRING entity, STRING element, STRING level,
						FIELD ufld, FIELD vfld,
						const MAP_PROJ *smproj, const MAP_PROJ *tmproj);

/* Declare all functions in field_prep.c */
void		prep_field(FIELD field, int dformat);

/* Now it has been included */
#endif
