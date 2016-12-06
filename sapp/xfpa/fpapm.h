/*========================================================================*/
/*
*	File:		fpapm.h
*
*   Purpose:    Header for fpapm script
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

/* The following two define blocks are used by the RunProgramManager()
*  function and by the fpapm script to run auxillary programs.  Note
*  that there must be agreement between the function and the script.
*/

#ifndef _FPAPM_H
#define _FPAPM_H

/* This is the name of the script to launch for program control.
*/
#define FPAPM "fpapm"

/* Set the information keys.
*/
#define PmNargs			"args"
#define PmNconfigFile	"config_file"
#define PmNdataPath		"data_path"
#define PmNdirectory  	"directory"
#define PmNdisplay    	"display"
#define PmNdpi        	"dpi"
#define PmNfcstClass  	"fcst_class"
#define PmNfcstId     	"fcst_id"
#define PmNfileName   	"file_name"
#define PmNinfoFile	  	"info_file"
#define PmNkey        	"key"
#define PmNmatrixCode 	"matrix_code"
#define PmNmodel      	"amodel"
#define PmNoutputFile 	"output_file"
#define PmNpageMode   	"mode"
#define PmNpdf        	"pdf"
#define PmNpostProcess 	"post_process"
#define PmNpreProcess   "pre_process"
#define PmNprocess      "process"
#define PmNprinter    	"printer"
#define PmNprogram    	"program"
#define PmNselect     	"select"
#define PmNsetupFile  	"setup"
#define PmNsource     	"source"
#define PmNsourceType 	"sourcetype"
#define PmNsubArea    	"subarea"
#define PmNt0         	"T0"
#define PmNtime       	"time"
#define PmNtimeDelta  	"time_delta"
#define PmNxOffset    	"xoff"
#define PmNyOffset    	"yoff"

/* Set the program types.
*/
#define PmALLIED_MODEL	    "allied_model"
#define PmAMEND	    		"amendment"
#define PmARCHIVE			"archive"
#define PmBULLETIN_EDIT		"bulletin_edit"
#define PmEDITOR            "editor"
#define PmFILE_PRINT		"file_print"
#define PmFOG				"FoG"
#define PmGRAPHIC           "graphic"
#define PmHELP_VIEWER		"help_viewer"
#define PmSCREEN_PRINT		"screen_print"
#define PmUPDATE	        "update"

#endif /* _FPAPM_H */
