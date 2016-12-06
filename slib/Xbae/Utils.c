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
 * MatrixWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: Utils.c,v 1.112 2005/07/12 09:13:30 dannybackx Exp $
 */

/*
 * Utils.c created by Andrew Lister (7 August, 1995)
 */

#define HAVE_STRNCASECMP 1

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <assert.h>

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>

#include <Xbae/MatrixP.h>
#include <Xbae/Macros.h>
#include <Xbae/Utils.h>
#include <Xbae/Actions.h>

#include "XbaeDebug.h"

int xbaeRowClip(XbaeMatrixWidget mw, int row)
{
	int clip;

	if(row == -1)
	{
		clip = CLIP_COLUMN_LABELS;
	}
	else if(IS_LEADING_FIXED_ROW(mw, row))
	{
		clip = CLIP_FIXED_ROWS;
	}
	else if(IS_TRAILING_FIXED_ROW(mw, row))
	{
		clip = CLIP_TRAILING_FIXED_ROWS;
	}
	else
	{
		clip = CLIP_VISIBLE_HEIGHT;
	}

	return clip;
}

int xbaeColumnClip(XbaeMatrixWidget mw, int column)
{
	int clip;

	if(column == -1)
	{
		clip = CLIP_ROW_LABELS;
	}
	else if(IS_LEADING_FIXED_COLUMN(mw, column))
	{
		clip = CLIP_FIXED_COLUMNS;
	}
	else if(IS_TRAILING_FIXED_COLUMN(mw, column))
	{
		clip = CLIP_TRAILING_FIXED_COLUMNS;
	}
	else
	{
		clip = CLIP_VISIBLE_WIDTH;
	}

	return clip;
}

int xbaeCellClip(XbaeMatrixWidget mw, int row, int column)
{
	return xbaeRowClip(mw, row) | xbaeColumnClip(mw, column);
}

/*
 * Returns the widget to which a cell belongs, i.e. the matrix, or one
 * or one of the extra clips which handle the fixed row/col cells/labels
 */
Widget xbaeGetCellClip(XbaeMatrixWidget mw, int row, int column)
{
	int clip = xbaeCellClip(mw, row, column);
	Widget w;

	switch (clip)
	{
		case CLIP_VISIBLE_WIDTH | CLIP_VISIBLE_HEIGHT:
			/* not fixed at all - on clip child */
			w = ClipChild(mw);
			break;

		case CLIP_FIXED_COLUMNS | CLIP_VISIBLE_HEIGHT:
			/* fixed col only - on left clip */
			w = LeftClip(mw);
			break;

		case CLIP_TRAILING_FIXED_COLUMNS | CLIP_VISIBLE_HEIGHT:
			/* fixed trailing col only - on right clip */
			w = RightClip(mw);
			break;

		case CLIP_ROW_LABELS | CLIP_VISIBLE_HEIGHT:
			/* non fixed row labels - on row label clip */
			w = RowLabelClip(mw);
			break;

		case CLIP_FIXED_ROWS | CLIP_VISIBLE_WIDTH:
			/* fixed row only - on top clip */
			w = TopClip(mw);
			break;

		case CLIP_TRAILING_FIXED_ROWS | CLIP_VISIBLE_WIDTH:
			/* fixed trailing row only - on bottom clip */
			w = BottomClip(mw);
			break;

		case CLIP_COLUMN_LABELS | CLIP_VISIBLE_WIDTH:
			/* non fixed column labels - on column label clip */
			w = ColumnLabelClip(mw);
			break;

		default:
			/* total fixed cell/labels - on parent matrix window */
			w = (Widget) mw;
			break;
	}
	assert(w);
	return w;
}

/*
 * Cache the virtual position of each row/column
 */

static void
xbaeGetPosition(int n, Boolean size_in_pixels, short *sizes, int *positions, int font_size, int border_size)
{
	int i, pos;

	if(size_in_pixels)
	{
		for(i = 0, pos = 0; i < n; pos += sizes[i], i++)
		{
			positions[i] = pos;
		}
	}
	else
	{
		for(i = 0, pos = 0; i < n; pos += sizes[i] * font_size + 2 * border_size, i++)
		{
			positions[i] = pos;
		}
	}
	/* Tobias: The positions arrays are one element longer than
	 * the number of columns/rows in the matrix. We need to initialze 
	 * the last element so wo can calculate sizes safely from the 
	 * the difference of any two positions
	 */
	positions[n] = pos;
}

void xbaeGetColumnPositions(XbaeMatrixWidget mw)
{
	xbaeGetPosition(mw->matrix.columns,
					mw->matrix.column_width_in_pixels,
					mw->matrix.column_widths,
					mw->matrix.column_positions, CELL_FONT_WIDTH(mw), CELL_BORDER_WIDTH(mw));
}

void xbaeGetRowPositions(XbaeMatrixWidget mw)
{
	xbaeGetPosition(mw->matrix.rows,
					mw->matrix.row_height_in_pixels,
					mw->matrix.row_heights,
					mw->matrix.row_positions, TEXT_HEIGHT(mw), CELL_BORDER_HEIGHT(mw));
}

static int
xbaeCheckPosition(int n, Boolean size_in_pixels, short *sizes, int *positions, int font_size, int border_size,
				  int j)
{
	int i, pos;

	if(size_in_pixels == True)
	{
		for(i = 0, pos = 0; i < n; pos += sizes[i], i++)
		{
			assert(positions[i] == pos);
		}
	}
	else
	{
		for(i = 0, pos = 0; i < n; pos += sizes[i] * font_size + 2 * border_size, i++)
		{
			assert(positions[i] == pos);
		}
	}
	assert(positions[n] == pos);

	assert(j >= 0 && j <= n);

	return positions[j];
}

int xbaeCheckColumnPosition(XbaeMatrixWidget mw, int column)
{
	return xbaeCheckPosition(mw->matrix.columns,
							 mw->matrix.column_width_in_pixels,
							 mw->matrix.column_widths,
							 mw->matrix.column_positions, CELL_FONT_WIDTH(mw), CELL_BORDER_WIDTH(mw), column);
}

