/*********************************************************************/
/** @file string_ext.c
 *
 * Natural extensions to the UNIX string library and string.h
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     s t r i n g _ e x t . c                                          *
*                                                                      *
*     Natural extensions to the UNIX string library and string.h       *
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

#include "string_ext.h"
#include <fpa_types.h>

/***********************************************************************
*                                                                      *
*     s t r l e n   - strlen(3C) that doesn't die on a NULL.           *
*                                                                      *
***********************************************************************/

#ifdef MACHINE_SUN
/*********************************************************************/
/** Same as strlen(3C) but doesn't die on a NULL.
 *
 * @return the size of the given string.
 *********************************************************************/
size_t	strlen

	(
	const char *s
	)

	{
	size_t	nc=0;

	if (!s) return nc;
	while(*s)
		{
		nc++;
		s++;
		}
	return nc;
	}
#endif

/***********************************************************************
*                                                                      *
*     s a f e _ s t r c p y   - copy string to given string safely     *
*     s a f e _ s t r c a t   - append string to given string safely   *
*     s a f e _ s t r l e n   - compute length of given string safely  *
*     s a f e _ s t r d u p   - allocate and copy given string safely  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Copy string to given string safely. Make sure the destination
 * string is not NULL
 * @param[out] str string to copy to
 * @param[in] from string to copy from
 *********************************************************************/
STRING	safe_strcpy

	(
	STRING			str,
	const STRING	from
	)

	{
	if (!str)  return str;				/* Nowhere to put results */
	if (!from) return strcpy(str, "");	/* No argument found */

	return strcpy(str, from);	/* Otherwise copy into destination */
	}

/*********************************************************************/
/** Append string to given string safely. Make sure the destination
 * string is not NULL
 * @param[in,out] str string to append to
 * @param[in]  from string to copy from
 *********************************************************************/
STRING	safe_strcat

	(
	STRING			str,
	const STRING	from
	)

	{
	if (!str)  return str;	/* Nowhere to put results */
	if (!from) return str;	/* No argument found */

	return strcat(str, from);	/* Otherwise copy into destination */
	}

/*********************************************************************/
/** Compute length of given string safely. Make sure the
 * string is not NULL
 *
 *	@param[in]	str		string variable
 * 	@return size of string.
 *********************************************************************/
size_t	safe_strlen

	(
	const STRING	str
	)

	{
	if (!str)  return 0;	/* Return 0 for NULL */

	return strlen(str);		/* Otherwise return length */
	}

/*********************************************************************/
/** Allocate and copy given string safely.
 *
 *	@param[in]	from	string to copy from
 * 	@return Pointer to the new string. You will need to Free this
 * 			memory when you are finished with it. (see FREEMEM)
 *********************************************************************/
STRING	safe_strdup

	(
	const STRING	from
	)

	{
	if (!from) return NullString;	/* No argument found */

	return strdup(from);	/* Otherwise copy into destination */
	}

#ifdef STANDALONE
main(void)
	{
	char	buf[50];
	size_t	m, n;

	while(1)
		{
		printf("> ");
		gets(buf);
		n = strlen(buf);
		m = strcspn(buf, "\n\r\f");
		if (m  < n)
			{
			buf[m] = '\0';
			n = strlen(buf);
			}

		printf("...%d\n", n);
		}
	}
#endif
