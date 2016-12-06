/*========================================================================*/
/*
*	File:		menu.h
*
*   Purpose:    This header contains definitions that assign
*               numerical identifiers to menu buttons.
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
#ifndef MENU_H
#define MENU_H

#define MENU_Depiction_save				101
#define MENU_Depiction_saveAll			104
#define MENU_Depiction_delete			105
#define MENU_Depiction_deleteAll		106
#define MENU_Depiction_quit				107
#define MENU_Depiction_print            108

#define MENU_Create_fields              111

#define MENU_Load_fields				121

#define MENU_Update_fields  			131

#define MENU_Import_fields              141

#define MENU_Option_overlays			201
#define MENU_Option_preferences			202
#define MENU_Option_save_profile		203
#define MENU_Option_manage_profile		204

#define MENU_Option_t0_active			211
#define MENU_Option_nearest_t0_clock	212
#define MENU_Option_t0_clock			213
#define MENU_Option_t0_to_new_depiction	214

#define MENU_Option_timeStep			221

#define MENU_Status_depiction           301
#define MENU_Status_products            302

#define MENU_Products_regularText		401
#define MENU_Products_graphics			402
#define MENU_Products_pointFcst			403
#define MENU_Products_alliedModels		404

#define MENU_Guidance_availability      501
#define MENU_Guidance_status            502
#define MENU_Guidance_select			503
#define MENU_Guidance_hide				504
#define MENU_Guidance_show				505
#define MENU_Guidance_displayed			506

#define MENU_Image_availability         601
#define MENU_Image_select			    602
#define MENU_Image_hide				    603
#define MENU_Image_show				    604

#define MENU_View_scratchpadShow		701
#define MENU_View_guidanceShow	 		702
#define MENU_View_synoptic				703
#define MENU_View_presetField           704
#define MENU_View_fieldVisibility		705
#define MENU_View_mapOverlays			706

#define MENU_Actions_animation          710
#define MENU_Actions_coview             711
#define MENU_Actions_Zoom_in			712
#define MENU_Actions_Zoom_out			713
#define MENU_Actions_Zoom_pan			714
#define MENU_Actions_Zoom_exit			715
#define MENU_Actions_Zoom_tear			716

#define MENU_Help_problems				801
#define MENU_Help_about					802

#define MENU_Bar_products               901
#define MENU_Bar_guidance_update		902


extern void CreateTimeStepPulldown	(XmString*);

#endif /* MENU_H */
