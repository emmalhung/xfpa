/*********************************************************************/
/** @file unix.c
 *
 * Assorted unix system calls with practical embellishments.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     u n i x . c                                                      *
*                                                                      *
*     Assorted unix system calls with practical embellishments.        *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)            *
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

#include "unix.h"
#include "parse.h"
#include "string_ext.h"

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#if defined(MACHINE_HP) || defined(MACHINE_SUN) || defined(MACHINE_PCLINUX)
# include <regex.h>
# undef REGEX_OLD
#else
# include <libgen.h>
# define REGEX_OLD
#endif

#if defined(MACHINE_PCLINUX)
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))
#endif

/* Set various debug modes */
#undef DEBUG_PATHNAME
#undef DEBUG_PC

#include <fpa_math.h>
#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>

/***********************************************************************
*                                                                      *
*     f s l e e p                                                      *
*                                                                      *
***********************************************************************/

static	void	trap_alarm(int);

static	void	trap_alarm(int ignore)	{ return; }

/* Undefine timercmp to avoid complaints about a previous definition */
#undef  timercmp
#define timercmp(tvp, uvp, cmp) \
          ((tvp)->tv_sec cmp (uvp)->tv_sec || \
           (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)


/*********************************************************************/
/** Sleep for sec seconds plus usec microseconds.
 *
 * i.e. non-integer version of sleep(3C).
 *
 * @param[in] sec whole seconds
 * @param[in] usec plust microseconds
 * @return Zero if the requested time has elapsed, or the number of
 * seconds left to sleep.
 *********************************************************************/
long	fsleep

	(
	long	sec,
	long	usec
	)

	{
	int		wait_old, old_set;
	long	sec_left, usec_left, old_sec, old_usec, snorm;
	void	(*prev_alarm)(int);

	static	struct itimerval	new_value, old_value;
	static	struct timeval		*new_val = &new_value.it_value;
	static	struct timeval		*new_int = &new_value.it_interval;
	static	struct timeval		*old_val = &old_value.it_value;
	/* May come in handy later
	static	struct timeval		*old_int = &old_value.it_interval;
	*/

	/* Make sure arguments are reasonable */
	if ((usec < 0) || (usec >= 1000000))
		{
		snorm = usec/1000000;
		if (usec < 0) snorm--;
		sec  += snorm;
		usec -= snorm*1000000;
		}
	if (sec < 0)                   return sec;
	if ((sec == 0) && (usec == 0)) return sec;

	/* Turn off alarm catching routine if any */
	prev_alarm = signal(SIGALRM, trap_alarm);

	/* Set up new timer value */
	new_val->tv_sec  = (unsigned long) sec;
	new_val->tv_usec = usec;
	new_int->tv_sec  = 0;
	new_int->tv_usec = 0;

	/* Obtain old timer value and see if it is set to go off earlier */
	/* than this new request */
	/* If set to go off earlier - wait for that alarm instead */
	(void) getitimer(ITIMER_REAL, &old_value);
	wait_old = 0;
	old_set  = 0;
	old_sec  = 0;
	old_usec = 0;
	if (timerisset(old_val))
		{
		wait_old = (int) (!timercmp(new_val, old_val, <));
		old_set  = 1;
		old_sec  = old_val->tv_sec;
		old_usec = old_val->tv_usec;
		}

	/* Determine how much time would be left if timer runs out fully */
	if (wait_old)
		{
		sec_left  = sec  - old_val->tv_sec;
		usec_left = usec - old_val->tv_usec;
		if (usec_left < 0)
			{
			sec_left--;
			usec_left += 1000000;
			}
		}
	else
		{
		sec_left  = 0;
		usec_left = 0;
		}

	/* Now set the timer and wait for a signal to come in */
	if (!wait_old) (void) setitimer(ITIMER_REAL, &new_value, &old_value);
	(void) pause();

	/* Good morning - See how much time is left */
	(void) getitimer(ITIMER_REAL, &new_value);
	sec_left  += new_val->tv_sec;
	usec_left += new_val->tv_usec;
	if (usec_left >= 1000000)
		{
		sec_left++;
		usec_left -= 1000000;
		}

	/* Restore the timer to decremented original setting if necesasary */
	if (!wait_old)
		{
		if (old_set)
			{
			old_sec  -= sec  - sec_left;
			old_usec -= usec - usec_left;
			if (old_usec < 0)
				{
				old_sec--;
				old_usec += 1000000;
				}
			old_val->tv_sec  = old_sec;
			old_val->tv_usec = old_usec;
			}
		(void) setitimer(ITIMER_REAL, &old_value, &new_value);
		}

	/* Restore alarm catching routine if any - and call it if we really */
	/* got a SIGALRM */
	(void) signal(SIGALRM, prev_alarm);
	/* We really got a SIGALRM if old timer ran out fully */
	if (wait_old && old_set)
		{
		/* Regenerating a SIGALRM signal will cause the catching */
		/* routine to be called */
		(void) raise(SIGALRM);
		}

	return sec_left;
	}

/***********************************************************************
*                                                                      *
*     s e t _ s t o p w a t c h                                        *
*     g e t _ s t o p w a t c h                                        *
*                                                                      *
*     Control a stopwatch timer.                                       *
*                                                                      *
***********************************************************************/

static	struct	timeval		tp1, tp2;
static	long	Nsec = 0,	Nusec = 0;
static	long	Csec = 0,	Cusec = 0;

/*********************************************************************/
/** Set a stop watch timer.
 *
 * Two timers are invoked:
 * 	- an elapsed timer, which counts time since the last call to
 * 	  set_stopwatch(),
 * 	- a cumulative time, which counts time since the last "reset".
 *	@param[in]	reset	OK to reset cumulative timer?
 *********************************************************************/
void	set_stopwatch

	(
	LOGICAL	reset
	)

	{
	/* Reset cumulative time */
	if (reset)
		{
		Csec  = 0;
		Cusec = 0;
		}

	/* Reset start time */
	(void) gettimeofday(&tp1, NULL);
	}

/*********************************************************************/
/** Get the elapsed time since the last 'set'.
 *
 * Two timers are checked:
 * 	- an elapsed timer, which counts time since the last call to
 * 	  set_stopwatch(),
 * 	- a cumulative time, which counts time since the last "reset".
 *
 * Both timers are returned as seconds plus microseconds.
 *	@param[out]	*nsec	 elapsed seconds since last 'set'
 *	@param[out]	*nusec	 elapsed microseconds since last 'set'
 *	@param[out]	*csec	 cumulative seconds since last 'reset'
 *	@param[out]	*cusec	 cumulative microseconds since last 'reset'
 *********************************************************************/
void	get_stopwatch

	(
	long	*nsec,
	long	*nusec,
	long	*csec,
	long	*cusec
	)

	{
	/* Get current time */
	(void) gettimeofday(&tp2, NULL);

	/* Compute elapsed time since last 'set' */
	Nsec  = tp2.tv_sec  - tp1.tv_sec;
	Nusec = tp2.tv_usec - tp1.tv_usec;
	if (Nusec < 0)
		{
		Nsec  -= 1;
		Nusec += 1000000;
		}

	/* Compute cumulative time since last 'reset' */
	Csec  += Nsec;
	Cusec += Nusec;
	if (Cusec >= 1000000)
		{
		Csec  += 1;
		Cusec -= 1000000;
		}

	/* Return what was asked for */
	if (nsec)  *nsec  = Nsec;
	if (nusec) *nusec = Nusec;
	if (csec)  *csec  = Csec;
	if (cusec) *cusec = Cusec;
	}

/***********************************************************************
*                                                                      *
*    s p a w n                                                         *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Spawn a child process - used for creating a server.
 *
 * The current process is "fork"ed, thereby producing a
 * detached child.  The child then disconnects its terminal
 * affiliation and becomes the server.  The Parent hangs around
 * long enough to make sure it worked.
 *
 * On success, the child (server) returns and the parent is
 * terminated.  The real server may be subsequently "exec"ed
 * if required.
 *
 * On failure, the parent returns and the child (if created)
 * is aborted
 *
 * ONLY the parent OR the child will return - NEVER both.
 *
 *	@param[in]	detach	OK to detach from terminal?
 * 	@return
 * 	-  0 :- success: I am now the server
 * 	- -1 :- failure: fork failed
 *********************************************************************/
int	spawn

	(
	LOGICAL	detach
	)

	{
	pid_t		pid;

	/* Fork myself:
	*
	*       - Child becomes the server, (the real server can be
	*         "exec"ed later).
	*
	*       - Parent becomes a client of the child, long enough
	*         to make sure the server got created.
	*/
	pid = fork();
	if (pid < 0)
		{
#		ifdef PRINT_ERRORS
		perror("[spawn] failed to spawn server process");
#		endif
		return -1;
		}

	/* Parent process */
	if (pid > 0)
		{
		/* Go away and let the child do the work */
#		ifdef PRINT_STATUS
		(void) printf("[spawn] started up server process %d\n", pid);
		(void) printf("[spawn] scheduler normal exit\n");
#		endif
		exit(0);
		}

	/* Child process */
	/* Disconnect from terminal session */
	if (detach) (void) setpgrp();
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);

	/* Success - Return 0 */
#	ifdef PRINT_STATUS
	(void) printf("[spawn] server ready\n");
#	endif
	return 0;
	}