int xbaeCheckRowPosition(XbaeMatrixWidget mw, int row)
{
	return xbaeCheckPosition(mw->matrix.rows,
							 mw->matrix.row_height_in_pixels,
							 mw->matrix.row_heights,
							 mw->matrix.row_positions, TEXT_HEIGHT(mw), CELL_BORDER_HEIGHT(mw), row);
}

/*
 * Find where a virtual coordinate falls by (binary) searching the positions array
 */
static int findPosition(int *positions, int start, int end, int pos)
{
	int middle;

	/* Tobias: Neither of the conditions should ever be true. If they are there is a bug somewhere 
	 * up the call stack. So far I found three. The rest of xbae tries to fix problems instead of failing
	 * so we do the same here to keep up with the debugging fun.
	 */
	if(pos < positions[start])
	{
		DEBUGOUT(_XbaeDebug
				 (__FILE__, NULL, "pos[start=%d]=%d pos[end=%d]=%d pos=%d\n", start,
				  positions[start], end, positions[end], pos));
		return start;
	}
	else if(pos > positions[end] - 1)
	{
		DEBUGOUT(_XbaeDebug
				 (__FILE__, NULL, "pos[start=%d]=%d pos[end=%d]=%d pos=%d\n", start,
				  positions[start], end, positions[end], pos));
		return end - 1;
	}

	for(;;)
	{
		middle = (start + end) / 2;
		if(positions[middle] > pos)
		{
			end = middle;
		}
		else if(positions[middle + 1] - 1 < pos)
		{
			start = middle;
		}
		else
		{
			break;
		}
	}

	return middle;
}

static int xbaeXtoCol(XbaeMatrixWidget mw, int x)
{
	return findPosition(mw->matrix.column_positions, 0, mw->matrix.columns, x);
}

static int xbaeYtoRow(XbaeMatrixWidget mw, int y)
{
	return findPosition(mw->matrix.row_positions, 0, mw->matrix.rows, y);
}

/*
 * Return the top and bottom-most visible non-fixed row
 */
int xbaeTopRow(XbaeMatrixWidget mw)
{
	return xbaeYtoRow(mw, FIXED_ROW_HEIGHT(mw) + VERT_ORIGIN(mw));
}

int xbaeBottomRow(XbaeMatrixWidget mw)
{
	return xbaeYtoRow(mw, FIXED_ROW_HEIGHT(mw) + VERT_ORIGIN(mw) + VISIBLE_NON_FIXED_HEIGHT(mw) - 1);
}

void xbaeGetVisibleRows(XbaeMatrixWidget mw, int *top_row, int *bottom_row)
{
	*top_row = xbaeTopRow(mw);
	*bottom_row = xbaeBottomRow(mw);
}

/*
 * Return the left and right-most visible non-fixed column
 */
int xbaeLeftColumn(XbaeMatrixWidget mw)
{
	return xbaeXtoCol(mw, FIXED_COLUMN_WIDTH(mw) + HORIZ_ORIGIN(mw));
}

int xbaeRightColumn(XbaeMatrixWidget mw)
{
	return xbaeXtoCol(mw, FIXED_COLUMN_WIDTH(mw) + HORIZ_ORIGIN(mw) + VISIBLE_NON_FIXED_WIDTH(mw) - 1);
}

void xbaeGetVisibleColumns(XbaeMatrixWidget mw, int *left_column, int *right_column)
{
	*left_column = xbaeLeftColumn(mw);
	*right_column = xbaeRightColumn(mw);
}

/*
 * Return the top and bottom row and left and right column of
 * the visible non-fixed cells
 */
void
xbaeGetVisibleCells(XbaeMatrixWidget mw, int *top_row, int *bottom_row, int *left_column, int *right_column)
{
	xbaeGetVisibleRows(mw, top_row, bottom_row);
	xbaeGetVisibleColumns(mw, left_column, right_column);
}

/*
 * Try to make a row/column top/left row/column. The row/column is relative to 
 * fixed_rows/columns - so 0 would be the first non-fixed row/column.
 * If we can't make it the top row/left column, make it as close as possible.
 */
int xbaeCalculateVertOrigin(XbaeMatrixWidget mw, int top_row)
{
	if(NON_FIXED_HEIGHT(mw) < VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		return 0;
	}
	else if(ROW_POSITION(mw, TRAILING_ROW_ORIGIN(mw)) -
			ROW_POSITION(mw, mw->matrix.fixed_rows + top_row) < VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		return NON_FIXED_HEIGHT(mw) - VISIBLE_NON_FIXED_HEIGHT(mw);
	}
	else
	{
		return ROW_POSITION(mw, mw->matrix.fixed_rows + top_row) - FIXED_ROW_HEIGHT(mw);
	}
}

int xbaeCalculateHorizOrigin(XbaeMatrixWidget mw, int left_column)
{
	if(NON_FIXED_WIDTH(mw) < VISIBLE_NON_FIXED_WIDTH(mw))
	{
		return 0;
	}
	else if(COLUMN_POSITION(mw, TRAILING_COLUMN_ORIGIN(mw)) -
			COLUMN_POSITION(mw, mw->matrix.fixed_columns + left_column) < VISIBLE_NON_FIXED_WIDTH(mw))
	{
		return NON_FIXED_WIDTH(mw) - VISIBLE_NON_FIXED_WIDTH(mw);
	}
	else
	{
		return COLUMN_POSITION(mw, mw->matrix.fixed_columns + left_column) - FIXED_COLUMN_WIDTH(mw);
	}
}

/*
 * Return True if a row is visible on the screen (not scrolled totally off)
 */
Boolean xbaeIsRowVisible(XbaeMatrixWidget mw, int row)
{
	/*
	 * If we are not in a fixed row or trailing fixed row,
	 * see if we are on the screen vertically
	 * (fixed rows are always on the screen)
	 */
	if(!IS_FIXED_ROW(mw, row))
	{
		/* SGO: used same method as IsColumnVisible */
		int y;
		y = ROW_POSITION(mw, row) - ROW_POSITION(mw, mw->matrix.fixed_rows) - VERT_ORIGIN(mw);

		if(y + ROW_HEIGHT(mw, row) > 0 && y < VISIBLE_NON_FIXED_HEIGHT(mw))
			return True;
	}
	else
		return True;

	return False;
}

/*
 * Return True if a column is visible on the screen (not scrolled totally off)
 */
