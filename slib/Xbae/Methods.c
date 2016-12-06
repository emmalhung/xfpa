/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2003, 2004 by the LessTif Developers.
 *
 *                        All rights reserved
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Bellcore not be used in advertising
 * or publicity pertaining to this material without the specific,
 * prior written permission of an authorized representative of
 * Bellcore.
 *
 * BELLCORE MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES, EX-
 * PRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST IN-
 * FRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL BELLCORE OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELAT-
 * ING TO THE SOFTWARE.
 *
 * $Id: Methods.c,v 1.158 2005/06/13 15:22:45 tobiasoed Exp $
 */

/*
 * Methods.c created by Andrew Lister (7 August, 1995)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <X11/Intrinsic.h>
#include <Xm/XmP.h>
#include <Xm/Text.h>
#include <Xm/TextP.h>

#include <Xbae/MatrixP.h>
#include <Xbae/Methods.h>
#include <Xbae/Actions.h>
#include <Xbae/ScrollMgr.h>
#include <Xbae/Utils.h>
#include <Xbae/Shadow.h>
#include <Xbae/Draw.h>
#include <Xbae/Create.h>
#include <Xbae/Input.h>

#include "XbaeDebug.h"

static void AddVarRowsToTable(XbaeMatrixWidget, int, String *, String *, short *, Pixel *, Pixel *, int);
static void DeleteRowsFromTable(XbaeMatrixWidget, int, int);
static void AddColumnsToTable(XbaeMatrixWidget, int, String *, String *, short *, int *,
							  unsigned char *, unsigned char *, Pixel *, Pixel *, int);
static void DeleteColumnsFromTable(XbaeMatrixWidget, int, int);
static void DoEditCell(XbaeMatrixWidget, XEvent *, int, int, String *, Cardinal);
static Boolean DoCommitEdit(XbaeMatrixWidget, XEvent *);

void *xbaeAddValuesToArray(void *values, void *new_values, void *default_value_p, size_t size,
						   int n_values, int n_new_values, int position)
{
	int i;

	if(values)
	{
		/*
		 * The existing items have a value, increase the size to accomodate 
		 * the new items
		 */
		values = XtRealloc(values, size * (n_values + n_new_values));

		/* 
		 * If the new items are to be inserted in the middle, make some space
		 */
		if(position < n_values)
		{
			memmove((char *) values + size * (position + n_new_values),
					(char *) values + size * position, size * (n_values - position));
		}

		if(new_values)
		{
			/* 
			 * The values of the new items are given, copy them
			 */
			memcpy((char *) values + size * position, new_values, size * n_new_values);
		}
		else
		{
			/* 
			 * The values are not given, init the new items to the default
			 */
			for(i = 0; i < n_new_values; i++)
			{
				memcpy((char *) values + size * (position + i), default_value_p, size);
			}
		}
	}
	else if(new_values)
	{
		/*
		 * The existing items have no values but the new ones do. Alloc an array 
		 * big enough for the existing and the new items
		 */
		values = XtMalloc(size * (n_values + n_new_values));

		/*
		 * Init the existing items to the default
		 */
		for(i = 0; i < position; i++)
		{
			memcpy((char *) values + size * i, default_value_p, size);
		}
		for(i = position + n_new_values; i < n_values + n_new_values; i++)
		{
			memcpy((char *) values + size * i, default_value_p, size);
		}
		/* 
		 * Copy the values of the new items
		 */
		memcpy((char *) values + size * position, new_values, size * n_new_values);
	}
	else
	{
		/*
		 * The existing and new items don't have a value, the result is that 
		 * no item has a value, we have nothing to do
		 */
	}

	return values;
}

void *xbaeDeleteValuesFromArray(void *values, size_t size, int n_values, int n_deleted, int position)
{
	if(values)
	{
		if(position + n_deleted < n_values)
		{
			memmove((char *) values + size * position,
					(char *) values + size * (position + n_deleted),
					size * (n_values - position - n_deleted));
		}
		values = XtRealloc(values, size * (n_values - n_deleted));
	}
	return values;
}

/*
 * Add rows to the internal cells data structure.
 * If rows or labels is NULL, add empty rows.
 */
static void
AddVarRowsToTable(XbaeMatrixWidget mw, int position, String * rows, String * labels, short *heights,
				  Pixel * colors, Pixel * backgrounds, int num_rows)
{
	int i, j;

	Boolean default_false = False;
	void *default_user_data = NULL;
	short default_row_height = DEFAULT_ROW_HEIGHT(mw);
	short default_row_shadow_type = 0;
	char *default_label = NULL;
	XmString default_xmlabel = NULL;

	mw->matrix.row_button_labels = xbaeAddValuesToArray(mw->matrix.row_button_labels,
														NULL, &default_false, sizeof default_false,
														mw->matrix.rows, num_rows, position);
	mw->matrix.row_user_data = xbaeAddValuesToArray(mw->matrix.row_user_data,
													NULL, &default_user_data, sizeof default_user_data,
													mw->matrix.rows, num_rows, position);
	mw->matrix.row_shadow_types = xbaeAddValuesToArray(mw->matrix.row_shadow_types,
													   NULL, &default_row_shadow_type,
													   sizeof default_row_shadow_type, mw->matrix.rows,
													   num_rows, position);
	mw->matrix.xmrow_labels =
		xbaeAddValuesToArray(mw->matrix.xmrow_labels, NULL, &default_xmlabel, sizeof default_xmlabel,
							 mw->matrix.rows, num_rows, position);
	mw->matrix.row_labels =
		xbaeAddValuesToArray(mw->matrix.row_labels, labels, &default_label, sizeof default_label,
							 mw->matrix.rows, num_rows, position);

	/*
	 * Copy the labels, not just a pointer to them and recompute row_label_maxwidth
	 */
	if(labels)
	{
		for(i = 0; i < num_rows; i++)
		{
			if(labels[i])
			{
				mw->matrix.row_labels[position + i] = XtNewString(labels[i]);
			}
		}
		mw->matrix.row_label_maxwidth = xbaeCalculateLabelMaxWidth(mw,
																	 mw->matrix.row_labels,
																	 mw->matrix.xmrow_labels,
																	 mw->matrix.rows + num_rows);
	}

	/*
	 * If the user added rows to a matrix without rows and he didn't specify the
	 * heights we need to create the row_heights array
	 */
	if(mw->matrix.rows == 0 && heights == 0)
	{
		mw->matrix.row_heights = (short *) XtMalloc(num_rows * sizeof(short *));
		for(i = 0; i < num_rows; i++)
		{
			mw->matrix.row_heights[i] = default_row_height;
		}
	}
	else
	{
		mw->matrix.row_heights = xbaeAddValuesToArray(mw->matrix.row_heights,
													  heights, &default_row_height, sizeof default_row_height,
													  mw->matrix.rows, num_rows, position);

		/* Don't allow rows smaller than 1 pixel/line */
		if(heights)
		{
			for(i = 0; i < num_rows; i++)
			{
				if(heights[i] < 1)
				{
					mw->matrix.row_heights[position + i] = 1;
				}
			}
		}
	}

	/*
	 * If rows, colors or backgrounds are specified, we need a per cell array
	 */

	if(mw->matrix.per_cell || rows || colors || backgrounds)
	{
		XbaeMatrixPerCellRec *default_per_cell_p = NULL;

		assert(mw->matrix.columns);

		if(mw->matrix.rows == 0)
		{
			mw->matrix.per_cell = (XbaeMatrixPerCellRec **) XtMalloc(num_rows * sizeof *mw->matrix.per_cell);
			for(i = 0; i < num_rows; i++)
			{
				mw->matrix.per_cell[i] = NULL;
			}
		}
		else
		{
			if(!mw->matrix.per_cell)
			{
				xbaeCreatePerCell(mw);
			}
			mw->matrix.per_cell = xbaeAddValuesToArray(mw->matrix.per_cell,
													   NULL, &default_per_cell_p, sizeof default_per_cell_p,
													   mw->matrix.rows, num_rows, position);
		}

		/*
		 * Malloc a new array of per cell records for each new row. 
		 */
		for(i = 0; i < num_rows; i++)
		{
			mw->matrix.per_cell[i + position] =
				(XbaeMatrixPerCellRec *) XtMalloc(mw->matrix.columns * sizeof(XbaeMatrixPerCellRec));
			/*
			 * Copy the rows arrays passed in into each new row, or if NULL
			 * was passed in initialize each row to NULL Strings. Copy the colors
			 * arrays passed in into each new row, if NULL was passed use foreground.
			 */
			for(j = 0; j < mw->matrix.columns; j++)
			{
				xbaeFill_WithEmptyValues_PerCell(mw, &mw->matrix.per_cell[i + position][j]);

				if(colors)
				{
					mw->matrix.per_cell[i + position][j].color = colors[i];
				}
				if(backgrounds)
				{
					mw->matrix.per_cell[i + position][j].background = backgrounds[i];
				}

				if(rows && rows[i * mw->matrix.columns + j])
				{
					mw->matrix.per_cell[i + position][j].CellValue
						= XtNewString(rows[i * mw->matrix.columns + j]);
				}
			}
		}
	}

	mw->matrix.rows += num_rows;

	/*
	 * Recalculate the row positions
	 */
	xbaeFreeRowPositions(mw);
	mw->matrix.row_positions = CreateRowPositions(mw);
	xbaeGetRowPositions(mw);
}

/*
 * Delete rows from the internal cells data structure.
 */
static void DeleteRowsFromTable(XbaeMatrixWidget mw, int position, int num_rows)
{
	int i, j;

	/*
	 * Deselect the rows so num_selected_cells gets updated.
	 * Free the row labels and recalculate row_label_maxwidth.
	 */
	for(i = position; i < position + num_rows; i++)
	{
		xbaeDeselectRow(mw, i);

		if(mw->matrix.row_labels && mw->matrix.row_labels[i])
		{
			XtFree((XtPointer) mw->matrix.row_labels[i]);
		}
		if(mw->matrix.xmrow_labels && mw->matrix.xmrow_labels[i])
		{
			XmStringFree(mw->matrix.xmrow_labels[i]);
		}
	}

	mw->matrix.row_labels = xbaeDeleteValuesFromArray(mw->matrix.row_labels,
													  sizeof *mw->matrix.row_labels,
													  mw->matrix.rows, num_rows, position);
	mw->matrix.xmrow_labels = xbaeDeleteValuesFromArray(mw->matrix.xmrow_labels,
														sizeof *mw->matrix.xmrow_labels,
														mw->matrix.rows, num_rows, position);

	mw->matrix.row_label_maxwidth = xbaeCalculateLabelMaxWidth(mw,
																 mw->matrix.row_labels,
																 mw->matrix.xmrow_labels,
																 mw->matrix.rows - num_rows);

	/*
	 * Free the per cell elements that are allocated
	 */
	if(mw->matrix.per_cell)
	{
		for(i = position; i < position + num_rows; i++)
		{
			for(j = 0; j < mw->matrix.columns; j++)
			{
				/* need to unmanage, unmap any cell widgets that might be visible */
				if(mw->matrix.per_cell[i][j].widget)
				{
					XtUnmanageChild(mw->matrix.per_cell[i][j].widget);
				}
				/* The per-cell free */
				xbaeFreePerCellEntity(mw, i, j);
			}
			XtFree((XtPointer) mw->matrix.per_cell[i]);
		}
		mw->matrix.per_cell = xbaeDeleteValuesFromArray(mw->matrix.per_cell,
														sizeof *mw->matrix.per_cell,
														mw->matrix.rows, num_rows, position);
	}

	/*
	 * Copy those rows which are below the ones deleted, up.
	 * (unless we deleted rows from the bottom).
	 */

	mw->matrix.row_button_labels = xbaeDeleteValuesFromArray(mw->matrix.row_button_labels,
															 sizeof *mw->matrix.row_button_labels,
															 mw->matrix.rows, num_rows, position);
	mw->matrix.row_user_data = xbaeDeleteValuesFromArray(mw->matrix.row_user_data,
														 sizeof *mw->matrix.row_user_data,
														 mw->matrix.rows, num_rows, position);
	mw->matrix.row_shadow_types = xbaeDeleteValuesFromArray(mw->matrix.row_shadow_types,
															sizeof *mw->matrix.row_shadow_types,
															mw->matrix.rows, num_rows, position);
	mw->matrix.row_heights = xbaeDeleteValuesFromArray(mw->matrix.row_heights,
													   sizeof *mw->matrix.row_heights,
													   mw->matrix.rows, num_rows, position);

	mw->matrix.rows -= num_rows;

	/*
	 * Recalculate the row positions
	 */
	xbaeFreeRowPositions(mw);
	mw->matrix.row_positions = CreateRowPositions(mw);
	xbaeGetRowPositions(mw);
}