/***********************************************************************
*                                                                      *
*      s h r u n                                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Spawn a Bourne shell and run a process.  The parent can
 * either wait for the child to complete or not.  If sucessful
 *
 *	@param[in]	pgm				string containing program or script to run
 *	@param[in]	waitforchild	OK to wait for the process to finish?
 * 	@return
 * 	-  0 If successful.
 *	- -1. If not successful.
 *********************************************************************/

int	shrun

	(
	STRING	pgm,
	LOGICAL	waitforchild
	)

	{
	pid_t	id;

	/* if we are not going to wait around for the child process we */
	/* must turn off the child signal to avoid zombie creation.    */

	fix_env(FALSE);

	if (!waitforchild) (void) signal(SIGCLD, SIG_IGN);
	if ( ( id = vfork() ) == 0 )
		{
			(void) execl("/bin/bash", "sh", "-c", pgm, NULL);
			exit(-1);
		}
	if (id < 0) return -1;
	if (waitforchild) (void) waitpid(id, NULL, 0);
	return 0;
	}

/***********************************************************************
*                                                                      *
*      r u n n i n g                                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Test whether the given process is actually running.
 *
 *	@param[in]	pid		process ID
 * 	@return
 * 	- TRUE if process is running.
 * 	- FALSE if not.
 *********************************************************************/

