/*=========================================================================*/
/*
*  File: source.c
*
*  Purpose: Contains functions which deal with the source structure and
*           source information. These functions also check the sources
*           for modification by placing the check function into the timer
*           loop or through use of the inotify system of linux.
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
/*=========================================================================*/

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ingred.h>
#include "resourceDefines.h"
/* Undefine bzero and bcopy to stop compiler complaints */
#undef bzero
#undef bcopy
#include "global.h"
#include "editor.h"
#include "guidance.h"
#include "observer.h"
#include "source.h"
#include "radarSTAT.h"

typedef struct {
	int wd;
	void (*notifyFcn)(XtPointer);
	XtPointer data;
} FileWatchStruct;
/*
 * Local variables.
 */
static int           notify_fd         = 0;
static int           nsource           = 0;
static SourceList    source            = NULL;
static SourceList    src_list          = NULL;
static unsigned long check_interval    = 60000;
static Boolean       check_in_progress = False;
static time_t        newest_field_time = 0;
static SETUP         *setup            = NULL;
static String        module            = "LoadSourceData";
static int           nfilewatch        = 0;
static FileWatchStruct *filewatch      = NULL;
/*
 * The output of any lists will be in this order.
 */
static long type_sort_order[] =
{
	SRC_FPA,
	SRC_NWP,
	SRC_ALLIED,
	SRC_DEPICT,
	SRC_INTERP,
	SRC_BACKUP,
	SRC_IMAGERY,
	SRC_IMPORT,
	SRC_RADAR_STAT
};
/*
 * Forward define of local functions.
 */
static Boolean allied_model_has_field_of_type    (Source, FpaCtimeDepTypeOption);
static void    check_depictions                  (XtPointer, XtIntervalId*);
static void    check_src_update                  (XtPointer, XtIntervalId*);
static Boolean depict_has_field_of_type          (Source, FpaCtimeDepTypeOption);
static Boolean depict_external_has_field_of_type (Source, FpaCtimeDepTypeOption);
static void    error_output                      (int, int, String);
static Source  find_src_by_fld_descript          (FLD_DESCRIPT*);
static int     get_depiction_list                (String**, time_t**);
static Boolean is_data                           (String);
static Boolean nwp_has_field_of_type             (Source, FpaCtimeDepTypeOption, Boolean);
static void    read_inotify                      (XtPointer, int*, XtInputId*);
static int     set_wd                            (Source);
static time_t  src_directory_mod_time            (Source);
/* 
 * Go through the setup file info and put the sources into the source data
 * structure. This is done here to ensure that the sources are pre-filtered
 * for all later use. The filter ensures that if more than one source points
 * to the same data directory that only the first instance of the source
 * will be entered into the list. Some sources, especially those associated
 * with allied models will do this but it is not wanted for presentation to
 * the users. The directory associated with a source does not have to exist
 * when FPA is first run as the notify logic will look for changes in the
 * nearest ancestor directory to determine if and when the directory is created.
 *
 * A source can also be forced to be hidden.  This is for special sources that
 * are used internally but are not meant for selection by the user. An example
 * of this is the automatic import sources. To do this we OR the SRC_HIDDEN key
 * with the source type. Note that the common source of hidden sources must come
 * before the hidden source in the source list. 
 *
 * We also have sources (imagery at the moment) that have one source but point to
 * multiple directories and do not have a source key that references the config
 * files or setup files. Unlike the single directory sources these directories
 * must exist when FPA is run in order for any changes to be detected.
 *
 * The last entry in the setup file for some sources is optional and can be
 * one of "notify" or "no_notify". If notify, then the bookshelf icon on the
 * pulldown menu bar will be activated if anything new arrives in the given source.
 * If no_notify, then no notification will be done.
 *
 * Notify can optionally be followed by a time delta that determines the minimum
 * time between source updates that will result in bookshelf flashing. For example
 * if a NWP model updates (normally about 12 hours apart) and the time delta is
 * 30 minutes, then the first source check will flash the bookshelf. The second
 * will probably occur the next time the source is scanned (say 2 minutes) and
 * show an update. Since this is less than the 30 minute delta specification the
 * bookshelf will not be flashed. The default is given in the inputs structure
 * below as a boolean default corresponding to "notify 0" or "no_notify".
 *
 * The inotify system is used to detect changes in the files and directories.
 * If this fails for some reason, then the program will drop back to using a
 * polling system. The polling system uses the stat function and there seem to be
 * some limitations. The documentation says that a directory's modification time
 * will change if a file is written to. I have found that this is not the case
 * on my system. Thus if there are configuration files in the Map directory that
 * need to be modified, then one must delete the old file first and then copy
 * the updated version in. This does not need to be done with the inotify system.
 *
 * Input sources structure follows. Note that the DEPICT source must be the first
 * source in the structure or things can break. Also AUTOIMPORT must come before
 * FIELD_AUTOIMPORT in the list as AUTOIMPORT is always added by default although it
 * can also appear in the FIELD_AUTOIMPORT block in the setup file so that some
 * parameters can be set (see utilities.c). 
 */

/* So that the code does not have a lot of defines around things I defined the
 * basic code elements here. If the inotify system is not used then none of the
 * functions will actually be called.
 */
#if defined(MACHINE_PCLINUX)
	#define CN           0
	#define CR           IN_CREATE|IN_MOVED_TO|IN_DELETE_SELF|IN_MOVE_SELF
	#define CM           IN_CREATE|IN_MOVED_TO|IN_DELETE_SELF|IN_MOVE_SELF|IN_MODIFY
	#define CD           IN_CREATE|IN_MOVED_TO|IN_DELETE_SELF|IN_MOVE_SELF|IN_DELETE
	#define CMD          IN_CREATE|IN_MOVED_TO|IN_DELETE_SELF|IN_MOVE_SELF|IN_MODIFY|IN_DELETE
	#define FCW          IN_CLOSE_WRITE
	#define NOTIFY       inotify_init()
	#define WATCH(a,b,c) inotify_add_watch(a,b,c)
#else
	struct inotify_event {int wd; int len; char *name;};/* Satisfies the compiler */
	#define CN           0
	#define CR           0
	#define CM           0
	#define CD           0
	#define CMD          0
	#define FCW          0
	#define NOTIFY       0		/* what inotify_init returns on failure */
	#define WATCH(a,b,c) -1		/* what inotify_add_watch returns on failure */
#endif


/* These keys are or'ed together for the action variable below
 */
