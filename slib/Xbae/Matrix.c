/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 * Copyright(c) 1995-99 Andrew Lister
 * Copyright © 1999, 2000, 2001, 2002, 2003, 2004, 2005 by the LessTif Developers.
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
 * $Id: Matrix.c,v 1.195 2005/07/31 18:35:32 tobiasoed Exp $
 */
/*
 * 2005.10.31 Modified for use in Aurora. See note in Matrix.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include <X11/StringDefs.h>
#include <X11/Xlib.h>

#include <Xm/XmP.h>

#include <Xm/TraitP.h>
#include <Xm/SpecRenderT.h>

#include <Xm/AtomMgr.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawP.h>
#include <Xm/DragIcon.h>
#include <Xm/DragC.h>

#include <Xbae/Input.h>
#include <Xbae/MatrixP.h>
#include <Xbae/Clip.h>
#include <Xbae/Converters.h>
#include <Xbae/ScrollMgr.h>
#include <Xbae/Actions.h>
#include <Xbae/Create.h>
#include <Xbae/Methods.h>
#include <Xbae/Utils.h>
#include <Xbae/Shadow.h>
#include <Xbae/Draw.h>

#include "XbaeDebug.h"

#ifdef	WIN32
#  define	EXTERNALREF	externalref __declspec(dllexport)
#else
#  define	EXTERNALREF			/* nothing */
#endif

/*
** This should have been a compile-time option.
** for backwards compatability reasons with pre 4.50 versions.
**
** Added here, but the default is the same as 4.50.2.
**
** A.J.Fountain, IST, August 2003.
*/

#ifndef   DEFAULT_USE_XBAE_INPUT
#  define   DEFAULT_USE_XBAE_INPUT    False
#endif /* DEFAULT_USE_XBAE_INPUT */

#ifndef XlibSpecificationRelease
#  define XrmPermStringToQuark XrmStringToQuark
#endif

/*
 * Translations for Matrix.
 */
static char defaultTranslations[] =
	"<Btn1Up>           :   DefaultAction()\n"
	"<Btn1Down>         :   DefaultAction() EditCell(Pointer)\n"
	"Shift<Btn2Down>    :   ResizeColumns()\n" "<Btn2Down>         :   ProcessDrag()\n"
#ifdef TRANSLATION_TEST
	"<Btn3Down>         :   CancelEdit(True)\n"
#endif
	"<Btn1Motion>       :   HandleMotion() HandleTracking()\n"
	"<Motion>           :   HandleTracking()\n"
	"<Btn4Down>         :   ScrollRows(-50)\n" "<Btn5Down>         :   ScrollRows( 50)\n";

/*
 * Default translations for XmNtextTranslations resource
 */
static char default_text_translations[] =
#ifdef TRANSLATION_TEST
	"Meta <Key>osfCancel                :   CancelEdit(True)\n"
	"Meta <Key>osfActivate              :   CommitEdit(True)\n"
	"Shift <Key>Tab                     :   TraversePrev()\n"
	"<Key>Tab                           :   TraverseNext()\n"
#else
	"Shift ~Ctrl ~Meta ~Alt <Key>Tab    :   EditCell(Left)\n"
	"~Ctrl ~Meta ~Alt <Key>Tab          :   EditCell(Right)\n"
	"Shift Ctrl ~Meta ~Alt <Key>Tab     :   TraversePrev()\n"
	"Ctrl ~Meta ~Alt <Key>Tab           :   TraverseNext()\n"
#endif
	"Ctrl <Key>osfUp                    :   EditCell(Up)\n"
	"Ctrl <Key>osfDown                  :   EditCell(Down)\n"
	"Ctrl <Key>osfLeft                  :   EditCell(Left)\n"
	"Ctrl <Key>osfRight                 :   EditCell(Right)\n"
	"<Key>osfCancel                     :   CancelEdit(False)\n"
	"<Key>osfActivate                   :   CommitEdit(False)\n"
	"~Shift ~Meta ~Alt <Key>Return      :   CommitEdit(False)\n"
	"<Key>osfPageDown                   :   PageDown()\n" "<Key>osfPageUp                     :   PageUp()\n";

static char default_dialog_text_translations[] =
#ifdef TRANSLATION_TEST
	"Meta <Key>osfCancel                :   CancelEdit(True)\n"
	"Meta <Key>osfActivate              :   CommitEdit(True)\n"
	"Shift <Key>Tab                     :   TraversePrev()\n"
	"<Key>Tab                           :   TraverseNext()\n"
#else
	"Shift ~Ctrl ~Meta ~Alt <Key>Tab    :   EditCell(Left)\n"
	"~Ctrl ~Meta ~Alt <Key>Tab          :   EditCell(Right)\n"
	"Shift Ctrl ~Meta ~Alt <Key>Tab     :   TraversePrev()\n"
	"Ctrl ~Meta ~Alt <Key>Tab           :   TraverseNext()\n"
#endif
	"Ctrl <Key>osfUp                    :   EditCell(Up)\n"
	"Ctrl <Key>osfDown                  :   EditCell(Down)\n"
	"Ctrl <Key>osfLeft                  :   EditCell(Left)\n"
	"Ctrl <Key>osfRight                 :   EditCell(Right)\n"
	"<Key>osfCancel                     :   CancelEdit(True)\n"
	"<Key>osfActivate                   :   CommitEdit(False)\n"
	"~Shift ~Meta ~Alt <Key>Return      :   CommitEdit(False)\n"
	"<Key>osfPageDown                   :   PageDown()\n" "<Key>osfPageUp                     :   PageUp()\n";

#define offset(field)	XtOffsetOf(XbaeMatrixRec, field)
static void CheckSetRenderTable (Widget, int, XrmValue*); 
	

