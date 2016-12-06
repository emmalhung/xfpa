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
 * MatrixWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 *
 * $Id: Matrix.h,v 1.47 2005/07/31 18:35:31 tobiasoed Exp $
 */
/*
 * Modifications - Environment Canada
 *
 * 2005.10.31 - Added resouces to modify the background and foreground
 *              colour of the individual row and column button labels.
 *
 *              XmNcolumnLabelForegrounds
 *              XmNcolumnLabelBackgrounds
 *              XmNrowLabelForegrounds
 *              XmNrowLabelBackgrounds
 */

#ifndef _Xbae_Matrix_h
#  define _Xbae_Matrix_h

/*
 * Matrix Widget public include file
 */

#  include <Xm/Xm.h>
#  include <X11/Core.h>

#  include <Xbae/patchlevel.h>

#  ifdef __cplusplus
extern "C" {
#  endif

/* get version info */
	extern const char *XbaeGetVersionTxt(void);
	extern int XbaeGetVersionNum(void);
	extern const char *XbaeGetXmVersionTxt(void);
	extern int XbaeGetXmVersionNum(void);


/* Resources:
 * Name							Class					RepType				Default Value
 * ----							-----					-------				-------------
 * allowColumnResize			AllowResize				Boolean				False
 * allowRowResize				AllowResize				Boolean				False
 * altRowCount					AltRowCount				int					1
 * boldLabels					BoldLabels				Boolean				False
 * buttonLabels					ButtonLabels			Boolean				False
 * buttonLabelBackground		Color					Pixel				dynamic
 * calcCursorPosition			CalcCursorPosition		Boolean				False
 * cellBackgrounds				Colors					PixelTable			NULL
 * cellRenditionTag				String					String				"cells"
 * cellHighlightThickness		HighlightThickness		HorizontalDimension 2
 * cellMarginHeight				MarginHeight			VerticalDimension   5
 * cellMarginWidth				MarginWidth				HorizontalDimension 5
 * cells						Cells					StringTable			NULL
 * cellShadowThickness			ShadowThickness			Dimension			2
 * cellShadowType				ShadowType				unsigned char		SHADOW_OUT
 * cellShadowTypes				CellShadowTypes			ShadowTypeTable		NULL
 * cellUserData					CellUserData			UserDataTable		NULL
 * clipWindow					XmCClipWindow			Widget				NULL (get only)
 * colors						Colors					PixelTable			NULL
 * columnAlignments				Alignments				AlignmentArray		dynamic
 * columnButtonLabels			ButtonLabels			BooleanArray		NULL
 * columnLabelBackgrounds		LabelColors				PixelArray			NULL
 * columnLabelForegrounds		LabelColors				PixelArray			NULL
 * columnFontBold				ButtonLabels			BooleanArray		NULL
 * columnLabelAlignments		Alignments				AlignmentArray		dynamic
 * columnLabelColor				Color					Pixel				dynamic
 * columnLabels					Labels					StringArray			NULL
 * xmColumnLabels				XmLabels				XmStringTable		NULL
 * columnMaxLengths				ColumnMaxLengths		MaxLengthArray		NULL
 * columnShadowTypes			ShadowTypes				ShadowTypeArray		NULL
 * columnUserData				UserDatas				UserDataArray		NULL
 * columnWidths					ColumnWidths			WidthArray			NULL
 * columnWidthInPixels			ColumnWidthInPixels     Boolean				False
 * rowHeightInPixels			RowHeightInPixels       Boolean				True
 * columns						Columns					int					0
 * defaultActionCallback		Callback				Callback			NULL
 * doubleClickInterval			Interval                int					dynamic
 * drawCellCallback				Callback				Callback			NULL
 * enterCellCallback			Callback				Callback			NULL
 * evenRowBackground			Background				Pixel				dynamic
 * fill							Fill					Boolean				False
 * vertFill						Fill                    Boolean				False
 * horzFill						Fill                    Boolean				False
 * fixedColumns					FixedColumns			Dimension			0
 * fixedRows					FixedRows				Dimension			0
 * fontList						FontList				FontList			fixed
 * labelFont					FontList				FontList			fixed
 * gridLineColor				Color					Pixel				dynamic
 * gridType						GridType				GridType			XmGRID_CELL_LINE
 * highlightedCells				HighlightedCells		HighlightTable		dynamic
 * horizonalScrollBar			HorizonalScrollBar		Widget				NULL (get only)
 * horizontalScrollBarDisplayPolicy
 *							XmCMatrixScrollBarDisplayPolicy
 *														unsigned char		AS_NEEDED
 * labelActivateCallback		Callback				Callback			NULL
 * labelRenditionTag					String					String				"labels"
 * leaveCellCallback			Callback				Callback			NULL
 * leftColumn					LeftColumn              int					0
 * modifyVerifyCallback			Callback				Callback			NULL
 * multiLineCell				MultiLineCell			Boolean				False
 * oddRowBackground				Background				Pixel				NULL
 * processDragCallback			Callback				Callback			NULL
 * resizeCallback				Callback				Callback			NULL
 * resizeColumnCallback			Callback				Callback			NULL
 * resizeRowCallback			Callback				Callback			NULL
 * reverseSelect				reverseSelect			Boolean				False
 * rowButtonLabels				ButtonLabels			BooleanArray		NULL
 * rowLabelBackgrounds			LabelColors				PixelArray			NULL
 * rowLabelForegrounds			LabelColors				PixelArray			NULL
 * rowHeights					RowHeights				WidthArray			NULL
 * rowLabelAlignment			Alignment				Alignment			XmALIGNMENT_END
 * rowLabelColor				Color					Pixel				dynamic
 * rowLabelWidth				RowLabelWidth			Short				dynamic
 * rowLabels					Labels					StringArray			NULL
 * rowShadowTypes				ShadowTypes				ShadowTypeArray		NULL
 * rowUserData					UserDatas				UserDataArray		NULL
 * rows							Rows					int					0
 * selectCellCallback			Callback				Callback			NULL
 * selectedBackground			Color					Pixel				dynamic
 * selectedCells				SelectedCells			BooleanTable		dynamic
 * selectedForeground			Color					Pixel				dynamic
 * selectScrollVisible			SelectScrollVisible		Boolean				True
 * space						Space					Dimension			6
 * shadowType					ShadowType				unsigned char		SHADOW_IN
 * textBackground				Backgound				Pixel				dynamic
 * textField					TextField				Widget				NULL (get only)
 * textShadowThickness			TextShadowThickness		Dimension			0
 * textTranslations				Translations			TranslationTable	dynamic
 * topRow						TopRow					int					0
 * trailingFixedColumns			TrailingFixedColumns	Dimension			0
 * trailingFixedRows			TrailingFixedRows		Dimension			0
 * traverseCellCallback			Callback				Callback			NULL
 * traverseFixedCells			TraverseFixedCells		Boolean				False
 * underlinedCells				UnderlinedCells			BooleanTable		dynamic
 * underlinePosition			UnderlinePosition		Position			1
 * underlineWidth				UnderlineWidth			Dimension			1
 * useXbaeInput					UseXbaeInput			Boolean				True
 * valueChangedCallback			Callback                Callback			NULL
 * verticalScrollBar			VerticalScrollBar		Widget				NULL (get only)
 * verticalScrollBarDisplayPolicy
 *							XmCMatrixScrollBarDisplayPolicy
 *														unsigned char		AS_NEEDED
 * visibleColumns				VisibleColumns			Dimension			0
 * visibleRows					VisibleRows				Dimension			0
 * writeCellCallback			Callback				Callback			NULL
 */

#  ifndef XmNallowColumnResize
#    define XmNallowColumnResize		"allowColumnResize"
#  endif

#  ifndef XmNallowRowResize
#    define XmNallowRowResize		"allowRowResize"
#  endif

#  ifndef XmNaltRowCount
#    define XmNaltRowCount			"altRowCount"
#  endif

#  ifndef XmNautomaticColumnResize
#    define XmNautomaticColumnResize	"automaticColumnResize"
#  endif

#  ifndef XmNboldLabels
#    define XmNboldLabels			"boldLabels"
#  endif

#  ifndef XmNbuttonLabels
#    define XmNbuttonLabels			"buttonLabels"
#  endif

#  ifndef XmNbuttonLabelBackground
#    define XmNbuttonLabelBackground	"buttonLabelBackground"
#  endif

#  ifndef XmNcalcCursorPosition
#    define XmNcalcCursorPosition		"calcCursorPosition"
#  endif

#  ifndef XmNcellBackgrounds
#    define XmNcellBackgrounds		"cellBackgrounds"
#  endif

#  ifndef XmNcellRenditionTag
#    define XmNcellRenditionTag		"cellRenditionTag"
#  endif

#  ifndef XmNlabelRenditionTag
#    define XmNlabelRenditionTag	"labelRenditionTag"
#  endif

#  ifndef XmNcellHighlightThickness
#    define XmNcellHighlightThickness	"cellHighlightThickness"
#  endif

#  ifndef XmNcellMarginHeight
#    define XmNcellMarginHeight		"cellMarginHeight"
#  endif

#  ifndef XmNcellMarginWidth
#    define XmNcellMarginWidth		"cellMarginWidth"
#  endif

#  ifndef XmNcellShadowType
#    define XmNcellShadowType		"cellShadowType"
#  endif

#  ifndef XmNcellShadowTypes
#    define XmNcellShadowTypes		"cellShadowTypes"
#  endif

#  ifndef XmNcellShadowThickness
#    define XmNcellShadowThickness		"cellShadowThickness"
#  endif

#  ifndef XmNcellUserData
#    define XmNcellUserData			"cellUserData"
#  endif

#  ifndef XmNcellWidgets
#    define XmNcellWidgets			"cellWidgets"
#  endif

#  ifndef XmNcells
#    define XmNcells			"cells"
#  endif

#  ifndef XmNcolors
#    define XmNcolors			"colors"
#  endif

#  ifndef XmNcolumnAlignments
#    define XmNcolumnAlignments		"columnAlignments"
#  endif

#  ifndef XmNcolumnButtonLabels
#    define XmNcolumnButtonLabels		"columnButtonLabels"
#  endif

#  ifndef XmNcolumnLabelBackgrounds
#    define XmNcolumnLabelBackgrounds	"columnLabelBackgrounds"
#  endif

#  ifndef XmNcolumnLabelForegrounds
#    define XmNcolumnLabelForegrounds	"columnLabelForegrounds"
#  endif

#  ifndef XmNcolumnFontBold
#    define XmNcolumnFontBold		"columnFontBold"
#  endif

#  ifndef XmNshowColumnArrows
#    define XmNshowColumnArrows		"showColumnArrows"
#  endif

#  ifndef XmNcolumnLabelAlignments
#    define XmNcolumnLabelAlignments	"columnLabelAlignments"
#  endif

#  ifndef XmNcolumnLabelBackground
#    define XmNcolumnLabelBackground	"columnLabelBackground"
#  endif

#  ifndef XmNcolumnLabelColor
#    define XmNcolumnLabelColor		"columnLabelColor"
#  endif

#  ifndef XmNcolumnLabels
#    define XmNcolumnLabels			"columnLabels"
#  endif

#  ifndef XmNxmColumnLabels
#    define XmNxmColumnLabels		"xmColumnLabels"
#  endif

#  ifndef XmNXmColumnLabels		/* For backwards compatibility */
#    define XmNXmColumnLabels		XmNxmColumnLabels
#  endif

#  ifndef XmNxmRowLabels
#    define XmNxmRowLabels		"xmRowLabels"
#  endif

#  ifndef XmNcolumnMaxLengths
#    define XmNcolumnMaxLengths		"columnMaxLengths"
#  endif

#  ifndef XmNcolumnShadowTypes
#    define XmNcolumnShadowTypes		"columnShadowTypes"
#  endif

#  ifndef XmNcolumnUserData
#    define XmNcolumnUserData		"columnUserData"
#  endif

#  ifndef XmNcolumnWidths
#    define XmNcolumnWidths			"columnWidths"
#  endif

#  ifndef XmNcolumnWidthInPixels
#    define XmNcolumnWidthInPixels         "columnWidthInPixels"
#  endif

#  ifndef XmNrowHeightInPixels
#    define XmNrowHeightInPixels           "rowHeightInPixels"
#  endif

#  ifndef XmNdrawCellCallback
#    define XmNdrawCellCallback		"drawCellCallback"
#  endif

#  ifndef XmNenterCellCallback
#    define XmNenterCellCallback		"enterCellCallback"
#  endif

#  ifndef XmNevenRowBackground
#    define XmNevenRowBackground		"evenRowBackground"
#  endif

#  ifndef XmNfill
#    define XmNfill				"fill"
#  endif

#  ifndef XmNvertFill
#    define XmNvertFill			"vertFill"
#  endif

#  ifndef XmNhorzFill
#    define XmNhorzFill			"horzFill"
#  endif

#  ifndef XmNfixedColumns
#    define XmNfixedColumns			"fixedColumns"
#  endif

#  ifndef XmNfixedRows
#    define XmNfixedRows			"fixedRows"
#  endif

#  ifndef XmNgridLineColor
#    define XmNgridLineColor		"gridLineColor"
#  endif

#  ifndef XmNgridType
#    define XmNgridType			"gridType"
#  endif

#  ifndef XmNhighlightedCells
#    define XmNhighlightedCells		"highlightedCells"
#  endif

#  ifndef XmNhorizontalScrollBarDisplayPolicy
#    define XmNhorizontalScrollBarDisplayPolicy "horizontalScrollBarDisplayPolicy"
#  endif

#  ifndef XmNlabelActivateCallback
#    define XmNlabelActivateCallback	"labelActivateCallback"
#  endif

#  ifndef XmNlabelFont
#    define XmNlabelFont			"labelFont"
#  endif

#  ifndef XmNleaveCellCallback
#    define XmNleaveCellCallback		"leaveCellCallback"
#  endif

#  ifndef XmNleftColumn
#    define XmNleftColumn			"leftColumn"
#  endif

#  ifndef XmNoddRowBackground
#    define XmNoddRowBackground		"oddRowBackground"
#  endif

#  ifndef XmNprocessDragCallback
#    define XmNprocessDragCallback		"processDragCallback"
#  endif

#  ifndef XmNresizeCallback
#    define XmNresizeCallback               "resizeCallback"
#  endif

#  ifndef XmNresizeRowCallback
#    define XmNresizeRowCallback		"resizeRowCallback"
#  endif

#  ifndef XmNresizeColumnCallback
#    define XmNresizeColumnCallback		"resizeColumnCallback"
#  endif

#  ifndef XmNreverseSelect
#    define XmNreverseSelect		"reverseSelect"
#  endif

#  ifndef XmNrowButtonLabels
#    define XmNrowButtonLabels		"rowButtonLabels"
#  endif

#  ifndef XmNrowLabelBackgrounds
#    define XmNrowLabelBackgrounds	"rowLabelBackgrounds"
#  endif

#  ifndef XmNrowLabelForegrounds
#    define XmNrowLabelForegrounds	"rowLabelForegrounds"
#  endif

#  ifndef XmNrowHeights
#    define XmNrowHeights			"rowHeights"
#  endif

#  ifndef XmNrowLabelAlignment
#    define XmNrowLabelAlignment		"rowLabelAlignment"
#  endif

#  ifndef XmNrowLabelWidth
#    define XmNrowLabelWidth		"rowLabelWidth"
#  endif

#  ifndef XmNrowLabelBackground
#    define XmNrowLabelBackground		"rowLabelBackground"
#  endif

#  ifndef XmNrowLabelColor
#    define XmNrowLabelColor		"rowLabelColor"
#  endif

#  ifndef XmNrowLabels
#    define XmNrowLabels			"rowLabels"
#  endif

#  ifndef XmNrowShadowTypes
#    define XmNrowShadowTypes		"rowShadowTypes"
#  endif

#  ifndef XmNrowUserData
#    define XmNrowUserData			"rowUserData"
#  endif

#  ifndef	XmNscrollBackground
#    define	XmNscrollBackground		"scrollBackground"
#  endif

#  ifndef XmNselectedCells
#    define XmNselectedCells		"selectedCells"
#  endif

#  ifndef XmNselectedBackground
#    define XmNselectedBackground		"selectedBackground"
#  endif

#  ifndef XmNselectCellCallback
#    define XmNselectCellCallback		"selectCellCallback"
#  endif

#  ifndef XmNselectedForeground
#    define XmNselectedForeground		"selectedForeground"
#  endif

#  ifndef XmNselectScrollVisible
#    define XmNselectScrollVisible		"selectScrollVisible"
#  endif

#  ifndef XmNtextBackground
#    define XmNtextBackground		"textBackground"
#  endif

#  ifndef XmNtextBackgroundIsCell
#    define XmNtextBackgroundIsCell		"textBackgroundIsCell"
#  endif

#  ifndef XmNtextField
#    define XmNtextField			"textField"
#  endif

#  ifndef XmNtopRow
#    define XmNtopRow			"topRow"
#  endif

#  ifndef XmNnonFixedDetachedTop
#    define XmNnonFixedDetachedTop	"nonFixedDetachedTop"
#  endif

#  ifndef XmNnonFixedDetachedLeft
#    define XmNnonFixedDetachedLeft	"nonFixedDetachedLeft"
#  endif

#  ifndef XmNtrailingAttachedBottom
#    define XmNtrailingAttachedBottom	"trailingAttachedBottom"
#  endif

#  ifndef XmNtrailingAttachedRight
#    define XmNtrailingAttachedRight	"trailingAttachedRight"
#  endif

#  ifndef XmNtrailingFixedColumns
#    define XmNtrailingFixedColumns		"trailingFixedColumns"
#  endif

#  ifndef XmNtrailingFixedRows
#    define XmNtrailingFixedRows		"trailingFixedRows"
#  endif

#  ifndef XmNleftColumn
#    define XmNleftColumn			"leftColumn"
#  endif

#  ifndef XmNtextShadowThickness
#    define XmNtextShadowThickness		"textShadowThickness"
#  endif

#  ifndef XmNtraverseCellCallback
#    define XmNtraverseCellCallback		"traverseCellCallback"
#  endif

#  ifndef XmNtraverseFixedCells
#    define XmNtraverseFixedCells		"traverseFixedCells"
#  endif

#  ifndef XmNunderlinedCells
#    define XmNunderlinedCells		"underlinedCells"
#  endif

#  ifndef XmNunderlinePosition
#    define XmNunderlinePosition		"underlinePosition"
#  endif

#  ifndef XmNunderlineWidth
#    define XmNunderlineWidth		"underlineWidth"
#  endif

#  ifndef XmNvalueChangedCallback
#    define XmNvalueChangedCallback               "valueChangedCallback"
#  endif

#  ifndef XmNverticalScrollBarDisplayPolicy
#    define XmNverticalScrollBarDisplayPolicy "verticalScrollBarDisplayPolicy"
#  endif

#  ifndef XmNvisibleColumns
#    define XmNvisibleColumns		"visibleColumns"
#  endif

#  ifndef XmNvisibleRows
#    define XmNvisibleRows			"visibleRows"
#  endif

#  ifndef XmNwriteCellCallback
#    define XmNwriteCellCallback		"writeCellCallback"
#  endif

#  ifndef	XmNtrackCellCallback
#    define	XmNtrackCellCallback		"trackCellCallback"
#  endif

#  ifndef XmCAlignments
#    define XmCAlignments			"Alignments"
#  endif

#  ifndef XmCAltRowCount
#    define XmCAltRowCount			"AltRowCount"
#  endif

#  ifndef XmCBoldLabels
#    define XmCBoldLabels			"BoldLabels"
#  endif

#  ifndef XmCButtonLabels
#    define XmCButtonLabels			"ButtonLabels"
#  endif

#  ifndef XmCLabelColors
#    define XmCLabelColors	"LabelColors"
#  endif

#  ifndef XmCCalcCursorPosition
#    define XmCCalcCursorPosition		"CalcCursorPosition"
#  endif

#  ifndef XmCCells
#    define XmCCells			"Cells"
#  endif

#  ifndef XmCCellShadowTypes
#    define XmCCellShadowTypes		"CellShadowTypes"
#  endif

#  ifndef XmCCellUserData
#    define XmCCellUserData			"CellUserData"
#  endif

#  ifndef XmCCellWidgets
#    define XmCCellWidgets			"CellWidgets"
#  endif

#  ifndef XmCColors
#    define XmCColors			"Colors"
#  endif

#  ifndef XmCColumnMaxLengths
#    define XmCColumnMaxLengths		"ColumnMaxLengths"
#  endif

#  ifndef XmCColumnResize
#    define XmCColumnResize			"ColumnResize"
#  endif

#  ifndef XmCAllowResize
#    define XmCAllowResize			"ColumnResize"
#  endif

#  ifndef XmCColumnWidths
#    define XmCColumnWidths			"ColumnWidths"
#  endif

#  ifndef XmCColumnWidthInPixels
#    define XmCColumnWidthInPixels		"ColumnWidthInPixels"
#  endif

#  ifndef XmCRowHeightInPixels
#    define XmCRowHeightInPixels		"RowHeightInPixels"
#  endif

#  ifndef XmCFill
#    define XmCFill				"Fill"
#  endif

#  ifndef XmCVertFill
#    define XmCVertFill			"VertFill"
#  endif

#  ifndef XmCHorzFill
#    define XmCHorzFill			"HorzFill"
#  endif

#  ifndef XmCFixedColumns
#    define XmCFixedColumns			"FixedColumns"
#  endif

#  ifndef XmCFixedRows
#    define XmCFixedRows			"FixedRows"
#  endif

#  ifndef XmCGridType
#    define XmCGridType			"GridType"
#  endif

#  ifndef XmCHighlightedCells
#    define XmCHighlightedCells		"HighlightedCells"
#  endif

#  ifndef XmCLabels
#    define XmCLabels			"Labels"
#  endif

#  ifndef XmCXmLabels
#    define XmCXmLabels			"XmLabels"
#  endif

#  ifndef XmCLeftColumn
#    define XmCLeftColumn			"LeftColumn"
#  endif

#  ifndef XmCMatrixScrollBarDisplayPolicy
#    define XmCMatrixScrollBarDisplayPolicy	"MatrixScrollBarDisplayPolicy"
#  endif

#  ifndef XmCReverseSelect
#    define XmCReverseSelect		"ReverseSelect"
#  endif

#  ifndef XmCRowLabelWidth
#    define XmCRowLabelWidth		"RowLabelWidth"
#  endif

#  ifndef XmCSelectedCells
#    define XmCSelectedCells		"SelectedCells"
#  endif

#  ifndef XmCSelectScrollVisible
#    define XmCSelectScrollVisible		"SelectScrollVisible"
#  endif

#  ifndef XmCShadowTypes
#    define XmCShadowTypes			"ShadowTypes"
#  endif

#  ifndef XmCTextBackground
#    define XmCTextBackground		"TextBackground"
#  endif

#  ifndef XmCTextBackgroundIsCell
#    define XmCTextBackgroundIsCell		"TextBackgroundIsCell"
#  endif

#  ifndef XmCTextField
#    define XmCTextField			"TextField"
#  endif

#  ifndef XmCTextShadowThickness
#    define XmCTextShadowThickness		"TextShadowThickness"
#  endif

#  ifndef XmCTraverseFixedCells
#    define XmCTraverseFixedCells		"TraverseFixedCells"
#  endif

#  ifndef XmCTopRow
#    define XmCTopRow			"TopRow"
#  endif

#  ifndef XmCNonFixedDetachedTop
#    define XmCNonFixedDetachedTop	"NonFixedDetachedTop"
#  endif

#  ifndef XmCNonFixedDetachedLeft
#    define XmCNonFixedDetachedLeft	"NonFixedDetachedLeft"
#  endif

#  ifndef XmCTrailingAttachedBottom
#    define XmCTrailingAttachedBottom	"TrailingAttachedBottom"
#  endif

#  ifndef XmCTrailingAttachedRight
#    define XmCTrailingAttachedRight	"TrailingAttachedRight"
#  endif

#  ifndef XmCTrailingFixedColumns
#    define XmCTrailingFixedColumns		"TrailingFixedColumns"
#  endif

#  ifndef XmCTrailingFixedRows
#    define XmCTrailingFixedRows		"TrailingFixedRows"
#  endif

#  ifndef XmCUnderlinedCells
#    define XmCUnderlinedCells		"UnderlinedCells"
#  endif

#  ifndef XmCUnderlinePosition
#    define XmCUnderlinePosition		"UnderlinePosition"
#  endif

#  ifndef XmCUnderlineWidth
#    define XmCUnderlineWidth		"UnderlineWidth"
#  endif

#  ifndef XmCUserDatas
#    define XmCUserDatas			"UserDatas"
#  endif

#  ifndef XmCVisibleColumns
#    define XmCVisibleColumns		"VisibleColumns"
#  endif

#  ifndef XmCVisibleRows
#    define XmCVisibleRows			"VisibleRows"
#  endif

#  ifndef XmRStringArray
#    define XmRStringArray			"StringArray"
#  endif

#  ifndef XmRBooleanArray
#    define XmRBooleanArray			"BooleanArray"
#  endif

#  ifndef XmRPixelArray
#    define XmRPixelArray			"PixelArray"
#  endif

#  ifndef XmRAlignmentArray
#    define XmRAlignmentArray		"AlignmentArray"
#  endif

#  ifndef XmRBooleanTable
#    define XmRBooleanTable			"BooleanTable"
#  endif

#  ifndef XmRCellTable
#    define XmRCellTable			"CellTable"
#  endif

#  ifndef XmRWidgetTable
#    define XmRWidgetTable			"WidgetTable"
#  endif

#  ifndef XmRGridType
#    define XmRGridType			"GridType"
#  endif

#  ifndef XmRHighlightTable
#    define XmRHighlightTable		"HighlightTable"
#  endif

#  ifndef XmRMatrixScrollBarDisplayPolicy
#    define XmRMatrixScrollBarDisplayPolicy "MatrixScrollBarDisplayPolicy"
#  endif

#  ifndef XmRMaxLengthArray
#    define XmRMaxLengthArray		"MaxLengthArray"
#  endif

#  ifndef XmRPixelTable
#    define XmRPixelTable			"PixelTable"
#  endif

#  ifndef XmRShadowTypeTable
#    define XmRShadowTypeTable		"ShadowTypeTable"
#  endif

#  ifndef XmRShadowTypeArray
#    define XmRShadowTypeArray		"ShadowTypeArray"
#  endif

#  ifndef XmRUserDataTable
#    define XmRUserDataTable		"UserDataTable"
#  endif

#  ifndef XmRUserDataArray
#    define XmRUserDataArray		"UserDataArray"
#  endif

#  ifndef XmRWidthArray
#    define XmRWidthArray			"WidthArray"
#  endif

#  ifndef	XmNuseXbaeInput
#    define	XmNuseXbaeInput			"useXbaeInput"
#  endif

#  ifndef	XmCUseXbaeInput
#    define	XmCUseXbaeInput			"UseXbaeInput"
#  endif

#  ifndef	XmNmultiLineCell
#    define	XmNmultiLineCell		"multiLineCell"
#  endif

#  ifndef	XmCMultiLineCell
#    define	XmCMultiLineCell		"MultiLineCell"
#  endif

#  ifndef	XmNwrapType
#    define	XmNwrapType				"wrapType"
#  endif

#  ifndef	XmCWrapType
#    define	XmCWrapType				"WrapType"
#  endif

#  ifndef	XmRWrapType
#    define	XmRWrapType				"WrapType"
#  endif

#  ifndef XmNdesiredHeight
#    define XmNdesiredHeight                "desiredHeight"
#  endif

#  ifndef XmCDesiredHeight
#    define XmCDesiredHeight                "DesiredHeight"
#  endif

#  ifndef XmNdesiredWidth
#    define XmNdesiredWidth                 "desiredWidth"
#  endif

#  ifndef XmCDesiredWidth
#    define XmCDesiredWidth                 "DesiredWidth"
#  endif


#  ifndef XbaeIsXbaeMatrix
#    define XbaeIsXbaeMatrix( w)	XtIsSubclass(w, xbaeMatrixWidgetClass)
#  endif						/* XbaeIsXbaeMatrix */

/* Class record constants */

#  ifdef	WIN32
#    define	INTERNALREF	externalref __declspec(dllimport)
#  else
#    define	INTERNALREF	extern
#  endif

	INTERNALREF WidgetClass xbaeMatrixWidgetClass;

#  undef	INTERNALREF

	typedef struct _XbaeMatrixClassRec *XbaeMatrixWidgetClass;
	typedef struct _XbaeMatrixRec *XbaeMatrixWidget;

/*
 * External interfaces to class methods
 */

	extern void XbaeMatrixAddColumns(Widget, int, String *, String *, short *,
									 int *, unsigned char *, unsigned char *, Pixel *, int);
	extern void XbaeMatrixAddRows(Widget, int, String *, String *, Pixel *, int);
	extern void XbaeMatrixAddVarRows(Widget, int, String *, String *, short *,
									 int *, unsigned char *, unsigned char *, Pixel *, int);
	extern void XbaeMatrixCancelEdit(Widget, Boolean);
	extern Boolean XbaeMatrixCommitEdit(Widget, Boolean);
	extern void XbaeMatrixDeleteColumns(Widget, int, int);
	extern void XbaeMatrixDeleteRows(Widget, int, int);
	extern void XbaeMatrixDeselectAll(Widget);
	extern void XbaeMatrixDeselectCell(Widget, int, int);
	extern void XbaeMatrixDeselectColumn(Widget, int);
	extern void XbaeMatrixDeselectRow(Widget, int);
	extern void XbaeMatrixDeunderlineCell(Widget, int, int);
	extern void XbaeMatrixDeunderlineColumn(Widget, int);
	extern void XbaeMatrixDeunderlineRow(Widget, int);
	extern void XbaeMatrixEditCell(Widget, int, int);
	extern void XbaeMatrixFirstSelectedCell(Widget, int *, int *);
	extern int XbaeMatrixFirstSelectedColumn(Widget);
	extern int XbaeMatrixFirstSelectedRow(Widget);
	extern String XbaeMatrixGetCell(Widget, int, int);
	extern Pixel XbaeMatrixGetCellBackground(Widget w, int row, int column);
	extern Pixel XbaeMatrixGetCellColor(Widget w, int row, int column);
	extern XtPointer XbaeMatrixGetCellUserData(Widget, int, int);
	extern Widget XbaeMatrixGetCellWidget(Widget, int, int);
	extern XtPointer XbaeMatrixGetColumnUserData(Widget, int);
	extern void XbaeMatrixGetCurrentCell(Widget, int *, int *);
	extern int XbaeMatrixGetEventRowColumn(Widget, XEvent *, int *, int *);
	extern Boolean XbaeMatrixEventToXY(Widget, XEvent *, int *, int *);
	extern Boolean XbaeMatrixRowColToXY(Widget, int, int, int *, int *);
	extern int XbaeMatrixGetNumSelected(Widget);
	extern XtPointer XbaeMatrixGetRowUserData(Widget, int);
	extern Boolean XbaeMatrixIsCellSelected(Widget, int, int);
	extern Boolean XbaeMatrixIsColumnSelected(Widget, int);
	extern Boolean XbaeMatrixIsRowSelected(Widget, int);
	extern void XbaeMatrixRefresh(Widget);
	extern void XbaeMatrixRefreshCell(Widget, int, int);
	extern void XbaeMatrixRefreshColumn(Widget, int);
	extern void XbaeMatrixRefreshRow(Widget, int);
	extern void XbaeMatrixSelectAll(Widget);
	extern void XbaeMatrixSelectCell(Widget, int, int);
	extern void XbaeMatrixSelectColumn(Widget, int);
	extern void XbaeMatrixSelectRow(Widget, int);
	extern void XbaeMatrixHighlightCell(Widget, int, int);
	extern void XbaeMatrixHighlightRow(Widget, int);
	extern void XbaeMatrixHighlightColumn(Widget, int);
	extern void XbaeMatrixUnhighlightCell(Widget, int, int);
	extern void XbaeMatrixUnderlineCell(Widget, int, int);
	extern void XbaeMatrixUnderlineColumn(Widget, int);
	extern void XbaeMatrixUnderlineRow(Widget, int);
	extern void XbaeMatrixUnhighlightRow(Widget, int);
	extern void XbaeMatrixUnhighlightColumn(Widget, int);
	extern void XbaeMatrixUnhighlightAll(Widget);
	extern void XbaeMatrixSetCell(Widget, int, int, const String);
	extern void XbaeMatrixSetCellBackground(Widget, int, int, Pixel);
	extern void XbaeMatrixSetCellColor(Widget, int, int, Pixel);
	extern void XbaeMatrixSetCellUserData(Widget, int, int, XtPointer);
	extern void XbaeMatrixSetCellWidget(Widget, int, int, Widget);
	extern void XbaeMatrixSetColumnBackgrounds(Widget, int, Pixel *, int);
	extern void XbaeMatrixSetColumnColors(Widget, int, Pixel *, int);
	extern void XbaeMatrixSetColumnUserData(Widget, int, XtPointer);
	extern void XbaeMatrixSetRowBackgrounds(Widget, int, Pixel *, int);
	extern void XbaeMatrixSetRowColors(Widget, int, Pixel *, int);
	extern void XbaeMatrixSetRowUserData(Widget, int, XtPointer);
	extern int XbaeMatrixVisibleColumns(Widget);
	extern int XbaeMatrixVisibleRows(Widget);
	extern int XbaeMatrixNumColumns(Widget);
	extern int XbaeMatrixNumRows(Widget);
	extern void XbaeMatrixDisableRedisplay(Widget);
	extern void XbaeMatrixEnableRedisplay(Widget, Boolean);
	extern void XbaeMatrixMakeCellVisible(Widget, int, int);
	extern Boolean XbaeMatrixIsRowVisible(Widget, int);
	extern Boolean XbaeMatrixIsColumnVisible(Widget, int);
	extern Boolean XbaeMatrixIsCellVisible(Widget, int, int);
	extern void XbaeMatrixVisibleCells(Widget, int *, int *, int *, int *);
	extern String XbaeMatrixGetColumnLabel(Widget, int);
	extern String XbaeMatrixGetRowLabel(Widget, int);
	extern void XbaeMatrixSetColumnLabel(Widget, int, String);
	extern void XbaeMatrixSetRowLabel(Widget, int, String);
	extern Widget XbaeCreateMatrix(Widget, String, ArgList, Cardinal);
	extern void XbaeMatrixSetCurrentCellPosition(Widget w, int current_row, int current_column);

	void XbaeMatrixSetCellShadow(Widget w, int row, int column, unsigned char shadow_type);
	void XbaeMatrixSetRowShadow(Widget w, int row, unsigned char shadow_type);
	void XbaeMatrixSetColumnShadow(Widget w, int column, unsigned char shadow_type);

	extern void XbaeMatrixSetColumnWidth(Widget mw, int column, int width);
	extern int XbaeMatrixGetColumnWidth(Widget w, int column);
	extern void XbaeMatrixSetRowHeight(Widget w, int row, int height);
	extern int XbaeMatrixGetRowHeight(Widget w, int row);

	extern void XbaeMatrixShowColumnArrows(Widget, int, Boolean);

	void XbaeMatrixSetCellTag(Widget w, int row, int column, XmStringTag tag);

	extern void XbaeMatrixSortRows(Widget w, int (*proc) (Widget, int, int, void *), void *user_data);
	extern void XbaeMatrixSortColumns(Widget w, int (*proc) (Widget, int, int, void *), void *user_data);
/* ARCAD SYSTEMHAUS */
	extern void XbaeMatrixSetCellPixmap(Widget w, int row, int column, Pixmap pixmap, Pixmap mask);
	extern int XbaeMatrixGetCellPixmap(Widget w, int row, int column, Pixmap * pixmap, Pixmap * mask);

/* Additional function */
	extern void XbaeMatrixResizeColumnsToCells(Widget, Boolean);



	typedef unsigned char Alignment;
	typedef Alignment *AlignmentArray;
	typedef String *StringTable;
	typedef short Width;
	typedef Width *WidthArray;
	typedef int MaxLength;
	typedef MaxLength *MaxLengthArray;

/*
 * cell shadow types
 */

	enum {
		XmGRID_NONE = 0x00,
		XmGRID_CELL_LINE = 0x02,
		XmGRID_CELL_SHADOW = 0x03,
		XmGRID_ROW_LINE = 0x04,
		XmGRID_ROW_SHADOW = 0x05,
		XmGRID_COLUMN_LINE = 0x08,
		XmGRID_COLUMN_SHADOW = 0x09,

		/* Deprecated types. Use will cause
		 * a run-time warning to be issued. */
		XmGRID_LINE = 0x20,
		XmGRID_SHADOW_IN = 0x40,
		XmGRID_SHADOW_OUT = 0x80
	};


/*
 * Enumeration for Matrix ScrollBar Display Policy
 */
	enum {
		XmDISPLAY_NONE,
		XmDISPLAY_AS_NEEDED,
		XmDISPLAY_STATIC
	};


/*
 * Enumeration for type of a cell
 */
	typedef enum {
		FixedCell, NonFixedCell, RowLabelCell, ColumnLabelCell
	} CellType;

/*
 * Enumeration for highlight reason/location
 */
	enum {
		HighlightNone = 0x0000,
		HighlightCell = 0x0001,
		HighlightRow = 0x0002,
		HighlightColumn = 0x0004,
		/* Deprecated */
		HighlightOther = 0x0008
	};

/*
 * Callback reasons.  Try to stay out of range of the Motif XmCR_* reasons.
 */
	typedef enum _XbaeReasonType {
		XbaeModifyVerifyReason = 102,
		XbaeEnterCellReason,
		XbaeLeaveCellReason,
		XbaeTraverseCellReason,
		XbaeSelectCellReason,
		XbaeDrawCellReason,
		XbaeWriteCellReason,
		XbaeResizeReason,
		XbaeResizeColumnReason,
		XbaeDefaultActionReason,
		XbaeProcessDragReason,
		XbaeLabelActivateReason,
		XbaeValueChangedReason,
		XbaeResizeRowReason,
		XbaeTrackCellReason
	} XbaeReasonType;

/*
 * DrawCell types.
 */
	typedef enum {
		XbaeString = 1,
		XbaePixmap = 2,
		XbaeStringFree = 5
	} XbaeCellType;

/*
 * Wrap types
 */
	enum {
		XbaeWrapNone,
		XbaeWrapContinuous,
		XbaeWrapWord
	};

/*
 * The 'Any' struct which can be used in callbacks used with different
 * Callback structs but only need to access its 4 members
 */
	typedef struct _XbaeMatrixAnyCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
	} XbaeMatrixAnyCallbackStruct;

