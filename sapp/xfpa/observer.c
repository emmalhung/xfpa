/****************************************************************************/
/*
 *  File:     observer.c
 *
 *	Purpose:  A set of functions to provide a system that registers an
 *	          observer for a given observable. When a procedure with some
 *	          state changes that need to be known about by other procedures
 *	          it uses a notification function to send a notification of 
 *	          change to all registered observers. The notify function may be
 *	          inserted into a procedure even if no observers register as no
 *	          action is taken if there are no observers.
 *
 * Functions: A set of generic functions where the obid is defined as an
 *            enumerated type in observer.h and the data passed to the
 *            registered function depends on the obid:
 *
 *            AddObserver     (int obid, void (*fcn)(String*,int))
 *            DeleteObserver  (int obid, void (*fcn)(String*,int))
 *            NotifyObservers (int obid, String *parms, int nparms)
 *
 *            A set of functions for Ingred messages:
 *
 *            AddIngredObserver     (void (*fcn)(CAL,String*,int))
 *            DeleteIngredObserver  (void (*fcn)(CAL,String*,int))
 *            NotifyIngredObservers (CAL cal, String *parms, int nparms)
 *
 *            A set of functions for source change notification:
 *
 *            AddSourceObserver     (Boolean (*fcn)(Boolean), String label)
 *            DeleteSourceObserver  (Boolean (*fcn)(Boolean))
 *            NotifySourceObservers (void (*fcn)(void), Boolean state)
 *
 *            The function used by the source observers is expected to return
 *            a value of True if it completes its actions or False if it does
 *            not. In this way things like guidance can do one source type at
 *            a time in a work procedure so as to minimize the effect of 
 *            updates on the program in general. The function parameter state
 *            indicates if the user should be made aware of the source change
 *            or not and the function given in the notify function will be
 *            called once all of the source observers have been notified.
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
/****************************************************************************/
#include <stdarg.h>
#include "global.h"

/* ids for the two special function sets */
#define INGRED_ID	-97
#define SOURCE_ID	-98


/* Called function type union */
typedef union {
	void    (*normal)(String*,int);
	void    (*ingred)(CAL,String*,int);
	Boolean (*source)(Boolean);
} CFTU;

/* The notification database structure */
typedef struct {
	int    id;		/* observer identifier */
	int    nfcn;	/* number of registered functions */
	CFTU   *fcn;	/* function to activate on notify */
	String *label;	/* Label associated with function, used only by source observer. */
} OBDS;

/* Observer source notify structure for use in the source notify work procedure */
typedef struct {
	int     count;			/* the number of times the function has been called */
	int     fndx;			/* number of functions registered */
	Boolean state;			/* state parameter from notify call */
	void    (*fcn)(void);	/* function to call after notify */
} OBSNS;


/*====================== Private Functions ============================*/



/* Find the element of the observer data array that corresponds to the id
 */
static OBDS *get_data(int id)
{
	static int  alloc_array = 0;
	static int  narray      = 0;
	static OBDS *array      = NULL;

	int  i;
	OBDS *data;

	for(i = 0; i < narray; i++)
	{
		if(id == array[i].id) return (array + i);
	}

	if(narray >= alloc_array)
	{
		array = MoreMem(array, OBDS, (alloc_array += 100));
	}

	data = array + narray;
	narray++;

	data->id    = id;
	data->nfcn  = 0;
	data->fcn   = NULL;
	data->label = NULL;
	return (data);
}


/*=========================  Public Functions ===========================*/


/* Register for notification with the given function to be called. Upon
 * notification the given function will be called with arguments as
 * specified for each odid in the observer.h header file.
 */
void AddObserver (int obid, void (*fcn)(String*,int))
{
	int n;
	OBDS *data = get_data(obid);

	/* Check for duplicate function registration */
	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].normal == fcn) return;
	}

	/* Check for null entries and recycle them */
	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].normal) continue;
		data->fcn[n].normal = fcn;
		return;
	}

	/* No entries available so add to function list */
	data->fcn = MoreMem(data->fcn, CFTU, data->nfcn+1);
	data->fcn[data->nfcn].normal = fcn;
	data->nfcn++;
}