static XtResource resources[] = {
	{
		XmNallowColumnResize, XmCAllowResize, XmRBoolean, sizeof(Boolean),
		offset(matrix.allow_column_resize), XmRImmediate, (XtPointer) False
	},{
		XmNallowRowResize, XmCAllowResize, XmRBoolean, sizeof(Boolean),
		offset(matrix.allow_row_resize), XmRImmediate, (XtPointer) False
	},{
		XmNaltRowCount, XmCAltRowCount, XmRInt, sizeof(int),
		offset(matrix.alt_row_count), XmRImmediate, (XtPointer) 1
	},{
		XmNboldLabels, XmCBoldLabels, XmRBoolean, sizeof(Boolean),
		offset(matrix.bold_labels), XmRImmediate, (XtPointer) False
	},{
		XmNbuttonLabels, XmCButtonLabels, XmRBoolean, sizeof(Boolean),
		offset(matrix.button_labels), XmRImmediate, (XtPointer) False
	},{
		XmNbuttonLabelBackground, XmCColor, XmRPixel, sizeof(Pixel),
		offset(matrix.button_label_background), XmRCallProc, (XtPointer) xbaeCopyBackground
	},{
		XmNcellRenditionTag, XmCString, XmRString, sizeof(String),
		offset(matrix.cell_rendition_tag), XmRImmediate, (XtPointer) "cells"
	},{
		XmNlabelRenditionTag, XmCString, XmRString, sizeof(String),
		offset(matrix.label_rendition_tag), XmRImmediate, (XtPointer) "labels"
	},{
		XmNcolumnLabelBackgrounds, XmCLabelColors, XmRPixelArray, sizeof(Pixel *),
		offset(matrix.column_label_backgrounds), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnLabelForegrounds, XmCLabelColors, XmRPixelArray, sizeof(Pixel *),
		offset(matrix.column_label_foregrounds), XmRImmediate, (XtPointer) NULL
	},{
		XmNrowLabelBackgrounds, XmCLabelColors, XmRPixelArray, sizeof(Pixel *),
		offset(matrix.row_label_backgrounds), XmRImmediate, (XtPointer) NULL
	},{
		XmNrowLabelForegrounds, XmCLabelColors, XmRPixelArray, sizeof(Pixel *),
		offset(matrix.row_label_foregrounds), XmRImmediate, (XtPointer) NULL
	},{
		XmNcalcCursorPosition, XmCCalcCursorPosition, XmRBoolean, sizeof(Boolean),
		offset(matrix.calc_cursor_position), XmRImmediate, (XtPointer) False
	},{
		XmNcellHighlightThickness, XmCHighlightThickness, XmRHorizontalDimension, sizeof(Dimension),
		offset(matrix.cell_highlight_thickness), XmRImmediate, (XtPointer) 2
	},{
		XmNcellMarginHeight, XmCMarginHeight, XmRVerticalDimension, sizeof(Dimension),
		offset(matrix.cell_margin_height), XmRImmediate, (XtPointer) 3
	},{
		XmNcellMarginWidth, XmCMarginWidth, XmRHorizontalDimension, sizeof(Dimension),
		offset(matrix.cell_margin_width), XmRImmediate, (XtPointer) 3
	},{
		XmNcellShadowThickness, XmCShadowThickness, XmRDimension, sizeof(Dimension),
		offset(matrix.cell_shadow_thickness), XmRImmediate, (XtPointer) 1
	},{
		XmNcellShadowType, XmCShadowType, XmRShadowType, sizeof(unsigned char),
		offset(matrix.cell_shadow_type), XmRImmediate, (XtPointer) XmSHADOW_OUT
	},{
		XmNclipWindow, XmCClipWindow, XmRWidget, sizeof(Widget),
		offset(matrix.clip_window), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnAlignments, XmCAlignments, XmRAlignmentArray, sizeof(unsigned char *),
		offset(matrix.column_alignments), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnButtonLabels, XmCButtonLabels, XmRBooleanArray, sizeof(Boolean *),
		offset(matrix.column_button_labels), XmRImmediate, (XtPointer) NULL
	},{
		XmNshowColumnArrows, XmCButtonLabels, XmRBooleanArray, sizeof(Boolean *),
		offset(matrix.show_column_arrows), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnFontBold, XmCButtonLabels, XmRBooleanArray, sizeof(Boolean *),
		offset(matrix.column_font_bold), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnLabelAlignments, XmCAlignments, XmRAlignmentArray, sizeof(unsigned char *),
		offset(matrix.column_label_alignments), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnLabelColor, XmCColor, XmRPixel, sizeof(Pixel),
		offset(matrix.column_label_color), XmRCallProc, (XtPointer) xbaeCopyForeground
	},{
		XmNcolumnLabels, XmCLabels, XmRStringArray, sizeof(String *),
		offset(matrix.column_labels), XmRImmediate, (XtPointer) NULL
	},{
		XmNxmColumnLabels, XmCXmLabels, XmRXmStringTable, sizeof(XmString *),
		offset(matrix.xmcolumn_labels), XmRImmediate, (XtPointer) NULL
	},{
		XmNxmRowLabels, XmCXmLabels, XmRXmStringTable, sizeof(XmString *),
		offset(matrix.xmrow_labels), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnMaxLengths, XmCColumnMaxLengths, XmRMaxLengthArray, sizeof(int *),
		offset(matrix.column_max_lengths), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnShadowTypes, XmCShadowTypes, XmRShadowTypeArray, sizeof(unsigned char *),
		offset(matrix.column_shadow_types), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnUserData, XmCUserDatas, XmRUserDataArray, sizeof(XtPointer *),
		offset(matrix.column_user_data), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnWidths, XmCColumnWidths, XmRWidthArray, sizeof(short *),
		offset(matrix.column_widths), XmRImmediate, (XtPointer) NULL
	},{
		XmNcolumnWidthInPixels, XmCColumnWidthInPixels, XmRBoolean, sizeof(Boolean),
		offset(matrix.column_width_in_pixels), XmRImmediate, (XtPointer) False
	},{
		XmNrowHeightInPixels, XmCRowHeightInPixels, XmRBoolean, sizeof(Boolean),
		offset(matrix.row_height_in_pixels), XmRImmediate, (XtPointer) True
	},{
		XmNcolumns, XmCColumns, XmRInt, sizeof(int),
		offset(matrix.columns), XmRImmediate, (XtPointer) 1
	},{
		XmNdefaultActionCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.default_action_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNdoubleClickInterval, XmCDoubleClickInterval, XmRInt, sizeof(int),
		offset(matrix.double_click_interval), XmRCallProc, (XtPointer) xbaeCopyDoubleClick
	},{
		XmNdrawCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.draw_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNenterCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.enter_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNtrackCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.track_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNevenRowBackground, XmCBackground, XmRPixel, sizeof(Pixel),
		offset(matrix.even_row_background), XmRCallProc, (XtPointer) xbaeCopyBackground
	},{
		XmNfill, XmCFill, XmRBoolean, sizeof(Boolean),
		offset(matrix.fill), XmRImmediate, (XtPointer) False
	},{
		XmNvertFill, XmCVertFill, XmRBoolean, sizeof(Boolean),
		offset(matrix.vert_fill), XmRImmediate, (XtPointer) False
	},{
		XmNhorzFill, XmCHorzFill, XmRBoolean, sizeof(Boolean),
		offset(matrix.horz_fill), XmRImmediate, (XtPointer) False
	},{
		XmNfixedColumns, XmCFixedColumns, XmRDimension, sizeof(Dimension),
		offset(matrix.fixed_columns), XmRImmediate, (XtPointer) 0
	},{
		XmNfixedRows, XmCFixedRows, XmRDimension, sizeof(Dimension),
		offset(matrix.fixed_rows), XmRImmediate, (XtPointer) 0
	},{
		XmNlabelFont, XmCFontList, XmRFontList, sizeof(XmFontList),
		offset(matrix.label_font_list), XmRString, (XtPointer) NULL
	},{
		"pri.vate", "Pri.vate", XmRBoolean, sizeof(Boolean),
		offset(matrix.check_set_render_table), XmRImmediate, (XtPointer) False
	},{
		XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
		offset(matrix.render_table), XmRCallProc, (XtPointer) CheckSetRenderTable
	},{
		XmNrenderTable, XmCRenderTable, XmRRenderTable, sizeof(XmRenderTable),
		offset(matrix.render_table), XmRCallProc, (XtPointer) CheckSetRenderTable
	},{
		XmNgridLineColor, XmCColor, XmRPixel, sizeof(Pixel),
		offset(matrix.grid_line_color), XmRCallProc, (XtPointer) xbaeCopyForeground
	},{
		XmNgridType, XmCGridType, XmRGridType,
		sizeof(unsigned char), offset(matrix.grid_type), XmRImmediate, (XtPointer) XmGRID_CELL_LINE
	},{
		XmNhorizontalScrollBar, XmCHorizontalScrollBar, XmRWidget, sizeof(Widget),
		offset(matrix.horizontal_sb), XmRImmediate, (XtPointer) NULL
	},{
		XmNhorizontalScrollBarDisplayPolicy, XmCMatrixScrollBarDisplayPolicy,
		XmRMatrixScrollBarDisplayPolicy, sizeof(unsigned char),
		offset(matrix.hsb_display_policy), XmRImmediate, (XtPointer) XmDISPLAY_AS_NEEDED
	},{
		XmNlabelActivateCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.label_activate_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNleaveCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.leave_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNmodifyVerifyCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.modify_verify_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNoddRowBackground, XmCBackground, XmRPixel, sizeof(Pixel),
		offset(matrix.odd_row_background), XmRCallProc, (XtPointer) xbaeCopyBackground
	},{
		XmNprocessDragCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.process_drag_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNresizeCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.resize_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNresizeColumnCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.resize_column_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNresizeRowCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.resize_row_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNreverseSelect, XmCReverseSelect, XmRBoolean, sizeof(Boolean),
		offset(matrix.reverse_select), XmRImmediate, (XtPointer) False
	},{
		XmNrowButtonLabels, XmCButtonLabels, XmRBooleanArray, sizeof(Boolean *),
		offset(matrix.row_button_labels), XmRImmediate, (XtPointer) NULL
	},{
		XmNrowHeights, XmCColumnWidths, XmRWidthArray, sizeof(short *),
		offset(matrix.row_heights), XmRImmediate, (XtPointer) NULL
	},{
		XmNrowLabelAlignment, XmCAlignment, XmRAlignment, sizeof(unsigned char),
		offset(matrix.row_label_alignment), XmRImmediate, (XtPointer) XmALIGNMENT_END
	},{
		XmNrowLabelColor, XmCColor, XmRPixel, sizeof(Pixel),
		offset(matrix.row_label_color), XmRCallProc, (XtPointer) xbaeCopyForeground
	},{
		XmNrowLabelWidth, XmCRowLabelWidth, XmRShort, sizeof(short),
		offset(matrix.row_label_width), XmRImmediate, (XtPointer) 0
	},{
		XmNrowLabels, XmCLabels, XmRStringArray, sizeof(String *),
		offset(matrix.row_labels), XmRImmediate, (XtPointer) NULL
	},{
		XmNrowShadowTypes, XmCShadowTypes, XmRShadowTypeArray, sizeof(unsigned char *),
		offset(matrix.row_shadow_types), XmRImmediate, (XtPointer) NULL
	},{
		XmNrowUserData, XmCUserDatas, XmRUserDataArray, sizeof(XtPointer *),
		offset(matrix.row_user_data), XmRImmediate, (XtPointer) NULL
	},{
		XmNrows, XmCRows, XmRInt, sizeof(int),
		offset(matrix.rows), XmRImmediate, (XtPointer) 1
	},{
		XmNscrollBackground, XmCColor, XmRPixel, sizeof(Pixel),
	 offset(matrix.scroll_background), XmRCallProc,
	 (XtPointer) xbaeCopyBackground
	},{
		XmNscrollBarPlacement, XmCScrollBarPlacement, XmRScrollBarPlacement, sizeof(unsigned char),
		offset(matrix.scrollbar_placement), XmRImmediate, (XtPointer) XmBOTTOM_RIGHT
	},{
		XmNselectCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.select_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNselectScrollVisible, XmCSelectScrollVisible, XmRBoolean, sizeof(Boolean),
		offset(matrix.scroll_select), XmRImmediate, (XtPointer) True
	},{
		XmNselectedBackground, XmCColor, XmRPixel, sizeof(Pixel),
		offset(matrix.selected_background), XmRCallProc, (XtPointer) xbaeCopyForeground
	},{
		XmNselectedForeground, XmCColor, XmRPixel, sizeof(Pixel),
		offset(matrix.selected_foreground), XmRCallProc, (XtPointer) xbaeCopyBackground
	},{
		XmNselectionPolicy, XmCSelectionPolicy, XmRSelectionPolicy, sizeof(unsigned char),
		offset(matrix.selection_policy), XmRImmediate, (XtPointer) XmSINGLE_SELECT
	},{
		XmNshadowThickness, XmCShadowThickness, XmRHorizontalDimension, sizeof(Dimension),
		XtOffsetOf(XmManagerRec, manager.shadow_thickness), XmRImmediate, (XtPointer) 2
	},{
		XmNshadowType, XmCShadowType, XmRShadowType, sizeof(unsigned char),
		offset(matrix.shadow_type), XmRImmediate, (XtPointer) XmSHADOW_IN
	},{
		XmNshowArrows, XmCShowArrows, XmRBoolean, sizeof(Boolean),
		offset(matrix.show_arrows), XmRImmediate, (XtPointer) False
	},{
		XmNspace, XmCSpace, XmRHorizontalDimension, sizeof(Dimension),
		offset(matrix.space), XmRImmediate, (XtPointer) 4
	},{
		XmNtextBackground, XmCTextBackground, XmRPixel, sizeof(Pixel),
		offset(matrix.text_background), XmRImmediate, (XtPointer) XmUNSPECIFIED_PIXEL
	},{
		XmNtextBackgroundIsCell, XmCTextBackgroundIsCell, XmRBoolean, sizeof(Boolean),
		offset(matrix.text_background_is_cell), XmRImmediate, (XtPointer) False
	},{
		XmNtextField, XmCTextField, XmRWidget, sizeof(Widget),
		offset(matrix.text_field), XmRImmediate, (XtPointer) NULL
	},{
		XmNtextShadowThickness, XmCTextShadowThickness, XmRDimension, sizeof(Dimension),
		offset(matrix.text_shadow_thickness), XmRImmediate, (XtPointer) 0
	},{
		XmNtextTranslations, XmCTranslations, XmRTranslationTable, sizeof(XtTranslations),
		offset(matrix.text_translations), XmRString, (XtPointer) NULL
	},{
		XmNnonFixedDetachedTop, XmCNonFixedDetachedTop, XmRBoolean, sizeof(Boolean),
		offset(matrix.non_fixed_detached_top), XmRImmediate, (XtPointer) False
	},{
		XmNnonFixedDetachedLeft, XmCNonFixedDetachedLeft, XmRBoolean, sizeof(Boolean),
		offset(matrix.non_fixed_detached_left), XmRImmediate, (XtPointer) False
	},{
		XmNtrailingAttachedBottom, XmCTrailingAttachedBottom, XmRBoolean, sizeof(Boolean),
		offset(matrix.trailing_attached_bottom), XmRImmediate, (XtPointer) False
	},{
		XmNtrailingAttachedRight, XmCTrailingAttachedRight, XmRBoolean, sizeof(Boolean),
		offset(matrix.trailing_attached_right), XmRImmediate, (XtPointer) False
	},{
		XmNtrailingFixedColumns, XmCTrailingFixedColumns, XmRDimension, sizeof(Dimension),
		offset(matrix.trailing_fixed_columns), XmRImmediate, (XtPointer) 0
	},{
		XmNtrailingFixedRows, XmCTrailingFixedRows, XmRDimension, sizeof(Dimension),
		offset(matrix.trailing_fixed_rows), XmRImmediate, (XtPointer) 0
	},{
		XmNtraverseCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.traverse_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNtraverseFixedCells, XmCTraverseFixedCells, XmRBoolean, sizeof(Boolean),
		offset(matrix.traverse_fixed), XmRImmediate, (XtPointer) False
	},{
		XmNunderlinePosition, XmCUnderlinePosition, XmRPosition, sizeof(Position),
		offset(matrix.underline_position), XmRImmediate, (XtPointer) 1
	},{
		XmNunderlineWidth, XmCUnderlineWidth, XmRDimension, sizeof(Dimension),
		offset(matrix.underline_width), XmRImmediate, (XtPointer) 1
	},{
		XmNvalueChangedCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.value_changed_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNverticalScrollBar, XmCVerticalScrollBar, XmRWidget, sizeof(Widget),
		offset(matrix.vertical_sb), XmRImmediate, (XtPointer) NULL
	},{
		XmNverticalScrollBarDisplayPolicy, XmCMatrixScrollBarDisplayPolicy,
		XmRMatrixScrollBarDisplayPolicy, sizeof(unsigned char),
		offset(matrix.vsb_display_policy), XmRImmediate, (XtPointer) XmDISPLAY_AS_NEEDED
	},{
		XmNvisibleColumns, XmCVisibleColumns, XmRDimension, sizeof(Dimension),
		offset(matrix.visible_columns), XmRImmediate, (XtPointer) 0
	},{
		XmNvisibleRows, XmCVisibleRows, XmRDimension, sizeof(Dimension),
		offset(matrix.visible_rows), XmRImmediate, (XtPointer) 0
	},{
		XmNwriteCellCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
		offset(matrix.write_cell_callback), XmRCallback, (XtPointer) NULL
	},{
		XmNdesiredHeight, XmCDesiredHeight, XmRDimension, sizeof(Dimension),
		offset(matrix.desired_height), XmRImmediate, (XtPointer) 0
	},{
		XmNdesiredWidth, XmCDesiredWidth, XmRDimension, sizeof(Dimension),
		offset(matrix.desired_width), XmRImmediate, (XtPointer) 0
	},{
		XmNuseXbaeInput, XmCUseXbaeInput, XmRBoolean, sizeof(Boolean),
		offset(matrix.useXbaeInput), XmRImmediate, (XtPointer) DEFAULT_USE_XBAE_INPUT
	},{
		XmNmultiLineCell, XmCMultiLineCell, XmRBoolean, sizeof(Boolean),
		offset(matrix.multi_line_cell), XmRImmediate, (XtPointer) False
	},{
		XmNwrapType, XmCWrapType, XmRWrapType, sizeof(unsigned char),
		offset(matrix.wrap_type), XmRImmediate, (XtPointer) XbaeWrapNone
	}
};

static XmSyntheticResource syn_resources[] = {
	{
		XmNcellHighlightThickness, sizeof(Dimension), offset(matrix.cell_highlight_thickness),
		XmeFromHorizontalPixels, XmeToHorizontalPixels
	},{
		XmNcellMarginHeight, sizeof(Dimension), offset(matrix.cell_margin_height),
		XmeFromVerticalPixels, XmeToVerticalPixels
	},{
		XmNcellMarginWidth, sizeof(Dimension), offset(matrix.cell_margin_width),
		XmeFromHorizontalPixels, XmeToHorizontalPixels
	},{
		XmNcellShadowThickness, sizeof(Dimension), offset(matrix.cell_shadow_thickness),
		XmeFromHorizontalPixels, XmeToHorizontalPixels
	},{
		XmNspace, sizeof(Dimension), offset(matrix.space),
		XmeFromHorizontalPixels, XmeToHorizontalPixels
	}
};

/*
 * Declaration of methods
 */
static void ClassInitialize(void);
static void xbaeRegisterConverters(void);
static void ClassPartInitialize(WidgetClass);
static void Initialize(Widget, Widget, ArgList, Cardinal *);
static void Realize(XbaeMatrixWidget, XtValueMask *, XSetWindowAttributes *);
static void InsertChild(Widget);
static void Redisplay(Widget, XEvent *, Region);
static Boolean SetValues(XbaeMatrixWidget, XbaeMatrixWidget, XbaeMatrixWidget, ArgList, Cardinal *);
static void SetValuesAlmost(XbaeMatrixWidget, XbaeMatrixWidget, XtWidgetGeometry *, XtWidgetGeometry *);
static void Destroy(XbaeMatrixWidget);
static XtGeometryResult GeometryManager(Widget, XtWidgetGeometry *, XtWidgetGeometry *);
static XtGeometryResult QueryGeometry(XbaeMatrixWidget, XtWidgetGeometry *, XtWidgetGeometry *);
static Boolean SetValuesHook(Widget w, ArgList args, Cardinal * nargs);
static void GetValuesHook(Widget w, ArgList args, Cardinal * nargs);

/*
 * Redraw function for clip widget
 */
static void ClipRedisplay(Widget, XRectangle *, XEvent *, Region);

/*
 * Private functions unique to Matrix
 */
static void ResizePerCell(XbaeMatrixWidget, XbaeMatrixWidget);

/*
 * Matrix actions
 */
static XtActionsRec actions[] = {
	{"EditCell", xbaeEditCellACT},
	{"CancelEdit", xbaeCancelEditACT},
	{"DefaultAction", xbaeDefaultActionACT},
	{"CommitEdit", xbaeCommitEditACT},
	{"ResizeColumns", xbaeResizeColumnsACT},
	{"SelectCell", xbaeSelectCellACT},
	{"TraverseNext", xbaeTraverseNextACT},
	{"TraversePrev", xbaeTraversePrevACT},
	{"ProcessDrag", xbaeProcessDragACT},
	{"HandleMotion", xbaeHandleMotionACT},
	{"HandleTracking", xbaeHandleTrackingACT},
	{"PageDown", xbaePageDownACT},
	{"PageUp", xbaePageUpACT},
	{"ScrollRows", xbaeScrollRowsACT},
	{"ScrollColumns", xbaeScrollColumnsACT}
};

/* *INDENT-OFF* */
static XmBaseClassExtRec BaseClassExtRec = {
        NULL,                       /* next_extension         */
        NULLQUARK,                  /* record_type            */
        XmBaseClassExtVersion,      /* version                */
        sizeof(XmBaseClassExtRec),  /* record_size            */
        NULL,                       /* InitializePrehook      */
        NULL,                       /* SetValuesPrehook       */
        NULL,                       /* InitializePosthook     */
        NULL,                       /* SetValuesPosthook      */
        NULL,                       /* secondaryObjectClass   */
        NULL,                       /* secondaryCreate        */
        NULL,                       /* getSecRes data         */
        {0},                        /* fastSubclass flags     */
        NULL,                       /* get_values_prehook     */
        NULL,                       /* get_values_posthook    */
        NULL,                       /* classPartInitPrehook   */
        NULL,                       /* classPartInitPosthook  */
        NULL,                       /* ext_resources          */
        NULL,                       /* compiled_ext_resources */
        0,                          /* num_ext_resources      */
        FALSE,                      /* use_sub_resources      */
        XmInheritWidgetNavigable,   /* widgetNavigable        */
        XmInheritFocusChange,       /* focusChange            */
        NULL                        /* wrapperdata            */
};

