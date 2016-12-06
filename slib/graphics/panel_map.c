/**************************************************************************
*                                                                         *
*     m a p _ p a n e l . c                                               *
*                                                                         *
*     Routines to manipulate a "map" dispnode.                            *
*                                                                         *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)               * 
*     Version 8 (c) Copyright 2011 Environment Canada                  *
*                                                                      *
*   This file is part of the Forecast Production Assistant (FPA).      *
*   The FPA is free software: you can redistribute it and/or modify it *
*   under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation, either version 3 of the License, or  *
*   any later version.                                                 *
*                                                                      *
*   The FPA is distributed in the hope that it will be useful, but     *
*   WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
*   See the GNU General Public License for more details.               *
*                                                                      *
*   You should have received a copy of the GNU General Public License  *
*   along with the FPA.  If not, see <http://www.gnu.org/licenses/>.   *
*                                                                         *
**************************************************************************/

#include "panel.h"
#include "gx.h"

#include <stdio.h>
#include <string.h>


LOGICAL	input_map

	(
	DISPNODE	mnode,	/* dispnode for 'map' window */
	STRING		name,	/* metafile name */
	MAP_PROJ	*bproj,	/* base map definition */
	LOGICAL		dsub	/* OK to delete subtree? */
	)

	{
	BOX			*mapview;
	MAP_PROJ	*mproj;
	METAFILE	meta;
#define FIX
#ifdef FIX
	int			ifld;
	FIELD		fld;
	BOX			box;
#endif

	/* Do nothing if no dispnode */
	if (!mnode) return FALSE;

	/* Get rid of previous contents */
	if (dsub) delete_dn_subtree(mnode);
	delete_dn_data(mnode);
	mapview = &mnode->viewport;
	define_dn_xform(mnode, "map", mapview, NullBox, bproj, NullXform);

	/* Try to read the metafile */
	if (blank(name)) return FALSE;
	meta = read_metafile(name, bproj);
	if (!meta) return FALSE;

#ifdef FIX
	box.left   = 0;
	box.right  = bproj->definition.xlen;
	box.bottom = 0;
	box.top    = bproj->definition.ylen;

	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = meta->fields[ifld];
		if (!fld) continue;

		/* If the field is a surface, refit it to the standard grid */
		/* Otherwise just throw away bits that are entirely outside the map */
		switch (fld->ftype)
			{
			case FtypeSfc:	reproject_surface(fld->data.sfc, bproj, bproj,
									&(bproj->grid));
							break;

			case FtypeSet:	strip_set(fld->data.set, &box);
#ifdef FIX_CLIP
							set = fld->data.set;
							if (set->num <= 0) break;
							if (!same(set->type, "curve")) break;

							/* Clip lines to given box */
							newset = create_set("curve");
							for (imem=0; imem<set->num; imem++)
								{
								reset_pipe();
								enable_save();
								curve = (CURVE) set->list[imem];
								...  line_pipe(curve->line);
								flush_pipe();
								nl = recall_save(&lines);
								...
								}
							reset_pipe();
							define_field_data(fld, "set", (POINTER)newset);
#endif
							break;

			case FtypePlot:	strip_plot(fld->data.plot, &box);
							break;
			}
		}
#endif

	/* Now load the metafile into the dispnode */
	mproj = &meta->mproj;
	define_dn_xform(mnode, "map", mapview, NullBox, mproj, NullXform);
	define_dn_data(mnode, "metafile", (POINTER) meta);
	return TRUE;
	}