Boolean xbaeIsColumnVisible(XbaeMatrixWidget mw, int column)
{
	/*
	 * If we are not in a fixed column, see if we are on the screen
	 * horizontally (fixed columns are always on the screen)
	 */
	if(!IS_FIXED_COLUMN(mw, column))
	{
		int x;

		/*
		 * Calculate the x position of the column relative to the clip
		 */
		x = COLUMN_POSITION(mw, column) - COLUMN_POSITION(mw, mw->matrix.fixed_columns) - HORIZ_ORIGIN(mw);

		/*
		 * Check if we are visible horizontally
		 */
		if(x + COLUMN_WIDTH(mw, column) > 0 && x < VISIBLE_NON_FIXED_WIDTH(mw))
			return True;
	}
	else
		return True;

	return False;
}

/*
 * Return True if a cell is visible on the screen (not scrolled totally off)
 */
Boolean xbaeIsCellVisible(XbaeMatrixWidget mw, int row, int column)
{
	return xbaeIsRowVisible(mw, row) && xbaeIsColumnVisible(mw, column);
}

/*
 * Scroll a row so it is visible on the screen.
 */
void xbaeMakeRowVisible(XbaeMatrixWidget mw, int row)
{
	int value, slider_size, increment, page_increment, y, vert_value;

	/*
	 * If we are in a fixed column, we are already visible.
	 */
	if(IS_FIXED_ROW(mw, row))
		return;

	/*
	 * Calculate the y position of this row
	 */
	y = ROW_POSITION(mw, row) - ROW_POSITION(mw, mw->matrix.fixed_rows);

	/*
	 * Figure out the new value of the VSB to scroll this cell
	 * onto the screen. If the whole cell won't fit, scroll so its
	 * top edge is visible.
	 */
	if(y < VERT_ORIGIN(mw) || ROW_HEIGHT(mw, row) >= VISIBLE_NON_FIXED_HEIGHT(mw))
	{
		vert_value = y;
	}
	else if(y + ROW_HEIGHT(mw, row) > VISIBLE_NON_FIXED_HEIGHT(mw) + VERT_ORIGIN(mw))
	{
		vert_value = y + ROW_HEIGHT(mw, row) - VISIBLE_NON_FIXED_HEIGHT(mw);
	}
	else
	{
		vert_value = VERT_ORIGIN(mw);
	}

	/*
	 * Give the VSB the new value and pass a flag to make it
	 * call our scroll callbacks
	 */
	if(vert_value != VERT_ORIGIN(mw))
	{
		XmScrollBarGetValues(VertScrollChild(mw), &value, &slider_size, &increment, &page_increment);
		XmScrollBarSetValues(VertScrollChild(mw), vert_value, slider_size, increment, page_increment, True);
	}
}

/*
 * Scroll a column so it is visible on the screen.
 */
void xbaeMakeColumnVisible(XbaeMatrixWidget mw, int column)
{
	int value, slider_size, increment, page_increment, x, horiz_value;

	/*
	 * If we are in a fixed column, we are already visible.
	 */
	if(IS_FIXED_COLUMN(mw, column))
		return;

	/*
	 * Calculate the x position of this column
	 */
	x = COLUMN_POSITION(mw, column) - COLUMN_POSITION(mw, mw->matrix.fixed_columns);

	/*
	 * Figure out the new value of the HSB to scroll this cell
	 * onto the screen. If the whole cell won't fit, scroll so its
	 * left edge is visible.
	 */
	if(x < HORIZ_ORIGIN(mw) || COLUMN_WIDTH(mw, column) >= VISIBLE_NON_FIXED_WIDTH(mw))
	{
		horiz_value = x;
	}
	else if(x + COLUMN_WIDTH(mw, column) > VISIBLE_NON_FIXED_WIDTH(mw) + HORIZ_ORIGIN(mw))
	{
		horiz_value = x + COLUMN_WIDTH(mw, column) - VISIBLE_NON_FIXED_WIDTH(mw);
	}
	else
	{
		horiz_value = HORIZ_ORIGIN(mw);
	}

	/*
	 * Give the HSB the new value and pass a flag to make it
	 * call our scroll callbacks
	 */
	if(horiz_value != HORIZ_ORIGIN(mw))
	{
		XmScrollBarGetValues(HorizScrollChild(mw), &value, &slider_size, &increment, &page_increment);
		XmScrollBarSetValues(HorizScrollChild(mw), horiz_value, slider_size, increment, page_increment, True);
	}
}

/*
 * Scrolls a fixed or non-fixed cell so it is visible on the screen.
 */
void xbaeMakeCellVisible(XbaeMatrixWidget mw, int row, int column)
{
	if(!xbaeIsRowVisible(mw, row))
		xbaeMakeRowVisible(mw, row);

	if(!xbaeIsColumnVisible(mw, column))
		xbaeMakeColumnVisible(mw, column);
}

