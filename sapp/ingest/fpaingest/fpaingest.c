/*******************************************************************************
*                                                                              *
*   f p a i n g e s t . c                                                      *
*                                                                              *
*   FPA Ingest Scheduler:  This program runs as a daemon.  It wakes period-    *
*                          ically to check whether any new GRIB data has       *
*                          arrived from CMC.  If so, it starts up the Ingest   *
*                          Process to decode and store the data.               *
*                                                                              *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)                    *
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
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
extern	FILE	*popen(const char *, const char *);

#undef DEBUG

#define MAXBUF 10000

/* Define structure for process types and associated buffer */
typedef struct
	{
	STRING	type;		/* Type of ingest */
	STRING	process;	/* Process to run for type */
	char    Buf[MAXBUF];/* Buffer for run string */
	}FpaMonitorTypeStruct;

/* >>> Use smarter way to track active ingests */
/* >>> Use more verbose status esp. when nothing happens */

/* Directory for maintaining lock files for active ingests */
static	STRING	LockDir = NullString;

/* Structure for calling modes */
typedef	enum
		{ Undefined, Startup, Shutdown, Wakeup, Status }
		SMODE;

static	char	Buf[MAXBUF];
static	STRING	MyName      = "fpaingest";
static	STRING	AltName     = "newgrib";
static	STRING	MyTitle     = "FPA Ingest Scheduler 2";
static	SMODE	MySmode     = NullPtr(SMODE);
static	STRING	MySfile     = NullString;
static	STRING	MySpath     = NullString;
static	char	MyLfile[50] = "";
static	STRING	MyLpath     = NullString;
static	pid_t	MyPid       = 0;
static	pid_t	MyPPid      = 0;
static	char	MyHost[150] = "";
static	char	MyUser[50]  = "";
static	char	MyLab[100]  = "";

static	STRING	BaseDir     = NullString;
static	STRING	WorkDir     = NullString;
static	STRING	*Patterns   = NullStringList;
static	STRING	*Dirs       = NullStringList;
static	STRING	*Types      = NullStringList;
static	int		NumPatterns = 0;
static	UNSIGN	WaitTime    = 0;
static	UNSIGN	RecheckTime = 60;
static	STRING	StatFile    = NullString;
static	STRING	LogFile     = NullString;
static	STRING	LogPrev     = NullString;
static	int		LogHour     = 0;
static	int		LogMin      = 0;
static	int		StatReady   = FALSE;

static	int		Monitor		= TRUE;
static	int		MoveLog		= TRUE;
static	int		StartLog	= TRUE;

#define nprintf (Monitor)?  (void) 0: (void) printf
#define dprintf (!Monitor)? (void) 0: (void) printf

static FpaMonitorTypeStruct	*MonitorList = NullPtr(FpaMonitorTypeStruct *);
static int					NumMonitorList = 0;
/* Internal static functions */
static	void	place_lock(void);
static	void	update_lock(void);
static	void	release_lock(void);
static	void	wake_trap(int);
static	void	term_trap(int);
static	void	mvlog_trap(int);
static	void	move_log(void);
static	int		check_files(void);
static	SMODE	get_smode(STRING);

FpaMonitorTypeStruct *find_type ( const STRING );
/*******************************************************************************
*                                                                              *
*   m a i n                                                                    *
*                                                                              *
*******************************************************************************/

