/*========================================================================*/
/*
*   File:     field_deleteDialog.c
*
*   Purpose:  To selectively delete fields from the depiction sequence.
*             This dialog also handles static and daily fields.
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
#include <ingred.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/TabStack.h>
#include <Xbae/Matrix.h>
#include "resourceDefines.h"
#include "depiction.h"
#include "editor.h"
#include "guidance.h"
#include "observer.h"
#include "help.h"

/* Defines for matrix index values for clarity
 */
#define NF	0
#define DF	1
#define SF	2


static void    exit_cb				       (Widget, XtPointer, XtPointer);
static Boolean create_matrix_widget        (Widget, int);
static void    delete_cb			       (Widget, XtPointer, XtPointer);
static void    field_matrix_select_cell_cb (Widget, XtPointer, XtPointer);
static void    select_all_cb               (Widget, XtPointer, XtPointer);
static int     time_compare                (const void*, const void*);
static void    tabs_cb                     (Widget, XtPointer, XtPointer);

static Widget  dialog          = NULL;
static Widget  normalFieldsTab = NULL;
static int     page_number     = 0;
static Boolean deleting        = False;

/* Define characteristics of the various fields.
*/
typedef struct {
	int      no; 			/* field number index */
	String  *times;         /* field time associated with each column */
	Boolean  selection_status;
} FI;

static struct {
	int      type;			/* type of field to handle */
	Widget   matrix;		/* associated Xbae matrix widget */
	String   label;			/* label widget resource identifier */
	int      nfields;		/* number of fields (rows) */
	FI      *field;			/* field data */
	int      ntimes;		/* number of times (columns) */
	String  *column_times;
	Boolean *column_selection_status;
} fd[] = {
	{FpaC_NORMAL, NULL, "normalFields", 0, NULL, 0, NULL, NULL},
	{FpaC_DAILY,  NULL, "dailyFields",  0, NULL, 0, NULL, NULL},
	{FpaC_STATIC, NULL, "staticFields", 0, NULL, 0, NULL, NULL}
};




