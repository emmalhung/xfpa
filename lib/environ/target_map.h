/**********************************************************************/
/** @file target_map.h
 *
 *  Routines to interpret the "target_map" setup block (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   t a r g e t _ m a p . h                                            *
*                                                                      *
*   Routines to interpret the "target_map" setup block (include file)  *
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
#ifndef TARGET_MAP_DEFS
#define TARGET_MAP_DEFS


/* We need definitions for low level types and other Objects */
#include <fpa_types.h>
#include <objects/objects.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for target_map routines                *
*                                                                      *
***********************************************************************/

#ifdef TARGET_MAP_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in target_map.c                          *
*                                                                      *
***********************************************************************/

MAP_PROJ	*get_target_map(void);


/* Now it has been included */
#endif
