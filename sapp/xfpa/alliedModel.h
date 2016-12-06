/*========================================================================*/
/*
*	File:		alliedModel.h
*
*   Purpose:    Header for allied model fuctions.
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
#ifndef _ALLIED_MODEL_H
#define _ALLIED_MODEL_H

#define ALLIED_MODEL_STATE_KEY	"am"

/* Define the globally available variables.
*/
typedef struct {
	int product_key;			/* for product status dialog */
	struct _source *source;		/* pointer to source data structure */
	Boolean selected;			/* selected in the run dialog? */
	Boolean running;			/* is model currently running? */
	Widget sel_btn;				/* selection button for model */
	Boolean automatic_import;	/* auto import fields when finished? */
	Widget btn;					/* used in permission dialog */
	Boolean *import;			/* used in conjunction with config file to give */
} AlliedModelStruct;			/* auto-import permission state                 */

#ifdef ALLIED_MODEL_MAIN
	int               GV_nallied_model = 0;
	AlliedModelStruct *GV_allied_model = (AlliedModelStruct*) NULL;
#else
	extern int               GV_nallied_model;
	extern AlliedModelStruct *GV_allied_model;
#endif

/* alliedModelInit.c */
extern void InitAlliedModels(void);

/* alliedModelOptionDialog.c */
extern void ACTIVATE_alliedModelOptionsDialog(Widget ref_widget);

/* alliedModelOptions.c */
extern void AlliedModelOptions(Widget parent);
extern void SetAlliedModelOptions(void);

/* alliedModelPermissionDialog.c */
extern void ACTIVATE_alliedModelImportPermissionDialog(AlliedModelStruct *model);

/* alliedModelSelect.c */
extern void ACTIVATE_alliedModelSelectDialog(Widget ref_widget);


#endif /* _ALLIED_MODEL_H */
