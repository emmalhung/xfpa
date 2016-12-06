/***********************************************************************
*                                                                      *
*    i p c . h                                                         *
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

/* Control functions */
int		server_control(STRING, STRING, STRING, LOGICAL, LOGICAL);
int		server(STRING, LOGICAL);
int		find_channel(void);

/* Client functions */
int		connect_server(STRING);
int		disconnect_server(int);
int		shutdown_server(int);
int		query_server(int, int, STRING, int *, STRING *, int);

/* Server functions */
int		connect_client(STRING);
int		disconnect_client(int);
int		shutdown_channel(int);
int		pending_client(int);
int		receive_client(int, int *, STRING *);
int		respond_client(int, int, STRING);
int		notify_client(int);
