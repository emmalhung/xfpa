/************************************************************************************/
/*
 * File:    field_autoImport.c
 *
 * Purpose: Checks the autoimport directory and determines if there are any
 *          fields to import. This can be run either from the automatic source
 *          update checking system or manually. If from the auto system then
 *          the autocheck parameter must be True. Set False for manual run.
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
/************************************************************************************/
#include <sys/stat.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include "global.h"
#include "editor.h"
#include "source.h"
#include "observer.h"

/* The interval at which to rescan the import directory in milliseconds.
 * Only used when in polling mode.
 */
#define RESCAN_TIME	15000

/* Structure to hold information on every import source specified
 * in the setup file. Note the the autoimport source is special
 * and will always appear.
 */
typedef struct {
	Source   src;					/* The import source */
	Boolean  modified;				/* Used when import_pending is true */
	Boolean  import_without_asking;	/* Import the field without asking permission? */
	Boolean  create_without_asking;	/* Create a depiction if necessary without permission? */
	int      nfile;					/* Number of files to import */
	String  *flist;					/* The file list */
} SRCDATA;

static int      nsrcdata  = 0;
static SRCDATA *srcdata   = NULL;
static Widget   dialog    = NULL;
static Widget   acceptBtn = NULL;
static Widget   *forms    = NULL;
static Widget   **rc      = NULL;
static String   module    = "AutoImport";
static Boolean  interrupt_allowed = True;	/* Can ingred be interrupted? */
static Boolean  import_pending    = False;	/* Import once interrupt allowed? */


/* Get a directory file list and filter out any directories found. There
 * is no rule saying that out cannot put other import directory sources
 * in the main import directory. If a given file name cannot be parsed then
 * complain and remove it from the directory, otherwise files could accumulate
 * forever if illegal files are copied in. It is the responsibility of the
 * calling function to free the returned list if required.
 */
static int file_list(SRCDATA *sd, String **rtnlist)
{
	int    i, nlist, nf;
	String *fl, *list;

	nlist = 0;
	if (rtnlist) *rtnlist = NULL;

	nf = dirlist(sd->src->dir, NULL, &fl);
	if(nf > 0)
	{
		if (rtnlist) list = NewMem(String, nf);
		for(i = 0; i < nf; i++ )
		{
			struct stat stbuf;
			if(stat(pathname(sd->src->dir,fl[i]), &stbuf) != 0) continue;
			if(S_ISDIR(stbuf.st_mode)) continue;
			if(!parse_file_identifier(fl[i], NULL, NULL, NULL))
			{
				pr_warning(module, "Deleting unrecognized file \"%s\" in auto import directory \"%s\"\n", fl[i], sd->src->dir);	
				(void) unlink(pathname(sd->src->dir,fl[i]));
				continue;
			}
			if (rtnlist) list[nlist] = XtNewString(fl[i]);
			nlist++;
		}
		if (rtnlist)
		{
			if( nlist > 0) *rtnlist = list;
			else           FreeItem(list);
		}
	}
	return nlist;
}


/* Function to actually do the field import from a given source.
 */
static void action_import(SRCDATA *key)
{
	int     n;
	String  parm[1] = {NULL};
	Boolean doall = key->create_without_asking;

	if(key->flist == NULL || key->nfile < 1) return;

	/* Tell ingred to get the files then delete from the directory. */
	for( n = 0; n < key->nfile; n++ )
	{
		String vtime;
		FpaConfigElementStruct *edef;
		FpaConfigLevelStruct   *ldef;
		
		/* The file name has passed through the parser already by this point
		 * but putting in some brackets is easy ;-)
		 */
		if(parse_file_identifier(key->flist[n], &edef, &ldef, &vtime))
		{
			/* The CreateDepiction function will only create a depiction if
			 * one does not already exist at the required time.
			 */
			if(edef->elem_tdep->time_dep != FpaC_NORMAL || DepictionAtTime(vtime))
			{
				if(IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s - - %s %s %s",
					SrcName(key->src), edef->name, ldef->name, vtime))
				{
					AddField(identify_field( edef->name, ldef->name ), False);
				}
			}
			else
			{
				int response = -1;
				if(!doall)
				{
					response = XuMakeActionRequest(GW_mainWindow, XuYYAN,
							"AutoImportCreateDepiction", DateString(vtime,MINUTES), NULL);
					if(response == XuYES_ALL) doall = True;
				}
				if(doall || response == XuYES)
				{
					if(CreateDepiction(vtime))
					{
						if(IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s - - %s %s %s",
							SrcName(key->src), edef->name, ldef->name, vtime))
						{
							AddField(identify_field( edef->name, ldef->name ), False);
						}
					}
				}
			}
		}
		(void)unlink(pathname( key->src->dir, key->flist[n] ));
	}
	FreeList(key->flist, key->nfile);
	key->nfile = 0;

	/* Let whoever wants to know that some field is available */
	NotifyObservers(OB_FIELD_AVAILABLE, parm, 1);
}


