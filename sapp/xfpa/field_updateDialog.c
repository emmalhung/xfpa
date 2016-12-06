/*========================================================================*/
/*
*   File:       field_updateDialog.c
*
*   Purpose:    To selectively update depiction sequence field from a
*               variety of numerical model and allied model sources. In
*               these terms update means to replace the existing field
*               with the most recent field of the same valid time from
*               the selected source.
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
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xbae/Matrix.h>
#include "resourceDefines.h"
#include "editor.h"
#include "help.h"
#include "depiction.h"
#include "observer.h"


/* Source information.
*/
typedef struct {
	Source  src;
	Boolean available;	/* Is any data available from this source? */
	TSTAMP  run_time;	/* last read in run time */
	Byte    refndx;		/* reference number */
} SIStruct;

typedef struct {
	FpaConfigFieldStruct *fptr;	/* field config data pointer */
	int     no; 				/* field number index */
	Boolean *available;			/* is field available at the corresponding time */
	Boolean *in_depict;			/* does field exist in depiction? */
	Byte    *src_from;			/* if field has been updated this gives the source */
} FIStruct;

typedef struct {
	int      ntime;				/* combined depict time and source valid time */
	String   *time;				/* the list of time */
	Boolean  *is_depict;		/* does a depiction exist at this time? */
	int      nfield;			/* number of fields in depiction sequence */
	FIStruct *field;			/* field information */
} DataMatrixStruct;


const String module = "Update Fields Dialog";


static Widget dialog = NULL;
static Widget sourceSelect;
static Widget fieldMatrix;
static Widget selectPresetBtn;
static Widget *src_btn;
static Widget updateBtn;
static Widget closeBtn;
static Widget validTimeDisplayPopup;

static DataMatrixStruct *dm = NULL;
static int       nsources = 0;				/* The number of sources */
static int       navailable_sources = 0;	/* How many of the sources are actually available for use */
static SIStruct *sources  = NULL;			/* The source information */
static SIStruct *selected_source = NULL;	/* The currently selected source */
static char      updated_char = 'U';
static Pixel     update_colour = 0;
static Pixel     exists_colour = 0;
static Pixel     no_field_colour = 0;
static Boolean   show_existing_depict_only = False;
static Boolean  *field_selection_status = NULL;
static Boolean  *time_selection_status = NULL;
static Boolean   updating = False;		/* Lock for when sources are changing */
static Boolean   processing = False;		/* lock for when user does an update */

static void    exit_cb					    (Widget, XtPointer, XtPointer);
static void    copy_from_src_data		    (DataMatrixStruct *, DataMatrixStruct *);
static void    update_matrix			    (void);
static void    data_matrix_free			    (DataMatrixStruct *);
static void    display_option_cb		    (Widget, XtPointer, XtPointer);
static Boolean field_update                 (Boolean);
static void    field_update_source_data	    (Boolean);
static Boolean same_data_matrix			    (DataMatrixStruct*, DataMatrixStruct*);
static void    source_select_cb			    (Widget, XtPointer, XtPointer);
static void    select_all_cb			    (Widget, XtPointer, XtPointer);
static void    select_from_office_list_cb   (Widget, XtPointer, XtPointer);
static void    show_legend_cb			    (Widget, XtPointer, XtPointer);
static void    track_cell_cb                (Widget, XtPointer, XtPointer);
static void    update_cb				    (Widget, XtPointer, XtPointer);
static void    field_matrix_select_cell_cb  (Widget, XtPointer, XtPointer); 
static void    number_of_depictions_changed (String*, int);
static void    update_column_time_display   (String*, int);

static DataMatrixStruct *make_data_matrix	(void);


