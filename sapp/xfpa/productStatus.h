/*========================================================================*/
/*
*	File:		productStatus.h
*
*   Purpose:    Header file for productStatus.c and productStatusDialog.c
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

#ifndef PS_HEADERH
#define PS_HEADERH

#define PSKEY 						"psi"
#define RELEASED_PROD_STATE_FILE	".geninfo"

typedef enum {
	PS_TEXT_FCST,
	PS_POINT_FCST,
	PS_GRAPHICS,
	PS_MODEL,
	PS_RUNNING,
	PS_UPDATE,
	PS_ENDED,
	PS_ERROR
} PS_TYPE;

typedef struct {
	String  key;
	String  label;
	Boolean has_release_status;
} PS_TYPE_DATA;

typedef struct {
	int      id;
	char     key[12];
	PS_TYPE  type;
	String   label;
	Boolean  is_running;
	String   statinfo;
	Boolean  (*release_fcn)(int);
} PRODUCT_INFO;

extern void    ACTIVATE_productStatusDialog	(Widget);
extern void    RedrawProductIndicatorCB		(Widget, XtPointer, XtPointer);
extern int     ProductStatusAddInfo			(PS_TYPE, String, Boolean(*)());
extern long    ProductStatusGetGenerateTime	(int);
extern void    ProductStatusGetInfo        	(PS_TYPE, int*, PRODUCT_INFO**);
extern Boolean ProductStatusReleaseCheck	(int);
extern void    ProductStatusRemoveInfo		(int);
extern void    ProductStatusUpdateInfo		(int, PS_TYPE, String);
extern void    ProductStatusDialogUpdate	(PRODUCT_INFO *);
extern void    UpdateProductDialogFromId	(int);

#ifdef PS_MAIN

	/* Set the product generator specific information as required by the status
	*  functions in this file. The first entry is the state store key, the second
	*  the label which has a reference into the resource file, and the third
	*  indicates if the product is "released" by the user.
	*  Be careful here! The order of this information must correspond to the order
	*  of the type keys in the enumerated list above (not including those from
	*  PS_RUNNING on).
	*/
	static PS_TYPE_DATA prod_data[] = {
		{"t", "textFcsts",  True },
		{"p", "pointFcsts", True },
		{"g", "graphics",   False},
		{"m", "models",     False}
	};

#endif

#endif /* PS_HEADERH */