/*
 * Add columns to the internal cells data structure.
 * If columns or labels is NULL, add empty columns.
 * If max_lengths is NULL, widths will be used.
 * If alignments is NULL, use XmALIGNMENT_BEGINNING.
 * If label_alignments is NULL, use alignments, or if it is NULL
 *   XmALIGNMENT_BEGINNING.
 */
static void
AddColumnsToTable(XbaeMatrixWidget mw, int position, String * columns, String * labels,
				  short *widths, int *max_lengths, unsigned char *alignments,
				  unsigned char *label_alignments, Pixel * colors, Pixel * backgrounds, int num_columns)
{
	int i, j;

	Boolean default_true = True;
	Boolean default_false = False;
	void *default_user_data = NULL;
	short default_column_width = DEFAULT_COLUMN_WIDTH(mw);
	short default_column_shadow_type = 0;
	int default_column_max_length = 0;
	unsigned char default_alignment = XmALIGNMENT_BEGINNING;
	String default_label = NULL;
	XmString default_xmlabel = NULL;

	mw->matrix.column_alignments = xbaeAddValuesToArray(mw->matrix.column_alignments,
														alignments, &default_alignment,
														sizeof default_alignment, mw->matrix.columns,
														num_columns, position);
	mw->matrix.column_label_alignments =
		xbaeAddValuesToArray(mw->matrix.column_label_alignments, label_alignments, &default_alignment,
							 sizeof default_alignment, mw->matrix.columns, num_columns, position);
	mw->matrix.column_font_bold =
		xbaeAddValuesToArray(mw->matrix.column_font_bold, NULL, &default_false, sizeof default_false,
							 mw->matrix.columns, num_columns, position);
	mw->matrix.show_column_arrows =
		xbaeAddValuesToArray(mw->matrix.show_column_arrows, NULL, &default_true, sizeof default_true,
							 mw->matrix.columns, num_columns, position);
	mw->matrix.column_max_lengths =
		xbaeAddValuesToArray(mw->matrix.column_max_lengths, NULL, &default_column_max_length,
							 sizeof default_column_max_length, mw->matrix.columns, num_columns, position);
	mw->matrix.column_button_labels =
		xbaeAddValuesToArray(mw->matrix.column_button_labels, NULL, &default_false, sizeof default_false,
							 mw->matrix.columns, num_columns, position);
	mw->matrix.column_user_data =
		xbaeAddValuesToArray(mw->matrix.column_user_data, NULL, &default_user_data, sizeof default_user_data,
							 mw->matrix.columns, num_columns, position);
	mw->matrix.column_shadow_types =
		xbaeAddValuesToArray(mw->matrix.column_shadow_types, NULL, &default_column_shadow_type,
							 sizeof default_column_shadow_type, mw->matrix.columns, num_columns, position);
	mw->matrix.xmcolumn_labels =
		xbaeAddValuesToArray(mw->matrix.xmcolumn_labels, NULL, &default_xmlabel, sizeof default_xmlabel,
							 mw->matrix.columns, num_columns, position);
	mw->matrix.column_labels =
		xbaeAddValuesToArray(mw->matrix.column_labels, labels, &default_label, sizeof default_label,
							 mw->matrix.columns, num_columns, position);

	/*
	 * Copy the labels, not just a pointer to them and recompute column_label_maxlines
	 */
	if(labels)
	{
		for(i = 0; i < num_columns; i++)
		{
			if(labels[i])
			{
				mw->matrix.column_labels[position + i] = XtNewString(labels[i]);
			}
		}
		mw->matrix.column_label_maxlines = xbaeCalculateLabelMaxLines(mw->matrix.column_labels,
																	  mw->matrix.xmcolumn_labels,
																	  mw->matrix.columns + num_columns);
	}

	/*
	 * If the user added columns to a matrix without columns and he didn't specify the
	 * widths we need to create the column_widths array
	 */
	if(mw->matrix.columns == 0 && widths == 0)
	{
		mw->matrix.column_widths = (short *) XtMalloc(num_columns * sizeof(short *));
		for(i = 0; i < num_columns; i++)
		{
			mw->matrix.column_widths[i] = default_column_width;
		}
	}
	else
	{
		mw->matrix.column_widths = xbaeAddValuesToArray(mw->matrix.column_widths,
														widths, &default_column_width,
														sizeof default_column_width, mw->matrix.columns,
														num_columns, position);

		/* Don't allow columns smaller than 1 pixel/character */
		if(widths)
		{
			for(i = 0; i < num_columns; i++)
			{
				if(widths[i] < 1)
				{
					mw->matrix.column_widths[position + i] = 1;
				}
			}
		}
	}

	/*
	 * If columns, colors or backgrounds are specified, we need a per cell array
	 */
	if(mw->matrix.per_cell || columns || colors || backgrounds)
	{
		XbaeMatrixPerCellRec default_per_cell;
		xbaeFill_WithEmptyValues_PerCell(mw, &default_per_cell);

		assert(mw->matrix.rows);

		if(mw->matrix.columns == 0)
		{
			mw->matrix.per_cell =
				(XbaeMatrixPerCellRec **) XtMalloc(mw->matrix.rows * sizeof *mw->matrix.per_cell);
			for(i = 0; i < mw->matrix.rows; i++)
			{
				mw->matrix.per_cell[i] =
					(XbaeMatrixPerCellRec *) XtMalloc(num_columns * sizeof *mw->matrix.per_cell[i]);
				for(j = 0; j < num_columns; j++)
				{
					mw->matrix.per_cell[i][j] = default_per_cell;
				}
			}
		}
		else
		{
			if(!mw->matrix.per_cell)
			{
				xbaeCreatePerCell(mw);
			}
			for(i = 0; i < mw->matrix.rows; i++)
			{
				mw->matrix.per_cell[i] = xbaeAddValuesToArray(mw->matrix.per_cell[i],
															  NULL, &default_per_cell,
															  sizeof default_per_cell, mw->matrix.columns,
															  num_columns, position);
			}
		}

		/*
		 * Copy all of the passed in info into each new column
		 * (except column_positions which will be recalculated below).
		 * If columns is NULL, add empty columns.
		 * If colors is NULL, use foreground.
		 * Use False for new button label flags.
		 */
		for(i = 0; i < mw->matrix.rows; i++)
		{
			for(j = 0; j < num_columns; j++)
			{

				if(colors)
				{
					mw->matrix.per_cell[i][j + position].color = colors[j];
				}
				if(backgrounds)
				{
					mw->matrix.per_cell[i][j + position].background = backgrounds[j];
				}

				if(columns && columns[i * num_columns + j])
				{
					mw->matrix.per_cell[i][j + position].CellValue
						= XtNewString(columns[i * num_columns + j]);
				}
			}
		}
	}

	mw->matrix.columns += num_columns;

	/*
	 * Recalculate the column positions
	 */
	xbaeFreeColumnPositions(mw);
	mw->matrix.column_positions = CreateColumnPositions(mw);
	xbaeGetColumnPositions(mw);
}

/*
 * Delete columns from the internal cells data structure.
 */
static void DeleteColumnsFromTable(XbaeMatrixWidget mw, int position, int num_columns)
{
	int i, j;

	/*
	 * Deselect the columns so num_selected_cells gets updated.
	 * Free the column labels and recalculate column_label_maxlines.
	 */

	for(j = position; j < position + num_columns; j++)
	{
		xbaeDeselectColumn(mw, j);

		if(mw->matrix.column_labels && mw->matrix.column_labels[j])
		{
			XtFree((XtPointer) mw->matrix.column_labels[j]);
		}
		if(mw->matrix.xmcolumn_labels && mw->matrix.xmcolumn_labels[j])
		{
			XmStringFree(mw->matrix.xmcolumn_labels[j]);
		}
	}

	mw->matrix.column_labels = xbaeDeleteValuesFromArray(mw->matrix.column_labels,
														 sizeof *mw->matrix.column_labels,
														 mw->matrix.columns, num_columns, position);
	mw->matrix.xmcolumn_labels = xbaeDeleteValuesFromArray(mw->matrix.xmcolumn_labels,
														   sizeof *mw->matrix.xmcolumn_labels,
														   mw->matrix.columns, num_columns, position);

	mw->matrix.column_label_maxlines = xbaeCalculateLabelMaxLines(mw->matrix.column_labels,
																  mw->matrix.xmcolumn_labels,
																  mw->matrix.columns - num_columns);

	/*
	 * Free the per cell elements that are allocated
	 */
	if(mw->matrix.per_cell)
	{
		for(i = 0; i < mw->matrix.rows; i++)
		{
			for(j = position; j < position + num_columns; j++)
			{
				/* need to unmanage, unmap any cell widgets that might be visible */
				if(mw->matrix.per_cell[i][j].widget)
				{
					XtUnmanageChild(mw->matrix.per_cell[i][j].widget);
				}

				/* The per-cell free */
				xbaeFreePerCellEntity(mw, i, j);
			}
			mw->matrix.per_cell[i] = xbaeDeleteValuesFromArray(mw->matrix.per_cell[i],
															   sizeof *mw->matrix.per_cell[i],
															   mw->matrix.columns, num_columns, position);
		}
		if(mw->matrix.columns == num_columns)
		{
			XtFree((XtPointer) mw->matrix.per_cell);
			mw->matrix.per_cell = NULL;
		}
	}

	/*
	 * Shift those columns after the ones being deleted, left.
	 * (unless we deleted columns from the right).
	 */

	mw->matrix.column_alignments = xbaeDeleteValuesFromArray(mw->matrix.column_alignments,
															 sizeof *mw->matrix.column_alignments,
															 mw->matrix.columns, num_columns, position);
	mw->matrix.column_label_alignments = xbaeDeleteValuesFromArray(mw->matrix.column_label_alignments,
																   sizeof *mw->matrix.column_label_alignments,
																   mw->matrix.columns, num_columns, position);
	mw->matrix.column_font_bold = xbaeDeleteValuesFromArray(mw->matrix.column_font_bold,
															sizeof *mw->matrix.column_font_bold,
															mw->matrix.columns, num_columns, position);
	mw->matrix.show_column_arrows = xbaeDeleteValuesFromArray(mw->matrix.show_column_arrows,
															  sizeof *mw->matrix.show_column_arrows,
															  mw->matrix.columns, num_columns, position);
	mw->matrix.column_max_lengths = xbaeDeleteValuesFromArray(mw->matrix.column_max_lengths,
															  sizeof *mw->matrix.column_max_lengths,
															  mw->matrix.columns, num_columns, position);
	mw->matrix.column_button_labels = xbaeDeleteValuesFromArray(mw->matrix.column_button_labels,
																sizeof *mw->matrix.column_button_labels,
																mw->matrix.columns, num_columns, position);
	mw->matrix.column_user_data = xbaeDeleteValuesFromArray(mw->matrix.column_user_data,
															sizeof *mw->matrix.column_user_data,
															mw->matrix.columns, num_columns, position);
	mw->matrix.column_shadow_types = xbaeDeleteValuesFromArray(mw->matrix.column_shadow_types,
															   sizeof *mw->matrix.column_shadow_types,
															   mw->matrix.columns, num_columns, position);
	mw->matrix.column_widths = xbaeDeleteValuesFromArray(mw->matrix.column_widths,
														 sizeof *mw->matrix.column_widths,
														 mw->matrix.columns, num_columns, position);

	mw->matrix.columns -= num_columns;

	/*
	 * Recalculate the column positions
	 */
	xbaeFreeColumnPositions(mw);
	mw->matrix.column_positions = CreateColumnPositions(mw);
	xbaeGetColumnPositions(mw);
}

