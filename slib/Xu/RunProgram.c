/*=========================================================================*/
/*
 * File:        RunProgram.c
 *
 * Purpose:     Contains functions for running programs and setting up
 *              communication (via pipes) between parent and child.
 *
 * Functions:   XuRunProgram
 *              XuRunSendingProgram
 *              XuRunReceiveProgram
 *              XuRunSendReceiveProgram
 *              XuRunSendingConfig
 *              XuSendToProgram
 *
 *  Overview:   XuRunProgram            - run programs without child-parent
 *                                        communication.
 *              XuRunSendingProgram     - run programs that send data from the
 *                                        child to the parent process.
 *              XuRunReceiveProgram     - runs programs that accept input from
 *                                        the parent process.
 *              XuRunSendReceiveProgram - runs programs that send data to and
 *                                        receive data from the parent process.
 *              XuRunSendingConfig      - change the output file descriptor for
 *                                        the child process to use.
 *              XuSendToProgram         - Transmits a string of data to a
 *                                        process that can receive it.
 *
 *  Descriptions:
 *
 *  >>>> XuRunProgram <<<<
 *  
 *   XuRunProgram(char *pgm, char **args)
 *
 *   where: pgm     - name of the program to run.
 *          args    - a null terminated string array containing the arguments
 *                    to pass to the program.
 *
 *                  
 *  >>>> XuRunSendingProgram <<<<
 *  
 *   XuRunSendingProgram(char *pgm, char **args, void (*callfcn)(), XtPointer data)
 *
 *   where: pgm     - name of the program to run.
 *          args    - a null terminated string array containing the arguments
 *                    to pass to the program.
 *          callfcn - the function to call when pgm terminates.
 *          data    - the data to pass to callfcn
 *
 *   callfcn must have a prototype of:
 *
 *      (void)callfcn()(XtPointer data, XuRUN_RETURN key, char *status)
 *
 *   Where: data: is the data passed in to the XuRunProgram function and is
 *                returned back to callfcn,
 *
 *           key: XuRUN_DATA   - data from program
 *                XuRUN_STATUS - status information from program
 *                XuRUN_ENDED  - program ended normally
 *                XuRUN_ERROR  - program terminated abnormally
 *
 *        status: is a string of information sent by the program and is program
 *                specific. The status string must be copied if the information
 *                needs to be retained.
 *
 *  This function dups a copy of its write pipe file descriptor as file
 *  descriptor number 3 in the exec of the program to run although this can be
 *  overridden by the XuRunConfig() function (see below). This program
 *  can communicate with this function by writing to this descriptor.
 *  The writes must be terminated by a new-line character '\n' as separate
 *  write statements can end up being read in one read depending on the current
 *  processing environment. The only recoginzed key words are:
 *
 *      END    - The called program is terminating.
 *      EXIT   - The same as END
 *      ERROR  - The called program terminated with an error.
 *      STATUS - The information following is status information.
 *
 *  Note: the above keywords ARE case sensitive.
 *
 *  STATUS may be combined with ERROR, EXIT and END into one line. An example,
 *  for abnormal termination:
 *
 *      ERROR STATUS No config
 *
 *  If none of the key words is recognized, then the received information is
 *  passed back in unaltered form as type XuRUN_DATA. Thus the program receiving
 *  the callfcn() call should check the return key and not make assumptions as
 *  to the returned data type (a program error could return a type of XuRUN_DATA
 *  when this is not a valid return).
 *
 *  This function automatically adds as a last parameter to the program call
 *  the item statfd=3. If the program wishes to use the status return ability
 *  then it should look for "statfd=" and use the appropriate file number or
 *  if a shell it should enumerate the variable and use >&$statfd as the
 *  output file descriptor. For example a shell would:
 *
 *      echo "ERROR STATUS No config" 1>&$statfd
 *
 *  It is possible to override this: see XuRunConfig()
 *
 *  If the procedure to be run does not send back the above information then
 *  when this function determines that the called program is no longer running
 *  and callfcn() is defined, it will call the function with the values:
 *
 *    callfcn( data, XuRUN_ENDED, NULL );
 *
 *
 *  >>>> XuRunSendingConfig <<<<
 *
 *  void XuRunSendingConfig( int fd, Boolean send_statfd )
 *
 *  Override the settings only for the next call to XuRunSendingProgram or
 *  XuRunSendReceiveProgram. The parameters are:
 *
 *      fd: The file descriptor that is used for the child program to use
 *          to communicate with the parent program. Must be one of 1, 2 or
 *          3. This is useful if the child program writes to say stderr and
 *          one does not want to make code changes.
 *
 *     send: Add the "statfd=x" key to the program parameters where x is the
 *           file number. Either True or False.
 *
 *  Please note again that a call to this function is effective only for the
 *  next call to the XuRun<>Program function which resets the values back to
 *  the default when it is finished.
 *
 *  >>>> XuRunReceiveProgram <<<<
 *
 *  int XuRunProgram(char *pgm, char **args)
 *
 *  This function is exactly like the XuRunProgram function for parameters,
 *  but it opens up a pipe to send data to the run program. The pipe is
 *  opened up for stdin.
 *
 *  Of course, the run program must be able to read from stdin. The function
 *  below must be used to send messages.
 *
 *
 *  >>>> XuRunSendReceiveProgram <<<<
 *
 *  int XuRunSendReceiveProgram(char *pgm, char **args, void (*callfcn)(), XtPointer data)
 *
 *  This function is exactly like the XuRunSendingProgram function for parameters,
 *  but it opens up pipes for two way communication with the run program. By default
 *  the pipes are opened up for stdin and stdout (NOT pfd 3) of the run program
 *  and "statfd=x" is not added to the run parameter list. This may be overridden
 *  by using XuRunSendingConfig().
 *
 *  Of course, the run program must be able to read from stdin. The function
 *  below must be used to send messages.
 *
 *  >>>> XuSendToProgram <<<<
 *
 *  Boolean XuSendToProgram( int key, char *buf )
 *
 *  This function sends data to a program run with XuRunProgramSR which is
 *  identified by the returned key. Returns True if the data send was successful
 *  or False if it was not. The usual reason for a failure is that the program
 *  is no longer running and must be started again.
 *
 *  This is should used in combination of XuRunProgramSR. For example:
 *
 *      static int key = NoId;
 *
 *      if (!XuSendToProgram(key, data))
 *      {
 *          .... parameter setup ....
 *
 *          key = XuRunReceiveProgram(pgm, ....);  [or XuRunSendReceiveProgram]
 *      }
 *
 *  -------------------------------------------------------
 *
 *     Version 8 (c) Copyright 2011 Environment Canada
 *
 *   This file is part of the Forecast Production Assistant (FPA).
 *   The FPA is free software: you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *   The FPA is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *   See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
 */
