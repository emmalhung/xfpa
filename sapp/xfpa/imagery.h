/*========================================================================*/
/*
*	File:		imagery.h
*
*   Purpose:    Data structure for radar and satellite info.
*               The *_last_item variables are used to stop the
*               ComboBox widget callbacks from activating when the
*               cursor passes over the widget.
*               There seems no other way to stop this action.
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

#ifndef _IMAGERY_H
#define _IMAGERY_H

/* The imagery type identifiers */
#define RADAR_NAME		"radar"
#define SATELLITE_NAME	"satellite"
#define OVERLAY_NAME	"overlay"
#define UNDERLAY_NAME	"underlay"

/* Number of image types */
#define NIMAGES	4

typedef struct {
	String tag;			/* product tag */
	String label;		/* product label */
	int    lutndx;		/* index into colour lookup table */
} PRODINFO;


typedef struct {
	String  tag;		/* site tag */
	String  label;		/* site label */
	int     selected;	/* Is this site selected? */
	int     nvtime;		/* number of valid times */
	String  *vtime;		/* valid times */
	String  *atime;		/* actual image times that correspond to the valid times */
	Boolean *vused;		/* valid times that have already been used in times array */
	String  *times;		/* time list for site that mirrors the all_times array */
} SITEINFO;				


typedef struct {
	String    name;				/* either radar, satellite or dataImage */
	int       type;				/* image type as found in glib.h */
	Boolean   exists;			/* Do the controls exist? */
	String    displayPlane;		/* Ingred display plane to put the images into */
	Boolean   display;			/* display the images ? */
	Widget    prodSelect;		/* product selection widget */
	int       nprod;			/* number of products */
	PRODINFO  *prod;			/* product information */
	int       prod_select;		/* which product is selected */
	int       prod_last_item;	/* see above note */
	int       nsite;			/* number of sites */
	int       site_end;			/* where to end the add image loop */
	SITEINFO  *site;			/* site info */
	int       selected;			/* number of sites selected */
	float     brightness;		/* brightness setting */
	TSTAMP    selected_time;	/* display time last selected for this data type */
	int       pre_hold_time;
	int       post_hold_time;
	Widget    blendAmt;
	Widget    source;
	int       source_last_item;
	Widget    lutW;
	Widget    lutW_label;
	int       luts_last_item;
	Widget    selectList;
	/* The following 2 are used just during the image selection process */
	int       nsel;				/* number of image selection changes */
	int       *sel;				/* list of selection changes */
	SAMPLE    sample_display;
	Image     sample_image;
	XmString  last_sample_item;
} IMDAT;



extern void ACTIVATE_imageryControlDialog (Widget);
extern void InitImagery                   (void);
extern Boolean ImageryExists              (String);
extern void ImageryUpdate                 (void);
extern void SendImageSampleCommand        (String);
extern void SetImageryDisplayState        (String, Boolean);
extern void ImageryAnimationUpdate        (String, String);

extern void ShowImageLegend(IMDAT*, Boolean);
extern void SetImageLegendTime(IMDAT*, String, Boolean);

#endif  /* _IMAGERY_H */
