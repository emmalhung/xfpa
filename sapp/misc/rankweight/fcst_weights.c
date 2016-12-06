/***********************************************************************/
/*
 *	File: fcst_weights.c
 *
 *	Purpose: Calculates forecast rank weights.
 *
 *	Important: The functions do not change existing forecast ranks
 *	           unless the rank weight changes. This is to preserve the
 *	           validity of historical files in case the forecast algorythm
 *	           changes.
 */
/***********************************************************************/
#include <values.h>
#include <time.h>
#include "rankweight.h"
#include "calc_weights.h"
#include "fcst_weights.h"
#include "inotify_utils.h"

#define NTHRESH	4
#define ELEM_NA	FLT_MIN

extern int     nforecasts;
extern int     forecast_time_interval;
extern int     ndata_points;
extern STRING  stat_dir;
extern STRING  file_mask;
extern STRING  file_time_mask;
extern STRING  internal_time_format;
extern STRING  rankweight_id;
extern int     nrankweight_elem;
extern RW_ELEM *rankweight_elem;
extern int     nrankweight_fcst;
extern RW_FCST *rankweight_fcst;
extern STRING  trend_program;
extern int     trend_program_timeout;
extern STRING  trend_program_infile;
extern STRING  trend_program_outfile;
extern queue_t q;								/* event quque */
extern int     inotify_fd;						/* inotify watch file descriptor */



typedef struct {
	time_t time;		/* storm instance time */
	double rank;		/* storm rank weight */
	float  *elemval;	/* values of the storm elements that used to calculate rank weight */
} RANK;

typedef struct {
	char id[32];	/* storm identification */
	time_t vtime;	/* most recent storm time */
	int ndata;		/* The number entries in the data variable */
	RANK *data;		/* The most recent data files */
	RANK *fcst;		/* the forecast files */
} StormInfo;


static int       nstorms = 0;
static StormInfo *storms = NULL;


/* Write out the input data file for the external processing program.
 */
static void write_infile(void)
{
	int i, j, n;
	int year, month, jday, day, hour, min;
	char buf[2000];
	xmlDocPtr  doc;
	xmlNodePtr root, sptr;
	size_t len = sizeof(buf);

	doc = xmlNewDoc("1.0");
	root = xmlNewNode(NULL,"RankWeightForecast");
	xmlDocSetRootElement(doc, root);

	/* Convert system seconds to yyyymmddhhmm format */
	decode_clock(storms[0].vtime, &year, &jday, &hour, &min, NULL);
	mdate(&year, &jday, &month, &day);
	snprintf(buf, len, "%.4d%.2d%.2d%.2d%.2d", year, month, day, hour, min);
	xmlNewChild(root, NULL, VALID_TIME, buf);

	snprintf(buf, len, "%d", nforecasts);
	xmlNewChild(root, NULL, "NFORECASTS", buf);
	snprintf(buf, len, "%d", forecast_time_interval);
	xmlNewChild(root, NULL, "TIMEINTERVAL", buf);

	for(n = 0; n < nstorms; n++)
	{
		StormInfo *storm = storms + n;
		sptr = xmlNewChild(root, NULL, STORM, NULL);
		xmlSetProp(sptr, STORM_ID, storm->id);

		snprintf(buf, len, "%d", storm->ndata);
		xmlNewChild(sptr, NULL, "NTIMES", buf);
		strcpy(buf,"");
		for(i = 0; i < storm->ndata; i++)
		{
			size_t blen = safe_strlen(buf);
			int dt = (int) (storm->data[i].time - storm->data[0].time)/60;
			snprintf(buf+blen, len-blen, "%d,", dt);
		}
		xmlNewChild(sptr, NULL, "TIMEDELTA", buf);

		strcpy(buf,"");
		for(i = 0; i < storm->ndata; i++)
		{
			size_t blen = safe_strlen(buf);
			if(storm->data[i].rank == ELEM_NA)
				snprintf(buf+blen, len-blen, "N/A,");
			else
				snprintf(buf+blen, len-blen, "%g,", (float) storm->data[i].rank);
		}
		xmlNewChild(sptr, NULL, rankweight_id, buf);

		for(i = 0; i < nrankweight_elem; i++)
		{
			strcpy(buf,"");
			for(j = 0; j < storm->ndata; j++)
			{
				size_t blen = safe_strlen(buf);
				if(storm->data[j].elemval[i] == ELEM_NA)
					snprintf(buf+blen, len-blen, "N/A,");
				else
					snprintf(buf+blen, len-blen, "%g,", storm->data[j].elemval[i]);
			}
			xmlNewChild(sptr, NULL, rankweight_elem[i].id, buf);
		}
	}

	xmlSaveFormatFileEnc(trend_program_infile, doc, "ISO-8859-1", 1);
	xmlFreeDoc(doc);
}


