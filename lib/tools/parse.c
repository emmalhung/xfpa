/*********************************************************************/
/** @file parse.c
 *
 * String Parsing Library
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*     p a r s e . c                                                    *
*                                                                      *
*     String Parsing Library                                           *
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

#include "parse.h"
#include "unix.h"

#include <fpa_math.h>
#include <fpa_types.h>
#include <fpa_getmem.h>
#include <fpa_macros.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* Use white space as normal delimiters */
#define WHITE " \t\n\r\f"

/***********************************************************************
*                                                                      *
*     f l u s h _ l i n e   - Dispose of input, up to the end of the   *
*                             current line.                            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Dispose of input, up to the end of the current line.
 *
 *	@param[in]	*fp		File pointer
 * 	@return
 * 	- TRUE if successful.
 * 	- FALSE If EOF is encountered.
 *********************************************************************/
LOGICAL	flush_line

	(
	FILE	*fp
	)

	{
	int		c;

	while ((c=getc(fp)) != '\n')
		{
		if (c == EOF) return FALSE;
		}
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     g e t w o r d   - Read a word of text from the file connected    *
*                       to the given pointer.                          *
*                                                                      *
*     g e t v a l i d l i n e  - Read a line of text from the file     *
*                                connected to the given pointer if the *
*                                line is not empty or does not start   *
*                                with the given comment character(s).  *
*                                                                      *
*     g e t f i l e l i n e   - Read a line of text from the file      *
*                               connected to the given pointer.        *
*                                                                      *
*     u n g e t f i l e l i n e   - Put back the last getfileline or   *
*                                   getvalidline.                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Read a word of text from the file connected to the given pointer.
 *
 * 	@param[in] *fp 	File pointer
 * 	@param[in] word word to parse the function may alter this string
 * 	@param[in] ncl 	length of word
 * 	@return Pointer to 'word'. NULL for failure.
 *********************************************************************/
STRING	getword

	(
	FILE	*fp,
	STRING	word,
	size_t	ncl
	)

	{
	size_t		i;

	/* Input a line */
	if ( !word )                   return NULL;
	if ( !getfileline(fp, word, ncl) ) return NULL;

	/* Find next delimiter and truncate */
	i = strcspn(word, WHITE);
	if (i < strlen(word)) word[i] = '\0';
	return word;
	}

/*********************************************************************/
/** Read a line of text from the file connected to the given pointer
 * if the line is not empty or does not start with the given comment
 * characters.
 *
 * @param[in] *fp      File pointer
 * @param[out] line    line text from file
 * @param[in]  ncl     length of line
 * @param[in]  comment comment character(s)
 * @return Pointer to 'line'. NULL for failure.
 *********************************************************************/
STRING	getvalidline

	(
	FILE	*fp,
	STRING	line,
	size_t	ncl,
	STRING	comment
	)

	{
	STRING	ptr;

	while(TRUE)
		{
		/* get the line, return false if no line */
		if( !getfileline(fp, line, ncl) ) return NULL;
		if( blank(line) ) continue;
		/* strip leading blanks and test first char for comment */
		ptr = line + strspn(line, WHITE);
		if( strchr(comment, *ptr) != NULL ) continue;

		/* we have a valid line */
		memmove(line, ptr, MIN(strlen(ptr)+1,ncl));
		return line;
		}
	}


static	FILE	*Lastfp = NULL;
static	long	Lastpos = 0L;

/*********************************************************************/
/** Read a line of text from the file connected to the given pointer.
 *
 * @param[in] *fp   File pointer
 * @param[out] line line to parse
 * @param[in]  ncl  length of line
 * @return Pointer to 'line'. NULL for failure.
 *********************************************************************/
STRING	getfileline

	(
	FILE	*fp,
	STRING	line,
	size_t	ncl
	)

	{
	size_t	i;
	long	pos;

	/* Input a line */
	if ( !line )                       return NULL;
	pos = ftell(fp);
	if ( !fgets(line, (int) ncl, fp) ) return NULL;
	Lastpos = pos;
	Lastfp  = fp;

	/* Strip trailing line-feed */
	i = strcspn(line, "\n\r\f");
	if (i < strlen(line)) line[i] = '\0';
	return line;
	}

/*********************************************************************/
/** Put back the last getfileline or getvalidline.
 *
 *	@param[in]	*fp		file pointer
 *********************************************************************/
void	ungetfileline

	(
	FILE	*fp
	)

	{
	if (fp != Lastfp)   return;
	if (Lastfp == NULL) return;
	if (Lastpos < 0)    return;
	(void) fseek(fp, Lastpos, SEEK_SET);
	Lastpos = -1L;
	}

/***********************************************************************
*                                                                      *
*     e s c _ e n c o d e           - Replace special characters with  *
*                                     escaped characters               *
*     e s c _ d e c o d e           - Replace escaped characters with  *
*                                     special characters               *
*     e s c _ e n c o d e _ l e n   - length of esc_encode output      *
*     e s c _ d e c o d e _ l e n   - length of esc_decode output      *
*                                                                      *
***********************************************************************/

static	STRING	EscBuf = NullString;
static	int		EscLen = 0;

/**********************************************************************/

/*********************************************************************/
/** Replace special characters with escape characters.
 *
 *	@param[in]	fbuf	buffer to replace
 * 	@return Pointer to replaced text string. The returned buffer is
 * 			static and owned by the esc_encode/decode functions.
 * 			If you are not going to use this value immediately you
 * 			should make a copy of it.
 *********************************************************************/
STRING	esc_encode

	(
	const STRING	fbuf
	)

	{
	char	*tp, *fp, c;
	int		n;

	if ( IsNull(fbuf) ) return NullString;

	n = esc_encode_len(fbuf);
	if (n > EscLen)
		{
		EscLen = n;
		EscBuf = GETMEM(EscBuf, char, n+1);
		}

	tp = EscBuf;
	fp = fbuf;

	while ( (c = *(fp++)) != '\0' )
		{
		/* Replace special characters with backslash sequences */
		switch (c)
			{
			case '\n':	*(tp++) = '\\';
						*(tp++) = 'n';
						break;

			case '\t':	*(tp++) = '\\';
						*(tp++) = 't';
						break;

			case '\f':	*(tp++) = '\\';
						*(tp++) = 'f';
						break;

			case '\r':	*(tp++) = '\\';
						*(tp++) = 'r';
						break;

			case '\\':	*(tp++) = '\\';
						*(tp++) = '\\';
						break;

			/* Take all other characters verbaitim */
			default:	*(tp++) = c;
			}
		}
	*tp = '\0';

	return EscBuf;
	}

/**********************************************************************/

/*********************************************************************/
/** Replace escaped characters with special characters.
 *
 *	@param[in]	fbuf	buffer to replace
 * 	@return Pointer to replaced text string. The returned buffer is
 * 			static and owned by the esc_encode/decode functions.
 * 			If you are not going to use this value immediately you
 * 			should make a copy of it.
 *********************************************************************/
STRING	esc_decode

	(
	const STRING	fbuf
	)

	{
	char	*tp, *fp, c;
	int		n;

	if ( IsNull(fbuf) ) return NullString;

	n = esc_decode_len(fbuf);
	if (n > EscLen)
		{
		EscLen = n;
		EscBuf = GETMEM(EscBuf, char, n+1);
		}

	tp = EscBuf;
	fp = fbuf;

	while ( (c = *(fp++)) != '\0' )
		{
		/* Take all characters verbatim except backslash */
		if (c != '\\') *(tp++) = c;

		/* Treat backslash as escape character */
		else
			{
			/* Examine character after backslash */
			c = *(fp++);
			if ( c == '\0' )
				{
				/* Leave trailing backslash */
				*(tp++) = '\\';
				break;
				}

			/* Supported cases ... */
			switch (c)
				{
				case 'n':	*(tp++) = '\n';
							break;

				case 't':	*(tp++) = '\t';
							break;

				case 'f':	*(tp++) = '\f';
							break;

				case 'r':	*(tp++) = '\r';
							break;

				/* Take any other escaped character as is */
				default:	*(tp++) = c;
				}
			}
		}
	*tp = '\0';

	return EscBuf;
	}

/**********************************************************************/

/*********************************************************************/
/** Get length of esc_encode output.
 *
 *	@param[in]	 fbuf	valid esc_encode output
 * 	@return Length of fbuf.
 *********************************************************************/
int		esc_encode_len

	(
	const STRING	fbuf
	)

	{
	char	*fp, c;
	int		n = 0;

	if ( IsNull(fbuf) ) return 0;

	fp = fbuf;

	while ( (c = *(fp++)) != '\0' )
		{
		switch (c)
			{
			/* Special characters will take 2 bytes */
			case '\n':
			case '\t':
			case '\f':
			case '\r':
			case '\\':	n += 2;
						break;

			/* All other characters will take one byte */
			default:	n += 1;
			}
		}

	return n;
	}

/**********************************************************************/

/*********************************************************************/
/** Get length of esc_decode output.
 *
 *	@param[in]	fbuf	valid esc_decode output
 * 	@return Length of fbuf.
 *********************************************************************/
int		esc_decode_len

	(
	const STRING	fbuf
	)

	{
	char	*fp, c;
	int		n = 0;

	if ( IsNull(fbuf) ) return 0;

	fp = fbuf;

	while ( (c = *(fp++)) != '\0' )
		{
		/* All non-escaped characters will take one byte */
		if (c != '\\') n += 1;

		/* Treat backslash as escape character */
		else
			{
			/* Examine character after backslash */
			c = *(fp++);
			if ( c == '\0' )
				{
				/* Trailing backslash will take one byte */
				n += 1;
				break;
				}

			/* Otherwise only the character after the backslash counts */
			n += 1;
			}
		}

	return n;
	}

/***********************************************************************
*                                                                       *
*     s t r c p y _ a r g    - copy string argument to given string     *
*     s t r  n c p y _ a r g - copy limited string argument to string   *
*     s t r d u p _ a r g    - allocate a copy of string argument       *
*     s t r e n v _ a r g    - get environment expansion of string      *
*     s t r r e m _ a r g    - get remainder of command line            *
*     s t r i n g _ a r g    - get string argument from command line    *
*     o p t _ a r g          - parse option=value from command line     *
*     i n t _ a r g          - get integer argument from command line   *
*     l o n g _ a r g        - get long argument from command line      *
*     h e x _ a r g          - get hex argument from command line       *
*     o c t a l _ a r g      - get octal argument from command line     *
*     b a s e _ a r g        - get arb-base argument from command line  *
*     u b a s e _ a r g      - get unsigned arb-base argument from c-l  *
*     f l o a t _ a r g      - get float argument from command line     *
*     d o u b l e _ a r g    - get double argument from command line    *
*     l o g i c a l _ a r g  - get logical argument from command line   *
*     y e s n o _ a r g      - get yes/no argument from command line    *
*                                                                       *
***********************************************************************/

/*********************************************************************/
/** Copy string argument to given string.
 *
 * @param[out] 		str     string variable to copy to
 * @param[in,out]  	line    line to parse
 * @param[out] 		*status did it work?
 * @return Pointer to 'str'.
 *********************************************************************/
STRING	strcpy_arg

	(
	STRING	str,
	STRING	line,
	LOGICAL	*status
	)

	{
	STRING	c;

	if (status) *status = FALSE;
	c = string_arg(line);				/* Read argument from line */
	if (!str) return str;				/* Nowhere to put results */
	if (!c)   return strcpy(str, "");	/* No argument found */

	if (status) *status = TRUE;
	return strcpy(str, c);	/* Otherwise copy into destination */
	}


/*********************************************************************/
/** Copy string argument to given string but limit the number of
 *  characters that can be copied.
 *
 * @param[out] 		str     string variable to copy to
 * @param[in] 		len     length of the str buffer in bytes
 * @param[in,out]  	line    line to parse
 * @param[out] 		*status did it work?
 * @return Pointer to 'str'.
 *********************************************************************/
STRING	strncpy_arg

	(
	STRING	str,
	size_t  len,
	STRING	line,
	LOGICAL	*status
	)

	{
	STRING	c;

	if (status) *status = FALSE;
	c = string_arg(line);					/* Read argument from line */
	if (!str) return str;					/* Nowhere to put results */
	len--;
	if(len < 1) return NULL;				/* Still nowhere to put results */
	if (!c)   return strcpy(str, "");		/* No argument found */
	if (status) *status = TRUE;
	str[len] = '\0';						/* Null terminate the buffer */
	memmove(str, c, MIN(strlen(c)+1,len));	/* Copy a maximum of len-1 bytes */
	return str;
	}


/*********************************************************************/
/** Allocate a copy of the string argument.
 *
 *	@param[in,out]	line		line to parse
 * 	@return Pointer to copy of given string. You will need to Free this
 * 			memory when you are finished with it.
 *********************************************************************/
STRING	strdup_arg

	(
	STRING	line
	)

	{
	STRING	c;

	c = string_arg(line);	/* Read argument from line no matter what */
	if (!c)   return NULL;	/* No argument found */

	return strdup(c);		/* Allocate memory for copy and return */
	}

/*********************************************************************/
/** Get environment expansion of string.
 *
 *	@param[in,out]	line		line to parse
 * 	@return Pointer expanded string. The buffer returned is an internal
 * 			static buffer. If you are not going to use it immediately
 * 			then you should make a copy of it.
 *********************************************************************/
STRING	strenv_arg

	(
	STRING	line
	)

	{
	STRING	c;

	c = string_arg(line);	/* Read argument from line no matter what */
	if (!c)   return NULL;	/* No argument found */

	return env_sub(c);		/* Return environment expansion */
	}

/*********************************************************************/
/** Extract the next string argument from the given line.
 *
 *	@param[in,out]	line		line to parse
 * 	@return Pointer to next string argument. The buffer returned is an
 * 			internal static buffer. If you are not going to use it
 * 			immediately then you should make a copy of it.
 *********************************************************************/
STRING	string_arg

	(
	STRING	line
	)

	{
	static			char	nextarg[256] = "";
	static	const	size_t	nca = sizeof(nextarg);

	size_t	k1, nc;
	STRING	l1, l2;

	/* Trim leading blanks from line */
	if (!line) return NULL;
	k1 = strspn(line, WHITE);
	l1 = line + k1;
	l2 = NULL;

	/* Return NULL if line is empty */
	(void) strncpy(nextarg, "", nca);
	if (strcmp(l1, "") == 0) return NULL;

	/* Find other end of token - balance quotes if present */
	if (line[k1] == '"')
		{
		nc = strlen(l1);
		if (l2 = strchr(l1+1, '"'))
			{
			l1++;
			nc = l2 - l1;
			l2++;
			}
		}
	else if (line[k1] == '\'')
		{
		nc = strlen(l1);
		if (l2 = strchr(l1+1, '\''))
			{
			l1++;
			nc = l2 - l1;
			l2++;
			}
		}
	else
		{
		nc = strcspn(l1, WHITE);
		l2 = l1 + nc;
		}

	/* Extract the token and remove it from line */
	nc = MIN(nc, nca-1);
	(void) strncpy(nextarg, l1, nc);
	if (l2) (void) memmove(line, l2, strlen(l2)+1);
	else    (void) strcpy(line, "");

	/* Indicate success */
	return nextarg;
	}

/*********************************************************************/
/** Parse option=value from command line.
 *
 * @param[in,out] line   Line to parse
 * @param[out] *name  name of option (pointer to internal buffer)
 * @param[out] *value value (pointer to internal buffer)
 * @return Null if failed. The buffers returned are  internal
 * static buffers. If you are not going to use them immediately then you
 * should make a copy of them.
 *********************************************************************/
STRING	opt_arg

	(
	STRING	line ,
	STRING	*name ,
	STRING	*value
	)

	{
	static			char	nextopt[256] = "";
	static			char	nextval[256] = "";
	static	const	size_t	nca = sizeof(nextopt);

	size_t	k1, nc, nce, ncs;
	LOGICAL	eq;
	STRING	l1, l2;

	/* Trim leading blanks from line */
	k1 = strspn(line, WHITE);
	l1 = line + k1;
	l2 = NULL;

	/* Return NULL if line is empty */
	if (name)  *name  = NULL;
	if (value) *value = NULL;
	(void) strncpy(nextopt, "", nca);
	(void) strncpy(nextval, "", nca);
	if (strcmp(l1, "") == 0) return NULL;

	/* Find the '=' or blanks whichever comes first */
	eq = FALSE;
	nc = strlen(l1);
	if (l2 = strchr(l1, '='))
		{
		nce = l2 - l1;
		ncs = strcspn(l1, WHITE);
		eq  = (LOGICAL) (nce < ncs);
		if (eq)
			{
			nc = nce;
			l2++;
			}
		else
			{
			nc = ncs;
			l2 = l1 + nc;
			}
		}
	else
		{
		nc = strcspn(l1, WHITE);
		l2 = l1 + nc;
		}

	/* Extract the option name and remove it from line */
	nc = MIN(nc, nca-1);
	(void) strncpy(nextopt, l1, nc);
	if (l2) (void) memmove(line, l2, strlen(l2)+1);
	else    (void) strcpy(line, "");

	/* If no '=' return just the option name */
	if (name)  *name  = nextopt;
	if (value) *value = NULL;
	if (!eq) return nextopt;

	/* Skip over the '=' to the option value */
	k1 = 0;
	l1 = line + k1;
	l2 = NULL;

	/* Find other end of option value - balance quotes if present */
	if (line[k1] == '"')
		{
		nc = strlen(l1);
		if (l2 = strchr(l1+1, '"'))
			{
			l1++;
			nc = l2 - l1;
			l2++;
			}
		}
	else if (line[k1] == '\'')
		{
		nc = strlen(l1);
		if (l2 = strchr(l1+1, '\''))
			{
			l1++;
			nc = l2 - l1;
			l2++;
			}
		}
	else
		{
		nc = strcspn(l1, WHITE);
		l2 = l1 + nc;
		}

	/* Extract the option value and remove it from line */
	nc = MIN(nc, nca-1);
	(void) strncpy(nextval, l1, nc);
	if (l2) (void) memmove(line, l2, strlen(l2)+1);
	else    (void) strcpy(line, "");

	/* Indicate success */
	if (name)  *name  = nextopt;
	if (value) *value = nextval;
	return nextopt;
	}

/*********************************************************************/
/**  Strip off leading white space.
 *
 *	@param[in,out]	line		line to parse
 * 	@return Pointer remainder of given line.
 *********************************************************************/
STRING	strrem_arg

	(
	STRING	line
	)

	{
	size_t	k1;
	STRING	l1;

	/* Trim leading blanks from line */
	k1 = strspn(line, WHITE);
	l1 = line + k1;

	/* Return NULL if line is empty */
	if (strcmp(l1, "") == 0) return NULL;

	/* Indicate success */
	(void) memmove(line, l1, strlen(l1)+1);
	return line;
	}

/*********************************************************************/
/** Extract the next integer argument from the given line.
 *
 * @param[in,out] line     line to parse
 * @param[out] *status did it work?
 * @return The next integer argument if available.
 *********************************************************************/
int	int_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return (int) base_arg(line, 10, status);
	}

