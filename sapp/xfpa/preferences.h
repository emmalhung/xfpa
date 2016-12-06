/*========================================================================*/
/*
*	File:		preferences.h
*
*   Purpose:    Header file for program preferences
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
#ifndef PREFERENCES_H
#define PREFERENCES_H

/* preferences_dialog.c */
extern void InitPreferences(void);
extern void ACTIVATE_preferencesDialog(Widget refw);

/* preferences_general.c */
extern void InitGeneralOptions(void);
extern void GeneralOptions(Widget parent );
extern void SetGeneralOptions(void);
extern void ExitGeneralOptions(void);

/* preferences_gui.c */
extern void InitMainGui(void);
extern void GuiOptions(Widget parent_widget );
extern void SetGuiOptions(void);

/* preferences_alliedModels.c */
extern void InitAlliedModelOptions(void);
extern void AlliedModelOptions(Widget parent_widget );
extern void SetAlliedModelOptions(void);

/* preferences_dialogLocation.c */
extern int  InitDialogLocationOptions(void);
extern void DialogLocationOptions(Widget parent_widget );
extern void SetDialogLocationOptions(void);
extern void ExitDialogLocationOptions(void);
extern void ACTIVATE_saveProfileDialog(Widget);

#endif /* PREFERENCES_H */