int		main

	(
	int		argc,
	STRING	argv[]
	)

	{
	int			status, ready;
	UNSIGN		wtime;
	int			ipattern;
	int			nslist;
	STRING		*slist;
	int			nlock, ilock;
	STRING		*locklist, lfile, lpath, sp;
	FILE		*pipefile, *lockfile;
	pid_t		pid;
	char		*cp, user[50], host[150], spath[1000], dbuf[20];
	struct stat	stat_buf;
	time_t		current_time;
	long		dtime;
	LOGICAL		all, active, cleaned, found;

	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Obtain a licence */
	(void) app_license("ingest");

	/* Interpret mode given in run-string */
	/* Only respond to "startup", "shutdown", "wakeup" or "status" request */
	if (argc <= 1) MySmode = Undefined;
	else           MySmode = get_smode(argv[1]);

	/* Undefined command */
	if (MySmode == Undefined)
		{
		if (argc <= 1) (void) printf("No mode given\n");
		else           (void) printf("Unknown mode '%s'\n", argv[1]);
		(void) printf("Usage:\n");
		(void) printf("   %s startup | start | go | on [setup]\n", MyName);
		(void) printf("   %s shutdown | stop | down | kill | off [setup | ALL]\n",
						MyName);
		(void) printf("   %s wakeup | wake [setup | ALL]\n", MyName);
		(void) printf("   %s status | stat\n", MyName);
		return (-1);
		}

	/* Interpret setup file given in run-string */
	if (argc <= 2)
		{
		sp      = get_setup(NullString);
		MySpath = (NotNull(sp))? strdup(sp): NullString;
		MySfile = MySpath;
		all     = FALSE;
		(void) printf("\n%s: Setup file: %s\n", MyTitle, MySpath);
		}
	else if (same_ic(argv[2], "all"))
		{
		MySfile = NullString;
		MySpath = NullString;
		all     = TRUE;
		}
	else
		{
		MySfile = argv[2];
		sp      = get_setup(MySfile);
		MySpath = (NotNull(sp))? strdup(sp): MySfile;
		all     = FALSE;
		if (!same(MySpath, argv[2]))
			(void) printf("\n%s: Setup file: %s\n", MyTitle, MySpath);
		}

	(void) gethostname(MyHost, sizeof(MyHost));
	MyPid  = getpid();
	MyPPid = getppid();
	pipefile = popen("whoami", "r");
	(void) fscanf(pipefile, "%s", MyUser);
	(void) pclose(pipefile);
	current_time = time(NULL);

	/* Set the directory for maintaining lock files for active ingests */
	BaseDir = getenv("FPA");
	if (blank(BaseDir))
		{
		(void) printf("[Ingest] Cannot find FPA directory!\n");
		return (-1);
		}
	LockDir = safe_strdup(pathname(BaseDir, "ingest"));

	/* Find out what other processes are running */
	Recheck:
	nlock = dirlist(LockDir, "^\\.INGEST:", &locklist);
	found = FALSE;
	for (ilock=0; ilock<nlock; ilock++)
		{
		/* Parse ingest description from lock file name and contents */
		lfile = locklist[ilock];
		lpath = pathname(LockDir, lfile);

		pid = 0;
		(void) safe_strcpy(host, "");
		(void) safe_strcpy(spath, "");

		/* Hostname and PID come from filename */
		cp = strchr(lfile, ':');
		(void) safe_strcpy(host, cp+1);
		cp = strrchr(host, '.');
		(void) sscanf(cp+1, "%d", &pid);
		(void) safe_strcpy(cp, "");

		/* File contains user and setup file */
		lockfile = fopen(lpath, "r");
		if (!lockfile)
			{
			perror("  Cannot read lock file");
			continue;
			}
		(void) fscanf(lockfile, "%s%s", user, spath);
		(void) fclose(lockfile);

		/* Check heartbeat - get age of file in hours */
		status = stat(lpath, &stat_buf);
		if (status != 0)
			{
			perror("  Cannot stat lock file");
			continue;
			}
		dtime = (current_time - stat_buf.st_mtime) / 3600;
		(void) sprintf(dbuf, "%d hours", dtime);

		/* Check if it really is active */
		if (same(host, MyHost))
			{
			/* On same host - see if PID is running */
			active = running(pid);

			/* Make sure it really is fpaingest */
			/*
			if (active)
				{
				}
			*/
			}
		else
			{
			/* On different host - check if heartbeat is dead */
			active = (LOGICAL) (dtime < 240);
			}

		/* Cleanup inactive ones */
		cleaned = FALSE;
		if (!active)
			{
			status = unlink(lpath);
			if (status == 0) cleaned = TRUE;
			else             perror("  Cannot remove lock file");
			}

		(void) sprintf(Buf, "%s@%s:%d %s", user, host, pid, spath);
		if (active)
			{
			if (dtime < 2) (void) strcat(Buf, " - Active");
			else           {
						   (void) strcat(Buf, " - No activity for ");
						   (void) strcat(Buf, dbuf);
						   }
			}
		else
			{
			if (same(host, MyHost))
						   (void) strcat(Buf, " - Process not found");
			else           {
						   (void) strcat(Buf, " - No activity for ");
						   (void) strcat(Buf, dbuf);
						   }
			if (cleaned)   (void) strcat(Buf, " - Cleaned up");
			else           (void) strcat(Buf, " - Cannot remove lock file");
			}

		/* Take specified action wrt the currently running Ingest Scheduler */
		switch (MySmode)
			{
			case Startup:
					/* Make sure the same ingest is not already running */
					if (!same(host, MyHost))   continue;
					if (!same(spath, MySpath)) continue;
					if (!active)               continue;

					if (!found)
						(void) printf("\n%s: Ingest already running:\n",
								MyTitle);
					(void) printf("  %s\n", Buf);
					found = TRUE;
					break;

			case Shutdown:
					/* Make sure we have the right ingest and kill it */
					if (!same(host, MyHost))           continue;
					if (!all && !same(spath, MySpath)) continue;
					if (!active)                       continue;

					if (!found)
						(void) printf("\n%s: Shutting down:\n", MyTitle);
					(void) printf("  %s\n", Buf);
					(void) kill(pid, SIGTERM);
					found = TRUE;
					break;

			case Wakeup:
					/* Make sure we have the right ingest and wake it */
					if (!same(host, MyHost))           continue;
					if (!all && !same(spath, MySpath)) continue;
					if (!active)                       continue;

					if (!found)
						(void) printf("\n%s: Waking:\n", MyTitle);
					(void) printf("  %s\n", Buf);
					(void) kill(pid, SIGINT);
					found = TRUE;
					break;

			case Status:
					/* Report all activity */
					if (!found)
						(void) printf("\n%s: Current ingests:\n", MyTitle);
					(void) printf("  %s\n", Buf);
					found = TRUE;
					break;
			}
		}

	/* So, what happened? */
	switch (MySmode)
		{
		case Startup:
				/* Cannot start if already running */
				if (found)
					{
					(void) printf("\n%s: Scheduler not started\n", MyTitle);
					return (-1);
					}

				/* Cannot start without a setup file */
				if (blank(MySpath))
					{
					(void) printf("\n%s: Setup file required\n", MyTitle);
					return (-1);
					}

				/* Time to start the daemon ... */
				break;

		case Shutdown:
		case Wakeup:
				/* Did we find an ingest to shut down or wake up? */
				if (all && !found)
					{
					(void) printf("\n%s: No ingests were found\n", MyTitle);
					return (-1);
					}
				else if (!found)
					{
					(void) printf("\n%s: No ingest on %s was found\n",
								MyTitle, MySpath);
					}
					
				/* Go back and do a status */
				MySmode = Status;
				goto Recheck;

		case Status:
				/* Were there any ingests? */
				if (!found)
					{
					(void) printf("\n%s: No ingests were found\n", MyTitle);
					return (-1);
					}
				return (0);
		}

	/*********************************************************************
	* If everything is OK up to this point, we can get ready to start up *
	*********************************************************************/

	/* Now check out the setup file */
	nslist = setup_files(MySpath, &slist);
	if (!define_setup(nslist, slist))
		{
		(void) printf("%s: Problem with setup file: '%s'\n", MyTitle, MySpath);
		(void) printf("    Not started\n");
		return (-1);
		}

	/* Move to working directory */
	WorkDir = get_directory("ingest.src");
	if (chdir(WorkDir) != 0)
		{
		(void) perror("chdir");
		(void) printf("%s: Cannot access ingest directory: '%s'\n",
				MyTitle, WorkDir);
		(void) printf("    Not started\n");
		return (-1);
		}

	/* Interpret the "ingest" setup block */
	NumPatterns = get_ingest_monitor(&Patterns, &Dirs, &Types);
	if (NumPatterns <= 0)
		{
		(void) printf("%s: No files to monitor\n", MyTitle);
		(void) printf("    Not started\n");
		return (-1);
		}
	/*
	for (ipattern=0; ipattern<NumPatterns; ipattern++)
		{
		if (same(Types[ipattern], "grib")
				|| same(Types[ipattern], "grib2")) break;
		}
	if (ipattern >= NumPatterns)
		{
		(void) printf("%s: No GRIB files to monitor\n", MyTitle);
		(void) printf("    Not started\n");
		return (-1);
		}
	*/
	WaitTime = get_ingest_wait();
	LogFile  = get_ingest_log(&LogPrev, &LogHour, &LogMin);
	StatFile = get_ingest_stat();
	dprintf("%s: Wait time: %d\n", MyTitle, WaitTime);
	dprintf("%s: Log file: %s %s\n", MyTitle, LogFile, LogPrev);
	dprintf("%s: Status file: %s\n", MyTitle, StatFile);

	/*******************************************************************
	* If everything is OK up to this point, we can now become a daemon *
	*******************************************************************/
	(void) spawn(TRUE);
	MyPid = getpid();
	(void) sprintf(MyLab, "%s [%s:%d]", MyTitle, MyHost, MyPid);
	(void) printf("%s (%s) Started up\n", MyLab, FpaRevision);
	(void) signal(SIGTERM, term_trap);
	(void) signal(SIGINT, SIG_IGN);

	/* Build the lock file name */
	(void) sprintf(MyLfile, ".INGEST:%s.%d", MyHost, MyPid);
	MyLpath = strdup(pathname(LockDir, MyLfile));
	place_lock();

	/* Start output to the log file */
	if (StartLog) move_log();

	/* Check for new data at pre-defined intervals */
	while (TRUE)
		{
		/* Check files and ingest any updated ones */
		ready = check_files();
		wtime = (ready)? WaitTime: RecheckTime;

		/* Sleep for appropriate time or until awakened */
		(void) signal(SIGINT, wake_trap);
		(void) sleep(wtime);
		(void) signal(SIGINT, SIG_IGN);

		/* Future enhancement: Check here if setup file has been modified */

		/* Move log file if the right time */
		update_lock();
		if (MoveLog) move_log();
		}
	}

