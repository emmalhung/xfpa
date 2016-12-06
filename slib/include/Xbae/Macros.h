/*
 * Copyright (c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright (c) 1995-99 Andrew Lister
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
 * $Id: Macros.h,v 1.69 2005/04/15 21:43:47 tobiasoed Exp $
 */

/*
 * Macros.h created by Andrew Lister (6 August, 1995)
 */

#ifndef _Xbae_Macros_h
#  define _Xbae_Macros_h

#  include <Xm/DrawP.h>
#  include <Xm/TextP.h>
#  include <Xm/PrimitiveP.h>


#  ifdef __cplusplus
extern "C" {
#  endif

/*
 * The third parameter is a string added for debugging purposes.
 */
#  if (XmVersion >= 2000)
#    define DRAW_SHADOW(dpy, draw, dbg, tgc, bgc, sz, x, y, w, h, type)		\
	do {									\
		DEBUGOUT(_XbaeDebug(__FILE__, NULL,				\
			"XmeDrawShadows(%s) [%d,%d,%d,%d - %d %s]\n",		\
			dbg, x, y, w, h, sz, _XbaeDebugShadowTypeToString(type)));	\
		XmeDrawShadows(dpy, draw, tgc, bgc, x, y, w, h, sz, type);	\
	} while(0)
#  else
#    define DRAW_SHADOW(dpy, draw, dbg, tgc, bgc, sz, x, y, w, h, type)		\
	do {									\
		DEBUGOUT(_XbaeDebug(__FILE__, NULL,				\
			"_XmDrawShadows(%s) [%d,%d,%d,%d - %d %s]\n",		\
			dbg, x, y, w, h, sz, _XbaeDebugShadowTypeToString(type)));	\
		_XmDrawShadows(dpy, draw, tgc, bgc, x, y, w, h, sz, type);	\
	} while(0)
#  endif

#  if XmVersion >= 2000
#    define DRAW_HIGHLIGHT(dpy, draw, gc, x, y, w, h, sz, type) \
	  XmeDrawHighlight(dpy, draw, gc, x, y, w, h, sz)
#  else
#    define DRAW_HIGHLIGHT(dpy, draw, gc, x, y, w, h, sz, type) \
	  _XmDrawHighlight(dpy, draw, gc, x, y, w, h, sz, type)
#  endif
/*
 * Macros to retrieve our children.
 */
#  define XbaeNumChildren		10

#  define HorizScrollChild(mw)    (mw->matrix.horizontal_sb)
#  define VertScrollChild(mw)     (mw->matrix.vertical_sb)
#  define TextChild(mw)           (mw->matrix.text_field)

#  define ClipChild(mw)           (mw->matrix.clip_window)
#  define LeftClip(mw)            (mw->matrix.left_clip)
#  define RightClip(mw)           (mw->matrix.right_clip)
#  define TopClip(mw)             (mw->matrix.top_clip)
#  define BottomClip(mw)          (mw->matrix.bottom_clip)
#  define RowLabelClip(mw)        (mw->matrix.row_label_clip)
#  define ColumnLabelClip(mw)     (mw->matrix.column_label_clip)

#  define VISIBLE_NON_FIXED_WIDTH(mw)     (mw->matrix.visible_non_fixed_width)
#  define VISIBLE_NON_FIXED_HEIGHT(mw)	(mw->matrix.visible_non_fixed_height)

#  define VISIBLE_FIXED_COLUMN_WIDTH(mw)	(mw->matrix.visible_fixed_column_width)
#  define VISIBLE_FIXED_ROW_HEIGHT(mw)	(mw->matrix.visible_fixed_row_height)

#  define VISIBLE_TRAILING_FIXED_COLUMN_WIDTH(mw)	(mw->matrix.visible_trailing_fixed_column_width)
#  define VISIBLE_TRAILING_FIXED_ROW_HEIGHT(mw)	(mw->matrix.visible_trailing_fixed_row_height)

/*
 * SGO: changed the meaning of VERT_ORIGIN:
 * now it contains the pixel position of the first row, same as
 * HORIZ_ORIGIN, the top_row should now be accessed by TOP_ROW
 * Tobias: I killed the top_row/left_column resource use 
 * xbaeTopRow/xbaeLeftColumn instaead
 */
#  define VERT_ORIGIN(mw)		(mw->matrix.vert_origin)
#  define HORIZ_ORIGIN(mw)	(mw->matrix.horiz_origin)
#  define TRAILING_ROW_ORIGIN(mw) (mw->matrix.rows - \
				  (int)mw->matrix.trailing_fixed_rows)
