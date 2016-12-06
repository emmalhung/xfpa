/*=====================================================================*/
/*
*	File: State.c
*
*   Purpose: Contains functions responsible for maintaining interface
*            state data.
*
*  Description: The functions work on the concept of a base state file,
*               a current profile file and an active file. The base state
*  file holds the record of what profiles exist and is used to define the
*  directory in which the profile files are created. The profile files
*  hold all of the state into and are used to read and write any program
*  state information. It is possible to set a state file other than the
*  default. In this case any read or write function needs to be preceeded
*  by a call to XuSetStateStore() and the read and write functions reset
*  to the current profile file when finished. If no profile is defined,
*  then all state information is read from and written to the base state file.
*
*  Functions: Any starting with XuVa are variable argument versions:
*
*            State file register functions.
*
*              int  XuSetStateFile()
*              void XuSetDefaultStateFile()
*              void XuSetStateStore
*
*            Get and save data from the state file defined by the global index.
*
*              Boolean XuStateDataGet()
*              int     XuVaStateDataGet()
*              void    XuStateDataSave()
*              void    XuStateDataSaveImmediate()
*              void    XuVaStateDataSave()
*              void    XuVaStateDataSaveImmediate()
*              void    XuStateDataRemove()
*
*            The current profile is handled by the functions:
*
*              int     XuSetProfile()
*              Boolean XuIsActiveProfile()
*              int     XuCreateProfile()
*              int     XuGetProfiles()
*              void    XuDestroyProfile()
*              void    XuSaveProfileStateData()
*              String  XuGetProfileStateData()
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
/*=====================================================================*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "XuP.h"


#define BUFLEN	4095
#define KEYLEN	255
#define PROFILE_LIST		"profilelist"
#define PROFILE_USER_DATA	"profilestatedata"
#define PROFILE_SUFFIX		"_profile"


/* Create single string key from input key strings.
 */
static void create_full_key(String k1, String k2, String k3, String rtnkey)
{
	char   key[KEYLEN+1];
	size_t len = 0;

	(void) memset(key, 0, KEYLEN+1);

	if(!blank(k1))
	{
		(void) strncpy(key, k1, KEYLEN);
		len = strlen(key);
	}
	if(!blank(k2))
	{
		(void) strncat(key, ".", KEYLEN-len);
		len++;
		(void) strncat(key, k2,  KEYLEN-len);
		len = strlen(key);
	}
	if(!blank(k3))
	{
		(void) strncat(key, ".", KEYLEN-len);
		len++;
		(void) strncat(key, k3,  KEYLEN-len);
	}
	(void) strcpy(rtnkey, key);
}


/* Create the default index. The associated file will not come into
 * existance until a write is asked for. This does not normally happen.
 */
static void create_default_index(void)
{
	char *p, buf[BUFLEN];

	p = getenv("HOME");
	if (!p) p = Fxu.home_dir;
	if (!p) p = "/tmp";

	(void) snprintf(buf, BUFLEN, "%s/%s/%s_state", p, XURESDIR, (Fxu.app_name)? Fxu.app_name : "default");

	Fxu.sf = (SFIS *)XTCALLOC(1, SFIS);
	Fxu.sf[Fxu.nsf].fname    = XtNewString(buf);
	Fxu.sf[Fxu.nsf].writable = True;
	Fxu.nsf++;
	Fxu.andx = Fxu.gndx = Fxu.bndx = Fxu.nsf;
}


