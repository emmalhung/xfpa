/**********************************************************************/
/** @file revision.c
 *
 *   Routines to control and test software features related to the
 *   current software revision and the revision codes of setup and
 *   config files.
 *
 *   Version 8 &copy; Copyright 2011 Environment Canada
 *
 **********************************************************************/
/***********************************************************************
*                                                                      *
*   r e v i s i o n . c                                                *
*                                                                      *
*   Routines to control and test software features related to the      *
*   current software revision and the revision codes of setup and      *
*   config files.                                                      *
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

#define REVISION_INIT
#include "revision.h"

#include <tools/tools.h>
#include <fpa_macros.h>

#include <string.h>
#include <ctype.h>

/***********************************************************************
*                                                                      *
*  Initialize defined constants for revison routines                   *
*                                                                      *
************************************************************************/

/* Define current software revision code */
GLOBAL_INIT(const STRING, RevMark1,       "\n[Rev:]");
GLOBAL_INIT(const STRING, FpaRevision,    "8.1");
GLOBAL_INIT(const STRING, RevMark2,       "\n[RevLabel:]");
GLOBAL_INIT(const STRING, FpaRevLabel,    "V8.1");
GLOBAL_INIT(const STRING, RevMark3,       "\n");

/* Define oldest supported revision code */
/* (prior to putting revision codes in config and setup files) */
GLOBAL_INIT(const STRING, FpaOldestRev, "3.5");


/***********************************************************************
*                                                                      *
*  Define revision-dependent features                                  *
*                                                                      *
*  The first member of RevTbl is the feature identifier.               *
*  The second member is the earliest revision to support that feature. *
*                                                                      *
***********************************************************************/

static	TABLE	RevTbl[] =	{ { FPA_REV_NONE,    "3.5" },
							  { FPA_REV_REVNUM,  "3.6" },
							  { FPA_REV_VLIST,   "3.8b" },
							  { FPA_REV_NEWCFG,  "4.0" },
							  { FPA_REV_VSTRUCT, "5.0" }
							};

static	int		RevSize = sizeof(RevTbl) / sizeof(TABLE);

/***********************************************************************
*                                                                      *
*   s a m e _ r e v i s i o n                                          *
*   n e w e r _ r e v i s i o n                                        *
*   o l d e r _ r e v i s i o n                                        *
*   c o m p a r e _ r e v i s i o n                                    *
*   v a l i d _ r e v i s i o n                                        *
*   p a r s e _ r e v i s i o n                                        *
*                                                                      *
***********************************************************************/

LOGICAL	same_revision

	(
	STRING		rev1,		/* revision number to compare against */
	STRING		rev2		/* revision number to compare */
	)

	{
	return (LOGICAL) (compare_revision(rev1, rev2) == 0);
	}

LOGICAL	newer_revision

	(
	STRING		rev1,		/* revision number to compare against */
	STRING		rev2		/* revision number to compare */
	)

	{
	return (LOGICAL) (compare_revision(rev1, rev2) == 1);
	}

LOGICAL	older_revision

	(
	STRING		rev1,		/* revision number to compare against */
	STRING		rev2		/* revision number to compare */
	)

	{
	return (LOGICAL) (compare_revision(rev1, rev2) == (-1));
	}

int		compare_revision

	(
	STRING		rev1,		/* revision number to compare against */
	STRING		rev2		/* revision number to compare */
	)

	{
	int		v1, v2, r1, r2;
	char	c1, c2;

	/* Parse both revisions */
	/* Results not guaranteed if either is missing or invalid */
	if (!parse_revision(rev1, &v1, &r1, &c1, NullStringPtr)) return -2;
	if (!parse_revision(rev2, &v2, &r2, &c2, NullStringPtr)) return 2;

	/* Now compare the pieces */
	if (v1 < v2) return 1;
	if (v1 > v2) return -1;
	if (r1 < r2) return 1;
	if (r1 > r2) return -1;
	if (c1 < c2) return 1;
	if (c1 > c2) return -1;
	return 0;
	}

LOGICAL	valid_revision

	(
	STRING		rev			/* revision number to check */
	)

	{
	return parse_revision(rev, NullInt, NullInt, NullChar, NullStringPtr);
	}

LOGICAL	parse_revision

	(
	STRING		rev,		/* revision number to parse */
	int			*version,	/* version number */
	int			*release,	/* release number */
	char		*letter,	/* intermediate release letter */
	STRING		*comment	/* additional comment (patch number) */
	)

	{
	int		val;
	STRING	ap, cp;

	if (version) *version = 0;
	if (release) *release = 0;
	if (letter)  *letter  = '\0';
	if (comment) *comment = NullString;

	if (blank(rev)) return FALSE;

    /* Skip leading whitespace */
    cp = rev + strspn(rev, " \t\n\r\f");

    /* First we must have a string of digits representing the version */
    ap = cp + strspn(cp, "0123456789");
    if (ap == cp)
        {
        return FALSE;
        }
    val = 0;
    while (cp < ap)
        {
        val = val*10 + (*cp-'0');	/* Assume digits collate in sequence */
        cp++;
        }
	if (version) *version = val;

	/* Now we must have a '.' */
	if (*cp != '.')
		{
		return FALSE;
		}
	cp++;

    /* Now we must have a string of digits representing the release */
    ap = cp + strspn(cp, "0123456789");
    if (ap == cp)
        {
        return FALSE;
        }
    val = 0;
    while (cp < ap)
        {
        val = val*10 + (*cp-'0');	/* Assume digits collate in sequence */
        cp++;
        }
	if (release) *release = val;

	/* Now we may have a letter to represent an intermediate release */
	if (!blank(cp))
		{
		if (isalpha(*cp))
			{
			if (letter) *letter = tolower(*cp);
			cp++;
			}
		else
			{
			return FALSE;
			}

		/* Next character must be whitespace or the end */
		if (*cp != '\0')
			{
			ap = cp + strspn(cp, " \t\n\r\f");
			if (ap == cp)
				{
				return FALSE;
				}
			cp = ap;
			}
		}

	/* Now we may have a comment */
	if (!blank(cp))
		{
		if (comment) *comment = cp;
		}

	/* It worked! */
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   r e v i s i o n _ f e a t u r e                                    *
*                                                                      *
***********************************************************************/

LOGICAL	revision_feature

	(
	STRING		feature,	/* feature to check */
	STRING		rev			/* revision number */
	)

	{
	int		i;

	/* Search feature table for given feature */
	for (i=0; i<RevSize; i++)
		{
		if (same(RevTbl[i].index, feature))
			{
			/* Found the given feature - see if it is supported */
			return newer_revision(RevTbl[i].value, rev);
			}
		}

	/* Feature not found */
	return FALSE;
	}