LOGICAL	running

	(
	pid_t	pid
	)

	{
	int		status;

	/* Send fake signal to pid - will fail if pid is not active */
	status = kill(pid, 0);
	if (status == 0) return TRUE;
	if (errno != ESRCH) return TRUE;
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*      k i l l _ p r o c e s s                                         *
*                                                                      *
*                                                                      *
*      k i l l _ b o t t o m _ u p                                     *
*                                                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Send the kill signal to the given process.
 *
 *	@param[in]	pid		process ID
 * @return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	kill_process

	(
	pid_t	pid
	)

	{
	int		status;

	/* Send the kill signal to pid - will fail if pid is not active */
	status = kill(pid, SIGKILL);
	return (status == 0)? TRUE: FALSE;
	}

/*********************************************************************/
/** Send the kill signal to all the children of the given process,
 * their children, etc., from the bottom up. Kills the given process
 * itself, only if killtop is TRUE.
 *
 *	@param[in]	pid		PID of process whose children are to be killed
 *	@param[in]	killtop	Kill the given process too?
 *	@param[in]	nap		Time to sleep between killing children and parent
 * 	@return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	kill_bottom_up

	(
	pid_t	pid,
	LOGICAL	killtop,
	UNSIGN	nap
	)

	{
	char	cmd[100];
	FILE	*p, *popen();
	int		ikid, nkids = 0;
	pid_t	*kids = NULL;

	/* Find out what processes have this one as parent */
	(void) sprintf(cmd, "pf -ef | cut -c9-20 | grep \"%.6d$\"", pid);
	p = popen(cmd, "r");
	if (!p) return FALSE;
	while (getfileline(p, cmd, sizeof(cmd)))
		{
		(void) strcpy(cmd+6, "");
		nkids++;
		kids = GETMEM(kids, pid_t, nkids);
		(void) sscanf(cmd, "%d", &(kids[nkids-1]));
		}
	(void) pclose(p);

	/* Kill each of the children's children */
	/* Go in reverse order, to kill the most recent first */
	for (ikid=nkids-1; ikid>=0; ikid--)
		{
		(void) kill_bottom_up(kids[ikid], TRUE, nap);
		}
	FREEMEM(kids);
	if (killtop)
		{
		(void) printf("Killing %d\n", pid);
		if (nkids > 0) (void) sleep(nap);
		(void) kill_process(pid);
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      f i n d _ f i l e                                               *
*      c r e a t e _ f i l e                                           *
*      r e m o v e _ f i l e                                           *
*      m o v e _ f i l e                                               *
*                                                                      *
*      Test whether the given file exists (etc.).                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Test whether the given file exists.
 *
 *	@param[in]	path	path of file to find
 * 	@return
 * 	- TRUE if file exists.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	find_file

	(
	STRING	path
	)

	{
	struct	stat	sbuf;
	int				status;

	if (blank(path)) return FALSE;

	status = stat(path, &sbuf);
	if (status != 0) return FALSE;

	if (S_ISREG(sbuf.st_mode)) return TRUE;
	return FALSE;
	}

/*********************************************************************/
/** Create a given file. If the file exists return without doing
 * anything. If the file does not already exist then create it and
 * indicate that the file had to be created.
 *
 * @param[in] path path of file to create
 * @param[out] *created did we have to create it?
 * @return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	create_file

	(
	STRING	path,
	LOGICAL	*created
	)

	{
	FILE	*fp;

	if (created) *created = FALSE;
	if (find_file(path)) return TRUE;

	fp = fopen(path, "w");
	if (!fp) return FALSE;

	(void) fclose(fp);
	if (created) *created = TRUE;
	return TRUE;
	}

/*********************************************************************/
/** Remove the given file. If the file does not exist do nothing. If
 * it does exist remove it and indicate that it had to be removed.
 *
 * @param[in] path path of file to remove
 * @param[out] *removed did we have to remove it?
 * @return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	remove_file

	(
	STRING	path,
	LOGICAL	*removed
	)

	{
	int		status;

	if (removed) *removed = FALSE;
	if (!find_file(path)) return TRUE;

	status = unlink(path);
	if (status != 0) return FALSE;

	if (removed) *removed = TRUE;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      f i n d _ d i r e c t o r y                                     *
*      c r e a t e _ d i r e c t o r y                                 *
*      r e m o v e _ d i r e c t o r y                                 *
*      m o v e _ d i r e c t o r y                                     *
*                                                                      *
*      Test whether the given directory exists (etc.).                 *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Test whether the given directory exists.
 *
 *	@param[in]	path	path of directory to find
 * 	@return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	find_directory

	(
	STRING	path
	)

	{
	struct	stat	sbuf;
	int				status;

	if (blank(path)) return FALSE;

	status = stat(path, &sbuf);
	if (status != 0) return FALSE;

	if (S_ISDIR(sbuf.st_mode)) return TRUE;
	return FALSE;
	}

/*********************************************************************/
/** Create the given directory. If the directory already exists do
 * nothing. If it does not exist create it and indicate it had to be
 * created.
 *
 * @param[in] path path of directory to create
 * @param[in] mode permissions (r/w/x read/write/execute)
 * @param[out] created did we have to creaet it?
 * @return
 * 	- TRUE if successful
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	create_directory

	(
	STRING	path,
	mode_t	mode,
	LOGICAL	*created
	)

	{
	STRING	op;
	int		status;
	LOGICAL	pc;

	if (created) *created = FALSE;
	if (blank(path)) return FALSE;

	if (find_directory(path)) return TRUE;

	/* Try to create the directory */
	status = mkdir(path, mode);
	if (status == 0)
		{
		if (created) *created = TRUE;
		return TRUE;
		}
	if (errno != ENOENT) return FALSE;

	/* Try to create the parent path */
	op = strdup(path);
	pc = create_directory(dir_name(op), mode, NULL);
	if ( !pc )
		{
		FREEMEM(op);
		return FALSE;
		}

	/* Now try to create the directory */
	status = mkdir(op, mode);
	FREEMEM(op);
	if (status == 0)
		{
		if (created) *created = TRUE;
		return TRUE;
		}
	return FALSE;
	}


/*********************************************************************/
/** Remove the given directory. If the directory does not exist do
 * nothing. If it does exist then remove it and indicate that it had
 * to be removed.
 *
 * @param[in] path path of directory to delete
 * @param[out] *removed did we have to delete it?
 * @return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	remove_directory

	(
	STRING	path,
	LOGICAL	*removed
	)

	{
	int			n, nrecs;
	STRING		buf, *list, ptr;
	LOGICAL     ok;
	struct stat	stbuf;

	if (removed) *removed = FALSE;
	if (blank(path)) return FALSE;
	if (!find_directory(path)) return FALSE;

	/* Obtain a list of the contents */
	(void) dirlist_reuse(FALSE);
	nrecs = dirlist(path, NULL, &list);
	(void) dirlist_reuse(TRUE);

	buf = INITMEM(char, strlen(path)+258);
	(void) strcpy(buf, path);
	(void) strcat(buf, "/");
	ptr = buf + strlen(buf);

	for(n=0; n<nrecs; n++)
	{
		(void) strcpy(ptr, list[n]);
		if(stat(buf, &stbuf) == 0)
			{
			if(S_ISDIR(stbuf.st_mode)) (void) remove_directory(buf, &ok);
			else                       (void) unlink(buf);
			}
	}
	FREEMEM(buf);
	FREELIST(list, nrecs);

	ok = (rmdir(path) == 0);
	if (removed) *removed = ok;
	return ok;
	}


/***********************************************************************
*                                                                      *
*     d i r l i s t                                                    *
*                                                                      *
***********************************************************************/

static	int	strpcmp(const void *, const void *);
static	int	strpcmp(const void *a, const void *b)
	{
	return strcmp(*(STRING *)a, *(STRING *)b);
	}

static	LOGICAL	Reuse = TRUE;

/*********************************************************************/
/** Set a switch to indicate whether you want to reuse the directory
 * list buffer or whether you want to regenerate it.
 *
 *	@param[in]	reuse	reuse listing buffer (TRUE)
 *						or regenerate (FALSE)
 *********************************************************************/
int	dirlist_reuse

	(
	LOGICAL	reuse
	)

	{
	Reuse = reuse;
	return 0;
	}

/*********************************************************************/
/** Return a listing for the given directory.
 *
 * 	@param[in] directory path of directory to read
 * 	@param[in] expression optional file mask (regular expression)
 * 	@param[out] **filelist list of files
 * 	@return The size of filelist.
 *********************************************************************/