/*********************************************************************/
/** Extract the next long integer argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next long integer argument if available.
 *********************************************************************/
long	long_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return base_arg(line, 10, status);
	}

/*********************************************************************/
/** Extract the next hex argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next hex argument if available.
 *********************************************************************/
long	hex_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return base_arg(line, 16, status);
	}

/*********************************************************************/
/** Extract the next octal argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next octal argument if available.
 *********************************************************************/
long	octal_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return base_arg(line, 8, status);
	}

/*********************************************************************/
/** Extract the next long integer of given base from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  base    base 2 ... 36
 * @param[out] *status did it work?
 * @return The next argument of given base if available.
 *********************************************************************/
long	base_arg

	(
	STRING	line,
	int		base,
	LOGICAL	*status
	)

	{
	long	value;
	STRING	token, p;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token = string_arg(line);
	if (!token) return 0;

	/* Interpret as a long integer if possible */
	value = strtol(token, &p, base);
	if (p == token) return 0;
	if (status) *status = TRUE;
	return value;
	}

/*********************************************************************/
/** Extract the next unsigned long integer of given base from the
 * given line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  base    base 2 ... 36
 * @param[out] *status did it work?
 * @return The next argument of given base if available.
 *********************************************************************/
unsigned long	ubase_arg

	(
	STRING	line,
	int		base,
	LOGICAL	*status
	)

	{
	unsigned long	value;
	STRING			token, p;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token   = string_arg(line);
	if (!token) return 0;

	/* Interpret as a long integer if possible */
	value = strtoul(token, &p, base);
	if (p == token) return 0;
	if (status) *status = TRUE;
	return value;
	}

