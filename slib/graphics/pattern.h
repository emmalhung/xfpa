/****************************************************************************
*                                                                           *
*  File:        pattern.h                                                   *
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

#ifndef PATTERNDEFS
#define PATTERNDEFS

#include <fpa.h>

/* Functions in pattern_*.c */
LOGICAL	centre_pattern(POINT *, int, STRING, HAND, float, float, HILITE);
LOGICAL	draw_pattern(POINT *, int, STRING, HAND, float, float, HILITE);
LOGICAL	centre_symbol(POINT, float, STRING, float, float, HILITE);
LOGICAL	draw_symbol(POINT, float, STRING, float, float, HILITE);
LOGICAL	get_pattern_info(STRING, LOGICAL *);

#endif