/*******************************************************************************
*                                                                              *
*   g e t g r i b . c                                                          *
*                                                                              *
*   Developed by R K R Trafford (ARMF)                                         *
*                                                                              *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)                    *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)                    *
*     Version 7 (c) Copyright 2006 Environment Canada                          *
*     Version 8 (c) Copyright 2011 Environment Canada                          *
*                                                                              *
*   This file is part of the Forecast Production Assistant (FPA).              *
*   The FPA is free software: you can redistribute it and/or modify it         *
*   under the terms of the GNU General Public License as published by          *
*   the Free Software Foundation, either version 3 of the License, or          *
*   any later version.                                                         *
*                                                                              *
*   The FPA is distributed in the hope that it will be useful, but             *
*   WITHOUT ANY WARRANTY; without even the implied warranty of                 *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
*   See the GNU General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.           *
*                                                                              *
*******************************************************************************/

#include <fpa.h>

#include <string.h>
#include <stdio.h>

#undef DEBUG

static	STRING	GribHost = "cidsv07";
static	STRING	GribDir  = "/data/ops/gribwan";

/*******************************************************************************
*                                                                              *
*   m a i n                                                                    *
*                                                                              *
*******************************************************************************/

void	main

	(
	int		argc,
	STRING	argv[]
	)

	{
	FILE	*pipefile;
	STRING	region, model, c, cm, cv;
	int		ivalid, nvalid, lvalid, rvalid, n;
	int		run, valid[120], avail[120], need[120], left[120];
	char	loctime[15], rtime[120][15], fname[120][25];
	int		getall, getany;
	char	dest[256], base[64], cmd[512], mname[20];
	int		cyear, cjday, chour, cmin, csec, cmonth, cmday;
	int		ryear, rjday, rhour, rmin, rsec, rmonth, rmday;
	int		lyear, ljday, lhour, lmin, lsec, lmonth, lmday;
	STRING	val;

	static	STRING	tfmt = "%.4d:%.3d:%.2d:%.2d";
	static	STRING	month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
								  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	if (argc < 4)
		{
		(void) printf("Usage: getgrib <region> <model> <run> [<valid> . . .]\n");
		(void) exit(1);
		}

	/* Get current date-time and go to destination directory */
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) sprintf(dest, "%s/data/remote", base_directory());
	val = getenv("FPA_LOCAL_GRIB");
	if (!blank(val)) (void) strcpy(dest, val);
	(void) chdir(dest);

	/* Interpret run string parameters */
	region = argv[1];
	model  = argv[2];
	run    = atoi(argv[3]);
	nvalid = argc - 4;
	getall = (int) (nvalid <= 0);
	(void) sprintf(base, "%s%s%.2d", region, model, run);
	for (ivalid=0; ivalid<nvalid; ivalid++)
		{
		valid[ivalid] = atoi(argv[ivalid+4]);
		avail[ivalid] = FALSE;
		need[ivalid]  = FALSE;
		left[ivalid]  = FALSE;
		(void) sprintf(rtime[ivalid], "-");
		(void) sprintf(fname[ivalid], "%s_%.3d", base, valid[ivalid]);
		}

	/* Set remote host and directory from environment if set */
	val = getenv("FPA_REMOTE_GRIB");
	if (!blank(val))
		{
		c = strchr(val,':');
		if (c)
			{
			GribHost = strdup(val);
			GribHost[c-val] = '\0';
			val = c+1;
			}
		if (!blank(val)) GribDir = strdup(val);
		}

	(void) printf("\nManual GRIB Download\n");
	(void) printf("\nSource: %s:%s\nDestination: %s\n", GribHost, GribDir, dest);
	(void) printf("\nChecking files...");
	(void) fflush(stdout);
	/* Let's just see what's over there */
	(void) sprintf(cmd, "echo \"cd %s\ndir %s*\nbye\" | ftp %s",
			GribDir, base, GribHost);
	pipefile = popen(cmd, "r");
	if (!pipefile)
		{
		(void) perror("pipe");
		(void) exit(1);
		}
