/********************************************************************************/
/*
 *      File: radarSTAT_calcFcns.c
 *
 *   Purpose: Contains the functions for calculating rank weight and for
 *            forecasting the rank weight and element trends. The forecasting
 *            can be done by an external process and the control for this is
 *            also in this file.
 */
/********************************************************************************/
#include "global.h"
#include "radarSTAT.h"

extern STATCFG	statcfg;
extern int      storm_list_len;
extern STORM    *storm_list;

/* Forward function declarations */
static void create_element_trend_infile (STORM*, int, ELEMCFG**, int);
static void create_rankweight_trend_infile (STORM*, ELEMCFG*);
static void run_external_process (void (*callFcn)(XtPointer));
static void read_element_trend_fcst (XtPointer);
static void read_rankweight_trend_fcst (XtPointer);

/************** Utility Functions *****************/

/* Non-destructive parse of the given string for a float value
 */
float rs_float_parse(String str, Boolean *ok)
{
	no_white(str);
	if(same_ic(str,DATA_NA))
	{
		*ok = False;
		return 0.0;
	}
	else
	{
		String end;
		float val = (float) strtod(str, &end);
		if (ok) *ok = (end != str);
		return val;
	}
}


/* Non-destructive parse of the given string for an int value
 */
int rs_int_parse(String str, Boolean *ok)
{
	no_white(str);
	if(same_ic(str,DATA_NA))
	{
		*ok = False;
		return 0;
	}
	else
	{
		String end;
		int val = (int) strtol(str, &end, 0);
		if (ok) *ok = (end != str);
		return val;
	}
}


/************ Trend forecast and rank weight calculation functions *************/

/* This is the function where the weight contribution of the
 * elements is calculated. For each element the equation is
 * (a*(m*x)⁴+b*(m*x)³+c*(m*x)²+d*(m*x))*f.
 * Any of a,b,c and d can be 0. (x⁴ is not in most equations).
 */
static float calc_element_rankweight_contribution(ELEMCFG *elem, float inval)
{
	double rval = 0;
	double sval = (double) inval * elem->rank_calc_const.m;

	if(elem->rank_calc_const.a != 0.0)
	{
		rval += elem->rank_calc_const.a * pow(sval,4);
	}
	if(elem->rank_calc_const.b != 0.0)
	{
		rval += elem->rank_calc_const.b * pow(sval,3);
	}
	if(elem->rank_calc_const.c != 0.0)
	{
		rval += elem->rank_calc_const.c * pow(sval,2);
	}
	if(elem->rank_calc_const.d != 0.0)
	{
		rval += elem->rank_calc_const.d * sval;
	}

	return ((float)(rval * elem->rank_calc_const.f));
}


/* Calculate the rank weight for the given storm instance index. This corresponds
 * to the storm->hist[ndx] data structure in storm.
 */
static void rs_calc_storm_instance_rankweight(STORM *storm, int ndx)
{
	int n, pos;
	float rw = 0;
	for(n = 0; n < statcfg.rankweight.nelem; n++)
	{
		pos = statcfg.rankweight.elem[n]->ndx;
		if(storm->hist[ndx].elem[pos].off) continue;
		if(storm->hist[ndx].elem[pos].na) continue;
		rw += calc_element_rankweight_contribution(statcfg.rankweight.elem[n], storm->hist[ndx].elem[pos].value);
	}
	pos = rs_get_element_array_pos_from_id(statcfg.rankweight.element_id);
	storm->hist[ndx].elem[pos].value = rw;
	storm->hist[ndx].elem[pos].na = False;
	storm->hist[ndx].elem[pos].usermod = False;
}


/* Calculate the rank weight for the most recent instance of the
 * storm only. This corresponds to storm->hist[0].
 */
void rs_calc_storm_T0_rankweight(STORM *storm)
{
	rs_calc_storm_instance_rankweight(storm, 0);
}