/*********************************************************************/
/** Extract the next float argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next float argument if available.
 *********************************************************************/
float	float_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return (float) double_arg(line, status);
	}

/*********************************************************************/
/** Extract the next double argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next double argument if available.
 *********************************************************************/
double	double_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	double	value;
	STRING	token, p;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token   = string_arg(line);
	if (!token) return 0.0;

	/* Interpret as a double if possible */
	value = strtod(token, &p);
	if (p == token) return 0.0;
	if (status) *status = TRUE;
	return value;
	}

/*********************************************************************/
/** Extract the next logical argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next logical argument if available.
 *********************************************************************/
LOGICAL	logical_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	STRING	token;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token   = string_arg(line);
	if (!token) return FALSE;

	/* Interpret as "TRUE", "FALSE", "T", or "F" if possible */
	if (status) *status = TRUE;
	if (same_ic(token, "true"))  return TRUE;
	if (same_ic(token, "t"))     return TRUE;
	if (same_ic(token, "false")) return FALSE;
	if (same_ic(token, "f"))     return FALSE;

	if (status) *status = FALSE;
	return FALSE;
	}

/*********************************************************************/
/** Extract the next yes/no argument from the given line.
 *
 * @param[in,out]  line    line to parse
 * @param[out] *status did it work?
 * @return The next yes/no argument if available.
 * 	- yes = TRUE
 * 	- no  = FALSE
 *********************************************************************/