void ACTIVATE_deleteFieldsDialog(Widget ref_widget)
{
	int i, j, m, n, ntimes, nfields;
	char mbuf[300];
	String  *elem, *level, *times, *elem_list, *level_list;
	Widget selectAllBtn, deselectAll, sep, label, tabManager, specialFieldsTab;

	static XuDialogActionsStruct action_items[] = {
		{ "okBtn",    delete_cb, NULL },
		{ "closeBtn", exit_cb, NULL },
		{ "helpBtn",  HelpCB, HELP_DELETE_FIELDS }
	};

	if (dialog) return;
	deleting = False;

	/* Copy the field list status return from Ingred.
	*/
	(void) GEStatus("FIELDS", &nfields, &elem, &level, NULL);
	elem_list  = NewStringArray(nfields);
	level_list = NewStringArray(nfields);
	for( i = 0; i < nfields; i++ )
	{
		elem_list[i]  = XtNewString(elem[i]);
		level_list[i] = XtNewString(level[i]);
	}

	/* The following creates the matrix values for all fields.
	*/
	for( m = 0; m < 3; m++ )
	{
		for( i = 0; i < GV_nfield; i++ )
		{
			if(GV_field[i]->info->element->elem_tdep->time_dep != fd[m].type) continue;
			if(!InFieldList(GV_field[i], nfields, elem_list, level_list, NULL)) continue;

			n = fd[m].nfields;
			fd[m].nfields++;
			fd[m].field = MoreMem(fd[m].field, FI, fd[m].nfields);
			fd[m].field[n].no = i;
			fd[m].field[n].times = NULL;
			fd[m].field[n].selection_status = False;

			/* Obtain list of valid times for this field and then create the list of valid times
			 * to associate with the columnt. Note that daily fields can have more then one time
			 * associated with any given column as the column refers to the day only.
			 */
			(void) snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s", GV_field[i]->info->element->name, GV_field[i]->info->level->name);
			if( GEStatus(mbuf, &ntimes, &times, NULL, NULL) == GE_VALID )
			{
				for( n = 0; n < ntimes; n++ )
				{
					InTimeListResolution((fd[m].type == FpaC_DAILY)? DAYS:MINUTES);
					if(!InTimeList(times[n], fd[m].column_times, fd[m].ntimes, NULL))
					{
						fd[m].column_times = MoreMem(fd[m].column_times, String, fd[m].ntimes+1);
						fd[m].column_times[fd[m].ntimes] = XtNewString(times[n]);
						fd[m].ntimes++;
					}
				}
			}
		}

		/* Don't assume the times will arrive in the required order and sort the columns.
		 */
		qsort((void*)fd[m].column_times, (size_t) fd[m].ntimes, sizeof(String), time_compare);

		/* Array to say which columns have been selected
		 */
		fd[m].column_selection_status = NewBooleanArray(fd[m].ntimes);

		/* 20071017: Go through the fields again as the columns were sorted and the field time
		 * index's may not agree with the columns. The time match for daily fields is restricted
		 * to the day but the folowing logic will find the specific field time for the day.
		 */
		for( i = 0; i < fd[m].nfields; i++ )
		{
			FIELD_INFO *fld = GV_field[fd[m].field[i].no];

			fd[m].field[i].times = NewStringArray(fd[m].ntimes);

			(void) snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s", fld->info->element->name, fld->info->level->name);
			if( GEStatus(mbuf, &ntimes, &times, NULL, NULL) == GE_VALID)
			{
				for( j = 0; j < ntimes; j++ )
				{
					int col;
					InTimeListResolution((fd[m].type == FpaC_DAILY)? DAYS:MINUTES);
					if(InTimeList(times[j], fd[m].column_times, fd[m].ntimes, &col))
						fd[m].field[i].times[col] = XtNewString(times[j]);
				}
			}
		}

			}

	FreeList(elem_list, nfields);
	FreeList(level_list, nfields);

	/* Now for the actual dialog creation.
	 */
	dialog = XuCreateFormDialog(ref_widget, "deleteFields",
		XuNmwmDeleteOverride, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 14,
		XmNverticalSpacing, 14,
		NULL);

	/* These buttons are under the tabs and their effect will depend on which tab is active.
	 */
	selectAllBtn = XmVaCreateManagedPushButton(dialog, "selectAll",
		XmNmarginWidth, 12,
		XmNmarginHeight, 4,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(selectAllBtn, XmNactivateCallback, select_all_cb, (XtPointer)True);

	deselectAll = XmVaCreateManagedPushButton(dialog, "deselectAll",
		XmNmarginWidth, 12,
		XmNmarginHeight, 4,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, selectAllBtn,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(deselectAll, XmNactivateCallback, select_all_cb, (XtPointer)False);

	/* The deletion matrix widgets are placed into tabs as there can be a lot of normal
	 * depictions. We create 2, one for normal fields and one for the rest.
	 */
	tabManager = XmVaCreateTabStack(dialog, "tabs",
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, selectAllBtn,
		NULL);

	XtAddCallback(tabManager, XmNtabSelectedCallback, tabs_cb, NULL);

	normalFieldsTab = XmVaCreateForm(tabManager, "normalFields",
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		XmNresizable, False,
		NULL);

	specialFieldsTab = XmVaCreateForm(tabManager, "otherFields",
		XmNhorizontalSpacing, 19,
		XmNverticalSpacing, 19,
		XmNresizable, False,
		NULL);

	/* Now to layout the matrix items and their labels. The commands
	 * "XmNwidth, 0" and "XmNheight, 0" cause the matrix widget to ask
	 * for a new minimum size for itself and compact the layout.
	 */
	(void) create_matrix_widget(normalFieldsTab, NF);

	label = NullWidget;
	if(create_matrix_widget(specialFieldsTab, SF))
	{
		label = XmVaCreateManagedLabel(specialFieldsTab, fd[SF].label,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		XtVaSetValues(fd[SF].matrix,
			XmNwidth, 0,
			XmNheight, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 0,
			XmNbottomAttachment, XmATTACH_NONE,
			NULL);
	}

	if(create_matrix_widget(specialFieldsTab, DF))
	{
		sep = NullWidget;
		if(label)
		{
			sep = XmVaCreateManagedSeparator(specialFieldsTab, "sep",
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, fd[SF].matrix,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_FORM,
				NULL);
		}

		label = XmVaCreateManagedLabel(specialFieldsTab, fd[DF].label,
			XmNtopAttachment, (sep)? XmATTACH_WIDGET:XmATTACH_FORM,
			XmNtopWidget, sep,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		XtVaSetValues(fd[DF].matrix,
			XmNwidth, 0,
			XmNheight, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 0,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	}

	XtManageChild(normalFieldsTab);
	XtManageChild(specialFieldsTab);
	XtManageChild(tabManager);
	XuShowDialog(dialog);
}


static int time_compare(const void *t1, const void *t2)
{
	return MinuteDif(*((String*)t2), *((String*)t1));
}


static Boolean create_matrix_widget(Widget parent, int ndx)
{
	int      i, j;
	char     mbuf[300];
	Pixel    exists_color, select_color, background, **cell_color;
	XmString *row_labels, *col_labels;
	
	if(fd[ndx].nfields < 1) return False;

	exists_color = XuLoadColorResource(dialog, RNdeleteFieldsKeep,"White");
	select_color = XuLoadColorResource(dialog, RNdeleteFieldsDelete,"Black"),
	XtVaGetValues(dialog, XmNbackground, &background, NULL);

	row_labels = NewMem(XmString, fd[ndx].nfields);
	for( i = 0; i < fd[ndx].nfields; i++ )
		row_labels[i] = XmStringCreate(GV_field[fd[ndx].field[i].no]->info->sh_label, NORMAL_FONT);

	col_labels = NewMem(XmString, fd[ndx].ntimes);
	for( i = 0; i < fd[ndx].ntimes; i++ )
	{
		if(fd[ndx].type == FpaC_NORMAL)
		{
			col_labels[i] = XmStringSequenceTime(fd[ndx].column_times[i], SEQUENCE_HOUR_FONT, SEQUENCE_MINUTE_FONT);
		}
		else if(fd[ndx].type == FpaC_DAILY)
		{
			(void) strcpy(mbuf, DateString(fd[ndx].column_times[i], SHORT_DAY_NAME_NR_OF_MONTH));
			col_labels[i] = XmStringCreateSimple(mbuf);
		}
		else
		{
			(void) strcpy(mbuf, TimeDiffFormat(GV_T0_depict, fd[ndx].column_times[i], minutes_in_depictions()));
			col_labels[i] = XmStringCreateSimple(mbuf);
		}
	}

	cell_color = NewMem(Pixel*, fd[ndx].nfields);
	for( i = 0; i < fd[ndx].nfields; i++ )
	{
		cell_color[i] = NewMem(Pixel, fd[ndx].ntimes);
		for( j = 0; j < fd[ndx].ntimes; j++ )
		{
			cell_color[i][j] = (valid_tstamp(fd[ndx].field[i].times[j]))? exists_color:background;
		}
	}

	(void) strcpy(mbuf, "fieldMatrix_");
	(void) strcat(mbuf, fd[ndx].label);

	fd[ndx].matrix = CreateXbaeMatrix(parent, mbuf,
		XmNrows, fd[ndx].nfields,
		XmNcolumns, fd[ndx].ntimes,
		XmNxmRowLabels, row_labels,
		XmNxmColumnLabels, col_labels,
		XmNcellBackgrounds, cell_color,
		XmNbuttonLabels, True,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNselectedBackground, select_color,
		XmNselectScrollVisible, False,
		XmNgridType, XmGRID_CELL_SHADOW,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	XbaeMatrixResizeColumnsToCells(fd[ndx].matrix, True);

	XtAddCallback(fd[ndx].matrix, XmNselectCellCallback, field_matrix_select_cell_cb, INT2PTR(ndx)); 

	XmStringArrayFree(row_labels, fd[ndx].nfields);
	XmStringArrayFree(col_labels, fd[ndx].ntimes);
	FreeList(cell_color, fd[ndx].nfields);

	return True;
}



/*ARGSUSED*/
static void tabs_cb( Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTabStackCallbackStruct *rtn = (XmTabStackCallbackStruct *)call_data;
	page_number = (rtn->selected_child == normalFieldsTab)? 0:1;
}


static void field_matrix_select_cell_cb (Widget w, XtPointer client_data, XtPointer call_data)
{
	int i, j;
	int m = PTR2INT(client_data);
	XbaeMatrixSelectCellCallbackStruct *rtn = (XbaeMatrixSelectCellCallbackStruct *)call_data;

	/* Note that a row or column value of -1 is a label callback. It is possible under
	 * some circumstances for both to be negative and this must be screened out.
	 */
	if( rtn->row < 0 && rtn->column < 0 ) return;
	
	/* Row label callback */
	if( rtn->column < 0 )
	{
		fd[m].field[rtn->row].selection_status = !fd[m].field[rtn->row].selection_status;
		for( j = 0; j < fd[m].ntimes; j++ )
		{
			if(valid_tstamp(fd[m].field[rtn->row].times[j]))
			{
				if(fd[m].field[rtn->row].selection_status)
					XbaeMatrixSelectCell(w, rtn->row, j);
				else
					XbaeMatrixDeselectCell(w, rtn->row, j);
			}
		}
	}
	/* Column label callback */
	else if( rtn->row < 0 )
	{
		fd[m].column_selection_status[rtn->column] = !fd[m].column_selection_status[rtn->column];
		for( i = 0; i < fd[m].nfields; i++ )
		{
			if(valid_tstamp(fd[m].field[i].times[rtn->column]))
			{
				if(fd[m].column_selection_status[rtn->column])
					XbaeMatrixSelectCell(w, i, rtn->column);
				else
					XbaeMatrixDeselectCell(w, i, rtn->column);
			}
		}
	}
	/* Cell callback */
	else
	{
		if(valid_tstamp(fd[m].field[rtn->row].times[rtn->column]))
		{
			if(XbaeMatrixIsCellSelected(w, rtn->row, rtn->column))
				XbaeMatrixDeselectCell(w, rtn->row, rtn->column);
			else
				XbaeMatrixSelectCell(w, rtn->row, rtn->column);
		}
	}
}


/*ARGSUSED*/
static void select_all_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j, m, start, end;
	Boolean status = PTR2BOOL(client_data);

	/* There matrix the buttons apply to depends on the active tab
	 */
	if(page_number == 0) {
		start = 0; end = 1;
	} else {
		start = 1; end = 3;
	}

	for( m = start; m < end; m++ )
	{
		for( i = 0; i < fd[m].nfields; i++ )
		{
			fd[m].field[i].selection_status = status;
			for( j = 0; j < fd[m].ntimes; j++ )
			{
				fd[m].column_selection_status[j] = status;
				if(valid_tstamp(fd[m].field[i].times[j]))
				{
					if(status)
						XbaeMatrixSelectCell(fd[m].matrix, i, j);
					else
						XbaeMatrixDeselectCell(fd[m].matrix, i, j);
				}
			}
		}
	}
}



/* Here we do the actual field deletion.  In the case of depictions, if
*  every field is marked for deletion we use a delete depiction call.
*/
/*ARGSUSED*/
static void delete_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j, m, ndepict_deleted;
	char mbuf[300];
	String *depict_deleted, notify[1];
	Boolean delete_all, depict_seq_change;

	deleting = True;
	XuSetBusyCursor(ON);
	DeactivateMenu();

	/* First we will scan for the deletion of complete depictions.  This is
	*  associated with all of the fields in the fd[0] section of the data
	*  structure.  Note that we must delete the depictions after the field
	*  deletes but that we must check for them before the field deletes.
	*/
	ndepict_deleted = 0;
	depict_deleted = NewStringArray(GV_ndepict);

	depict_seq_change = False;
	if(fd[0].nfields > 0)
	{
		for( i = 0; i < fd[0].ntimes; i++ )
		{
			delete_all = True;
			for( j = 0; j < fd[0].nfields; j++ )
			{
				if(!valid_tstamp(fd[0].field[j].times[i])) continue;
				if(XbaeMatrixIsCellSelected(fd[0].matrix, j, i)) continue;
				delete_all = False;
				break;
			}
			if(delete_all)
			{
				depict_deleted[ndepict_deleted] = fd[0].column_times[i];
				ndepict_deleted++;
			}
		}
	}
	/* Now delete individual fields.  If a field is removed from every depiction
	*  we call the remove field function to remove the field from the edit
	*  control panel.
	*/
	for( m = 0; m < 3; m++ )
	{
		for( j = 0; j < fd[m].nfields; j++ )
		{
			delete_all = True;
			for( i = 0; i < fd[m].ntimes; i++ )
			{
				if(!valid_tstamp(fd[m].field[j].times[i])) continue;
				if(m == 0 && InTimeList(fd[m].field[j].times[i], depict_deleted, ndepict_deleted, NULL)) continue;
				if(XbaeMatrixIsCellSelected(fd[m].matrix, j, i))
				{
					(void) snprintf(mbuf, sizeof(mbuf), "DELETE_FIELD %s %s %s",
						GV_field[fd[m].field[j].no]->info->element->name,
						GV_field[fd[m].field[j].no]->info->level->name,
						fd[m].field[j].times[i]);
					(void) IngredCommand(GE_SEQUENCE, mbuf);
					depict_seq_change = True;
					FreeItem(fd[m].field[j].times[i]);
				}
				else
				{
					delete_all = False;
				}
			}
			if(delete_all)
			{
				RemoveField(GV_field[fd[m].field[j].no]);
			}
		}
	}
	/*
	 * As noted above we must now delete the depictions. Also reset the
	 * active depiction. If it has been removed select the first one in
	 * the sequence.
	 */
	if(ndepict_deleted > 0)
	{
		TSTAMP dt;
		(void) safe_strcpy(dt, ActiveDepictionTime(FIELD_INDEPENDENT));
		depict_seq_change = True;

		for( i = 0; i < ndepict_deleted; i++ )
			RemoveDepiction(depict_deleted[i]);

		for( i = 0; i < GV_ndepict; i++ )
			if(matching_tstamps(dt, GV_depict[i])) break;
		if(i >= GV_ndepict)
			MakeActiveDepiction(GV_depict[0]);
		else
			MakeActiveDepiction(dt);
	}
	else
	{
		ActivateMenu();
	}

	notify[0] = (String) ((long)depict_seq_change);
	NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);

	FreeItem(depict_deleted);
	XuSetBusyCursor(OFF);
	deleting = False;
	exit_cb(dialog, NULL, NULL);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int m, i;
	
	if (!dialog) return;
	if (deleting) return;

	for( m = 0; m < 3; m++)
	{
		for( i = 0; i < fd[m].nfields; i++ )
		{
			FreeList(fd[m].field[i].times, fd[m].ntimes);
		}
		FreeItem(fd[m].field);
		FreeList(fd[m].column_times, fd[m].ntimes);
		FreeItem(fd[m].column_selection_status);
		fd[m].nfields = 0;
		fd[m].ntimes = 0;
	}
	XuDestroyDialog(dialog);
	dialog = NULL;
}
