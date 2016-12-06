/***********************************************************************
*                                                                      *
*     a c t i v e . c                                                  *
*                                                                      *
*     Set the active fields and other common behaviour for edit,       *
*     label and sample functions.                                      *
*                                                                      *
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

#include "ingred_private.h"

/***********************************************************************
*                                                                      *
*     e v a l _ l i s t _ g r i d                                      *
*     e v a l _ l i s t _ a d d                                        *
*     e v a l _ l i s t _ r e s e t                                    *
*                                                                      *
***********************************************************************/

LOGICAL	eval_list_grid

	(
	STRING	valx,
	STRING	valy
	)

	{
	int		nx, ny, ix, iy, ip;
	float	dx, dy;

	if (blank(valx)) return FALSE;
	if (blank(valy)) return FALSE;
	if (sscanf(valx, "%d", &nx) < 1) return FALSE;
	if (sscanf(valy, "%d", &ny) < 1) return FALSE;

	EditUseList = TRUE;
	EditNumP    = nx * ny;
	EditPlist   = GETMEM(EditPlist, POINT, EditNumP);

	dx = MapProj->definition.xlen / (nx+1);
	dy = MapProj->definition.ylen / (ny+1);

	ip = 0;
	for (ix=1; ix<=nx; ix++)
		{
		for (iy=1; iy<=ny; iy++)
			{
			EditPlist[ip][X] = ix*dx;
			EditPlist[ip][Y] = iy*dy;
			ip++;
			}
		}

	EditFullSam = False;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	eval_list_add

	(
	STRING	slat,
	STRING	slon
	)

	{
	float	lat, lon;
	POINT	pos;
	LOGICAL	ok;

	if (blank(slat)) return FALSE;
	if (blank(slon)) return FALSE;
	lat = read_lat(slat, &ok);	if (!ok) return FALSE;
	lon = read_lon(slon, &ok);	if (!ok) return FALSE;

	ll_to_pos(MapProj, lat, lon, pos);
	if (!inside_map_def(&MapProj->definition, pos)) return FALSE;

	EditNumP++;
	EditPlist = GETMEM(EditPlist, POINT, EditNumP);
	copy_point(EditPlist[EditNumP-1], pos);
	EditUseList = TRUE;

	EditFullSam = False;
	return TRUE;
	}

/**********************************************************************/

LOGICAL	eval_list_reset(void)

	{
	if (!EditUseList) return TRUE;

	FREEMEM(EditPlist);
	EditNumP    = 0;
	EditPlist   = NULL;
	EditUseList = FALSE;

	EditFullSam = TRUE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     l a b e l _ a p p e a r a n c e                                  *
*     s a m p l e _ a p p e a r a n c e                                *
*                                                                      *
***********************************************************************/

void	label_appearance

	(
	COLOUR	colour,
	LSTYLE	style,
	float	width,
	LOGICAL	dohilo
	)

	{
	EditColour = colour;
	EditStyle  = style;
	EditWidth  = width;
	EditDoHiLo = dohilo;
	}

/**********************************************************************/

void	sample_appearance

	(
	FONT	font,
	float	size,
	COLOUR	colour
	)

	{
	EditFont   = font;
	EditColour = colour;
	EditLsize  = size;
	EditBsize  = size * WbarbSize / LabelSize;
	EditFact   = MapProj->definition.units;
	EditXoff   = MapProj->definition.xlen * zoom_factor() / 100;
	EditYoff   = 0;
	}

/***********************************************************************
*                                                                      *
*     a c t i v e _ s p l i n e _ f i e l d s                          *
*     a c t i v e _ a r e a _ f i e l d s                              *
*     a c t i v e _ l i n e _ f i e l d s                              *
*     a c t i v e _ p o i n t _ f i e l d s                            *
*     a c t i v e _ l c h a i n _ f i e l d s                          *
*     a c t i v e _ s c r a t c h _ f i e l d s                        *
*                                                                      *
***********************************************************************/

void	active_spline_fields

	(
	LOGICAL	undoable,
	SURFACE	sfc,
	SET		labels
	)

	{
	EditUndoable = undoable;
	EditRetain   = Not(undoable);
	EditSfc      = sfc;
	EditLabs     = labels;

	MaxSpread = 0.0;
	if (sfc)
		{
		MaxSpread = (float) (sfc->nupatch + sfc->nvpatch) / 10.0;
		MaxSpread = MIN(MaxSpread, LimSpread);
		}
	}

/**********************************************************************/

void	active_area_fields

	(
	LOGICAL	undoable,
	SET		areas,
	SET		labels
	)

	{
	EditUndoable = undoable;
	EditRetain   = Not(undoable);
	EditAreas    = areas;
	EditLabs     = labels;
	}

/**********************************************************************/

void	active_line_fields

	(
	LOGICAL	undoable,
	SET		curves,
	SET		labels
	)

	{
	EditUndoable = undoable;
	EditRetain   = Not(undoable);
	EditCurves   = curves;
	EditLabs     = labels;
	}

/**********************************************************************/

void	active_point_fields

	(
	LOGICAL	undoable,
	SET		points
	)

	{
	EditUndoable = undoable;
	EditRetain   = Not(undoable);
	EditPoints   = points;
	}

/**********************************************************************/

void	active_lchain_fields

	(
	LOGICAL	undoable,
	SET		lchains
	)

	{
	EditUndoable = undoable;
	EditRetain   = Not(undoable);
	EditLchains  = lchains;
	}

/**********************************************************************/

void	active_scratch_fields

	(
	LOGICAL	undoable,
	SET		curves,
	SET		labels,
	SET		marks
	)

	{
	EditUndoable = undoable;
	EditRetain   = Not(undoable);
	EditCurves   = curves;
	EditLabs     = labels;
	EditMarks    = marks;
	}

/***********************************************************************
*                                                                      *
*     a c t i v e _ f i e l d _ i n f o                                *
*     a c t i v e _ v a l u e _ i n f o                                *
*     a c t i v e _ w i n d _ i n f o                                  *
*                                                                      *
***********************************************************************/

void	active_field_info

	(
	STRING	elem,
	STRING	level,
	STRING	source,
	STRING	subsrc,
	STRING	rtime,
	STRING	vtime
	)

	{
	/* Save the basic field description */
	(void) init_fld_descript(&EditFd);
	(void) set_fld_descript(&EditFd,
					FpaF_MAP_PROJECTION,	MapProj,
					FpaF_SOURCE_NAME,		source,
					FpaF_SUBSOURCE_NAME,	subsrc,
					FpaF_RUN_TIME,			rtime,
					FpaF_VALID_TIME,		vtime,
					FpaF_ELEMENT_NAME,		elem,
					FpaF_LEVEL_NAME,		level,
					FpaF_END_OF_LIST);

	/* Set up default value and wind calculations */
	active_value_info(NULL);
	active_wind_info(FALSE, NULL);
	}

/**********************************************************************/

void	active_value_info

	(
	STRING	vtype
	)

	{
	FpaConfigVectorSamplingStruct		*vsdef;
	FpaConfigContinuousSamplingStruct	*csdef;
	FpaConfigDiscreteSamplingStruct		*dsdef;
	FpaConfigWindSamplingStruct			*wsdef;

	/* Start with a copy of the basic field description */
	copy_fld_descript(&ValFd, &EditFd);

	/* If we have a default value, find the first in the list */
	if (blank(vtype) || same(vtype, "DEFAULT"))
		{
		switch (EditFd.edef->fld_type)
			{
			case FpaC_VECTOR:
				vsdef = EditFd.edef->elem_detail->sampling->type.vector;
				vtype = (vsdef->nsample > 0)?
							vsdef->samples[0]->samp_name:
							NULL;
				break;

			case FpaC_CONTINUOUS:
				csdef = EditFd.edef->elem_detail->sampling->type.continuous;
				vtype = (csdef->nsample > 0)?
							csdef->samples[0]->samp_name:
							NULL;
				break;

			case FpaC_WIND:
				wsdef = EditFd.edef->elem_detail->sampling->type.wind;
				vtype = (wsdef->nsample > 0)?
							wsdef->samples[0]->samp_name:
							NULL;
				break;

			default:
				vtype = NULL;
			}
		}

	/* Set the value calculation */
	(void) set_fld_descript(&ValFd,
				FpaF_VALUE_FUNCTION_NAME,	vtype,
				FpaF_END_OF_LIST);

	}

/**********************************************************************/

void	active_wind_info

	(
	LOGICAL	wxref,
	STRING	wtype
	)

	{
	FpaConfigVectorSamplingStruct		*vsdef;
	FpaConfigContinuousSamplingStruct	*csdef;
	FpaConfigWindSamplingStruct			*wsdef;

	/* Start with a copy of the basic field description */
	copy_fld_descript(&WindFd, &EditFd);

	/* Do we have a cross reference? */
	WindXref = wxref;
	if (WindXref)
		{
		/* Use the specified wind regardless of element and level */
		(void) set_fld_descript(&WindFd,
					FpaF_WIND_FUNCTION_NAME,	NULL,
					FpaF_ELEMENT,				NULL,
					FpaF_LEVEL,					NULL,
					FpaF_END_OF_LIST);

		WindType = STRMEM(WindType, wtype);
		}

	/* Otherwise, must be a regular wind */
	else
		{
		/* If we have a default wind, find the first in the list */
		if (blank(wtype) || same(wtype, "DEFAULT"))
			{
			switch (EditFd.edef->fld_type)
				{
				case FpaC_VECTOR:
					vsdef = EditFd.edef->elem_detail->sampling->type.vector;
					wtype = (vsdef->nwindsamp > 0)?
								vsdef->windsamps[0]->samp_func:
								NULL;
					break;

				case FpaC_CONTINUOUS:
					csdef = EditFd.edef->elem_detail->sampling->type.continuous;
					wtype = (csdef->nwindsamp > 0)?
								csdef->windsamps[0]->samp_func:
								NULL;
					break;

				case FpaC_WIND:
					wsdef = EditFd.edef->elem_detail->sampling->type.wind;
					wtype = wsdef->windsample->samp_func;
					break;

				default:
					wtype = NULL;
				}
			}

		/* Set the wind calculation */
		FREEMEM(WindType);
		(void) set_fld_descript(&WindFd,
					FpaF_WIND_FUNCTION_NAME,	wtype,
					FpaF_END_OF_LIST);
		}
	}