void xbaeComputeSize(XbaeMatrixWidget mw, Boolean compute_width, Boolean compute_height)
{
	int full_width = TOTAL_WIDTH(mw) + ROW_LABEL_WIDTH(mw) + 2 * mw->manager.shadow_thickness;
	int full_height = TOTAL_HEIGHT(mw) + COLUMN_LABEL_HEIGHT(mw) + 2 * mw->manager.shadow_thickness;
	int width, height;

	/*
	 * Calculate our width.
	 * If visible_columns is set, then base it on that.
	 * Otherwise, if the compute_width flag is set, then we are full width.
	 * Otherwise we keep whatever width we are.
	 */

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw,
						"xbaeComputeSize compute_width = %s   compute_height = %s\n"
						"                visible_columns = %d visible_rows = %d\n"
						"                width = %d           height =%d\n"
						"                full_width = %d      full_height = %d\n",
						compute_width ? "True" : "False",
						compute_height ? "True" : "False",
						mw->matrix.visible_columns,
						mw->matrix.visible_rows, mw->core.width, mw->core.height, full_width, full_height));

	if(mw->matrix.visible_columns)
	{
		width = ROW_LABEL_WIDTH(mw) + 2 * mw->manager.shadow_thickness
			+ FIXED_COLUMN_WIDTH(mw) + TRAILING_FIXED_COLUMN_WIDTH(mw)
			+ NON_FIXED_WIDTH(mw) * mw->matrix.visible_columns
			/ (mw->matrix.columns - mw->matrix.fixed_columns - mw->matrix.trailing_fixed_columns);
	}
	else if(compute_width)
	{
		width = full_width;
	}
	else
	{
		width = mw->core.width;
	}

	/*
	 * Calculate our height.
	 * If visible_rows is set, then base it on that.
	 * Otherwise, if the compute_height flag is set, then we are full height.
	 * Otherwise we keep whatever height we are.
	 */
	if(mw->matrix.visible_rows)
	{
		height = COLUMN_LABEL_HEIGHT(mw) + 2 * mw->manager.shadow_thickness
			+ FIXED_ROW_HEIGHT(mw) + TRAILING_FIXED_ROW_HEIGHT(mw)
			+ NON_FIXED_HEIGHT(mw) * mw->matrix.visible_rows
			/ (mw->matrix.rows - mw->matrix.fixed_rows - mw->matrix.trailing_fixed_rows);
	}
	else if(compute_height)
	{
		height = full_height;
	}
	else
	{
		height = mw->core.height;
	}

	/*
	 * If we are allowed to modify our height and we need to display a hsb 
	 * include it's size in the computation
	 */
	if((compute_height || mw->matrix.visible_rows)
	   && ((mw->matrix.hsb_display_policy == XmDISPLAY_STATIC)
		   || (mw->matrix.hsb_display_policy == XmDISPLAY_AS_NEEDED && width < full_width)))
		height += HORIZ_SB_HEIGHT(mw);

	/*
	 * If we are allowed to modify our width and we need to display a vsb 
	 * include it's size in the computation
	 */
	if((compute_width || mw->matrix.visible_columns)
	   && ((mw->matrix.vsb_display_policy == XmDISPLAY_STATIC)
		   || (mw->matrix.vsb_display_policy == XmDISPLAY_AS_NEEDED && height < full_height)))
		width += VERT_SB_WIDTH(mw);

	/*
	 * Store our calculated size.
	 */
	mw->core.width = width;
	mw->core.height = height;

	/*
	 * Save our calculated size for use in our query_geometry method.
	 * This is the size we really want to be (not necessarily the size
	 * we will end up being).
	 */
	mw->matrix.desired_width = width;
	mw->matrix.desired_height = height;

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "xbaeComputeSize -> w %d h %d\n", width, height));
}

/*
 * Return the length of the longest line in string
 */
static int xbaeMaxLen(String string)
{
	char *nl;
	int max_len = 0;

	/*
	 * Get the length of the longest line terminated by \n
	 */
	while((nl = strchr(string, '\n')) != NULL)
	{
		int len = nl - string;
		if(len > max_len)
		{
			max_len = len;
		}
		string = nl + 1;
	}

	/* 
	 * If the last line wasn't terminated with a \n take it into account
	 */
	if(*string != '\0')
	{
		int len = strlen(string);
		if(len > max_len)
		{
			max_len = len;
		}
	}

	return max_len;
}

/*
 * Return the number of lines in string
 */
static int xbaeCountLines(String string)
{
	char *nl;
	int n_lines = 0;

	/*
	 * Count the number of lines terminated by \n
	 */
	while((nl = strchr(string, '\n')) != NULL)
	{
		n_lines++;
		string = nl + 1;
	}

	/*
	 * If the last line wasn't terminated by a \n take it into account
	 */
	if(*string != '\0')
	{
		n_lines++;
	}

	return n_lines;
}


/* Determine the width of the given string in pixels. If the string has multiple lines as indicated by
 * the line break character '\n' and multiline is true, the length of the longest line is returned
 * otherwise '\n' is ignored.
 */
int xbaeStringWidth(XbaeMatrixWidget mw, XbaeMatrixFontInfo *font, String string, Boolean multiline)
{
	int max_width = 0;

	/* If string is NULL or nothing but white space return a 0 width */
	if(!string) return max_width;
	if(strspn(string, " \t\n") >= strlen(string)) return max_width;

	if(font->type == XmFONT_IS_XFT)
	{
		XftFont *xftFont = (XftFont *) font->fontp;
		if(multiline)
		{
			String ptr, cur = string;
			while((ptr = strchr(cur,'\n')))
			{
				int width = xbaeXftStringWidth(mw, xftFont, cur, (int)(ptr - cur));
				if(width > max_width) max_width = width;
				cur = ptr+1;
			}
			if(*cur)
			{
				int width = xbaeXftStringWidth(mw, xftFont, cur, strlen(cur));
				if(width > max_width) max_width = width;
			}
		}
		else
		{
			max_width = xbaeXftStringWidth(mw, xftFont, string, strlen(string));
		}
	}
	else if(font->type == XmFONT_IS_FONTSET)
	{
		XRectangle *ink_array = NULL;
		XRectangle *logical_array = NULL;
		XRectangle overall_logical;
		int num_chars, length;
		XFontSet font_set = (XFontSet )font->fontp;

		length = strlen(string);
		ink_array = (XRectangle *) XtMalloc(length * sizeof(XRectangle));
		logical_array = (XRectangle *) XtMalloc(length * sizeof(XRectangle));

		if(multiline)
		{
			String ptr, cur = string;
			while((ptr = strchr(cur,'\n')))
			{
				int len = (int)(ptr - cur);
				XmbTextPerCharExtents(font_set, cur, len, ink_array, logical_array,
									len, &num_chars, NULL, &overall_logical);
				if(overall_logical.width > max_width)
					max_width = overall_logical.width;
				cur = ptr+1;
			}
			if(*cur)
			{
				int len = strlen(cur);
				XmbTextPerCharExtents(font_set, cur, len, ink_array, logical_array,
									len, &num_chars, NULL, &overall_logical);
				if(overall_logical.width > max_width)
					max_width = overall_logical.width;
			}
		}
		else
		{
			XmbTextPerCharExtents(font_set, string, length, ink_array, logical_array,
								  length, &num_chars, NULL, &overall_logical);
			max_width = overall_logical.width;
		}

		if(ink_array) XtFree((char *) ink_array);
		if(logical_array) XtFree((char *) logical_array);
	}
	else
	{
		XFontStruct *font_struct = (XFontStruct *) font->fontp;
		if(multiline)
		{
			String ptr, cur = string;
			while((ptr = strchr(cur,'\n')))
			{
				int width = XTextWidth(font_struct, cur, (int)(ptr-cur));
				if(width > max_width) max_width = width;
				cur = ptr+1;
			}
			if(*cur)
			{
				int width = XTextWidth(font_struct, cur, strlen(cur));
				if(width > max_width) max_width = width;
			}
		}
		else
		{
			max_width = XTextWidth(font_struct, string, strlen(string));
		}
	}
	return max_width;
}

