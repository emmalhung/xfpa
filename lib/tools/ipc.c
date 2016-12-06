/***********************************************************************
*                                                                      *
*    i p c . c                                                         *
*                                                                      *
*        Routines to handle Inter-Process Communication via System-V   *
*        Message System calls                                          *
*                                                                      *
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

#undef PRINT_STATUS
#undef PRINT_ERRORS

#include "ipc.h"
#include "parse.h"
#include "unix.h"

#include <fpa_getmem.h>
#include <fpa_types.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
extern	int	errno;

/* Structure to keep track of communications channel between a client */
/* and a server */
typedef	struct
	{
	key_t	key;	/* channel name key */
	int		query;	/* query queue-id */
	int		reply;	/* reply queue-id */
	}	CHANNEL;

/* Internal functions */
static	int		ipc_create(STRING, CHANNEL *);
static	int		ipc_get(STRING, CHANNEL *);
static	int		ipc_destroy(CHANNEL *);
static	int		ipc_pending(int);
static	int		ipc_send(int, int, STRING);
static	int		ipc_receive(int, int, int *, STRING *);

/***********************************************************************
*                                                                      *
*    s e r v e r _ c o n t r o l                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Start up or shut down the server.
 *
 *	@param[in]	pname	program name (arbitrary)
 *	@param[in]	mode	"startup" or "shutdown" mode
 *	@param[in]	chname	3-letter channel identifire
 *	@param[in]	detach	OK to detach from terminal?
 *	@param[in]	debug	OK to print messages?
 *	@return	+1 :- success: (startup)  server already active
 *                         (shutdown) server shut down
 *         	 0 :- success: I am now the server
 *         	-1 :- failure: (startup)  failed to start up
 *                         (shutdown) failed to shut down
 *         	-2 :- failure: unknown mode
 **********************************************************************/

int		server_control

(
    STRING	pname,
	STRING	mode,
	STRING	chname,
	LOGICAL	detach,
	LOGICAL	debug
	)

	{
	int		status, chid;

#	ifdef PRINT_STATUS
	(void) printf("[Server_Control] Called by process %d\n",getpid());
	(void) fflush(stdout);
#	endif /* PRINT_STATUS */

	/* "Startup" mode */
	if ( same(mode,"startup") )
		{
		status = server(chname,detach);
		if (status == -1)
			{
			if (debug) (void) printf("%s already active\n",pname);
			return 1;
			}
		if (status < 0)
			{
			if (debug) (void) printf("%s startup failed (%d)\n",pname,status);
			return -1;
			}
		if (debug) (void) printf("%s started up\n",pname);
		return 0;
		}

	/* "Shutdown" mode */
	else if ( same(mode,"shutdown") )
		{
		chid   = connect_server(chname);
		status = shutdown_server(chid);
		if (status < 0)
			{
			if (debug) (void) printf("%s shutdown failed (%d)\n",pname,status);
			return -1;
			}
		if (status == 1)
			{
			if (debug) (void) printf("%s not responding - cleaned up\n",pname);
			return 1;
			}
		if (debug) (void) printf("%s shut down\n",pname);
		return 1;
		}

	/* Otherwise - unknown mode */
	if (debug)
		{
		(void) printf("Unknown mode: \"%s\"\n\n",mode);
		(void) printf("Useage:\n");
		(void) printf("   %s startup\n",pname);
		(void) printf("   %s shutdown\n",pname);
		(void) fflush(stdout);
		}
	return -2;
	}

/***********************************************************************
*                                                                      *
*    s e r v e r                                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Become a server process, with an active communications channel.
 *
 *    The server communications (listen) channel is created.
 *    Then the current process is "fork"ed, thereby producing a
 *    detached child.  The child then disconnects its terminal
 *    affiliation and becomes the server.  The Parent hangs around
 *    long enough to make sure it worked.
 *
 *    On success, the child (server) returns and the parent is
 *    terminated.  The real server may be subsequently "exec"ed
 *    if required.
 *
 *    On failure, the parent returns and the child (if created)
 *    is aborted.
 *
 *    ONLY the parent OR the child will return - NEVER both.
 *
 *	@param[in]	chname	 	3-letter channel identifier
 *	@param[in]	detach  	OK to detach from terminal?
 *	@return	0 :- success: I am now the server
 *         -1 :- failure: server already active
 *         -2 :- failure: cannot create channel
 *         -3 :- failure: fork failed
 ***********************************************************************/

