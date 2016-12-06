/************************************************************************
*
*  File:     XrmUpdate.c
*
*  Function: XuXrmUpdate
*
*  Purpose:  A function to re-apply resources contained in a resource
*            file to the application opened with this library
*            (XuVaAppInitialize).
*
*
*  This function lets you change resources 'on the fly'. To apply
*  changes make the call:
*
*              XuXrmUpdate("a_resource_file");
*
*  The function will read and reapply resource files to existing widget
*  heirarchies. It will add new resources and change existing ones to
*  their new values. If the given file is an absolute path it will be
*  used as given. If not then XuFindFile is used and the file is
*  searched for.
*
*  A few uses for this function are:
*
*    - create a UI with 2 langages labels
*    - to create any style of palette usable with your User Interface.
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
*
************************************************************************/
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include "XuP.h"

#define QLMINELEMS	2		/* minimum number of elements in quark array */
#define ARGSMAX		200		/* maximum number of resource arguments we can handle */
#define	NLEVELSMAX	20		/* Maximum number of levels in the name */
#define SLMINSIZE	16		/* minimin size for hash table */
#define SLINC		64		/* level multiplier for hash table */
#define SLMAXSIZE	1024	/* hash table maximum size */

#define QName(w)	w->core.xrm_name	/* the widget name as a quark */


/* Forward function declaration */
static void update_application_resources(Widget w, XrmDatabase rDB);

const String MyName = "XuXrmUpdate";


/************* PUBLIC FUNCTION *****************/


void XuXrmUpdate(String infile)
{
	String      file      = NULL;
	XrmDatabase	rDB       = (XrmDatabase)0;
	XrmDatabase defaultDB = (XrmDatabase)0;

	file = XuFindFile(infile);
	if(!file) return;

	rDB = XrmGetFileDatabase(file);
	XtFree(file);
	if(!rDB) return;

	update_application_resources(Fxu.top_level, rDB);
	defaultDB = XtDatabase(DefaultAppDisplayInfo->display);
	XrmMergeDatabases(rDB, &defaultDB);
}



/********** LOCAL INTERNAL FUNCTIONS ********************/


static void array_error(void)
{
	XtWarningMsg( "arrayError", MyName, "Functions", "Quark table is too small", NULL, 0);
}


/* Return the "complete name" for a widget in the form of a list
 * of quarks.
 */
static XrmQuarkList q_full_name(Widget w, XrmQuarkList ql, int nb)
{
	if (nb >= QLMINELEMS)
	{
		if (XtParent(w))
		{
			ql = q_full_name(XtParent(w), ql, nb - 1);
		}
		*ql = QName(w);
		return(ql + 1);
	}
	else
	{
		array_error();
		return(ql);
	}
}

static void quark_full_name(Widget w, XrmQuarkList ql, int nb)
{
	if (nb >= QLMINELEMS)
	{
		if (XtParent(w))
		{
			ql = q_full_name(XtParent(w), ql, nb - 1);
		}
		*ql = QName(w);
		*(ql + 1) = NULLQUARK;
	}
	else
	{
		array_error();
	}
}



/* Return the class name of a widget as a quark
 */
static XrmQuark quark_class_name(Widget w)
{
	WidgetClass	wc;
	XrmQuark	result;

	if (XtParent(w))
	{
		wc = XtClass(w);
		result = (wc->core_class).xrm_class;
	}
	else
	{
		String	class, name;

		XtGetApplicationNameAndClass( XtDisplay(w), &name, &class);
		result = XrmStringToQuark(class);
	}

	return(result);
}


		
/* Return the "complete class name" for a widget in the form of a list
 * of quarks.
 */
static XrmQuarkList q_class_full_name(Widget w, XrmQuarkList ql, int nb)
{
	if (nb >= QLMINELEMS)
	{
		if (XtParent(w))
		{
			ql = q_class_full_name(XtParent(w), ql, nb - 1);
		}
		*ql = quark_class_name(w);
		return(ql + 1);
	}
	else
	{
		array_error();
		return(ql);
	}
}

void quark_class_full_name(Widget w, XrmQuarkList ql, int nb)
{
	if (nb >= QLMINELEMS)
	{
		if (XtParent(w))
		{
			ql = q_class_full_name(XtParent(w), ql, nb - 1);
		}
		*ql = quark_class_name(w);
		*(ql + 1) = NULLQUARK;
	}
	else
	{
		array_error();
	}
}


/*  Very specialized function to allocate memory.
 */
static XrmHashTable *q_list_realloc(XrmHashTable **psl, int *p_nelems)
{
	int		newSize;

	static int	level = 0;

	*p_nelems = SLMINSIZE + level * SLINC;

	newSize = (*p_nelems) * sizeof(XrmHashTable);

	if (newSize > (SLMAXSIZE * sizeof(XrmHashTable)))
	{
		XtErrorMsg( "allocError", MyName, "Functions", "Memory allocation failure", NULL, 0);
	}
	else
	{
		level++;
		return((XrmHashTable *) XtRealloc((void *)(*psl), newSize));
	}
}