/* Read the output file from the processing program. Note that only "RankWeightForecast" type
 * files are processed and any others are flagged as an error.
 */
static void read_outputfile(void)
{
	time_t vtime = 0;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL, cur = NULL, elem = NULL;

	if((doc = xmlReadFile(trend_program_outfile, NULL, 0)))
		root = xmlDocGetRootElement(doc);
	if(!doc || !root)
	{
		printlog("ERROR: Could not read output file from element trend forecast program.");
		if(doc) xmlFreeDoc(doc);
		return;
	}
	else if(xmlStrcmp(root->name,"RankWeightForecast"))
	{
		printlog("ERROR: Output file from element trend forecast program is wrong type.");
		printlog("       Is of type \'%s\' and \'RankWeightForecast\' is required.", root->name);
		if(doc) xmlFreeDoc(doc);
		return;
	}

	/* The following assumes that the validtime will be found before the first storm.
	 */
	for(cur = root->children; cur; cur = cur->next)
	{
		if(xmlStrcmp(cur->name,VALID_TIME) == 0)
		{
			int year, jday, month, day, hour, minute;
			STRING val = xmlNodeGetContent(cur);
			no_white(val);
			sscanf(val,"%4d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute);
			xmlFree(val);
			jdate(&year, &month, &day, &jday);
			vtime = encode_clock(year, jday, hour, minute, 0);
		}
		else if(xmlStrcmp(cur->name,STORM) == 0)
		{
			int n;
			int snum = 0;
			StormInfo *storm = NULL;
			STRING val = xmlGetProp(cur, STORM_ID);
			no_white(val);

			for(n = 0; n < nstorms; n++)
			{
				if(same(val,storms[n].id))
				{
					storm = storms + n;
					break;
				}
			}
			if(!storm)
			{
				printlog("ERROR: Unknown storm \'%s\' in trend program output file.", val);
			}
			else if(storm->vtime != vtime)
			{
				printlog("ERROR: Valid time mismatch in trend program output file.");
				break;
			}
			else
			{
				for(elem = cur->children; elem; elem = elem->next)
				{
					STRING p, str;
					if(elem->type != XML_ELEMENT_NODE) continue;
					if(xmlStrcmp(elem->name, rankweight_id)) continue;
					str = xmlNodeGetContent(elem);
					p = strtok(str,",");
					for(n = 0; n < nforecasts; n++)
					{
						LOGICAL ok;
						storm->fcst[n].rank = float_arg(p, &ok);
						if(!ok) storm->fcst[n].rank = ELEM_NA;
						p = strtok(NULL,",");
					}
					xmlFree(str);
				}
			}
			xmlFree(val);
		}
	}
	xmlFreeDoc(doc);
}


/* Produce rank weight forecasts using an external program defined in the
 * configuration file. See the management manual and the STAT table display
 * program for details on the input/output file structure requirements. The
 * code is somewhat complex as the external program is run with a timeout and
 * events other than just the external program finishing and writing to the
 * output file are captured so that we don't miss any important events that
 * may occur while the external program is processing.
 */
static void make_forecasts_with_external_program(void)
{
	time_t start_time = 0;
	struct timeval timeout;
	int res;
	fd_set rfds;
	LOGICAL wait_here = TRUE;

	static int outfile_wd = -1;

	if(!trend_program) return;
	if(nstorms < 1) return;

	if(access(trend_program_outfile,F_OK))
	{
		FILE *fp;
		if(outfile_wd > 0)
			ignore_wd(inotify_fd, outfile_wd);
		outfile_wd = -1;
		fp = fopen(trend_program_outfile,"w");
		if(fp)
		{
			fprintf(fp,"opened");
			fclose(fp);
		}
		else
		{
			printlog("ERROR: Unable to open external rank weight outfile \'%s\'", trend_program_outfile);
			goto program_error;
		}

	}
	if(outfile_wd < 0)
		outfile_wd = inotify_add_watch(inotify_fd, trend_program_outfile, IN_CLOSE_WRITE);
	if(outfile_wd < 0)
	{
		printlog("ERROR: Unable to watch external rank weight outfile \'%s\'", trend_program_outfile);
		goto program_error;
	}

	start_time = time(NULL);
	timeout.tv_sec = (long) trend_program_timeout;
	timeout.tv_usec = 0;

	write_infile();

	if(shrun(trend_program, FALSE))
	{
		printlog("ERROR: Unable to run \'%s\'", trend_program);
		goto program_error;
	}

	while(wait_here)
	{
		FD_ZERO(&rfds);
		FD_SET(inotify_fd, &rfds);
		res = select(FD_SETSIZE, &rfds, NULL, NULL, &timeout);
		if(res > 0)
		{
			if(read_events(q, inotify_fd) < 0)
			{
				printlog("ERROR: inotify event queue read error");
				break;
			}
			else
			{
				wait_here = (handle_events(q) == 0);
				if(wait_here)
				{
					timeout.tv_sec -= (time(NULL) - start_time);
					if(timeout.tv_sec < 0)
					{
						printlog("ERROR: External rank weight forecast program has timed out");
						break;
					}
				}
				else
				{
					read_outputfile();
					return;
				}
			}
		}
		else if(res == 0)	/* timeout */
		{
			printlog("ERROR: External rank weight forecast program has timed out");
			break;
		}
	}

program_error:
	printlog("ERROR: Deactivating external rank weight forecasting program \'%s\'", trend_program);
	FREEMEM(trend_program);
	trend_program = NULL;
}


/* Algorithms for producing the rank weight forecasts depending on the
 * number of data points available. Note that storms[n].data[0] contains
 * the most recent data points.
 */
static void make_forecasts(void)
{
	int i, n;
	double m, b;
	time_t t;

	for(n = 0; n < nstorms; n++)
	{
		StormInfo *storm = &storms[n];

		/* As the time values are very large and they are squared, normalize to minutes */
		t = storm->data[storm->ndata-1].time;
		for(i = 0; i < storm->ndata; i++)
		{
			storm->data[i].time = (storm->data[i].time - t)/60;
		}

		/* Forecast method depends on the number of data points.
		 */
		if(storm->ndata == 1)
		{
			/* If just one instance extrapolate */
			for(t = 0, i = 0; i < nforecasts; i++)
			{
				t += (time_t) forecast_time_interval;
				storm->fcst[i].time = t;
				storm->fcst[i].rank = storm->data[0].rank;
			}
		}
		else if(storm->ndata == 2)
		{
			/* Only 2 data points so use the equation of a line for extrapolation */
			m = (storm->data[0].rank - storm->data[1].rank)/(double)(storm->data[0].time - storm->data[1].time);
			b = storm->data[0].rank - m*(double)storm->data[0].time;
			for(t = 0, i = 0; i < nforecasts; i++)
			{
				t += (time_t) forecast_time_interval;
				storm->fcst[i].time = t;
				storm->fcst[i].rank = m*(double)(t+storm->data[0].time) + b;
				/* Limit the possible rank values */
				storm->fcst[i].rank = MAX(0,storm->fcst[i].rank);
				storm->fcst[i].rank = MIN(10,storm->fcst[i].rank);
			}
		}
		else if(storm->ndata > 2)
		{
			/* Use a least square linear fit to the data points
			 */
			double sumx  = 0.0;	/* sum of x */
			double sumx2 = 0.0;	/* sum of x ** 2 */
			double sumxy = 0.0;	/* sum of x * y */
			double sumy  = 0.0;	/* sum of y */
			double n = 3;

			for(i = 0; i < MIN(storm->ndata,ndata_points); i++)
			{
				double vtime = (double) storm->data[i].time;
				sumx += vtime;
				sumx2 += vtime * vtime;
				sumxy += vtime * storm->data[i].rank;
				sumy += storm->data[i].rank;
			}
			m = (n * sumxy - sumx * sumy) / (n * sumx2 - sumx * sumx);
			b = (sumy * sumx2 - sumx * sumxy) / (n * sumx2 - sumx * sumx);

			for(t = 0, i = 0; i < nforecasts; i++)
			{
				t += (time_t) forecast_time_interval;
				storm->fcst[i].time = t;
				storm->fcst[i].rank = m*(double)(t+storm->data[0].time) + b;
				/* Limit the possible rank values */
				storm->fcst[i].rank = MAX(0,storm->fcst[i].rank);
				storm->fcst[i].rank = MIN(10,storm->fcst[i].rank);
			}
		}
	}
}


static void clear_storm_list(void)
{
	int i;
	for(i = 0; i < nstorms; i++)
	{
		int j;
		for(j = 0; j > storms[i].ndata; j++)
			FREEMEM(storms[i].data[j].elemval);
		FREEMEM(storms[i].data);
		FREEMEM(storms[i].fcst);
	}
	FREEMEM(storms);
	nstorms = 0;
}


/* Add the given storm to the table.
 *
 * id    = storm identification
 * time  = time of storm in unix seconds
 * rank  = storm rank weight
 * first = if true then add the storm regardless. If false only add the
 *         storm data if the storm is already in the table. Thus only the
 *         storms in the first data file found will be forecast.
 *
 * return: TRUE if the storm is added to the table, FALSE otherwise.
 */
static LOGICAL add_storm_to_table(STRING id, time_t time, double rank, float *elemval, LOGICAL first)
{
	int j, k, n;

	if(!id) return FALSE;

	if(first)
	{
		n = nstorms;
		nstorms++;
		storms = GETMEM(storms, StormInfo, nstorms);
		strncpy(storms[n].id, id, 31);
		storms[n].vtime = time;
		storms[n].data = INITMEM(RANK, 1);
		storms[n].ndata = 1;
		storms[n].data[0].time = time;
		storms[n].data[0].rank = rank;
		storms[n].data[0].elemval = INITMEM(float, nrankweight_elem);
		for(j = 0; j < nrankweight_elem; j++)
			storms[n].data[0].elemval[j] = elemval[j];
		storms[n].fcst = INITMEM(RANK, nforecasts);
		for(j = 0; j < nforecasts; j++)
			storms[n].fcst[j].rank = ELEM_NA;
		return TRUE;
	}

	for(n = 0; n < nstorms; n++)
	{
		if(!same(storms[n].id, id)) continue;
		k = storms[n].ndata;
		storms[n].ndata++;
		storms[n].data = GETMEM(storms[n].data, RANK, storms[n].ndata);
		storms[n].data[k].time = time;
		storms[n].data[k].rank = rank;
		storms[n].data[k].elemval = INITMEM(float, nrankweight_elem);
		for(j = 0; j < nrankweight_elem; j++)
			storms[n].data[k].elemval[j] = elemval[j];
		return TRUE;
	}
	return FALSE;
}


/* Extract existing storm information from data files.
 */
static LOGICAL populate_storm_table(xmlNodePtr root, STRING fname, LOGICAL first)
{
	int n;
	struct tm dt;
	time_t vtime = LONG_MIN;
	LOGICAL storms_found = FALSE;
	STRING val;
	float *elemvals;
	xmlNodePtr top, cur, data, rank_node;

	/* Get valid time */
	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,VALID_TIME)) continue;
		val = xmlNodeGetContent(top);
		memset((void*)&dt, 0, sizeof(struct tm));
		strptime(val, internal_time_format, &dt);
		vtime = encode_clock(dt.tm_year+1900, dt.tm_yday+1, dt.tm_hour, dt.tm_min, 0);
		xmlFree(val);
		break;
	}
	if(vtime == LONG_MIN)
	{
		printlog("ERROR: Unable to get valid time from SCIT file: %s", fname);
		return storms_found;
	}

	/*
	 * Find the storms and add to the storm table
	 */
	elemvals = INITMEM(float, nrankweight_elem);
	for(n = 0; n < nrankweight_elem; n++)
		elemvals[n] = ELEM_NA;

	for(top = root->children; top; top = top->next)
	{
		double rank = 0;
		STRING storm = NULL;
		LOGICAL rwok = FALSE;

		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(top->name,STORM)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			if(cur->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(cur->name,STORM_ID) == 0)
			{
				storm = xmlNodeGetContent(cur);
			}
			else if(xmlStrcmp(cur->name,rankweight_id) == 0) 
			{
				val = xmlNodeGetContent(cur);
				rank = double_arg(val, &rwok);
				xmlFree(val);
			}
			else
			{
				for(n = 0; n < nrankweight_elem; n++)
				{
					LOGICAL ok;
					if(xmlStrcmp(cur->name,rankweight_elem[n].id)) continue;
					val = xmlNodeGetContent(cur);
					elemvals[n] = float_arg(val, &ok);
					if(!ok) elemvals[n] = ELEM_NA;
					xmlFree(val);
				}
			}
		}
		if(rwok)
		{
			if(add_storm_to_table(storm, vtime, rank, elemvals, first))
				storms_found = TRUE;
		}
		else
		{
			printlog("ERROR: Rank weight not valid for storm \'%s\' in file \'%s\'",
				storm, fname);
		}
		xmlFree(storm);
	}
	FREEMEM(elemvals);
	return storms_found;
}