/*
 * Set the contents of the textchild
 */

void xbaeUpdateTextChildFont(XbaeMatrixWidget mw, XrmQuark qtag)
{
	static XrmQuark default_qtag = NULLQUARK;
	if(default_qtag == NULLQUARK)
	{
		default_qtag = XrmUniqueQuark();
	}

	if(mw->matrix.current_text_qtag == NULLQUARK
	   || (qtag == NULLQUARK && mw->matrix.current_text_qtag != default_qtag)
	   || (qtag != NULLQUARK && mw->matrix.current_text_qtag != qtag))
	{

		XmRendition rendition;
		XmRenderTable render_table;

		if(qtag)
		{
			XmStringTag tag = XrmQuarkToString(qtag);
			mw->matrix.current_text_qtag = qtag;
			rendition = XmRenderTableGetRendition(mw->matrix.render_table, tag);
		}
		else
		{
			mw->matrix.current_text_qtag = default_qtag;
			rendition = XmRenderTableGetRendition(mw->matrix.render_table, _MOTIF_DEFAULT_LOCALE);
			if(!rendition)
			{
				rendition = XmRenderTableGetRendition(mw->matrix.render_table, XmFONTLIST_DEFAULT_TAG);
			}
		}
		render_table = XmRenderTableAddRenditions(NULL, &rendition, 1, XmMERGE_NEW);

		XtVaSetValues(TextChild(mw), XmNrenderTable, render_table, NULL);

		XmRenditionFree(rendition);
		XmRenderTableFree(render_table);
	}
}

void xbaeUpdateTextChild(XbaeMatrixWidget mw, Boolean UpdateValue)
{
	XbaeMatrixCellValuesStruct cell_values;

	int row = mw->matrix.current_row;
	int column = mw->matrix.current_column;

	assert(mw->matrix.text_child_is_mapped
		   && row >= 0 && row < mw->matrix.rows && column >= 0 && column < mw->matrix.columns);

	xbaeGetCellValues(mw, row, column, True, &cell_values);

	if(cell_values.drawCB.type & XbaeString)
	{

		XtVaSetValues(TextChild(mw),
					  XmNbackground, cell_values.drawCB.background,
					  XmNforeground, cell_values.drawCB.foreground, NULL);

		if(UpdateValue)
		{
			/* Remove the modify verify callback when the text field is set.
			   It thinks we are modifying the value - Motif thinks that
			   it knows best but we know better! */

			XtRemoveCallback(TextChild(mw), XmNmodifyVerifyCallback, xbaeModifyVerifyCB, (XtPointer) mw);

			XmTextSetString(TextChild(mw), cell_values.drawCB.string);

			XtAddCallback(TextChild(mw), XmNmodifyVerifyCallback, xbaeModifyVerifyCB, (XtPointer) mw);
		}

	}
	else
	{
		xbaeHideTextChild(mw);
	}

	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtFree((XtPointer) cell_values.drawCB.string);
	}
}

/*
 * Matrix set_cell method
 */
void xbaeSetCell(XbaeMatrixWidget mw, int row, int column, String value, Boolean update_text)
{
	Boolean changed = False;

	if(row >= mw->matrix.rows || row < 0 || column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "xbaeSetCell",
						"badIndex", "XbaeMatrix",
						"XbaeMatrix: Row or column out of bounds for xbaeSetCell.", NULL, 0);
		return;
	}

	/*
	 * If we have a draw cell callback, we must have a write cell callback
	 * also if we want to set the data.  Use this callback to write the
	 * new data back to the application.
	 */
	if(mw->matrix.draw_cell_callback)
	{
		XbaeMatrixWriteCellCallbackStruct call_data;
		if(mw->matrix.write_cell_callback)
		{
			call_data.reason = XbaeWriteCellReason;
			call_data.event = (XEvent *) NULL;
			call_data.row = row;
			call_data.column = column;
			call_data.string = value;
			call_data.type = XbaeString;
			call_data.pixmap = (Pixmap) NULL;
			call_data.mask = (Pixmap) NULL;
			XtCallCallbackList((Widget) mw, mw->matrix.write_cell_callback, (XtPointer) & call_data);
		}
		changed = True;
	}
#if 0
	/* To make DrawCellCallback easier, also store the cell values in the widget */
	else
#endif
	{
		/*
		 * Store the new value in the cell.
		 */
		if(!mw->matrix.per_cell && value && value[0] != 0)
			xbaeCreatePerCell(mw);

		if(mw->matrix.per_cell)
		{
			if(mw->matrix.per_cell[row][column].CellValue == NULL)
			{
				if(value && value[0] != 0)
				{
					mw->matrix.per_cell[row][column].CellValue = XtNewString(value);
					changed = True;
				}
			}
			else if(value == NULL || value[0] == 0)
			{
				XtFree((char *) mw->matrix.per_cell[row][column].CellValue);
				mw->matrix.per_cell[row][column].CellValue = NULL;
				changed = True;
			}
			else if(strcmp(mw->matrix.per_cell[row][column].CellValue, value) != 0)
			{
				XtFree((char *) mw->matrix.per_cell[row][column].CellValue);
				mw->matrix.per_cell[row][column].CellValue = XtNewString(value);
				changed = True;
			}
		}
	}

	if(changed)
	{
		/*
		 * Value has changed, draw the cell
		 */
		if(xbaeIsCellVisible(mw, row, column))
		{
			xbaeDrawCell(mw, row, column);
		}

		/*
		 * If we are editing this cell, load the textField too if update_text set.
		 */
		if(update_text
		   && mw->matrix.text_child_is_mapped
		   && mw->matrix.current_row == row && mw->matrix.current_column == column)
		{
			xbaeUpdateTextChild(mw, True);
		}
	}
}

/* 
 * The meat of Matrix commit_edit method. 
 */
static Boolean DoCommitEdit(XbaeMatrixWidget mw, XEvent * event)
{
	String cell;

	if(!mw->matrix.text_child_is_mapped)
		return True;

	/*
	 * Get the value the user entered in the textField (this is a copy)
	 */
	cell = XmTextGetString(TextChild(mw));

	/*
	 * Call the leaveCellCallback to see if we can leave the current cell.
	 */
	if(mw->matrix.leave_cell_callback)
	{
		XbaeMatrixLeaveCellCallbackStruct call_data;

		call_data.reason = XbaeLeaveCellReason;
		call_data.event = event;
		call_data.row = mw->matrix.current_row;
		call_data.column = mw->matrix.current_column;
		call_data.value = cell;
		call_data.doit = True;

		XtCallCallbackList((Widget) mw, mw->matrix.leave_cell_callback, (XtPointer) & call_data);

		/*
		 * Application doesn't want to leave this cell. Make the cell visible
		 * and traverse to it so the user can see where they screwed up.
		 */
		if(!call_data.doit)
		{
			xbaeMakeCellVisible(mw, mw->matrix.current_row, mw->matrix.current_column);
			XmProcessTraversal(TextChild(mw), XmTRAVERSE_CURRENT);
			XtFree((char *) cell);
			return False;
		}

		/*
		 * Use the applications value if it is different.
		 * If the application modified the string inplace, we will pick that
		 * up automatically.
		 */
		if(call_data.value != cell)
		{
			XtFree((char *) cell);
			cell = call_data.value;
		}
	}

	/*
	 * Call the set_cell method to store the new value in the cell and redraw.
	 */
	((XbaeMatrixWidgetClass) XtClass(mw))->matrix_class.set_cell(mw, mw->matrix.current_row,
																 mw->matrix.current_column, cell, True);

	XtFree((char *) cell);

	return True;
}

/*
 * Position and size the scrollbars and clip widget for our size.
 */