/*
 * Return the length of the longest line of all row_labels
 */
int xbaeCalculateLabelMaxWidth(XbaeMatrixWidget mw, String * labels, XmString * xmlabels, int n_labels)
{
	int i;
	int max_len = 0;

	/*
	 * Determine the length of the longest row label
	 */
	if(labels || xmlabels)
	{
		XmStringTag tag = XrmQuarkToString(mw->matrix.label_font.qtag);
		for(i = 0; i < n_labels; i++)
		{
			int len = 0;
			if(xmlabels && xmlabels[i])
			{
				len = (int) XmStringWidth(mw->matrix.render_table, xmlabels[i]);
			}
			else if(labels && labels[i])
			{
				len = xbaeStringWidth(mw, &mw->matrix.label_font, labels[i], True);
			}
			if(len > max_len)
			{
				max_len = len;
			}
		}
	}
	return max_len;
}

/*
 * Return the maximum number of lines in labels
 */
int xbaeCalculateLabelMaxLines(String * labels, XmString * xmlabels, int n_labels)
{
	int i;
	int max_lines = 0;
	if(labels || xmlabels)
	{
		for(i = 0; i < n_labels; i++)
		{
			int n_lines = 0;
			if(xmlabels && xmlabels[i])
			{
				n_lines = XmStringLineCount(xmlabels[i]);
			}
			else if(labels && labels[i])
			{
				n_lines = xbaeCountLines(labels[i]);
			}
			if(n_lines > max_lines)
			{
				max_lines = n_lines;
			}
		}
	}
	return max_lines;
}

/*
 * Convert the coordinates in an event to coordinates relative to the matrix window
 */

/* ARGSUSED */
Boolean xbaeEventToMatrixXY(XbaeMatrixWidget mw, XEvent * event, int *x, int *y)
{
	switch (event->type)
	{
		case ButtonPress:
		case ButtonRelease:
			*x = event->xbutton.x;
			*y = event->xbutton.y;
			break;
		case KeyPress:
		case KeyRelease:
			*x = event->xkey.x;
			*y = event->xkey.y;
			break;
		case MotionNotify:
			*x = event->xmotion.x;
			*y = event->xmotion.y;
			break;
		default:
			return False;
	}

	if(event->xbutton.window == XtWindow(TextChild(mw)))
	{
		*x += xbaeColumnToMatrixX(mw, mw->matrix.current_column);
		*y += xbaeRowToMatrixY(mw, mw->matrix.current_row);
	}
	else if(event->xbutton.window == XtWindow(LeftClip(mw)))
	{
		*x += FIXED_COLUMN_POSITION(mw);
		*y += NON_FIXED_ROW_POSITION(mw);
	}
	else if(event->xbutton.window == XtWindow(RightClip(mw)))
	{
		*x += TRAILING_FIXED_COLUMN_POSITION(mw);
		*y += NON_FIXED_ROW_POSITION(mw);
	}
	else if(event->xbutton.window == XtWindow(TopClip(mw)))
	{
		*x += NON_FIXED_COLUMN_POSITION(mw);
		*y += FIXED_ROW_POSITION(mw);
	}
	else if(event->xbutton.window == XtWindow(BottomClip(mw)))
	{
		*x += NON_FIXED_COLUMN_POSITION(mw);
		*y += TRAILING_FIXED_ROW_POSITION(mw);
	}
	else if(event->xbutton.window == XtWindow(ClipChild(mw)))
	{
		*x += NON_FIXED_COLUMN_POSITION(mw);
		*y += NON_FIXED_ROW_POSITION(mw);
	}
	else if(event->xbutton.window == XtWindow(RowLabelClip(mw)))
	{
		*x += VERT_SB_OFFSET(mw);
		*y += NON_FIXED_ROW_POSITION(mw);
	}
	else if(event->xbutton.window == XtWindow(ColumnLabelClip(mw)))
	{
		*x += NON_FIXED_COLUMN_POSITION(mw);
		*y += HORIZ_SB_OFFSET(mw);
	}
	else if(event->xbutton.window != XtWindow(mw))
	{
		return False;
	}

	return True;
}

/* ARGSUSED */
Boolean xbaeNewEventToMatrixXY(XbaeMatrixWidget mw, Widget w, XEvent * event, int *x, int *y)
{
	switch (event->type)
	{
		case ButtonPress:
		case ButtonRelease:
			*x = event->xbutton.x;
			*y = event->xbutton.y;
			break;
		case KeyPress:
		case KeyRelease:
			*x = event->xkey.x;
			*y = event->xkey.y;
			break;
		case MotionNotify:
			*x = event->xmotion.x;
			*y = event->xmotion.y;
			break;
		default:
			return False;
	}

	for(; w && w != (Widget) mw; w = XtParent(w))
	{
		*x += w->core.x;
		*y += w->core.y;
	}

	return w == (Widget) mw;
}

/*
 * Convert a matrix window x position to a column.
 * Return true if x falls in a column or a row label. If so adjust x so that it's
 * relative to the column/label. If not return false and leave x alone
 */

