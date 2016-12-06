/*========================================================================*/
/*
*	File:		field_atributeDisplayDialog.c
*
*	Purpose:	Displays the attribute-value pairs from a sampled area
*               given the CAL structure for the area. Note that the
*               displayed attributes can be filtered to reduce the number
*               according to a user supplied setup file.
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

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include "global.h"
#include "editor.h"
#include "guidance.h"

#define MODULE       	"ACTIVATE_attributeDisplayDialog"
#define UNRECOGNIZED    "Unrecognized attribute \"%s\" in filter group %s\n"
#define STATE_KEY		"fadd"
#define ELEMENT			GV_active_field->info->element->name
#define LEVEL			GV_active_field->info->level->name


typedef struct {
	String filter;	/* String identifying the filter as found in the preset list file */
	int    nlist;
	String *list;
	String *label;
} FLIST;

typedef struct {
	String     name;
	PANEL_ID   id;
	Widget     dialog;
	Widget     col1, col2;
	int        nattrib;
	WidgetList attrib;
	WidgetList vals;
	int        nflist;
	FLIST      *flist;
	FLIST      *fl;
	int        state_ndx;
	int        state[2];
	CAL        cal;
} DD;

static DD dd[2] = {
	{ "editAttributeSampleDisplay", ELEMENT_EDIT, NULL, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, {0,0}, NULL },
	{ "guidAttributeSampleDisplay", GUIDANCE,     NULL, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, {0,0}, NULL },
};


static void display_attributes(DD *d)
{
	int    i, n, nnames;
	String value;
	String *names;

	if(!d->cal) return;

	XtUnmanageChildren(d->attrib, d->nattrib);
	XtUnmanageChildren(d->vals  , d->nattrib);

	for(n = 0, i = 0; i < d->fl->nlist; i++)
	{
		value = CAL_get_attribute(d->cal, d->fl->list[i]);
		if(CAL_is_value(value))
		{
			XuWidgetPrint(d->attrib[n], "%s:", d->fl->label[i]);
			XuWidgetPrint(d->vals[n], "%s", DateString(value, DEFAULT_FORMAT));
			n++;
		}
	}

	/* If the selected attribute list is "all attributes" display all attributes
	 * including those in the cal structure but not in the list of configured
	 * attributes at the end of the list.
	 */
	if(d->fl == d->flist)
	{
		CAL_get_attribute_names(d->cal, &names, &nnames);
		for(i = 0; i < nnames; i++)
		{
			if(InList(names[i], d->fl->nlist, d->fl->list, NULL)) continue;
			value = CAL_get_attribute(d->cal, names[i]);
			if(CAL_is_value(value))
			{
				if(n >= d->nattrib)
				{
					d->attrib = MoreWidgetArray(d->attrib, d->nattrib+1);
					d->vals   = MoreWidgetArray(d->vals,   d->nattrib+1);
					d->attrib[d->nattrib] = XmCreateLabel(d->col1, "a", NULL, 0);
					d->vals[d->nattrib]   = XmCreateLabel(d->col2, "v", NULL, 0);
					d->nattrib++;
				}
				XuWidgetPrint(d->attrib[n], "%s:", names[i]);
				XuWidgetPrint(d->vals[n], "%s", DateString(value, DEFAULT_FORMAT));
				n++;
			}
		}
		FreeItem(names);
	}

	XtManageChildren(d->attrib, n);
	XtManageChildren(d->vals,   n);
}


/*ARGSUSED*/
static void filter_cb(Widget w, XtPointer client_data, XtPointer cd)
{
	XtPointer ptr;
	DD        *d;

	XtVaGetValues(w, XmNuserData, &ptr, NULL);
	d = (DD *)ptr;
	d->state[d->state_ndx] = PTR2INT(client_data);
	d->fl = d->flist + d->state[d->state_ndx];
	display_attributes(d);
	/* Save to the state file only if not for guidance */
	if(d->id != GUIDANCE)
		XuStateDataSave(STATE_KEY, ELEMENT, LEVEL, d->fl->filter);
}


/*ARGSUSED*/
static void exit_cb(Widget w, XtPointer cld, XtPointer cd)
{
	int n;
	DD  *d = (DD *)cld;
	/*
	 * Free all allocated memory and set the widgets to NULL.
	 */
	for(n = 0; n < d->nflist; n++)
	{
		FreeItem(d->flist[n].filter);
		FreeItem(d->flist[n].list);
		FreeItem(d->flist[n].label);
	}
	FreeItem(d->flist);
	FreeItem(d->attrib);
	FreeItem(d->vals);

	d->nflist  = 0;
	d->nattrib = 0;
	d->attrib  = NullWidgetList;
	d->vals    = NullWidgetList;
	d->dialog  = NullWidget;
	d->cal     = CAL_destroy(d->cal);
	/* 
	 * This dialog will only be visible when in active sampling
	 * mode, thus clear any samples on the map when exiting.
	 */
	(void) IngredCommand(GE_EDIT, "CANCEL");
}