LOGICAL	yesno_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	STRING	token;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token   = string_arg(line);
	if (!token) return FALSE;

	/* Interpret as "YES", "NO", "Y", or "N" if possible */
	if (status) *status = TRUE;
	if (same_ic(token, "yes")) return TRUE;
	if (same_ic(token, "y"))   return TRUE;
	if (same_ic(token, "no"))  return FALSE;
	if (same_ic(token, "n"))   return FALSE;

	if (status) *status = FALSE;
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     f i x e d _ s t r i n g _ a r g   - Get fixed length string      *
*                                         argument from command line   *
*     f i x e d _ i n t _ a r g         - Get fixed length integer     *
*                                         argument from command line   *
*     f i x e d _ l o n g _ a r g       - Get fixed length long        *
*                                         argument from command line   *
*     f i x e d _ h e x _ a r g         - Get fixed length hex         *
*                                         argument from command line   *
*     f i x e d _ o c t a l _ a r g     - Get fixed length octal       *
*                                         argument from command line   *
*     f i x e d _ b a s e _ a r g       - Get fixed length arb-base    *
*                                         argument from command line   *
*     f i x e d _ f l o a t _ a r g     - Get fixed length float       *
*                                         argument from command line   *
*     f i x e d _ d o u b l e _ a r g   - Get fixed length double      *
*                                         argument from command line   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Get fixed length string argument from the command line.
 *
 * 	@param[in,out]  line    line to parse
 * 	@param[in]  nc      fixed length of string to extract
 * 	@return Pointer to next string argument. The buffer returned is an
 * 			internal static buffer. If you are not going to use it
 * 			immediately then you should make a copy of it.
 *********************************************************************/
STRING	fixed_string_arg

	(
	STRING	line,
	size_t	nc
	)

	{
	static			char	nextarg[256] = "";
	static	const	size_t	nca = sizeof(nextarg);

	/* Error if fixed length is too long */
	if (nc > nca) return NULL;

	/* Copy fixed length of string */
	(void) strncpy(nextarg, "", nca);
	(void) strncpy(nextarg, line, nc);

	/* Remove copied string from line */
	if (nc < strlen(line)) (void) memmove(line, line+nc, strlen(line+nc)+1);
	else                   (void) strcpy(line, "");

	/* Return copied string */
	return nextarg;
	}

/*********************************************************************/
/** Get fixed length integer argument from command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of integer to extract
 * @param[out] *status did it work?
 * @return The next integer argument if available.
 *********************************************************************/
int	fixed_int_arg

	(
	STRING	line,
	size_t	nc,
	LOGICAL	*status
	)

	{
	return (int) fixed_base_arg(line, nc, 10, status);
	}

/*********************************************************************/
/** Get fixed length long integer argument from command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of long to extract
 * @param[out] *status did it work?
 * @return The next long integer argument if available.
 *********************************************************************/
long	fixed_long_arg

	(
	STRING	line,
	size_t	nc,
	LOGICAL	*status
	)

	{
	return fixed_base_arg(line, nc, 10, status);
	}

/*********************************************************************/
/** Get fixed length hex argument from command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of hex value to extract
 * @param[out] *status did it work?
 * @return The next hex argument if available.
 *********************************************************************/
long	fixed_hex_arg

	(
	STRING	line,
	size_t	nc,
	LOGICAL	*status
	)

	{
	return fixed_base_arg(line, nc, 16, status);
	}

/*********************************************************************/
/** Get fixed length octal argument from command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of octal value to extract
 * @param[out] *status did it work?
 * @return The next octal argument if available.
 *********************************************************************/
long	fixed_octal_arg

	(
	STRING	line,
	size_t	nc,
	LOGICAL	*status
	)

	{
	return fixed_base_arg(line, nc, 8, status);
	}

