/**********************************************************************/
/** @file license.h
 *
 *  Routines to check and get license permissions (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    l i c e n s e . h                                                 *
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

#ifndef LICENSE_DEFS
#define LICENSE_DEFS

#include <fpa_types.h>

/* Special client IDs (It won't do you any good to know these!) */
#define ClientCommon (0)
#define ClientAES    (1)
#define ClientMaster (127)

/* Access Modes */
typedef	enum { FpaAccessNone, FpaAccessRead, FpaAccessLib, FpaAccessFull }
	ACCESS;

LOGICAL	app_license(STRING appname);
LOGICAL	fpalib_license(ACCESS mode);
LOGICAL	pro_license(int client, LOGICAL);
LOGICAL	fpa_license(STRING appname, int *ndays, int *code);
LOGICAL	obtain_license(STRING appname, int client, int *ndays, int *code);

LOGICAL	fpalib_verify(ACCESS mode);

STRING	temp_codeword(STRING oldcode, STRING newaddr, int days);

LOGICAL	get_os_license(STRING appname);
LOGICAL	get_license(STRING appname, int client, int *expiry);
LOGICAL	check_license(void);
LOGICAL	release_license(void);

/* Obsolete */
LOGICAL	gen_license(void);
LOGICAL	lib_license(void);
LOGICAL	lib_verify(void);
LOGICAL	app_verify(void);

#endif