void ClearAttributeDisplayDialogs(void)
{
	int n;
	for( n = 0; n < XtNumber(dd); n++ )
	{
		if(IsNull(dd[n].dialog)) continue;
		XtUnmanageChildren(dd[n].attrib, dd[n].flist[0].nlist);
		XtUnmanageChildren(dd[n].vals  , dd[n].flist[0].nlist);
	}
}


void DEACTIVATE_attributeDisplayDialogs(void)
{
	int n;
	for( n = 0; n < XtNumber(dd); n++ )
		if(NotNull(dd[n].dialog)) XuDestroyDialog(dd[n].dialog);
}


void ACTIVATE_attributeDisplayDialog(String type, CAL cal)
{
	int      n;
	char     mbuf[256];
	String   block, label, ptr, filter_file, state_data, *names, *labels;
	Widget   default_btn, sw, btn, cw, set_btn, filter_select, sep;
	INFOFILE fd;
	DD       *d;
	FpaConfigFieldStruct *fld;
	
	static XuDialogActionsStruct action_items[] = {
		{ "cancelBtn", XuDestroyDialogCB, NULL }
	};


	/* We will not get a valid return from GetGuidanceSampleField() unless the
	*  guidance sample tab panel is in sample mode.
	*/
	if (NotNull(fld = GetGuidanceSampleField()))
	{
		d = &dd[1];
	}
	else
	{
		d   = &dd[0];
		fld = GV_active_field->info;
	}
	d->cal = CAL_destroy(d->cal);
	d->cal = CAL_duplicate(cal);


	if(NotNull(d->dialog))
	{
		display_attributes(d);
		XuShowDialog(d->dialog);
		return;
	}
	else if(same(type,E_LABEL))
	{
		d->state_ndx = 0;
		filter_file  = AREA_SAMPLE_FILTERS_FILE;
		d->nattrib   = fld->element->elem_detail->attributes->nattribs;
		names        = fld->element->elem_detail->attributes->attrib_names;
		labels       = fld->element->elem_detail->attributes->attrib_labels;
	}
	else if(same(type,E_SAMPLE))
	{
		d->state_ndx = 1;
		filter_file  = LABEL_SAMPLE_FILTERS_FILE;
		d->nattrib   = fld->element->elem_detail->attributes->nattribs;
		names        = fld->element->elem_detail->attributes->attrib_names;
		labels       = fld->element->elem_detail->attributes->attrib_labels;
	}
	else
	{
		return;
	}

	/* If we get to here we need to create the dialog for the current field.
	*/
	d->dialog = XuCreateFormDialog(GW_drawingWindow, d->name,
		XmNdialogStyle, XmDIALOG_MODELESS,
		XuNdestroyCallback, exit_cb,
		XuNdestroyData, (XtPointer)d,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		XuNretainGeometry, XuRETAIN_ALL,
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	filter_select = XuVaMenuBuildOption(d->dialog, "filter", NULL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	/* Always include the "All Attributes" filter. We can always override
	*  it later.
	*/
	d->nflist = 1;
	d->flist = OneMem(FLIST);
	d->flist[0].nlist  = d->nattrib;
	d->flist[0].list   = NewStringArray(d->nattrib);
	d->flist[0].label  = NewStringArray(d->nattrib);
	for(n = 0; n < d->nattrib; n++)
	{
		d->flist[0].list[n]  = names[n];
		d->flist[0].label[n] = labels[n];
	}
	default_btn = XuMenuAddButton(filter_select, "default_filter", 0, NoId, filter_cb, 0);
	XtVaSetValues(default_btn, XmNuserData, (XtPointer)d, NULL);
	set_btn = default_btn;

	/* Read the attribute display filter file.
	*/
	(void) snprintf(mbuf, sizeof(mbuf), "%s.%s", fld->element->name, fld->level->name);
	fd = info_file_open(get_file(PRESET_LISTS, filter_file));
	if(NotNull(fd))
	{
		Boolean reuse = False;
		while(!blank(block = info_file_find_next_block(fd)))
		{
			if(same_ic(block,"all_attributes"))
			{
				/* In this special case we want to include all attributes
				*  except for those given in the list.
				*/
				int    nexclude = 0;
				String *exclude = NewStringArray(d->nattrib);

				while(!blank(ptr = info_file_get_next_line(fd)))
				{
					if(InList(ptr, d->nattrib, names, &n))
					{
						exclude[nexclude] = names[n];
						nexclude++;
					}
					else
					{
						pr_warning(MODULE, UNRECOGNIZED, ptr, block);
					}
				}
				d->flist[0].filter = XtNewString(" ");
				d->flist[0].nlist = 0;
				for(n = 0; n < d->nattrib; n++)
				{
					if(InList(names[n], nexclude, exclude, NULL)) continue;
					d->flist[0].list[d->flist[0].nlist]  = names[n];
					d->flist[0].label[d->flist[0].nlist] = labels[n];
					d->flist[0].nlist++;
				}
				FreeItem(exclude);
				if(!blank(label = info_file_get_block_label(fd)))
				{
					XuWidgetLabel(default_btn, label);
				}
			}
			else if(same_ic(block,"all_fields") || same_ic(block,mbuf))
			{
				if(blank(label = info_file_get_block_label(fd))) continue;
				if(!reuse)
				{
					d->flist = MoreMem(d->flist, FLIST, d->nflist+1);
					d->flist[d->nflist].nlist = 0;
					d->flist[d->nflist].list  = NewStringArray(d->nattrib);
					d->flist[d->nflist].label = NewStringArray(d->nattrib);
				}
				d->flist[d->nflist].filter = XtNewString(label);
				while(!blank(ptr = info_file_get_next_line(fd)))
				{
					if(InList(ptr, d->nattrib, names, &n))
					{
						d->flist[d->nflist].list[d->flist[d->nflist].nlist]  = names[n];
						d->flist[d->nflist].label[d->flist[d->nflist].nlist] = labels[n];
						d->flist[d->nflist].nlist++;
					}
					else
					{
						pr_warning(MODULE, UNRECOGNIZED, ptr, block);
					}
				}
				reuse = (d->flist[d->nflist].nlist == 0);
				if(!reuse)
				{
					btn = XuMenuAddButton(filter_select, label, 0, NoId,
							filter_cb, INT2PTR(d->nflist));
					XtVaSetValues(btn, XmNuserData, (XtPointer)d, NULL);
					if(d->state[d->state_ndx] == d->nflist) set_btn = btn;
					d->nflist++;
				}
			}
		}
		info_file_close(fd);
	}
	d->fl = (d->state[d->state_ndx] < d->nflist)? d->flist + d->state[d->state_ndx] : d->flist;

	/* If the filter is not for guidance, get the last filter selected from the 
	 * state file and display this as the default.
	 */
	if(d == dd && XuStateDataGet(STATE_KEY, ELEMENT, LEVEL, &state_data))
	{
		btn = XuMenuFindButtonByName(filter_select, state_data);
		if (btn)
		{
			set_btn = btn;
			for(n = 0; n < d->nflist; n++)
			{
				if(same(state_data,d->flist[n].filter))
					d->fl = d->flist + n;
			}
		}
		FreeItem(state_data);
	}

	/* Set the visible button in the option menu
	 */
	XtVaSetValues(filter_select, XmNmenuHistory, set_btn, NULL);

	sep = XmVaCreateManagedSeparator(d->dialog, "sep",
		XmNorientation, XmHORIZONTAL,
		XmNseparatorType, XmSHADOW_ETCHED_IN,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, filter_select,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);

	sw = XmVaCreateManagedScrolledWindow(d->dialog, "sw",
		XmNscrollingPolicy, XmAUTOMATIC,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sep,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	cw = XmVaCreateManagedForm(sw, "cw",
		XmNhorizontalSpacing, 9,
		XmNverticalSpacing, 9,
		NULL);

	d->col1 = XmVaCreateManagedRowColumn(cw, "col1",
		XmNentryAlignment, XmALIGNMENT_END,
		XmNpacking, XmPACK_COLUMN,
		XmNorientation, XmVERTICAL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	d->col2 = XmVaCreateManagedRowColumn(cw, "col2",
		XmNentryAlignment, XmALIGNMENT_BEGINNING,
		XmNpacking, XmPACK_COLUMN,
		XmNorientation, XmVERTICAL,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, d->col1,
		XmNleftOffset, 0,
		NULL);

	d->attrib = NewWidgetArray(d->nattrib);
	d->vals   = NewWidgetArray(d->nattrib);

	for(n = 0; n < d->nattrib; n++)
	{
		d->attrib[n] = XmCreateLabel(d->col1, "a", NULL, 0);
		d->vals[n]   = XmCreateLabel(d->col2, "v", NULL, 0);
	}
	display_attributes(d);
	XuShowDialog(d->dialog);
}