/*=========================================================================*/

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <values.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "XuP.h"

#define INTERVAL	60000	/* process list check repeat time */
#define MAXARG		50		/* max arguments allowed in parameter list */
#define NDX_START	21		/* where to start the unique send key */
#define READ_LEN	256		/* number of chars to read at a time */
#define TOKEN_LEN	32		/* size of token buffer */


typedef struct {
	Boolean    valid;	/* entry contains valid program info */
	Boolean    locked;	/* entry is not available for use even if not valid */
	Boolean    wait;	/* entry is in the process of being destroyed */
	int        ndx;		/* unique identification index */
	pid_t      pid;		/* process identification number */
	int        pfdr;	/* file descriptor to use for reading */
	int        pfdw;	/* file descriptor to use for writing */
	char       *buf;	/* data buffer */
	int        lbuf;	/* length of data buffer */
	XtInputId  sid;		/* socket listen identifier */
	void       (*fcn)(XtPointer, XuRUN_RETURN, char*);	/* user function to call */
	XtPointer  data;	/* user data to pass back in the fcn call */
} PROGITEM;


static int      next_ndx     = NDX_START;	/* unique send to receiving program index number */
static int      npl          = 0;			/* number of progitems */
static PROGITEM *pl          = 0;			/* progitem list */
static Boolean  out_override = False;		/* is the output override in effect ? */
static int      out_fd       = 3;			/* file number to use for output */
static Boolean  out_send_fd  = True;		/* send the "statfd=x" key */



/* Find a new program entry slot
 */
static int create_entry(void)
{
	int n;

	for( n = 0; n < npl; n++ )
	{
		if( pl[n].valid ) continue;
		if( pl[n].wait  ) continue;
		if(!pl[n].locked) break;
	}

	if( n < npl )	/* entry available for reuse */
	{
		if (pl[n].buf) free((void*)pl[n].buf);
		(void)memset((void*)&pl[n], 0, sizeof(PROGITEM));
	}
	else			/* add new entry set */
	{
		npl += 15;
		pl = (PROGITEM *)XtRealloc((void*)pl, npl * sizeof(PROGITEM));
		(void)memset((void*)&pl[n], 0, 15 * sizeof(PROGITEM));
	}

	pl[n].valid = True;
	pl[n].ndx   = NoId;

	return (n);
}


