/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2003, 2004 by the LessTif Developers.
 *
 *			All rights reserved
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
 * MatrixWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: Public.c,v 1.90 2005/06/06 19:50:22 dannybackx Exp $
 */

/*
 * Public.c created by Andrew Lister (7 August, 1995)
 */

#include <stdio.h>
#include <stdlib.h>


#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>
#include <Xbae/MatrixP.h>
#include <Xbae/Shadow.h>
#include <Xbae/Draw.h>
#include <Xbae/ScrollMgr.h>
#include <Xbae/Actions.h>
#include <Xbae/Utils.h>
#include <Xbae/Clip.h>
#include <Xbae/Create.h>
#include <Xbae/Methods.h>

#include "XbaeDebug.h"

#define XBAE_CHECK_CLASS(w)	  xbaeCheckClass(w, __FUNCTION__)
#define XBAE_CHECK_ROW(w, row)       xbaeCheckRow(w, row, __FUNCTION__)
#define XBAE_CHECK_COLUMN(w, column) xbaeCheckColumn(w, column, __FUNCTION__)

/**********************************************************************************************/

Widget XbaeCreateMatrix(Widget parent, String name, ArgList args, Cardinal ac)
{
	return XtCreateWidget(name, xbaeMatrixWidgetClass, parent, args, ac);
}

/**********************************************************************************************/

/* 
 * The following 2 functions are here to provide information about the
 * version of this library - since we may often build it as a shared/dynamic
 * library it's important to have this builtin
 */
const char *XbaeGetVersionTxt(void)
{
	return XbaeVersionTxt;
}

int XbaeGetVersionNum(void)
{
	return XbaeVersion;
}

/* 
 * The following 2 functions contain/return info about the
 * M*tif/LessTif version this library was built with.
 * Building against one toolkit and linking on runtime against
 * another is likely to fail!
 */
const char *XbaeGetXmVersionTxt(void)
{
	return XmVERSION_STRING;
}

int XbaeGetXmVersionNum(void)
{
	return XmVersion;
}

/**********************************************************************************************/

static XbaeMatrixWidget xbaeCheckClass(Widget w, const char *fcn)
{
	XbaeMatrixWidget mw = NULL;

	if(!w)
		return mw;

	if(XtIsSubclass(w, xbaeMatrixWidgetClass))
	{
		mw = (XbaeMatrixWidget) w;
	}
	else
	{
		char *msg1 = "XbaeMatrix: Not a matrixwidget in ";
		char *msg2 = XtMalloc(strlen(msg1) + strlen(fcn) + 1);
		strcpy(msg2, msg1);
		strcat(msg2, fcn);
		XtAppWarningMsg(XtWidgetToApplicationContext(w), fcn, "badWidget", "XbaeMatrix", msg2, NULL, 0);

	}
	return mw;
}

static Boolean xbaeCheckRow(XbaeMatrixWidget mw, int row, const char *fcn)
{
	if(row < 0 || row >= mw->matrix.rows)
	{
		char *msg1 = "XbaeMatrix: Row out of bounds in ";
		char *msg2 = XtMalloc(strlen(msg1) + strlen(fcn) + 1);
		strcpy(msg2, msg1);
		strcat(msg2, fcn);
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
						fcn, "badIndex", "XbaeMatrix", msg2, NULL, 0);

		return False;
	}
	return True;
}

static Boolean xbaeCheckColumn(XbaeMatrixWidget mw, int column, const char *fcn)
{
	if(column < 0 || column >= mw->matrix.columns)
	{
		char *msg1 = "XbaeMatrix: Column out of bounds in ";
		char *msg2 = XtMalloc(strlen(msg1) + strlen(fcn) + 1);
		strcpy(msg2, msg1);
		strcat(msg2, fcn);
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
						fcn, "badIndex", "XbaeMatrix", msg2, NULL, 0);

		return False;
	}
	return True;
}

/**********************************************************************************************/

/*
 * Public interface to set_cell method
 */
void XbaeMatrixSetCell(Widget w, int row, int column, const String value)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_cell method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_cell((XbaeMatrixWidget) w, row,
																column, value, True);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to edit_cell method
 */
void XbaeMatrixEditCell(Widget w, int row, int column)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the edit_cell method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.edit_cell((XbaeMatrixWidget) w, NULL,
																 row, column, NULL, 0);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to get_cell method
 */