void xbaeRelayout(XbaeMatrixWidget mw)
{
	Boolean has_horiz, has_vert;
	Boolean scrollbar_top = SCROLLBAR_TOP(mw);
	Boolean scrollbar_left = SCROLLBAR_LEFT(mw);
	int non_fixed_rows = mw->matrix.rows - mw->matrix.fixed_rows - mw->matrix.trailing_fixed_rows;
	int non_fixed_columns = mw->matrix.columns - mw->matrix.fixed_columns - mw->matrix.trailing_fixed_columns;
	int width = mw->core.width;
	int height = mw->core.height;
	int full_width = TOTAL_WIDTH(mw) + ROW_LABEL_WIDTH(mw) + 2 * mw->manager.shadow_thickness;
	int full_height = TOTAL_HEIGHT(mw) + COLUMN_LABEL_HEIGHT(mw) + 2 * mw->manager.shadow_thickness;

	int column_label_height = COLUMN_LABEL_HEIGHT(mw);
	int row_label_width = ROW_LABEL_WIDTH(mw);

	int fixed_row_position, non_fixed_row_position, trailing_fixed_row_position;
	int fixed_column_position, non_fixed_column_position, trailing_fixed_column_position;

	/* 
	 * We are trying to figure these out so we can't use the macros here. 
	 */
	mw->matrix.visible_fixed_row_height = FIXED_ROW_HEIGHT(mw);
	mw->matrix.visible_fixed_column_width = FIXED_COLUMN_WIDTH(mw);
	mw->matrix.visible_trailing_fixed_column_width = TRAILING_FIXED_COLUMN_WIDTH(mw);
	mw->matrix.visible_trailing_fixed_row_height = TRAILING_FIXED_ROW_HEIGHT(mw);

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "xbaeRelayout(wid %d ht %d), fullw %d fullh %d\n",
			  mw->core.width, mw->core.height, full_width, full_height));

	/*
	 * If our horizontal scrollbar display policy is constant,
	 * then we always have the horizontal scrollbar. If it is
	 * none, then we never have it. Otherwise, check if it
	 * is needed: if we are wider than the matrix's width,
	 * then we don't need it; if we are smaller, we do.
	 */
	if(mw->matrix.hsb_display_policy == XmDISPLAY_NONE || non_fixed_columns == 0)
	{
		has_horiz = FALSE;
	}
	else if(mw->matrix.hsb_display_policy == XmDISPLAY_STATIC)
	{
		has_horiz = TRUE;
	}
	else
	{
		if(width >= full_width)
			has_horiz = False;
		else
			has_horiz = True;
	}
	if(has_horiz)
		height -= HORIZ_SB_HEIGHT(mw);

	/*
	 * Same reasoning for the vertical scrollbar.
	 */
	if(mw->matrix.vsb_display_policy == XmDISPLAY_NONE || non_fixed_rows == 0)
	{
		has_vert = FALSE;
	}
	else if(mw->matrix.vsb_display_policy == XmDISPLAY_STATIC)
	{
		has_vert = TRUE;
	}
	else
	{
		if(height >= full_height)
			has_vert = False;
		else
			has_vert = True;
	}

	/*
	 * If we have a vertical scrollbar, adjust the width and
	 * recheck if we need the horizontal scrollbar.
	 */
	if(has_vert)
	{
		width -= VERT_SB_WIDTH(mw);
		if(!has_horiz && width < full_width
		   && mw->matrix.hsb_display_policy != XmDISPLAY_NONE && non_fixed_columns > 0)
		{
			has_horiz = True;
			height -= HORIZ_SB_HEIGHT(mw);
		}
	}

	/*
	 * If widget is smaller than full size, move/resize the scrollbar and
	 * set sliderSize, also if visible_non_fixed_width/visible_non_fixed_height is greater than
	 * the amount of cell area visible, then we need to drag the cells
	 * back into the visible part of the clip widget and set the
	 * scrollbar value.
	 *
	 * Otherwise, the widget is larger than full size, so set
	 * visible_non_fixed_width/visible_non_fixed_height to size of cells and set origin to 0
	 * to force full cell area to be displayed
	 *
	 * We also need to move the textField correspondingly
	 */

	DEBUGOUT(_XbaeDebug
			 (__FILE__, (Widget) mw, "xbaeRelayout(), w=%d, fullw=%d, h=%d, fullh=%d\n", width,
			  full_width, height, full_height));

	if(width < full_width)
	{
		/*
		 * We were resized smaller than our max width
		 */
		mw->matrix.visible_non_fixed_width =
			width - (FIXED_COLUMN_WIDTH(mw) + TRAILING_FIXED_COLUMN_WIDTH(mw) + row_label_width +
					 2 * mw->manager.shadow_thickness);

		if(mw->matrix.visible_non_fixed_width <= 0)
		{
			mw->matrix.visible_non_fixed_width = 0;
			HORIZ_ORIGIN(mw) = 0;
		}
		else if(HORIZ_ORIGIN(mw) + VISIBLE_NON_FIXED_WIDTH(mw) > NON_FIXED_WIDTH(mw))
		{
			/*
			 * If the cells are scrolled off to the left, then drag them
			 * back onto the screen.
			 */
			HORIZ_ORIGIN(mw) = NON_FIXED_WIDTH(mw) - VISIBLE_NON_FIXED_WIDTH(mw);
		}
	}
	else
	{
		/*
		 * We were resized larger than our max width.
		 */
		mw->matrix.visible_non_fixed_width = NON_FIXED_WIDTH(mw);

		if(mw->matrix.fill)
		{
			int empty_width = width - full_width;
			if(mw->matrix.non_fixed_detached_left && mw->matrix.fixed_columns)
			{
				mw->matrix.visible_fixed_column_width += empty_width;
			}
			else if(mw->matrix.trailing_fixed_columns == 0 || mw->matrix.trailing_attached_right)
			{
				mw->matrix.visible_non_fixed_width += empty_width;
			}
			else
			{
				mw->matrix.visible_trailing_fixed_column_width += empty_width;
			}
		}

		DEBUGOUT(_XbaeDebug2
				 (__FILE__, (Widget) mw, ClipChild(mw), "YOW visible_non_fixed_width = %d\n",
				  mw->matrix.visible_non_fixed_width));

		if(mw->matrix.visible_non_fixed_width <= 0)
		{
			mw->matrix.visible_non_fixed_width = 0;
		}

		/* 
		 * Drag the cells back onto the screen if they were scrolled off to the left.
		 */
		HORIZ_ORIGIN(mw) = 0;
	}

	if(HorizScrollChild(mw))
	{
		/*
		 * Setup the HSB to reflect our new size.
		 */

		int hsb_x;
		int hsb_y;
		int hsb_width;
		int slider_size = Min(NON_FIXED_WIDTH(mw), VISIBLE_NON_FIXED_WIDTH(mw));

		XtVaSetValues(HorizScrollChild(mw),
					  XmNpageIncrement, slider_size ? slider_size : 1,
					  XmNsliderSize, slider_size ? slider_size : 1,
					  XmNvalue, HORIZ_ORIGIN(mw),
					  XmNmaximum, NON_FIXED_WIDTH(mw) ? NON_FIXED_WIDTH(mw) : 1, NULL);

		/*
		 * If the window is not full height, then place the HSB at the edge
		 * of the window.  If the window is larger than full height, then
		 * place the HSB immediately below the cell region.
		 */

		hsb_x = row_label_width;
		if(has_vert && scrollbar_left)
		{
			hsb_x += VERT_SB_WIDTH(mw);
		}
		if(mw->matrix.fixed_columns)
		{
			hsb_x += VISIBLE_FIXED_COLUMN_WIDTH(mw) + mw->manager.shadow_thickness;
		}

		if(scrollbar_top)
		{
			hsb_y = 0;
		}
		else if(height < full_height || mw->matrix.fill)
		{
			hsb_y = mw->core.height - HorizScrollChild(mw)->core.height
				- 2 * HorizScrollChild(mw)->core.border_width;
		}
		else
		{
			hsb_y = full_height + mw->matrix.space;
		}

		if(mw->matrix.fill)
		{
			hsb_width = VISIBLE_NON_FIXED_WIDTH(mw);
		}
		else
		{
			hsb_width = Min(NON_FIXED_WIDTH(mw), VISIBLE_NON_FIXED_WIDTH(mw));
		}

		if(!has_horiz || hsb_width == 0)
		{
			/* 
			 * Unmapp the scrollbar if it is mapped 
			 */
			if(XtIsManaged(HorizScrollChild(mw)))
			{
				XtUnmanageChild(HorizScrollChild(mw));
			}
		}
		else
		{
			hsb_width += mw->manager.shadow_thickness
				* ((mw->matrix.fixed_columns == 0) + (mw->matrix.trailing_fixed_columns == 0));

			XtConfigureWidget(HorizScrollChild(mw),
							  hsb_x, hsb_y, hsb_width, HorizScrollChild(mw)->core.height,
							  HorizScrollChild(mw)->core.border_width);

			/* 
			 * Map the scrollbar if it is not already mapped 
			 */
			if(!XtIsManaged(HorizScrollChild(mw)))
			{
				XtManageChild(HorizScrollChild(mw));
			}
		}
	}

	/*
	 * We were resized smaller than our max height.
	 */
	if(height < full_height)
	{
		/*
		 * We were resized smaller than our max height
		 */
		mw->matrix.visible_non_fixed_height = height - (FIXED_ROW_HEIGHT(mw) + TRAILING_FIXED_ROW_HEIGHT(mw) +
														column_label_height +
														2 * mw->manager.shadow_thickness);

		if(mw->matrix.visible_non_fixed_height <= 0)
		{
			mw->matrix.visible_non_fixed_height = 0;
			VERT_ORIGIN(mw) = 0;
		}
		else if(VERT_ORIGIN(mw) + VISIBLE_NON_FIXED_HEIGHT(mw) > NON_FIXED_HEIGHT(mw))
		{
			/*
			 * If the cells are scrolled off the top, then drag them
			 * back onto the screen.
			 */
			VERT_ORIGIN(mw) = NON_FIXED_HEIGHT(mw) - VISIBLE_NON_FIXED_HEIGHT(mw);
		}
	}
	else
	{
		/*
		 * We were resized larger than our max height.
		 */
		mw->matrix.visible_non_fixed_height = NON_FIXED_HEIGHT(mw);

		if(mw->matrix.fill)
		{
			int empty_height = height - full_height;
			if(mw->matrix.non_fixed_detached_top && mw->matrix.fixed_rows)
			{
				mw->matrix.visible_fixed_row_height += empty_height;
			}
			else if(mw->matrix.trailing_fixed_rows == 0 || mw->matrix.trailing_attached_bottom)
			{
				mw->matrix.visible_non_fixed_height += empty_height;
			}
			else
			{
				mw->matrix.visible_trailing_fixed_row_height += empty_height;
			}
		}

		DEBUGOUT(_XbaeDebug2
				 (__FILE__, (Widget) mw, ClipChild(mw), "YOW visible_non_fixed_height = %d\n",
				  mw->matrix.visible_non_fixed_height));

		if(mw->matrix.visible_non_fixed_height <= 0)
		{
			mw->matrix.visible_non_fixed_height = 0;
		}

		/*
		 * Drag the cells back onto the screen if they were scrolled off the top.
		 */
		VERT_ORIGIN(mw) = 0;
	}

	if(VertScrollChild(mw))
	{
		/*
		 * Setup the VSB to reflect our new size.
		 */
		int vsb_x;
		int vsb_y;
		int vsb_height;
		int slider_size = Min(NON_FIXED_HEIGHT(mw), VISIBLE_NON_FIXED_HEIGHT(mw));

		XtVaSetValues(VertScrollChild(mw),
					  XmNpageIncrement, slider_size ? slider_size : 1,
					  XmNsliderSize, slider_size ? slider_size : 1,
					  XmNvalue, VERT_ORIGIN(mw),
					  XmNmaximum, NON_FIXED_HEIGHT(mw) ? NON_FIXED_HEIGHT(mw) : 1, NULL);

		/*
		 * If the window is not full width, then place the VSB at the edge
		 * of the window.  If the window is larger than full width, then
		 * place the VSB immediately to the right of the cell region.
		 */

		vsb_y = column_label_height;
		if(has_horiz && scrollbar_top)
		{
			vsb_y += HORIZ_SB_HEIGHT(mw);
		}
		if(mw->matrix.fixed_rows)
		{
			vsb_y += VISIBLE_FIXED_ROW_HEIGHT(mw) + mw->manager.shadow_thickness;
		}

		if(scrollbar_left)
		{
			vsb_x = 0;
		}
		else if(width < full_width || mw->matrix.fill)
		{
			vsb_x = mw->core.width - VertScrollChild(mw)->core.width
				- 2 * VertScrollChild(mw)->core.border_width;
		}
		else
		{
			vsb_x = full_width + mw->matrix.space;
		}

		if(mw->matrix.fill)
		{
			vsb_height = VISIBLE_NON_FIXED_HEIGHT(mw);
		}
		else
		{
			vsb_height = Min(NON_FIXED_HEIGHT(mw), VISIBLE_NON_FIXED_HEIGHT(mw));
		}

		if(!has_vert || vsb_height == 0)
		{
			/* 
			 * Unmapp the scrollbar if it is mapped 
			 */
			if(XtIsManaged(VertScrollChild(mw)))
			{
				XtUnmanageChild(VertScrollChild(mw));
			}
		}
		else
		{
			vsb_height += mw->manager.shadow_thickness
				* ((mw->matrix.fixed_rows == 0) + (mw->matrix.trailing_fixed_rows == 0));

			XtConfigureWidget(VertScrollChild(mw),
							  vsb_x, vsb_y, VertScrollChild(mw)->core.width, vsb_height,
							  VertScrollChild(mw)->core.border_width);

			/* 
			 * Map the scrollbar if it is not already mapped 
			 */
			if(!XtIsManaged(VertScrollChild(mw)))
			{
				XtManageChild(VertScrollChild(mw));
			}
		}
	}

	/*
	 * Now that we have all the widths and heights, we can calculate the positions
	 */

	fixed_row_position = HORIZ_SB_OFFSET(mw) + column_label_height + mw->manager.shadow_thickness;
	non_fixed_row_position = fixed_row_position + VISIBLE_FIXED_ROW_HEIGHT(mw);
	trailing_fixed_row_position = non_fixed_row_position + VISIBLE_NON_FIXED_HEIGHT(mw);

	fixed_column_position = VERT_SB_OFFSET(mw) + row_label_width + mw->manager.shadow_thickness;
	non_fixed_column_position = fixed_column_position + VISIBLE_FIXED_COLUMN_WIDTH(mw);
	trailing_fixed_column_position = non_fixed_column_position + VISIBLE_NON_FIXED_WIDTH(mw);

	DEBUGOUT(_XbaeDebug2
			 (__FILE__, (Widget) mw, ClipChild(mw), "XtConfigureWidget clip %d %d %d %d\n",
			  NON_FIXED_COLUMN_POSITION(mw), NON_FIXED_ROW_POSITION(mw), VISIBLE_NON_FIXED_WIDTH(mw),
			  VISIBLE_NON_FIXED_HEIGHT(mw)));

	/*
	 * This one causes the 'traversal' redraw problem when wildly resizing :
	 * the clip widget's height should include not only the visible_non_fixed_height but
	 * also the shadow thickness.
	 * Bug #702560.
	 *
	 * Bugfix for Xbae 4.50.3 : overwriting bottom shadow around the clip widget
	 */

	/* Resize all the other clips */
	if(!row_label_width || !VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		if(XtIsManaged(RowLabelClip(mw)))
			XtUnmanageChild(RowLabelClip(mw));
	}
	else
	{
		XtConfigureWidget(RowLabelClip(mw),
						  VERT_SB_OFFSET(mw),
						  non_fixed_row_position, row_label_width, VISIBLE_NON_FIXED_HEIGHT(mw), 0);
		if(!XtIsManaged(RowLabelClip(mw)))
			XtManageChild(RowLabelClip(mw));
	}

	if(!column_label_height || !VISIBLE_NON_FIXED_WIDTH(mw))
	{
		if(XtIsManaged(ColumnLabelClip(mw)))
			XtUnmanageChild(ColumnLabelClip(mw));
	}
	else
	{
		XtConfigureWidget(ColumnLabelClip(mw),
						  non_fixed_column_position,
						  HORIZ_SB_OFFSET(mw), VISIBLE_NON_FIXED_WIDTH(mw), column_label_height, 0);
		if(!XtIsManaged(ColumnLabelClip(mw)))
			XtManageChild(ColumnLabelClip(mw));
	}

	if(!VISIBLE_FIXED_COLUMN_WIDTH(mw) || !VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		if(XtIsManaged(LeftClip(mw)))
			XtUnmanageChild(LeftClip(mw));
	}
	else
	{
		XtConfigureWidget(LeftClip(mw),
						  fixed_column_position,
						  non_fixed_row_position,
						  VISIBLE_FIXED_COLUMN_WIDTH(mw), VISIBLE_NON_FIXED_HEIGHT(mw), 0);
		if(!XtIsManaged(LeftClip(mw)))
			XtManageChild(LeftClip(mw));
	}

	if(!VISIBLE_NON_FIXED_WIDTH(mw) || !VISIBLE_FIXED_ROW_HEIGHT(mw))
	{
		if(XtIsManaged(TopClip(mw)))
			XtUnmanageChild(TopClip(mw));
	}
	else
	{
		XtConfigureWidget(TopClip(mw),
						  non_fixed_column_position,
						  fixed_row_position, VISIBLE_NON_FIXED_WIDTH(mw), VISIBLE_FIXED_ROW_HEIGHT(mw), 0);
		if(!XtIsManaged(TopClip(mw)))
			XtManageChild(TopClip(mw));
	}

	if(!VISIBLE_NON_FIXED_WIDTH(mw) || !VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		if(XtIsManaged(ClipChild(mw)))
			XtUnmanageChild(ClipChild(mw));
	}
	else
	{
		XtConfigureWidget(ClipChild(mw),
						  non_fixed_column_position,
						  non_fixed_row_position,
						  VISIBLE_NON_FIXED_WIDTH(mw), VISIBLE_NON_FIXED_HEIGHT(mw), 0);
		if(!XtIsManaged(ClipChild(mw)))
			XtManageChild(ClipChild(mw));
	}

	if(!VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw) || !VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		if(XtIsManaged(RightClip(mw)))
			XtUnmanageChild(RightClip(mw));
	}
	else
	{
		XtConfigureWidget(RightClip(mw),
						  trailing_fixed_column_position,
						  non_fixed_row_position,
						  VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw), VISIBLE_NON_FIXED_HEIGHT(mw), 0);
		if(!XtIsManaged(RightClip(mw)))
			XtManageChild(RightClip(mw));
	}

	if(!VISIBLE_NON_FIXED_WIDTH(mw) || !VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw))
	{
		if(XtIsManaged(BottomClip(mw)))
			XtUnmanageChild(BottomClip(mw));
	}
	else
	{
		/* height of 0 produces X Error */
		XtConfigureWidget(BottomClip(mw),
						  non_fixed_column_position,
						  trailing_fixed_row_position,
						  VISIBLE_NON_FIXED_WIDTH(mw), VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw), 0);
		if(!XtIsManaged(BottomClip(mw)))
			XtManageChild(BottomClip(mw));
	}

	/*
	 * Move the text widget where it belongs
	 */
	if(mw->matrix.text_child_is_mapped)
	{
		xbaePositionTextChild(mw);
	}

	/*
	 * Move the cell widgets where they belong
	 */
	if(mw->matrix.per_cell)
	{
		int row, column;
		for(row = 0; row < mw->matrix.rows; row++)
		{
			for(column = 0; column < mw->matrix.columns; column++)
			{
				xbaePositionCellWidget(mw, row, column);
			}
		}
		xbaeSetInitialFocus(mw);
	}
}

