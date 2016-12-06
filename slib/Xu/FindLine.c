/****************************************************************************/
/*
*  File:     XuFindKeyLine.c
*
*  Functions: XuFindKeyLine()
*             XuGetMdbLine()
*
*  Notes:    These functions will handle message database files with #include
*            directives in them. The syntax must be #include file_name.
*            The file given by file_name should be in the same directory as
*            the message database file. This can be used to split up the
*            message database into several files.
*
*  XuFindKeyLine(key, type, error)
*
*            Given a key string and a type string find the first line in
*            the message database file which starts with "key.type:" and
*            return the remainder of the line.  If the line is terminated
*            with a backslash ("\") then the line is assumed to continue
*            on the next line. All leading spaces are eliminated from the
*            first and any continuation lines. If an error is encounterd
*            then the string error is returned. The returned string must
*            not be freed as it points to an internal static.
*
*  XuGetMdbLine(key, type)
*
*            This function uses the XuFindKeyLine() function and allocates
*            memory to the returned string. It is the responsibility of
*            the calling function to free the memory.
*
*  XuVaGetMdbLine(key, type, ...)
*
*            Similar to the above function  The line found can have
*            embedded format commands in the printf style.  The variable
*            argument list must contain the variables corresponding to
*            the format string and must terminate with a NULL. If an
*            error is detected then key is returned.  It is the
*            responsibility of the calling function to free the memory.
*
*     Version 8 (c) Copyright 2011 Environment Canada
*
*   This file is part of the Forecast Production Assistant (FPA).
*   The FPA is free software: you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   The FPA is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*   See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************
*
* History: 2013.07.22 - Bug fix. Multiline messages were not parsed correctly.
*/
/****************************************************************************/

#include <stdarg.h>
#include "XuP.h"

/* This defines the number of keywords to store in memory to reduce the
 * amount of disk access required.
 */
#define NKEYS 100


String XuGetMdbLine(String key, String type)
{
	char *ptr;
	static char errstr[] = ".-?-.";

	ptr = XuFindKeyLine(key, type, errstr);
	if(same(ptr, errstr)) return XtNewString(key);
	return XtNewString(ptr);
}


String XuVaGetMdbLine(String key, String type, ...)
{
	char mbuf[2048], *ptr;
	va_list args;

	static char errstr[] = ".-?-.";

	ptr = XuFindKeyLine(key, type, errstr);
	if(same(ptr, errstr)) return XtNewString(key);
    va_start(args, type);
    (void) vsnprintf(mbuf, 2048, ptr, args);
    va_end(args);
    return XtNewString(mbuf);
}


/* Recursive function to read a file with a given name into the given temporary
*  file. Handles include directive.
*/
static void ReadMdbFile( String fname, FILE *mfp )
{
	int    n;
	char   line[256];
	String msgDb, ptr, sp;
	FILE   *fp, *fopen();

	if( (msgDb = XuFindFile(fname)) == NULL )
	{
		(void) fprintf(stderr,"XuFindKeyLine: Message database \"%s\" not found.\n", fname);
	}
	else if( (fp = fopen(msgDb,"r")) == (FILE *)NULL )
	{
		(void) fprintf(stderr,"XuFindKeyLine: Unable to open Message database \"%s\".\n", fname);
	}
	else
	{
		while(fgets(line, 256, fp))
		{
			n = strcspn(line, "\n\r\f");
			if( n < 256 ) line[n] = '\0';                /* remove ending lf */
			ptr = line + strspn(line," \t\n\r\f");       /* Strip leading space */
			if(blank(ptr)) continue;                     /* remove blank lines */
			if(*ptr == '!') continue;                    /* remove comment lines */

			if(strncasecmp(ptr,"#include ",9))                 /* if not an include directive */
			{
				sp = ptr + (int) safe_strlen(ptr) - 1;              /* find last char */
				while(*sp == ' ') sp--;                  /* find last non-blank char */
				if(*sp == '\\')                          /* is it a continuation char? */
				{
					*sp = '\0';                          /* remove continuation char */
					(void) fputs(line,mfp);                     /* no blank strip if continue */
				}
				else
				{
					(void) fputs(ptr,mfp);
					(void) fputs("\n", mfp);                    /* line termination */
				}
			}
			else
			{
				ptr += 9;
				ptr = ptr + strspn(ptr," \t\n\r\f");     /* Strip leading space */
				sp = ptr + (int) safe_strlen(ptr) - 1;              /* find last char */
				while(*sp == ' ') { *sp = '\0'; sp--; }  /* Strip traiing space */
				ReadMdbFile(ptr, mfp);
			}
		}
		(void) fclose(fp);
	}
	XtFree(msgDb);
}