int	dirlist

	(
	STRING	directory,
	STRING	expression,
	STRING	**filelist
	)

/* Need to use obsolescent form of regcmp(), regex() */
#ifdef REGEX_OLD
	{
	DIR				*dirpt;
	struct dirent	*dp;
	char			*exptr, *mptr;
	int             nfilelist;

	static	STRING	*list = NULL;
	static	int		nrecs = 0;
	static	int		mrecs = 0;
	static	int		drecs = 50;

	/* Free the existing list if there is one */
	if (Reuse) FREELIST(list, nrecs);
	list  = NULL;
	nrecs = 0;
	mrecs = 0;

	/* Make sure we can return something */
	if (!filelist) return nrecs;
	*filelist = NULL;

	/* Compile the regular expression */
	if (!expression) expression = "^.*";
	exptr = regcmp(expression, 0);
	if (!exptr) return nrecs;

	/* Open the directory */
	if (!directory) dirpt = opendir(".");
	else            dirpt = opendir(directory);

	/* Create the list of file names */
	while ((dp = readdir(dirpt)) != NULL)
		{
		if (same(dp->d_name, ".") || same(dp->d_name, "..")) continue;
		mptr = regex(&exptr, dp->d_name);
		if (!mptr) continue;
		if (nrecs >= mrecs)
			{
			mrecs += drecs;
			list = GETMEM(list, STRING, mrecs);
			}
		list[nrecs] = strdup(dp->d_name);
		nrecs++;
		}
	(void) closedir(dirpt);
	free(&exptr);

	/* Sort the list if required */
	if (nrecs > 1) qsort((void *)list, nrecs, sizeof(STRING), strpcmp);
	*filelist = list;
	nfilelist = nrecs;

	if(!Reuse)
		{
		list  = NULL;
		nrecs = 0;
		}
	return nfilelist;
	}

/* Use newer form of regcomp(), regexec() */
#else
	{
	DIR				*dirpt;
	struct dirent	*dp;
	regex_t			regptr;
	int				nfilelist, status;

	static	STRING	*list = NULL;
	static	int		nrecs = 0;
	static	int		mrecs = 0;
	static	int		drecs = 50;

	/* Free the existing list if there is one */
	if (Reuse) FREELIST(list, nrecs);
	list  = NULL;
	nrecs = 0;
	mrecs = 0;

	/* Make sure we can return something */
	if (!filelist) return nrecs;
	*filelist = NULL;

	/* Compile the regular expression */
	if (!expression) expression = "^.*";
	status = regcomp(&regptr, expression, REG_NOSUB);
	if (status != 0) return nrecs;

	/* Open the directory */
	if (!directory) dirpt = opendir(".");
	else            dirpt = opendir(directory);
	if (!dirpt) return nrecs;

	/* Create the list of file names */
	while ((dp = readdir(dirpt)) != NULL)
		{
		if (same(dp->d_name, ".") || same(dp->d_name, "..")) continue;
		status = regexec(&regptr, dp->d_name, (size_t)0, NULL, 0);
		if (status != 0) continue;
		if (nrecs >= mrecs)
			{
			mrecs += drecs;
			list = GETMEM(list, STRING, mrecs);
			}
		list[nrecs] = strdup(dp->d_name);
		nrecs++;
		}
	(void) closedir(dirpt);
	regfree(&regptr);

	/* Sort the list if required */
	if (nrecs > 1) qsort((void *)list, nrecs, sizeof(STRING), strpcmp);
	*filelist = list;
	nfilelist = nrecs;

	if(!Reuse)
		{
		list  = NULL;
		nrecs = 0;
		}
	return nfilelist;
	}
#endif

/***********************************************************************
*                                                                      *
*     f i x _ e n v                                                    *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Check the environment for any problems.
 *
 *	@param[in]	print	OK to print debug info?
 *********************************************************************/

void	fix_env

	(
	LOGICAL	print
	)

	{
	int				dp;
	STRING			*ep;
	extern STRING	*environ;

	if (print) (void) fprintf(stdout, "[fix_env] Environment:\n");

	dp = 0;
	ep = environ;
	while (*ep != NULL)
		{
		/* See if variable is empty */
		if (blank(*ep))
			{
			(void) fprintf(stderr,
						   "[fix_env] Removing empty environment variable\n");
			*ep = NULL;
			dp++;
			}

		/* Move up rest of environment */
		else if (dp > 0)
			{
			*(ep-dp) = *ep;
			*ep      = NULL;
			}

		/* Next */
		if (print) (void) fprintf(stdout, "\t'%s'\n", ((*ep)? *ep: ""));
		ep++;
		}
	}


/***********************************************************************
*                                                                      *
*     e n v _ s u b                                                    *
*     e n v _ c o m p u t e                                            *
*                                                                      *
*     Expand the given string if it contains an environment variable.  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Expand the given string if it contains an environment variable.
 *
 *	@param[in]	arg		string to be expanded
 * 	@return Pointer to the expanded string. The buffer returned is static
 * 			and owned by the function; if you are not going to use it
 * 			immediately make a copy of it with safe_copy or safe_strdup.
 *********************************************************************/
STRING	env_sub

	(
	STRING	arg
	)

	{
	int		subsize;

	static	char	buffer[2048] = "";

	if (IsNull(arg)) return NULL;
	subsize = env_compute(arg, buffer, sizeof(buffer));
	if (subsize >= sizeof(buffer))
		{
		/* If too long do something later <<< */
		return buffer;
		}
	return buffer;
	}

/*********************************************************************/
/** Expand the given string if it contains an environment variable.
 *
 *	@param[in]	arg		string to be expanded
 *	@param[in]	buffer	buffer to hold expanded string
 *	@param[in]	size	size of expanded string
 * 	@return
 * 	- length of expanded string if successful.
 * 	- 0 otherwise.
 *********************************************************************/