XbaeMatrixClassRec xbaeMatrixClassRec = {
    {
        /* core_class fields */
        (WidgetClass) & xmManagerClassRec,      /* superclass            */
        "XbaeMatrix",                           /* class_name            */
        sizeof(XbaeMatrixRec),                  /* widget_size           */
        ClassInitialize,                        /* class_initialize      */
        ClassPartInitialize,                    /* class_part_initialize */
        False,                                  /* class_inited          */
        Initialize,                             /* initialize            */
        NULL,                                   /* initialize_hook       */
        (XtRealizeProc) Realize,                /* realize               */
        actions,                                /* actions               */
        XtNumber(actions),                      /* num_actions           */
        resources,                              /* resources             */
        XtNumber(resources),                    /* num_resources         */
        NULLQUARK,                              /* xrm_class             */
        True,                                   /* compress_motion       */
        XtExposeCompressMultiple 
        | XtExposeGraphicsExpose 
        | XtExposeNoExpose,                     /* compress_exposure     */
        True,                                   /* compress_enterleave   */
        False,                                  /* visible_interest      */
        (XtWidgetProc) Destroy,                 /* destroy               */
        (XtWidgetProc) xbaeResize,              /* resize                */
        Redisplay,                              /* expose                */
        (XtSetValuesFunc) SetValues,            /* set_values            */
        SetValuesHook,                          /* set_values_hook       */
        (XtAlmostProc) SetValuesAlmost,         /* set_values_almost     */
        GetValuesHook,                          /* get_values_hook       */
        XtInheritAcceptFocus,                   /* accept_focus          */
        XtVersionDontCheck,                     /* version               */
        NULL,                                   /* callback_private      */
        defaultTranslations,                    /* tm_table              */
        (XtGeometryHandler) QueryGeometry,      /* query_geometry        */
        NULL,                                   /* display_accelerator   */
        (XtPointer) & BaseClassExtRec           /* extension             */
    },{
        /* composite_class fields */
        GeometryManager,                        /* geometry_manager      */
        NULL,                                   /* change_managed        */
        InsertChild,                            /* insert_child          */
        XtInheritDeleteChild,                   /* delete_child          */
        NULL,                                   /* extension             */
    },{
        /* constraint_class fields */
        NULL,                                   /* resources             */
        0,                                      /* num_resources         */
        0,                                      /* constraint_size       */
        NULL,                                   /* initialize            */
        NULL,                                   /* destroy               */
        NULL,                                   /* set_values            */
        NULL                                    /* extension             */
    },{
        /* manager_class fields */
        XtInheritTranslations,                  /* translations          */
        syn_resources,                          /* syn_resources         */
        XtNumber(syn_resources),                /* num_syn_resources     */
        NULL,                                   /* syn_constraint_resources     */
        0,                                      /* num_syn_constraint_resources */
        XmInheritParentProcess,                 /* parent_process        */
        NULL                                    /* extension             */
    },{
        /* matrix_class fields */
        xbaeSetCell,                            /* set_cell              */
        xbaeGetCell,                            /* get_cell              */
        xbaeEditCell,                           /* edit_cell             */
        xbaeSelectCell,                         /* select_cell           */
        xbaeShowColumnArrows,                   /* show_column_arrows    */
        xbaeSelectRow,                          /* select_row            */
        xbaeSelectColumn,                       /* select_column         */
        xbaeDeselectAll,                        /* deselect_all          */
        xbaeSelectAll,                          /* select_all            */
        xbaeDeselectCell,                       /* deselect_cell         */
        xbaeDeselectRow,                        /* deselect_row          */
        xbaeDeselectColumn,                     /* deselect_column       */
        xbaeCommitEdit,                         /* commit_edit           */
        xbaeCancelEdit,                         /* cancel_edit           */
        xbaeAddRows,                            /* add_rows              */
        xbaeAddVarRows,                         /* add_var_rows          */
        xbaeDeleteRows,                         /* delete_rows           */
        xbaeAddColumns,                         /* add_columns           */
        xbaeDeleteColumns,                      /* delete_columns        */
        xbaeSetRowColors,                       /* set_row_colors        */
        xbaeSetColumnColors,                    /* set_column_colors     */
        xbaeSetCellColor,                       /* set_cell_color        */
        NULL,                                   /* extension             */
    }
};

EXTERNALREF WidgetClass xbaeMatrixWidgetClass = (WidgetClass) & xbaeMatrixClassRec;

static XtConvertArgRec convertArg[] = { 
    {XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.screen), sizeof(Screen *)},
    {XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.colormap), sizeof(Colormap)}
};
/* *INDENT-ON* */

static void xbaeRegisterConverters(void)
{
	/*
	 * String to StringArray is used for XmNrowLabels and XmNcolumnLabels
	 * We make a private copy of this table
	 */
	XtSetTypeConverter(XmRString, XmRStringArray, XbaeCvtStringToStringArray, NULL, 0,
					   XtCacheAll | XtCacheRefCount, XbaeStringArrayDestructor);

	/*
	 * String to String2DArray is used for XmNcells resource
	 * We make a private copy of this table
	 */
	XtSetTypeConverter(XmRString, XmRCellTable, XbaeCvtStringToCellTable, NULL, 0,
					   XtCacheNone, XbaeStringCellDestructor);

	/*
	 * String to ShortArray is used for XmNcolumnWidths resource.
	 * We make a private copy of this table
	 */
	XtSetTypeConverter(XmRString, XmRWidthArray, XbaeCvtStringToWidthArray, NULL, 0,
					   XtCacheNone, XbaeWidthArrayDestructor);

	/*
	 * String to IntArray is used for XmNcolumnMaxLengths resource.
	 * We make a private copy of this table
	 */
	XtSetTypeConverter(XmRString, XmRMaxLengthArray, XbaeCvtStringToMaxLengthArray, NULL, 0,
					   XtCacheAll | XtCacheRefCount, XbaeMaxLengthArrayDestructor);

	/*
	 * String to PixelArray is used for buttonLabel background and foreground
	 */
	XtSetTypeConverter(XmRString, XmRPixelArray, XbaeCvtStringToPixelArray,
					   convertArg, XtNumber(convertArg), XtCacheNone, XbaePixelArrayDestructor);

	/*
	 * String to PixelTable is used for XmNcolors
	 * and XmNcellBackgrounds resources.
	 */
	XtSetTypeConverter(XmRString, XmRPixelTable, XbaeCvtStringToPixelTable,
					   convertArg, XtNumber(convertArg), XtCacheNone, XbaePixelTableDestructor);

	/*
	 * String to BooleanArray is used for XmNcolumnButtonLabels, XmNshowColumnArrows,
	 * XmNcolumnFontBold, and XmNrowButtonLabels resources.
	 */
	XtSetTypeConverter(XmRString, XmRBooleanArray, XbaeCvtStringToBooleanArray, NULL, 0,
					   XtCacheAll | XtCacheRefCount, XbaeBooleanArrayDestructor);

	/*
	 * String to AlignmentArray is used for XmNcolumnAlignments
	 * and XmNcolumnLabelAlignments resources.
	 */
	XtSetTypeConverter(XmRString, XmRAlignmentArray, XbaeCvtStringToAlignmentArray, NULL, 0,
					   XtCacheAll | XtCacheRefCount, XbaeAlignmentArrayDestructor);

	/*
	 * String to ShadowtypesArray.
	 */
	XtSetTypeConverter(XmRString, XmRShadowTypeArray, XbaeCvtStringToShadowTypeArray, NULL, 0,
					   XtCacheAll | XtCacheRefCount, XbaeShadowTypeArrayDestructor);

	/*
	 * String to grid type is used for XmNgridType
	 */
	XtSetTypeConverter(XmRString, XmRGridType, XbaeCvtStringToGridType, NULL, 0, XtCacheAll, NULL);

	/*
	 * String to wrap type is used for XmNwrapType
	 */
	XtSetTypeConverter(XmRString, XmRWrapType, XbaeCvtStringToWrapType, NULL, 0, XtCacheAll, NULL);

	/*
	 * String to matrix display policy is used for
	 * XmN{vertical,horizontal}ScrollBarDisplayPolicy
	 */
	XtSetTypeConverter(XmRString, XmRMatrixScrollBarDisplayPolicy,
					   XbaeCvtStringToMatrixScrollBarDisplayPolicy,
					   NULL, 0, XtCacheAll, NULL);
}

static void ClassInitialize(void)
{
	xbaeRegisterConverters();
}

static void ClassPartInitialize(WidgetClass wc)
{
	XbaeMatrixWidgetClass mwc = (XbaeMatrixWidgetClass) wc;
	XbaeMatrixWidgetClass super = (XbaeMatrixWidgetClass) mwc->core_class.superclass;

	/*
	 * Allow subclasses to inherit new Matrix methods
	 */
	if(mwc->matrix_class.set_cell == XbaeInheritSetCell)
		mwc->matrix_class.set_cell = super->matrix_class.set_cell;
	if(mwc->matrix_class.get_cell == XbaeInheritGetCell)
		mwc->matrix_class.get_cell = super->matrix_class.get_cell;
	if(mwc->matrix_class.edit_cell == XbaeInheritEditCell)
		mwc->matrix_class.edit_cell = super->matrix_class.edit_cell;
	if(mwc->matrix_class.set_show_column_arrows == XbaeInheritShowColumnArrows)
		mwc->matrix_class.set_show_column_arrows = super->matrix_class.set_show_column_arrows;
	if(mwc->matrix_class.select_cell == XbaeInheritSelectCell)
		mwc->matrix_class.select_cell = super->matrix_class.select_cell;
	if(mwc->matrix_class.select_row == XbaeInheritSelectRow)
		mwc->matrix_class.select_row = super->matrix_class.select_row;
	if(mwc->matrix_class.select_column == XbaeInheritSelectColumn)
		mwc->matrix_class.select_column = super->matrix_class.select_column;
	if(mwc->matrix_class.deselect_all == XbaeInheritDeselectAll)
		mwc->matrix_class.deselect_all = super->matrix_class.deselect_all;
	if(mwc->matrix_class.select_all == XbaeInheritSelectAll)
		mwc->matrix_class.select_all = super->matrix_class.select_all;
	if(mwc->matrix_class.deselect_cell == XbaeInheritDeselectCell)
		mwc->matrix_class.deselect_cell = super->matrix_class.deselect_cell;
	if(mwc->matrix_class.deselect_row == XbaeInheritDeselectRow)
		mwc->matrix_class.deselect_row = super->matrix_class.deselect_row;
	if(mwc->matrix_class.deselect_column == XbaeInheritDeselectColumn)
		mwc->matrix_class.deselect_column = super->matrix_class.deselect_column;
	if(mwc->matrix_class.commit_edit == XbaeInheritCommitEdit)
		mwc->matrix_class.commit_edit = super->matrix_class.commit_edit;
	if(mwc->matrix_class.cancel_edit == XbaeInheritCancelEdit)
		mwc->matrix_class.cancel_edit = super->matrix_class.cancel_edit;
	if(mwc->matrix_class.add_rows == XbaeInheritAddRows)
		mwc->matrix_class.add_rows = super->matrix_class.add_rows;
	if(mwc->matrix_class.add_var_rows == XbaeInheritAddVarRows)
		mwc->matrix_class.add_var_rows = super->matrix_class.add_var_rows;
	if(mwc->matrix_class.delete_rows == XbaeInheritDeleteRows)
		mwc->matrix_class.delete_rows = super->matrix_class.delete_rows;
	if(mwc->matrix_class.add_columns == XbaeInheritAddColumns)
		mwc->matrix_class.add_columns = super->matrix_class.add_columns;
	if(mwc->matrix_class.delete_columns == XbaeInheritDeleteColumns)
		mwc->matrix_class.delete_columns = super->matrix_class.delete_columns;
	if(mwc->matrix_class.set_row_colors == XbaeInheritSetRowColors)
		mwc->matrix_class.set_row_colors = super->matrix_class.set_row_colors;
	if(mwc->matrix_class.set_column_colors == XbaeInheritSetColumnColors)
		mwc->matrix_class.set_column_colors = super->matrix_class.set_column_colors;
	if(mwc->matrix_class.set_cell_color == XbaeInheritSetCellColor)
		mwc->matrix_class.set_cell_color = super->matrix_class.set_cell_color;
}


/* used/referenced only #ifdef NEED_24BIT_VISUAL */
Widget _XbaeGetShellAncestor(Widget w)
{
	Widget sh;

	for(sh = w; !XtIsShell(sh); sh = XtParent(sh));
	return sh;
}


/*
 * Callbacks for our scrollbars.
 */
static XtCallbackRec VSCallback[] = {
	{(XtCallbackProc) xbaeScrollVertCB, (XtPointer) NULL},
	{(XtCallbackProc) NULL, NULL}
};

static XtCallbackRec HSCallback[] = {
	{(XtCallbackProc) xbaeScrollHorizCB, (XtPointer) NULL},
	{(XtCallbackProc) NULL, NULL}
};

/*
 * This is all to initialize resources that are no more
 */
typedef struct {
	char ***cells;
	Pixel **colors;
	Pixel **background;
	int left_column;
	int top_row;
} subr_t;

static XtResource subresources[] = {
	{
	 XmNcells,
	 XmCCells,
	 XmRCellTable,
	 sizeof(XtPointer),
	 XtOffsetOf(subr_t, cells),
	 XmRImmediate,
	 NULL
	},{
	 XmNcolors,
	 XmCColors,
	 XmRPixelTable,
	 sizeof(XtPointer),
	 XtOffsetOf(subr_t, colors),
	 XmRImmediate,
	 NULL
	},{
	 XmNcellBackgrounds,
	 XmCColors,
	 XmRPixelTable,
	 sizeof(XtPointer),
	 XtOffsetOf(subr_t, background),
	 XmRImmediate,
	 NULL
	},{
	 XmNleftColumn,
	 XmCLeftColumn,
	 XmRInt,
	 sizeof(int),
	 XtOffsetOf(subr_t, left_column),
	 XmRImmediate,
	 NULL
	},{
	 XmNtopRow,
	 XmCTopRow,
	 XmRInt,
	 sizeof(int),
	 XtOffsetOf(subr_t, top_row),
	 XmRImmediate,
	 NULL
	}
};

#include <Xm/DialogS.h>