static void create_directory_tree(String fname)
{
	String path, dir;
	path = XtNewString(fname);
	dir  = dirname(path);
	if(access(dir, R_OK|W_OK))
	{
		create_directory_tree(dir);
		(void) mkdir(dir, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	}
	XtFree(path);
}


static FILE *open_file_for_write(String fname)
{
	FILE *fp = fopen(fname,"w");
	if (!fp)
	{
		create_directory_tree(fname);
		fp = fopen(fname,"w");
	}
	return fp;
}


/* Read the given state file, which must be a full path name. No error message is generated
 * by this function.
 */
static Boolean read_state_data_file(String fname, String k1, String k2, String k3, String line)
{
	Boolean found = False;
	FILE *fp = fopen(fname, "r");
	if(fp)
	{
		char    key[KEYLEN+1];
		String  ptr  = NULL;
		String  bufr = NULL;

		create_full_key(k1, k2, k3, key);
		if (!line) line = bufr = malloc(BUFLEN+1);
		(void) memset(line, 0, BUFLEN+1);

		while(fgets(line, BUFLEN, fp))
		{
			if(!(ptr = strchr(line,':'))) continue;
			*ptr = '\0';
			if(strcmp(key, line)) continue;
			ptr++;
			ptr[strcspn(ptr,"\n\r\f")] = '\0';
			ptr += strspn(ptr," ");
			memmove(line, ptr, strlen(ptr)+1);
			found = !blank(line);
			break;
		}
		(void) fclose(fp);
		if (bufr) (void) free(bufr);
	}
	return found;
}


/* Only issue a read failure the first time it is encountered. Otherwise it gets annoying
 */
static Boolean read_state_data(int ndx, String key1, String key2, String key3, String line)
{
	static Boolean first = True;

	if (!Fxu.sf) create_default_index();
	if (ndx > Fxu.nsf) return False;

	if(access(Fxu.sf[ndx-1].fname, R_OK))
	{
		if (first)
			(void) fprintf(stderr, "ERROR: Unable to access state save file %s\n", Fxu.sf[ndx-1].fname);
		first = False;
		return False;
	}
	return read_state_data_file(Fxu.sf[ndx-1].fname, key1, key2, key3, line);
}



/* The write_data, collect_data and write_state_data functions together
 * act so that when there is a lot of data to be written into the state
 * file it is collected into one batch. This also allows for data which
 * may have an overwrite to be processed only once. Note that a data
 * value of NULL effectively removes an entry. Also note that write_data
 * is passed an index value and not a pointer to the Fxu information
 * because the array pointer might change with allocation.
 */
/*ARGSUSED*/
static void write_data( XtPointer client_data, XtIntervalId id)
{
	int   j;
	char  line[BUFLEN+1];
	FILE *fp, *tp, *tmpfile();
	SFIS *s = Fxu.sf + PTR2INT(client_data) - 1;

	s->id = (XtIntervalId)0;
	if(s->nkd < 1) return;

	/* Write the state data into a temp file. A null data entry
	 * results in the item not being saved
	 */
	tp = tmpfile();
	for(j = 0; j < s->nkd; j++)
	{
		if(blank(s->data[j])) continue;
		(void) fputs(s->key[j],tp);
		(void) fputs(":", tp);
		(void) fputs(s->data[j], tp);
		(void) fputs("\n",tp);
	}

	/* Copy all data but the state data in the input array which was
	 * already copied to the temp file (above) 2005.05.02 fixed bug
	 * which sometimes improperly matched the key in the line.
	 */
	if((fp = fopen(s->fname,"r")))
	{
		while(fgets(line, BUFLEN, fp))
		{
			size_t len = strcspn(line,":");
			for(j = 0; j < s->nkd; j++)
				if(s->keylen[j] == len && strncmp(s->key[j],line,len) == 0) break;
			if(j >= s->nkd) (void) fputs(line, tp);
		}
		(void) fclose(fp);
	}

	/* Copy the data in the temp file to the state file */
	rewind(tp);
	if((fp = open_file_for_write(s->fname)))
	{
		while(fgets(line, BUFLEN, tp)) (void) fputs(line, fp);
		(void) fclose(fp);
	}
	else
	{
		s->writable = False;
		(void) fprintf(stderr,"ERROR: Could not open state file '%s' for write.\n", s->fname);
	}
	(void) fclose(tp);

	/* Free the allocated data memory */
	for(j = 0; j < s->nkd; j++)
	{
		XtFree((void *)s->key[j]);
		XtFree((void *)s->data[j]);
	}
	XtFree((void *)s->key);
	XtFree((void *)s->keylen);
	XtFree((void *)s->data);
	s->key     = NULL;
	s->keylen  = NULL;
	s->data    = NULL;
	s->nkd     = 0;
}


static void collect_data( SFIS *s, String key, String data )
{
	int  i;

	if(blank(key)) return;

	for(i = 0; i < s->nkd; i++)
		if(strcmp(s->key[i],key) == 0) break;

	if(i >= s->nkd)
	{
		s->key     = (String*) XtRealloc((void *)s->key,   (s->nkd+1)*sizeof(String));
		s->keylen  = (size_t*) XtRealloc((void *)s->keylen,(s->nkd+1)*sizeof(size_t));
		s->data    = (String*) XtRealloc((void *)s->data,  (s->nkd+1)*sizeof(String));
		s->key[s->nkd]    = XtNewString(key);
		s->keylen[s->nkd] = strlen(key);
		s->data[s->nkd]   = NULL;
		s->nkd++;
	}
	XtFree((void *)s->data[i]);
	s->data[i] = (blank(data)) ? NULL : XtNewString(data);
}


static void write_state_data(int ndx, String key1, String key2, String key3, String data)
{
	char key[KEYLEN+1];
	SFIS *s;

	if (!Fxu.sf) create_default_index();
	if (ndx > Fxu.nsf) return;

	s = Fxu.sf + ndx - 1;
	if (!s->writable) return;

	create_full_key(key1, key2, key3, key);
	collect_data(s, key, data);

	/* We should always have a display - but if not do an immediate write */
	if(DefaultAppDisplayInfo->display)
	{
		if(s->id) XtRemoveTimeOut(s->id);
		s->id = XtAppAddTimeOut(Fxu.app_context, 500, (XtTimerCallbackProc) write_data, INT2PTR(ndx));
	}
	else
	{
		write_data(INT2PTR(ndx), (XtIntervalId) NULL);
	}
}


/*==================== PRIVATE TO LIBRARY FUNCTIONS =========================*/


/* Save the data immediately without a delayed write.
 */
void _xu_state_data_save(String key1, String key2, String key3, String data)
{
	char key[KEYLEN+1];

	if (!Fxu.sf) create_default_index();
	if (!Fxu.sf[Fxu.andx-1].writable) return;

	create_full_key(key1, key2, key3, key);
	collect_data(Fxu.sf+Fxu.andx-1, key, data);
	write_data(INT2PTR(Fxu.andx), (XtIntervalId) NULL);

	Fxu.andx = Fxu.gndx;
}



/*================== PUBLIC FUNCTIONS ====================*/


/* Set the state store file to be used in the next read or write. Once the
 * read and write functions finish they will revert to the current profile
 * index (gndx). Thus this function must be called before every read or write
 * if other than the current profile is wanted.
 *
 * Parameter: ndx - the state file index as returned from a call to
 *                  XuSetStateFile()
 */
void XuSetStateStore( int ndx )
{
	if(ndx > 0 && ndx <= Fxu.nsf)
		Fxu.andx = ndx;
	else
		fprintf(stderr, "ERROR: XuSetStateStore - index does not refer to a valid state file.\n");
}


/* The writable parameter is for those ocasions when a program may read
*  from the state files but is not allowed to write to them. We do
*  check to see if the file is already registered and if so return 
*  the existing index number.
*/
int XuSetStateFile(String fname, const Boolean writable)
{
	int  i;

	if(blank(fname))
	{
		if (!Fxu.sf) create_default_index();
		return Fxu.gndx;
	}

	/* We need a case sensitive comparison here */
	for(i = 0; i < Fxu.nsf; i++)
	{
		if(!same(fname,Fxu.sf[i].fname)) continue;
		Fxu.sf[i].writable = writable;
		return (i+1);
	}

	/* Look for an empty entry */
	for(i = 0; i < Fxu.nsf; i++)
	{
		if(blank(Fxu.sf[i].fname)) break;
	}

	/* No existing entry so add one */
	if(i >= Fxu.nsf)
	{
		Fxu.nsf++;
		Fxu.sf = (SFIS *)XtRealloc((void *)Fxu.sf, Fxu.nsf*sizeof(SFIS));
	}

	Fxu.sf[i].fname    = XtNewString(fname);
	Fxu.sf[i].writable = writable;
	Fxu.sf[i].profile  = NULL;
	Fxu.sf[i].id       = 0;
	Fxu.sf[i].nkd      = 0;
	Fxu.sf[i].key      = NULL;
	Fxu.sf[i].keylen   = NULL;
	Fxu.sf[i].data     = NULL;

	return (i+1);
}


/* Remove the data associated with the given keys from the state file.
*  Any key given as "*" will be taken as a wild card match and will remove
*  any key in the given position. If a key is given as a string followed
*  by "*" as in "mem*", then the key will match any string starting with
*  "mem".
*/
void XuStateDataRemove(String key1, String key2, String key3)
{
	char   line[KEYLEN+1], found_key[KEYLEN+1];
	String ptr, a1, a2, a3, p1, p2, p3, k1, k2, k3;
	FILE   *fp;

	if (!Fxu.sf) create_default_index();
	if(!Fxu.sf[Fxu.andx-1].writable) return;

	/* Look for any wild card entries
	 */
	a1 = NotNull(key1) ? strchr(key1,'*') : NULL;
	a2 = NotNull(key2) ? strchr(key2,'*') : NULL;
	a3 = NotNull(key3) ? strchr(key3,'*') : NULL;

	if( a1 || a2 || a3 )
	{
		/* Scan through the state file looking for keys that match our wild card input.
		 */
		if((fp = fopen(Fxu.sf[Fxu.andx-1].fname,"r")))
		{
			while(fgets(line, KEYLEN, fp))
			{	/*
				 * Extract the key word
				 */
				line[KEYLEN] = '\0';
				if(!(ptr = strchr(line,':'))) continue;
				*ptr = '\0';
				(void) strcpy(found_key, line);
				/*
				 * Get the pieces of the key.
				 */
				p1 = found_key;
				p2 = p3 = NULL;
				if((p2=strchr(p1,'.'))) { *p2 = '\0'; p2++; }
				if(p2 && (p3=strchr(p2,'.'))) { *p3 = '\0'; p3++; }
				/*
				 * If a key piece matches the wild card representation then accept the entire key.
				 */
				k1 = (a1 && (*a1 == '*' || (p1 && strncmp(p1,key1,(size_t)(a1-key1)) == 0))) ? p1:key1;
				k2 = (a2 && (*a2 == '*' || (p2 && strncmp(p2,key2,(size_t)(a2-key2)) == 0))) ? p2:key2;
				k3 = (a3 && (*a3 == '*' || (p3 && strncmp(p3,key3,(size_t)(a3-key3)) == 0))) ? p3:key3;
				/*
				 * Create a key from the pieces
				 */
				create_full_key(k1, k2, k3, found_key);
				/*
				 * If the constructed key and the read key are the same erase by writing NULL data.
				 */
				if(strcmp(line,found_key) == 0)
					write_state_data(Fxu.andx, k1, k2, k3, NULL);
			}
			(void) fclose(fp);
		}
	}
	else
	{
		write_state_data(Fxu.andx, key1, key2, key3, NULL);
	}
	Fxu.andx = Fxu.gndx;
}


/* Does the profile as defined by the given keys exist?
 */
Boolean XuStateDataExists(String key1, String key2, String key3)
{
	Boolean rtn;
	if (!Fxu.sf) create_default_index();
	rtn = read_state_data(Fxu.andx, key1, key2, key3, NULL);
	Fxu.andx = Fxu.gndx;
	return (rtn);
}


/* Get the data string stored in the current profile file that is associated
 * with the given keys.
 */
Boolean XuStateDataGet(String key1, String key2, String key3, String *data)
{
	char line[BUFLEN+1];

	if (!Fxu.sf) create_default_index();
	*data = NULL;
	if(read_state_data(Fxu.andx, key1, key2, key3, line))
		*data = XtNewString(line);
	Fxu.andx = Fxu.gndx;
	return (*data != NULL);
}


int XuVaStateDataGet(String key1, String key2, String key3, String fmt, ...)
{
	int     count = 0;
	char    line[BUFLEN+1];
	va_list args;

	if (!Fxu.sf) create_default_index();
	if(read_state_data(Fxu.andx, key1, key2, key3, line))
	{
		va_start(args, fmt);
		count = vsscanf(line, fmt, args);
		va_end(args);
	}
	Fxu.andx = Fxu.gndx;
	return count;
}


void XuStateDataSave(String key1, String key2, String key3, String data)
{
	write_state_data(Fxu.andx, key1, key2, key3, data);
	Fxu.andx = Fxu.gndx;
}


void XuVaStateDataSave(String key1, String key2, String key3, String fmt, ...)
{
	char mbuf[BUFLEN];
	va_list args;

	va_start(args, fmt);
	(void) vsnprintf(mbuf, BUFLEN, fmt, args);
	va_end(args);
	write_state_data(Fxu.andx, key1, key2, key3, mbuf);
	Fxu.andx = Fxu.gndx;
}


/*==================== Profile handling functions =======================*/


/* Make the state store key associated with a given profile name.
 * The file is in the same directory as the default state file
 * and has the name .profile_<in_profile> will all spaces replaced
 * with "_".
 */
static String profile_file_name( String in_profile )
{
	String s, fname, path;
	s = XtNewString(Fxu.sf[Fxu.bndx-1].fname);
	path = dirname(s);
	fname = XtMalloc(safe_strlen(s)+safe_strlen(in_profile)+12);
	(void) safe_strcpy(fname, path);
	(void) safe_strcat(fname, "/.profile_");
	(void) safe_strcat(fname, in_profile);
	XtFree(s);
	/* replace all spaces with underscores */
	s = fname;
	while(*s) {if(*s == ' ') *s = '_'; s++;}
	return fname;
}


/* Set the active profile. Note that a profile file must exist and be
 * writable for the profile to be accepted. If the input profile is NULL
 * then the the current profile will be set to the base state file.
 */
void XuSetProfile( String profile )
{
	/* To be a valid profile there needs to be a matching file */
	if(blank(profile))
	{
		Fxu.andx = Fxu.gndx = Fxu.bndx;
	}
	else
	{
		String fname = profile_file_name(profile);
		if(fname != NULL && access(fname, R_OK|W_OK) == 0)
		{
			/* Note that Fxu.sf[Fxu.gndx-1].fname is set by XuSetStateFile */
			Fxu.andx = Fxu.gndx = XuSetStateFile(fname, True);
			XtFree(Fxu.sf[Fxu.gndx-1].profile);
			Fxu.sf[Fxu.gndx-1].profile = XtNewString(profile);
		}
		XtFree(fname);
	}
}


/* Return the name of the active profile. The returned string is an internal
 * static so do not free.
 */
String XuGetActiveProfile(void)
{
	return Fxu.sf[Fxu.gndx-1].profile;
}


Boolean XuIsActiveProfile(String in_profile)
{
	return (same(in_profile,Fxu.sf[Fxu.gndx-1].profile));
}


/* Create a profile by creating a new file, adding the profile to our profile
 * list and to our internal data structures. 
 */
Boolean XuCreateProfile( String profile )
{
	int    ndx;
	char   line[BUFLEN+1];
	size_t len;
	Boolean ok;
	String  fname;
	FILE   *fp1, *fp2;

	if(blank(profile)) return False;
	if (!Fxu.sf) create_default_index();

	/* If the new profile is not the same as our active profile and it does
	 * not already exist we clone the active profile.
	 */
	fname = profile_file_name(profile);
	if(access(fname,R_OK|W_OK) != 0 && !same(profile, Fxu.sf[Fxu.gndx-1].profile))
	{
		if(!blank(Fxu.sf[Fxu.gndx-1].fname) && (fp1 = fopen(Fxu.sf[Fxu.gndx-1].fname,"r")))
		{
			if((ok = ((fp2 = open_file_for_write(fname)) != NULL)))
			{
				char line[BUFLEN+1];
				while(fgets(line, BUFLEN, fp1)) (void) fputs(line, fp2);
				(void) fclose(fp2);
			}
			(void) fclose(fp1);
		}
		else if(!blank(Fxu.sf[Fxu.bndx-1].fname) && (fp1 = fopen(Fxu.sf[Fxu.bndx-1].fname,"r")))
		{
			if((ok = ((fp2 = open_file_for_write(fname)) != NULL)))
			{
				char line[BUFLEN+1];
				while(fgets(line, BUFLEN, fp1)) (void) fputs(line, fp2);
				(void) fclose(fp2);
			}
			(void) fclose(fp1);
		}
		else
		{
			if((ok = ((fp2 = open_file_for_write(fname)) != NULL)))
			{
				(void) fputs("profile", fp2);
				(void) fclose(fp2);
			}
		}
		if (!ok)
		{
			(void) fprintf(stderr,"XuLib: Could not create profile '%s' as file '%s'.\n", profile,fname);
			XtFree(fname);
			return False;
		}
	}

	/* Add the profile to our state structure */
	ndx = XuSetStateFile(fname, True);
	XtFree(Fxu.sf[ndx-1].profile);
	Fxu.sf[ndx-1].profile = XtNewString(profile);
	XtFree(fname);

	/* Add the profile to our profile list in the base state file */
	len = safe_strlen(profile);
	if(read_state_data(Fxu.bndx, PROFILE_LIST, NULL, NULL, line))
	{
		String p, s = line;
		while((p = strchr(s,',')))
		{
			size_t nc = (size_t)(p-s);
			if(nc == len && strncmp(profile, s, nc) == 0)
				return True;
			s = p+1;
		}
		if(same(profile,s)) 
			return True;
		(void) safe_strcat(line,",");
		(void) safe_strcat(line, profile);
	}
	else
	{
		(void) safe_strcpy(line, profile);
	}
	write_state_data(Fxu.bndx, PROFILE_LIST, NULL, NULL, line);

	return True;
}


/* Return an allocated array of sorted strings that contains the names of the profiles.
 */
int XuGetProfiles( String **list )
{
	int     i, n, nlist;
	char    line[BUFLEN+1];
	String  *statelist = NULL;
	String  *reallist  = NULL;

	if (!Fxu.sf) create_default_index();
	if (list) *list = NULL;
	if(!read_state_data(Fxu.bndx, PROFILE_LIST, NULL, NULL, line)) return 0;
	if((n = _xu_parse_comma_separated_list(line, &statelist)) < 1) return 0;

	if (list) reallist = XTCALLOC(n, String);
	for(nlist = 0, i = 0; i < n; i++)
	{
		String fname = profile_file_name(statelist[i]);
		if(access(fname, R_OK|W_OK))
		{
			XtFree((void*)statelist[i]);
		}
		else if(list)
		{
			reallist[nlist++] = statelist[i];
		}
		else
		{
			XtFree((void*)statelist[i]);
			nlist++;
		}
		XtFree((void*)fname);
	}
	XtFree((void*)statelist);

	if(list)
	{
		/* Sort the returned list */
		Boolean sort = True;
		while(sort)
		{
			sort = False;
			for(i = 1; i < nlist; i++)
			{
				String ptr = reallist[i-1];
				if(strcmp(ptr,reallist[i]) > 0)
				{
					sort = True;
					reallist[i-1] = reallist[i];
					reallist[i] = ptr;
				}
			}
		}
		*list = reallist;
	}
	return nlist;
}


/*  Save profile state data.  The input parameters are:
 *
 *		auto_save  = Should the profile be automatically saved on exit from the program?
 *		fcstr_name = The name of the forecaster associated with the profile.
 */
void XuSaveProfileStateData( Boolean auto_save, String fcstr_name )
{
	char buf[BUFLEN];
	
	if(blank(fcstr_name))
		(void) snprintf(buf, BUFLEN, "%s -", (auto_save)? "T":"F");
	else
		(void) snprintf(buf, BUFLEN, "%s %s", (auto_save)? "T":"F", fcstr_name);
	write_state_data(Fxu.gndx, PROFILE_USER_DATA, NULL, NULL, buf);
}


/* Get the state data from a profile. If the profile parameter is XuActiveProfile (NULL)
 * then assume the current profile else look for a profile that matches the input one.
 * The return parameters are:
 *
 *		auto_save  = Should the profile be automatically saved on exit from the program?
 *		fcstr_name = The name of the forecaster associated with the profile.
 */
Boolean XuGetProfileStateData( String profile, Boolean *auto_save, String *fcstr_name )
{
	int     n, ok;
	char    line[BUFLEN+1];
	String  ptr, fname;
	Boolean rtn = False;

	if (auto_save)  *auto_save  = True;
	if (fcstr_name) *fcstr_name = NULL;

	if(Fxu.gndx == Fxu.bndx) return True;

	ptr = blank(profile)? Fxu.sf[Fxu.gndx-1].profile : profile;
	if(!blank(ptr))
	{
		fname = profile_file_name(ptr);
		if(read_state_data_file(fname, PROFILE_USER_DATA, NULL, NULL, line))
		{
			rtn = True;
			ptr = string_arg(line);
			if (auto_save)
				*auto_save = same_start_ic(ptr,"T");

			ptr = string_arg(line);
			if (fcstr_name && !blank(ptr) && !same(ptr,"-"))
			{
				/* a paranoid check to make sure the name is ok */
				for(ok = 0, n = 0; n < safe_strlen(ptr); n++)
					if(!(ok = isalnum((int) ptr[n]))) break;
				if (ok) *fcstr_name = XtNewString(ptr);
			}
		}
		XtFree(fname);
	}
	return rtn;
}


/* Remove the profile file and all references to it and check to see
 * if the current index needs to be reassigned.
 */
void XuDestroyProfile( String profile )
{
	int    n;
	char   line[BUFLEN+1];
	String fname;
	size_t len;
	
	if(blank(profile)) return;
	
	fname = profile_file_name(profile);
	if(!blank(fname)) (void) unlink(fname);
	for(n = 0; n < Fxu.nsf; n++)
	{
		if(!same(profile, Fxu.sf[n].profile)) continue;
		XtFree(Fxu.sf[n].fname);
		XtFree(Fxu.sf[n].profile);
		(void) memset((void *)&Fxu.sf[n], 0, sizeof(SFIS));
		if(n+1 == Fxu.gndx) Fxu.andx = Fxu.gndx = Fxu.bndx;
		break;
	}
	XtFree(fname);

	/* Remove the profile from our internal list */
	len = safe_strlen(profile);
	if(read_state_data(Fxu.bndx, PROFILE_LIST, NULL, NULL, line))
	{
		String p, s = line;
		while((p = strchr(s,',')))
		{
			size_t nc = (size_t)(p-s);
			if(nc == len && strncmp(profile, s, nc) == 0)
				(void) safe_strcpy(s, s+nc+1);
			else
				s = p+1;
		}
		if( s != line && same(s,profile))
			*(s-1) = '\0';
		else if(same(s,profile))
			*line = '\0';
		write_state_data(Fxu.bndx, PROFILE_LIST, NULL, NULL, line);
	}
}