/* ARGSUSED */
Boolean xbaeMatrixXtoColumn(XbaeMatrixWidget mw, int *x, int *column)
{

	if(*x >= VERT_SB_OFFSET(mw) && *x < VERT_SB_OFFSET(mw) + ROW_LABEL_WIDTH(mw))
	{
		/* It's in the row labels */
		*column = -1;

		/* Make x relative to the row labels */
		*x -= VERT_SB_OFFSET(mw);
	}
	else
	{
		/* Transform x so it's relative to the virtual matrix */
		if(*x >= FIXED_COLUMN_POSITION(mw) && *x < FIXED_COLUMN_POSITION(mw) + VISIBLE_FIXED_COLUMN_WIDTH(mw))
		{
			*x -= FIXED_COLUMN_POSITION(mw);
			/* Check if we are not in the horizontal fill */
			if(*x >= COLUMN_POSITION(mw, mw->matrix.fixed_columns))
			{
				*column = mw->matrix.fixed_columns - 1;
				*x -= COLUMN_POSITION(mw, *column);
				return True;
			}
		}
		else if(*x >= TRAILING_FIXED_COLUMN_POSITION(mw)
				&& *x < TRAILING_FIXED_COLUMN_POSITION(mw) + VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw))
		{
			*x -= TRAILING_FIXED_COLUMN_POSITION(mw) - COLUMN_POSITION(mw, TRAILING_COLUMN_ORIGIN(mw));
			/* Check if we are not in the horizontal fill */
			if(*x >= COLUMN_POSITION(mw, mw->matrix.columns))
			{
				*column = mw->matrix.columns - 1;
				*x -= COLUMN_POSITION(mw, *column);
				return True;
			}
		}
		else if(*x >= NON_FIXED_COLUMN_POSITION(mw)
				&& *x < NON_FIXED_COLUMN_POSITION(mw) + VISIBLE_NON_FIXED_WIDTH(mw))
		{
			*x -= NON_FIXED_COLUMN_POSITION(mw) - HORIZ_ORIGIN(mw) - FIXED_COLUMN_WIDTH(mw);
			/* Check if we are not in the horizontal fill */
			if(*x >= COLUMN_POSITION(mw, TRAILING_COLUMN_ORIGIN(mw)))
			{
				*column = TRAILING_COLUMN_ORIGIN(mw) - 1;
				*x -= COLUMN_POSITION(mw, *column);
				return True;
			}
		}
		else
		{
			*column = -2;
			return False;
		}
		/* Get the column it corresponds to */
		*column = xbaeXtoCol(mw, *x);

		/* Make x relative to the that column */
		*x -= COLUMN_POSITION(mw, *column);
	}

	return True;
}

/*
 * Convert a matrix window y position to a row.
 * Return true if y falls in a row or a column label. If so adjust y so that it's 
 * relative to the row/label. If not return false and leave y alone
 */

/* ARGSUSED */
Boolean xbaeMatrixYtoRow(XbaeMatrixWidget mw, int *y, int *row)
{

	if(*y >= HORIZ_SB_OFFSET(mw) && *y < HORIZ_SB_OFFSET(mw) + COLUMN_LABEL_HEIGHT(mw))
	{
		/* It's in the column labels */
		*row = -1;

		/* Make y relative to the column labels */
		*y -= HORIZ_SB_OFFSET(mw);
	}
	else
	{
		/* Transform the y coordinate so it's relative to the virtual matrix */
		if(*y >= FIXED_ROW_POSITION(mw) && *y < FIXED_ROW_POSITION(mw) + VISIBLE_FIXED_ROW_HEIGHT(mw))
		{
			*y -= FIXED_ROW_POSITION(mw);
			/* Check if we are not in the vertical fill */
			if(*y >= ROW_POSITION(mw, mw->matrix.fixed_rows))
			{
				*row = mw->matrix.fixed_rows - 1;
				*y -= ROW_POSITION(mw, *row);
				return True;
			}
		}
		else if(*y >= TRAILING_FIXED_ROW_POSITION(mw)
				&& *y < TRAILING_FIXED_ROW_POSITION(mw) + VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw))
		{
			*y -= TRAILING_FIXED_ROW_POSITION(mw) - ROW_POSITION(mw, TRAILING_ROW_ORIGIN(mw));
			/* Check if we are not in the vertical fill */
			if(*y >= ROW_POSITION(mw, mw->matrix.rows))
			{
				*row = mw->matrix.rows - 1;
				*y -= ROW_POSITION(mw, *row);
				return True;
			}
		}
		else if(*y >= NON_FIXED_ROW_POSITION(mw)
				&& *y < NON_FIXED_ROW_POSITION(mw) + VISIBLE_NON_FIXED_HEIGHT(mw))
		{
			*y -= NON_FIXED_ROW_POSITION(mw) - VERT_ORIGIN(mw) - FIXED_ROW_HEIGHT(mw);
			/* Check if we are not in the vertical fill */
			if(*y >= ROW_POSITION(mw, TRAILING_ROW_ORIGIN(mw)))
			{
				*row = TRAILING_ROW_ORIGIN(mw) - 1;
				*y -= ROW_POSITION(mw, *row);
				return True;
			}
		}
		else
		{
			*row = -2;
			return False;
		}
		/* Get the row it corresponds to */
		*row = xbaeYtoRow(mw, *y);

		/* Make y relative to the that row */
		*y -= ROW_POSITION(mw, *row);
	}

	return True;
}

/*
 * Convert a matrix window x, y to a row, column pair.
 * Return true if x, y falls in a cell. If so adjust x and y so they are
 * relative to the cell.
 * Return false if x, y falls in a label. Then one of row/column is -1. If so
 * adjust x and y so they are relative to the labels origin.
 * Return false if x, y doesn't fall in a cell nor a label and set both row and 
 * col to -1 leaving x and y alone.
 */

/* ARGSUSED */
Boolean xbaeMatrixXYToRowCol(XbaeMatrixWidget mw, int *x, int *y, int *row, int *column)
{
	int ret_x = *x;
	int ret_y = *y;

	if(!xbaeMatrixXtoColumn(mw, &ret_x, column) || !xbaeMatrixYtoRow(mw, &ret_y, row)
	   || (*row == -1 && *column == -1))
	{
		/* Not a cell nor a label */
		*row = -1;
		*column = -1;
		return False;
	}

	*x = ret_x;
	*y = ret_y;

	return *row != -1 && *column != -1;
}

/*
 * Convert a row/column cell position to the x/y of its upper left corner
 * wrt the matrix window 
 */