/* ARGSUSED */
static void Initialize(Widget request, Widget nw, ArgList args, Cardinal * num_args)
{
	XbaeMatrixWidget new = (XbaeMatrixWidget) nw;
	Dimension marginHeight;
	subr_t base;
	Arg al[5];
	Cardinal ac;

	DEBUGOUT(_XbaeDebug(__FILE__, nw, "Initialize args:\n"));
	DEBUGOUT(_XbaeDebugPrintArgList(__FILE__, nw, args, *num_args, False));

	/* To clean up if our display connection disappears on us */
	xbaeRegisterDisplay((Widget) new);

	/*
	 * Initialize redisplay counters
	 */
	new->matrix.disable_redisplay = 0;

	new->matrix.per_cell = NULL;
	new->matrix.num_selected_cells = 0;
	new->matrix.last_click_time = (Time) 0;
	new->matrix.cursor = (Cursor) NULL;

	/*
	 * We can only create/modify GCs after we have been realized
	 */
	new->matrix.draw_gc = 0;
	new->matrix.pixmap_gc = 0;
	new->matrix.label_gc = 0;
	new->matrix.grid_line_gc = 0;
	new->matrix.resize_top_shadow_gc = 0;
	new->matrix.resize_bottom_shadow_gc = 0;

	/*
	 ** Private State Initialization, missing from 4.50.2
	 **
	 ** A.J.Fountain, IST.
	 */
	new->matrix.current_text_qtag = NULLQUARK;
	new->matrix.current_draw_qtag = NULLQUARK;
	new->matrix.cell_font.id = 0;
	new->matrix.cell_font.qtag = NULLQUARK;
	new->matrix.label_font.id = 0;
	new->matrix.label_font.qtag = NULLQUARK;

	/*
	 * No cell ever had the focus
	 */
	new->matrix.current_row = -1;
	new->matrix.current_column = -1;
	new->matrix.text_child_is_mapped = False;

	/*
	 * No cell has ever been clicked
	 */
	new->matrix.last_row = -1;
	new->matrix.last_column = -1;

	/*
	 * Check rows/cols set by resources for consistency/validity
	 */
	if(new->matrix.rows < 0 || new->matrix.columns < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "initialize", "badSize",
						"XbaeMatrix",
						"XbaeMatrix: Number of rows or columns is less than zero",
						(String *) NULL, (Cardinal *) NULL);
		if(new->matrix.rows < 0)
			new->matrix.rows = 0;
		if(new->matrix.columns < 0)
			new->matrix.columns = 0;
	}

	/*
	 * We can't have more fixed rows/columns than there are rows/columns
	 */
	if(new->matrix.fixed_rows + new->matrix.trailing_fixed_rows > new->matrix.rows)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "initialize",
						"tooManyFixed", "XbaeMatrix",
						"XbaeMatrix: There can't be more fixed rows than there are rows", NULL, 0);
		new->matrix.fixed_rows = 0;
		new->matrix.trailing_fixed_rows = 0;
	}
	if(new->matrix.fixed_columns + new->matrix.trailing_fixed_columns > new->matrix.columns)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "initialize",
						"tooManyFixed", "XbaeMatrix",
						"XbaeMatrix: There can't be more fixed columns than there are columns", NULL, 0);
		new->matrix.fixed_columns = 0;
		new->matrix.trailing_fixed_columns = 0;
	}

	/*
	 * Warn if a deprecated grid_type was specified
	 */
	if(new->matrix.grid_type >= XmGRID_LINE)
		/* Deprecated types. To be removed in next version. */
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "cvtStringToGridType",
						"deprecatedType", "XbaeMatrix",
						"Value for GridType is deprecated and will "
						"be removed in next release", NULL, NULL);

	/*
	 * Initialize the cell_font and label_font strucures.
	 */
	if(!new->matrix.render_table)
		new->matrix.render_table = XmeGetDefaultRenderTable((Widget) new, XmLABEL_RENDER_TABLE);
	new->matrix.render_table = XmRenderTableCopy(new->matrix.render_table, NULL, 0);
	/*
	 * If the legacy XmNlabelFont resource was specified merge it in with the render table
	 * tagged with whatever is used for the label font, but only if the tag does not exist
	 * in the render table already. This is done only on create and is only for backwards
	 * compatability. It is depreciated.
	 */
	if(new->matrix.label_font_list)
	{
		XmRendition rendition = XmRenderTableGetRendition(new->matrix.render_table, new->matrix.label_rendition_tag);
		if(rendition)
		{
			XmRenditionFree(rendition);
		}
		else
		{
			rendition = XmRenderTableGetRendition(new->matrix.label_font_list, _MOTIF_DEFAULT_LOCALE);
			if(!rendition) rendition = XmRenderTableGetRendition(new->matrix.label_font_list, XmFONTLIST_DEFAULT_TAG);
			if(rendition)
			{
				Arg args[1];
				XtSetArg(args[0], XmNtag, new->matrix.label_rendition_tag);
				XmRenditionUpdate(rendition, args, 1);
				new->matrix.render_table =
					XmRenderTableAddRenditions(new->matrix.render_table, &rendition, 1, XmMERGE_NEW); 
				XmRenditionFree(rendition);
			}
		}
		XmFontListFree(new->matrix.label_font_list);
		new->matrix.label_font_list = NULL;
	}

	xbaeInitFonts(new);

	/*
	 * If column_widths or row_heights weren't specified use a default value.
	 */
	if(new->matrix.columns)
	{
		if(new->matrix.column_widths == NULL)
		{
			int i;

			new->matrix.column_widths = (short *) XtMalloc(new->matrix.columns * sizeof(short));
			for(i = 0; i < new->matrix.columns; i++)
				new->matrix.column_widths[i] = DEFAULT_COLUMN_WIDTH(new);
		}
		else
		{
			xbaeCopyColumnWidths(new);
		}
	}
	if(new->matrix.rows)
	{
		if(new->matrix.row_heights == NULL)
		{
			int i;

			new->matrix.row_heights = (short *) XtMalloc(new->matrix.rows * sizeof(short));
			for(i = 0; i < new->matrix.rows; i++)
				new->matrix.row_heights[i] = DEFAULT_ROW_HEIGHT(new);
		}
		else
		{
			xbaeCopyRowHeights(new);
		}
	}

	/*
	 * Copy the pointed to resources.
	 */
	if(new->matrix.column_alignments)
		xbaeCopyColumnAlignments(new);
	if(new->matrix.column_label_alignments)
		xbaeCopyColumnLabelAlignments(new);
	if(new->matrix.column_font_bold)
		xbaeCopyColumnFontBold(new);
	if(new->matrix.show_column_arrows)
		xbaeCopyShowColumnArrows(new);
	if(new->matrix.column_max_lengths)
		xbaeCopyColumnMaxLengths(new);

	if(new->matrix.column_button_labels)
		xbaeCopyColumnButtonLabels(new);
	if(new->matrix.row_button_labels)
		xbaeCopyRowButtonLabels(new);

	if(new->matrix.column_label_foregrounds)
		xbaeCopyColumnLabelForegrounds(new);
	if(new->matrix.column_label_backgrounds)
		xbaeCopyColumnLabelBackgrounds(new);

	if(new->matrix.row_label_foregrounds)
		xbaeCopyRowLabelForegrounds(new);
	if(new->matrix.row_label_backgrounds)
		xbaeCopyRowLabelBackgrounds(new);

	if(new->matrix.column_user_data)
		xbaeCopyColumnUserData(new);
	if(new->matrix.row_user_data)
		xbaeCopyRowUserData(new);

	if(new->matrix.column_shadow_types)
		xbaeCopyColumnShadowTypes(new);
	if(new->matrix.row_shadow_types)
		xbaeCopyRowShadowTypes(new);

	if(new->matrix.column_labels || new->matrix.xmcolumn_labels)
	{
		xbaeCopyColumnLabels(new);
		new->matrix.column_label_maxlines = xbaeCalculateLabelMaxLines(new->matrix.column_labels,
																	   new->matrix.xmcolumn_labels,
																	   new->matrix.columns);
	}
	else
	{
		new->matrix.column_label_maxlines = 0;
	}
	if(new->matrix.row_labels || new->matrix.xmrow_labels)
	{
		xbaeCopyRowLabels(new);
		new->matrix.row_label_maxwidth = xbaeCalculateLabelMaxWidth(new,
																	  new->matrix.row_labels,
																	  new->matrix.xmrow_labels,
																	  new->matrix.rows);
	}
	else
	{
		new->matrix.row_label_maxwidth = 0;
	}

	/*
	 * Create the 2 SBs unmanaged
	 */
	new->matrix.horizontal_sb =
		XtVaCreateWidget("horizScroll", xmScrollBarWidgetClass, (Widget) new,
						 XmNorientation, XmHORIZONTAL,
						 XmNdragCallback, HSCallback,
						 XmNvalueChangedCallback, HSCallback,
						 XmNincrement, CELL_FONT_WIDTH(new),
						 XmNminimum, 0,
						 XmNmaximum, 1,
						 XmNsliderSize, 1,
						 XmNbackground, new->matrix.scroll_background,
						 XmNforeground, new->manager.foreground,
						 XmNbottomShadowColor, new->manager.bottom_shadow_color,
						 XmNbottomShadowPixmap, new->manager.bottom_shadow_pixmap,
						 XmNhighlightColor, new->manager.highlight_color,
						 XmNhighlightPixmap, new->manager.highlight_pixmap,
						 XmNtopShadowColor, new->manager.top_shadow_color,
						 XmNtopShadowPixmap, new->manager.top_shadow_pixmap, NULL);

	HORIZ_ORIGIN(new) = 0;

	new->matrix.vertical_sb = XtVaCreateWidget("vertScroll", xmScrollBarWidgetClass, (Widget) new, XmNorientation, XmVERTICAL, XmNdragCallback, VSCallback, XmNvalueChangedCallback, VSCallback, XmNincrement, TEXT_HEIGHT(new), XmNminimum, 0,	/*  gonna be corrected in relayout() */
											   XmNmaximum, 1,
											   XmNsliderSize, 1,
											   XmNbackground, new->matrix.scroll_background,
											   XmNforeground, new->manager.foreground,
											   XmNbottomShadowColor, new->manager.bottom_shadow_color,
											   XmNbottomShadowPixmap, new->manager.bottom_shadow_pixmap,
											   XmNhighlightColor, new->manager.highlight_color,
											   XmNhighlightPixmap, new->manager.highlight_pixmap,
											   XmNtopShadowColor, new->manager.top_shadow_color,
											   XmNtopShadowPixmap, new->manager.top_shadow_pixmap, NULL);

	VERT_ORIGIN(new) = 0;

	/*
	 * Create 7 clips for the 7 scrollable regions.
	 */
	new->matrix.clip_window = XtVaCreateWidget("clip", xbaeClipWidgetClass, (Widget) new,
											   XmNexposeProc, ClipRedisplay,
											   XmNtraversalOn, False,
											   XmNbackground, new->core.background_pixel, NULL);
	new->matrix.left_clip = XtVaCreateWidget("leftclip", xbaeClipWidgetClass, (Widget) new,
											 XmNexposeProc, ClipRedisplay,
											 XmNtraversalOn, False,
											 XmNbackground, new->core.background_pixel, NULL);
	new->matrix.right_clip = XtVaCreateWidget("rightclip", xbaeClipWidgetClass, (Widget) new,
											  XmNexposeProc, ClipRedisplay,
											  XmNtraversalOn, False,
											  XmNbackground, new->core.background_pixel, NULL);
	new->matrix.top_clip = XtVaCreateWidget("topclip", xbaeClipWidgetClass, (Widget) new,
											XmNexposeProc, ClipRedisplay,
											XmNtraversalOn, False,
											XmNbackground, new->core.background_pixel, NULL);
	new->matrix.bottom_clip = XtVaCreateWidget("bottomclip", xbaeClipWidgetClass, (Widget) new,
											   XmNexposeProc, ClipRedisplay,
											   XmNtraversalOn, False,
											   XmNbackground, new->core.background_pixel, NULL);
	new->matrix.row_label_clip = XtVaCreateWidget("rowlabelclip", xbaeClipWidgetClass, (Widget) new,
												  XmNexposeProc, ClipRedisplay,
												  XmNtraversalOn, False,
												  XmNbackground, new->core.background_pixel, NULL);
	new->matrix.column_label_clip = XtVaCreateWidget("columnlabelclip", xbaeClipWidgetClass, (Widget) new,
													 XmNexposeProc, ClipRedisplay,
													 XmNtraversalOn, False,
													 XmNbackground, new->core.background_pixel, NULL);

	/*
	 * Bug Fix for Release Xbae-4.50.3 with margin width -
	 * If it 0 or less, then user cannot backspace in the Text Widget
	 * doesn't work correctly.
	 */
	if(new->matrix.cell_margin_width < 1)
	{
		new->matrix.cell_margin_width = 1;
	}

	/*
	 * Calculate the baselines at which to draw the row labels, the
	 * cell and the marginHeight of the textChild based on the 
	 * largest of the label and cell font. 
	 * Column lables baselines are always - new->matrix.label_font.y
	 */
	new->matrix.cell_baseline = -new->matrix.cell_font.y;
	new->matrix.row_label_baseline = -new->matrix.label_font.y;
	marginHeight = new->matrix.cell_margin_height;
	if(LABEL_FONT_HEIGHT(new) > CELL_FONT_HEIGHT(new))
	{
		new->matrix.cell_baseline += (LABEL_FONT_HEIGHT(new) - CELL_FONT_HEIGHT(new)) / 2;
		marginHeight += (LABEL_FONT_HEIGHT(new) - CELL_FONT_HEIGHT(new)) / 2;
	}
	else if(LABEL_FONT_HEIGHT(new) < CELL_FONT_HEIGHT(new))
	{
		new->matrix.row_label_baseline += (CELL_FONT_HEIGHT(new) - LABEL_FONT_HEIGHT(new)) / 2;
	}

	/*
	 * Create text field managed so we can use it for traversal
	 */
	new->matrix.text_field = XtVaCreateManagedWidget("textField",
													 new->matrix.useXbaeInput
													 ? xbaeInputWidgetClass
													 : xmTextWidgetClass,
													 (Widget) new,
													 XmNmarginWidth, new->matrix.cell_margin_width,
													 XmNmarginHeight, marginHeight,
													 XmNrenderTable, new->matrix.render_table,
													 XmNshadowThickness, new->matrix.text_shadow_thickness,
													 XmNforeground, new->manager.foreground,
													 XmNbottomShadowColor, new->manager.bottom_shadow_color,
													 XmNbottomShadowPixmap, new->manager.bottom_shadow_pixmap,
													 XmNhighlightThickness, new->matrix.cell_highlight_thickness,
													 XmNhighlightColor, new->manager.highlight_color,
													 XmNhighlightPixmap, new->manager.highlight_pixmap,
													 XmNeditMode, (new->matrix.multi_line_cell) ? XmMULTI_LINE_EDIT : XmSINGLE_LINE_EDIT,
													 XmNwordWrap, (new->matrix.wrap_type == XbaeWrapNone) ? False : True,
													 XmNnavigationType, XmNONE, XmNtraversalOn,
													 new->manager.traversal_on,
													 NULL);

	XtAddCallback(TextChild(new), XmNfocusCallback, xbaeFocusCB, (XtPointer) new);
	XtAddCallback(TextChild(new), XmNlosingFocusCallback, xbaeLosingFocusCB, (XtPointer) new);
	XtAddCallback(TextChild(new), XmNmodifyVerifyCallback, xbaeModifyVerifyCB, (XtPointer) new);
	XtAddCallback(TextChild(new), XmNvalueChangedCallback, xbaeValueChangedCB, (XtPointer) new);

	/* Add a handler on top of the text field to handle clicks on it */
	XtAddEventHandler(TextChild(new),
					  ButtonPressMask | ButtonReleaseMask,
					  True, (XtEventHandler) xbaeHandleClick, (XtPointer) new);
	XtAddEventHandler((Widget) new,
					  ButtonPressMask | ButtonReleaseMask,
					  True, (XtEventHandler) xbaeHandleClick, (XtPointer) new);

	/* Install the text_translations on the textChild */
	if(new->matrix.text_translations == NULL)
	{
		Widget shell = _XbaeGetShellAncestor(nw);

		if(XtIsSubclass(shell, xmDialogShellWidgetClass))
		{
			new->matrix.text_translations = XtParseTranslationTable(default_dialog_text_translations);
		}
		else
		{
			new->matrix.text_translations = XtParseTranslationTable(default_text_translations);
		}
	}

	XtOverrideTranslations(TextChild(new), new->matrix.text_translations);

	/*
	 * Cache the pixel position of each column
	 */

	new->matrix.column_positions = CreateColumnPositions(new);
	new->matrix.row_positions = CreateRowPositions(new);

	xbaeGetColumnPositions(new);
	xbaeGetRowPositions(new);

	/*
	 * Compute our size.  If either dimension was explicitly set to 0,
	 * then that dimension is computed.
	 * Use request because superclasses modify width/height.
	 */
	if(request->core.width == 0 || request->core.height == 0)
		xbaeComputeSize(new, request->core.width == 0, request->core.height == 0);

	/*
	 * Layout the scrollbars and clip widget based on our size
	 */
	xbaeRelayout(new);

	/*
	 * Deal with the hidden resources (that are no longer resources but elements of
	 * the per cell structure).
	 */
	base.cells = 0;
	XtGetSubresources(nw, &base, XtName(nw), "xbaeMatrixWidgetClass",
					  subresources, XtNumber(subresources), args, *num_args);
	ac = 0;
	XtSetArg(al[ac], XmNcells, base.cells);
	ac++;
	XtSetArg(al[ac], XmNcolors, base.colors);
	ac++;
	XtSetArg(al[ac], XmNcellBackgrounds, base.background);
	ac++;
	XtSetArg(al[ac], XmNleftColumn, base.left_column);
	ac++;
	XtSetArg(al[ac], XmNtopRow, base.top_row);
	ac++;
	SetValuesHook(nw, al, &ac);
