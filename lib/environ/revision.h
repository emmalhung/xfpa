/**********************************************************************/
/** @file revision.h
 *
 *  Routines to handle revision codes (include file)
 *
 *  Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r e v i s i o n . h                                                *
*                                                                      *
*   Routines to handle revision codes (include file)                   *
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
#ifndef REVISION_DEFS
#define REVISION_DEFS

#include <fpa_types.h>

/***********************************************************************
*                                                                      *
*  Initialize defined constants for revison routines                   *
*                                                                      *
************************************************************************/

extern	const	STRING	FpaRevision;
extern	const	STRING	FpaRevLabel;
extern	const	STRING	AuroraRevLabel;
extern	const	STRING	FpaOldestRev;

/***********************************************************************
*                                                                      *
*  Define revision-dependent features                                  *
*                                                                      *
*  The first member of RevTbl is the feature identifier.               *
*  The second member is the earliest revision to support that feature. *
*                                                                      *
***********************************************************************/

#define	FPA_REV_NONE "none"
#define	FPA_REV_REVNUM "revision numbers"
#define	FPA_REV_VLIST "value lists"
#define	FPA_REV_NEWCFG "new config"
#define	FPA_REV_VSTRUCT "value struct"

/***********************************************************************
*                                                                      *
*  Declare external functions in revision.c                            *
*                                                                      *
***********************************************************************/

LOGICAL	same_revision(STRING rev1, STRING rev2);
LOGICAL	newer_revision(STRING rev1, STRING rev2);
LOGICAL	older_revision(STRING rev1, STRING rev2);
int		compare_revision(STRING rev1, STRING rev2);
LOGICAL	valid_revision(STRING rev);
LOGICAL	parse_revision(STRING rev, int *version, int *release, char *letter,
						STRING *comment);
LOGICAL	revision_feature(STRING feature, STRING rev);


/* Now it has been included */
#endif
