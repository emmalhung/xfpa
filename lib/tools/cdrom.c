/*********************************************************************/
/** @file cdrom.c
 *
 * Functions to access files on CD-ROM.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   c d r o m . c                                                      *
*                                                                      *
*   Functions to access files on CD-ROM.                               *
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

#include "cdrom.h"
#include "parse.h"
#include "unix.h"
#include "message.h"
#include "string_ext.h"

#include <fpa_types.h>
#include <fpa_macros.h>
#include <fpa_getmem.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

/***********************************************************************
*                                                                      *
*     C D _ m o u n t _ p o i n t                                      *
*                                                                      *
***********************************************************************/

static	STRING	Mount = NULL;
static	int		Mlen  = 0;

/*********************************************************************/
/** Identify the mount point for the CD-ROM so that this part of
 * a given pathname will be unaffected by the pathname conversion
 * in CD_path().
 * 	
 *	@param[in]	path		path name of mount point
 *********************************************************************/
void	CD_mount_point

	(
	STRING	path
	)

	{
	Mount = STRMEM(Mount, path);
	Mlen  = strlen(Mount);
	}

/***********************************************************************
*                                                                      *
*     C D _ o p e n                                                    *
*     C D _ c l o s e                                                  *
*     C D _ a c c e s s                                                *
*     C D _ s t a t                                                    *
*                                                                      *
*     Open/close/check/inquire a file on CD-ROM.                       *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Open a file on CD-ROM.
 *
 *	@param[in]	path	Mount point
 *	@param[in]	mode	read/write mode as per fopen
 * 	@return Pointer to opened file. NULL if failed. If successful you
 * 			will need to close this file when you are finished with it.
 *********************************************************************/
FILE	*CD_open

	(
	STRING	path,
	STRING	mode
	)

	{
	STRING	cdp;

	cdp = CD_path(path);
	if (blank(cdp)) return NULL;

	return fopen(cdp, mode);
	}

/**********************************************************************/

/*********************************************************************/
/** Close a file on CD-ROM.
 *
 *	@param[in]	*fp		File to close
 *********************************************************************/
void	CD_close

	(
	FILE	*fp
	)

	{
	(void) fclose(fp);
	}

/**********************************************************************/

/*********************************************************************/
/** Check a file on CD-ROM. Determine whether a file can be accessed
 * with the specified mode.
 *
 *	@param[in]	path	File path on cd
 *	@param[in]	amode	access mode. one or more letters of rwx
 *						(read/write/execute)
 * 	@return Positive if file can be accessed with given mode. Negative
 * 			otherwise.
 *********************************************************************/
int	CD_access

	(
	STRING	path,
	int		amode
	)

	{
	STRING	cdp;

	cdp = CD_path(path);
	if (blank(cdp)) return -1;

	return access(cdp, amode);
	}

/**********************************************************************/

/*********************************************************************/
/**	Returns the status structure associated with the given file.
 * @param[in] path file path on CD.
 * @param[out] *buf status structure.
 * @return Positive if successful, Negative otherwise.
 *********************************************************************/
int		CD_stat

	(
	STRING	path,
	struct stat	*buf
	)

	{
	STRING	cdp;

	cdp = CD_path(path);
	if (blank(cdp)) return -1;

	return stat(cdp, buf);
	}

/***********************************************************************
*                                                                      *
*     C D _ p a t h                                                    *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Convert a given pathname to the style as it would appear on CD-ROM.
 *
 *	@param[in]	path	Path to convert
 * 	@return converted path.
 *********************************************************************/