#if 0
	fprintf(stderr, "BASE cells %p\n", base.cells);
	{
		int i, j;
		for(i = 0; i < new->matrix.rows; i++)
			for(j = 0; j < new->matrix.columns; j++)
				fprintf(stderr, "\tCell[%d][%d] = {%s}\n", i, j, base.cells[i][j]);
	}
#endif
}

/* 2006.10.11 - this function was not set to static in the official one */
static void Realize(XbaeMatrixWidget mw, XtValueMask * valueMask, XSetWindowAttributes * attributes)
{

	*valueMask |= CWDontPropagate;
	attributes->do_not_propagate_mask =
		ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask;

	/*
	 * Don't call our superclasses realize method, because Manager sets
	 * bit_gravity
	 */
	XtCreateWindow((Widget) mw, InputOutput, CopyFromParent, *valueMask, attributes);

	/*
	 * Now that we have a window...
	 * Get/create our GCs
	 */

	xbaeCreateDrawGC(mw);
	xbaeCreatePixmapGC(mw);
	xbaeCreateLabelGC(mw);
	xbaeGetGridLineGC(mw);
	xbaeGetResizeTopShadowGC(mw);
	xbaeGetResizeBottomShadowGC(mw);

	/*
	 * Realize our children
	 */
	XtRealizeWidget(TextChild(mw));
	XtRealizeWidget(ClipChild(mw));
	XtRealizeWidget(LeftClip(mw));
	XtRealizeWidget(RightClip(mw));
	XtRealizeWidget(TopClip(mw));
	XtRealizeWidget(BottomClip(mw));
	XtRealizeWidget(RowLabelClip(mw));
	XtRealizeWidget(ColumnLabelClip(mw));

	/*
	 * Hide the textChild
	 */
	xbaeHideTextChild(mw);

	/*
	 * Make sure the user widgets get realized and
	 * then reparented.
	 * -- Linas */
	if(mw->matrix.per_cell)
	{
		int row, col;
		for(row = 0; row < mw->matrix.rows; row++)
		{
			for(col = 0; col < mw->matrix.columns; col++)
			{
				Widget uw = mw->matrix.per_cell[row][col].widget;
				if(uw)
				{
					XtRealizeWidget(uw);

					if(XmIsGadget(uw))
					{
						/*
						 * FIX ME don't know how to deal with gadgets
						 */
					}
					else
					{
						xbaePositionCellWidget(mw, row, col);
					}
				}
			}
		}
		xbaeSetInitialFocus(mw);
	}

	/*
	 * Init
	 */
	mw->matrix.prev_column = -1;	/* Used to compare tracking callback */
	mw->matrix.prev_row = -1;	/* Used to compare tracking callback */
}

static void InsertChild(Widget w)
{
	((XmManagerWidgetClass) (xbaeMatrixWidgetClass->core_class.superclass))->composite_class.insert_child(w);
}

/*
 * This is the expose method for the Matrix widget.
 * It redraws the fixed labels, the cells in totally fixed cells
 * and the shadow.
 */

/* ARGSUSED */
static void Redisplay(Widget w, XEvent * event, Region region)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) w;
	XRectangle expose;

	if(mw->matrix.disable_redisplay)
		return;

	if(!XtIsRealized(w))
		return;

	/*
	 * Get the expose rectangle from the XEvent
	 */

	switch (event->type)
	{
		case Expose:

			expose.x = event->xexpose.x;
			expose.y = event->xexpose.y;
			expose.width = event->xexpose.width;
			expose.height = event->xexpose.height;

			break;

		case GraphicsExpose:

			expose.x = event->xgraphicsexpose.x;
			expose.y = event->xgraphicsexpose.y;
			expose.width = event->xgraphicsexpose.width;
			expose.height = event->xgraphicsexpose.height;

			break;

		case NoExpose:
		default:
			return;
	}
	xbaeRedrawLabelsAndFixed(mw, &expose);
}

/*
 * This is the exposeProc function for the Clip widgets.
 * It handles expose events for the Clip widgets by redrawing those
 * non-fixed cells which were damaged.
 * It receives Expose, GraphicsExpose and NoExpose events.
 */

/* ARGSUSED */
static void ClipRedisplay(Widget w, XRectangle * expose, XEvent * event, Region r)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) XtParent(w);
	XRectangle region;

	if(mw->matrix.disable_redisplay)
		return;

	/*
	 * Make the expose rectangle relative to the matrix 
	 */

	expose->x += w->core.x;
	expose->y += w->core.y;

	region.x = w->core.x;
	region.y = w->core.y;
	region.width = w->core.width;
	region.height = w->core.height;

	xbaeRedrawRegion(mw, expose, &region);
}

/*
 * Handle XmNcells, which is no longer a widget resource, but hidden in the per cell structure.
 */
static Boolean SetValuesHook(Widget w, ArgList args, Cardinal * nargs)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) w;
	Boolean redisplay = False;
	int i, row, col;

	DEBUGOUT(_XbaeDebug(__FILE__, w, "SetValuesHook args:\n"));
	DEBUGOUT(_XbaeDebugPrintArgList(__FILE__, w, args, *nargs, False));

	for(i = 0; i < *nargs; i++)
	{
		if(strcmp(args[i].name, XmNcells) == 0)
		{
			char ***cells = (char ***) args[i].value;

			if(!cells)
				continue;

			/*
			 * Create the per cell structure if needed
			 */
			if(!mw->matrix.per_cell)
				xbaeCreatePerCell(mw);

			/*
			 * This is a two-dimensional array of cells.
			 * Each cell is a string.
			 * Copy them.
			 */
			for(row = 0; row < mw->matrix.rows; row++)
			{
				if(cells[row] == NULL)
				{
					XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
									"setValuesHook", "tooShort",
									"XbaeMatrix", "XbaeMatrix: Not enough rows in cells array", NULL, 0);
					break;
				}

				for(col = 0; col < mw->matrix.columns; col++)
				{
					if(cells[row][col] == &xbaeBadString)
					{
						XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
										"setValuesHook", "tooShort",
										"XbaeMatrix",
										"XbaeMatrix: Not enough columns in cells array", NULL, 0);
						break;
					}

					if(mw->matrix.per_cell[row][col].CellValue)
					{
						if(strcmp(mw->matrix.per_cell[row][col].CellValue, cells[row][col]) != 0)
						{
							XtFree(mw->matrix.per_cell[row][col].CellValue);
							mw->matrix.per_cell[row][col].CellValue = XtNewString(cells[row][col]);
							xbaeDrawCell(mw, row, col);
						}
					}
					else
					{
						mw->matrix.per_cell[row][col].CellValue = XtNewString(cells[row][col]);
						xbaeDrawCell(mw, row, col);
					}
				}
			}

			if(mw->matrix.text_child_is_mapped)
			{
				xbaeUpdateTextChild(mw, True);
			}

		}
		else if(strcmp(args[i].name, XmNcellShadowTypes) == 0)
		{
			unsigned char **st = (unsigned char **) args[i].value;

			/* Tobias: FIXME There is no converter for this resource */

			if(!st)
				continue;

			/*
			 * Create the per cell structure if needed
			 */
			if(!mw->matrix.per_cell)
				xbaeCreatePerCell(mw);

			/*
			 * This is a two-dimensional array of data, each item is one byte.
			 */
			for(row = 0; row < mw->matrix.rows; row++)
			{
				if(st[row] == NULL)
				{
					XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
									"setValuesHook", "tooShort",
									"XbaeMatrix",
									"XbaeMatrix: Not enough rows in cellsShadowTypes array", NULL, 0);
					break;
				}

				for(col = 0; col < mw->matrix.columns; col++)
				{
					if(st[row][col] == BAD_SHADOW)
					{
						XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
										"setValuesHook", "tooShort",
										"XbaeMatrix",
										"XbaeMatrix: Not enough columns in cellsShadowTypes array", NULL, 0);
						break;
					}

					if(mw->matrix.per_cell[row][col].shadow_type != st[row][col])
					{
						mw->matrix.per_cell[row][col].shadow_type = st[row][col];
						xbaeDrawCell(mw, row, col);
					}
				}
			}

		}
		else if(strcmp(args[i].name, XmNcellBackgrounds) == 0)
		{
			Pixel **bg = (Pixel **) args[i].value;;

			if(!bg)
				continue;

			/*
			 * Create the per cell structure if needed
			 */
			if(!mw->matrix.per_cell)
				xbaeCreatePerCell(mw);

			/*
			 * This is a two-dimensional array of data, each item is one byte.
			 */
			for(row = 0; row < mw->matrix.rows; row++)
			{
				if(bg[row] == NULL)
				{
					XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
									"setValuesHook", "tooShort",
									"XbaeMatrix",
									"XbaeMatrix: Not enough rows in backgrounds array", NULL, 0);
					break;
				}

				for(col = 0; col < mw->matrix.columns; col++)
				{
					if(bg[row][col] == BAD_PIXEL)
					{
						XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
										"setValuesHook", "tooShort",
										"XbaeMatrix",
										"XbaeMatrix: Not enough columns in backgrounds array", NULL, 0);
						break;
					}

					if(mw->matrix.per_cell[row][col].background != bg[row][col])
					{
						mw->matrix.per_cell[row][col].background = bg[row][col];
						xbaeDrawCell(mw, row, col);
					}
				}
			}

			if(mw->matrix.text_child_is_mapped)
			{
				xbaeUpdateTextChild(mw, False);
			}

		}
		else if(strcmp(args[i].name, XmNcolors) == 0)
		{
			Pixel **fg = (Pixel **) args[i].value;

			if(!fg)
				continue;

			/*
			 * Create the per cell structure if needed
			 */
			if(!mw->matrix.per_cell)
				xbaeCreatePerCell(mw);

			/*
			 * This is a two-dimensional array of data, each item is one byte.
			 */
			for(row = 0; row < mw->matrix.rows; row++)
			{
				if(fg[row] == NULL)
				{
					XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
									"setValuesHook", "tooShort",
									"XbaeMatrix", "XbaeMatrix: Not enough rows in colors array", NULL, 0);
					break;
				}

				for(col = 0; col < mw->matrix.columns; col++)
				{
					if(fg[row][col] == BAD_PIXEL)
					{
						XtAppWarningMsg(XtWidgetToApplicationContext((Widget) mw),
										"setValuesHook", "tooShort",
										"XbaeMatrix",
										"XbaeMatrix: Not enough columns in colors array", NULL, 0);
						break;
					}

					if(mw->matrix.per_cell[row][col].color != fg[row][col])
					{
						mw->matrix.per_cell[row][col].color = fg[row][col];
						xbaeDrawCell(mw, row, col);
					}
				}
			}

			if(mw->matrix.text_child_is_mapped)
			{
				xbaeUpdateTextChild(mw, False);
			}

		}
		else if(strcmp(args[i].name, XmNleftColumn) == 0)
		{
			XmScrollBarCallbackStruct call_data;
			call_data.value = xbaeCalculateHorizOrigin(mw, args[i].value);
			xbaeScrollHorizCB((Widget) HorizScrollChild(mw), NULL, &call_data);
			XtVaSetValues(HorizScrollChild(mw), XmNvalue, HORIZ_ORIGIN(mw), NULL);
		}
		else if(strcmp(args[i].name, XmNtopRow) == 0)
		{
			XmScrollBarCallbackStruct call_data;
			call_data.value = xbaeCalculateVertOrigin(mw, args[i].value);
			xbaeScrollVertCB((Widget) VertScrollChild(mw), NULL, &call_data);
			XtVaSetValues(VertScrollChild(mw), XmNvalue, VERT_ORIGIN(mw), NULL);
		}
	}
	return redisplay;
}