/*ARGSUSED*/
static void import_cb( Widget w, XtPointer client_data, XtPointer unused )
{
	int n;

	if (!dialog) return;

	/* BuxFix 2005.09.26: If an import is not wanted free the list */
	if(!client_data)
	{
		for(n = 0; n < nsrcdata; n++)
		{
			FreeList(srcdata[n].flist, srcdata[n].nfile);
			srcdata[n].nfile = 0;
		}
	}
	else
	{
		DeactivateMenu();
		for(n = 0; n < nsrcdata; n++)
		{
			action_import(srcdata+n);
		}
		ActivateMenu();
	}
	XuDestroyDialog(dialog);
	dialog = (Widget)NULL;
}


/* Display the files available for import
 */
static void display_files(void)
{
	int      n, ns;
	Boolean  have_some;
	
	/* Only proceed if the dialog exists. */
	if (!dialog) return;

	for(have_some = False, ns = 0; ns < nsrcdata; ns++)
	{
		Boolean have_one = False;

		XtUnmanageChildren(rc[ns],3);

		/* Destroy any existing label children */
		for(n = 0; n < 3; n++)
		{
			int        i, nchild;
			WidgetList children;
			
			XtVaGetValues(rc[ns][n], XtNchildren, &children, XtNnumChildren, &nchild, NULL);
			for(i = 0; i < nchild; i++) XtDestroyWidget(children[i]);
		}
		XuDelay(dialog, 50);

		/* Only create a new list if we don't have one already */
		if (!srcdata[ns].flist)
			srcdata[ns].nfile = file_list(&srcdata[ns], &srcdata[ns].flist);
		
		for( n = 0; n < srcdata[ns].nfile; n++ )
		{
			FpaConfigElementStruct *edef;
			FpaConfigLevelStruct   *ldef;
			String vtime;
			
			if(!parse_file_identifier(srcdata[ns].flist[n], &edef, &ldef, &vtime)) continue;

			have_one = True;
			(void)XmVaCreateManagedLabel(rc[ns][0], ldef->label, NULL);
			(void)XmVaCreateManagedLabel(rc[ns][1], edef->label, NULL);
			(void)XmVaCreateManagedLabel(rc[ns][2], DateString(vtime,MINUTES), NULL);
		}

		if (have_one)
			have_some = True;
		else
			(void)XmVaCreateManagedLabel(rc[ns][0], "msgNone", NULL);

		XtManageChildren(rc[ns],3);
		XtManageChild(forms[ns]);
	}
	XtSetSensitive(acceptBtn, have_some);
}