/*
 * This is what Xt calls when we get resized
 */
void xbaeResize(XbaeMatrixWidget mw)
{
	xbaeRelayout(mw);

	if(mw->matrix.resize_callback != NULL)
	{
		XbaeMatrixResizeCallbackStruct call_data;

		call_data.reason = XbaeResizeReason;
		call_data.event = (XEvent *) NULL;
		call_data.row = mw->matrix.rows;
		call_data.column = mw->matrix.columns;
		call_data.width = mw->core.width;
		call_data.height = mw->core.height;
		XtCallCallbackList((Widget) mw, mw->matrix.resize_callback, (XtPointer) & call_data);
	}
}

/*
 * These are the modifyVerifyCallback and valueChangedCallback we added to 
 * textChild. We need to call Matrix's modifyVerifyCallback/valueChangedCallback 
 * list with the textField info and the row/col that is changing.
 */

/* ARGSUSED */
void xbaeModifyVerifyCB(Widget w, XtPointer client, XtPointer call)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) client;
	XmTextVerifyCallbackStruct *verify = (XmTextVerifyCallbackStruct *) call;
	XbaeMatrixModifyVerifyCallbackStruct call_data;

	if(!mw->matrix.text_child_is_mapped
	   || !xbaeIsCellVisible(mw, mw->matrix.current_row, mw->matrix.current_column))
	{
		verify->doit = False;
		return;
	}

	if(!mw->matrix.modify_verify_callback)
	{
		return;
	}

	call_data.reason = XbaeModifyVerifyReason;
	call_data.row = mw->matrix.current_row;
	call_data.column = mw->matrix.current_column;
	call_data.event = (XEvent *) NULL;
	call_data.verify = verify;

	call_data.prev_text = XmTextGetString(TextChild(mw));
	if(!call_data.prev_text)
		call_data.prev_text = "";

	XtCallCallbackList((Widget) mw, mw->matrix.modify_verify_callback, (XtPointer) & call_data);

	XtFree((char *) call_data.prev_text);
}

/* ARGSUSED */
void xbaeValueChangedCB(Widget w, XtPointer client, XtPointer call)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) client;
	XbaeMatrixValueChangedCallbackStruct call_data;
	XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) call;

	if(!mw->matrix.value_changed_callback)
		return;

	call_data.reason = XbaeValueChangedReason;
	call_data.row = mw->matrix.current_row;
	call_data.column = mw->matrix.current_column;
	call_data.event = cbs->event;

	XtCallCallbackList((Widget) mw, mw->matrix.value_changed_callback, (XtPointer) & call_data);
}

/*
 * This is the FocusCB we added to the textChild.
 */
void xbaeFocusCB(Widget w, XtPointer client, XtPointer call)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) XtParent(w);
	int row, column;

	char *param = "Focus";
	static XrmQuark qparam = NULLQUARK;
	/*
	 * Get a static quarks for our parm
	 */
	if(qparam == NULLQUARK)
	{
		qparam = XrmPermStringToQuark(param);
	}

	DEBUGOUT(_XbaeDebug2(__FILE__, (Widget) mw, w, "FocusCB\n"));

	if(mw->matrix.current_row == -1 || mw->matrix.current_column == -1)
	{
		/* 
		 * We gain the focus and there is no current cell. 
		 * Start editing a new one.
		 */
		row = xbaeTopRow(mw);
		column = xbaeLeftColumn(mw);
	}
	else
	{
		row = mw->matrix.current_row;
		column = mw->matrix.current_column;
	}

	if(mw->matrix.traverse_cell_callback)
	{
		XbaeMatrixTraverseCellCallbackStruct call_data;

		call_data.reason = XbaeTraverseCellReason;
		call_data.event = NULL;
		call_data.row = mw->matrix.current_row;
		call_data.column = mw->matrix.current_column;
		call_data.next_row = row;
		call_data.next_column = column;
		call_data.fixed_rows = mw->matrix.fixed_rows;
		call_data.fixed_columns = mw->matrix.fixed_columns;
		call_data.trailing_fixed_rows = mw->matrix.trailing_fixed_rows;
		call_data.trailing_fixed_columns = mw->matrix.trailing_fixed_columns;
		call_data.num_rows = mw->matrix.rows;
		call_data.num_columns = mw->matrix.columns;
		call_data.param = param;
		call_data.qparam = qparam;

		XtCallCallbackList((Widget) mw, mw->matrix.traverse_cell_callback, (XtPointer) & call_data);

		row = call_data.next_row;
		column = call_data.next_column;
	}

	if(row >= 0 && row < mw->matrix.rows && column >= 0 && column < mw->matrix.columns
	   && (row != mw->matrix.current_row || column != mw->matrix.current_column))
	{
		if(mw->matrix.per_cell == NULL || mw->matrix.per_cell[row][column].widget == NULL)
		{
			DoEditCell(mw, NULL, row, column, NULL, 0);
		}
		mw->matrix.current_row = row;
		mw->matrix.current_column = column;
	}
}

void xbaeLosingFocusCB(Widget w, XtPointer client, XtPointer call)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) XtParent(w);

	char *param = "LosingFocus";
	static XrmQuark qparam = NULLQUARK;
	/*
	 * Get a static quarks for our parm 
	 */
	if(qparam == NULLQUARK)
	{
		qparam = XrmPermStringToQuark(param);
	}

	DEBUGOUT(_XbaeDebug2(__FILE__, (Widget) mw, w, "LostFocusCB\n"));

	if(mw->matrix.traverse_cell_callback)
	{
		XbaeMatrixTraverseCellCallbackStruct call_data;

		call_data.reason = XbaeTraverseCellReason;
		call_data.event = NULL;
		call_data.row = mw->matrix.current_row;
		call_data.column = mw->matrix.current_column;
		call_data.next_row = mw->matrix.current_row;
		call_data.next_column = mw->matrix.current_column;
		call_data.fixed_rows = mw->matrix.fixed_rows;
		call_data.fixed_columns = mw->matrix.fixed_columns;
		call_data.trailing_fixed_rows = mw->matrix.trailing_fixed_rows;
		call_data.trailing_fixed_columns = mw->matrix.trailing_fixed_columns;
		call_data.num_rows = mw->matrix.rows;
		call_data.num_columns = mw->matrix.columns;
		call_data.param = param;
		call_data.qparam = qparam;

		XtCallCallbackList((Widget) mw, mw->matrix.traverse_cell_callback, (XtPointer) & call_data);

	}
}

/*
 * The meat of Matrix edit_cell method.
 * Tobias: This function mustn't call XmProcessTraversal (that's why it's broken 
 * out of the real edit_cell method). It gets called from the textChild's focusCB 
 * that may have been triggered by a XmProcessTraversal and recursive calls to 
 * XmProcessTraversal are disallowed in motif >= 1.2.
 */
