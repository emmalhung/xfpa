/*********************************************************************/
/**	@file metafile.c
 *
 * Routines to handle the METAFILE object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 *********************************************************************/
/***********************************************************************
*                                                                      *
*      m e t a f i l e . c                                             *
*                                                                      *
*      Routines to handle the METAFILE object.                         *
*                                                                      *
*     Version 4 (c) Copyright 1996 Environment Canada (AES)            *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
*     Version 7 (c) Copyright 2006 Environment Canada                  *
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
*                                                                      *
***********************************************************************/

#define METAFILE_INIT
#include "metafile.h"

#include <tools/tools.h>
#include <fpa_getmem.h>
#include <fpa_math.h>
#include <string.h>

int			MetafileCount = 0;

/***********************************************************************
*                                                                      *
*      c r e a t e _ m e t a f i l e                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a new metafile with given attributes.
 *
 * @return Pointer to the new metafile object. You will need to
 * destroy this object when you are finished with it.
 *********************************************************************/

METAFILE    create_metafile(void)

	{
	METAFILE    mf;

	/* Allocate memory for structure */
	mf = INITMEM(struct METAFILE_struct,1);
	if (!mf) return NullMeta;

	/* Initialize the structure */
	mf->istamp   = NULL;
	mf->vstamp   = NULL;
	mf->tag      = NULL;
	copy_map_projection(&mf->mproj,&NoMapProj);
	mf->nsrc     = 0;
	mf->sproj    = NullMapProj;
	mf->scomp    = ((COMP_INFO *) 0);
	mf->bgndname = NULL;
	mf->bgnd     = NullMeta;
	mf->lgnd1    = NULL;
	mf->lgnd2    = NULL;
	mf->fields   = NullFldList;
	mf->numfld   = 0;
	mf->maxfld   = 0;

	/* Return the new metafile */
	MetafileCount++;
	return mf;
	}

/***********************************************************************
*                                                                      *
*      c o p y _ m e t a f i l e                                       *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Make a copy of a metafile.
 *
 *	@param[in] 	meta	metafile to copy
 *  @return Pointer to a copy of the given metafile. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/

METAFILE    copy_metafile

	(
	const METAFILE	meta
	)

	{
	METAFILE    mf;
	int			isrc, ifld;
	FIELD		fld;

	/* Do nothing if not there */
	if (!meta) return NullMeta;

	/* Create a metafile to hold the copy */
	mf = create_metafile();
	if (!mf) return NullMeta;

	/* Copy the description */
	(void) define_mf_tstamp(mf, meta->istamp, meta->vstamp);
	if ( !blank(meta->tag) )
			mf->tag = STRMEM(mf->tag, meta->tag);

	/* Copy the map projection */
	(void) copy_map_projection(&mf->mproj, &meta->mproj);

	/* Copy the source map projections */
	for (isrc=0; isrc<meta->nsrc; isrc++)
		{
		(void) add_mf_source_proj(mf, &meta->sproj[isrc]);
		(void) define_mf_source_comp(mf, isrc, &meta->scomp[isrc]);
		}

	/* Copy the background */
	(void) define_mf_bgnd(mf, meta->bgndname, copy_metafile(meta->bgnd));

	/* Copy the legends */
	(void) define_mf_lgnd(mf, meta->lgnd1, meta->lgnd2);

	/* Copy the fields */
	for (ifld=0; ifld<meta->numfld; ifld++)
		{
		fld = copy_field(meta->fields[ifld]);
		(void) add_field_to_metafile(mf, fld);
		}

	/* Return the copy of the metafile */
	return mf;
	}

/***********************************************************************
*                                                                      *
*      m e r g e _ m e t a f i l e s                                   *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Create a metafile using data from one or more input metafiles.
 * All metafiles are merged with the first metafile in the list,
 * which serves as the "background" for each field.
 *
 * Merging of PLOT type fields is currently not supported.
 *
 * Merging of SET type fields is currently not supported.
 *
 *	@param[in] 	nummeta	number of metafiles to merge
 *	@param[in] 	*metain	metafiles to merge
 *  @return Pointer to a new metafile object which is a merge of
 *  		all the metafiles in the given list. You will need to
 *  		destroy this object when you are finished with it.
 *********************************************************************/

