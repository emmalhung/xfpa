/*========================================================================*/
/*
*	File:		pointFcstInit.c
*
*	Purpose:	Provides a mechanism for the selection and viewing of
*               the point forecasts to be generated.
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

#include <sys/stat.h>
#include "global.h"
#include "productStatus.h"
#include "pointFcst.h"


/* Global variables.
*/
int pf_nclass = 0;
PFCLASS *pf_class = NULL;
PFCLASS *pf_active_class = NULL;
String  pf_info_file = NULL;

static Boolean InitBlock(INFOFILE, PFCLASS *, int);


/*========================================================================*/
/*
*	InitPointFcstDialog() - Initialize for point forecast selection. 
*	Read the point forecast data from the information file and set into
*	the product status dialog.
*/
/*========================================================================*/
void InitPointFcstDialog(void)

{
	int    i, j, count;
	char   mbuf[256];
	INFOFILE fh;
	SETUP  *setup;

	/* Read the list of classes and initialize with any forecasts in
	*  the pfd file list which follows the class label and id.
	*/
	setup = GetSetup(PROD_POINT);
	pf_class = NewMem(PFCLASS, setup->nentry);
	pf_nclass = setup->nentry;
	pf_active_class = pf_class;
	for(i = 0; i <  setup->nentry; i++ )
	{
		pf_class[i].id       = SetupParm(setup,i,1);
		pf_class[i].label    = SetupParm(setup,i,0);
		pf_class[i].selected = 1;

		for(j = 2; j < setup->entry[i].nparms; j++)
		{
			fh = info_file_open(get_file(POINT_FCST,SetupParm(setup,i,j)));
			(void) InitBlock(fh, &pf_class[i], -1);
			info_file_close(fh);
		}
	}

	/* Read the point forecast data from the information file.
	*/
	(void) snprintf(mbuf, sizeof(mbuf), "%s_%s", INFO_FILE, base_name(GetSetupFile(0, NULL), NULL));
	pf_info_file = XtNewString(get_file(FCST_WORK, mbuf));
	if((fh = info_file_open(pf_info_file)))
	{
		for( i = 0; i < pf_nclass; i++ )
		{
			count = 0;
			while(InitBlock(fh, &pf_class[i], count)) count++;
		}
		info_file_close(fh);
	}

	/* Write the data back into the forecast info file in case there
	*  is a new entry in the point forecast config file list.
	*/
	pf_WriteInfoData();
}


/* Determine if the forecast given by the product identification number
*  has been released or not. It is considered released if the released
*  file has a later time than the last generated time.
*/
Boolean pf_IsReleased(int pid )

{
    int i, n;
    long dt;
	char mbuf[256];
    String prod;
    struct stat rinfo;

	for(n = 0; n < pf_nclass; n++)
	{
		for( i = 0; i < pf_class[n].ndata; i++ )
		{
			if(pf_class[n].data[i]->pid != pid) continue;
        	dt = ProductStatusGetGenerateTime(pid);
        	if(dt == 0) return False;
            pf_CreateFileName(pf_class[n].data[i], mbuf);
            prod = get_file(FCST_RELEASE, mbuf);
        	if(IsNull(prod) || stat(prod, &rinfo) == -1) return False;
        	if(dt > rinfo.st_mtime) return False;
        	return True;
		}
	}
    return False;
}


/* Read the data from the file and put into data block.  The return from this
*  function is False only if the block is not found.  No processing is done if
*  a point with the same label already exists for the given class.
*/
static Boolean InitBlock(INFOFILE fh , PFCLASS *class , int count )
{
	int i;
	char mbuf[256];
	String fcst_name, info;
	PFDATA *data;
	Boolean can_modify = (count >= 0);

	if(can_modify)
	{
		pf_MakeDataId(mbuf, class);
		if(!info_file_find_block(fh, mbuf)) return False;
	}

	fcst_name = info_file_get_data(fh, keylabel);
	for( i = 0; i < class->ndata; i++ )
	{
		if(same(fcst_name, class->data[i]->label))
			return True;
	}

	data = OneMem(PFDATA);
	pf_MakeDataId(mbuf, class);
	data->id = XtNewString(mbuf);
	data->generate = False;
	data->generating = False;
	data->modifable = can_modify;
	info = info_file_get_data(fh, keylabel);
	data->label = XtNewString(info);
	data->class = class->id;
	info = info_file_get_data(fh, keylanguage);
	pf_GetFmtData(data, info, keylanguage);
	info = info_file_get_data(fh, keytimezone);
	pf_GetFmtData(data, info, keytimezone);
	info = info_file_get_data(fh, keylat);
	pf_GetFmtData(data, info,  keyinfolat);
	info = info_file_get_data(fh, keylong);
	pf_GetFmtData(data, info, keyinfolong);
	info = info_file_get_data(fh, keyissue);
	pf_GetFmtData(data, info, keyissue);

	data->pid = ProductStatusAddInfo(PS_POINT_FCST,data->label,pf_IsReleased);

	class->data = MoreMem(class->data, PFDATA*, class->ndata+1);
	class->data[class->ndata] = data;
	class->ndata++;
	return True;
}
