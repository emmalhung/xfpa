/*========================================================================*/
/*
*	File:		source.h
*
*   Purpose:    Header for source.c
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
*/
/*========================================================================*/
#ifndef _SOURCE_H
#define _SOURCE_H

#if defined(MACHINE_PCLINUX)
	#include <sys/inotify.h>	/* inotify system header */
	#define WTYPE uint32_t
#else							/* these satisfy the compiler */
	#define WTYPE int
	struct inotify_event{ int wd,mask,cookie,len; char name[];};
#endif
/*
 * If the source is defined to be equivalent to another source we set a key
 *  bit in the type key. If we want to use the source the key must then be
 *  masked in order to be "visible" to the selection process.
 */
#define SRC_HIDDEN	1L
/*
 * These are the source type keys. Note that the SRC_ALL key does NOT
 *  include SRC_BACKUP, SRC_IMAGERY or SRC_MAPS as these are special cases.
 */
#define SRC_DEPICT	(1L<<1)
#define SRC_INTERP	(1L<<2)
#define SRC_BACKUP	(1L<<3)
#define SRC_IMPORT	(1L<<4)
#define SRC_FPA		(1L<<5)
#define SRC_NWP 	(1L<<6)
#define SRC_ALLIED	(1L<<7)
#define SRC_IMAGERY	(1L<<8)
#define SRC_MAPS	(1L<<9)
#define SRC_RADAR_STAT (1L<<10)
#define SRC_ALL		(SRC_DEPICT|SRC_INTERP|SRC_FPA|SRC_NWP|SRC_ALLIED)
/*
 * This key means that any source in a returned list must contain data
 */
#define SRC_HAS_DATA    (1L<<15)
/*
 * This is used by SourceListByType() by or'ing it with the above to
 *  indicate that only the most recent run time is to be considered.
 */
#define SRC_LAST_RUN_ONLY (1L<<16)
/*
 * This is the above two combined
 */
#define SRC_LAST_RUN_DATA	SRC_HAS_DATA|SRC_LAST_RUN_ONLY
/*
 * Utility defines.
 */
#define SrcDef(s)			(s)->fd->sdef
#define SrcSubDef(s)		(s)->fd->subdef
#define SrcLabel(s)			(s)->fd->subdef->label
#define SrcShortLabel(s)	(s)->fd->subdef->sh_label
#define SrcName(s)			(s)->fd->sdef->name
#define SrcSubName(s)		(s)->fd->subdef->name
#define SrcSubDashName(s)	(blank(SrcSubName(s)))?"-":SrcSubName(s)

#define MAXSRCPARM	5

typedef struct _source {
	long           type;             /* source type and keys (if any) from above defines */
	FLD_DESCRIPT   *fd;              /* field descriptor to hold source info */
	String         parms[MAXSRCPARM];/* auxillary parameters from setup file */
	String         dir;              /* directory where source data is stored */
	String         ancestor;         /* closest ancestor directory to dir if dir does not exist */
	int            wd;               /* inotify descriptor */
	WTYPE          watch;            /* the type of watch on the directory */
	time_t         last_mod_time;    /* time source last modified */
	time_t         notify_delta;     /* only notify user when time difference greater than this */
	Boolean        modified;         /* source modified since last checked */
	Boolean        isdata;           /* is there any data in the directory? */
	Boolean        force_notify;     /* force source observer notification */
} SourceStruct,    *Source, **SourceList;

extern Source  FindSourceByName	 (String, String);
extern void    SourceListByType	 (long, FpaCtimeDepTypeOption, SourceList*, int*);
extern void    SourceListByField (long, FpaConfigFieldStruct*, SourceList*, int*);
extern void    LoadSourceData	 (void);
extern int     AddFileWriteCloseWatch (String, void (*notifyFcn)(XtPointer), XtPointer);
extern void    RemoveFileWatch   (int);

#endif