/* Create the rank weight forecasts. If the force parameter is true then calculate the
 * forecast rank weights. If false then only calculate the rank weights if they do not
 * already exist in the file.
 */
LOGICAL calc_fcst_rankweights(xmlNodePtr root, STRING fname, LOGICAL force)
{
	int n, i, pos, nfilelist;
	LOGICAL storms_found, file_changed = FALSE;
	STRING *filelist;
	xmlNodePtr node, data;

	clear_storm_list();

	/* Scan the files in reverse order looking for the newest data file.
	 */
	dirlist_reuse(FALSE);
	nfilelist = dirlist(stat_dir, file_mask, &filelist);
	dirlist_reuse(TRUE);
	time_sort_files(filelist, nfilelist);
	for(pos = 0; pos < nfilelist; pos++)
	{
		if(same(fname,filelist[pos])) break;
	}
	/*
	 * This should never happen ... but ...
	 */
	if(pos >= nfilelist)
	{
		FREELIST(filelist, nfilelist);
		return FALSE;
	}
	/*
	 * Run backwards populating the table
	 */
	(void) populate_storm_table(root, fname, TRUE);
	storms_found = TRUE;
	for(n = pos-1; n >= 0 && storms_found; n--)
	{
		xmlDocPtr  doc;
		xmlNodePtr fileroot;
		doc = xmlReadFile(pathname(stat_dir,filelist[n]), NULL, XML_PARSE_NOBLANKS);
		if(!doc) continue;
		fileroot = xmlDocGetRootElement(doc);
		if(fileroot)
		{
			storms_found = populate_storm_table(fileroot, filelist[n], FALSE);
		}
		xmlFreeDoc(doc);
	}
	FREELIST(filelist, nfilelist);

	/* Always make rank weight forecasts using the internal least square functions
	 * and then if using an external program override them. This way there is always
	 * a value even if the external program fails.
	 */
	make_forecasts();
	make_forecasts_with_external_program();

	for(node = root->children; node; node = node->next)
	{
		STRING storm_id = NULL;
		if(node->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcasecmp(node->name,STORM)) continue;
		for(data = node->children; data; data = data->next)
		{
			if(data->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcasecmp(data->name,STORM_ID)) continue;
			storm_id = xmlNodeGetContent(data);
			break;
		}
		for(n = 0; n < nstorms; n++)
		{
			if(xmlStrcmp(storm_id, storms[n].id)) continue;
			for(i = 0; i < nforecasts; i++)
			{
				int k, ndecimals = 1;
				STRING  old_rank_val;
				LOGICAL found = FALSE;
				char rank_id[32], new_rank_val[32];
				/*
				 * Search for an existing RankWeightxx node. If one is found replace
				 * the existing value if different. If not found create one.
				 */
				snprintf(rank_id, sizeof(rank_id), "%s%ld", rankweight_id, storms[n].fcst[i].time);
				/*
				 * Find the number of decimals required for output for the forecast time
				 */
				for(k = 0; k < nrankweight_fcst; k++)
				{
					if(rankweight_fcst[k].minutes == storms[n].fcst[i].time)
					{
						ndecimals = rankweight_fcst[k].ndecimals;
						break;
					}
				}
				if(storms[n].fcst[i].rank == ELEM_NA)
					strcpy(new_rank_val, "N/A");
				else
					snprintf(new_rank_val, sizeof(new_rank_val), "%.*f", ndecimals, storms[n].fcst[i].rank);

				for(data = node->children; data; data = data->next)
				{
					if(data->type != XML_ELEMENT_NODE) continue;
					if(xmlStrcasecmp(data->name,rank_id)) continue;
					found = TRUE;
					/*
					 * Only change an existing forecast if force is TRUE. Check for an override
					 * on the forecast and if found do not change the existing value. 
					 */
					if(force)
					{
						STRING over;

						over = xmlGetProp(data,OVERRIDE_PROP);
						if(over)
						{
							xmlFree(over);
						}
						else
						{
							old_rank_val = xmlNodeGetContent(data);
							if(!same(old_rank_val, new_rank_val))
							{
								file_changed = TRUE;
								xmlNodeSetContent(data, new_rank_val);
							}
							xmlFree(old_rank_val);
						}
					}
				}
				if(!found)
				{
					file_changed = TRUE;
					xmlNewChild(node, NULL, rank_id, new_rank_val);
				}
			}
		}
		xmlFree(storm_id);
	}
	return file_changed;
}