void ACTIVATE_updateFieldsDialog(Widget ref_widget )
{
	int      i, active = 0;
	Pixel    bkg;
	XmString xlabel;
	Widget   selectAllBtn, deselectAll, depictOnlyBtn;

	static XuDialogActionsStruct action_items[] = {
		{ "updateBtn", update_cb, NULL },
		{ "legendBtn", show_legend_cb, NULL },
		{ "closeBtn",  exit_cb, NULL },
		{ "helpBtn",   HelpCB, HELP_UPDATE_FIELDS }
	};

	if(NotNull(dialog))
	{
		XuShowDialog(dialog);
		return;
	}

	processing = False;

	/* Check for that case where we have no data at all!
	*/
	if(navailable_sources < 1)
	{
		XuShowMessage(ref_widget, "NoValidSources", NULL);
		return;
	}

	dialog = XuCreateToplevelFormDialog(ref_widget, "fieldUpdate",
		XuNmwmDeleteOverride, exit_cb,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 14,
		XmNverticalSpacing, 14,
		NULL);

	updateBtn = XuGetActionAreaBtn(dialog, action_items[0]);
	closeBtn  = XuGetActionAreaBtn(dialog, action_items[2]);

	update_colour   = XuLoadColorResource(dialog, RNfieldUpdateUpdate,"White");
	exists_colour   = XuLoadColorResource(dialog, RNfieldUpdateExists,"Grey");
	no_field_colour = XuLoadColorResource(dialog, RNfieldUpdateNoField,"Black");
	updated_char    = *XuGetStringResource(RNfieldUpdateDoneChar, "U");

	sourceSelect = XuVaMenuBuildOption(dialog, "sourceSelect", NULL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	/* Create the source list buttons.
	*/
	for(i = 0; i < nsources; i++)
	{
		char mbuf[256];
		if(selected_source == &sources[i]) active = i;
		(void) snprintf(mbuf, sizeof(mbuf), "%d)  %s", (int) sources[i].refndx, SrcLabel(sources[i].src));
		src_btn[i] = XuMenuAddButton(sourceSelect, "btn", mbuf, i, source_select_cb, INT2PTR(i));
		XtSetSensitive(src_btn[i], sources[i].available);
	}
	XuMenuSelectItem(sourceSelect, active);

	/* Toggle to switch between match depictions only or not state. */
	depictOnlyBtn = XtVaCreateManagedWidget("depictOnlyBtn", xmToggleButtonWidgetClass, dialog,
		XmNset, (show_existing_depict_only) ? XmSET:XmUNSET,
		XmNborderWidth, 2,
		XmNmarginHeight, 5,
		XmNmarginWidth, 5,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sourceSelect,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(depictOnlyBtn, XmNvalueChangedCallback, display_option_cb, NULL);

	selectPresetBtn = XtVaCreateManagedWidget("selectPresetList", xmPushButtonWidgetClass, dialog,
		XmNmarginWidth, 12,
		XmNmarginHeight, 4,
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(selectPresetBtn, XmNactivateCallback, select_from_office_list_cb, NULL);

	selectAllBtn = XtVaCreateManagedWidget("selectAll", xmPushButtonWidgetClass, dialog,
		XmNmarginWidth, 12,
		XmNmarginHeight, 4,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, selectPresetBtn,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(selectAllBtn, XmNactivateCallback, select_all_cb, (XtPointer)True);

	deselectAll = XtVaCreateManagedWidget("deselectAll", xmPushButtonWidgetClass, dialog,
		XmNmarginWidth, 12,
		XmNmarginHeight, 4,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, selectAllBtn,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	XtAddCallback(deselectAll, XmNactivateCallback, select_all_cb, (XtPointer)False);

	/* A label used as a popup to display the valid time of the column when the
	 * mouse is over the column time label. This is done so that the user can see
	 * the absolute time and not just the T0 offset time. Must be created unmanaged
	 * and before the matrix so that it will appear on top when popped up. The label
	 * is given a string here so that when it first pops up it it sized correctly.
	 */
	xlabel = XmStringCreateLocalized("1234567891234567");
	validTimeDisplayPopup = XmVaCreateLabel(dialog, "vtdp",
		XmNlabelString, xlabel,
		XmNborderWidth, 1,
		XmNmarginWidth, 3,
		XmNmarginHeight, 3,
		XmNshadowThickness, 0,
		XmNforeground, XuLoadColor(dialog,"Black"),
		XmNbackground, XuLoadColor(dialog,"White"),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
	XmStringFree(xlabel);

	/* To explain the border width strangeness ... The popup for the valid time
	 * would not go away when the cursor left the top of the matrix. The only way
	 * to get the tracking callback to activate in this case was to use a large
	 * border width. Since I did not want this to be visible it is set to the
	 * background colour. The offsets had to be set accordingly for the layout.
	 */
	XtVaGetValues(dialog, XmNbackground, &bkg, NULL);

	fieldMatrix = CreateXbaeMatrix(dialog, "fieldMatrix",
		XmNrows, 1,
		XmNcolumns, 1,
		XmNbuttonLabels, True,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNselectedBackground, update_colour,
		XmNselectScrollVisible, False,
		XmNgridType, XmGRID_CELL_SHADOW,
		XmNborderWidth, 20,
		XmNborderColor, bkg,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, depictOnlyBtn,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, -6,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, -6,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, selectPresetBtn,
		NULL);

	XtAddCallback(fieldMatrix, XmNselectCellCallback, field_matrix_select_cell_cb, NULL); 
	XtAddCallback(fieldMatrix, XmNtrackCellCallback, track_cell_cb, NULL);

	field_update_source_data(True);

	XuShowDialog(dialog);
}


void InitFieldUpdateSystem(void)
{
	int i, k, n;
	String *list;
	SourceList src;

	if (sources) return;

	SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED, FpaC_TIMEDEP_ANY, &src, &nsources);
	sources = NewMem(SIStruct, nsources);
	src_btn = NewMem(Widget, nsources);
	for(i = 0; i < nsources; i++)
	{
		sources[i].src    = src[i];
		sources[i].refndx = (Byte) (i+1);
	}

	selected_source = NULL;
	SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED|SRC_LAST_RUN_DATA, FpaC_NORMAL, &src, &navailable_sources);
	for(k = 0; k < nsources; k++)
	{
		sources[k].available = False;
		for(i = 0; i < navailable_sources; i++)
		{
			if(sources[k].src != src[i]) continue;
			sources[k].available = True;
			if (!selected_source) selected_source = &sources[k];
			sources[k].run_time[0] = '\0';
			n = source_run_time_list(src[i]->fd, &list);
			if(n > 0) (void) safe_strcpy(sources[k].run_time, list[0]);
			(void)source_run_time_list_free(&list, n);
			break;
		}
	}
	if (!selected_source) selected_source = sources;

	AddSourceObserver(field_update,"FieldUpdate");
	AddObserver(OB_DEPICTION_NUMBER_CHANGE, number_of_depictions_changed);
	AddObserver(OB_DEPICTION_TZERO_CHANGE, update_column_time_display);
	AddObserver(OB_FIELD_AVAILABLE, number_of_depictions_changed);
}



/*=================== LOCAL FUNCTIONS =======================*/


/* Callback used to popup the label containing the valid time of the column.
 * The valid times are in the label row with an address of -1 and the matrix
 * functions cannot get the position of this so row 1 is used for vertical
 * positioning.
 */
/*ARGSUSED*/
static void track_cell_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	XbaeMatrixTrackCellCallbackStruct *rtn = (XbaeMatrixTrackCellCallbackStruct *)call_data;

	if( rtn->row != -1 || rtn->column < 0 )
	{
		XtUnmanageChild(validTimeDisplayPopup);
	}
	else
	{
		int i, n, x, y;
		Position wx, wy, dx, dy;
		Widget clipWindow;

		XtTranslateCoords(dialog, 0, 0, &dx, &dy);
		XtVaGetValues(w, XmNclipWindow, &clipWindow, NULL);
		XtTranslateCoords(clipWindow, 0, 0, &wx, &wy);

		(void) XbaeMatrixRowColToXY(w, 1, rtn->column, &x, &y);

		XtVaSetValues(validTimeDisplayPopup,
			XmNleftOffset, wx + (Position)x - dx, 
			XmNtopOffset, wy - dy, 
			NULL);

		for(n = 0, i = 0; i < dm->ntime; i++ )
		{
			if(show_existing_depict_only && !dm->is_depict[i]) continue;
			if(n == rtn->column) break;
			n++;
		}
		XuWidgetLabel(validTimeDisplayPopup, DateString(dm->time[i], DEFAULT_FORMAT));
		XtManageChild(validTimeDisplayPopup);
	}
}


/* This function is called when depictions are added or removed from
*  the sequence. Only action if we do not already know of the changes.
*/
static void number_of_depictions_changed(String *parms, int nparms)
{
	int i, n;
	DataMatrixStruct *d;
	Boolean changed = False;

	if (IsNull(dm)) return;
	if (processing) return;

	/* See if the depiction sequence has changed */
	if(nparms > 0 && PTR2INT(parms[0]))
	{
		changed = True;
	}
	else
	{
		for(n = 0, i = 0; i < dm->ntime; i++) if(dm->is_depict[i]) n++;
		changed = (n != GV_ndepict);
	}

	/* Check to see if there has been a change in the number of fields */
	d = make_data_matrix();
	if(changed || d->nfield != dm->nfield)
		field_update_source_data(True);
	else
		data_matrix_free(d);
}


/* Called when the T0 depiction changes */
/*ARGSUSED*/
static void update_column_time_display(String *parms, int nparms)
{
	int      i, n;
	XmString *col_labels;

	if (!dialog)       return;
	if (updating)      return;
	if (processing)    return;
	if (!dm)           return;
	if (dm->ntime < 1) return;

	col_labels = NewXmStringArray(dm->ntime);
	for(n = 0, i = 0; i < dm->ntime; i++ )
	{
		if(show_existing_depict_only && !dm->is_depict[i]) continue;
		col_labels[n] = XmStringSequenceTime(dm->time[i], SEQUENCE_HOUR_FONT, SEQUENCE_MINUTE_FONT);
		n++;
	}
	XtVaSetValues(fieldMatrix,
		XmNcolumns, n,
		XmNxmColumnLabels, col_labels,
		NULL);
	XbaeMatrixResizeColumnsToCells(fieldMatrix, True);
	XuUpdateDisplay(dialog);
	XmStringArrayFree(col_labels, n);
}


/*ARGSUSED*/
static void field_matrix_select_cell_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	int i, j, n;
	XbaeMatrixSelectCellCallbackStruct *rtn = (XbaeMatrixSelectCellCallbackStruct *)call_data;

	/* Note that a row or column value of -1 is a label callback. It is possible under
	 * some circumstances for both to be negative and this must be screened out.
	 */
	if( rtn->row < 0 && rtn->column < 0 ) return;
	
	/* Row label callback */
	if( rtn->column < 0 )
	{
		field_selection_status[rtn->row] = !field_selection_status[rtn->row];
		for( n = 0, j = 0; j < dm->ntime; j++ )
		{
			if(show_existing_depict_only && !dm->is_depict[j]) continue;
			if(dm->field[rtn->row].available[j])
			{
				if(field_selection_status[rtn->row])
					XbaeMatrixSelectCell(w, rtn->row, n);
				else
					XbaeMatrixDeselectCell(w, rtn->row, n);
			}
			else
			{
				XbaeMatrixDeselectCell(w, rtn->row, n);
			}
			n++;
		}
	}
	/* Column label callback */
	else if( rtn->row < 0 )
	{
		for( n = 0, j = 0; j < dm->ntime; j++ )
		{
			if( n == rtn->column ) break;
			if(show_existing_depict_only && !dm->is_depict[j]) continue;
			n++;
		}
		time_selection_status[n] = !time_selection_status[n];
		for( i = 0; i < dm->nfield; i++ )
		{
			if(dm->field[i].available[j])
			{
				if(time_selection_status[n])
					XbaeMatrixSelectCell(w, i, n);
				else
					XbaeMatrixDeselectCell(w, i, n);
			}
			else
			{
				XbaeMatrixDeselectCell(w, i, n);
			}
		}
	}
	/* Cell callback */
	else
	{
		for( n = 0, j = 0; j < dm->ntime; j++ )
		{
			if(show_existing_depict_only && !dm->is_depict[j]) continue;
			if(rtn->column == n)
			{
				if(dm->field[rtn->row].available[j])
				{
					if(XbaeMatrixIsCellSelected(w, rtn->row, rtn->column))
						XbaeMatrixDeselectCell(w, rtn->row, rtn->column);
					else
						XbaeMatrixSelectCell(w, rtn->row, rtn->column);
				}
				else
				{
					XbaeMatrixDeselectCell(w, rtn->row, rtn->column);
				}
				break;
			}
			n++;
		}
	}
}


/*ARGSUSED*/
/*
 * Note that the logic in this function is designed so that the field_update_source_data
 * function is only called when really necessary.
 */
static Boolean field_update(Boolean notused)
{
	if(NotNull(sources) && NotNull(dialog))
	{
		int n, nsrc;
		SourceList src;

		/* If a field update is being processed then return false so that this
		 * function will be called again.
		 */
		if (processing) return False;

		SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED|SRC_LAST_RUN_DATA, FpaC_NORMAL, &src, &nsrc);
		if(navailable_sources != nsrc)
		{
			field_update_source_data(False);
		}
		else
		{
			for(n = 0; n < nsources; n++)
			{
				int i;
				/*
				 * Check to make sure that the list of sources just obtained agrees with
				 * the existing source availability.
				 */
				for(i = 0; i < nsrc; i++)
				{
					if(sources[n].src == src[i] && !sources[n].available)
					{
						field_update_source_data(False);
						return True;
					}
				}
				if(!sources[n].available) continue;
				if(!sources[n].src->modified) continue;
				field_update_source_data(False);
				return True;
			}
		}
	}
	return True;
}


/* Called to check on changes to the sources.
*/
static void field_update_source_data(Boolean show_matrix )
{
	int i, j, k, n, num_src;
	String *list, ptr;
	SourceList src;
	FLD_DESCRIPT fd;
	DataMatrixStruct *mptr;
	Boolean new_run_time, found;
	PARM     *setup;
	INFOFILE infofile;

	if(IsNull(dialog)) return;

	updating = True;
	if (!processing)
	{
		XtSetSensitive(updateBtn, False);
		XtSetSensitive(closeBtn, False);
		XmUpdateDisplay(closeBtn);
	}

	/* Check to see if we have a run time different from the
	*  last run time for the active source.
	*/
	copy_fld_descript(&fd, selected_source->src->fd);

	if((n = source_run_time_list(&fd, &list)) > 0)
	{
		new_run_time = !same(list[0], selected_source->run_time);
		(void) safe_strcpy(selected_source->run_time, list[0]);
		mptr = make_data_matrix();
	}
	else if(valid_tstamp(selected_source->run_time))
	{
		/* If this is the case the active source has no data */
		new_run_time = True;
		selected_source->run_time[0] = '\0';
		for(n = 0; n < nsources; n++)
		{
			if(!sources[n].available) continue;
			selected_source = sources + n;
			break;
		}
		XuMenuSelectItem(sourceSelect, 0);
		mptr = make_data_matrix();
	}
	else
	{
		selected_source->run_time[0] = '\0';
		mptr = make_data_matrix();
		new_run_time = !same_data_matrix(mptr, dm);
	}
	(void)source_run_time_list_free(&list, n);

	/* If we have a new run time for the active source we must 
	 * reset the information as it is no longer valid.
	 */
	if(new_run_time && NotNull(dm))
	{
		for(i = 0; i < dm->nfield; i++)
		{
			for(j = 0; j < dm->ntime; j++)
			{
				if(dm->field[i].src_from[j] == selected_source->refndx)
					dm->field[i].src_from[j] = (Byte)0;
			}
		}
	}

	/* Has the number of valid sources changed? 
	*/
	SourceListByType(SRC_FPA|SRC_NWP|SRC_ALLIED|SRC_LAST_RUN_DATA, FpaC_NORMAL, &src, &num_src);
	if(navailable_sources != num_src)
	{
		int active = -1;

		/* Reset the source availablity and the run times.
		*/
		for(i = 0; i < nsources; i++)
		{
			sources[i].available = False;
			for(k = 0; k < num_src; k++)
			{
				if(sources[i].src != src[k]) continue;
				sources[i].available = True;
				sources[i].run_time[0] = '\0';
				n = source_run_time_list(sources[i].src->fd, &list);
				if(n > 0) (void) safe_strcpy(sources[i].run_time, list[0]);
				if(selected_source == &sources[i]) active = i;
				break;
			}
		}
		selected_source = &sources[active];

		/* Update the dialog if active */
		if (dialog)
		{
			for(i = 0; i < nsources; i++)
			{
				XtSetSensitive(src_btn[i], sources[i].available);
				if(active < 0 && sources[i].available) active = i;
			}
			XuMenuSelectItem(sourceSelect, active);
		}
		navailable_sources = num_src;
	}

	/* We regenerate the matrix buttons if the data has changed.
	*/

	if(show_matrix || !same_data_matrix(mptr, dm))
	{
		copy_from_src_data(mptr, dm);
		data_matrix_free(dm);
		dm = mptr;
		update_matrix();
	}
	else
	{
		data_matrix_free(mptr);
	}

	/* Do we have a matching office default selection entry?
	 */
	found = False;
	setup = GetSetupKeyParms(PRESET_LISTS, UPDATE_FIELDS_OFFICE_DEFAULT_FILE);
	ptr = get_file(PRESET_LISTS, NotNull(setup) ? setup->parm[1] : UPDATE_FIELDS_OFFICE_DEFAULT_FILE);
	infofile = info_file_open(ptr);
	while(!blank(ptr = info_file_find_next_block(infofile)))
	{
		Source s = FindSourceByName(ptr, NULL);
		if((found = (s != NULL && s == selected_source->src))) break;
	}
	info_file_close(infofile);

	if (!processing)
	{
		XtSetSensitive(updateBtn, True);
		XtSetSensitive(closeBtn, True);
	}
	XtSetSensitive(selectPresetBtn, found);
	updating = False;
}


static void update_matrix(void)
{
	int i, j, n;
	String *row_labels = NULL;
	XmString *col_labels = NULL;
	Pixel *bga = NULL;
	Pixel *fga = NULL;
	Pixel  bg = 0;

	if(IsNull(dialog) || IsNull(dm)) return;

	FreeItem(field_selection_status);
	FreeItem(time_selection_status);

	if(dm->nfield > 0)
	{
		row_labels = NewStringArray(dm->nfield);
		field_selection_status = NewMem(Boolean, dm->nfield);
		for( i = 0; i < dm->nfield; i++ )
			row_labels[i] = dm->field[i].fptr->label;
	}

	/* Label and manage the time selection buttons.
	*/
	if(dm->ntime > 0)
	{
		col_labels = NewXmStringArray(dm->ntime);
		bga = NewMem(Pixel, dm->ntime);
		fga = NewMem(Pixel, dm->ntime);
		time_selection_status = NewMem(Boolean, dm->ntime);
	}
	for(n = 0, i = 0; i < dm->ntime; i++ )
	{
		Pixel fg;
		if(show_existing_depict_only && !dm->is_depict[i]) continue;
		GetSequenceBtnColor(dm->time[i], &fg, &bg);
		col_labels[n] = XmStringSequenceTime(dm->time[i], SEQUENCE_HOUR_FONT, SEQUENCE_MINUTE_FONT);
		fga[n] = fg;
		bga[n] = dm->is_depict[i]? bg:no_field_colour;
		n++;
	}

	XbaeMatrixDeselectAll(fieldMatrix);

	XtVaSetValues(fieldMatrix,
		XmNrows, dm->nfield,
		XmNrowLabels, row_labels,
		XmNcolumns, n,
		XmNxmColumnLabels, col_labels,
		XmNcolumnLabelForegrounds, fga,
		XmNcolumnLabelBackgrounds, bga,
		NULL);

	FreeItem(fga);
	FreeItem(bga);
	FreeItem(row_labels);
	XmStringArrayFree(col_labels, n);

	/* Label and manage the field-time selection matrix.
	*/
	XtVaGetValues(fieldMatrix, XmNbackground, &bg, NULL);
	for( i = 0; i < dm->nfield; i++ )
	{
		for( n = 0, j = 0; j < dm->ntime; j++ )
		{
			char mbuf[256];

			if(show_existing_depict_only && !dm->is_depict[j]) continue;

			if(!dm->field[i].in_depict[j]) dm->field[i].src_from[j] = 0;

			XbaeMatrixSetCellTag(fieldMatrix, i, n, SEQUENCE_MINUTE_FONT);

			if(dm->field[i].src_from[j] != 0)
				(void) snprintf(mbuf, sizeof(mbuf), "%c%d", updated_char, (int) dm->field[i].src_from[j]);
			else
				(void) safe_strcpy(mbuf, " ");
			XbaeMatrixSetCell(fieldMatrix, i, n, mbuf);

			if(dm->field[i].available[j])
				XbaeMatrixSetCellBackground(fieldMatrix, i, n, dm->field[i].in_depict[j]? exists_colour:no_field_colour);
			else
				XbaeMatrixSetCellBackground(fieldMatrix, i, n, bg);
			n++;
		}
	}

	XbaeMatrixResizeColumnsToCells(fieldMatrix, True);
	XuUpdateDisplay(dialog);
}


/* Create a new data matrix from the active source.
*/
static DataMatrixStruct *make_data_matrix(void)
{
	int i, j, nlist, nfield_time;
	char mbuf[256];
	String *list, *field_time;
	DataMatrixStruct *m;
	FLD_DESCRIPT fd;

	m = OneMem(DataMatrixStruct);

	/* Merge the depiction time list and the source valid time list.
	*  First copy in the depiction list.
	*/
	if(GV_ndepict > 0)
	{
		m->ntime = GV_ndepict;
		m->time = strlistdup(GV_ndepict, GV_depict);
		m->is_depict = NewBooleanArray(m->ntime);
		for(i = 0; i < m->ntime; i++) m->is_depict[i] = True;
	}

	/* Put the source valid time into our sequence list in ascending order.
	*/
	copy_fld_descript(&fd, selected_source->src->fd);
	nlist = FilteredValidTimeList(&fd, FpaC_NORMAL, &list);
	for(i = 0; i < nlist; i++)
	{
		if(InTimeList(list[i], m->time, m->ntime, NULL)) continue;
		m->time = MoreStringArray(m->time, m->ntime+1);
		m->is_depict = MoreBooleanArray(m->is_depict, m->ntime+1);
		for(j = m->ntime; j > 0; j--)
		{
			if(strcmp(m->time[j-1], list[i]) < 0) break;
			m->time[j] = m->time[j-1];
			m->is_depict[j] = m->is_depict[j-1];
		}
		m->time[j] = XtNewString(list[i]);
		m->is_depict[j] = False;
		m->ntime++;
	}
	(void)FilteredValidTimeListFree(&list, nlist);

	/* Construct the field data structures.
	*/
	for(i = 0; i < GV_nfield; i++)
	{
		if(GV_field[i]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
		m->field = MoreMem(m->field, FIStruct, m->nfield+1);
		m->field[m->nfield].fptr = GV_field[i]->info;
		m->field[m->nfield].no = m->nfield;
		m->field[m->nfield].available = NewBooleanArray(m->ntime);
		m->field[m->nfield].in_depict = NewBooleanArray(m->ntime);
		m->field[m->nfield].src_from  = NewMem(Byte, m->ntime);
		m->nfield++;
	}

	/* Is the field in the depiction sequence?
	*/
	for(i = 0; i < m->nfield; i++)
	{
		(void) snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s",
			m->field[i].fptr->element->name, m->field[i].fptr->level->name);
		(void) GEStatus(mbuf, &nfield_time, &field_time, NULL, NULL);
		for(j = 0; j < m->ntime; j++)
		{
			m->field[i].in_depict[j] = InTimeList(m->time[j],field_time,nfield_time,NULL);
		}
	}

	/* Is the field available? If we get a valid time list check this. If
	*  not the field may be calculated and we need to check it against the
	*  complete source valid time list.
	*/
	for(i = 0; i < m->nfield; i++)
	{
		if(!set_fld_descript(&fd,
			FpaF_ELEMENT_NAME, m->field[i].fptr->element->name,
			FpaF_LEVEL_NAME, m->field[i].fptr->level->name,
			FpaF_VALID_TIME, NULL,
			FpaF_END_OF_LIST)) continue;
		nlist = FilteredValidTimeList(&fd, FpaC_NORMAL, &list);
		for(j = 0; j < m->ntime; j++)
		{
			m->field[i].available[j] = InTimeList(m->time[j], list, nlist, NULL);
		}
		(void)FilteredValidTimeListFree(&list, nlist);
	}
	return m;
}


static Boolean same_data_matrix(DataMatrixStruct *m1 , DataMatrixStruct *m2 )
{
	int i, j;

	if(IsNull(m1) || IsNull(m2)) return False;
	if(m1->ntime != m2->ntime) return False;
	if(m1->nfield != m2->nfield) return False;

	for(i = 0; i < m1->ntime; i++)
	{
		if(!same(m1->time[i],m2->time[i])) return False;
		for(j = 0; j < m1->nfield; j++)
		{
			if(m1->field[j].fptr != m2->field[j].fptr) return False;
			if(m1->field[j].available[i] != m2->field[j].available[i]) return False;
			if(m1->field[j].in_depict[i] != m2->field[j].in_depict[i]) return False;
		}
	}
	return True;
}


static void copy_from_src_data(DataMatrixStruct *m1 , DataMatrixStruct *m2 )
{
	int i, j, n, *xref;

	if(IsNull(m1) || IsNull(m2)) return;

	xref = NewMem(int, m1->ntime);
	for(i = 0; i < m1->ntime; i++)
	{
		xref[i] = -1;
		if(!m1->is_depict[i]) continue;
		for(j = 0; j < m2->ntime; j++)
		{
			if(!same(m1->time[i], m2->time[j])) continue;
			xref[i] = j;
			break;
		}
	}
	for(i = 0; i < m1->nfield; i++)
	{
		for(j = 0; j < m2->nfield; j++)
		{
			if(m1->field[i].fptr != m2->field[j].fptr) continue;
			for(n = 0; n < m1->ntime; n++)
			{
				if(xref[n] >= 0)
					m1->field[i].src_from[n] = m2->field[j].src_from[xref[n]];
				else
					m1->field[i].src_from[n] = (Byte)0;
			}
		}
	}
	FreeItem(xref);
}


static void data_matrix_free(DataMatrixStruct *matrix )
{
	int i;

	if(IsNull(matrix)) return;

	for(i = 0; i < matrix->nfield; i++)
	{
		FreeItem(matrix->field[i].available);
		FreeItem(matrix->field[i].in_depict);
		FreeItem(matrix->field[i].src_from);
	}
	FreeItem(matrix->field);
	for(i = 0; i < matrix->ntime; i++)
	{
		FreeItem(matrix->time[i]);
	}
	FreeItem(matrix->time);
	FreeItem(matrix->is_depict);
	FreeItem(matrix);
}


/*ARGSUSED*/
static void source_select_cb(Widget w , XtPointer client_data , XtPointer call_data )
{

	XuSetBusyCursor(ON);
	select_all_cb(NULL,NULL,NULL);
	selected_source = &sources[PTR2INT(client_data)];
	field_update_source_data(False);
	XuSetBusyCursor(OFF);
}


/*ARGSUSED*/
static void display_option_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	show_existing_depict_only = XmToggleButtonGetState(w);
	update_matrix();
}


