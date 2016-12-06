/***********************************************************************/
/**	@file	surface_mem.c
 *
 * Routines to handle the SURFACE object.
 *
 * Version 8 &copy; Copyright 2011 Environment Canada
 *
 ***********************************************************************/
/***********************************************************************
*                                                                      *
*      s u r f a c e _ m e m . c                                       *
*                                                                      *
*      Routines to handle the SURFACE object.                          *
*                                                                      *
*     Version 5 (c) Copyright 1998 Environment Canada (AES)            *
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

#include "surface.h"
#include <tools/tools.h>
#include <stdlib.h>

/***********************************************************************
*                                                                      *
*      g e t _ s f c _ p a t c h                                       *
*      p r e p a r e _ s f c _ p a t c h                               *
*      d i s p o s e _ s f c _ p a t c h                               *
*      d e s t r o y _ s f c _ p a t c h                               *
*      r e d e f _ s f c _ p a t c h                                   *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Retrieve a pointer to a specified patch.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return pointer to requested patch or NullPatch if it would not be found.
 ***********************************************************************/
PATCH	get_sfc_patch

	(
	SURFACE	sfc,
	int		iu,
	int		iv
	)

	{
	PATCH	patch;

	if (IsNull(sfc))          return NullPatch;
	if (IsNull(sfc->patches)) return NullPatch;
	if (iu < 0)               return NullPatch;
	if (iu >= sfc->nupatch)   return NullPatch;
	if (iv < 0)               return NullPatch;
	if (iv >= sfc->nvpatch)   return NullPatch;

	/* Obtain the patch */
	patch = sfc->patches[iu][iv];
	return patch;
	}

/**********************************************************************/

/***********************************************************************/
/**	Prepare the given patch for use in contouring or evaluation,
 *
 *      including:
 *
 *        - allocate and initialize if necessary
 *        - generate the patch function if necessary
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return pointer to requested patch or NullPatch if it would not be found.
 ***********************************************************************/
PATCH	prepare_sfc_patch

	(
	SURFACE	sfc,
	int		iu,
	int		iv
	)

	{
	PATCH	patch;

	if (IsNull(sfc))          return NullPatch;
	if (IsNull(sfc->patches)) return NullPatch;
	if (iu < 0)               return NullPatch;
	if (iu >= sfc->nupatch)   return NullPatch;
	if (iv < 0)               return NullPatch;
	if (iv >= sfc->nvpatch)   return NullPatch;

	/* Obtain the patch */
	patch = sfc->patches[iu][iv];
	if (IsNull(patch))
		{
		/* Allocate if necessary */
		patch = create_patch();
		sfc->patches[iu][iv] = patch;
		}

	/* Define the patch function if necessary */
	define_patch(patch, &sfc->sp, iu, iv);

	return patch;
	}

/**********************************************************************/

/***********************************************************************/
/**	Prepare the given patch for disposal.
 *
 *	Deallocate the patch after use only if it's not contoured.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return NullPatch if patch could be destroyed, pointer to patch otherwise.
 ***********************************************************************/
PATCH	dispose_sfc_patch

	(
	SURFACE sfc,
	int     iu,
	int     iv
	)

	{
	PATCH   patch;

	if (IsNull(sfc))          return NullPatch;
	if (IsNull(sfc->patches)) return NullPatch;
	if (iu < 0)               return NullPatch;
	if (iu >= sfc->nupatch)   return NullPatch;
	if (iv < 0)               return NullPatch;
	if (iv >= sfc->nvpatch)   return NullPatch;

	/* Obtain the patch */
	patch = sfc->patches[iu][iv];
	if (IsNull(patch))      return NullPatch;
	if (get_MMM() == MMM_AllocateAndFree)
		{
		/* Deallocate if required unless contoured */
		if (NotNull(patch->contours)) return patch;
		if (NotNull(patch->extrema))  return patch;
		if (NotNull(patch->vectors))  return patch;
		patch = destroy_patch(patch);
		sfc->patches[iu][iv] = NullPatch;
		return NullPatch;
		}

	return patch;
	}

/**********************************************************************/

/***********************************************************************/
/**	Destroy surface patch.
 *
 * Recover any memory allocated to patch.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return NullPatch
 ***********************************************************************/
PATCH	destroy_sfc_patch

	(
	SURFACE sfc,
	int     iu,
	int     iv
	)

	{
	PATCH   patch;

	if (IsNull(sfc))          return NullPatch;
	if (IsNull(sfc->patches)) return NullPatch;
	if (iu < 0)               return NullPatch;
	if (iu >= sfc->nupatch)   return NullPatch;
	if (iv < 0)               return NullPatch;
	if (iv >= sfc->nvpatch)   return NullPatch;

	/* Obtain the patch */
	patch = sfc->patches[iu][iv];
	if (IsNull(patch)) return NullPatch;

	/* Destroy the patch */
	patch = destroy_patch(patch);
	sfc->patches[iu][iv] = NullPatch;
	return NullPatch;
	}

/**********************************************************************/

/***********************************************************************/
/**	Redefine a surface patch function if necessary.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @param[in]	force	force the recalculation.
 * @return pointer to redefined patch or NullPatch if unable to define function.
 ***********************************************************************/
