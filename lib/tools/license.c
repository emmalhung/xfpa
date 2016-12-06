/*********************************************************************/
/** @file license.c
 *
 * Functions to handle license codewords.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*   l i c e n s e . c                                                  *
*                                                                      *
*   Functions to handle license codewords.                             *
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

#define LICENSE_INIT
#include "license.h"
#include "unix.h"
#include "time.h"
#include "parse.h"

#include <fpa_types.h>
#include <fpa_math.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

static	LOGICAL	define_license_directory(STRING);
static	LOGICAL	find_license(int, int, STRING, UNLONG, int*, int*, int*);
static	LOGICAL	define_os_license_directory(STRING);
static	LOGICAL	find_os_license(int);
static	STRING	build_IP(int, int, int, int);
static	LOGICAL	parse_IP(STRING, int*, int*, int*, int*);
static	LOGICAL	match_IP(STRING, STRING);
static	int		application_ID(STRING, LOGICAL*);
static	int		add_app_to_appmask(int, int);
static	LOGICAL	appmask_has_app(int, int);
static	LOGICAL	appmask_counted(int);
static	STRING	app_appname(int);
static	STRING	build_codeword(int, int, STRING, UNLONG, int, int, int);
static	LOGICAL	parse_codeword(STRING, int*, int*, STRING*, UNLONG*, int*,
						int*, int*);
static	int		parse_client(STRING);
static	STRING	parse_address(STRING);
static	STRING	parse_expiry(STRING);

static	ACCESS	Access = FpaAccessNone;
static	LOGICAL	Report = TRUE;

#define LICENSE_DIR "/usr/workstation/fpa/license"
#define OS_LICENSE_DIR "license"

/***********************************************************************
*                                                                      *
*     a p p _ l i c e n s e                                            *
*     f p a l i b _ l i c e n s e                                      *
*     p r o _ l i c e n s e                                            *
*     f p a _ l i c e n s e                                            *
*     o b t a i n _ l i c e n s e                                      *
*     f p a l i b _ v e r i f y                                        *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Obtain a license for the given application or die trying.
 *
 * @param[in] appname Application Name
 * @return True if license obtained.
 *********************************************************************/
LOGICAL	app_license(STRING appname)
	{
	return obtain_license(appname, ClientCommon, NullInt, NullInt);
	}

/*********************************************************************/
/** Obtain a license for the development library.
 *
 * @param[in] mode The type of license.
 * @return True if license obtained.
 *********************************************************************/
LOGICAL	fpalib_license(ACCESS mode)
	{
	switch (mode)
		{
		case FpaAccessFull:
		case FpaAccessLib:
			return obtain_license("library", ClientCommon, NullInt, NullInt);

		case FpaAccessRead:
			return obtain_license("libread", ClientCommon, NullInt, NullInt);

		case FpaAccessNone:
		default:
			return FALSE;
		}
	}

/*********************************************************************/
/** Obtain a license for proprietary (client-specific) features.
 *
 * @param[in] client
 * @param[in] abort_if_none
 * @return True if license obtained.
 *********************************************************************/
LOGICAL	pro_license(int client, LOGICAL abort_if_none)
	{
	static	int		clorg = 0;
	static	LOGICAL	allow = FALSE;
	static	int		code  = 0;

	if (client != clorg)
		{
		clorg = client;
		allow = obtain_license("generic", client, NullInt, &code);
		}

	if (!allow && abort_if_none)
		{
		(void) fprintf(stderr,
				"[License Handler] No licence for proprietary feature");
		(void) fprintf(stderr, " - Aborting!\n");
		exit(code);
		}

	return allow;
	}

/*********************************************************************/
/** Obtain a license with failure checking.
 *
 * @param[in] appname Application name
 * @param[out] ndays number of days left on license
 * @param[out] code Error code
 * @return True if license obtained.
 *********************************************************************/
LOGICAL	fpa_license(STRING appname, int *ndays, int *code)
	{
	return obtain_license(appname, ClientCommon, ndays, code);
	}

/*********************************************************************/
/** General license function used by all of the above.
 *
 * @param[in] appname Application name
 * @param[in] client
 * @param[out] ndays number of days left on license
 * @param[out] code Error code
 * @return True if license obtained.
 *********************************************************************/
LOGICAL	obtain_license(STRING appname, int client, int *ndays, int *code)
	{
	LOGICAL	valid;
	int		ndx;

	/* Obtain an Open Source license, if available */
	(void) define_os_license_directory(OS_LICENSE_DIR);
	valid = get_os_license(appname);
	if (valid)
		{
		/* Valid Open Source license found */
		if (code)  *code  = 0;
		if (ndays) *ndays = 365;
		return TRUE;
		}

	/* Obtain a license */
	(void) define_license_directory(LICENSE_DIR);
	valid = get_license(appname, client, &ndx);
	if (valid)
		{
		/* Valid license found */
		if (code)  *code  = 0;
		if (ndays) *ndays = ndx;
		return TRUE;
		}
	else if (ndx < 0)
		{
		/* Would have been valid, but has expired */
		if (!blank(appname))
			(void) fprintf(stderr,
					"[License Handler] License has expired for \"%s\"!\n",
					appname);
		else
			(void) fprintf(stderr,
					"[License Handler] License has expired!\n");
		if (!code) exit(1);
		*code = 1;
		return FALSE;
		}
	else if (ndx > 0)
		{
		/* Database may not be accessible or all licenses may be in use */
		if (!blank(appname))
			{
			(void) fprintf(stderr,
					"[License Handler] Database may not be accessible!\n");
			(void) fprintf(stderr,
					"[License Handler]  OR\n");
			(void) fprintf(stderr,
					"[License Handler] All licenses are currently being used for \"%s\"!\n",
					appname);
			}
		else
			{
			(void) fprintf(stderr,
					"[License Handler] Database may not be accessible!\n");
			(void) fprintf(stderr,
					"[License Handler]  OR\n");
			(void) fprintf(stderr,
					"[License Handler] All licenses are currently being used!\n");
			}
		if (!code) exit(3);
		*code = 3;
		return FALSE;
		}
	else
		{
		/* No valid license */
		if (!blank(appname))
			(void) fprintf(stderr,
					"[License Handler] Cannot obtain a license for \"%s\"!\n",
					appname);
		else
			(void) fprintf(stderr,
					"[License Handler] Cannot obtain a license!\n");
		if (!code) exit(2);
		*code = 2;
		return FALSE;
		}
	}

