/********************************************************************************/
/*
 *  Program: rankweightDaemon
 *
 *  Purpose: This daemon program has two purposes:
 *
 *           1. To monitor a directory containing radar statictics files
 *           (called SCIT files when they come from URP).
 *
 *				a) If a file is modified or is moved into or created in the
 *				directory the rank weight of the storms is calculated and
 *				forecast values of rank weight produced which are inserted
 *				into the stat files.
 *
 *				b) It can also (optionally) produce forecast files that follow the
 *              time of the last stat data file. The forecast files are tagged as
 *              such by including the property type="fourecast" with the root node. 
 *              Activate this by changing #undef MAKE_FORECAST_FILES found below
 *              to #define MAKE_FORECAST_FILES. This was done as the STAT file reader
 *              will not go beyond the current "T0" and thus producing forecast files
 *              does not make sense anymore.
 *
 *				c) The storms are scanned for the existance of an "Environment"
 *              element. If one is found nothing is done. If not then one is added.
 *              The previous file is scanned and if the element is found then the storm
 *              in the current file is set to the same value. If not a value of "default"
 *              is used.
 *              
 *              d) The most recent STAT is scanned for a "ratio" property associated
 *              with the editable elements of storms. If found a newly arriving STAT
 *              file will have the corresponding element value of the storm modified
 *              by this ratio. This means that any adjustments to values done by the
 *              forecaster will be carried forward to any newly arriving files.
 *
 *           2. To monitor two or more directories that URP puts files into. When
 *           files arrive in all directories that match given templates and having
 *           the same valid time a program specified in the configuration file is
 *           launched.
 *
 *  To Run:  rankweightDaemon start|stop <setup_file>
 *
 *           where 'start' starts up the daemon using information from the given
 *           setup file and 'stop' stops the daemon associated with the setup
 *           file. The setup_file is optional in that if not specified the
 *           setup file defined in the standard FPA environment will be used.
 *
 *           If debug output is wanted then prepend "debug" to the above run
 *           string as in:
 *
 *           rankweightDaemon debug start <setup_file>
 *
 *   Notes:  This program makes use of a configuration file as defined in the
 *           associated setup file. This file is documented both in the config
 *           file itself and at the start of the FPA source file 
 *           radarSTAT_dialog.c
 */
/********************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/resource.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <errno.h>
#include "rankweight.h"
#include "storm_environment.h"
#include "inotify_utils.h"

/* Define this if files containing only forecast rankweight values are wanted */
#undef MAKE_FORECAST_FILES

/* Global variables */
STRING  pgm = "rankweightDaemon";
int     nforecasts = 3;					/* number of forecast files to produce */
int     forecast_time_interval = 10;	/* interval between forecast files in minutes */
int     ndata_points = 3;				/* number of data points needed to create a rank weight forecast */
STRING  trend_program = NULL;			/* external program for calculating forecast rank weights */
int     trend_program_timeout = 10;		/* external program timeout */
STRING  trend_program_infile = NULL;
STRING  trend_program_outfile = NULL;
LOGICAL keep_running = TRUE;			/* program stops when this is unset */
int     stat_dir_wd = 0;				/* Watch directory */
STRING  stat_dir = NULL;				/* Directory where STAT files are found */
STRING  file_mask = NULL;				/* STAT file name mask */
STRING  file_time_mask = NULL;			/* STAT file time mask */
STRING  internal_time_format = NULL;	/* How time is formated in SCIT file internaly */
STRING  rankweight_id = "RankWeight";	/* element id of the rank weight element */
STRING  rank_id = NULL;					/* element id of the rank element (if any) */
int     nrankweight_elem = 0;			/* number of weight calculation elements */
RW_ELEM *rankweight_elem = NULL;		/* list of elements that are used for weight calculation */
int     nrankweight_fcst = 0;
RW_FCST *rankweight_fcst = NULL;
int     nwfiles = 0;					/* number of files written by this daemon */
STRING  *wfiles = NULL;					/* list of files written by this daemon */
queue_t q;								/* event quque */
int     inotify_fd;						/* inotify watch file descriptor */

/* Private variables */
static STRING  pidfile = NULL;			/* program interlock file */
static STRING  logfile  = NULL;			/* file to log messages to when in daemon mode */
static LOGICAL debug = FALSE;			/* debug messages output? */
static STRING  mod_rankweight_program = NULL;


/* The following 4 functions are dummy functions required to satify the loader as
 * the user library of FPA is not used yet the standard library references them.
 */
void userlib_verify(void)
{
	return;
}

LOGICAL identify_user_wind_function(STRING name, WINDFUNC *func, int *nreq)
{
	if (func) *func = NullWindFunc;
	if (nreq) *nreq = 0;
	return FALSE;
}

ERULE identify_user_rule_function(STRING name)
{
	return FALSE;
}

