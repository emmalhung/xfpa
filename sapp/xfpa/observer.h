/*========================================================================*/
/*
*	File:		observer.h
*
*   Purpose:    The header file for the observer functions.
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

#ifndef OBSERVER_H
#define OBSERVER_H

/*
 * Description of the enumerated observable types.
 *
 * OB_ANIMATION_RUNNING			Some animation loop is running
 * OB_DEPICTION_ABOUT_TO_CHANGE	Depiction is about to change
 * OB_DEPICTION_CHANGE			Depiction has changed
 * OB_DEPICTION_TZERO_CHANGE	The T0 depiction time has changed
 * OB_DEPICTION_SAVED			One or more depictions have been saved
 * OB_DIALOG_SAMPLING           Some dialog is starting or ending sampling
 * OB_EDIT_FUNCTION_TO_CHANGE	The edit function is about to change (sent before the change is done)
 * OB_FIELD_AVAILABLE			Which fields are available has changed
 * OB_GROUP_CHANGE				Active group has changed
 * OB_FIELD_CHANGE				Active field has changed
 * OB_GUIDANCE_READY			Guidance has been initialized
 * OB_GUIDANCE_VISIBILITY       Guidance visibility setting changed
 * OB_IMAGE_SELECTED            An image has been selected/deselected
 * OB_INTERPOLATE				Interpolation done
 * OB_MAIN_INITIALIZED			The mainline initialization is complete
 * OB_MAIN_MENU_ACTIVATION		One of the main menu items has been activated.
 * OB_MENU_ACTIVATION			A menu in one of the field type panels has been activated.
 * OB_PROFILE_SAVE				The current profile is being saved
 * OB_TIMELINK_EXIT				Exiting the timelink panel
 * OB_ZOOM					    Zoom action being performed
 *
 * Parameters sent with notification
 *
 * observed type				parameters sent
 * --------------------------------------------
 *
 * OB_ANIMATION_RUNNING         id of sender, "on"|"off" - Sent before/after the start/end of loop.
 * OB_DEPICTION_ABOUT_TO_CHANGE	none
 * OB_DEPICTION_CHANGE			none
 * OB_DEPICTION_TZERO_CHANGE	none
 * OB_DEPICTION_SAVED			none
 * OB_DIALOG_SAMPLING           "I"|"G" "on"|"off" *fcn (See note below)
 * OB_EDIT_FUNCTION_TO_CHANGE	none
 * OB_FIELD_AVAILABLE			True/False - has depiction sequence changed
 * OB_GROUP_CHANGE				none
 * OB_FIELD_CHANGE				none
 * OB_GUIDANCE_READY			none
 * OB_GUIDANCE_VISIBILITY       none
 * OB_IMAGE_SELECTED            name of image (radar/satellite) and display state (true/false)
 * OB_INTERPOLATE				True/False - has depiction sequence changed
 * OB_MAIN_INITIALIZED			none
 * OB_MAIN_MENU_ACTIVATION      Enumerated value of key associated with the change (see menu.h).
 * OB_MENU_ACTIVATION			Key identifying menu
 * OB_PROFILE_SAVE				True/False for automatically save on exit or not
 * OB_TIMELINK_EXIT				none
 * OB_ZOOM                      zoom command, state (True/False)
 *
 * Notes:
 *
 * The OB_DIALOG_SAMPLING parameters are:
 *    I|G    - the dialog identifier, either "IMAGERY" or "GUIDANCE"
 *    on|off - sampling is turning on or off
 *    *fcn   - pointer to a function of type (*fcn)(void) to call when the ActivateMenu
 *             function is called. Can be NULL.
 */

enum {
	OB_ANIMATION_RUNNING,
	OB_DEPICTION_ABOUT_TO_CHANGE,
	OB_DEPICTION_CHANGE,
	OB_DEPICTION_TZERO_CHANGE,
	OB_DEPICTION_NUMBER_CHANGE,
	OB_DEPICTION_SAVED,
	OB_DIALOG_SAMPLING,
	OB_EDIT_FUNCTION_TO_CHANGE,
	OB_FIELD_AVAILABLE,
	OB_GROUP_CHANGE,
	OB_FIELD_CHANGE,
	OB_GUIDANCE_READY,
	OB_GUIDANCE_VISIBILITY,
	OB_IMAGE_SELECTED,
	OB_INTERPOLATE,
	OB_MAIN_INITIALIZED,
	OB_MAIN_MENU_ACTIVATION,
	OB_MENU_ACTIVATION,
	OB_PROFILE_SAVE,
	OB_TIMELINK_EXIT,
	OB_ZOOM
};

/* These are key words used when using OB_DEPICTION_SAMPLING to define the
 * dialog the command is sent from and defined here to avoid finger problems.
 * They are also used by OB_ANIMATION_RUNNING.
 */
#define OB_KEY_GUIDANCE	"guidance"
#define OB_KEY_IMAGERY	"imagery"
#define OB_KEY_DEPICT   "depict"

/* Useful defines */
#define OB_KEY_ON	"on"
#define OB_KEY_OFF	"off"

/* Function prototypes */
extern void AddObserver           (int obid, void (*fcn)(String*,int));
extern void DeleteObserver        (int obid, void (*fcn)(String*,int));
extern void NotifyObservers       (int obid, String *parms, int nparms);
extern void AddIngredObserver     (void (*fcn)(CAL,String*,int));
extern void DeleteIngredObserver  (void (*fcn)(CAL,String*,int));
extern void NotifyIngredObservers (CAL cal, String *parms, int nparms);
extern void AddSourceObserver     (Boolean (*fcn)(Boolean), String);
extern void DeleteSourceObserver  (Boolean (*fcn)(Boolean));
extern void NotifySourceObservers (void (*fcn)(void), Boolean state);

#endif /* OBSERVER_H */