/*  This is a function to correctly use the XrmValue with XtSetArg.
 *  The problem is that the size of the XrmValue depends on the 
 *  size of the contents of the address. Thus the following.
 */
static int set_arg(ArgList p_args, int nargs, XtResource resource, XrmValue xrmv)
{
	int      result = nargs;
	XtArgVal value;	
	
	if (xrmv.addr != NULL)
	{ 
		if (strcmp(resource.resource_type, XtRString) == 0)
		{
			value = (XtArgVal)(xrmv.addr);
		}
		else
		{
			switch(xrmv.size)
			{
		    	case sizeof(char):
					value = (XtArgVal)(* (char *) xrmv.addr);
					break;
	
	 	   		case sizeof(short):
					value = (XtArgVal)(* (short *) xrmv.addr);
					break;

				case sizeof(long):
					value = (XtArgVal)(* (long *) xrmv.addr);
					break;

				default:
					value = (XtArgVal)(xrmv.addr);
					break;
			}

		}
		XtSetArg( p_args[nargs], resource.resource_name, value);
		result = nargs + 1;
	}
	return(result);
}
				

/* Interrogate the given base for the widget resource by using quarks.
 */
static Bool get_search_resource(XrmSearchList sl, XtResource r, String *p_rtype, XrmValuePtr p_rvalue)
{
	XrmQuark	resourceQName, resourceQClassName, resourceQDBType;
	Bool 		result;

	(void) memset((void *)p_rvalue, 0, sizeof(XrmValue));

	/* We need the complete name and class as quarks */
	resourceQName = XrmStringToQuark(r.resource_name);
	resourceQClassName = XrmStringToQuark(r.resource_class);

	/* Search the given base */
	result = XrmQGetSearchResource( sl, resourceQName, resourceQClassName,
									&resourceQDBType, p_rvalue);

	*p_rtype = (String) XrmQuarkToString(resourceQDBType);

	return(result);
}


/* Search the existing resources from the given widget base. The table of
 * arguments returned is directly used by XtSetValues. The search uses
 * quarks for better performance.
 */
static void search_database(Widget w, XrmDatabase rDB, ArgList p_args, int *p_nargs)
{
	XrmValue       resourceDBValue, resourceValue;
	XtResourceList rl;
	String		   resourceDBType;
	XrmQuark	   widgetQFullName[NLEVELSMAX + 1];
	XrmQuark       widgetQClassFullName[NLEVELSMAX + 1];
	XrmHashTable   *searchList;
	int		       nSLElems = SLMINSIZE;
	Cardinal       n;

	*p_nargs = 0;

	/* Get the resource list for the widget */
	XtGetResourceList( XtClass(w), &rl, &n);

	/* obtain the identification quraks */
	quark_full_name(w, widgetQFullName, NLEVELSMAX + 1);
	quark_class_full_name(w, widgetQClassFullName, NLEVELSMAX + 1);

	/* Search the list from the base */
	searchList = (XrmHashTable *) XtMalloc( nSLElems * sizeof(XrmHashTable));

	while (!XrmQGetSearchList( rDB, widgetQFullName, widgetQClassFullName, searchList, nSLElems))
	{
		searchList = q_list_realloc(&searchList, &nSLElems);
	}

	/* For each instance of the resources we must see if it necessary to
	 * interrogate the resource manager.
	 */
	while(n --)
	{
		if (get_search_resource( searchList, rl[n], &resourceDBType, &resourceDBValue))
		{
			/* Type conversion */
			XtConvert( w, resourceDBType, &resourceDBValue, rl[n].resource_type, &resourceValue);

			/* Construct argument table */
			*p_nargs = set_arg( p_args, *p_nargs, rl[n], resourceValue);
		}
	}

	XtFree((void *)searchList);
	XtFree((void *)rl);
}


/* Recursive update of the widget resources
 */
static void update_application_resources(Widget w, XrmDatabase rDB)
{
	WidgetList  widgets    = (WidgetList)0;
	int         numWidgets = 0;
	Arg         args[ARGSMAX];
	int         numArgs = 0;

	if (XtIsComposite(w))
	{
		numWidgets = (((CompositeWidget)w)->composite).num_children;
		widgets    = (((CompositeWidget)w)->composite).children;

		while(numWidgets--)
		{
			update_application_resources( widgets[numWidgets], rDB);
		}
	}

	widgets    = (WidgetList)0;
	numWidgets = 0;

	if (XtIsWidget(w))
	{
		numWidgets = (w -> core).num_popups;
		widgets    = (w -> core).popup_list;

		while(numWidgets--)
		{
			update_application_resources( widgets[numWidgets], rDB);
		}
	}

	search_database(w, rDB, args, &numArgs);

	if (numArgs) XtSetValues(w, args, numArgs);
}