#define DUPOK        (1L<<1) /* do not complain if duplicate source encountered */
#define NO_INIT_TIME (1L<<2) /* do not initialize last mod time to current directory time */
#define CHECK_DIR    (1L<<3) /* check for duplicate directories and hide all but first one */
#define NOTIFY_USER  (1L<<4) /* Notify the user if the source changes */
#define SETUP_PARMS  (1L<<5) /* Look for paramters in the setup file */
#define SPECIAL      (1L<<6) /* Use special function for processing */
#define IMAGE_DIRS   (1L<<7) /* Special function for image directories */
#define SETUP_DIR    (1L<<8) /* Special function using setup file directory entry */

/* To shorten code line
 */
#define INIT_MOD_TIME(sss) ((inputs[n].action & NO_INIT_TIME)?0:src_directory_mod_time(sss))

static struct {
	String   id;           /* directory key or setup file block identifier key. None if none */
	long     type;         /* bitfield key identifier */
	WTYPE    watch;        /* what inotify events to watch for */
	long     action;       /* or'ed keys defined above */
} inputs[] = {
	{ DEPICT,           SRC_DEPICT,      CN, None                                  },
	{ INTERP,           SRC_INTERP,      CN, None                                  },
	{ BACKUP,           SRC_BACKUP,      CN, None                                  },
	{ MAPS,             SRC_MAPS,        CM, None                                  },
	{ AUTOIMPORT,       SRC_IMPORT,      CR, NO_INIT_TIME                          },
	{ FIELD_AUTOIMPORT, SRC_IMPORT,      CR, SETUP_PARMS | DUPOK | NO_INIT_TIME    },
	{ DEPICT_EXTERNAL,  SRC_FPA,         CR, SETUP_PARMS | NOTIFY_USER             },
	{ NWP_MODELS,       SRC_NWP,        CMD, SETUP_PARMS | NOTIFY_USER             },
	{ ALLIED_MODELS,    SRC_ALLIED,      CD, SETUP_PARMS | CHECK_DIR | NOTIFY_USER },
	{ None,             SRC_IMAGERY,     CD, SPECIAL | IMAGE_DIRS                  },
	{ RADAR_STAT,       SRC_RADAR_STAT, CMD, SPECIAL | SETUP_DIR                   }
};


/* This finally is the function that loads in the source information. One has to
 * be careful here. The specification of source variable load order is important.
 * dir and watch assignment must come before wd assignment and src_directory_mod_time
 * must come after these.
 */