/*********************************************************************/
/** Get fixed length long integer argument of given base from
 * command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of value to extract
 * @param[in]  base    base 2 ... 36
 * @param[out] *status did it work?
 * @return The next argument of given if available.
 *********************************************************************/
long	fixed_base_arg

	(
	STRING	line,
	size_t	nc,
	int		base,
	LOGICAL	*status
	)

	{
	long	value;
	STRING	token, p;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token   = fixed_string_arg(line, nc);
	if (!token) return 0;

	/* Interpret as a long integer if possible */
	value = strtol(token, &p, base);
	if (p == token) return 0;
	if (status) *status = TRUE;
	return value;
	}

/*********************************************************************/
/** Get fixed length float argument from command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of float to extract
 * @param[out] *status did it work?
 * @return The next integer argument if available.
 *********************************************************************/
float	fixed_float_arg

	(
	STRING	line,
	size_t	nc,
	LOGICAL	*status
	)

	{
	return (float) fixed_double_arg(line, nc, status);
	}

/*********************************************************************/
/** Get fixed length double argument from command line.
 *
 * @param[in,out]  line    line to parse
 * @param[in]  nc      fixed length of double to extract
 * @param[out] *status did it work?
 * @return The next double argument if available.
 *********************************************************************/
double	fixed_double_arg

	(
	STRING	line,
	size_t	nc,
	LOGICAL	*status
	)

	{
	double	value;
	STRING	token, p;

	/* Get first string token on line */
	if (status) *status = FALSE;
	token   = fixed_string_arg(line, nc);
	if (!token) return 0.0;

	/* Interpret as a double if possible */
	value = strtod(token, &p);
	if (p == token) return 0.0;
	if (status) *status = TRUE;
	return value;
	}

/***********************************************************************
*                                                                      *
*     s t r t o k _ a r g       - return pointer to string argument    *
*     i n t t o k _ a r g       - return integer argument              *
*     l o n g t o k _ a r g     - return long integer argument         *
*     f l o a t t o k _ a r g   - return float argument                *
*     d o u b l e t o k _ a r g - return double argument               *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Parse a line like the function strtok and return the next
 * argument. Leaves input line alone.
 *
 * @param[in] line line to parse
 * @return Pointer into the given line that indicates the start of
 * the next string token. The pointer is a static internal variable if
 * you are not going to use it immediately you should make a copy of
 * it.
 *********************************************************************/
STRING	strtok_arg

	(
	STRING	line
	)

	{
	static STRING nextptr = NULL;

	STRING	bgnptr;

	if (line) nextptr = line;
	if (!nextptr || *nextptr == '\0') return NULL;

	/* Trim leading blanks from line */
	bgnptr = nextptr + strspn(nextptr, WHITE);

	/* Return false if line is empty */
	if (strcmp(bgnptr, "") == 0) return NULL;

	/* Find other end of token - balance quotes if present */
	if (*bgnptr == '"')
		{
		nextptr = strchr(bgnptr+1, '"');
		if (nextptr) bgnptr++;
		}
	else if (*bgnptr == '\'')
		{
		nextptr = strchr(bgnptr+1, '\'');
		if (nextptr) bgnptr++;
		}
	else
		{
		nextptr = bgnptr + strcspn(bgnptr, WHITE);
		}
	if (nextptr && *nextptr)
		{
		*nextptr = '\0';
		nextptr++;
		}
	return bgnptr;
	}

/*********************************************************************/
/** Parse a line and return the next integer argument.
 * Does not affect the 'line' buffer.
 *
 * @param[in] line    line to parse
 * @param[out] *status did it work?
 * @return The next integer argument in the line.
 *********************************************************************/
int	inttok_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return (int) longtok_arg(line, status);
	}

/*********************************************************************/
/** Parse a line and return the next long integer argument.
 * Does not affect the 'line' buffer.
 *
 * @param[in] line    line to parse
 * @param[out] *status did it work?
 * @return The next long integer argument in the line.
 *********************************************************************/
long	longtok_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	long	value;
	STRING	token, p;

	if (status) *status = FALSE;
	token   = strtok_arg(line);
	if (!token) return 0;

	/* Interpret as a long integer if possible */
	value = strtol(token, &p, 10);
	if (p == token) return 0;
	if (status) *status = TRUE;
	return value;
	}

/*********************************************************************/
/** Parse a line and return the next float argument.
 * Does not affect the 'line' buffer.
 *
 * @param[in] line    line to parse
 * @param[out] *status did it work?
 * @return The next float argument in the line.
 *********************************************************************/
float	floattok_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	return (float) doubletok_arg(line, status);
	}

/*********************************************************************/
/** Parse a line and return the next double argument.
 * Does not affect the 'line' buffer.
 *
 * @param[in] line    line to parse
 * @param[out] *status did it work?
 * @return The next double argument in the line.
 *********************************************************************/
double	doubletok_arg

	(
	STRING	line,
	LOGICAL	*status
	)

	{
	double	value;
	STRING	token, p;

	if (status) *status = FALSE;
	token = strtok_arg(line);
	if (!token) return 0.0;

	/* Interpret as a double if possible */
	value = strtod(token, &p);
	if (p == token) return 0.0;
	*status = TRUE;
	return value;
	}

/***********************************************************************
*                                                                      *
*     s a m e         - determine if two strings are the same.         *
*     s a m e _ i c   - case insensitive same().                       *
*                                                                      *
*     s a m e _ s t a r t         - determine if two strings are the   *
*                                   same up to the end of the second   *
*                                   string.                            *
*     s a m e _ s t a r t _ i c   - case insensitive same_start().     *
*                                                                      *
*     m a t c h         - determine if one string contains a second.   *
*     m a t c h _ i c   - case insensitive match().                    *
*                                                                      *
*     b l a n k   - determine if a string is blank (any white space).  *
*                                                                      *
*     n o _ w h i t e   - remove white space from both ends of string. *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Determine if two strings are the same. Case sensitive version.
 *
 * @param[in] string1 first string to test
 * @param[in] string2 second string to test
 * @return
 * 	- TRUE if strings are the same.
 * 	- FALSE if string are not the same.
 *********************************************************************/
LOGICAL	same

	(
	STRING	string1,
	STRING	string2
	)

	{
	if (!string1 && !string2)         return TRUE;
	if (!string1 && string2[0]=='\0') return TRUE;
	if (!string2 && string1[0]=='\0') return TRUE;
	if (!string1)                     return FALSE;
	if (!string2)                     return FALSE;
	return ( strcmp(string1, string2) == 0 );
	}

