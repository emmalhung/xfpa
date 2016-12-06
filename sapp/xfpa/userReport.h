/*========================================================================*/
/*
*	File:		userReport.h
*
*   Purpose:    Header file for userReportDialog.c and userReportDbDialog.c.
*               Set the directory and file where the reference number
*               history will be kept.  The entire file name is made up
*               as $FPA/USER_REPORT_DIR/USER_REPORT_FILE.
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

#define NEW_LINE_CHAR			'\n'
#define USER_REPORT_DIR  		"data"
#define USER_REPORT_FILE 		".reportlog"
#define USER_REPORT_MAIL_DELETE	"FPA_DELETE_STATUS_MAIL"

extern void ACTIVATE_userReportDbDialog		(Widget, String);
extern void ACTIVATE_problemReportingDialog	(Widget);
extern void UpdateUserReportDatabase		(void);
extern void GetLicense						(int, String*, String);
extern void ShowHelloMessage				(void);
extern void RemoveHelloMessage				(void);
