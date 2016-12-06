/*========================================================================*/
/*
*	File:		depiction_statusDialog.c
*
*   Functions:  ACTIVATE_depictionStatusDialog()
*
*	Purpose:	Shows what fields exist in what depictions in the sequence.
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

#include <string.h>
#include "global.h"
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xbae/Matrix.h>
#include <ingred.h>
#include "resourceDefines.h"
#include "help.h"
#include "timelink.h"

static Widget dialog = NULL;

void ACTIVATE_depictionStatusDialog(Widget w )
{
	int i, j, k, n, m, nfield, maxflds, nspecial;
	char mbuf[300], nbuf[50];
	String *times, *elem, *level;
	XmString *xmRow, *xmColumn;
	Boolean found;
	Pixel colour, black, notlinkable, nolink, partial, linked, fldonly, interp;
	Widget title, topTitle, labelColumn, statusColumn;
	Widget fieldMatrix;

	static int field_type[] = {FpaC_STATIC, FpaC_DAILY};
	static String field_type_id[] = {"staticFields", "dailyFields"};

	static XuDialogActionsStruct action_items[] = {
		{"legendBtn", ShowFieldStatusLegendCB, NULL},
		{"closeBtn",  XuDestroyDialogCB, NULL },
		{"helpBtn",   HelpCB, HELP_DEPICT_STATUS }
	};

	if (dialog) return;

	dialog = XuCreateFormDialog(w, "depictionStatus",
		XuNdestroyCallback, XuExitOnDestroyCallback,
		XuNdestroyData, &dialog,
		XuNactionAreaItems, action_items,
		XuNnumActionAreaItems, XtNumber(action_items),
		NULL);

	notlinkable = XuLoadColorResource(dialog, RNnotLinkableColorBg,"IndianRed");
	nolink      = XuLoadColorResource(dialog, RNnoLinkColorBg,"red");
	partial     = XuLoadColorResource(dialog, RNpartialLinkColorBg,"blue");
	linked      = XuLoadColorResource(dialog, RNalmostColorBg,"yellow");
	fldonly     = XuLoadColorResource(dialog, RNfieldInterpColorBg,"green");
	interp      = XuLoadColorResource(dialog, RNallSetColorBg,"ForestGreen");
	black       = XuLoadColor(dialog, "Black");

	/* First the daily and static fields. Note these are attached to the bottom
	 * of the dialog.
	 */
	nspecial = 0;
	title    = NullWidget;

	for( i = 0; i < XtNumber(field_type); i++)
	{
		/* Check to see is there are any fields to report on.
		*/
		found = False;
		for(j = 0; j < GV_nfield; j++)
		{
			if((found = (GV_field[j]->info->element->elem_tdep->time_dep == field_type[i]))) break;
		}
		if(!found) continue;
		nspecial++;

		labelColumn = XmVaCreateRowColumn(dialog, "labelColumn",
			XmNpacking, XmPACK_COLUMN,
			XmNspacing, 5,
			XmNentryAlignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 9,
			XmNbottomAttachment, (title)? XmATTACH_WIDGET:XmATTACH_FORM,
			XmNbottomWidget, title,
			XmNbottomOffset, 9,
			NULL);

		title = XmVaCreateManagedLabel(dialog, field_type_id[i],
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 9,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, labelColumn,
			XmNbottomOffset, 9,
			NULL);

		maxflds = nfield = 0;
		for(j = 0; j < GV_nfield; j++)
		{
			if( GV_field[j]->info->element->elem_tdep->time_dep != field_type[i]) continue;
			(void) XmVaCreateManagedLabel(labelColumn, GV_field[j]->info->sh_label, NULL);
			nfield++;
			(void) snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s",
				GV_field[j]->info->element->name,
				GV_field[j]->info->level->name);
            (void) GEStatus(mbuf, &k, &times, NULL, NULL);
			maxflds = MAX(maxflds, k);
		}

		statusColumn = XmVaCreateRowColumn(dialog, "statusColumn",
			XmNorientation, XmHORIZONTAL,
			XmNpacking, XmPACK_COLUMN,
			XmNnumColumns, nfield,
			XmNspacing, 3,
			XmNadjustLast, False,
			XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget, labelColumn,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, labelColumn,
			NULL);

		for(j = 0; j < GV_nfield; j++)
		{
			if( GV_field[j]->info->element->elem_tdep->time_dep != field_type[i]) continue;
			(void) snprintf(mbuf, sizeof(mbuf), "FIELD TIMES %s %s",
				GV_field[j]->info->element->name,
				GV_field[j]->info->level->name);
            (void) GEStatus(mbuf, &k, &times, NULL, NULL);
			for(m = 0; m < maxflds; m++)
			{
				if(m < k)
				{
					switch(GV_field[j]->link_status)
					{
						case SOME_LINKS:   colour = partial; break;
						case LINKED:       colour = linked;  break;
						case FIELD_INTERP: colour = fldonly; break;
						case INTERPOLATED: colour = interp;  break;
						default:           colour = linked;  break;
					}
					if(field_type[i] == FpaC_DAILY)
					{
						strcpy(nbuf, DateString(times[m], SHORT_DAY_NAME_NR_OF_MONTH));
					}
					else
					{
						strcpy(nbuf, TimeDiffFormat(GV_T0_depict, times[m], minutes_in_depictions()));
					}
					(void) XmVaCreateManagedLabel(statusColumn, nbuf,
						XmNalignment, XmALIGNMENT_CENTER,
						XmNborderWidth, 1,
						XmNforeground, black,
						XmNbackground, colour,
						NULL);
				}
				else
				{
					(void) XmVaCreateManagedLabel(statusColumn, " ",
						XmNmappedWhenManaged, False,
						XmNborderWidth, 1,
						NULL);
				}
			}
		}
		XtManageChild(labelColumn);
		XtManageChild(statusColumn);
	}


	/* This first part shows the status of the normal depiction fields.
	*/
	xmRow    = NewXmStringArray(GV_nfield);
	xmColumn = NewXmStringArray(GV_ndepict);

	for(nfield = 0, i = 0; i < GV_nfield; i++)
	{
		if(GV_field[i]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
		xmRow[nfield] = XmStringCreateSimple(GV_field[i]->info->sh_label);
		nfield++;
	}

	for(i = 0; i < GV_ndepict; i++)
	{
		xmColumn[i] = XmStringSequenceTime(GV_depict[i], SEQUENCE_HOUR_FONT, SEQUENCE_MINUTE_FONT);
	}
	
	topTitle = XmVaCreateLabel(dialog, "fieldStatus",
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 9,
		NULL);

	fieldMatrix = CreateXbaeMatrix(dialog, "fieldMatrix",
		XmNrows, nfield,
		XmNxmRowLabels, xmRow,
		XmNvisibleRows, nfield,
		XmNcolumns, GV_ndepict,
		XmNxmColumnLabels, xmColumn,
		XmNallowRowResize, False,
		XmNallowColumnResize, False,
		XmNselectScrollVisible, False,
		XmNgridType, XmGRID_CELL_SHADOW,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, topTitle,
		XmNtopOffset, 9,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 29,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, (title)? XmATTACH_WIDGET:XmATTACH_FORM,
		XmNbottomWidget, title,
		XmNbottomOffset, 9,
		NULL);

	XbaeMatrixResizeColumnsToCells(fieldMatrix, True);
	XmStringArrayFree(xmRow,    GV_nfield);
	XmStringArrayFree(xmColumn, GV_ndepict);

	for(i = 0; i < GV_ndepict; i++)
	{
		(void) snprintf(mbuf, sizeof(mbuf), "FIELDS %s", GV_depict[i]);
		(void) GEStatus(mbuf, &nfield, &elem, &level, NULL);
		for( n = -1, j = 0; j < GV_nfield; j++ )
		{
			if(GV_field[j]->info->element->elem_tdep->time_dep != FpaC_NORMAL) continue;
			n++;
			if(GV_field[j]->link_state == LINKED_TO_SPECIAL) continue;
			if(InFieldList(GV_field[j], nfield, elem, level, NULL))
			{
				switch(GV_field[j]->link_status)
				{
					case SOME_LINKS:   colour = partial; break;
					case LINKED:       colour = linked;  break;
					case FIELD_INTERP: colour = fldonly; break;
					case INTERPOLATED: colour = interp;  break;
					case NOT_LINKABLE: colour = notlinkable; break;
					default:           colour = nolink;  break;
				}
				XbaeMatrixSetCellBackground(fieldMatrix, n, i, colour);
			}
		}
	}

	XuShowDialog(dialog);
}