/* Calculate the rank weight for all of the storm instances
 */
void rs_calc_storm_rankweight(STORM *storm)
{
	int i, pos;
	for(i = 0; i < storm->nhist; i++)
		rs_calc_storm_instance_rankweight(storm, i);
}


/* Algorithms for producing a least square forecast for a specific element in a storm.
 * Note that storm_list->hist[0] contains the most recent data points.
 */
static void make_least_square_trend_forecast(STORM *storm, ELEMCFG *elem)
{
	int i, npoints, pos, ndata;
	time_t t, *times;
	Boolean allpos = True, allneg = True;

	/* The element has to be numeric in order to be processed */
	if(elem->data_type != DT_NUMERIC) return;
	if(storm->nhist < 1) return;

	pos = elem->ndx;

	/* As the time values are very large and they are squared, normalize
	 * and make the time values minutes.
	 * Stop searching if a data point is not available. Normally the radar
	 * parameters are either positive or negative and thus have a ceiling
	 * or floor of 0. This fact is tested for in this loop and used later
	 * to set a floor or ceiling.
	 */
	npoints = MIN(statcfg.num_fcst_data_points,storm->nhist);
	times = NewMem(time_t, npoints);
	for(ndata = 0; ndata < npoints; ndata++)
	{
		if(storm->hist[ndata].elem[pos].na) break;
		if(storm->hist[ndata].elem[pos].off) break;
		times[ndata] = (storm->hist[ndata].vtime - storm->hist[npoints-1].vtime)/60;
		if(storm->hist[ndata].elem[pos].value > 0) allneg = False;
		if(storm->hist[ndata].elem[pos].value < 0) allpos = False;
	}

	/* Forecast method depends on the number of data points.
	 */
	if(ndata == 1)
	{
		/* If just one instance extrapolate */
		for(t = 0, i = 0; i < statcfg.num_forecasts; i++)
		{
			t += (time_t) statcfg.time_interval;
			storm->fcst[i].vtime = storm->vtime + t * 60;
			storm->fcst[i].elem[pos].value = storm->data[pos].value;
			storm->fcst[i].elem[pos].na = False;
			storm->fcst[i].elem[pos].usermod = False;
		}
	}
	else if(ndata == 2)
	{
		double m, b;
		/* Only 2 data points so use the equation of a line for extrapolation */
		m = (double) (storm->hist[0].elem[pos].value - storm->hist[1].elem[pos].value)/(double)(times[0] - times[1]);
		b = (double) storm->hist[0].elem[pos].value - m*(double)times[0];
		for(t = 0, i = 0; i < statcfg.num_forecasts; i++)
		{
			t += (time_t) statcfg.time_interval;
			storm->fcst[i].vtime = storm->vtime + t * 60;
			storm->fcst[i].elem[pos].value = (float)(m*(double)(times[0]+t) + b);
			storm->fcst[i].elem[pos].na = False;
			storm->fcst[i].elem[pos].usermod = False;
			if(allpos) storm->fcst[i].elem[pos].value = MAX(0,storm->fcst[i].elem[pos].value);
			if(allneg) storm->fcst[i].elem[pos].value = MIN(0,storm->fcst[i].elem[pos].value);
		}
	}
	else if(ndata > 2)
	{
		/* Use a least square linear fit to the data points
		 */
		double m, b;
		double sumx  = 0.0;	/* sum of x */
		double sumx2 = 0.0;	/* sum of x ** 2 */
		double sumxy = 0.0;	/* sum of x * y */
		double sumy  = 0.0;	/* sum of y */
		double nr = (double) ndata;

		for(i = 0; i < ndata; i++)
		{
			sumx += (double) times[i];
			sumx2 += (double)(times[i] * times[i]);
			sumxy += (double) times[i] * (double) storm->hist[i].elem[pos].value;
			sumy += (double) storm->hist[i].elem[pos].value;
		}
		m = (nr * sumxy - sumx * sumy) / (nr * sumx2 - sumx * sumx);
		b = (sumy * sumx2 - sumx * sumxy) / (nr * sumx2 - sumx * sumx);

		for(t = 0, i = 0; i < statcfg.num_forecasts; i++)
		{
			t += (time_t) statcfg.time_interval;
			storm->fcst[i].vtime = storm->vtime + t * 60;
			storm->fcst[i].elem[pos].value = (float)(m*(double)(times[0]+t) + b);
			storm->fcst[i].elem[pos].na = False;
			storm->fcst[i].elem[pos].usermod = False;
			if(allpos) storm->fcst[i].elem[pos].value = MAX(0,storm->fcst[i].elem[pos].value);
			if(allneg) storm->fcst[i].elem[pos].value = MIN(0,storm->fcst[i].elem[pos].value);
		}
	}

	FreeItem(times);
}


