/*********************************************************************/
/** @file lock.c
 *
 * Routines to provide file locking capability.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l o c k . c                                                       *
*                                                                      *
*    Routines to provide file locking capability.                      *
*                                                                      *
*    NOTE: Better file locking may be available with fcntl(2) or       *
*          lockf(2).                                                   *
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

#include "lock.h"
#include "parse.h"

#include <fpa_types.h>

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

/***********************************************************************
*                                                                      *
*    f s l o c k   - lock some resource using a file as a semaphore.   *
*    f s u n l k   - unlock a resource that was locked with fslock.    *
*    f s t e s t   - test if a resource has been locked with fslock.   *
*    f s w a i t   - wait until a resource is unlocked with fsunlk.    *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Lock some resource using a file as a semaphore.
 *
 *	@param[in]	file	Name of the file to lock
 *	@param[in]	stime	Time between attempts
 *	@param[in]	tries	How many attempts should we make?
 * 	@return
 * 	-  0 :- success: lock has been aquired
 * 	- -1 :- failure: already locked to other process (too many attempts)
 * 	- -2 :- failure: bad filename used
 *********************************************************************/
int fslock

	(
	STRING	file,
	int		stime,
	int		tries
	)

	{
	int		fd;

	if (blank(file)) return -2;
	if (tries <= 0) tries = 1;
	if (stime <= 0) tries = 1;

	/* Keep trying for a lock */
	while (tries--)
		{
		/* Try to create the lock file */
		fd = open(file,O_WRONLY | O_CREAT | O_EXCL,0666);
		if (fd >= 0)
			{
			/* Success - Close the created file */
			(void) close(fd);
			return 0;
			}
		if (errno != EEXIST)
			{
			/* >>>>> for testing <<<<< */
			(void) fprintf(stderr,
					"[fslock] File: %s  Unknown problem!\n", file);
			/* >>>>> for testing <<<<< */
			return -2;
			}

		/* File already exists - Try again */
		/* >>>>> for testing <<<<< */
		(void) fprintf(stderr,
				"[fslock] File: %s  Tries left: %d\n", file, tries);
		/* >>>>> for testing <<<<< */
		(void) sleep((UNSIGN) stime);
		}

	/* Failure - Too many tries */
	return -1;
	}

/*********************************************************************/
/** Unlock a resource that was locked with fslock.
 *
 *	@param[in]	file	file to unlock
 * 	@return
 * 	-  0 :- success: lock has been released
 * 	- -1 :- failure: wasn't locked
 * 	- -2 :- failure: bad filename used
 *********************************************************************/
int	fsunlk

	(
	STRING	file
	)

	{
	int	status;

	if (blank(file)) return -2;

	status = unlink(file);
	if (status >= 0)     return 0;
	if (errno == ENOENT) return -1;
	return -2;
	}

/*********************************************************************/
/** Test if a resource has been locked with fslock.
 *
 *	@param[in]	file	file to test
 * 	@return
 * 	-  0 :- success: lock not present (currently)
 * 	- -1 :- failure: already locked
 * 	- -2 :- failure: bad filename used
 *********************************************************************/
int	fstest

	(
	STRING	file
	)

	{
	int			status;
	struct stat	buf;

	if (blank(file)) return -2;

	status = stat(file,&buf);
	if (status >= 0)     return -1;
	if (errno == ENOENT) return 0;
	return -2;
	}

/*********************************************************************/
/** Wait until a resource is unlocked with fsunlk.
 *
 *	@param[in]	file	File to test
 *	@param[in]	stime	Time between attempts
 *	@param[in]	tries	How many attempts should we make?
 * 	@return
 * 	-  0 :- success: lock has been released
 * 	- -1 :- failure: lock not released
 * 	- -2 :- failure: bad filename used
 *********************************************************************/
int fswait

	(
	STRING	file,
	int		stime,
	int		tries
	)

	{
	int		status;

	if (blank(file)) return -2;
	if (tries <= 0) tries = 1;
	if (stime <= 0) tries = 1;

	/* Keep checking for existence of lock file */
	while (tries--)
		{
		status = fstest(file);
		if      (status >=  0) return 0;
		else if (status == -1)
			{
			/* >>>>> for testing <<<<< */
			(void) fprintf(stderr,
					"[fswait] File: %s  Tries left: %d  Status: %d\n",
					file, tries, status);
			/* >>>>> for testing <<<<< */
			(void) sleep((UNSIGN) stime);
			continue;
			}
		else if (status <= -2)
			{
			/* >>>>> for testing <<<<< */
			(void) fprintf(stderr,
					"[fswait] File: %s  Status: %d\n", file, status);
			/* >>>>> for testing <<<<< */
			return -2;
			}
		}

	/* Failure - Too many tries */
	return -1;
	}