LOGICAL identify_user_value_function(STRING name, VALUEFUNC *func, int *nreq)
{
	if (func) *func = NullValueFunc;
	if (nreq) *nreq = 0;
	return FALSE;
}



/* Functions to print status information to a log file. The
 * file is opened in append mode and then closed once the
 * write is done.
 */
static void printinfo(STRING fmt, va_list ap)
{
	time_t ctime;
	char buf[100];
	FILE *log = NULL;

	log = fopen(logfile,"a");
	if(!log) return;

	/* Add the system time to the start of the log message */
	ctime = time((time_t *)0);
	(void) strftime(buf, sizeof(buf), "%F %T", gmtime(&ctime));
	(void) fprintf(log, "%s ", buf);
	(void) vfprintf(log, fmt, ap);
	(void) fprintf(log,"\n");
	(void) fclose(log);
}


/* Function to print logging messages to the log file preceeded
 * by the system time of the message.
 */
void printlog(STRING fmt, ...)
{
	va_list	ap;
	va_start(ap, fmt);
	printinfo(fmt, ap);
	va_end(ap);
}


void printdebug(STRING fmt, ...)
{
	va_list	ap;
	if(!debug) return;
	va_start(ap, fmt);
	printinfo(fmt, ap);
	va_end(ap);
}



/* Signal handler that simply resets a flag to cause termination */
void signal_handler (int signum)
{
	keep_running = FALSE;
}


/* Determine if there is a copy of the program already running. The program
 * creates a write protected file that contains the pid of the process. This
 * file is checked and if the write lock is on then the program is already
 * running. Termination of the program automatically removes the write lock
 * so this process is good even for daemons that die a horrible death ;-)
 */
