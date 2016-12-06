/***********************************************************************
*                                                                      *
*    i n f o _ f i l e . h                                             *
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

#ifndef INFO_FILE_H
#define INFO_FILE_H

#include <stdio.h>
#include <fpa_types.h>

typedef struct {
	STRING fnam;
	FILE *fp;
	STRING open_state;
	STRING bid;
	STRING blabel;
	long int bpos;
	int ndata;
	STRING data;
} *INFOFILE, INFOFILE_STRUCT;

INFOFILE	info_file_open(STRING fname);
INFOFILE	info_file_open_for_append(STRING fname);
INFOFILE	info_file_create(STRING fname);
void		info_file_close(INFOFILE fd);
void		info_file_rewind(INFOFILE fd);
LOGICAL		info_file_find_block(INFOFILE fd, STRING block_key);
void		info_file_remove_block(INFOFILE fd, STRING block_key);
STRING		info_file_find_next_block(INFOFILE fd);
STRING		info_file_get_block_label(INFOFILE fd);
void		info_file_copy_block(INFOFILE fd2, INFOFILE fd1, STRING block_key);
STRING		info_file_get_data(INFOFILE fd, STRING line_key);
STRING		info_file_get_next_line(INFOFILE fd);
void		info_file_write_block_header(INFOFILE fd, STRING block_key);
void		info_file_write_block_header_with_label(INFOFILE fd,
						STRING block_key, STRING label);
void		info_file_write_line(INFOFILE fd, STRING line_key, STRING data);
void		info_file_replace_line(INFOFILE fd, STRING line_key, STRING data);

#endif
