/*********************************************************************/
/** @file language.c
 *
 * Language dependence library
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*    l a n g u a g e . c                                               *
*                                                                      *
*    Language dependence library                                       *
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

#include "language.h"
#include "message.h"
#include "parse.h"
#include "string_ext.h"

#include <fpa_types.h>
#include <fpa_getmem.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Use white space as normal delimiters */
#define WHITE " \t\n\r\f"

static STRING full_key     = (STRING)NULL;
static STRING lang_key     = (STRING)NULL;
static STRING start_chars  = "<*";
static STRING end_chars    = "*>";
static STRING default_key  = "<*default*>";
static STRING locale_reset = (STRING)NULL;
static int    locale_count = 0;



/***********************************************************************
*
*     s e t _ l o c a l e _ f r o m _ e n v i r o n m e n t
*     r e s e t _ l o c a l e
*
***********************************************************************/
/**********************************************************************/
/**     Set the current locale category from the environment.
 *     LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC,
 *     LC_TIME and LC_MESSAGES. See setlocale for details.
 *
 *     The category parameter is the standard setlocale ones:
 *     LC_ALL, LC_COLLATE, LC_TYPE, LC_MESSAGES, LC_MONETARY,
 *     LC_NUMERIC and LC_TIME. See setlocale for details.
 *
 *     The original locale is saved the first time into this function
 *     as it is assumed that this will always be the default to reset
 *     to. These functions must be called in pairs. If this is not done
 *     then the second and subsequent set_locale_from_environment
 *     calls will be ignored.
 *
 *     @param[in]	category	locale category
 **********************************************************************/

void set_locale_from_environment(int category)
{
	if(!locale_reset)
		locale_reset = safe_strdup(setlocale(LC_ALL, NULL));

	locale_count++;
	if(locale_count < 2)
	{
		if(category)
			(void) setlocale(category, "");
		else
			(void) setlocale(LC_ALL, "");
	}
}

/**********************************************************************/
/**     Resets the current locale category from the environment.
 *     LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC,
 *     LC_TIME and LC_MESSAGES. See setlocale for details.
 *
 *     The category parameter is the standard setlocale ones:
 *     LC_ALL, LC_COLLATE, LC_TYPE, LC_MESSAGES, LC_MONETARY,
 *     LC_NUMERIC and LC_TIME. See setlocale for details.
 *
 *     The original locale is saved the first time into this function
 *     as it is assumed that this will always be the default to reset
 *     to. These functions must be called in pairs. If this is not done
 *     then the second and subsequent set_locale_from_environment
 *     calls will be ignored.
 **********************************************************************/
void reset_locale(void)
{
	locale_count--;
	if(locale_count < 1)
	{
		locale_count = 0;
		(void) setlocale(LC_ALL, locale_reset);
	}
}





/***********************************************************************
*
*     s e t _ l a n g u a g e _ t o k e n
*
***********************************************************************/

/*********************************************************************/
/** Set the language token to be used by the strip_language_tokens
 *  function below.
 *
 *  @param[in]	key	
 *********************************************************************/
void set_language_token(STRING key)
{
	char	mbuf[256];

	if(!same(full_key,default_key)) FREEMEM(full_key);
	FREEMEM(lang_key);

	if(blank(key))
		{
		full_key = default_key;
		}
	else
		{
		/* Construct the current language key */
		(void) safe_strcpy(mbuf, start_chars);
		(void) safe_strcat(mbuf, key);
		(void) safe_strcat(mbuf, end_chars);
		full_key = strdup(mbuf);

		/* Construct the "language part" only key */
		if(key = strpbrk(mbuf,"_."))
			{
			*key = '\0';
			(void) safe_strcat(mbuf, end_chars);
			lang_key = strdup(mbuf);
			}
		}
	pr_diag("[FPA Language]","Full key = %s  Lang key = %s\n", full_key, lang_key);
}


/***********************************************************************
*                                                                      *
*     s t r i p _ l a n g u a g e _ t o k e n s                        *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Scans the line for language dependent tokens.
 *
 * Whenever a token is to be language specific is must be preceeded
 * by the token <*lang*> where lang is the language string which
 * will be set in the full_key or lang_key. If these keys
 * have not been set by the set_language_token() function then they
 * will be set to <*default*>. The full_key is the key as given.
 * The lang_key is the language part of the full_key. This assumes
 * the standard input form: language[_territory[.codeset]]
 * For example fr_CA.iso8821
 *
 * If we had more than one language associated with a given key
 * the line could like:  <*en*> Yes <*fr*> Oui and the function
 * would remove all tokens but "Yes" from line if the language was
 * english and all but "Oui" if the language was french. One
 * language key should be specified as <*default*>.  This will
 * ensure that a language token is found no matter what. Eg.
 *
 * <*default*> Yes <*fr*> Oui
 *
 * In this case the token "Yes" will be retained if the environment
 * language is anything but french.
 *
 * If no keys are found in the line then the line is returned
 * unmodified. This means that if only one language is present the
 * <*default*> key is not required.
 *
 * @param[in]	line
 *********************************************************************/
void	strip_language_tokens(STRING line)
	{
	int		nc;
	STRING	key, l1, l2, l3;

	if (!full_key) set_language_token(NULL);

	/* If there is a current language key in the given line we use it.*/
	/* If not use the default key setting */
	if((l1 = strstr(line,start_chars)) == (STRING)NULL)
		return;
	else if(strstr(line,full_key) != (STRING)NULL)
		key = full_key;
	else if(lang_key != (STRING)NULL && strstr(line,lang_key) != (STRING)NULL)
		key = lang_key;
	else
		key = default_key;

	do {
		/* If this is the key strip it - otherwise strip it and */
		/* the following token as it is for another language.   */
		if( strncmp(l1,key,strlen(key)) == 0 )
			{
			nc = strlen(key);
			}
		else
			{
			/* Find the other end of the key - if not found bail out */
			l3 = strstr(l1,end_chars);
			if (!l3) break;

			/* Find the beginning of the next token */
			l3 += 2;
			l2 = l3 + strspn(l3,WHITE);

			/* Find other end of the token - balance quotes if present */
			nc = 0;
			if (*l2 == '"')
				{
				if(l3 = strchr(l2+1,'"'))
					{
					nc = l3 - l1 + 1;
					l2++;
					}
				}
			else if (*l2 == '\'')
				{
				if(l3 = strchr(l2+1,'\''))
					{
					nc = l3 - l1 + 1;
					l2++;
					}
				}
			else
				{
				nc = strcspn(l2,WHITE);
				nc += l2 - l1;
				}
			}

		/* Make sure that we found something then remove tokens */
		if(nc > 0)
			{
			l2 = l1 + nc;
			memmove(l1, l2, strlen(l2)+1);
			}
		} while(l1 = strstr(line,start_chars));
	}