static int already_running(void)
{
	int fd;
	char buf[32];
	struct flock fl;

	fd = open(pidfile, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fd < 0)
	{
		printlog("ERROR: can't open %s: %s", pidfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Place a write lock on the file. If the following is true then
	 * the write lock is in place and there is another version running
	 */
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	if(fcntl(fd, F_SETLK, &fl) < 0)
	{
		if(errno == EACCES || errno == EAGAIN)
		{
			close(fd);
			return TRUE;
		}
		printlog("ERROR: can't lock %s: %s", pidfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* truncate the file and write the pid into it */
	ftruncate(fd, 0);
	snprintf(buf, 32, "%ld", (long) getpid());
	write(fd, buf, strlen(buf)+1);
	return FALSE;
}


/* This is the function that actually turns the process into a daemon
 */
static daemonize(void)
{
	int              i, fd0, fd1, fd2;
	pid_t            pid;
	struct rlimit    rl;
	struct sigaction sa;

	/* Clear file creation mask */
	umask(0);

	/* Get max number of file descriptors */
	if(getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		printf("ERROR: can't get file limit");
		exit(EXIT_FAILURE);
	}

	/* Become a session leader */
	if((pid = fork()) < 0)
	{
		printf("ERROR: can't fork");
		exit(EXIT_FAILURE);
	}
	else if(pid != 0) /* parent */
	{
		exit(EXIT_SUCCESS);
	}

	/* Ensure future opens won't allocate controlling TTYs. */
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP, &sa, NULL) < 0)
	{
		printf("ERROR: can't ignore SIGHUP - terminating");
		exit(EXIT_FAILURE);
	}
	/* Yes we fork again to ensure the above non allocation */
	if((pid = fork()) < 0)
	{
		printf("ERROR: can't fork - terminating");
		exit(EXIT_FAILURE);
	}
	else if(pid != 0) /* parent */
	{
		exit(EXIT_SUCCESS);
	}

	/* change current working directory to the root so that
	 * we won't prevent file systems from being unmounted.
	 */
	if(chdir("/") < 0)
		printf("WARNING: can't change directory to \"/\"");

	/* Close all open file descriptors */
	if(rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for(i = 0; i < rl.rlim_max; i++)
		close(i);

	/* Attach descriptors 0, 1 and 2 to /dev/null */
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
}


/* Process the equation constants for the element and save
 */
static RW_ELEM *assign_equation_constants(const xmlChar *name, STRING constants)
{
	int i, j, n, ndx;
	double a1[6], a2[6];
	STRING ptr;
	LOGICAL ok;

	n = 0;
	ptr = strtok(constants,",");
	while(n < 6 && ptr != NULL)
	{
		a1[n++] = double_arg(ptr, &ok);
		if(!ok) printlog("ERROR: failed to read rankWeightCalc constant number %d for element %s", n, name);
		ptr = strtok(NULL,",");
	}

	memset((void*) a2, 0, 6*sizeof(double));
	for(j = 0, i = (6-n); i < 6; i++, j++)
	{
		a2[i] = a1[j];
	}

	ndx = nrankweight_elem;
	nrankweight_elem++;
	rankweight_elem = GETMEM(rankweight_elem, RW_ELEM, nrankweight_elem);
	rankweight_elem[ndx].id = safe_strdup((STRING) name);
	rankweight_elem[ndx].ndecimals = 1;
	rankweight_elem[ndx].a  = a2[0];
	rankweight_elem[ndx].b  = a2[1];
	rankweight_elem[ndx].c  = a2[2];
	rankweight_elem[ndx].d  = a2[3];
	rankweight_elem[ndx].m  = a2[4];
	rankweight_elem[ndx].f  = a2[5];

	if(debug) printf("Equation Constants - %s:  a = %g  b = %g  c = %g  d = %g  m = %g  f = %g\n",
		name, a2[0],a2[1],a2[2],a2[3],a2[4],a2[5]);	

	return &rankweight_elem[ndx];
}


/* Read the statistics config file that is in xml format. This is the
 * same one used by the SCIT display dialog in Aurora.
 */
static LOGICAL read_stat_config_file(STRING fname)
{
	int n, ndx;
	STRING value;
	LOGICAL rtn = TRUE;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL;
	xmlNodePtr top, cur, node, data;

	if(!fname)
	{
		printf("ERROR: Config file name is NULL.\n");
		return FALSE;
	}

	LIBXML_TEST_VERSION;

	doc = xmlReadFile(fname, NULL, XML_PARSE_NOBLANKS);
	if(!doc)
	{
		printf("ERROR: Could not parse file \'%s\'\n", fname);
		return FALSE;
	}

	root = xmlDocGetRootElement(doc);
	if(!root)
	{
		printf("ERROR: Could not get root element of config file \"%s\'\n", fname);
		xmlFreeDoc(doc);
		return FALSE;
	}

	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,FILEINFO)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			LOGICAL ok;
			if (cur->type != XML_ELEMENT_NODE) continue;
			value = xmlNodeGetContent(cur);
			no_white(value);
			if(xmlStrcasecmp(cur->name,NAME_MASK) == 0)
			{
				file_mask = malloc(strlen(value)+2);
				strcpy(file_mask, "^");
				strcat(file_mask, value);
			}
			else if(xmlStrcasecmp(cur->name,TIME_MASK) == 0)
			{
				file_time_mask = strdup(value);
			}
			else if(xmlStrcasecmp(cur->name,FILE_TIME_FMT) == 0)
			{
				internal_time_format = strdup(value);
			}
			else if(xmlStrcasecmp(cur->name,NUM_FCSTS) == 0)
			{
				nforecasts = int_arg(value, &ok);
				if(!ok)
				{
					printf("ERROR: Parse value for keyword \'%s\'\n", NUM_FCSTS);
					rtn = FALSE;
				}
			}
			else if(xmlStrcasecmp(cur->name,TIME_INTERVAL) == 0)
			{
				forecast_time_interval = int_arg(value, &ok);
				if(!ok)
				{
					printf("ERROR: Parse value for keyword \'%s\'\n", TIME_INTERVAL);
					rtn = FALSE;
				}
			}
			else if(xmlStrcasecmp(cur->name,NDATA_POINTS) == 0)
			{
				ndata_points = int_arg(value, &ok);
				if(!ok)
				{
					printf("ERROR: Parse value for keyword \'%s\'\n", NDATA_POINTS);
					rtn = FALSE;
				}
			}
			else if(xmlStrcasecmp(cur->name,RANK_WEIGHT_ID) == 0)
			{
				rankweight_id = strdup(value);
			}
			else if(xmlStrcasecmp(cur->name,RANK_ID) == 0)
			{
				if(!same_ic(value,"none"))
					rank_id = strdup(value);
			}
			else if(xmlStrcasecmp(cur->name,EXTERNAL_PROGRAM) == 0)
			{
				/* The keyword "default" is used to indicate the use of the internal
				 * functions. As the user might put in "default [infile] [outfile]" or
				 * some such look for a start of "default".
				 */
				if(same_start_ic(value,"default"))
				{
					trend_program = NULL;
				}
				else if(!strstr(value,"[infile]") || !strstr(value,"[outfile]"))
				{
					printf("ERROR: trendProgram: [infile] and/or [outfile] keywords not in run string.\n");
					rtn = FALSE;
				}
				else
				{
					size_t len;
					STRING p, pbuf, buf1, buf2;
					pbuf = safe_strdup(env_sub(value));
					trend_program_infile  = strdup(pathname(stat_dir,".trendProgramInfile"));
					trend_program_outfile = strdup(pathname(stat_dir,".trendProgramOutfile"));
					len = safe_strlen(pbuf)+safe_strlen(trend_program_infile)
							+safe_strlen(trend_program_outfile)+safe_strlen(logfile);
					buf1 = INITMEM(char, len);
					memset(buf1, 0, len);
					buf2 = INITMEM(char, len);
					memset(buf2, 0, len);
					p = strstr(pbuf,"[infile]");
					strncpy(buf1, pbuf, p-pbuf);
					strcat(buf1, trend_program_infile);
					strcat(buf1, p + 8);
					p = strstr(buf1, "[outfile]");
					strncpy(buf2, buf1, p-buf1);
					strcat(buf2, trend_program_outfile);
					strcat(buf2, p+9);
					if((p = strstr(buf2,"[logfile]")))
					{
						memset(buf1, 0, len);
						strncpy(buf1, buf2, p-buf2);
						strcat(buf1, logfile);
						strcat(buf1, p + 9);
						trend_program = safe_strdup(buf1);
					}
					else
					{
						trend_program = safe_strdup(buf2);
					}
					FREEMEM(pbuf);
					FREEMEM(buf1);
					FREEMEM(buf2);
				}
			}
			else if(xmlStrcasecmp(cur->name,PROGRAM_TIMEOUT) == 0)
			{
				/* Arbitrary, but I gave the daemon twice the timeout of the interactive
				 * program just to be safe.
				 */
				float timeout = (float) atof(value);
				if(timeout > 0)
				{
					trend_program_timeout = (int)timeout * 2;
				}
				else
				{
					printf("ERROR: trendProgramTimeout = %s - must be > 0\n", value);
					rtn = FALSE;
				}

			}
			xmlFree(value);
		}
		break;
	}

	/* Read the blocks that deal with defining the elements and put any
	 * elements involved in rank weight calculation into the rankweight_elem
	 * array. Also look for the forecast rank weight elements.
	 */
	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,"ELEMENTS")) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if(cur->type != XML_ELEMENT_NODE) continue;
			/*
			 * If the element starts with the rankweight element id process it
			 */
			if(xmlStrncasecmp(cur->name, rankweight_id, strlen(rankweight_id)) == 0)
			{
				ndx = nrankweight_fcst++;
				rankweight_fcst = GETMEM(rankweight_fcst, RW_FCST, nrankweight_fcst);
				rankweight_fcst[ndx].minutes = (time_t) atoi(cur->name+strlen(rankweight_id));
				rankweight_fcst[ndx].ndecimals = 1;
				for(node = cur->children; node; node = node->next)
				{
					if(node->type != XML_ELEMENT_NODE) continue;
					if(xmlStrcasecmp(node->name,"numDecimals")) continue;
					value = xmlNodeGetContent(node);
					rankweight_fcst[ndx].ndecimals = atoi(value);
					xmlFree(value);
				}
			}
			else
			{
				RW_ELEM *elem = NULL;
				for(node = cur->children; node; node = node->next)
				{
					if(node->type != XML_ELEMENT_NODE) continue;
					if(xmlStrcasecmp(node->name,"rankWeightCalc")) continue;
					value = xmlNodeGetContent(node);
					elem = assign_equation_constants(cur->name, value);
					xmlFree(value);
				}
				for(node = cur->children; node; node = node->next)
				{
					if(node->type != XML_ELEMENT_NODE) continue;
					if(xmlStrcasecmp(node->name,"numDecimals")) continue;
					value = xmlNodeGetContent(node);
					if (elem) elem->ndecimals = atoi(value);
					xmlFree(value);
				}
			}
		}
		break;
	}

	/* Read the ACTIONS block
	 */
	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,"ACTIONS")) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if(cur->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcasecmp(cur->name,"rankWeightForecastModifyProgram")) continue;
			value = xmlNodeGetContent(cur);
			no_white(value);
			mod_rankweight_program = safe_strdup(value);
			xmlFree(value);
			break;
		}
		break;
	}

	/* Read the blocks that deal with monitoring incoming urp files.
	 */
	if(!read_monitor_config(root, logfile)) rtn = FALSE;

	if(!file_mask)
	{
		printf("ERROR: Could not find key \'%s\' in config file \'%s\"\n", NAME_MASK, fname);
		rtn = FALSE;
	}
	if(!file_time_mask)
	{
		printf("ERROR: Could not find key \'%s\' in config file \'%s\"\n", TIME_MASK, fname);
		rtn = FALSE;
	}

	xmlFreeDoc(doc);
	return rtn;
}