METAFILE	merge_metafiles

	(
	int				nummeta,
	const METAFILE	*metain
	)

	{
	METAFILE	mf;
	SURFACE		sfc;
	int			imeta, ifld, isrc, numsrc;
	STRING		entity, element, level;

	/* Internal buffers for merging METAFILE Objects */
	int			*nscmp  = NULL;
	int			**scmps = NULL;
	SURFACE		*sfcin  = NULL;

	/* Return immediately if no "background" metafile for merging */
	if ( nummeta < 1 || !metain[0] ) return NullMeta;

	/* Make a copy of the "background" metafile for merging */
	mf = copy_metafile(metain[0]);
	if ( !mf ) return NullMeta;

	/* Return the metafile for merging if it is the only valid one */
	if ( nummeta == 1 ) return mf;
	for ( imeta=1; imeta<nummeta; imeta++ )
		{
		if ( metain[imeta] ) break;
		}
	if ( imeta >= nummeta ) return mf;

	/* Remove all fields from the metafile for merging */
	(void) empty_metafile(mf);

	/* Remove all source projections form the metafile for merging */
	(void) free_mf_source_proj(mf);

	/* Extract source projections from metafiles for merging */
	for ( imeta=0; imeta<nummeta; imeta++ )
		{

		/* Check source projections from each metafile */
		for ( isrc=0; isrc<metain[imeta]->nsrc; isrc++ )
			{

			/* Add the source projection if not already in list */
			numsrc = find_mf_source_proj(mf, &metain[imeta]->sproj[isrc]);
			if ( numsrc < 0 )
				{
				(void) add_mf_source_proj(mf, &metain[imeta]->sproj[isrc]);
				(void) define_mf_source_comp(mf, (mf->nsrc - 1),
						&metain[imeta]->scomp[isrc]);

				/* Initialize number of metafiles containing projection */
				nscmp = GETMEM(nscmp, int,   mf->nsrc);
				scmps = GETMEM(scmps, int *, mf->nsrc);
				nscmp[mf->nsrc - 1] = 1;
				scmps[mf->nsrc - 1] = INITMEM(int, 1);
				scmps[mf->nsrc - 1][0] = imeta;
				}

			/* Merge source components (if they exist) */
			else
				{
				if ( merge_mf_source_comp(mf, numsrc,
												&metain[imeta]->scomp[isrc]) )
					{
					++nscmp[numsrc];
					scmps[numsrc] = GETMEM(scmps[numsrc], int, nscmp[numsrc]);
					scmps[numsrc][nscmp[numsrc] - 1] = imeta;
					}
				}
			}
		}

	/* Allocate space for all field objects to merge */
	sfcin = INITMEM(SURFACE, nummeta);

	/* Extract all fields matching those in the "background" metafile */
	for ( ifld=0; ifld<metain[0]->numfld; ifld++ )
		{

		/* Identify field in "background" metafile */
		(void) recall_fld_info(metain[0]->fields[ifld],
				&entity, &element, &level);

		/* Extract matching PLOT type fields */
		if ( metain[0]->fields[ifld]->ftype == FtypePlot )
			{
			/* >>> No current ability to merge PLOT type fields <<< */
			}

		/* Extract matching SET type fields */
		else if ( metain[0]->fields[ifld]->ftype == FtypeSet )
			{
			/* >>> No current ability to merge SET type fields <<< */
			}

		/* Extract matching RASTER type fields */
		else if ( metain[0]->fields[ifld]->ftype == FtypeRaster )
			{
			/* >>> No current ability to merge RASTER type fields <<< */
			}

		/* Extract matching SURFACE type fields */
		else if ( metain[0]->fields[ifld]->ftype == FtypeSfc )
			{

			/* Check for SURFACE type field in each valid metafile */
			for ( imeta=0; imeta<nummeta; imeta++ )
				{
				if ( metain[imeta] )
					{

					/* Extract the SURFACE and remap it (if required) */
					sfcin[imeta] = find_mf_sfc(metain[imeta], entity,
															element, level);
					(void) remap_surface(sfcin[imeta], &metain[imeta]->mproj,
											&mf->mproj);
					}
				else
					sfcin[imeta] = NullSfc;
				}

			/* Merge the SURFACE type fields                 */
			/*  ... and add the merged field to the metafile */
			sfc = merge_surfaces(nummeta, sfcin, &mf->mproj,
									mf->nsrc, mf->sproj, nscmp, scmps);
			if ( !sfc ) continue;
			(void) add_sfc_to_metafile(mf, entity, element, level, sfc);
			}
		}

	/* Check that merged metafile contains some merged fields */
	if ( mf->numfld < 1 )
		{
		(void) destroy_metafile(mf);
		return NullMeta;
		}

	/* Return pointer to METAFILE Object */
	return mf;
	}

/***********************************************************************
*                                                                      *
*      d e s t r o y _ m e t a f i l e                                 *
*      e m p t y _ m e t a f i l e                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Destroy a metafile.
 *
 *	@param[in]     mf	metafile to be destroyed
 *  @return NullMeta
 *********************************************************************/

METAFILE    destroy_metafile

	(
	METAFILE    mf
	)

	{
	/* Do nothing if not there */
	if (!mf) return NullMeta;

	/* Return space for field list */
	empty_metafile(mf);

	/* Return space for background metafile */
	mf->bgnd = destroy_metafile(mf->bgnd);

	/* Return space for source projections */
	free_mf_source_proj(mf);

	/* Return space for other parameters */
	FREEMEM(mf->istamp);
	FREEMEM(mf->vstamp);
	FREEMEM(mf->tag);
	FREEMEM(mf->bgndname);
	FREEMEM(mf->lgnd1);
	FREEMEM(mf->lgnd2);

	/* Return structure itself */
	FREEMEM(mf);
	MetafileCount--;
	return NullMeta;
	}

/**********************************************************************/

/*********************************************************************/
/** Empty the metafile object.
 *
 *	@param[in]     mf	metafile to be emptied
 *********************************************************************/
void		empty_metafile

	(
	METAFILE    mf
	)

	{
	int	i;
	FIELD	*cp;

	/* Do nothing if not there */
	if (!mf) return;

	if (cp = mf->fields)
		{
		for (i=0; i<mf->numfld; i++)
			cp[i] = destroy_field(cp[i]);
		FREEMEM(mf->fields);
		}
	mf->numfld = 0;
	mf->maxfld = 0;
	}