static void
DoEditCell(XbaeMatrixWidget mw, XEvent * event, int row, int column, String * params, Cardinal nparams)
{
	XbaeMatrixEnterCellCallbackStruct call_data;

	assert(row >= 0 && row < mw->matrix.rows && column >= 0 && column < mw->matrix.columns);
	assert(mw->matrix.per_cell == NULL || mw->matrix.per_cell[row][column].widget == NULL);
	assert(!mw->matrix.text_child_is_mapped
		   || row != mw->matrix.current_row || column != mw->matrix.current_column);

	/*
	 * We actually don't check for traverse_fixed here even though
	 * it looks like it might be needed. The reason is that we may
	 * need to reparent back onto the clip in case we were on the
	 * fixed area and then traverse_fixed has been set to False
	 * via SetValues. Doing this on the next traversal is probably
	 * preferable to magically warping the textField off the
	 * matrix on to the clip when traverseFixedCells changes. It
	 * also allows the user to finish editing the existing cell,
	 * but won't allow continued traversal on the fixed area. -CG
	 */

	/* Save this as our current cell */
	mw->matrix.current_row = row;
	mw->matrix.current_column = column;

	/*
	 * If we have an enterCellCallback, call it to see if the cell is
	 * editable.
	 */
	XtVaGetValues(TextChild(mw),
				  XmNoverwriteMode, &call_data.overwrite_mode,
				  XmNautoFill, &call_data.auto_fill, XmNconvertCase, &call_data.convert_case, NULL);

	call_data.map = True;
	call_data.doit = True;
	call_data.position = -1;
	call_data.pattern = NULL;
	call_data.select_text = False;

	if(mw->matrix.enter_cell_callback)
	{
		call_data.reason = XbaeEnterCellReason;
		call_data.event = event;
		call_data.row = row;
		call_data.column = column;
		call_data.num_params = nparams;
		call_data.params = params;

		XtCallCallbackList((Widget) mw, mw->matrix.enter_cell_callback, (XtPointer) & call_data);
	}

	/*
	 * Hide the textChild
	 */
	xbaeHideTextChild(mw);

	/*
	 * We double check for a widget here in case one was set in the enterCellCB
	 * That's apparently what danny does in Xquote
	 */

	if((call_data.doit || call_data.map)
	   && (mw->matrix.per_cell == NULL || mw->matrix.per_cell[row][column].widget == NULL))
	{
		/*
		 * Get the contents of the new cell
		 */
		XbaeMatrixCellValuesStruct cell_values;

		xbaeGetCellValues(mw, row, column, True, &cell_values);

		/*
		 * If the new cell contains a string we can go there
		 */
		if(cell_values.drawCB.type & XbaeString)
		{
			/*
			 * We can't show the text child after everything else is done 
			 * because it confuses Motif. So we disable redisplay and reinable 
			 * it later.
			 */
			XmTextDisableRedisplay(TextChild(mw));

			/*
			 * Update the textChild's font
			 */
			xbaeUpdateTextChildFont(mw, cell_values.qtag);

			/*
			 * Show the textChild.
			 * 
			 */
			xbaePositionTextChild(mw);

			/*
			 * Setup the textField for the new cell. If the modifyVerify CB
			 * rejects the new value, then it is the applications fault for
			 * loading the cell with a bad value to begin with.
			 */
			XtRemoveCallback(TextChild(mw), XmNmodifyVerifyCallback, xbaeModifyVerifyCB, (XtPointer) mw);

			XtVaSetValues(TextChild(mw),
						  XmNvalue, cell_values.drawCB.string,
						  XmNbackground, cell_values.drawCB.background,
						  XmNforeground, cell_values.drawCB.foreground,
						  XmNeditable, call_data.doit,
						  XmNcursorPositionVisible, call_data.doit,
						  XmNmaxLength, (mw->matrix.column_max_lengths
										 && mw->matrix.column_max_lengths[column] !=
										 0) ? mw->matrix.column_max_lengths[column] : (COLUMN_WIDTH(mw,
																									column) -
																					   2 *
																					   CELL_BORDER_WIDTH(mw))
						  / CELL_FONT_WIDTH(mw), XmNpattern, call_data.pattern, XmNoverwriteMode,
						  call_data.overwrite_mode, XmNautoFill, call_data.auto_fill, XmNconvertCase,
						  call_data.convert_case, NULL);

			XtAddCallback(TextChild(mw), XmNmodifyVerifyCallback, xbaeModifyVerifyCB, (XtPointer) mw);

			/*
			 * We need to EnableRedisplay before calling XmTextXYToPos
			 */
			XmTextEnableRedisplay(TextChild(mw));

			if(call_data.doit)
			{
				/*
				 * Set the insert position of the cursor. We need to do this after 
				 * showing the textchild or else XmTextXYToPos gets confused.
				 */
				int position = call_data.position;
				int length = strlen(cell_values.drawCB.string);

				if(event && (event->type == ButtonPress || event->type == ButtonRelease)
				   && position < 0 && mw->matrix.calc_cursor_position)
				{
					/*
					 * The location of the pointer click needs to be calculated
					 * so the cursor can be positioned.  If position is >= 0,
					 * it has been set in the enterCellCallback and must
					 * be honoured elsewhere.
					 */
					int r, c;
					int x, y;

					/*
					 * The event must have occurred in a legal position
					 * otherwise control wouldn't have made it here
					 */
					(void) xbaeEventToMatrixXY(mw, event, &x, &y);
					(void) xbaeMatrixXYToRowCol(mw, &x, &y, &r, &c);
					x -= mw->matrix.cell_shadow_thickness;
					y -= mw->matrix.cell_shadow_thickness;
					position = XmTextXYToPos(TextChild(mw), x, y);
				}

				if(position < 0 || position > length)
				{
					XmTextSetInsertionPosition(TextChild(mw), length);
				}
				else
				{
					XmTextSetInsertionPosition(TextChild(mw), position);
				}

				if(call_data.select_text)
				{
					XmTextSetSelection(TextChild(mw), 0, length, CurrentTime);
				}
			}
		}

		if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
		{
			XtFree((XtPointer) cell_values.drawCB.string);
		}
	}
}

/*
 * Matrix select_cell method
 */
void xbaeSelectCell(XbaeMatrixWidget mw, int row, int column)
{
	Boolean visible;

	if(row >= mw->matrix.rows || row < 0 || column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "selectCell", "badIndex",
						"XbaeMatrix", "XbaeMatrix: Row or column out of bounds for SelectCell.", NULL, 0);
		return;
	}

	/* If no cells have been selected yet, allocate memory here */
	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Scroll the cell onto the screen
	 */
	visible = xbaeIsCellVisible(mw, row, column);

	if(mw->matrix.scroll_select && !visible)
	{
		xbaeMakeCellVisible(mw, row, column);
		visible = True;
	}

	/*
	 * If the cell is not already selected, select it and redraw it
	 */
	if(!mw->matrix.per_cell[row][column].selected)
	{
		mw->matrix.per_cell[row][column].selected = True;
		mw->matrix.num_selected_cells++;
		if(visible)
		{
			xbaeDrawCell(mw, row, column);
		}
	}
}

/*
 * Matrix select_row method
 */
void xbaeSelectRow(XbaeMatrixWidget mw, int row)
{
	int j, lc, rc;
	Boolean visible;

	if(row >= mw->matrix.rows || row < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "selectRow", "badIndex",
						"XbaeMatrix", "XbaeMatrix: Row out of bounds for SelectRow.", NULL, 0);
		return;
	}

	/* If no cells have been selected yet, allocate memory here */
	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Scroll the row onto the screen
	 */
	visible = xbaeIsRowVisible(mw, row);

	if(mw->matrix.scroll_select && !visible)
	{
		xbaeMakeRowVisible(mw, row);
		visible = True;
	}

	/*
	 * For each cell in the row, if the cell is not already selected,
	 * select it and redraw it
	 */
	xbaeGetVisibleColumns(mw, &lc, &rc);
	for(j = 0; j < mw->matrix.columns; j++)
	{
		if(!mw->matrix.per_cell[row][j].selected)
		{
			mw->matrix.per_cell[row][j].selected = True;
			mw->matrix.num_selected_cells++;
			if(visible && ((j >= lc && j <= rc) || IS_FIXED_COLUMN(mw, j)))
			{
				xbaeDrawCell(mw, row, j);
			}
		}
	}
}

/*
 * Matrix select_column method
 */
void xbaeSelectColumn(XbaeMatrixWidget mw, int column)
{
	int i, tr, br;
	Boolean visible;

	if(column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "selectColumn",
						"badIndex", "XbaeMatrix",
						"XbaeMatrix: Column out of bounds for SelectColumn.", NULL, 0);
		return;
	}

	/* If no cells have been selected yet, allocate memory here */
	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Scroll the column onto the screen
	 */
	visible = xbaeIsColumnVisible(mw, column);

	if(mw->matrix.scroll_select && !visible)
	{
		xbaeMakeColumnVisible(mw, column);
		visible = True;
	}

	/*
	 * For each cell in the column, if the cell is not already selected,
	 * select it and redraw it
	 */
	xbaeGetVisibleRows(mw, &tr, &br);
	for(i = 0; i < mw->matrix.rows; i++)
	{
		if(!mw->matrix.per_cell[i][column].selected)
		{
			mw->matrix.per_cell[i][column].selected = True;
			mw->matrix.num_selected_cells++;
			if(visible && ((i >= tr && i <= br) || IS_FIXED_ROW(mw, i)))
			{
				xbaeDrawCell(mw, i, column);
			}
		}
	}
}

/*
 * Matrix select_all method
 */
void xbaeSelectAll(XbaeMatrixWidget mw)
{
	int i, j;
	int tr, br, lc, rc;

	xbaeGetVisibleCells(mw, &tr, &br, &lc, &rc);

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	for(i = 0; i < mw->matrix.rows; i++)
	{
		for(j = 0; j < mw->matrix.columns; j++)
		{
			if(!mw->matrix.per_cell[i][j].selected)
			{
				mw->matrix.num_selected_cells++;
				mw->matrix.per_cell[i][j].selected = True;
				if(((i >= tr && i <= br) || IS_FIXED_ROW(mw, i))
				   && ((j >= lc && j <= rc) || IS_FIXED_COLUMN(mw, j)))
				{
					xbaeDrawCell(mw, i, j);
				}
			}
		}
	}
}

/*
 * Matrix deselect_cell method
 */
void xbaeDeselectCell(XbaeMatrixWidget mw, int row, int column)
{
	if(row >= mw->matrix.rows || row < 0 || column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deselectCell",
						"badIndex", "XbaeMatrix",
						"XbaeMatrix: Row or column out of bounds for DeselectCell.", NULL, 0);
		return;
	}

	if(!mw->matrix.per_cell || mw->matrix.num_selected_cells == 0)
		return;

	if(mw->matrix.per_cell[row][column].selected)
	{
		mw->matrix.num_selected_cells--;
		mw->matrix.per_cell[row][column].selected = False;
		if(xbaeIsCellVisible(mw, row, column))
		{
			xbaeDrawCell(mw, row, column);
		}
	}
}

/*
 * Matrix deselect_row method
 */
