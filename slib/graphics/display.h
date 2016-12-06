/****************************************************************************
*                                                                           *
*  File:        display.h                                                   *
*                                                                           *
*     Version 8 (c) Copyright 2011 Environment Canada                       *
*                                                                           *
*   This file is part of the Forecast Production Assistant (FPA).           *
*   The FPA is free software: you can redistribute it and/or modify it      *
*   under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation, either version 3 of the License, or       *
*   any later version.                                                      *
*                                                                           *
*   The FPA is distributed in the hope that it will be useful, but          *
*   WITHOUT ANY WARRANTY; without even the implied warranty of              *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
*   See the GNU General Public License for more details.                    *
*                                                                           *
*   You should have received a copy of the GNU General Public License       *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.        *
*                                                                           *
*****************************************************************************/

#ifndef DISPLAYDEFS
#define DISPLAYDEFS

#include <fpa.h>

/* Functions in display.c */
void	display_dispnode(DISPNODE);
void	capture_dn_raster(DISPNODE);
void	free_dn_raster(DISPNODE);
void	display_dn_parent(DISPNODE);
void	display_dn_subtree(DISPNODE, LOGICAL);
void	display_dn_edge(DISPNODE);
void	display_metafile(METAFILE);
void	display_field(FIELD);
void	display_surface(SURFACE);
void	display_set(SET);
void	display_set_order(SET);
void	display_plot(PLOT);
void	display_special(STRING, POINTER);
void	display_item(STRING, ITEM);
void	display_lchain(LCHAIN);
void	display_spot(SPOT);
void	display_area(AREA);
void	display_subarea(SUBAREA);
void	display_curve(CURVE);
void	display_label(LABEL);
void	display_mark(MARK);
void	display_barb(BARB);
void	display_barb_wind(BARB);
void	display_barb_arrow(BARB);
void	display_barb_value(BARB);
void	display_button(BUTTON);

#endif