void LoadSourceData(void)
{
	int n, i, ndx;
	long type;
	String dir = NullString;
	String depict_dir = NullString;
	time_t notify_delta;
	Source src;
	FLD_DESCRIPT fd;
	FpaConfigSourceIOStruct  *sio;
	FpaConfigSourceSubStruct *subdef;

	/* This initializes the inotify file and directory change mechanism.
	 */
	if (GV_edit_mode)
	{
		notify_fd = NOTIFY;
		GV_inotify_process_used = (notify_fd > 0);
		/* Set the notify file descriptor to non-blocking mode */
		if(GV_inotify_process_used)
			fcntl(notify_fd, F_SETFL, O_NONBLOCK);
		pr_diag("LoadSourceData","Source change notification procedure: %s\n",
			(GV_inotify_process_used)? "inotify":"polling");
	}

	for(n = 0; n < (int) XtNumber(inputs); n++)
	{
		Boolean watch = (GV_inotify_process_used && inputs[n].watch);

		/* Process for special sources. The codes are:
		 *
		 * IMAGE_DIRS = Image sources with multiple directories. The function called
		 *     must provide a static copy of the directory list
		 * SETUP_DIR = Single directory with the directory defined in the directory
		 *     block of the setup file and accessed by using the associated key.
		 */
		if(inputs[n].action & SPECIAL)
		{
			int    ndirs = 0;
			String *dirs = NULL;
			dir = NULL;

			if(inputs[n].action & IMAGE_DIRS)
				ndirs = glImageInfoGetDirectories(&dirs);
			else if(inputs[n].action & SETUP_DIR)
				dir = get_directory(inputs[n].id);

			if(ndirs > 0)
			{
				for(i = 0; i < ndirs; i++)
				{
					source = MoreMem(source, Source, nsource+1);
					source[nsource]                = OneMem(SourceStruct);
					source[nsource]->dir           = XtNewString(dirs[i]);
					source[nsource]->watch         = inputs[n].watch;
					source[nsource]->wd            = watch? set_wd(source[nsource]) : -1;
					source[nsource]->isdata        = is_data(dirs[i]);
					source[nsource]->type          = inputs[n].type;
					source[nsource]->last_mod_time = INIT_MOD_TIME(source[nsource]);
					source[nsource]->notify_delta  = (inputs[n].action & NOTIFY_USER )? 0 : INT_MAX;
					nsource++;
				}
			}
			else if(!blank(dir))
			{
				source = MoreMem(source, Source, nsource+1);
				source[nsource]                = OneMem(SourceStruct);
				source[nsource]->dir           = XtNewString(dir);
				source[nsource]->watch         = inputs[n].watch;
				source[nsource]->wd            = watch? set_wd(source[nsource]) : -1;
				source[nsource]->isdata        = is_data(dir);
				source[nsource]->type          = inputs[n].type;
				source[nsource]->last_mod_time = INIT_MOD_TIME(source[nsource]);
				source[nsource]->notify_delta  = (inputs[n].action & NOTIFY_USER )? 0 : INT_MAX;
				nsource++;
			}
		}
		/*
		 * Sources with no setup file input are handled here. The id is thus the actual
		 * source directory key. This key is used to directly set the field descriptor.
		 * Little complaining is done here as not all configurations of FPA will have
		 * all of these sources.
		 */
		else if(!(inputs[n].action & SETUP_PARMS))
		{
			init_fld_descript(&fd);
			if(!set_fld_descript(&fd, FpaF_SOURCE_NAME, inputs[n].id, NULL)) continue;

			sio    = fd.sdef->src_io;
			subdef = fd.subdef;
			if(IsNull(sio) || IsNull(subdef)) continue;

			/* This function is used as we need to know what the directory path is
			 * even if it does not actually exist. If should never be NULL if the
			 * source is defined.
			 */
			dir = data_directory_path(sio->src_tag, sio->src_path, subdef->sub_path);
			if(blank(dir))
			{
				pr_error(module, "No data directory associated with source \"%s\".\n", inputs[n].id);
				continue;
			}

			/* Save the depiction directory as we need it for source checking below.
			 * It is ok to do this here as long as DEPICT remains the first source
			 * item in the inputs structure.
			 */
			if (same(DEPICT,inputs[n].id)) depict_dir = XtNewString(dir);

			source = MoreMem(source, Source, nsource+1);
			source[nsource]                = OneMem(SourceStruct);
			source[nsource]->dir           = XtNewString(dir);
			source[nsource]->watch         = inputs[n].watch;
			source[nsource]->wd            = watch? set_wd(source[nsource]) : -1;
			source[nsource]->type          = inputs[n].type;
			source[nsource]->last_mod_time = INIT_MOD_TIME(source[nsource]);
			source[nsource]->isdata        = is_data(dir);
			source[nsource]->notify_delta  = (inputs[n].action & NOTIFY_USER)? 0 : INT_MAX;
			source[nsource]->fd            = OneMem(FLD_DESCRIPT);
			copy_fld_descript(source[nsource]->fd, &fd);
			nsource++;
		}
		/*
		 * Sources that must have entries in the setup file source block. Having no entry
		 * in the setup file is not an error, but if we do have entries then for any errors
		 * we must to lots of complaining.
		 */
		else
		{
			setup = GetSetup(inputs[n].id);
			if(IsNull(setup)) continue;

			for(i = 0; i < setup->nentry; i++)
			{
				int k, m, nids, nparms;
				Boolean hide_src = False;

				/* Find the number of source identifiers. This can be a single identifier or
				 * source-subsource pairs. The parsing here assumes a certain structure in the
				 * setup file where a source having 2 possible specifiers will not have extra
				 * parms on the end but be limited to notify and no_notify or true and false.
				 * Any remaining parameters are put into the source struct variable parms.
				 */
				notify_delta = (inputs[n].action & NOTIFY_USER)? 0 : INT_MAX;
				for(nids = 0, nparms = 0; nparms < 3; nparms++)
				{
					String parm = SetupParm(setup,i,nparms);
					if(blank(parm))
					{
						break;
					}
					else if(same_ic(parm,"no_notify") || same_ic(parm,"nonotify"))
					{
						notify_delta = INT_MAX;
						break;
					}
					else if(same_ic(parm,"notify"))
					{
						notify_delta = 0;
						/* Check for the optional time interval (in minutes) */
						parm = SetupParm(setup,i,nparms+1);
						if (parm) notify_delta = (time_t) (atol(parm) * 60);
						break;
					}
					else if(same_ic(parm,"true") || same_ic(parm,"false"))
					{
						break;
					}
					nids++;
				}

				/* This should never happen! */
				if (!nids)
				{
					error_output(n, i, "No source found");
					continue;
				}
				
				init_fld_descript(&fd);

				/* Note that source-subsource pairs can be separate entries
				 * or source (no sub-source) or source:sub-source.
				 */
				if(nids == 1)
				{
					if(!set_fld_descript(&fd,
						FpaF_SOURCE_NAME, SetupParm(setup,i,0),
						FpaF_END_OF_LIST))
					{
						error_output(n, i, "Unrecognized source specified.");
						continue;
					}
				}
				else
				{
					if(!set_fld_descript(&fd,
						FpaF_SOURCE_NAME,    SetupParm(setup,i,0),
						FpaF_SUBSOURCE_NAME, SetupParm(setup,i,1),
						FpaF_END_OF_LIST))
					{
						error_output(n, i, "Unrecognized source specified.");
						continue;
					}
				}

				/* Check that the file time stamps and the time stamp type expected in
				 * the directory are the same. If not the function will complain.
				 */
				if(!check_source_minutes_in_filenames(SetupParm(setup,i,0)))
					error_output(n, i, "Directory time type (hour or minutes) and files in conflict.");

				/* Complain if a duplicate source found unless stated otherwise.
				 * The most recently found parameters will replace any existing ones.
				 */
				if((src = find_src_by_fld_descript(&fd)))
				{
					if(inputs[n].action & DUPOK)
					{
						for(k = 0; k < MAXSRCPARM; k++) src->parms[k] = NULL;
						m = 0; k = nparms;
						while((src->parms[m] = SetupParm(setup,i,k)))
						{
							m++; k++;
							if(m >= MAXSRCPARM) break;
						}
					}
					else
					{
						error_output(n, i, "Source has been specified more than once.");
					}
					continue;
				}

				sio    = fd.sdef->src_io;
				subdef = fd.subdef;
				if(IsNull(sio) || IsNull(subdef))
				{
					error_output(n, i, "Source field description error.");
					continue;
				}

				/* This function is used as we need to know what the directory path is
				*  even if it does not actually exist. Should never return NULL as long
				*  as the source is defined.
				*/
				dir = data_directory_path(sio->src_tag, sio->src_path, subdef->sub_path);
				if(blank(dir))
				{
					error_output(n, i, "No data directory associated with source.\n");
					continue;
				}

				/* If the source type is identified in the configuration as a depiction
				 * then we do not want to check for updates if it is the depiction source
				 * that we actually edit. Since there can be more than one source defined
				 * as a depiction type we check the source directory against our depict
				 * directory. Change the type in this case.
				 */
				type = same(dir,depict_dir) ? SRC_DEPICT : inputs[n].type;

				source = MoreMem(source, Source, nsource+1);
				source[nsource] = OneMem(SourceStruct);

				/* Assign any remaining setup parameters to the source parms array. These
				 * are normally used by other functions that need source setup information
				 * and are specific to any given source type.
				 */
				m = 0;
				k = nparms;
				while((source[nsource]->parms[m] = SetupParm(setup,i,k)))
				{
					m++;
					k++;
					if(m >= MAXSRCPARM) break;
				}

				/* If the source directory for this source is the same as for a
				 * previous source, we will save it (if requested) as a hidden source.
				 * Only the first source will show up in any list presented to the user.
				 */
				for(ndx = 0; ndx < nsource; ndx++)
				{
					if(!same(dir, source[ndx]->dir)) continue;
					hide_src = (inputs[n].action & CHECK_DIR);
					break;
				}
				source[nsource]->dir           = XtNewString(dir);
				source[nsource]->watch         = inputs[n].watch;
				source[nsource]->wd            = watch? set_wd(source[nsource]) : -1;
				source[nsource]->type          = hide_src? type|SRC_HIDDEN : type;
				source[nsource]->modified      = False;
				source[nsource]->last_mod_time = INIT_MOD_TIME(source[nsource]);
				source[nsource]->isdata        = is_data(dir);
				source[nsource]->notify_delta  = notify_delta;
				source[nsource]->fd            = OneMem(FLD_DESCRIPT);
				copy_fld_descript(source[nsource]->fd, &fd);
				nsource++;
			}
		}
	
	}

	FreeItem(depict_dir);

	/* This is for the SourceListByType() function return.
	*/
	src_list = NewMem(Source, nsource);

	/* If we can use the inotify system register it now else revert back to the polling system.
	 */
	if(GV_inotify_process_used)
	{
		(void) XtAppAddInput(GV_app_context, notify_fd, (XtPointer)XtInputReadMask, read_inotify, NULL);
	}
	else if(GV_edit_mode)
	{
		/* Schedule the first polling check of the source directories */
		float td;
		if(sscanf(XuGetStringResource(RNguidanceCheckInterval, "1"), "%f", &td) == 1)
			check_interval = (unsigned long)(td * 60000.0);
		check_src_update((XtPointer)NULL, NULL);
	}
	else
	{
		/* Not in edit mode - only check the depictions */
		float td;
		time_t *tlist;
		n = get_depiction_list(NULL, &tlist);
		for(i = 0; i < n; i++) newest_field_time = MAX(tlist[i],newest_field_time);
		FreeItem(tlist);
		if(sscanf(XuGetStringResource(RNviewerUpdateInterval, "1"), "%f", &td) == 1)
			check_interval = (unsigned long)(td * 60000.0);
		(void) XtAppAddTimeOut(GV_app_context, check_interval, check_depictions, NULL);
	}
}


