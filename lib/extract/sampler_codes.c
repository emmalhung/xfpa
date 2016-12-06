/***********************************************************************
*                                                                      *
*     s a m p l e r _ c o d e s . c                                    *
*                                                                      *
*     Routines to translate between pre-defined codes supported by     *
*     the sampler ipc library, and the corresponding IPC message type  *
*     codes.                                                           *
*                                                                      *
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

#include "sampler.h"

/***********************************************************************
*                                                                      *
*     S A M P Q u e r y C o d e                                        *
*     S A M P Q u e r y                                                *
*     S A M P R e p l y C o d e                                        *
*     S A M P R e p l y                                                *
*                                                                      *
*     Convert between SAMPQUERY or SAMPREPLY values and integer IPC    *
*     message type codes.                                              *
*                                                                      *
***********************************************************************/

const	int	SAMP_MSG_OFFSET = 1;

int			SAMPQueryCode

	(
	SAMPQUERY	query
	)

	{
	int	code;

	code = (int) query;
	if (code < 0) return -1;
	else          return code + SAMP_MSG_OFFSET;
	}

SAMPQUERY	SAMPQuery

	(
	int			code
	)

	{
	code -= SAMP_MSG_OFFSET;
	if (code < 0) return (SAMPQUERY) -1;
	else          return (SAMPQUERY) code;
	}

int			SAMPReplyCode

	(
	SAMPREPLY	reply
	)

	{
	return SAMPQueryCode((SAMPQUERY) reply);
	}

SAMPREPLY	SAMPReply

	(
	int			code
	)

	{
	return (SAMPREPLY) SAMPQuery(code);
	}