void xbaeDeselectRow(XbaeMatrixWidget mw, int row)
{
	int j, lc, rc;
	Boolean visible;

	if(row >= mw->matrix.rows || row < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deselectRow",
						"badIndex", "XbaeMatrix",
						"XbaeMatrix: Row parameter out of bounds for DeselectRow.", NULL, 0);
		return;
	}

	if(!mw->matrix.per_cell || mw->matrix.num_selected_cells == 0)
		return;

	visible = xbaeIsRowVisible(mw, row);

	/*
	 * For each cell in the row, if the cell is selected,
	 * deselect it and redraw it
	 */
	xbaeGetVisibleColumns(mw, &lc, &rc);
	for(j = 0; j < mw->matrix.columns; j++)
	{
		if(mw->matrix.per_cell[row][j].selected)
		{
			mw->matrix.num_selected_cells--;
			mw->matrix.per_cell[row][j].selected = False;
			if(visible && ((j >= lc && j <= rc) || IS_FIXED_COLUMN(mw, j)))
			{
				xbaeDrawCell(mw, row, j);
			}
		}
	}
}

/*
 * Matrix deselect_column method
 */
void xbaeDeselectColumn(XbaeMatrixWidget mw, int column)
{
	int i, tr, br;
	Boolean visible;

	if(column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deselectColumn",
						"badIndex", "XbaeMatrix",
						"XbaeMatrix: Column parameter out of bounds for DeselectColumn.", NULL, 0);
		return;
	}

	if(!mw->matrix.per_cell || mw->matrix.num_selected_cells == 0)
		return;

	visible = xbaeIsColumnVisible(mw, column);

	/*
	 * For each cell in the column, if the cell is selected,
	 * deselect it and redraw it
	 */
	xbaeGetVisibleRows(mw, &tr, &br);
	for(i = 0; i < mw->matrix.rows; i++)
	{
		if(mw->matrix.per_cell[i][column].selected)
		{
			mw->matrix.num_selected_cells--;
			mw->matrix.per_cell[i][column].selected = False;
			if(visible && ((i >= tr && i <= br) || IS_FIXED_ROW(mw, i)))
			{
				xbaeDrawCell(mw, i, column);
			}
		}
	}
}

/*
 * Matrix deselect_all method
 */
void xbaeDeselectAll(XbaeMatrixWidget mw)
{
	int i, j;
	int tr, br, lc, rc;

	if(!mw->matrix.per_cell || mw->matrix.num_selected_cells == 0)
		return;

	mw->matrix.num_selected_cells = 0;

	xbaeGetVisibleCells(mw, &tr, &br, &lc, &rc);

	for(i = 0; i < mw->matrix.rows; i++)
	{
		for(j = 0; j < mw->matrix.columns; j++)
		{
			if(mw->matrix.per_cell[i][j].selected)
			{
				mw->matrix.per_cell[i][j].selected = False;
				if(((i >= tr && i <= br) || IS_FIXED_ROW(mw, i))
				   && ((j >= lc && j <= rc) || IS_FIXED_COLUMN(mw, j)))
				{
					xbaeDrawCell(mw, i, j);
				}
			}
		}
	}
}

/*
 * Matrix get_cell method
 */
String xbaeGetCell(XbaeMatrixWidget mw, int row, int column)
{
	XbaeMatrixCellValuesStruct cell_values;

	if(row >= mw->matrix.rows || row < 0 || column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "getCell", "badIndex",
						"XbaeMatrix", "XbaeMatrix: Row or column out of bounds for GetCell.", NULL, 0);
		return NULL;
	}

	xbaeGetCellValues(mw, row, column, False, &cell_values);

	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "getCell", "memleak",
						"XbaeMatrix",
						"XbaeMatrix: xbaeGetCell is likely to leak memory when a "
						"drawCellCallBack uses the XbaeStringFree type", NULL, 0);
	}

	return cell_values.drawCB.string;
}

/*
 * Matrix edit_cell method
 */
void xbaeEditCell(XbaeMatrixWidget mw, XEvent * event, int row, int column, String * params, Cardinal nparams)
{
	Widget userWidget;

	if(row >= mw->matrix.rows || row < 0 || column >= mw->matrix.columns || column < 0)
	{
		/*
		 * If we have zero rows or columns, there are no cells
		 * available on which to place the text field so just return
		 */
		if(mw->matrix.rows == 0 || mw->matrix.columns == 0)
			return;

		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "editCell", "badIndex",
						"XbaeMatrix", "XbaeMatrix: Row or column out of bounds for EditCell.", NULL, 0);
		return;
	}

	userWidget = mw->matrix.per_cell ? mw->matrix.per_cell[row][column].widget : NULL;

	/*
	 * Make the cell visible
	 */
	xbaeMakeCellVisible(mw, row, column);

	if(userWidget)
	{
		/*
		 * We have a user widget in the new cell. Try to traverse to it
		 */

		if(!XmProcessTraversal(userWidget, XmTRAVERSE_CURRENT))
		{
			/* 
			 * The widget didn't accept the focus. Try to commit the
			 * current cell and continue in stelth mode
			 */

			if(DoCommitEdit(mw, event))
			{
				/* Save this as our current cell */
				mw->matrix.current_row = row;
				mw->matrix.current_column = column;

				/* Hide the textChild */
				xbaeHideTextChild(mw);

				XmProcessTraversal(TextChild(mw), XmTRAVERSE_CURRENT);
			}
		}
	}
	else
	{
		if(!mw->matrix.text_child_is_mapped || row != mw->matrix.current_row
		   || column != mw->matrix.current_column)
		{
			if(DoCommitEdit(mw, event))
			{
				DoEditCell(mw, event, row, column, params, nparams);
			}
		}
		XmProcessTraversal(TextChild(mw), XmTRAVERSE_CURRENT);
	}
}

/*
 * Matrix commit_edit method
 */
Boolean xbaeCommitEdit(XbaeMatrixWidget mw, XEvent * event, Boolean unmap)
{
	Boolean commit;

	if(!mw->matrix.text_child_is_mapped)
		return True;

	/*
	 * Attempt to commit the edit
	 */
	commit = DoCommitEdit(mw, event);

	if(unmap)
	{
		/*
		 * If unmap is set, hide the textField.
		 */
		xbaeHideTextChild(mw);
	}
	return commit;
}

/*
 * Matrix cancel_edit method
 */
void xbaeCancelEdit(XbaeMatrixWidget mw, Boolean unmap)
{
	if(!mw->matrix.text_child_is_mapped)
		return;

	if(unmap)
	{
		/*
		 * If unmap is set, hide the textField.
		 */
		xbaeHideTextChild(mw);
	}
	else
	{
		/*
		 * Don't unmap, just restore original contents
		 */
		xbaeUpdateTextChild(mw, True);
	}
}

/*
 * Matrix add_rows method
 */
void
xbaeAddVarRows(XbaeMatrixWidget mw, int position, String * rows, String * labels, short *heights,
			   int *max_heights, unsigned char *alignments, unsigned char *label_alignments,
			   Pixel * colors, Pixel * backgrounds, int num_rows)
{
	Boolean haveVSB, haveHSB;

	/*
	 * Do some error checking.
	 */
	if(num_rows <= 0)
		return;
	if(position < 0 || position > mw->matrix.rows)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "addRows", "badPosition",
						"XbaeMatrix", "XbaeMatrix: Position out of bounds in AddRows.", NULL, 0);
		return;
	}
	if(mw->matrix.columns == 0 && (rows || colors || backgrounds))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "AddColumns", "noRows",
						"XbaeMatrix", "XbaeMatrix: Attempting to add rows with no columns.", NULL, 0);
		return;
	}

	haveVSB = XtIsManaged(VertScrollChild(mw));
	haveHSB = XtIsManaged(HorizScrollChild(mw));

	/*
	 * Add the new rows into the internal cells/labels data structure.
	 */
	AddVarRowsToTable(mw, position, rows, labels, heights, colors, backgrounds, num_rows);

	/*
	 * Adjust the position of the textWidget if the rows are added above it
	 */
	if(position <= mw->matrix.current_row)
	{
		mw->matrix.current_row += num_rows;
	}

	/*
	 * Relayout.
	 */
	xbaeRelayout(mw);

	if(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw))
	{
		/*
		 * Redraw everything below the insertion point
		 */
		xbaeRedrawAll(mw, position, -1, mw->matrix.rows - 1, mw->matrix.columns - 1);

		/*
		 * If a scrollbar has just been mapped at the top or left the fixed labels 
		 * and totally fixed cells shift arround. Redraw everything that is fixed
		 */
		if((!haveHSB && XtIsManaged(HorizScrollChild(mw)) && SCROLLBAR_TOP(mw))
		   || (!haveVSB && XtIsManaged(VertScrollChild(mw)) && SCROLLBAR_LEFT(mw)))
		{
			XClearArea(XtDisplay(mw), XtWindow(mw), 0, 0, 0, 0, True);
		}
	}
}

void
xbaeAddRows(XbaeMatrixWidget mw, int position, String * rows, String * labels,
			Pixel * colors, Pixel * backgrounds, int num_rows)
{
	xbaeAddVarRows(mw, position, rows, labels, NULL, 0, NULL, NULL, colors, backgrounds, num_rows);
}

/*
 * Matrix delete_rows method
 */
void xbaeDeleteRows(XbaeMatrixWidget mw, int position, int num_rows)
{
	int vert_origin;
	Boolean haveVSB;
	Boolean haveHSB;
	int row_label_width = ROW_LABEL_WIDTH(mw);

	/*
	 * Do some error checking.
	 */
	if(num_rows <= 0)
		return;
	if(position < 0 || position + num_rows > mw->matrix.rows)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deleteRows",
						"badPosition", "XbaeMatrix",
						"XbaeMatrix: Position out of bounds in DeleteRows.", NULL, 0);
		return;
	}
	if(num_rows > (mw->matrix.rows - (int) mw->matrix.fixed_rows - (int) mw->matrix.trailing_fixed_rows))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deleteRows", "tooMany",
						"XbaeMatrix",
						"XbaeMatrix: Attempting to delete too many rows in DeleteRows.", NULL, 0);
		return;
	}

	haveVSB = XtIsManaged(VertScrollChild(mw));
	haveHSB = XtIsManaged(HorizScrollChild(mw));
	vert_origin = VERT_ORIGIN(mw);

	/*
	 * If the text child is in a row that's going to be deleted cancel the edit
	 * else move it up if the rows being deleted are above it
	 */
	if(position <= mw->matrix.current_row)
	{
		if(position + num_rows <= mw->matrix.current_row)
		{
			mw->matrix.current_row -= num_rows;
		}
		else
		{
			mw->matrix.current_row = -1;
			mw->matrix.current_column = -1;
			xbaeHideTextChild(mw);
		}
	}

	/*
	 * Delete the new rows from the internal cells/labels data structure.
	 */
	DeleteRowsFromTable(mw, position, num_rows);

	/*
	 * Relayout.
	 */
	xbaeRelayout(mw);

	if(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw))
	{
		/*
		 * Redraw everything below the insertion point
		 */
		if(vert_origin != VERT_ORIGIN(mw))
		{
			/* 
			 * If Resize had to change VERT_ORIGIN because cells were scrolled
			 * off, we need to redraw the whole VISIBLE_NON_FIXED_HEIGHT
			 */
			xbaeRedrawAll(mw,
						  Min(position, mw->matrix.fixed_rows), -1,
						  mw->matrix.rows - 1, mw->matrix.columns - 1);
		}
		else
		{
			xbaeRedrawAll(mw, position, -1, mw->matrix.rows - 1, mw->matrix.columns - 1);
		}

		/*
		 * If a scrollbar has just been unmapped at the top or left the fixed labels 
		 * and totally fixed cells shift arround. Redraw everything that is fixed
		 * Same thing if the the row label width changed
		 */
		if((haveHSB && !XtIsManaged(HorizScrollChild(mw)) && SCROLLBAR_TOP(mw))
		   || (haveVSB && !XtIsManaged(VertScrollChild(mw)) && SCROLLBAR_LEFT(mw))
		   || (row_label_width != ROW_LABEL_WIDTH(mw)))
		{
			XClearArea(XtDisplay(mw), XtWindow(mw), 0, 0, 0, 0, True);
		}
		else
		{
			/* don't leave dangeling bits at the bottom */
			XClearArea(XtDisplay(mw), XtWindow(mw),
					   0,
					   TRAILING_FIXED_ROW_POSITION(mw) + VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw) +
					   mw->manager.shadow_thickness, 0, 0, False);
			/* also clear the area bellow the labels and as heigh as the shadow */
			XClearArea(XtDisplay(mw), XtWindow(mw),
					   HORIZ_SB_OFFSET(mw),
					   TRAILING_FIXED_ROW_POSITION(mw) + VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw),
					   ROW_LABEL_WIDTH(mw), mw->manager.shadow_thickness, False);
		}
	}
}