/* If the subname is NULL or is entered as a dash (-), we take this as meaning
*  that there is no sub-soruce.
*/
Source FindSourceByName(String sname , String subname )
{
	FLD_DESCRIPT fd;

	init_fld_descript(&fd);
	if(blank(subname) || same(subname,"-"))
	{
		if(!set_fld_descript(&fd,
			FpaF_SOURCE_NAME, sname,
			FpaF_END_OF_LIST)) return (Source)NULL;
	}
	else
	{
		if(!set_fld_descript(&fd,
			FpaF_SOURCE_NAME,    sname,
			FpaF_SUBSOURCE_NAME, subname,
			FpaF_END_OF_LIST)) return (Source)NULL;
	}

	return find_src_by_fld_descript(&fd);
}


/* Return a list of pointers to the sources defined by the given
*  source type which may consist of ore'd source type keys. If type
*  is ore'd with SRC_HIDDEN then return all sources hidden or not.
*  The fld_type parameter will check to ensure that there is at
*  least one field of the type specified in the source. If fld_type
*  has a value of FpaC_TIMEDEP_ANY no check is done.
*
*  NOTE: The returned list is an internal static. Do not free.
*/
void SourceListByType(long type, FpaCtimeDepTypeOption fld_type, SourceList *src, int *nsrc)
{
	int i, j, n;
	Boolean hidden_unwanted, last_run_only, needs_data;

	hidden_unwanted = ((type & SRC_HIDDEN)        == 0);
	last_run_only   = ((type & SRC_LAST_RUN_ONLY) != 0);
	needs_data      = ((type & SRC_HAS_DATA)      != 0);
	type           &= ~(SRC_HIDDEN|SRC_LAST_RUN_ONLY);

	n = 0;
	for(j = 0; j < (int) XtNumber(type_sort_order); j++)
	{
		if(!(type & type_sort_order[j])) continue;
		for(i = 0; i < nsource; i++)
		{
			if(!(type_sort_order[j] & source[i]->type)) continue;
			if(hidden_unwanted && (source[i]->type & SRC_HIDDEN)) continue;
			if(needs_data && !source[i]->isdata) continue;

			if(fld_type != FpaC_TIMEDEP_ANY)
			{
				if(source[i]->type & SRC_DEPICT)
				{
					if(!depict_has_field_of_type(source[i], fld_type)) continue;
				}
				if(source[i]->type & SRC_INTERP)
				{
					if(!depict_has_field_of_type(source[i], fld_type)) continue;
				}
				else if(source[i]->type & SRC_FPA)
				{
					if(!depict_external_has_field_of_type(source[i], fld_type)) continue;
				}
				else if(source[i]->type & SRC_ALLIED)
				{
					if(!allied_model_has_field_of_type(source[i], fld_type)) continue;
				}
				else if(source[i]->type & SRC_NWP)
				{
					if(!nwp_has_field_of_type(source[i], fld_type, last_run_only)) continue;
				}
			}
			src_list[n] = source[i];
			n++;
		}
	}
	if (src)  *src  = src_list;
	if (nsrc) *nsrc = n;
}


/* Return a list of pointers to the sources defined by the given
*  source type which may consist or ore'd source type keys and
*  containing the specified field. If type is ore'd with SRC_HIDDEN
*  then return all sources hidden or not.
*
*  NOTE: The returned list is an internal static. Do not free.
*/
void SourceListByField(long type , FpaConfigFieldStruct *fld , SourceList *src , int *nsrc )
{
	int i, j, k, n, nlist, nv;
	String *list;
	Boolean hidden_unwanted, needs_data;
	FLD_DESCRIPT fd;

	hidden_unwanted = ((type & SRC_HIDDEN)   == 0);
	needs_data      = ((type & SRC_HAS_DATA) != 0);
	type &= ~(SRC_HIDDEN|SRC_LAST_RUN_ONLY);
	n = 0;
	for(j = 0; j < (int) XtNumber(type_sort_order); j++)
	{
		if(!(type & type_sort_order[j])) continue;
		for(i = 0; i < nsource; i++)
		{
			if(!(type_sort_order[j] & source[i]->type)) continue;
			if(hidden_unwanted && (source[i]->type & SRC_HIDDEN)) continue;
			if(needs_data && !source[i]->isdata) continue;

			init_fld_descript(&fd);
			copy_fld_descript(&fd, source[i]->fd);
			nlist = source_run_time_list(&fd, &list);

			if(!set_fld_descript(&fd,
				FpaF_ELEMENT, fld->element,
				FpaF_LEVEL, fld->level,
				FpaF_END_OF_LIST)) continue;

			if(nlist > 0)
			{
				for(k = 0; k < nlist; k++)
				{
					(void) set_fld_descript(&fd, FpaF_RUN_TIME, list[k], FpaF_END_OF_LIST);
					nv = FilteredValidTimeList(&fd, fld->element->elem_tdep->time_dep, NULL);
					if(nv > 0) break;
				}
				nlist = source_run_time_list_free(&list, nlist);
			}
			else
			{
				 nv = FilteredValidTimeList(&fd, fld->element->elem_tdep->time_dep, NULL);
			}
			if(nv > 0)
			{
				src_list[n] = source[i];
				n++;
			}
		}
	}
	if (src)  *src  = src_list;
	if (nsrc) *nsrc = n;
}


