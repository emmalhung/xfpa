/*========================================================================*/
/*
*	File:		help.h
*
*   Purpose:    Include file help.h
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

#ifndef _HELP_H
#define _HELP_H

extern void Help   (XtPointer);
extern void HelpCB (Widget, XtPointer, XtPointer);

/* File in the help directory which cross references the keys found
 * below with the appropriate hyperlink in the help files.
 */
#define CROSS_REF_FILE	"onlineHelpHyperlinkReference"

/* Name of the files which contain the table of contents and the 
*  index file.
*/
#define HELP_TOC	"toc"
#define HELP_INDEX	"index"
#define MAIN_INDEX	"index"

/* Keys used in identifying the help documentation hypertext links in 
*  the online documentation. These are keys into the message database
*  file where the actual hypertext links are specified.
*/
#define HELP_ALLIED_MODEL_IMPORT   (XtPointer)"alliedModelImport"
#define HELP_ALLIED_MODEL_SELECT   (XtPointer)"alliedModelSelect"
#define HELP_ALLIED_MODEL_STATUS   (XtPointer)"alliedModelStatus"
#define HELP_ANIMATION             (XtPointer)"animation"
#define HELP_BKGND_WIND            (XtPointer)"bkgndWind"
#define HELP_CHANGE_CURSOR         (XtPointer)"changeCursor"
#define HELP_CREATE                (XtPointer)"createField"
#define HELP_COPY_DAILY            (XtPointer)"copyDaily"
#define HELP_COPY_HOURLY_FIELD     (XtPointer)"copyHourly"
#define HELP_COPY_STATIC           (XtPointer)"copyStatic"
#define HELP_DELETE_FIELDS         (XtPointer)"deleteFields"
#define HELP_DEPICT_STATUS         (XtPointer)"depictStatus"
#define HELP_DIALOG_LOCATION       (XtPointer)"dialogLocation"
#define HELP_EDIT_AREAS            (XtPointer)"editAreas"
#define HELP_EDIT_LINES            (XtPointer)"editLines"
#define HELP_EDIT_SFC              (XtPointer)"editSfc"
#define HELP_FIELD_DISPLAY_CONTROL (XtPointer)"fieldDisplayControl"
#define HELP_FIELD_DISPLAY_STATE   (XtPointer)"fieldDisplayState"
#define HELP_GFA_ENTRY             (XtPointer)"gfaEntry"
#define HELP_GRAPHIC_PRODUCTS      (XtPointer)"graphicProducts"
#define HELP_GUIDANCE              (XtPointer)"guid"
#define HELP_GUIDANCE_AVAIL        (XtPointer)"guidAvail"
#define HELP_GUIDANCE_LIST_ADD     (XtPointer)"guidListAdd"
#define HELP_GUIDANCE_FIELD_ADD    (XtPointer)"guidFieldAdd"
#define HELP_GUIDANCE_FIELD_APPEAR (XtPointer)"guidFieldAppearance"
#define HELP_GUIDANCE_FIELD_DEL    (XtPointer)"guidFieldDelete"
#define HELP_GUIDANCE_FIELD_SAMPLE (XtPointer)"guidFieldSample"
#define HELP_GUIDANCE_STATUS       (XtPointer)"guidStatus"
#define HELP_IMAGERY               (XtPointer)"imagery"
#define HELP_LCHAIN_ATTRIB_ENTRY   (XtPointer)"lchainAttributeEntry"
#define HELP_LNODE_ATTRIB_ENTRY    (XtPointer)"lnodeAttributeEntry"
#define HELP_MAIN                  (XtPointer)HELP_TOC
#define HELP_MAP_OVERLAYS          (XtPointer)"mapOverlays"
#define HELP_GENERAL_OPTIONS       (XtPointer)"optGeneral"
#define HELP_MAP_OPTIONS           (XtPointer)"optMap"
#define HELP_POINT_FCSTS           (XtPointer)"pointFcst"
#define HELP_POINT_FCST_EDIT       (XtPointer)"pointFcstEdit"
#define HELP_PROBLEM_REPORTING     (XtPointer)"probReport"
#define HELP_PROBLEM_LOG           (XtPointer)"progLog"
#define HELP_PRODUCT_STATUS        (XtPointer)"productStatus"
#define HELP_QUICK_REF             (XtPointer)"quickRef"
#define HELP_SCRATCHPAD            (XtPointer)"scratchpad"
#define HELP_TEXT_FCST             (XtPointer)"textFcst"
#define HELP_TEXT_FCST_PRIORITY    (XtPointer)"textFcstPriority"
#define HELP_TEXT_FCST_AREAS       (XtPointer)"textFcstAreas"
#define HELP_TIMELINK              (XtPointer)"timelink"
#define HELP_UPDATE_FIELDS         (XtPointer)"updateField"
#define HELP_WIND_ENTRY            (XtPointer)"windEntry"
#define HELP_WX_ENTRY              (XtPointer)"weatherEntry"

#endif /* _HELP_H */
