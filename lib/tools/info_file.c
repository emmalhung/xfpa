/*============================================================================*/
/*
*   These are a series of functions designed to write and read files in the
*   FoG "Info" file format.  All information is divided into blocks and
*   each line item is keyed.  The format is:
*
*        [block_key_1]  <block label>
*        line_key_1=data line
*        line_key_2=.....
*        .
*        [block_key_2]  <block label>
*        .
*
*   The block label is optional. Any leading and trailing spaces are
*   stripped, so if one wants these the label must be enclosed in quotes.
*   The block label may be made multilingual by the use of the language
*   identification feature of the strip_language_tokens() function. See
*   this function for details.
*
*   Any line starting with the charasters # or * is taken as a comment.
*   The functions will also handle the case where there is only one block
*   in a file so that no block key is required.  The functions are:
*
*        INFOFILE info_file_create(STRING file_name)
*
*            create an info type file and assign a handle to it.  If
*            the file cannot be created the return is NULL.
*
*        INFOFILE info_file_open(STRING file_name)
*
*            open an info type file and assign a handle to it.  If
*            the file cannot be created the return is NULL.
*
*        INFOFILE info_file_open_for_append(STRING file_name)
*
*            open an info type file in append mode and assign a handle
*            to it. If the file cannot be created the return is NULL.
*
*        void info_file_close(INFOFILE fd)
*
*            close an info file with handle fd
*
*        void info_file_rewind(INFOFILE fd)
*
*            rewind an info file with handle fd.
*
*        LOGICAL info_file_find_block(INFOFILE fd, STRING block_key)
*
*            find the given block key associated with the file defined
*            by handle fd. The block key search is case insensitive.
*
*        void info_file_remove_block(INFOFILE fd, STRING block_key)
*
*            remove the given block from the file with handle fd.
*
*        STRING info_file_find_next_block(INFOFILE fd)
*
*            find the next block in the info file with handle fd and
*            return the block key.
*
*        STRING info_file_get_block_label(INFOFILE fd)
*
*            return the block label. This function is for convienience as
*            it looks for language tokens and removes any enclosing quotes
*            from the label line.
*
*        void info_file_copy_block(INFOFILE into, INFOFILE from, STRING block_key)
*
*            Copy the given block from the file with handle from
*            into the file with handle into.
*
*        STRING info_file_get_data(INFOFILE fd, STRING line_key)
*
*            Get the data from the currently active block, found with either
*            block find function, associated with the given line key.
*            The line key search is case insensitive. If there is no active
*            block then the data is returned from the first block in the
*            file.
*
*        STRING info_file_get_next_line(INFOFILE fd)
*
*            Get the next data in the currently active block without reference
*            to the line key. The line does not have to have a valid key=
*            entry to be retreived.
*
*        void info_file_write_block_header(INFOFILE fd, STRING block_key)
*
*            write a block header into the info file.
*
*        void info_file_write_line(INFOFILE fd, STRING line_key, STRING line_data)
*
*            write a data line header into the info file.
*
*        void info_file_replace_line(INFOFILE fd, STRING line_key, STRING line_data)
*
*            replace an existing data line, in the currently active block,
*            with new data.
*
*     Version 4.1 (c) Copyright 1996 Environment Canada (AES)
*     Version 5 (c) Copyright 1998 Environment Canada (AES)
*     Version 6 (c) Copyright 2003 Environment Canada (MSC)
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
*/
/*============================================================================*/

#include "info_file.h"
#include "language.h"
#include "parse.h"

#include <fpa_types.h>
#include <fpa_getmem.h>
#include <stdio.h>
#include <ctype.h>

#define COMMENTS "#*"
#define WHITE    " \t\n\r\f"

static STRING IsLineKey(STRING line, STRING key);
static FILE *TmpFileCopy(INFOFILE fd);
static INFOFILE InitInfofileData(STRING fname, FILE *fp, STRING type);
static STRING find_block_label(STRING lptr);

/* If the block key is NULL then the return will be true if the line is
*  identified as a block key of any sort.
*/
static LOGICAL IsBlockKey(STRING line, STRING key)
	{
	int i;

	line += strspn(line,WHITE);
	if(*line != '[') return FALSE;
	if(key)
		{
		line++;
		line += strspn(line,WHITE);
		for(i = 0; i < strlen(key); i++)
			{
			if(toupper(*line) != toupper(key[i])) return FALSE;
			line++;
			}
		line += strspn(line,WHITE);
		if(*line != ']') return FALSE;
		}
	else
		{
			line++;
			if(strchr(line,']') == NULL) return FALSE;
		}
	return TRUE;
	}