/* Read the UPDATE_FIELDS_OFFICE_DEFAULT_FILE in the PRESET_LISTS directory and select
 * all fields listed in the file in the update matrix.
 */
/*ARGSUSED*/
static void select_from_office_list_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int         i, j, n;
	String      dt, del, fname, ptr, src_name, element, level;
	Source      src;
	PARM       *setup;
	INFOFILE    fd;
	FIELD_INFO *fld;
	
	setup = GetSetupKeyParms(PRESET_LISTS, UPDATE_FIELDS_OFFICE_DEFAULT_FILE);
	fname = get_file(PRESET_LISTS, NotNull(setup) ? setup->parm[1] : UPDATE_FIELDS_OFFICE_DEFAULT_FILE);
	fd = info_file_open(fname);
	if(!fd) return;

	while(!blank(src_name = info_file_find_next_block(fd)))
	{
		src = FindSourceByName(src_name, NULL);
		if(!src) pr_error(module,"Unrecognized source name \"%s\" in file \"%s\"\n", src_name, fname);
		if(!src || src != selected_source->src) continue;

		while(!blank(ptr = info_file_get_next_line(fd)))
		{
			element = strtok_arg(ptr);
			level   = strtok_arg(NULL);
			fld = FindField(element, level);
			if(!fld) pr_error(module, "Unrecognized element and level \"%s %s\" for source \"%s\" in file \"%s\"\n",
					element, level, src_name, fname);
			if(!fld) continue;

			for( i = 0; i < dm->nfield; i++ )
				if(fld->info == dm->field[i].fptr) break;
			if( i >= dm->nfield ) continue;

			while((del = strtok_arg(NULL)))
			{
				char buf[24];
				/*
				 * ParseTimeDeltaString expects a time delta string in the form hours:minutes
				 * and if it gets a single number it intreprets this as minutes. In this case
				 * an input with no ":" is hours so we must ensure that the delta is extended
				 * with zero minutes.
				 */
				if(strlen(del) > 21) continue;
				(void) safe_strcpy(buf, del);
				if(!strchr(buf,':')) (void) safe_strcat(buf,":0");
				dt = ParseTimeDeltaString(buf);
				for( n = 0, j = 0; j < dm->ntime; j++ )
				{
					if(show_existing_depict_only && !dm->is_depict[j]) continue;
					if(dm->field[i].available[j] && MinuteDif(dt, dm->time[j]) == 0)
					{
						XbaeMatrixSelectCell(fieldMatrix, i, n);
						break;
					}
					n++;
				}
			}
		}
		break;
	}
	info_file_close(fd);
}