#	ifdef DEBUG
	(void) printf("\nRemote:\n");
#	endif /* DEBUG */
	while (getfileline(pipefile, cmd, 512))
		{
		c = strstr(cmd, base);
		if (!c) continue;
#		ifdef DEBUG
		(void) printf("\t%s\n", cmd);
#		endif /* DEBUG */

		cv = c + strlen(base);
		if (getall && blank(cv)) rvalid = -1;
		else if (blank(cv))      continue;
		else if (cv[0] == '_')   rvalid = atoi(cv+1);
		else                     continue;
		if (getall)
			{
			ivalid = nvalid++;
			valid[ivalid] = rvalid;
			if (rvalid < 0) (void) sprintf(fname[ivalid], "%s", base);
			else            (void) sprintf(fname[ivalid], "%s_%.3d",
																base, rvalid);
			}
		else
			{
			for (ivalid=0; ivalid<nvalid; ivalid++)
				{
				if (rvalid == valid[ivalid]) break;
				}
			if (ivalid >= nvalid) continue;
			}

		avail[ivalid] = TRUE;
		need[ivalid]  = TRUE;
		left[ivalid]  = TRUE;
		cm   = c - 13;
		n    = sscanf(cm, "%s %d %d:%d", mname, &rmday, &rhour, &rmin);
		rsec = 0;
		if (n != 4)
			{
			n = sscanf(cm, "%s %d %d", mname, &rmday, &ryear);
			rhour = 0;
			rmin  = 0;
			}
		for (rmonth=1; rmonth<=12; rmonth++)
			{
			if (same(mname, month[rmonth-1])) break;
			}
		if (n == 4)
			{
			ryear = (rmonth <= cmonth)? cyear: cyear-1;
			}
		(void) jdate(&ryear, &rmonth, &rmday, &rjday);
		(void) sprintf(rtime[ivalid], tfmt, ryear, rjday, rhour, rmin);
		}
	(void) pclose(pipefile);

	/* Now let's see what's already here */
	(void) sprintf(cmd, "TZ=GMT; export TZ; ll %s* 2>/dev/null", base);
	pipefile = popen(cmd, "r");
	if (!pipefile)
		{
		(void) perror("pipe");
		(void) exit(1);
		}
#	ifdef DEBUG
	(void) printf("\nLocal:\n");
#	endif /* DEBUG */
	while (getfileline(pipefile, cmd, 512))
		{
		c = strstr(cmd, base);
		if (!c) continue;
#		ifdef DEBUG
		(void) printf("\t%s\n", cmd);
#		endif /* DEBUG */

		cv = c + strlen(base);
		if (getall && blank(cv)) lvalid = -1;
		else if (blank(cv))      continue;
		else if (cv[0] == '_')   lvalid = atoi(cv+1);
		else                     continue;
		for (ivalid=0; ivalid<nvalid; ivalid++)
			{
			if (lvalid == valid[ivalid]) break;
			}
		if (ivalid >= nvalid) continue;

		if (avail[ivalid])
			{
			cm   = c - 13;
			n    = sscanf(cm, "%s %d %d:%d", mname, &lmday, &lhour, &lmin);
			lsec = 0;
			if (n != 4)
				{
				n = sscanf(cm, "%s %d %d", mname, &lmday, &lyear);
				lhour = 0;
				lmin  = 0;
				}
			for (lmonth=1; lmonth<=12; lmonth++)
				{
				if (same(mname, month[lmonth-1])) break;
				}
			if (n == 4)
				{
				lyear = (lmonth <= cmonth)? cyear: cyear-1;
				}
			(void) jdate(&lyear, &lmonth, &lmday, &ljday);
			(void) sprintf(loctime, tfmt, lyear, ljday, lhour, lmin);

			need[ivalid] = (int) (strcmp(rtime[ivalid], loctime) > 0);
			}
		}
	(void) pclose(pipefile);

	/* Finally, let's try to retrieve the files we need */
	(void) printf(" File status:\n");
	(void) sprintf(cmd, "echo \"cd %s\nbin\nprompt\nmget", GribDir);
	getany = FALSE;
	for (ivalid=0; ivalid<nvalid; ivalid++)
		{
		(void) printf("\t%s", fname[ivalid]);
		if (!avail[ivalid])     (void) printf(" not available\n");
		else if (!need[ivalid]) (void) printf(" already up to date\n");
		else                    (void) printf(" transfer\n");

		if (!need[ivalid]) continue;
		getany = TRUE;

		c = cmd + strlen(cmd);
		(void) sprintf(c, " %s", fname[ivalid]);
		}
	if (!getany)
		{
		(void) printf("\nNo files to transfer\n");
		(void) exit(0);
		}
	(void) printf("\nTransfering files...");
	(void) fflush(stdout);
	c = cmd + strlen(cmd);
	(void) sprintf(c, "\nbye\" | ftp %s", GribHost);
	pipefile = popen(cmd, "r");
	if (!pipefile)
		{
		(void) perror("pipe");
		(void) exit(1);
		}