int		server

	(
	STRING	chname,
	LOGICAL	detach
	)

	{
	int		status;
	CHANNEL	channel;

	/* Make sure desired communications channel is not in use */
	status = ipc_get(chname,&channel);
	if (status >= 0)
		{
#		ifdef PRINT_ERRORS
		(void) fprintf(stderr,"[server] server already active\n");
#		endif
		return -1;
		}

	/* Create communications channel */
	status = ipc_create(chname,&channel);
	if (status < 0)
		{
#		ifdef PRINT_ERRORS
		(void) fprintf(stderr,"[server] cannot set up communications channel\n");
		(void) fprintf(stderr,"[server] server not started\n");
#		endif
		return -2;
		}

	/* Spawn a child to become the server */
	status = spawn(detach);
	if (status < 0)
		{
#		ifdef PRINT_STATUS
		perror("[server] failed to spawn server process");
#		endif
		return -3;
		}

	/* Success - Return 0 */
#	ifdef PRINT_STATUS
	(void) printf("[server] server ready\n");
#	endif
	return 0;
	}

/***********************************************************************
*                                                                      *
*    f i n d _ c h a n n e l                                           *
*                                                                      *
***********************************************************************/

#define MAXCHAN 5
static	CHANNEL channels[MAXCHAN] = { 0 };

/**********************************************************************/
/** Find next available communications channel
 *
 *    @return >= 0 :- success: channel-id
 *              -1 :- failure: no free channels
 **********************************************************************/
int		find_channel(void)

	{
	int		chid;
	CHANNEL	*channel;

	/* Find next available communications channel */
	for (chid=0; chid<MAXCHAN; chid++)
		{
		channel = &channels[chid];
		if (channel->key > 0) continue;
		return chid;
		}

	return -1;
	}

/***********************************************************************
*                                                                      *
*    CLIENT ROUTINES:                                                  *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*    c o n n e c t _ s e r v e r                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Obtain the communications channel for the given server.
 *
 *	@param[in]	chname  3-letter channel identifier
 *	@return	>= 0 :- success: channel-id
 *         	  -1 :- failure: channel not present
 ***********************************************************************/

int		connect_server

	(
	STRING	chname
	)

	{
	int		status, chid;
	CHANNEL	*channel;

	/* Find next available channel */
	chid    = find_channel();
	channel = &channels[chid];

	/* Obtain access to server communications channel */
	status = ipc_get(chname,channel);
	if (status < 0) return -1;

	/* Success */
	return chid;
	}

/***********************************************************************
*                                                                      *
*    d i s c o n n e c t _ s e r v e r                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Free up the communications channel for the given server.
 *
 *	@param[in]	chid  channel-id
 *	@return	 0 :- success:
 *       	-1 :- failure: invalid channel-id
 **********************************************************************/

int		disconnect_server

	(
	int		chid
	)

	{
	CHANNEL	*channel;

	/* Make sure channel-id is valid */
	if (chid < 0)        return -1;
	if (chid >= MAXCHAN) return -1;
	channel = &channels[chid];

	/* Free up current channel */
	channel->key   = 0;
	channel->query = 0;
	channel->reply = 0;
	return 0;
	}

/***********************************************************************
*                                                                      *
*    s h u t d o w n _ s e r v e r                                     *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Send the server a "shutdown" request, and destroy its
 *    communications channel.
 *
 *	@param[in]	chid 	channel-id
 *	@return	+1 :- success: server not responding - cleaned up
 *         	 0 :- success:
 *          -1 :- failure: invalid
 *          -2 :- failure: server not responding
 *          -3 :- failure: cannot destroy channel
 *
 ***********************************************************************/

int		shutdown_server

(
 int		chid
)

{
	CHANNEL	*channel;
	int		status, rtype, success;
	STRING	reply;

	/* Make sure channel-id is valid */
	if (chid < 0)        return -1;
	if (chid >= MAXCHAN) return -1;
	channel = &channels[chid];

	/* See if channel is already free */
	success = 0;
	if (channel->key <= 0) return success;

	/* Send "shutdown" message on query queue */
	status = query_server(chid,99,"shutdown",&rtype,&reply,5);
	if (status == -3)
		{
		success = 1;
#		ifdef PRINT_STATUS
		(void) printf("[shutdown_server] server not responding\n");
		(void) printf("[shutdown_server] shutting down channel\n");
#		endif
		/* Destroy the communications channel */
		status = ipc_destroy(channel);
		if (status < 0) return -3;
		}
	else if (status < 0) return -2;

	return success;
	}

