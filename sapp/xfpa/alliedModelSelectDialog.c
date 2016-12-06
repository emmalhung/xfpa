/*========================================================================*/
/*
*	File:		alliedModelSelectDialog.c
*
*	Purpose:	Functions used for selecting and launching allied models.
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
#include <Xm/Column.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include "depiction.h"
#include "productStatus.h"
#include "resourceDefines.h"
#include "fpapm.h"
#include "alliedModel.h"
#include "editor.h"
#include "help.h"
#include "observer.h"

/* Forward function declarations */
static void    auto_import_model_fields (AlliedModelStruct *);
static void    close_cb (Widget, XtPointer, XtPointer);
static Boolean check_run_fields_ok (Source, String);
static void    model_run_cb (Widget, XtPointer, XtPointer);
static void    run_models (XtPointer, XtIntervalId*);

/* Local static variables */
static Widget  dialog = NULL;
static Pixmap  pixmaps[8];
static Boolean something_running = False;


/* Public function */
void ACTIVATE_alliedModelSelectDialog( Widget ref_widget )
{
	int i;
	Widget listManager;

	static XuDialogActionsStruct action_items[] = {
		{"runBtn",    model_run_cb, NULL},
		{"closeBtn",  close_cb,     NULL},
		{"helpBtn",   HelpCB,       HELP_ALLIED_MODEL_SELECT}
	};

	if (dialog) return;

	dialog = XuCreateFormDialog(ref_widget, "alliedModelSelect",
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	for(i = 0; i < 8; i++)
	{
		char buf[32];
		snprintf(buf, 32, "rotatingArrow-20x20-%d", i);
		pixmaps[i] = XuGetPixmap(dialog, buf);
	}

	listManager = XmVaCreateColumn(dialog, "modelBtns",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < GV_nallied_model; i++)
	{
		GV_allied_model[i].sel_btn = XmVaCreateManagedToggleButton(listManager,
			SrcLabel(GV_allied_model[i].source),
			XmNsensitive, !GV_allied_model[i].running,
			XmNentryLabelType, XmPIXMAP,
			XmNentryLabelPixmap, XmUNSPECIFIED_PIXMAP,
			NULL);
	}
	XtManageChild(listManager);
	XuShowDialog(dialog);
}

/* Private functions */

/* Put up a pixmap of rotating arrows next to any selected model that is currently
 * running and cycle through them to give the illusion of rotating arrows. This
 * will be a positive indicator of which model is running.
 */
static void cycle_busy_indicator (XtPointer client_data, XtIntervalId *id)
{
	int i;

	static Boolean indicator_running = False;
	static int cycle_count = 0;

	/* Just in case dialog exits while we are looping */
	if (!dialog) return;

	/* If nothing running reset everything just to be safe */
	if (!something_running)
	{
		indicator_running = False;
		cycle_count = 0;
		for(i = 0; i < GV_nallied_model; i++)
		{
			XtSetSensitive(GV_allied_model[i].sel_btn, True);
			XtVaSetValues(GV_allied_model[i].sel_btn, XmNentryLabelPixmap, XmUNSPECIFIED_PIXMAP, NULL);
		}
	}
	/* Function called by time out procedure */
	else if (id)
	{
		for(i = 0; i < GV_nallied_model; i++)
		{
			XtVaSetValues(GV_allied_model[i].sel_btn,
				XmNentryLabelPixmap, (GV_allied_model[i].running)? pixmaps[cycle_count] : XmUNSPECIFIED_PIXMAP,
				NULL);
			XtSetSensitive(GV_allied_model[i].sel_btn, !GV_allied_model[i].running);
		}
		cycle_count = (cycle_count+1)%8;
		(void)XtAppAddTimeOut(GV_app_context, 250, cycle_busy_indicator, NULL);
	}
	/* Function called from function in this file */
	else if (!indicator_running)
	{
		(void) XtAppAddTimeOut(GV_app_context, 0, cycle_busy_indicator, NULL);
		indicator_running = True;
	}
}


/* Run the selected allied models */
/*ARGSUSED*/
static void model_run_cb( Widget w , XtPointer client_data , XtPointer unused )
{
	int i, rtn;
	char fld_list[512];

	/* Check for pending edits and as permission to run if there is one */
	if(XtIsSensitive(GW_editorAcceptBtn))
	{
		if(XuAskUser(w, "pending_edit", NULL) == XuNO) return;
	}

	for(i = 0; i < GV_nallied_model; i++)
	{
		if(GV_allied_model[i].running) continue;
		GV_allied_model[i].selected = XmToggleButtonGetState(GV_allied_model[i].sel_btn);
		if(!GV_allied_model[i].selected) continue;
		if(!check_run_fields_ok(GV_allied_model[i].source, fld_list))
		{
			if(blank(fld_list))
			{
				XuShowError(w, "alliedModelError", SrcLabel(GV_allied_model[i].source));
				GV_allied_model[i].selected = False;
			}
			else
			{
				rtn = XuAskUser(w, "alliedModelNoField", SrcLabel(GV_allied_model[i].source), fld_list);
				GV_allied_model[i].selected = (rtn == XuYES);
			}
		}
	}

	/* Run the models with a delayed call.  This gives Ingred a chance to clean up
	 * before the models are run and looks better to the user.
	*/
	(void)XtAppAddTimeOut(GV_app_context, 400, run_models, NULL);
}


/* Called when notification is received of status information from a model run or that
 * a model run has ended. This notification comes via the RunProgramManager() function
 * (see run_models) which is responsible for running the process.
 */
static void model_run_ending( XtPointer client_data , int status_key , String status )
{
	int i;
	char mbuf[300];
	AlliedModelStruct *model = (AlliedModelStruct *)client_data;

	switch(status_key)
	{
		case XuRUN_STATUS:
			if(same_ic("start", string_arg(status)))
			{
				(void)strcpy(mbuf, XuGetLabel(string_arg(status)));
				(void)strcat(mbuf, " ");
				(void)strcat(mbuf, XuGetLabel("running"));
				ProductStatusUpdateInfo(model->product_key, PS_UPDATE, mbuf);
			}
			else
			{
				ProductStatusUpdateInfo(model->product_key, PS_UPDATE, status);
			}
			break;

		case XuRUN_ENDED:
			if(model->automatic_import)
			{
				ProductStatusUpdateInfo(model->product_key, PS_ENDED, NULL);
				if(GV_pref.ask_to_import_allied_models)
					ACTIVATE_alliedModelImportPermissionDialog(model);
				else
					auto_import_model_fields(model);
			}
			else
			{
				ProductStatusUpdateInfo(model->product_key, PS_ENDED, NULL);
			}
			model->running = False;
			break;

		case XuRUN_ERROR:
			model->running = False;
			(void)strcpy(mbuf, XuGetLabel(status));
			(void)strcat(mbuf, " ");
			(void)strcat(mbuf, XuGetLabel("abort"));
			ProductStatusUpdateInfo(model->product_key, PS_ERROR, mbuf);
			XuShowError(GW_mainWindow, "alliedAbort", SrcLabel(model->source));
			break;
	}

	something_running = False;
	for(i = 0; i < GV_nallied_model; i++)
		if(GV_allied_model[i].running) something_running = True;
}


/* Function to substitute values in the allied model processing strings.
 *  The keywords are expected to be between angle brackets "<keyword>" and
 *  should be in upper case, although this function does not care about
 *  case. The recognized keywords are:
 *
 *    <SETUP>     - setup of the database to use
 *    <SOURCE>    - allied model source
 *    <SUBSOURCE> - allied model sub-source
 *    <ELEMENT>   - element
 *    <LEVEL>     - level
 *    <RTIME>     - run time of the model (T0).
 *    <VTIME>     - valid time of the model
 */
static String macro_sub( Source src, int process_ndx )
{
	size_t n;
	char   mbuf[1000];
	String p, pb, ps;

	if (IsNull(src->fd->sdef->allied)) return (String)NULL;

	switch(process_ndx)
	{
		case 0: ps = src->fd->sdef->allied->pre_process;  break;
		case 1: ps = src->fd->sdef->allied->process;      break;
		case 2: ps = src->fd->sdef->allied->post_process; break;
	}

	if(blank(ps)) return (String)NULL;

	ZeroBuffer(mbuf);

	while(NotNull(pb = strchr(ps,'<')))
	{
		/* Is the angle bracket escaped? If so ignore and continue.
		*/
		if( pb > ps && *(pb-1) == '\\' )
		{
			ps = pb+1;
			continue;
		}

		/* Is there and ending angle bracket? If not stop processing. */
		if(IsNull(strchr(pb,'>'))) break;

		/* Copy everything before the '<' */
		(void)strncat(mbuf, ps, (size_t)(pb - ps));

		/* Find the keyword embedded between the angle brackets. We allow
		*  for the case where there is leading and trailing white space.
		*/
		p  = pb + 1;
		p  = p + strspn(p," \t\n\r\f");
		n  = strcspn(p," >\t\n\r\f");
		ps = strchr(p,'>') + 1;

		if(strncasecmp(p,"SETUP",n) == 0)
		{
			(void)strcat(mbuf, GetSetupFile(0,NULL));
		}
		else if(strncasecmp(p,"SOURCE",n) == 0)
		{
			(void)strcat(mbuf, SrcName(src));
		}
		else if(strncasecmp(p,"SUBSOURCE",n) == 0)
		{
			if(blank(SrcSubName(src)))
				(void)strcat(mbuf, "\"\"");
			else
				(void)strcat(mbuf, SrcSubName(src));
		}
		else if(strncasecmp(p,"ELEMENT",n) == 0)
		{
			(void) strcat(mbuf, GV_active_field->info->element->name);
		}
		else if(strncasecmp(p,"LEVEL",n) == 0)
		{
			(void) strcat(mbuf, GV_active_field->info->level->name);
		}
		else if(strncasecmp(p,"RTIME",n) == 0)
		{
			(void)strcat(mbuf, GV_T0_depict);
		}
		else if(strncasecmp(p,"VTIME",n) == 0)
		{
			(void) strcat(mbuf, ActiveDepictionTime(FIELD_DEPENDENT));
		}
		else
		{
			(void)strncat(mbuf, pb, (size_t) (ps-pb));
		}
	}
	(void)strcat(mbuf, ps);
	return XtNewString(mbuf);
}


/*ARGSUSED*/
static void run_models( XtPointer client_data , XtIntervalId *id )
{
	int    i, ac;
	String pre_process, process, post_process;
	Arg    al[5];

	for( i = 0; i < GV_nallied_model; i++)
	{
		if(!GV_allied_model[i].selected) continue;

		/* 2003/04/08 - Deselect the model no matter what its run state.
		 */
		GV_allied_model[i].selected = False;

		if(GV_allied_model[i].running) continue;

		pre_process  = macro_sub( GV_allied_model[i].source, 0 );
		process      = macro_sub( GV_allied_model[i].source, 1 );
		post_process = macro_sub( GV_allied_model[i].source, 2 );

		XtSetArg(al[0], PmNprogram, PmALLIED_MODEL);

		ac = 1;
		if(NotNull(pre_process))  { XtSetArg(al[ac], PmNpreProcess,  pre_process ); ac++; }
		if(NotNull(process))      { XtSetArg(al[ac], PmNprocess,     process     ); ac++; }
		if(NotNull(post_process)) { XtSetArg(al[ac], PmNpostProcess, post_process); ac++; }

		/* For backward compatability, if none of the process strings exists
		*  we will assume the old allied model run procedures.
		*/
		if( ac == 1 )
		{
			XtSetArg(al[ac], PmNsourceType, "source"); ac++;
			XtSetArg(al[ac], PmNmodel, SrcName(GV_allied_model[i].source)); ac++;
			XtSetArg(al[ac], PmNsubArea, SrcSubName(GV_allied_model[i].source)); ac++;
			XtSetArg(al[ac], PmNtime, GV_T0_depict); ac++;
		}

		if( RunProgramManager(model_run_ending, (XtPointer)&GV_allied_model[i], al, ac) )
		{
			GV_allied_model[i].running = True;
			ProductStatusUpdateInfo(GV_allied_model[i].product_key, PS_RUNNING, NULL);
		}

		FreeItem(pre_process);
		FreeItem(process);
		FreeItem(post_process);
	}

	something_running = False;
	for(i = 0; i < GV_nallied_model; i++)
		if(GV_allied_model[i].running) something_running = True;
	cycle_busy_indicator(NULL, NULL);
}


/*  Check for required fields, wind cross-references, and value
 *  cross-references needed to run Allied Model. Returns True if all
 *  required fields and cross-references are found.
 *
 *  Note that the number of required fields, wind cross-references,
 *  and value cross-references are given in the FpaConfigSourceStruct
 *  in the field descriptor, that is, fdesc->sdef!
 */
static Boolean check_run_fields_ok( Source sdef , String fld_list )
{
	Boolean							ok, valid;
	int								nn;
	FLD_DESCRIPT					fd;
	FpaConfigAlliedFieldsStruct		*fields;
	FpaConfigAlliedWindsStruct		*winds;
	FpaConfigAlliedValuesStruct		*values;

	/* Return False if no structure passed
	*/
	(void)strcpy(fld_list, "");
	if(!sdef || !sdef->fd->sdef->allied) return False;

	/* Initialize return parameters
	*/
	valid = True;

	/* Check required fields for Allied Model
	*/
	if(sdef->fd->sdef->allied->fields)
	{
		/* Set pointer to AlliedFields structure
		*/
		fields =  sdef->fd->sdef->allied->fields;

		/* Check for each required field
		*/
		init_fld_descript(&fd);
		for( nn=0; nn<fields->nfields; nn++ )
		{
			/* Reset field descriptor for required field
			*/
			ok = set_fld_descript(&fd,
					FpaF_SOURCE, fields->src_defs[nn],
					FpaF_SUBSOURCE, fields->sub_defs[nn],
					FpaF_ELEMENT, fields->flds[nn]->element,
					FpaF_LEVEL, fields->flds[nn]->level,
					FpaF_VALUE_FUNCTION_NAME, FpaDefaultValueFunc,
					FpaF_RUN_TIME, GV_T0_depict,
					FpaF_VALID_TIME, GV_T0_depict,
					FpaF_END_OF_LIST);

			if(ok) ok = check_extract_value(1, &fd, sdef->fd->sdef->allied->time_match, 
							ZeroPointListNum, ZeroPointList);
			if(!ok)
			{
				(void)strcat(fld_list, "     ");
				(void)strcat(fld_list, fields->flds[nn]->label);
				(void)strcat(fld_list, "\n");
				valid = False;
			}
		}
	}
	/* Check required wind cross-references for Allied Model
	*/
	if(sdef->fd->sdef->allied->winds)
	{
		/* Set pointer to AlliedWinds structure
		*/
		winds =  sdef->fd->sdef->allied->winds;

		/* Check for each required wind cross-reference
		*/
		init_fld_descript(&fd);
		for( nn=0; nn<winds->nwinds; nn++ )
		{
			/* Reset field descriptor for required wind cross-reference
			*/
			ok = set_fld_descript(&fd,
					FpaF_SOURCE, winds->src_defs[nn],
					FpaF_SUBSOURCE, winds->sub_defs[nn],
					FpaF_RUN_TIME, GV_T0_depict,
					FpaF_VALID_TIME, GV_T0_depict,
					FpaF_END_OF_LIST);

			if (ok) ok = check_extract_wind_by_crossref(winds->wcrefs[nn]->name,
							&fd,  sdef->fd->sdef->allied->time_match,
							ZeroPointListNum, ZeroPointList);
			if(!ok)
			{
				(void)strcat(fld_list, "     ");
				(void)strcat(fld_list, winds->wcrefs[nn]->label);
				(void)strcat(fld_list, "    (");
				(void)strcat(fld_list, XuGetLabel("xrefwind"));
				(void)strcat(fld_list, ")\n");
				valid = False;
			}
		}
	}
	/* Check required value cross-references for Allied Model
	*/
	if(sdef->fd->sdef->allied->values)
	{
		/* Set pointer to AlliedValues structure
		*/
		values = sdef->fd->sdef->allied->values;

		/* Check for each required value cross-reference
		*/
		init_fld_descript(&fd);
		for ( nn=0; nn<values->nvalues; nn++ )
		{
			/* Reset field descriptor for required value cross-reference
			*/
			ok = set_fld_descript(&fd,
					FpaF_SOURCE, values->src_defs[nn],
					FpaF_SUBSOURCE, values->sub_defs[nn],
					FpaF_RUN_TIME, GV_T0_depict,
					FpaF_VALID_TIME, GV_T0_depict,
					FpaF_END_OF_LIST);

			if (ok) ok = check_extract_value_by_crossref(values->vcrefs[nn]->name,
							&fd,  sdef->fd->sdef->allied->time_match,
							ZeroPointListNum, ZeroPointList);
			if(!ok)
			{
				(void)strcat(fld_list, "     ");
				(void)strcat(fld_list, values->vcrefs[nn]->label);
				(void)strcat(fld_list, "    (");
				(void)strcat(fld_list, XuGetLabel("xrefvalue"));
				(void)strcat(fld_list, ")\n");
				valid  = False;
			}
		}
	}
	return valid;
}


static void auto_import_model_fields( AlliedModelStruct *m )
{
	int j, n, nmeta, nfld, nvalid;
	char mbuf[256];
	String *run_time_list, *valid_time_list;
	FLD_DESCRIPT fd;
	FpaConfigFieldStruct *fld;
	Boolean found_one = False;


	nmeta = 0;
	if ( m->source->fd->sdef->allied && m->source->fd->sdef->allied->metafiles)
		nmeta = m->source->fd->sdef->allied->metafiles->nfiles;

	for( j = 0; j < nmeta; j++)
	{
		if(!m->import[j]) continue;

		/* If this is a regular field we delete all instances of the field
		*  in every depiction.
		*/
		fld = m->source->fd->sdef->allied->metafiles->flds[j];
		if(fld->element->elem_tdep->time_dep == FpaC_NORMAL)
		{
			(void) snprintf(mbuf,sizeof(mbuf),"DELETE_FIELD %s %s ALL",fld->element->name,fld->level->name);
			(void) IngredCommand(GE_SEQUENCE, mbuf);
		}

		copy_fld_descript(&fd, m->source->fd);
		if(!set_fld_descript(&fd,
			FpaF_ELEMENT, fld->element,
			FpaF_LEVEL, fld->level,
			FpaF_END_OF_LIST)) continue;

		nfld = source_run_time_list(&fd, &run_time_list);
		if(nfld < 1) continue;

		/* Add the field to the interface controls.
		*/
		AddField(fld, False);

		if(!set_fld_descript(&fd,
			FpaF_RUN_TIME, run_time_list[0],
			FpaF_END_OF_LIST)) continue;

		nvalid = FilteredValidTimeList(&fd, FpaC_TIMEDEP_ANY, &valid_time_list);

		for(n = 0; n < nvalid; n++)
		{
			/* If the field is normal and there is a corresponding depiction we
			*  import the field.  If it is daily or static we delete the instance
			*  of the existing field.
			*/
			if(fld->element->elem_tdep->time_dep == FpaC_NORMAL)
			{
			   if(!InTimeList(valid_time_list[n], GV_depict, GV_ndepict, NULL))
				continue;
			}
			else
			{
				(void) snprintf(mbuf, sizeof(mbuf), "DELETE_FIELD %s %s %s",
					fld->element->name, fld->level->name, valid_time_list[n]);
				(void) IngredCommand(GE_SEQUENCE, mbuf);
			}
			(void) IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s %s %s %s %s %s",
				SrcName(m->source), SrcSubDashName(m->source),
				run_time_list[0],
				fld->element->name,
				fld->level->name,
				valid_time_list[n]);
			found_one = True;
		}
		nvalid = FilteredValidTimeListFree(&valid_time_list, nvalid);
		nfld = source_run_time_list_free(&run_time_list, nfld);
	}

	/* If something found notify observers
	 */
	if(found_one)
	{
		String parm[1] = {NULL};
		NotifyObservers(OB_FIELD_AVAILABLE, parm, 1);
	}
}


/*ARGSUSED*/
static void close_cb(Widget w, XtPointer na, XtPointer unused)
{
	int i;
	for( i = 0; i < 8; i++)
		XuFreePixmap(dialog, pixmaps[i]);
	XuDestroyDialog(dialog);
	dialog = NULL;
}