/*
 * Struct passed to modifyVerifyCallback
 */
	typedef struct _XbaeMatrixModifyVerifyCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		XmTextVerifyCallbackStruct *verify;
		const char *prev_text;
	} XbaeMatrixModifyVerifyCallbackStruct;

/*
 * Struct passed to modifyVerifyCallback
 */
	typedef struct _XbaeMatrixValueChangedCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
	} XbaeMatrixValueChangedCallbackStruct;

/*
 * Struct passed to enterCellCallback
 */
	typedef struct _XbaeMatrixEnterCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		int position;
		String pattern;
		Boolean auto_fill;
		Boolean convert_case;
		Boolean overwrite_mode;
		Boolean select_text;
		Boolean map;
		Cardinal num_params;
		String *params;
		Boolean doit;

	} XbaeMatrixEnterCellCallbackStruct;

/*
 * Struct passed to leaveCellCallback
 */
	typedef struct _XbaeMatrixLeaveCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		String value;
		Boolean doit;
	} XbaeMatrixLeaveCellCallbackStruct;

/*
 * Struct passed to traverseCellCallback
 */
	typedef struct _XbaeMatrixTraverseCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		int next_row;
		int next_column;
		int fixed_rows;
		int fixed_columns;
		int trailing_fixed_rows;
		int trailing_fixed_columns;
		int num_rows;
		int num_columns;
		String param;
		XrmQuark qparam;
	} XbaeMatrixTraverseCellCallbackStruct;

