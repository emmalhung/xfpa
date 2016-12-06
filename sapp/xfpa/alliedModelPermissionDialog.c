/*========================================================================*/
/*
*	File:		alliedModelPermissionDialog.c
*
*	Purpose:	Automatic field import handling.  A dialog is put up when
*               the first model is finished with the models listed as
*               buttons.  As more models finish they are added to the list.
*               Note that the dialog management is delayed. This was done
*               to improve visual performance.
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
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include "alliedModel.h"
#include "editor.h"
#include "observer.h"

static void AutoImportCB ( Widget, XtPointer, XtPointer );

static Widget dialog = NULL;
static Widget timeBar;


void ACTIVATE_alliedModelImportPermissionDialog(AlliedModelStruct *model )
{
	int i;
	Widget text, list, form;
	XmString label;

	static XuDialogActionsStruct action_items[] = {
		{ "yesBtn", AutoImportCB, NULL},
		{ "noBtn",  XuDestroyDialogCB, NULL}
	};

	if(dialog)
	{
		XtManageChild(model->btn);
		XuToggleButtonSet(model->btn, True, True);
		XuShowDialog(dialog);
		return;
	}

	dialog = XuCreateFormDialog(GW_mainWindow, "alliedModelImport",
		XmNnoResize, True,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 9,
		NULL);

	text = XmVaCreateManagedLabel(dialog, "text",
		XmNalignment, XmALIGNMENT_BEGINNING,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	list = XmVaCreateRowColumn(dialog, "modelBtns",
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	for(i = 0; i < GV_nallied_model; i++)
	{
		GV_allied_model[i].btn = XmVaCreateToggleButton(list,
								SrcLabel(GV_allied_model[i].source),
								NULL);
	}
	XtManageChild(model->btn);
	XuToggleButtonSet(model->btn, True, True);
	XtManageChild(list);

	form = XmVaCreateManagedForm(dialog, "timeBarManager",
		XmNborderWidth, 1,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNshadowThickness, 2,
		XmNtopWidget, list,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	label = XuNewXmString(" ");
	timeBar = XmVaCreateManagedLabel(form, "timeBar",
		XmNlabelString, label,
		XmNmappedWhenManaged, False,
		XmNheight, 10,
		XmNresizable, False,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 5,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XmStringFree(label);

	XuShowDialog(dialog);
}


/*ARGSUSED*/
static void AutoImportCB(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j, n, nmeta, nfld, nvalid, total, count, pcnt;
	char mbuf[256];
	String *run_time_list, *valid_time_list;
	AlliedModelStruct *m;
	FLD_DESCRIPT fd;
	FpaConfigFieldStruct *fld;

	XuSetBusyCursor(ON);
	XuUpdateDisplay(dialog);

	/* We need to count the number of fields we are to import so that
	*  we can set the percent completed bar.
	*/
	count = total = 0;
	for(i = 0; i < GV_nallied_model; i++)
	{
		m = GV_allied_model + i;
		if(!XtIsManaged(m->btn) || !XmToggleButtonGetState(m->btn)) continue;
		nmeta = 0;
		if ( m->source->fd->sdef->allied &&
				m->source->fd->sdef->allied->metafiles)
			nmeta = m->source->fd->sdef->allied->metafiles->nfiles;
		for( j = 0; j < nmeta; j++)
			if(m->import[j]) total++;
	}

	/* Ok. Now import the fields.
	*/
	DeactivateMenu();
	for(i = 0; i <GV_nallied_model; i++)
	{
		m = GV_allied_model + i;
		if(!XtIsManaged(m->btn) || !XmToggleButtonGetState(m->btn)) continue;

		nmeta = 0;
		if ( m->source->fd->sdef->allied &&
				m->source->fd->sdef->allied->metafiles)
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
				/* I discovered that a position of 5 was the minimum required
				*  for proper layout.  Thus this logic switch.
				*/
				count++;
				if((pcnt = (count*100)/total) >= 5)
				{
					XtVaSetValues(timeBar,
						XmNmappedWhenManaged, True,
						XmNrightPosition, pcnt, 
						NULL);
				}
				XuUpdateDisplay(dialog);

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
			}
			nvalid = FilteredValidTimeListFree(&valid_time_list, nvalid);
			nfld = source_run_time_list_free(&run_time_list, nfld);
		}
	}
	XuDestroyDialog(dialog);
	ActivateMenu();
	XuSetBusyCursor(OFF);

	/* If something actually imported send a notification of a field change. */
	if(count > 0)
	{
		String parm[1] = {NULL};
		NotifyObservers(OB_FIELD_AVAILABLE, parm, 1);
	}
}