/*******************************************************************************
*                                                                              *
*   c o m p a r e _ t y p e s                                                  *
*   c o m p a r e _ k e y                                                      *
*   s o r t _ t y p e s                                                        *
*   f i n d _ t y p e s                                                        *
*                                                                              *
*******************************************************************************/
/* Compare function for qsort
 * compares structure to structure */
static	int		compare_types
	(
	 const void	*idlist1, 
	 const void *idlist2
	)
	{
	int cmp;
	/* Error returns for missing types */
	if ( IsNull(idlist1) )                                  return  1;
	if ( IsNull(((FpaMonitorTypeStruct *)idlist1)->type) )  return  1;
	if ( IsNull(idlist2) )                                  return -1;
	if ( IsNull(((FpaMonitorTypeStruct *)idlist2)->type) )  return -1;

	cmp = strcmp( ((FpaMonitorTypeStruct *)idlist1)->type, 
			      ((FpaMonitorTypeStruct *)idlist2)->type);
	return cmp;
	}

/* Compare function for bsearch 
 * compares structure to key */
static	int		compare_key
	(
	 const void *idlist1,
	 const void	*idlist2 
	)
	{
	int cmp;
	STRING key = (STRING)idlist1;
	FpaMonitorTypeStruct s1 = *(FpaMonitorTypeStruct *)idlist2;

	/* Error returns for missing types */
	if ( IsNull(idlist1) )                                  return  1;
	if ( IsNull(idlist2) )                                  return -1;
	if ( IsNull(((FpaMonitorTypeStruct *)idlist2)->type) )  return -1;

	cmp = strcmp( (STRING)idlist1, ((FpaMonitorTypeStruct *)idlist2)->type);
	return cmp;
	}