/* Reduce the list of elements down to those that need trend forecasts produced for them.
 * Rankweight elements are filtered out as they are a special case.
 * The calling function is responsible for freeing the allocated elems array.
 */
static ELEMCFG **element_trend_filtered_list(ELEMCFG *element, int nelement, int *nelems)
{
	int n, k;
	ELEMCFG **elems = NewMem(ELEMCFG*, statcfg.nelement);
	for(k = 0, n = 0; n < nelement; n++)
	{
		elems[k] = element+n;
		if(elems[k]->data_type != DT_NUMERIC) continue;
		if(!elems[k]->flag_severe) continue;
		if(elems[k]->type == ET_RANK_ELEM) continue;
		if(elems[k]->type == ET_RANK_FCST) continue;
		k++;
	}
	*nelems = k;
	return elems;
}


/* initialize the forecast array for the storm elements.
 */
static void init_storm_trend_forecasts(STORM *storm, ELEMCFG **elems, int nelems)
{
	int i, n;
	for(i = 0; i < statcfg.num_forecasts; i++)
	{
		/* vtime is seconds while time_interval is minutes, thus the *60 */
		storm->fcst[i].vtime = storm->vtime + (time_t) (statcfg.time_interval*(i+1)*60);
		for(n = 0; n < nelems; n++)
		{
			int ndx = elems[n]->ndx;
			storm->fcst[i].elem[ndx].value = 0;
			storm->fcst[i].elem[ndx].off = False;
			storm->fcst[i].elem[ndx].na = True;
			storm->fcst[i].elem[ndx].usermod = False;
		}
	}
}


/* Make a forecast for storm elements. Note that forecasts are not done for
 * non-numeric elements or for rank weights as this is a special case.
 */
void rs_make_storm_element_trend_forecast(STORM *storm, ELEMCFG *element, int nelement)
{
	int i, n, nelems;
	ELEMCFG **elems;

	elems = element_trend_filtered_list(element, nelement, &nelems);

	if(statcfg.trend.program)
	{
		create_element_trend_infile(storm, 1, elems, nelems);
		run_external_process(read_element_trend_fcst);
	}
	else
	{
		init_storm_trend_forecasts(storm, elems, nelems);
		for(n = 0; n < nelems; n++)
			make_least_square_trend_forecast(storm, elems[n]);
	}
	FreeItem(elems);
}


/* Make trend forecasts for all storms at once. If using an external
 * program this makes the process more efficient.
 */
void rs_make_all_storms_element_trend_forecasts(void)
{
	int n;

	if(statcfg.trend.program)
	{
		int nelems;
		ELEMCFG **elems = element_trend_filtered_list(statcfg.element, statcfg.nelement, &nelems);
		create_element_trend_infile(storm_list, storm_list_len, elems, nelems);
		run_external_process(read_element_trend_fcst);
		FreeItem(elems);
	}
	else
	{
		for(n = 0; n < storm_list_len; n++)
			rs_make_storm_element_trend_forecast(storm_list + n, statcfg.element, statcfg.nelement);
	}
}