/***********************************************************************
*                                                                      *
*    q u e r y _ s e r v e r                                           *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Submit a query request to the server associated with the
 *    given communications channel-id and await the reply.
 *
 *  @param[in]	chid 		 channel-id of server communications channel
 *	@param[in]	qtype 		 query message type
 *	@param[in]	qtext 		 query message
 *	@param[out]	*rtype  	 reply message type
 *	@param[out]	*rtext  	 reply message
 *	@param[in]	timeout 	 timeout in seconds
 *  @return 0 :- success
 *         -1 :- failure: invalid channel-id
 *         -2 :- failure: communications channel lost
 *         -3 :- failure: timeout
 *         -4 :- failure: server aborted (with notification)
 *
 **********************************************************************/

int		query_server

(
	int		chid,
	int		qtype,
	STRING	qtext,
	int		*rtype,
	STRING	*rtext,
	int		timeout
	)

	{
	int		status;
	CHANNEL	*channel;

	/* Make sure channel-id is valid and in use */
	if (chid < 0)          return -1;
	if (chid >= MAXCHAN)   return -1;
	channel = &channels[chid];
	if (channel->key <= 0) return -1;

	/* Make sure return parameters are available */
	if (!rtype) return -1;
	if (!rtext) return -1;

	/* Send query message on query queue */
	status = ipc_send(channel->query,qtype,qtext);
	if (status < 0) return -2;

	/* Receive reply message on reply queue */
	status = ipc_receive(channel->reply,timeout,rtype,rtext);
	if (status == -2) return -3;
	if (status < 0)   return -2;

	/* Handle priority messages */
	if (*rtype == 99)
		{
		if (same(*rtext,"aborted"))
			{
			return -4;
			}
		}

	return 0;
	}

/***********************************************************************
*                                                                      *
*    SERVER ROUTINES:                                                  *
*                                                                      *
***********************************************************************/

/***********************************************************************
*                                                                      *
*    c o n n e c t _ c l i e n t                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Obtain the communications channel for a client process.
 *
 *	@param[in]	chname 	3-letter channel identifier
 *	@return   >= 0 :- success: channel-id
 *         		-1 :- failure: channel not present
 **********************************************************************/

int		connect_client

	(
	STRING	chname
	)

	{
	return connect_server(chname);
	}

/***********************************************************************
*                                                                      *
*    d i s c o n n e c t _ c l i e n t                                 *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Free up the communications channel for the given client.
 *
 *	@param[in]	chid	channel-id
 *	@return	0 :- success:
 *         -1 :- failure: invalid channel-id
 *
 **********************************************************************/

int		disconnect_client

	(
	int		chid
	)

	{
	return disconnect_server(chid);
	}

/***********************************************************************
*                                                                      *
*    s h u t d o w n _ c h a n n e l                                   *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Destroy the communications channel.
 *
 *	@param[in]	chid	channel-id
 *	@return	0 :- success:
 *         -1 :- failure: invalid channel-id
 *         -2 :- failure: cannot destroy channel
 *
 **********************************************************************/

int		shutdown_channel

	(
	int		chid
	)

	{
	CHANNEL	*channel;
	int		status;

	/* Make sure channel-id is valid */
	if (chid < 0)        return -1;
	if (chid >= MAXCHAN) return -1;
	channel = &channels[chid];

	/* See if channel is already free */
	if (channel->key <= 0) return 0;

	/* Destroy the communications channel */
	status = ipc_destroy(channel);
	if (status < 0) return -2;

	return 0;
	}

/***********************************************************************
*                                                                      *
*    p e n d i n g _ c l i e n t                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Test whether a query/request from the client is waiting.
 *
 *	@param[in]	chid	channel-id
 *	@return	+1 :- success: request waiting
 *           0 :- success: nothing waiting
 *          -1 :- failure: channel disconnected
 **********************************************************************/

int		pending_client

	(
	int		chid
	)

	{
	CHANNEL	*channel;

	/* Make sure channel-id is valid and in use */
	if (chid < 0)          return -1;
	if (chid >= MAXCHAN)   return -1;
	channel = &channels[chid];
	if (channel->key <= 0) return -1;

	/* Now test the channel */
	return ipc_pending(channel->query);
	}

