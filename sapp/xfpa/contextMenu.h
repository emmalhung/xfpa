/*========================================================================*/
/*
*	File:		contextMenu.h
*
*   Purpose:    Header file for right mouse button context menus
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
#ifndef _CONTEXT_MENU_H
#define _CONTEXT_MENU_H

/* These are used in the SetActiveContextMenu() function to select the appropriate
 * default menu when a popup widget is not provided. None is normally defined in
 * X11.h, thus the ifndef.
 */
#ifndef None
#define None 0L
#endif
#define FieldEditContextMenu	(Widget)2

/* Define the valid button ids that can be used in the CONTEXT structure. The enumerated
 * types and the labels in the CONTEXT_BUTTON_LABELS define must be in the same order.
 * The labels must correspond to a <label>.labelString resource in the resource file.
 */
enum BTN_ID {
	undefinedBtn, setBtnId, mergeBtnId, translateBtnId, rotateBtnId, cutBtnId, copyBtnId,
	deleteAreaBtnId, deleteHoleBtnId, deleteLineBtnId, deletePointBtnId,
	flipBtnId, reverseBtnId, flipReverseBtnId, leaveAsIsBtnId,
	endChainBtnId, chooseLinkChainId, newChainBtnId
};

#define CONTEXT_BUTTON_LABELS	\
	"???","setBtn","mergeBtn","translateBtn","rotateBtn","cutBtn","copyBtn",\
	"deleteAreaBtn","deleteHoleBtn","deleteLineBtn","deletePointBtn",\
	"flipBtn","reverseBtn","flipReverseBtn","leaveAsIsBtn",\
	"endChainBtn","chooseLinkChainBtn","newChainBtn"

/* Structure to hold information for the selected object popup context menus.
 * This structure is returned by the context menu buttons with selected_item
 * set to the item corresponding to the push button activated. The defined
 * MAX_CONTEXT_ITEMS is the maximum number of items allowed, but if the number
 * of items ever goes above the limit just increase the limit to whatever is
 * needed as this just sets the number of push button widgets created in the
 * menu. It was done this way to make the code easier.
 */
#define MAX_CONTEXT_ITEMS	4

typedef struct {
		enum BTN_ID btn_id;		/* button id as seen in resource file like "cancelBtn" */
		String      edit_cmd;	/* command associated with the button: E_SET, E_TRANSLATE and such */
} CONTEXT_ITEMS;

typedef struct {
	String         edit_mode;		/* E_DRAW, E_MODIFY, etc */
	CONTEXT_ITEMS *selected_item;	/* On return - which of the items was selected from the context menu */
	int            nitems;			/* Number of items defined */
	CONTEXT_ITEMS  items[MAX_CONTEXT_ITEMS];	/* Item data */
} CONTEXT;

extern void   InitContextMenuAttributes               (String, void (*fcn)(int));
extern void   AddToContextMenuAttributes              (String);
extern void   ShowContextMenuAttributes               (Boolean);
extern void   ConfigureSelectContextMenuCascadeButtons(Widget, Widget, Widget, Widget);
extern Widget CreatePanelContextMenu                  (String);
extern void   ActivateSelectContextMenu               (CONTEXT*, void (*fcn)(CONTEXT*,CAL), CAL);
extern void   ConfigureMainContextMenuForField        (void);
extern void   ConfigureMainContextMenuFieldButtons    (void);
extern void   CreateContextMenus                      (void);
extern void   CreateMainContextMenu                   (void);
extern void   CreateMainContextMenuFieldButtons       (void);
extern void   DeactivateSelectContextMenu             (void);
extern void   SetContextPasteSelectAllButtonsState    (int);
extern void   ConfigureMainContextMenuAuxPushButton   (String, void (*fcn)(XtPointer), XtPointer);
extern void   ReleaseMainContextMenuAuxPushButton     (void);
extern void   ConfigureMainContextMenuAuxCascade      (Widget, Widget, Widget, Widget);
extern  void  ReleaseMainContextMenuAuxCascade        (void);
extern Widget CreateMainContextMenuAuxCascadePulldown (String);
extern void   SetActiveContextMenu                    (Widget);


#endif /* CONTEXT_MENU_H */
