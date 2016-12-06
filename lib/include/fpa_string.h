/***********************************************************************
*                                                                      *
*     f p a _ s t r i n g . h                                          *
*                                                                      *
*     Natural extensions to the UNIX string library and string.h       *
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

/* See if already included */
#ifndef FPA_STRING

#	include <string.h>

/* Convenient in-line functions */
#	ifndef bcopy
#		define bcopy(s, d, n)	memcpy((void *)d, (const void *)s, (size_t)n)
#	endif
#	ifndef bzero
#		define bzero(s, n)	memset((void *)s, 0, (size_t)n)
#	endif
#	ifndef index
#		define index(s, c)	strchr(s, c)
#	endif
#	ifndef rindex
#		define rindex(s, c)	strrchr(s, c)
#	endif

/* Now it has been included */
#	define FPA_STRING
#endif