/* Generic file watch addition for the specialized functions below.
 */
static int add_file_watch(String fname, uint32_t mask, void (*notifyFcn)(XtPointer), XtPointer data)
{
	int n, wd;
	/*
	 * In order for the monitoring process to work the file
	 * must exist when the watch is implemented.
	 */
	if(access(fname,R_OK|W_OK))
	{
		FILE *fp = fopen(fname, "w");
		if(!fp)
		{
			pr_error(module,"Unable to open file \'%s\' for write-close watch\n",
					fname);
			return -1;
		}
		fclose(fp);
	}

	errno = 0;
	wd = WATCH(notify_fd, fname, mask);
	if(wd < 0)
	{
		String s;
		pr_error(module,"inotify_add_watch() failure for file \'%s\'\n", fname);
		switch(errno)
		{
			case 0: s = "inotify system not compiled in\n";
			case EACCES: s = "File does not have read access.\n"; break;
			case ENOENT: s = "A directory component does not exist.\n"; break;
			case ENOSPC: s = "The  user  limit  on  the  total  number  of inotify watches was reached.\n"; break;
			default: s = "Unspecified error\n";
		}
		pr_error(module,s);
		return -1;
	}
	/*
	 * Is this file already resistered?
	 */
	for(n = 0; n < nfilewatch; n++)
	{
		if(filewatch[n].wd != wd) continue;
		filewatch[n].notifyFcn = notifyFcn;
		filewatch[n].data = data;
		return wd;
	}
	/*
	 * Is there an empty block available?
	 */
	for(n = 0; n < nfilewatch; n++)
	{
		if(filewatch[n].wd >= 0) continue;
		filewatch[n].wd = wd;
		filewatch[n].notifyFcn = notifyFcn;
		filewatch[n].data = data;
		return wd;
	}
	/*
	 * Need array allocation
	 */
	n = nfilewatch++;
	filewatch = MoreMem(filewatch, FileWatchStruct, nfilewatch);
	filewatch[n].wd = wd;
	filewatch[n].notifyFcn = notifyFcn;
	filewatch[n].data = data;
	return wd;
}


/* Add a watch on a file for a write followed by a close.
 *
 * fname     - The full path name of the file
 * notifyFcn - The function to call when the watch is activated. The function
 *             prototype must be void notifyFcn(XtPointer).
 * data      - Data as an XtPointer that is to be passed to the notifyFcn.
 *
 * return: The watch descriptor as returned by inotify_add_watch().
 */
int AddFileWriteCloseWatch(String fname, void (*notifyFcn)(XtPointer), XtPointer data)
{
	return add_file_watch(fname, FCW, notifyFcn, data);
}


void RemoveFileWatch(int wd)
{
	int n;
	for(n = 0; n < nfilewatch; n++)
	{
		if(filewatch[n].wd != wd) continue;
		inotify_rm_watch(notify_fd, wd);
		filewatch[n].wd = -1;
		break;
	}
}


/*=================== LOCAL FUNCTIONS ==========================*/


static Source find_src_by_fld_descript(FLD_DESCRIPT *fdp)
{
	int i;
	for(i = 0; i < nsource; i++)
	{
		if(same_fld_descript(fdp, source[i]->fd)) return source[i];
	}
	return ((Source) NULL);
}

static void error_output(int input_ndx, int setup_line, String msg)
{
	int    j = 0;
	size_t len = 0;
	char   *ptr, buf[200];

	if(!blank(msg))
	{
		(void) snprintf(buf, sizeof(buf), "%s\n", msg);
		pr_error(module, buf);
	}
	(void) strcpy(buf, "");
	while((ptr = SetupParm(setup, setup_line, j)))
	{
		(void) snprintf(buf + len, sizeof(buf)-len, "\'%s\'  ", ptr);
		len = safe_strlen(buf);
		j++;
	}
	pr_error("setup", "Setup file block [%s] line: %s\n", inputs[input_ndx].id, buf);
}


/*  Determines if the given numerical guidance model has any fields of the given type. */
static Boolean nwp_has_field_of_type(Source src, FpaCtimeDepTypeOption type, Boolean last_run_only )
{
	int i, j, n, nlist, nvl, ndx;
	String *list, *vl;
	Boolean found;
	FLD_DESCRIPT fd;

	found = False;

	copy_fld_descript(&fd, src->fd);
	nlist = source_run_time_list(&fd, &list);
	ndx = last_run_only ? MIN(nlist,1) : nlist;

	for(i = 0; i < ndx; i++)
	{
		if(!set_fld_descript(&fd, FpaF_RUN_TIME, list[i], FpaF_END_OF_LIST)) continue;
		for(j = 0; j < GV_nfield; j++)
		{
			if(GV_field[j]->info->element->elem_tdep->time_dep != type) continue;
			(void) set_fld_descript(&fd,
				FpaF_ELEMENT, GV_field[j]->info->element,
				FpaF_LEVEL, GV_field[j]->info->level,
				FpaF_END_OF_LIST);
			nvl = FilteredValidTimeList(&fd, type, &vl);
			for(n = 0; n < nvl; n++)
			{
				(void) set_fld_descript(&fd, FpaF_VALID_TIME, vl[n], FpaF_END_OF_LIST);
				found = check_retrieve_metasfc(&fd);
				if(found) break;
			}
			(void)FilteredValidTimeListFree(&vl, nvl);
			if(found) break;
		}
		if(found) break;
	}
	(void)source_run_time_list_free(&list, nlist);

	return found;
}


/*  Determines if the given allied model creates any fields of the
*   given type. Note that type can be any of the time dependencies
*   or'ed together to search for more than one type.
*/
static Boolean allied_model_has_field_of_type( Source src, FpaCtimeDepTypeOption type )
{
	int i;

	if(IsNull(src->fd->sdef))                    return False;
	if(IsNull(src->fd->sdef->allied))            return False;
	if(IsNull(src->fd->sdef->allied->metafiles)) return False;

	for(i = 0; i < src->fd->sdef->allied->metafiles->nfiles; i++)
	{
		if(IsNull(src->fd->sdef->allied->metafiles->flds[i]->element)) return False;
		if(src->fd->sdef->allied->metafiles->flds[i]->element->elem_tdep->time_dep == type)
			return True;
	}
	return False;
}


