/********************************************************************************/
/*
 *  File: monitor.c
 *
 *  Purpose: To monitor directories and files within those directories looking
 *           for 2 or more files that arrive with the same time stamp. When all
 *           of the files arrive an external program is run.
 */
/********************************************************************************/
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include "rankweight.h"
#include "monitor.h"

/* Structure to hold information on directories and files that are monitored
 * for existance that will lead to launching the tracking program.
 */
#define LENTL	10

typedef struct {
	STRING name_mask;
	STRING time_mask;
	time_t tlist[LENTL];
} FINFO;

typedef struct {
	int    wd;			/* watch directory inotify assigned key */
	STRING dir;			/* watched directory */
	int    nfile_info;
	FINFO  *file_info;
	int    nfiles;
	STRING *files;
} MONITOR;


static int     nmonitored = 0;			/* How many file masks are monitored */
static int     nmonitor = 0;			/* number of monitor structures */
static MONITOR *monitor = NULL;			/* file monitor structure */
static STRING  monitor_program = NULL;


/* Read the configuration blocks that deal with monitoring urp files.
 */
LOGICAL read_monitor_config(xmlNodePtr root, STRING logfile)
{
	int n, ndx;
	STRING value, p;
	xmlNodePtr top, cur, node, data;
	LOGICAL rtn = TRUE;

	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,"MONITOR")) continue;

		value = xmlGetProp(top,"program");
		monitor_program = safe_strdup(env_sub(value));
		xmlFree(value);
		if(!monitor_program)
		{
			printf("\nERROR: Monitor program to launch not specified in config file.\n\n");
			rtn = FALSE;
			break;
		}

		/* Look for the log file keyword and substitute. */
		if((p = strstr(monitor_program,"[LOGFILE]")))
		{
			size_t len = strlen(monitor_program)+strlen(logfile)+1;
			STRING ptr = INITMEM(char, len);
			memset(ptr, 0, len);
			strncpy(ptr, monitor_program, p-monitor_program);
			strcat(ptr, logfile);
			strcat(ptr, p+9);
			FREEMEM(monitor_program);
			monitor_program = ptr;
		}

		for(cur = top->children; cur; cur = cur->next)
		{
			if(cur->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcasecmp(cur->name,"DIR")) continue;

			ndx = nmonitor;
			nmonitor++;
			monitor = GETMEM(monitor, MONITOR, nmonitor);
			memset((void *) &monitor[ndx], 0, sizeof(MONITOR));
			value = xmlGetProp(cur,"keyword");
			monitor[ndx].dir = get_directory(value);
			if(!monitor[ndx].dir || access(monitor[ndx].dir,R_OK))
			{
				printf("\nERROR: Unable to monitor directory given by keyword property \'%s\'\n\n",
					value);
				rtn = FALSE;
			}
			xmlFree(value);

			for(node = cur->children; node; node = node->next)
			{
				if(node->type != XML_ELEMENT_NODE) continue;
				if(xmlStrcasecmp(node->name,"FILE")) continue;

				nmonitored++;
				n = monitor[ndx].nfile_info;
				monitor[ndx].nfile_info++;
				monitor[ndx].file_info = GETMEM(monitor[ndx].file_info, FINFO, monitor[ndx].nfile_info);
				memset((void *)&monitor[ndx].file_info[n], 0, sizeof(FINFO));

				for(data = node->children; data; data = data->next)
				{
					if(data->type != XML_ELEMENT_NODE) continue;
					value = xmlNodeGetContent(data);
					if(!value) continue;
					if(xmlStrcasecmp(data->name,NAME_MASK) == 0)
					{
						monitor[ndx].file_info[n].name_mask = malloc(strlen(value)+2);
						strcpy(monitor[ndx].file_info[n].name_mask, "^");
						strcat(monitor[ndx].file_info[n].name_mask, value);
					}
					else if(xmlStrcasecmp(data->name,TIME_MASK) == 0)
					{
						monitor[ndx].file_info[n].time_mask = safe_strdup(value);
					}
					xmlFree(value);
				}
			}
		}
		break;
	}

	for(ndx = 0; ndx < nmonitor; ndx++)
	{
		for(n = 0; n < monitor[ndx].nfile_info; n++)
		{
			STRING fmt = "ERROR: No file %s mask for monitoring in one of the files for directory \'%s\'\n";
			if(!monitor[ndx].file_info[n].name_mask)
			{
				printf(fmt, "name", monitor[n].dir);
				rtn = FALSE;
			}
			if(!monitor[ndx].file_info[n].time_mask)
			{
				printf(fmt, "time", monitor[n].dir);
				rtn = FALSE;
			}
		}
	}
	return rtn;
}


void set_monitor_watch(int fd)
{
	int n;
	unsigned long mask = IN_MODIFY|IN_MOVED_FROM|IN_CREATE|IN_DELETE_SELF;
	
	for(n = 0; n < nmonitor; n++)
	{
		monitor[n].wd = watch_dir(fd, monitor[n].dir, mask);
		if(monitor[n].wd > 0)
			printlog("Beginning watch of directory \'%s\'", monitor[n].dir);
		else
			printlog("ERROR: Unable to set watch on directory \'%s\'", monitor[n].dir);
	}
}


/* If an event related to the monitor list is found add the file to our list
 * of files that have been detected as arriving.
 */