void rs_make_rankweight_forecast(STORM *storm)
{
	int i, pos;

	pos = rs_get_element_array_pos_from_id(statcfg.rankweight.element_id);
	if(statcfg.trend.program)
	{
		create_rankweight_trend_infile(storm, &statcfg.element[pos]);
		run_external_process(read_rankweight_trend_fcst);
	}
	else
	{
		make_least_square_trend_forecast(storm, &statcfg.element[pos]);
	}

	/* Copy the values from the rankweight forecast array to the data values
	 * of the rankweight forecast elements.
	 */
	for(i = 0; i < statcfg.num_forecasts; i++)
	{
		/* the position function wants time in minutes */
		int m, dt = (int) ((storm->fcst[i].vtime - storm->vtime) / 60);
		if((m = rs_get_rankweight_element_array_pos_from_time(dt)) >= 0)
		{
			storm->data[m].value = storm->fcst[i].elem[pos].value;
			storm->data[m].na = False;
			storm->data[m].usermod = False;
		}
	}
}


/*************** External Program Execution *********************/

/* The functions in this section are for running an external process that takes
 * its data from a file, produces trend forecasts for the provided elements and
 * returns the trends using another file. A non-blocking wait with a timeout is
 * done on the program to ensure that things do not hang if there are problems.
 *
 * Input and output file formats. All files in xml.
 *
 * The input files have a root name of "ElementForecast" or "RankWeightForecast".
 * The difference is that "ElementForecast" wants trend forecasts for all of the
 * elements provided for all storms, while "RankWeightForecast" wants trend forecasts
 * only for the rank weight element of the storms. In the later case the elements used
 * to calculate the rank weight are also provided, but only because there is no way of
 * knowing what at the current time what the calculation program will need as input.
 *
 */


/* Add the storm along with the list of elements to the root node
 */
static void add_storm_data(xmlNodePtr root, STORM *storm, ELEMCFG **elems, int nelems)
{
	int i, n;
	char buf[2000];	/* This should handle any real world storm time length */
	size_t len = sizeof(buf);

	xmlNodePtr sptr = xmlNewChild(root, NULL, STORM_KEY, NULL);
	snprintf(buf, len, "%d", storm->num);
	xmlSetProp(sptr, STORM_NUMBER, buf);
	snprintf(buf, len, "%d", storm->nhist);
	xmlNewChild(sptr, NULL, NTIMES, buf);
	strcpy(buf,"");
	for(n = 0; n < storm->nhist; n++)
	{
		size_t blen = safe_strlen(buf);
		int dt = (int) (storm->hist[n].vtime - storm->vtime)/60;
		snprintf(buf+blen, len-blen, "%d,", dt);
	}
	xmlNewChild(sptr, NULL, TIMEDELTA, buf);
	for(n = 0; n < nelems; n++)
	{
		strcpy(buf,"");
		for(i = 0; i < storm->nhist; i++)
		{
			size_t blen = safe_strlen(buf);
			if(storm->hist[i].elem[elems[n]->ndx].na || storm->hist[i].elem[elems[n]->ndx].off)
				snprintf(buf+blen, len-blen, "%s,", DATA_NA);
			else
				snprintf(buf+blen, len-blen, "%g,", storm->hist[i].elem[elems[n]->ndx].value);
		}
		xmlNewChild(sptr, NULL, elems[n]->id, buf);
	}
}


/* Convert the seconds from reference to yyymmddhhmm format. Note that all storms begin
 * at the same valid time, thus the first storm in any sequence will do.
 */
static void make_storm_time_string(STORM *storm, String buf)
{
	int year, month, jday, day, hour, min;
	decode_clock(storm->vtime, &year, &jday, &hour, &min, NULL);
	mdate(&year, &jday, &month, &day);
	snprintf(buf, 32, "%.4d%.2d%.2d%.2d%.2d", year, month, day, hour, min);
}