/* Use qsort to sort list of monitor types */
void	sort_types ()
	{
	(void) qsort( (POINTER) MonitorList, (size_t)NumMonitorList, 
				  sizeof(FpaMonitorTypeStruct), compare_types);
	}

/* Look up process and buffer strings for given monitor type. */
/* 	if one does not exist create it. */
FpaMonitorTypeStruct *find_type
	(
	 const STRING	key
	)
	{
	FpaMonitorTypeStruct *mtype;
	mtype = (FpaMonitorTypeStruct *) bsearch( key, (POINTER) MonitorList,
									  (size_t)NumMonitorList,
									  sizeof(FpaMonitorTypeStruct), 
									  compare_key);
	if ( IsNull(mtype) )
			{ /* Add a new type to the list */
			NumMonitorList++;
			MonitorList = GETMEM(MonitorList, FpaMonitorTypeStruct , NumMonitorList);
			mtype = &MonitorList[NumMonitorList-1];
			mtype->type = safe_strdup(key);
			if ( same(key,"grib") )
				mtype->process = safe_strdup("gribin");
			else if ( same(key,"grib2") )
				mtype->process = safe_strdup("gribin2");
			else
				mtype->process = safe_strdup(key);

			(void) safe_strcpy(mtype->Buf, "");
			(void) sort_types();
			}
	return mtype;
	}