/***********************************************************************
*                                                                      *
*      d e f i n e _ m f _ t s t a m p                                 *
*      d e f i n e _ m f _ m o d e l                                   *
*      d e f i n e _ m f _ p r o j e c t i o n                         *
*      d e f i n e _ m f _ b g n d                                     *
*      d e f i n e _ m f _ r a s t e r                                 *
*      d e f i n e _ m f _ l g n d                                     *
*                                                                      *
*      Set attributes of given metafile.                               *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Set metafile timestamp attributes
 *
 *	@param[in]   mf		given metafile
 *	@param[in] 	issue	issue timestamp
 *	@param[in] 	valid	valid timestamp
 *********************************************************************/
void		define_mf_tstamp

	(
	METAFILE    mf,
	STRING		issue,
	STRING		valid
	)

	{
	/* Do nothing if no metafile given */
	if (!mf) return;

	/* Allocate memory and copy string */
	if (!blank(issue)) mf->istamp = STRMEM(mf->istamp,issue);
	if (!blank(valid)) mf->vstamp = STRMEM(mf->vstamp,valid);
	return;
	}

/**********************************************************************/

/*********************************************************************/
/** Set metafile model attribute
 *
 *	@param[in] 	mf		given metafile
 *	@param[in] 	model	model name
 *********************************************************************/
void		define_mf_model

	(
	METAFILE    mf,
	STRING		model
	)

	{
	/* Do nothing if no metafile given */
	if (!mf) return;

	/* Allocate memory and copy string */
	if (!blank(model)) mf->tag = STRMEM(mf->tag,model);
	return;
	}

/**********************************************************************/

/*********************************************************************/
/** Set metafile projection attribute
 *
 *	@param[in]   mf		given metafile
 *	@param[in] 	*mproj	map projection
 *********************************************************************/
void		define_mf_projection

	(
	METAFILE		mf,
	const MAP_PROJ	*mproj
	)

	{
	/* Do nothing if no metafile given */
	if (!mf) return;

	/* Copy the map projection */
	copy_map_projection(&mf->mproj,mproj);
	return;
	}

/**********************************************************************/

/*********************************************************************/
/** Set metafile background attribute
 *
 *	@param[in] 	mf			given metafile
 *	@param[in] 	bgndname	background metafile name
 *	@param[in] 	bgnd		background metafile
 *********************************************************************/
void		define_mf_bgnd

	(
	METAFILE    mf,
	STRING		bgndname,
	METAFILE	bgnd
	)

	{
	/* Do nothing if no metafile given */
	if (!mf) return;

	/* Destroy old background */
	mf->bgnd = destroy_metafile(mf->bgnd);
	mf->bgnd = bgnd;

	/* Allocate memory and copy string */
	mf->bgndname = STRMEM(mf->bgndname,bgndname);
	return;
	}

/**********************************************************************/

/*********************************************************************/
/** Set metafile legend attribute
 *
 *	@param[in]    mf		given metafile
 *	@param[in] 	lgnd1	1st legend
 *	@param[in] 	lgnd2	2nd legend
 *********************************************************************/
void		define_mf_lgnd

	(
	METAFILE    mf,
	STRING		lgnd1,
	STRING		lgnd2
	)

	{
	/* Do nothing if no metafile given */
	if (!mf) return;

	/* Allocate memory and copy strings */
	mf->lgnd1 = STRMEM(mf->lgnd1,lgnd1);
	mf->lgnd2 = STRMEM(mf->lgnd2,lgnd2);
	return;
	}

/***********************************************************************
*                                                                      *
*      f r e e _ m f _ s o u r c e _ p r o j                           *
*      f i n d _ m f _ s o u r c e _ p r o j                           *
*      a d d _ m f _ s o u r c e _ p r o j                             *
*      c o v e r a g e _ m f _ s o u r c e _ p r o j                   *
***********************************************************************/

/*********************************************************************/
/** Free metafile source projection for metafile stitching.
 *
 *	@param[in] 	mf	metafile to prepare
 *********************************************************************/
void		free_mf_source_proj

	(
	METAFILE	mf
	)

	{
	/* Do nothing if no metafile given */
	if (!mf) return;

	if (mf->nsrc <= 0) return;
	FREEMEM(mf->sproj);
	FREEMEM(mf->scomp);
	mf->nsrc = 0;
	}

/**********************************************************************/

/*********************************************************************/
/** Find metafile source projection for metafile stitching.
 *
 *	@param[in] 	mf		metafile to prepare
 *	@param[in]  *mproj	Projection to match
 *  @return Index of the projection in the list if it is found in the
 * 			source projection list.
 *********************************************************************/
int			find_mf_source_proj

	(
	METAFILE		mf,
	const MAP_PROJ	*mproj
	)

	{
	int		isrc;

	/* Return -1 if no metafile given */
	if (!mf)    return -1;
	if (!mproj) return -1;

	/* Return location if it is in list */
	for (isrc=0; isrc<mf->nsrc; isrc++)
		{
		if (same_projection(&mproj->projection, &mf->sproj[isrc].projection)
				&& equivalent_map_def(&mproj->definition,
						&mf->sproj[isrc].definition))
			return isrc;
		}

	/* Return -1 if not found in list */
	return -1;
	}

/**********************************************************************/

/*********************************************************************/
/** Add a metafile source projection to the list.
 *
 *	@param[in] 	mf		metafile to prepare
 *	@param[in] 	*mproj	projection to add
 *********************************************************************/