/*********************************************************************/
/** Determine if two strings are the same. Case insensitive version.
 *
 * @param[in] string1 first string to test
 * @param[in] string2 second string to test
 * @return
 * 	- TRUE if strings are the same.
 * 	- FALSE if string are not the same.
 *********************************************************************/
LOGICAL	same_ic

	(
	STRING	string1,
	STRING	string2
	)

	{
	if (!string1 && !string2)         return TRUE;
	if (!string1 && string2[0]=='\0') return TRUE;
	if (!string2 && string1[0]=='\0') return TRUE;
	if (!string1)                     return FALSE;
	if (!string2)                     return FALSE;
	return ( strcasecmp(string1, string2) == 0 );
	}

/*********************************************************************/
/** Determine if two strings are the same up to the end of the second
 * string. Case sensitive version.
 *
 * @param[in] string1 first string to test
 * @param[in] string2 second string to test
 * @return
 * 	- TRUE if the strings are the same up to the end of the second
 * 	       string.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	same_start

	(
	STRING	string1,
	STRING	string2
	)

	{
	size_t		n;

	if (blank(string1) && blank(string2)) return TRUE;
	if (blank(string1))                   return FALSE;
	if (blank(string2))                   return FALSE;

	n = strlen(string2);
	return ( strncmp(string1, string2, n) == 0 );
	}

/*********************************************************************/
/** Determine if two strings are the same up to the end of the second
 * string. Case insensitive version.
 *
 * @param[in] string1 first string to test
 * @param[in] string2 second string to test
 * @return
 * 	- TRUE if the strings are the same up to the end of the second
 * 	       string.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	same_start_ic

	(
	STRING	string1,
	STRING	string2
	)

	{
	size_t		n;

	if (blank(string1) && blank(string2)) return TRUE;
	if (blank(string1))                   return FALSE;
	if (blank(string2))                   return FALSE;

	n = strlen(string2);
	return ( strncasecmp(string1, string2, n) == 0 );
	}

/*********************************************************************/
/** Determine if the first string contains the second. Case sensitive
 * version.
 *
 * @param[in] string1 first string to test
 * @param[in] string2 second string to test
 * @return
 * 	- TRUE if the first string contains the second.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	match

	(
	STRING	string1,
	STRING	string2
	)

	{
	if (!string1 && !string2)         return TRUE;
	if (!string1 && string2[0]=='\0') return TRUE;
	if (!string2 && string1[0]=='\0') return TRUE;
	if (!string1)                     return FALSE;
	if (!string2)                     return FALSE;
	return ( strstr(string1, string2) != NULL );
	}

/*********************************************************************/
/** Determine if a string is blank (any white space).
 *
 * @param[in] string  string to test
 * @return
 * 	- TRUE if the string contains only white space.
 * 	- FALSE if the string contains any non-white space.
 *********************************************************************/
LOGICAL	blank

	(
	STRING	string
	)

	{
	if (!string) return TRUE;
	return ( strspn(string, WHITE) >= strlen(string) );
	}

/*********************************************************************/
/** Remove white space from both ends of the string.
 * @param[in] string string to strip. String is modified by this function.
 *********************************************************************/
void	no_white

	(
	STRING		string
	)

	{
	size_t		start, ii;

	/* Return immediately if Null buffer */
	if ( IsNull(string) ) return;

	/* Return an empty buffer immediately */
	if ( blank(string) )
		{
		string[0] = '\0';
		return;
		}

	/* Remove leading white space */
	start = strspn(string, WHITE);
	(void) memmove(string, string+start, strlen(string+start)+1);

	/* Remove trailing white space */
	for ( ii = strlen(string); ii > 0; ii-- )
		{
		if ( strcspn(string+ii-1, WHITE) > 0 ) break;
		}
	string[ii] = '\0';
	return;
	}

/***********************************************************************
*                                                                      *
*     i n t _ s t r i n g   - Write an integer to a string             *
*                                                                      *
*     f f o r m a t   - Format a float to use the minimum number of    *
*                       decimal places up to a given maximum.          *
*                                                                      *
*     f r o u n d   - Round a float to use the minimum number of       *
*                     decimal places up to a given maximum.            *
*                                                                      *
*     f c o m p a r e   - Compare two floats to a given tolerance.     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Write an integer to a string.
 *
 * @param[in] ival numter to be written
 * @param[out] a   integer as a string
 * @param[in] nca  maximum length of a
 * @return Number of digits written.
 *********************************************************************/
int	int_string

	(
	int		ival,
	STRING	a,
	size_t	nca
	)

	{
	int		nd;

	nd = ndigit(ival);
	if (nd >= (int) nca)
		{
		(void) strncpy(a, "", nca);
		return 0;
		}

	(void) sprintf(a, "%d", ival);
	return nd;
	}

/*********************************************************************/
/** Format a float to use the minimum number of decimal places up to
 * a given maximum.
 *
 * @param[in] val value to format
 * @param[in] dmax maximum number of decimal places
 * @return Pointer to the formatted string. The returned buffer is an
 * internal static buffer; if you are not going to use it immediately
 * you should make a copy of it.
 *********************************************************************/
STRING	fformat

	(
	float	val,
	int		dmax
	)

	{
	float	cval;
	int		ival, p, pmax, d;
	char	fmt[10];

	static	char	fval[50];

	pmax = 1;
	for (d=0; d<dmax; d++)
		pmax *= 10;
	ival = NINT(pmax * val);
	cval = (float)ival / (float)pmax;

	p = pmax;
	for (d=0; d<dmax; d++)
		{
		if (ival%p == 0) break;
		p /= 10;
		}
	(void) sprintf(fmt, "%%.%df", d);
	(void) sprintf(fval, fmt, cval);

	return fval;
	}

/*********************************************************************/
/** Round a float to use the minimum number of decimal places up to a
 * given maximum.
 *
 * @param[in] val value to round
 * @param[in] dmax maximum number of decimal places
 *
 * @return The rounded number.
 *********************************************************************/
float	fround

	(
	float	val,
	int		dmax
	)

	{
	double	cval;
	int		ival, pmax, d;

	pmax = 1;
	for (d=0; d<dmax; d++)
		pmax *= 10;
	ival = NINT(pmax * val);
	cval = (double)ival / (double)pmax;

	return (float)cval;
	}