/* In order for the rank weight forecasts to be properly generated
 * the list of files needs to be in time order so that the files
 * are processed in oldest to newest order. This is because the
 * forecast depends on preceding files and the assumption is made
 * that the files are in time order. We cannot assume that the file
 * names will be in the right order after an alphabetic sort so an
 * explicit time sort is done.
 */
static int timecmp(const void *a, const void *b)
{
	struct tm dt;
	time_t ta, tb, td;

	memset((void*)&dt, 0, sizeof(struct tm));
	strptime(a, file_time_mask, &dt);
	ta = encode_clock(dt.tm_year+1900, dt.tm_yday+1, dt.tm_hour, dt.tm_min, 0);

	memset((void*)&dt, 0, sizeof(struct tm));
	strptime(b, file_time_mask, &dt);
	tb = encode_clock(dt.tm_year+1900, dt.tm_yday+1, dt.tm_hour, dt.tm_min, 0);

	return (int)(tb - ta);
}


/* Public function to sort a list of files in time order using the
 * file_time_mask variable.
 */
void time_sort_files(STRING *files, int nfiles)
{
	if(nfiles > 1) qsort((void *)files, nfiles, sizeof(STRING), timecmp);
}


/* Find the most recent SCIT file that does not belong to the list
 * of file names passed in the parameter list. If found read it into
 * an xmlDocPtr.
 */