/*******************************************************************************
*                                                                              *
*   p l a c e _ l o c k                                                        *
*   u p d a t e _ l o c k                                                      *
*   r e l e a s e _ l o c k                                                    *
*                                                                              *
*******************************************************************************/

static	void	place_lock(void)

	{
	FILE	*file;

	file = fopen(MyLpath, "w");
	if (!file)
		{
		(void) printf("%s Could not create lock file\n", MyLab);
		return;
		}
	(void) fprintf(file, "%s %s\n", MyUser, MySpath);
	(void) fclose(file);
	}

static	void	update_lock(void)

	{
	int		status;

	status = utime(MyLpath, 0);
	if (status != 0)
		{
		(void) printf("%s Could not update lock file\n", MyLab);
		return;
		}
	}

static	void	release_lock(void)

	{
	int		status;

	status = unlink(MyLpath);
	if (status != 0)
		{
		(void) printf("%s Could not remove lock file\n", MyLab);
		return;
		}
	}

/*******************************************************************************
*                                                                              *
*   w a k e _ t r a p                                                          *
*   t e r m _ t r a p                                                          *
*   m v l o g _ t r a p                                                        *
*                                                                              *
*******************************************************************************/

static	void	wake_trap

	(
	int		unused
	)

	{
	(void) printf("\n");
	(void) printf("%s Waking up\n", MyLab);
	}

static	void	term_trap

	(
	int		unused
	)

	{
	release_lock();
	(void) printf("\n");
	(void) printf("%s Shutting down\n", MyLab);
	(void) exit(0);
	}

static	void	mvlog_trap

	(
	int		unused
	)

	{
	MoveLog = TRUE;
	}

/*******************************************************************************
*                                                                              *
*   m o v e _ l o g                                                            *
*                                                                              *
*******************************************************************************/