/*ARGSUSED*/
static void select_all_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int i, j, n;
	Boolean status = PTR2BOOL(client_data);

	for( i = 0; i < dm->nfield; i++ )
		field_selection_status[i] = status;

	for( n = 0, j = 0; j < dm->ntime; j++ )
	{
		if(show_existing_depict_only && !dm->is_depict[j]) continue;
		time_selection_status[n] = status;
		for( i = 0; i < dm->nfield; i++ )
		{
			if(dm->field[i].available[j])
			{
				if(status)
					XbaeMatrixSelectCell(fieldMatrix, i, n);
				else
					XbaeMatrixDeselectCell(fieldMatrix, i, n);
			}
			else
			{
				XbaeMatrixDeselectCell(fieldMatrix, i, n);
			}
		}
		n++;
	}
}


/* Here we do the actual field update.
*/
/*ARGSUSED*/
static void update_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	int        i, j, k, n, nl, *l;
	char       label[100];
	String     notify[1];
	Boolean    **doit = NULL;
	Boolean    sequence_change = False;
	FIELD_INFO *fld;

	processing = True;
	XtSetSensitive(updateBtn, False);
	XtSetSensitive(closeBtn, False);
	XmUpdateDisplay(closeBtn);
	XuSetBusyCursor(ON);
	DeactivateMenu();
	nl = 0;
	l = NewMem(int, dm->nfield);

	/* The XbaeMatrix does not take kindly to messing around with the cells
	 * and then testing for a cell being selected. Thus the code puts the
	 * selection state into a boolean matrix first before going through the
	 * matrix and updating the fields.
	 */
	doit = NewMem(Boolean*, dm->ntime);
	for(n = 0, j = 0; j < dm->ntime; j++)
	{
		if(show_existing_depict_only && !dm->is_depict[j]) continue;
		doit[n] = NewMem(Boolean, dm->nfield);
		for(i = 0; i < dm->nfield; i++)
			doit[n][i] = XbaeMatrixIsCellSelected(fieldMatrix, i, n);
		n++;
	}

	/* Now go through the matrix and input those fields that have been selected.
	 * The state of the cell is changed to reflect this.
	 */
	for(n = 0, j = 0; j < dm->ntime; j++)
	{
		if(show_existing_depict_only && !dm->is_depict[j]) continue;

		for(i = 0; i < dm->nfield; i++)
		{
			if(!doit[n][i]) continue;

			if(!dm->is_depict[j])
			{
				dm->is_depict[j] = CreateDepiction(dm->time[j]);
				if(!dm->is_depict[j]) continue;
				sequence_change = True;
			}

			for(k = 0; k < nl; k++) if(l[k] == i) break;
			if(k == nl) { l[nl] = i; nl++; }

			if(!IngredVaCommand(GE_SEQUENCE, "GET_FIELD %s %s %s %s %s %s %s",
				SrcName(selected_source->src), SrcSubDashName(selected_source->src),
				blank(selected_source->run_time)? "-":selected_source->run_time,
				dm->field[i].fptr->element->name,
				dm->field[i].fptr->level->name,
				dm->time[j], dm->time[j])) continue;

			dm->field[i].src_from[j]  = selected_source->refndx;
			dm->field[i].in_depict[j] = True;
			(void) snprintf(label, sizeof(label), "%c%d", updated_char, (int) dm->field[i].src_from[j]);
			XbaeMatrixDeselectCell(fieldMatrix, i, n);
			XbaeMatrixSetCell(fieldMatrix, i, n, label);
			XbaeMatrixSetCellBackground(fieldMatrix, i, n, exists_colour);
		}
		n++;
	}
	FreeList(doit, n);

	XbaeMatrixResizeColumnsToCells(fieldMatrix, True);

	/* Update the field existance status.
	*/
	for(i = 0; i < nl; i++)
	{
		fld = FindField(dm->field[l[i]].fptr->element->name, dm->field[l[i]].fptr->level->name);
		if (fld) SetFieldExistance(fld);
	}
	FreeItem(l);

	SetT0Depiction(T0_INITIALIZE_NEW_ONLY);
	MakeActiveDepiction(ACTIVE);
	ActivateMenu();
	notify[0] = (String) ((long) sequence_change);
	NotifyObservers(OB_FIELD_AVAILABLE, notify, 1);
	XuSetBusyCursor(OFF);
	XtSetSensitive(updateBtn, True);
	XtSetSensitive(closeBtn, True);
	processing = False;
}