/*********************************************************************/
/** Compare two floats to a given tolerance.
 *
 * @param[in] val1 first value to compare
 * @param[in] val2 second value to compare
 * @param[in] range expected range as benchmark
 * @param[in] portion portion of range to use as tolerance
 * @return
 * 	- TRUE if floats are the same to a given tolerance.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	fcompare

	(
	float	val1,
	float	val2,
	float	range,
	float	portion
	)

	{
	float	val_diff, tol;

	if ( val1 == val2 ) return TRUE;

	val_diff = (float) fabs(val1 - val2);
	tol      = (float) fabs(range * portion);
	if (val_diff < tol) return TRUE;

	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     n d i g i t   - Determine how many digits in a given integer     *
*     f d i g i t   - Determine highest order digit in a given float   *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Determine how many digits in a given integer.
 *
 *	@param[in]	ival	number
 * 	@return the number of digits in the integer.
 *********************************************************************/
int	ndigit

	(
	int	ival
	)

	{
	double	val;
	int		ndig;

	if (ival == 0) return 1;

	val  = (double) abs(ival) + 0.5;
	ndig = fdigit(val);
	if (ival > 0) return ndig;
	if (ival < 0) return ndig+1;
	return 1;
	}

/*********************************************************************/
/** Determine the highest order digit in a given float.
 *
 *	@param[in]	val		number
 * 	@return The highest order digit in the float.
 *********************************************************************/
int	fdigit

	(
	double	val
	)

	{
	if (val == 0) return 1;
	else          return 1 + (int) log10(fabs(val));
	}

/***********************************************************************
*                                                                      *
*     r a n g e _ n o r m   - Normalize a number to within given range *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Normalize a number to within a given range.
 *
 * @param[in] val value of to normalize
 * @param[in] vmin range minimum
 * @param[in] vmax range maximum
 * @param[out] *carry ?
 * @return The normalized number.
 *********************************************************************/
float	range_norm
	(
	float	val,
	float	vmin,
	float	vmax,
	int		*carry
	)

	{
	float	rval, width;
	int		nwid = 0;

	rval = val;
	if (carry) *carry = nwid;

	/* Compute range from limits given */
	width = vmax - vmin;
	if (width <= 0) return rval;

	/* Val is below range */
	if (val < vmin)
		{
		nwid  = -(int) ceil( (vmin-val)/width );
		rval -= nwid*width;
		}

	/* Val is above range */
	else if (val > vmax)
		{
		nwid  = (int) ceil( (val-vmax)/width );
		rval -= nwid*width;
		}

	if (carry) *carry = nwid;
	return rval;
	}

/***********************************************************************
*                                                                      *
*     p a r s e _ o p t i o n   - parse option of the form name=value  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Parse a keyword=keywordvalue pair into name and value.
 *
 * @param[in]  option option to parse
 * @param[out] *name  name of option
 * @param[out] *value value of option
 * @return
 * 	- TRUE if successful.
 * 	- FALSE if not.
 *********************************************************************/