static	void		move_log

	(void)

	{
	int		status, tnext;
	int		cyear, cjday, chour, cmin, csec, cmonth, cmday;
	int		nyear, njday, nhour, nmin, nsec, nmonth, nmday;

	/* Back-up and delete the current log file */
	if (find_file(LogFile))
		{
		(void) unlink(LogPrev);
		(void) link(LogFile, LogPrev);
		(void) unlink(LogFile);
		}

	/* Move stdout and stderr output to the new log file */
	(void) freopen(LogFile, "a", stdout);
	(void) freopen(LogFile, "a", stderr);
	status = setvbuf(stdout, NullString, _IOLBF, 0);
	status = setvbuf(stderr, NullString, _IOLBF, 0);

	/* Print a welcome message */
	(void) printf("\n");
	if (StartLog) (void) printf("%s Started up\n", MyLab);
	else          (void) printf("%s Restarting log\n", MyLab);
	(void) systime(&cyear, &cjday, &chour, &cmin, &csec);
	(void) mdate(&cyear, &cjday, &cmonth, &cmday);
	(void) printf("    Log started at: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
					cyear, cmonth, cmday, chour, cmin, csec);

	/* Set the alarm for the next move */
	StartLog = FALSE;
	MoveLog  = FALSE;
	nyear = cyear;
	njday = cjday;
	nhour = LogHour;
	nmin  = LogMin;
	nsec  = 0;
	if ((nhour == chour) && (nmin <= cmin)) njday++;
	else if (nhour < chour)                 njday++;
	(void) tnorm(&nyear, &njday, &nhour, &nmin, &nsec);
	(void) mdate(&nyear, &njday, &nmonth, &nmday);
	(void) printf("    Log to move at: %d/%.2d/%.2d %.2d:%.2d:%.2d GMT\n",
					nyear, nmonth, nmday, nhour, nmin, nsec);
	tnext = sdif(cyear, cjday, chour, cmin, csec,
					nyear, njday, nhour, nmin, nsec);
	(void) signal(SIGALRM, mvlog_trap);
	(void) alarm(tnext);
	}

/*******************************************************************************
*                                                                              *
*   c h e c k _ f i l e s                                                      *
*                                                                              *
*******************************************************************************/

