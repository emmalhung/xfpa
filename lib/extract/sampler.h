/**********************************************************************/
/** @file sampler.h
 *
 *  Header file for sampler.a (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*  File:        sampler.h                                              *
*                                                                      *
*  Purpose:     Header file for sampler.a                              *
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

#ifndef SAMPLERDEFS
#define SAMPLERDEFS

#include "fpa_types.h"
#include "fpa_macros.h"

/* Inter-process communications channel identifier name */
#define CHNAME "FS0"

typedef enum
	{
		SAMP_SETUP,
		SAMP_SOURCE,
		SAMP_RTIME,
		SAMP_TPLUS,
		SAMP_FIELD,
		SAMP_POINT,
		SAMP_EVALUATE
	} SAMPQUERY;

typedef enum
	{
		SAMP_INVALID,
		SAMP_VALID
	} SAMPREPLY;

#undef GLOBAL
#ifdef SAMPLER_INIT
#	define GLOBAL GLOBAL_INIT
#else
#	define GLOBAL GLOBAL_EXTERN
#endif

/* Return values from most functions in this module */
GLOBAL(const int, SUCCESS,  0);
GLOBAL(const int, FAILURE, -1);
GLOBAL(const int, PROBLEM, -2);

/* Functions in sampler_access.c */
int		fpa_sampler_connect(STRING setup_file);
int		fpa_sampler_disconnect(void);
int		fpa_sampler_setup(STRING setup_file);

int		fpa_sampler_source(STRING source, STRING subsrc);

int		fpa_sampler_rtime(int year, int jday, int hour);
int		fpa_sampler_avail_rtime(int **year, int **jday, int **hour);

int		fpa_sampler_tplus(int nprog, const int *progs);
int		fpa_sampler_empty_tplus(void);
int		fpa_sampler_add_tplus(int prog);
int		fpa_sampler_avail_tplus(int **progs);

int		fpa_sampler_field(int nfld, const STRING *elems, const STRING *levels);
int		fpa_sampler_empty_field(void);
int		fpa_sampler_add_field(STRING elem, STRING level);
int		fpa_sampler_avail_field( STRING **, STRING **);

int		fpa_sampler_point(int npt, const STRING *lats, const STRING *lons);
int		fpa_sampler_empty_point(void);
int		fpa_sampler_add_point(STRING lat, STRING lon);

int		fpa_sampler_evaluate(void);
STRING	fpa_sampler_get_value(int iprog, int ifld, int ipt);

/* Functions in sampler_old.c retained for compatibility */
int		sampler_connect(STRING setup_file);
int		sampler_disconnect(void);
int		sampler_setup(STRING setup_file);
int		sampler_sequence(STRING dir, int year, int jday, int hour, int maxprog);
int		sampler_prog_time(int ichart);
int		sampler_evaluate(STRING elem, STRING level, int lat, int lon);
STRING	sampler_get_value(int ichart);

/* Functions in sampler_codes.c */
int			SAMPQueryCode(SAMPQUERY query);
SAMPQUERY	SAMPQuery(int code);
int			SAMPReplyCode(SAMPREPLY reply);
SAMPREPLY	SAMPReply(int code);

#endif