/*********************************************************************/
/** Verify that adequate access has been requested and granted.
 *
 * @param[in] mode
 * @return True if adequate access has been granted.
 *********************************************************************/
LOGICAL	fpalib_verify(ACCESS mode)
	{
	FILE	*fp;
	LOGICAL	created;
	STRING	libdesc;
	LOGICAL	valid;

	static	LOGICAL	DoneFull = FALSE;
	static	LOGICAL	DoneLib  = FALSE;
	static	LOGICAL	DoneRead = FALSE;
	static	STRING	File = "/tmp/.fpalib";
	static	mode_t	Mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;

	valid = FALSE;
	switch (mode)
		{
		case FpaAccessFull:
				if (DoneFull) return TRUE;
				libdesc = "Application";
				switch (Access)
					{
					case FpaAccessFull:	valid = TRUE;	break;
					}
				DoneFull = TRUE;
				DoneLib  = TRUE;
				DoneRead = TRUE;
				break;

		case FpaAccessLib:
				if (DoneLib) return TRUE;
				libdesc = "Read-Write Client";
				switch (Access)
					{
					case FpaAccessFull:	valid = TRUE;	break;
					case FpaAccessLib:	valid = TRUE;	break;
					}
				DoneLib  = TRUE;
				DoneRead = TRUE;
				break;

		case FpaAccessRead:
				if (DoneRead) return TRUE;
				libdesc = "Read-Only Client";
				switch (Access)
					{
					case FpaAccessFull:	valid = TRUE;	break;
					case FpaAccessLib:	valid = TRUE;	break;
					case FpaAccessRead:	valid = TRUE;	break;
					}
				DoneRead = TRUE;
				break;

		case FpaAccessNone:
		default:
				return FALSE;
		}

	if (!valid)
		{
		switch (Access)
			{
			case FpaAccessNone:
				(void) printf("The FPA %s Library cannot be used", libdesc);
				(void) printf(" without requesting a license!\n");
				break;
			default:
				(void) printf("The FPA %s Library cannot be used", libdesc);
				(void) printf(" with the current level of access!\n");
			}
		(void) printf("Terminating!\n");
		exit(1);
		}

	if (Report) (void) printf("The FPA %s Library is being used!\n", libdesc);

	if (!create_file(File, &created)) return FALSE;
	if (created) (void) chmod(File, Mode);

	fp = fopen(File, "w");
	if (!fp)
		{
		if (!remove_file(File, (LOGICAL *)0)) return FALSE;
		if (!create_file(File, &created)) return FALSE;
		if (created) (void) chmod(File, Mode);
		fp = fopen(File, "w");
		if (!fp) return FALSE;
		}

	(void) fprintf(fp, "The FPA %s Library is being used!\n", libdesc);
	(void) fprintf(fp, "Process ID: %d\n", getpid());
	(void) fclose(fp);

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     t e m p _ c o d e w o r d                                        *
*                                                                      *
*         Generate a replacement license.                              *
*                                                                      *
***********************************************************************/

STRING	temp_codeword(STRING oldcode, STRING newaddr, int days)
	{
	int		apps, clid, copy, year, jday, h, m, s;
	UNLONG	newmid, oldmid;
	STRING	newcode, oldaddr;
	LOGICAL	valid;

	/* Parse the old codeword */
	if (same(oldcode, "DEMO"))
		{
		apps = 0;
		copy = 10;
		}
	else
		{
		valid = parse_codeword(oldcode, &apps, &clid, &oldaddr, &oldmid,
					&year, &jday, &copy);
		if (!valid) return NULL;
		}

	/* See if we have an IP address or a machine ID */
	if (strchr(newaddr, '.'))
		{
		newmid = 0;
		}
	else
		{
		newmid  = strtoul(newaddr, (char **)NULL, 10);
		newaddr = NULL;
		}

	/* Replace the IP address and the expiry date */
	systime(&year, &jday, &h, &m, &s);
	jday += days;
	jnorm(&year, &jday);
	newcode = build_codeword(apps, clid, newaddr, newmid, year, jday, copy);
	if (!newcode) return NULL;

	/* Match the form (old or new) of the code that was sent */
	if (strlen(oldcode)<29 && newmid==0) newcode[24] = '\0';
	return newcode;
	}

/***********************************************************************
*                                                                      *
*     p a r s e _ c l i e n t                                          *
*     p a r s e _ a d d r                                              *
*     p a r s e _ e x p i r y                                          *
*                                                                      *
*         Extract info from a valid codeword.                          *
*                                                                      *
***********************************************************************/

static	int		parse_client(STRING code)
	{
	int		apps, clid, copy, year, jday;
	UNLONG	mid;
	STRING	addr;
	LOGICAL	valid;

	valid = parse_codeword(code, &apps, &clid, &addr, &mid,
				&year, &jday, &copy);

	return (valid)? clid: -1;
	}

/**********************************************************************/

static	STRING	parse_address(STRING code)
	{
	int		apps, clid, copy, year, jday;
	UNLONG	mid;
	STRING	addr;
	LOGICAL	valid;

	static	char	maddr[20];

	valid = parse_codeword(code, &apps, &clid, &addr, &mid,
				&year, &jday, &copy);
	if (!valid) return NullString;

	if (blank(addr)) (void) sprintf(maddr, "%lu", mid);
	else             (void) strcpy(maddr, addr);
	return maddr;
	}

/**********************************************************************/

static	STRING	parse_expiry(STRING code)
	{
	int		apps, clid, copy, year, jday;
	UNLONG	mid;
	STRING	addr;
	LOGICAL	valid;

	static	char	exbuf[20];

	valid = parse_codeword(code, &apps, &clid, &addr, &mid,
				&year, &jday, &copy);
	if (!valid) return NullString;

	(void) sprintf(exbuf, "%.4d:%.3d", year, jday);
	return exbuf;
	}

/***********************************************************************
*                                                                      *
*     g e t _ o s _ l i c e n s e                                      *
*                                                                      *
*         Find an Open Source license for the calling application, and *
*         assign it to the application.                                *
*                                                                      *
*     g e t _ l i c e n s e                                            *
*                                                                      *
*         Find a valid license for the calling application, and        *
*         assign it to the application.                                *
*                                                                      *
*     c h e c k _ l i c e n s e                                        *
*                                                                      *
*         Determine whether this application (still) has a valid       *
*         license assigned to it.                                      *
*                                                                      *
*     r e l e a s e _ l i c e n s e                                    *
*                                                                      *
*         Release the license that was assigned to the calling         *
*         application.                                                 *
*                                                                      *
***********************************************************************/

static  char    Ldir[250]    = "";		/* Licence directory */
static  char    OSLdir[250]  = "";		/* OpenSource licence directory */
static  STRING  Cfile        = "Codes";	/* Name of codeword file */
static  STRING  Lfile        = NULL;	/* Name of lock file */
static  char    Cpath[275]   = "";		/* Full path of codeword file */
static  char    OSCpath[275] = "";		/* Full path of OpenSource codeword file */
static  char    Lpath[275]   = "";		/* Full path of lock file */

/**********************************************************************/

LOGICAL	get_os_license

	(
	STRING	appname	/* application name */
	)

	{
	char    mask[75], path[50];
	int		nlock, ilock, lpid, lcount, appid;
	STRING  host, *locks, lock, pos;
	LOGICAL status, counted;

	/* Check out the application */
	appid = application_ID(appname, &counted);
	if (appid < 0)
		{
		(void) fprintf(stderr,
				"[License Handler] Unsupported application \"%s\"!\n",
				appname);
		return FALSE;
		}

	/* Make sure we aren't already locked */
	if (counted)
		{
		if (check_license()) return TRUE;
		}

	/* Find an Open Source license */
	host   = fpa_host_name();
	status = find_os_license(appid);
	if (!status)
		{
		(void) fprintf(stderr,
				"[License Handler] Cannot find Open Source license\n");
		return FALSE;
		}

	/* Only the graphics editor has copy count restrictions */
	if (counted)
		{
		/* Create a lock file right away then see if any copies are in use */
		(void) sprintf(Lpath, "%s/.%s.%d", OSLdir, host, getpid());
		Lfile = Lpath + strlen(OSLdir) + 1;
		if(!create_file(Lpath, (LOGICAL *)0))
			{
			(void) fprintf(stderr,
					"[License Handler] Cannot create lock file: \"%s\"\n", Lpath);
			return FALSE;
			}

		/* Now check how many copies are currently running */
		(void) sprintf(mask, "^\\.%s\\.[0-9]*$", host);
		nlock  = dirlist(OSLdir, mask, &locks);
		lcount = 0;
		for (ilock=0; ilock<nlock; ilock++)
			{
			lock = locks[ilock];
			if (same(lock, Lfile)) continue;

			/* See if this lock file has a process running */
			pos  = strrchr(lock, '.');
			(void) sscanf(pos, ".%d", &lpid);
			if (!running(lpid))
				{
				/* Locked process not running - Destroy lock */
				(void) sprintf(path, "%s/%s", OSLdir, lock);
				(void) unlink(path);
				continue;
				}

			/* Locked process is still running - Count this lock */
			lcount++;
			}
		}

	/* Got the license! - Set or upgrade access mode */
	if (same(appname, "libread"))
		{
		switch (Access)
			{
			case FpaAccessFull:	break;
			case FpaAccessLib:	break;
			case FpaAccessRead:
			case FpaAccessNone:
			default:			Access = FpaAccessRead;
			}
		}
	else if (same(appname, "library"))
		{
		switch (Access)
			{
			case FpaAccessFull:	break;
			case FpaAccessLib:
			case FpaAccessRead:
			case FpaAccessNone:
			default:			Access = FpaAccessLib;
			}
		}
	else
		{
		Access = FpaAccessFull;
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	get_license

	(
	STRING	appname	/* application name */ ,
	int		client	/* client ID */ ,
	int		*expiry	/* number of days left until expiry */
	)

	{
	char    mask[75], path[50], address[25];
	int     year, jday, h, m, s, xyear, xjday, expy, copy;
	int		ip, nip, nlock, ilock, lpid, lcount, appid;
	int     ipa, ipb, ipc, ipd;
	UNLONG	mid=0;
	STRING  host, *iplist, addr, *locks, lock, pos;
	LOGICAL status, counted;

	if (expiry) *expiry = 0;

	/* Check out the application */
	appid = application_ID(appname, &counted);
	if (appid < 0)
		{
		(void) fprintf(stderr,
				"[License Handler] Unsupported application \"%s\"!\n",
				appname);
		return FALSE;
		}

	/* Make sure we aren't already locked */
	if (counted)
		{
		if (check_license()) return TRUE;
		}

	/* Find an un-expired license for this machine */
	/* First try the machine ID */
	host   = fpa_host_name();
	mid    = fpa_host_id();
	status = find_license(appid, client, NULL, mid, &xyear, &xjday, &copy);

	/* If this fails - try an old PC Linux IP based license */
	if (!status && (mid = fpa_host_id_pc_ip()) > 0)
		{
		status = find_license(appid, client, NULL, mid, &xyear, &xjday, &copy);
		}

	/* If this fails - try IP address */
	if (!status)
		{

		/* Get the IP address list for this machine */
		nip = fpa_host_ip_list(&iplist);
		if (nip <= 0)
			{
			(void) fprintf(stderr,
					"[License Handler] No host info available!\n");
			return FALSE;
			}

		/* Find an un-expired license for any of the IP addresses */
		for (ip=0; ip<nip; ip++)
			{
			addr = iplist[ip];
			if (!addr) return FALSE;

			ipa = addr[0];	if (ipa < 0) ipa += 256;
			ipb = addr[1];	if (ipb < 0) ipb += 256;
			ipc = addr[2];	if (ipc < 0) ipc += 256;
			ipd = addr[3];	if (ipd < 0) ipd += 256;
			/* (void) sprintf(address, "%d.%d.%d.%d", ipa, ipb, ipc, ipd); */
			(void) strcpy(address, build_IP(ipa, ipb, ipc, ipd));
			if (find_license(appid, client, address, 0, &xyear, &xjday, &copy))
				break;
			}
		if (ip >= nip)
			{
			(void) fprintf(stderr,
					"[License Handler] No license available for any IP address!\n");
			return FALSE;
			}
		}

	/* A valid license has just been found */
	/* Has the license expired? */
	systime(&year, &jday, &h, &m, &s);
	expy = jdif(year, jday, xyear, xjday);
	if (expiry) *expiry = expy;
	if (expy < 0) return FALSE;

	/* Only the graphics editor has copy count restrictions */
	if (counted)
		{
		/* Create a lock file right away then see if any copies are in use */
		(void) sprintf(Lpath, "%s/.%s.%d", Ldir, host, getpid());
		Lfile = Lpath + strlen(Ldir) + 1;
		if(!create_file(Lpath, (LOGICAL *)0))
			{
			(void) fprintf(stderr,
					"[License Handler] Cannot create lock file: \"%s\"\n", Lpath);
			return FALSE;
			}

		/* Now check how many copies are currently running */
		(void) sprintf(mask, "^\\.%s\\.[0-9]*$", host);
		nlock  = dirlist(Ldir, mask, &locks);
		lcount = 0;
		for (ilock=0; ilock<nlock; ilock++)
			{
			lock = locks[ilock];
			if (same(lock, Lfile)) continue;

			/* See if this lock file has a process running */
			pos  = strrchr(lock, '.');
			(void) sscanf(pos, ".%d", &lpid);
			if (!running(lpid))
				{
				/* Locked process not running - Destroy lock */
				(void) sprintf(path, "%s/%s", Ldir, lock);
				(void) unlink(path);
				continue;
				}

			/* Locked process is still running - Count this lock */
			lcount++;
			}

		/* See if there are any license copies left to use */
		if (lcount >= copy)
			{
			/* All license copies are in use - Remove the lock file */
			(void) unlink(Lpath);
			return FALSE;
			}
		}

	/* Got the license! - Set or upgrade access mode */
	if (same(appname, "libread"))
		{
		switch (Access)
			{
			case FpaAccessFull:	break;
			case FpaAccessLib:	break;
			case FpaAccessRead:
			case FpaAccessNone:
			default:			Access = FpaAccessRead;
			}
		}
	else if (same(appname, "library"))
		{
		switch (Access)
			{
			case FpaAccessFull:	break;
			case FpaAccessLib:
			case FpaAccessRead:
			case FpaAccessNone:
			default:			Access = FpaAccessLib;
			}
		}
	else
		{
		Access = FpaAccessFull;
		}
	return TRUE;
	}

/**********************************************************************/

LOGICAL	check_license(void)
	{
	/* Check if the lock file exists */
	if (blank(Lpath)) return FALSE;
	return find_file(Lpath);
	}

/**********************************************************************/

LOGICAL	release_license(void)
	{
	if (blank(Lpath)) return TRUE;

	/* Lock is present - Remove the lock file */
	(void) unlink(Lpath);
	(void) strcpy(Lpath, "");
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     Static:                                                          *
*                                                                      *
*     d e f i n e _ l i c e n s e _ d i r e c t o r y                  *
*                                                                      *
*         Define the directory to find the license codeword file       *
*         and license lock files.                                      *
*                                                                      *
*     f i n d _ l i c e n s e                                          *
*                                                                      *
*         Find a valid license, but do not assign it.                  *
*                                                                      *
*     d e f i n e _ o s _ l i c e n s e _ d i r e c t o r y            *
*                                                                      *
*         Define the directory to find the open source license         *
*         codeword file and open source license lock files.            *
*                                                                      *
*     f i n d _ o s _ l i c e n s e                                    *
*                                                                      *
*         Find a valid open source license.                            *
*                                                                      *
***********************************************************************/

static	LOGICAL	define_license_directory

	(
	STRING	ldir	/* license directory */
	)

	{
	if (blank(ldir)) ldir = ".";
	if (!blank(Ldir))
		{
		if (same(ldir, Ldir)) return TRUE;
		(void) fprintf(stderr, "[License Handler] Cannot re-define path!\n");
		return FALSE;
		}

	if (!find_directory(ldir))
		{
		(void) fprintf(stderr,
				"[License Handler] Directory not found: %s\n", ldir);
		return FALSE;
		}

	(void) strcpy(Ldir, ldir);
	(void) sprintf(Cpath, "%s/%s", Ldir, Cfile);
	if (!find_file(Cpath))
		{
		(void) fprintf(stderr,
				"[License Handler] Codeword file not found: %s\n",
				Cpath);
		(void) strcpy(Ldir, "");
		(void) strcpy(Cpath, "");
		return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	find_license

	(
	int		appid	/* application ID */ ,
	int		client	/* client ID */ ,
	STRING	address	/* IP address */ ,
	UNLONG	mid		/* machine ID */ ,
	int		*year	/* expiry year */ ,
	int		*jday	/* expiry day of year */ ,
	int		*copy	/* number of copies allowed */
	)

	{
	FILE    *fp;
	char    code[250];
	STRING	addr;
	int		aid, cid, xyr, xjd, ncp;
	UNLONG	mmid;

	if (year) *year = 0;
	if (jday) *jday = 0;
	if (copy) *copy = 0;

	if (blank(Cpath))
		{
		(void) fprintf(stderr,
				"[License Handler] License directory not defined!\n");
		return FALSE;
		}

	fp = fopen(Cpath, "r");
	if (!fp)
		{
		(void) fprintf(stderr,
				"[License Handler] Codeword file not accessible!\n");
		return FALSE;
		}

	while (getfileline(fp, code, sizeof(code)))
		{
		/* See if this license matches the given info and has not expired */
		if (!parse_codeword(code, &aid, &cid, &addr, &mmid, &xyr, &xjd, &ncp))
										                 continue;
		if (appid!=0 && !(appid & aid))                  continue;
		if (client!=ClientCommon && cid!=client
			&& cid!=ClientMaster)                        continue;
		if (!blank(address) && !match_IP(addr, address)) continue;
		if (mid!=0 && mmid!=mid)                         continue;

		/* Found a current license for the requested application, client */
		/* and machine */
		(void) fclose(fp);
		if (year) *year = xyr;
		if (jday) *jday = xjd;
		if (copy) *copy = ncp;
		return TRUE;
		}

	/* No valid license found */
	(void) fclose(fp);
	return FALSE;
	}

/**********************************************************************/

static	LOGICAL	define_os_license_directory

	(
	STRING	ldir	/* OpenSource license directory */
	)

	{
	STRING	basedir;

	/* Find base FPA directory */
	basedir = getenv("FPA");
	if (blank(basedir))
		{
		(void) fprintf(stderr, "[License Handler] Cannot find FPA directory!\n");
		return FALSE;
		}

	/* Build the directory path */
	(void) sprintf(OSLdir, "%s/%s", basedir, ldir);
	if (!find_directory(OSLdir))
		{
		(void) fprintf(stderr,
				"[License Handler] Open Source License directory not found: %s\n",
				OSLdir);
		(void) strcpy(OSLdir, "");
		return FALSE;
		}

	(void) sprintf(OSCpath, "%s/%s", OSLdir, Cfile);
	if (!find_file(OSCpath))
		{
		(void) fprintf(stderr,
				"[License Handler] Open Source Codeword file not found: %s\n",
				OSCpath);
		(void) strcpy(OSLdir, "");
		(void) strcpy(OSCpath, "");
		return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	find_os_license

	(
	int		appid	/* application ID */
	)

	{
	FILE    *fp;
	char    code[250];
	STRING	addr;
	int		aid, cid, xyr, xjd, ncp;
	UNLONG	mmid;

	if (blank(OSCpath))
		{
		(void) fprintf(stderr,
				"[License Handler] Open Source license directory not defined!\n");
		return FALSE;
		}

	fp = fopen(OSCpath, "r");
	if (!fp)
		{
		(void) fprintf(stderr,
				"[License Handler] Open Source codeword file not accessible!\n");
		return FALSE;
		}

	while (getfileline(fp, code, sizeof(code)))
		{
		/* See if this license matches the Open Source "machine id" */
		if (!parse_codeword(code, &aid, &cid, &addr, &mmid, &xyr, &xjd, &ncp))
										                 continue;
		if (appid!=0 && !(appid & aid))                  continue;
		if (mmid!=14780)                                 continue;

		/* Found an Open Source license for the requested application */
		(void) fprintf(stderr,
				"[License Handler] Using Open Source codeword\n");
		(void) fclose(fp);
		return TRUE;
		}

	/* No valid Open Source license found */
	(void) fclose(fp);
	return FALSE;
	}

/***********************************************************************
*                                                                      *
*     Static:                                                          *
*                                                                      *
*     b u i l d _ I P                                                  *
*     p a r s e _ I P                                                  *
*     m a t c h _ I P                                                  *
*                                                                      *
*         Decode and encode an IP address.                             *
*         Determine if two IP addresses match.  Note that a member     *
*         of a subnet matches the subnet.                              *
*                                                                      *
***********************************************************************/

static	STRING	build_IP

	(
	int		ipa,
	int		ipb,
	int		ipc,
	int		ipd
	)

	{
	static	char	abuf[16];

	char	buf[10];

	(void) strcpy(abuf, "");

	if (ipa < 0 || ipa > 255) return NullString;
	(void) sprintf(buf, "%d", ipa);
	(void) strcat(abuf, buf);
	(void) strcat(abuf, ".");

	if (ipb < 0 || ipb > 255) return NullString;
	(void) sprintf(buf, "%d", ipb);
	(void) strcat(abuf, buf);
	(void) strcat(abuf, ".");

	if (ipc < 0 || ipc > 255) return NullString;
	if (ipc > 0) (void) sprintf(buf, "%d", ipc);
	else         (void) strcpy(buf, "*");
	(void) strcat(abuf, buf);
	(void) strcat(abuf, ".");

	if (ipd < 0 || ipd > 255) return NullString;
	if (ipd > 0) (void) sprintf(buf, "%d", ipd);
	else         (void) strcpy(buf, "*");
	(void) strcat(abuf, buf);
	(void) strcat(abuf, ".");

	return abuf;
	}

/**********************************************************************/

static	LOGICAL	parse_IP

	(
	STRING	address,
	int		*ipa,
	int		*ipb,
	int		*ipc,
	int		*ipd
	)

	{
	static	STRING	digits = "0123456789";

	int		len;
	char	*cp, buf[20];

	*ipa = 0;
	*ipb = 0;
	*ipc = 0;
	*ipd = 0;

	if (blank(address)) return FALSE;
	cp = address;

	len = strspn(cp, digits);
	if (len <= 0) return FALSE;
	(void) strncpy(buf, cp, len);
	buf[len] = '\0';
	if (sscanf(buf, "%d", ipa) < 1) return FALSE;

	cp += len;
	if (*cp != '.') return FALSE;
	cp++;

	len = strspn(cp, digits);
	if (len <= 0) return FALSE;
	(void) strncpy(buf, cp, len);
	buf[len] = '\0';
	if (sscanf(buf, "%d", ipb) < 1) return FALSE;

	cp += len;
	if (*cp != '.') return FALSE;
	cp++;

	if (*cp == '*')
		{
		*ipc = 0;
		len = 1;
		}
	else
		{
		len = strspn(cp, digits);
		if (len <= 0) return FALSE;
		(void) strncpy(buf, cp, len);
		buf[len] = '\0';
		if (sscanf(buf, "%d", ipc) < 1) return FALSE;
		}

	cp += len;
	if (*cp != '.') return FALSE;
	cp++;

	if (*cp == '*')
		{
		*ipd = 0;
		len = 1;
		}
	else
		{
		len = strspn(cp, digits);
		if (len <= 0) return FALSE;
		(void) strncpy(buf, cp, len);
		buf[len] = '\0';
		if (sscanf(buf, "%d", ipd) < 1) return FALSE;
		}

	return TRUE;
	}

/**********************************************************************/

static	LOGICAL	match_IP

	(
	STRING	addr1,
	STRING	addr2
	)

	{
	int		ipa, ipb, ipc, ipd;
	int		jpa, jpb, jpc, jpd;

	if (!parse_IP(addr1, &ipa, &ipb, &ipc, &ipd)) return FALSE;
	if (!parse_IP(addr2, &jpa, &jpb, &jpc, &jpd)) return FALSE;

	if (ipa != jpa) return FALSE;

	if (ipb != jpb) return FALSE;

	if (ipc != jpc)
		{
		if (ipc != 0 && jpc != 0) return FALSE;
		}

	if (ipd != jpd)
		{
		if (ipd != 0 && jpd != 0) return FALSE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*     Static:                                                          *
*                                                                      *
*     a p p l i c a t i o n _ I D                                      *
*                                                                      *
*         Produce an application ID from the given name.               *
*                                                                      *
***********************************************************************/

typedef	struct
			{
			STRING	name;
			int		id;
			int		oid;
			LOGICAL	counted;
			} APPTBL;

/* List of supported applications */
/*******************************************************
*           D A N G E R ! ! !                          *
*    Do not change the ID for existing applications!   *
*******************************************************/
static	const	APPTBL	AppList[] =
				{
				"generic",	0,				0,		FALSE,
				"editor",	BIT(0),			BIT(0),	TRUE,
				"viewer",	BIT(0),			BIT(0),	FALSE,
				"mapgen",	BIT(1),			BIT(1),	FALSE,
				"ingest",	BIT(2),			BIT(2),	FALSE,
				"sampler",	BIT(3),			BIT(3),	FALSE,
				"product",	BIT(4),			BIT(4),	FALSE,
				"model",	BIT(5),			BIT(5),	FALSE,
				"library",	BIT(6),			BIT(6),	FALSE,
				"libread",	BIT(6)|BIT(7),	BIT(7),	FALSE
				};

static	const	int		AppNum  = sizeof(AppList) / sizeof(APPTBL);

static	const	int		AppAll  = 0x7F;
static	const	int		AppNone = 0;

/**********************************************************************/

static	int	application_ID

	(
	STRING	appname	,
	LOGICAL	*counted
	)

	{
	int		i;
	char	name[20];
	char	*c;

	if (counted) *counted = FALSE;
	if (blank(appname)) return -1;

	(void) strcpy(name, appname);
	if (c = strchr(name, '.')) *c = '\0';
	if (c = strchr(name, '-')) *c = '\0';

	for (i=0; i<AppNum; i++)
		{
		if (same_ic(name, AppList[i].name))
			{
			if (counted) *counted = AppList[i].counted;
			return AppList[i].id;
			}
		}

	return -1;
	}

/**********************************************************************/

static	int		add_app_to_appmask

	(
	int		apps,	/* Original appmask */
	int		iapp	/* New appnum to add */
	)

	{
	if (iapp >= AppNum) return apps;
	if (iapp <  0)      return apps;

	return (apps | AppList[iapp].oid);
	}

/**********************************************************************/

static	LOGICAL	appmask_has_app

	(
	int		apps,	/* Given appmask */
	int		iapp	/* Appnum to test */
	)

	{
	if (iapp >= AppNum) return FALSE;
	if (iapp <  0)      return FALSE;

	if (AppList[iapp].id == 0)   return TRUE;
	if (apps & AppList[iapp].id) return TRUE;
	return FALSE;
	}

/**********************************************************************/

static	LOGICAL	appmask_counted

	(
	int		apps	/* Given appmask */
	)

	{
	int		iapp;

	for (iapp=1; iapp<AppNum; iapp++)
		{
		if ((apps & AppList[iapp].id) && AppList[iapp].counted) return TRUE;
		}

	return FALSE;
	}

/**********************************************************************/

static	STRING	app_appname

	(
	int		iapp	/* Given appnum */
	)

	{
	if (iapp >= AppNum) return NULL;
	if (iapp <  0)      return NULL;

	return AppList[iapp].name;
	}

/***********************************************************************
*                                                                      *
*     Static:                                                          *
*                                                                      *
*     b u i l d _ c o d e w o r d                                      *
*     p a r s e _ c o d e w o r d                                      *
*                                                                      *
*         Encode/decode a license codeword.                            *
*                                                                      *
***********************************************************************/

static	const	int		AppCode = 0xAAAA;
static	const	int		MachID  = BIT(15);
static	const	int		NewForm = BIT(7);
static			UNSIGN	Ran[4]  = {0,0,0,0};
static			LOGICAL	UseLast = FALSE;

/**********************************************************************/

static	STRING build_codeword

	(
	int		appmask,
	int		client,
	STRING	address,
	UNLONG	mid,
	int		year,
	int		jday,
	int		copy
	)

	{
	long    tval;
	UNSIGN  seed;
	int     ipa, ipb, ipc, ipd, cent;
	LOGICAL	usemid;
	UNSIGN  pos[5][4];

	static  char    Code[30];

	static	LOGICAL	First = TRUE;

	/* Interpret IP address */
	if (!blank(address))
		{
		if (!parse_IP(address, &ipa, &ipb, &ipc, &ipd)) return NULL;
		usemid = FALSE;
		}
	else if (mid > 0)
		{
		ipa = (mid>>24) & 255;
		ipb = (mid>>16) & 255;
		ipc = (mid>>8) & 255;
		ipd = mid & 255;
		usemid = TRUE;
		}
	else	return NULL;

	/* Interpret expiry date */
	if (year < 0)            return NULL;
	if (jday<=0 || jday>366) return NULL;
	cent  = year / 100;
	year  = year % 100;
	year |= NewForm;	/* New format bit */

	/* Interpret license copy count */
	if (copy<=0 || copy>16) return NULL;

	/* Interpret application/client mask */
	if (appmask < 0)       return NULL;
	if (appmask >= BIT(8)) return NULL;
	if (client < 0)        return NULL;
	if (client >= BIT(7))  return NULL;
	appmask |= (client << 8);
	if (usemid) appmask |= MachID; /* Machine ID bit */
	appmask ^= AppCode;

	/* Generate 4 randomizers */
	if (UseLast)
		{
		UseLast = FALSE;
		}
	else
		{
		if (First)
			{
			tval = (long) time((time_t *) 0);
			seed = (UNSIGN) (tval % (1<<31));
			}
#		if defined(MACHINE_HP) && defined(PRE_POSIX)
			if (First) srand(seed);
			Ran[0] = (UNSIGN) (rand() % 16);
			Ran[1] = (UNSIGN) (rand() % 16);
			Ran[2] = (UNSIGN) (rand() % 16);
			Ran[3] = (UNSIGN) (rand() % 16);
#		else
			if (First) srandom(seed);
			Ran[0] = (UNSIGN) (random() % 16);
			Ran[1] = (UNSIGN) (random() % 16);
			Ran[2] = (UNSIGN) (random() % 16);
			Ran[3] = (UNSIGN) (random() % 16);
#		endif
		First = FALSE;
		}

	/* Fill in the first block */
	pos[0][0] = (ipa/16 + Ran[0]) % 16;
	pos[0][1] = (ipb/16 + Ran[1]) % 16;
	pos[0][2] = (ipc/16 + Ran[2]) % 16;
	pos[0][3] = (ipd/16 + Ran[3]) % 16;

	/* Fill in the second block */
	pos[1][0] = (ipa%16 + Ran[0]) % 16;
	pos[1][1] = (ipb%16 + Ran[1]) % 16;
	pos[1][2] = (ipc%16 + Ran[2]) % 16;
	pos[1][3] = (ipd%16 + Ran[3]) % 16;

	/* Fill in the third block */
	pos[2][0] = (cent/16 + Ran[0]) % 16;
	pos[2][1] = (year/16 + Ran[1]) % 16;
	pos[2][2] = (jday/256 + Ran[2]) % 16;
	pos[2][3] = (jday%256/16 + Ran[3]) % 16;

	/* Fill in the fourth block */
	pos[3][0] = (cent%16 + Ran[0]) % 16;
	pos[3][1] = (year%16 + Ran[1]) % 16;
	pos[3][2] = (jday%16 + Ran[2]) % 16;
	pos[3][3] = (copy%16 + Ran[3]) % 16;

	/* Fill in the fifth block */
	pos[4][0] = ((appmask>>12)%16 + Ran[0]) % 16;
	pos[4][1] = ((appmask>>8)%16 + Ran[1]) % 16;
	pos[4][2] = ((appmask>>4)%16 + Ran[2]) % 16;
	pos[4][3] = (appmask%16 + Ran[3]) % 16;

	/* Create the codeword */
	(void) sprintf(Code, "%X%X%X%X-%X%X%X%X-%X%X%X%X-%X%X%X%X-%X%X%X%X-%X%X%X%X",
	Ran[0],    Ran[1],    Ran[2],    Ran[3],
	pos[0][0], pos[0][1], pos[0][2], pos[0][3],
	pos[1][0], pos[1][1], pos[1][2], pos[1][3],
	pos[2][0], pos[2][1], pos[2][2], pos[2][3],
	pos[3][0], pos[3][1], pos[3][2], pos[3][3],
	pos[4][0], pos[4][1], pos[4][2], pos[4][3]);

	/* Done */
	return Code;
	}

/**********************************************************************/

static	LOGICAL parse_codeword

	(
	STRING	code,
	int		*appmask,
	int		*client,
	STRING	*address,
	UNLONG	*mid,
	int		*year,
	int		*jday,
	int		*copy
	)

	{
	int     n;
	int     ipa, ipb, ipc, ipd;
	UNSIGN  ran[4], pos[5][4];
	LOGICAL	smallcode = FALSE;

	static  char    Address[25];
	static  UNLONG  MID;
	static  int     Cent;
	static  int     Year;
	static  int     Jday;
	static  int     Copy;
	static  int     Apps;
	static  int     Client;

	*appmask = 0;
	*address = NULL;
	*mid     = 0;
	*year    = 0;
	*jday    = 0;
	*copy    = 0;

	/* Break up the codeword */
	n = sscanf(code, "%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x",
	&ran[0],    &ran[1],    &ran[2],    &ran[3],
	&pos[0][0], &pos[0][1], &pos[0][2], &pos[0][3],
	&pos[1][0], &pos[1][1], &pos[1][2], &pos[1][3],
	&pos[2][0], &pos[2][1], &pos[2][2], &pos[2][3],
	&pos[3][0], &pos[3][1], &pos[3][2], &pos[3][3],
	&pos[4][0], &pos[4][1], &pos[4][2], &pos[4][3]);
	if (n != 24)
		{
		if (n != 20) return FALSE;
		smallcode = TRUE;
		}
	Ran[0] = ran[0];
	Ran[1] = ran[1];
	Ran[2] = ran[2];
	Ran[3] = ran[3];

	/* Decode the first block */
	ipa = ( (pos[0][0] + 16 - Ran[0]) % 16 ) * 16;
	ipb = ( (pos[0][1] + 16 - Ran[1]) % 16 ) * 16;
	ipc = ( (pos[0][2] + 16 - Ran[2]) % 16 ) * 16;
	ipd = ( (pos[0][3] + 16 - Ran[3]) % 16 ) * 16;

	/* Decode the second block */
	ipa += ( (pos[1][0] + 16 - Ran[0]) % 16 );
	ipb += ( (pos[1][1] + 16 - Ran[1]) % 16 );
	ipc += ( (pos[1][2] + 16 - Ran[2]) % 16 );
	ipd += ( (pos[1][3] + 16 - Ran[3]) % 16 );

	/* Decode the third block */
	Cent  = ( (pos[2][0] + 16 - Ran[0]) % 16 ) * 16;
	Year  = ( (pos[2][1] + 16 - Ran[1]) % 16 ) * 16;
	Jday  = ( (pos[2][2] + 16 - Ran[2]) % 16 ) * 256;
	Jday += ( (pos[2][3] + 16 - Ran[3]) % 16 ) * 16;

	/* Decode the fourth block */
	Cent += ( (pos[3][0] + 16 - Ran[0]) % 16 );
	Year += ( (pos[3][1] + 16 - Ran[1]) % 16 );
	Jday += ( (pos[3][2] + 16 - Ran[2]) % 16 );
	Copy  = ( (pos[3][3] + 16 - Ran[3]) % 16 );
	if (Copy == 0) Copy = 16;

	/* Decode the fifth block */
	if (smallcode) Apps = 0;
	else
		{
		Apps  = ( (pos[4][0] + 16 - Ran[0]) % 16 ) << 12;
		Apps += ( (pos[4][1] + 16 - Ran[1]) % 16 ) << 8;
		Apps += ( (pos[4][2] + 16 - Ran[2]) % 16 ) << 4;
		Apps += ( (pos[4][3] + 16 - Ran[3]) % 16 );
		Apps ^= AppCode;
		}

	/* Re-build IP address or machine ID */
	if (Apps & MachID)
		{
		Apps ^= MachID;
		MID = (ipa<<24) + (ipb<<16) + (ipc<<8) + ipd;
		(void) strcpy(Address, "");
		}
	else
		{
		/* (void) sprintf(Address, "%d.%d.%d.%d", ipa, ipb, ipc, ipd); */
		(void) strcpy(Address, build_IP(ipa, ipb, ipc, ipd));
		MID = 0;
		}

	/* See if using new format */
	if (Year & NewForm)
		{
		Year  ^= NewForm;
		Client = Apps/256;
		Apps  &= 255;
		}
	else if (smallcode)
		{
		Client = ClientMaster;
		Apps   = AppAll;	/* enable everything (old form only) */
		}
	else
		{
		Client = ClientMaster;
		n = Apps;			Apps  = BIT(0);	/* enable editor/viewer/generic */
		if (n & BIT(0))		Apps |= BIT(1);	/* mapgen */
		if (n & BIT(1))		Apps |= BIT(2);	/* ingest */
		if (n & BIT(2))		Apps |= BIT(3);	/* sampler */
		if (n & BIT(3))		Apps |= BIT(4);	/* prod-public */
		if (n & BIT(4))		Apps |= BIT(4);	/* prod-graphic */
		if (n & BIT(5))		Apps |= BIT(4);	/* prod-table */
		if (n & BIT(6))		Apps |= BIT(5);	/* model-proprietary */
		if (n & BIT(7))		Apps |= BIT(5);	/* model-proprietary */
		if (n & BIT(8))		Apps |= BIT(5);	/* model-fpawarp */
		if (n & BIT(13))	Apps |= BIT(0);	/* proprietary */
		if (n & BIT(14))	Apps |= BIT(6);	/* fpa-lib */
		}

	*appmask = Apps;
	*client  = Client;
	*address = Address;
	*mid     = MID;
	*year    = Cent*100 + Year;
	*jday    = Jday;
	*copy    = Copy;
	return TRUE;
	}

/***********************************************************************
*                                                                      *
*   Test program for license handling functions.                       *
*                                                                      *
***********************************************************************/

#ifdef STANDALONE
void	get_test(void);
void	check_test(void);
void	release_test(void);

void	main(void)
	{
	char    buf[25], dir[250];

	while (1)
		{
		(void) printf("\n");
		(void) printf("Enter license directory: ");
		getfileline(stdin, dir, sizeof(dir));

		if (blank(dir)) exit(0);
		if (define_license_directory(dir)) break;
		}

	while (1)
		{
		(void) printf("\n");
		(void) printf("Test modes:\n");
		(void) printf("   1 - Get licence\n");
		(void) printf("   2 - Check licence\n");
		(void) printf("   3 - Release licence\n");
		(void) printf("Enter test mode: ");
		getfileline(stdin, buf, sizeof(buf));

		if (blank(buf)) break;
		else if (same(buf, "1")) get_test();
		else if (same(buf, "2")) check_test();
		else if (same(buf, "3")) release_test();
		}

	if (check_license())
		{
		(void) printf("\n");
		(void) printf("Releasing license.\n");
		(void) release_license();
		}
	}

void	get_test(void)
	{
	LOGICAL	valid;
	int		expy;

	(void) printf("\n");
	valid = check_license();
	if (valid) (void) printf("You already have a license!\n");
	else
		{
		valid = get_license("editor", ClientAES, &expy);
		if (valid) (void) printf("License obtained! Expires in %d days\n", expy);
		else       (void) printf("Sorry - No license available!\n");
		}
	}

void	check_test(void)
	{
	LOGICAL	valid;

	(void) printf("\n");
	valid = check_license();
	if (valid) (void) printf("You have a license!\n");
	else       (void) printf("You do not have a license!\n");
	}

void	release_test(void)
	{
	LOGICAL	valid;

	(void) printf("\n");
	valid = check_license();
	if (valid)
		{
		valid = release_license();
		if (valid) (void) printf("License released!\n");
		else       (void) printf("Warning - Licence not released!\n");
		}
	else (void) printf("You do not have a license!\n");
	}
#endif /* STANDALONE */