static void GetValuesHook(Widget w, ArgList args, Cardinal * nargs)
{
	XbaeMatrixWidget mw = (XbaeMatrixWidget) w;
	int i, row, col;

	for(i = 0; i < *nargs; i++)
	{
		if(strcmp(args[i].name, XmNcells) == 0)
		{
			String ***p = (String ***) args[i].value;
			String **cells;
			/*
			 * If the per cell structure isn't there yet, don't return anything.
			 */
			if(!mw->matrix.per_cell)
			{
				*p = NULL;
				continue;		/* on to the next ARG */
			}

			/*
			 * This is a two-dimensional array of cells.
			 * Each cell is a string.
			 * Copy them.
			 */
			cells = (String **) XtMalloc(mw->matrix.rows * sizeof(char **));
			for(row = 0; row < mw->matrix.rows; row++)
			{
				cells[row] = (String *) XtMalloc(mw->matrix.columns * sizeof(char *));
				for(col = 0; col < mw->matrix.columns; col++)
				{
					cells[row][col] = XtNewString(mw->matrix.per_cell[row][col].CellValue);
				}
			}
			*p = cells;
		}
		else if(strcmp(args[i].name, XmNcellShadowTypes) == 0)
		{
			unsigned char ***p = (unsigned char ***) args[i].value;
			unsigned char **shadow_types;
			/*
			 * If the per cell structure isn't there yet, don't return anything.
			 */
			if(!mw->matrix.per_cell)
			{
				*p = NULL;
				continue;		/* on to the next ARG */
			}

			/*
			 * This is a two-dimensional array of cells.
			 * Each cell is a string.
			 * Copy them.
			 */
			shadow_types = (unsigned char **) XtMalloc(mw->matrix.rows * sizeof(unsigned char *));
			for(row = 0; row < mw->matrix.rows; row++)
			{
				shadow_types[row] = (unsigned char *) XtMalloc(mw->matrix.columns);
				for(col = 0; col < mw->matrix.columns; col++)
				{
					shadow_types[row][col] = mw->matrix.per_cell[row][col].shadow_type;
				}
			}
			*p = shadow_types;
		}
		else if(strcmp(args[i].name, XmNcellBackgrounds) == 0)
		{
			Pixel ***p = (Pixel ***) args[i].value;
			Pixel **bg;
			/*
			 * If the per cell structure isn't there yet, don't return anything.
			 */
			if(!mw->matrix.per_cell)
			{
				*p = NULL;
				continue;		/* on to the next ARG */
			}

			/*
			 * This is a two-dimensional array of cells.
			 * Each cell is a string.
			 * Copy them.
			 */
			bg = (Pixel **) XtMalloc(mw->matrix.rows * sizeof(Pixel *));
			for(row = 0; row < mw->matrix.rows; row++)
			{
				bg[row] = (Pixel *) XtMalloc(mw->matrix.columns * sizeof(Pixel));
				for(col = 0; col < mw->matrix.columns; col++)
				{
					bg[row][col] = mw->matrix.per_cell[row][col].background;
				}
			}
			*p = bg;
		}
		else if(strcmp(args[i].name, XmNcolors) == 0)
		{
			Pixel ***p = (Pixel ***) args[i].value;
			Pixel **fg;
			/*
			 * If the per cell structure isn't there yet, don't return anything.
			 */
			if(!mw->matrix.per_cell)
			{
				*p = NULL;
				continue;		/* on to the next ARG */
			}

			/*
			 * This is a two-dimensional array of cells.
			 * Each cell is a string.
			 * Copy them.
			 */
			fg = (Pixel **) XtMalloc(mw->matrix.rows * sizeof(Pixel *));
			for(row = 0; row < mw->matrix.rows; row++)
			{
				fg[row] = (Pixel *) XtMalloc(mw->matrix.columns * sizeof(Pixel));
				for(col = 0; col < mw->matrix.columns; col++)
				{
					fg[row][col] = mw->matrix.per_cell[row][col].color;
				}
			}
			*p = fg;
		}
		else if(strcmp(args[i].name, XmNleftColumn) == 0)
		{
			int *p = (int *) args[i].value;
			*p = xbaeLeftColumn(mw);
		}
		else if(strcmp(args[i].name, XmNtopRow) == 0)
		{
			int *p = (int *) args[i].value;
			*p = xbaeTopRow(mw);
		}
	}
}

/* ARGSUSED */
static Boolean
SetValues(XbaeMatrixWidget current, XbaeMatrixWidget request, XbaeMatrixWidget new, ArgList args,
		  Cardinal * num_args)
{

	Boolean hide_text_child = False;	/* need to hide the textChild */
	Boolean update_text_child = False;	/* need to update the textChild */
	Boolean redisplay = False;	/* need to redraw */
	Boolean relayout = False;	/* need to layout, but same size */
	Boolean new_column_widths = False;	/* column widths changed */
	Boolean new_row_heights = False;	/* row heights changed */
	int n;
	Arg wargs[11];

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) new, "SetValues args:\n"));
	DEBUGOUT(_XbaeDebugPrintArgList(__FILE__, (Widget) new, args, *num_args, False));