/*  Determines if any fields of the given type are in the depiction sequence. */
/*ARGSUSED*/
static Boolean depict_has_field_of_type(Source src, FpaCtimeDepTypeOption type )
{
	int i, nfld;
	String *elm, *lev;

	(void) GEStatus("FIELDS", &nfld, &elm, &lev, NULL);
	for(i = 0; i < GV_nfield; i++)
	{
		if(GV_field[i]->info->element->elem_tdep->time_dep != type) continue;
		if(!InFieldList(GV_field[i], nfld, elm, lev, NULL)) continue;
		return True;
	}
	return False;
}


static Boolean depict_external_has_field_of_type(Source src, FpaCtimeDepTypeOption type)
{
	int          j, n, nvl;
	String      *vl;
	Boolean      found;
	FLD_DESCRIPT fd;

	copy_fld_descript(&fd, src->fd);
	for(j = 0; j < GV_nfield; j++)
	{
		if(GV_field[j]->info->element->elem_tdep->time_dep != type) continue;
		(void) set_fld_descript(&fd,
			FpaF_ELEMENT, GV_field[j]->info->element,
			FpaF_LEVEL, GV_field[j]->info->level,
			FpaF_END_OF_LIST);
		nvl = FilteredValidTimeList(&fd, type, &vl);
		for(found = False, n = 0; n < nvl; n++)
		{
			(void) set_fld_descript(&fd, FpaF_VALID_TIME, vl[n], FpaF_END_OF_LIST);
			if((found = check_retrieve_metasfc(&fd))) break;
		}
		(void)FilteredValidTimeListFree(&vl, nvl);
		if (found) return True;
	}
	return False;
}


static time_t src_directory_mod_time(Source src)
{
	int    n;
	time_t dt = 0;
	char   dir[1024];
	struct stat stbuf;

	if(!blank(src->ancestor))
	{
		if(stat(src->ancestor, &stbuf) == 0)
			dt = stbuf.st_mtime;
	}
	else if(!blank(src->dir))
	{
		/* The following allows for those cases where a sub-directory is
		*  modified without the primary directory time being changed.
		*/
		(void)strcpy(dir, src->dir);
		while(stat(dir, &stbuf) == 0)
		{
			dt = MAX(dt, stbuf.st_mtime);
			if(safe_strlen(dir)+safe_strlen(FpaFile_Prev)+2 > sizeof(dir)) break;
			(void)strcat(dir, "/");
			(void)strcat(dir, FpaFile_Prev);
		}
	}
	return dt;
}


/* Check to see if the directory and all previous directories are empty (no valid files).
 * A valid file is considered to be not hidden and not the date stamp file.
*/
static Boolean is_data(String dir)
{
	char          *mbuf;
	DIR           *dirpt;
	struct dirent *dp;
	struct stat   stbuf;
	Boolean       rtn = False;

	if(!blank(dir) && NotNull(dirpt = opendir(dir)))
	{
		while(!rtn && NotNull(dp = readdir(dirpt)))
		{
			if(*dp->d_name == '.') continue;
			if((mbuf = AllocPrint("%s/%s", dir, dp->d_name)))
			{
				if(stat(mbuf, &stbuf) == 0)
				{
					if(S_ISDIR(stbuf.st_mode))
					{
						rtn = (same(dp->d_name,FpaFile_Prev)) ? is_data(mbuf):False;
					}
					else
					{
						rtn = !same(dp->d_name,FpaFile_Dstamp);
					}
				}
				XtFree(mbuf);
			}
		}
		(void)closedir(dirpt);
	}
	return rtn;
}


/* Called when all of the registered observer functions have been called.
 */
static void done_source_data_check(void)
{
	int n;
	for(n = 0; n < nsource; n++)
		source[n]->modified = False;	
	check_in_progress = False;
}


/*-------------- Source change polling functions --------------------*/


/* A work procedure is used here to make sure that the source checking 
 * procedure is a unobtrusive as possible.
*/
/*ARGSUSED*/
static Boolean check_source_wp( XtPointer client_data )
{
	time_t dt;
	static int     ndx    = 0;
	static Boolean dirmod = False;
	static Boolean notify = False;

	if(ndx < nsource && !check_in_progress)
	{
		if(source[ndx]->type & (SRC_DEPICT|SRC_INTERP|SRC_BACKUP))
		{
			/* We don't want to process these! */
		}
		else
		{
			/* The following allows for those rare cases where a sub-directory
			*  is modified without the primary source directory being changed.
			*/
			dt = src_directory_mod_time(source[ndx]);
			if(source[ndx]->last_mod_time != dt)
			{
				dirmod = True;
				if((dt - source[ndx]->last_mod_time) > source[ndx]->notify_delta) notify = True;
				source[ndx]->modified = True;
				source[ndx]->last_mod_time = dt;
				if(!source[ndx]->isdata) source[ndx]->isdata = is_data(source[ndx]->dir);
			}
		}
		ndx++;
		return False;
	}
	else
	{
		if (dirmod)
		{
			/* Something has changed. Notify all of the registered observers of this.
			 * Note that it is up to the registered functions to determine if their
			 * data has changed by using the source[n]->modified flag. The notify flag
			 * will be true if any one of the sources meets the notify_delta requirement.
			 */
			check_in_progress = True;
			NotifySourceObservers(done_source_data_check, notify);
		}

		check_src_update((XtPointer)NULL, NULL);
		ndx    = 0;
		dirmod = False;
		notify = False;
		return True;
	}
}


/* Function to run in the timer loop to check for changes in source
*  update times. Note that the first time in we put ourselves into
*  the loop with a timeout of only one second. This will ensure that
*  the sources will be checked immediately after the program starts.
*/
/*ARGSUSED*/
static void check_src_update(XtPointer client_data , XtIntervalId *id )
{
	static Boolean      first      = False;
	static XtIntervalId current_id = (XtIntervalId)NULL;

	if(first)
	{
		first = False;
		current_id = XtAppAddTimeOut(GV_app_context, 60000, check_src_update, NULL);
	}
	else if(id)
	{
		if(check_in_progress)
		{
			current_id = XtAppAddTimeOut(GV_app_context, check_interval, check_src_update, NULL);
		}
		else
		{
			current_id = (XtIntervalId)NULL;
			(void) XtAppAddWorkProc(GV_app_context, check_source_wp, (XtPointer)NULL);
		}
	}
	else
	{
		if(current_id) XtRemoveTimeOut(current_id);
		current_id = XtAppAddTimeOut(GV_app_context, check_interval, check_src_update, NULL);
	}
}


/*------------- Viewer mode depiction change detection -----------------*/