/* To check existance we first do a waitpid() on the process to reap any
 * defunct processes, then issue a kill() command to see if the process
 * still exists. If defunct the waitpid() sould have taken care of this
 * and the kill should indicate no such program existing.
 */
static Boolean prog_exists( int nx )
{
	if (!pl[nx].pid) return False;
	(void) waitpid(pl[nx].pid, 0, WNOHANG);
	return (kill(pl[nx].pid, 0) == 0);
}



/* Used by destroy_entry() to do a wait on a single process to avoid
 * zombie creation. The timeout is increased with each loop until we
 * decide that something is wrong and issue a kill order for the process.
 * Once the kill is issued we do one more loop to ensure that the kill
 * was processed before complaining.
 */
/*ARGSUSED*/
static void check_entry( XtPointer cd, XtIntervalId *id )
{
	int nx = PTR2INT(cd);

	if(prog_exists(nx))
	{
		if(pl[nx].ndx < 10)
		{
			if(pl[nx].ndx > 8)
			{
				(void) kill(pl[nx].pid, SIGKILL);
			}
			pl[nx].ndx++;
			(void) XtAppAddTimeOut(Fxu.app_context, (unsigned long)(pl[nx].ndx*500), check_entry, cd);
			return;
		}
		else
		{
			(void)fprintf(stderr,"XuRunProgram: Unable to kill and clean up process %d\n", pl[nx].pid);
		}
	}
	pl[nx].wait = False;
}


/* removes the given prog entry from the internal list and closes
 * the parent connections. The process must be dead already.
 */
static void remove_entry( int nx )
{
	if (pl[nx].sid ) XtRemoveInput(pl[nx].sid);
	if (pl[nx].pfdw) (void) close(pl[nx].pfdw);
	if (pl[nx].pfdr) (void) close(pl[nx].pfdr);

	pl[nx].valid = False;
	pl[nx].sid   = 0;
	pl[nx].pfdw  = 0;
	pl[nx].pfdr  = 0;
}


/* removes the given prog entry from the internal list, closes
 * the parent connections, but does not assume that the process
 * is already dead.
 */
static void destroy_entry( int nx )
{
	remove_entry(nx);
	pl[nx].wait = True;
	pl[nx].ndx  = 0;
	check_entry(INT2PTR(nx), 0);
}


/* Check the pid list to see if the programs flagged as running are
 * still actually running. 
*/
static void check_entries(XtPointer client_data, XtIntervalId *id )
{
	static XtIntervalId lid = 0;

	/* id is only defined if we are called by the process timer */
	if(id)
	{
		int n;

		/* Make sure we reap all defunct processes. Although the
		 * prog_exists function does a wait, we do a general one
		 * here in the unlikely event that something is missed.
		 */
		(void) waitpid((pid_t)-1, 0, WNOHANG|WUNTRACED);
		lid = 0;

		for(n = 0; n < npl; n++ )
		{
			if(!pl[n].valid ) continue;
			if( pl[n].wait  ) continue;
			if( pl[n].locked) continue;

			/* check to see if the process is still running */
			if( prog_exists(n) )
			{
				if (!lid) lid = XtAppAddTimeOut(Fxu.app_context, INTERVAL, check_entries, 0);
			}
			else
			{
				/* remove_entry must be called before calling pl[n].fcn
				 * to avoid an endless loop situation.
				 */
				remove_entry(n);
				if (pl[n].fcn) pl[n].fcn(pl[n].data, XuRUN_ENDED, 0);
			}
		}
	}
	else
	{
		unsigned long interval = (client_data) ? (unsigned long)client_data : INTERVAL;
		if (lid) XtRemoveTimeOut(lid);
		lid = XtAppAddTimeOut(Fxu.app_context, interval, check_entries, 0);
	}
} 


/* Return blank or tab separated tokens from the given string.
 */
