/*========================================================================*/
/*
*	File:		product_status.c
*
*   Functions:  ProductStatusAddInfo()
*				ProductStatusGetGenerateTime()
*               ProductStatusGetInfo()
*               ProductStatusRemoveInfo()
*               ProductStatusUpdateInfo()
*
*	Purpose:	Stores the status of all dependent product and model
*               processes.  The status is saved in the interface state
*				file as the key "ps.n" where n is an arbitrary count.
*               For those products which are "released" from the FPA the
*               status information is stored in the forecast release
*               directory so that the release state can be determined.
*               This was done as one client (Arctic) puts the release
*               as a common directory to many databases and the users
*               of these various databases want to know when something
*               is released! The only way I could think of handling this
*               was to put all of the relevant status info into a file
*               in the release directory.
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
/*========================================================================*/
#include "global.h"
#include "resourceDefines.h"
#include "iconBar.h"
#define  PS_MAIN
#include "productStatus.h"

static void cycle_product_indicator (XtPointer, XtIntervalId*);

static int ntotal = 0;
static int npi = 0;
static PRODUCT_INFO *pi = NULL;
static int p_running_count = 0;
static int new_p_id = 0;
static int state_file_id = 0;
static Widget indicator_widget = NullWidget;
static Pixmap staticPixmap = XmUNSPECIFIED_PIXMAP;


/*============================================================================*/
/*
*	ProductStatusAddInfo() - Adds a product to the product status database.
*	
*	Notes: The type parameter must be one of the macros defined in global.h.
*          The label parameter must be a static string in the calling procedure
*          as the database does not make a copy. If the product has a release
*          mechanism, that is the user "releases" the product with a button
*          push into a file, then a function to call which will return true
*          or false if the product has been released must be given in
*          release_fcn. This function will be called with the product is as
*          returned by this add function.
*/
/*============================================================================*/
int ProductStatusAddInfo(PS_TYPE type , String label , Boolean (*release_fcn)() )
{
	int n;
	char key[16];

	if (!state_file_id) state_file_id = XuSetStateFile(
						get_path(FCST_RELEASE,RELEASED_PROD_STATE_FILE), True);


	/* Create a unique key for this entry. The label is hashed and the string
	*  length and type added as an additional precaution. (See "C Programming
	*  Language" second edition by Kernighan and Ritchie, pg. 144.)
	*/
	{
		unsigned long hashval;
		Byte *s = (Byte *)label;
		hashval = *s++;
		while (*s) hashval = *s++ + 31 * hashval;
		hashval = (hashval%1000001)*1000+safe_strlen(label)*10+type;
		(void) snprintf(key, sizeof(key), "%.9lu", hashval);
	}

	/* Exit if we already have an entry for the product.
	*/
	for(n = 0; n < npi; n++)
	{
		if(same(key, pi[n].key)) return pi[n].id;
	}

	/* Assign space for the product and fill in the data.
	*/
	if( npi >= ntotal )
	{
		ntotal++;
		pi = MoreMem(pi, PRODUCT_INFO, ntotal);
	}
	new_p_id++;
	pi[npi].id          = new_p_id;
	strcpy(pi[npi].key, key);
	pi[npi].type        = type;
	pi[npi].label       = label;
	pi[npi].is_running  = False;
	pi[npi].release_fcn = (release_fcn)? release_fcn:NULL;

	if(pi[npi].release_fcn) XuSetStateStore(state_file_id);
	if(!XuStateDataGet(PSKEY, prod_data[type].key, pi[npi].key, &pi[npi].statinfo))
	{
		pi[npi].statinfo = XtNewString(XuGetLabel("noPrevRun"));
	}
	npi++;
	return new_p_id;
}


/*============================================================================*/
/*
*	ProductStatusUpdateInfo() - Update the product status information.  Note
*   that the product must have been added to the database with add above.
*/
/*============================================================================*/
void ProductStatusUpdateInfo(int product_id , PS_TYPE run_status , String statinfo )
{
	int  i;
	char status[256];
	Boolean is_running;

	if (!state_file_id) state_file_id = XuSetStateFile(
						get_path(FCST_RELEASE,RELEASED_PROD_STATE_FILE), True);

	/* Keep track of the number of products running.  This is used to 
	*  determine if the product running indicator light is to be on.
	*  The count is clamped at a minimum of 0 as a safety measure.
	*/
	ZeroBuffer(status);

	if(!blank(statinfo)) (void) strncpy(status, statinfo, 255);

	switch(run_status)
	{
		case PS_RUNNING:
			p_running_count++;
			is_running = True;
			if(blank(statinfo)) strcpy(status, XuGetLabel("running"));
			break;

		case PS_UPDATE:
			is_running = True;
			if(blank(statinfo)) strcpy(status, XuGetLabel("running"));
			break;

		case PS_ENDED:
			p_running_count--;
			is_running = False;
			if(blank(statinfo)) (void) snprintf(status, sizeof(status), "%ld", (long int) time((time_t*)0));
			break;

		case PS_ERROR:
			p_running_count--;
			is_running = False;
			if(blank(statinfo)) strcpy(status, XuGetLabel("abort"));
			break;
	}

	pr_diag("ProductStatusUpdateInfo", "Product running count = %d\n", p_running_count);

	/* If the indicator widget is not assigned yet get it and also get the
	 * default pixmap from the widget.
	 */
	if(!indicator_widget)
	{
		indicator_widget = GetIconBarWidget(PRODUCT_STATUS_ICON);
		if (indicator_widget) 
			XtVaGetValues(indicator_widget, XmNlabelPixmap, &staticPixmap, NULL);
	}

    if(p_running_count > 0 )
	{
		cycle_product_indicator((XtPointer)ON, NULL);
	}
	else
	{
		cycle_product_indicator((XtPointer)OFF, NULL);
		p_running_count = 0;
	}

	/* Find the product and set the state.  Save the data only after the
	*  product has actually completed.
	*/
	for( i = 0; i < npi; i++ )
	{
		if(product_id != pi[i].id) continue;
		pi[i].is_running = is_running;
		XtFree(pi[i].statinfo);
		pi[i].statinfo = XtNewString(status);
		ProductStatusDialogUpdate(&pi[i]);
		if(!is_running)
		{
			if(pi[i].release_fcn) XuSetStateStore(state_file_id);
			XuStateDataSave(PSKEY, prod_data[pi[i].type].key, pi[i].key, pi[i].statinfo);
		}
		break;
	}
}