/*ARGSUSED*/
static void check_depictions(XtPointer client_data , XtIntervalId *id )
{
	int i, j, nlist, nfld;
	char mbuf[128];
	time_t dt, *tlist;
	String *dtl, *list, *elem, *level, vtime, fname;
	Boolean active_off = False;
	FLD_DESCRIPT fd;
	FpaConfigFieldStruct *fptr;
	FpaConfigElementStruct *eptr;
	FpaConfigLevelStruct *lptr;
	Boolean found_one = False;


	/* Check to see if any depictions have disappeared.
	*/
	init_fld_descript(&fd);
	(void) set_fld_descript(&fd, FpaF_SOURCE_NAME, DEPICT, FpaF_END_OF_LIST);
	nlist = FilteredValidTimeList(&fd, FpaC_TIMEDEP_ANY, &list);
	dtl = (GV_ndepict > 0)? NewStringArray(GV_ndepict):NULL;
	nfld = 0;
	for(i = GV_ndepict-1; i >= 0; i--)
	{
		if(InTimeList(GV_depict[i], list, nlist, NULL)) continue;
		dtl[nfld] = XtNewString(GV_depict[i]);
		nfld++;
	}
	(void)FilteredValidTimeListFree(&list, nlist);

	for(i = 0; i < nfld; i++) RemoveDepiction(dtl[i]);
	FreeList(dtl, nfld);

	/* Check to see if any fields have disappeared.
	*/
	nlist = get_depiction_list(&list, &tlist);

	strcpy(mbuf, "FIELDS ");
	for(i = 0; i < GV_ndepict; i++)
	{
		strcpy(&mbuf[7], GV_depict[i]);
		(void) GEStatus(mbuf, &nfld, &elem, &level, NULL);
		for(j = 0; j < nfld; j++)
		{
			/* Get the "new format" file identifiers */
			fname = construct_file_identifier(elem[j], level[j], GV_depict[i]);
			if(InList(fname, nlist, list, NULL)) continue;
			/* If no luck try the old format */
			fname = build_file_identifier(elem[j], level[j], GV_depict[i]);
			if(InList(fname, nlist, list, NULL)) continue;
			/* No luck so delete the field */
			(void) IngredVaCommand(GE_SEQUENCE, "DELETE_FIELD %s %s %s",
				elem[j], level[j], GV_depict[i]);
			RemoveField(FindField(elem[j], level[j]));
		}
	}

	/* Check to see if any fields have been modified. If the field is of normal
	*  type then create a depiction for it if there is not one already at the
	*  time of the field. Turn off the active field if any fields are found to
	*  have been updated. This reduces flashing when multiple fields are 
	*  modified.
	*/
	dt = newest_field_time;
	for(i = 0; i < nlist; i++)
	{
		if(tlist[i] <= newest_field_time) continue;
		if(tlist[i] > dt) dt = tlist[i];
		if(!parse_file_identifier(list[i], &eptr, &lptr, &vtime)) continue;
		if(	eptr->elem_tdep->time_dep == FpaC_NORMAL )
		{
			if( !InTimeList(vtime, GV_depict, GV_ndepict, NULL) &&
				!CreateDepiction(vtime)) continue;
		}
		if(!active_off) DeactivateMenu();
		active_off = True;
		(void) IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s - - %s %s %s %s",
			DEPICT, eptr->name, lptr->name, vtime, vtime);
		fptr = get_field_info(eptr->name, lptr->name);
		AddField(fptr, True);
		found_one = True;
	}
	newest_field_time = dt;

	if(active_off) ActivateMenu();

	FreeItem(tlist);
	(void) XtAppAddTimeOut(GV_app_context, check_interval, check_depictions, NULL);

	/* If at least one field modified then send a notification.
	 */
	if(found_one)
	{
		String parm[1] = {NULL};
		NotifyObservers(OB_FIELD_AVAILABLE, parm, 1);
	}
}


/* Return a list of depiction names and last modified times.
*/
static int get_depiction_list(String **list , time_t **tlist )
{
	int i, nl;
	time_t *dt;
	String *l;
	struct stat sbuf;

	nl = dirlist(source_directory_by_name(DEPICT, NULL, NULL), ":", &l);
	dt = NewMem(time_t, nl);
	for(i = 0; i < nl; i++)
	{
		stat(source_path_by_name(DEPICT, NULL, NULL, l[i]), &sbuf);
		dt[i] = sbuf.st_mtime;
	}
	if (list)  *list = l;
	if (tlist) *tlist = dt;
	return nl;
}


/*---------- Functions used with the inotify system ----------------*/


static Boolean watched_file(int wd)
{
	int n;
	for(n = 0; n < nfilewatch; n++)
	{
		if(filewatch[n].wd != wd) continue;
		if(filewatch[n].notifyFcn)
			filewatch[n].notifyFcn(filewatch[n].data);
		return True;
	}
	return False;
}

/* Set a watch on the source directory if it exists. If not set a watch on
 * the nearest ancestor directory that exists.
 */
static int set_wd(Source src)
{
	int wd;

	FreeItem(src->ancestor);

	if(blank(src->dir))
	{
		wd = -1;
		pr_error(module,"Blank source directory entry found.\n");
	}
	else if(access(src->dir,F_OK) == 0)
	{
		wd = WATCH(notify_fd, src->dir, src->watch);
		pr_diag(module,"Watching directory \'%s\' with watch id = %d\n", src->dir,wd);
	}
	else
	{
		src->ancestor = XtNewString(src->dir);
		while(access(src->ancestor,F_OK))
		{
			String c = strrchr(src->ancestor,'/');
			if(c) *c = '\0';
			else
			{
				FreeItem(src->ancestor);
				pr_error(module, "Directory %s does not exist and no ancestor found.\n", src->dir);
				return -1;
			}
		}
		wd = WATCH(notify_fd, src->ancestor, src->watch);
		pr_diag(module,"Directory \'%s\' does not exist. Watching ancestor \'%s\' with watch id = %d\n",
			src->dir, src->ancestor, wd);
	}
	return wd;
}


/* Only remove the watch if no other source is using it
 */
static void remove_watch(Source src)
{
	int n;
	Boolean wd_used = False;
	for(n = 0; n < nsource; n++)
	{
		if(source[n] == src) continue;
		if(source[n]->wd != src->wd) continue;
		wd_used = True;
		break;
	}
	if(!wd_used)
	{
		inotify_rm_watch(notify_fd, src->wd);
		pr_diag(module,"Removing watch for \'%s\' id = %d\n",
			NotNull(src->ancestor)? src->ancestor:src->dir, src->wd);
	}
}


