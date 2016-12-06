/***********************************************************************
*                                                                      *
*     f p a _ m a c r o s . h                                          *
*                                                                      *
*     Handy macros and constants for c programs.                       *
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
#ifndef HANDY_MACROS

/* Convenient in-line functions */
#	ifndef IsNull
#		define IsNull(x) ((LOGICAL)((POINTER)(x)==(POINTER)0))
#	endif
#	ifndef NotNull
#		define NotNull(x) ((LOGICAL)((POINTER)(x)!=(POINTER)0))
#	endif
#	ifndef SafeStr
#		define SafeStr(x) ((NotNull(x))? (x): "")
#	endif

/* Macros for testing and setting bits in an array. Such an array is normally used as a logical mask
* to determine if a corresponding element in an image array has a valid pixel. m is the array and n 
* is the bit number to set.
*/
#define SET_MASK_BIT(m,n)    (m[(n)>>3]|=(1<<((n)&0x7)))		/* Set bit number n */
#define UNSET_MASK_BIT(m,n)  (m[(n)>>3]&=(~(1<<((n)&0x7))))		/* Unset (clear) bit number n */
#define MASK_BIT_SET(m,n)    (m[(n)>>3]&(1<<((n)&0x7)))			/* Test to see if bit number n is set */
/*
* Given an array of size w x h, determine the array size required to contain enough bits
* to assign one bit to every element of the array.
*/
#define MASK_SIZE(w,h)       ((((w)*(h))-1)/8+1)


/* Useful macros for initializing or just declaring global variables.
*  Use in a header file where a defined variable indicates whether or not
*  initialization should be done on the current invocation, as in:
*
*  #ifdef DO_INIT
*  #	define GLOBAL GLOBAL_INIT
*  #else
*  #	define GLOBAL GLOBAL_EXTERN
*  #endif
*
*  Then initialize or declare your global variables, as in:
*
*  GLOBAL( int, Count, 0);
*  GLOBAL( const char, *Title, "Title String");
*  GLOBAL( struct value, val[5], {...initialization...});
*
*/
#   define GLOBAL_INIT( TYPE, NAME, VAL )   TYPE NAME = VAL
#   define GLOBAL_EXTERN( TYPE, NAME, VAL ) extern TYPE NAME

/* Now it has been included */
#	define HANDY_MACROS
#endif
