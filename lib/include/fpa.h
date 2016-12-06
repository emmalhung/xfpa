/***********************************************************************
*                                                                      *
*   Master header file for FPA User Library                            *
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

#ifndef FPALIB_INCLUDED

	/* Common definitions and macros */
#	include <fpa_types.h>
#	include <fpa_macros.h>
#	include <fpa_getmem.h>
#	include <fpa_string.h>
#	include <fpa_math.h>

	/* Prototypes and structures for library modules */
#	include <tools/tools.h>
#	include <objects/objects.h>
#	include <environ/environ.h>
#	include <extract/extract.h>
#	include <supportlib/support.h>
#	include <glib/glib.h>

#	define FPALIB_INCLUDED
#endif
