/*
 *	Find a file given its type, name and suffix. Both type and suffix
 *  may be NULL.  The search is done using XtResolvePathname.  If the
 *  file is not found the return will be NULL. Note that is the calling
 *  application's responsibility to free the returned string.
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "XuP.h"

#define APPDEF "app-defaults"


/* Return true if the given name is a valid directory */
static Boolean dirtest( const String dname )
{
	struct stat sb;
	return( stat(dname,&sb) == 0 && S_ISDIR(sb.st_mode) != 0);
}

static String find_file_type(String type, String filename, String suffix, const Boolean isdir)
{
	int    i, j, len;
	String rtn_path, path;
	struct stat sb;
	extern String getenv();
	static int pathlen = 0;

	static SubstitutionRec sub[] = {
		{'X', NULL},
		{'x', NULL},
		{'Z', APPDEF}
	};

	static String dir[] = {
		NULL,
		NULL,
		NULL,
		".",
		"/etc/X11",			/* Searching this directory may be obsolete */
		"/usr/lib/X11",		/* Searching this directory may be obsolete */
		"/usr/lib/include"	/* Searching this directory may be obsolete */
	};

	static String info[] = {
		"/%Z/%l/%T/%X/%N%S:",
		"/%Z/%l/%T/%x/%N%S:",
		"/%Z/%L/%T/%X/%N%S:",
		"/%Z/%L/%T/%x/%N%S:",
		"/%Z/%l/%T/%N%S:",
		"/%Z/%L/%T/%N%S:",
		"/%Z/%T/%X/%N%S:",
		"/%Z/%T/%x/%N%S:",
		"/%Z/%T/%N%S:",
		"/%Z/%N%S:",
		"/%l/%T/%N%S:",
		"/%l/%T/%X/%N%S:",
		"/%l/%T/%x/%N%S:",
		"/%L/%T/%N%S:",
		"/%L/%T/%X/%N%S:",
		"/%L/%T/%x/%N%S:",
		"/%T/%X/%N%S:",
		"/%T/%x/%N%S:",
		"/%T/%N%S:",
		"/%N%S:"
	};

	/* Calculate and store this once. The substitutions are internal fixed strings
	 * so we do not have to copy them and they stay the same for the life of the
	 * program
	 */
	if (!pathlen)
	{
		String p;

		sub[0].substitution = Fxu.app_name;
		sub[1].substitution = Fxu.app_class;

		/* Set the base directories to be searched */
		dir[0] = (p = getenv(XAPPLRESDIR))? XtNewString(p):NULL;
		dir[1] = Fxu.home_dir;
		dir[2] = (p = getenv("HOME"))? XtNewString(p):NULL;

		for(len = 0, i = 0; i < XtNumber(info); i++)
			len += (int) safe_strlen(info[i]);

		/* Since the %Z is keyed to "app-defaults" we need to filter out any
		 * directories that are the same so that we don't get double entries.
		 */
		for(i = 0; i < XtNumber(dir); i++)
		{
			if(same(dir[i],APPDEF)) dir[i] = NULL;
			if (blank(dir[i])) continue;
			pathlen += (int) safe_strlen(dir[i]) * XtNumber(info) + len + 1;
		}
	}

	if (blank(filename)) return NULL;

	/* If the filename starts with "/" then it is an absolute pathname
	*/
	if(filename[0] == '/')
	{
		if(isdir)
		{
			if(stat(filename,&sb) == 0 && S_ISDIR(sb.st_mode) != 0 && access(filename,R_OK))
				return XtNewString(filename);
		}
		else if(blank(suffix))
		{
			if(stat(filename,&sb) == 0 && S_ISDIR(sb.st_mode) == 0 && access(filename,R_OK))
				return XtNewString(filename);
		}
		else
		{
			path = XTCALLOC(safe_strlen(filename)+safe_strlen(suffix)+1, char);
			(void) safe_strcpy(path, filename);
			(void) safe_strcat(path, suffix);
			if(stat(path,&sb) == 0 && S_ISDIR(sb.st_mode) == 0 && access(path,R_OK))
				return path;
			XtFree((void*)path);
		}
		return NULL;
	}

	/* Construct the search path string. This never changes but the path variable
	 * will be fairly big and would be a waste of memory to hold onto
	 */
	path = XTCALLOC(pathlen, char);
	for(i = 0; i < XtNumber(dir); i++)
	{
		if (blank(dir[i])) continue;
		for(j = 0; j < XtNumber(info); j++)
		{
			(void) safe_strcat(path, dir[i]);
			(void) safe_strcat(path, info[j]);
		}
	}

	if(isdir)
		rtn_path = XtResolvePathname(DefaultAppDisplayInfo->display, type, filename, suffix, path, sub, 3, dirtest);
	else
		rtn_path = XtResolvePathname(DefaultAppDisplayInfo->display, type, filename, suffix, path, sub, 3, NULL);

	XtFree((void*)path);
	return rtn_path;
}


String XuFindDirectory( String dname )
{
	return find_file_type(NULL, dname, NULL, True);
}


String XuFindFile(String filename )
{
	return find_file_type(NULL, filename, NULL, False);
}


String XuFindTypeFile(String type, String filename, String suffix)
{
	return find_file_type(type, filename, suffix, False);
}
