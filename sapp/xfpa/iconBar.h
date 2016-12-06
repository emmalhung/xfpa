/*========================================================================*/
/*
*	File:		iconBar.h
*
*   Purpose:    Header file for things related to the icon button bar.
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
#ifndef ICON_BAR_H
#define ICON_BAR_H

/* names as found in the resource file that are used for
 * the popup icon descriptions
 */
#define LOAD_FIELDS_ICON		"loadFieldsIcon"
#define UPDATE_FIELDS_ICON		"updateFieldsIcon"
#define IMPORT_FIELDS_ICON		"importFieldsIcon"
#define DELETE_FIELDS_ICON		"deleteFieldsIcon"
#define GRAPHIC_PRODUCTION_ICON	"graphicProdIcon"
#define ALLIED_MODELS_ICON		"alliedModelsIcon"
#define GUIDANCE_SELECT_ICON	"guidSelectIcon"
#define GUIDANCE_STATUS_ICON	"guidStatusIcon"
#define GUID_SELECT_STATUS_ICON	"guidSelectStatusIcon"
#define IMAGE_SELECT_ICON		"imageSelectIcon"
#define MAP_OVERLAYS_ICON		"mapOverlaysIcon"
#define FIELD_DISPLAY_ICON		"fieldDisplayIcon"
#define PRODUCT_STATUS_ICON		"productStatusIcon"
#define ZOOM_PAN_ICON			"zoomPan"
#define ZOOM_IN_ICON			"zoomIn"
#define ZOOM_OUT_ICON			"zoomOut"

#define SEPARATOR_ICON			"-sep-"


extern void   SetIconBarButtonSensitivity(String, Boolean);
extern Widget GetIconBarWidget(String);

#endif