/*ARGSUSED*/
static void show_legend_cb(Widget w , XtPointer unused , XtPointer notused )
{
	int i, n, limit;
	Pixel colours[4];
	Widget rc1, rc2, label;
	static String labels[] = {"depict","fields"};
	static String names[] =
		{"depExists","depNone","existsTxt","noFieldTxt","updateTxt","noneTxt"};

	static Widget legendDialog;
	static XuDialogActionsStruct action_items[] = {
		{"closeBtn",  XuDestroyDialogCB, NULL }
	};

	if (legendDialog) return;

	legendDialog = XuCreateFormDialog(w, "fieldUpdateLegend",
		XmNdialogStyle, XmDIALOG_MODELESS,
		XmNnoResize, True,
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &legendDialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XmNhorizontalSpacing, 14,
		XmNverticalSpacing, 14,
		NULL);

	colours[0] = exists_colour;
	colours[1] = no_field_colour;
	colours[2] = update_colour;
	XtVaGetValues(legendDialog, XmNbackground, &colours[3], NULL);

	rc1 = NullWidget;

	for(n = 0; n < 2; n++)
	{
		label = XmVaCreateManagedLabel(legendDialog, labels[n],
			XmNtopAttachment, (n)? XmATTACH_WIDGET:XmATTACH_FORM,
			XmNtopWidget, rc1,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

		rc1 = XmVaCreateManagedRowColumn(legendDialog, "rc1",
			XmNspacing, 3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 9,
			XmNleftAttachment, XmATTACH_FORM,
			XmNbottomAttachment, (n)? XmATTACH_FORM:XmATTACH_NONE,
			NULL);

		rc2 = XmVaCreateManagedRowColumn(legendDialog, "rc2",
			XmNspacing, 5,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNtopOffset, 9,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, rc1,
			XmNleftOffset, 0,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

		limit = (n)? 4:2;
		for(i = 0; i < limit; i++)
		{
			(void) XmVaCreateManagedLabel(rc1, "  ",
				XmNborderWidth, 1,
				XmNbackground, colours[i],
				NULL);
			(void) XmVaCreateManagedLabel(rc2, names[(n)? i+2:i], NULL);
		}
	}
	XuShowDialog(legendDialog);
}


/*ARGSUSED*/
static void exit_cb(Widget w , XtPointer client_data , XtPointer call_data )
{
	if (!dialog) return;
	if (updating) return;
	if (processing) return;

	FreeItem(field_selection_status);
	FreeItem(time_selection_status);
	XuDestroyDialog(dialog);
	dialog = NullWidget;
}