int	env_compute

	(
	STRING	arg,
	STRING	buffer,
	int		size
	)

	{
	int		nlen, nrem, nc;
	STRING	magic, p, q;
	char	ebuf[256];

	static	STRING	Full = "\'\"\\$";
	static	STRING	Part = "\"\\$";
	static	STRING	Name =
			"0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if (buffer) (void) strcpy(buffer, "");
	if (blank(arg)) return 0;

	nlen  = 0;
	nrem  = size - 1;
	magic = Full;
	while (1)
		{
		/* Find the next magic character */
		p = strpbrk(arg, magic);

		/* No magic characters left - copy the rest and return */
		if (!p)
			{
			nc = strlen(arg);
			if (nc > nrem)
				{
				(void) strncat(buffer, arg, nrem);
				buffer[size-1] = '\0';
				return size-1;
				}
			else
				{
				(void) safe_strcat(buffer, arg);
				return strlen(buffer);
				}
			}

		/* Found a magic character - copy everything up to it */
		nc = p - arg;
		if (nc > nrem)
			{
			(void) strncat(buffer, arg, nrem);
			buffer[size-1] = '\0';
			return size-1;
			}
		(void) strncat(buffer, arg, nc);
		nlen += nc;
		nrem -= nc;
		buffer[nlen] = '\0';
		arg += nc;

		/* Which character did we find? */
		switch (*p)
			{
			/* Found a single quote - copy up to the next single quote */
			/*                        without substitution */
			case '\'':	arg++;
						q = strchr(arg, '\'');

						/* No matching quote - copy the rest and return  */
						if (!q)
							{
							nc = strlen(arg);
							if (nc > nrem)
								{
								(void) strncat(buffer, arg, nrem);
								buffer[size-1] = '\0';
								return size-1;
								}
							else
								{
								(void) safe_strcat(buffer, arg);
								return strlen(buffer);
								}
							}

						/* Found matching quote - copy up to it then skip */
						nc = q - arg;
						if (nc > nrem)
							{
							(void) strncat(buffer, arg, nrem);
							buffer[size-1] = '\0';
							return size-1;
							}
						(void) strncat(buffer, arg, nc);
						nlen += nc;
						nrem -= nc;
						buffer[nlen] = '\0';
						arg += nc;
						break;

			/* Found a double quote - toggle between full and partial */
			/*                        (quoted) substitution */
			case '\"':	arg++;
						magic = (magic == Full)? Part: Full;
						break;

			/* Found a back-slash - copy the next character explicitly */
			case '\\':	arg++;

						/* Nothing follows - return */
						if (strlen(arg) <= 0)
							{
							return strlen(buffer);
							}

						/* Copy the next character and skip */
						nc = 1;
						if (nc > nrem)
							{
							(void) strncat(buffer, arg, nrem);
							buffer[size-1] = '\0';
							return size-1;
							}
						(void) strncat(buffer, arg, nc);
						nlen += nc;
						nrem -= nc;
						buffer[nlen] = '\0';
						arg += nc;
						break;

			/* Found a dollar sign - substitute the next word */
			case '$':	arg++;
						nc = strspn(arg, Name);

						/* No legal name follows - carry on */
						if (nc <= 0) continue;

						/* Build variable name and substitute */
						(void) strncpy(ebuf, arg, nc);
						ebuf[nc] = '\0';
						arg += nc;
						q = getenv(ebuf);
						if (q)
							{
							nc = strlen(q);
							if (nc > nrem)
								{
								(void) strncat(buffer, q, nrem);
								buffer[size-1] = '\0';
								return size-1;
								}
							(void) strncat(buffer, q, nc);
							nlen += nc;
							nrem -= nc;
							buffer[nlen] = '\0';
							}
						break;

			}
		}
	}

/***********************************************************************
*                                                                      *
*     b a s e n a m e   - extract filename portion same as basename(1) *
*     d i r n a m e     - extract directory portion same as dirname(1) *
*     p a t h n a m e   - combine directory and filename into a path   *
*     a b s p a t h     - is this a full or relative path?             *
*                                                                      *
*     Extract filename and directory portions of a file pathname or    *
*     put them together.                                               *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Extract filename portion of path. This function is the same as
 * basename(1).
 *
 *	@param[in]	string	full path
 *	@param[in]	suffix	optional suffix
 * 	@return Pointer to filename. The buffer returned is static
 * 			and owned by the function; if you are not going to use it
 * 			immediately make a copy of it with safe_copy or safe_strdup.
 *********************************************************************/
STRING	base_name

	(
	STRING	string,
	STRING	suffix
	)

	{
	static	STRING	base = NULL;

	int		l1, l2;
	STRING	c;

	FREEMEM(base);
	if (blank(string)) return base;

	c = strrchr(string, '/');
	if (!c) c = string;
	else    c++;
	base = strdup(c);

	if (blank(suffix)) return base;

	l1 = strlen(base);
	l2 = strlen(suffix);
	if (l1 >= l2)
		{
		c = base + l1 - l2;
		if (same(c, suffix)) (void) strcpy(c, "");
		}

	return base;
	}

/*********************************************************************/
/** Extract directory portion of path. This function is the same as
 * dirname(1).
 *
 *	@param[in]	string	full path
 * 	@return Pointer to directory. The buffer returned is static
 * 			and owned by the function; if you are not going to use it
 * 			immediately make a copy of it with safe_copy or safe_strdup.
 *********************************************************************/
STRING	dir_name

	(
	STRING	string
	)

	{
	static	STRING	dir = NULL;

	int		n;
	STRING	c;

	FREEMEM(dir);
	if (blank(string))
		{
		dir = strdup(".");
		return dir;
		}

	dir = strdup(string);
	c = strrchr(string, '/');
	if (!c)
		{
		FREEMEM(dir);
		dir = strdup(".");
		return dir;
		}

	n = strlen(dir) - strlen(c);
	if (n >= 0) (void) strcpy(dir+n, "");
	return dir;
	}

/*********************************************************************/
/** Combine directory and filename into a path.
 *
 *	@param[in]	dir		directory path
 *	@param[in]	file	local filename
 * 	@return Pointer to path. The buffer returned is static
 * 			and owned by the function; if you are not going to use it
 * 			immediately make a copy of it with safe_copy or safe_strdup.
 *********************************************************************/