static Boolean get_token( char *buf, char **next, char *token )
{
	size_t  n;
	char *b, *e;

	/* find beginning of the token */
	if (*next)
		b = *next;
	else
		b = buf + strspn(buf," \t");

	if( b == NULL || *b == '\0') return False;

	/* find end of token */
	e = b + strcspn(b, " \t\0");

	/* copy into our buffer */
	(void) memset((void*)token, 0, TOKEN_LEN);
	n = (size_t)(e - b);
	if( n >= TOKEN_LEN ) n = TOKEN_LEN - 1;
	(void) strncpy(token, b, n);

	/* return start position of next token */
	*next = e + strspn(e," \t");
	
	return True;
}


/* Called by select input when the pipe we created is written to.
*/
/*ARGSUSED*/
static void read_input(XtPointer client_data , int *src , XtInputId *id )
{
	size_t nread;
	char   *ptr, *buf;
	int    count = READ_LEN;
	int    nx    = PTR2INT(client_data);

	/* We don't want this item to recycled while we are using it
	 * even if it is released. This could happen as we need to call
	 * the destroy before we finish processing input.
	 */
	pl[nx].locked = True;

	/* The buffer we create must be stored with the program data as there
	 * is no assurance that the user function will not be in some process
	 * loop that will create multiple instances of this function.
	 */
	if(!pl[nx].buf)
	{
		pl[nx].lbuf = READ_LEN;
		pl[nx].buf  = malloc(READ_LEN);
	}
	(void) memset((void*)pl[nx].buf, 0, (size_t)pl[nx].lbuf);

	/* If the read is -1, then we have a zero read from a running
	 * program.  If 0, we have a dead program.
	 */
	nread = (size_t) read(*src, pl[nx].buf, READ_LEN);
	if( nread < 1 )
	{
		if (!nread) destroy_entry(nx);
		pl[nx].locked = False;
		return;
	}

	/* Read the pipe until it is empty and extend the buffer
	 * as necessary.
	 */
	while(nread == READ_LEN)
	{
		if( pl[nx].lbuf <= count )
		{
			pl[nx].lbuf += READ_LEN;
			pl[nx].buf = realloc(pl[nx].buf, (size_t)pl[nx].lbuf);
			(void)memset((void *)(pl[nx].buf + count), 0, READ_LEN);
		}
		nread = (size_t) read(*src, pl[nx].buf + count, READ_LEN);
		count += READ_LEN;
	}

	/* Adding a line feed to the end simplifies the parsing logic
	 * if the child program does not put a line feed at the end
	 * as it is supposed to.
	 */
	(void) safe_strcat(pl[nx].buf,"\n");

	/* The loop is in case we receive multiple messages in one read.
	 * This can happen even if the writes were separate in the program.
	 * Note that it is important that destroy_entry() be called before
	 * the pl[nx].fcn call. If the child program terminates, calling
	 * pl[nx].fcn before doing an XtRemoveInput results in a endless
	 * loop as Xt will put the zero read resulting from program
	 * termination back in the queue.
	 */
	buf = pl[nx].buf;
	while((ptr = strchr(buf, '\n')))
	{
		char         *status = NULL, *next = NULL;
		char         key[TOKEN_LEN];
		XuRUN_RETURN rtn;

		*ptr = '\0';
		if(!get_token(buf, &next, key)) break;

		if(!strcmp(key,"STATUS"))
		{
			rtn    = XuRUN_STATUS;
			status = next;
		}
		else if(!strcmp(key,"ERROR"))
		{
			rtn = XuRUN_ERROR;
			if(get_token(buf,&next,key) && !strcmp(key,"STATUS")) status = next;
			destroy_entry(nx);
		}
		else if(!strcmp(key,"END") || !strcmp(key,"EXIT"))
		{
			rtn = XuRUN_ENDED;
			if(get_token(buf,&next,key) && !strcmp(key,"STATUS")) status = next;
			destroy_entry(nx);
		}
		else
		{
			rtn    = XuRUN_DATA;
			status = buf;
		}

		if (pl[nx].fcn) pl[nx].fcn(pl[nx].data, rtn, status);

		/* increment to where beginning of next line should be */
		buf = ptr + 1;
	}
	pl[nx].locked = False;
}


/* fd and send are the default values to use for the output file descriptor and
 * the send flag when the override is not in effect. 
 */
