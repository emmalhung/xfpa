/**********************************************************************/
/** @file pdf_info.c
 *
 * Functions to access the contents of the PSMet/TexMet PDF files.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   p d f _ i n f o . c                                                *
*                                                                      *
*   Functions to access the contents of PSMet/TexMet PDF files.        *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#include "pdf_info.h"
#include "read_setup.h"

#include <tools/tools.h>
#include <fpa_types.h>
#include <fpa_getmem.h>
#include <fpa_math.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/***********************************************************************
*                                                                      *
*   G e t P d f P r o g s                                              *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Determine the list of prog charts required by the given PDF file.
 *
 *	@param[in]	gen			Generator (psmet or texmet)
 *	@param[in]	sdir		PSMet/TexMet sub-directory
 *	@param[in]	fnam		PDF file name (without the '.pdf')
 *	@param[out]	**out_plist	list of prog times
 *  @return The size of the list.
 **********************************************************************/
int		GetPdfProgs

	(
	STRING	gen,
	STRING	sdir,
	STRING	fnam,
	int		**out_plist
	)

	{
	FILE	*fp;
	STRING	dir;
	char	path[256], line[256];

	/* Internal buffer for prog times */
	int		*plist = NullInt;
	int		np     = 0;

	/* Build the full path for the PDF file and attempt to open it */
	dir = get_directory(gen);
	if (blank(dir))
		{
		pr_error("PDF Access", "Cannot evaluate PDF directory: %s\n", gen);
		if (out_plist) *out_plist = NullInt;
		return 0;
		}
	if (!blank(sdir)) (void) strcpy(path, pathname(dir, sdir));
	(void) strcpy(path, pathname(path, fnam));
	(void) strcat(path, ".pdf");
	/* (void) sprintf(path, "%s/%s/%s.pdf", dir, sdir, fnam); */
	fp = fopen (path, "r");
	if (!fp)
		{
		pr_error("PDF Access", "Cannot open PDF file: %s\n", path);
		if (out_plist) *out_plist = NullInt;
		return 0;
		}

	/* Read each line */
	while ( getfileline(fp, line, sizeof(line)) )
		{
		/* Check for data file requests */
		if ( same_start(line, "@CHANGE_DATA_FILE:") )
			{
			np++;
			plist = GETMEM(plist, int, np);
			(void) sscanf(line, "@CHANGE_DATA_FILE: %d", &plist[np-1]);
			}
		}

	/* All done - clean up */
	(void) fclose(fp);
	if (out_plist) *out_plist = plist;
	return np;
	}