void		add_mf_source_proj

	(
	METAFILE		mf,
	const MAP_PROJ	*mproj
	)

	{
	int		isrc;

	/* Do nothing if no metafile given */
	if (!mf)    return;
	if (!mproj) return;

	/* Return if it is already there */
	if (find_mf_source_proj(mf, mproj) >= 0) return;

	/* Not there - add it */
	isrc = mf->nsrc++;
	mf->sproj = GETMEM(mf->sproj, MAP_PROJ, mf->nsrc);
	mf->scomp = GETMEM(mf->scomp, COMP_INFO, mf->nsrc);

	copy_map_projection(mf->sproj+isrc, mproj);
	define_mf_source_comp(mf, isrc, (COMP_INFO *) 0);
	}

/**********************************************************************/

/*********************************************************************/
/** Return the percentage of the map projection covered by at least
 * one of the source projections.
 *
 *	@param[in] 	mf	metafile to prepare
 * 	@return Percentage of map projection covered by some source
 * 			projection.
 *********************************************************************/
float		coverage_mf_source_proj

	(
	METAFILE	mf
	)

	{
	int		Inumx, Inumy;
	float	**Alats, **Alons;
	int		iix, iiy, coverage;

	/* Return 0% if no metafile given */
	if (!mf) return 0.0;

	/* Return 100% if no source projections in metafile */
	if (mf->nsrc <= 0) return 100.0;

	/* Set FPA grid locations from map projection */
	if ( !grid_positions(&mf->mproj, &Inumx, &Inumy, NullFloat,
			NULL, &Alats, &Alons) ) return 0.0;

	/* Determine coverage at each FPA grid location */
	coverage = 0;
	for (iiy=0; iiy<Inumy; iiy++)
		for (iix=0; iix<Inumx; iix++)
			{
			if ( closest_map_projection(Alats[iiy][iix], Alons[iiy][iix],
					mf->nsrc, mf->sproj, NullChar, TRUE) >= 0 )
				coverage++;
			}

	/* Return percentage coverage */
	return (float) coverage / (float) (Inumx * Inumy) * 100.0;
	}

/***********************************************************************
*                                                                      *
*      d e f i n e _ m f _ s o u r c e _ c o m p                       *
*      m e r g e _ m f _ s o u r c e _ c o m p                         *
*                                                                      *
*                                                                      *
*      r e a d y _ m f _ s o u r c e _ c o m p                         *
*                                                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Define and alter the information about which components have been
 * used, for given source projections.
 *
 *	@param[in] 	mf		given metafile
 *	@param[in] 	isrc	source projection index
 *	@param[in]  *cinfo	Component information
 *********************************************************************/
void		define_mf_source_comp

	(
	METAFILE		mf,
	int				isrc,
	const COMP_INFO	*cinfo
	)

	{
	/* Do nothing if no metafile given or no source projections */
	if (!mf)              return;
	if (mf->nsrc <= 0)    return;
	if (isrc < 0)         return;
	if (isrc >= mf->nsrc) return;

	if (!cinfo)
		{
		mf->scomp[isrc].need = No_Comp;
		mf->scomp[isrc].have = No_Comp;
		}
	else
		{
		mf->scomp[isrc].need = cinfo->need;
		mf->scomp[isrc].have = No_Comp;
		(void) add_component(mf->scomp+isrc, cinfo->have);
		}
	}

/**********************************************************************/

/*********************************************************************/
/** Merge and alter the information about which components have been
 * used, for given source projections.
 *
 *	@param[in] 	mf		Given metafile
 *	@param[in] 	isrc	source index
 *	@param[in]  *cinfo	component info
 * 	@return Ture if successful.
 *********************************************************************/
LOGICAL		merge_mf_source_comp

	(
	METAFILE		mf,
	int				isrc,
	const COMP_INFO	*cinfo
	)

	{
	/* Return FALSE if no metafile given or no source projections */
	if (!mf)              return FALSE;
	if (mf->nsrc <= 0)    return FALSE;
	if (isrc < 0)         return FALSE;
	if (isrc >= mf->nsrc) return FALSE;

	/* Return FALSE if no component or component info does not match */
	if (!cinfo)                              return FALSE;
	if (cinfo->need != mf->scomp[isrc].need) return FALSE;

	/* Return FALSE if we do not need or already have the component */
	if (cinfo->need == No_Comp)              return FALSE;
	if (cinfo->have == mf->scomp[isrc].have) return FALSE;

	/* Return result of adding component */
	return add_component(mf->scomp+isrc, cinfo->have);
	}

/**********************************************************************/

/*********************************************************************/
/** Check whether all components have been used in all current source
 * projections.
 *
 *	@param[in] 	mf		metafile to check
 *  @return True if all components have been used in all
 * 			source prjections.
 *********************************************************************/
LOGICAL		ready_mf_source_comp

	(
	METAFILE	mf
	)

	{
	int		isrc;

	/* Do nothing if no metafile given */
	if (!mf) return FALSE;

	/* Check the component status of each source projection */
	for (isrc=0; isrc<mf->nsrc; isrc++)
		{
		if (!ready_components(mf->scomp+isrc)) return FALSE;
		}

	return TRUE;
	}