/***********************************************************************
*                                                                      *
*    r e c e i v e _ c l i e n t                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Await the next query/request from the client process.
 *
 *	@param[in]		chid	channel-id
 *	@param[out]		*type	message type
 *	@param[out]		*text   message
 *    @return +1 :- success: shutdown request
 *             0 :- success
 *            -1 :- failure: channel disconnected
 *            -2 :- failure: read timed out
 **********************************************************************/

int		receive_client

	(
	int		chid,
	int		*type,
	STRING	*text
	)

	{
	CHANNEL	*channel;
	int		status;
	char	reply[21];

	/* Make sure channel-id is valid and in use */
	if (chid < 0)          return -1;
	if (chid >= MAXCHAN)   return -1;
	channel = &channels[chid];
	if (channel->key <= 0) return -1;

	/* Make sure return parameters are available */
	if (!type) return -1;
	if (!text) return -1;

more:	status = ipc_receive(channel->query,0,type,text);
	if (status < 0) return status;

	/* Handle priority messages */
	if (*type == 99)
		{
		if (same(*text,"shutdown"))
			{
			status = ipc_send(channel->reply,99,"OK");
			(void) sleep(1);
			status = ipc_destroy(channel);
			return 1;
			}
		if (same(*text,"identify"))
			{
			(void) sprintf(reply,"%d",getpid());
			status = ipc_send(channel->reply,99,reply);
			goto more;
			}
		}

	return status;
	}

/***********************************************************************
*                                                                      *
*    r e s p o n d _ c l i e n t                                       *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Send back a response to the client process.
 *
 *	@param[in]	chid	channel-id
 *	@param[in]	type	message type
 *	@param[in]	text	message
 *	@return	0 :- success
 *         -1 :- failure: channel disconnected
 **********************************************************************/

int		respond_client

	(
	int		chid,
	int		type,
	STRING	text
	)

	{
	CHANNEL	*channel;

	/* Make sure channel-id is valid and in use */
	if (chid < 0)          return -1;
	if (chid >= MAXCHAN)   return -1;
	channel = &channels[chid];
	if (channel->key <= 0) return -1;

	return ipc_send(channel->reply,type,text);
	}

/***********************************************************************
*                                                                      *
*    n o t i f y _ c l i e n t                                         *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Send regrets to the client process if server has aborted
 *    or has better things to do.
 *
 *	@param[in]	chid 	channel-id
 *	@return	0 :- success
 *         -1 :- failure: channel disconnected
 **********************************************************************/

int		notify_client

	(
	int		chid
	)

	{
	return respond_client(chid,99,"aborted");
	}

/***********************************************************************
*                                                                      *
*    Low-Level Routines:                                               *
*                                                                      *
*    i p c _ c r e a t e     - Create 2-way communications channel     *
*    i p c _ g e t           - Get communications channel structure    *
*    i p c _ d e s t r o y   - Destroy communications channel          *
*    i p c _ s e n d         - Send a message                          *
*    i p c _ r e c e i v e   - Receive a message                       *
*                                                                      *
*        Low level routines to set up inter-process communications.    *
*                                                                      *
*    Arguments:  chname :- 3-letter channel identifier                 *
*               channel :- channel structure for communications        *
*                 queue :- message queue (query or reply queue)        *
*                  type :- message type                                *
*                  text :- message                                     *
*                  tmax :- maximum size of message                     *
*                                                                      *
*    Returns:         0 :- success                                     *
*                    -1 :- failure                                     *
*                                                                      *
***********************************************************************/

#undef PRINT_STATUS
#undef PRINT_ERRORS

/* Structure to hold message */
#define MSIZE 40
typedef	struct
	{
	long	type;
	char	text[MSIZE];
	}	MSG;