#  define TRAILING_COLUMN_ORIGIN(mw) (mw->matrix.columns - \
				   (int)mw->matrix.trailing_fixed_columns)

/*
 * Macros
 */

#  ifndef Max
#    define Max(x, y)       (((x) > (y)) ? (x) : (y))
#  endif
#  ifndef Min
#    define Min(x, y)       (((x) < (y)) ? (x) : (y))
#  endif

#  define CELL_BORDER_WIDTH(mw)	( \
				 mw->matrix.cell_shadow_thickness + \
				 mw->matrix.cell_highlight_thickness + \
				 mw->matrix.text_shadow_thickness + \
                 mw->matrix.cell_margin_width)

#  define CELL_BORDER_HEIGHT(mw)	( \
				 mw->matrix.cell_shadow_thickness + \
				 mw->matrix.cell_highlight_thickness + \
				 mw->matrix.text_shadow_thickness + \
                 mw->matrix.cell_margin_height)

#  define CELL_FONT_WIDTH(mw)  (mw->matrix.cell_font.width)
#  define CELL_FONT_HEIGHT(mw) (mw->matrix.cell_font.height)
#  define CELL_BASELINE(mw)    (mw->matrix.cell_baseline)

#  define LABEL_FONT_WIDTH(mw)      (mw->matrix.label_font.width)
#  define LABEL_FONT_HEIGHT(mw)     (mw->matrix.label_font.height)
#  define ROW_LABEL_BASELINE(mw)    (mw->matrix.row_label_baseline)
#  define COLUMN_LABEL_BASELINE(mw) (- mw->matrix.label_font.y)

/*
 * The text height defines the row height.  It needs to be the biggest
 * we can expect from both font and label font
 */
#  define TEXT_HEIGHT(mw)		(Max(CELL_FONT_HEIGHT(mw), LABEL_FONT_HEIGHT(mw)))



#  define ROW_LABEL_WIDTH(mw)	( \
    (mw->matrix.row_labels || mw->matrix.xmrow_labels) \
     ? (mw->matrix.row_label_width) \
       ? 2 * CELL_BORDER_WIDTH(mw) + mw->matrix.row_label_width * LABEL_FONT_WIDTH(mw) \
       : 2 * CELL_BORDER_WIDTH(mw) + mw->matrix.row_label_maxwidth \
     : 0)

#  define COLUMN_LABEL_HEIGHT(mw) ( \
     (mw->matrix.column_labels || mw->matrix.xmcolumn_labels) \
      ? 2 * CELL_BORDER_HEIGHT(mw) + mw->matrix.column_label_maxlines * LABEL_FONT_HEIGHT(mw) \
      : 0)

#  define DEFAULT_ROW_HEIGHT(mw) ( \
        (mw->matrix.row_height_in_pixels) \
        ? (1 * TEXT_HEIGHT(mw) + 2 * CELL_BORDER_HEIGHT(mw)) \
        : 1)

#  define DEFAULT_COLUMN_WIDTH(mw) ( \
        (mw->matrix.column_width_in_pixels) \
        ? (5 * CELL_FONT_WIDTH(mw) + 2 * CELL_BORDER_WIDTH(mw)) \
        : 5)



#  define SCROLLBAR_TOP(mw) ( \
                 mw->matrix.scrollbar_placement == XmTOP_LEFT || \
				 mw->matrix.scrollbar_placement == XmTOP_RIGHT \
)

#  define HORIZ_SB_WIDTH(mw)	(HorizScrollChild(mw)->core.width + \
				 2 * HorizScrollChild(mw)->core.border_width)

#  define HORIZ_SB_HEIGHT(mw)	(HorizScrollChild(mw)->core.height + \
				 2 * HorizScrollChild(mw)->core.border_width +\
				 mw->matrix.space)

#  define HORIZ_SB_SPACE(mw) (! HorizScrollChild(mw)->core.managed ? 0 : HORIZ_SB_HEIGHT(mw))