/*
 * Struct passed to selectCellCallback
 */
	typedef struct _XbaeMatrixSelectCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		Boolean **selected_cells;
		String **cells;
		Cardinal num_params;
		String *params;
	} XbaeMatrixSelectCellCallbackStruct;

/*
 * Struct passed to drawCellCallback
 */
	typedef struct _XbaeMatrixDrawCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		int width;
		int height;
		XbaeCellType type;
		String string;
		Pixmap pixmap;
		Pixmap mask;
		Pixel foreground;
		Pixel background;
		int depth;
	} XbaeMatrixDrawCellCallbackStruct;

/*
 * Struct passed to writeCellCallback
 */
	typedef struct _XbaeMatrixWriteCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		XbaeCellType type;
		String string;
		Pixmap pixmap;
		Pixmap mask;
	} XbaeMatrixWriteCellCallbackStruct;


/*
 * Struct passed to resizeCallback
 */
	typedef struct _XbaeMatrixResizeCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		Dimension width;
		Dimension height;
	} XbaeMatrixResizeCallbackStruct;

/*
 * Struct passed to resizeColumnCallback
 *
 */
	typedef struct _XbaeMatrixResizeColumnCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		int which;
		int columns;
		short *column_widths;
	} XbaeMatrixResizeColumnCallbackStruct;

	typedef struct _XbaeMatrixResizeRowCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		int which;
		int rows;
		short *row_heights;
	} XbaeMatrixResizeRowCallbackStruct;