/*
 * Matrix add_columns method.
 */
void
xbaeAddColumns(XbaeMatrixWidget mw, int position, String * columns, String * labels, short *widths,
			   int *max_lengths, unsigned char *alignments, unsigned char *label_alignments,
			   Pixel * colors, Pixel * backgrounds, int num_columns)
{
	Boolean haveVSB;
	Boolean haveHSB;

	/*
	 * Do some error checking.
	 */
	if(num_columns <= 0)
		return;
	if(position < 0 || position > mw->matrix.columns)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "addColumns",
						"badPosition", "XbaeMatrix",
						"XbaeMatrix: Position out of bounds in AddColumns.", NULL, 0);
		return;
	}
	if(mw->matrix.rows == 0 && (columns || colors || backgrounds))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "AddColumns", "noRows",
						"XbaeMatrix", "XbaeMatrix: Attempting to add columns with no rows.", NULL, 0);
		return;
	}

	haveVSB = XtIsManaged(VertScrollChild(mw));
	haveHSB = XtIsManaged(HorizScrollChild(mw));

	/*
	 * Add the new columns into the internal cells/labels data structure.
	 */
	AddColumnsToTable(mw, position, columns, labels, widths, max_lengths, alignments,
					  label_alignments, colors, backgrounds, num_columns);

	/*
	 * Adjust the position of the textWidget if the columns are added to the left of it
	 */
	if(position <= mw->matrix.current_column)
	{
		mw->matrix.current_column += num_columns;
	}

	/*
	 * Relayout.
	 */
	xbaeRelayout(mw);

	if(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw))
	{
		/*
		 * Redraw everything to the right of the insertion point
		 */
		xbaeRedrawAll(mw, -1, position, mw->matrix.rows - 1, mw->matrix.columns - 1);

		/*
		 * If a scrollbar has just been mapped at the top or left the fixed labels 
		 * and totally fixed cells shift arround. Redraw everything that is fixed
		 */
		if((!haveHSB && XtIsManaged(HorizScrollChild(mw)) && SCROLLBAR_TOP(mw))
		   || (!haveVSB && XtIsManaged(VertScrollChild(mw)) && SCROLLBAR_LEFT(mw)))
		{
			XClearArea(XtDisplay(mw), XtWindow(mw), 0, 0, 0, 0, True);
		}
	}
}

/*
 * Matrix delete_columns method
 */
void xbaeDeleteColumns(XbaeMatrixWidget mw, int position, int num_columns)
{
	int horiz_origin;
	Boolean haveVSB;
	Boolean haveHSB;
	int column_label_height = COLUMN_LABEL_HEIGHT(mw);

	/*
	 * Do some error checking.
	 */
	if(num_columns <= 0)
		return;
	if(position < 0 || position + num_columns > mw->matrix.columns)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deleteColumns",
						"badPosition", "XbaeMatrix",
						"XbaeMatrix: Position out of bounds in DeleteColumns.", NULL, 0);
		return;
	}
	if(num_columns >
	   (mw->matrix.columns - (int) mw->matrix.fixed_columns - (int) mw->matrix.trailing_fixed_columns))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "deleteColumns",
						"tooMany", "XbaeMatrix",
						"XbaeMatrix: Attempting to delete too many columns in DeleteColumns.", NULL, 0);
		return;
	}

	haveVSB = XtIsManaged(VertScrollChild(mw));
	haveHSB = XtIsManaged(HorizScrollChild(mw));
	horiz_origin = HORIZ_ORIGIN(mw);

	/*
	 * If the text child is in a column that's going to be deleted cancel the edit
	 * else move it left if the columns being deleted are to its left
	 */

	if(position <= mw->matrix.current_column)
	{
		if(position + num_columns <= mw->matrix.current_column)
		{
			mw->matrix.current_column -= num_columns;
		}
		else
		{
			mw->matrix.current_row = -1;
			mw->matrix.current_column = -1;
			xbaeHideTextChild(mw);
		}
	}

	/*
	 * Delete the new columns from the internal cells/labels data structure.
	 */
	DeleteColumnsFromTable(mw, position, num_columns);

	/*
	 * Relayout.
	 */
	xbaeRelayout(mw);

	if(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw))
	{
		/*
		 * Redraw everything below the deletion point
		 */
		if(horiz_origin != HORIZ_ORIGIN(mw))
		{
			/* 
			 * If Resize had to change HORIZ_ORIGIN because cells were scrolled
			 * off, we need to redraw the whole VISIBLE_NON_FIXED_WIDTH
			 */
			xbaeRedrawAll(mw,
						  -1, Min(position, mw->matrix.fixed_columns),
						  mw->matrix.rows - 1, mw->matrix.columns - 1);
		}
		else
		{
			xbaeRedrawAll(mw, -1, position, mw->matrix.rows - 1, mw->matrix.columns - 1);
		}

		/*
		 * If a scrollbar has just been unmapped at the top or left the fixed labels 
		 * and totally fixed cells shift arround. Redraw everything that is fixed
		 * Same thing if the highest label got deleted
		 */
		if((haveHSB && !XtIsManaged(HorizScrollChild(mw)) && SCROLLBAR_TOP(mw))
		   || (haveVSB && !XtIsManaged(VertScrollChild(mw)) && SCROLLBAR_LEFT(mw))
		   || (column_label_height != COLUMN_LABEL_HEIGHT(mw)))
		{
			XClearArea(XtDisplay(mw), XtWindow(mw), 0, 0, 0, 0, True);
		}
		else
		{
			/* don't leave dangeling bits to the right */
			XClearArea(XtDisplay(mw), XtWindow(mw),
					   TRAILING_FIXED_COLUMN_POSITION(mw) + VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw) +
					   mw->manager.shadow_thickness, 0, 0, 0, False);
			/* also clear the area to the right of the labels and as wide as the shadow */
			XClearArea(XtDisplay(mw), XtWindow(mw),
					   TRAILING_FIXED_COLUMN_POSITION(mw) + VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw),
					   VERT_SB_OFFSET(mw), mw->manager.shadow_thickness, COLUMN_LABEL_HEIGHT(mw), False);
		}
	}
}

/*
 * Matrix set_row_colors method
 */
void xbaeSetRowColors(XbaeMatrixWidget mw, int position, Pixel * colors, int num_colors, Boolean bg)
{
	int i, j;

	/*
	 * Do some error checking.
	 */
	if(num_colors <= 0)
		return;
	if(position < 0 || position + num_colors > mw->matrix.rows)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "setRowColors",
						"badPosition", "XbaeMatrix",
						"XbaeMatrix: Position out of bounds or too many colors in SetRowColors.", NULL, 0);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Set each row to the appropriate color
	 */
	if(!bg)
	{
		for(i = 0; i < num_colors; i++)
			for(j = 0; j < mw->matrix.columns; j++)
				mw->matrix.per_cell[i + position][j].color = colors[i];
	}
	else
	{
		for(i = 0; i < num_colors; i++)
			for(j = 0; j < mw->matrix.columns; j++)
				mw->matrix.per_cell[i + position][j].background = colors[i];
	}

	if(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw))
	{
		/*
		 * Redraw all the affected visible cells (but not the labels).
		 * We don't need to clear first since only the color changed.
		 */
		xbaeRedrawAll(mw, position, 0, position + num_colors - 1, mw->matrix.columns - 1);

		if(mw->matrix.text_child_is_mapped
		   && position <= mw->matrix.current_row && position + num_colors > mw->matrix.current_row)
		{
			xbaeUpdateTextChild(mw, False);
		}
	}
}

/*
 * Matrix set_column_colors method
 */
void xbaeSetColumnColors(XbaeMatrixWidget mw, int position, Pixel * colors, int num_colors, Boolean bg)
{
	int i, j;

	/*
	 * Do some error checking.
	 */
	if(num_colors <= 0)
		return;
	if(position < 0 || position + num_colors > mw->matrix.columns)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "setColumnColors",
						"badPosition", "XbaeMatrix",
						"XbaeMatrix: Position out of bounds or too many colors in SetColumnColors.", NULL, 0);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Set each column to the appropriate color
	 */
	if(!bg)
	{
		for(i = 0; i < mw->matrix.rows; i++)
			for(j = 0; j < num_colors; j++)
				mw->matrix.per_cell[i][j + position].color = colors[j];
	}
	else
	{
		for(i = 0; i < mw->matrix.rows; i++)
			for(j = 0; j < num_colors; j++)
				mw->matrix.per_cell[i][j + position].background = colors[j];
	}

	if(!mw->matrix.disable_redisplay && ((Widget) mw))
	{
		/*
		 * Redraw all the visible cells (but not the labels).
		 * We don't need to clear first since only the color changed.
		 */
		xbaeRedrawAll(mw, 0, position, mw->matrix.rows - 1, position + num_colors - 1);

		if(mw->matrix.text_child_is_mapped
		   && position <= mw->matrix.current_column && position + num_colors > mw->matrix.current_column)
		{
			xbaeUpdateTextChild(mw, False);
		}
	}
}

/*
 * Matrix set_cell_color method
 */
void xbaeSetCellColor(XbaeMatrixWidget mw, int row, int column, Pixel color, Boolean bg)
{
	int i, j;

	/*
	 * Do some error checking.
	 */
	if(row >= mw->matrix.rows || row < 0 || column >= mw->matrix.columns || column < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw), "xbaeSetCellColor",
						"badIndex", "XbaeMatrix",
						"XbaeMatrix: Row or column out of bounds for xbaeSetCellColor.", NULL, 0);
		return;
	}

	/*
	 * If we don't have any colors yet, malloc them and initialize them
	 */
	if(!mw->matrix.per_cell)
	{
		xbaeCreatePerCell(mw);
		for(i = 0; i < mw->matrix.rows; i++)
			for(j = 0; j < mw->matrix.columns; j++)
				mw->matrix.per_cell[i][j].color = mw->manager.foreground;
	}

	/*
	 * Set the cell's color
	 */
	if(!bg)
		mw->matrix.per_cell[row][column].color = color;
	else
		mw->matrix.per_cell[row][column].background = color;


	if(!mw->matrix.disable_redisplay && XtIsRealized((Widget) mw))
	{
		/*
		 * Redraw the cell if it is visible
		 */
		if(xbaeIsCellVisible(mw, row, column))
		{
			xbaeDrawCell(mw, row, column);
		}

		if(mw->matrix.text_child_is_mapped
		   && row == mw->matrix.current_row && column == mw->matrix.current_column)
		{
			xbaeUpdateTextChild(mw, False);
		}
	}
}

void xbaeShowColumnArrows(XbaeMatrixWidget mw, int column, Boolean show)
{
	if(mw->matrix.show_column_arrows == NULL)
	{
		int i;
		mw->matrix.show_column_arrows = (Boolean *) XtMalloc(mw->matrix.columns * sizeof(Boolean));
		for(i = 0; i < mw->matrix.columns; i++)
			mw->matrix.show_column_arrows[i] = True;

	}

	mw->matrix.show_column_arrows[column] = show;
}
