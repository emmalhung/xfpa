/** @file	fpa_getmem.h */
/***********************************************************************
*                                                                      *
*     f p a _ g e t m e m . h                                          *
*                                                                      *
*     Handy macros for easy memory allocation:                         *
*                                                                      *
*     SIZE(number,type) - computes number of bytes needed by "number"  *
*                         items of type "type".                        *
*                                                                      *
*     INITMEM(type,number) - allocates space for "number" items of     *
*                            type "type" and sets var to point to the  *
*                            allocated space.                          *
*                                                                      *
*     MOREMEM(var,type,number) - changes the amount of space that var  *
*                                already has allocated to it.          *
*                                                                      *
*     GETMEM(var,type,number) - decides which of the two preceding     *
*                               macros to use.                         *
*                                                                      *
*     FREEMEM(var) - frees space that was allocated to var.            *
*                                                                      *
*     FREELIST(var,number) - frees space that was allocated to var     *
*                            as well as its members.                   *
*                                                                      *
*     INITSTR(string) - allocates space for a copy of string and       *
*                       makes the copy.                                *
*                                                                      *
*     STRMEM(string1,string2) - copies string2 to string1 and makes    *
*                               sure string1 is long enough.           *
*                                                                      *
*     SETSTR(string1,string2) - same as STRMEM except that string2     *
*                               must be constant and guaranteed to     *
*                               not be NULL.                           *
*                                                                      *
*     WARNING:                                                         *
*        These macros use (type *) as a cast.  This causes problems    *
*        for some complex types (e.g. float [5], should be cast as     *
*        (float (*)[5]) but GETMEM tries (float [5] *), syntax error). *
*        When you are dealing with complex types of this sort use a    *
*        typedef declaration.  The above example would be done with:   *
*                                                                      *
*            typedef float5 float[5];                                  *
*                    .                                                 *
*                    .                                                 *
*                    .                                                 *
*            GETMEM(x,float5,x);                                       *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* See if already included */
#ifndef GETMEM_MACROS

/* Functions in getmem_ext.c */
void	memreset(void);
void	memstart(void);
void	memstop(void);
size_t	memaddm(size_t);
size_t	memaddr(size_t);
size_t	memgetm(void);
size_t	memgetr(void);

/***********************************************************************
*                                                                      *
*     SIZE(number,type) - computes number of bytes needed by "number"  *
*                         items of type "type".                        *
*                                                                      *
***********************************************************************/

#	define SIZE(NUMBER,TYPE) ( (size_t) ((NUMBER)*sizeof(TYPE)) )

/***********************************************************************
*                                                                      *
*     INITMEM(type,number) - allocates space for "number" items of     *
*                            type "type" and sets var to point to the  *
*                            allocated space.                          *
*                                                                      *
***********************************************************************/
/**********************************************************************/
/** Allocate space for a number of items of a particular type.
 *
 * 	@param[in]	TYPE	Type of variable
 * 	@param[in]	NUMBER	Number of items to allocate for
 * 	@return Pointer to the allocated memory or NullPtr.
 **********************************************************************/
#		define INITMEM(TYPE,NUMBER) \
		  (TYPE *) malloc(memaddm(SIZE(NUMBER,TYPE)))

/***********************************************************************
*                                                                      *
*     MOREMEM(var,type,number) - changes the amount of space that var  *
*                                already has allocated to it.          *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Reallocate space for a variable.
 *
 *  @param[in]	VAR		Pointer to existing space
 * 	@param[in]	TYPE	Type of variable
 * 	@param[in]	NUMBER	New size
 * 	@return Pointer to the allocated memory or NullPtr.
 **********************************************************************/
#		define MOREMEM(VAR,TYPE,NUMBER) \
 		  (TYPE *) realloc(((void *) (VAR)),memaddr(SIZE(NUMBER,TYPE)))

/***********************************************************************
*                                                                      *
*     GETMEM(var,type,number) - decides which of the two preceding     *
*                               macros to use.                         *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Allocates memory if VAR is Null or Reallocates memory if not Null.
 *
 *  @param[in]	VAR		Pointer 
 * 	@param[in]	TYPE	Type of variable
 * 	@param[in]	NUMBER	size
 * 	@return Pointer to the allocated memory or NullPtr.
 **********************************************************************/
#	define GETMEM(VAR,TYPE,NUMBER) \
	   (!(VAR)) ? ( INITMEM(TYPE,NUMBER) ) : ( MOREMEM(VAR,TYPE,NUMBER) )

/***********************************************************************
*                                                                      *
*     FREEMEM(var) - frees space that was allocated to var.            *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Free space that was allocated to var.
 *
 *  @param[in]	VAR		Pointer 
 **********************************************************************/
#	define FREEMEM(VAR) \
		{ \
		if (VAR) \
			{ \
			free((void *) (VAR)); \
			(VAR) = NULL; \
			} \
		}

/***********************************************************************
*                                                                      *
*     FREELIST(var,number) - frees space that was allocated to var     *
*                            as well as its members.                   *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Free space that was allocated to var as well as it's members.
 *
 *  @param[in]	VAR		Pointer 
 *  @param[in]  NUMBER  Number of members to free
 **********************************************************************/
#	define FREELIST(VAR,NUMBER) \
		{ \
		if (VAR && (NUMBER>0)) \
			{ \
			int	IFREE; \
			for (IFREE=0; IFREE<NUMBER; IFREE++) \
				FREEMEM((VAR)[IFREE]); \
			} \
		FREEMEM(VAR); \
		}

/***********************************************************************
*                                                                      *
*     INITSTR(string) - allocates space for a copy of string and       *
*                       makes the copy.                                *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Allocates space for a copy of string and makes the copy.
 *
 *  @param[in]	S		String to copy 
 *  @return		Copy of string
 **********************************************************************/
#	define INITSTR(S) \
	   (!(S)) ? NULL : ( strcpy( INITMEM(char,strlen(S)+1) , S) )

/***********************************************************************
*                                                                      *
*     STRMEM(string1,string2) - copies string2 to string1 and makes    *
*                               sure string1 is long enough.           *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Copies string2 to string1 and makes sure string1 is long enough.
 *
 *  @param[in]	S1		String1
 *  @param[in]	S2		String2
 **********************************************************************/
#	define STRMEM(S1,S2) \
	   (!(S2)) ? ( strcpy( GETMEM(S1,char,1)            , "") ) \
		   : ( strcpy( GETMEM(S1,char,strlen(S2)+1) , S2) )

/***********************************************************************
*                                                                      *
*     SETSTR(string1,string2) - like STRMEM except that string2 is not *
*                               tested for existance and therefore     *
*     must be a constant string like "value". Created to remove the    *
*     compile time complaines from the pgi compiler due to a test of   *
*     S2 when S2 is a string constant like "value".                    *
*                                                                      *
***********************************************************************/

/**********************************************************************/
/** Copies constant string2 (which must be guaranteed to not be NULL)
 *  to string1 and makes sure string1 is long enough.
 *
 *  @param[in]	S1		String1
 *  @param[in]	S2		String2
 **********************************************************************/
#	define SETSTR(S1,S2) \
		   strcpy( GETMEM(S1,char,strlen(S2)+1) , S2)


/* Now it has been included */
#define GETMEM_MACROS
#endif