LOGICAL	parse_option

	(
	STRING	option ,
	STRING	*name ,
	STRING	*value
	)

	{
	int		n;

	static	STRING	opnam = NULL;
	static	STRING	opval = NULL;

	/* Default everything blank */
	FREEMEM(opnam);
	FREEMEM(opval);
	if (name)  *name  = opnam;
	if (value) *value = opval;
	if (blank(option)) return FALSE;

	/* Find the '=' if present */
	n = strcspn(option, "=");

	/* Take everything up to the '=' as option name */
	opnam = strdup(option);
	opnam[n] = '\0';

	/* Take everything after the '=' as option value */
	if (n < strlen(option)) opval = strdup(option+n+1);

	if (name)  *name  = opnam;
	if (value) *value = opval;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     p a r s e _ m s t r i n g                                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Parse a string made up of modifiers and values, into its
 * separate pieces.  The string is accompanied by a control string
 * to identify what parts are what.  The control string is the
 * same length as the main string.  Each byte signals what the
 * corresponding byte represents in the main string.  The digits
 * 1-9 indicate modifiers (0 is the first default modifier).
 * White space is ignored, and anything else is assumed to be
 * actual values.
 *********************************************************************/

int	parse_mstring

	(
	STRING	val ,
	STRING	ctl ,
	STRING	*vlist ,
	STRING	*clist
	)

	{
	size_t	nc;
	int		imod;
	char	c, delims[50];

	/* Storage for parsed lists */
	static	STRING	vtable[10];
	static	STRING	ctable[10];
	static	int		nmods = sizeof(vtable)/sizeof(STRING);

	/* Setup default return values */
	if (vlist) *vlist = NULL;
	if (clist) *clist = NULL;
	if (blank(val)) return 0;
	if (blank(ctl)) return 0;
	if (strlen(val) != strlen(ctl)) return 0;

	/* Clean out old data */
	for (imod=0; imod<nmods; imod++)
		{
		FREEMEM(vtable[imod]);
		FREEMEM(ctable[imod]);
		}

	/* Strip leading white space until first modifier or value */
	nc   = strspn(ctl, WHITE);
	val += nc;
	ctl += nc;
	imod = -1;

	/* Repeat until string is exhausted */
	while (!blank(val) || !blank(ctl))
		{
		/* Is this a modifier ? */
		c = ctl[0];
		if ((c >= '0') && (c <= '9'))
			{
			(void) sprintf(delims, "%c%s", c, WHITE);
			nc   = strspn(ctl, delims);
			val += nc;
			ctl += nc;
			imod = (int) c - (int) '0';
			}

		/* Must be a value */
		else
			{
			nc   = strcspn(ctl, "0123456789");
			if ((imod >= 0) && (imod < nmods))
				{
				vtable[imod] = GETMEM(vtable[imod], char, nc+1);
				ctable[imod] = GETMEM(ctable[imod], char, nc+1);
				(void) strncpy(vtable[imod], val, nc);
				(void) strncpy(ctable[imod], ctl, nc);
				vtable[imod][nc] = '\0';
				ctable[imod][nc] = '\0';
				}
			val += nc;
			ctl += nc;
			imod = -1;
			}
		}

	/* Done - return max list size */
	(void) printf("Parsed string components:\n");
	for (imod=0; imod<nmods; imod++)
		{
		if (blank(ctable[imod])) continue;
		if (blank(vtable[imod])) continue;
		(void) printf("    modifier %d: \"%s\" --> \"%s\"\n", imod,
		   vtable[imod], ctable[imod]);
		}
	(void) printf("    end\n"); (void) fflush(stdout);
	return nmods;
	}

/***********************************************************************
*                                                                      *
*      u p p e r _ c a s e                                             *
*      l o w e r _ c a s e                                             *
*                                                                      *
*      Convert character strings to upper/lower case.                  *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Convert character strings to upper case.
 * @param [in] string string to convert (function may modify this string)
 *********************************************************************/
void   upper_case

	(
	STRING	string
	)

	{
	int		i;

	for (i=0; i<strlen(string); i++)
		string[i] = toupper(string[i]);
	}

/*********************************************************************/
/** Convert character strings to lower case.
 * @param [in] string string to convert (function may modify this string)
 *********************************************************************/
void   lower_case

	(
	STRING	string
	)

	{
	int		i;

	for (i=0; i<strlen(string); i++)
		string[i] = tolower(string[i]);
	}

/***********************************************************************
*                                                                      *
*     a r g _ l i s t                                                  *
*                                                                      *
*     Return the requested argument from the argument list.            *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Lookup the requested argument from the argument list.
 *
 * @param[in]  argc  size of argument list
 * @param[in]  argv[] list of arguments
 * @param[in]  iarg index of requested argument
 * @return Pointer to the argument into the agument list provided.
 *********************************************************************/
STRING	arglist

	(
	int		argc ,
	STRING	argv[] ,
	int		iarg
	)

	{
	static char	Blank[] = "";

	if (iarg < 0) return Blank;
	if (iarg > argc) return Blank;
	return argv[iarg];
	}

/***********************************************************************
*                                                                      *
*     b u i l d _ s t r i n g                                          *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Input the next keystroke from the keyboard and merge it with
* the given string.  Non-printable control characters have no
* effect.  The BACK-SPACE and DEL keys provide an emulation of
* the rub-out feature.
*
* @param[in]  string string to add to (function modfies this string)
* @param[in]  key  key stroke
* @param[in]  maxlen maximum size of string
* @return
* 	 - More (1)   - indicates more keystrokes to follow,
* 	 - Done (0)   - indicates end of text (i.e. newline received),
*	 - Abort (-1) - signals receipt of ESCAPE character - to be
*                   interpretted by calling routine.
 *********************************************************************/

int	build_string

	(
	STRING	string ,
	UNCHAR	key ,
	int		maxlen
	)

	{
	int		len;

	static	const	int		Abort = -1;
	static	const	int		Done  = 0;
	static	const	int		More  = 1;

	/* Determine current length of string */
	len = strlen(string);

	/* Test for various control characters */
	switch (key)
		{
		/* NULL - no effect */
		case '\000':	return More;

		/* ESCAPE - abort */
		case '\033':	return Abort;

		/* RETURN or LINE-FEED - done */
		case '\012':
		case '\015':	return Done;

		/* BACK-SPACE or RUB-OUT - remove a character from the end */
		case '\010':
		case '\177':	if (len > 0) string[--len] = '\0';
						return More;
		}

	/* Non-printable character - complain */
	if (key < '\040') return More;
	if (key > '\177' && key < L'\240') return More;

	/* Printable character - add character to the end */
	if (len >= maxlen) return More;
	string[len++] = key;
	string[len]   = '\0';
	return More;
	}

/***********************************************************************
*                                                                      *
*     f g e t 2 c                                                      *
*     f g e t 3 c                                                      *
*     f g e t 4 c                                                      *
*                                                                      *
*     Read an integer value coded in 2, 3 or 4 bytes.                  *
*                                                                      *
*     d f g e t 4 c                                                    *
*                                                                      *
*     Read a real value coded in 4 bytes.                              *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Read an integer value coded in 2 bytes.
 *
 *	@param[in]	*input	file pointer
 * 	@return value as an int.
 *********************************************************************/
int	fget2c

	(
	FILE	*input
	)

	{
	int		c;

	c  = getc(input) << 8;
	c += getc(input);

	return c;
	}

/*********************************************************************/
/** Read an integer value coded in 3 bytes.
 *
 *	@param[in]	*input	file pointer
 * 	@return value as an long.
 *********************************************************************/
long	fget3c

	(
	FILE	*input
	)

	{
	long	c;

	c  = getc(input) << 16;
	c += getc(input) << 8;
	c += getc(input);

	return c;
	}

/*********************************************************************/
/** Read an integer value coded in 4 bytes.
 *
 *	@param[in]	*input	file pointer
 * 	@return value as an long.
 *********************************************************************/
long	fget4c

	(
	FILE	*input
	)

	{
	long	c;

	c  = getc(input) << 24;
	c += getc(input) << 16;
	c += getc(input) << 8;
	c += getc(input);

	return c;
	}

/*********************************************************************/
/** Read a real value coded in 4 bytes.
 *
 *	@param[in]	*input	file pointer
 * 	@return value as a double.
 *********************************************************************/
double	dfget4c

	(
	FILE	*input
	)

	{
	UNCHAR		ca, cb, cc, cd;
	unsigned	mbits, ebits, sbits;
	double		value, mantissa;
	int			exponent;

	ca = (UNCHAR) getc(input);
	cb = (UNCHAR) getc(input);
	cc = (UNCHAR) getc(input);
	cd = (UNCHAR) getc(input);

	sbits  = GETBIT(ca,7);	/* bit 7 (leftmost) of first btye is sign bit */
	ebits  = ca & 127;		/* bits 6-0 of first byte is the exponent */
	mbits  = cb<<16;		/* remaining 3 bytes form the mantissa */
	mbits += cc<<8;
	mbits += cd;

	mantissa = (double) mbits;
	exponent = ((int) ebits - 64)*4 - 24;
	value    = ldexp(mantissa, exponent);
	if (sbits) value = 0-value;

	return value;
	}

#ifdef STANDALONE

void	main(void)
	{
	char			line[250];
	unsigned long	val;
	LOGICAL			valid;

	(void) printf("Hex-Octal-Decimal Converter\n");
	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter a number (0xnnn hex, \\nnn octal), ");
		(void) printf("or a quoted character: ");
		getfileline(stdin, line, sizeof(line));

		if (blank(line)) exit(0);

		if (same_start_ic(line, "0x"))
				val = ubase_arg(line+2, 16, &valid);
		else if (line[0] == '\\')
				val = ubase_arg(line+1, 8, &valid);
		else if (line[0] == '\'' || line[0] == '"')
				{
				val = (UNCHAR) line[1];
				valid = TRUE;
				}
		else	val = ubase_arg(line, 10, &valid);
		if (!valid)
				{
				(void) printf("Not a valid number\n");
				continue;
				}

		(void) printf("  Hex:      0x%lx\n", val);
		(void) printf("  Octal:    \\%lo\n", val);
		(void) printf("  Decimal:  %ld\n",   val);
		(void) printf("  Unsigned: %lu\n",   val);
		(void) printf("  Char:     '%lc'\n", val);
		}
	}

#endif