STRING	pathname

	(
	STRING	dir,
	STRING	file
	)

	{
	static	STRING	path = NULL;

	LOGICAL	samedir=FALSE, samefile=FALSE;
	STRING	xdir, xfile;
	size_t	nc, nd, nf;

#	ifdef DEBUG_PATHNAME
	(void) printf("[pathname] dir: %d \"%s\"   file: %d \"%s\"\n",
		   &dir, dir, &file, file);
	if (path) (void) printf("[pathname]  Old path: %d \"%s\"\n", &path, path);
#	endif /* DEBUG_PATHNAME */

	/* Embedded calls to pathname() will return "path" in "dir" or "file" */
	if (path)
		{
		samedir  = (LOGICAL) (dir  == path);
		samefile = (LOGICAL) (file == path);
		}
	if (!samedir && !samefile) FREEMEM(path);

	/* Return immediately if missing directory or filename */
	if (blank(dir)  && samefile) return file;
	if (blank(file) && samedir)  return dir;
	if (blank(dir))
		{
		path = strdup(file);

#		ifdef DEBUG_PATHNAME
		(void) printf("[pathname]  New path: %d \"%s\"\n", &path, path);
#		endif /* DEBUG_PATHNAME */

		return path;
		}
	if (blank(file))
		{
		path = strdup(dir);

#		ifdef DEBUG_PATHNAME
		(void) printf("[pathname]  New path: %d \"%s\"\n", &path, path);
#		endif /* DEBUG_PATHNAME */

		return path;
		}

	/* Return immediately if file uses an absolute path */
	/*  or a path relative to "." or ".."               */
	if (abspath(file) && samefile) return file;
	if (abspath(file))
		{
		path = strdup(file);

#		ifdef DEBUG_PATHNAME
		(void) printf("[pathname]  New path: %d \"%s\"\n", &path, path);
#		endif /* DEBUG_PATHNAME */

		return path;
		}

	/* Set the directory and filename */
	xdir  = (samedir)  ? strdup(dir)  : dir;
	xfile = (samefile) ? strdup(file) : file;

	/* Prefix the filename with the base directory */
	nd = strlen(xdir);
	nf = strlen(xfile);
	nc = nd + nf + 1;
	path = GETMEM(path, char, nc+1);
	(void) strncpy(path, xdir, nc);
	if (xdir[nd-1] != '/') (void) strncat(path, "/", nc);
	(void) strncat(path, xfile, nc);

	if (samedir)  free(xdir);
	if (samefile) free(xfile);

#	ifdef DEBUG_PATHNAME
	(void) printf("[pathname]  New path: %d \"%s\"\n", &path, path);
#	endif /* DEBUG_PATHNAME */

	return path;
	}

/*********************************************************************/
/** Determine whether the given path is full or relative.
 *
 *	@param[in]	file	file path
 * 	@return
 * 	- TRUE if path is absolute.
 * 	- FALSE if path is relative.
 *********************************************************************/
LOGICAL	abspath

	(
	STRING	file
	)

	{
	if (blank(file))       return  FALSE;

	/* If file uses an absolute path, or a path relative to */
	/* "." or "..", then call it absolute */
	if (same(file, "."))         return TRUE;
	if (same(file, ".."))        return TRUE;
	if (same_start(file, "/"))   return TRUE;
	if (same_start(file, "./"))  return TRUE;
	if (same_start(file, "../")) return TRUE;

	return  FALSE;
	}

/***********************************************************************
*                                                                      *
*     s t r l i s t d u p                                              *
*     f r e e s t r l i s t                                            *
*                                                                      *
*     Duplicate and free list of strings.                              *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Duplicate a list of strings.
 *
 *	@param[in]	nr		number of strings to copy
 *	@param[in]	*list	list of strings to copy
 * 	@return A copy of the given list of strings. New memory is
 * 			allocated so you will need to free it when you are
 * 			finished with it.
 *********************************************************************/
STRING	*strlistdup

	(
	int		nr,
	STRING	*list
	)

	{
	int		i;
	STRING	*copy_list;

	if (nr <= 0) return (STRING *) NULL;
	if (!list)   return (STRING *) NULL;

	copy_list = INITMEM(STRING, nr);
	for (i=0; i<nr; i++)
		{
		copy_list[i] = strdup(list[i]);
		}
	return copy_list;
	}

/*********************************************************************/
/** Free a list of strings.
 *
 *	@param[in]	nr		number of strings to free
 *	@param[in]	*list	list of strings to free
 * 	@return NULL
 *********************************************************************/
STRING	*freelist

	(
	int		nr,
	STRING	*list
	)

	{
	FREELIST(list, nr);
	return (STRING *) NULL;
	}


/************************************************
 *                                              *
 * t i m e _ m a c r o _ s u b s t i t u t e    *
 *                                              *
 ************************************************/
/* Use white space as normal delimiters */
#define WHITE " \t\n\r\f"

static int count, lmax;
static STRING bgnptr;

static void copy_up_to(STRING, STRING);
static void copy_up_to(STRING mbuf, STRING ptr)
	{
	(void) strncat(mbuf, bgnptr, MIN(ptr-bgnptr,lmax-count));
	count = MIN(count + ptr - bgnptr, lmax);
	}


static void tdif(STRING, STRING, long);
static void tdif(STRING mbuf, STRING fmt, long tval)
	{
	char nbuf[32];
	STRING ptr, f;

	bgnptr = fmt;
	while(ptr = strchr(bgnptr, '%'))
		{
		copy_up_to(mbuf, ptr);
		ptr++;
		f = "%d";
		if(isupper(*ptr)) f = "%.2d";
		bgnptr = nbuf;
		switch (*ptr)
			{
			case 'h':
			case 'H':
				(void) sprintf(nbuf, f, tval/3600);
				copy_up_to(mbuf, nbuf + strlen(nbuf));
				ptr++;
				break;
			case 'm':
			case 'M':
				(void) sprintf(nbuf, f, (tval/60)%60);
				copy_up_to(mbuf, nbuf + strlen(nbuf));
				ptr++;
				break;
			case 's':
			case 'S':
				(void) sprintf(nbuf, f, tval%60);
				copy_up_to(mbuf, nbuf + strlen(nbuf));
				ptr++;
				break;
			}
		bgnptr = ptr;
		}
	copy_up_to(mbuf, bgnptr+strlen(bgnptr));
	}


