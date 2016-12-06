/*********************************************************************/
/**	@file field.c
 *
 * Routines to handle the FIELD object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      f i e l d . c                                                   *
*                                                                      *
*      Routines to handle the FIELD object.                            *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#define FIELD_INIT
#include "field.h"

#include <tools/tools.h>
#include <fpa_getmem.h>

#include <string.h>

int		FieldCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ f i e l d                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new field with given attributes.
 *
 *	@param[in] 	entity		?
 *	@param[in] 	element		Element name
 *	@param[in] 	level		Level name
 *  @return Pointer to new field object. You will need to destroy
 * 			this object when you are finished with it.
 *********************************************************************/
FIELD	create_field

	(
	STRING	entity,
	STRING	element,
	STRING	level
	)

	{
	FIELD	fld;

	/* Allocate memory for structure */
	fld = INITMEM(struct FIELD_struct,1);
	if (!fld) return NullFld;

	/* Set given attributes */
	fld->entity  = NULL;
	fld->element = NULL;
	fld->level   = NULL;
	define_fld_info(fld,entity,element,level);

	/* Initialize data */
	fld->ftype      = FtypeNone;
	fld->data.plot  = NullPlot;
	fld->data.set   = NullSet;
	fld->data.sfc   = NullSfc;
	fld->data.raster  = NullRaster;
	define_fld_data(fld,"none",(POINTER) 0);

	/* Return the new field */
	FieldCount++;
	return fld;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ f i e l d                                             *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make a copy of a field.
 *
 *	@param[in] 	fld	Field to copy
 *  @return Pointer to copy of given field object. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/

FIELD	copy_field

	(
	const FIELD	fld
	)

	{
	FIELD		fldnew;
	STRING		entity, element, level, type;
	POINTER		data, datanew;

	/* Do nothing if not there */
	if (!fld) return NullFld;

	/* Create a new field using the field attributes */
	(void) recall_fld_info(fld, &entity, &element, &level);
	fldnew = create_field(entity, element, level);

	/* Copy the given field based on the type */
	(void) recall_fld_data(fld, &type, &data);
	switch (fld->ftype)
		{
		case FtypeSfc:	 datanew = (POINTER) copy_surface((SURFACE) data, FALSE);
						 break;
		case FtypeSet:	 datanew = (POINTER) copy_set((SET) data);
						 break;
		case FtypePlot:	 datanew = (POINTER) copy_plot((PLOT) data);
						 break;
		case FtypeRaster: datanew = (POINTER) copy_raster((RASTER) data);
						 break;
		default:		 datanew = NullPointer;
		}
	(void) define_fld_data(fldnew, type, datanew);

	/* Return the copy */
	return fldnew;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ f i e l d                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy the given field.
 *
 *	@param[in] 	fld	the given field
 *  @return NullFld
 *********************************************************************/

FIELD	destroy_field

	(
	FIELD	fld
	)

	{
	/* Do nothing if fld is NULL */
	if (!fld) return NullFld;

	/* Free the attributes */
	FREEMEM(fld->entity);
	FREEMEM(fld->element);
	FREEMEM(fld->level);

	/* Free the data storage */
	delete_fld_data(fld);

	/* Now free the structure itself */
	FREEMEM(fld);
	FieldCount--;
	return NullFld;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ f l d _ i n f o                                   *
*      r e c a l l _ f l d _ i n f o                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Set attributes of given field.
 *
 *	@param[in] 	fld		given field
 *	@param[in] 	entity	?
 *	@param[in] 	element	Element name
 *	@param[in] 	level	Level name
 *********************************************************************/

void	define_fld_info

	(
	FIELD	fld,
	STRING	entity,
	STRING	element,
	STRING	level
	)

	{
	/* Do nothing if no field given */
	if (!fld) return;

	/* Allocate memory and copy strings */
	fld->entity  = STRMEM(fld->entity, entity);
	fld->element = STRMEM(fld->element,element);
	fld->level   = STRMEM(fld->level,  level);
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve attributes of given field.
 *
 *	@param[in] 	fld		 given field
 *	@param[out]	*entity	 ?
 *	@param[out]	*element Element name
 *	@param[out]	*level	 Level name
 *********************************************************************/
void	recall_fld_info

	(
	FIELD	fld,
	STRING	*entity,
	STRING	*element,
	STRING	*level
	)

	{
	/* Do nothing if no field given */
	*entity  = NULL;
	*element = NULL;
	*level   = NULL;
	if (!fld) return;

	/* Return info */
	*entity  = fld->entity;
	*element = fld->element;
	*level   = fld->level;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ f l d _ d a t a                                   *
*      r e c a l l _ f l d _ d a t a                                   *
*      d e l e t e _ f l d _ d a t a                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Define the given field as the specified type, and provide
 * the corresponding data.
 *
 *	@param[in] 	fld		the given field
 *	@param[in] 	type	specified data type
 *	@param[in] 	data	the data
 *********************************************************************/

void	define_fld_data

	(
	FIELD	fld,
	STRING	type,
	POINTER	data
	)

	{
	/* Do nothing if fld is NULL */
	if (!fld) return;

	/* Delete original data */
	delete_fld_data(fld);
	if (!data) return;

	/* Set new data type and add the data */
	if (same(type, "surface"))
		{
		fld->ftype     = FtypeSfc;
		fld->data.sfc  = (SURFACE) data;
		}

	else if (same(type, "set"))
		{
		fld->ftype     = FtypeSet;
		fld->data.set  = (SET) data;
		}

	else if (same(type, "plot"))
		{
		fld->ftype      = FtypePlot;
		fld->data.plot  = (PLOT) data;
		}

	else if (same(type, "raster"))
		{
		fld->ftype       = FtypeRaster;
		fld->data.raster = (RASTER) data;
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the type and data information for the given field.
 *
 *	@param[in] 	fld	    the given field
 *	@param[out]	*type	specified data type
 *	@param[out]	*data	the data
 *********************************************************************/
void	recall_fld_data

	(
	FIELD	fld,
	STRING	*type,
	POINTER	*data
	)

	{
	/* Do nothing if fld is NULL */
	*type = "";
	*data = (POINTER) 0;

	/* Return data */
	if (!fld) return;

	switch (fld->ftype)
		{
		case FtypeSfc:	 *type = "surface";
						 *data = (POINTER) fld->data.sfc;
						 break;

		case FtypeSet:	 *type = "set";
						 *data = (POINTER) fld->data.set;
						 break;

		case FtypePlot:	 *type = "plot";
						 *data = (POINTER) fld->data.plot;
						 break;

		case FtypeRaster: *type = "raster";
						 *data = (POINTER) fld->data.raster;
						 break;

		default:		*type = "none";
						*data = (POINTER) 0;
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Delete the contents of a field object.
 *
 *	@param[in] 	fld	the given field
 *********************************************************************/
void	delete_fld_data

	(
	FIELD	fld
	)

	{
	if (!fld) return;

	/* Free the actual data */
	switch (fld->ftype)
		{
		case FtypeSfc:	 fld->data.sfc  = destroy_surface(fld->data.sfc);
						 break;

		case FtypeSet:	 fld->data.set  = destroy_set(fld->data.set);
						 break;

		case FtypePlot:	 fld->data.plot = destroy_plot(fld->data.plot);
						 break;
		case FtypeRaster: fld->data.raster = destroy_raster(fld->data.raster);
						 break;
		}

	/* Reset type to no data */
	fld->ftype      = FtypeNone;
	}

/***********************************************************************
*                                                                      *
*      c h a n g e _ f l d _ p s p e c                                 *
*      r e c a l l _ f l d _ p s p e c                                 *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Override the presentation specs of the given field.
 *
 *	@param[in] 	fld		given field
 *	@param[in] 	param	parameter to override
 *	@param[in] 	value	new value
 *********************************************************************/

void	change_fld_pspec

	(
	FIELD	fld,
	PPARAM	param,
	POINTER	value
	)

	{
	if (!fld) return;

	/* Change the presentation */
	switch (fld->ftype)
		{
		case FtypeSfc:	change_surface_pspec(fld->data.sfc,param,value);
						break;

		case FtypeSet:	change_set_pspec(fld->data.set,param,value);
						break;

		/*
		case FtypePlot:	 change_plot_pspec(fld->data.plot,param,value);
						 break;
		case FtypeRaster: change_raster_pspec(fld->data.raster,param,value);
						 break;
		*/
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Retrieve the presentation specs of the given field.
 *
 *	@param[in] 	fld			given field
 *	@param[in] 	param		parameter to retrieve
 *	@param[out]	value		value of parameter
 *********************************************************************/
void	recall_fld_pspec

	(
	FIELD	fld,
	PPARAM	param,
	POINTER	value
	)

	{
	if (!fld) return;

	/* Retrieve the presentation */
	switch (fld->ftype)
		{
		case FtypeSfc:	recall_surface_pspec(fld->data.sfc,param,value);
						break;

		case FtypeSet:	recall_set_pspec(fld->data.set,param,value);
						break;

		/*
		case FtypePlot:	 recall_plot_pspec(fld->data.plot,param,value);
						 break;
		case FtypeRaster: recall_raster_pspec(fld->data.raster,param,value);
						 break;
		*/
		}
	}

/***********************************************************************
*                                                                      *
*      h i g h l i g h t _ f i e l d                                   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Change the highlight level of the given field.
 *
 *	@param[in] 	fld		the given field
 *	@param[in] 	code	hilite code
 *********************************************************************/
void	highlight_field

	(
	FIELD	fld,
	HILITE	code
	)

	{
	if (!fld) return;

	/* Highlight appropriate object */
	switch (fld->ftype)
		{
		case FtypeSfc:	highlight_surface(fld->data.sfc,code);
						break;

		case FtypeSet:	highlight_set(fld->data.set,code);
						break;

		case FtypePlot:	highlight_plot(fld->data.plot,code);
						break;
		/*
		case FtypeRaster: highlight_raster(fld->data.raster,code);
						break;
		*/
		}
	}

/***********************************************************************
*                                                                      *
*      r e p r o j e c t _ x y _ f i e l d s                           *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reproject two fields which are understood to contain the x and
 * y component fields of the same vector field.
 *
 * @note Will become obsolete with Vector Fields.
 * See reproject_xy_surfaces() in surface_oper.c for more info.
 *
 *	@param[in] 	ufld		field containing u (x) component
 *	@param[in] 	vfld		field containing v (y) component
 *	@param[in] 	*smproj		source proj
 *	@param[in] 	*tmproj		target proj
 * @return True if successful.
 *********************************************************************/

LOGICAL	reproject_xy_fields

	(
	FIELD			ufld,
	FIELD			vfld,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	SURFACE	usfc = NullSfc;
	SURFACE	vsfc = NullSfc;

	if (IsNull(ufld)) return FALSE;
	if (IsNull(vfld)) return FALSE;
	if (ufld->ftype != FtypeSfc) return FALSE;
	if (vfld->ftype != FtypeSfc) return FALSE;
	if (IsNull(smproj)) return FALSE;
	if (IsNull(tmproj)) return FALSE;

	usfc = ufld->data.sfc;
	vsfc = vfld->data.sfc;
	return reproject_xy_surfaces(usfc, vsfc, smproj, tmproj);
	}

/***********************************************************************
*                                                                      *
*      b u i l d _ f i e l d _ 2 D                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reproject the given pair of fields, which are understood to
 * contain the x and y components (as 1D surfaces) of the same
 * vector field.  Then combine them into a single 2D vector
 * field.
 *
 *	@param[in] 	entity		?
 *	@param[in] 	element		Element name
 *	@param[in] 	level		Level name
 *	@param[in] 	ufld		field containing u (x) component
 *	@param[in] 	vfld		field containing v (y) component
 *	@param[in] 	*smproj		source proj
 *	@param[in] 	*tmproj		target proj
 *  @return Pointer to new Field. You will need to destroy this
 * 			object when you are finished with it.
 *********************************************************************/

FIELD	build_field_2D

	(
	STRING			entity,
	STRING			element,
	STRING			level,
	FIELD			ufld,
	FIELD			vfld,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	SURFACE	usfc = NullSfc;
	SURFACE	vsfc = NullSfc;
	SURFACE	uvsfc;
	FIELD	uvfld;

	if (IsNull(ufld)) return FALSE;
	if (IsNull(vfld)) return FALSE;
	if (ufld->ftype != FtypeSfc) return FALSE;
	if (vfld->ftype != FtypeSfc) return FALSE;
	if (IsNull(smproj)) return FALSE;
	if (IsNull(tmproj)) return FALSE;

	usfc  = ufld->data.sfc;
	vsfc  = vfld->data.sfc;
	uvsfc = build_surface_2D(usfc, vsfc, smproj, tmproj);
	if (IsNull(uvsfc)) return NullFld;

	uvfld = create_field(entity, element, level);
	(void) define_fld_data(uvfld, "surface", (POINTER)uvsfc);
	return uvfld;
	}