static xmlDocPtr get_most_recent_file_doc(STRING *fnames, int nfnames)
{
	int i, n, nfilelist;
	STRING *filelist;
	xmlDocPtr doc = NULL;

	dirlist_reuse(FALSE);
	nfilelist = dirlist(stat_dir, file_mask, &filelist);
	dirlist_reuse(TRUE);
	time_sort_files(filelist, nfilelist);

	for(n = nfilelist-1; n >= 0 && doc == NULL; n--)
	{
		LOGICAL in_fnames = FALSE;
		for(i = 0; i < nfnames; i++)
		{
			if(same(fnames[i], filelist[n]))
			{
				in_fnames = TRUE;
				break;
			}
		}
		if(!in_fnames)
		{
			doc = xmlReadFile(pathname(stat_dir,filelist[n]), NULL, XML_PARSE_NOBLANKS);
		}
	}
	FREELIST(filelist, nfilelist);
	return doc;
}


/* Look for the ratio between the element value and its original unmodified value.
 * This is given by the "ratio" property of the element.
 *
 * Parameters: prev_root = The file to be scanned for information
 *             insnum    = storm number wanted
 *             element   = element of the storm to find ratio for.
 *
 *  Return: The ratio as a string. Note that this must be free'd by using
 *          xmlFree by the calling procedure.
 */
static STRING get_prev_ratio(xmlNodePtr prev_root, int insnum, const xmlChar *element)
{
	xmlNodePtr top, elem;
	int snum = -1;

	for(top = prev_root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;
		/*
		 * Get the storm id and check for a match
		 */
		for(elem = top->children; elem; elem = elem->next)
		{
			STRING val;
			if(elem->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(elem->name, STORM_ID)) continue;
			val = xmlNodeGetContent(elem);
			snum = atoi(val);
			xmlFree(val);
			break;
		}
		if(snum == insnum)
		{
			for(elem = top->children; elem; elem = elem->next)
			{
				if(elem->type != XML_ELEMENT_NODE) continue;
				if(xmlStrcmp(elem->name, element)) continue;
				return xmlGetProp(elem,RATIO_PROP);
			}
			return NULL;
		}
	}
	return NULL;
}



/* Check to see if the given element is turned off in the previous file.
 *
 * Parameters: prev_root = The file to be scanned for information
 *             insnum    = storm number wanted
 *             element   = element of the storm to check.
 *
 *  Return: TRUE if the element is off, FALSE otherwise.
 */
static LOGICAL prev_off_state(xmlNodePtr prev_root, int insnum, const xmlChar *element)
{
	xmlNodePtr top, elem;
	int snum = -1;

	for(top = prev_root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;
		/*
		 * Get the storm id and check for a match
		 */
		for(elem = top->children; elem; elem = elem->next)
		{
			STRING val;
			if(elem->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(elem->name, STORM_ID)) continue;
			val = xmlNodeGetContent(elem);
			snum = atoi(val);
			xmlFree(val);
			break;
		}
		if(snum == insnum)
		{
			for(elem = top->children; elem; elem = elem->next)
			{
				LOGICAL off;
				STRING val;
				if(elem->type != XML_ELEMENT_NODE) continue;
				if(xmlStrcmp(elem->name, element)) continue;
				if((val = xmlGetProp(elem,OFF_PROP)))
				{
					xmlFree(val);
					return TRUE;
				}
				break;
			}
			break;
		}
	}
	return FALSE;
}



/* For each STAT and for each storm in each file, look for a corresponding storm in the
 * xml database given by prev_root. If found:
 *
 * 1. check all of the elements that are used in calculating rank weight for the "ratio"
 * property. If found save the existing value as a recovery property and modify the value
 * by the ratio found. Save this ratio as an element property in the modified element.
 * Note that this is not done if the value is found to be not available ("N/A").
 *
 * 2. check for the "off" property and if set turn the corresponding element in the current
 * file to off as well. Note that off superceeds ratio.
 */
