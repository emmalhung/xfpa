/*
 * Geometry.h - XmpGeometry Public header
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

#ifndef XmpGeometry_h
#define XmpGeometry_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmpIsGeometry
#define XmpIsGeometry(w) XtIsSubclass(w, xmpGeometryWidgetClass)
#endif

externalref WidgetClass xmpGeometryWidgetClass;

typedef struct XmpGeometryClassRec *XmpGeometryWidgetClass;
typedef struct XmpGeometryRec *XmpGeometryWidget;

#ifdef __cplusplus
}
#endif

#endif /* XmpGeometry_h */