/* This will output a data file request for any number of storms and any nubmer of elements
 * within those storms.
 */
static void create_element_trend_infile(STORM *storms, int nstorms, ELEMCFG **elems, int nelems)
{
	int n;
	char buf[32];
	xmlDocPtr doc;
	xmlNodePtr root;

	doc = xmlNewDoc("1.0");
	root = xmlNewNode(NULL,ELEMENT_FCST);
	xmlDocSetRootElement(doc, root);
	make_storm_time_string(storms, buf);
	xmlNewChild(root, NULL, VALIDTIME, buf);
	snprintf(buf, sizeof(buf), "%d", statcfg.num_forecasts);
	xmlNewChild(root, NULL, NFORECASTS, buf);
	snprintf(buf, sizeof(buf), "%d", statcfg.time_interval);
	xmlNewChild(root, NULL, TIMEINTERVAL, buf);
	for(n = 0; n < nstorms; n++)
	{
		init_storm_trend_forecasts(storms+n, elems, nelems);
		add_storm_data(root, storms+n, elems, nelems);
	}
	xmlSaveFormatFileEnc(statcfg.trend.infile, doc, "ISO-8859-1", 1);
	xmlFreeDoc(doc);
}


/* Produce the input file for calculating the rank weight of a single storm
 */
static void create_rankweight_trend_infile(STORM *storm, ELEMCFG *rw_elem)
{
	int n;
	char buf[32];
	ELEMCFG **elems;
	xmlDocPtr  doc;
	xmlNodePtr root;

	doc = xmlNewDoc("1.0");
	root = xmlNewNode(NULL,RANKWEIGHT_FCST);
	xmlDocSetRootElement(doc, root);
	make_storm_time_string(storm, buf);
	xmlNewChild(root, NULL, VALIDTIME, buf);
	snprintf(buf, sizeof(buf), "%d", statcfg.num_forecasts);
	xmlNewChild(root, NULL, NFORECASTS, buf);
	snprintf(buf, sizeof(buf), "%d", statcfg.time_interval);
	xmlNewChild(root, NULL, TIMEINTERVAL, buf);

	elems = NewMem(ELEMCFG*, statcfg.rankweight.nelem+1);
	elems[0] = rw_elem;
	for(n = 0; n < statcfg.rankweight.nelem; n++)
		elems[n+1] = statcfg.rankweight.elem[n];
	init_storm_trend_forecasts(storm, elems, 1);
	add_storm_data(root, storm, elems, statcfg.rankweight.nelem+1);
	FreeItem(elems);

	xmlSaveFormatFileEnc(statcfg.trend.infile, doc, "ISO-8859-1", 1);
	xmlFreeDoc(doc);
}


/* When the output file is closed after a write this function is called. This should
 * mean that the external process has written its data into the file. The assumption
 * is made that there are statcfg.num_forecasts data points and that they are for
 * storm vtime + statcfg.time_interval * n where n goes from 1 to num_forecasts.
 */
