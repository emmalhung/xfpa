/*========================================================================*/
/*
*	File:		alliedModelInit.c
*
*   Purpose:    Initialize the data structures.
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
#include "global.h"
#include "productStatus.h"
#define ALLIED_MODEL_MAIN
#include "alliedModel.h"

void InitAlliedModels(void)
{
	int i, n, na, namod, nmeta;
	String state_data;
	Boolean ok;
	SourceList src;


	SourceListByType(SRC_ALLIED|SRC_HIDDEN, FpaC_TIMEDEP_ANY, &src, &namod);
	GV_allied_model = NewMem(AlliedModelStruct, namod);

	for(i = 0; i < namod; i++)
	{
		/* We need to ensure that we can get the detailed source information as
		*  the allied model info we need is down in that area of the config info.
		*/
		if(IsNull(get_source_info(SrcName(src[i]), SrcSubName(src[i]))))
			continue;
		na = GV_nallied_model++;

		GV_allied_model[na].source = src[i];
		GV_allied_model[na].product_key =
			ProductStatusAddInfo(PS_MODEL, SrcLabel(GV_allied_model[na].source), NULL);
		nmeta = 0;
		if ( GV_allied_model[na].source->fd->sdef->allied &&
				GV_allied_model[na].source->fd->sdef->allied->metafiles)
			nmeta = GV_allied_model[na].source->fd->sdef->allied->metafiles->nfiles;
		GV_allied_model[na].import = NewBooleanArray(nmeta);

		if(XuStateDataGet(ALLIED_MODEL_STATE_KEY, SrcName(GV_allied_model[na].source),
			SrcSubName(GV_allied_model[na].source), &state_data))
		{
			n = 0;
			while(n < nmeta)
			{
				GV_allied_model[na].import[n] = (Boolean)int_arg(state_data, &ok);
				if(GV_allied_model[na].import[n])
					GV_allied_model[na].automatic_import = True;
				n++;
			}
			XtFree(state_data);
		}
	}
}