static void copy_args(char **newargs, char **args, char *pgm, int fd, const Boolean send)
{
	char **ac = newargs + MAXARG - 2;
	*newargs++ = pgm;
	while(newargs < ac && *args != (char*)0) *newargs++ = *args++;

	/* The output file descriptor must be valid to proceed */
	if(fd)
	{
		if(!out_override)
		{
			out_fd      = fd;
			out_send_fd = send;
		}

		if (out_send_fd)
		{
			switch(out_fd)
			{
				case 1:  *newargs++ = "statfd=1"; break;
				case 2:  *newargs++ = "statfd=2"; break;
				case 3:  *newargs++ = "statfd=3"; break;
			}
		}
	}
	*newargs++ = (char*)0;
	out_override = False;
}


/* close all display connections and set the SIG_IGN as appropriate
 */
static void close_connections(void)
{
	int n;
	for(n = 0; n < Fxu.ndd; n++)
	{
		if (Fxu.dd[n]->display) (void) close(ConnectionNumber(Fxu.dd[n]->display));
	}
	(void)signal(SIGINT,  SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGHUP,  SIG_IGN);
	(void)signal(SIGCHLD, SIG_IGN);
}


static Boolean make_pipe( char *module, int pfd[2] )
{
	if(pipe(pfd) == -1)
	{
		char *msg = "Unknown";
		(void)fprintf(stderr,"%s: pipe() function call failure.\n", module);
		switch(errno)
		{
			case EMFILE:
				msg = "Concurrent open file descriptor limit exceeded.";
				break;
			case ENFILE:
				msg = "The system file table is full.";
				break;
			case ENOSPC:
				msg = "The file system lacks sufficient space to create the pipe.";
				break;
			case ENOSR:
				msg = "Could not allocate resources for both Stream heads";
				break;
		}
		(void)fprintf(stderr, "Reason: %s\n", msg);
		return False;
	}
	return True;
}


static void fork_fail_message(char *module, char *pgm, int eno)
{
	perror(pgm);
	(void)fprintf(stderr,"%s: Fork failed for program %s\n", module, pgm);
	switch(eno)
	{
		case EAGAIN:
			(void)fprintf(stderr, "Reason: The system-imposed limit on the total number of\n");
            (void)fprintf(stderr, "        processes under execution would be exceeded\n");
			(void)fprintf(stderr, "              -or-\n");
			(void)fprintf(stderr, "        The system-imposed limit on the total number of processes\n");
			(void)fprintf(stderr, "        under execution by a single user would be exceeded.\n");
			break;
		case ENOMEM:
			(void)fprintf(stderr, "Reason: There is insufficient swap space and/or physical memory\n");
			(void)fprintf(stderr, "        available in which to create the new process.\n");
			break;
		default:
			(void)fprintf(stderr, "Reason: Unknown\n");
			break;
	}
}


/*=============== PUBLIC FUNCTIONS ==================*/


void XuRunSendingConfig( int fd, Boolean send )
{
	if( fd > 0 && fd < 4 )
	{
		out_override = True;
		out_fd       = fd;
		out_send_fd  = send;
	}
	else
	{
		(void)fprintf(stderr,"XuRunSendinfConfig: file descriptor = %d and is out of range.\n", fd);
	}
}


Boolean XuRunProgram(char *pgm, char **args)
{
	int         nx;
	char        *newargs[MAXARG];
	static char *module = "XuRunProgram";

	nx = create_entry();
	copy_args(newargs, args, pgm, 0, False);

	switch(pl[nx].pid = fork())
	{
		case 0: /* child */
			(void) close_connections();
			(void) execvp(pgm,newargs);
			perror(pgm);
			exit(255);

		case -1:
			remove_entry(nx);
			fork_fail_message(module, pgm, errno);
			return False;
	}

	check_entries(NULL, NULL);
	return True;
}



Boolean XuRunSendingProgram(char *pgm, char **args, void (*callfcn)(), XtPointer rtn_data)
{
	int          nx, pfdr[2];
	char         *newargs[MAXARG];
	Boolean      run_ok = True;
	static char  *module = "XuRunSendingProgram";

	if(!make_pipe(module, pfdr)) return False;

	/* Set no block on receiving end of the transmission pipe */
	(void) fcntl(pfdr[0], F_SETFL, O_NONBLOCK);

	nx = create_entry();

	pl[nx].fcn  = callfcn;
	pl[nx].data = rtn_data;
	pl[nx].pfdr = pfdr[0];
	pl[nx].sid  = XtAppAddInput(Fxu.app_context,pfdr[0],(XtPointer)XtInputReadMask,read_input,INT2PTR(nx));

	copy_args(newargs, args, pgm, 3, True);

	switch(pl[nx].pid = fork())
	{
		case 0: /* child */
			(void) close_connections();
			(void) close(out_fd);
			if(dup(pfdr[1]) != out_fd)
			{
				perror("dup");
				exit(1);
			}
			(void) close(pfdr[0]);
			(void) close(pfdr[1]);
			(void) execvp(pgm,newargs);
			perror(pgm);
			exit(255);

		case -1:
			remove_entry(nx);
			fork_fail_message(module, pgm, errno);
			run_ok = False;
	}
	(void) close(pfdr[1]);

	check_entries(NULL, NULL);
	return run_ok;
}