static void read_trend_data(String root_name)
{
	time_t vtime = 0;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL, cur = NULL, elem = NULL;

	rs_user_message(UM_STATUS,NULL);

	if((doc = xmlReadFile(statcfg.trend.outfile, NULL, 0)))
		root = xmlDocGetRootElement(doc);
	if(!doc || !root)
	{
		rs_user_message(UM_ERROR, "Could not read output file from element trend forecast program.");
		if(doc) xmlFreeDoc(doc);
		return;
	}
	else if(!NODENAME(root,root_name))
	{
		rs_user_message(UM_ERROR, "Output file from element trend forecast program is wrong type.");
		if(doc) xmlFreeDoc(doc);
		return;
	}

	/* The following assumes that the validtime will be found before the first storm.
	 */
	for(cur = root->children; cur; cur = cur->next)
	{
		if(KEYNODE(cur,VALIDTIME))
		{
			int year, jday, month, day, hour, minute;
			String val = xmlNodeGetContent(cur);
			no_white(val);
			sscanf(val,"%4d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute);
			xmlFree(val);
			jdate(&year, &month, &day, &jday);
			vtime = encode_clock(year, jday, hour, minute, 0);
		}
		else if(KEYNODE(cur,STORM_KEY))
		{
			int snum = 0;
			STORM *storm = NULL;
			String val = xmlGetProp(cur, STORM_NUMBER);
			snum = atoi(val);
			xmlFree(val);
			storm = rs_get_storm_ptr_from_storm_number(snum);
			if(!storm)
			{
				rs_user_message(UM_ERROR,"Unknown storm \'%d\' in trend program output file.", snum);
			}
			else if(storm->vtime != vtime)
			{
				rs_user_message(UM_ERROR,"Valid time mismatch in trend program output file.");
			}
			else
			{
				for(elem = cur->children; elem; elem = elem->next)
				{
					int pos;
					if(elem->type != XML_ELEMENT_NODE) continue;
					/* For rank weight forecasts the element looked at is restricted to just rank weight */
					if(same(root_name,RANKWEIGHT_FCST) && !NODENAME(elem, statcfg.rankweight.element_id)) continue;
					pos = rs_get_element_array_pos_from_id((String) elem->name);
					if(pos >= 0)
					{
						int n;
						String p;
						val = xmlNodeGetContent(elem);
						p = strtok(val,",");
						for(n = 0; n < statcfg.num_forecasts; n++)
						{
							Boolean ok;
							storm->fcst[n].elem[pos].value = rs_float_parse(p, &ok);
							storm->fcst[n].elem[pos].na = !ok;
							p = strtok(NULL,",");
						}
						xmlFree(val);
					}
				}
			}
		}
	}
	xmlFreeDoc(doc);
}


/* Read the output file from the external program. We want to see a root node
 * name of "ElementForecast" to ensure that it is the proper file type.
 */
static void read_element_trend_fcst(XtPointer data)
{
	/* Set the flag indicating that the function has been called */
	*((long*)data) = 0;
	read_trend_data(ELEMENT_FCST);
}


/* Read the output file from the external program. We want to see a root node
 * name of "RankWeightForecast" to ensure that it is the proper file type.
 */
static void read_rankweight_trend_fcst(XtPointer data)
{
	/* Set the flag indicating that the function has been called */
	*((long*)data) = 0;
	read_trend_data(RANKWEIGHT_FCST);
}


/* Called if the time out interval is reached before read_trend_data is called.
 * This usually means that the trend program hung or aborted.
 */
static void timeout_cb(XtPointer data, XtIntervalId id)
{
	*((long*)data) = 0;
	rs_user_message(UM_ERROR,"Trend forecast calculation program time out.");
}


/* Run the external program defined in the configuration file to calculate forecast
 * trends for elements and rank weights. The program waits for the program to finish,
 * as indicated by a write and close of the output file. A timeout is set to continue
 * after a specified interval if the external program does not write to the file. This
 * probably means it broke !
 */
static void run_external_process(void (*rtnFcn)(XtPointer))
{
	long wait = 1;
	int wd = AddFileWriteCloseWatch(statcfg.trend.outfile, rtnFcn, (XtPointer) &wait);
	rs_user_message(UM_STATUS,NULL);
	if(shrun(statcfg.trend.program, False) == 0)
	{
		XtIntervalId id = XtAppAddTimeOut(GV_app_context, statcfg.trend.timeout,
				(XtTimerCallbackProc) timeout_cb, (XtPointer) &wait);
		while(wait || XtAppPending(GV_app_context))
			XtAppProcessEvent(GV_app_context, XtIMAll);
		XtRemoveTimeOut(id);
	}
	else
	{
		rs_user_message(UM_ERROR,"Unable to run trend forecast calculation program");
	}
}
