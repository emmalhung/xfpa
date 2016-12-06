
/*
 * Progress - A simple progress meter widget, incorporating display of
 *		percentage complete, and estimated time to completion.
 *
 * Copyright (c) 1997 Richard H. Porter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Dick Porter <dick@cymru.net>
 *
 */

#ifndef _PROGRESS_METER_H_
#define _PROGRESS_METER_H_

#define XmNmaxValue	"maxValue"
#define XmNshowPercent	"showPercent"
#define XmCShowPercent	"ShowPercent"
#define XmNshowTime	"showTime"
#define XmCShowTime	"ShowTime"

/* Class record constants */

extern WidgetClass xmpProgressBarWidgetClass;

typedef struct _XmpProgressBarClassRec *XmpProgressBarWidgetClass;
typedef struct _XmpProgressBarRec      *XmpProgressBarWidget;

extern Widget XmpCreateProgressBar (Widget, String, ArgList, int);
extern Widget XmpCreateManagedProgressBar (Widget, String, ArgList, int);
extern Widget XmpVaCreateProgressBar (Widget, String, ...);
extern Widget XmpVaCreateManagedProgressBar (Widget, String, ...);
extern void   XmpProgressBarSetValue        (Widget, int);

#endif /* _PROGRESS_METER_H_ */
