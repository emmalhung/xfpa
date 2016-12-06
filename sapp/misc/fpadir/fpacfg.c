/***********************************************************************
*                                                                      *
*     f p a c f g . c                                                  *
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
	char		fname[256], key[80];
	int			nslist;
	STRING		*slist, cfg, name;

	/* Proprietary license call! */ app_license("generic");
	pr_control(NULL, 0, 0);

	if (argc < 3 || blank(argv[1]) || blank(argv[2]))
		{
		name = strrchr(argv[0], '/');
		if (!name) name = argv[0];
		else       name++;

		printf("[%s] Usage: %s <setup> <key>|ALL\n", name, name);
		return 1;
		}

	/* Get the setup file name and config file key */
	strcpy(fname, argv[1]);
	strcpy(key, argv[2]);

	/* Get hold of the setup file */
	nslist = setup_files(fname, &slist);
	if (!define_setup(nslist, slist))
		{
		printf("Problem with setup file: %s\n", fname);
		return 1;
		}

	/* Output the config files */
	if (same_ic(key, "ALL"))
		{
		report_config_files(stdout);
		}
	else
		{
		cfg = config_file_name(key);
		if (blank(cfg))
			{
			printf("Config file unknown: %s\n", key);
			return 2;
			}
		printf("%s\n", cfg);
		}

	return 0;
	}
