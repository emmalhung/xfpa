/****************************************************************************
*                                                                           *
*  File:        panel.h                                                     *
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

#ifndef PANELDEFS
#define PANELDEFS

#include <fpa.h>

#define	update_screen	display_dispnode
#define	update_panel	display_dispnode
#define	update_text		display_dispnode
#define	update_map		display_dispnode

/* Functions in panel_map.c */
LOGICAL	input_map(DISPNODE, STRING, MAP_PROJ *, LOGICAL);

/* Functions in panel_text.c */
void	print_text(DISPNODE, COLOUR, STRING);
void	blank_text(DISPNODE);
void	define_text(DISPNODE, COLOUR, STRING);
STRING	build_text(DISPNODE, COLOUR, STRING, STRING, UNCHAR);
STRING	yesno_text(DISPNODE, COLOUR, STRING, STRING, STRING, LOGICAL, UNCHAR);

#endif