void add_to_monitor_list( queue_entry_t event )
{
	int i, n;

	if(event->inot_ev.len <= 0) return;

	for(i = 0; i < nmonitor; i++)
	{
		if(event->inot_ev.wd != monitor[i].wd) continue;
		for(n = 0; n < monitor[i].nfiles; n++)
		{
			if(xmlStrcmp(event->inot_ev.name, monitor[i].files[n]) == 0) return;
		}
		monitor[i].files = GETMEM(monitor[i].files, STRING, monitor[i].nfiles+1);
		monitor[i].files[monitor[i].nfiles] = strdup(event->inot_ev.name);
		monitor[i].nfiles++;
		break;
	}
}



/* Sort function for a time_t variable. This will leave the
 * greatest value in the first element of the storted array
 */
static int tlistcmp(const void *a, const void *b)
{
	return (int)(*((time_t *)b) - *((time_t *)a));
}


/* Process the monitored directories. If files arrive in all of the directories
 * that have the same valid time then the processing program as set in the
 * config file is run.
 */
void process_monitor_events(void)
{
	int i, j, k, n, status, count, ntlist;
	LOGICAL launch = FALSE;
	time_t tlist[LENTL];
	struct tm dt;
	regex_t regptr;

	if(nmonitor <= 0) return;

	/* Run through all of the monitored directories and files in the directories
	 * and add the valid time of the file to the time list associated with the
	 * specific file.
	 */
	for(i = 0; i < nmonitor; i++)
	{
		for(j = 0; j < monitor[i].nfile_info; j++)
		{
			/* Compile the regular expression for the given file mask */
			status = regcomp(&regptr, monitor[i].file_info[j].name_mask, REG_NOSUB);
			if(status)
			{
				printlog("ERROR: Unable to compile file mask \'%s\'",
					monitor[i].file_info[j].name_mask);
				continue;
			}

			/* Run through the file list looking for files that match the name mask */
			for(n = 0; n < monitor[i].nfiles; n++)
			{
				time_t ft;

				/* Filter out any files that do not fit the file mask */
				status = regexec(&regptr, monitor[i].files[n], 0, NULL, 0);
				if(status) continue;

				/* Extract the valid time from the file name using the time_mask */
				memset((void*)&dt, 0, sizeof(struct tm));
				strptime(monitor[i].files[n], monitor[i].file_info[j].time_mask, &dt);
				ft = encode_clock(dt.tm_year+1900, dt.tm_yday+1, dt.tm_hour, dt.tm_min, 0);

				/* Add the time to the time list only if it is new */
				for(k = 0; k < LENTL; k++)
				{
					if(monitor[i].file_info[j].tlist[k] == ft) break;
				}

				/* If new, the time is added to the end of the fixed buffer and then the
				 * buffer is sorted in descending time order. This way the oldest times
				 * are overwritten if the buffer should fill up due to unmatched files.
				 */
				if(k >= LENTL)
				{
					printdebug(">>> Adding file to list: %s/%s", monitor[i].dir, monitor[i].files[n]);				
					monitor[i].file_info[j].tlist[LENTL-1] = ft;
					qsort((void *)monitor[i].file_info[j].tlist, LENTL, sizeof(time_t), tlistcmp);
				}
			}
		}

		/* Free the file list */
		FREELIST(monitor[i].files, monitor[i].nfiles);
		monitor[i].nfiles = 0;
	}

	/* Now check to see if files with the same time exist in every
	 * monitored directory for each monitored file name mask.
	 */
	for(ntlist = 0, n = 0; n < LENTL; n++)
	{
		time_t ft = 0;

		/* Find the first valid time we can */
		for(i = 0; i < nmonitor; i++)
		{
			if(monitor[i].nfile_info == 0) continue;
			ft = monitor[i].file_info[0].tlist[n];
			break;
		}
		if(ft == 0) continue;

		/* Count the number of monitored files that match the time */
		for(count = 0, i = 0; i < nmonitor; i++)
		{
			for(j = 0; j < monitor[i].nfile_info; j++)
			{
				for(k = 0; k < LENTL; k++)
				{
					if(monitor[i].file_info[j].tlist[k] != ft) continue;
					count++;
					break;
				}
			}
		}

		/* If count is the same as the number monitored we launch the program.
		 * Also go through all of the time lists and set to 0 any time
		 * entries that match the valid time from above.
		 */
                
		if(count >= nmonitored)
		{
			launch = TRUE;
			tlist[ntlist++] = ft;
			for(i = 0; i < nmonitor; i++)
			{
				for(j = 0; j < monitor[i].nfile_info; j++)
				{
					for(k = 0; k < LENTL; k++)
					{
						if(monitor[i].file_info[j].tlist[k] == ft)
							monitor[i].file_info[j].tlist[k] = 0;
					}
				}
			}
		}
	}

	/* If the same time was found in every directory launch the processing
	 * program as set in the config file. The program can have time specifiers
	 * as per strftime so process the line before running. The tlist array
	 * is sorted so that tlist[0] contains the most recent time if more than
	 * one was found. In normal operational use this should not happen but ...
	 */
	if(launch)
	{
		char nbuf[100];
		/* Using bif_size instead of sizeof(buf) because sizeof(buf) was returning 8 */
                int  buf_size = safe_strlen(monitor_program) + 30;
		STRING buf = malloc(buf_size);

		qsort((void *)tlist, ntlist, sizeof(time_t), tlistcmp);
		(void) strftime(buf, buf_size, monitor_program, gmtime(tlist));		
		(void) strftime(nbuf, sizeof(nbuf),
				"Files with time %Y%m%d%H%M found in all monitored directories.", gmtime(tlist));
		printdebug(nbuf);
		printdebug("Running program \'%s\'", buf);		
		shrun(buf, FALSE);
		free(buf);
	}
}