/* If a monitored change happens for an ancestor directory of the source
 * directory check to see if our source directory has been created. If
 * not check to see if the ancestor is still the same. If not reset the
 * ancestor watch.
 */
static Boolean process_ancestor(Source src)
{
	int n;

	if(access(src->dir,F_OK) == 0)
	{
		remove_watch(src);
		FreeItem(src->ancestor);
		src->wd = WATCH(notify_fd, src->dir, src->watch);
		src->modified = True;
		pr_diag(module,"Directory \'%s\' now exists and put under watch id = %d\n",
			src->dir, src->wd);
	}
	else
	{
		String dir = XtNewString(src->dir);
		while(access(dir,F_OK))
		{
			String c = strrchr(dir,'/');
			if(c) *c = '\0';
			else break;
		}
		if(!same(dir, src->ancestor))
		{
			int wd = WATCH(notify_fd, dir, src->watch);
			pr_diag(module,"Reassigning watched ancestor from \'%s\' with watch id = %d to \'%s\' with watch id = %d\n",
				src->ancestor, src->wd, dir, wd);
			remove_watch(src);
			FreeItem(src->ancestor);
			src->ancestor = XtNewString(dir);
			src->wd = wd;
		}
		FreeItem(dir);
		src->modified = False;
	}
	return src->modified;
}


/* To avoid handling the events one at a time when they come in a flood,
 * of multiple event notifications, this function provides delays in
 * processing. For normal guidance sources this is a 6 second delay between
 * the last event and the action of notification. If there are enough events
 * so that this function has been called for more than 60 seconds without
 * actually triggering, then a notification is forced. The 6 second delay
 * is to allow time for the program that converts guidance fields into
 * splines to do it job.
 */
static void buffer_inotify_events(XtPointer client_data , XtIntervalId *id )
{
	int     n;
	time_t  del_time = 0;

	static Boolean      notify     = False;
	static time_t       begin_time = 0;
	static XtIntervalId current_id = (XtIntervalId)NULL;

	if(client_data) notify = True;
	if(!begin_time) begin_time = time(NULL);
	if(!id && current_id) XtRemoveTimeOut(current_id);

	del_time = time(NULL) - begin_time;

	if(check_in_progress)
	{
		current_id = XtAppAddTimeOut(GV_app_context, 500, buffer_inotify_events, NULL);
	}
	else if(((id != NULL && del_time >= 6) || del_time >= 60))
	{
		Boolean source_change = False;
		begin_time = 0;
		current_id = (XtIntervalId)NULL;

		for(n = 0; n < nsource; n++)
		{
			if(!source[n]->modified) continue;

			if(source[n]->force_notify)
			{
				/* A directory has been removed so check for data existance but
				 * only if there is a single directory to check. Ignore the
				 * multiple directory case as isdata is always true.
				 */
				source_change = True;
				if(NotNull(source[n]->dir))
					source[n]->isdata = is_data(source[n]->dir);
			}
			else if(IsNull(source[n]->ancestor) || process_ancestor(source[n]))
			{
				/* If not responding to an ancestor or if processing the ancestor
				 * shows that the wanted data directory now exists check for data
				 * if we don't already have some.
				 */
				source_change = True;
				if(!source[n]->isdata)
					source[n]->isdata = is_data(source[n]->dir);
			}
		}
		if(source_change)
		{
			pr_diag(module,"Notifying source observers of changes.\n");
			check_in_progress = True;
			NotifySourceObservers(done_source_data_check, notify);
		}
		notify = False;
	}
	else
	{
		current_id = XtAppAddTimeOut(GV_app_context, 6000, buffer_inotify_events, NULL);
	}
}


/* The function called by select input when something arrives on the inotify
 * file descriptor.
 */
/*ARGSUSED*/
static void read_inotify(XtPointer client_data , int *src , XtInputId *id )
{
	int    len        = 0;
	int    clen       = 0;
	int    i          = 0;
	Boolean modified  = False;
	long   notify     = 0;
	int    event_size = sizeof(struct inotify_event);
	int    flen       = event_size + FILENAME_MAX;
	int    blen       = flen * 2;
	int    max_size   = flen * 256;
	STRING buf        = XtMalloc(blen);
	STRING ptr        = buf;

	/* Read on the notify descriptor. This is done in stages as we do not
	 * know in advance how big the read needs to be. Increment the buffer
	 * until we get a valid size. Check for a reasonable allocation in case
	 * something goes wrong. The return seems to be in block sizes related
	 * to the inotify_event struct. The starting size should be adequate
	 * for most file name lengths. notify_fd was set non-blocking.
	 */
	while((len = read(notify_fd, ptr, flen)) > 0)
	{
		if(len + clen + flen >= blen)
		{
			blen += flen;
			buf = XtRealloc(buf, blen);
		}
		clen += len;
		if(clen > max_size) break;
		ptr = buf + clen;
	}

	while(i < clen)
	{
		int n;
		struct inotify_event *event = (struct inotify_event *)&buf[i];
		i += (event_size + event->len);
		if(event->wd < 0) continue;
		if(watched_file(event->wd)) continue;
		/*
		 * Scan the sources looking for wd match.
		 */
		for(n = 0; n < nsource; n++)
		{
			Boolean dir_delete = (event->mask & IN_DELETE_SELF || event->mask & IN_MOVE_SELF);
			/*
			 * If the source is already flagged as modified skip it.
			 */
			if(!dir_delete && source[n]->modified)
			{
				modified = True;
				continue;
			}
			if(source[n]->wd != event->wd) continue;

			modified = True;
			source[n]->modified = True;
			source[n]->force_notify = False;

			if(dir_delete)
			{
				pr_diag(module,"Directory \'%s\' has been moved or deleted.\n", source[n]->dir);
				remove_watch(source[n]);
				source[n]->wd = set_wd(source[n]);
				source[n]->last_mod_time = src_directory_mod_time(source[n]);
				source[n]->force_notify = True;
			}
			else if(event->len > 0)
			{
				String dirbuf = AllocPrint("%s/%s",source[n]->dir,event->name);
				if(dirbuf)
				{
					struct stat sbuf;
					if(stat(dirbuf, &sbuf) == 0)
					{
						if((sbuf.st_mtime - source[n]->last_mod_time) > source[n]->notify_delta) notify = 1;
						source[n]->last_mod_time = sbuf.st_mtime;
					}
					XtFree(dirbuf);
				}
			}
		}
	}
	FreeItem(buf);
	if(modified) buffer_inotify_events((XtPointer) notify, NULL);
}