void DeleteObserver (int obid, void (*fcn)(String*,int))
{
	int n;
	OBDS *data = get_data(obid);

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].normal == fcn)
			data->fcn[n].normal = NULL;
	}
}


void NotifyObservers (int obid, String *parms, int nparms)
{
	int n;
	OBDS *data = get_data(obid);

	for(n = 0; n < data->nfcn; n++)
	{
		if(data->fcn[n].normal)
			data->fcn[n].normal(parms, nparms);
	}
}


void AddIngredObserver (void (*fcn)(CAL,String*,int))
{
	int n;
	OBDS *data = get_data(INGRED_ID);

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].ingred == fcn) return;
	}

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].ingred) continue;
		data->fcn[n].ingred = fcn;
		return;
	}

	data->fcn = MoreMem(data->fcn, CFTU, data->nfcn+1);
	data->fcn[data->nfcn].ingred = fcn;
	data->nfcn++;
}


void DeleteIngredObserver (void (*fcn)(CAL,String*,int))
{
	int n;
	OBDS *data = get_data(INGRED_ID);

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].ingred == fcn)
			data->fcn[n].ingred = NULL;
	}
}


void NotifyIngredObservers (CAL cal, String *parms, int nparms)
{
	int n;
	OBDS *data = get_data(INGRED_ID);

	for(n = 0; n < data->nfcn; n++)
	{
		if(data->fcn[n].ingred)
			data->fcn[n].ingred(cal, parms, nparms);
	}
}


void AddSourceObserver (Boolean (*fcn)(Boolean), String label )
{
	int n;
	OBDS *data = get_data(SOURCE_ID);

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].source == fcn) return;
	}

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].source) continue;
		data->fcn[n].source = fcn;
		data->label[n] = XtNewString(label);
		return;
	}

	data->fcn = MoreMem(data->fcn, CFTU, data->nfcn+1);
	data->fcn[data->nfcn].source = fcn;
	data->label = MoreStringArray(data->label, data->nfcn+1);
	data->label[data->nfcn] = XtNewString(label);
	data->nfcn++;
}


void DeleteSourceObserver (Boolean (*fcn)(Boolean))
{
	int n;
	OBDS *data = get_data(SOURCE_ID);

	for( n = 0; n < data->nfcn; n++ )
	{
		if(data->fcn[n].source == fcn)
		{
			data->fcn[n].source = NULL;
			XtFree(data->label[n]);
		}
	}
}


/* The registered functions are called in order in a work procedure to keep
 * busy times to a minimum. If the return from the notification function is
 * False, the function will be called again until the return is True. This
 * in essence puts the notification function in it own work procedure. The
 * number of times this will loop has been limited in case an endless loop
 * happens. I figured that 1000 iterations probably constitutes such a
 * situation.
 */
static Boolean source_wp( XtPointer client_data )
{
	OBDS  *data;
	OBSNS *obds = (OBSNS *) client_data;

	if(!obds) return True;

	data = get_data(SOURCE_ID);

	if(obds->fndx < data->nfcn)
	{
		if(data->fcn[obds->fndx].source)
		{
			if(obds->count < 1000)
			{
				obds->count++;
				if(!data->fcn[obds->fndx].source(obds->state))
					return False;
			}
			else
			{
				pr_error("NotifySourceObserver","%s - Source notification function called 1000 times.\n",
					data->label[obds->fndx]);
			}
		}
		obds->fndx++;
		obds->count = 0;
		return False;
	}
	else
	{
		if (obds->fcn) obds->fcn();
		FreeItem(obds);
		return True;
	}
}


/* Send a notification to all of the functions registered. The parameter
 * fcn will be called after all of the registered observers have been
 * notified. The notifying procedure can then take action if required.
 */
void NotifySourceObservers (void (*fcn)(void), Boolean state)
{
	OBSNS *obds  = OneMem(OBSNS);
	obds->count  = 0;
	obds->fndx   = 0;
	obds->fcn    = fcn;
	obds->state  = state;
	(void) XtAppAddWorkProc(GV_app_context, source_wp, (XtPointer) obds);
}