String XbaeMatrixGetCell(Widget w, int row, int column)
{
	String s;

	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the get_cell method
	 */
	s = ((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.get_cell((XbaeMatrixWidget) w, row, column);

	xbaeObjectUnlock(w);

	return s;
}

/*
 * Public interface to commit_edit method
 */
Boolean XbaeMatrixCommitEdit(Widget w, Boolean unmap)
{
	Boolean b;

	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the commit_edit method
	 */
	b = ((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.commit_edit((XbaeMatrixWidget) w, NULL, unmap);

	xbaeObjectUnlock(w);

	return b;
}

/*
 * Public interface to cancel_edit method
 */
void XbaeMatrixCancelEdit(Widget w, Boolean unmap)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the cancel_edit method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.cancel_edit((XbaeMatrixWidget) w, unmap);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to set_show_column_arrows method
 */
void XbaeMatrixShowColumnArrows(Widget w, int column, Boolean show)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the select_row method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_show_column_arrows((XbaeMatrixWidget) w, column,
																			  show);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Public interface to add_rows method
 * ONLY use this method for rows with same heights
 */
void XbaeMatrixAddRows(Widget w, int position, String * rows, String * labels, Pixel * colors, int num_rows)
{
	xbaeObjectLock(w);

	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the add_rows method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.add_rows((XbaeMatrixWidget) w,
																position, rows, labels,
																colors, NULL, num_rows);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to add_rows method
 * This method should be used to support flexible height rows.
 */
void
XbaeMatrixAddVarRows(Widget w, int position, String * rows, String * labels, short *heights,
					 int *max_heights, unsigned char *alignments, unsigned char *label_alignments,
					 Pixel * colors, int num_rows)
{
	xbaeObjectLock(w);

	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the add_rows method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.add_var_rows((XbaeMatrixWidget) w,
																	position, rows, labels,
																	heights, max_heights,
																	alignments,
																	label_alignments, colors, NULL, num_rows);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to delete_rows method
 */
void XbaeMatrixDeleteRows(Widget w, int position, int num_rows)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the delete_rows method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.delete_rows((XbaeMatrixWidget) w, position, num_rows);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to add_columns method
 */
void
XbaeMatrixAddColumns(Widget w, int position, String * columns, String * labels, short *widths,
					 int *max_lengths, unsigned char *alignments, unsigned char *label_alignments,
					 Pixel * colors, int num_columns)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the add_columns method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.add_columns((XbaeMatrixWidget) w,
																   position, columns,
																   labels, widths,
																   max_lengths, alignments,
																   label_alignments, colors,
																   NULL, num_columns);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to delete_columns method
 */
void XbaeMatrixDeleteColumns(Widget w, int position, int num_columns)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the delete_columns method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.delete_columns((XbaeMatrixWidget) w,
																	  position, num_columns);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

Pixel XbaeMatrixGetCellColor(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;
	XbaeMatrixCellValuesStruct cell_values;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	xbaeGetCellValues(mw, row, column, False, &cell_values);
	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtFree((XtPointer) cell_values.drawCB.string);
	}

	xbaeObjectUnlock(w);
	return cell_values.drawCB.foreground;
}

Pixel XbaeMatrixGetCellBackground(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;
	XbaeMatrixCellValuesStruct cell_values;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	xbaeGetCellValues(mw, row, column, False, &cell_values);
	if((cell_values.drawCB.type & XbaeStringFree) == XbaeStringFree)
	{
		XtFree((XtPointer) cell_values.drawCB.string);
	}

	xbaeObjectUnlock(w);
	return cell_values.drawCB.background;
}

/*
 * Public interface to set_cell_color method
 */
void XbaeMatrixSetCellColor(Widget w, int row, int column, Pixel color)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_cell_color method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_cell_color((XbaeMatrixWidget) w,
																	  row, column, color, False);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to set_cell_color method
 */
void XbaeMatrixSetCellBackground(Widget w, int row, int column, Pixel color)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_cell_color method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_cell_color((XbaeMatrixWidget) w,
																	  row, column, color, True);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to set_row_colors method
 */
void XbaeMatrixSetRowColors(Widget w, int position, Pixel * colors, int num_colors)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_row_colors method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_row_colors((XbaeMatrixWidget) w,
																	  position, colors, num_colors, False);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to set_row_colors method
 */
void XbaeMatrixSetRowBackgrounds(Widget w, int position, Pixel * colors, int num_colors)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_row_colors method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_row_colors((XbaeMatrixWidget) w,
																	  position, colors, num_colors, True);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to set_column_colors method
 */
void XbaeMatrixSetColumnColors(Widget w, int position, Pixel * colors, int num_colors)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_column_colors method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_column_colors((XbaeMatrixWidget) w,
																		 position, colors, num_colors, False);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to set_column_colors method
 */
void XbaeMatrixSetColumnBackgrounds(Widget w, int position, Pixel * colors, int num_colors)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the set_column_colors method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.set_column_colors((XbaeMatrixWidget) w,
																		 position, colors, num_colors, True);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Public interface to select_cell method
 */
void XbaeMatrixSelectCell(Widget w, int row, int column)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the select_cell method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.select_cell((XbaeMatrixWidget) w, row, column);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to select_row method
 */
void XbaeMatrixSelectRow(Widget w, int row)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the select_row method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.select_row((XbaeMatrixWidget) w, row);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to select_column method
 */
void XbaeMatrixSelectColumn(Widget w, int column)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the select_column method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.select_column((XbaeMatrixWidget) w, column);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to select_all method
 */
void XbaeMatrixSelectAll(Widget w)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the deselect_all method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.select_all((XbaeMatrixWidget) w);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Public interface to deselect_cell method
 */
void XbaeMatrixDeselectCell(Widget w, int row, int column)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the deselect_cell method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.deselect_cell((XbaeMatrixWidget) w, row, column);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to deselect_row method
 */
void XbaeMatrixDeselectRow(Widget w, int row)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the deselect_row method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.deselect_row((XbaeMatrixWidget) w, row);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to deselect_column method
 */
void XbaeMatrixDeselectColumn(Widget w, int column)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the deselect_column method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.deselect_column((XbaeMatrixWidget) w, column);

	xbaeObjectUnlock(w);
}

/*
 * Public interface to deselect_all method
 */
void XbaeMatrixDeselectAll(Widget w)
{
	xbaeObjectLock(w);
	/*
	 * Make sure w is a Matrix or a subclass
	 */
	XtCheckSubclass(w, xbaeMatrixWidgetClass, NULL);

	/*
	 * Call the deselect_all method
	 */
	((XbaeMatrixWidgetClass) XtClass(w))->matrix_class.deselect_all((XbaeMatrixWidget) w);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

void XbaeMatrixUnderlineCell(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	/* If no cells have been underlined or deunderlined yet, allocate memory */
	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * If the cell is not already underlined, underline it and redraw it
	 */
	if(!mw->matrix.per_cell[row][column].underlined)
	{
		mw->matrix.per_cell[row][column].underlined = True;
		if(xbaeIsCellVisible(mw, row, column))
		{
			xbaeDrawCell(mw, row, column);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixUnderlineRow(Widget w, int row)
{
	int j, lc, rc;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	/* If no cells have been underlined or deunderlined yet, allocate memory */
	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * If the row is not visible, there's no need to redraw - but, we do
	 * need to update the underlined cell resource
	 */
	if(!xbaeIsRowVisible(mw, row))
	{
		for(j = 0; j < mw->matrix.columns; j++)
		{
			mw->matrix.per_cell[row][j].underlined = True;
		}
	}
	else
	{
		/*
		 * For each cell in the row, if the cell is not already underlined,
		 * underline it and redraw it
		 */
		xbaeGetVisibleColumns(mw, &lc, &rc);
		for(j = 0; j < mw->matrix.columns; j++)
		{
			if(!mw->matrix.per_cell[row][j].underlined)
			{
				mw->matrix.per_cell[row][j].underlined = True;
				if((j >= lc && j <= rc) || IS_FIXED_COLUMN(mw, j))
				{
					xbaeDrawCell(mw, row, j);
				}
			}
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixUnderlineColumn(Widget w, int column)
{
	int i, tr, br;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	/* If no cells have been underlined or deunderlined yet, allocate memory */
	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * No need to redraw unless the column is visible
	 */
	if(!xbaeIsColumnVisible(mw, column))
	{
		for(i = 0; i < mw->matrix.rows; i++)
		{
			mw->matrix.per_cell[i][column].underlined = True;
		}
	}
	else
	{
		/*
		 * For each cell in the column, if the cell is not already underlined,
		 * underline it and redraw it
		 */
		xbaeGetVisibleRows(mw, &tr, &br);
		for(i = 0; i < mw->matrix.rows; i++)
		{
			if(!mw->matrix.per_cell[i][column].underlined)
			{
				mw->matrix.per_cell[i][column].underlined = True;
				if((i >= tr && i <= br) || IS_FIXED_ROW(mw, i))
				{
					xbaeDrawCell(mw, i, column);
				}
			}
		}
	}

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

void XbaeMatrixDeunderlineCell(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	/*
	 * If the cell is already underlined, deunderline it and redraw it
	 */
	if(mw->matrix.per_cell[row][column].underlined)
	{
		mw->matrix.per_cell[row][column].underlined = False;
		if(xbaeIsCellVisible(mw, row, column))
		{
			xbaeDrawCell(mw, row, column);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixDeunderlineRow(Widget w, int row)
{
	int j, lc, rc;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	/*
	 * If the row is not visible, there's no need to redraw - but, we do
	 * need to update the underlined cell resource
	 */
	if(!xbaeIsRowVisible(mw, row))
	{
		for(j = 0; j < mw->matrix.columns; j++)
		{
			mw->matrix.per_cell[row][j].underlined = False;
		}
	}
	else
	{
		/*
		 * For each cell in the row, if the cell is not already underlined,
		 * underline it and redraw it
		 */
		xbaeGetVisibleColumns(mw, &lc, &rc);
		for(j = 0; j < mw->matrix.columns; j++)
		{
			if(mw->matrix.per_cell[row][j].underlined)
			{
				mw->matrix.per_cell[row][j].underlined = False;
				if((j >= lc && j <= rc) || IS_FIXED_COLUMN(mw, j))
				{
					xbaeDrawCell(mw, row, j);
				}
			}
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixDeunderlineColumn(Widget w, int column)
{
	int i, tr, br;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	/*
	 * No need to redraw unless the column is visible
	 */
	if(!xbaeIsColumnVisible(mw, column))
	{
		for(i = 0; i < mw->matrix.rows; i++)
		{
			mw->matrix.per_cell[i][column].underlined = False;
		}
	}
	else
	{
		/*
		 * For each cell in the column, if the cell is already underlined,
		 * deunderline it and redraw it
		 */
		xbaeGetVisibleRows(mw, &tr, &br);
		for(i = 0; i < mw->matrix.rows; i++)
		{
			if(mw->matrix.per_cell[i][column].underlined)
			{
				mw->matrix.per_cell[i][column].underlined = False;
				if((i >= tr && i <= br) || IS_FIXED_ROW(mw, i))
				{
					xbaeDrawCell(mw, i, column);
				}
			}
		}
	}

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Help the user know what row & column he is in given an x & y (via an event).
 * Return True on success, False on failure.
 */
int XbaeMatrixGetEventRowColumn(Widget w, XEvent * event, int *row, int *column)
{
	int x, y;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return False;
	}

	/* Convert the event to the correct XY for the matrix widget. */
	if(!xbaeEventToMatrixXY(mw, event, &x, &y))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	/* Convert the point to a row,column. If it does not pick a valid cell,
	   then return. */
	if(!xbaeMatrixXYToRowCol(mw, &x, &y, row, column))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	xbaeObjectUnlock(w);

	return True;
}

/*
 * Public interface for xbaeEventToXY()
 */
Boolean XbaeMatrixEventToXY(Widget w, XEvent * event, int *x, int *y)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return False;
	}

	if(!xbaeEventToMatrixXY(mw, event, x, y))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	/* Tobias: FIXME: Using xbaeEventToMatrixXY instead of the old xbaeEventToXY
	 * changes this functions semantics and may break 3rd party code. 
	 * I'm not sure if it's worth fixing as the old semantics are unusable (cell isn't returned)
	 */

	xbaeObjectUnlock(w);

	return True;
}

/*
 * Public interface for xbaeRowColToXY().  From Philip Aston
 * (philipa@parallax.co.uk)
 */
Boolean XbaeMatrixRowColToXY(Widget w, int row, int column, int *x, int *y)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	xbaeRowColToClipXY(mw, row, column, x, y);

	xbaeObjectUnlock(w);

	return True;
}

/**********************************************************************************************/

/*
 * Help the programmer to know what row & column we are currently at.
 */
void XbaeMatrixGetCurrentCell(Widget w, int *row, int *column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	/* 
	 * Make sure we return something valid. 
	 * At least Grace depends on it... 
	 */

	if(mw->matrix.current_row >= 0 && mw->matrix.current_row < mw->matrix.rows)
	{
		*row = mw->matrix.current_row;
	}
	else
	{
		*row = xbaeTopRow(mw);
	}

	if(mw->matrix.current_column >= 0 && mw->matrix.current_column < mw->matrix.columns)
	{
		*column = mw->matrix.current_column;
	}
	else
	{
		*column = xbaeLeftColumn(mw);
	}

	xbaeObjectUnlock(w);
}

/*
 * Set current cell
 */
void XbaeMatrixSetCurrentCellPosition(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	mw->matrix.current_row = row;
	mw->matrix.current_column = column;

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Allow the programmer to call the Expose method directly. This should only
 * be needed in drawcellCB mode.
 */
void XbaeMatrixRefresh(Widget w)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		xbaeRefresh(mw, True);

		if(mw->matrix.text_child_is_mapped)
		{
			xbaeUpdateTextChild(mw, True);
		}
	}

	xbaeObjectUnlock(w);
}

/*
 * Public interface for redrawing one cell
 */
void XbaeMatrixRefreshCell(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(xbaeIsCellVisible(mw, row, column))
	{
		xbaeDrawCell(mw, row, column);
	}

	if(mw->matrix.text_child_is_mapped
	   && row == mw->matrix.current_row && column == mw->matrix.current_column)
	{
		xbaeUpdateTextChild(mw, True);
	}

	xbaeObjectUnlock(w);
}

/*
 * Redraw an entire column
 */
void XbaeMatrixRefreshColumn(Widget w, int column)
{
	int row, tr, br;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	/* we attempt to be mildly efficient about this */
	if(xbaeIsColumnVisible(mw, column))
	{
		xbaeGetVisibleRows(mw, &tr, &br);
		for(row = 0; row < mw->matrix.rows; row++)
		{
			if((row >= tr && row <= br) || IS_FIXED_ROW(mw, row))
			{
				xbaeDrawCell(mw, row, column);
			}
		}
	}

	if(mw->matrix.text_child_is_mapped && column == mw->matrix.current_column)
	{
		xbaeUpdateTextChild(mw, True);
	}

	xbaeObjectUnlock(w);
}

/*
 * Redraw an entire row
 */
void XbaeMatrixRefreshRow(Widget w, int row)
{
	int column, lc, rc;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	/* we attempt to be mildly efficient about this */
	if(xbaeIsRowVisible(mw, row))
	{
		xbaeGetVisibleColumns(mw, &lc, &rc);
		for(column = 0; column < mw->matrix.columns; column++)
		{
			if((column >= lc && column <= rc) || IS_FIXED_COLUMN(mw, column))
			{
				xbaeDrawCell(mw, row, column);
			}
		}
	}

	if(mw->matrix.text_child_is_mapped && row == mw->matrix.current_row)
	{
		xbaeUpdateTextChild(mw, True);
	}

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Get per-cell user data
 */
XtPointer XbaeMatrixGetCellUserData(Widget w, int row, int column)
{
	XtPointer data;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(mw->matrix.per_cell)
	{
		data = mw->matrix.per_cell[row][column].user_data;
	}
	else
	{
		data = NULL;
	}

	xbaeObjectUnlock(w);

	return data;
}

/*
 * Set per-cell user data
 */
void XbaeMatrixSetCellUserData(Widget w, int row, int column, XtPointer data)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	mw->matrix.per_cell[row][column].user_data = data;

	xbaeObjectUnlock(w);
}

/*
 * Get per-row user data
 */
XtPointer XbaeMatrixGetRowUserData(Widget w, int row)
{
	XtPointer data;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(mw->matrix.row_user_data)
		data = mw->matrix.row_user_data[row];
	else
	{
		data = NULL;
	}

	xbaeObjectUnlock(w);

	return data;
}


/*
 * Set per-row user data
 */
void XbaeMatrixSetRowUserData(Widget w, int row, XtPointer data)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.row_user_data)
	{
		mw->matrix.row_user_data = (XtPointer *) XtCalloc(mw->matrix.rows, sizeof(XtPointer));
	}

	mw->matrix.row_user_data[row] = data;

	xbaeObjectUnlock(w);
}

/*
 * Get per-column user data
 */
XtPointer XbaeMatrixGetColumnUserData(Widget w, int column)
{
	XtPointer data;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(mw->matrix.column_user_data)
		data = mw->matrix.column_user_data[column];
	else
	{
		data = NULL;
	}

	xbaeObjectUnlock(w);

	return data;
}

/*
 * Set per-column user data
 */
void XbaeMatrixSetColumnUserData(Widget w, int column, XtPointer data)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.column_user_data)
	{
		mw->matrix.column_user_data = (XtPointer *) XtCalloc(mw->matrix.columns, sizeof(XtPointer));
	}

	mw->matrix.column_user_data[column] = data;

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/* 
 * ARCAD SYSTEMHAUS
 */
int XbaeMatrixGetCellPixmap(Widget w, int row, int column, Pixmap * pixmap, Pixmap * mask)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return -1;
	}

	if(mw->matrix.per_cell)
	{
		*pixmap = mw->matrix.per_cell[row][column].pixmap;
		*mask = mw->matrix.per_cell[row][column].mask;
		xbaeObjectUnlock(w);
		return 0;
	}

	xbaeObjectUnlock(w);

	return -1;
}

void XbaeMatrixSetCellPixmap(Widget w, int row, int column, Pixmap pixmap, Pixmap mask)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	mw->matrix.per_cell[row][column].pixmap = pixmap;
	mw->matrix.per_cell[row][column].mask = mask;

	if(xbaeIsCellVisible(mw, row, column))
		xbaeDrawCell(mw, row, column);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Set per-cell widget
 */
void XbaeMatrixSetCellWidget(Widget w, int row, int column, Widget widget)
{
	XbaeMatrixWidget mw;
	Widget old;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(widget && XmIsGadget(widget))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext(w), "SetCellWidget",
						"child is a gadget", "XbaeMatrix",
						"XbaeMatrix: the child is a gadget - currently unsupported", NULL, 0);
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeCreatePerCell(mw);
	}

	old = mw->matrix.per_cell[row][column].widget;

	if(old != widget)
	{
		mw->matrix.per_cell[row][column].widget = widget;
		xbaeSetInitialFocus(mw);

		/*
		 * If the matrix is not yet realized we don't have to move windows arround
		 */
		if(XtIsRealized((Widget) mw))
		{

			/* 
			 * Obscure the (former) cell widget 
			 */
			if(old)
			{
				xbaeHideCellWidget(mw, old);
			}

			/*
			 * If we're just removing a cell widget, we're done
			 */
			if(widget)
			{
				/*
				 * If the textChild happens to be in this cell, hide it
				 */
				if(mw->matrix.text_child_is_mapped
				   && row == mw->matrix.current_row && column == mw->matrix.current_column)
				{
					xbaeHideTextChild(mw);
				}

				/*
				 * Make sure the widget is realized before trying to set it's position
				 */
				if(!XtIsRealized(widget))
					XtRealizeWidget(widget);

				xbaePositionCellWidget(mw, row, column);
			}
		}
	}

	xbaeObjectUnlock(w);
}

Widget XbaeMatrixGetCellWidget(Widget w, int row, int column)
{
	Widget r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(mw->matrix.per_cell)
	{
		r = mw->matrix.per_cell[row][column].widget;
	}
	else
	{
		r = NULL;
	}

	xbaeObjectUnlock(w);

	return r;
}

/**********************************************************************************************/

void XbaeMatrixSetCellTag(Widget w, int row, int column, XmStringTag tag)
{
	XrmQuark qtag;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/* Quarkify the tag and set the cells qtag to it */
	qtag = (tag)? XrmStringToQuark(tag) : NULLQUARK;
	if(mw->matrix.per_cell[row][column].qtag != qtag)
	{

		mw->matrix.per_cell[row][column].qtag = qtag;

		if(xbaeIsCellVisible(mw, row, column))
			xbaeDrawCell(mw, row, column);

		if(mw->matrix.text_child_is_mapped
		   && row == mw->matrix.current_row && column == mw->matrix.current_column)
		{
			/* Update the font the textChild uses */
			xbaeUpdateTextChildFont(mw, qtag);
			/* we need to reset the size of the text child as the above has adverse effects */
			xbaePositionTextChild(mw);
		}
	}

	xbaeObjectUnlock(w);
}

XmStringTag XbaeMatrixGetCellTag(Widget w, int row, int column)
{
	XmStringTag tag = NULL;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(mw->matrix.per_cell[row][column].qtag != NULLQUARK)
		tag = XrmQuarkToString(mw->matrix.per_cell[row][column].qtag);

	xbaeObjectUnlock(w);

	return tag;
}

/**********************************************************************************************/

Boolean XbaeMatrixIsCellSelected(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return False;
	}

	if(!mw->matrix.per_cell[row][column].selected)
	{
		xbaeObjectUnlock(w);
		return False;
	}

	xbaeObjectUnlock(w);

	return True;
}

Boolean XbaeMatrixIsRowSelected(Widget w, int row)
{
	int col;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return False;
	}

	/*
	 * Check all the cells in the row
	 */
	for(col = 0; col < mw->matrix.columns; col++)
	{
		if(!mw->matrix.per_cell[row][col].selected)
		{
			xbaeObjectUnlock(w);
			return False;
		}
	}

	xbaeObjectUnlock(w);

	return True;
}

Boolean XbaeMatrixIsColumnSelected(Widget w, int column)
{
	int row;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return False;
	}

	/*
	 * Check all the cells in the row
	 */
	for(row = 0; row < mw->matrix.rows; row++)
	{
		if(!mw->matrix.per_cell[row][column].selected)
		{
			xbaeObjectUnlock(w);
			return False;
		}
	}

	xbaeObjectUnlock(w);

	return True;
}

int XbaeMatrixFirstSelectedRow(Widget w)
{
	int i;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return -1;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return -1;
	}

	/*
	 * Linear search for first selected
	 */
	for(i = 0; i < mw->matrix.rows; i++)
	{
		if(XbaeMatrixIsRowSelected(w, i))
		{
			xbaeObjectUnlock(w);
			return i;
		}
	}

	xbaeObjectUnlock(w);

	return -1;
}

int XbaeMatrixFirstSelectedColumn(Widget w)
{
	int i;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return -1;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return -1;
	}

	/*
	 * Linear search for first selected
	 */
	for(i = 0; i < mw->matrix.columns; i++)
	{
		if(XbaeMatrixIsColumnSelected(w, i))
		{
			xbaeObjectUnlock(w);
			return i;
		}
	}

	xbaeObjectUnlock(w);

	return -1;
}

void XbaeMatrixFirstSelectedCell(Widget w, int *row, int *column)
{
	int i, j;
	XbaeMatrixWidget mw;

	*row = -1;
	*column = -1;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	for(i = 0; i < mw->matrix.rows; i++)
	{
		for(j = 0; j < mw->matrix.columns; j++)
		{
			if(mw->matrix.per_cell[i][j].selected)
			{
				*row = i;
				*column = j;
				xbaeObjectUnlock(w);
				return;
			}
		}
	}

	xbaeObjectUnlock(w);
}

int XbaeMatrixGetNumSelected(Widget w)
{
	int i;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	i = mw->matrix.num_selected_cells;

	xbaeObjectUnlock(w);

	return i;
}

int XbaeMatrixNumColumns(Widget w)
{
	int i;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	i = mw->matrix.columns;

	xbaeObjectUnlock(w);

	return i;
}

int XbaeMatrixNumRows(Widget w)
{
	int i;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	i = mw->matrix.rows;

	xbaeObjectUnlock(w);

	return i;
}

/**********************************************************************************************/

void XbaeMatrixSetCellShadow(Widget w, int row, int column, unsigned char shadow_type)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Change the shadow and redraw the cell
	 */
	if(mw->matrix.per_cell[row][column].shadow_type != shadow_type)
	{
		mw->matrix.per_cell[row][column].shadow_type = shadow_type;
		if(xbaeIsCellVisible(mw, row, column))
		{
			xbaeDrawCell(mw, row, column);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixSetRowShadow(Widget w, int row, unsigned char shadow_type)
{
	XbaeMatrixWidget mw;
	int r;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.row_shadow_types)
	{
		mw->matrix.row_shadow_types = (unsigned char *) XtMalloc(mw->matrix.rows *
																 sizeof *mw->matrix.row_shadow_types);
		for(r = 0; r < mw->matrix.rows; r++)
		{
			mw->matrix.row_shadow_types[r] = 0;
		}
	}

	if(mw->matrix.row_shadow_types[row] != shadow_type)
	{
		mw->matrix.row_shadow_types[row] = shadow_type;
		if(IN_GRID_ROW_MODE(mw) && xbaeIsRowVisible(mw, row))
		{
			XbaeMatrixRefreshRow((Widget) mw, row);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixSetColumnShadow(Widget w, int column, unsigned char shadow_type)
{
	XbaeMatrixWidget mw;
	int c;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.column_shadow_types)
	{
		mw->matrix.column_shadow_types = (unsigned char *) XtMalloc(mw->matrix.columns *
																	sizeof *mw->matrix.column_shadow_types);
		for(c = 0; c < mw->matrix.rows; c++)
		{
			mw->matrix.column_shadow_types[c] = 0;
		}
	}

	if(mw->matrix.column_shadow_types[column] != shadow_type)
	{
		mw->matrix.column_shadow_types[column] = shadow_type;
		if(IN_GRID_COLUMN_MODE(mw) && xbaeIsColumnVisible(mw, column))
		{
			XbaeMatrixRefreshColumn((Widget) mw, column);
		}
	}

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

void XbaeMatrixUnhighlightAll(Widget w)
{
	int row, column;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	for(row = 0; row < mw->matrix.rows; row++)
	{
		for(column = 0; column < mw->matrix.columns; column++)
		{
			/*
			 * If the cell is visible and highlighted
			 */
			if(mw->matrix.per_cell[row][column].highlighted)
			{
				unsigned char new_hl = HighlightNone;
				if(xbaeIsCellVisible(mw, row, column))
					xbaeChangeHighlight(mw, row, column, new_hl);
				mw->matrix.per_cell[row][column].highlighted = new_hl;
			}
		}
	}

	xbaeObjectUnlock(w);
}


void XbaeMatrixHighlightCell(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	/*
	 * Scroll the cell onto the screen
	 */
	if(mw->matrix.scroll_select)
		xbaeMakeCellVisible(mw, row, column);

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * If the cell is not already highlighted
	 */
	if(!(mw->matrix.per_cell[row][column].highlighted & HighlightCell))
	{
		unsigned char new_hl = mw->matrix.per_cell[row][column].highlighted | HighlightCell;
		if(xbaeIsCellVisible(mw, row, column))
			xbaeChangeHighlight(mw, row, column, new_hl);
		mw->matrix.per_cell[row][column].highlighted = new_hl;
	}

	xbaeObjectUnlock(w);
}


void XbaeMatrixUnhighlightCell(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(mw->matrix.per_cell[row][column].highlighted & HighlightCell)
	{
		unsigned char new_hl = mw->matrix.per_cell[row][column].highlighted & ~HighlightCell;
		if(xbaeIsCellVisible(mw, row, column))
			xbaeChangeHighlight(mw, row, column, new_hl);
		mw->matrix.per_cell[row][column].highlighted = new_hl;
	}

	xbaeObjectUnlock(w);
}


void XbaeMatrixHighlightRow(Widget w, int row)
{
	int column;
	Boolean visible;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Scroll the row onto the screen
	 */
	if(mw->matrix.scroll_select)
		xbaeMakeRowVisible(mw, row);

	/*
	 * For each cell in the row, if the cell is not already highlighted,
	 * highlight it and redraw it if it is visible
	 */

	visible = xbaeIsRowVisible(mw, row);

	for(column = 0; column < mw->matrix.columns; column++)
	{
		if(!(mw->matrix.per_cell[row][column].highlighted & HighlightRow))
		{
			unsigned char new_hl = mw->matrix.per_cell[row][column].highlighted | HighlightRow;
			if(visible && xbaeIsColumnVisible(mw, column))
				xbaeChangeHighlight(mw, row, column, new_hl);
			mw->matrix.per_cell[row][column].highlighted = new_hl;
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixUnhighlightRow(Widget w, int row)
{
	int column;
	Boolean visible;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	visible = xbaeIsRowVisible(mw, row);

	/*
	 * For each cell in the row, if the cell is highlighted,
	 * unhighlight it and redraw it if it is visible
	 */

	for(column = 0; column < mw->matrix.columns; column++)
	{
		if(mw->matrix.per_cell[row][column].highlighted & HighlightRow)
		{
			unsigned char new_hl = mw->matrix.per_cell[row][column].highlighted & ~HighlightRow;
			if(visible && xbaeIsColumnVisible(mw, column))
				xbaeChangeHighlight(mw, row, column, new_hl);
			mw->matrix.per_cell[row][column].highlighted = new_hl;
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixHighlightColumn(Widget w, int column)
{
	int row;
	Boolean visible;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
		xbaeCreatePerCell(mw);

	/*
	 * Scroll the row onto the screen
	 */
	if(mw->matrix.scroll_select)
		xbaeMakeColumnVisible(mw, column);

	/*
	 * For each cell in the column, if the cell is not already highlighted,
	 * highlight it and redraw it if it is visible
	 */

	visible = xbaeIsColumnVisible(mw, column);
	for(row = 0; row < mw->matrix.rows; row++)
	{
		if(!(mw->matrix.per_cell[row][column].highlighted & HighlightColumn))
		{
			unsigned char new_hl = mw->matrix.per_cell[row][column].highlighted | HighlightColumn;
			if(visible && xbaeIsRowVisible(mw, row))
				xbaeChangeHighlight(mw, row, column, new_hl);
			mw->matrix.per_cell[row][column].highlighted = new_hl;
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixUnhighlightColumn(Widget w, int column)
{
	int row;
	Boolean visible;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(!mw->matrix.per_cell)
	{
		xbaeObjectUnlock(w);
		return;
	}

	visible = xbaeIsColumnVisible(mw, column);

	/*
	 * For each cell in the row, if the cell is highlighted,
	 * unhighlight it and redraw it if it is visible.
	 */
	for(row = 0; row < mw->matrix.rows; row++)
	{
		if(mw->matrix.per_cell[row][column].highlighted & HighlightColumn)
		{
			unsigned char new_hl = mw->matrix.per_cell[row][column].highlighted & ~HighlightColumn;
			if(visible && xbaeIsRowVisible(mw, row))
				xbaeChangeHighlight(mw, row, column, new_hl);
			mw->matrix.per_cell[row][column].highlighted = new_hl;
		}
	}

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

void XbaeMatrixDisableRedisplay(Widget w)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	mw->matrix.disable_redisplay++;
	xbaeObjectUnlock(w);
}


void XbaeMatrixEnableRedisplay(Widget w, Boolean redisplay)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw != NULL)
	{
		if(mw->matrix.disable_redisplay)
			mw->matrix.disable_redisplay--;

		if(redisplay && mw->matrix.disable_redisplay == 0)
			XbaeMatrixRefresh(w);
	}
	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Public interface for xbaeIsCellVisible()
 */
Boolean XbaeMatrixIsCellVisible(Widget w, int row, int column)
{
	Boolean r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	r = xbaeIsCellVisible(mw, row, column);

	xbaeObjectUnlock(w);

	return r;
}

/*
 * Public interface for xbaeIsRowVisible()
 */
Boolean XbaeMatrixIsRowVisible(Widget w, int row)
{
	Boolean r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	r = xbaeIsRowVisible(mw, row);

	xbaeObjectUnlock(w);

	return r;
}

/*
 * Public interface for xbaeIsColumnVisible()
 */
Boolean XbaeMatrixIsColumnVisible(Widget w, int column)
{
	Boolean r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return False;
	}

	r = xbaeIsColumnVisible(mw, column);

	xbaeObjectUnlock(w);

	return r;
}

/*
 *  This routine returns the number of rows that are visible in the matrix.
 */
int XbaeMatrixVisibleRows(Widget w)
{
	int top_row;
	int bottom_row;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	xbaeGetVisibleRows(mw, &top_row, &bottom_row);

	xbaeObjectUnlock(w);

	return bottom_row - top_row + 1;
}

/*
 *  This routine returns the number of columns that are visible in the matrix.
 */
int XbaeMatrixVisibleColumns(Widget w)
{
	int left_column;
	int right_column;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	xbaeGetVisibleColumns(mw, &left_column, &right_column);

	xbaeObjectUnlock(w);

	return right_column - left_column + 1;

}

/*
 *  This routine returns the range of cells that are visible in the matrix.
 */
void XbaeMatrixVisibleCells(Widget w, int *top_row, int *bottom_row, int *left_column, int *right_column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	xbaeGetVisibleRows(mw, top_row, bottom_row);
	xbaeGetVisibleColumns(mw, left_column, right_column);

	xbaeObjectUnlock(w);
}

/*
 * Public interface for xbaeMakeCellVisible()
 */
void XbaeMatrixMakeCellVisible(Widget w, int row, int column)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row) || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	xbaeMakeCellVisible(mw, row, column);

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

/*
 * Get the label of the column passed here.
 */
String XbaeMatrixGetColumnLabel(Widget w, int column)
{
	String r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(!mw->matrix.column_labels)
	{
		r = NULL;
	}
	else
	{
		r = mw->matrix.column_labels[column];
	}

	xbaeObjectUnlock(w);

	return r;
}

XmString XbaeMatrixGetXmColumnLabel(Widget w, int column)
{
	XmString r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(!mw->matrix.xmcolumn_labels)
	{
		r = NULL;
	}
	else
	{
		r = mw->matrix.xmcolumn_labels[column];
	}

	xbaeObjectUnlock(w);

	return r;
}

/*
 * Get the label of the row passed here.
 */
String XbaeMatrixGetRowLabel(Widget w, int row)
{
	String r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(!mw->matrix.row_labels)
	{
		r = NULL;
	}
	else
	{
		r = mw->matrix.row_labels[row];
	}

	xbaeObjectUnlock(w);

	return r;
}

XmString XbaeMatrixGetXmRowLabel(Widget w, int row)
{
	XmString r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return NULL;
	}

	if(!mw->matrix.xmrow_labels)
	{
		r = NULL;
	}
	else
	{
		r = mw->matrix.xmrow_labels[row];
	}

	xbaeObjectUnlock(w);

	return r;
}

void XbaeMatrixSetColumnLabel(Widget w, int column, String value)
{
	XbaeMatrixWidget mw;
	int column_label_height;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	column_label_height = COLUMN_LABEL_HEIGHT(mw);

	if(!mw->matrix.column_labels)
	{
		/*
		 * We have no labels so far. Create empty ones for all the columns.
		 */
		int c;
		mw->matrix.column_labels = (String *) XtMalloc(mw->matrix.columns * sizeof(String));
		for(c = 0; c < mw->matrix.columns; c++)
		{
			mw->matrix.column_labels[c] = NULL;
		}
	}
	else if(mw->matrix.column_labels[column])
	{
		/*
		 * Free the memory that was used by the old label.
		 */
		XtFree((char *) mw->matrix.column_labels[column]);
	}

	/*
	 * Copy the new value and update column_label_maxlines.
	 */
	mw->matrix.column_labels[column] = (value == NULL) ? NULL : XtNewString(value);
	mw->matrix.column_label_maxlines = xbaeCalculateLabelMaxLines(mw->matrix.column_labels,
																  mw->matrix.xmcolumn_labels,
																  mw->matrix.columns);

	/*
	 * If COLUMN_LABEL_HEIGHT changed, redraw the whole matrix 
	 * else redraw only the label that was changed
	 */
	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		if(column_label_height != COLUMN_LABEL_HEIGHT(mw))
		{
			xbaeRefresh(mw, True);
		}
		else if(xbaeIsColumnVisible(mw, column))
		{
			xbaeDrawColumnLabel(mw, column, False);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixSetXmColumnLabel(Widget w, int column, XmString value)
{
	XbaeMatrixWidget mw;
	int column_label_height;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	column_label_height = COLUMN_LABEL_HEIGHT(mw);

	if(!mw->matrix.xmcolumn_labels)
	{
		/*
		 * We have no labels so far. Create empty ones for all the columns.
		 */
		int c;
		mw->matrix.xmcolumn_labels = (XmString *) XtMalloc(mw->matrix.columns * sizeof(XmString));
		for(c = 0; c < mw->matrix.columns; c++)
		{
			mw->matrix.xmcolumn_labels[c] = NULL;
		}
	}
	else if(mw->matrix.column_labels[column])
	{
		/*
		 * Free the memory that was used by the old label.
		 */
		XmStringFree(mw->matrix.xmcolumn_labels[column]);
	}

	/*
	 * Copy the new value and update column_label_maxlines.
	 */
	mw->matrix.xmcolumn_labels[column] = (value == NULL) ? NULL : XmStringCopy(value);
	mw->matrix.column_label_maxlines = xbaeCalculateLabelMaxLines(mw->matrix.column_labels,
																  mw->matrix.xmcolumn_labels,
																  mw->matrix.columns);

	/*
	 * If COLUMN_LABEL_HEIGHT changed, redraw the whole matrix 
	 * else redraw only the label that was changed
	 */
	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		if(column_label_height != COLUMN_LABEL_HEIGHT(mw))
		{
			xbaeRefresh(mw, True);
		}
		else if(xbaeIsColumnVisible(mw, column))
		{
			xbaeDrawColumnLabel(mw, column, False);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixSetRowLabel(Widget w, int row, String value)
{
	XbaeMatrixWidget mw;
	int row_label_width;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	row_label_width = ROW_LABEL_WIDTH(mw);

	if(!mw->matrix.row_labels)
	{
		/*
		 * We have no labels so far. Create empty ones for all the rows.
		 */
		int r;
		mw->matrix.row_labels = (String *) XtMalloc(mw->matrix.rows * sizeof(String));
		for(r = 0; r < mw->matrix.rows; r++)
		{
			mw->matrix.row_labels[r] = NULL;
		}
	}
	else if(mw->matrix.row_labels[row])
	{
		/*
		 * Free the memory that was used by the old label.
		 */
		XtFree((XtPointer) mw->matrix.row_labels[row]);
	}

	/*
	 * Copy the new value and update row_label_maxwidth.
	 */
	mw->matrix.row_labels[row] = (value == NULL) ? NULL : XtNewString(value);
	mw->matrix.row_label_maxwidth = xbaeCalculateLabelMaxWidth(mw,
																 mw->matrix.row_labels,
																 mw->matrix.xmrow_labels, mw->matrix.rows);

	/*
	 * If ROW_LABEL_WIDTH changed, redraw the whole matrix 
	 * else redraw only the label that was changed
	 */
	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		if(row_label_width != ROW_LABEL_WIDTH(mw))
		{
			xbaeRefresh(mw, True);
		}
		else if(xbaeIsRowVisible(mw, row))
		{
			xbaeDrawRowLabel(mw, row, False);
		}
	}

	xbaeObjectUnlock(w);
}

void XbaeMatrixSetXmRowLabel(Widget w, int row, XmString value)
{
	XbaeMatrixWidget mw;
	int row_label_width;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	row_label_width = ROW_LABEL_WIDTH(mw);

	if(!mw->matrix.xmrow_labels)
	{
		/*
		 * We have no labels so far. Create empty ones for all the rows.
		 */
		int r;
		mw->matrix.xmrow_labels = (XmString *) XtMalloc(mw->matrix.rows * sizeof(XmString));
		for(r = 0; r < mw->matrix.rows; r++)
		{
			mw->matrix.xmrow_labels[r] = NULL;
		}
	}
	else if(mw->matrix.xmrow_labels[row])
	{
		/*
		 * Free the memory that was used by the old label.
		 */
		XmStringFree(mw->matrix.xmrow_labels[row]);
	}

	/*
	 * Copy the new value and update row_label_maxwidth.
	 */
	mw->matrix.xmrow_labels[row] = (value == NULL) ? NULL : XmStringCopy(value);
	mw->matrix.row_label_maxwidth = xbaeCalculateLabelMaxWidth(mw,
																 mw->matrix.row_labels,
																 mw->matrix.xmrow_labels, mw->matrix.rows);

	/*
	 * If ROW_LABEL_WIDTH changed, redraw the whole matrix 
	 * else redraw only the label that was changed
	 */
	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		if(row_label_width != ROW_LABEL_WIDTH(mw))
		{
			xbaeRefresh(mw, True);
		}
		else if(xbaeIsRowVisible(mw, row))
		{
			xbaeDrawRowLabel(mw, row, False);
		}
	}

	xbaeObjectUnlock(w);
}

/**********************************************************************************************/

void XbaeMatrixSetColumnWidth(Widget w, int column, int width)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(width < 1)
	{
		mw->matrix.column_widths[column] = DEFAULT_COLUMN_WIDTH(mw);
	}
	else
	{
		mw->matrix.column_widths[column] = width;
	}

	xbaeGetColumnPositions(mw);

	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		xbaeRefresh(mw, True);
	}

	xbaeObjectUnlock(w);
}

int XbaeMatrixGetColumnWidth(Widget w, int column)
{
	int r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_COLUMN(mw, column))
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	r = mw->matrix.column_widths[column];

	xbaeObjectUnlock(w);

	return r;
}

void XbaeMatrixSetRowHeight(Widget w, int row, int height)
{
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(height < 1)
	{
		mw->matrix.row_heights[row] = DEFAULT_ROW_HEIGHT(mw);
	}
	else
	{
		mw->matrix.row_heights[row] = height;
	}

	xbaeGetRowPositions(mw);

	if(!mw->matrix.disable_redisplay && XtIsRealized(w))
	{
		xbaeRefresh(mw, True);
	}

	xbaeObjectUnlock(w);
}

int XbaeMatrixGetRowHeight(Widget w, int row)
{
	int r;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL || !XBAE_CHECK_ROW(mw, row))
	{
		xbaeObjectUnlock(w);
		return 0;
	}

	r = mw->matrix.row_heights[row];

	xbaeObjectUnlock(w);

	return r;
}

/**********************************************************************************************/

struct sort_common {
	Widget w;
	int (*proc) (Widget, int, int, void *);
	void *user_data;
};

struct sort_index {
	int index;
	struct sort_common *common;
};

static int compare(const void *a, const void *b)
{
	const struct sort_index *sort_index1 = a;
	const struct sort_index *sort_index2 = b;
	const struct sort_common *common = sort_index1->common;

	return common->proc(common->w, sort_index1->index, sort_index2->index, common->user_data);
}

void *reorder(void *array, size_t size, int n, struct sort_index *sort_indices)
{
	void *ordered_array = NULL;

	if(array)
	{
		int i;
		ordered_array = XtMalloc(n * size);

		for(i = 0; i < n; i++)
		{
			memcpy((char *) ordered_array + size * i, (char *) array + size * sort_indices[i].index, size);
		}

		XtFree(array);
	}

	return ordered_array;
}

void XbaeMatrixSortRows(Widget w, int (*proc) (Widget, int, int, void *), void *user_data)
{
	int row;
	int n_rows;

	struct sort_common common;
	struct sort_index *row_indices;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(mw->matrix.rows == 0 || mw->matrix.columns == 0 || mw->matrix.per_cell == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	n_rows = mw->matrix.rows;

	common.w = w;
	common.proc = proc;
	common.user_data = user_data;

	row_indices = (struct sort_index *) XtMalloc(n_rows * sizeof *row_indices);

	for(row = 0; row < n_rows; row++)
	{
		row_indices[row].index = row;
		row_indices[row].common = &common;
	}

	qsort(row_indices, n_rows, sizeof *row_indices, compare);

	mw->matrix.per_cell = reorder(mw->matrix.per_cell, sizeof *mw->matrix.per_cell, n_rows, row_indices);
	mw->matrix.row_button_labels = reorder(mw->matrix.row_button_labels,
										   sizeof *mw->matrix.row_button_labels, n_rows, row_indices);
	mw->matrix.row_user_data = reorder(mw->matrix.row_user_data,
									   sizeof *mw->matrix.row_user_data, n_rows, row_indices);
	mw->matrix.row_shadow_types = reorder(mw->matrix.row_shadow_types,
										  sizeof *mw->matrix.row_shadow_types, n_rows, row_indices);
	mw->matrix.xmrow_labels = reorder(mw->matrix.xmrow_labels,
									  sizeof *mw->matrix.xmrow_labels, n_rows, row_indices);
	mw->matrix.row_labels = reorder(mw->matrix.row_labels,
									sizeof *mw->matrix.row_labels, n_rows, row_indices);
	mw->matrix.row_heights = reorder(mw->matrix.row_heights,
									 sizeof *mw->matrix.row_heights, n_rows, row_indices);

	XtFree((XtPointer) row_indices);

	xbaeGetRowPositions(mw);

	XbaeMatrixRefresh(w);

	xbaeObjectUnlock(w);
}


void XbaeMatrixSortColumns(Widget w, int (*proc) (Widget, int, int, void *), void *user_data)
{
	int row, column;
	int n_rows, n_columns;

	struct sort_common common;
	struct sort_index *column_indices;
	XbaeMatrixWidget mw;

	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	if(mw->matrix.rows == 0 || mw->matrix.columns == 0 || mw->matrix.per_cell == NULL)
	{
		xbaeObjectUnlock(w);
		return;
	}

	n_rows = mw->matrix.rows;
	n_columns = mw->matrix.columns;

	common.w = w;
	common.proc = proc;
	common.user_data = user_data;

	column_indices = (struct sort_index *) XtMalloc(n_columns * sizeof *column_indices);

	for(column = 0; column < n_columns; column++)
	{
		column_indices[column].index = column;
		column_indices[column].common = &common;
	}

	qsort(column_indices, n_columns, sizeof *column_indices, compare);

	for(row = 0; row < n_rows; row++)
	{
		mw->matrix.per_cell[row] = reorder(mw->matrix.per_cell[row],
										   sizeof *mw->matrix.per_cell[row], n_columns, column_indices);
	}

	mw->matrix.column_alignments = reorder(mw->matrix.column_alignments,
										   sizeof *mw->matrix.column_alignments, n_columns, column_indices);
	mw->matrix.column_label_alignments = reorder(mw->matrix.column_label_alignments,
												 sizeof *mw->matrix.column_label_alignments, n_columns,
												 column_indices);
	mw->matrix.column_font_bold =
		reorder(mw->matrix.column_font_bold, sizeof *mw->matrix.column_font_bold, n_columns, column_indices);
	mw->matrix.show_column_arrows =
		reorder(mw->matrix.show_column_arrows, sizeof *mw->matrix.show_column_arrows, n_columns,
				column_indices);
	mw->matrix.column_max_lengths =
		reorder(mw->matrix.column_max_lengths, sizeof *mw->matrix.column_max_lengths, n_columns,
				column_indices);
	mw->matrix.column_button_labels =
		reorder(mw->matrix.column_button_labels, sizeof *mw->matrix.column_button_labels, n_columns,
				column_indices);
	mw->matrix.column_user_data =
		reorder(mw->matrix.column_user_data, sizeof *mw->matrix.column_user_data, n_columns, column_indices);
	mw->matrix.column_shadow_types =
		reorder(mw->matrix.column_shadow_types, sizeof *mw->matrix.column_shadow_types, n_columns,
				column_indices);
	mw->matrix.xmcolumn_labels =
		reorder(mw->matrix.xmcolumn_labels, sizeof *mw->matrix.xmcolumn_labels, n_columns, column_indices);
	mw->matrix.column_labels =
		reorder(mw->matrix.column_labels, sizeof *mw->matrix.column_labels, n_columns, column_indices);
	mw->matrix.column_widths =
		reorder(mw->matrix.column_widths, sizeof *mw->matrix.column_widths, n_columns, column_indices);

	XtFree((XtPointer) column_indices);

	xbaeGetColumnPositions(mw);

	XbaeMatrixRefresh(w);

	xbaeObjectUnlock(w);
}



/* Set the width of the columns to the maximum width of the data cells, optionally 
 * including the column headers. Return True if any column widths change.
 */
static Boolean check_set_column_widths(XbaeMatrixWidget mw, Boolean include_headers)
{
	Boolean changed = False;
	int row, column;

	for(column = 0; column < mw->matrix.columns; column++)
	{
		int maxwidth = -1;

		if(include_headers)
		{
			if(mw->matrix.xmcolumn_labels != NULL && mw->matrix.xmcolumn_labels[column] != NULL)
			{
				maxwidth = (int) XmStringWidth(mw->matrix.render_table, mw->matrix.xmcolumn_labels[column]);
			}
			else if(mw->matrix.column_labels != NULL && mw->matrix.column_labels[column] != NULL)
			{
				maxwidth = xbaeStringWidth(mw, &mw->matrix.label_font, mw->matrix.column_labels[column], True);
			}
		}

		if(mw->matrix.per_cell)
		{
			for(row = 0; row < mw->matrix.rows; row++)
			{
				if(mw->matrix.per_cell[row][column].widget)
				{
					maxwidth = -1;
					break;
				}
				else
				{
					int width;
					String label = ((XbaeMatrixWidgetClass) XtClass((Widget) mw))->matrix_class.get_cell(mw, row, column);
					/*
					 * As we are not drawing the font do not set the draw_font gc as this function
					 * may be called before the widget is realized.
					 */
					xbaeSetDrawFont(mw, mw->matrix.per_cell[row][column].qtag, False);
					width = xbaeStringWidth(mw, &mw->matrix.draw_font, label, mw->matrix.multi_line_cell);
					if(width > maxwidth) maxwidth = width;
				}
			}
		}

		if(maxwidth >= 0)
		{
			short width = (short) ((Dimension) maxwidth +
								   (mw->matrix.cell_margin_width +
									mw->matrix.cell_highlight_thickness +
									mw->matrix.cell_shadow_thickness) * 2);

			if(!mw->matrix.column_width_in_pixels)
				width = (width - 2 * CELL_BORDER_WIDTH(mw)) / CELL_FONT_WIDTH(mw);

			if(mw->matrix.column_widths[column] != width)
			{
				changed = True;
				mw->matrix.column_widths[column] = width;
			}
		}
	}

	return changed;
}



/***********************************************************************/
/* Purpose: To resize the columns to fit the size of the maximum string
 *          width in the label and cells of the column. If any of the
 *          cells contain widgets the column will not be resized.
 *
 * Parameters: w - The matrix widget
 *             include_headers - If true include the headers in the
 *                               resize. If false do not. Note that not
 *             including the headers may crop the headers, but that
 *             may be what is desired.
 */
/***********************************************************************/
void XbaeMatrixResizeColumnsToCells(Widget w, Boolean include_headers)
{
	XbaeMatrixWidget mw;
	xbaeObjectLock(w);
	mw = XBAE_CHECK_CLASS(w);
	if(mw != NULL && mw->matrix.rows != 0 && mw->matrix.columns != 0)
	{
		Boolean modified = check_set_column_widths(mw, include_headers);
		if(modified)
		{
			xbaeGetColumnPositions(mw);
			if(!mw->matrix.disable_redisplay && XtIsRealized(w))
			{
				xbaeRefresh(mw, True);
				if(mw->matrix.text_child_is_mapped)
					xbaeUpdateTextChild(mw, True);
			}
		}
	}
	xbaeObjectUnlock(w);
}