int XuRunReceiveProgram(char *pgm, char **args)
{
	int           nx, pfdw[2];
	char          *newargs[MAXARG];
	Boolean       run_ok = True;
	static char   *module = "XuRunReceiveProgram";

	if(!make_pipe(module, pfdw)) return 0;

	nx = create_entry();

	pl[nx].ndx  = next_ndx++;
	pl[nx].pfdw = pfdw[1];

	copy_args(newargs, args, pgm, 0, False);

	switch(pl[nx].pid = fork())
	{
		case 0: /* child */
			(void) close_connections();
			(void) close(0);
			if(dup(pfdw[0]) != 0)
			{
				perror("dup");
				exit(1);
			}
			(void) close(pfdw[0]);
			(void) close(pfdw[1]);
			(void) execvp(pgm,newargs);
			perror(pgm);
			exit(255);

		case -1:
			run_ok = False;
			remove_entry(nx);
			fork_fail_message(module, pgm, errno);
	}
	(void) close(pfdw[0]);
	check_entries(NULL, NULL);
	return ((run_ok)? pl[nx].ndx : 0);
}


int XuRunSendReceiveProgram(char *pgm, char **args, void (*callfcn)(), XtPointer rtn_data)
{
	int           nx, pfdw[2] = {0,0}, pfdr[2] = {0,0};
	char          *newargs[MAXARG];
	Boolean       run_ok = True;
	static char   *module = "XuRunSendReceiveProgram";

	if(!make_pipe(module, pfdw)) return 0;
	if(!make_pipe(module, pfdr))
	{
		(void) close(pfdw[0]);
		(void) close(pfdw[1]);
		return 0;
	}
	/* Set no block on receiving end of the transmission pipe */
	(void)fcntl(pfdr[0], F_SETFL, O_NONBLOCK);

	nx = create_entry();

	pl[nx].ndx  = next_ndx++;
	pl[nx].fcn  = callfcn;
	pl[nx].data = rtn_data;
	pl[nx].pfdw = pfdw[1];
	pl[nx].pfdr = pfdr[0];
	pl[nx].sid  = XtAppAddInput(Fxu.app_context,pfdr[0],(XtPointer)XtInputReadMask,read_input,INT2PTR(nx));

	copy_args(newargs, args, pgm, 1, False);

	switch(pl[nx].pid = fork())
	{
		case 0: /* child */
			(void) close_connections();
			(void) close(0);
			if(dup(pfdw[0]) != 0)
			{
				perror("dup");
				exit(1);
			}
			(void) close(out_fd);
			if(dup(pfdr[1]) != out_fd)
			{
				perror("dup");
				exit(1);
			}
			(void) close(pfdr[0]);
			(void) close(pfdr[1]);
			(void) close(pfdw[0]);
			(void) close(pfdw[1]);
			(void) execvp(pgm,newargs);
			perror(pgm);
			exit(255);

		case -1:
			run_ok = False;
			remove_entry(nx);
			fork_fail_message(module, pgm, errno);
	}

	(void) close(pfdw[0]);
	(void) close(pfdr[1]);
	check_entries(NULL, NULL);
	return ((run_ok)? pl[nx].ndx : 0);
}


Boolean XuSendToProgram( int ndx, char *buf )
{
	if( ndx >= NDX_START )
	{
		int n;
		for( n = 0; n < npl; n++ )
		{
			if(!pl[n].valid     ) continue;
			if( pl[n].ndx != ndx) continue;

			if(prog_exists(n))
			{
				if(pl[n].pfdw) (void) write(pl[n].pfdw, buf, safe_strlen(buf)+1);
				return True;
			}
			else
			{
				remove_entry(n);
			}
		}
	}
	return False;
}