#	ifdef DEBUG
	(void) printf("\nExecuting:\n%s\n", cmd);
#	endif /* DEBUG */
	while (getfileline(pipefile, cmd, 512))
		{
#		ifdef DEBUG
		(void) printf("\t%s\n", cmd);
#		endif /* DEBUG */
		}
	(void) pclose(pipefile);

	/* Now let's see what's been received */
	(void) sprintf(cmd, "TZ=GMT; export TZ; ll %s* 2>/dev/null", base);
	pipefile = popen(cmd, "r");
	if (!pipefile)
		{
		(void) perror("pipe");
		(void) exit(1);
		}
#	ifdef DEBUG
	(void) printf("\nLocal:\n");
#	endif /* DEBUG */
	while (getfileline(pipefile, cmd, 512))
		{
		c = strstr(cmd, base);
		if (!c) continue;
#		ifdef DEBUG
		(void) printf("\t%s\n", cmd);
#		endif /* DEBUG */

		cv = c + strlen(base);
		if (getall && blank(cv)) lvalid = -1;
		else if (blank(cv))      continue;
		else if (cv[0] == '_')   lvalid = atoi(cv+1);
		else                     continue;
		for (ivalid=0; ivalid<nvalid; ivalid++)
			{
			if (lvalid == valid[ivalid]) break;
			}
		if (ivalid >= nvalid) continue;

		if (avail[ivalid])
			{
			cm   = c - 13;
			n    = sscanf(cm, "%s %d %d:%d", mname, &lmday, &lhour, &lmin);
			lsec = 0;
			if (n != 4)
				{
				n = sscanf(cm, "%s %d %d", mname, &lmday, &lyear);
				lhour = 0;
				lmin  = 0;
				}
			for (lmonth=1; lmonth<=12; lmonth++)
				{
				if (same(mname, month[lmonth-1])) break;
				}
			if (n == 4)
				{
				lyear = (lmonth <= cmonth)? cyear: cyear-1;
				}
			(void) jdate(&lyear, &lmonth, &lmday, &ljday);
			(void) sprintf(loctime, tfmt, lyear, ljday, lhour, lmin);

			left[ivalid] = (int) (strcmp(rtime[ivalid], loctime) > 0);
			}
		}
	(void) pclose(pipefile);
	(void) printf(" Transfer status:\n");
	for (ivalid=0; ivalid<nvalid; ivalid++)
		{
		if (!need[ivalid]) continue;

		(void) printf("\t%s", fname[ivalid]);
		if (left[ivalid]) (void) printf(" not received\n");
		else              (void) printf(" received\n");
		}

	(void) printf("\nDone\n");
	return;
	}