#  define HORIZ_SB_OFFSET(mw) ((SCROLLBAR_TOP(mw)) ?  HORIZ_SB_SPACE(mw) : 0)


#  define SCROLLBAR_LEFT(mw) ( \
                 mw->matrix.scrollbar_placement == XmTOP_LEFT || \
				 mw->matrix.scrollbar_placement == XmBOTTOM_LEFT \
)

#  define VERT_SB_WIDTH(mw) (VertScrollChild(mw)->core.width + \
				 2 * VertScrollChild(mw)->core.border_width + \
				 mw->matrix.space)

#  define VERT_SB_HEIGHT(mw) (VertScrollChild(mw)->core.height + \
				 2 * VertScrollChild(mw)->core.border_width)

#  define VERT_SB_SPACE(mw) (! VertScrollChild(mw)->core.managed ? 0 : VERT_SB_WIDTH(mw))

#  define VERT_SB_OFFSET(mw) ((SCROLLBAR_LEFT(mw)) ? VERT_SB_SPACE(mw) : 0)



#  if 1							/* Whether or not to check each column_position access for debugging */
#    define COLUMN_POSITION(mw, column) mw->matrix.column_positions[column]
#    define ROW_POSITION(mw, row) mw->matrix.row_positions[row]
#  else
#    define COLUMN_POSITION(mw, column) xbaeCheckColumnPosition(mw, column)
#    define ROW_POSITION(mw, row) xbaeCheckRowPosition(mw, row)
#  endif

#  define TOTAL_HEIGHT(mw) ROW_POSITION(mw, mw->matrix.rows)
#  define TOTAL_WIDTH(mw) COLUMN_POSITION(mw, mw->matrix.columns)

#  define FIXED_COLUMN_WIDTH(mw) COLUMN_POSITION(mw, mw->matrix.fixed_columns)
#  define FIXED_ROW_HEIGHT(mw) ROW_POSITION(mw, mw->matrix.fixed_rows)

#  define ROW_HEIGHT(mw, row) \
    (ROW_POSITION(mw, row + 1) - ROW_POSITION(mw, row))
#  define COLUMN_WIDTH(mw, column) \
    (COLUMN_POSITION(mw, column + 1) - COLUMN_POSITION(mw, column))

#  define TRAILING_FIXED_ROW_HEIGHT(mw) \
    (ROW_POSITION(mw, mw->matrix.rows) - ROW_POSITION(mw, TRAILING_ROW_ORIGIN(mw)))
#  define TRAILING_FIXED_COLUMN_WIDTH(mw) \
    (COLUMN_POSITION(mw, mw->matrix.columns) - COLUMN_POSITION(mw, TRAILING_COLUMN_ORIGIN(mw)))

#  define NON_FIXED_HEIGHT(mw) \
    (ROW_POSITION(mw, TRAILING_ROW_ORIGIN(mw)) - ROW_POSITION(mw, mw->matrix.fixed_rows))
#  define NON_FIXED_WIDTH(mw) \
    (COLUMN_POSITION(mw, TRAILING_COLUMN_ORIGIN(mw)) - COLUMN_POSITION(mw, mw->matrix.fixed_columns))

#  define FIXED_COLUMN_POSITION(mw)	(ROW_LABEL_WIDTH(mw) + \
     VERT_SB_OFFSET(mw) + mw->manager.shadow_thickness)
#  define FIXED_ROW_POSITION(mw)	(COLUMN_LABEL_HEIGHT(mw) + \
     HORIZ_SB_OFFSET(mw) + mw->manager.shadow_thickness)

#  define NON_FIXED_COLUMN_POSITION(mw) \
    (FIXED_COLUMN_POSITION(mw) + VISIBLE_FIXED_COLUMN_WIDTH(mw))
#  define NON_FIXED_ROW_POSITION(mw) \
    (FIXED_ROW_POSITION(mw) + VISIBLE_FIXED_ROW_HEIGHT(mw))

#  define TRAILING_FIXED_COLUMN_POSITION(mw) \
    (NON_FIXED_COLUMN_POSITION(mw) + VISIBLE_NON_FIXED_WIDTH(mw))
#  define TRAILING_FIXED_ROW_POSITION(mw) \
    (NON_FIXED_ROW_POSITION(mw)	+ VISIBLE_NON_FIXED_HEIGHT(mw))