static	int		ipc_create

	(
	STRING	chname,
	CHANNEL	*channel
	)

	{
	key_t	key, qkey, rkey;
	int		qid, rid;
	int		perm = 0666 | IPC_CREAT;

	/* Make sure information is valid */
	if (blank(chname)) return -1;
	if (strlen(chname) > 3) return 1;
	if (!channel) return -1;

	/* Construct channel key */
	key = (chname[0] << 24)
		+ (chname[1] << 16)
		+ (chname[2] << 8);

	/* Create the query queue */
	qkey = key + 'Q';
	qid  = msgget(qkey,perm);
	if (qid < 0)
		{
#		ifdef PRINT_ERRORS
		perror("[ipc_create] query queue not created");
#		endif
		return -1;
		}
#	ifdef PRINT_STATUS
	(void) printf("[ipc_create] query queue created: %sQ/%d\n",chname,qid);
#	endif

	/* Create the reply queue */
	rkey = key + 'R';
	rid  = msgget(rkey,perm);
	if (rid < 0)
		{
#		ifdef PRINT_ERRORS
		perror("[ipc_create] reply queue not created");
#		endif
		(void) msgctl(qid,IPC_RMID,(struct msqid_ds *)0);
		return -1;
		}
#	ifdef PRINT_STATUS
	(void) printf("[ipc_create] reply queue created: %sR/%d\n",chname,rid);
#	endif

	channel->key   = key;
	channel->query = qid;
	channel->reply = rid;
	return 0;
	}



static	int		ipc_get

	(
	STRING	chname,
	CHANNEL	*channel
	)

	{
	key_t	key, qkey, rkey;
	int		qid, rid;
	int		perm = 0666;

	/* Make sure information is valid */
	if (blank(chname)) return -1;
	if (strlen(chname) > 3) return 1;
	if (!channel) return -1;

	/* Construct channel key */
	key = (chname[0] << 24)
		+ (chname[1] << 16)
		+ (chname[2] << 8);

	/* Obtain the query queue if it exists */
	qkey = key + 'Q';
	qid  = msgget(qkey,perm);
	if (qid < 0)
		{
#		ifdef PRINT_ERRORS
		perror("[ipc_get] query queue not found");
#		endif
		return -1;
		}
#	ifdef PRINT_STATUS
	(void) printf("[ipc_get] query queue opened: %sQ/%d\n",chname,qid);
#	endif

	/* Obtain the reply queue if it exists */
	rkey = key + 'R';
	rid  = msgget(rkey,perm);
	if (rid < 0)
		{
#		ifdef PRINT_ERRORS
		perror("[ipc_get] reply queue not found");
#		endif
		return -1;
		}
#	ifdef PRINT_STATUS
	(void) printf("[ipc_get] reply queue opened: %sR/%d\n",chname,rid);
#	endif

	channel->key   = key;
	channel->query = qid;
	channel->reply = rid;
	return 0;
	}



static	int		ipc_destroy

	(
	CHANNEL	*channel
	)

	{
	/* See if channel is in use */
	if (!channel) return 0;
	if (channel->key <= 0) return 0;

	/* Remove the query queue */
	(void) msgctl(channel->query,IPC_RMID,(struct msqid_ds *)0);
#	ifdef PRINT_STATUS
	(void) printf("[ipc_destroy] query queue removed: %d/%d\n",
		channel->key,channel->query);
#	endif

	/* Remove the reply queue */
	(void) msgctl(channel->reply,IPC_RMID,(struct msqid_ds *)0);
#	ifdef PRINT_STATUS
	(void) printf("[ipc_destroy] reply queue removed: %d/%d\n",
		channel->key,channel->reply);
#	endif

	channel->key   = 0;
	channel->query = 0;
	channel->reply = 0;
	return 0;
	}



static	int		ipc_pending

	(
	int		queue
	)

	{
	int		status;
	struct	msqid_ds	sbuf;

	/* Make sure queue is valid */
	if (queue <= 0) return -1;

	/* Retrieve queue status buffer */
	status = msgctl(queue,IPC_STAT,&sbuf);
	if (status != 0)
		{
		return -1;
		}

	/* Check number of messages in queue */
/*	(void) printf("-------\n");
	(void) printf("qnum:   %d\n",sbuf.msg_qnum);
	(void) printf("qbytes: %d\n",sbuf.msg_qbytes);
	(void) printf("lspid:  %d\n",sbuf.msg_lspid);
	(void) printf("lrpid:  %d\n",sbuf.msg_lrpid);
	(void) printf("stime:  %d\n",sbuf.msg_stime);
	(void) printf("rtime:  %d\n",sbuf.msg_rtime);
	(void) printf("ctime:  %d\n",sbuf.msg_ctime);
	(void) printf("-------\n");
	(void) fflush(stdout);
*/
	return (int) (sbuf.msg_qnum > 0);
	}



