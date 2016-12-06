/**********************************************************************/
/** @file pdf_info.h
 *
 *  Routines to interpret PSMet/TexMet PDF files (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   p d f _ i n f o . h                                                *
*                                                                      *
*   Routines to interpret PSMet/TexMet PDF files (include file)        *
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
#ifndef PDF_INFO_DEFS
#define PDF_INFO_DEFS


/* We need definitions for low level types and other Objects */
#include <fpa_types.h>


/***********************************************************************
*                                                                      *
*  Initialize defined constants                                        *
*                                                                      *
***********************************************************************/

#ifdef PDF_INFO_INIT


#endif


/***********************************************************************
*                                                                      *
*  Declare external functions in pdf_info.c.                           *
*                                                                      *
***********************************************************************/

int		GetPdfProgs(STRING gen, STRING sdir, STRING fnam, int **out_plist);


/* Now it has been included */
#endif