/* Load in the thresholds for rank weight only from the given data file root.
 * These will be put into the forecast files. nthresholds is the number of
 * threshold levels actually found in the file.
 */
static void get_rankweight_thresholds(xmlNodePtr root, double *thresholds, int *nthresholds)
{
	int nthresh = 0;
	xmlNodePtr top, cur, data;

	for(top = root->children; top; top = top->next)
	{
		if(top->type != XML_ELEMENT_NODE) continue;
		if(xmlStrcmp(top->name,THRESHOLDS)) continue;
		for(cur = top->children; cur; cur = cur->next)
		{
			STRING val;
			int level = 0;
			if(cur->type != XML_ELEMENT_NODE) continue;
			if(xmlStrcmp(cur->name,THRESHOLD)) continue;
			val = xmlGetProp(cur, LEVEL_PROP);
			if(!val) continue;
			level = atoi(val) - 1;
			xmlFree(val);
			if(level >= NTHRESH) continue;
			nthresh++;
			for(data = cur->children; data; data = data->next)
			{
				if(data->type != XML_ELEMENT_NODE) continue;
				if(xmlStrcmp(data->name,rankweight_id)) continue;
				val = xmlNodeGetContent(data);
				if(val) thresholds[level] = atof(val);
				xmlFree(val);
			}
		}
	}
	*nthresholds = nthresh;
}