/***********************************************************************
*                                                                      *
*      a d d _ f i e l d _ t o _ m e t a f i l e                       *
*                                                                      *
*                                                                      *
*      a d d _ s f c _ t o _ m e t a f i l e                           *
*      a d d _ r a s t e r _ t o _ m e t a f i l e                     *
*      a d d _ s e t _ t o _ m e t a f i l e                           *
*      a d d _ p l o t _ t o _ m e t a f i l e                         *
*                                                                      *
*                                                                      *
*      a d d _ i t e m _ t o _ m e t a f i l e                         *
*      a d d _ i t e m _ t o _ m e t a f i l e _ s t a r t             *
*      a d d _ a r e a _ t o _ m e t a f i l e                         *
*      a d d _ a r e a _ t o _ m e t a f i l e _ s t a r t             *
*                                                                      *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Add the given field to the given metafile.
 *
 *	@param[in]   mf	metafile to add field to
 *	@param[in] 	fld	field to add to metafile
 *  @return Pointer to the Field if successful.
 *********************************************************************/
FIELD		add_field_to_metafile

	(
	METAFILE    mf,
	FIELD		fld
	)

	{
	/* Do nothing if metafile not there */
	if (!mf)  return NullFld;
	if (!fld) return NullFld;

	/* See if we need more space */
	if (mf->numfld >= mf->maxfld)
		{
		mf->maxfld += DELTA_FIELD;
		mf->fields  = GETMEM(mf->fields,FIELD,mf->maxfld);
		}

	/* Copy the given field to the field list */
	mf->fields[mf->numfld] = fld;
	mf->numfld++;
	return fld;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given source to the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *	@param[in] 	sfc		surface to add
 *  @return Pointer to the surface if successful.
 *********************************************************************/
SURFACE		add_sfc_to_metafile

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl,
	SURFACE		sfc
	)

	{
	SURFACE	osfc;
	FIELD	fld;

	/* Do nothing if metafile not there */
	if (!meta) return NullSfc;
	if (!sfc)  return NullSfc;

	/* Make sure there isn't already a corresponding surface */
	osfc = find_mf_sfc(meta,ent,elem,levl);
	if (osfc) return NullSfc;

	/* Add the surface */
	fld = make_mf_field(meta,"surface",NULL,ent,elem,levl);
	define_fld_data(fld,"surface",(POINTER) sfc);
	return sfc;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given source to the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *	@param[in] 	rast	RASTER to add
 *  @return Pointer to the RASTER if successful.
 *********************************************************************/
RASTER		add_raster_to_metafile

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl,
	RASTER		rast
	)

	{
	RASTER	orast;
	FIELD	fld;

	/* Do nothing if metafile not there */
	if (!meta)  return NullRaster;
	if (!rast) 	return NullRaster;

	/* Make sure there isn't already a corresponding RASTER */
	orast = find_mf_raster(meta,ent,elem,levl);
	if (orast) return NullRaster;

	/* Add the surface */
	fld = make_mf_field(meta,"raster",NULL,ent,elem,levl);
	define_fld_data(fld,"raster",(POINTER) rast);
	return rast;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given set to the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *	@param[in] 	set		set to add
 *  @return Pointer to the set object if successful
 *********************************************************************/
SET			add_set_to_metafile

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl,
	SET			set
	)

	{
	SET		oset;
	FIELD	fld;

	/* Do nothing if metafile not there */
	if (!meta) return NullSet;
	if (!set)  return NullSet;

	/* Make sure there isn't already a corresponding set */
	oset = find_mf_set(meta,set->type,ent,elem,levl);
	if (oset) return NullSet;

	/* Add the set */
	fld = make_mf_field(meta,"set",set->type,ent,elem,levl);
	define_fld_data(fld,"set",(POINTER) set);
	return set;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given plot to the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *	@param[in] 	plot	Plot to add
 *  @return Pointer to plot object if successful.
 *********************************************************************/
PLOT		add_plot_to_metafile

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl,
	PLOT		plot
	)

	{
	PLOT	oplot;
	FIELD	fld;

	/* Do nothing if metafile not there */
	if (!meta) return NullPlot;
	if (!plot) return NullPlot;

	/* Make sure there isn't already a corresponding plot */
	oplot = find_mf_plot(meta,ent,elem,levl);
	if (oplot) return NullPlot;

	/* Add the plot */
	fld = make_mf_field(meta,"plot",NULL,ent,elem,levl);
	define_fld_data(fld,"plot",(POINTER) plot);
	return plot;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given data to the appropriate field member of the given
 * metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	type	type of item
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *	@param[in] 	item	item to add
 *  @return  Pointer to the set object if successful.
 *********************************************************************/
SET			add_item_to_metafile

	(
	METAFILE    meta,
	STRING		type,
	STRING		ent,
	STRING		elem,
	STRING		levl,
	ITEM		item
	)

	{
	SET	set;

	/* Search for the field which contains the desired set as data */
	set = make_mf_set(meta,type,ent,elem,levl);

	/* Now add item to set */
	add_item_to_set(set,item);
	return set;
	}

/**********************************************************************/

/*********************************************************************/
/** Add the given data to the appropriate field member of the given
 * metafile. Add data at the top of the metafile object.
 *
 *	@param[in]   meta	Given metafile object
 *	@param[in] 	type	item type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *	@param[in] 	item	item to add at the start
 *********************************************************************/
SET			add_item_to_metafile_start

	(
	METAFILE    meta,
	STRING		type,
	STRING		ent,
	STRING		elem,
	STRING		levl,
	ITEM		item
	)

	{
	SET	set;

	/* Search for the field which contains the desired set as data */
	set = make_mf_set(meta,type,ent,elem,levl);

	/* Now add item to set */
	add_item_to_set_start(set,item);
	return set;
	}

/***********************************************************************
*                                                                      *
*      m a k e _ m f _ f i e l d                                       *
*      m a k e _ m f _ p l o t                                         *
*      m a k e _ m f _ s e t                                           *
*      m a k e _ m f _ s f c                                           *
*      m a k e _ m f _ r a s t e r                                     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Find the specified field in the given metafile.
 *
 * If not present - create an empty one, add it to the metafile
 * and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ftype	Field type
 *	@param[in] 	dtype	set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	element name
 *  @return Pointer to the Field object found. You do not need to
 * 			destroy this object as it will get destroyed when you
 * 			destroy the metafile object.
 *********************************************************************/
FIELD		make_mf_field

	(
	METAFILE    meta,
	STRING		ftype,
	STRING		dtype,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;

	/* Search for the field which contains the desired data */
	if (!meta) return NullFld;
	fld = find_mf_field(meta,ftype,dtype,ent,elem,levl);

	/* If not found create it */
	if (!fld)
		{
		if (!ent)  ent  = "";
		if (!elem) elem = "";
		if (!levl) levl = "";
		fld = create_field(ent,elem,levl);
		(void) add_field_to_metafile(meta,fld);
		}

	return fld;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified plot member in the given metafile.
 *
 * If not present - create an empty one, add it to the metafile
 * and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	element name
 * 	@return Pointer to the Plot object found. You do not need to
 * 			destroy this object as it will get destroyed when you
 * 			destroy the metafile object.
 *********************************************************************/
PLOT		make_mf_plot

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	PLOT	plot;

	plot = find_mf_plot(meta,ent,elem,levl);

	/* If not found, create a new field to accomodate the data */
	if (!plot)
		{
		fld  = create_field(ent,elem,levl);
		plot = create_plot();
		define_fld_data(fld,"plot",(POINTER) plot);
		(void) add_field_to_metafile(meta,fld);
		}

	return plot;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified set member in the given metafile.
 *
 * If not present - create an empty one, add it to the metafile
 * and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	type	Set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the set object found. You do not need to
 * 			destroy this object as it will get destroyed when you
 * 			destroy the metafile object.
 *********************************************************************/
SET			make_mf_set

	(
	METAFILE    meta,
	STRING		type,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	SET		set;

	set = find_mf_set(meta,type,ent,elem,levl);

	/* If not found, create a new field to accomodate the data */
	if (!set)
		{
		fld = create_field(ent,elem,levl);
		set = create_set(type);
		define_fld_data(fld,"set",(POINTER) set);
		(void) add_field_to_metafile(meta,fld);
		}

	return set;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified surface member in the given metafile.
 *
 * If not present - create an empty one, add it to the metafile
 * and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the surface object found. You do not need to
 * 			destroy this object as it will get destroyed when you
 * 			destroy the metafile object.
 *********************************************************************/
SURFACE		make_mf_sfc

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	SURFACE	sfc;

	sfc = find_mf_sfc(meta,ent,elem,levl);

	/* If not found, create a new field to accomodate the data */
	if (!sfc)
		{
		fld = create_field(ent,elem,levl);
		sfc = create_surface();
		define_fld_data(fld,"surface",(POINTER) sfc);
		(void) add_field_to_metafile(meta,fld);
		}

	return sfc;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified RASTER member in the given metafile.
 *
 * If not present - create an empty one, add it to the metafile
 * and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the RASTER object found. You do not need to
 * 			destroy this object as it will get destroyed when you
 * 			destroy the metafile object.
 *********************************************************************/
RASTER		make_mf_raster

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	RASTER	rast;

	rast = find_mf_raster(meta,ent,elem,levl);

	/* If not found, create a new field to accomodate the data */
	if (!rast)
		{
		fld = create_field(ent,elem,levl);
		rast = create_raster_obj();
		define_fld_data(fld,"raster",(POINTER) rast);
		(void) add_field_to_metafile(meta,fld);
		}

	return rast;
	}

/***********************************************************************
*                                                                      *
*     c o p y _ m f _ f i e l d                                        *
*     c o p y _ m f _ p l o t                                          *
*     c o p y _ m f _ s e t                                            *
*     c o p y _ m f _ s f c                                            *
*     c o p y _ m f _ r a s t e r                                      *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the specified field in the given metafile and return a
 * working copy.
 *
 *	@param[in] 	meta	given metafile
 *	@param[in] 	ftype	field type
 *	@param[in] 	dtype	set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to a copy of the Field found. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/

FIELD		copy_mf_field

	(
	METAFILE	meta,
	STRING		ftype,
	STRING		dtype,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;

	fld = find_mf_field(meta,ftype,dtype,ent,elem,levl);

	if (fld) return copy_field(fld);
	else     return create_field(ent,elem,levl);
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (PLOT) in the given metafile
 * and return  a working copy.
 *
 *	@param[in] 	meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 * 	@return Pointer to a copy of the plot found. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/
PLOT		copy_mf_plot

	(
	METAFILE	meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	PLOT	plot;

	plot = find_mf_plot(meta,ent,elem,levl);

	if (plot) return copy_plot(plot);
	else      return create_plot();
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (SET) in the given metafile
 * and return a working copy.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	type	field type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 * 	@return Pointer to a copy of the set found. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/
SET			copy_mf_set

	(
	METAFILE    meta,
	STRING		type,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	SET		set;

	set = find_mf_set(meta,type,ent,elem,levl);

	if (set) return copy_set(set);
	else     return create_set(type);
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (SURFACE) in the given metafile
 * and return a working copy.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 * 	@return Pointer to a copy of the surface found. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/
SURFACE		copy_mf_sfc

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	SURFACE	sfc;

	sfc = find_mf_sfc(meta,ent,elem,levl);

	if (sfc) return copy_surface(sfc,TRUE);
	else     return create_surface();
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (RASTER) in the given metafile
 * and return a working copy.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 * 	@return Pointer to a copy of the RASTER found. You will need to
 * 			destroy this object when you are finished with it.
 *********************************************************************/
RASTER		copy_mf_raster

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	RASTER	rast;

	rast = find_mf_raster(meta,ent,elem,levl);

	if (rast) return copy_raster(rast);
	else     return create_raster_obj();
	}

/***********************************************************************
*                                                                      *
*      f i n d _ m f _ f i e l d                                       *
*      f i n d _ m f _ p l o t                                         *
*      f i n d _ m f _ s e t                                           *
*      f i n d _ m f _ s f c                                           *
*      f i n d _ m f _ r a s t e r                                     *
*                                                                      *
***********************************************************************/

/*********************************************************************/
/** Find the specified field in the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ftype	field type
 *	@param[in] 	dtype	set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 * 	@return Pointer to the specified field.
 *********************************************************************/
FIELD		find_mf_field

	(
	METAFILE    meta,
	STRING		ftype,
	STRING		dtype,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	STRING	type;
	POINTER	data;
	int		ip;

	/* Search for the field which contains the desired data */
	if (!meta) return NullFld;
	for (ip=0; ip<meta->numfld; ip++)
		{
		fld = meta->fields[ip];
		if (ent  && !same(fld->entity ,ent )) continue;
		if (elem && !same(fld->element,elem)) continue;
		if (levl && !same(fld->level  ,levl)) continue;
		recall_fld_data(fld,&type,&data);
		if (!ftype)            return fld;
		if (!same(type,ftype)) continue;
		if (!dtype)            return fld;
		if ((fld->ftype == FtypeSet))
			{
			recall_set_type((SET) data,&type);
			if (same(type,dtype)) return fld;
			}
		if ((fld->ftype == FtypePlot))
			{
			return fld;
			}
		if ((fld->ftype == FtypeSfc))
			{
			return fld;
			}
		if ((fld->ftype == FtypeRaster))
			{
			return fld;
			}
		}

	return NullFld;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (PLOT) in the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the specified plot.
 *********************************************************************/
PLOT		find_mf_plot

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;

	fld = find_mf_field(meta,"plot",NULL,ent,elem,levl);

	if (fld) return fld->data.plot;
	else     return NullPlot;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (SET) in the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	type	set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the specified set.
 *********************************************************************/
SET			find_mf_set

	(
	METAFILE    meta,
	STRING		type,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;

	fld = find_mf_field(meta,"set",type,ent,elem,levl);

	if (fld) return fld->data.set;
	else     return NullSet;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (SURFACE) in the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the specified surface.
 *********************************************************************/
SURFACE		find_mf_sfc

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;

	fld = find_mf_field(meta,"surface",NULL,ent,elem,levl);

	if (fld) return fld->data.sfc;
	else     return NullSfc;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified field member (RASTER) in the given metafile.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the specified RASTER object.
 *********************************************************************/
RASTER		find_mf_raster

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;

	fld = find_mf_field(meta,"raster",NULL,ent,elem,levl);

	if (fld) return fld->data.raster;
	else     return NullRaster;
	}

/***********************************************************************
*                                                                      *
*      t a k e _ m f _ f i e l d                                       *
*      t a k e _ m f _ p l o t                                         *
*      t a k e _ m f _ s e t                                           *
*      t a k e _ m f _ s f c                                           *
*      t a k e _ m f _ r a s t e r                                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Find the specified set in the given metafile,
 * remove it and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ftype	field type
 *	@param[in] 	dtype	set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the removed field object. You are not responsible
 * 			for destroying this object when you are finished with it.
 *********************************************************************/

FIELD		take_mf_field

	(
	METAFILE    meta,
	STRING		ftype,
	STRING		dtype,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	STRING	type;
	POINTER	data;
	int		ip, jp;

	/* Search for the field which contains the desired data */
	if (!meta) return NullFld;
	for (ip=0; ip<meta->numfld; ip++)
		{
		fld = meta->fields[ip];
		if (ent  && !same(fld->entity ,ent )) continue;
		if (elem && !same(fld->element,elem)) continue;
		if (levl && !same(fld->level  ,levl)) continue;
		recall_fld_data(fld,&type,&data);
		if (!ftype)            break;
		if (!same(type,ftype)) continue;
		if (!dtype)            break;
		if ((fld->ftype == FtypeSet))
			{
			recall_set_type((SET) data,&type);
			if (same(type,dtype)) break;
			}
		if ((fld->ftype == FtypePlot))
			{
			break;
			}
		if ((fld->ftype == FtypeSfc))
			{
			break;
			}
		if ((fld->ftype == FtypeRaster))
			{
			break;
			}
		}

	/* Did we find it or not ? */
	if (ip >= meta->numfld) return NullFld;

	/* Now remove it from the metafile structure */
	for (jp=ip+1; jp<meta->numfld; jp++)
		{
		meta->fields[jp-1] = meta->fields[jp];
		}
	meta->numfld--;
	return fld;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified plot member in the given metafile,
 * remove it and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 * 	@return Pointer to the removed plot object. You are not responsible
 * 			for destroying this object when you are finished with it.
 *********************************************************************/
PLOT		take_mf_plot

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	PLOT	plot;

	fld = take_mf_field(meta,"plot",NULL,ent,elem,levl);

	if (fld)
		{
		plot = fld->data.plot;
		fld->data.plot = NullPlot;
		destroy_field(fld);
		return plot;
		}
	else     return NullPlot;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified set member in the given metafile,
 * remove it and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	type	set type
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the removed set object. You are not responsible
 * 			for destroying this object when you are finished with it.
 *********************************************************************/
SET			take_mf_set

	(
	METAFILE    meta,
	STRING		type,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	SET		set;

	fld = take_mf_field(meta,"set",type,ent,elem,levl);

	if (fld)
		{
		set = fld->data.set;
		fld->data.set = NullSet;
		destroy_field(fld);
		return set;
		}
	else     return NullSet;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified surface member in the given metafile,
 * remove it and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the removed surface object. You are not responsible
 * 			for destroying this object when you are finished with it.
 *********************************************************************/
SURFACE		take_mf_sfc

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	SURFACE	sfc;

	fld = take_mf_field(meta,"surface",NULL,ent,elem,levl);

	if (fld)
		{
		sfc = fld->data.sfc;
		fld->data.sfc = NullSfc;
		destroy_field(fld);
		return sfc;
		}
	else     return NullSfc;
	}

/**********************************************************************/

/*********************************************************************/
/** Find the specified  raster in the given metafile,
 * remove it and return it.
 *
 *	@param[in]   meta	given metafile
 *	@param[in] 	ent		entity
 *	@param[in] 	elem	element name
 *	@param[in] 	levl	level name
 *  @return Pointer to the removed surface object. You are not responsible
 * 			for destroying this object when you are finished with it.
 *********************************************************************/
RASTER		take_mf_raster

	(
	METAFILE    meta,
	STRING		ent,
	STRING		elem,
	STRING		levl
	)

	{
	FIELD	fld;
	RASTER	rast;

	fld = take_mf_field(meta,"raster",NULL,ent,elem,levl);

	if (fld)
		{
		rast = fld->data.raster;
		fld->data.raster = NullRaster;
		destroy_field(fld);
		return rast;
		}
	else     return NullRaster;
	}

/***********************************************************************
*                                                                      *
*      r e p r o j e c t _ x y _ m e t a f i l e s                     *
*                                                                      *
***********************************************************************/
/*********************************************************************/
/** Reproject two metafiles which are understood to contain the x
 * and y component fields of the same vector field.
 *
 * See reproject_xy_fields() in fields.c, as well as
 * reproject_xy_surfaces() in surface_oper.c for more info.
 *
 *	@param[in] 	umeta		metafile containing u (x) component
 *	@param[in] 	vmeta		metafile containing v (y) component
 *	@param[in] 	*smproj		source proj (not required)
 *	@param[in] 	*tmproj		target proj (required)
 *  @return True if successful.
 *********************************************************************/

LOGICAL	reproject_xy_metafiles

	(
	METAFILE		umeta,
	METAFILE		vmeta,
	const MAP_PROJ	*smproj,
	const MAP_PROJ	*tmproj
	)

	{
	int		ifld;
	FIELD	fld;
	FIELD	ufld = NullFld;
	FIELD	vfld = NullFld;

	if (IsNull(umeta)) return FALSE;
	if (IsNull(vmeta)) return FALSE;
	if (umeta->numfld <= 0) return FALSE;
	if (vmeta->numfld <= 0) return FALSE;
	if (IsNull(tmproj)) return FALSE;

	/* Make sure the source projections match */
	if (IsNull(smproj))
		{
		smproj = &(umeta->mproj);
		if (!same_map_projection(smproj, &(vmeta->mproj))) return FALSE;
		}
	else
		{
		if (!same_map_projection(smproj, &(umeta->mproj))) return FALSE;
		if (!same_map_projection(smproj, &(vmeta->mproj))) return FALSE;
		}

	/* Extract the u component field from the first metafile */
	for (ifld=0; ifld<umeta->numfld; ifld++)
		{
		fld = umeta->fields[ifld];
		if (IsNull(fld)) continue;
		if (fld->ftype != FtypeSfc) continue;

		if (NotNull(ufld)) return FALSE;
		ufld = fld;
		}
	if (IsNull(ufld)) return FALSE;

	/* Extract the v component field from the first metafile */
	for (ifld=0; ifld<vmeta->numfld; ifld++)
		{
		fld = vmeta->fields[ifld];
		if (IsNull(fld)) continue;
		if (fld->ftype != FtypeSfc) continue;

		if (NotNull(vfld)) return FALSE;
		vfld = fld;
		}
	if (IsNull(vfld)) return FALSE;

	return reproject_xy_fields(ufld, vfld, smproj, tmproj);
	}
