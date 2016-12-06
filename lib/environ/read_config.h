/**********************************************************************/
/** @file read_config.h
 *
 *  Routines for reading new Version 4.0 configuration files (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r e a d _ c o n f i g . h                                          *
*                                                                      *
*   Routines for reading new Version 4.0 configuration files           *
*    (include file)                                                    *
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
************************************************************************/

/* See if already included */
#ifndef READ_CONFIG_DEFS
#define READ_CONFIG_DEFS


/* We need definitions for low level types */
#include <fpa_types.h>

/* We need definitions for standard I/O */
#include <stdio.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for read_config routines               *
*                                                                      *
************************************************************************/

#ifdef READ_CONFIG_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in read_config.c                         *
*                                                                      *
***********************************************************************/

LOGICAL	first_config_file_open(STRING cfgname, FILE **fpcfg);
LOGICAL	config_file_open(STRING cfgname, FILE **fpcfg);
void	config_file_close(FILE **fpcfg);
STRING	read_config_file_line(FILE **fpcfg);
LOGICAL	skip_config_file_block(FILE **fpcfg);
LOGICAL	skip_to_end_of_block(FILE **fpcfg);
LOGICAL	config_file_location(FILE *fpcfg, STRING *cfgname, long int *position);


/* Now it has been included */
#endif
