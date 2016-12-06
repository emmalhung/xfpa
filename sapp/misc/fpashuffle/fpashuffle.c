/***********************************************************************
*                                                                      *
*     f p a s h u f f l e .                                            *
*                                                                      *
*   Usage:  fpashuffle              <setup_file> <source> <rtime>      *
*                                                                      *
*     where  <setup_file>         is the local setup file name         *
*            <source>             is the source directory to shuffle   *
*            <rtime>              is the run time of the source        *
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

/* FPA library definitions */
#include <fpa.h>

/* Standard library definitions */
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

static	LOGICAL	DebugMode = FALSE;

/* Structure to hold data */

static	const	STRING	MyTitle = "FPA Shuffle";
static			char	MyLabel[MAX_BCHRS];

/* Trap for error situations */
static	void	error_trap();


/* Base directory shuffle and file lock information */
static	char	DestDir[MAX_BCHRS]   = "";

/***********************************************************************
*                                                                      *
*     m a i n                                                          *
*                                                                      *
***********************************************************************/

int		main(argc, argv)

int		argc;
STRING	argv[];
	{
	int			bak, new;
	int			status, numargs, nslist;
	int			cyear, cjday, cmonth, cmday, chour, cmin, csec;
	STRING		setupfile, *slist, dir;
	char		homedir[MAX_BCHRS]; 

	STRING		fpa_source  = NullString;
	STRING		import_dir  = NullString;
	STRING		rtime       = NullString;

	FLD_DESCRIPT	fdesc;
	int 		devnull, oldstdout;
	static          FILE        *fp= NULL;

	/* Ignore hangup, interrupt and quit signals so we can survive after */
	/* logging off */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	status = setvbuf(stderr, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Set debug mode (if requested) */
	if ( DebugMode ) (void) pr_control(NULL, 5, 1);
	else             (void) pr_control(NULL, 1, 1);

	/*****************************************************************/
	/*****************************************************************/
	/** Validate run string parameters                              **/
	/*****************************************************************/
	/*****************************************************************/
	numargs = 3;
	if ( argc < (numargs + 1) )
		{
		(void) fprintf(stderr, "Usage:\n");
		(void) fprintf(stderr, "  <setup_file>");
		(void) fprintf(stderr, "  <source> <rtime>\n\n");
		return (-1);
		}

	/**********************************/
	/* REDIRECT STDOUT AWAY FROM SCRN */
	oldstdout = dup(1);
	devnull = open("/dev/null", "w");
	dup2(devnull,1);
	/**********************************/

	/* Obtain a licence */
	(void) fpalib_license(FpaAccessLib);


	/* Trap all signals that would abort the process by default */
	(void) set_error_trap(error_trap);

	/* Startup message */
	(void) sprintf(MyLabel, "[%d] %s:", getpid(), MyTitle);
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stderr, "%s Beginning: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);

	/* Set run string parameters */
	setupfile   = strdup(get_setup(argv[1]));
	fpa_source  = strdup(argv[2]);
	rtime       = strdup(argv[3]);
	

	/* Read the setup file */
	/* This moves to standard FPA directory */
	/*	nslist = setup_files(setupfile, &slist);*/
	if ( !define_setup(1, &setupfile) )
		{
		(void) fprintf(stderr, "%s Problem with setup file \"%s\"\n",
				MyLabel, setupfile);
		(void) fprintf(stderr, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Read the Config files */
	if ( !read_complete_config_file() )
		{
		(void) fprintf(stderr, "%s Problem with Config Files\n", MyLabel);
		(void) fprintf(stderr, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/**************************************************************************/
	/** Note that the "homedir" directory is the base data directory for FPA **/
	/**************************************************************************/
	dir = home_directory();
	(void) strcpy(homedir, dir);

	/* Initialize the field descriptor for files */
	(void) init_fld_descript(&fdesc);
	if ( !set_fld_descript(&fdesc,
							FpaF_DIRECTORY_PATH, homedir,
							FpaF_SOURCE_NAME,    fpa_source,
							FpaF_RUN_TIME,       rtime,
							FpaF_END_OF_LIST) )
		{
		(void) fprintf(stderr, "%s Problem initializing field descriptor\n",
				MyLabel);
		(void) fprintf(stderr, "  for \"%s\"  at \"%s\"\n", fpa_source, rtime);
		(void) fprintf(stderr, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Prepare data directory for output */
	dir = prepare_source_directory(&fdesc);
	(void) strcpy(DestDir, dir);
	if ( blank(DestDir) )
		{
		(void) fprintf(stderr, "%s Problem preparing data directory", MyLabel);
		(void) fprintf(stderr, " for \"%s %s\"  at \"%s\"\n",
				fdesc.sdef->name, fdesc.subdef->name, fdesc.rtime);
		(void) fprintf(stderr, "%s Aborted\n", MyLabel);
		return (-1);
		}

	/* Shutdown message */
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) fprintf(stderr, "\n%s Finished: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
			MyLabel, cyear, cmonth, cmday, chour, cmin, csec);

	/**********************************/
	/* REDIRECT STDOUT BACK TO SCRN   */
	fflush(stdout);
	dup2(oldstdout,1);
	/**********************************/

	/* Return the destination directory */
	(void) fseek(stdout,0,SEEK_SET);
	(void) fprintf(stdout, "%s\n", DestDir);
	return 0;
	}

/***********************************************************************
*                                                                      *
*     e r r o r _ t r a p                                              *
*                                                                      *
***********************************************************************/

static	void	error_trap(sig)
int		sig;
	{
	char	*sname;

	/* Ignore all further signals */
	(void) set_error_trap(SIG_IGN);
	(void) signal(sig, SIG_IGN);

	/* Get the signal name if possible */
	sname = signal_name(sig);

	/* Provide a message */
	(void) fprintf(stderr, "%s !!! %s Has Occurred - Terminating\n",
			MyLabel, sname);

	exit(1);
	}