/*********************************************************************/
/** To take a string with embedded time macros and return
 * the string with the macros replaced by the appropriate
 * time type. The recognized macros are:
 *
 * 	- $ISSUE(<...>)  $VALID(<...>)  $DELTA(<fmt>)
 * 	- $issue(<...>)  $valid(<...>)  $delta(<fmt>)
 *
 * 	- $ISSUE returns the formatted issue time.
 * 	- $VALID returns the formatted valid time.
 * 	- $DELTA returns the difference in hours and minutes between the
 * 	  valid and issue times. If minutes is 0, then no minutes are
 * 	  given.
 *
 * ... is a format string as recognized by the strftime
 * function. The entire macro is replaced by the string
 * returned by strftime.
 *
 * fmt is one or more of %h, %m and/or %s where %h is hours
 * %m minutes and %s seconds. Thus %h:%m would return the
 * time difference of 36 hours 6 minutes as 36:6. %H, %M
 * and %S will pad with zeros to two digits. %% will give
 * the character %.
 *
 * The two sets of macros do the same thing, but the macros
 * in upper case will return the date string in upper case
 * and the macros in lower case will return the date string
 * in mixed case.
 *
 *	@param[out] mbuf	formatted string
 *						Note: This function assigns memory for the
 *						returned string. It is the responsibility of
 *						the calling procedure to free this memory.
 *	@param[in]  maxlen	Maximum characters allowed in returned string
 *	@param[in]  fmtstr	String containing format with embedded macros
 *	@param[in]  issue	Issue time as seconds from epoch
 *	@param[in]  valid	Valid time as seconds from epoch
 *********************************************************************/
int time_macro_substitute

	(
	STRING mbuf,
	int maxlen,
	STRING fmtstr,
	long issue,
	long valid
	)

	{
	int ucase, usetdif;
	time_t tval;
	STRING dollar, ptr, endptr, fmt, instr;
	struct tm *tms;

	(void) memset(mbuf, 0, maxlen);
	lmax = maxlen;
	count = 0;
	instr = strdup(fmtstr);					/* copy the input string */
	bgnptr = instr;
	while(dollar = strchr(bgnptr, '$'))		/* look for possible macros */
	{
		ptr = dollar + 1;					/* start after "$" */
		ptr += strspn(ptr,WHITE);			/* strip following space */
		endptr = ptr + 5;					/* jump to end of macro */
		endptr += strspn(endptr,WHITE);		/* strip following white space */
		if(*endptr != '(')					/* do we have beginning bracket */
		{									/* no */
			copy_up_to(mbuf, endptr);		/* copy string to this point */
			bgnptr = endptr;				/* reset pointer for another scan */
			continue;
		}
		if(strncmp(ptr, "VALID", 5) == 0 || strncmp(ptr, "valid", 5) == 0)
		{
			ucase = (strncmp(ptr, "VALID", 5) == 0);
			tval = valid;
			usetdif = FALSE;
		}
		else if(strncmp(ptr,"ISSUE", 5) == 0 || strncmp(ptr,"issue", 5) == 0)
		{
			ucase = (strncmp(ptr, "ISSUE", 5) == 0);
			tval = issue;
			usetdif = FALSE;
		}
		else if(strncmp(ptr,"DELTA", 5) == 0 || strncmp(ptr,"delta", 5) == 0)
		{
			ucase = (strncmp(ptr, "DELTA", 5) == 0);
			tval = valid - issue;
			usetdif = TRUE;
		}
		else								/* unrecognized */
		{
			copy_up_to(mbuf, endptr);		/* copy string to this point */
			bgnptr = endptr;				/* reset for another scan */
			continue;
		}
		copy_up_to(mbuf, dollar);
		ptr = strchr(endptr, ')');			/* find closing bracket */
		if(!ptr) break;						/* if none found give up */
		*ptr = '\0';						/* terminate for strftime */
		fmt = endptr + 1;
		if(usetdif)
		{
			tdif(mbuf, fmt, (long) tval);
			bgnptr = ptr + 1;
		}
		else
		{
			bgnptr = ptr + 1;
			ptr = &mbuf[strlen(mbuf)];
			tms = gmtime(&tval);
			(void)strftime(ptr, maxlen-count, fmt, tms);
		}
		if(ucase) upper_case(ptr);
	}
	(void) strncat(mbuf, bgnptr, maxlen-count);	/* copy any remaining string */
	free(instr);

	return count;
	}

/***********************************************************************
*                                                                      *
*     f p a _ h o s t _ n a m e                                        *
*     f p a _ h o s t _ i d                                            *
*     f p a _ h o s t _ i d _ p c _ i p                                *
*     f p a _ h o s t _ i p _ l i s t                                  *
*                                                                      *
*     Obtain host identification info (some may be O/S dependent).     *
*                                                                      *
***********************************************************************/

#if defined(MACHINE_HP) && defined(PRE_POSIX)
#	include <sys/utsname.h>
#endif
#if defined(MACHINE_SUN) && defined(PRE_POSIX)
#	include <sys/systeminfo.h>
#endif
#if defined(MACHINE_PCLINUX)
#	include <sys/ioctl.h>
#	include <sys/ioctl.h>
#	include <net/if.h>
#	include <arpa/inet.h>
#	include <net/if_arp.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/**********************************************************************/

/*********************************************************************/
/** Obtain name of host running this fpa application.
 *
 * @return Pointer to the host name. The buffer returned is static
 * and owned by the function; if you are not going to use it
 * immediately make a copy of it with safe_copy or safe_strdup.
 *********************************************************************/
STRING	fpa_host_name(void)

	{
	static	char	Name[50] = "";

	if (blank(Name))
		{
		/* Get the hostname for this machine */
		(void) gethostname(Name, sizeof(Name));
		}
	return Name;
	}

/**********************************************************************/

/*********************************************************************/
/** Obtain the host id of host running this fpa application.
 *
 * @return Pointer to the host id. The buffer returned is static
 * and owned by the function; if you are not going to use it
 * immediately make a copy of it.
 *********************************************************************/
