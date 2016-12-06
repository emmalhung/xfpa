/***********************************************************************
*                                                                      *
*     m e t a d i f f . c                                              *
*                                                                      *
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

#include <fpa.h>
#include <stdio.h>

int		main
	(
	int		argc,
	STRING	argv[]
	)

	{
	int			il, ival1, ival2, idiff, maxdiff, ilm;
	char		buf1[1024], buf2[1024];
	char		xbuf1[1024], xbuf2[1024], dbuf1[1024], dbuf2[1024];
	LOGICAL		valok;
	FILE		*meta1, *meta2;

	/* Validate run string parameters */
	if ( argc != 3 )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "   metadiff <metafile_1> <metafile_2>\n");
		return (-1);
		}

	/* Obtain a generic licence */
	(void) app_license("generic");

	/* Open the two metafiles */
	if ( IsNull( meta1 = fopen(argv[1], "r") ) )
		{
		(void) fprintf(stderr, " Cannot open metafile ... \"%s\"\n", argv[1]);
		return (-1);
		}
	if ( IsNull( meta2 = fopen(argv[2], "r") ) )
		{
		(void) fprintf(stderr, " Cannot open metafile ... \"%s\"\n", argv[2]);
		return (-1);
		}

	/* Compare the two metafiles line by line */
	(void) fprintf(stdout, " Comparing metafile ... \"%s\"\n",   argv[1]);
	(void) fprintf(stdout, "      with metafile ... \"%s\"\n\n", argv[2]);
	il      = 0;
	maxdiff = 0;
	while (TRUE)
		{

		/* Check for end of file */
		if ( IsNull(getfileline(meta1, buf1, sizeof(buf1))) ) break;
		(void) strcpy(xbuf1, buf1);
		(void) no_white(buf1);
		if ( IsNull(getfileline(meta2, buf2, sizeof(buf2))) ) break;
		(void) strcpy(xbuf2, buf2);
		(void) no_white(buf2);
		il++;

		/* Compare two lines of values ... saving largest difference */
		if ( isdigit(buf1[0]) || buf1[0] == '-' )
			{
			while (TRUE)
				{
				/* Compare values ... and save largest difference */
				ival1 = int_arg(buf1, &valok); if (!valok) break;
				ival2 = int_arg(buf2, &valok); if (!valok) break;

				idiff = ival2 - ival1;
				if ( abs(idiff) > abs(maxdiff) )
					{
					ilm     = il;
					maxdiff = idiff;
					(void) strcpy(dbuf1, xbuf1);
					(void) strcpy(dbuf2, xbuf2);
					}
				}
			}
		else
			{
			if ( !same(buf1, buf2) )
				{
				(void) fprintf(stdout,
					"   Difference at line %d ...\n", il);
				(void) fprintf(stdout, "     \"%s\"\n", xbuf1);
				(void) fprintf(stdout, "     \"%s\"\n", xbuf2);
				}
			}
		}

	/* Display largest difference in values */
	if ( abs(maxdiff) > 0 )
		{
		(void) fprintf(stdout,
			"   Largest difference in values (%d) at line %d ...\n",
			maxdiff, ilm);
		(void) fprintf(stdout, "     \"%s\"\n", dbuf1);
		(void) fprintf(stdout, "     \"%s\"\n", dbuf2);
		}

	(void) fprintf(stdout, " Finished comparing ...\n");
	return 0;
	}