static STRING IsLineKey(STRING line, STRING key)
	{
	int i;
	char *ptr;

	line += strspn(line,WHITE);
	if((ptr = strchr(line, '=')) == NULL) return NULL;
	ptr--;
	while(strchr(WHITE,*ptr) != NULL && ptr > line ) ptr--;
	if( strlen(key) != ptr-line+1) return NULL;
	for(i = 0; i < strlen(key); i++)
		{
		if(toupper(line[i]) != toupper(key[i])) return NULL;
		}
	ptr = strchr(line,'=') + 1;
	ptr += strspn(ptr,WHITE);
	return ptr;
	}


static FILE *TmpFileCopy(INFOFILE fd)
	{
	int c;
	FILE *fp;

	if((fp = tmpfile()) == NULL) return NULL;

	if(!fd) return NULL;
	if(*fd->open_state != 'r')
		{
		(void) fclose(fd->fp);
		fd->fp = fopen(fd->fnam, "r");
		if(!fd->fp) return NULL;
		}
	rewind(fd->fp);
	while((c = getc(fd->fp)) != EOF) {(void) putc(c, fp);}
	rewind(fp);
	rewind(fd->fp);
	if(*fd->open_state != 'r')
		{
		(void) fclose(fd->fp);
		fd->fp = fopen(fd->fnam, fd->open_state);
		}
	return fp;
	}


static INFOFILE InitInfofileData(STRING fname, FILE *fp, STRING type)
	{
	INFOFILE ifd;

	ifd = INITMEM(INFOFILE_STRUCT, 1);
	ifd->fnam = INITSTR(fname);
	ifd->fp = fp;
	ifd->open_state = type;
	ifd->bpos = 0;
	ifd->bid = NULL;
	ifd->blabel = NULL;
	ifd->ndata = 127;
	ifd->data = malloc(128);
	return ifd;
	}


static STRING find_block_label(STRING lptr)
	{
	char mbuf[16];
	STRING d, p;

	if(blank(lptr)) return NULL;

	/* Strip leading white space including the "]" bracket */
	(void) strcpy(mbuf, WHITE);
	(void) strcat(mbuf, "]");
	lptr += strspn(lptr, mbuf);

	/* Strip trailing white space */
	p = lptr + strlen(lptr) - 1;
	while(strchr(WHITE, *p) != NULL && p > lptr) p--;
	*(p+1) = '\0';

	strip_language_tokens(lptr);

	/* Remove any enclosing quotes - we want to retain embedded spaces */
	d = strchr("\"'", *lptr);
	if(d != NULL)
		{
		p = strchr(lptr+1, *lptr);
		if (p && *p)
			{
			*p = '\0';
			lptr++;
			}
		}
	return (!blank(lptr)? INITSTR(lptr):NULL);
	}


INFOFILE info_file_open(STRING fname)
	{
	FILE *fp, *fopen();

	if(blank(fname)) return NULL;
	if((fp = fopen(fname, "r")) == NULL) return NULL;
	return InitInfofileData(fname, fp, "r");
	}


INFOFILE info_file_open_for_append(STRING fname)
	{
	FILE *fp, *fopen();

	if(blank(fname)) return NULL;
	if((fp = fopen(fname, "a")) == NULL) return NULL;
	return InitInfofileData(fname, fp, "a");
	}


INFOFILE info_file_create(STRING fname)
	{
	FILE *fp, *fopen();

	if(blank(fname)) return NULL;
	if((fp = fopen(fname, "w")) == NULL) return NULL;
	return InitInfofileData(fname, fp, "w");
	}


void info_file_close(INFOFILE fd)
	{
	if(!fd) return;
	(void) fclose(fd->fp);
	FREEMEM(fd->fnam);
	FREEMEM(fd->bid);
	FREEMEM(fd->blabel);
	FREEMEM(fd->data);
	FREEMEM(fd);
	return;
	}


void info_file_rewind(INFOFILE fd)
	{
    if (!fd) return;
    fd->bpos = 0;
    rewind(fd->fp);
	}