void modify_elements_from_previous_file(STRING fname)
{
	int snum;
	LOGICAL save = FALSE;
	xmlDocPtr doc, prev_doc;
	xmlNodePtr prev_root, root, top, elem, prev_storm;

	prev_doc = get_most_recent_file_doc(&fname, 1);
	if(!prev_doc) return;
	prev_root = xmlDocGetRootElement(prev_doc);
	if(!prev_root)
	{
		xmlFreeDoc(prev_doc);
		return;
	}

	doc = xmlReadFile(pathname(stat_dir,fname), NULL, XML_PARSE_NOBLANKS);
	if(!doc)
	{
		xmlFreeDoc(prev_doc);
		return;
	}

	root = xmlDocGetRootElement(doc);
	if(!root)
	{
		xmlFreeDoc(doc);
		xmlFreeDoc(prev_doc);
		return;
	}

	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;
		/*
		 * Get the storm number
		 */
		for(snum = -1, elem = top->children; elem; elem = elem->next)
		{
			if(elem->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(elem->name, STORM_ID) == 0)
			{
				STRING val = xmlNodeGetContent(elem);
				snum = atoi(val);
				xmlFree(val);
				break;
			}
		}
		if(snum < 0) continue;
		/*
		 * Scan the elements
		 */
		for(elem = top->children; elem; elem = elem->next)
		{
			int k;
			STRING ratio_str;

			if(elem->type != XML_ELEMENT_NODE) continue;
			for(k = 0; k < nrankweight_elem; k++)
				if(xmlStrcmp(elem->name, rankweight_elem[k].id) == 0) break;
			if(k >= nrankweight_elem) continue;
				
			if(prev_off_state(prev_root, snum, elem->name))
			{
				STRING recover;
					
				save = TRUE;
				STRING val = xmlNodeGetContent(elem);
				if(!(recover = xmlGetProp(elem, RECOVER_PROP)))
					xmlSetProp(elem, RECOVER_PROP, val);
				else
					xmlFree(recover);
				xmlSetProp(elem, OFF_PROP, val);
				xmlNodeSetContent(elem, DATA_NA);
				xmlFree(val);
			}
			else if((ratio_str = get_prev_ratio(prev_root, snum, elem->name)))
			{
				STRING val = xmlNodeGetContent(elem);
				if(xmlStrcasecmp(val,"N/A"))
				{
					double value, ratio;
					char buf[32];
					STRING recover;

					save = TRUE;
					if(!(recover = xmlGetProp(elem, RECOVER_PROP)))
						xmlSetProp(elem, RECOVER_PROP, val);
					else
						xmlFree(recover);
					value = atof(val);
					ratio = atof(ratio_str);
					xmlSetProp(elem, RATIO_PROP, ratio_str);
					snprintf(buf, 32, "%.*f", rankweight_elem[k].ndecimals,
						(float) (value * ratio));
					xmlNodeSetContent(elem, buf);
				}
				xmlFree(val);
				xmlFree(ratio_str);
				break;
			}
		}
	}
	if (save)
	{			
		wfiles = MOREMEM(wfiles, STRING, nwfiles+1);
		wfiles[nwfiles++] = strdup(fname);
		xmlSaveFormatFile(pathname(stat_dir,fname), doc, 1);
	}
	xmlFreeDoc(doc);
	xmlFreeDoc(prev_doc);
}


/* Recalculate the rank weights for the storms in the file and then calculate
 * the forecast rank weights. Only if there is a change of value in the file
 * is the file then written to disk. The list of files needs to be in time
 * order for the forecasts to be valid.
 *
 * Parameters:
 *
 * fnames     = the string array of file names
 * nfnames    = the number of files
 * initialize = Force the creation of forecast files if true. If false only
 *              create the files if there is a change in the data files or
 *              if no rank weight forecast elements exist. If not initializing
 *              the element values may be scaled by the change in the most
 *              recent STAT file.
 */