/* Check to see if the directory has been modified. If so we reconstitute and redisplay the
*  file list. The function will only be active while the dialog exists. During this time it
*  puts itself back into the timer list at RESCAN_TIME intervals which is much shorter than
*  the general source check interval. Only used in polling mode.
*/
static void check_directory( XtPointer client_data, XtIntervalId *id )
{
	int         ns;
	Boolean     update;
	struct stat stbuf;
	
	/* Only proceed if the dialog is still active. */
	if (!dialog) return;

	for(update = False, ns = 0; ns < nsrcdata; ns++)
	{
		/* BugFix 2005.09.26: If flist is not NULL then we already have an
		 * active list and we do not want to generate a new list if the old
		 * list is still being processed.
		 */
		if (srcdata[ns].flist) continue;

		/* Paranoid check to ensure the directory is valid. */
		if(stat(srcdata[ns].src->dir, &stbuf) != 0)
		{
			(void)XmVaCreateManagedLabel(rc[ns][0], "msgNone", NULL);
			XtManageChild(forms[ns]);
			continue;
		}

		/* Do nothing if the directory mod time is the same. */
		if(NotNull(id) && srcdata[ns].src->last_mod_time == stbuf.st_mtime) continue;

		srcdata[ns].src->last_mod_time = stbuf.st_mtime;

		update = True;
	}
	if (update) display_files();

	(void)XtAppAddTimeOut(GV_app_context, RESCAN_TIME, check_directory, client_data);	
}