/*============================================================================*/
/*
*	ProductStatusReleaseCheck() - Check to see if the given product has been
*                                 released and update its generate time from
*   the state file if it has been.
*/
/*============================================================================*/
Boolean ProductStatusReleaseCheck(int product_id )
{
	int i;
	Boolean released;

	if (!state_file_id) state_file_id = XuSetStateFile(
						get_path(FCST_RELEASE,RELEASED_PROD_STATE_FILE), True);

	for( i = 0; i < npi; i++ )
	{
		if(product_id != pi[i].id) continue;
		if(pi[i].is_running) return False;
		if(!prod_data[pi[i].type].has_release_status) return False;
		if(!pi[i].release_fcn) return False;
		released = pi[i].release_fcn(pi[i].id);
		if(released)
		{
			XtFree(pi[i].statinfo);
			XuSetStateStore(state_file_id);
			(void) XuStateDataGet(PSKEY, prod_data[pi[i].type].key, pi[i].key, &pi[i].statinfo);
		}
		return released;
	}
	return False;
}


/*============================================================================*/
/*
*	ProductStatusRemoveInfo() - Remove an entry;
*/
/*============================================================================*/
void ProductStatusRemoveInfo(int product_id )
{
	int i, posn;

	posn = -1;
	for( i = 0; i < npi; i++ )
	{
		if( product_id != pi[i].id) continue;
		posn = i;
		XtFree(pi[i].statinfo);
		break;
	}
	if(posn < 0) return;

	npi--;
	for( i = posn; i < npi; i++ )
	{
		pi[i].type        = pi[i+1].type;
		pi[i].id          = pi[i+1].id;
		strcpy(pi[i].key, pi[i+1].key);
		pi[i].label       = pi[i+1].label;
		pi[i].is_running  = pi[i+1].is_running;
		pi[i].release_fcn = pi[i+1].release_fcn;
		pi[i].statinfo    = pi[i+1].statinfo;
	}
}


/*===================================================================*/
/*
*   ProductStatusGetInfo() - Returns a list of information for the
*                            specified type of product. If the type
*   is PS_RUNNING then a list of all running product is returned.
*   If the parameter list is NULL, then the list is not returned.
*
*   NOTE that it is the responsibility of the calling product to
*   free the memory allocated for the list.
*/
/*===================================================================*/
void ProductStatusGetInfo(PS_TYPE type , int *nlist , PRODUCT_INFO **list )
{
	int i, total;
	PRODUCT_INFO *plist;

	total = 0;
	plist = NULL;
	for(i = 0; i < npi; i++)
	{
		if(type == PS_RUNNING && !pi[i].is_running ) continue;
		if(type != PS_RUNNING && pi[i].type != type) continue;
		total++;
	}
	if(total > 0 && list)
	{
		plist = NewMem(PRODUCT_INFO, total);
		total = 0;
		for(i = 0; i < npi; i++)
		{
			if(type == PS_RUNNING && !pi[i].is_running ) continue;
			if(type != PS_RUNNING && pi[i].type != type) continue;
			plist[total].type        = pi[i].type;
			plist[total].id          = pi[i].id;
			strcpy(plist[total].key, pi[i].key);
			plist[total].label       = pi[i].label;
			plist[total].is_running  = pi[i].is_running;
			plist[total].release_fcn = pi[i].release_fcn;
			plist[total].statinfo    = pi[i].statinfo;
			total++;
		}
	}
	if (nlist) *nlist = total;
	if (list)  *list  = plist;
}


long ProductStatusGetGenerateTime(int pid )
{
	int i;
	for(i = 0; i < npi; i++)
	{
		if(pid == pi[i].id) return atol(pi[i].statinfo);
	}
	return 0;
}


static void cycle_product_indicator(XtPointer client_data, XtIntervalId *id)
{
	static int    cycle_count = -1;
	static Pixmap pixmaps[8];

	if(!indicator_widget) return;

	if(id)
	{
		if (cycle_count >= 0)
		{
			XtVaSetValues(indicator_widget, XmNlabelPixmap, pixmaps[cycle_count], NULL);
			cycle_count = (cycle_count+1)%8;
			(void)XtAppAddTimeOut(GV_app_context, 250, cycle_product_indicator, NULL);
		}
		else
		{
			XtVaSetValues(indicator_widget, XmNlabelPixmap, staticPixmap, NULL);
		}
	}
	else if(client_data)
	{
		if (cycle_count < 0)
		{
			int n;
			char buf[32];
			for(n = 0; n < 8; n++)
			{
				snprintf(buf, 32, "rotatingArrow-20x20-%d", n);
				pixmaps[n] = XuGetPixmap(GW_mainWindow, buf);
			}
			cycle_count = 0;
			(void)XtAppAddTimeOut(GV_app_context, 0, cycle_product_indicator, NULL);
		}
	}
	else
	{
		if(cycle_count >= 0)
		{
			int n;
			for(n = 0; n < 8; n++)
				XuFreePixmap(GW_mainWindow, pixmaps[n]);
		}
		cycle_count = -1;
	}
}
