/**********************************************************************/
/** @file read_setup.h
 *
 *  Routines to handle setup and configuration files (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r e a d _ s e t u p . h                                            *
*                                                                      *
*   Routines to handle setup and configuration files (include file)    *
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
#ifndef READ_SETUP_DEFS


/* We need definitions for low level types */
#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>
#include <stdio.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants for read_setup routines                *
*                                                                      *
***********************************************************************/

#ifdef READ_SETUP_INIT

#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in read_setup.c                          *
*                                                                      *
***********************************************************************/

LOGICAL	fpa_connect(STRING setup, ACCESS mode);
int		setup_files(STRING lfile, STRING **sfiles);
STRING	get_setup(STRING sfile);
STRING	base_directory(void);
STRING	home_directory(void);
STRING	work_directory(void);
int		define_setup(int nsetup, STRING *sfiles);
int		current_setup_list(STRING **sfiles);
int		find_setup_block(STRING block_key, LOGICAL required);
STRING	setup_block_line(void);
STRING	app_service(STRING name);
int		open_config_file(STRING key);
void	close_config_file(void);
STRING	config_file_line(void);
STRING	config_file_name(STRING key);
STRING	get_directory(STRING key);
STRING	get_path(STRING key, STRING fname);
STRING	get_file(STRING key, STRING fname);
void	report_directories(FILE *fp);
void	report_config_files(FILE *fp);

/* User defined library confirmation */
void	userlib_verify(void);

/* Now it has been included */
#define READ_SETUP_DEFS
#endif