STRING	CD_path

	(
	STRING	path
	)

	{
	char			dir[4096];
	char			base[128];
	size_t			blen;
	DIR				*dirpt;
	struct dirent	*dp;
	STRING			fname, frem;

	/* Only access files on the CD */
	if (!same_start_ic(path, Mount))
		{
		pr_diag("CD", "Non-CD: %s %s\n", path, Mount);
		return NULL;
		}

	/* Obtain directory portion from path */
	(void) safe_strcpy(dir, dir_name(path));

	/* Try to access the directory as is */
	if (blank(dir)) dirpt = opendir(".");
	else            dirpt = opendir(dir);

	/* If unsuccessful try case folding the directory */
	if (IsNull(dirpt))
		{
		if (same_start_ic(dir, Mount)) (void) upper_case(dir+Mlen);
		else                           (void) upper_case(dir);
		dirpt = opendir(dir);
		}

	/* If still unsuccessful give up */
	if (IsNull(dirpt))
		{
		pr_diag("CD", "No Dir: %s %s\n", path, dir);
		return NULL;
		}

	/* Obtain filename portion from path */
	(void) safe_strcpy(base, base_name(path, NULL));
	blen = strlen(base);

	/* Scan the directory for the best match */
	fname = NULL;
	while ((dp = readdir(dirpt)) != NULL)
		{
		if (same(dp->d_name, "."))  continue;
		if (same(dp->d_name, "..")) continue;

		/* Beginning of filename must match base */
		fname = dp->d_name;
		if (!same_start_ic(fname, base))
			{
			fname = NULL;
			continue;
			}

		/* Did it match the whole thing? */
		frem = fname + blen;
		if (blank(frem)) break;

		/* Handle optional "." */
		if (frem[0] == '.')
			{
			frem++;
			if (blank(frem)) break;
			}

		/* Handle optional ";" with version number (must be digits) */
		if (frem[0] == ';')
			{
			frem++;
			while (frem[0]!='\0' && isdigit(frem[0]))
				frem++;
			if (blank(frem)) break;
			}

		/* Not a match - keep looking */
		fname = NULL;
		}

	/* Close the directory */
	(void) closedir(dirpt);

	/* Return the full path if file found */
	if (!blank(fname))
		{
		fname = pathname(dir, fname);
		pr_diag("CD", "Match: %s %s\n", path, fname);
		return fname;
		}

	/* Not found */
	pr_diag("CD", "No File: %s %s\n", path, base);
	return NULL;
	}

/***********************************************************************
*                                                                      *
*     C D _ n f i l e s                                                *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Count the number of files in the given directory, optionally
 * matching the given pattern.
 *
 *	@param[in]	dpath		directory path
 *	@param[in]	pattern		pattern to match (optional)
 * @return number of files counted.
 *********************************************************************/

int		CD_nfiles

	(
	STRING	dpath,
	STRING	pattern
	)

	{
	char			dir[4096];
	DIR				*dirpt;
	struct dirent	*dp;
	int				count;
	STRING			fname, frem;

	/* Only access files on the CD */
	if (!same_start_ic(dpath, Mount))
		{
		pr_diag("CD", "Non-CD: %s %s\n", dpath, Mount);
		return 0;
		}

	/* Obtain directory portion from path */
	(void) safe_strcpy(dir, dpath);

	/* Try to access the directory as is */
	if (blank(dir)) dirpt = opendir(".");
	else            dirpt = opendir(dir);

	/* If unsuccessful try case folding the directory */
	if (IsNull(dirpt))
		{
		if (same_start_ic(dir, Mount)) (void) upper_case(dir+Mlen);
		else                           (void) upper_case(dir);
		dirpt = opendir(dir);
		}

	/* If still unsuccessful give up */
	if (IsNull(dirpt))
		{
		pr_diag("CD", "No Dir: %s %s\n", dpath, dir);
		return 0;
		}

	/* Scan the directory for matching files */
	count = 0;
	while ((dp = readdir(dirpt)) != NULL)
		{
		if (same(dp->d_name, "."))  continue;
		if (same(dp->d_name, "..")) continue;

		/* Just count if not match required */
		if (blank(pattern))
			{
			count++;
			continue;
			}

		/* For now we are just matching the extension */
		fname = dp->d_name;
		frem  = strchr(fname, '.');
		if (IsNull(frem)) continue;
		frem++;

		/* Does the extension match? */
		if (!same_start_ic(frem, pattern)) continue;
		frem += strlen(pattern);
		if (blank(frem))
			{
			count++;
			continue;
			}

		/* Handle optional ";" with version number (must be digits) */
		if (frem[0] == ';')
			{
			frem++;
			while (frem[0]!='\0' && isdigit(frem[0]))
				frem++;
			if (blank(frem))
				{
				count++;
				continue;
				}
			}
		}

	/* Close the directory */
	(void) closedir(dirpt);

	return count;
	}
