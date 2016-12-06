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
 * $Id: MatrixP.h,v 1.76 2005/07/31 18:35:31 tobiasoed Exp $
 */

/*
 * MatrixP.h - Private definitions for Matrix widget
 */
/*
 * 2005.10.31 Modified for use in Aurora. See note in Matrix.h
 */


#ifndef _Xbae_MatrixP_h
#  define _Xbae_MatrixP_h

#  include <wchar.h>
#  include <X11/Xft/Xft.h>
#  include <Xm/ManagerP.h>
#  include <Xbae/Matrix.h>

#  ifdef __cplusplus
extern "C" {
#  endif


/*
 * New types for the class methods
 */

	typedef void (*XbaeMatrixSetCellProc) (XbaeMatrixWidget, int, int, const String, Boolean);

	typedef String(*XbaeMatrixGetCellProc) (XbaeMatrixWidget, int, int);

	typedef void (*XbaeMatrixEditCellProc) (XbaeMatrixWidget, XEvent *, int, int, String *, Cardinal);

	typedef void (*XbaeMatrixSelectCellProc) (XbaeMatrixWidget, int, int);

	typedef void (*XbaeMatrixSelectRowProc) (XbaeMatrixWidget, int);

	typedef void (*XbaeMatrixShowColumnArrowsProc) (XbaeMatrixWidget, int, Boolean);

	typedef void (*XbaeMatrixSelectColumnProc) (XbaeMatrixWidget, int);

	typedef void (*XbaeMatrixDeselectAllProc) (XbaeMatrixWidget);

	typedef void (*XbaeMatrixSelectAllProc) (XbaeMatrixWidget);

	typedef void (*XbaeMatrixDeselectCellProc) (XbaeMatrixWidget, int, int);

	typedef void (*XbaeMatrixDeselectRowProc) (XbaeMatrixWidget, int);

	typedef void (*XbaeMatrixDeselectColumnProc) (XbaeMatrixWidget, int);

	typedef Boolean(*XbaeMatrixCommitEditProc) (XbaeMatrixWidget, XEvent *, Boolean);

	typedef void (*XbaeMatrixCancelEditProc) (XbaeMatrixWidget, Boolean);

	typedef void (*XbaeMatrixAddRowsProc) (XbaeMatrixWidget, int, String *, String *, Pixel *, Pixel *, int);

	typedef void (*XbaeMatrixAddVarRowsProc) (XbaeMatrixWidget, int, String *,
											  String *, short *, int *,
											  unsigned char *, unsigned char *, Pixel *, Pixel *, int);

	typedef void (*XbaeMatrixDeleteRowsProc) (XbaeMatrixWidget, int, int);

	typedef void (*XbaeMatrixAddColumnsProc) (XbaeMatrixWidget, int, String *,
											  String *, short *, int *,
											  unsigned char *, unsigned char *, Pixel *, Pixel *, int);

	typedef void (*XbaeMatrixDeleteColumnsProc) (XbaeMatrixWidget, int, int);

	typedef void (*XbaeMatrixSetRowColorsProc) (XbaeMatrixWidget, int, Pixel *, int, Boolean);

	typedef void (*XbaeMatrixSetColumnColorsProc) (XbaeMatrixWidget, int, Pixel *, int, Boolean);

	typedef void (*XbaeMatrixSetCellColorProc) (XbaeMatrixWidget, int, int, Pixel, Boolean);


/*
 * New fields for the Matrix widget class record
 */
	typedef struct {
		XbaeMatrixSetCellProc set_cell;
		XbaeMatrixGetCellProc get_cell;
		XbaeMatrixEditCellProc edit_cell;
		XbaeMatrixSelectCellProc select_cell;
		XbaeMatrixShowColumnArrowsProc set_show_column_arrows;
		XbaeMatrixSelectRowProc select_row;
		XbaeMatrixSelectColumnProc select_column;
		XbaeMatrixDeselectAllProc deselect_all;
		XbaeMatrixSelectAllProc select_all;
		XbaeMatrixDeselectCellProc deselect_cell;
		XbaeMatrixDeselectRowProc deselect_row;
		XbaeMatrixDeselectColumnProc deselect_column;
		XbaeMatrixCommitEditProc commit_edit;
		XbaeMatrixCancelEditProc cancel_edit;
		XbaeMatrixAddRowsProc add_rows;
		XbaeMatrixAddVarRowsProc add_var_rows;
		XbaeMatrixDeleteRowsProc delete_rows;
		XbaeMatrixAddColumnsProc add_columns;
		XbaeMatrixDeleteColumnsProc delete_columns;
		XbaeMatrixSetRowColorsProc set_row_colors;
		XbaeMatrixSetColumnColorsProc set_column_colors;
		XbaeMatrixSetCellColorProc set_cell_color;
		XtPointer extension;
	} XbaeMatrixClassPart;

/*
 * Full class record declaration
 */
	typedef struct _XbaeMatrixClassRec {
		CoreClassPart core_class;
		CompositeClassPart composite_class;
		ConstraintClassPart constraint_class;
		XmManagerClassPart manager_class;
		XbaeMatrixClassPart matrix_class;
	} XbaeMatrixClassRec;

	externalref XbaeMatrixClassRec xbaeMatrixClassRec;


/*
 * Inheritance constants for set/get/edit methods
 */
#  define XbaeInheritGetCell ((XbaeMatrixGetCellProc) _XtInherit)
#  define XbaeInheritSetCell ((XbaeMatrixSetCellProc) _XtInherit)
#  define XbaeInheritEditCell ((XbaeMatrixEditCellProc) _XtInherit)
#  define XbaeInheritSelectCell ((XbaeMatrixSelectCellProc) _XtInherit)
#  define XbaeInheritShowColumnArrows ((XbaeMatrixShowColumnArrowsProc) _XtInherit)
#  define XbaeInheritSelectRow ((XbaeMatrixSelectRowProc) _XtInherit)
#  define XbaeInheritSelectColumn ((XbaeMatrixSelectColumnProc) _XtInherit)
#  define XbaeInheritHighlightCell ((XbaeMatrixHighlightCellProc) _XtInherit)
#  define XbaeInheritHighlightRow ((XbaeMatrixHighlightRowProc) _XtInherit)
#  define XbaeInheritHighlightColumn ((XbaeMatrixHighlightColumnProc) _XtInherit)
#  define XbaeInheritDeselectAll ((XbaeMatrixDeselectAllProc) _XtInherit)
#  define XbaeInheritSelectAll ((XbaeMatrixSelectAllProc) _XtInherit)
#  define XbaeInheritDeselectCell ((XbaeMatrixDeselectCellProc) _XtInherit)
#  define XbaeInheritDeselectRow ((XbaeMatrixDeselectRowProc) _XtInherit)
#  define XbaeInheritDeselectColumn ((XbaeMatrixDeselectColumnProc) _XtInherit)
#  define XbaeInheritCommitEdit ((XbaeMatrixCommitEditProc) _XtInherit)
#  define XbaeInheritCancelEdit ((XbaeMatrixCancelEditProc) _XtInherit)
#  define XbaeInheritAddRows ((XbaeMatrixAddRowsProc) _XtInherit)
#  define XbaeInheritAddVarRows ((XbaeMatrixAddVarRowsProc) _XtInherit)
#  define XbaeInheritDeleteRows ((XbaeMatrixDeleteRowsProc) _XtInherit)
#  define XbaeInheritAddColumns ((XbaeMatrixAddColumnsProc) _XtInherit)
#  define XbaeInheritDeleteColumns ((XbaeMatrixDeleteColumnsProc)_XtInherit)
#  define XbaeInheritSetRowColors ((XbaeMatrixSetRowColorsProc)_XtInherit)
#  define XbaeInheritSetColumnColors ((XbaeMatrixSetColumnColorsProc)_XtInherit)
#  define XbaeInheritSetCellColor ((XbaeMatrixSetCellColorProc)_XtInherit)

/*
 * Bundle per cell attributes into one structure
 */
	typedef struct _XbaeMatrixPerCellRec {
		unsigned char shadow_type;	/* Per cell shadow type     */
		unsigned char highlighted;	/* Is the cell highlighted ?    */
		Boolean selected;			/* Is the cell selected ?   */
		Boolean underlined;			/* Is the cell underlined ? */
		XtPointer user_data;		/* userdata; cant be edited in xbae */
		Pixel background;			/* Background pixel per cell    */
		Pixel color;				/* Foreground pixel per cell    */
		Widget widget;				/* So-called cell widgets   */
		Pixmap pixmap;				/* pixmap; cant be edited in xbae */
		Pixmap mask;				/* clipmask; cant be edited in xbae */
		String CellValue;			/* String */
		XrmQuark qtag;				/* The quarkified tag of the font used to draw the CellValue */
	} XbaeMatrixPerCellRec;

	typedef struct {
		XmFontType type;	/* XmFONT_IS_FONT, XmFONT_IS_FONTSET or XmFONT_IS_XFT */
		XtPointer fontp;	/* generic pointer to font information */
		XrmQuark qtag;		/* quarkified assigned tag */
		short width;		/* average character width */
		short height;		/* max character height */
		short y;			/* baseline offset */
		Font id;			/* used for XmFONT_IS_FONT only */
	} XbaeMatrixFontInfo;

	typedef struct {
		Window win;
		XftDraw *draw;
	} XbaeXftDrawCache;

	typedef struct {
		Pixel pixel;
		XftColor xftcolor;
	} XbaeXftColorCache;

/*
 * New fields for the Matrix widget record
 */
	typedef struct {
		Boolean allow_column_resize;	/* Public: can columns dynamically resize?   */
		Boolean allow_row_resize;	/* Public: can rows dynamically resize?   */

		Boolean fill;			/* Public: fill available space?      */
		Boolean horz_fill;		/* Public: when filled, extend the 'last' column to fill available space? */
		Boolean vert_fill;		/* Public: when filled, extend the 'last' row to fill available space? */
		Boolean non_fixed_detached_left;	/* Public: when filled, put empty space after fixed columns */
		Boolean non_fixed_detached_top;	/* Public: when filled, put empty space after fixed rows */
		Boolean trailing_attached_right;	/* Public: when filled, put trailing columns fixed to right */
		Boolean trailing_attached_bottom;	/* Public: when filled, put trailing rows fixed to bottom   */

		Boolean bold_labels;	/* Public: draw bold row/column labels?      */
		Boolean useXbaeInput;	/* Public: Whether to use XbaeInput widget */
		Boolean reverse_select;	/* Public: reverse colours - selected cells? */
		Boolean scroll_select;	/* Public: flag to scroll a selected cell    */
		Boolean traverse_fixed;	/* Public: allow traversal to fixed cells?   */
		Boolean calc_cursor_position;	/* Public: calculate insert pos from click   */
		Boolean text_child_is_mapped;	/* Private: Is the text child on top of a cell? */
		unsigned int disable_redisplay;	/* Private: disable redisplay counter      */

		Boolean show_arrows;	/* Public: sow arrows when text obscured?    */
		Boolean *show_column_arrows;	/* Public: which columns will show arrows    */
		Boolean *column_font_bold;	/* Public: which columns have bold fonts     */

		Boolean button_labels;	/* Public: draw labels as buttons?      */
		Boolean *column_button_labels;	/* Public: which column labels are buttons   */
		Boolean *row_button_labels;	/* Public: which row labels are buttons      */

		Boolean column_width_in_pixels;	/* Public: column width mesured in pixels?    */
		Boolean row_height_in_pixels;	/* Public: row height mesured in pixels?    */
		short row_label_width;	/* Public: max width of row labels in chars  */
		short *column_widths;	/* Public: width of each column in chars or pixels */
		short *row_heights;		/* Public: height of each row in chars or pixels */
		int *column_positions;	/* Private: pixel position of each column     */
		int *row_positions;		/* Private: pixel position of each row */

		int visible_fixed_column_width;
		int visible_fixed_row_height;
		int visible_trailing_fixed_column_width;
		int visible_trailing_fixed_row_height;
		int visible_non_fixed_height;
		int visible_non_fixed_width;

		int *column_max_lengths;	/* Public: max length of each col in chars   */

		Boolean multi_line_cell;	/* Public: Whether to draw more than one line in a cell */
		unsigned char wrap_type;	/* Public: How to wrap the lines in multi_line_cell mode */

		unsigned char scrollbar_placement;	/* Public: placement of the scrollbars      */
		unsigned char selection_policy;	/* Public: as for XmList */
		unsigned char grid_type;	/* Public: shadowed in/shadowed out/plain    */
		unsigned char shadow_type;	/* Public: matrix window shadow type      */
		unsigned char cell_shadow_type;	/* Public: cell shadow type       */

		unsigned char hsb_display_policy;	/* Public: horiz scroll bar display policy   */
		unsigned char vsb_display_policy;	/* Public: vert scroll bar display policy    */

		unsigned char row_label_alignment;	/* Public: alignment of row labels      */
		unsigned char *column_label_alignments;	/* Public: alignment of each column label */
		unsigned char *column_alignments;	/* Public: alignment of each column      */

		unsigned char *column_shadow_types;	/* Public: 1D array of per col shadow types  */
		unsigned char *row_shadow_types;	/* Public: 1D array of per row shadow types  */

		XmString *xmcolumn_labels;	/* Public: array of xmlabels above each column */
		XmString *xmrow_labels;	/* Public: array of xmlabels next to each row  */
		String *column_labels;	/* Public: array of labels above each column   */
		String *row_labels;		/* Public: array of labels next to each row    */
		int column_label_maxlines;	/* Private: max number of lines in column labels */
		int row_label_maxwidth;	/* Private: max line length of row labels        */

		XtPointer *column_user_data;	/* Public: 1D array of per column user data  */
		XtPointer *row_user_data;	/* Public: 1D array of per row user data     */

		int columns;			/* Public: number of cells per row      */
		int rows;				/* Public: number of rows per column      */
		Dimension fixed_columns;	/* Public: number of leading fixed columns   */
		Dimension fixed_rows;	/* Public: number of leading fixed rows      */
		Dimension trailing_fixed_columns;	/* Public: number of trailing fixed columns  */
		Dimension trailing_fixed_rows;	/* Public: number of trailing fixed rows     */
		Dimension visible_columns;	/* Public: number of columns to make visible */
		Dimension visible_rows;	/* Public: number of rows to make visible    */

		Dimension cell_margin_height;	/* Public: margin height for textField      */
		Dimension cell_margin_width;	/* Public: margin width for textField      */
		Dimension cell_highlight_thickness;	/* Public: highlight thickness for textField  */
		Dimension cell_shadow_thickness;	/* Public: shadow thickness for each cell    */
		Dimension text_shadow_thickness;	/* Public: shadow thickness for text field   */
		Dimension space;		/* Public: spacing for scrollbars      */
		Dimension underline_width;	/* Public:  number of pixels thick the underline is */

		int alt_row_count;		/* Public: # of rows for e/o background      */
		Pixel even_row_background;	/* Public: even row background color      */
		Pixel odd_row_background;	/* Public: odd row background color      */

		Pixel column_label_color;	/* Public: color of column label      */
		Pixel row_label_color;	/* Public: color of row label       */

		Pixel button_label_background;	/* Public: color of button label background */
		Pixel *column_label_backgrounds;	/*Public: background colors of column labels */
		Pixel *column_label_foregrounds;	/*Public: foreground colors of column labels */
		Pixel *row_label_backgrounds;	/*Public: background colors of row labels */
		Pixel *row_label_foregrounds;	/*Public: foreground colors of row labels */
		Pixel grid_line_color;	/* Public: color of grid, for XmGrid_LINE    */
		Pixel selected_background;	/* Public: background for selected cells     */
		Pixel selected_foreground;	/* Public: foreground for selected cells     */
		Pixel scroll_background;	/* Public: bacground for scrollbar */
		Pixel text_background;	/* Public: background for the text child   */
		Boolean text_background_is_cell;	/* Public: background for the text child when text_background is undefined */

		Position underline_position;	/* Public: number of pixels below the text baseline */

		XtTranslations text_translations;	/* Public: translations for textField widget */

		XtCallbackList default_action_callback;	/* Public: called for a double click     */
		XtCallbackList draw_cell_callback;	/* Public: called when a cell is drawn      */
		XtCallbackList enter_cell_callback;	/* Public: called when a cell is entered     */
		XtCallbackList track_cell_callback;	/* Public: called when a cell is crossed     */
		XtCallbackList label_activate_callback;	/* Public: called when label pressed     */
		XtCallbackList leave_cell_callback;	/* Public: called when a cell is left      */
		XtCallbackList modify_verify_callback;	/* Public: verify change to textField     */
		/* Public: and a draw_cell_callback is set   */
		XtCallbackList process_drag_callback;	/* Public: called when a drag is initiated */
		XtCallbackList resize_callback;	/* Public: called when Matrix is resized     */
		XtCallbackList resize_row_callback;	/* Public: called when row is resized  */
		XtCallbackList resize_column_callback;	/* Public:  called when column is resized  */
		XtCallbackList select_cell_callback;	/* Public: called when cells are selected   */
		XtCallbackList traverse_cell_callback;	/* Public: next cell to traverse to      */
		XtCallbackList value_changed_callback;	/* Public: same as XmText(3)                    */
		XtCallbackList write_cell_callback;	/* Public: called when a cell needs to be set */
		XmRenderTable render_table;	/* Public: renderTable from which we get the fonts used to draw cells/labels */
		Boolean check_set_render_table;	/* Private: set state of render table */
		String cell_rendition_tag;	/* Public: tag associated with rendition to be used for cell rendering */
		String label_rendition_tag;	/* Public: tag associated with rendition to be used for label rendering */
		XmFontList label_font_list;	/* Public: fontList of labels          */
		XbaeMatrixFontInfo cell_font;	/* Private: Cashed info on the font used to draw cells with no tags */
		XbaeMatrixFontInfo label_font;	/* Private: Cashed info on the font used to draw labels */
		XbaeMatrixFontInfo draw_font;	/* Private: Cashed info on the font currently used for cell rendering */
		XrmQuark current_draw_qtag;	/* Private: The quarkified tag of the font in the draw_gc */
		XrmQuark current_text_qtag;	/* Private: The quarkified tag of the rendition installed on the text child */

		/*
		 * private state
		 */
		Dimension desired_height;	/* Private: height widget wants to be      */
		Dimension desired_width;	/* Private: width widget wants to be      */

		int num_selected_cells;		/* Private: The number selected cells      */

		int horiz_origin;			/* Private: horiz origin (in pixel space)     */
		int vert_origin;			/* Private: vert origin (in pixel space)     */

		int row_label_baseline;		/* Private: baseline of row labels       */
		int cell_baseline;			/* Private: baseline of text in each cell     */

		Time last_click_time;		/* Private: Used to detect double clicks    */
		int double_click_interval;	/* Public: interval between clicks      */
		int last_column;			/* Private: Used to detect double clicks    */
		int last_row;				/* Private: Used to detect double clicks    */

		int prev_column;			/* Private: Used to compare tracking callback */
		int prev_row;				/* Private: Used to compare tracking callback */

		int current_column;			/* Private: column of the text field      */
		int current_row;			/* Private: row of the text field      */

		Widget text_field;			/* Private: the text field       */
		Widget horizontal_sb;		/* Private: the horizontal scrollbar      */
		Widget vertical_sb;			/* Private: the vertical scrollbar    */

		Widget clip_window;			/* Private: the clips           */
		Widget left_clip;
		Widget right_clip;
		Widget top_clip;
		Widget bottom_clip;
		Widget row_label_clip;
		Widget column_label_clip;

		GC grid_line_gc;			/* Private: GC for grid line           */
		GC draw_gc;					/* Private: GC for drawing cells       */
		GC label_gc;				/* Private: GC for drawing labels      */
		GC pixmap_gc;				/* Private: GC for drawing pixmap cells   */
		GC resize_bottom_shadow_gc;
		GC resize_top_shadow_gc;

		Cursor cursor;

		XbaeXftDrawCache *xft_draw_cache;	/* Private: cache for XftDraw data */
		int xft_draw_cache_len;
		XbaeXftColorCache *xft_color_cache;	/* Private: cache for XftColor data */
		int xft_color_cache_len;

		XbaeMatrixPerCellRec **per_cell;	/* Private: 2D array */

	} XbaeMatrixPart;

/*
 * Full instance record declaration
 */
	typedef struct _XbaeMatrixRec {
		CorePart core;
		CompositePart composite;
		ConstraintPart constraint;
		XmManagerPart manager;
		XbaeMatrixPart matrix;
	} XbaeMatrixRec;

/*
 * End of array indicator for converters of strings
 */
	extern char xbaeBadString;

/*
 * Macro replacements
 */
	Widget _XbaeGetShellAncestor(Widget w);

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_MatrixP_h */