LOGICAL info_file_find_block(INFOFILE fd, STRING block_key)
	{
	char line[1024], *lptr;

	if(!fd) return FALSE;

	FREEMEM(fd->bid);
	FREEMEM(fd->blabel);
	fd->bpos = 0;

	line[1023] = '\0';
	rewind(fd->fp);
	while(lptr = getvalidline(fd->fp,line,1023,COMMENTS))
		{
		if(!IsBlockKey(lptr,block_key)) continue;
		fd->bid = INITSTR(block_key);
		fd->bpos = ftell(fd->fp);
		fd->blabel = find_block_label(lptr);
		return TRUE;
		}
	return False;
	}


/* It does not make any sense for this function to operate on
*  an info file opened for write.
*/
void info_file_remove_block(INFOFILE fd, STRING block_key)
	{
	LOGICAL  copyline;
	char line[2048];
	STRING lptr;
	FILE *fp, *fopen();

	if(!fd) return;
	if(*fd->open_state == 'w') return;

	/* If we can not open a temporary file return.
	*/
	if((fp = TmpFileCopy(fd)) == NULL) return;

	(void) fclose(fd->fp);
	fd->fp = fopen(fd->fnam, "w");

	line[2047] = '\0';
	copyline = TRUE;
	while(lptr = getvalidline(fp,line,2047,COMMENTS))
		{
		if(IsBlockKey(lptr,block_key))
			{
			copyline = FALSE;
			}
		else if(copyline)
			{
			(void) fputs(line, fd->fp);
			(void) fputs("\n", fd->fp);
			}
		else if(IsBlockKey(lptr,NULL))
			{
			copyline = TRUE;
			(void) fputs(line, fd->fp);
			(void) fputs("\n", fd->fp);
			}
		}
	(void) fclose(fp);
	(void) fclose(fd->fp);
	fd->fp = fopen(fd->fnam, fd->open_state);
	}


STRING info_file_find_next_block(INFOFILE fd)
	{
	char   line[1024];
	STRING ptr, lptr;

	if(!fd) return NULL;

	(void) fseek(fd->fp, fd->bpos, SEEK_SET);
	line[1023] = '\0';
	while(lptr = getvalidline(fd->fp,line,1023,COMMENTS))
		{
		if(!IsBlockKey(lptr,NULL) || fd->bpos == ftell(fd->fp)) continue;
		fd->bpos = ftell(fd->fp);
		lptr = strchr(lptr,'[') + 1;
		lptr += strspn(lptr,WHITE);
		ptr = strchr(lptr,']') - 1;
		while(strchr(WHITE,*ptr) != NULL && ptr > lptr) ptr--;
		*(ptr+1) = '\0';
		FREEMEM(fd->bid);
		FREEMEM(fd->blabel);
		fd->bid = INITSTR(lptr);
		fd->blabel = find_block_label(ptr+2);
		return fd->bid;
		}
	return NULL;
	}


STRING info_file_get_block_label(INFOFILE fd)
	{
	if(!fd) return " ";
	return fd->blabel ? fd->blabel : " ";
	}


void info_file_copy_block(INFOFILE fd2, INFOFILE fd1, STRING block_key)
	{
	LOGICAL found;
	char line[2048], *lptr;

	if(!fd1 || !fd2) return;

	found = FALSE;
	line[2047] = '\0';
	rewind(fd1->fp);
	while(lptr = getvalidline(fd1->fp,line,2047,COMMENTS))
		{
		if(!IsBlockKey(lptr,block_key)) continue;
		found = TRUE;
		break;
		}
	if(!found) return;

	if(*fd2->open_state == 'r')
		{
		(void) fclose(fd2->fp);
		fd2->fp = fopen(fd2->fnam, "a");
		}
	info_file_write_block_header(fd2, block_key);
	while(lptr = getvalidline(fd1->fp,line,2047,COMMENTS))
		{
		if(IsBlockKey(lptr,NULL)) break;
		(void) fputs(lptr, fd2->fp);
		(void) fputs("\n", fd2->fp);
		}
	if(*fd2->open_state == 'r')
		{
		(void) fclose(fd2->fp);
		fd2->fp = fopen(fd2->fnam, "r");
		}
	}