#define NE(field)	(current->field != new->field)
#define EQ(field)	(current->field == new->field)

	/*
	 * We cannot re-set either of the scrollbars, the textField or
	 * clip window.
	 */
	if(NE(matrix.vertical_sb) || NE(matrix.horizontal_sb) || NE(matrix.clip_window) || NE(matrix.text_field))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "setValues",
						"set matrix children", "XbaeMatrix",
						"XbaeMatrix: Cannot set matrix widget children", NULL, 0);
		new->matrix.vertical_sb = current->matrix.vertical_sb;
		new->matrix.horizontal_sb = current->matrix.horizontal_sb;
		new->matrix.clip_window = current->matrix.clip_window;
		new->matrix.text_field = current->matrix.text_field;
	}

	/*
	 * If rows changed, then:
	 *  row_labels must change or be NULL
	 *  xmrow_labels must change or be NULL
	 *  row_button_labels must change or be NULL
	 */
	if(NE(matrix.rows)
	   && ((new->matrix.row_labels && EQ(matrix.row_labels))
		   || (new->matrix.xmrow_labels && EQ(matrix.xmrow_labels))
		   || (new->matrix.row_label_backgrounds && EQ(matrix.row_label_backgrounds))
		   || (new->matrix.row_label_foregrounds && EQ(matrix.row_label_foregrounds))
		   || (new->matrix.row_button_labels && EQ(matrix.row_button_labels))))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "setValues", "rows",
						"XbaeMatrix",
						"XbaeMatrix: Number of rows changed but dependent resources did not", NULL, 0);
		new->matrix.rows = current->matrix.rows;
		new->matrix.row_labels = current->matrix.row_labels;
		new->matrix.xmrow_labels = current->matrix.xmrow_labels;
		new->matrix.row_button_labels = current->matrix.row_button_labels;
		new->matrix.row_label_backgrounds = current->matrix.row_label_backgrounds;
		new->matrix.row_label_foregrounds = current->matrix.row_label_foregrounds;
	}

	/*
	 * If columns changed, then:
	 *  column_max_lengths must change or be NULL
	 *  column_labels must change or be NULL
	 *  xmcolumn_labels must change or be NULL
	 *  column_alignments must change or be NULL
	 *  column_button_labels must change or be NULL
	 *  column_label_alignments must change or be NULL
	 *  show_column_arrows must change or be NULL
	 */
	if(NE(matrix.columns)
	   && ((new->matrix.column_labels && EQ(matrix.column_labels))
		   || (new->matrix.xmcolumn_labels && EQ(matrix.xmcolumn_labels))
		   || (new->matrix.column_max_lengths && EQ(matrix.column_max_lengths))
		   || (new->matrix.column_alignments && EQ(matrix.column_alignments))
		   || (new->matrix.column_font_bold && EQ(matrix.column_font_bold))
		   || (new->matrix.column_button_labels && EQ(matrix.column_button_labels))
		   || (new->matrix.column_label_alignments && EQ(matrix.column_label_alignments))
		   || (new->matrix.column_label_foregrounds && EQ(matrix.column_label_foregrounds))
		   || (new->matrix.column_label_backgrounds && EQ(matrix.column_label_backgrounds))
		   || (new->matrix.show_column_arrows && EQ(matrix.show_column_arrows))))
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "setValues", "columns",
						"XbaeMatrix",
						"XbaeMatrix: Number of columns changed but dependent resources did not", NULL, 0);
		/* FIX ME Are there memory leaks here ? */
		new->matrix.columns = current->matrix.columns;
		new->matrix.column_max_lengths = current->matrix.column_max_lengths;
		new->matrix.column_labels = current->matrix.column_labels;
		new->matrix.xmcolumn_labels = current->matrix.xmcolumn_labels;
		new->matrix.column_alignments = current->matrix.column_alignments;
		new->matrix.column_font_bold = current->matrix.column_font_bold;
		new->matrix.column_button_labels = current->matrix.column_button_labels;
		new->matrix.column_label_alignments = current->matrix.column_label_alignments;
		new->matrix.column_label_backgrounds = current->matrix.column_label_backgrounds;
		new->matrix.column_label_foregrounds = current->matrix.column_label_foregrounds;
		new->matrix.show_column_arrows = current->matrix.show_column_arrows;
	}

	/*
	 * Make sure we have a sane number of rows/columns.
	 */
	if(new->matrix.columns < 0 || new->matrix.rows < 0)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "setValues", "size",
						"XbaeMatrix", "XbaeMatrix: Number of rows or columns is less than zero", NULL, 0);
		if(new->matrix.columns < 0)
			new->matrix.columns = current->matrix.columns;
		if(new->matrix.rows < 0)
			new->matrix.rows = current->matrix.rows;
	}

	/*
	 * We can't have more fixed rows/columns than there are rows
	 */
	if(new->matrix.fixed_rows + new->matrix.trailing_fixed_rows > new->matrix.rows)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "setValues",
						"tooManyFixed", "XbaeMatrix",
						"XbaeMatrix: There can't be more fixed rows than there are rows", NULL, 0);

		if(NE(matrix.fixed_rows))
			new->matrix.fixed_rows = current->matrix.fixed_rows;
		if(NE(matrix.trailing_fixed_rows))
			new->matrix.trailing_fixed_rows = current->matrix.trailing_fixed_rows;
		if(NE(matrix.rows))
			new->matrix.rows = current->matrix.rows;
	}
	if(new->matrix.fixed_columns + new->matrix.trailing_fixed_columns > new->matrix.columns)
	{
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "setValues",
						"tooManyFixed", "XbaeMatrix",
						"XbaeMatrix: There can't be more fixed columns than there are columns", NULL, 0);

		if(NE(matrix.fixed_columns))
			new->matrix.fixed_columns = current->matrix.fixed_columns;
		if(NE(matrix.trailing_fixed_columns))
			new->matrix.trailing_fixed_columns = current->matrix.trailing_fixed_columns;
		if(NE(matrix.columns))
			new->matrix.columns = current->matrix.columns;
	}

	if(NE(matrix.grid_type) && (new->matrix.grid_type >= XmGRID_LINE))
		/* Deprecated types. To be removed in next version. */
		XtAppWarningMsg(XtWidgetToApplicationContext((Widget) new), "cvtStringToGridType",
						"deprecatedType", "XbaeMatrix",
						"Value for GridType is deprecated and will be removed in next release", NULL, NULL);

	/*
	 * Check if any of the resources changed that only requires a redisplay
	 */
	if(NE(matrix.grid_type)
	   || NE(matrix.even_row_background)
	   || NE(matrix.odd_row_background)
	   || NE(matrix.selected_foreground)
	   || NE(matrix.selected_background)
	   || NE(matrix.reverse_select) || NE(matrix.shadow_type) || NE(matrix.show_arrows))
		redisplay = True;

	if(NE(matrix.even_row_background)
	   || NE(matrix.odd_row_background)
	   || (NE(matrix.alt_row_count) && new->matrix.even_row_background != new->matrix.odd_row_background))
	{
		redisplay = True;
		if(new->matrix.text_background == XmUNSPECIFIED_PIXEL && new->matrix.text_background_is_cell)
		{
			update_text_child = True;
		}
	}

	if(NE(matrix.text_background)
	   || (new->matrix.text_background == XmUNSPECIFIED_PIXEL
		   && (NE(matrix.text_background_is_cell) || NE(core.background_pixel))))
	{
		update_text_child = True;
	}

	if(NE(matrix.grid_line_color) && IN_GRID_LINE_MODE(new))
		redisplay = True;

	if(NE(matrix.cell_shadow_type) && IN_GRID_SHADOW_MODE(new))
		redisplay = True;

	/*
	 * If we have labels and the way they are displayed changed we need to redisplay
	 */
	if((new->matrix.row_labels
		|| new->matrix.column_labels
		|| new->matrix.xmrow_labels
		|| new->matrix.xmcolumn_labels)
	   && (NE(matrix.bold_labels)
		   || NE(matrix.button_labels)
		   || NE(matrix.button_label_background)
		   || NE(matrix.column_label_color) || NE(matrix.row_label_color) || NE(matrix.row_label_alignment)))
		redisplay = True;

	/*
	 * Check if any of the resources changed that requires a redisplay and
	 * needs to be passed down to our children
	 */
	n = 0;
	if(NE(manager.foreground))
	{
		XtSetArg(wargs[n], XmNforeground, new->manager.foreground);
		n++;
		redisplay = True;
	}
	if(NE(manager.bottom_shadow_color))
	{
		XtSetArg(wargs[n], XmNbottomShadowColor, new->manager.bottom_shadow_color);
		n++;
		redisplay = True;
	}
	if(NE(manager.bottom_shadow_pixmap))
	{
		XtSetArg(wargs[n], XmNbottomShadowPixmap, new->manager.bottom_shadow_pixmap);
		n++;
		redisplay = True;
	}
	if(NE(manager.top_shadow_color))
	{
		XtSetArg(wargs[n], XmNtopShadowColor, new->manager.top_shadow_color);
		n++;
		redisplay = True;
	}
	if(NE(manager.top_shadow_pixmap))
	{
		XtSetArg(wargs[n], XmNtopShadowPixmap, new->manager.top_shadow_pixmap);
		n++;
		redisplay = True;
	}
	if(NE(manager.highlight_color))
	{
		XtSetArg(wargs[n], XmNhighlightColor, new->manager.highlight_color);
		n++;
		redisplay = True;
	}
	if(NE(manager.highlight_pixmap))
	{
		XtSetArg(wargs[n], XmNhighlightPixmap, new->manager.highlight_pixmap);
		n++;
		redisplay = True;
	}
	if(NE(core.background_pixel))
	{
		/*
		 * Set the new core background of all our clips (thanks Daiji).
		 */
		XtVaSetValues(ClipChild(new), XmNbackground, new->core.background_pixel, NULL);
		XtVaSetValues(LeftClip(new), XmNbackground, new->core.background_pixel, NULL);
		XtVaSetValues(RightClip(new), XmNbackground, new->core.background_pixel, NULL);
		XtVaSetValues(TopClip(new), XmNbackground, new->core.background_pixel, NULL);
		XtVaSetValues(BottomClip(new), XmNbackground, new->core.background_pixel, NULL);
		XtVaSetValues(RowLabelClip(new), XmNbackground, new->core.background_pixel, NULL);
		XtVaSetValues(ColumnLabelClip(new), XmNbackground, new->core.background_pixel, NULL);
		/*
		 * Set the background of the scollbars without polluting wargs since we need it for textChild
		 */
		XtSetArg(wargs[n], XmNbackground, new->core.background_pixel);
		DEBUGOUT(_XbaeDebug(__FILE__, (Widget) new, "SetValues for vsb, hsb with args:"));
		DEBUGOUT(_XbaeDebugPrintArgList(__FILE__, (Widget) new, wargs, n + 1, False));
		XtSetValues(VertScrollChild(new), wargs, n + 1);
		XtSetValues(HorizScrollChild(new), wargs, n + 1);
		redisplay = True;
	}
	else if(n)
	{
		DEBUGOUT(_XbaeDebug(__FILE__, (Widget) new, "SetValues for vsb, hsb with args:"));
		DEBUGOUT(_XbaeDebugPrintArgList(__FILE__, (Widget) new, wargs, n, False));
		XtSetValues(VertScrollChild(new), wargs, n);
		XtSetValues(HorizScrollChild(new), wargs, n);
	}

	if(NE(manager.traversal_on))
	{
		XtSetArg(wargs[n], XmNtraversalOn, new->manager.traversal_on);
		n++;
	}
	if(NE(matrix.multi_line_cell))
	{
		XtSetArg(wargs[n], XmNeditMode,
				 (new->matrix.multi_line_cell) ? XmMULTI_LINE_EDIT : XmSINGLE_LINE_EDIT);
		n++;
		redisplay = True;
	}
	if(NE(matrix.wrap_type))
	{
		XtSetArg(wargs[n], XmNwordWrap, (new->matrix.wrap_type == XbaeWrapNone) ? False : True);
		n++;
		redisplay = True;
	}

	/*
	 * Pass resources on to the textchild.
	 */
	if(n)
	{
		DEBUGOUT(_XbaeDebug(__FILE__, (Widget) new, "SetValues for textChild with args:"));
		DEBUGOUT(_XbaeDebugPrintArgList(__FILE__, (Widget) new, wargs, n, False));
		XtSetValues(TextChild(new), wargs, n);
	}
	if(NE(matrix.text_translations))
	{
		XtOverrideTranslations(TextChild(new), new->matrix.text_translations);
	}

	/*
	 * If fonts change, recalculate their parameters and reset the SB increment.
	 * Relayout will be set later should cell sizes change.
	 */
	if(NE(matrix.render_table) || NE(matrix.cell_rendition_tag) || NE(matrix.label_rendition_tag))
	{
		if(NE(matrix.render_table))
		{
			if(!new->matrix.render_table)
				new->matrix.render_table = XmeGetDefaultRenderTable((Widget) new, XmLABEL_FONTLIST);
			new->matrix.render_table = XmRenderTableCopy(new->matrix.render_table, NULL, 0);

			if(current->matrix.render_table)
				XmRenderTableFree(current->matrix.render_table);
		}

		xbaeInitFonts(new);

		XtVaSetValues(HorizScrollChild(new), XmNincrement, CELL_FONT_WIDTH(new), NULL);
		XtVaSetValues(VertScrollChild(new), XmNincrement, TEXT_HEIGHT(new), NULL);

		/*
		 * Tobias: FIXME this will only update the font of the textChild on the next 
		 * editCell. See DoEditCell in Methods.c on how to fix this.
		 */
		new->matrix.current_text_qtag = NULLQUARK;

		redisplay = True;
	}

	/*
	 * Recalculate our baselines and pass the cell resources on to the textField.
	 * Relayout will be set later should cell sizes change.
	 */
	if(CELL_FONT_HEIGHT(new) != CELL_FONT_HEIGHT(current)
	   || LABEL_FONT_HEIGHT(new) != LABEL_FONT_HEIGHT(current)
	   || NE(matrix.cell_font.y)
	   || NE(matrix.label_font.y)
	   || NE(matrix.cell_margin_width)
	   || NE(matrix.cell_margin_height)
	   || NE(matrix.cell_highlight_thickness) || NE(matrix.text_shadow_thickness))
	{
		int marginHeight, current_marginHeight;

		/*
		 * Bugfix for 4.50.3 : if cell_margin_width <= 0, the text field
		 * doesn't handle backspace correctly.
		 */
		if(new->matrix.cell_margin_width < 1)
		{
			new->matrix.cell_margin_width = 1;
		}

		/*
		 * Recalculate the baselines at which to draw the row labels, the
		 * cell and the marginHeight of the textChild based on the 
		 * largest of the label and cell font. 
		 * Column lables baselines are always - new->matrix.label_font.y
		 */
		new->matrix.cell_baseline = -new->matrix.cell_font.y;
		new->matrix.row_label_baseline = -new->matrix.label_font.y;
		marginHeight = new->matrix.cell_margin_height;
		if(LABEL_FONT_HEIGHT(new) > CELL_FONT_HEIGHT(new))
		{
			new->matrix.cell_baseline += (LABEL_FONT_HEIGHT(new) - CELL_FONT_HEIGHT(new)) / 2;
			marginHeight += (LABEL_FONT_HEIGHT(new) - CELL_FONT_HEIGHT(new)) / 2;
		}
		else if(LABEL_FONT_HEIGHT(new) < CELL_FONT_HEIGHT(new))
		{
			new->matrix.row_label_baseline += (CELL_FONT_HEIGHT(new) - LABEL_FONT_HEIGHT(new)) / 2;
		}

		current_marginHeight = current->matrix.cell_margin_height;
		if(LABEL_FONT_HEIGHT(current) > CELL_FONT_HEIGHT(current))
		{
			current_marginHeight += (LABEL_FONT_HEIGHT(current) - CELL_FONT_HEIGHT(current)) / 2;
		}

		if(marginHeight != current_marginHeight
		   || NE(matrix.cell_margin_width)
		   || NE(matrix.cell_highlight_thickness) || NE(matrix.text_shadow_thickness))
		{
			XtVaSetValues(TextChild(new),
						  XmNhighlightThickness, new->matrix.cell_highlight_thickness,
						  XmNshadowThickness, new->matrix.text_shadow_thickness,
						  XmNmarginWidth, new->matrix.cell_margin_width, XmNmarginHeight, marginHeight, NULL);
			if(!new->matrix.text_child_is_mapped)
			{
				hide_text_child = True;
			}
		}

		redisplay = True;
	}

	/*
	 * Hide the textChild if the cell that was edited disapeared.
	 */
	if(new->matrix.current_row >= new->matrix.rows || new->matrix.current_column >= new->matrix.columns)
	{
		new->matrix.current_row = -1;
		new->matrix.current_column = -1;
		if(new->matrix.text_child_is_mapped)
		{
			hide_text_child = True;
		}
	}

	/*
	 * Copy the per column/row resources
	 */
	if(NE(matrix.column_alignments))
	{
		xbaeFreeColumnAlignments(current);
		if(new->matrix.column_alignments)
			xbaeCopyColumnAlignments(new);
		redisplay = True;
	}
	if(NE(matrix.column_label_alignments))
	{
		xbaeFreeColumnLabelAlignments(current);
		if(new->matrix.column_label_alignments)
			xbaeCopyColumnLabelAlignments(new);
		redisplay = True;
	}
	if(NE(matrix.column_label_backgrounds))
	{
		xbaeFreeColumnLabelBackgrounds(current);
		if(new->matrix.column_label_backgrounds)
			xbaeCopyColumnLabelBackgrounds(new);
		redisplay = True;
	}
	if(NE(matrix.column_label_foregrounds))
	{
		xbaeFreeColumnLabelForegrounds(current);
		if(new->matrix.column_label_foregrounds)
			xbaeCopyColumnLabelForegrounds(new);
		redisplay = True;
	}
	if(NE(matrix.row_label_backgrounds))
	{
		xbaeFreeRowLabelBackgrounds(current);
		if(new->matrix.row_label_backgrounds)
			xbaeCopyRowLabelBackgrounds(new);
		redisplay = True;
	}
	if(NE(matrix.row_label_foregrounds))
	{
		xbaeFreeRowLabelForegrounds(current);
		if(new->matrix.row_label_foregrounds)
			xbaeCopyRowLabelForegrounds(new);
		redisplay = True;
	}
	if(NE(matrix.column_font_bold))
	{
		xbaeFreeColumnFontBold(current);
		if(new->matrix.column_font_bold)
			xbaeCopyColumnFontBold(new);
		redisplay = True;
	}
	if(NE(matrix.show_column_arrows))
	{
		xbaeFreeShowColumnArrows(current);
		if(new->matrix.show_column_arrows)
			xbaeCopyShowColumnArrows(new);
		redisplay = True;
	}
	if(NE(matrix.column_max_lengths))
	{
		xbaeFreeColumnMaxLengths(current);
		if(new->matrix.column_max_lengths)
			xbaeCopyColumnMaxLengths(new);
		redisplay = True;
	}


	if(NE(matrix.column_button_labels))
	{
		xbaeFreeColumnButtonLabels(current);
		if(new->matrix.column_button_labels)
			xbaeCopyColumnButtonLabels(new);
		redisplay = True;
	}
	if(NE(matrix.row_button_labels))
	{
		xbaeFreeRowButtonLabels(current);
		if(new->matrix.row_button_labels)
			xbaeCopyRowButtonLabels(new);
		redisplay = True;
	}

	if(NE(matrix.column_user_data))
	{
		xbaeFreeColumnUserData(current);
		if(new->matrix.column_user_data)
			xbaeCopyColumnUserData(new);
	}
	if(NE(matrix.row_user_data))
	{
		xbaeFreeRowUserData(current);
		if(new->matrix.row_user_data)
			xbaeCopyRowUserData(new);
	}

	if(NE(matrix.column_shadow_types))
	{
		xbaeFreeColumnShadowTypes(current);
		if(new->matrix.column_shadow_types)
			xbaeCopyColumnShadowTypes(new);
		if(new->matrix.grid_type == XmGRID_COLUMN_SHADOW)
			redisplay = True;
	}
	if(NE(matrix.row_shadow_types))
	{
		xbaeFreeRowShadowTypes(current);
		if(new->matrix.row_shadow_types)
			xbaeCopyRowShadowTypes(new);
		if(new->matrix.grid_type == XmGRID_ROW_SHADOW)
			redisplay = True;
	}

	if(NE(matrix.column_labels) || NE(matrix.xmcolumn_labels))
	{
		if(new->matrix.column_labels || new->matrix.xmcolumn_labels)
		{
			xbaeCopyColumnLabels(new);
			new->matrix.column_label_maxlines =
				xbaeCalculateLabelMaxLines(new->matrix.column_labels,
						new->matrix.xmcolumn_labels, new->matrix.columns);
		}
		else
		{
			new->matrix.column_label_maxlines = 0;
		}
		xbaeFreeColumnLabels(current);
		redisplay = True;
	}
	if(NE(matrix.row_labels) || NE(matrix.xmrow_labels))
	{
		if(new->matrix.row_labels || new->matrix.xmrow_labels)
		{
			xbaeCopyRowLabels(new);
			new->matrix.row_label_maxwidth = xbaeCalculateLabelMaxWidth(new,
				  new->matrix.row_labels, new->matrix.xmrow_labels, new->matrix.rows);
		}
		else
		{
			new->matrix.row_label_maxwidth = 0;
		}
		xbaeFreeRowLabels(current);
		redisplay = True;
	}
	if(COLUMN_LABEL_HEIGHT(new) != COLUMN_LABEL_HEIGHT(current)
	   || ROW_LABEL_WIDTH(new) != ROW_LABEL_WIDTH(current))
	{
		relayout = True;
	}

	if(NE(matrix.column_widths))
	{
		xbaeFreeColumnWidths(current);
		if(new->matrix.column_widths)
			xbaeCopyColumnWidths(new);
		new_column_widths = True;
	}
	else
	{
		if(NE(matrix.column_width_in_pixels))
		{
			/* 
			 * Convert the measuremnt of column_widths from pixels to characters and vice versa
			 */
			int i;
			if(new->matrix.column_width_in_pixels == True)
			{
				for(i = 0; i < current->matrix.columns; i++)
				{
					new->matrix.column_widths[i] =
						new->matrix.column_widths[i] * CELL_FONT_WIDTH(new) + 2 * CELL_BORDER_WIDTH(new);
				}
			}
			else
			{
				for(i = 0; i < current->matrix.columns; i++)
				{
					new->matrix.column_widths[i] =
						(new->matrix.column_widths[i] - 2 * CELL_BORDER_WIDTH(new)) / CELL_FONT_WIDTH(new);
				}
			}

			new_column_widths = True;
		}

		if(NE(matrix.columns))
		{
			/* 
			 * The number of columns changed but no new widths were given
			 */
			new->matrix.column_widths =
				(short *) XtRealloc((XtPointer) new->matrix.column_widths,
									new->matrix.columns * sizeof(short));
			current->matrix.column_widths = NULL;
			if(new->matrix.columns > current->matrix.columns)
			{
				/* 
				 * There are new columns, init their width to the default 
				 */
				int i;
				for(i = current->matrix.columns; i < new->matrix.columns; i++)
				{
					new->matrix.column_widths[i] = DEFAULT_COLUMN_WIDTH(new);
				}
			}
			new_column_widths = True;
		}
	}

	if(NE(matrix.row_heights))
	{
		xbaeFreeRowHeights(current);
		if(new->matrix.row_heights)
			xbaeCopyRowHeights(new);
		new_row_heights = True;
	}
	else
	{
		if(NE(matrix.row_height_in_pixels))
		{
			/* 
			 * Convert the measuremnt of row_heights from pixels to lines and vice versa
			 */
			int i;
			if(new->matrix.row_height_in_pixels == True)
			{
				for(i = 0; i < current->matrix.rows; i++)
				{
					new->matrix.row_heights[i] =
						new->matrix.row_heights[i] * TEXT_HEIGHT(new) + 2 * CELL_BORDER_HEIGHT(new);
				}
			}
			else
			{
				for(i = 0; i < current->matrix.rows; i++)
				{
					new->matrix.row_heights[i] =
						(new->matrix.row_heights[i] - 2 * CELL_BORDER_HEIGHT(new)) / TEXT_HEIGHT(new);
				}
			}

			new_row_heights = True;
		}

		if(NE(matrix.rows))
		{
			/* 
			 * The number of rows changed but no new heights were given
			 */
			new->matrix.row_heights =
				(short *) XtRealloc((XtPointer) new->matrix.row_heights, new->matrix.rows * sizeof(short));
			current->matrix.row_heights = NULL;
			if(new->matrix.rows > current->matrix.rows)
			{
				/* 
				 * There are new rows, init their height to the default 
				 */
				int i;
				for(i = current->matrix.rows; i < new->matrix.rows; i++)
				{
					new->matrix.row_heights[i] = DEFAULT_ROW_HEIGHT(new);
				}
			}
			new_row_heights = True;
		}
	}

	/*
	 * If the number of rows or columns changed, we need to allocate new arrays for 
	 * the private state
	 */
	if(NE(matrix.rows) || NE(matrix.columns))
	{
		ResizePerCell(current, new);

		if(NE(matrix.rows))
		{
			xbaeFreeRowPositions(current);
			new->matrix.row_positions = CreateRowPositions(new);
		}

		if(NE(matrix.columns))
		{
			xbaeFreeColumnPositions(current);
			new->matrix.column_positions = CreateColumnPositions(new);
		}
	}

	/*
	 * If cell heights or widths changed recalculate the positions, relayout and redisplay
	 */
	if(new_row_heights
	   || (!new->matrix.row_height_in_pixels
		   && (TEXT_HEIGHT(new) != TEXT_HEIGHT(current)
			   || NE(matrix.cell_margin_height)
			   || NE(matrix.cell_highlight_thickness)
			   || NE(matrix.cell_shadow_thickness) || NE(matrix.text_shadow_thickness))))
	{

		xbaeGetRowPositions(new);
		redisplay = relayout = True;
	}

	if(new_column_widths
	   || (!new->matrix.column_width_in_pixels
		   && (CELL_FONT_WIDTH(new) != CELL_FONT_WIDTH(current)
			   || NE(matrix.cell_margin_width)
			   || NE(matrix.cell_highlight_thickness)
			   || NE(matrix.cell_shadow_thickness) || NE(matrix.text_shadow_thickness))))
	{

		xbaeGetColumnPositions(new);
		redisplay = relayout = True;
	}

	if(NE(matrix.rows)
	   || NE(matrix.columns)
	   || NE(matrix.fixed_rows)
	   || NE(matrix.fixed_columns) || NE(matrix.trailing_fixed_rows) || NE(matrix.trailing_fixed_columns))
	{
		relayout = True;
	}

	/*
	 * If our fill policy or an attachment changed redisplay and relayout.
	 */
	if(NE(matrix.fill) || NE(matrix.vert_fill) || NE(matrix.horz_fill)
	   || NE(matrix.non_fixed_detached_top) || NE(matrix.non_fixed_detached_left)
	   || NE(matrix.trailing_attached_bottom) || NE(matrix.trailing_attached_right))
	{
		redisplay = relayout = True;
	}

	/*
	 * If what's arround the matrix changed relayout.
	 */
	if(NE(matrix.scrollbar_placement)
	   || NE(matrix.vsb_display_policy)
	   || NE(matrix.hsb_display_policy) || NE(matrix.space) || NE(manager.shadow_thickness))
		relayout = True;

	/*
	 * Compute a new size if:
	 *   visible_rows or visible_columns changed.
	 *   user set our width or height to zero.
	 */