void calculate_rankweight(STRING *fnames, int nfnames, LOGICAL initialize)
{
	int n;
	LOGICAL new_fcst_files = FALSE;
	xmlDocPtr prev_doc = NULL;
	xmlNodePtr prev_root = NULL;

	time_sort_files(fnames, nfnames);

	/* Storm element values are only modified by the ratio property for
	 * newly arriving STAT files and not during initialization. This is
	 * because only modifications to the most recent STAT file are carried
	 * forward by this daemon. Any other changes are handled by the STAT
	 * display dialog of FPA.
	 */
	if(!initialize)
	{
		prev_doc = get_most_recent_file_doc(fnames, nfnames);
		if(prev_doc)
			prev_root = xmlDocGetRootElement(prev_doc);
	}

	for(n = 0; n < nfnames; n++)
	{
		xmlDocPtr  doc  = NULL;
		xmlNodePtr root = NULL;
		STRING     file = fnames[n];

		printdebug("Processing file: %s", file);

		if(!(doc = xmlReadFile(pathname(stat_dir,file), NULL, XML_PARSE_NOBLANKS)))
		{
			printlog("ERROR: Could not parse file \'%s\'", file);
		}
		else if(!(root = xmlDocGetRootElement(doc)))
		{
			printlog("ERROR: Could not get root element of file \"%s\'", file);
		}
		else
		{
			/* Ignore if a file just contains forecasts. The root property key
			 * and value are set to indicate this. 
			 */
			LOGICAL process = TRUE;
			STRING val = xmlGetProp(root,TYPE_PROP);
			if(val)
			{
				process = (strstr(val,FCST_KEY) == NULL);
				xmlFree(val);
			}
			if(process)
			{
				LOGICAL data_change = FALSE;
				if(check_storm_environment(root, prev_root, file))
					data_change = TRUE;
				if(calc_data_rankweights(root))
					data_change = TRUE;
				if(calc_fcst_rankweights(root, file, data_change))
					data_change = TRUE;
				if(data_change)
				{
					new_fcst_files = TRUE;
					wfiles = MOREMEM(wfiles, STRING, nwfiles+1);
					wfiles[nwfiles++] = strdup(file);
					xmlSaveFormatFile(pathname(stat_dir,file), doc, 1);
				}
			}
		}
		if(doc) xmlFreeDoc(doc);
	}

	if(prev_doc)
		xmlFreeDoc(prev_doc);

#ifdef MAKE_FORECAST_FILES
	if(initialize || new_fcst_files)
		create_forecast_rankweight_files();
#endif
}


/* If a STAT file is detected as having its forecast rank weights
 * manually modified, this function launches a program as set in
 * the configuration file.
 */
void process_manual_rankweight_override(STRING *files, int nfiles)
{
	STRING p, ptr;

	if(!mod_rankweight_program) return;

	ptr = strdup(mod_rankweight_program);

	/* Look for the log file keyword and substitute. */
	if((p = strstr(ptr,"[logfile]")))
	{
		size_t len = safe_strlen(ptr)+safe_strlen(logfile)+1;
		STRING fptr = INITMEM(char, len);
		memset(fptr, 0, len);
		strncpy(fptr, ptr, p-ptr);
		strcat(fptr, logfile);
		strcat(fptr, p+9);
		FREEMEM(ptr);
		ptr = fptr;
	}

	/* Look for file list substitution */
	if((p = strstr(ptr, "[files]")))
	{
		int n;
		STRING fptr;
		size_t len, total_size = strlen(ptr);

		for(n = 0; n < nfiles; n++)
			total_size += safe_strlen(files[n])+1;
		fptr = INITMEM(char, total_size);
		memset(fptr, 0, total_size);
		strncpy(fptr, ptr, p-ptr);
		for(n = 0; n < nfiles; n++)
		{
			strcat(fptr, files[n]);
			if(n < nfiles-1) strcat(fptr,",");
		}
		strcat(fptr, p+7);
		FREEMEM(ptr);
		ptr = fptr;
	}
	shrun(ptr, FALSE);
	FREEMEM(ptr);
}


