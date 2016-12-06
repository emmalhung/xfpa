/**********************************************************************/
/** @file depict_fields.h
 *
 *  Routines to interpret the "depiction" setup block (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   d e p i c t _ f i e l d s . h                                      *
*                                                                      *
*   Routines to interpret the "depiction" setup block (include file)   *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
*	  Version 7 (c) Copyright 2006 Environment Canada				   *
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
#ifndef DEPICT_FIELDS_DEFS
#define DEPICT_FIELDS_DEFS


/* We need definitions for configuration file structures */
#include "config_structs.h"


/***********************************************************************
*                                                                      *
*  Initialize defined constants for depict_fields routines             *
*                                                                      *
***********************************************************************/

#ifdef DEPICT_FIELDS_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in depict_fields.c                       *
*                                                                      *
***********************************************************************/

int		depict_field_list(FpaConfigFieldStruct ***fdefs);


/* Now it has been included */
#endif
