/***********************************************************************
*                                                                      *
*   s u p p o r t _ a r e a . c                                        *
*                                                                      *
*   Obsolescent functions to handle the AREA object.                   *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (AES)            *
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

#include "support.h"

#include <string.h>
#include <stdio.h>

/***********************************************************************
*                                                                      *
*      r e d e f i n e _ a r e a _ b o u n d a r y                     *
*      r e d e f i n e _ a r e a _ d i v i d e                         *
*                                                                      *
*      Obsolescent functions!                                          *
*                                                                      *
*      Use equivalent replace_area... functions.  Additional arguments *
*      have been added.                                                *
*                                                                      *
***********************************************************************/

void	redefine_area_boundary

	(
	AREA	area,		/* given area */
	LINE	boundary	/* line to define boundary */
	)

	{
	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    AREA  area;\n");
		(void) printf("    LINE  boundary;\n");
		(void) printf("    redefine_area_boundary(area, boundary);\n");
		(void) printf("With:\n");
		(void) printf("    AREA     area;\n");
		(void) printf("    LINE     boundary;\n");
		(void) printf("    DIVSTAT  dstat;\n");
		(void) printf("    replace_area_boundary(area, boundary, &dstat);\n");
		(void) printf("*** End\n");
		}

	replace_area_boundary(area, boundary, (DIVSTAT *)0);
	}

/**********************************************************************/

void	redefine_area_divide

	(
	AREA	area,	/* given area */
	int		cdiv,	/* which dividing line to change */
	LINE	moddiv	/* new dividing line */
	)

	{
	static	LOGICAL	done = FALSE;
	if (!done)
		{
		done = TRUE;
		(void) printf("*** Obsolete function temoprarily supported:\n");
		(void) printf("Replace:\n");
		(void) printf("    AREA  area;\n");
		(void) printf("    int   cdiv;\n");
		(void) printf("    LINE  moddiv;\n");
		(void) printf("    redefine_area_divide(area, cdiv, moddiv);\n");
		(void) printf("With:\n");
		(void) printf("    AREA     area;\n");
		(void) printf("    int      cdiv;\n");
		(void) printf("    LINE     moddiv;\n");
		(void) printf("    DIVSTAT  dstat;\n");
		(void) printf("    replace_area_divide(area, cdiv, moddiv, &dstat);\n");
		(void) printf("*** End\n");
		}

	replace_area_divide(area, cdiv, moddiv, (DIVSTAT *)0);
	}