UNLONG	fpa_host_id(void)

	{
	static	UNLONG	ID = 0;

	if (ID != 0) return ID;

#	if defined(MACHINE_HP) && defined(PRE_POSIX)
		{
		STRING			p;
		struct	utsname	uts;

		if (uname(&uts) < 0)
			{
			(void) fprintf(stderr, "No machine ID info available!\n");
			return ID;
			}
		ID = strtoul(uts.idnumber, &p, 10);
		if (p == uts.idnumber)
			{
			(void) fprintf(stderr, "Bad machine ID!\n");
			return ID;
			}
		}

#	elif defined(MACHINE_SUN) && defined(PRE_POSIX)
		{
		STRING	p;
		char	sibuf[40];
		long	nc;

		nc = sysinfo(SI_HW_SERIAL, sibuf, 40);
		if (nc<0 && nc>40)
			{
			(void) fprintf(stderr, "No machine ID info available!\n");
			return ID;
			}
		ID = strtoul(sibuf, &p, 10);
		if (p == sibuf)
			{
			(void) fprintf(stderr, "Bad machine ID!\n");
			return ID;
			}
		}

#	elif defined(MACHINE_PCLINUX)

        /* This is a modification of Floyd Davidson's method which has
		   been in the public domain for years. */
		{
		UNCHAR	*mac;
		STRING	p;
		char	eth_name[256], sid[9];
		int		sockfd, size = 1;
		struct	ifreq	*ifr, *eth_ifr;
		struct	ifconf	ifc;

		if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)))
			{
			fprintf(stderr, "Cannot open socket!\n");
			return ID;
			}

		ifc.ifc_len = IFRSIZE;
		ifc.ifc_req = NULL;

		/* Allocate memory for all interfaces. */
		do {
			++size;
			/* realloc buffer size until no overflow occurs  */
			if (NULL == (ifc.ifc_req = realloc(ifc.ifc_req, IFRSIZE)))
				{
				fprintf(stderr, "Out of memory!\n");
				return ID;
				}
			ifc.ifc_len = IFRSIZE;
			if (ioctl(sockfd, SIOCGIFCONF, &ifc))
				{
				perror("ioctl SIOCFIFCONF");
				return ID;
				}
			} while (IFRSIZE <= ifc.ifc_len);

		/* Search for "eth0" interface.
		   If not found, use the first instance of "ethX". */
		ifr = ifc.ifc_req;
		eth_ifr = ifr;
		(void) strcpy(eth_name,"");
		for ( ; (char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr)
			{
			if ( ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data )
				continue;  /* duplicate, skip it */

			if ( ioctl(sockfd, SIOCGIFFLAGS, ifr) )
				continue;  /* failed to get flags, skip it */

			if ( strcmp(ifr->ifr_name,"eth0" ) == 0)
				{
				(void) strcpy(eth_name,ifr->ifr_name);
				eth_ifr = ifr;
				break;     /* found it */
				}
			else if ( strncmp(ifr->ifr_name,"eth",3) == 0 )
				if ( blank(eth_name) )	/* only save the first one */
					{
					(void) strcpy(eth_name,ifr->ifr_name);
					eth_ifr = ifr;
					}
			}
		if ( blank(eth_name) )
			{
			perror( "Appropriate ethernet interface not found!" );
			return ID;
			}

	#	ifdef DEBUG_PC
		(void) printf( "Interface:  %s\n", eth_name );
	#	endif

		/* Read hardware MAC address from interface. */
		if (0 == ioctl(sockfd, SIOCGIFHWADDR, eth_ifr))
			{
			mac = (unsigned char *) &eth_ifr->ifr_addr.sa_data;

			/* Check for non-zero MAC address. */
			if ( mac[0] + mac[1] + mac[2] + mac[3] + mac[4] + mac[5] )
				{
	#			ifdef DEBUG_PC
				(void)
				printf("HW Address: %2.2x.%2.2x.%2.2x.%2.2x.%2.2x.%2.2x\n",
						   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	#			endif
				/* Compute a 32-bit hostid.  Note that the first 3 bytes
				   of the MAC address are a manufacturer's code.  See
				   http://www.cavebear.com/CaveBear/Ethernet/vendor.html
				   for a partial list.  Sun is 08:00:20.  In the PC world,
				   Ethernet cards could come from anywhere.  Since the
				   whole address space is used, there's not much point
				   trying to compress the 3-bytes into one.  The likelihood
				   of duplicating the hostid below within a single
				   organization is extremely small. */
				(void) sprintf( sid, "%2.2x%2.2x%2.2x%2.2x",
						   mac[2], mac[3], mac[4], mac[5]);
				ID = strtoul( sid, &p, 16 );
				if (p == sid)
					{
					(void) fprintf(stderr, "Bad machine ID!\n");
					return ID;
					}
				}
			}

		close( sockfd );
		}

#	else
		{
		long	gethostid();

		ID = gethostid();
		}
#	endif

	return ID;
	}

/**********************************************************************/

/*********************************************************************/
/** Obtain the host id of a pc host running this fpa application.
 *
 * @return Pointer to the host id. The buffer returned is static
 * and owned by the function; if you are not going to use it
 * immediately make a copy of it.
 *********************************************************************/
UNLONG	fpa_host_id_pc_ip(void)

	{
	static	UNLONG	IDpcip = 0;

	if (IDpcip != 0) return IDpcip;

#	if defined(MACHINE_PCLINUX)
		{
		long	gethostid();

		/* PC Linux id based on IP address */
		IDpcip = gethostid();
		}
#	endif

	return IDpcip;
	}

/**********************************************************************/

/*********************************************************************/
/** Fetch a list of ip addresses for this host running this fpa
 * application.
 *
 * @param[out] **iplist list of IP address
 * @return The size of the list
 *********************************************************************/
int		fpa_host_ip_list

	(
	STRING	**iplist
	)

	{
	struct	hostent	*ent;
	int				iaddr;
	STRING			addr;
	STRING			host;

	static	int		IPCount = 0;
	static	STRING	*IPList = NullStringList;

	if (IPCount == 0)
		{

		/* Get the IP address list for this machine */
		host = fpa_host_name();
		ent  = gethostbyname(host);
		if (IsNull(ent))
			{
			if (NotNull(iplist)) *iplist =IPList;
			return IPCount;
			}

		/* Add IP address list entries for this machine */
		for (iaddr=0; ; iaddr++)
			{
			addr = ent->h_addr_list[iaddr];
			if (IsNull(addr)) break;

			IPCount++;
			IPList = GETMEM(IPList, STRING, IPCount);
			IPList[IPCount-1] = strdup(addr);
			}
		}

	if (NotNull(iplist)) *iplist =IPList;
	return IPCount;
	}

#ifdef STANDALONE

int	main(void)

	{
	STRING	name;
	UNLONG	id;
	int		i, nip, ipa, ipb, ipc, ipd;
	STRING	*iplist;

	name = fpa_host_name();
	printf("Host Name: %s\n", name);

	id = fpa_host_id();
	printf("Machine ID: %lu (0x%lx)\n", id, id);

	nip = fpa_host_ip_list(&iplist);
	for (i=0; i<nip; i++)
		{
		ipa = iplist[i][0];	if (ipa < 0) ipa += 256;
		ipb = iplist[i][1];	if (ipb < 0) ipb += 256;
		ipc = iplist[i][2];	if (ipc < 0) ipc += 256;
		ipd = iplist[i][3];	if (ipd < 0) ipd += 256;
		printf("IP Adress: %d.%d.%d.%d\n", ipa, ipb, ipc, ipd);
		}
	}

#endif