/* The retreived keys are kept in a memory array. Each time a key is accessed it is
 * moved to the top of the stack. This way the most often used ones tend to remain
 * in memory and the least used will eventually fall off the bottom of the stack.
 */
String XuFindKeyLine(String key, String type, String errstr)
{
	int     i;
	char    keystr[128], line[2048];
	String  ptr, spt;

	static String *keys = NULL;
	static String *data = NULL;
	static int    nkeys = 0;

	if (!keys)
	{
		nkeys = NKEYS - 1;
		keys  = (String *)calloc(NKEYS,sizeof(String));
		data  = (String *)calloc(NKEYS,sizeof(String));
	}

	(void) safe_strcpy(keystr, key);
	(void) safe_strcat(keystr, ".");
	(void) safe_strcat(keystr, type);

	for(i = nkeys; i >= 0; i--)
	{
		if(!keys[i]) break;
		if(!same(keystr,keys[i])) continue;
		if(i < nkeys)
		{
			ptr = keys[i];
			spt = data[i];
			(void) memmove(&keys[i], &keys[i+1], (size_t)(nkeys-i)*sizeof(String));
			(void) memmove(&data[i], &data[i+1], (size_t)(nkeys-i)*sizeof(String));
			keys[nkeys] = ptr;
			data[nkeys] = spt;
		}
		return data[nkeys];
	}

	/* If the combined message database file does not exist yet create it.
	*/
	if(!Fxu.mdbfp)
	{
		Fxu.mdbfp = tmpfile();
		(void) safe_strcpy(line, Fxu.app_class);
		(void) safe_strcat(line, XuMESSAGE_DB_SUFFIX);
		ReadMdbFile(line, Fxu.mdbfp);
	}

	/* Find the message in the message database file.
	*/
	rewind(Fxu.mdbfp);
	while((ptr = fgets(line, 2048, Fxu.mdbfp)) != NULL)
	{
		String s;
		line[2047] = '\0';                             /* so i'm paranoid */
		spt = strchr(ptr,':');                         /* find keyword separator */
		if (!spt) continue;                            /* paranoid again */
		*spt = '\0';
		s = spt - 1;
		while(*s == ' ' && s > ptr) {*s = '\0'; s--;}  /* remove training blanks */
		if(!same(ptr, keystr)) continue;               /* test keyword */
		ptr = spt + 1;
		ptr += strspn(ptr, " \t\n\r\f");               /* remove leading blanks */
	    ptr[strcspn(ptr, "\n\r\f")] = '\0';            /* remove terminating '\n' */
		while((spt = strstr(ptr, "\\n")))              /* replace "\n" with '\n' char */
		{
			*spt++ = '\n';
			*spt++ = '\0';
			spt += strspn(spt, " \t\n\r\f");            /* remove leading blanks before */
			/* Bug 2013.07.22 safe_strlen(ptr) missing */
			memmove(ptr+safe_strlen(ptr), spt, safe_strlen(spt)+1);
		}
		break;
	}

	XtFree(keys[0]);
	XtFree(data[0]);

	(void) memmove(keys, keys+1, nkeys*sizeof(String));
	(void) memmove(data, data+1, nkeys*sizeof(String));

	keys[nkeys] = XtNewString(keystr);
	data[nkeys] = XtNewString((ptr != NULL)? ptr:errstr);

	return data[nkeys];
}