#if 1
	/* Cheap trick that works, provided by arcad.de */
	if(NE(core.width))
		new->matrix.visible_columns = 0;
	if(NE(core.height))
		new->matrix.visible_rows = 0;
#endif
	if(NE(matrix.visible_rows) || NE(matrix.visible_columns)
	   || request->core.height == 0 || request->core.width == 0)
		xbaeComputeSize(new, request->core.width == 0, request->core.height == 0);

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) new, "SetValues redisplay is %s relayout is %s\n",
						redisplay ? "True" : "False", relayout ? "True" : "False"));

	/*
	 * Do things that only make sense post realize
	 */
	if(XtIsRealized((Widget) new))
	{
		/*
		 * Hide/update the textChild if needed
		 */
		if(hide_text_child)
		{
			xbaeHideTextChild(new);
		}
		else if(update_text_child && new->matrix.text_child_is_mapped)
		{
			xbaeUpdateTextChild(new, False);
		}

		/*
		 * Changes to the GCs
		 */
		if(XtIsSensitive((Widget) current) != XtIsSensitive((Widget) new))
		{
			unsigned long valuemask = GCFillStyle;
			XGCValues values;
			Display *dpy = XtDisplay(new);

			values.fill_style = (XtIsSensitive((Widget) new)) ? FillSolid : FillStippled;

			XChangeGC(dpy, new->matrix.draw_gc, valuemask, &values);
			XChangeGC(dpy, new->matrix.pixmap_gc, valuemask, &values);
			XChangeGC(dpy, new->matrix.label_gc, valuemask, &values);

			redisplay = True;
		}
		if(NE(matrix.label_font.id) && new->matrix.label_font.type == XmFONT_IS_FONT)
		{
			XSetFont(XtDisplay(new), new->matrix.label_gc, new->matrix.label_font.id);
			redisplay = True;
		}

		if(NE(matrix.grid_line_color))
		{
			xbaeGetGridLineGC(new);
		}
		if(NE(manager.foreground) || NE(manager.top_shadow_color) || NE(manager.top_shadow_pixmap))
		{
			xbaeGetResizeTopShadowGC(new);
		}
		if(NE(manager.foreground) || NE(manager.bottom_shadow_color) || NE(manager.bottom_shadow_pixmap))
		{
			xbaeGetResizeBottomShadowGC(new);
		}

		/*
		 * Force the Clip widget to redisplay.  Note: this may generate an
		 * expose event for the current size of the Clip widget, and the Clip
		 * widget may be sized smaller in set_values_almost.  The ClipRedisplay
		 * function can handle this case.
		 */
		if(redisplay && !new->matrix.disable_redisplay)
		{
			xbaeRefresh(new, relayout && EQ(core.width) && EQ(core.height));
			relayout = False;
		}
	}

	if(relayout && EQ(core.width) && EQ(core.height))
	{
		/*
		 * If our size didn't change, but we need to layout, call Relayout.
		 * If our size did change, then Xt will call our Resize method for us.
		 * If our size did change, but the new size is later refused,
		 *   then SetValuesAlmost will call Resize to layout.
		 *
		 * JDS: Don't need to force a redisplay on a relayout, since the Clip
		 * widget's resize method (now non-NULL) will be called and Xt will
		 * automatically do an expose after that occurs. Seems to work, anyways :).
		 */
		xbaeRelayout(new);
	}

	/*
	 * We want to return True when we need to redisplay or relayout.
	 */
	return redisplay || relayout;

#undef NE
#undef EQ
}

/* ARGSUSED */
static void
SetValuesAlmost(XbaeMatrixWidget old, XbaeMatrixWidget new, XtWidgetGeometry * request,
				XtWidgetGeometry * reply)
{
	/*
	 * If XtGeometryAlmost, accept compromize - Resize will take care of it
	 */
	if(reply->request_mode)
	{
		*request = *reply;

#if XtSpecificationRelease > 4
		/*
		 * In R5, XtSetValues changed so that when a widgets parent
		 * returns XtGeometryAlmost, Xt will only call the widgets resize
		 * method if the widgets size actually changed.  It turns out that
		 * some manager widgets (old Wcl XmpTable and 1.1.x XmForm) return
		 * XtGeometryAlmost with a compromise size which is the widgets
		 * original size (not much of a compromise)!  This means as of R5,
		 * Matrix's resize method won't get called in that case.
		 *
		 * So, for R5 we explicitly call our relayout method here for the
		 * case of XtGeometryAlmost where our size did not change.
		 */
		if((reply->request_mode & CWWidth || reply->request_mode & CWHeight)
		   && (old->core.width == new->core.width && old->core.height == new->core.height))
			xbaeRelayout(new);
#endif
	}

	/*
	 * If XtGeometryNo, call Relayout if it was a size change that was denied.
	 * Accept the original geometry.
	 * (we need to force a Relayout even though the size didn't 
	 * change - set_values relies on this)
	 */
	else
	{
		if((request->request_mode & CWWidth || request->request_mode & CWHeight))
			xbaeRelayout(new);

		request->request_mode = 0;
	}
}

static void Destroy(XbaeMatrixWidget mw)
{
	int n;

	for(n = 0; n < mw->matrix.xft_draw_cache_len; n++)
		XftDrawDestroy(mw->matrix.xft_draw_cache[n].draw);
	XtFree((void *) mw->matrix.xft_draw_cache);

	XtFree((void *) mw->matrix.xft_color_cache);

	/* SGO: just delete GCs when set. otherwise crashes could be produced */
	if(mw->matrix.label_gc)
	{
		XFreeGC(XtDisplay(mw), mw->matrix.label_gc);
		mw->matrix.label_gc = NULL;
	}
	if(mw->matrix.draw_gc)
	{
		XFreeGC(XtDisplay(mw), mw->matrix.draw_gc);
		mw->matrix.draw_gc = NULL;
	}
	if(mw->matrix.pixmap_gc)
	{
		XFreeGC(XtDisplay(mw), mw->matrix.pixmap_gc);
		mw->matrix.pixmap_gc = NULL;
	}

	if(mw->matrix.grid_line_gc)
	{
		XtReleaseGC((Widget) mw, mw->matrix.grid_line_gc);
		mw->matrix.grid_line_gc = NULL;
	}
	if(mw->matrix.resize_top_shadow_gc)
	{
		XtReleaseGC((Widget) mw, mw->matrix.resize_top_shadow_gc);
		mw->matrix.resize_top_shadow_gc = NULL;
	}
	if(mw->matrix.resize_bottom_shadow_gc)
	{
		XtReleaseGC((Widget) mw, mw->matrix.resize_bottom_shadow_gc);
		mw->matrix.resize_bottom_shadow_gc = NULL;
	}

	xbaeFreeRowLabels(mw);
	xbaeFreeRowHeights(mw);
	xbaeFreeRowPositions(mw);
	xbaeFreeRowButtonLabels(mw);
	xbaeFreeRowUserData(mw);
	xbaeFreeRowShadowTypes(mw);
	xbaeFreeRowLabelBackgrounds(mw);
	xbaeFreeRowLabelForegrounds(mw);

	xbaeFreeColumnLabels(mw);
	xbaeFreeColumnWidths(mw);
	xbaeFreeColumnPositions(mw);
	xbaeFreeColumnButtonLabels(mw);
	xbaeFreeColumnUserData(mw);
	xbaeFreeColumnShadowTypes(mw);
	xbaeFreeColumnLabelBackgrounds(mw);
	xbaeFreeColumnLabelForegrounds(mw);

	xbaeFreeColumnMaxLengths(mw);
	xbaeFreeColumnAlignments(mw);
	xbaeFreeColumnLabelAlignments(mw);
	xbaeFreeShowColumnArrows(mw);
	xbaeFreeColumnFontBold(mw);

	xbaeFreePerCell(mw);
	XmRenderTableFree(mw->matrix.render_table);
	mw->matrix.render_table = NULL;
}

/*
 * Since we totally control our childrens geometry, allow anything.
 */

/* ARGSUSED */
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry * desired, XtWidgetGeometry * allowed)
{
#define Wants(flag) (desired->request_mode & flag)

	DEBUGOUT(_XbaeDebug(__FILE__, w, "GeometryManager\n"));

	if(Wants(XtCWQueryOnly))
		return (XtGeometryYes);

	if(Wants(CWWidth))
		w->core.width = desired->width;
	if(Wants(CWHeight))
		w->core.height = desired->height;
	if(Wants(CWX))
		w->core.x = desired->x;
	if(Wants(CWY))
		w->core.y = desired->y;
	if(Wants(CWBorderWidth))
		w->core.border_width = desired->border_width;

	return (XtGeometryYes);

#undef Wants
}

/*
 * We would prefer to be the size calculated in ComputeSize and saved in
 * desired_width/height
 */
static XtGeometryResult
QueryGeometry(XbaeMatrixWidget mw, XtWidgetGeometry * proposed, XtWidgetGeometry * desired)
{
#define Set(bit) (proposed->request_mode & bit)

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) mw, "QueryGeometry\n"));

	desired->width = mw->matrix.desired_width;
	desired->height = mw->matrix.desired_height;
	desired->request_mode = CWWidth | CWHeight;

	if(Set(CWWidth) && proposed->width == desired->width
	   && Set(CWHeight) && proposed->height == desired->height)
		return (XtGeometryYes);

	if(desired->width == mw->core.width && desired->height == mw->core.height)
		return (XtGeometryNo);

	return (XtGeometryAlmost);

#undef Set
}

/*
 * Add rows/columns of per-cell flags when set_values changes our rows/columns
 */
static void ResizePerCell(XbaeMatrixWidget current, XbaeMatrixWidget new)
{
	int i, j;
	int safe_rows = 0;

	DEBUGOUT(_XbaeDebug(__FILE__, (Widget) current, "ResizePerCell (%d,%d) -> (%d,%d)\n",
						current->matrix.rows, current->matrix.columns,
						new->matrix.rows, new->matrix.columns));

	if(!new->matrix.per_cell)
		return;

	if(new->matrix.rows > current->matrix.rows)
	{
		/*
		 * Adding rows
		 */
		new->matrix.per_cell =
			(XbaeMatrixPerCellRec **) XtRealloc((XtPointer) new->matrix.per_cell,
												new->matrix.rows * sizeof(XbaeMatrixPerCellRec *));

		/*
		 * Calloc a new row array for each row. Use the new column size.
		 */
		for(i = current->matrix.rows; i < new->matrix.rows; i++)
		{
			new->matrix.per_cell[i] =
				(XbaeMatrixPerCellRec *) XtMalloc(new->matrix.columns * sizeof(XbaeMatrixPerCellRec));
			for(j = 0; j < new->matrix.columns; j++)
				xbaeFill_WithEmptyValues_PerCell(new, &new->matrix.per_cell[i][j]);
		}

		safe_rows = current->matrix.rows;
	}
	else if(new->matrix.rows < current->matrix.rows)
	{
		/*
		 * Deleting rows
		 */
		for(i = new->matrix.rows; i < current->matrix.rows; i++)
			xbaeFreePerCellRow(new, i);
		safe_rows = new->matrix.rows;
	}
	else
	{
		safe_rows = new->matrix.rows;
	}

	if(new->matrix.columns > current->matrix.columns)
	{
		/*
		 * Adding columns
		 */
		/*
		 * Realloc each row array. Do not touch any rows added/deleted above
		 * (use safe_rows)
		 */
		for(i = 0; i < safe_rows; i++)
		{
			int j;
			new->matrix.per_cell[i] =
				(XbaeMatrixPerCellRec *) XtRealloc((char *) new->matrix.per_cell[i],
												   new->matrix.columns * sizeof(XbaeMatrixPerCellRec));
			for(j = current->matrix.columns; j < new->matrix.columns; j++)
				xbaeFill_WithEmptyValues_PerCell(new, &new->matrix.per_cell[i][j]);
		}
	}

	/*
	 * Deleting columns
	 *   if (new->matrix.columns < current->matrix.columns)
	 * We don't bother to realloc, just leave some wasted space.
	 * XXX is this a problem?
	 */
}


/* XmRCallProc routine for checking RenderTable. If "check_set_render_table"
 * is True, then function has been called twice on same widget, thus resource
 * needs to be set NULL, otherwise leave it alone.
 */
/*ARGSUSED*/
static void CheckSetRenderTable(Widget w, int offset, XrmValue *value)
{
	XbaeMatrixWidget wm = (XbaeMatrixWidget) w;

	if (wm->matrix.check_set_render_table)
	{
		value->addr = NULL;
	}
	else
	{
		wm->matrix.check_set_render_table = True;
		value->addr = (char*)&(wm->matrix.render_table);
	}
}


#ifdef	WIN32
#  ifdef	USING_EXCEED
/*
 * Exceed has a somewhat strange environment for DLL's : they require the application
 * to call some DLL initialisation function (i.e. porting to Exceed requires a source
 * change).
 *
 * In analogy to their approach with e.g. HCLXawInit() and HCLXtInit(), we now add a
 * function called HCLXbaeInit() to be called prior to any Xbae action.
 * In other Windows based environments, this doesn't appear to be necessary.
 *
 * Thanks to Karthik Rajagopalan for figuring this out for me.
 */
export void HCLXbaeInit(void)
{
	HCLFixXtPointers(_XtInherit, XtInheritTranslations);
}
#  else
/*
 * A Windows-based environment other than Exceed.
 */
int __stdcall DllMain(unsigned long mod_handle, unsigned long flag, void *routine)
{
	switch (flag)
	{
		case 1:				/* DLL_PROCESS_ATTACH - process attach */
			/* FIX ME What should happen here ? */
			break;
		case 0:				/* DLL_PROCESS_DETACH - process detach */
			break;
	}
	return 1;
}
#  endif
#endif
