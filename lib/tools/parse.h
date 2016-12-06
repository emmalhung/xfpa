/**********************************************************************/
/** @file parse.h
 *
 *  Routines to parse STRING objects with (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*    p a r s e . h                                                     *
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

#include <fpa_types.h>
#include <stdio.h>

LOGICAL	flush_line (FILE *fp);
STRING	getword (FILE *fp, STRING word, size_t ncl);
STRING	getvalidline (FILE *fp, STRING line, size_t ncl, STRING comment);
STRING	getfileline (FILE *fp, STRING line, size_t ncl);
void	ungetfileline (FILE *fp);

STRING	esc_encode(const STRING fbuf);
STRING	esc_decode(const STRING fbuf);
int		esc_encode_len(const STRING fbuf);
int		esc_decode_len(const STRING fbuf);

STRING	strcpy_arg (STRING str, STRING line, LOGICAL *status);
STRING	strncpy_arg (STRING str, size_t len, STRING line, LOGICAL *status);
STRING	strdup_arg (STRING line);
STRING	strenv_arg (STRING line);
STRING	string_arg (STRING line);
STRING	opt_arg (STRING line, STRING *name, STRING *value);
STRING	strrem_arg (STRING line);
int		int_arg (STRING line, LOGICAL *status);
long	long_arg (STRING line, LOGICAL *status);
long	hex_arg (STRING line, LOGICAL *status);
long	octal_arg (STRING line, LOGICAL *status);
long	base_arg (STRING line, int base, LOGICAL *status);
UNLONG	ubase_arg (STRING line, int base, LOGICAL *status);
float	float_arg (STRING line, LOGICAL *status);
double	double_arg (STRING line, LOGICAL *status);
LOGICAL	logical_arg (STRING line, LOGICAL *status);
LOGICAL	yesno_arg (STRING line, LOGICAL *status);

STRING	fixed_string_arg (STRING line, size_t nc);
int		fixed_int_arg (STRING line, size_t nc, LOGICAL *status);
long	fixed_long_arg (STRING line, size_t nc, LOGICAL *status);
long	fixed_hex_arg (STRING line, size_t nc, LOGICAL *status);
long	fixed_octal_arg (STRING line, size_t nc, LOGICAL *status);
long	fixed_base_arg (STRING line, size_t nc, int base, LOGICAL *status);
float	fixed_float_arg (STRING line, size_t nc, LOGICAL *status);
double	fixed_double_arg (STRING line, size_t nc, LOGICAL *status);

STRING	strtok_arg (STRING line);
int		inttok_arg (STRING line, LOGICAL *status);
long	longtok_arg (STRING line, LOGICAL *status);
float	floattok_arg (STRING line, LOGICAL *status);
double	doubletok_arg (STRING line, LOGICAL *status);

LOGICAL	same (STRING string1, STRING string2);
LOGICAL	same_ic (STRING string1, STRING string2);
LOGICAL	same_start (STRING string1, STRING string2);
LOGICAL	same_start_ic (STRING string1, STRING string2);
LOGICAL	match (STRING string1, STRING string2);
LOGICAL	match_ic (STRING string1, STRING string2);
LOGICAL	blank (STRING string);
void	no_white (STRING string);

int		int_string (int ival, STRING a, size_t nca);
STRING	fformat (float val, int dmax);
float	fround (float val, int dmax);
LOGICAL	fcompare (float val1, float val2, float range, float portion);
int		ndigit (int ival);
int		fdigit (double val);
float	range_norm(float val, float vmin, float vmax, int *carry);

LOGICAL	parse_option (STRING option, STRING *name, STRING *value);
int		parse_mstring (STRING val, STRING ctl, STRING *vlist, STRING *clist);

void	upper_case (STRING string);
void	lower_case (STRING string);

STRING	arglist (int argc, STRING argv[], int iarg);

int		build_string (STRING string, UNCHAR key, int maxlen);

int		fget2c (FILE *input);
long	fget3c (FILE *input);
long	fget4c (FILE *input);
double	dfget4c (FILE *input);
