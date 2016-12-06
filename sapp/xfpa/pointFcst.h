/*========================================================================*/
/*
*	File:		pointFcst.h
*
*   Purpose:    Header file for pointFcstSelectDialog.c and pointFcstEditDialog.c
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

#ifndef _POINT_FCST_H
#define _POINT_FCST_H

#define INFO_FILE		".PFI"
#define MISSING			9999
#define TIMEZONES		"timezones"

#define  dashes         "----------"
#define  clear          ""
#define  degree_symbols "³\'\":"
#define  keylabel       "label"
#define  keyclass       "class"
#define  keylanguage    "language"
#define  keylat         "lat"
#define  keyinfolat     "iflat"
#define  keylong        "long"
#define  keyid          "id"
#define  keyinfolong    "iflong"
#define  keytimezone    "timezone"
#define  keyissue       "issue"

typedef struct {
	String  id;
	String  label;
	String  class;
	String  language;
	String  timezone;
	float   latitude;
	float   longitude;
	int     nissuetimes;
	int     issuetimes[24];
	Boolean generate;
	Boolean generating;
	Boolean modifable;
	int     pid;			/* product id as returned by ProductStatusAddInfo() */
} PFDATA;

typedef struct {
	String id;
	String label;
	int    ndata;
	PFDATA **data;
	int    selected;
} PFCLASS;

extern void    ACTIVATE_pointFcstEditDialog		(Widget);
extern void    ACTIVATE_pointFcstSelectDialog	(Widget);
extern void    InitPointFcstDialog			(void);
extern void    pf_CreateFileName			(PFDATA*, String);
extern void    pf_DegreeComponents			(float, int*, int*, int*);
extern void    pf_GetFmtData				(PFDATA*, String, String);
extern Boolean pf_IsReleased				(int);
extern String  pf_FmtData					(PFDATA*, String);
extern void    pf_FreeData					(PFDATA*);
extern void    pf_MakeClassSelection		(void);
extern void    pf_MakeDataId				(String, PFCLASS*);
extern void    pf_MakeFileName				(PFDATA*, String);
extern void    pf_ParseLatLongString		(String, String);
extern void    pf_WriteInfoData				(void);

#endif /* _POINT_FCST_H */