int main (int argc, STRING *argv)
{
	int  n, ndx, nfilelist;
	char buf[256];
	STRING *filelist;
	STRING setup_file = NULL;
	STRING config_file = NULL;
	LOGICAL do_stop = FALSE;
	FILE *fp, *f;
	struct stat sbuf;
	pid_t pid, sid;

	/* A list of directories of where to put the log file.
	 * The first valid one will be used.
	 */
	STRING logdirs[] = {"$FPA","$HOME","/tmp"};

	/* inotify monitor mask */
	unsigned long mask = IN_MODIFY|IN_MOVED_FROM|IN_CREATE|IN_DELETE_SELF;

	ndx = 1;
	if(argc == 1 || (argc == 2 && strchr("-h?",*argv[1]) != NULL))
	{
		printf("To run: rankweightDaemon [debug] start|stop <setup_file>\n");
		exit(EXIT_SUCCESS);
	}
	if(same_ic(argv[ndx],"debug"))
	{
		ndx++;
		debug = TRUE;
	}
	if(same_ic(argv[ndx],"stop"))
	{
		ndx++;
		do_stop = TRUE;
	}
	else if(same_ic(argv[ndx],"start"))
	{
		ndx++;
	}
	else
	{
		printf("\nERROR: Unrecognized argument. Must be 'start','debug start' or 'stop'.\n");
		exit(EXIT_FAILURE);
	}

	if(argc > ndx) setup_file = argv[ndx];
	if(!fpa_connect(setup_file, FpaAccessFull))
		exit(EXIT_FAILURE);

	/* Get the data directory where all of the SCIT files are */
	stat_dir = get_directory(RADAR_STAT_KEY);
	if(!stat_dir) exit(EXIT_FAILURE);
	if(stat(stat_dir,&sbuf) != 0 || !S_ISDIR(sbuf.st_mode))
	{
		printf("\nERROR: Data directory \'%s\' is not accessable for read/write.\n", stat_dir);
		exit(EXIT_FAILURE);
	}

	/* Create the lock file file name */
	snprintf(buf, sizeof(buf), "%s/.%s.pid", stat_dir, pgm);
	pidfile = strdup(buf);

	/* Are we stopping an existing instance of ourselves? */
	if(do_stop)
	{
		long lpid;
		fp = fopen(pidfile,"r");
		if(!fp) exit(EXIT_FAILURE);

		if(fscanf(fp, "%ld", &lpid) == 1)
		{
			pid = (pid_t) lpid;
			if(kill(pid,0) != 0)
			{
				printf("\n%s is not active\n\n", pgm);
			}
			else
			{
				printf("\n%s sending termination signal.\n\n", pgm);
				kill(pid, SIGINT);
				for(n = 0; n < 10; n++)
				{
					sleep(1);
					if(kill(pid,0) != 0) break;
				}
				if(n < 10)
				{
					printf("Program terminated\n\n");
				}
				else
				{
					printf("ERROR: The program did not terminate after 10 seconds.\n");
					printf("       A manual kill may be necessary.\n\n");
				}
			}
		}
		else
		{
			printf("ERROR: Cannot get pid of running daemon from pidfile \'%s\'\n",
					pidfile);
		}
		fclose(fp);
		free(pidfile);
		exit(EXIT_SUCCESS);
	}

	/* Read the configuration file */
	config_file = config_file_name(RADAR_STAT_KEY);
	if(!config_file)
	{
		printf("\nERROR: Unable to get config file associated with the key \'%s\' from setup file.\n\n",
				RADAR_STAT_KEY);
		exit(EXIT_FAILURE);
	}

	/* Create the log file if it does not exist. Note that the
	 * /var/log directory will only succeed if the program is
	 * run as root.
	 */
	for(n = 0; n < sizeof(logdirs)/sizeof(logdirs[0]); n++)
	{
		STRING fname;
		FILE *log = NULL;
		snprintf(buf, sizeof(buf), "%s/%s.log", logdirs[n], pgm);
		fname = env_sub(buf);
		if(access(fname, R_OK|W_OK) == 0 || (log = fopen(fname,"w")) != NULL)
		{
			logfile = strdup(fname);
			printf("\nAll messages from now on logged in file: %s\n\n", logfile);
			if (log) fclose(log);
			break;
		}
	}
	if(!logfile)
	{
		printf("ERROR: Unable to create a logfile in any directory!\n");
		exit(EXIT_FAILURE);
	}

	if(!read_stat_config_file(config_file))
		exit(EXIT_FAILURE);
        
	/* Now create our daemon. */
	daemonize();

	/* Check if a version is already running */
	if(already_running())
	{
		printlog("ERROR: Attempt to run %s when it is already running.", pgm);
		exit(EXIT_FAILURE);
	}

	/* Start logging */
	truncate(logfile, 0);
	printlog("Starting");

	/* Set the ctrl-c (SIGINT) signal handler */
	(void) signal(SIGINT, signal_handler);

	/* Open the inotify dev entry.
	 */
	printlog("Opening inotify processes.");
	inotify_fd = open_inotify_fd();
	if (inotify_fd < 0) exit(EXIT_FAILURE);

	/* We need a place to enqueue inotify events, because if you
	 * do not read events fast enough, you will miss them.
	 */
	q = queue_create();
	
	/* Read all of the existing files and process for rankweight */
	dirlist_reuse(FALSE);
	nfilelist = dirlist(stat_dir, file_mask, &filelist);
	dirlist_reuse(TRUE);
	printlog("Checking existing STAT file rank weights.");
	calculate_rankweight(filelist, nfilelist, TRUE);
	FREELIST(filelist, nfilelist);

	/* Watch events for the directory. Note that a SIGINT (ctrl-c) will
	 * terminate the program
	 */
	printlog("Beginning watch of directory \'%s\'", stat_dir);
	stat_dir_wd = watch_dir(inotify_fd, stat_dir, mask);
	set_monitor_watch(inotify_fd);
	if(stat_dir_wd > 0)
		process_inotify_events (q, inotify_fd);

	/* Finish up by closing the fd and destroying the queue */
	close_inotify_fd(inotify_fd);
	queue_destroy(q);
	xmlCleanupParser();
	printlog("Terminated by signal.");
	free(pidfile);
	free(logfile);

	exit(EXIT_SUCCESS);
}
