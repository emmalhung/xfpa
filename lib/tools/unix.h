/**********************************************************************/
/** @file unix.h
 *
 *  Assorted unix system calls with practical embellishments.
 *  (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    u n i x . h                                                       *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 6 (c) Copyright 2002 Environment Canada (MSC)            *
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

#include <fpa_types.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

long	fsleep (long sec, long usec);
void	set_stopwatch (LOGICAL reset);
void	get_stopwatch (long *nsec, long *nusec, long *csec, long *cusec);

int		spawn (LOGICAL detach);
int		shrun (STRING pgm, LOGICAL waitforchild);
LOGICAL	running (pid_t pid);
LOGICAL	kill_process (pid_t pid);
LOGICAL	kill_bottom_up (pid_t pid, LOGICAL killtop, UNSIGN nap);

LOGICAL	find_file (STRING path);
LOGICAL	create_file (STRING path, LOGICAL *created);
LOGICAL	remove_file (STRING path, LOGICAL *removed);
LOGICAL	find_directory (STRING path);
LOGICAL	create_directory (STRING path, mode_t mode, LOGICAL *created);
LOGICAL remove_directory (STRING path, LOGICAL *removed);

int		dirlist_reuse (LOGICAL reuse);
int		dirlist (STRING directory, STRING expression, STRING **filelist);

void	fix_env (LOGICAL print);
STRING	env_sub (STRING arg);
int		env_compute (STRING arg, STRING buffer, int size);

STRING	base_name (STRING string, STRING suffix);
STRING	dir_name (STRING string);
STRING	pathname (STRING dir, STRING file);
LOGICAL	abspath (STRING file);

STRING	*strlistdup (int nr, STRING *list);
STRING	*freelist (int nr, STRING *list);

int		time_macro_substitute (STRING mbuf, int maxlen, STRING fmtstr,
			long issue, long valid);

STRING	fpa_host_name (void);
UNLONG	fpa_host_id (void);
UNLONG	fpa_host_id_pc_ip (void);
int		fpa_host_ip_list (STRING **iplist);