#  define MATRIX_VISIBLE_WIDTH(mw) ((int)(mw->core.width - \
                    2 * mw->manager.shadow_thickness - \
                    ROW_LABEL_WIDTH(mw) - \
                    VERT_SB_SPACE(mw)))
#  define MATRIX_VISIBLE_HEIGHT(mw) ((int)(mw->core.height -\
					2 * mw->manager.shadow_thickness - \
					COLUMN_LABEL_HEIGHT(mw) -\
					HORIZ_SB_SPACE(mw)))

#  define EMPTY_HEIGHT(mw) ( \
    MATRIX_VISIBLE_HEIGHT(mw) > TOTAL_HEIGHT(mw) \
    ? MATRIX_VISIBLE_HEIGHT(mw) - TOTAL_HEIGHT(mw) \
    : 0)
#  define EMPTY_WIDTH(mw) ( \
    MATRIX_VISIBLE_WIDTH(mw) > TOTAL_WIDTH(mw) \
    ? MATRIX_VISIBLE_WIDTH(mw) - TOTAL_WIDTH(mw) \
    : 0)

#  define FILL_ROW(mw) ( \
    (mw->matrix.non_fixed_detached_top && mw->matrix.fixed_rows) \
    ? (mw->matrix.fixed_rows - 1) \
    : (mw->matrix.trailing_attached_bottom && mw->matrix.trailing_fixed_rows) \
      ? (TRAILING_ROW_ORIGIN(mw) - 1) \
      : (mw->matrix.rows - 1))

#  define FILL_COLUMN(mw) ( \
    (mw->matrix.non_fixed_detached_left && mw->matrix.fixed_columns) \
    ? (mw->matrix.fixed_columns - 1) \
    : (mw->matrix.trailing_attached_right && mw->matrix.trailing_fixed_columns) \
      ? (TRAILING_COLUMN_ORIGIN(mw) - 1) \
      : (mw->matrix.columns - 1))

#  define IS_FILL_ROW(mw, row) (mw->matrix.fill && row == FILL_ROW(mw))
#  define IS_FILL_COLUMN(mw, column) (mw->matrix.fill && column == FILL_COLUMN(mw))




#  define IS_LEADING_FIXED_COLUMN(mw, column) (column < (int)mw->matrix.fixed_columns)

#  define IS_TRAILING_FIXED_COLUMN(mw, column) (column >= TRAILING_COLUMN_ORIGIN(mw))

#  define IS_FIXED_COLUMN(mw, column) (IS_LEADING_FIXED_COLUMN(mw, column) || \
				     IS_TRAILING_FIXED_COLUMN(mw, column))

#  define IS_LEADING_FIXED_ROW(mw, row) (row < (int)mw->matrix.fixed_rows)

#  define IS_TRAILING_FIXED_ROW(mw, row) (row >= TRAILING_ROW_ORIGIN(mw))

#  define IS_FIXED_ROW(mw, row) (IS_LEADING_FIXED_ROW(mw, row) || \
			       IS_TRAILING_FIXED_ROW(mw, row))

#  define IS_FIXED(mw, row, column) (IS_FIXED_ROW(mw, row) || \
				   IS_FIXED_COLUMN(mw, column))


/* Inline functions */

#  define xbaeFreeArray(array) \
    do{ \
        if(array) { \
            XtFree((XtPointer) array); \
            array = NULL; \
        } \
    } while(0)

#  define xbaeFreeColumnWidths(mw) \
    xbaeFreeArray(mw->matrix.column_widths)

#  define xbaeFreeRowHeights(mw) \
    xbaeFreeArray(mw->matrix.row_heights)

#  define xbaeFreeColumnMaxLengths(mw) \
    xbaeFreeArray(mw->matrix.column_max_lengths)

#  define xbaeFreeColumnPositions(mw) \
    xbaeFreeArray(mw->matrix.column_positions)

#  define xbaeFreeRowPositions(mw) \
    xbaeFreeArray(mw->matrix.row_positions)

#  define xbaeFreeColumnAlignments(mw) \
    xbaeFreeArray(mw->matrix.column_alignments)