static	int		check_files

	(void)

	{
	int			ifile, nfiles, ipattern, status, isf, irf, itypes;
	int			modified, ready, all_ready, keep_checking, buf_modified;
	STRING		*files, file, type, code;
	FILE		*fp;
	struct stat	stat_buf;
	char		mline[1024], mname[1024], mtype[64];
	time_t		mtime, ptime;
	off_t		msize;
	LOGICAL		mok;
	int			cyear, cjday, chour, cmin, csec;
	FpaMonitorTypeStruct *type_struct;
	static	STRING	*sfiles = NullStringList,		*rfiles = NullStringList;
	static	time_t	*stimes = NullPtr(time_t *),	*rtimes = NullPtr(time_t *);
	static	off_t	*ssizes = NullPtr(off_t *),		*rsizes = NullPtr(off_t *);
	static	STRING	*stypes = NullStringList,		*rtypes = NullStringList;
	static	int		nsfiles = 0,					nrfiles = 0;

	/* Keep checking for updated files until no more files are ready */
	do
		{
		(void) systime(&cyear, &cjday, &chour, &cmin, &csec);

		/* First time only - check the status file */
		if (!StatReady)
			{
			if (!find_file(StatFile))
				{
				dprintf("%s No initial file status\n", MyLab);
				}
			else if (fp = fopen(StatFile, "r"))
				{
				dprintf("%s Initial file status:\n", MyLab);
				while (NotNull(getfileline(fp, mline, sizeof(mline))))
					{
					(void) safe_strcpy(mname, string_arg(mline));
					if (blank(mname)) continue;
					mtime = long_arg(mline, &mok);
					if (!mok) continue;
					msize = long_arg(mline, &mok);
					if (!mok) continue;
					(void) safe_strcpy(mtype, string_arg(mline));
					if (blank(mtype)) (void) safe_strcpy(mtype, "grib");

					isf    = nsfiles++;
					sfiles = GETMEM(sfiles, STRING, nsfiles);
					stimes = GETMEM(stimes, time_t, nsfiles);
					ssizes = GETMEM(ssizes,  off_t, nsfiles);
					stypes = GETMEM(stypes, STRING, nsfiles);
					sfiles[isf] = strdup(mname);
					stimes[isf] = mtime;
					ssizes[isf] = msize;
					stypes[isf] = strdup(mtype);
					dprintf("    - %s %d %d %s\n", mname, mtime, msize, mtype);
					}
				(void) fclose(fp);
				}
			StatReady = TRUE;
			}

		/* Otherwise update internal status */
		else
			{
			FREEMEM(sfiles);	sfiles = rfiles;	rfiles = NullStringList;
			FREEMEM(stimes);	stimes = rtimes;	rtimes = NullPtr(time_t *);
			FREEMEM(ssizes);	ssizes = rsizes;	rsizes = NullPtr(off_t *);
			FREEMEM(stypes);	stypes = rtypes;	rtypes = NullStringList;
			nsfiles = nrfiles;
			nrfiles = 0;
			}

		/* Now check modification times */
		all_ready = TRUE;
		dprintf("%s Monitoring files:\n", MyLab);

		/* Reset all buffers */
		for (itypes = 0; itypes < NumMonitorList; itypes++)	
			{
			type_struct = find_type(Types[itypes]);
			(void) safe_strcpy( type_struct->Buf, "" );
			}
		buf_modified = FALSE;

		for (ipattern=0; ipattern<NumPatterns; ipattern++)
			{
			type = Types[ipattern];
			type_struct = find_type(Types[ipattern]);

			nfiles = dirlist(Dirs[ipattern], Patterns[ipattern], &files);
			for (ifile=0; ifile<nfiles; ifile++)
				{
				if (same(Dirs[ipattern], "."))
					 file = pathname(WorkDir, files[ifile]);
				else file = pathname(Dirs[ipattern], files[ifile]);
				status = stat(file, &stat_buf);
				if (status != 0) continue;
				mtime  = stat_buf.st_mtime;
				msize  = stat_buf.st_size;

				/* Find file in status list */
				code     = "new and not ready";
				modified = TRUE;
				ready    = FALSE;
				ptime    = 0;
				for (isf=0; isf<nsfiles; isf++)
					{
					if (same(sfiles[isf], file) && same(stypes[isf], type))
						{
						if (mtime > stimes[isf])
							{
							if (msize == ssizes[isf])
								{
								code     = "updated";
								modified = TRUE;
								ready    = TRUE;
								ptime    = mtime;
								}
							else
								{
								code     = "updated but not ready";
								modified = TRUE;
								ready    = FALSE;
								ptime    = stimes[isf];
								}
							}
						else if (mtime < stimes[isf])
							{
							code     = "backdated???";
							modified = FALSE;
							ready    = FALSE;
							ptime    = mtime;
							}
						else
							{
							code     = "unchanged";
							modified = FALSE;
							ready    = FALSE;
							ptime    = mtime;
							}
						break;
						}
					}

				/* Add to ingest run string if modified */
				if (modified && !ready) all_ready = FALSE;
				if (modified && ready)
					{
					if (!buf_modified)
						nprintf("%s %.2d:%.2d:%.2d Updated files:\n", MyLab, chour, cmin, csec);
					nprintf("    %s\n", file);

					/* Run the ingest on the current buffer and empty it */
					/*  if adding this file will overflow the buffer     */
					if ( (strlen(type_struct->Buf) + strlen(file)) 
							> (MAXBUF-(strlen(LogFile)+10)) )
						{
						(void) printf("%s Processing updated files\n",
											MyLab);
						(void) strcat(type_struct->Buf, " >> ");
						(void) strcat(type_struct->Buf, LogFile);
						(void) strcat(type_struct->Buf, " 2>&1");
						dprintf("Executing: '%s'\n", type_struct->Buf);
						(void) shrun(type_struct->Buf, TRUE); /* wait for return!
																 Otherwise you have 
																 log file issues. */
						(void) safe_strcpy(type_struct->Buf, "");
						}
					if (same(type_struct->Buf,"")) /* Setup run string */
						(void) sprintf(type_struct->Buf, "%s %s", type_struct->process, MySfile);
					(void) strcat(type_struct->Buf, " ");
					(void) strcat(type_struct->Buf, file); /* What if the file name is too long? 
															  Not likely but possible? */
					buf_modified = TRUE;
					}

				/* Add file to new status list */
				irf    = nrfiles++;
				rfiles = GETMEM(rfiles, STRING, nrfiles);
				rtimes = GETMEM(rtimes, time_t, nrfiles);
				rsizes = GETMEM(rsizes,  off_t, nrfiles);
				rtypes = GETMEM(rtypes, STRING, nrfiles);
				rfiles[irf] = strdup(file);
				rtimes[irf] = ptime;
				rsizes[irf] = msize;
				rtypes[irf] = strdup(type);
				dprintf("    - %s %d %d %s %s\n", file, mtime, msize, type, code);
				}
			}
		/* Now run ingest if required */
		if (!buf_modified)
			{
			(void) printf("%s %.2d:%.2d:%.2d No files updated\n",
						MyLab, chour, cmin, csec);
			keep_checking = FALSE;
			}
		else
			{
			(void) printf("%s Processing updated files\n", MyLab);
			for (itypes = 0; itypes < NumMonitorList; itypes++)
				{
				type_struct = &MonitorList[itypes];
				if ( !blank(type_struct->Buf) )
					{
					(void) strcat(type_struct->Buf, " >> ");
					(void) strcat(type_struct->Buf, LogFile);
					(void) strcat(type_struct->Buf, " 2>&1 ");
					dprintf("Executing: '%s'\n", type_struct->Buf);
					(void) shrun(type_struct->Buf, TRUE); /* Wait */
					}
				}
			(void) printf("%s %.2d:%.2d:%.2d Updated files processing\n",
						MyLab, chour, cmin, csec);

			/* Need to go back and check if anything else has been updated */
			buf_modified  = FALSE;
			keep_checking = TRUE;
			}

		/* Now update the status file */
		fp = fopen(StatFile, "w");
		for (irf=0; irf<nrfiles; irf++)
			{
			(void) fprintf(fp, "%s %d %d %s\n",
					rfiles[irf], rtimes[irf], rsizes[irf], rtypes[irf]);
			}
		(void) fclose(fp);

		} while (keep_checking);

	if (!all_ready) (void) printf("%s Waiting for files to finish updating\n",
									MyLab);
	return all_ready;
	}

/*******************************************************************************
*                                                                              *
*   g e t _ s m o d e                                                          *
*                                                                              *
*******************************************************************************/

static	SMODE	get_smode

	(
	STRING	mode
	)

	{
	if (blank(mode))           return Undefined;

	/* Recognized "startup" modes */
	if (same(mode, "startup"))  return Startup;
	if (same(mode, "start"))    return Startup;
	if (same(mode, "go"))       return Startup;
	if (same(mode, "on"))       return Startup;

	/* Recognized "shutdown" modes */
	if (same(mode, "shutdown")) return Shutdown;
	if (same(mode, "stop"))     return Shutdown;
	if (same(mode, "down"))     return Shutdown;
	if (same(mode, "kill"))     return Shutdown;
	if (same(mode, "off"))      return Shutdown;

	/* Recognized "wakeup" modes */
	if (same(mode, "wakeup"))   return Wakeup;
	if (same(mode, "wake"))     return Wakeup;

	/* Recognized "Status" modes */
	if (same(mode, "status"))   return Status;
	if (same(mode, "stat"))     return Status;

	return Undefined;
	}