/* See note above for the parameter settings.
*/
static void create_dialog( void )
{
	int     n;
	Widget  sw, label, swform;	
	
	static XuDialogActionsStruct action_items[] = {
		{"acceptBtn", import_cb, (XtPointer)True },
		{"cancelBtn", import_cb, (XtPointer)False}
	};

	if (!dialog)
	{
		dialog = XuCreateFormDialog(GW_mainWindow, "autoImport",
			XuNmwmDeleteOverride, import_cb,
			XuNdestroyData, 0,
			XuNactionAreaItems, action_items,
			XuNnumActionAreaItems, XtNumber(action_items),
			XmNhorizontalSpacing, 9,
			XmNverticalSpacing, 9,
			NULL);

		acceptBtn = XuGetActionAreaBtn(dialog, action_items[0]);

		label = XmVaCreateManagedLabel(dialog, "msgTop",
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		sw = XmVaCreateManagedScrolledWindow(dialog, "sw",
			XmNscrollingPolicy, XmAUTOMATIC,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);

		swform = XmVaCreateManagedForm(sw, "swform",
			NULL);

		for(n = 0; n < nsrcdata; n++)
		{
			forms[n] = XmVaCreateForm(swform, "form",
				XmNtopAttachment, (n)? XmATTACH_WIDGET:XmATTACH_FORM,
				XmNtopWidget, (n)? forms[n-1]:swform,
				XmNtopOffset, (n)? 19:5,
				XmNleftAttachment, XmATTACH_FORM,
				NULL);

			label = XmVaCreateManagedLabel(forms[n], SrcShortLabel(srcdata[n].src),
				XmNtopAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_FORM,
				NULL);

			rc[n][0] = XmVaCreateManagedRowColumn(forms[n], "rc1",
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, label,
				XmNtopOffset, 3,
				XmNleftAttachment, XmATTACH_FORM,
				XmNleftOffset, 5,
				NULL);

			rc[n][1] = XmVaCreateManagedRowColumn(forms[n], "rc2",
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopOffset, 3,
				XmNtopWidget, label,
				XmNleftAttachment, XmATTACH_WIDGET,
				XmNleftWidget, rc[n][0],
				XmNleftOffset, 5,
				NULL);

			rc[n][2] = XmVaCreateManagedRowColumn(forms[n], "rc3",
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopOffset, 3,
				XmNtopWidget, label,
				XmNleftAttachment, XmATTACH_WIDGET,
				XmNleftWidget, rc[n][1],
				XmNleftOffset, 5,
				NULL);	
		}
	}
	if (GV_inotify_process_used)
	{
		display_files();
	}
	else
	{
		check_directory( NULL, NULL );
	}
	XuShowDialog(dialog);	
}


/* Function called to actually scan the import data directories and do the import.
 * Note that this is done as a work procedure to allow any pending events to be
 * processed first. Could make for a better "feel".
 */
/*ARGSUSED*/
static Boolean import_scan(XtPointer unused)
{
	int i;
	Boolean need_dialog = False;
	Boolean import_without_asking = False;

	/* Just to be paranoid unset this flag here. */
	import_pending = False;

	/* Scan the AutoImport sources to see if any have the import without
	 * asking flag set.
	 */
	for( i = 0; i < nsrcdata; i++)
	{
		if(srcdata[i].modified && srcdata[i].import_without_asking)
			import_without_asking = True;
	}

	if (import_without_asking) DeactivateMenu();

	for(i = 0; i < nsrcdata; i++)
	{
		if(srcdata[i].modified)
		{
			if(srcdata[i].import_without_asking)
			{
				srcdata[i].nfile = file_list(&srcdata[i], &srcdata[i].flist);
				action_import(srcdata+i);
			}
			else
			{
				/* Bug 20080325: Final check to be sure we have at least one
				 * file as dialog was popping up with no files to import
				 */
				if(file_list(&srcdata[i], NULL) > 0)
					need_dialog = True;
			}
			srcdata[i].modified = False;
		}
	}

	if (import_without_asking)  ActivateMenu();
	if (need_dialog) create_dialog();

	return True;
}


/* Check the returns from the source modification scans and see if any
 * data has arrived. Note that the srcdata value modified is set here
 * as if the import is to be delayed until ingred give us permission
 * the modified flag associated with the source may have been reset and
 * we need to save this information ourselves.
 */
/*ARGSUSED*/
static Boolean check_autoimport_directory(Boolean noused)
{
	if(dialog)
	{
		if (GV_inotify_process_used)
		{
			display_files();
		}
		XuShowDialog(dialog);
	}
	else
	{
		int i;
		Boolean something_to_import = False;

		for( i = 0; i < nsrcdata; i++)
		{
			if( srcdata[i].src->modified && srcdata[i].src->isdata )
			{
				something_to_import = True;
				srcdata[i].modified = True;
			}
		}
		if (something_to_import)
		{
			if(!(import_pending = !interrupt_allowed))
				(void) XtAppAddWorkProc(GV_app_context, import_scan, (XtPointer)NULL);
		}
	}
	return True;
}


/*ARGSUSED*/
static void ingred_message(CAL cal, String *parms, int nparms)
{
	if(nparms > 2 && same_ic(parms[0],"INTERRUPT"))
	{
		interrupt_allowed = same_ic(parms[1],"ON");
		if(interrupt_allowed && import_pending)
		{
			import_pending = False;
			(void) XtAppAddWorkProc(GV_app_context, import_scan, (XtPointer)NULL);
		}
	}
}


/*================= Public Functions ==================*/


/*ARGSUSED*/
void ACTIVATE_autoImportDialog(Widget w)
{
	if (dialog)
	{
		if (GV_inotify_process_used)
		{
			display_files();
		}
		XuShowDialog(dialog);
	}
	else
	{
		create_dialog();
	}
}


void InitAutoImportSystem(void)
{
	int        n;
	SourceList src;

	SourceListByType(SRC_IMPORT, FpaC_TIMEDEP_ANY, &src, &nsrcdata);

	/* Allocate memory for the widgets used in the dialog to display
	 * information about the fields to import and for the source data.
	 */
	srcdata = NewMem(SRCDATA, nsrcdata);
	forms = (Widget *) NewMem(Widget, nsrcdata);
	rc = (Widget **) NewMem(Widget *, nsrcdata);
	for(n = 0; n < nsrcdata; n++)
		rc[n] = (Widget *) NewMem(Widget, 3);

	/* Set the import flags from the setup file data which is referenced
	 * by the parms variable in the Source struct.
	 */
	for(n = 0; n < nsrcdata; n++)
	{
		srcdata[n].src = src[n];
		if(!blank(src[n]->parms[0]))
			srcdata[n].import_without_asking = (strchr("TtYy",*src[n]->parms[0]) != NULL);
		if(!blank(src[n]->parms[1]))
			srcdata[n].create_without_asking = (strchr("TtYy",*src[n]->parms[1]) != NULL);
	}


	AddSourceObserver(check_autoimport_directory,"AutoImport");
	AddIngredObserver(ingred_message);
}