/* When reading lines allow a backslash at the end of a line to
*  indicate continuation of the line.
*/
STRING info_file_get_data(INFOFILE fd, STRING line_key)
	{
	int n;
	char line[2048];
	STRING ptr, lptr;
	LOGICAL line_continue;

	if(!fd) return "";

	if (fd->bid)
		(void) fseek(fd->fp, fd->bpos, SEEK_SET);
	else
		rewind(fd->fp);

	fd->data[0] = '\0';
	line[2027] = '\0';
	line_continue = FALSE;

	while(lptr = getvalidline(fd->fp,line,2027,COMMENTS))
		{
		if(IsBlockKey(lptr,NULL)) break;
		if(!line_continue)
			{
			if((lptr = IsLineKey(line, line_key)) == NULL) continue;
			if(blank(lptr)) break;
			}
		n = strlen(fd->data) + esc_decode_len(lptr);
		if(n > fd->ndata)
			{
			fd->ndata = n;
			fd->data = GETMEM(fd->data, char, fd->ndata+1);
			}
		(void) strcat(fd->data, esc_decode(lptr));
		ptr = fd->data + strlen(fd->data) - 1;
		line_continue = (*ptr == '\\');
		if(!line_continue) break;
		*ptr = '\0';
		}
	(void) no_white(fd->data);
	return fd->data;
	}


/* This function will return a blank line when it runs out of
*  lines to read in the currently active block.  When reading
*  lines allow a backslash at the end of a line to indicate
*  continuation of the line.
*/
STRING info_file_get_next_line(INFOFILE fd)
	{
	int n;
	char line[2048];
	STRING ptr, lptr;

	if(!fd) return "";

	fd->data[0] = '\0';
	line[2027] = '\0';

	while(lptr = getvalidline(fd->fp,line,2027,COMMENTS))
		{
		if(IsBlockKey(lptr,NULL)) break;
		n = strlen(fd->data) + esc_decode_len(lptr);
		if(n > fd->ndata)
			{
			fd->ndata = n;
			fd->data = GETMEM(fd->data, char, fd->ndata+1);
			}
		(void) strcat(fd->data, esc_decode(lptr));
		ptr = fd->data + strlen(fd->data) - 1;
		if(*ptr != '\\') break;
		*ptr = '\0';
		}
	(void) no_white(fd->data);
	return fd->data;
	}


void info_file_write_block_header(INFOFILE fd, STRING block_key)
	{
	if(!fd) return;

	(void) fputs("[",       fd->fp);
	(void) fputs(block_key, fd->fp);
	(void) fputs("]",       fd->fp);
	(void) fputs("\n",      fd->fp);
	}


void info_file_write_block_header_with_label(INFOFILE fd, STRING block_key, STRING label)
	{
	if(!fd) return;

	(void) fputs("[",        fd->fp);
	(void) fputs(block_key,  fd->fp);
	(void) fputs("]",        fd->fp);
	(void) fputs(" ",        fd->fp);
	(void) fputs(label,      fd->fp);
	(void) fputs("\n",       fd->fp);
	}


void info_file_write_line(INFOFILE fd, STRING line_key, STRING data)
	{
	if(!fd) return;

	(void) fputs(line_key,         fd->fp);
	(void) fputs("=",              fd->fp);
	(void) fputs(esc_encode(data), fd->fp);
	(void) fputs("\n",             fd->fp);
	}


/* It does not make any sense for this function to operate on
*  an info file opened for write.
*/
void info_file_replace_line(INFOFILE fd, STRING line_key, STRING data)
	{
	LOGICAL at_block;
	char line[2048];
	STRING lptr;
	FILE *fp, *fopen();

	if(!fd) return;
	if(*fd->open_state == 'w') return;

	if((fp = TmpFileCopy(fd)) == NULL) return;

	(void) fclose(fd->fp);
	fd->fp = fopen(fd->fnam, "w");
	line[2047] = '\0';
	at_block = FALSE;
	while(lptr = getvalidline(fp,line,2047,COMMENTS))
		{
		if(IsBlockKey(lptr,NULL)) at_block = FALSE;
		if(IsBlockKey(lptr,fd->bid)) at_block = TRUE;
		if(at_block && IsLineKey(line, line_key))
			{
			at_block = FALSE;
			info_file_write_line(fd, line_key, data);
			}
		else
			{
			(void) fputs(line, fd->fp);
			(void) fputs("\n", fd->fp);
			}
		}
	(void) fclose(fp);

	(void) fclose(fd->fp);
	fd->fp = fopen(fd->fnam, fd->open_state);
	}