static	int		ipc_send

	(
	int		queue,
	int		type,
	STRING	text
	)

	{
	MSG		msg;
	int		status, tsize, psize;

	/* Make sure queue is valid */
	if (queue <= 0) return -1;

	/* Determine number of bytes to send including terminating NULL */
	tsize = strlen(text) + 1;

	/* Send in packets of MSIZE bytes until none left */
	/* Message type 98 means "another packet to follow" */
	/* hence a response will not be expected */
	while (tsize > 0)
		{
		/* Set up the message structure to be sent */
		if (tsize > MSIZE)
			{
			msg.type = 98;
			psize    = MSIZE;
			}
		else	{
			msg.type = type;
			psize    = tsize;
			}
		(void) strncpy(msg.text,text,psize);

		/* Send the packet */
		status = msgsnd(queue,&msg,psize,0);
		if (status < 0)
			{
#			ifdef PRINT_ERRORS
			perror("[ipc_send] send failed");
#			endif
			return 1;
			}

		/* Advance to next packet */
		tsize -= MSIZE;
		text  += MSIZE;
		}

	return 0;
	}



static	void	trapfcn(int);
static	void	trapfcn(int unused)
		{ (void) fprintf(stderr,"IPC Receive Timeout\n"); }
static	STRING	tbuf = 0;
static	int		tsize = 0;

static	int		ipc_receive

	(
	int		queue,
	int		timeout,
	int		*type,
	STRING	*text
	)

	{
	MSG		msg;
	int		osize, psize;
	void	(*prevfcn)(int);

	/* Clean up text buffer */
	FREEMEM(tbuf);
	tsize = 0;

	/* Make sure queue is valid */
	if (queue <= 0) return -1;
	if (!type) return -1;
	if (!text) return -1;

	/* Receive in packets of MSIZE bytes until none left */
	/* Message type 98 means "another packet to follow" */
	/* so hang on until the next one arrives */
	do	{
		/* Receive the next packet - Use alarm to catch timeout */
		psize   = -1;
		prevfcn = signal(SIGALRM,trapfcn);
		(void) alarm(timeout);
		psize   = msgrcv(queue,&msg,MSIZE,0,0);
		prevfcn = signal(SIGALRM,prevfcn);
		(void) alarm(0);
		if (psize < 0)
			{
#			ifdef PRINT_ERRORS
			perror("[ipc_receive] receive failed");
#			endif
			if (errno == EINTR) return -2;
			return -1;
			}

		/* Deposit into text buffer and make sure it is */
		/* NULL terminated */
		if (psize > 0)
			{
			osize  = tsize;
			tsize += psize;
			tbuf   = GETMEM(tbuf,char,tsize+1);
			(void) strncpy(tbuf+osize,msg.text,psize);
			tbuf[tsize] = '\0';
			}

		} while (msg.type == 98);

	*text = tbuf;
	*type = msg.type;
	return 0;
	}

#ifdef LATER
/***********************************************************************
*                                                                      *
*    s e n d _ s i g n a l                                             *
*                                                                      *
*        Routines to send the given signal to the named process.       *
*        The signal will not be sent to the calling process.           *
*                                                                      *
***********************************************************************/

int		send_signal

	(
	int		sig,
	STRING	pname,
	LOGICAL	debug
	)

	{
	int		mypid, pid;
	LOGICAL	status;
	FILE	*fd, *popen();
	char	line[128];
	STRING	owner;

	if (blank(pname)) return -1;
	if (sig <= 0)     return -1;

	if (debug) (void) printf("[send_signal] Sending signal %d to '%s'\n",sig,pname);
	mypid = getpid();

	/* Do a 'ps -ef' with a grep to find the named process(es) */
	(void) sprintf(line,"ps -ef | grep -v grep | grep '%s'",pname);
	fd = popen(line,"r");
	if (!fd) return -1;

	/* Read back the response to determine what processes to signal */
	while (getfileline(fd,line,128))
		{
		if (debug) (void) printf("[send_signal] %s\n",line);

		owner = string_arg(line);
		if (!owner) continue;
		if (debug) (void) printf("[send_signal] Owner '%s'\n",owner);

		pid = int_arg(line,&status);
		if (!status) continue;
		if (debug) (void) printf("[send_signal] Process %d\n",pid);
		if (pid == mypid) continue;

		if (debug) (void) printf("[send_signal] Signalling %d\n",pid);
		kill(pid,sig);
		}

	pclose(fd);
	if (debug) (void) printf("[send_signal] Done\n");
	return 0;
	}
#endif /* LATER */
