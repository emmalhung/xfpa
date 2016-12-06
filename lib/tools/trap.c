/**********************************************************************/
/** @file trap.c
 *
 *  Routines to set error trap and handle errors (include file)
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*  t r a p . c                                                         *
*                                                                      *
*  Functions to assist in trapping terminal errors.                    *
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

#include "trap.h"

#include <fpa_types.h>

#include <signal.h>
#include <stdio.h>

/*******************************************************************************
*                                                                              *
*  s e t _ e r r o r _ t r a p                                                 *
*  s e t _ t e r m _ t r a p                                                   *
*                                                                              *
*******************************************************************************/

void	set_error_trap(void (*action)())

	{
	/* Trap all signals that would abort the process by default */
	(void) signal(SIGHUP,    action);
	(void) signal(SIGINT,    action);
	(void) signal(SIGQUIT,   action);
	(void) signal(SIGILL,    action);
	(void) signal(SIGTRAP,   action);
	(void) signal(SIGABRT,   action);
	(void) signal(SIGIOT,    action);
#ifdef SIGEMT
	(void) signal(SIGEMT,    action);
#endif
	(void) signal(SIGFPE,    action);
	(void) signal(SIGBUS,    action);
	(void) signal(SIGSEGV,   action);
#ifdef SIGSYS
	(void) signal(SIGSYS,    action);
#endif
	(void) signal(SIGPIPE,   action);
	(void) signal(SIGALRM,   action);
	(void) signal(SIGTERM,   action);
	(void) signal(SIGUSR1,   action);
	(void) signal(SIGUSR2,   action);
	(void) signal(SIGVTALRM, action);
	(void) signal(SIGPROF,   action);
	}

/******************************************************************************/

void	set_term_trap(void (*action)())

	{
	/* Trap all external signals that would abort the process by default */
	(void) signal(SIGHUP,  action);
	(void) signal(SIGINT,  action);
	(void) signal(SIGQUIT, action);
	(void) signal(SIGPIPE, action);
	(void) signal(SIGTERM, action);
	}

/*******************************************************************************
*                                                                              *
*  s e t _ n u m _ t r a p                                                     *
*  u n s e t _ n u m _ t r a p                                                 *
*                                                                              *
*******************************************************************************/

static	void	(*oldiot)()  = SIG_DFL;
#ifdef SIGEMT
static	void	(*oldemt)()  = SIG_DFL;
#endif
static	void	(*oldfpe)()  = SIG_DFL;
static	void	(*oldbus)()  = SIG_DFL;
static	void	(*oldsegv)() = SIG_DFL;

void	set_num_trap(void (*action)())

	{
	/* Trap all numerical signals that would abort the process by default */
	oldiot  = signal(SIGIOT,  action);
#ifdef SIGEMT
	oldemt  = signal(SIGEMT,  action);
#endif
	oldfpe  = signal(SIGFPE,  action);
	oldbus  = signal(SIGBUS,  action);
	oldsegv = signal(SIGSEGV, action);
	}

/******************************************************************************/

void	unset_num_trap(void)

	{
	/* Reset traps on numerical signals to their original states */
	(void) signal(SIGIOT,  oldiot);
#ifdef SIGEMT
	(void) signal(SIGEMT,  oldemt);
#endif
	(void) signal(SIGFPE,  oldfpe);
	(void) signal(SIGBUS,  oldbus);
	(void) signal(SIGSEGV, oldsegv);
	}

/*******************************************************************************
*                                                                              *
*  s i g n a l _ n a m e                                                       *
*                                                                              *
*******************************************************************************/

STRING	signal_name(int sig)

	{
	static	char	sname[20] = "";

	/* See if we know the signal name */
	switch (sig)
		{
		case SIGHUP:	return "SIGHUP";
		case SIGINT:	return "SIGINT";
		case SIGQUIT:	return "SIGQUIT";
		case SIGILL:	return "SIGILL";
		case SIGTRAP:	return "SIGTRAP";
		case SIGIOT:	return "SIGIOT";
#ifdef SIGEMT
		case SIGEMT:	return "SIGEMT";
#endif
		case SIGFPE:	return "SIGFPE";
		case SIGBUS:	return "SIGBUS";
		case SIGSEGV:	return "SIGSEGV";
#ifdef SIGSYS
		case SIGSYS:	return "SIGSYS";
#endif
		case SIGPIPE:	return "SIGPIPE";
		case SIGALRM:	return "SIGALRM";
		case SIGTERM:	return "SIGTERM";
		case SIGUSR1:	return "SIGUSR1";
		case SIGUSR2:	return "SIGUSR2";
		case SIGCHLD:	return "SIGCHLD";
		case SIGPWR:	return "SIGPWR";
		case SIGVTALRM:	return "SIGVTALRM";
		case SIGPROF:	return "SIGPROF";
		case SIGIO:		return "SIGIO";
		case SIGSTOP:	return "SIGSTOP";
		case SIGTSTP:	return "SIGTSTP";
		case SIGCONT:	return "SIGCONT";
		case SIGTTIN:	return "SIGTTIN";
		case SIGTTOU:	return "SIGTTOU";
		case SIGURG:	return "SIGURG";
		default:		(void) sprintf(sname, "Signal #%d", sig);
						return sname;
		}
	}
