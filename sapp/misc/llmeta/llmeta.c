/***********************************************************************
*                                                                      *
*     l l m e t a . c                                                  *
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
	char		cwd[256], fname[256], yesno[10];
	int			nslist;
	STRING		*slist;
	METAFILE	meta;

	/* Get the setup file name */
	/* Proprietary license call! */ app_license("generic");
	if (argc >= 2 && !blank(argv[1])) strcpy(fname, argv[1]);
	else
		{
		printf("Setup File: ");
		getword(stdin, fname, 256);
		if (blank(fname))
			{
			printf("No setup filename given\n");
			exit(1);
			}
		}

	/* Get hold of the setup file */
	getcwd(cwd, 256);
	nslist = setup_files(fname, &slist);
	if (!define_setup(nslist, slist))
		{
		printf("Problem with setup file: %s\n", fname);
		exit(1);
		}
	chdir(cwd);

	/* Get the input metafile name */
	if (argc >= 3 && !blank(argv[2])) strcpy(fname, argv[2]);
	else
		{
		printf("Input Metafile: ");
		getword(stdin, fname, 256);
		if (blank(fname))
			{
			printf("No filename given\n");
			exit(1);
			}
		}

	/* Read the input metafile */
	meta = read_metafile(fname, NullMapProj);
	if (!meta)
		{
		printf("Cannot read metafile: %s\n", fname);
		exit(1);
		}

	/* Get the input metafile name */
	if (argc >= 4 && !blank(argv[3])) strcpy(fname, argv[3]);
	else                              strcpy(fname, "");

	/* If it exists get permission */
	while (TRUE)
		{
		if (blank(fname))
			{
			printf("Output Metafile: ");
			getword(stdin, fname, 256);
			if (blank(fname))
				{
				printf("No filename given\n");
				exit(1);
				}
			}

		if (find_file(fname))
			{
			printf("File %s exists!\nOverwrite? (y/n): ", fname);
			getword(stdin, yesno, 10);
			if (same_ic(yesno, "yes")) break;
			if (same_ic(yesno, "y"))   break;
			if (same_ic(yesno, "oui")) break;
			if (same_ic(yesno, "o"))   break;
			strcpy(fname, "");
			}
		else break;
		}

	/* Write the output metafile */
	write_metafile_special(fname, meta, 4, META_LATLON);
	if (!meta)
		{
		printf("Cannot write metafile: %s\n", fname);
		exit(1);
		}

	printf("Done\n");
	return 0;
	}
