/***********************************************************************
*                                                                      *
*       f p a _ t y p e s . h                                          *
*                                                                      *
*       assorted low level type definitions (include file)             *
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
#ifndef TYPE_DEFS

#	include <string.h>

/* Generic pointer */	typedef	void			*POINTER;
/* STRING object */		typedef	char			*STRING;
/* Unsigned */			typedef	unsigned		UNSIGN;
/* Unsigned char */		typedef	unsigned char	UNCHAR;
/* Unsigned short */	typedef	unsigned short	UNSHORT;
/* Unsigned long */		typedef	unsigned long	UNLONG;
/* Generic handle */	typedef	int				HANDLE;

/* Convenient definitions for typecast Null pointers */
#	define	NullPtr(type)	((type) 0)
#	define	NullChar		NullPtr(char *)
#	define	NullShort		NullPtr(short *)
#	define	NullInt			NullPtr(int *)
#	define	NullLong		NullPtr(long *)
#	define	NullEnum		(0)
#	define	NullFloat		NullPtr(float *)
#	define	NullDouble		NullPtr(double *)
#	define	NullPointer		NullPtr(POINTER)
#	define	NullPointerList	NullPtr(POINTER *)
#	define	NullString		NullPtr(STRING)
#	define	NullStringPtr	NullPtr(STRING *)
#	define	NullStringList	NullPtr(STRING *)

/* Boolean logical */
	typedef	char			LOGICAL;
#	define	LogicalF		((LOGICAL) 0)
#	define	LogicalT		((LOGICAL) 1)
#	ifndef TRUE
#		define	TRUE		LogicalT
#		define	FALSE		LogicalF
#	endif
#	ifndef True
#		define	True		LogicalT
#		define	False		LogicalF
#	endif
#	ifndef Not
#		define	Not(x)		((x)? LogicalF: LogicalT)
#	endif
#	ifndef LogicalAgree
#		define	LogicalAgree(a, b)	((LOGICAL) ( (a&&b) || (Not(a)&&Not(b)) ))
#		define	LogicalDiffer(a, b)	((LOGICAL) ( (a&&Not(b)) || (Not(a)&&b) ))
#	endif
#	define	NullLogical 	NullPtr(char *)
#	define	NullLogicalPtr 	NullPtr(LOGICAL *)
#	define	NullLogicalList	NullPtr(LOGICAL *)

/* Handedness */		typedef	enum { Left='l', Right='r', Ambi='a' } HAND;


/* Define "TABLE" structure: (index;value) pair */
typedef struct
	{
	STRING	index;
	STRING	value;
	} TABLE;


/* Define "TABLE2" structure: (index;value1;value2) triplet */
typedef struct
	{
	STRING	index;
	STRING	value1;
	STRING	value2;
	} TABLE2;


/* Define "FPA_MACRO_LIST" structure: (macro;label) pair */
typedef struct
	{
	int		macro;
	STRING	label;
	} FPA_MACRO_LIST;


/* Define "FPA_FUNCTION_LIST" structure: (function_pointer;label) pair */
typedef struct
	{
	POINTER	function;
	STRING	label;
	} FPA_FUNCTION_LIST;


/* Now it has been included */
#define TYPE_DEFS
#endif
