/**********************************************************************/
/** @file message.h
 *
 *  Routines to manage status and diagnostic messages (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    m e s s a g e . h                                                 *
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

#include <fpa_types.h>

/* Obsolescent */
void	pr_mode(int olevel, int elevel, int mstyle);
LOGICAL	pr_module(int olevel);

void	pr_control(STRING module, int olevel, int mstyle);
LOGICAL	pr_level(STRING module, int olevel);
void	pr_diag(STRING module, STRING format, ...);
void	pr_info(STRING module, STRING format, ...);
void	pr_status(STRING module, STRING format, ...);
void	pr_warning(STRING module, STRING format, ...);
void	pr_error(STRING module, STRING format, ...);

void	set_feature_mode(STRING feature, STRING mode);
STRING	get_feature_mode(STRING feature);

#ifdef MACHINE_SUN
#	include <stdio.h>
int		vscanf(const STRING, ...);
int		vfscanf(FILE *, const STRING, ...);
int		vsscanf(STRING, const STRING, ...);
#endif