/*
 * Struct passed to processDragCallback
 */
	typedef struct _XbaeMatrixProcessDragCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		String string;
		XbaeCellType type;
		Pixmap pixmap;
		Pixmap mask;
		Cardinal num_params;
		String *params;
	} XbaeMatrixProcessDragCallbackStruct;

/*
 * Struct passed to defaultActionCallback
 */
	typedef struct _XbaeMatrixDefaultActionCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
	} XbaeMatrixDefaultActionCallbackStruct;

/*
 * Struct passed to labelActivateCallback
 */
	typedef struct _XbaeMatrixLabelActivateCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row;
		int column;
		Boolean row_label;
		String label;
	} XbaeMatrixLabelActivateCallbackStruct;


	typedef struct _XbaeMatrixTrackCellCallbackStruct {
		XbaeReasonType reason;
		XEvent *event;
		int row, column;
		int prev_row, prev_column;
		Position pointer_x, pointer_y;
	} XbaeMatrixTrackCellCallbackStruct;

/* Lesstiff 0.94.0 needs this */
#  ifndef _MOTIF_DEFAULT_LOCALE
#    define _MOTIF_DEFAULT_LOCALE "_MOTIF_DEFAULT_LOCALE"
#  endif

#  ifdef __cplusplus
}
#  endif
#endif							/* _Xbae_Matrix_h */