int xbaeColumnToMatrixX(XbaeMatrixWidget mw, int column)
{
	int x;

	if(column == -1)
	{
		x = VERT_SB_OFFSET(mw);
	}
	else if(IS_LEADING_FIXED_COLUMN(mw, column))
	{
		x = FIXED_COLUMN_POSITION(mw) + COLUMN_POSITION(mw, column);
	}
	else if(IS_TRAILING_FIXED_COLUMN(mw, column))
	{
		x = TRAILING_FIXED_COLUMN_POSITION(mw) + COLUMN_POSITION(mw, column) - COLUMN_POSITION(mw,
																							   TRAILING_COLUMN_ORIGIN
																							   (mw));
	}
	else
	{
		x = NON_FIXED_COLUMN_POSITION(mw) + COLUMN_POSITION(mw, column) - COLUMN_POSITION(mw,
																						  mw->matrix.
																						  fixed_columns) -
			HORIZ_ORIGIN(mw);
	}

	return x;
}

int xbaeRowToMatrixY(XbaeMatrixWidget mw, int row)
{
	int y;

	if(row == -1)
	{
		y = HORIZ_SB_OFFSET(mw);
	}
	else if(IS_LEADING_FIXED_ROW(mw, row))
	{
		y = FIXED_ROW_POSITION(mw) + ROW_POSITION(mw, row);
	}
	else if(IS_TRAILING_FIXED_ROW(mw, row))
	{
		y = TRAILING_FIXED_ROW_POSITION(mw) + ROW_POSITION(mw, row) - ROW_POSITION(mw,
																				   TRAILING_ROW_ORIGIN(mw));
	}
	else
	{
		y = NON_FIXED_ROW_POSITION(mw) + ROW_POSITION(mw, row) - ROW_POSITION(mw,
																			  mw->matrix.fixed_rows) -
			VERT_ORIGIN(mw);
	}

	return y;
}

void xbaeRowColToMatrixXY(XbaeMatrixWidget mw, int row, int column, int *widget_x, int *widget_y)
{
	*widget_x = xbaeColumnToMatrixX(mw, column);
	*widget_y = xbaeRowToMatrixY(mw, row);
}

/*
 * Convert a row/column cell position to the x/y of its upper left corner
 * wrt the window it will be drawn in (either the matrix window for
 * fixed cells, or the clip window for non-fixed).
 */
Widget xbaeRowColToClipXY(XbaeMatrixWidget mw, int row, int column, int *x, int *y)
{
	Widget w = xbaeGetCellClip(mw, row, column);

	*x = xbaeColumnToMatrixX(mw, column);
	*y = xbaeRowToMatrixY(mw, row);

	if(w != (Widget) mw)
	{
		*x -= w->core.x;
		*y -= w->core.y;
	}

	return w;
}

void xbaePositionWidgetOverCell(XbaeMatrixWidget mw, Widget w, int row, int column)
{
	Widget new_parent = xbaeGetCellClip(mw, row, column);

	int x = xbaeColumnToMatrixX(mw, column) + mw->matrix.cell_shadow_thickness;
	int y = xbaeRowToMatrixY(mw, row) + mw->matrix.cell_shadow_thickness;
	int width = COLUMN_WIDTH(mw, column) - 2 * mw->matrix.cell_shadow_thickness;
	int height = ROW_HEIGHT(mw, row) - 2 * mw->matrix.cell_shadow_thickness;

	if(IS_FILL_COLUMN(mw, column) && mw->matrix.horz_fill)
	{
		width += EMPTY_WIDTH(mw);
	}

	if(IS_FILL_ROW(mw, row) && mw->matrix.vert_fill)
	{
		height += EMPTY_HEIGHT(mw);
	}

	XtConfigureWidget(w,
					  /* position */ x, y,
					  /* size     */ width, height,
					  /* bw       */ XtBorderWidth(w));

	if(XtWindow(new_parent))
	{
		if(new_parent != (Widget) mw)
		{
			/* The widget is drawn in one of the clip windows */
			x -= new_parent->core.x;
			y -= new_parent->core.y;
		}
		XReparentWindow(XtDisplay(mw), XtWindow(w), XtWindow(new_parent), x, y);
	}
}

void xbaePositionCellWidget(XbaeMatrixWidget mw, int row, int column)
{
	Widget cellWidget = mw->matrix.per_cell ? mw->matrix.per_cell[row][column].widget : NULL;

	if(cellWidget && XtIsRealized(cellWidget) && XtIsManaged(cellWidget))
	{
		xbaePositionWidgetOverCell(mw, cellWidget, row, column);
	}
}

void xbaePositionTextChild(XbaeMatrixWidget mw)
{
	xbaePositionWidgetOverCell(mw, TextChild(mw), mw->matrix.current_row, mw->matrix.current_column);

	/*
	 * The textChild is now visible
	 */
	mw->matrix.text_child_is_mapped = True;
}

void xbaeHideCellWidget(XbaeMatrixWidget mw, Widget w)
{
	Dimension width = XtWidth(w);
	Dimension height = XtHeight(w);
	Dimension bw = XtBorderWidth(w);

	XtConfigureWidget(w,
					  /* position */ -1 - width - bw, -1 - height - bw,
					  /* size */ width, height,
					  /* bw */ bw);
}

void xbaeHideTextChild(XbaeMatrixWidget mw)
{
	/*
	 * Let Xt believe the textChild is still visible so we can traverse to it
	 */
	XtConfigureWidget(TextChild(mw),
					  /* position */ 0, 0,
					  /* size     */ 1, 1,
					  /* bw       */ XtBorderWidth(TextChild(mw)));

	/*
	 * But have X not show the window
	 */
	if(XtIsRealized(TextChild(mw)))
		XReparentWindow(XtDisplay(mw), XtWindow(TextChild(mw)), XtWindow(mw), -1, -1);

	/*
	 * The textChild is now hidden
	 */
	mw->matrix.text_child_is_mapped = False;
}

void xbaeSetInitialFocus(XbaeMatrixWidget mw)
{
	int row = xbaeTopRow(mw);
	int column = xbaeLeftColumn(mw);
	Widget widget;

	/* 2006.10.11 - The row was sometimes being returned as -1 in HPUX.
	 * I did not have the inclination to try and track down the problem, as
	 * all is ok in Linux, so this check will just ignore it.
	 */
	if(row < 0 || column < 0)
		return;

	if(mw->matrix.per_cell && mw->matrix.per_cell[row][column].widget)
	{
		widget = mw->matrix.per_cell[row][column].widget;
	}
	else
	{
		widget = TextChild(mw);
	}

	if(widget != mw->manager.initial_focus)
	{
		XtVaSetValues((Widget) mw, XmNinitialFocus, widget, NULL);
	}
}

