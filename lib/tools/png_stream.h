/*********************************************************************/
/** @file png_stream.h
 *
 * Contains code to allow us to read and write a png stream from memory 
 * instead of a file.
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
#include <fpa_types.h>
#include <stdio.h>
#include <stdlib.h>

/***********************************************************************
*                                                                      *
*    r e a d _ p n g                                                   *
*    w r i t e _ p n g                                                 *
*                                                                      *
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

#ifdef MACHINE_PCLINUX
int	read_png(UNCHAR *pngbuf, int *width, int *height, int nbits, UNCHAR *cout);
int	write_png(UNCHAR *data, int width, int height, int nbits, UNCHAR *pngbuf);
#endif