#  define xbaeFreeColumnLabelBackgrounds(mw) \
	xbaeFreeArray(mw->matrix.column_label_backgrounds);

#  define xbaeFreeColumnLabelForegrounds(mw) \
	xbaeFreeArray(mw->matrix.column_label_foregrounds);

#  define xbaeFreeRowLabelBackgrounds(mw) \
	xbaeFreeArray(mw->matrix.row_label_backgrounds);

#  define xbaeFreeRowLabelForegrounds(mw) \
	xbaeFreeArray(mw->matrix.row_label_foregrounds);

#  define xbaeFreeColumnButtonLabels(mw) \
    xbaeFreeArray(mw->matrix.column_button_labels)

#  define xbaeFreeShowColumnArrows(mw) \
    xbaeFreeArray(mw->matrix.show_column_arrows)

#  define xbaeFreeColumnFontBold(mw) \
    xbaeFreeArray(mw->matrix.column_font_bold)

#  define xbaeFreeRowButtonLabels(mw) \
    xbaeFreeArray(mw->matrix.row_button_labels)

#  define xbaeFreeColumnLabelAlignments(mw) \
    xbaeFreeArray(mw->matrix.column_label_alignments)

#  define xbaeFreeRowUserData(mw) \
    xbaeFreeArray(mw->matrix.row_user_data)

#  define xbaeFreeColumnUserData(mw) \
    xbaeFreeArray(mw->matrix.column_user_data)

#  define xbaeFreeRowShadowTypes(mw) \
    xbaeFreeArray(mw->matrix.row_shadow_types)

#  define xbaeFreeColumnShadowTypes(mw) \
    xbaeFreeArray(mw->matrix.column_shadow_types)

#  define CreateRowPositions(mw) \
	(int *)XtMalloc((mw->matrix.rows+1) * sizeof(int))
#  define CreateColumnPositions(mw) \
	(int *)XtMalloc((mw->matrix.columns+1) * sizeof(int))


#  ifdef NEED_WCHAR
#    define TWO_BYTE_FONT(mw)	(mw->matrix.font->max_byte1 != 0)
#  endif

#  define GC_PARENT_WINDOW(w)	XtWindow(_XbaeGetShellAncestor((Widget)w))

/*
 * End of array flags for the array type converters
 */
#  define BAD_SIZE	-1
#  define BAD_MAXLENGTH	-1
#  define BAD_SHADOW	(126)	/* see Xm.h */
#  define BAD_ALIGNMENT	3		/* see Xm.h */
#  define BAD_PIXEL	0x10000000	/* normally 256 indices */

/*
 * SetClipMask flags for indicating clip areas
 */
#  define CLIP_FIXED_COLUMNS          0x0001
#  define CLIP_FIXED_ROWS             0x0002
#  define CLIP_TRAILING_FIXED_COLUMNS	0x0004
#  define CLIP_TRAILING_FIXED_ROWS	0x0008
#  define CLIP_VISIBLE_WIDTH          0x0010
#  define CLIP_VISIBLE_HEIGHT         0x0020
#  define CLIP_TRAILING_HORIZ_FILL	0x0040
#  define CLIP_TRAILING_VERT_FILL		0x0080
#  define CLIP_COLUMN_LABELS     		0x0100
#  define CLIP_ROW_LABELS     		0x0200

/*
 * Grid shadow/line detectors
 */
#  define GRID_MODE_ROW		(XmGRID_ROW_LINE & XmGRID_ROW_SHADOW)
#  define GRID_MODE_COLUMN	(XmGRID_COLUMN_LINE & XmGRID_COLUMN_SHADOW)
#  define GRID_MODE_SHADOW	(XmGRID_CELL_SHADOW & XmGRID_ROW_SHADOW & XmGRID_COLUMN_SHADOW)

#  define IN_GRID_ROW_MODE(mw)	(mw->matrix.grid_type & GRID_MODE_ROW)
#  define IN_GRID_COLUMN_MODE(mw)	(mw->matrix.grid_type & GRID_MODE_COLUMN)
#  define IN_GRID_SHADOW_MODE(mw) (mw->matrix.grid_type & GRID_MODE_SHADOW)
#  define IN_GRID_LINE_MODE(mw)   (mw->matrix.grid_type && !IN_GRID_SHADOW_MODE(mw))

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Macros_h */