void xbaeSaneRectangle(XbaeMatrixWidget mw, XRectangle * rect_p, int rs, int cs, int re, int ce)
{

	int x1, x2, y1, y2;

	x1 = xbaeColumnToMatrixX(mw, cs);
	if(!IS_FIXED_COLUMN(mw, cs))
	{
		if(x1 < NON_FIXED_COLUMN_POSITION(mw))
		{
			x1 = NON_FIXED_COLUMN_POSITION(mw);
		}
		else if(x1 >= TRAILING_FIXED_COLUMN_POSITION(mw))
		{
			x1 = TRAILING_FIXED_COLUMN_POSITION(mw) - 1;
		}
	}

	x2 = xbaeColumnToMatrixX(mw, ce) + ((ce == -1) ? ROW_LABEL_WIDTH(mw) : COLUMN_WIDTH(mw, ce)) - 1;
	if(!IS_FIXED_COLUMN(mw, ce))
	{
		if(x2 < NON_FIXED_COLUMN_POSITION(mw))
		{
			x2 = NON_FIXED_COLUMN_POSITION(mw);
		}
		else if(x2 >= TRAILING_FIXED_COLUMN_POSITION(mw))
		{
			x2 = TRAILING_FIXED_COLUMN_POSITION(mw) - 1;
		}
	}

	y1 = xbaeRowToMatrixY(mw, rs);
	if(!IS_FIXED_ROW(mw, rs))
	{
		if(y1 < NON_FIXED_ROW_POSITION(mw))
		{
			y1 = NON_FIXED_ROW_POSITION(mw);
		}
		else if(y1 >= TRAILING_FIXED_ROW_POSITION(mw))
		{
			y1 = TRAILING_FIXED_ROW_POSITION(mw) - 1;
		}
	}

	y2 = xbaeRowToMatrixY(mw, re) + ((re == -1) ? COLUMN_LABEL_HEIGHT(mw) : ROW_HEIGHT(mw, re)) - 1;
	if(!IS_FIXED_ROW(mw, re))
	{
		if(y2 < NON_FIXED_ROW_POSITION(mw))
		{
			y2 = NON_FIXED_ROW_POSITION(mw);
		}
		else if(y2 >= TRAILING_FIXED_ROW_POSITION(mw))
		{
			y2 = TRAILING_FIXED_ROW_POSITION(mw) - 1;
		}
	}

	rect_p->x = x1;
	rect_p->y = y1;
	rect_p->width = x2 - x1 + 1;
	rect_p->height = y2 - y1 + 1;
}

/*
 * Below are some functions to deal with a multi-threaded environment.
 * Use these instead of the Xt stuff, to ensure that we can still cope
 * with X11r5 where this stuff didn't exist.
 */
#ifdef  XtSpecificationRelease
#  if     XtSpecificationRelease > 5
#    define R6plus
#  endif
#endif

void xbaeObjectLock(Widget w)
{
	if(!w)
		return;
#ifdef  R6plus
	if(XmIsGadget(w))
		XtAppLock(XtWidgetToApplicationContext(XtParent(w)));
	else
		XtAppLock(XtWidgetToApplicationContext(w));
#endif
}

void xbaeObjectUnlock(Widget w)
{
	if(!w)
		return;
#ifdef  R6plus
	if(XmIsGadget(w))
		XtAppUnlock(XtWidgetToApplicationContext(XtParent(w)));
	else
		XtAppUnlock(XtWidgetToApplicationContext(w));
#endif
}

/* Some string functions, since strcasecmp()/strncasecmp() are
   not always available */
int _xbaeStrcasecmp(const char *s1, const char *s2)
{
#ifdef HAVE_STRNCASECMP
	return strcasecmp(s1, s2);
#else
	int c1, c2;

	while(*s1 && *s2)
	{
		c1 = tolower(*s1);
		c2 = tolower(*s2);
		if(c1 != c2)
		{
			return (c1 - c2);
		}
		s1++;
		s2++;
	}
	return (int) (*s1 - *s2);
#endif
}


int _xbaeStrncasecmp(const char *s1, const char *s2, size_t count)
{
#ifdef HAVE_STRNCASECMP
	return strncasecmp(s1, s2, count);
#else
	int c1, c2;
	int i = 0;

	while(*s1 && *s2 && i < (int) count)
	{
		c1 = tolower(*s1);
		c2 = tolower(*s2);
		if(c1 != c2)
		{
			return (c1 - c2);
		}
		s1++;
		s2++;
		i++;
	}
	return (int) (*s1 - *s2);
#endif
}

void xbaeScrollRows(XbaeMatrixWidget mw, Boolean Up, int step)
{
	int value, slider_size, increment, page_increment, limit;
	XtVaGetValues(VertScrollChild(mw), (Up ? XmNminimum : XmNmaximum), &limit, NULL);
	XmScrollBarGetValues(VertScrollChild(mw), &value, &slider_size, &increment, &page_increment);
	if(Up)
	{
		XmScrollBarSetValues(VertScrollChild(mw),
							 ((value - step < limit) ? limit : value - step), slider_size,
							 increment, page_increment, True);
	}
	else
	{
		limit -= slider_size;
		XmScrollBarSetValues(VertScrollChild(mw),
							 ((value + step > limit) ? limit : value + step), slider_size,
							 increment, page_increment, True);
	}
}

void xbaeScrollColumns(XbaeMatrixWidget mw, Boolean Left, int step)
{
	int value, slider_size, increment, page_increment, limit;
	XtVaGetValues(HorizScrollChild(mw), (Left ? XmNminimum : XmNmaximum), &limit, NULL);
	XmScrollBarGetValues(HorizScrollChild(mw), &value, &slider_size, &increment, &page_increment);
	if(Left)
	{
		XmScrollBarSetValues(HorizScrollChild(mw),
							 ((value - step < limit) ? limit : value - step), slider_size,
							 increment, page_increment, True);
	}
	else
	{
		limit -= slider_size;
		XmScrollBarSetValues(HorizScrollChild(mw),
							 ((value + step > limit) ? limit : value + step), slider_size,
							 increment, page_increment, True);
	}
}
