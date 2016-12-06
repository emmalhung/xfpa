/*========================================================================*/
/*
*	File:		loadFields.h
*
*   Purpose:    Header for loading various fields.
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
#ifndef LOAD_FIELD_H
#define LOAD_FIELD_H

struct SRCDAT {
    SourceList ids;			/* source identifiers */
    XmString  *names;		/* source display names */
    int        number;		/* number of sources of this type */
    int        last_select;	/* last selected source of this type */
};

typedef struct {
    long    id;				/* source type identifier */
    String  btn_id;			/* name of button to create */
    Boolean is_model;		/* is this source model data? */
    Boolean is_depict;		/* is this source depiction data? */
    Boolean has_run_time;	/* does this source have a run time? */
	struct SRCDAT h;		/* hourly fields data */
	struct SRCDAT d;		/* daily fields data */
	struct SRCDAT s;		/* static fields data */
} SRCINFO;


#ifdef LOAD_FIELD_MAIN

SRCINFO load_sources[] = {
	{SRC_NWP,    "ngmBtn",      True,  False, True,  NULL,NULL,0,1, NULL,NULL,0,1, NULL,NULL,0,1},
	{SRC_FPA,    "synoProgBtn", True,  False, False, NULL,NULL,0,1, NULL,NULL,0,1, NULL,NULL,0,1},
	{SRC_ALLIED, "alliedBtn",   True,  False, True,  NULL,NULL,0,1, NULL,NULL,0,1, NULL,NULL,0,1},
	{SRC_DEPICT, "depictBtn",   False, True,  False, NULL,NULL,0,1, NULL,NULL,0,1, NULL,NULL,0,1},
	{SRC_INTERP, "interpBtn",   False, False, False, NULL,NULL,0,1, NULL,NULL,0,1, NULL,NULL,0,1},
	{SRC_BACKUP, "backupBtn",   False, False, False, NULL,NULL,0,1, NULL,NULL,0,1, NULL,NULL,0,1}
};
int nload_sources = 6;

#else

extern SRCINFO load_sources[];
extern int     nload_sources;

#endif



extern Widget CreateLoadHourlyFields (Widget);
extern Widget CreateLoadDailyFields  (Widget);
extern Widget CreateLoadStaticFields (Widget);

extern void InitLoadHourlyFields (void);
extern void InitLoadDailyFields  (void);
extern void InitLoadStaticFields (void);

extern void ImportHourlyFields (Widget);
extern void ImportDailyFields  (Widget);
extern void ImportStaticFields (Widget);

extern void LoadHourlyFieldsExit (void);
extern void LoadDailyFieldsExit  (void);
extern void LoadStaticFieldsExit (void);

extern void SetLoadBtnLabel (int, String);

#endif