PATCH	redef_sfc_patch

	(
	SURFACE sfc,
	int     iu,
	int     iv,
	LOGICAL	force
	)

	{
	PATCH   patch;

	if (IsNull(sfc))          return NullPatch;
	if (IsNull(sfc->patches)) return NullPatch;
	if (iu < 0)               return NullPatch;
	if (iu >= sfc->nupatch)   return NullPatch;
	if (iv < 0)               return NullPatch;
	if (iv >= sfc->nvpatch)   return NullPatch;

	/* Obtain the patch */
	patch = sfc->patches[iu][iv];

	if (!force)
		{
		switch (get_MMM())
			{
			case MMM_AllocateWhenNeeded:
					if (NotNull(patch)) empty_patch(patch);
					return patch;

			case MMM_AllocateAndFree:
					if (NotNull(patch))
						{
						patch = destroy_patch(patch);
						sfc->patches[iu][iv] = NullPatch;
						}
					return patch;
			}
		}

	/* Redefine the patch function if necessary */
	if (NotNull(patch)) patch->defined = FALSE;

	return prepare_sfc_patch(sfc, iu, iv);
	}

/***********************************************************************
*                                                                      *
*      p r e p a r e _ s f c _ u l i s t                               *
*      d i s p o s e _ s f c _ u l i s t                               *
*      d e s t r o y _ s f c _ u l i s t                               *
*      p r e p a r e _ s f c _ v l i s t                               *
*      d i s p o s e _ s f c _ v l i s t                               *
*      d e s t r o y _ s f c _ v l i s t                               *
*                                                                      *
*      Prepare the given ilist for use in contouring or evaluation,    *
*      including:                                                      *
*                                                                      *
*        - allocate and initialize if necessary                        *
*        - deallocate the after use if required                        *
*                                                                      *
***********************************************************************/

/***********************************************************************/
/**	Prepare the given u component ilist for use in contouring or evaluation.
 *
 * Allocate and initialize if necessary.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return pointer prepared ILIST or NullIlist if it could not be prepared.
 ***********************************************************************/
ILIST	prepare_sfc_ulist

	(
	SURFACE	sfc,
	int		iu,
	int		iv
	)

	{
	ILIST	list;

	if (IsNull(sfc))          return NullIlist;
	if (IsNull(sfc->ulist))   return NullIlist;
	if (iu < 0)               return NullIlist;
	if (iu >= sfc->nupatch)   return NullIlist;
	if (iv < 0)               return NullIlist;
	if (iv >= sfc->nvpatch+1) return NullIlist;

	/* Obtain the ilist */
	list = sfc->ulist[iu][iv];
	if (IsNull(list))
		{
		/* Allocate if necessary */
		list = create_ilist();
		sfc->ulist[iu][iv] = list;
		}

	return list;
	}

/**********************************************************************/

/***********************************************************************/
/**	Destroy a surface  u component ilist.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return NullIlist
 ***********************************************************************/
ILIST	destroy_sfc_ulist

	(
	SURFACE	sfc,
	int		iu,
	int		iv
	)

	{
	ILIST	list;

	if (IsNull(sfc))          return NullIlist;
	if (IsNull(sfc->ulist))   return NullIlist;
	if (iu < 0)               return NullIlist;
	if (iu >= sfc->nupatch)   return NullIlist;
	if (iv < 0)               return NullIlist;
	if (iv >= sfc->nvpatch+1) return NullIlist;

	/* Obtain the ilist */
	list = sfc->ulist[iu][iv];
	if (IsNull(list)) return NullIlist;

	/* Destroy the ilist */
	list = destroy_ilist(list);
	sfc->ulist[iu][iv] = NullIlist;
	return NullIlist;
	}

/**********************************************************************/

/***********************************************************************/
/**	Prepare the given v component ilist for use in contouring or evaluation.
 *
 * Allocate and initialize if necessary.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return pointer prepared ILIST or NullIlist if it could not be prepared.
 ***********************************************************************/
ILIST	prepare_sfc_vlist

	(
	SURFACE	sfc,
	int		iu,
	int		iv
	)

	{
	ILIST	list;

	if (IsNull(sfc))          return NullIlist;
	if (IsNull(sfc->vlist))   return NullIlist;
	if (iu < 0)               return NullIlist;
	if (iu >= sfc->nupatch+1) return NullIlist;
	if (iv < 0)               return NullIlist;
	if (iv >= sfc->nvpatch)   return NullIlist;

	/* Obtain the ilist */
	list = sfc->vlist[iu][iv];
	if (IsNull(list))
		{
		/* Allocate if necessary */
		list = create_ilist();
		sfc->vlist[iu][iv] = list;
		}

	return list;
	}

/**********************************************************************/

/***********************************************************************/
/**	Destroy a surface  v component ilist.
 *
 * @param[in]	sfc	surface to search.
 * @param[in]	iu	u component index.
 * @param[in]	iv	v component index.
 * @return NullIlist
 ***********************************************************************/
ILIST	destroy_sfc_vlist

	(
	SURFACE	sfc,
	int		iu,
	int		iv
	)

	{
	ILIST	list;

	if (IsNull(sfc))          return NullIlist;
	if (IsNull(sfc->vlist))   return NullIlist;
	if (iu < 0)               return NullIlist;
	if (iu >= sfc->nupatch+1) return NullIlist;
	if (iv < 0)               return NullIlist;
	if (iv >= sfc->nvpatch)   return NullIlist;

	/* Obtain the ilist */
	list = sfc->vlist[iu][iv];
	if (IsNull(list)) return NullIlist;

	/* Destroy the ilist */
	list = destroy_ilist(list);
	sfc->vlist[iu][iv] = NullIlist;
	return NullIlist;
	}