/* Create the set of files containing just the forecast rankweight
 * after the last SCIT file in the sequence. The first forecast file
 * will contain all nforecast forecasts, the second nforecast - 1
 * and so on.
 */
void create_forecast_rankweight_files(void)
{
	int i, k, n, nflist, nthreshold_val;
	double threshold_val[NTHRESH] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};
	time_t data_time;
	STRING *flist;
	xmlDocPtr  doc;
	xmlNodePtr root;

	dirlist_reuse(FALSE);
	nflist = dirlist(stat_dir, file_mask, &flist);
	dirlist_reuse(TRUE);
	if(nflist == 0) return;

	time_sort_files(flist, nflist);
	clear_storm_list();

	/* Run backwards populating the storms table. Test to see if
	 * a file is a forecast, and if so ignore it.
	 */
	for(i = 0, n = nflist-1; n >= 0 && i < ndata_points; n--)
	{
		doc = xmlReadFile(pathname(stat_dir,flist[n]), NULL, XML_PARSE_NOBLANKS);
		if(!doc) continue;
		root = xmlDocGetRootElement(doc);
		if(root)
		{
			/* This is the forecast file test */
			STRING val = xmlGetProp(root,TYPE_PROP);
			if(val == NULL || strstr(val,FCST_KEY) == NULL)
			{
				if(i==0) get_rankweight_thresholds(root, threshold_val, &nthreshold_val);
				populate_storm_table(root, flist[n], (i==0));
				i++;
			}
			xmlFree(val);
		}
		xmlFreeDoc(doc);
	}
	FREELIST(flist, nflist);

	if(nstorms == 0) return;

	/* At this point the storm arrays contain the actual times and
	 * have not yet been normalized.
	 */
	data_time = storms[0].data[0].time;

	/* Always create a forecast using the internal least square fitting and
	 * if an external program is used override these values. This way there
	 * is always a value.
	 */
	make_forecasts();
	make_forecasts_with_external_program();

	/* Create the forecast files. As the time goes further out fewer of
	 * the rank weight forecasts are included in the file. That is fhe
	 * first file will contain all of the forecast time points. I did 
	 * not include the threshold information.
	 */
	for(i = 0; i < nforecasts; i++)
	{
		time_t ftime;
		char valid_time[32], fname[100];
		xmlNodePtr root, node, thresholds, threshold;

		doc = xmlNewDoc("1.0");
		root = xmlNewNode(NULL,ROOT_ID);
		xmlSetProp(root,TYPE_PROP,FCST_KEY);
		xmlDocSetRootElement(doc, root);

		ftime = data_time + storms[0].fcst[i].time*60;
		(void) strftime(valid_time, sizeof(valid_time), internal_time_format, gmtime(&ftime));
		xmlNewChild(root, NULL, VALID_TIME, valid_time);

		thresholds = xmlNewChild(root, NULL, THRESHOLDS, NULL);
		for(n = 0; n < nthreshold_val; n++)
		{
			char level[32], weight[32];
			snprintf(level, sizeof(level), "%d", n+1);
			snprintf(weight, sizeof(weight), "%.1f", threshold_val[n]); 
			threshold = xmlNewChild(thresholds, NULL, THRESHOLD, NULL);
			xmlSetProp(threshold, LEVEL_PROP, level);
			xmlNewChild(threshold, NULL, rankweight_id, weight);
		}

		for(n = 0; n < nstorms; n++)
		{
			xmlNodePtr storm = xmlNewChild(root, NULL, STORM, NULL);
			xmlNewChild(storm, NULL, STORM_ID, storms[n].id);
			for(k = i; k < nforecasts; k++)
			{
				char rank_id[32], weight[32];
				snprintf(rank_id, sizeof(rank_id), "%s%ld", rankweight_id, storms[n].fcst[k].time);
				snprintf(weight, sizeof(weight), "%.1f", storms[n].fcst[k].rank);
				xmlNewChild(storm, NULL, rank_id, weight);
			}
		}
		(void) strftime(fname, sizeof(fname), file_time_mask, gmtime(&ftime));
		xmlSaveFormatFileEnc(pathname(stat_dir,fname), doc, "ISO-8859-1", 1);
		printdebug("Created forecast file \'%s\'", fname);
		xmlFreeDoc(doc);
	}
}
